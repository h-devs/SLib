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

#ifndef CHECKHEADER_SLIB_NETWORK_NTLM
#define CHECKHEADER_SLIB_NETWORK_NTLM

#include "definition.h"

#include "../core/flags.h"
#include "../core/mio.h"
#include "../core/memory_buffer.h"

namespace slib
{

	enum class NtlmMessageType
	{
		Challenge = 2
	};

	SLIB_DEFINE_FLAGS(NtlmNegotiateFlags, {
		NegotiateUnicode = 0x00000001,
		NegotiateOEM = 0x00000002,
		RequestTarget = 0x00000004,
		NegotiateSign = 0x00000010, // Message integrity
		NegotiateSeal = 0x00000020, // Message confidentiality
		NegotiateDatagram = 0x00000040,
		NegotiateLanManagerKey = 0x00000080,
		NegotiateNetware = 0x00000100,
		NegotiateNTLM = 0x00000200,
		NegotiateNtOnly = 0x00000400,
		Anonymous = 0x00000800,
		NegotiateOEMDomainSupplied = 0x00001000,
		NegotiateOEMWorkstationSupplied = 0x00002000,
		NegotiateThisIsLocalCall = 0x00004000,
		NegotiateAlwaysSign = 0x00008000,
		TargetTypeDomain = 0x00010000,
		TargetTypeServer = 0x00020000,
		TargetTypeShare = 0x00040000,
		NegotiateExtenedSecurity = 0x00080000,
		NegotiateIdentify = 0x00100000,
		RequestNonNtSessionkey = 0x00400000,
		NegotiateTargetInfo = 0x00800000,
		NegotiateVersion = 0x02000000,
		Negotiate128 = 0x20000000, // 128-bit encryption
		NegotiateKeyExchange = 0x40000000,
		Negotiate56 = 0x80000000
	})

	class SLIB_EXPORT NtlmBlobDesc
	{
	public:
		sl_uint16 getLength() const noexcept
		{
			return MIO::readUint16LE(_length);
		}

		void setLength(sl_uint16 length) noexcept
		{
			MIO::writeUint16LE(_length, length);
		}

		sl_uint16 getMaxLength() const noexcept
		{
			return MIO::readUint16LE(_maxLength);
		}

		void setMaxLength(sl_uint16 length) noexcept
		{
			MIO::writeUint16LE(_maxLength, length);
		}

		void setLengthAndMaxLength(sl_uint16 length) noexcept
		{
			MIO::writeUint16LE(_length, length);
			MIO::writeUint16LE(_maxLength, length);
		}

		sl_uint32 getOffset() const noexcept
		{
			return MIO::readUint32LE(_offset);
		}

		void setOffset(sl_uint32 offset) noexcept
		{
			MIO::writeUint32LE(_offset, offset);
		}

	private:
		sl_uint8 _length[2];
		sl_uint8 _maxLength[2];
		sl_uint8 _offset[4];
	};

	class SLIB_EXPORT NtlmChallengeHeader
	{
	public:
		// 8 Bytes
		const char* getId() const noexcept
		{
			return _id;
		}

		// 8 Bytes
		char* getId() noexcept
		{
			return _id;
		}

		NtlmMessageType getMessageType() const noexcept
		{
			return (NtlmMessageType)(MIO::readUint32LE(_messageType));
		}

		void setMessageType(NtlmMessageType type) noexcept
		{
			MIO::writeUint32LE(_messageType, (sl_uint32)type);
		}

		NtlmBlobDesc* getTargetName() const noexcept
		{
			return (NtlmBlobDesc*)_targetName;
		}

		NtlmNegotiateFlags getNegotiateFlags() const noexcept
		{
			return (NtlmNegotiateFlags)(MIO::readUint32LE(_negotiateFlags));
		}

		void setNegotiateFlags(const NtlmNegotiateFlags& flags) noexcept
		{
			MIO::writeUint32LE(_negotiateFlags, (sl_uint32)(flags.value));
		}

		// 8 bytes
		const sl_uint8* getServerChallenge() const noexcept
		{
			return _serverChallenge;
		}

		// 8 bytes
		sl_uint8* getServerChallenge() noexcept
		{
			return _serverChallenge;
		}

		NtlmBlobDesc* getTargetInfo() const noexcept
		{
			return (NtlmBlobDesc*)_targetInfo;
		}

		sl_uint8 getMajorVersion() const noexcept
		{
			return _majorVersion;
		}

		void setMajorVersion(sl_uint8 version) noexcept
		{
			_majorVersion = version;
		}

		sl_uint8 getMinorVersion() const noexcept
		{
			return _minorVersion;
		}

		void setMinorVersion(sl_uint8 version) noexcept
		{
			_minorVersion = version;
		}

		sl_uint16 getBuildNumber() const noexcept
		{
			return MIO::readUint16LE(_buildNumber);
		}

		void setBuildNumber(sl_uint16 value) noexcept
		{
			MIO::writeUint16LE(_buildNumber, value);
		}

		sl_uint8 getNtlmCurrentRevision() const noexcept
		{
			return _ntlmCurrentRevision;
		}

		void setNtlmCurrentRevision(sl_uint8 value) noexcept
		{
			_ntlmCurrentRevision = value;
		}

	public:
		char _id[8]; // "NTLMSSP"
		sl_uint8 _messageType[4]; // 2: NTLMSSP_CHALLENGE
		sl_uint8 _targetName[8];
		sl_uint8 _negotiateFlags[4];
		sl_uint8 _serverChallenge[8];
		sl_uint8 _reserved[8];
		sl_uint8 _targetInfo[8];
		sl_uint8 _majorVersion;
		sl_uint8 _minorVersion;
		sl_uint8 _buildNumber[2];
		sl_uint8 _reserved2[3];
		sl_uint8 _ntlmCurrentRevision;

	};

	enum class NtlmTargetInfoItemType
	{
		EndOfList = 0,
		NetBIOS_ComputerName = 1,
		NetBIOS_DomainName = 2,
		DNS_ComputerName = 3,
		DNS_DomainName = 4,
		Timestamp = 7
	};

	class SLIB_EXPORT NtlmTargetInfo
	{
	public:
		void addItem(NtlmTargetInfoItemType type, const void* data, sl_size size) noexcept;
		
		void addItem(NtlmTargetInfoItemType type, const StringParam& str) noexcept;

		void addTimestamp() noexcept;

		Memory end() noexcept;

	private:
		MemoryBuffer m_buf;

	};

}

#endif
