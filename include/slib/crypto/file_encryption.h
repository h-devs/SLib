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

#ifndef CHECKHEADER_SLIB_CRYPTO_FILE_ENCRYPTION
#define CHECKHEADER_SLIB_CRYPTO_FILE_ENCRYPTION

#include "definition.h"

#include "../core/object.h"

namespace slib
{

	class SLIB_EXPORT FileEncryption : public Object
	{
		SLIB_DECLARE_OBJECT

	public:
		FileEncryption();

		~FileEncryption();

	public:
		virtual sl_uint32 getHeaderSize() = 0;

		virtual void generateHeader(void* _out) = 0;

		virtual sl_bool open(const void* header) = 0;

		virtual void encrypt(sl_uint64 offset, const void* input, void* output, sl_size size) = 0;

		virtual void decrypt(sl_uint64 offset, const void* input, void* output, sl_size size) = 0;

	};

}

#endif
