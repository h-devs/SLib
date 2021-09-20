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

#ifndef CHECKHEADER_SLIB_NETWORK_SMB_PACKET
#define CHECKHEADER_SLIB_NETWORK_SMB_PACKET

#include "smb_constant.h"

#include "../core/mio.h"
#include "../core/file.h"

namespace slib
{

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

	SLIB_DEFINE_FLAGS(Smb2HeaderFlags, {
		Response = 0x1,
		AsyncCommand = 0x2,
		Chained = 0x4,
		Signing = 0x8,
		Priority = 0x10,
		DFS = 0x10000000,
		Replay = 0x20000000
	})

	enum class Smb2NegotiateContextType
	{
		PREAUTH_INTEGRITY_CAPABILITIES = 0x0001
	};

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

		SmbStatus getStatus() const noexcept
		{
			return (SmbStatus)(MIO::readUint32LE(_status));
		}

		void setStatus(SmbStatus status) noexcept
		{
			MIO::writeUint32LE(_status, (sl_uint32)status);
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

		SmbStatus getStatus() const noexcept
		{
			return (SmbStatus)(MIO::readUint32LE(_status));
		}

		void setStatus(SmbStatus value) noexcept
		{
			MIO::writeUint32LE(_status, (sl_uint32)value);
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

		sl_uint16 getCreditRequested() const noexcept
		{
			return MIO::readUint16LE(_creditGranted);
		}

		void setCreditRequested(sl_uint16 value) noexcept
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

		// 16 Bytes
		const sl_uint8* getSignature() const noexcept
		{
			return _signature;
		}

		// 16 Bytes
		sl_uint8* getSignature() noexcept
		{
			return _signature;
		}

	private:
		sl_uint8 _serverComponent[4]; // 0xFE, SMB
		sl_uint8 _headerLength[2];
		sl_uint8 _creditCharge[2];
		sl_uint8 _status[4];
		sl_uint8 _command[2];
		sl_uint8 _creditGranted[2]; // credits requested (On Request Header)
		sl_uint8 _flags[4];
		sl_uint8 _chainOffset[4];
		sl_uint8 _messageId[8];
		sl_uint8 _processId[4];
		sl_uint8 _treeId[4];
		sl_uint8 _sessionId[8];
		sl_uint8 _signature[16];

	};

	class SLIB_EXPORT Smb2Message
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

		void setDynamicSize(sl_bool flagDynamic = sl_true) noexcept
		{
			MIO::writeUint16LE(_structureSize, getFixedSize() | (flagDynamic ? 1 : 0));
		}

		sl_bool checkSize(sl_uint16 fixedSize, sl_bool flagDynamic) const noexcept
		{
			if (getFixedSize() == fixedSize) {
				if (flagDynamic) {
					return isDynamicSize();
				} else {
					return !(isDynamicSize());
				}
			}
			return sl_false;
		}

	private:
		sl_uint8 _structureSize[2];

	};

	class SLIB_EXPORT Smb2NegotiateResponseMessage : public Smb2Message
	{
	public:
		Smb2SecurityMode getSecurityMode() const noexcept
		{
			return _securityMode;
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
			return MIO::readUint32LE(_capabilities);
		}

		void setCapabilities(Smb2Capabilities caps) noexcept
		{
			MIO::writeUint32LE(_capabilities, (sl_uint32)(caps.value));
		}

		sl_uint32 getMaxTransationSize() const noexcept
		{
			return MIO::readUint32LE(_maxTransactionSize);
		}

		void setMaxTransationSize(sl_uint32 value) noexcept
		{
			MIO::writeUint32LE(_maxTransactionSize, value);
		}

		sl_uint32 getMaxReadSize() const noexcept
		{
			return MIO::readUint32LE(_maxReadSize);
		}

		void setMaxReadSize(sl_uint32 value) noexcept
		{
			MIO::writeUint32LE(_maxReadSize, value);
		}

		sl_uint32 getMaxWriteSize() const noexcept
		{
			return MIO::readUint32LE(_maxWriteSize);
		}

		void setMaxWriteSize(sl_uint32 value) noexcept
		{
			MIO::writeUint32LE(_maxWriteSize, value);
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
		sl_uint8 _blobOffset[2]; // offset from start of SMB header
		sl_uint8 _blobLength[2];
		sl_uint8 _contextOffset[4]; // offset from start of SMB header (aligned to 16 bytes)

	};

	class SLIB_EXPORT Smb2NegotiateContextHeader
	{
	public:
		Smb2NegotiateContextType getType() const noexcept
		{
			return (Smb2NegotiateContextType)(MIO::readUint16LE(_type));
		}

		void setType(Smb2NegotiateContextType value) noexcept
		{
			MIO::writeUint16LE(_type, (sl_uint16)value);
		}

		sl_uint16 getDataLength() const noexcept
		{
			return MIO::readUint16LE(_dataLength);
		}

		void setDataLength(sl_uint16 value) noexcept
		{
			MIO::writeUint16LE(_dataLength, value);
		}

	private:
		sl_uint8 _type[2];
		sl_uint8 _dataLength[2];
		sl_uint8 _reserved[4];

	};

	class SLIB_EXPORT Smb2SessionSetupResponseMessage : public Smb2Message
	{
	public:
		Smb2SessionFlags getSessionFlags() const noexcept
		{
			return MIO::readUint16LE(_sessionFlags);
		}

		void setSessionFlags(const Smb2SessionFlags& flags) noexcept
		{
			MIO::writeUint16LE(_sessionFlags, (sl_uint16)(flags.value));
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

	private:
		sl_uint8 _sessionFlags[2];
		sl_uint8 _blobOffset[2]; // offset from start of SMB header
		sl_uint8 _blobLength[2];

	};

	class SLIB_EXPORT Smb2TreeConnectRequestMessage : public Smb2Message
	{
	public:
		sl_uint16 getTreeOffset() const noexcept
		{
			return MIO::readUint16LE(_treeOffset);
		}

		void setTreeOffset(sl_uint16 value) noexcept
		{
			MIO::writeUint16LE(_treeOffset, value);
		}

		sl_uint16 getTreeLength() const noexcept
		{
			return MIO::readUint16LE(_treeLength);
		}

		void setTreeLength(sl_uint16 value) noexcept
		{
			MIO::writeUint16LE(_treeLength, value);
		}

	private:
		sl_uint8 _reserved[2];
		sl_uint8 _treeOffset[2]; // offset from start of SMB header
		sl_uint8 _treeLength[2];

	};

	class SLIB_EXPORT Smb2TreeConnectResponseMessage : public Smb2Message
	{
	public:
		Smb2ShareType getShareType() const noexcept
		{
			return (Smb2ShareType)_shareType;
		}

		void setShareType(Smb2ShareType type) noexcept
		{
			_shareType = (sl_uint8)type;
		}

		Smb2ShareFlags getShareFlags() const noexcept
		{
			return MIO::readUint32LE(_shareFlags);
		}

