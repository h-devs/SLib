/*
 *   Copyright (c) 2008-2021 SLIBIO <https://github.com/SLIBIO>
 *
 *   Permission is hereby granted, free of charge, to any person obtaining a copy
 *   of this software and associated documentation files (the "Software"), to deal
 *   in the Software without restriction, including without limitation the rights
 *   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *   copies of the Software, and to permit persons to whom the Software is
 *   furnished to do so, subject to the following conditions:
 *
 *   The above copyright notice and this permission notice shall be included in
 *   all copies or substantial portions of the Software.
 *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 *   THE SOFTWARE.
 */

#include "slib/network/smb.h"
#include "slib/network/smb_packet.h"
#include "slib/network/ntlm.h"

#include "slib/network/event.h"
#include "slib/network/netbios.h"
#include "slib/crypto/asn1.h"
#include "slib/core/memory_output.h"
#include "slib/core/process.h"
#include "slib/core/service_manager.h"

#define SERVER_TAG "SMB SERVER"

#define MAX_TREE_ID 0x100000

#define IPC_PATH SLIB_UNICODE("IPC$")
#define IPC_WKSSVC SLIB_UNICODE("wkssvc")
#define IPC_SRVSVC SLIB_UNICODE("srvsvc")

namespace slib
{

	namespace priv
	{
		namespace smb
		{

			static sl_bool WriteNetBIOSHeader(const SmbServer::Connection& connection, sl_size sizeMessage)
			{
				sl_uint8 buf[4];
				MIO::writeUint32BE(buf, (sl_uint32)sizeMessage);
				buf[0] = 0;
				return connection.socket->sendFully(buf, 4, connection.event) == 4;
			}

			static void InitSmb2ResponseHeader(Smb2Header& header)
			{
				Base::zeroMemory(&header, sizeof(header));
				header.setSmb2();
				header.setHeaderLength(sizeof(header));
				header.setCreditGranted(1);
				header.setFlags(Smb2HeaderFlags::Response);
			}

			static void InitSmb2ResponseHeader(Smb2Header& response, const Smb2Header& request)
			{
				Base::zeroMemory(&response, sizeof(response));
				response.setSmb2();
				response.setCommand(request.getCommand());
				response.setHeaderLength(sizeof(response));
				response.setCreditCharge(1);
				response.setCreditGranted(1);
				response.setFlags(Smb2HeaderFlags::Response | Smb2HeaderFlags::Priority);
				response.setProcessId(request.getProcessId());
				response.setMessageId(request.getMessageId());
				response.setSessionId(request.getSessionId());
				response.setTreeId(request.getTreeId());
			}

			static sl_bool WriteResponse(const SmbServer::Connection& connection, const Smb2Header& smb, const void* response, sl_size sizeResponse, const void* blob = sl_null, sl_size sizeBlob = 0)
			{
				if (!(WriteNetBIOSHeader(connection, sizeof(smb) + sizeResponse + sizeBlob))) {
					return sl_false;
				}
				if (connection.socket->sendFully(&smb, sizeof(smb), connection.event) != sizeof(smb)) {
					return sl_false;
				}
				if (connection.socket->sendFully(response, sizeResponse, connection.event) != sizeResponse) {
					return sl_false;
				}
				if (sizeBlob) {
					if (connection.socket->sendFully(blob, sizeBlob, connection.event) != sizeBlob) {
						return sl_false;
					}
				}
				return sl_true;
			}

			template <class RESPONSE>
			static sl_bool WriteResponse(const SmbServer::Connection& connection, const Smb2Header& smb, const RESPONSE& response, const void* blob = sl_null, sl_size sizeBlob = 0)
			{
				return WriteResponse(connection, smb, &response, sizeof(response), blob, sizeBlob);
			}

			static sl_bool WriteSmb2NegotiateContext(const SmbServer::Connection& connection, Smb2NegotiateContextType type, const void* data, sl_size len)
			{
				Smb2NegotiateContextHeader header;
				Base::zeroMemory(&header, sizeof(header));
				header.setType(type);
				header.setDataLength((sl_uint16)len);
				if (connection.socket->sendFully(&header, sizeof(header), connection.event) == sizeof(header)) {
					return connection.socket->sendFully(data, len, connection.event) == len;
				}
				return sl_false;
			}

			// RFC 2743 - Generic Security Service Application Program Interface Version 2, Update 1
			static Memory Gssapi_BuildNegTokenInit()
			{
				MemoryBuffer body;
				Asn1Tag<SLIB_ASN1_TAG_CONTEXT(0), Asn1Tag<SLIB_ASN1_TAG_SEQUENCE, Asn1Body> >::serialize(&body, SLIB_ASN1_ENCODED_OID_NTLMSSP); // mechTypes
				Asn1Tag<SLIB_ASN1_TAG_CONTEXT(3), Asn1Tag<SLIB_ASN1_TAG_SEQUENCE, Asn1Tag<SLIB_ASN1_TAG_CONTEXT(0), Asn1Tag<SLIB_ASN1_TAG_TYPE_GENERAL_STRING, Asn1Body> > > >::serialize(&body, "not_defined_in_RFC4178@please_ignore"); // negHints
				MemoryBuffer body2;
				SerializeStatic(&body2, SLIB_ASN1_ENCODED_OID_SPNEGO);
				Asn1Tag<SLIB_ASN1_TAG_CONTEXT(0), Asn1Tag<SLIB_ASN1_TAG_SEQUENCE, Asn1Body> >::serialize(&body2, body);
				MemoryBuffer buf;
				Asn1Tag<SLIB_ASN1_TAG_APP(0), Asn1Body>::serialize(&buf, body2);
				return buf.merge();
			}

