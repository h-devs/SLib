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

#ifndef CHECKHEADER_SLIB_NETWORK_SMB
#define CHECKHEADER_SLIB_NETWORK_SMB

#include "socket.h"

#include "../core/flags.h"
#include "../core/mio.h"
#include "../core/time.h"
#include "../core/thread_pool.h"

namespace slib
{

	enum class SmbCommand
	{
		Negotiate = 0x72
	};

	SLIB_DEFINE_FLAGS(SmbHeaderFlags, {
		Response = 0x800000,
		Notify = 0x400000,
		Oplocks = 0x200000,
		CanonicalizedPathnames = 0x100000,
		Caseless = 0x80000,
		ReceiveBufferPosted = 0x20000,
		SupportLock = 0x10000,
		Unicode = 0x8000,
		NtErrorCode = 0x4000,
		PermitReadExecuteOnly = 0x2000,
		ResovePathnameWithDfs = 0x1000,
		SupportExtendedSecurityNegotiation = 0x0800,
		ReparsePath = 0x0400,
		LongNames = 0x0040, // Long Names Used: Path names in request are long file names
		RequireSecuritySignature = 0x0010,
		Compressed = 0x0008,
		SupportSecuritySignatures = 0x0004,
		SupportExtendedAttributes = 0x0002,
		AllowedLongNames = 0x0001 // Long Names Allowed: Long file names are allowed in the response
	})

	class SLIB_EXPORT SmbHeader
	{
	public:
		sl_bool isSmb() const noexcept
		{
			return _serverComponent[0] == 0xff && _serverComponent[1] == 'S' && _serverComponent[2] == 'M' && _serverComponent[3] == 'B';
		}

		void setSmb() noexcept
		{
			_serverComponent[0] = 0xff;
			_serverComponent[1] = 'S';
			_serverComponent[2] = 'M';
			_serverComponent[3] = 'B';
		}

		SmbCommand getCommand() const noexcept
		{
			return (SmbCommand)_command;
		}

		void setCommand(SmbCommand command) noexcept
		{
			_command = (sl_uint8)command;
		}

		SmbHeaderFlags getFlags() const noexcept
		{
			return SLIB_MAKE_DWORD(0, _flags[0], _flags[2], _flags[1]);
		}

		void setFlags(const SmbHeaderFlags& flags) noexcept
		{
			sl_uint32 v = (sl_uint32)(flags.value);
			_flags[0] = SLIB_GET_BYTE2(v);
			_flags[1] = SLIB_GET_BYTE0(v);
			_flags[2] = SLIB_GET_BYTE1(v);
		}

		// 8 Bytes
		const sl_uint8* getSignature() const noexcept
		{
			return _signature;
		}

		// 8 Bytes
		sl_uint8* getSignature() noexcept
		{
			return _signature;
		}

		sl_uint32 getProcessId() const noexcept
		{
			return SLIB_MAKE_DWORD(_processIdHigh[1], _processIdHigh[0], _processId[1], _processId[0]);
		}

		void setProcessId(sl_uint32 _id) noexcept
		{
			_processId[0] = SLIB_GET_BYTE0(_id);
			_processId[1] = SLIB_GET_BYTE1(_id);
			_processIdHigh[0] = SLIB_GET_BYTE2(_id);
			_processIdHigh[1] = SLIB_GET_BYTE3(_id);
		}

		sl_int16 getTreeId() const noexcept
		{
			return MIO::readInt16LE(_treeId);
		}

		void setTreeId(sl_uint16 _id) noexcept
		{
			MIO::writeUint16LE(_treeId, _id);
		}

		sl_uint16 getUserId() const noexcept
		{
			return MIO::readUint16LE(_userId);
		}

		void setUserId(sl_uint16 _id) noexcept
		{
			MIO::writeUint16LE(_userId, _id);
		}

		sl_uint16 getMultiplexId() const noexcept
		{
			return MIO::readUint16LE(_multiplexId);
		}

		void setMultiplexId(sl_uint16 _id) noexcept
		{
			MIO::writeUint16LE(_multiplexId, _id);
		}

	private:
		sl_uint8 _serverComponent[4]; // 0xFF, SMB
		sl_uint8 _command;
		sl_uint8 _status[4];
		sl_uint8 _flags[3];
		sl_uint8 _processIdHigh[2];
		sl_uint8 _signature[8];
		sl_uint8 _reserved[2];
		sl_uint8 _treeId[2];
		sl_uint8 _processId[2];
		sl_uint8 _userId[2];
		sl_uint8 _multiplexId[2];

	};

	enum class Smb2Command
	{
		Negotiate = 0
	};

	SLIB_DEFINE_FLAGS(Smb2HeaderFlags, {
		Response = 0x1,
		AsyncCommand = 0x2,
		Chained = 0x4,
		Signing = 0x8,
		DFS = 0x10000000,
		Replay = 0x20000000
	})