		void setShareFlags(const Smb2ShareFlags& flags) noexcept
		{
			MIO::writeUint32LE(_shareFlags, (sl_uint32)(flags.value));
		}

		Smb2ShareCapabilities getShareCapabilities() const noexcept
		{
			return MIO::readUint32LE(_shareCaps);
		}

		void setShareCapabilities(const Smb2ShareCapabilities& caps) noexcept
		{
			MIO::writeUint32LE(_shareCaps, (sl_uint32)(caps.value));
		}

		SmbAccessMask getAccessMask() const noexcept
		{
			return MIO::readUint32LE(_accessMask);
		}

		void setAccessMask(const SmbAccessMask& mask) noexcept
		{
			MIO::writeUint32LE(_accessMask, (sl_uint32)(mask.value));
		}

	private:
		sl_uint8 _shareType;
		sl_uint8 _reserved;
		sl_uint8 _shareFlags[4];
		sl_uint8 _shareCaps[4];
		sl_uint8 _accessMask[4];

	};

	class SLIB_EXPORT Smb2CreateRequestMessage : public Smb2Message
	{
	public:
		Smb2OplockLevel getOplockLevel() const noexcept
		{
			return (Smb2OplockLevel)_opcodeLevel;
		}

		void setOplockLevel(Smb2OplockLevel value) noexcept
		{
			_opcodeLevel = (sl_uint8)value;
		}

		Smb2ImpersonationLevel getImpersonationLevel() const noexcept
		{
			return (Smb2ImpersonationLevel)(MIO::readUint32LE(_impersonationLevel));
		}

		void setImpersonationLevel(Smb2ImpersonationLevel value) noexcept
		{
			MIO::writeUint32LE(_impersonationLevel, (sl_uint32)value);
		}

		SmbAccessMask getAccessMask() const noexcept
		{
			return MIO::readUint32LE(_accessMask);
		}

		void setAccessMask(const SmbAccessMask& mask) noexcept
		{
			MIO::writeUint32LE(_accessMask, (sl_uint32)(mask.value));
		}

		FileAttributes getFileAttributes() const noexcept
		{
			return MIO::readUint32LE(_fileAttributes);
		}

		void setFileAttributes(const FileAttributes& attrs) noexcept
		{
			MIO::writeUint32LE(_fileAttributes, (sl_uint32)(attrs.value));
		}

		SmbShareAccess getShareAccess() const noexcept
		{
			return MIO::readUint32LE(_shareAccess);
		}

		void setShareAccess(const SmbShareAccess& access) noexcept
		{
			MIO::writeUint32LE(_shareAccess, (sl_uint32)(access.value));
		}

		SmbDisposition getDisposition() const noexcept
		{
			return (SmbDisposition)(MIO::readUint32LE(_disposition));
		}

		void setDisposition(SmbDisposition value) noexcept
		{
			MIO::writeUint32LE(_disposition, (sl_uint32)(value));
		}

		SmbCreateOptions getCreateOptions() const noexcept
		{
			return MIO::readUint32(_createOptions);
		}

		void setShareAccess(const SmbCreateOptions& options) noexcept
		{
			MIO::writeUint32LE(_createOptions, (sl_uint32)(options.value));
		}

		sl_uint16 getFileNameOffset() const noexcept
		{
			return MIO::readUint16LE(_fileNameOffset);
		}

		void setFileNameOffset(sl_uint16 value) noexcept
		{
			MIO::writeUint16LE(_fileNameOffset, value);
		}

		sl_uint16 getFileNameLength() const noexcept
		{
			return MIO::readUint16LE(_fileNameLength);
		}

		void setFileNameLength(sl_uint16 value) noexcept
		{
			MIO::writeUint16LE(_fileNameLength, value);
		}

		sl_uint32 getBlobOffset() const noexcept
		{
			return MIO::readUint32LE(_blobOffset);
		}

		void setBlobOffset(sl_uint32 value) noexcept
		{
			MIO::writeUint32LE(_blobOffset, value);
		}

		sl_uint32 getBlobLength() const noexcept
		{
			return MIO::readUint32LE(_blobLength);
		}

		void setBlobLength(sl_uint32 value) noexcept
		{
			MIO::writeUint32LE(_blobLength, value);
		}

	private:
		sl_uint8 _reserved;
		sl_uint8 _opcodeLevel;
		sl_uint8 _impersonationLevel[4];
		sl_uint8 _createFlags[8];
		sl_uint8 _reserved2[8];
		sl_uint8 _accessMask[4];
		sl_uint8 _fileAttributes[4];
		sl_uint8 _shareAccess[4];
		sl_uint8 _disposition[4];
		sl_uint8 _createOptions[4];
		sl_uint8 _fileNameOffset[2];
		sl_uint8 _fileNameLength[2];
		sl_uint8 _blobOffset[4];
		sl_uint8 _blobLength[4];
		
	};

	class SLIB_EXPORT Smb2CreateResponseMessage : public Smb2Message
	{
	public:
		Smb2OplockLevel getOplockLevel() const noexcept
		{
			return (Smb2OplockLevel)_opcodeLevel;
		}

		void setOplockLevel(Smb2OplockLevel value) noexcept
		{
			_opcodeLevel = (sl_uint8)value;
		}

		SmbCreateAction getAction() const noexcept
		{
			return (SmbCreateAction)(MIO::readUint32LE(_createAction));
		}

		void setAction(SmbCreateAction action) noexcept
		{
			MIO::writeUint32LE(_createAction, (sl_uint32)action);
		}

		Time getCreationTime() const noexcept
		{
			return Time::fromWindowsFileTime(MIO::readUint64LE(_createTime));
		}

		void setCreationTime(const Time& time) noexcept
		{
			MIO::writeUint64LE(_createTime, time.toWindowsFileTime());
		}

		Time getLastAccessTime() const noexcept
		{
			return Time::fromWindowsFileTime(MIO::readUint64LE(_lastAccessTime));
		}

		void setLastAccessTime(const Time& time) noexcept
		{
			MIO::writeUint64LE(_lastAccessTime, time.toWindowsFileTime());
		}

		Time getLastWriteTime() const noexcept
		{
			return Time::fromWindowsFileTime(MIO::readUint64LE(_lastWriteTime));
		}

		void setLastWriteTime(const Time& time) noexcept
		{
			MIO::writeUint64LE(_lastWriteTime, time.toWindowsFileTime());
		}

		Time getLastChangeTime() const noexcept
		{
			return Time::fromWindowsFileTime(MIO::readUint64LE(_lastChangeTime));
		}

		void setLastChangeTime(const Time& time) noexcept
		{
			MIO::writeUint64LE(_lastChangeTime, time.toWindowsFileTime());
		}

		sl_uint64 getAllocationSize() const noexcept
		{
			return MIO::readUint64LE(_allocationSize);
		}

		void setAllocationSize(sl_uint64 value) noexcept
		{
			MIO::writeUint64LE(_allocationSize, value);
		}