			static Memory Gssapi_BuildNegTokenTargCompleted()
			{
				MemoryBuffer buf;
				Asn1Tag<SLIB_ASN1_TAG_CONTEXT(1), Asn1Tag<SLIB_ASN1_TAG_SEQUENCE, Asn1Tag<SLIB_ASN1_TAG_CONTEXT(0), Asn1Tag<SLIB_ASN1_TAG_ENUMERATED, Asn1Body> > > >::serialize(&buf, "\x00");
				return buf.merge();
			}

			static Memory Gssapi_BuildNegTokenTargIncompleted(const Memory& token)
			{
				MemoryBuffer body;
				Asn1Tag<SLIB_ASN1_TAG_CONTEXT(0), Asn1Tag<SLIB_ASN1_TAG_ENUMERATED, Asn1Body> >::serialize(&body, "\x01"); // acccept-incomplete
				Asn1Tag<SLIB_ASN1_TAG_CONTEXT(1), Asn1Body >::serialize(&body, SLIB_ASN1_ENCODED_OID_NTLMSSP); // supportedMech
				Asn1Tag<SLIB_ASN1_TAG_CONTEXT(2), Asn1Tag<SLIB_ASN1_TAG_OCTET_STRING, Asn1Body> >::serialize(&body, token); // responseToken
				MemoryBuffer buf;
				Asn1Tag<SLIB_ASN1_TAG_CONTEXT(1), Asn1Tag<SLIB_ASN1_TAG_SEQUENCE, Asn1Body> >::serialize(&buf, body);
				return buf.merge();
			}

		}
	}

	using namespace priv::smb;


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(SmbServerParam)

	SmbServerParam::SmbServerParam()
	{
		port = 445;

		SLIB_STATIC_STRING(_targetName, "Server")
			targetName = _targetName;

		maxThreadsCount = 16;
		flagStopWindowsService = sl_true;

		flagAutoStart = sl_true;
	}


	SLIB_DEFINE_OBJECT(SmbServer, Object)

	SmbServer::SmbServer()
	{
		m_flagReleased = sl_false;
		m_flagRunning = sl_false;

		Math::randomMemory(m_serverGuid, sizeof(m_serverGuid));
		Math::randomMemory(m_serverChallenge, sizeof(m_serverChallenge));
		Math::randomMemory(m_hashSalt, sizeof(m_hashSalt));
		m_lastSessionId = Time::now().toInt();

		m_lastTreeId = 0;
		m_lastFileId = 0;
	}

	SmbServer::~SmbServer()
	{
		release();
	}

	Ref<SmbServer> SmbServer::create(const SmbServerParam& param)
	{
#ifdef SLIB_PLATFORM_IS_WIN32
		sl_bool flagStopSystemService = sl_false;
		if (param.port == 445) {
			if (Process::isCurrentProcessAdmin()) {
				if (ServiceManager::getState("LanmanServer") == ServiceState::Running) {
					flagStopSystemService = sl_true;
					ServiceManager::stop("LanmanServer");
				}
				if (ServiceManager::getState("srv2") == ServiceState::Running) {
					ServiceManager::stop("srv2");
				}
				if (ServiceManager::getState("srvnet") == ServiceState::Running) {
					ServiceManager::stop("srvnet");
				}
			}
		}
#endif
		Socket socket = Socket::openTcp(SocketAddress(param.bindAddress, param.port));
#ifdef SLIB_PLATFORM_IS_WIN32
		if (flagStopSystemService) {
			ServiceManager::start("LanmanServer");
		}
#endif
		if (socket.isOpened()) {
			Ref<SmbServer> server = new SmbServer;
			if (server.isNotNull()) {
				server->m_socketListen = Move(socket);
				server->m_param = param;
				if (param.flagAutoStart) {
					server->start();
				}
				return server;
			}
		}
		return sl_null;
	}

	sl_bool SmbServer::start()
	{
		ObjectLocker lock(this);

		if (m_flagReleased) {
			return sl_false;
		}
		if (m_flagRunning) {
			return sl_true;
		}

		Ref<ThreadPool> threadPool = ThreadPool::create(0, m_param.maxThreadsCount);
		if (threadPool.isNull()) {
			return sl_false;
		}
		Ref<Thread> threadListen = Thread::start(SLIB_FUNCTION_MEMBER(SmbServer, _onRunListen, this));
		if (threadListen.isNull()) {
			return sl_false;
		}

		m_threadListen = Move(threadListen);
		m_threadPool = Move(threadPool);
		m_flagRunning = sl_true;

		return sl_true;
	}