	class SLIB_EXPORT Smb2Header
	{
	public:
		sl_bool isSmb2() const noexcept
		{
			return _serverComponent[0] == 0xfe && _serverComponent[1] == 'S' && _serverComponent[2] == 'M' && _serverComponent[3] == 'B';
		}

		void setSmb2() noexcept
		{
			_serverComponent[0] = 0xfe;
			_serverComponent[1] = 'S';
			_serverComponent[2] = 'M';
			_serverComponent[3] = 'B';
		}

		sl_uint16 getHeaderLength() const noexcept
		{
			return MIO::readUint16LE(_headerLength);
		}

		void setHeaderLength(sl_uint16 value) noexcept
		{
			MIO::writeUint16LE(_headerLength, value);
		}

		sl_uint16 getCreditCharge() const noexcept
		{
			return MIO::readUint16LE(_creditCharge);
		}

		void setCreditCharge(sl_uint16 value) noexcept
		{
			MIO::writeUint16LE(_creditCharge, value);
		}

		Smb2Command getCommand() const noexcept
		{
			return (Smb2Command)(MIO::readUint16LE(_command));
		}

		void setCommand(Smb2Command command) noexcept
		{
			MIO::writeUint16LE(_command, (sl_uint16)command);
		}

		sl_uint16 getCreditGranted() const noexcept
		{
			return MIO::readUint16LE(_creditGranted);
		}

		void setCreditGranted(sl_uint16 value) noexcept
		{
			MIO::writeUint16LE(_creditGranted, value);
		}

		Smb2HeaderFlags getFlags() const noexcept
		{
			return MIO::readUint32LE(_flags);
		}

		void setFlags(const Smb2HeaderFlags& flags) noexcept
		{
			MIO::writeUint32LE(_flags, (sl_uint32)(flags.value));
		}

		sl_uint32 getChainOffset() const noexcept
		{
			return MIO::readUint32LE(_chainOffset);
		}

		void setChainOffset(sl_uint32 value) noexcept
		{
			MIO::writeUint32LE(_chainOffset, value);
		}

		sl_uint64 getMessageId() const noexcept
		{
			return MIO::readUint64LE(_messageId);
		}

		void setMessageId(sl_uint64 value) noexcept
		{
			MIO::writeUint64LE(_messageId, value);
		}

		sl_uint32 getProcessId() const noexcept
		{
			return MIO::readUint32LE(_processId);
		}

		void setProcessId(sl_uint32 value) noexcept
		{
			MIO::writeUint32LE(_processId, value);
		}

		sl_uint32 getTreeId() const noexcept
		{
			return MIO::readUint32LE(_treeId);
		}

		void setTreeId(sl_uint32 value) noexcept
		{
			MIO::writeUint32LE(_treeId, value);
		}

		sl_uint64 getSessionId() const noexcept
		{
			return MIO::readUint64LE(_sessionId);
		}

		void setSessionId(sl_uint64 value) noexcept
		{
			MIO::writeUint64LE(_sessionId, value);
		}

		// 8 Bytes
		const sl_uint8* getSignature() const noexcept
		{
			return _signature;
		}

		// 8 Bytes
		sl_uint8* getSignature() noexcept
		{
			return _signature;
		}

	private:
		sl_uint8 _serverComponent[4]; // 0xFE, SMB
		sl_uint8 _headerLength[2];
		sl_uint8 _creditCharge[2];
		sl_uint8 _command[2];
		sl_uint8 _creditGranted[2];
		sl_uint8 _flags[4];
		sl_uint8 _chainOffset[4];
		sl_uint8 _messageId[8];
		sl_uint8 _processId[4];
		sl_uint8 _treeId[4];
		sl_uint8 _sessionId[4];
		sl_uint8 _signature[8];

	};

	SLIB_DEFINE_FLAGS(Smb2SecurityMode, {
		SigningEnabled = 1,
		SigningRequired = 2
	})

	SLIB_DEFINE_FLAGS(Smb2Capabilities, {
		DFS = 0x00000001,
		Leasing = 0x00000002,
		LargeMtu = 0x00000004,
		MultiChannel = 0x00000008,
		PersistentHandles = 0x00000010,
		DirectoryLeasing = 0x00000020,
		Encryption = 0x00000040
	})

	class SLIB_EXPORT Smb2NegotiateResponse
	{
	public:
		sl_uint16 getFixedSize() const noexcept
		{
			return MIO::readUint16LE(_structureSize) & 0xFE;
		}

		sl_bool isDynamicSize() const noexcept
		{
			return *_structureSize & 1;
		}