		sl_uint64 getEndOfFile() const noexcept
		{
			return MIO::readUint64LE(_endOfFile);
		}

		void setEndOfFile(sl_uint64 value) noexcept
		{
			MIO::writeUint64LE(_endOfFile, value);
		}

		FileAttributes getAttributes() const noexcept
		{
			return MIO::readUint32LE(_attributes);
		}

		void setAttributes(const FileAttributes& attrs) noexcept
		{
			MIO::writeUint32LE(_attributes, (sl_uint32)(attrs.value));
		}

		// 16 bytes
		const sl_uint8* getGuid() const noexcept
		{
			return _guid;
		}

		// 16 bytes
		sl_uint8* getGuid() noexcept
		{
			return _guid;
		}

		sl_uint32 getBlobOffset() const noexcept
		{
			return MIO::readUint32LE(_blobOffset);
		}

		void setBlobOffset(sl_uint32 value) noexcept
		{
			MIO::writeUint32LE(_blobOffset, value);
		}

		sl_uint32 getBlobLength() const noexcept
		{
			return MIO::readUint32LE(_blobLength);
		}

		void setBlobLength(sl_uint32 value) noexcept
		{
			MIO::writeUint32LE(_blobLength, value);
		}

	private:
		sl_uint8 _opcodeLevel;
		sl_uint8 _responseFlags;
		sl_uint8 _createAction[4];
		sl_uint8 _createTime[8];
		sl_uint8 _lastAccessTime[8];
		sl_uint8 _lastWriteTime[8];
		sl_uint8 _lastChangeTime[8];
		sl_uint8 _allocationSize[8];
		sl_uint8 _endOfFile[8];
		sl_uint8 _attributes[4];
		sl_uint8 _reserved[4];
		sl_uint8 _guid[16];
		sl_uint8 _blobOffset[4];
		sl_uint8 _blobLength[4];

	};

	class SLIB_EXPORT Smb2ExtraInfoItemHeader
	{
	public:
		sl_uint32 getChainOffset() const noexcept
		{
			return MIO::readUint32(_chainOffset);
		}

		void setChainOffset(sl_uint32 value) noexcept
		{
			MIO::writeUint32(_chainOffset, value);
		}

		sl_uint16 getTagOffset() const noexcept
		{
			return MIO::readUint16(_tagOffset);
		}

		void setTagOffset(sl_uint16 value) noexcept
		{
			MIO::writeUint16(_tagOffset, value);
		}

		sl_uint32 getTagLength() const noexcept
		{
			return MIO::readUint32(_tagLength);
		}

		void setTagLength(sl_uint32 value) noexcept
		{
			MIO::writeUint32(_tagLength, value);
		}

		sl_uint16 getBlobOffset() const noexcept
		{
			return MIO::readUint16(_blobOffset);
		}

		void setBlobOffset(sl_uint16 value) noexcept
		{
			MIO::writeUint16(_blobOffset, value);
		}

		sl_uint32 getBlobLength() const noexcept
		{
			return MIO::readUint32(_blobLength);
		}

		void setBlobLength(sl_uint32 value) noexcept
		{
			MIO::writeUint32(_blobLength, value);
		}

	private:
		sl_uint8 _chainOffset[4];
		sl_uint8 _tagOffset[2];
		sl_uint8 _tagLength[4];
		sl_uint8 _blobOffset[2];
		sl_uint8 _blobLength[4];

	};

	// Maximal Access
	class SLIB_EXPORT Smb2ExtraInfoItem_MxAcResponse
	{
	public:
		SmbStatus getQueryStatus() const noexcept
		{
			return (SmbStatus)(MIO::readUint32(_queryStatus));
		}

		void setQueryStatus(SmbStatus status) noexcept
		{
			MIO::writeUint32(_queryStatus, (sl_uint32)status);
		}

		SmbAccessMask getAccessMask() const noexcept
		{
			return MIO::readUint32(_accessMask);
		}

		void setAccessMask(const SmbAccessMask& mask) noexcept
		{
			MIO::writeUint32(_accessMask, (sl_uint32)(mask.value));
		}

	public:
		sl_uint8 _queryStatus[4];
		sl_uint8 _accessMask[4];

	};

	// Query On Disk ID
	class SLIB_EXPORT Smb2ExtraInfoItem_QFidResponse
	{
	public:
		// 32 Bytes
		const sl_uint8* getOpaqueFileId() const noexcept
		{
			return _opaqueFileId;
		}

		// 32 Bytes
		sl_uint8* getOpaqueFileId() noexcept
		{
			return _opaqueFileId;
		}

	public:
		sl_uint8 _opaqueFileId[32];

	};

	class SLIB_EXPORT Smb2GetInfoRequestMessage : public Smb2Message
	{
	public:
		Smb2GetInfoClass getClass() const noexcept
		{
			return (Smb2GetInfoClass)_class;
		}

		void setClass(Smb2GetInfoClass value) noexcept
		{
			_class = (sl_uint8)value;
		}

		Smb2GetInfoLevel getLevel() const noexcept
		{
			return (Smb2GetInfoLevel)_level;
		}

		void setLevel(Smb2GetInfoLevel value) noexcept
		{
			_level = (sl_uint8)value;
		}

		sl_uint32 getMaxResponseSize() const noexcept
		{
			return MIO::readUint32LE(_maxResponseSize);
		}

		void setMaxResponseSize(sl_uint32 value) noexcept
		{
			MIO::writeUint32LE(_maxResponseSize, value);
		}

		sl_uint16 getInputOffset() const noexcept
		{
			return MIO::readUint16LE(_inputOffset);
		}

		void setInputOffset(sl_uint16 value) noexcept
		{
			MIO::writeUint16LE(_inputOffset, value);
		}

		sl_uint32 getInputSize() const noexcept
		{
			return MIO::readUint32LE(_inputSize);
		}

		void setInputSize(sl_uint32 value) noexcept
		{
			MIO::writeUint32LE(_inputSize, value);
		}

		sl_uint32 getAdditionalInfo() const noexcept
		{
			return MIO::readUint32LE(_additionalInfo);
		}

		void setAdditionalInfo(sl_uint32 value) noexcept
		{
			MIO::writeUint32LE(_additionalInfo, value);
		}

		sl_uint32 getFlags() const noexcept
		{
			return MIO::readUint32LE(_flags);
		}

		void setFlags(sl_uint32 value) noexcept
		{
			MIO::writeUint32LE(_flags, value);
		}

		// 16 bytes
		const sl_uint8* getGuid() const noexcept
		{
			return _guid;
		}

		// 16 bytes
		sl_uint8* getGuid() noexcept
		{
			return _guid;
		}

	private:
		sl_uint8 _class;
		sl_uint8 _level;
		sl_uint8 _maxResponseSize[4];
		sl_uint8 _inputOffset[2];
		sl_uint8 _reserved[2];
		sl_uint8 _inputSize[4];
		sl_uint8 _additionalInfo[4];
		sl_uint8 _flags[4];
		sl_uint8 _guid[16];

	};

