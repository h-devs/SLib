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
#include "slib/network/dce_rpc.h"

#include "slib/network/event.h"
#include "slib/network/netbios.h"
#include "slib/crypto/asn1.h"
#include "slib/core/memory_reader.h"
#include "slib/core/memory_output.h"
#include "slib/core/process.h"
#include "slib/core/service_manager.h"

#define SERVER_TAG "SMB SERVER"

#define IPC_PATH SLIB_UNICODE("IPC$")
#define IPC_WKSSVC SLIB_UNICODE("wkssvc")
#define IPC_SRVSVC SLIB_UNICODE("srvsvc")

#define MAX_RESERVED_ID 0x10000
#define TREE_ID_IPC 1
#define FILE_ID_WKSSVC 1
#define FILE_ID_SRVSVC 2

#define FILE_ACCESS_MASK SmbAccessMask::Read | SmbAccessMask::ReadAttributes | SmbAccessMask::ReadExtendedAttributes | SmbAccessMask::ReadControl | SmbAccessMask::Execute | SmbAccessMask::Synchronize

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
				response.setCreditGranted(request.getCreditGranted());
				response.setFlags(Smb2HeaderFlags::Response | Smb2HeaderFlags::Priority);
				response.setProcessId(request.getProcessId());
				response.setMessageId(request.getMessageId());
				response.setSessionId(request.getSessionId());
				response.setTreeId(request.getTreeId());
			}

			static sl_uint64 GetFileId(sl_uint8* guid)
			{
				return MIO::readUint64LE(guid);
			}

			static void SetFileId(sl_uint8* guid, sl_uint64 fileId)
			{
				MIO::writeUint64(guid, fileId);
			}

			static sl_uint32 ToNetworkAttrs(const FileAttributes& attrs)
			{
				sl_uint32 n = (sl_uint32)(attrs & (FileAttributes::Directory | FileAttributes::ReadOnly | FileAttributes::Hidden));
				if (!(n & FileAttributes::Directory)) {
					n |= FileAttributes::Normal;
				}
				return n;
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

			static sl_bool WriteErrorResponse(SmbServer::Smb2Param& param, SmbStatus status)
			{
				Smb2Header smb;
				InitSmb2ResponseHeader(smb, *(param.smb));
				smb.setStatus(status);

				Smb2ErrorResponseMessage response;
				Base::zeroMemory(&response, sizeof(response));
				response.setSize(sizeof(response), sl_true);

				return WriteResponse(param, smb, response);
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

			// DCE/RPC
			static String16 RpcReadString(MemoryReader& reader)
			{
				sl_uint32 maxCount;
				if (!(reader.readUint32(&maxCount))) {
					return sl_null;
				}
				sl_uint32 offset;
				if (!(reader.readUint32(&offset))) {
					return sl_null;
				}
				sl_uint32 actualCount;
				if (!(reader.readUint32(&actualCount))) {
					return sl_null;
				}
				if (offset + actualCount > maxCount) {
					return sl_null;
				}
				if (!maxCount) {
					return String16::getEmpty();
				}
				sl_uint32 allocCount = maxCount;
				if (allocCount & 1) {
					allocCount += 1;
				}
				if (reader.getRemainedSize() < allocCount) {
					return sl_null;
				}
				String16 ret = String16::allocate(actualCount);
				if (ret.isNull()) {
					return sl_null;
				}
				sl_char16* str =  ret.getData();
				sl_uint8* data = (sl_uint8*)(reader.getBuffer() + reader.getPosition()) + (offset << 1);
				sl_uint32 len = 0;
				for (sl_uint32 i = 0; i < actualCount; i++) {
					str[i] = MIO::readUint16LE(data);
					if (str[i]) {
						len = i + 1;
					}
					data += 2;
				}
				reader.skip(allocCount << 1);
				ret.setLength(len);
				return ret;
			}

			static Memory RpcWriteString(const StringParam& _str)
			{
				StringData16 str(_str);
				sl_uint32 len = (sl_uint32)(str.getLength());
				sl_uint32 lenAlloc = len + 1;
				if (lenAlloc & 1) {
					lenAlloc += 1;
				}
				Memory mem = Memory::create(12 + (lenAlloc << 1));
				if (mem.isNull()) {
					return sl_null;
				}
				sl_uint8* buf = (sl_uint8*)(mem.getData());
				MIO::writeUint32LE(buf, len + 1); // max count
				MIO::writeUint32LE(buf + 4, 0); // offset
				MIO::writeUint32LE(buf + 8, len + 1); // actual count
				buf += 12;
				sl_char16* s = str.getData();
				sl_uint32 i = 0;
				for (; i < len; i++) {
					MIO::writeUint16LE(buf, s[i]);
					buf += 2;
				}
				for (; i < lenAlloc; i++) {
					MIO::writeUint16LE(buf, 0);
					buf += 2;
				}
				return mem;
			}

			Memory GenerateFileIdBothDirectoryInfo(const String16& fileName, SmbFileInfo& info)
			{
				sl_uint32 lenFileName = (sl_uint32)(fileName.getLength());
				
				sl_size size = sizeof(Smb2FindFileIdBothDirectoryInfo) + (lenFileName << 1);
				size = ((size - 1) | 15) + 1;
				
				Memory mem = Memory::create(size);
				if (mem.isNull()) {
					return sl_null;
				}
				sl_uint8* buf = (sl_uint8*)(mem.getData());
				Base::zeroMemory(buf, size);

				Smb2FindFileIdBothDirectoryInfo* header = (Smb2FindFileIdBothDirectoryInfo*)buf;
				header->setNextOffset((sl_uint32)size);
				header->setCreationTime(info.createdAt);
				header->setLastAccessTime(info.modifiedAt);
				header->setLastChangeTime(info.modifiedAt);
				header->setLastWriteTime(info.modifiedAt);
				header->setEndOfFile(info.size);
				header->setAllocationSize(info.size);
				header->setAttributes(ToNetworkAttrs(info.attributes));
				header->setFileNameLength(lenFileName << 1);

				buf += sizeof(Smb2FindFileIdBothDirectoryInfo);
				sl_char16* dataFileName = fileName.getData();
				for (sl_uint32 i = 0; i < lenFileName; i++) {
					MIO::writeUint16LE(buf, dataFileName[i]);
					buf += 2;
				}
				return mem;
			}

		}
	}

	using namespace priv::smb;


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(SmbFileInfo)

	SmbFileInfo::SmbFileInfo() noexcept
	{
	}


	SLIB_DEFINE_ROOT_OBJECT(SmbServerFileContext)

	SmbServerFileContext::SmbServerFileContext()
	{
		m_flagReturnedList = sl_false;
	}

	SmbServerFileContext::~SmbServerFileContext()
	{
	}

	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(SmbCreateFileParam)

	SmbCreateFileParam::SmbCreateFileParam()
	{
	}


	SLIB_DEFINE_OBJECT(SmbServerShare, Object)
	
	SmbServerShare::SmbServerShare()
	{
	}

	SmbServerShare::~SmbServerShare()
	{
	}

	String SmbServerShare::getComment()
	{
		return m_comment;
	}

	void SmbServerShare::setComment(const String& comment)
	{
		m_comment = comment;
	}


	SmbServerFileShare::SmbServerFileShare(const String& rootPath): m_rootPath(rootPath)
	{
	}

	SmbServerFileShare::SmbServerFileShare(const String& rootPath, const String& comment): m_rootPath(rootPath)
	{
		setComment(comment);
	}

	SmbServerFileShare::~SmbServerFileShare()
	{
	}

	Ref<SmbServerFileContext> SmbServerFileShare::createFile(const SmbCreateFileParam& param)
	{
		String path = getFilePath(param.path);
		File file = File::openForRead(path);
		if (file.isNotNone()) {
			return new FileContext(Move(path), Move(file));
		} else {
			if (File::isDirectory(path)) {
				return new FileContext(Move(path));
			}
		}
		return sl_null;
	}

	sl_uint32 SmbServerFileShare::readFile(SmbServerFileContext* _context, sl_uint64 offset, void* buf, sl_uint32 size)
	{
		FileContext* context = (FileContext*)_context;
		if (context) {
			sl_int32 n = context->file.readAt32(offset, buf, size);
			if (n > 0) {
				return n;
			}
		}
		return 0;
	}

	sl_bool SmbServerFileShare::getFileInfo(SmbServerFileContext* _context, SmbFileInfo& _out)
	{
		FileContext* context = (FileContext*)_context;
		if (context) {
			_out.attributes = File::getAttributes(context->path);
			_out.size = File::getSize(context->path);
			_out.createdAt = File::getCreatedTime(context->path);
			_out.modifiedAt = File::getModifiedTime(context->path);
			return sl_true;
		}
		return sl_false;
	}

	HashMap<String16, SmbFileInfo> SmbServerFileShare::getFiles(SmbServerFileContext* _context)
	{
		FileContext* context = (FileContext*)_context;
		if (context) {
			HashMap<String16, SmbFileInfo> ret;
			for (auto& item : File::getFileInfos(context->path)) {
				SmbFileInfo info;
				info.size = item.value.size;
				info.attributes = item.value.attributes;
				info.createdAt = item.value.createdAt;
				info.modifiedAt = item.value.modifiedAt;
				ret.put_NoLock(String16::from(item.key), info);
			}
			return ret;
		}
		return sl_null;
	}

	String SmbServerFileShare::getFilePath(const StringView16& path) noexcept
	{
#ifdef SLIB_PLATFORM_IS_WINDOWS
		return String::join(m_rootPath, "\\", path);
#else
		String ret = String::join(m_rootPath, "/", path);
		sl_char8* data = ret.getData();
		sl_size len = ret.getLength();
		for (sl_size i = 0; i < len; i++) {
			if (data[i] == '\\') {
				data[i] = '/';
			}
		}
		return ret;
#endif
	}

	SmbServerFileShare::FileContext::FileContext(String&& _path, File&& _file) : path(Move(_path)), file(Move(_file))
	{
	}

	SmbServerFileShare::FileContext::FileContext(String&& _path) : path(Move(_path))
	{
	}

	SmbServerFileShare::FileContext::~FileContext()
	{
	}


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

	void SmbServerParam::initNames()
	{
		if (domainName.isNull()) {
			domainName = targetName;
		}
		if (targetDescription.isNull()) {
			targetDescription = targetName;
		}
		if (computerName_NetBIOS.isNull()) {
			computerName_NetBIOS = targetName;
		}
		if (domainName_NetBIOS.isNull()) {
			domainName_NetBIOS = targetName;
		}
		if (computerName_DNS.isNull()) {
			computerName_DNS = domainName;
		}
		if (domainName_DNS.isNull()) {
			domainName_DNS = domainName;
		}
	}

	void SmbServerParam::addShare(const String& name, const Ref<SmbServerShare>& share)
	{
		shares.put(String16::from(name), share);
	}

	void SmbServerParam::addFileShare(const String& name, const String& rootPath)
	{
		shares.put(String16::from(name), new SmbServerFileShare(rootPath));
	}

	void SmbServerParam::addFileShare(const String& name, const String& rootPath, const String& comment)
	{
		shares.put(String16::from(name), new SmbServerFileShare(rootPath, comment));
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
				server->m_param.initNames();
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

		SmbServerSession session;
		session.server = this;

		NetBIOS_SessionMessage msg;
		while (thread->isNotStopping()) {
			sl_int32 n = msg.read(socket);
			if (n == SLIB_IO_ENDED) {
				IOParam param;
				param.socket = &socket;
				param.event = ev.get();
				param.data = msg.message;
				param.size = msg.sizeMessage;
				param.session = &session;
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
		case Smb2Command::TreeDisconnect:
			return _onProcessTreeDisconnect(param);
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
		case Smb2Command::Find:
			return _onProcessFind(param);
		case Smb2Command::Notify:
			return _onProcessNotify(param);
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

			response.setDialect(0x0311);
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
				targetInfo.addItem(NtlmTargetInfoItemType::NetBIOS_DomainName, m_param.domainName_NetBIOS);
				targetInfo.addItem(NtlmTargetInfoItemType::NetBIOS_ComputerName, m_param.computerName_NetBIOS);
				targetInfo.addItem(NtlmTargetInfoItemType::DNS_DomainName, m_param.domainName_DNS);
				targetInfo.addItem(NtlmTargetInfoItemType::DNS_ComputerName, m_param.computerName_DNS);
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
			treeId = TREE_ID_IPC;
			response.setShareType(Smb2ShareType::NamedPipe);
			response.setAccessMask(SmbAccessMask::Read | SmbAccessMask::Synchronize);
		} else {
			treeId = param.session->connectTree(path);
			if (treeId) {
				response.setShareType(Smb2ShareType::Disk);
				response.setAccessMask(FILE_ACCESS_MASK);
			}
		}

		if (treeId) {
			Smb2Header smb;
			InitSmb2ResponseHeader(smb, *(param.smb));
			smb.setTreeId(treeId);
			return WriteResponse(param, smb, response);
		} else {
			return WriteErrorResponse(param, SmbStatus::BadNetworkName);
		}
	}

	sl_bool SmbServer::_onProcessTreeDisconnect(SmbServer::Smb2Param& param)
	{
		Smb2Header smb;
		InitSmb2ResponseHeader(smb, *(param.smb));
		
		Smb2EmptyMessage response;
		Base::zeroMemory(&response, sizeof(response));
		response.setSize(sizeof(response), sl_false);

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
		Memory memExtraInfo;

		StringView16 filePath((sl_char16*)(param.data + fileNameOffset), fileNameLength >> 1);
		sl_uint64 fileId = 0;
		sl_uint32 treeId = param.smb->getTreeId();
		if (treeId < MAX_RESERVED_ID) {
			if (treeId == TREE_ID_IPC) {
				if (filePath == IPC_WKSSVC) {
					fileId = FILE_ID_WKSSVC;
				} else if (filePath == IPC_SRVSVC) {
					fileId = FILE_ID_SRVSVC;
				}
				if (fileId) {
					response.setAction(SmbCreateAction::Existed);
					response.setAttributes(FileAttributes::Normal);
				}
			}
		} else {
			SmbServerShare* share = param.session->getTree(treeId);
			if (share) {
				SmbCreateFileParam cp;
				cp.path = filePath;
				Ref<SmbServerFileContext> file = share->createFile(cp);
				if (file.isNotNull()) {
					fileId = param.session->registerFile(file.get());
					if (fileId) {
						response.setAction(SmbCreateAction::Existed);
						SmbFileInfo info;
						if (share->getFileInfo(file.get(), info)) {
							if (!(info.attributes & FileAttributes::NotExist)) {
								response.setAttributes(ToNetworkAttrs(info.attributes));
								if (!(info.attributes & FileAttributes::Directory)) {
									response.setAllocationSize(info.size);
									response.setEndOfFile(info.size);
								}
								response.setCreationTime(info.createdAt);
								response.setLastAccessTime(info.modifiedAt);
								response.setLastChangeTime(info.modifiedAt);
								response.setLastWriteTime(info.modifiedAt);
								
								memExtraInfo = Memory::create(sizeof(Smb2ExtraInfoItemHeader) + 8 + sizeof(Smb2ExtraInfoItem_MxAcResponse));
								if (memExtraInfo.isNotNull()) {
									sl_uint8* buf = (sl_uint8*)(memExtraInfo.getData());
									Base::zeroMemory(buf, memExtraInfo.getSize());
									Smb2ExtraInfoItemHeader& info = *((Smb2ExtraInfoItemHeader*)buf);
									info.setTagOffset(sizeof(info));
									info.setTagLength(4);
									info.setBlobOffset(sizeof(info) + 8);
									info.setBlobLength(sizeof(Smb2ExtraInfoItem_MxAcResponse));
									Base::copyMemory(buf + sizeof(info), "MxAc", 4);
									Smb2ExtraInfoItem_MxAcResponse& item = *((Smb2ExtraInfoItem_MxAcResponse*)(buf + sizeof(info) + 8));
									item.setAccessMask(FILE_ACCESS_MASK);
								}
							}
						} else {
							response.setAttributes(FileAttributes::Normal);
						}
					}
				}
			}
		}
		if (fileId) {
			MIO::writeUint64LE(response.getGuid(), fileId);
			if (fileId < MAX_RESERVED_ID) {
				Math::randomMemory(response.getGuid() + 8, 8);
			}
		} else {
			return WriteErrorResponse(param, SmbStatus::ObjectNameNotFound);
		}
		Smb2Header smb;
		InitSmb2ResponseHeader(smb, *(param.smb));
		if (memExtraInfo.isNotNull()) {
			response.setDynamicSize();
			response.setBlobOffset(sizeof(smb) + sizeof(response));
			response.setBlobLength((sl_uint32)(memExtraInfo.getSize()));
		}
		return WriteResponse(param, smb, response, memExtraInfo.getData(), memExtraInfo.getSize());
	}

	sl_bool SmbServer::_onProcessClose(SmbServer::Smb2Param& param)
	{
		if (param.size < sizeof(Smb2Header) + sizeof(Smb2CloseRequestMessage)) {
			return sl_false;
		}

		Smb2CloseRequestMessage* request = (Smb2CloseRequestMessage*)(param.data + sizeof(Smb2Header));

		sl_uint64 fileId = GetFileId(request->getGuid());
		if (fileId >= MAX_RESERVED_ID) {
			param.session->unregisterFile(fileId);
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
		sl_uint8 stack[65536];
		MemoryData data;

		sl_uint64 fileId = GetFileId(request->getGuid());
		if (fileId < MAX_RESERVED_ID) {
			if (fileId == FILE_ID_WKSSVC) {
				// DCE/RPC
				data = MemoryData(
					"\x05\x00\x0c\x03\x10\x00\x00\x00\x44\x00\x00\x00\x02\x00\x00\x00" \
					"\xb8\x10\xb8\x10\xf0\x53\x00\x00\x0d\x00\x5c\x50\x49\x50\x45\x5c" \
					"\x77\x6b\x73\x73\x76\x63\x00\x00\x01\x00\x00\x00\x00\x00\x00\x00" \
					"\x04\x5d\x88\x8a\xeb\x1c\xc9\x11\x9f\xe8\x08\x00\x2b\x10\x48\x60" \
					"\x02\x00\x00\x00");
			} else if (fileId == FILE_ID_SRVSVC) {
				// DCE/RPC
				data = MemoryData(
					"\x05\x00\x0c\x03\x10\x00\x00\x00\x44\x00\x00\x00\x02\x00\x00\x00" \
					"\xb8\x10\xb8\x10\xf0\x53\x00\x00\x0d\x00\x5c\x50\x49\x50\x45\x5c" \
					"\x73\x72\x76\x73\x76\x63\x00\x00\x01\x00\x00\x00\x00\x00\x00\x00" \
					"\x04\x5d\x88\x8a\xeb\x1c\xc9\x11\x9f\xe8\x08\x00\x2b\x10\x48\x60" \
					"\x02\x00\x00\x00");
			}
		} else {
			SmbServerShare* share = param.session->getTree(param.smb->getTreeId());
			if (share) {
				Ref<SmbServerFileContext> file = param.session->getFile(fileId);
				if (file.isNotNull()) {
					sl_uint32 len = request->getReadLength();
					if (len <= sizeof(stack)) {
						data.data = stack;
					} else {
						data = MemoryData(Memory::create(len));
						if (!(data.data)) {
							return WriteErrorResponse(param, SmbStatus::Unsuccessful);
						}
					}
					data.size = share->readFile(file.get(), request->getFileOffset(), data.data, len);
				}
			}
		}

		if (!(data.size)) {
			return WriteErrorResponse(param, SmbStatus::Unsuccessful);
		}

		Smb2Header smb;
		InitSmb2ResponseHeader(smb, *(param.smb));

		Smb2ReadResponseMessage response;
		Base::zeroMemory(&response, sizeof(response));
		response.setSize(sizeof(response), sl_true);
		response.setDataOffset(sizeof(smb) + sizeof(response));
		response.setReadCount((sl_uint32)(data.size));
		
		return WriteResponse(param, smb, response, data.data, data.size);
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
		sl_uint64 fileId = GetFileId(request->getGuid());
		if (fileId < MAX_RESERVED_ID) {
			sizeWritten = dataLength;
		}

		if (!sizeWritten) {
			return WriteErrorResponse(param, SmbStatus::Unsuccessful);
		}

		Smb2Header smb;
		InitSmb2ResponseHeader(smb, *(param.smb));

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
		if (!(request->checkSize(sizeof(Smb2IoctlRequestMessage), sl_true))) {
			return sl_false;
		}
		sl_uint16 inputOffset = request->getDataOffset();
		sl_uint32 inputLength = request->getDataLength();
		if ((sl_uint32)(inputOffset + inputLength) > param.size) {
			return sl_false;
		}

		Smb2Header smb;
		InitSmb2ResponseHeader(smb, *(param.smb));

		Memory data;
		sl_uint32 func = request->getFunction();
		if (func == 0x0011c017) { // FSCTL_PIPE_TRANSCEIVE
			sl_uint64 fileId = GetFileId(request->getGuid());
			if (fileId < MAX_RESERVED_ID) {
				if (fileId == FILE_ID_WKSSVC || fileId == FILE_ID_SRVSVC) {
					// DCE/RPC
					data = _processRpc(fileId, param.data + inputOffset, inputLength);
				}
			}
			if (data.isNull()) {
				return WriteErrorResponse(param, SmbStatus::Unsuccessful);
			}
		} else {
			return WriteErrorResponse(param, SmbStatus::NotFound);
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

	sl_bool SmbServer::_onProcessFind(SmbServer::Smb2Param& param)
	{
		if (param.size < sizeof(Smb2Header) + sizeof(Smb2FindRequestMessage)) {
			return sl_false;
		}
		Smb2FindRequestMessage* request = (Smb2FindRequestMessage*)(param.data + sizeof(Smb2Header));
		if (!(request->checkSize(sizeof(Smb2FindRequestMessage), sl_true))) {
			return sl_false;
		}
		sl_uint16 patternOffset = request->getSearchPatternOffset();
		sl_uint32 patternLength = request->getSearchPatternLength();
		if ((sl_uint32)(patternOffset + patternLength) > param.size) {
			return sl_false;
		}

		Memory memOutput;
		if (request->getLevel() == Smb2FindLevel::FindIdBothDirectoryInfo) {
			StringView16 pattern((sl_char16*)(param.data + patternOffset), patternLength >> 1);
			sl_uint64 fileId = GetFileId(request->getGuid());
			if (fileId >= MAX_RESERVED_ID) {
				SmbServerShare* share = param.session->getTree(param.smb->getTreeId());
				if (share) {
					Ref<SmbServerFileContext> file = param.session->getFile(fileId);
					if (file.isNotNull()) {
						if (file->m_flagReturnedList) {
							return WriteErrorResponse(param, SmbStatus::NoMoreFiles);
						}
						MemoryBuffer bufTotal;
						if (pattern == SLIB_UNICODE("*")) {
							SmbFileInfo info;
							Base::zeroMemory(&info, sizeof(info));
							info.attributes = FileAttributes::Directory;
							SLIB_STATIC_STRING16(s1, ".")
							SLIB_STATIC_STRING16(s2, "..")
							bufTotal.add(GenerateFileIdBothDirectoryInfo(s1, info));
							bufTotal.add(GenerateFileIdBothDirectoryInfo(s2, info));
							for (auto& item : share->getFiles(file.get())) {
								bufTotal.add(GenerateFileIdBothDirectoryInfo(item.key, item.value));
							}
						} else if (pattern.startsWith('*')) {
							for (auto& item : share->getFiles(file.get())) {
								if (item.key.endsWith(pattern.substring(1))) {
									bufTotal.add(GenerateFileIdBothDirectoryInfo(item.key, item.value));
								}
							}
						} else if (pattern.endsWith('*')) {
							for (auto& item : share->getFiles(file.get())) {
								if (item.key.startsWith(pattern.substring(0, pattern.getLength() - 1))) {
									bufTotal.add(GenerateFileIdBothDirectoryInfo(item.key, item.value));
								}
							}
						} else {
							for (auto& item : share->getFiles(file.get())) {
								if (item.key == pattern) {
									bufTotal.add(GenerateFileIdBothDirectoryInfo(item.key, item.value));
								}
							}
						}
						MemoryData* lastElement = bufTotal.getLastData();
						if (lastElement) {
							Smb2FindFileIdBothDirectoryInfo* info = (Smb2FindFileIdBothDirectoryInfo*)(lastElement->data);
							info->setNextOffset(0);
						}
						memOutput = bufTotal.merge();
						file->m_flagReturnedList = sl_true;
					}
				}
			}
		}
		
		Smb2Header smb;
		InitSmb2ResponseHeader(smb, *(param.smb));

		Smb2FindResponseMessage response;
		Base::zeroMemory(&response, sizeof(response));
		response.setSize(sizeof(response), sl_true);
		response.setBlobOffset(sizeof(smb) + sizeof(response));
		response.setBlobLength((sl_uint32)(memOutput.getSize()));

		return WriteResponse(param, smb, response, memOutput.getData(), memOutput.getSize());
	}

	sl_bool SmbServer::_onProcessNotify(SmbServer::Smb2Param& param)
	{
		return WriteErrorResponse(param, SmbStatus::NotImplemented);
	}

	sl_bool SmbServer::_onProcessGetInfo(SmbServer::Smb2Param& param)
	{
		if (param.size < sizeof(Smb2Header) + sizeof(Smb2GetInfoRequestMessage)) {
			return sl_false;
		}

		Smb2GetInfoRequestMessage* request = (Smb2GetInfoRequestMessage*)(param.data + sizeof(Smb2Header));
		Memory data;

		if (request->getClass() == Smb2GetInfoClass::File) {
			Smb2GetInfoLevel level = request->getLevel();
			if (level == Smb2GetInfoLevel::FileStandardInfo) {
				sl_uint64 fileId = GetFileId(request->getGuid());
				if (fileId < MAX_RESERVED_ID) {
					if (fileId == FILE_ID_WKSSVC || fileId == FILE_ID_SRVSVC) {
						Smb2FileStandardInfo info;
						Base::zeroMemory(&info, sizeof(info));
						info.setAllocationSize(4096);
						info.setLinkCount(1);
						data = Memory::create(&info, sizeof(info));
					}
				}
			} else if (level == Smb2GetInfoLevel::FileNetworkOpenInfo) {
				sl_uint64 fileId = GetFileId(request->getGuid());
				if (fileId >= MAX_RESERVED_ID) {
					SmbServerShare* share = param.session->getTree(param.smb->getTreeId());
					if (share) {
						Ref<SmbServerFileContext> file = param.session->getFile(fileId);
						if (file.isNotNull()) {
							SmbFileInfo si;
							if (share->getFileInfo(file.get(), si)) {
								Smb2FileNetworkOpenInfo info;
								Base::zeroMemory(&info, sizeof(info));
								info.setCreationTime(si.createdAt);
								info.setLastAccessTime(si.modifiedAt);
								info.setLastChangeTime(si.modifiedAt);
								info.setLastWriteTime(si.modifiedAt);
								info.setAllocationSize(si.size);
								info.setEndOfFile(si.size);
								info.setAttributes(ToNetworkAttrs(si.attributes));
								data = Memory::create(&info, sizeof(info));
							}
						}
					}
				}
			} else {
				return WriteErrorResponse(param, SmbStatus::InvalidInfoClass);
			}
		} else {
			return WriteErrorResponse(param, SmbStatus::InvalidInfoClass);
		}

		if (data.isNull()) {
			return WriteErrorResponse(param, SmbStatus::Unsuccessful);
		}
		
		Smb2Header smb;
		InitSmb2ResponseHeader(smb, *(param.smb));

		Smb2GetInfoResponseMessage response;
		Base::zeroMemory(&response, sizeof(response));
		response.setSize(sizeof(response), sl_true);
		response.setBlobOffset(sizeof(smb) + sizeof(response));
		response.setBlobLength((sl_uint32)(data.getSize()));

		return WriteResponse(param, smb, response, data.getData(), data.getSize());
	}

	Memory SmbServer::_processRpc(sl_uint64 fileId, sl_uint8* packet, sl_uint32 size)
	{
		if (sizeof(DceRpcHeader) > size) {
			return sl_null;
		}
		DceRpcHeader* inputHeader = (DceRpcHeader*)packet;
		if ((inputHeader->getPacketFlags() & (DceRpcPacketFlags::FirstFragment | DceRpcPacketFlags::LastFragment)) != (DceRpcPacketFlags::FirstFragment | DceRpcPacketFlags::LastFragment)) {
			return sl_null;
		}
		if (!(inputHeader->isLittleEndian())) {
			return sl_null;
		}
		if (inputHeader->getFragmentLength() != size) {
			return sl_null;
		}
		DceRpcPacketType inputType = inputHeader->getPacketType();

		packet += sizeof(DceRpcHeader);
		size -= sizeof(DceRpcHeader);

		DceRpcHeader outputHeader;
		Base::zeroMemory(&outputHeader, sizeof(outputHeader));
		outputHeader.setVersion(5);
		outputHeader.setPacketFlags(DceRpcPacketFlags::FirstFragment | DceRpcPacketFlags::LastFragment);
		outputHeader.setLittleEndian();
		outputHeader.setCallId(inputHeader->getCallId());

		if (inputType == DceRpcPacketType::Request) {

			if (sizeof(DceRpcRequestHeader) > size) {
				return sl_null;
			}
			DceRpcRequestHeader* requestHeader = (DceRpcRequestHeader*)packet;
			DceRpcRequestOperation op = requestHeader->getOperation();

			DceRpcResponseHeader responseHeader;
			Base::zeroMemory(&responseHeader, sizeof(responseHeader));
			outputHeader.setPacketType(DceRpcPacketType::Response);

			packet += sizeof(DceRpcRequestHeader);
			size -= sizeof(DceRpcRequestHeader);
			MemoryReader reader(packet, size);
			Memory content;

			switch (op) {
			case DceRpcRequestOperation::NetWkstaGetInfo:
				if (fileId == FILE_ID_WKSSVC) {
					sl_uint32 refId = reader.readUint32();
					String16 serverName = RpcReadString(reader);
					if (serverName.isNull()) {
						return sl_null;
					}
					sl_uint32 level = reader.readUint32();
					if (level != 100) {
						return sl_null;
					}
					refId >>= 2;
					refId++;
					MemoryOutput output;
					output.writeUint32(level);
					output.writeUint32((refId++) << 2); // Info
					output.writeUint32((sl_uint32)(SRVSVC_PlatformId::NT));
					output.writeUint32((refId++) << 2); // Server Name
					output.writeUint32((refId++) << 2); // Domain Name
					output.writeUint32(6); // Major Version (Windows OS)
					output.writeUint32(1); // Minor Version (Windows OS)
					output.write(RpcWriteString(m_param.targetName));
					output.write(RpcWriteString(m_param.domainName));
					output.writeUint32(0); // Windows Error
					content = output.getData();
				}
				break;
			case DceRpcRequestOperation::NetSrvGetInfo:
				if (fileId == FILE_ID_SRVSVC) {
					sl_uint32 refId = reader.readUint32();
					String16 serverName = RpcReadString(reader);
					if (serverName.isNull()) {
						return sl_null;
					}
					sl_uint32 level = reader.readUint32();
					if (level != 101) {
						return sl_null;
					}
					refId >>= 2;
					refId++;
					MemoryOutput output;
					output.writeUint32(level);
					output.writeUint32((refId++) << 2); // Info
					output.writeUint32((sl_uint32)(SRVSVC_PlatformId::NT));
					output.writeUint32((refId++) << 2); // Server Name
					output.writeUint32(6); // Major Version (Windows OS)
					output.writeUint32(1); // Minor Version (Windows OS)
					output.writeUint32(SRVSVC_ServerType::Workstation | SRVSVC_ServerType::Server | SRVSVC_ServerType::UnixServer | SRVSVC_ServerType::NtWorkstation | SRVSVC_ServerType::NtServer); // Server Type
					output.writeUint32((refId++) << 2); // Comment
					output.write(RpcWriteString(m_param.targetName));
					output.write(RpcWriteString(m_param.targetDescription));
					output.writeUint32(0); // Windows Error
					content = output.getData();
				}
				break;
			case DceRpcRequestOperation::NetShareEnumAll:
				if (fileId == FILE_ID_SRVSVC) {
					sl_uint32 refId = reader.readUint32();
					String16 serverUnc = RpcReadString(reader);
					if (serverUnc.isNull()) {
						return sl_null;
					}
					sl_uint32 level = reader.readUint32();
					if (level != 1) {
						return sl_null;
					}
					sl_uint32 ctl = reader.readUint32();
					if (ctl != 1) {
						return sl_null;
					}
					refId = reader.readUint32();
					refId >>= 2;
					refId++;

					ListElements< Pair< String16, Ref<SmbServerShare> > > list(m_param.shares.toList());
					sl_uint32 nShares = (sl_uint32)(list.count);

					MemoryOutput output;
					output.writeUint32(level);
					output.writeUint32(ctl);
					output.writeUint32((refId++) << 2); // Ctrl
					output.writeUint32(nShares); // Count
					output.writeUint32((refId++) << 2); // Array
					output.writeUint32(nShares); // Max Count
					sl_uint32 i;
					for (i = 0; i < nShares; i++) {
						output.writeUint32((refId++) << 2); // Name
						output.writeUint32(0); // Type: Disk
						output.writeUint32((refId++) << 2); // Comment
					}
					for (i = 0; i < nShares; i++) {
						output.write(RpcWriteString(list[i].first));
						output.write(RpcWriteString(list[i].second->getComment()));
					}
					output.writeUint32(nShares); // Total Entries
					output.writeUint32(0); // Resume Handle
					output.writeUint32(0); // Windows Error
					content = output.getData();
				}
				break;
			case DceRpcRequestOperation::NetShareGetInfo:
				if (fileId == FILE_ID_SRVSVC) {
					sl_uint32 refId = reader.readUint32();
					String16 serverUnc = RpcReadString(reader);
					if (serverUnc.isNull()) {
						return sl_null;
					}
					String16 shareName = RpcReadString(reader);
					if (shareName.isNull()) {
						return sl_null;
					}
					sl_uint32 level = reader.readUint32();
					if (level != 1) {
						return sl_null;
					}

					refId >>= 2;
					refId++;

					Ref<SmbServerShare> share = m_param.shares.getValue(shareName);
					if (share.isNull()) {
						return sl_null;
					}

					MemoryOutput output;
					output.writeUint32(level);
					output.writeUint32((refId++) << 2); // Info1
					output.writeUint32((refId++) << 2); // Name
					output.writeUint32(0); // Type: Disk
					output.writeUint32((refId++) << 2); // Comment
					output.write(RpcWriteString(shareName));
					output.write(RpcWriteString(share->getComment()));
					output.writeUint32(0); // Windows Error
					content = output.getData();
				}
				break;
			}

			if (content.isNull()) {
				return sl_null;
			}
			sl_uint32 sizeContent = (sl_uint32)(content.getSize());
			responseHeader.setAllocHint(sizeContent);
			outputHeader.setFragmentLength((sl_uint16)(sizeof(outputHeader) + sizeof(responseHeader) + sizeContent));

			MemoryBuffer buf;
			buf.addNew(&outputHeader, sizeof(outputHeader));
			buf.addNew(&responseHeader, sizeof(responseHeader));
			buf.add(Move(content));
			return buf.merge();
		}
		return sl_null;
	}


	SmbServerSession::SmbServerSession(): server(sl_null)
	{
	}

	SmbServerSession::~SmbServerSession()
	{
	}

	sl_uint32 SmbServerSession::connectTree(const String16& path) noexcept
	{
		sl_uint32 treeId = treeIds.getValue_NoLock(path);
		if (treeId) {
			return treeId;
		}
		Ref<SmbServerShare> share = server->m_param.shares.getValue(path);
		if (share.isNull()) {
			return 0;
		}
		treeId = Base::interlockedIncrement32(&(server->m_lastTreeId));
		treeId = MAX_RESERVED_ID + (treeId & 0x7fffffff);
		trees.put_NoLock(treeId, Move(share));
		treeIds.put_NoLock(path, treeId);
		return treeId;
	}

	SmbServerShare* SmbServerSession::getTree(sl_uint32 treeId) noexcept
	{
		return trees.getValue_NoLock(treeId).get();
	}

	sl_uint64 SmbServerSession::registerFile(SmbServerFileContext* context) noexcept
	{
		sl_uint64 fileId = Base::interlockedIncrement64(&(server->m_lastFileId));
		fileId += MAX_RESERVED_ID;
		files.put_NoLock(fileId, context);
		return fileId;
	}

	void SmbServerSession::unregisterFile(sl_uint64 fileId) noexcept
	{
		files.remove_NoLock(fileId);
	}

	Ref<SmbServerFileContext> SmbServerSession::getFile(sl_uint64 fileId) noexcept
	{
		return files.getValue_NoLock(fileId);
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
