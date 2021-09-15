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
 *   THE SOFTWARE.*/


#ifndef CHECKHEADER_SLIB_NETWORK_DCE_RPC
#define CHECKHEADER_SLIB_NETWORK_DCE_RPC

/*
	Distributed Computing Environment / Remote Procedure Calls
*/

#include "definition.h"

#include "../core/flags.h"
#include "../core/mio.h"

namespace slib
{

	enum class DceRpcPacketType
	{
		Request = 0, // Ordinary request
		Ping = 1, // Connectionless is server alive ?
		Response = 2, // Ordinary reply.
		Fault = 3, // Fault in processing of call.
		Working = 4, // Connectionless reply to a ping when server busy.
		Nocall = 5, // Connectionless reply to a ping when server has lost part of clients call.
		Reject = 6, // Refuse a request with a code.
		Ack = 7, // Connectionless client to server code.
		ClCancel = 8, // Connectionless Cancel
		FragAck = 9, // Connectionless fragment ack. Both client and server send.
		CancelAck = 10, // Server ACK to client cancel request.
		Bind = 11, // Bind to interface.
		BindAck = 12, // Server ack of bind.
		BindNack = 13, // Server nack of bind.
		Alter = 14, // Alter auth.
		AlterReply = 15, // Reply to alter auth.
		Auth3 = 16,
		Shutdown = 17, // Server to client request to shutdown.
		CoCancel = 18, // Connection-oriented cancel request.
		Orphaned = 19, // Client telling server it's aborting a partially sent request or telling server to stop sending replies.
		PktRts = 20 // RTS packets used in ncacn_http
	};

	SLIB_DEFINE_FLAGS(DceRpcPacketFlags, {
		FirstFragment = 0x01,
		LastFragment = 0x02,
		CancelPending = 0x04,
		Multiplex = 0x10,
		DidNotExecute = 0x20,
		Maybe = 0x40,
		Object = 0x80
	})

	class SLIB_EXPORT DceRpcHeader
	{
	public:
		sl_uint8 getVersion() const noexcept
		{
			return _version;
		}

		void setVersion(sl_uint8 version) noexcept
		{
			_version = version;
		}

		sl_uint8 getMinorVersion() const noexcept
		{
			return _version;
		}

		void setMinorVersion(sl_uint8 version) noexcept
		{
			_version = version;
		}

		DceRpcPacketType getPacketType() const noexcept
		{
			return (DceRpcPacketType)_packetType;
		}

		void setPacketType(DceRpcPacketType type) noexcept
		{
			_packetType = (sl_uint8)type;
		}

		DceRpcPacketFlags getPacketFlags() const noexcept
		{
			return _packetFlags;
		}

		void setPacketFlags(const DceRpcPacketFlags& flags) noexcept
		{
			_packetFlags = (sl_uint8)(flags.value);
		}

		sl_uint32 getDataRepresentation() const noexcept
		{
			return MIO::readUint32LE(_dataRepresentation);
		}

		void setDataRepresentation(sl_uint32 value) noexcept
		{
			MIO::writeUint32LE(_dataRepresentation, value);
		}

		sl_bool isLittleEndian() const noexcept
		{
			return _dataRepresentation[0] == 0x10;
		}

		void setLittleEndian() noexcept
		{
			_dataRepresentation[0] = 0x10;
		}

		void setBigEndian() noexcept
		{
			_dataRepresentation[0] = 0;
		}

		sl_uint16 getFragmentLength() const noexcept
		{
			return MIO::readUint16LE(_fragLength);
		}

		void setFragmentLength(sl_uint16 len) noexcept
		{
			MIO::writeUint16LE(_fragLength, len);
		}

		sl_uint16 getAuthenticatorLength() const noexcept
		{
			return MIO::readUint16LE(_authLength);
		}

		void setAuthenticatorLength(sl_uint16 len) noexcept
		{
			MIO::writeUint16LE(_authLength, len);
		}

		sl_uint32 getCallId() const noexcept
		{
			return MIO::readUint32LE(_callId);
		}

		void setCallId(sl_uint32 value) noexcept
		{
			MIO::writeUint32LE(_callId, value);
		}