	class SLIB_EXPORT Smb2GetInfoResponseMessage : public Smb2Message
	{
	public:
		sl_uint16 getBlobOffset() const noexcept
		{
			return MIO::readUint16LE(_blobOffset);
		}

		void setBlobOffset(sl_uint16 value) noexcept
		{
			MIO::writeUint16LE(_blobOffset, value);
		}

		sl_uint32 getBlobLength() const noexcept
		{
			return MIO::readUint32LE(_blobLength);
		}

		void setBlobLength(sl_uint32 value) noexcept
		{
			MIO::writeUint32LE(_blobLength, value);
		}

	private:
		sl_uint8 _blobOffset[2];
		sl_uint8 _blobLength[4];

	};

	class SLIB_EXPORT Smb2FileStandardInfo
	{
	public:
		sl_uint64 getAllocationSize() const noexcept
		{
			return MIO::readUint64LE(_allocationSize);
		}

		void setAllocationSize(sl_uint64 value) noexcept
		{
			MIO::writeUint64LE(_allocationSize, value);
		}

		sl_uint64 getEndOfFile() const noexcept
		{
			return MIO::readUint64LE(_endOfFile);
		}

		void setEndOfFile(sl_uint64 value) noexcept
		{
			MIO::writeUint64LE(_endOfFile, value);
		}

		sl_uint32 getLinkCount() const noexcept
		{
			return MIO::readUint32LE(_linkCount);
		}

		void setLinkCount(sl_uint32 value) noexcept
		{
			MIO::writeUint32LE(_linkCount, value);
		}

		sl_bool isDeletePending() const noexcept
		{
			return _flagDeletePending != 0;
		}

		void setDeletePending(sl_bool flag) noexcept
		{
			_flagDeletePending = flag ? 1 : 0;
		}

		sl_bool isDirectory() const noexcept
		{
			return _flagDirectory != 0;
		}

		void setDirectory(sl_bool flag) noexcept
		{
			_flagDirectory = flag ? 1 : 0;
		}

	private:
		sl_uint8 _allocationSize[8];
		sl_uint8 _endOfFile[8];
		sl_uint8 _linkCount[4];
		sl_uint8 _flagDeletePending;
		sl_uint8 _flagDirectory;
		sl_uint8 _reserved[2];

	};

	class SLIB_EXPORT Smb2FileAllInfo
	{
	public:
		Time getCreationTime() const noexcept
		{
			return Time::fromWindowsFileTime(MIO::readUint64LE(_createTime));
		}

		void setCreationTime(const Time& time) noexcept
		{
			MIO::writeUint64LE(_createTime, time.toWindowsFileTime());
		}

		Time getLastAccessTime() const noexcept
		{
			return Time::fromWindowsFileTime(MIO::readUint64LE(_lastAccessTime));
		}

		void setLastAccessTime(const Time& time) noexcept
		{
			MIO::writeUint64LE(_lastAccessTime, time.toWindowsFileTime());
		}

		Time getLastWriteTime() const noexcept
		{
			return Time::fromWindowsFileTime(MIO::readUint64LE(_lastWriteTime));
		}

		void setLastWriteTime(const Time& time) noexcept
		{
			MIO::writeUint64LE(_lastWriteTime, time.toWindowsFileTime());
		}

		Time getLastChangeTime() const noexcept
		{
			return Time::fromWindowsFileTime(MIO::readUint64LE(_lastChangeTime));
		}

		void setLastChangeTime(const Time& time) noexcept
		{
			MIO::writeUint64LE(_lastChangeTime, time.toWindowsFileTime());
		}

		FileAttributes getAttributes() const noexcept
		{
			return MIO::readUint32LE(_attributes);
		}

		void setAttributes(const FileAttributes& attrs) noexcept
		{
			MIO::writeUint32LE(_attributes, (sl_uint32)(attrs.value));
		}

		sl_uint64 getAllocationSize() const noexcept
		{
			return MIO::readUint64LE(_allocationSize);
		}

		void setAllocationSize(sl_uint64 value) noexcept
		{
			MIO::writeUint64LE(_allocationSize, value);
		}

		sl_uint64 getEndOfFile() const noexcept
		{
			return MIO::readUint64LE(_endOfFile);
		}

		void setEndOfFile(sl_uint64 value) noexcept
		{
			MIO::writeUint64LE(_endOfFile, value);
		}

		sl_uint32 getLinkCount() const noexcept
		{
			return MIO::readUint32LE(_linkCount);
		}

		void setLinkCount(sl_uint32 value) noexcept
		{
			MIO::writeUint32LE(_linkCount, value);
		}

		sl_bool isDeletePending() const noexcept
		{
			return _flagDeletePending != 0;
		}

		void setDeletePending(sl_bool flag) noexcept
		{
			_flagDeletePending = flag ? 1 : 0;
		}

		sl_bool isDirectory() const noexcept
		{
			return _flagDirectory != 0;
		}

		void setDirectory(sl_bool flag) noexcept
		{
			_flagDirectory = flag ? 1 : 0;
		}

		sl_uint64 getFileId(sl_uint64 fileId) const noexcept
		{
			return MIO::readUint64LE(_fileId);
		}

		void setFileId(sl_uint64 value) noexcept
		{
			MIO::writeUint64LE(_fileId, value);
		}

		sl_uint32 getExtendedAttributesSize() const noexcept
		{
			return MIO::readUint32LE(_eaSize);
		}

		void setExtendedAttributesSize(sl_uint32 value) noexcept
		{
			MIO::writeUint32LE(_eaSize, value);
		}

		SmbAccessMask getAccessMask() const noexcept
		{
			return (SmbAccessMask)(MIO::readUint32LE(_accessMask));
		}

		void setAccessMask(SmbAccessMask value) noexcept
		{
			MIO::writeUint32LE(_accessMask, (sl_uint32)value);
		}

		sl_uint32 getFileNameLength() const noexcept
		{
			return MIO::readUint32LE(_fileNameLength);
		}

		void setFileNameLength(sl_uint32 value) noexcept
		{
			MIO::writeUint32LE(_fileNameLength, value);
		}

	private:
		sl_uint8 _createTime[8];
		sl_uint8 _lastAccessTime[8];
		sl_uint8 _lastWriteTime[8];
		sl_uint8 _lastChangeTime[8];
		sl_uint8 _attributes[4];
		sl_uint8 _reserved[4];
		sl_uint8 _allocationSize[8];
		sl_uint8 _endOfFile[8];
		sl_uint8 _linkCount[4];
		sl_uint8 _flagDeletePending;
		sl_uint8 _flagDirectory;
		sl_uint8 _reserved2[2];
		sl_uint8 _fileId[8];
		sl_uint8 _eaSize[4];
		sl_uint8 _accessMask[4];
		sl_uint8 _positionInfo[8];
		sl_uint8 _modeInfo[4];
		sl_uint8 _alignInfo[4];
		sl_uint8 _fileNameLength[4];
	};