	void SmbServer::release()
	{
		ObjectLocker lock(this);

		if (m_flagReleased) {
			return;
		}
		m_flagReleased = sl_true;
		m_flagRunning = sl_false;

		Ref<Thread> threadListen = m_threadListen;
		if (threadListen.isNotNull()) {
			threadListen->finishAndWait();
			threadListen.setNull();
		}

		Ref<ThreadPool> threadPool = m_threadPool;
		if (threadPool.isNotNull()) {
			threadPool->release();
			m_threadPool.setNull();
		}

		m_socketListen.close();

	}

	sl_bool SmbServer::isReleased()
	{
		return m_flagReleased;
	}

	sl_bool SmbServer::isRunning()
	{
		return m_flagRunning;
	}

	const SmbServerParam& SmbServer::getParam()
	{
		return m_param;
	}

	void SmbServer::_onRunListen()
	{
		Thread* thread = Thread::getCurrent();
		if (!thread) {
			return;
		}
		Ref<ThreadPool> threadPool = m_threadPool;
		if (threadPool.isNull()) {
			return;
		}
		Socket& socket = m_socketListen;
		socket.setNonBlockingMode();
		socket.listen();
		Ref<SocketEvent> ev = SocketEvent::createRead(socket);
		if (ev.isNull()) {
			return;
		}
		while (thread->isNotStopping()) {
			SocketAddress address;
			MoveT<Socket> client = socket.accept(address);
			if (client.isNotNone()) {
				threadPool->addTask([client, this]() {
					_onRunClient(client);
				});
			} else {
				ev->wait();
			}
		}
	}

	void SmbServer::_onRunClient(const Socket& socket)
	{
		Thread* thread = Thread::getCurrent();
		if (!thread) {
			return;
		}

		socket.setNonBlockingMode();
		Ref<SocketEvent> ev = SocketEvent::createReadWrite(socket);
		if (ev.isNull()) {
			return;
		}

		NetBIOS_SessionMessage msg;
		while (thread->isNotStopping()) {
			sl_int32 n = msg.read(socket);
			if (n == SLIB_IO_ENDED) {
				IOParam param;
				param.socket = &socket;
				param.event = ev.get();
				param.data = msg.message;
				param.size = msg.sizeMessage;
				if (!(_onProcessMessage(param))) {
					break;
				}
				msg.reset();
			} else if (n < 0) {
				if (n == SLIB_IO_WOULD_BLOCK) {
					ev->wait();
				} else {
					break;
				}
			}
		}
	}

	sl_bool SmbServer::_onProcessMessage(SmbServer::IOParam& param)
	{
		if (param.size < 4) {
			return sl_false;
		}
		sl_uint8* data = param.data;
		if (data[1] == 'S' && data[2] == 'M' && data[3] == 'B') {
			if (data[0] == 0xff) {
				if (param.size >= sizeof(SmbHeader)) {
					SmbParam smb;
					*((IOParam*)&smb) = param;
					smb.smb = (SmbHeader*)data;
					return _onProcessSMB(smb);
				}
			} else if (data[0] == 0xfe) {
				if (param.size >= sizeof(Smb2Header)) {
					Smb2Param smb;
					*((IOParam*)&smb) = param;
					smb.smb = (Smb2Header*)data;
					return _onProcessSMB2(smb);
				}
			}
		}
		return sl_false;
	}

	sl_bool SmbServer::_onProcessSMB(SmbServer::SmbParam& param)
	{
		if (param.smb->getCommand() == SmbCommand::Negotiate) {
			Smb2Param smb2;
			*((IOParam*)&smb2) = param;
			smb2.smb = sl_null;
			return _onProcessNegotiate(smb2);
		}
		return sl_false;
	}

	sl_bool SmbServer::_onProcessSMB2(SmbServer::Smb2Param& param)
	{
		Smb2Command command = param.smb->getCommand();
		switch (command) {
		case Smb2Command::Negotiate:
			return _onProcessNegotiate(param);
		case Smb2Command::SessionSetup:
			return _onProcessSessionSetup(param);
		case Smb2Command::TreeConnect:
			return _onProcessTreeConnect(param);
		case Smb2Command::Create:
			return _onProcessCreate(param);
		case Smb2Command::Close:
			return _onProcessClose(param);
		case Smb2Command::Read:
			return _onProcessRead(param);
		case Smb2Command::Write:
			return _onProcessWrite(param);
		case Smb2Command::Ioctl:
			return _onProcessIoctl(param);
		case Smb2Command::GetInfo:
			return _onProcessGetInfo(param);
		}
		return sl_false;
	}