	private:
		sl_uint8 _version;
		sl_uint8 _minorVersion;
		sl_uint8 _packetType;
		sl_uint8 _packetFlags;
		sl_uint8 _dataRepresentation[4];
		sl_uint8 _fragLength[2];
		sl_uint8 _authLength[2];
		sl_uint8 _callId[4];

	};

	enum class DceRpcRequestOperation
	{
		NetWkstaGetInfo = 0,
		NetShareEnumAll = 15,
		NetSrvGetInfo = 21
	};

	class SLIB_EXPORT DceRpcRequestHeader
	{
	public:
		sl_uint32 getAllocHint() const noexcept
		{
			return MIO::readUint32LE(_allocHint);
		}

		void setAllocHint(sl_uint32 value) noexcept
		{
			MIO::writeUint32LE(_allocHint, value);
		}

		sl_uint16 getContextId() const noexcept
		{
			return MIO::readUint16LE(_contextId);
		}

		void setContextId(sl_uint16 value) noexcept
		{
			MIO::writeUint16LE(_contextId, value);
		}

		DceRpcRequestOperation getOperation() const noexcept
		{
			return (DceRpcRequestOperation)(MIO::readUint16LE(_opnum));
		}

		void setOperation(DceRpcRequestOperation op) noexcept
		{
			MIO::writeUint16LE(_opnum, (sl_uint16)op);
		}

	private:
		sl_uint8 _allocHint[4];
		sl_uint8 _contextId[2];
		sl_uint8 _opnum[2];

	};

	class SLIB_EXPORT DceRpcResponseHeader
	{
	public:
		sl_uint32 getAllocHint() const noexcept
		{
			return MIO::readUint32LE(_allocHint);
		}

		void setAllocHint(sl_uint32 value) noexcept
		{
			MIO::writeUint32LE(_allocHint, value);
		}

		sl_uint16 getContextId() const noexcept
		{
			return MIO::readUint16LE(_contextId);
		}

		void setContextId(sl_uint16 value) noexcept
		{
			MIO::writeUint16LE(_contextId, value);
		}

		sl_uint8 getCancelCount() const noexcept
		{
			return _cancelCount;
		}

		void setCancelCount(sl_uint8 value) noexcept
		{
			_cancelCount = value;
		}

	private:
		sl_uint8 _allocHint[4];
		sl_uint8 _contextId[2];
		sl_uint8 _cancelCount;
		sl_uint8 _reserved;

	};

	enum class SRVSVC_PlatformId
	{
		DOS = 300,
		OS2 = 400,
		NT = 500,
		OSF = 600,
		VMS = 700
	};

	SLIB_DEFINE_FLAGS(SRVSVC_ServerType, {
		Workstation = 0x00000001,
		Server = 0x00000002,
		SqlServer = 0x00000004,
		DomainController = 0x00000008,
		BackupController = 0x00000010,
		TimeSource = 0x00000020,
		Apple = 0x00000040,
		Novell = 0x00000080,
		DomainMember = 0x00000100,
		PrintQueueServer = 0x00000200,
		DialinServer = 0x00000400,
		UnixServer = 0x00000800,
		NtWorkstation = 0x00001000,
		WfW = 0x00002000,
		MFPN = 0x00004000,
		NtServer = 0x00008000,
		PotentialBrowser = 0x00010000,
		BackupBrowser = 0x00020000,
		MasterBrowser = 0x00040000,
		DomainMaster = 0x00080000,
		OSF = 0x00100000,
		VMS = 0x00200000,
		Win95Plus = 0x00400000,
		DFS = 0x00800000,
		AlternateXport = 0x20000000,
		LocalListOnly = 0x40000000,
		DomainEnum = 0x80000000
	})

	class SLIB_EXPORT WKSSVC_NetWkstaInfo100
	{
	public:
		SRVSVC_PlatformId getPlatformId() const noexcept
		{
			return (SRVSVC_PlatformId)(MIO::readUint32LE(_platformId));
		}

		void setPlatformId(SRVSVC_PlatformId value) noexcept
		{
			MIO::writeUint32LE(_platformId, (sl_uint32)value);
		}


	private:
		sl_uint8 _platformId[4];

	};

}

#endif