	class SLIB_EXPORT Smb2FileNetworkOpenInfo
	{
	public:
		Time getCreationTime() const noexcept
		{
			return Time::fromWindowsFileTime(MIO::readUint64LE(_createTime));
		}

		void setCreationTime(const Time& time) noexcept
		{
			MIO::writeUint64LE(_createTime, time.toWindowsFileTime());
		}

		Time getLastAccessTime() const noexcept
		{
			return Time::fromWindowsFileTime(MIO::readUint64LE(_lastAccessTime));
		}

		void setLastAccessTime(const Time& time) noexcept
		{
			MIO::writeUint64LE(_lastAccessTime, time.toWindowsFileTime());
		}

		Time getLastWriteTime() const noexcept
		{
			return Time::fromWindowsFileTime(MIO::readUint64LE(_lastWriteTime));
		}

		void setLastWriteTime(const Time& time) noexcept
		{
			MIO::writeUint64LE(_lastWriteTime, time.toWindowsFileTime());
		}

		Time getLastChangeTime() const noexcept
		{
			return Time::fromWindowsFileTime(MIO::readUint64LE(_lastChangeTime));
		}

		void setLastChangeTime(const Time& time) noexcept
		{
			MIO::writeUint64LE(_lastChangeTime, time.toWindowsFileTime());
		}

		sl_uint64 getAllocationSize() const noexcept
		{
			return MIO::readUint64LE(_allocationSize);
		}

		void setAllocationSize(sl_uint64 value) noexcept
		{
			MIO::writeUint64LE(_allocationSize, value);
		}

		sl_uint64 getEndOfFile() const noexcept
		{
			return MIO::readUint64LE(_endOfFile);
		}

		void setEndOfFile(sl_uint64 value) noexcept
		{
			MIO::writeUint64LE(_endOfFile, value);
		}

		FileAttributes getAttributes() const noexcept
		{
			return MIO::readUint32LE(_attributes);
		}

		void setAttributes(const FileAttributes& attrs) noexcept
		{
			MIO::writeUint32LE(_attributes, (sl_uint32)(attrs.value));
		}

	private:
		sl_uint8 _createTime[8];
		sl_uint8 _lastAccessTime[8];
		sl_uint8 _lastWriteTime[8];
		sl_uint8 _lastChangeTime[8];
		sl_uint8 _allocationSize[8];
		sl_uint8 _endOfFile[8];
		sl_uint8 _attributes[4];
		sl_uint8 _reserved[4];

	};

	class SLIB_EXPORT Smb2FileFsVolumeInformation
	{
	public:
		Time getCreationTime() const noexcept
		{
			return Time::fromWindowsFileTime(MIO::readUint64LE(_createTime));
		}

		void setCreationTime(const Time& time) noexcept
		{
			MIO::writeUint64LE(_createTime, time.toWindowsFileTime());
		}

		sl_uint32 getSerialNumber() const noexcept
		{
			return MIO::readUint32LE(_serialNumber);
		}

		void setSerialNumber(sl_uint32 value) noexcept
		{
			MIO::writeUint32LE(_serialNumber, value);
		}

		sl_uint32 getLabelLength() const noexcept
		{
			return MIO::readUint32LE(_labelLength);
		}

		void setLabelLength(sl_uint32 value) noexcept
		{
			MIO::writeUint32LE(_labelLength, value);
		}

	private:
		sl_uint8 _createTime[8];
		sl_uint8 _serialNumber[4];
		sl_uint8 _labelLength[4];
		sl_uint8 _reserved[2];

	};

	class SLIB_EXPORT Smb2FileFsAttributeInformation
	{
	public:
		SmbFileSystemAttributes getAttributes() const noexcept
		{
			return MIO::readUint32LE(_attrs);
		}

		void setAttributes(const SmbFileSystemAttributes& attrs) noexcept
		{
			MIO::writeUint32LE(_attrs, (sl_uint32)(attrs.value));
		}

		sl_uint32 getMaxNameLength() const noexcept
		{
			return MIO::readUint32LE(_maxNameLength);
		}

		void setMaxNameLength(sl_uint32 value) noexcept
		{
			MIO::writeUint32LE(_maxNameLength, value);
		}

		sl_uint32 getLabelLength() const noexcept
		{
			return MIO::readUint32LE(_labelLength);
		}

		void setLabelLength(sl_uint32 value) noexcept
		{
			MIO::writeUint32LE(_labelLength, value);
		}

	private:
		sl_uint8 _attrs[4];
		sl_uint8 _maxNameLength[4];
		sl_uint8 _labelLength[4];

	};

	struct SLIB_EXPORT Smb2FileObjectIdBuffer
	{
		sl_uint8 objectId[16];
		sl_uint8 birthVolumeId[16];
		sl_uint8 birthObjectId[16];
		sl_uint8 domainId[16];
	};

	class SLIB_EXPORT Smb2ReadRequestMessage : public Smb2Message
	{
	public:
		sl_uint32 getReadLength() const noexcept
		{
			return MIO::readUint32LE(_readLength);
		}

		void setReadLength(sl_uint32 value) noexcept
		{
			MIO::writeUint32LE(_readLength, value);
		}

		sl_uint64 getFileOffset() const noexcept
		{
			return MIO::readUint64LE(_fileOffset);
		}

		void setFileOffset(sl_uint64 value) noexcept
		{
			MIO::writeUint64LE(_fileOffset, value);
		}

		// 16 bytes
		const sl_uint8* getGuid() const noexcept
		{
			return _guid;
		}

		// 16 bytes
		sl_uint8* getGuid() noexcept
		{
			return _guid;
		}

		sl_uint32 getMinCount() const noexcept
		{
			return MIO::readUint32LE(_minCount);
		}

		void setMinCount(sl_uint32 value) noexcept
		{
			MIO::writeUint32LE(_minCount, value);
		}

		sl_uint32 getChannel() const noexcept
		{
			return MIO::readUint32LE(_channel);
		}

		void setChannel(sl_uint32 value) noexcept
		{
			MIO::writeUint32LE(_channel, value);
		}

		sl_uint32 getRemainingBytes() const noexcept
		{
			return MIO::readUint32LE(_remainingBytes);
		}

		void setRemainingBytes(sl_uint32 value) noexcept
		{
			MIO::writeUint32LE(_remainingBytes, value);
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

	private:
		sl_uint8 _reserved[2];
		sl_uint8 _readLength[4];
		sl_uint8 _fileOffset[8];
		sl_uint8 _guid[16];
		sl_uint8 _minCount[4];
		sl_uint8 _channel[4];
		sl_uint8 _remainingBytes[4];
		sl_uint8 _blobOffset[2];
		sl_uint8 _blobLength[2];

	};

	class SLIB_EXPORT Smb2ReadResponseMessage : public Smb2Message
	{
	public:
		sl_uint16 getDataOffset() const noexcept
		{
			return MIO::readUint16LE(_dataOffset);
		}