	sl_bool SmbServer::_onProcessNegotiate(SmbServer::Smb2Param& param)
	{
		Memory memSecurityBlob = Gssapi_BuildNegTokenInit();
		sl_uint16 nSizeSecurityBlob = (sl_uint16)(memSecurityBlob.getSize());

		Smb2Header smb;
		InitSmb2ResponseHeader(smb);

		Smb2NegotiateResponseMessage response;
		Base::zeroMemory(&response, sizeof(response));
		response.setSize(sizeof(response), sl_true);
		Base::copyMemory(response.getGuid(), m_serverGuid, 16);
		response.setCapabilities(Smb2Capabilities::LargeMtu);
		response.setMaxTransationSize(0x800000); // 8MB
		response.setMaxReadSize(0x800000);
		response.setMaxWriteSize(0x800000);
		response.setCurrentTime(Time::now());
		response.setBlobOffset(sizeof(smb) + sizeof(response));
		response.setBlobLength(nSizeSecurityBlob);

		sl_uint32 nSizeBeforeContext = (sl_uint32)(sizeof(smb) + sizeof(response) + nSizeSecurityBlob);

		MemoryOutput memPreauthContext;
		sl_uint32 nPaddingBeforeContext = 0;

		if (param.smb) {

			sl_uint32 nSizeBeforeContextPadded = ((nSizeBeforeContext - 1) | 15) + 1;
			nPaddingBeforeContext = nSizeBeforeContextPadded - nSizeBeforeContext;
			nSizeBeforeContext = nSizeBeforeContextPadded;

			response.setDialect(0x0210);
			response.setContextCount(1);
			response.setContextOffset(nSizeBeforeContext);

			smb.setProcessId(param.smb->getProcessId());
			smb.setMessageId(param.smb->getMessageId());

			memPreauthContext.writeUint16(1); // Hash Algorithm Count
			memPreauthContext.writeUint16(32); // Salt Length
			memPreauthContext.writeUint16(1); // Hash Algorithm: SHA-512
			memPreauthContext.write(m_hashSalt, sizeof(m_hashSalt));

			if (!(WriteNetBIOSHeader(param, nSizeBeforeContext + sizeof(Smb2NegotiateContextHeader) + memPreauthContext.getSize()))) {
				return sl_false;
			}

		} else {

			response.setDialect(0x02ff);

			if (!(WriteNetBIOSHeader(param, nSizeBeforeContext))) {
				return sl_false;
			}
		}

		if (param.socket->sendFully(&smb, sizeof(smb), param.event) != sizeof(smb)) {
			return sl_false;
		}
		if (param.socket->sendFully(&response, sizeof(response), param.event) != sizeof(response)) {
			return sl_false;
		}
		if (nSizeSecurityBlob) {
			if (param.socket->sendFully(memSecurityBlob.getData(), nSizeSecurityBlob, param.event) != nSizeSecurityBlob) {
				return sl_false;
			}
		}
		if (memPreauthContext.getSize()) {
			if (nPaddingBeforeContext) {
				sl_uint8 zeros[16] = { 0 };
				if (param.socket->sendFully(zeros, nPaddingBeforeContext, param.event) != nPaddingBeforeContext) {
					return sl_false;
				}
			}
			Memory mem = memPreauthContext.getData();
			if (!(WriteSmb2NegotiateContext(param, Smb2NegotiateContextType::PREAUTH_INTEGRITY_CAPABILITIES, mem.getData(), mem.getSize()))) {
				return sl_false;
			}
		}
		return sl_true;
	}

