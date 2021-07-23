/*
 *   Copyright (c) 2008-2018 SLIBIO <https://github.com/SLIBIO>
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

#ifndef CHECKHEADER_SLIB_CRYPTO_TLS
#define CHECKHEADER_SLIB_CRYPTO_TLS

#include "definition.h"

#include "../core/async_stream.h"
#include "../core/memory.h"
#include "../core/hash_map.h"
#include "../core/string.h"

namespace slib
{

	enum class TlsRecordType
	{
		ChangeCipherSpec = 0x14,
		Alert = 0x15,
		Handshake = 0x16,
		Application = 0x17,
		Heartbeat = 0x18
	};

	enum class TlsVersion
	{
		SSL3_0 = SLIB_MAKE_WORD(3, 0),
		TLS1_0 = SLIB_MAKE_WORD(3, 1),
		TLS1_1 = SLIB_MAKE_WORD(3, 2),
		TLS1_2 = SLIB_MAKE_WORD(3, 3),
		TLS1_3 = SLIB_MAKE_WORD(3, 4)
	};

	enum class TlsHandshakeType
	{
		HelloRequest = 0,
		ClientHello = 1,
		ServerHello = 2,
		NewSessionTicket = 4,
		EncryptedExtensions = 8,
		Certificate = 11,
		ServerKeyExchange = 12,
		CertificateRequest = 13,
		ServerHelloDone = 14,
		CertificateVerify = 15,
		ClientKeyExchange = 16,
		Finished = 20
	};

	// http://www.iana.org/assignments/tls-extensiontype-values/tls-extensiontype-values.xhtml
	enum class TlsExtensionType
	{
		ServerName = 0,
		MaxFragmentLength = 1,
		ClientCertificateUrl = 2,
		TrustedCaKeys = 3,
		TruncatedHmac = 4,
		StatusRequest = 5,
		UserMapping = 6,
		ClientAuthz = 7,
		ServerAuthz = 8,
		CertType = 9,
		SupportedGroups = 10,
		EcPointFormats = 11,
		Srp = 12,
		SignatureAlgorithms = 13,
		UseSrtp = 14,
		Heartbeat = 15,
		ApplicationLayerProtocolNegotiation = 16,
		StatusRequest_v2 = 17,
		SignedCertificateTimestamp = 18,
		ClientCertificateType = 19,
		ServerCertificateType = 20,
		Padding = 21,
		EncryptThenMac = 22,
		ExtendedMaster_secret = 23,
		TokenBinding = 24,
		CachedInfo = 25,
		TlsLts = 26,
		CompressCertificate = 27,
		RecordSizeLimit = 28,
		PwdProtect = 29,
		PwdClear = 30,
		PasswordSalt = 31,
		TicketPinning = 32,
		TlsCertWithExternPsk = 33,
		DelegatedCredentials = 34,
		SessionTicket = 35,
		TLMSP = 36,
		TLMSP_Proxying = 37,
		TLMSP_Delegate = 38,
		SupportedEktCiphers = 39,
		Reserved = 40,
		PreSharedKey = 41,
		EarlyData = 42,
		SupportedVersions = 43,
		Cookie = 44,
		PskKeyExchangeModes = 45,
		Certificate_authorities = 47,
		OidFilters = 48,
		PostHandshakeAuth = 49,
		SignatureAlgorithmsCert = 50,
		KeyShare = 51,
		TransparencyInfo = 52,
		ConnectionId = 54,
		ExternalIdHash = 55,
		ExternalSessionId = 56,
		QuicTransportParameters = 57,
		TicketRequest = 58,
		DnssecChain = 59,
		RenegotiationInfo = 65281
	};

	class SLIB_EXPORT TlsRecordHeader
	{
	public:
		TlsRecordType getType() const noexcept
		{
			return (TlsRecordType)_type;
		}

		TlsVersion getVersion() const noexcept
		{
			return (TlsVersion)(SLIB_MAKE_WORD(_majorVersion, _minorVersion));
		}

		sl_uint16 getContentLength() const noexcept
		{
			return SLIB_MAKE_WORD(_length[0], _length[1]);
		}

		void* getContent() const noexcept
		{
			return (sl_uint8*)this + 5;
		}

	private:
		sl_uint8 _type;
		sl_uint8 _majorVersion;
		sl_uint8 _minorVersion;
		sl_uint8 _length[2];
	};

	class SLIB_EXPORT TlsHandshakeProtocolHeader
	{
	public:
		TlsHandshakeType getType() const noexcept
		{
			return (TlsHandshakeType)_type;
		}

		sl_uint16 getContentLength() const noexcept
		{
			return SLIB_MAKE_DWORD(0, _length[0], _length[1], _length[2]);
		}

		void* getContent() const noexcept
		{
			return (sl_uint8*)this + 4;
		}

	private:
		sl_uint8 _type;
		sl_uint8 _length[3];
	};

	class SLIB_EXPORT TlsExtension
	{
	public:
		TlsExtensionType type;
		sl_uint16 length;
		void* data;
	};

	class SLIB_EXPORT TlsClientHelloMessage
	{
	public:
		TlsVersion version;
		void* random; // 32 bytes
		sl_uint8 sessionIdLength;
		void* sessionId;
		sl_uint16 cipherSuitesCount;
		sl_uint16* cipherSuites;
		sl_uint8 compressionMethodsCount;
		sl_uint8* compressionMethods;
		sl_uint16 extentionsSize;
		List<TlsExtension> extensions;

	public:
		// returns passed size (positive), 0: incomplete packet, <0: error
		sl_int32 parse(const void* _data, sl_size size) noexcept;
		
	private:
		sl_int32 _parseExtensions(const void* _data, sl_size size) noexcept;

	};

	// SNI
	class SLIB_EXPORT TlsServerNameIndicationExtension
	{
	public:
		List<StringView> serverNames;

	public:
		sl_bool parse(const void* _data, sl_size size) noexcept;

	};

	class SLIB_EXPORT TlsContextParam
	{
	public:
		Memory certificate; // X.509 Certificate (or chain) in PEM format
		Memory privateKey; // Private Key in PEM format
		HashMap<String, Memory> certificates;
		HashMap<String, Memory> privateKeys;

		sl_bool flagVerify;
		
		String serverName; // At Client, sets the server name indication ClientHello extension to contain the value name

	public:
		TlsContextParam();
		
		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(TlsContextParam)
		
	public:
		void setCertificate(const Memory& certificate);
		
		void setCertificate(const String& serverName, const Memory& certificate);
		
		void setPrivateKey(const Memory& privateKey);
		
		void setPrivateKey(const String& serverName, const Memory& privateKey);
		
		void setCertificateFile(const String& path_PEM);
		
		void setPrivateKeyFile(const String& path_PEM);
		
		void setCertificateFile(const String& serverName, const String& path_PEM);
		
		void setPrivateKeyFile(const String& serverName, const String& path_PEM);
		
	};
	
	class SLIB_EXPORT TlsStreamResult
	{
	public:
		AsyncStream* stream;
		sl_bool flagError;
		
	public:
		TlsStreamResult(AsyncStream*);
		
		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(TlsStreamResult)
		
	};
	
	class TlsContext;
	
	class SLIB_EXPORT TlsStreamParam : public TlsContextParam
	{
	public:
		Ref<TlsContext> context;
		
		sl_uint32 readingBufferSize;
		sl_uint32 writingBufferSize;
		
		sl_bool flagAutoStartHandshake;

		Function<void(TlsStreamResult&)> onHandshake;
		
	public:
		TlsStreamParam();
		
		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(TlsStreamParam)
		
	};
	
	class SLIB_EXPORT TlsConnectStreamParam : public TlsStreamParam
	{
	public:
		TlsConnectStreamParam();
		
		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(TlsConnectStreamParam)
		
	};
	
	class SLIB_EXPORT TlsAcceptStreamParam : public TlsStreamParam
	{
	public:
		TlsAcceptStreamParam();
		
		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(TlsAcceptStreamParam)
		
	};
	
	class SLIB_EXPORT TlsContext : public Object
	{
		SLIB_DECLARE_OBJECT
		
	protected:
		TlsContext();
		
		~TlsContext();
		
	};
	
	class SLIB_EXPORT TlsAsyncStream : public AsyncStream
	{
		SLIB_DECLARE_OBJECT
		
	protected:
		TlsAsyncStream();
		
		~TlsAsyncStream();
		
	public:
		virtual void handshake() = 0;
		
	};
	
}

#endif