		void setDataOffset(sl_uint16 value) noexcept
		{
			MIO::writeUint16LE(_dataOffset, value);
		}
		
		sl_uint32 getReadCount() const noexcept
		{
			return MIO::readUint32LE(_readCount);
		}

		void setReadCount(sl_uint32 value) noexcept
		{
			MIO::writeUint32LE(_readCount, value);
		}

		sl_uint32 getReadRemaining() const noexcept
		{
			return MIO::readUint32LE(_readRemaining);
		}

		void setReadRemaining(sl_uint32 value) noexcept
		{
			MIO::writeUint32LE(_readRemaining, value);
		}

	private:
		sl_uint8 _dataOffset[2];
		sl_uint8 _readCount[4];
		sl_uint8 _readRemaining[4];
		sl_uint8 _reserved[4];

	};

	class SLIB_EXPORT Smb2WriteRequestMessage : public Smb2Message
	{
	public:
		sl_uint16 getDataOffset() const noexcept
		{
			return MIO::readUint16LE(_dataOffset);
		}

		void setDataOffset(sl_uint16 value) noexcept
		{
			MIO::writeUint16LE(_dataOffset, value);
		}

		sl_uint32 getWriteLength() const noexcept
		{
			return MIO::readUint32LE(_writeLength);
		}

		void setWriteLength(sl_uint32 value) noexcept
		{
			MIO::writeUint32LE(_writeLength, value);
		}

		sl_uint64 getFileOffset() const noexcept
		{
			return MIO::readUint64LE(_fileOffset);
		}

		void setFileOffset (sl_uint64 value) noexcept
		{
			MIO::writeUint64LE(_fileOffset, value);
		}

		// 16 bytes
		const sl_uint8* getGuid() const noexcept
		{
			return _guid;
		}

		// 16 bytes
		sl_uint8* getGuid() noexcept
		{
			return _guid;
		}

		sl_uint32 getChannel() const noexcept
		{
			return MIO::readUint32LE(_channel);
		}

		void setChannel(sl_uint32 value) noexcept
		{
			MIO::writeUint32LE(_channel, value);
		}

		sl_uint32 getRemainingBytes() const noexcept
		{
			return MIO::readUint32LE(_remainingBytes);
		}

		void setRemainingBytes(sl_uint32 value) noexcept
		{
			MIO::writeUint32LE(_remainingBytes, value);
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

		sl_uint32 getFlags() const noexcept
		{
			return MIO::readUint32LE(_flags);
		}

		void setFlags(sl_uint32 value) noexcept
		{
			MIO::writeUint32LE(_flags, value);
		}

	private:
		sl_uint8 _dataOffset[2];
		sl_uint8 _writeLength[4];
		sl_uint8 _fileOffset[8];
		sl_uint8 _guid[16];
		sl_uint8 _channel[4];
		sl_uint8 _remainingBytes[4];
		sl_uint8 _blobOffset[2];
		sl_uint8 _blobLength[2];
		sl_uint8 _flags[4];

	};

	class SLIB_EXPORT Smb2WriteResponseMessage : public Smb2Message
	{
	public:
		sl_uint32 getWriteCount() const noexcept
		{
			return MIO::readUint32LE(_writeCount);
		}

		void setWriteCount(sl_uint32 value) noexcept
		{
			MIO::writeUint32LE(_writeCount, value);
		}

		sl_uint32 getWriteRemaining() const noexcept
		{
			return MIO::readUint32LE(_writeRemaining);
		}

		void setWriteRemaining(sl_uint32 value) noexcept
		{
			MIO::writeUint32LE(_writeRemaining, value);
		}

		sl_uint16 getChannelInfoOffset() const noexcept
		{
			return MIO::readUint16LE(_channelInfoOffset);
		}

		void setChannelInfoOffset(sl_uint16 value) noexcept
		{
			MIO::writeUint16LE(_channelInfoOffset, value);
		}

		sl_uint16 getChannelInfoLength() const noexcept
		{
			return MIO::readUint16LE(_channelInfoLength);
		}

		void setChannelInfoLength(sl_uint16 value) noexcept
		{
			MIO::writeUint16LE(_channelInfoLength, value);
		}

	private:
		sl_uint8 _reserved[2];
		sl_uint8 _writeCount[4];
		sl_uint8 _writeRemaining[4];
		sl_uint8 _channelInfoOffset[2];
		sl_uint8 _channelInfoLength[2];

	};

	class SLIB_EXPORT Smb2CloseRequestMessage : public Smb2Message
	{
	public:
		sl_uint16 getFlags() const noexcept
		{
			return MIO::readUint16LE(_flags);
		}

		void setFlags(sl_uint16 value) noexcept
		{
			MIO::writeUint16LE(_flags, value);
		}

		// 16 bytes
		const sl_uint8* getGuid() const noexcept
		{
			return _guid;
		}

		// 16 bytes
		sl_uint8* getGuid() noexcept
		{
			return _guid;
		}

	private:
		sl_uint8 _flags[2];
		sl_uint8 _reserved[4];
		sl_uint8 _guid[16];

	};

	class SLIB_EXPORT Smb2CloseResponseMessage : public Smb2Message
	{
	public:
		sl_uint16 getFlags() const noexcept
		{
			return MIO::readUint16LE(_flags);
		}

		void setFlags(sl_uint16 value) noexcept
		{
			MIO::writeUint16LE(_flags, value);
		}

		Time getCreationTime() const noexcept
		{
			return Time::fromWindowsFileTime(MIO::readUint64LE(_createTime));
		}

		void setCreationTime(const Time& time) noexcept
		{
			MIO::writeUint64LE(_createTime, time.toWindowsFileTime());
		}

		Time getLastAccessTime() const noexcept
		{
			return Time::fromWindowsFileTime(MIO::readUint64LE(_lastAccessTime));
		}

		void setLastAccessTime(const Time& time) noexcept
		{
			MIO::writeUint64LE(_lastAccessTime, time.toWindowsFileTime());
		}

		Time getLastWriteTime() const noexcept
		{
			return Time::fromWindowsFileTime(MIO::readUint64LE(_lastWriteTime));
		}

		void setLastWriteTime(const Time& time) noexcept
		{
			MIO::writeUint64LE(_lastWriteTime, time.toWindowsFileTime());
		}

		Time getLastChangeTime() const noexcept
		{
			return Time::fromWindowsFileTime(MIO::readUint64LE(_lastChangeTime));
		}

		void setLastChangeTime(const Time& time) noexcept
		{
			MIO::writeUint64LE(_lastChangeTime, time.toWindowsFileTime());
		}

		sl_uint64 getAllocationSize() const noexcept
		{
			return MIO::readUint64LE(_allocationSize);
		}

		void setAllocationSize(sl_uint64 value) noexcept
		{
			MIO::writeUint64LE(_allocationSize, value);
		}