	sl_bool SmbServer::_onProcessSessionSetup(SmbServer::Smb2Param& param)
	{
		Smb2Header smb;
		InitSmb2ResponseHeader(smb, *(param.smb));

		Smb2SessionSetupResponseMessage response;
		Base::zeroMemory(&response, sizeof(response));
		response.setSize(sizeof(response), sl_true);

		Memory securityBlob;

		sl_uint64 sessionId = param.smb->getSessionId();
		if (sessionId) {
			response.setSessionFlags(Smb2SessionFlags::Guest);
			securityBlob = Gssapi_BuildNegTokenTargCompleted();
		} else {
			sessionId = Base::interlockedIncrement64(&m_lastSessionId);
			smb.setStatus(SmbStatus::MoreProcessingRequired);

			NtlmChallengeHeader ntlm;
			Memory memNtlm;
			{
				Base::zeroMemory(&ntlm, sizeof(ntlm));
				Base::copyMemory(ntlm.getId(), "NTLMSSP", 8);
				ntlm.setMessageType(NtlmMessageType::Challenge);
				ntlm.setNegotiateFlags(NtlmNegotiateFlags::NegotiateUnicode | NtlmNegotiateFlags::RequestTarget | NtlmNegotiateFlags::NegotiateSign | NtlmNegotiateFlags::NegotiateNTLM | NtlmNegotiateFlags::NegotiateAlwaysSign | NtlmNegotiateFlags::TargetTypeServer | NtlmNegotiateFlags::NegotiateExtenedSecurity | NtlmNegotiateFlags::NegotiateTargetInfo | NtlmNegotiateFlags::NegotiateVersion | NtlmNegotiateFlags::Negotiate128 | NtlmNegotiateFlags::NegotiateKeyExchange | NtlmNegotiateFlags::Negotiate56);
				Base::copyMemory(ntlm.getServerChallenge(), m_serverChallenge, sizeof(m_serverChallenge));
				ntlm.setMajorVersion(6);
				ntlm.setMinorVersion(1);
				ntlm.setNtlmCurrentRevision(15);

				String16 targetName = String16::from(m_param.targetName);
				sl_uint16 lenTargetName = (sl_uint16)(targetName.getLength());
				sl_uint16 sizeTargetName = lenTargetName << 1;
				NtlmBlobDesc* blobTargetName = ntlm.getTargetName();
				blobTargetName->setLengthAndMaxLength(sizeTargetName);
				blobTargetName->setOffset(sizeof(ntlm));

				NtlmTargetInfo targetInfo;
				if (m_param.domainName_NetBIOS.isNotNull()) {
					targetInfo.addItem(NtlmTargetInfoItemType::NetBIOS_DomainName, m_param.domainName_NetBIOS);
				} else {
					targetInfo.addItem(NtlmTargetInfoItemType::NetBIOS_DomainName, m_param.targetName);
				}
				if (m_param.computerName_NetBIOS.isNotNull()) {
					targetInfo.addItem(NtlmTargetInfoItemType::NetBIOS_ComputerName, m_param.computerName_NetBIOS);
				} else {
					targetInfo.addItem(NtlmTargetInfoItemType::NetBIOS_ComputerName, m_param.targetName);
				}
				if (m_param.domainName_DNS.isNotNull()) {
					targetInfo.addItem(NtlmTargetInfoItemType::DNS_DomainName, m_param.domainName_DNS);
				} else {
					targetInfo.addItem(NtlmTargetInfoItemType::DNS_DomainName, m_param.targetName);
				}
				if (m_param.computerName_DNS.isNotNull()) {
					targetInfo.addItem(NtlmTargetInfoItemType::DNS_ComputerName, m_param.computerName_DNS);
				} else {
					targetInfo.addItem(NtlmTargetInfoItemType::DNS_ComputerName, m_param.targetName);
				}
				targetInfo.addTimestamp();
				Memory memTargetInfo = targetInfo.end();
				sl_uint16 sizeTargetInfo = (sl_uint16)(memTargetInfo.getSize());
				NtlmBlobDesc* blobTargetInfo = ntlm.getTargetInfo();
				blobTargetInfo->setLengthAndMaxLength(sizeTargetInfo);
				blobTargetInfo->setOffset(sizeof(ntlm) + sizeTargetName);

				memNtlm = Memory::create(sizeof(ntlm) + sizeTargetName + sizeTargetInfo);
				if (memNtlm.isNull()) {
					return sl_false;
				}
				sl_uint8* buf = (sl_uint8*)(memNtlm.getData());
				Base::copyMemory(buf, &ntlm, sizeof(ntlm));
				buf += sizeof(ntlm);
				sl_char16* dataTargetName = targetName.getData();
				for (sl_uint16 i = 0; i < lenTargetName; i++) {
					MIO::writeUint16LE(buf, dataTargetName[i]);
					buf += 2;
				}
				Base::copyMemory(buf, memTargetInfo.getData(), sizeTargetInfo);
			}
			securityBlob = Gssapi_BuildNegTokenTargIncompleted(memNtlm);
		}

		response.setBlobOffset(sizeof(smb) + sizeof(response));
		response.setBlobLength((sl_uint16)(securityBlob.getSize()));
		smb.setSessionId(sessionId);

		return WriteResponse(param, smb, response, securityBlob.getData(), securityBlob.getSize());
	}

	sl_bool SmbServer::_onProcessTreeConnect(SmbServer::Smb2Param& param)
	{
		if (param.size < sizeof(Smb2Header) + sizeof(Smb2TreeConnectRequestMessage)) {
			return sl_false;
		}

		Smb2TreeConnectRequestMessage* request = (Smb2TreeConnectRequestMessage*)(param.data + sizeof(Smb2Header));
		if (!(request->checkSize(sizeof(Smb2TreeConnectRequestMessage), sl_true))) {
			return sl_false;
		}
		sl_uint16 treeOffset = request->getTreeOffset();
		sl_uint16 treeLength = request->getTreeLength();
		if ((sl_uint32)(treeOffset + treeLength) > param.size) {
			return sl_false;
		}
		sl_char16* szPath = (sl_char16*)(param.data + treeOffset);
		sl_uint16 lenPath = treeLength >> 1;
		if (lenPath > 2 && szPath[0] == '\\' && szPath[1] == '\\') {
			// truncate host
			szPath += 2;
			lenPath -= 2;
			for (sl_uint16 i = 0; i < lenPath; i++) {
				if (szPath[i] == '\\') {
					szPath += i + 1;
					lenPath -= i + 1;
					break;
				}
			}
		}

		Smb2TreeConnectResponseMessage response;
		Base::zeroMemory(&response, sizeof(response));
		response.setSize(sizeof(response), sl_false);

		sl_uint32 treeId = 0;

		StringView16 path(szPath, lenPath);
		if (path == IPC_PATH) {
			treeId = _registerTree(path);
			response.setShareType(Smb2ShareType::NamedPipe);
			response.setAccessMask(SmbAccessMask::Read | SmbAccessMask::Synchronize);
		}

		Smb2Header smb;
		InitSmb2ResponseHeader(smb, *(param.smb));
		if (treeId) {
			smb.setTreeId(treeId);
		} else {
			smb.setStatus(SmbStatus::Unsuccessful);
		}
		return WriteResponse(param, smb, response);
	}