		void setSize(sl_uint16 fixedSize, sl_bool flagDynamic) noexcept
		{
			MIO::writeUint16LE(_structureSize, (fixedSize & 0xFE) | (flagDynamic ? 1 : 0));
		}

		Smb2SecurityMode getSecurityMode() const noexcept
		{
			return (Smb2SecurityMode)_securityMode;
		}

		void setSecurityMode(Smb2SecurityMode mode) noexcept
		{
			_securityMode = (sl_uint8)(mode.value);
		}

		sl_uint16 getDialect() const noexcept
		{
			return MIO::readUint16LE(_dialect);
		}

		void setDialect(sl_uint16 value) noexcept
		{
			MIO::writeUint16LE(_dialect, value);
		}

		sl_uint16 getContextCount() const noexcept
		{
			return MIO::readUint16LE(_contextCount);
		}

		void setContextCount(sl_uint16 value) noexcept
		{
			MIO::writeUint16LE(_contextCount, value);
		}

		const sl_uint8* getGuid() const noexcept
		{
			return _guid;
		}

		sl_uint8* getGuid() noexcept
		{
			return _guid;
		}

		Smb2Capabilities getCapabilities() const noexcept
		{
			return (Smb2Capabilities)(MIO::readUint32LE(_capabilities));
		}

		void setSecurityMode(Smb2Capabilities caps) noexcept
		{
			MIO::writeUint32LE(_capabilities, (sl_uint32)(caps.value));
		}

		Time getCurrentTime() const noexcept
		{
			return Time::fromWindowsFileTime(MIO::readUint64LE(_currentTime));
		}

		void setCurrentTime(const Time& time) noexcept
		{
			MIO::writeUint64LE(_currentTime, time.toWindowsFileTime());
		}

		Time getBootTime() const noexcept
		{
			return Time::fromWindowsFileTime(MIO::readUint64LE(_bootTime));
		}

		void setBootTime(const Time& time) noexcept
		{
			MIO::writeUint64LE(_bootTime, time.toWindowsFileTime());
		}

		sl_uint16 getBlobOffset() const noexcept
		{
			return MIO::readUint16LE(_blobOffset);
		}

		void setBlobOffset(sl_uint16 value) noexcept
		{
			MIO::writeUint16LE(_blobOffset, value);
		}

		sl_uint16 getBlobLength() const noexcept
		{
			return MIO::readUint16LE(_blobLength);
		}

		void setBlobLength(sl_uint16 value) noexcept
		{
			MIO::writeUint16LE(_blobLength, value);
		}

		sl_uint32 getContextOffset() const noexcept
		{
			return MIO::readUint32LE(_contextOffset);
		}

		void setContextOffset(sl_uint32 value) noexcept
		{
			MIO::writeUint32LE(_contextOffset, value);
		}

	private:
		sl_uint8 _structureSize[2];
		sl_uint8 _securityMode;
		sl_uint8 _reserved;
		sl_uint8 _dialect[2];
		sl_uint8 _contextCount[2];
		sl_uint8 _guid[16];
		sl_uint8 _capabilities[4];
		sl_uint8 _maxTransactionSize[4];
		sl_uint8 _maxReadSize[4];
		sl_uint8 _maxWriteSize[4];
		sl_uint8 _currentTime[8];
		sl_uint8 _bootTime[8];
		sl_uint8 _blobOffset[2]; // offset from SMB header
		sl_uint8 _blobLength[2];
		sl_uint8 _contextOffset[4];

	};

	class SLIB_EXPORT SmbServerParam
	{
	public:
		IPAddress bindAddress;
		sl_uint16 port;

		sl_uint32 maxThreadsCount;
		sl_bool flagStopWindowsService;

		sl_bool flagAutoStart;

	public:
		SmbServerParam();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(SmbServerParam)

	};

	class SLIB_EXPORT SmbServer : public Object
	{
		SLIB_DECLARE_OBJECT

	protected:
		SmbServer();

		~SmbServer();

	public:
		static Ref<SmbServer> create(const SmbServerParam& param);

	public:
		sl_bool start();

		void release();

		sl_bool isReleased();

		sl_bool isRunning();

		const SmbServerParam& getParam();

	protected:
		void _onRunListen();

		void _onRunClient(const Socket& socket);

		sl_bool _onProcessMessage(const Socket& socket, SocketEvent* ev, sl_uint8* msg, sl_uint32 size);

		sl_bool _onProcessSMB(const Socket& socket, SocketEvent* ev, SmbHeader* header, sl_uint32 size);

	protected:
		sl_bool m_flagReleased;
		sl_bool m_flagRunning;

		Socket m_socketListen;
		AtomicRef<Thread> m_threadListen;
		AtomicRef<ThreadPool> m_threadPool;
		
		SmbServerParam m_param;

	};

}

#endif