		sl_uint64 getEndOfFile() const noexcept
		{
			return MIO::readUint64LE(_endOfFile);
		}

		void setEndOfFile(sl_uint64 value) noexcept
		{
			MIO::writeUint64LE(_endOfFile, value);
		}

		FileAttributes getAttributes() const noexcept
		{
			return MIO::readUint32LE(_attributes);
		}

		void setAttributes(const FileAttributes& attrs) noexcept
		{
			MIO::writeUint32LE(_attributes, (sl_uint32)(attrs.value));
		}

	private:
		sl_uint8 _flags[2];
		sl_uint8 _reserved[4];
		sl_uint8 _createTime[8];
		sl_uint8 _lastAccessTime[8];
		sl_uint8 _lastWriteTime[8];
		sl_uint8 _lastChangeTime[8];
		sl_uint8 _allocationSize[8];
		sl_uint8 _endOfFile[8];
		sl_uint8 _attributes[4];

	};

	class SLIB_EXPORT Smb2IoctlRequestMessage : public Smb2Message
	{
	public:
		sl_uint32 getFunction() const noexcept
		{
			return MIO::readUint32LE(_function);
		}

		void setFunction(sl_uint32 value) noexcept
		{
			MIO::writeUint32LE(_function, value);
		}

		// 16 bytes
		const sl_uint8* getGuid() const noexcept
		{
			return _guid;
		}

		// 16 bytes
		sl_uint8* getGuid() noexcept
		{
			return _guid;
		}

		sl_uint32 getDataOffset() const noexcept
		{
			return MIO::readUint32LE(_dataOffset);
		}

		void setDataOffset(sl_uint32 value) noexcept
		{
			MIO::writeUint32LE(_dataOffset, value);
		}

		sl_uint32 getDataLength() const noexcept
		{
			return MIO::readUint32LE(_dataLength);
		}

		void setDataLength(sl_uint32 value) noexcept
		{
			MIO::writeUint32LE(_dataLength, value);
		}

		sl_uint32 getMaxInSize() const noexcept
		{
			return MIO::readUint32LE(_maxInSize);
		}

		void setMaxInSize(sl_uint32 value) noexcept
		{
			MIO::writeUint32LE(_maxInSize, value);
		}

		sl_uint32 getBlobOffset() const noexcept
		{
			return MIO::readUint32LE(_blobOffset);
		}

		void setBlobOffset(sl_uint32 value) noexcept
		{
			MIO::writeUint32LE(_blobOffset, value);
		}

		sl_uint32 getBlobLength() const noexcept
		{
			return MIO::readUint32LE(_blobLength);
		}

		void setBlobLength(sl_uint32 value) noexcept
		{
			MIO::writeUint32LE(_blobLength, value);
		}

		sl_uint32 getMaxOutSize() const noexcept
		{
			return MIO::readUint32LE(_maxOutSize);
		}

		void setMaxOutSize(sl_uint32 value) noexcept
		{
			MIO::writeUint32LE(_maxOutSize, value);
		}

		sl_uint32 getFlags() const noexcept
		{
			return MIO::readUint32LE(_flags);
		}

		void setFlags(sl_uint32 value) noexcept
		{
			MIO::writeUint32LE(_flags, value);
		}

	private:
		sl_uint8 _reserved[2];
		sl_uint8 _function[4];
		sl_uint8 _guid[16];
		sl_uint8 _dataOffset[4];
		sl_uint8 _dataLength[4];
		sl_uint8 _maxInSize[4];
		sl_uint8 _blobOffset[4];
		sl_uint8 _blobLength[4];
		sl_uint8 _maxOutSize[4];
		sl_uint8 _flags[4];
		sl_uint8 _reserved2[4];

	};

	class SLIB_EXPORT Smb2IoctlResponseMessage : public Smb2Message
	{
	public:
		sl_uint32 getFunction() const noexcept
		{
			return MIO::readUint32LE(_function);
		}

		void setFunction(sl_uint32 value) noexcept
		{
			MIO::writeUint32LE(_function, value);
		}

		// 16 bytes
		const sl_uint8* getGuid() const noexcept
		{
			return _guid;
		}

		// 16 bytes
		sl_uint8* getGuid() noexcept
		{
			return _guid;
		}

		sl_uint32 getBlobOffset() const noexcept
		{
			return MIO::readUint32LE(_blobOffset);
		}

		void setBlobOffset(sl_uint32 value) noexcept
		{
			MIO::writeUint32LE(_blobOffset, value);
		}

		sl_uint32 getBlobLength() const noexcept
		{
			return MIO::readUint32LE(_blobLength);
		}

		void setBlobLength(sl_uint32 value) noexcept
		{
			MIO::writeUint32LE(_blobLength, value);
		}

		sl_uint32 getDataOffset() const noexcept
		{
			return MIO::readUint32LE(_dataOffset);
		}

		void setDataOffset(sl_uint32 value) noexcept
		{
			MIO::writeUint32LE(_dataOffset, value);
		}

		sl_uint32 getDataLength() const noexcept
		{
			return MIO::readUint32LE(_dataLength);
		}

		void setDataLength(sl_uint32 value) noexcept
		{
			MIO::writeUint32LE(_dataLength, value);
		}

	private:
		sl_uint8 _reserved[2];
		sl_uint8 _function[4];
		sl_uint8 _guid[16];
		sl_uint8 _blobOffset[4];
		sl_uint8 _blobLength[4];
		sl_uint8 _dataOffset[4];
		sl_uint8 _dataLength[4];
		sl_uint8 _reserved2[8];

	};

	class SLIB_EXPORT Smb2FindRequestMessage : public Smb2Message
	{
	public:
		Smb2FindLevel getLevel() const noexcept
		{
			return (Smb2FindLevel)_level;
		}

		void setLevel(Smb2FindLevel value) noexcept
		{
			_level = (sl_uint8)value;
		}

		Smb2FindFlags getFlags() const noexcept
		{
			return _flags;
		}

		void setFlags(const Smb2FindFlags& flags) noexcept
		{
			_flags = (sl_uint8)(flags.value);
		}

		sl_uint32 getFileIndex() const noexcept
		{
			return MIO::readUint32LE(_fileIndex);
		}

		void setFileIndex(sl_uint32 value) noexcept
		{
			MIO::writeUint32BE(_fileIndex, value);
		}

		// 16 bytes
		const sl_uint8* getGuid() const noexcept
		{
			return _guid;
		}

		// 16 bytes
		sl_uint8* getGuid() noexcept
		{
			return _guid;
		}

		sl_uint16 getSearchPatternOffset() const noexcept
		{
			return MIO::readUint16LE(_searchPatternOffset);
		}

		void setSearchPatternOffset(sl_uint16 value) noexcept
		{
			MIO::writeUint16LE(_searchPatternOffset, value);
		}

		sl_uint16 getSearchPatternLength() const noexcept
		{
			return MIO::readUint16LE(_searchPatternLength);
		}