	sl_bool SmbServer::_onProcessCreate(SmbServer::Smb2Param& param)
	{
		if (param.size < sizeof(Smb2Header) + sizeof(Smb2CreateRequestMessage)) {
			return sl_false;
		}

		Smb2CreateRequestMessage* request = (Smb2CreateRequestMessage*)(param.data + sizeof(Smb2Header));
		if (!(request->checkSize(sizeof(Smb2CreateRequestMessage), sl_true))) {
			return sl_false;
		}

		sl_uint16 fileNameOffset = request->getFileNameOffset();
		sl_uint16 fileNameLength = request->getFileNameLength();
		if ((sl_uint32)(fileNameOffset + fileNameLength) > param.size) {
			return sl_false;
		}
		
		Smb2CreateResponseMessage response;
		Base::zeroMemory(&response, sizeof(response));
		response.setSize(sizeof(response), sl_false);

		Shared<FileContext> file;
		String16 tree = _getTree(param.smb->getTreeId());
		if (tree.isNotNull()) {
			StringView16 fileName((sl_char16*)(param.data + fileNameOffset), fileNameLength >> 1);
			if (tree == IPC_PATH) {
				if (fileName == IPC_WKSSVC || fileName == IPC_SRVSVC) {
					file = _createFile(tree, fileName);
					if (file.isNotNull()) {
						response.setAction(SmbCreateAction::Existed);
						response.setAttributes(FileAttributes::Normal);
						MIO::writeUint64LE(response.getGuid(), file->fileId);
					}
				}
			}
		}
		
		Smb2Header smb;
		InitSmb2ResponseHeader(smb, *(param.smb));
		if (file.isNull()) {
			smb.setStatus(SmbStatus::Unsuccessful);
		}
		return WriteResponse(param, smb, response);
	}

	sl_bool SmbServer::_onProcessClose(SmbServer::Smb2Param& param)
	{
		if (param.size < sizeof(Smb2Header) + sizeof(Smb2CloseRequestMessage)) {
			return sl_false;
		}
		
		Smb2Header smb;
		InitSmb2ResponseHeader(smb, *(param.smb));

		Smb2CloseResponseMessage response;
		Base::zeroMemory(&response, sizeof(response));
		response.setSize(sizeof(response), sl_false);

		return WriteResponse(param, smb, response);
	}

	sl_bool SmbServer::_onProcessRead(SmbServer::Smb2Param& param)
	{
		if (param.size < sizeof(Smb2Header) + sizeof(Smb2ReadRequestMessage)) {
			return sl_false;
		}

		Smb2ReadRequestMessage* request = (Smb2ReadRequestMessage*)(param.data + sizeof(Smb2Header));
		Memory data;

		Shared<FileContext> file = _getFile(request->getGuid());
		if (file.isNotNull()) {
			if (file->tree == IPC_PATH) {
				// DCE/RPC
				if (file->fileName == IPC_WKSSVC) {
					data = Memory::createStatic(
						"\x05\x00\x0c\x03\x10\x00\x00\x00\x44\x00\x00\x00\x02\x00\x00\x00" \
						"\xb8\x10\xb8\x10\xf0\x53\x00\x00\x0d\x00\x5c\x50\x49\x50\x45\x5c" \
						"\x77\x6b\x73\x73\x76\x63\x00\x00\x01\x00\x00\x00\x00\x00\x00\x00" \
						"\x04\x5d\x88\x8a\xeb\x1c\xc9\x11\x9f\xe8\x08\x00\x2b\x10\x48\x60" \
						"\x02\x00\x00\x00");
				} else if (file->fileName == IPC_SRVSVC) {
					data = Memory::createStatic(
						"\x05\x00\x0c\x03\x10\x00\x00\x00\x44\x00\x00\x00\x02\x00\x00\x00" \
						"\xb8\x10\xb8\x10\xf0\x53\x00\x00\x0d\x00\x5c\x50\x49\x50\x45\x5c" \
						"\x73\x72\x76\x73\x76\x63\x00\x00\x01\x00\x00\x00\x00\x00\x00\x00" \
						"\x04\x5d\x88\x8a\xeb\x1c\xc9\x11\x9f\xe8\x08\x00\x2b\x10\x48\x60" \
						"\x02\x00\x00\x00");
				}
			}
		}
		
		Smb2Header smb;
		InitSmb2ResponseHeader(smb, *(param.smb));
		if (data.isNull()) {
			smb.setStatus(SmbStatus::Unsuccessful);
		}

		Smb2ReadResponseMessage response;
		Base::zeroMemory(&response, sizeof(response));
		response.setSize(sizeof(response), sl_true);
		response.setDataOffset(sizeof(smb) + sizeof(response));
		response.setReadCount((sl_uint32)(data.getSize()));
		
		return WriteResponse(param, smb, response, data.getData(), data.getSize());
	}

