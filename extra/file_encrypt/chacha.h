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

#ifndef CHECKHEADER_SLIB_EXTRA_FILE_ENCRYPT_CHACHA
#define CHECKHEADER_SLIB_EXTRA_FILE_ENCRYPT_CHACHA

#include <slib/crypto/chacha.h>
#include <slib/crypto/file_encryption.h>
#include <slib/core/string.h>

/*
				Header Format

	Check Pattern = PBKDF(SHA256(password))
	Main Encryption Key = PBKDF(password) ^ Xor Pattern

	Total Size: 128 Bytes
	_________________________________________________________________
	| Offset |  Size  |                 Content                     |
	|   0    |   12   |   PBKDF Salt for Check-Pattern              |
	|   12   |   4    |   PBKDF Iteration for Check-Pattern         |
	|   16   |   32   |   Check Pattern                             |
	|   48   |   12   |   PBKDF Salt for Main Encryption Key        |
	|   60   |   4    |   PBKDF Iteration for Main Encryption Key   |
	|   64   |   16   |   IV                                        |
	|   80   |   32   |   Xor Pattern                               |
	|   112  |   16   |   Reserved                                  |
	-----------------------------------------------------------------

*/

namespace slib
{

	class SLIB_EXPORT ChaChaFileEncryption : public FileEncryption
	{
	public:
		enum {
			HeaderSize = 128
		};

	public:
		ChaChaFileEncryption();

		ChaChaFileEncryption(const String& password, sl_int32 iterationBitCount = -1);

		~ChaChaFileEncryption();

	public:
		sl_uint32 getHeaderSize() override;

		// outHeader: `HeaderSize` bytes
		void generateHeader(void* outHeader) override;
		void generateHeader(void* outHeader, const void* password, sl_size lenPassword, sl_int32 iterationBitCount = -1) noexcept;

		// header: `HeaderSize` bytes
		sl_bool open(const void* header) override;
		sl_bool open(const void* header, const void* password, sl_size lenPassword, sl_int32 iterationBitCountLimit = -1) noexcept;

		// header: `HeaderSize` bytes
		static sl_bool checkPassword(const void* header, const void* password, sl_size lenPassword) noexcept;
		static sl_bool checkPassword(const void* header, const void* password, sl_size lenPassword, sl_uint32 iterationBitCountLimit) noexcept;

		// header: `HeaderSize` bytes
		static sl_bool changePassword(void* header, const void* oldPassword, sl_size lenOldPassword, const void* newPassword, sl_size lenNewPassword) noexcept;
		static sl_bool changePassword(void* header, const void* oldPassword, sl_size lenOldPassword, const void* newPassword, sl_size lenNewPassword, sl_uint32 iterationBitCountLimit) noexcept;

		void encrypt(sl_uint64 offset, const void* input, void* output, sl_size size) override;
		void decrypt(sl_uint64 offset, const void* input, void* output, sl_size size) override;

		// key: 32 bytes
		void getKey(void* key) noexcept;
		// iv: 16 Bytes
		void getIV(void* iv) noexcept;

	protected:
		String m_defaultPassword;
		sl_int32 m_defaultIterationBitCount;
		ChaCha20_IO m_base;

	};

}

#endif