		void setSearchPatternLength(sl_uint16 value) noexcept
		{
			MIO::writeUint16LE(_searchPatternLength, value);
		}

		sl_uint32 getOutputBufferLength() const noexcept
		{
			return MIO::readUint32LE(_outputBufferLength);
		}

		void setOutputBufferLength(sl_uint32 value) noexcept
		{
			MIO::writeUint32BE(_outputBufferLength, value);
		}

	private:
		sl_uint8 _level;
		sl_uint8 _flags;
		sl_uint8 _fileIndex[4];
		sl_uint8 _guid[16];
		sl_uint8 _searchPatternOffset[2];
		sl_uint8 _searchPatternLength[2];
		sl_uint8 _outputBufferLength[4];

	};

	class SLIB_EXPORT Smb2FindResponseMessage : public Smb2Message
	{
	public:
		sl_uint16 getBlobOffset() const noexcept
		{
			return MIO::readUint16LE(_blobOffset);
		}

		void setBlobOffset(sl_uint16 value) noexcept
		{
			MIO::writeUint16LE(_blobOffset, value);
		}

		sl_uint32 getBlobLength() const noexcept
		{
			return MIO::readUint32LE(_blobLength);
		}

		void setBlobLength(sl_uint32 value) noexcept
		{
			MIO::writeUint32LE(_blobLength, value);
		}

	private:
		sl_uint8 _blobOffset[2];
		sl_uint8 _blobLength[4];

	};

	class SLIB_EXPORT Smb2FindFileIdBothDirectoryInfo
	{
	public:
		sl_uint32 getNextOffset() const noexcept
		{
			return MIO::readUint32LE(_nextOffset);
		}

		void setNextOffset(sl_uint32 value) noexcept
		{
			MIO::writeUint32LE(_nextOffset, value);
		}

		sl_uint32 getFileIndex() const noexcept
		{
			return MIO::readUint32LE(_fileIndex);
		}

		void setFileIndex(sl_uint32 value) noexcept
		{
			MIO::writeUint32LE(_fileIndex, value);
		}

		Time getCreationTime() const noexcept
		{
			return Time::fromWindowsFileTime(MIO::readUint64LE(_createTime));
		}

		void setCreationTime(const Time& time) noexcept
		{
			MIO::writeUint64LE(_createTime, time.toWindowsFileTime());
		}

		Time getLastAccessTime() const noexcept
		{
			return Time::fromWindowsFileTime(MIO::readUint64LE(_lastAccessTime));
		}

		void setLastAccessTime(const Time& time) noexcept
		{
			MIO::writeUint64LE(_lastAccessTime, time.toWindowsFileTime());
		}

		Time getLastWriteTime() const noexcept
		{
			return Time::fromWindowsFileTime(MIO::readUint64LE(_lastWriteTime));
		}

		void setLastWriteTime(const Time& time) noexcept
		{
			MIO::writeUint64LE(_lastWriteTime, time.toWindowsFileTime());
		}

		Time getLastChangeTime() const noexcept
		{
			return Time::fromWindowsFileTime(MIO::readUint64LE(_lastChangeTime));
		}

		void setLastChangeTime(const Time& time) noexcept
		{
			MIO::writeUint64LE(_lastChangeTime, time.toWindowsFileTime());
		}

		sl_uint64 getEndOfFile() const noexcept
		{
			return MIO::readUint64LE(_endOfFile);
		}

		void setEndOfFile(sl_uint64 value) noexcept
		{
			MIO::writeUint64LE(_endOfFile, value);
		}

		sl_uint64 getAllocationSize() const noexcept
		{
			return MIO::readUint64LE(_allocationSize);
		}

		void setAllocationSize(sl_uint64 value) noexcept
		{
			MIO::writeUint64LE(_allocationSize, value);
		}

		FileAttributes getAttributes() const noexcept
		{
			return MIO::readUint32LE(_attributes);
		}

		void setAttributes(const FileAttributes& attrs) noexcept
		{
			MIO::writeUint32LE(_attributes, (sl_uint32)(attrs.value));
		}

		sl_uint32 getFileNameLength() const noexcept
		{
			return MIO::readUint32LE(_fileNameLength);
		}

		void setFileNameLength(sl_uint32 value) noexcept
		{
			MIO::writeUint32LE(_fileNameLength, value);
		}

		sl_uint32 getExtendedAttributeSize() const noexcept
		{
			return MIO::readUint32LE(_eaSize);
		}

		void setExtendedAttributeSize(sl_uint32 value) noexcept
		{
			MIO::writeUint32LE(_eaSize, value);
		}

		sl_uint8 getShortNameLength() const noexcept
		{
			return _shortNameLength;
		}

		void setShortNameLength(sl_uint8 value)
		{
			_shortNameLength = value;
		}

		// 24 Bytes (8.3)
		const sl_uint8* getShortName() const noexcept
		{
			return _shortName;
		}

		// 24 Bytes (8.3)
		sl_uint8* getShortName() noexcept
		{
			return _shortName;
		}

		sl_uint64 getFileId() const noexcept
		{
			return MIO::readUint64LE(_fileId);
		}

		void setFileId(sl_uint64 value)
		{
			MIO::writeUint64LE(_fileId, value);
		}

	private:
		sl_uint8 _nextOffset[4];
		sl_uint8 _fileIndex[4];
		sl_uint8 _createTime[8];
		sl_uint8 _lastAccessTime[8];
		sl_uint8 _lastWriteTime[8];
		sl_uint8 _lastChangeTime[8];
		sl_uint8 _endOfFile[8];
		sl_uint8 _allocationSize[8];
		sl_uint8 _attributes[4];
		sl_uint8 _fileNameLength[4];
		sl_uint8 _eaSize[4];
		sl_uint8 _shortNameLength;
		sl_uint8 _reserved;
		sl_uint8 _shortName[24];
		sl_uint8 _reserved2[2];
		sl_uint8 _fileId[8];

	};

	class SLIB_EXPORT Smb2ErrorResponseMessage : public Smb2Message
	{
	public:
		sl_uint8 getErrorContextCount() const noexcept
		{
			return _errorContextCount;
		}

		void setErrorContextCount(sl_uint8 value) noexcept
		{
			_errorContextCount = value;
		}

		sl_uint32 getByteCount() const noexcept
		{
			return MIO::readUint32LE(_byteCount);
		}

		void setByteCount(sl_uint32 value) noexcept
		{
			MIO::writeUint32BE(_byteCount, value);
		}

		sl_uint8 getErrorData() const noexcept
		{
			return _errorData;
		}

		void setErrorData(sl_uint8 data) noexcept
		{
			_errorData = data;
		}

	private:
		sl_uint8 _errorContextCount;
		sl_uint8 _reserved;
		sl_uint8 _byteCount[4];
		sl_uint8 _errorData;

	};

	class SLIB_EXPORT Smb2EmptyMessage : public Smb2Message
	{
	private:
		sl_uint8 _padding[2];
	};

}

#endif