	sl_bool SmbServer::_onProcessWrite(SmbServer::Smb2Param& param)
	{
		if (param.size < sizeof(Smb2Header) + sizeof(Smb2WriteRequestMessage)) {
			return sl_false;
		}
		Smb2WriteRequestMessage* request = (Smb2WriteRequestMessage*)(param.data + sizeof(Smb2Header));
		if (!(request->checkSize(sizeof(Smb2WriteRequestMessage), sl_true))) {
			return sl_false;
		}
		sl_uint16 dataOffset = request->getDataOffset();
		sl_uint32 dataLength = request->getWriteLength();
		if ((sl_uint32)(dataOffset + dataLength) > param.size) {
			return sl_false;
		}

		sl_uint32 sizeWritten = 0;
		Shared<FileContext> file = _getFile(request->getGuid());
		if (file.isNotNull()) {
			if (file->tree == IPC_PATH && (file->fileName == IPC_WKSSVC || file->fileName == IPC_SRVSVC)) {
				sizeWritten = dataLength;
			}
		}

		Smb2Header smb;
		InitSmb2ResponseHeader(smb, *(param.smb));
		if (!sizeWritten) {
			smb.setStatus(SmbStatus::Unsuccessful);
		}

		Smb2WriteResponseMessage response;
		Base::zeroMemory(&response, sizeof(response));
		response.setSize(sizeof(response), sl_true);
		response.setWriteCount(sizeWritten);

		return WriteResponse(param, smb, response);
	}

	sl_bool SmbServer::_onProcessIoctl(SmbServer::Smb2Param& param)
	{
		if (param.size < sizeof(Smb2Header) + sizeof(Smb2IoctlRequestMessage)) {
			return sl_false;
		}

		Smb2IoctlRequestMessage* request = (Smb2IoctlRequestMessage*)(param.data + sizeof(Smb2Header));
		Memory data;

		Shared<FileContext> file = _getFile(request->getGuid());
		if (file.isNotNull()) {
			if (file->tree == IPC_PATH) {
				// DCE/RPC
				if (file->fileName == IPC_WKSSVC) {
					data = Memory::createStatic(
						"\x05\x00\x02\x03\x10\x00\x00\x00\x6c\x00\x00\x00\x02\x00\x00\x00" \
						"\x54\x00\x00\x00\x00\x00\x00\x00\x64\x00\x00\x00\x04\x00\x02\x00" \
						"\xf4\x01\x00\x00\x08\x00\x02\x00\x0c\x00\x02\x00\x06\x00\x00\x00" \
						"\x01\x00\x00\x00\x07\x00\x00\x00\x00\x00\x00\x00\x07\x00\x00\x00" \
						"\x53\x00\x45\x00\x52\x00\x56\x00\x45\x00\x52\x00\x00\x00\x00\x00" \
						"\x06\x00\x00\x00\x00\x00\x00\x00\x06\x00\x00\x00\x53\x00\x48\x00" \
						"\x41\x00\x52\x00\x45\x00\x00\x00\x00\x00\x00\x00");
				} else if (file->fileName == IPC_SRVSVC) {
					data = Memory::createStatic(
						"\x05\x00\x02\x03\x10\x00\x00\x00\x7c\x00\x00\x00\x02\x00\x00\x00" \
						"\x64\x00\x00\x00\x00\x00\x00\x00" \
						"\x65\x00\x00\x00\x04\x00\x02\x00\xf4\x01\x00\x00\x08\x00\x02\x00" \
						"\x06\x00\x00\x00\x01\x00\x00\x00\x03\x9a\x80\x00\x0c\x00\x02\x00" \
						"\x07\x00\x00\x00\x00\x00\x00\x00\x07\x00\x00\x00\x53\x00\x45\x00" \
						"\x52\x00\x56\x00\x45\x00\x52\x00\x00\x00\x00\x00\x0c\x00\x00\x00" \
						"\x00\x00\x00\x00\x0c\x00\x00\x00\x4c\x00\x69\x00\x67\x00\x68\x00" \
						"\x74\x00\x53\x00\x65\x00\x72\x00\x76\x00\x65\x00\x72\x00\x00\x00" \
						"\x00\x00\x00\x00"
					);
				}
			}
		}

		Smb2Header smb;
		InitSmb2ResponseHeader(smb, *(param.smb));
		if (data.isNull()) {
			smb.setStatus(SmbStatus::Unsuccessful);
		}

		Smb2IoctlResponseMessage response;
		Base::zeroMemory(&response, sizeof(response));
		response.setSize(sizeof(response), sl_true);
		response.setFunction(request->getFunction());
		Base::copyMemory(response.getGuid(), request->getGuid(), 16);
		response.setDataOffset(sizeof(smb) + sizeof(response));
		response.setDataLength((sl_uint32)(data.getSize()));

		return WriteResponse(param, smb, response, data.getData(), data.getSize());
	}

