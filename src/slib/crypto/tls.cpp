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

#include "slib/crypto/tls.h"

#include "slib/core/file.h"
#include "slib/core/serialize.h"

namespace slib
{

	/*
		majorVersion				1 byte
		minorVersion				1 byte
		random						32 bytes
		session_id_length			1 byte (byte unit)
		session_id					variable
		cipher_suites_size			2 bytes (byte unit)
		cipher_suites				variable
		compression_methods_size	1 byte (byte unit)
		compression_methods			variable
		----------------------------------------
		extensions_size				2 bytes (byte unit)
		extension1
			extension_type			2 bytes
			length					2 bytes (byte unit)
			data					variable
		extension2
		...
		extension(n)
	*/
	sl_int32 TlsClientHelloMessage::parse(const void* data, sl_size size) noexcept
	{
		if (size < 35) {
			return 0;
		}
		SerializeBuffer buf(data, size);
		sl_uint16 _version;
		if (!(buf.readUint16BE(_version))) {
			return 0;
		}
		version = (TlsVersion)_version;
		random = buf.current;
		if (!(buf.skip(32))) {
			return 0;
		}
		if (!(buf.readUint8(sessionIdLength))) {
			return 0;
		}
		if (sessionIdLength > 32) {
			return -1;
		}
		sessionId = buf.current;
		if (!(buf.skip(sessionIdLength))) {
			return 0;
		}
		sl_uint16 cipherSuitesSize;
		if (!(buf.readUint16BE(cipherSuitesSize))) {
			return 0;
		}
		if (cipherSuitesSize & 1) {
			return -1;
		}
		cipherSuiteCount = cipherSuitesSize >> 1;
		cipherSuites = (sl_uint16*)(buf.current);
		if (!(buf.skip(cipherSuitesSize))) {
			return 0;
		}
		if (!(buf.readUint8(compressionMethodCount))) {
			return 0;
		}
		compressionMethods = buf.current;
		if (!(buf.skip(compressionMethodCount))) {
			return 0;
		}
		if (buf.current == buf.end) {
			return (sl_int32)(buf.current - buf.begin);
		}
		if (!(buf.readUint16BE(extentionsSize))) {
			return 0;
		}
		if (buf.current + extentionsSize <= buf.end) {
			sl_int32 ret = _parseExtensions(buf.current, extentionsSize);
			if (ret > 0) {
				return (sl_int32)(buf.current - buf.begin) + ret;
			} else {
				return ret;
			}
		} else {
			sl_int32 ret = _parseExtensions(buf.current, buf.end - buf.current);
			if (ret >= 0) {
				return 0;
			} else {
				return ret;
			}
		}
	}

	sl_int32 TlsClientHelloMessage::_parseExtensions(const void* data, sl_size size) noexcept
	{
		SerializeBuffer buf(data, size);
		while (buf.current < buf.end) {
			TlsExtension extension;
			sl_uint16 type;
			if (!(buf.readUint16BE(type))) {
				return 0;
			}
			extension.type = (TlsExtensionType)type;
			if (!(buf.readUint16BE(extension.length))) {
				return 0;
			}
			extension.data = buf.current;
			if (!(buf.skip(extension.length))) {
				return 0;
			}
			extensions.add_NoLock(extension);
		}
		return (sl_int32)(buf.current - buf.begin);
	}

	sl_bool TlsServerNameIndicationExtension::parse(const void* data, sl_size size) noexcept
	{
		SerializeBuffer buf(data, size);
		sl_uint16 n;
		if (!(buf.readUint16BE(n))) {
			return sl_false;
		}
		if (buf.current + n > buf.end) {
			return sl_false;
		}
		while (buf.current < buf.end) {
			sl_uint8 type;
			if (!(buf.readUint8(type))) {
				return sl_false;
			}
			sl_uint16 len;
			if (!(buf.readUint16BE(len))) {
				return sl_false;
			}
			char* name = (char*)(buf.current);
			if (!(buf.skip(len))) {
				return sl_false;
			}
			serverNames.add_NoLock(StringView(name, len));
		}
		return sl_true;
	}
	

	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(TlsContextParam)

	TlsContextParam::TlsContextParam(): flagVerify(sl_false)
	{
	}
	
	void TlsContextParam::setCertificate(const Memory& _certificate)
	{
		certificate = _certificate;
	}
	
	void TlsContextParam::setCertificate(const String& serverName, const Memory& certificate)
	{
		certificates.put(serverName, certificate);
	}
	
	void TlsContextParam::setPrivateKey(const Memory& _privateKey)
	{
		privateKey = _privateKey;
	}
		
	void TlsContextParam::setPrivateKey(const String& serverName, const Memory& privateKey)
	{
		privateKeys.put(serverName, privateKey);
	}
	
	void TlsContextParam::setCertificateFile(const String& path_PEM)
	{
		certificate = File::readAllBytes(path_PEM);
	}
	
	void TlsContextParam::setCertificateFile(const String& serverName, const String& path_PEM)
	{
		setCertificate(serverName, File::readAllBytes(path_PEM));
	}
	
	void TlsContextParam::setPrivateKeyFile(const String& path_PEM)
	{
		privateKey = File::readAllBytes(path_PEM);
	}
	
	void TlsContextParam::setPrivateKeyFile(const String& serverName, const String& path_PEM)
	{
		setPrivateKey(serverName, File::readAllBytes(path_PEM));
	}
	

	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(TlsStreamResult)
	
	TlsStreamResult::TlsStreamResult(AsyncStream* _stream): stream(_stream), flagError(sl_true)
	{
	}
	

	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(TlsStreamParam)
	
	TlsStreamParam::TlsStreamParam():
		readingBufferSize(0x40000),
		writingBufferSize(0x40000),
		flagAutoStartHandshake(sl_true)
	{
	}
	

	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(TlsConnectStreamParam)

	TlsConnectStreamParam::TlsConnectStreamParam()
	{
		readingBufferSize = 0x100000;
		writingBufferSize = 0x20000;
	}


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(TlsAcceptStreamParam)
	
	TlsAcceptStreamParam::TlsAcceptStreamParam()
	{
		readingBufferSize = 0x10000;
		writingBufferSize = 0x80000;
	}
	

	SLIB_DEFINE_OBJECT(TlsContext, Object)
	
	TlsContext::TlsContext()
	{
	}
	
	TlsContext::~TlsContext()
	{
	}
	
	
	SLIB_DEFINE_OBJECT(TlsAsyncStream, AsyncStream)
	
	TlsAsyncStream::TlsAsyncStream()
	{
	}

	TlsAsyncStream::~TlsAsyncStream()
	{
	}

}
