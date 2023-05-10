/*
*   Copyright (c) 2008-2023 SLIBIO <https://github.com/SLIBIO>
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

#include "chacha.h"

#include <slib/core/mio.h>
#include <slib/math/math.h>
#include <slib/crypto/pbkdf.h>

#define CHECK_LEN_HASH_ITERATION 1001
#define FILE_ENCRYPT_ITERATION_CREATE_DEFAULT 13
#define FILE_ENCRYPT_ITERATION_OPEN_DEFAULT 20

namespace slib
{

	ChaChaFileEncryption::ChaChaFileEncryption(): m_defaultIterationBitCount(-1)
	{
	}

	ChaChaFileEncryption::ChaChaFileEncryption(const String& password, sl_int32 iterationBitCount): m_defaultPassword(password), m_defaultIterationBitCount(iterationBitCount)
	{
	}

	ChaChaFileEncryption::~ChaChaFileEncryption()
	{
	}

	namespace {

		static sl_uint32 GetMainIteration(sl_uint32 code, sl_uint32 len) noexcept
		{
			sl_uint32 n = 1 << (len - 1);
			return n | (code & (n - 1));
		}

		static sl_uint32 GenerateCheckIteration(sl_uint32& code, sl_uint32 len) noexcept
		{
			code = (code & 0xFFFFFFF) | ((len - 11) << 28);
			return GetMainIteration(code, len);
		}

		static sl_uint32 GetCheckIteration(sl_uint32 code, sl_uint32& len) noexcept
		{
			len = (code >> 28) + 11;
			return GetMainIteration(code, len);
		}

		static sl_uint32 GetCheckIteration(sl_uint32 code) noexcept
		{
			sl_uint32 len;
			return GetCheckIteration(code, len);
		}

		static sl_bool CheckPassword(const void* _header, const void* password, sl_size lenPassword, sl_uint32 iterationBitCountLimit, sl_uint32& nIterationBitCount) noexcept
		{
			sl_uint8* header = (sl_uint8*)_header;
			sl_uint8 h[32];
			PBKDF2_HMAC_SHA256::generateKey(header + 48, 12, header, 12, CHECK_LEN_HASH_ITERATION, h, 4);
			sl_uint32 code = MIO::readUint32LE(header + 12) ^ MIO::readUint32LE(h);
			sl_uint32 iter = GetCheckIteration(code, nIterationBitCount);
			if (nIterationBitCount > iterationBitCountLimit) {
				return sl_false;
			}
			SHA256::hash(password, lenPassword, h);
			sl_uint8 c[32];
			PBKDF2_HMAC_SHA256::generateKey(h, 32, header, 12, iter, c, 32);
			return Base::equalsMemory(c, header + 16, 32);
		}

		static sl_bool CheckPassword(const void* header, const void* password, sl_size lenPassword, sl_uint32 iterationBitCountLimit) noexcept
		{
			sl_uint32 nIterationBitCount;
			return CheckPassword(header, password, lenPassword, iterationBitCountLimit, nIterationBitCount);
		}

		// key: 32 Bytes
		static sl_bool GetEncryptionKey(sl_uint8* key, const void* _header, const void* password, sl_size lenPassword, sl_uint32 iterationBitCountLimit, sl_uint32& iteration) noexcept
		{
			sl_uint8* header = (sl_uint8*)_header;
			sl_uint32 nIterationBitCount;
			if (!(CheckPassword(header, password, lenPassword, iterationBitCountLimit, nIterationBitCount))) {
				return sl_false;
			}
			sl_uint32 code = MIO::readUint32LE(header + 60);
			iteration = GetMainIteration(code, nIterationBitCount);
			PBKDF2_HMAC_SHA256::generateKey(password, lenPassword, header + 48, 12, iteration, key, 32);
			for (sl_uint32 i = 0; i < 32; i++) {
				key[i] ^= header[80 + i];
			}
			return sl_true;
		}

		// key: 32 Bytes
		static sl_bool GetEncryptionKey(sl_uint8* key, const void* header, const void* password, sl_size lenPassword, sl_uint32 iterationBitCountLimit) noexcept
		{
			sl_uint32 iteration;
			return GetEncryptionKey(key, header, password, lenPassword, iterationBitCountLimit, iteration);
		}

	}

	sl_uint32 ChaChaFileEncryption::getHeaderSize()
	{
		return HeaderSize;
	}

	void ChaChaFileEncryption::generateHeader(void* header)
	{
		generateHeader(header, m_defaultPassword.getData(), m_defaultPassword.getLength(), m_defaultIterationBitCount);
	}

	void ChaChaFileEncryption::generateHeader(void* _header, const void* password, sl_size lenPassword, sl_int32 _iterationBitCount) noexcept
	{
		sl_uint8* header = (sl_uint8*)_header;
		Math::randomMemory(header, HeaderSize);

		sl_uint32 iterationBitCount;
		if (_iterationBitCount >= 0) {
			iterationBitCount = _iterationBitCount;
			if (iterationBitCount < 11) {
				iterationBitCount = 11;
			}
			if (iterationBitCount > 26) {
				iterationBitCount = 26;
			}
		} else {
			iterationBitCount = FILE_ENCRYPT_ITERATION_CREATE_DEFAULT;
		}
		{
			sl_uint8 h[32];
			sl_uint32 code = MIO::readUint32LE(header + 12);
			sl_uint32 iter = GenerateCheckIteration(code, iterationBitCount);
			PBKDF2_HMAC_SHA256::generateKey(header + 48, 12, header, 12, CHECK_LEN_HASH_ITERATION, h, 4);
			MIO::writeUint32LE(header + 12, code ^ MIO::readUint32LE(h));
			SHA256::hash(password, lenPassword, h);
			PBKDF2_HMAC_SHA256::generateKey(h, 32, header, 12, iter, header + 16, 32);
		}
		{
			sl_uint32 code = MIO::readUint32LE(header + 60);
			sl_uint32 iter = GetMainIteration(code, iterationBitCount);
			sl_uint8 key[32];
			PBKDF2_HMAC_SHA256::generateKey(password, lenPassword, header + 48, 12, iter, key, 32);
			for (sl_uint32 i = 0; i < 32; i++) {
				key[i] ^= header[80 + i];
			}
			m_base.setKey(key);
			m_base.setIV(header + 64);
		}
	}

	sl_bool ChaChaFileEncryption::open(const void* header)
	{
		return open(header, m_defaultPassword.getData(), m_defaultPassword.getLength(), m_defaultIterationBitCount);
	}

	sl_bool ChaChaFileEncryption::open(const void* _header, const void* password, sl_size lenPassword, sl_int32 iterationBitCountLimit) noexcept
	{
		if (iterationBitCountLimit < 0) {
			iterationBitCountLimit = FILE_ENCRYPT_ITERATION_OPEN_DEFAULT;
		}
		sl_uint8* header = (sl_uint8*)_header;
		sl_uint8 key[32];
		if (GetEncryptionKey(key, header, password, lenPassword, iterationBitCountLimit)) {
			m_base.setKey(key);
			m_base.setIV(header + 64);
			return sl_true;
		}
		return sl_false;
	}

	sl_bool ChaChaFileEncryption::checkPassword(const void* header, const void* password, sl_size lenPassword, sl_uint32 iterationBitCountLimit) noexcept
	{
		return CheckPassword(header, password, lenPassword, iterationBitCountLimit);
	}

	sl_bool ChaChaFileEncryption::checkPassword(const void* header, const void* password, sl_size lenPassword) noexcept
	{
		return checkPassword(header, password, lenPassword, FILE_ENCRYPT_ITERATION_OPEN_DEFAULT);
	}

	sl_bool ChaChaFileEncryption::changePassword(void* _header, const void* oldPassword, sl_size lenOldPassword, const void* newPassword, sl_size lenNewPassword, sl_uint32 iterationBitCountLimit) noexcept
	{
		sl_uint8* header = (sl_uint8*)_header;
		sl_uint8 key[32];
		sl_uint32 iteration = 0;
		if (GetEncryptionKey(key, header, oldPassword, lenOldPassword, iterationBitCountLimit, iteration)) {
			sl_uint8 t[32];
			PBKDF2_HMAC_SHA256::generateKey(newPassword, lenNewPassword, header + 48, 12, iteration, t, 32);
			for (sl_uint32 i = 0; i < 32; i++) {
				header[80 + i] = t[i] ^ key[i];
			}
			PBKDF2_HMAC_SHA256::generateKey(header + 48, 12, header, 12, CHECK_LEN_HASH_ITERATION, t, 4);
			sl_uint32 code = MIO::readUint32LE(header + 12) ^ MIO::readUint32LE(t);
			sl_uint32 checkIteration = GetCheckIteration(code);
			SHA256::hash(newPassword, lenNewPassword, t);
			PBKDF2_HMAC_SHA256::generateKey(t, 32, header, 12, checkIteration, header + 16, 32);
			return sl_true;
		}
		return sl_false;
	}

	sl_bool ChaChaFileEncryption::changePassword(void* header, const void* oldPassword, sl_size lenOldPassword, const void* newPassword, sl_size lenNewPassword) noexcept
	{
		return changePassword(header, oldPassword, lenOldPassword, newPassword, lenNewPassword, FILE_ENCRYPT_ITERATION_OPEN_DEFAULT);
	}

	void ChaChaFileEncryption::encrypt(sl_uint64 offset, const void* input, void* output, sl_size size)
	{
		return m_base.encrypt(offset, input, output, size);
	}

	void ChaChaFileEncryption::decrypt(sl_uint64 offset, const void* input, void* output, sl_size size)
	{
		return m_base.decrypt(offset, input, output, size);
	}

	void ChaChaFileEncryption::getKey(void* key) noexcept
	{
		m_base.getKey(key);
	}

	void ChaChaFileEncryption::getIV(void* iv) noexcept
	{
		m_base.getIV(iv);
	}

}