	sl_bool SmbServer::_onProcessGetInfo(SmbServer::Smb2Param& param)
	{
		if (param.size < sizeof(Smb2Header) + sizeof(Smb2GetInfoRequestMessage)) {
			return sl_false;
		}

		Smb2GetInfoRequestMessage* request = (Smb2GetInfoRequestMessage*)(param.data + sizeof(Smb2Header));
		Memory data;

		Shared<FileContext> file = _getFile(request->getGuid());
		if (file.isNotNull()) {
			if (file->tree == IPC_PATH && (file->fileName == IPC_WKSSVC || file->fileName == IPC_SRVSVC)) {
				Smb2FileStandardInfo info;
				Base::zeroMemory(&info, sizeof(info));
				info.setAllocationSize(4096);
				info.setLinkCount(1);
				info.setDeletePending(sl_true);
				data = Memory::create(&info, sizeof(info));
			}
		}

		Smb2Header smb;
		InitSmb2ResponseHeader(smb, *(param.smb));
		if (data.isNull()) {
			smb.setStatus(SmbStatus::Unsuccessful);
		}

		Smb2GetInfoResponseMessage response;
		Base::zeroMemory(&response, sizeof(response));
		response.setSize(sizeof(response), sl_true);
		response.setBlobOffset(sizeof(smb) + sizeof(response));
		response.setBlobLength((sl_uint32)(data.getSize()));

		return WriteResponse(param, smb, response, data.getData(), data.getSize());
	}

	sl_uint32 SmbServer::_registerTree(const String16& path) noexcept
	{
		MutexLocker locker(&m_mutexTrees);
		sl_uint32 id = m_treeIds.getValue_NoLock(path);
		if (id) {
			return id;
		}
		m_lastTreeId++;
		if (m_lastTreeId > MAX_TREE_ID) {
			m_lastTreeId = MAX_TREE_ID >> 4;
		}
		m_trees.put_NoLock(m_lastTreeId, path);
		m_treeIds.put_NoLock(path, m_lastTreeId);
		return m_lastTreeId;
	}

	String16 SmbServer::_getTree(sl_uint32 treeId) noexcept
	{
		MutexLocker locker(&m_mutexTrees);
		return m_trees.getValue_NoLock(treeId);
	}

	SmbServer::FileContext::FileContext()
	{
	}

	SmbServer::FileContext::~FileContext()
	{
	}

	Shared<SmbServer::FileContext> SmbServer::_createFile(const String16& tree, const String16& fileName) noexcept
	{
		MutexLocker locker(&m_mutexFiles);
		String16 path = String16::join(tree, SLIB_UNICODE("\\"), fileName);
		Shared<FileContext> context = m_filesByPath.getValue_NoLock(path);
		if (context.isNotNull()) {
			return context;
		}
		context = Shared<FileContext>::create();
		if (context.isNotNull()) {
			context->tree = tree;
			context->fileName = fileName;
			m_lastFileId++;
			context->fileId = m_lastFileId;
			m_files.put_NoLock(m_lastFileId, context);
			m_filesByPath.put_NoLock(path, context);
			return context;
		}
		return sl_null;
	}

	Shared<SmbServer::FileContext> SmbServer::_getFile(sl_uint8* guid) noexcept
	{
		MutexLocker locker(&m_mutexFiles);
		return m_files.getValue_NoLock(MIO::readUint64LE(guid));
	}


	void NtlmTargetInfo::addItem(NtlmTargetInfoItemType type, const void* data, sl_size size) noexcept
	{
		sl_uint8 buf[4];
		MIO::writeUint16LE(buf, (sl_uint16)type);
		MIO::writeUint16LE(buf + 2, (sl_uint16)size);
		m_buf.addNew(buf, 4);
		if (size) {
			m_buf.addNew(data, (sl_uint16)size);
		}
	}

	void NtlmTargetInfo::addItem(NtlmTargetInfoItemType type, const StringParam& _str) noexcept
	{
		StringData16 str(_str);
		sl_uint16 len = (sl_uint16)(str.getLength());
		if (!len) {
			return;
		}
		if (len >> 15) {
			return;
		}
		sl_uint16 size = len << 1;
		Memory mem = Memory::create(4 + size);
		if (mem.isNull()) {
			return;
		}
		sl_uint8* buf = (sl_uint8*)(mem.getData());
		MIO::writeUint16LE(buf, (sl_uint16)type);
		MIO::writeUint16LE(buf + 2, (sl_uint16)size);
		buf += 4;
		sl_char16* data = str.getData();
		for (sl_uint16 i = 0; i < len; i++) {
			MIO::writeUint16LE(buf, data[i]);
			buf += 2;
		}
		m_buf.add(Move(mem));
	}

	void NtlmTargetInfo::addTimestamp() noexcept
	{
		sl_uint8 buf[8];
		MIO::writeUint64LE(buf, Time::now().toWindowsFileTime());
		addItem(NtlmTargetInfoItemType::Timestamp, buf, 8);
	}

	Memory NtlmTargetInfo::end() noexcept
	{
		addItem(NtlmTargetInfoItemType::EndOfList, sl_null, 0);
		return m_buf.merge();
	}

}
