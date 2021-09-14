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

#include "slib/crypto/asn1.h"

#include "slib/core/memory_buffer.h"

namespace slib
{

	sl_size Asn1::getSerializedLengthSize(sl_size value) noexcept
	{
		if (value < 128) {
			return 1;
		} else {
			value >>= 8;
			sl_size count = 1;
			while (value) {
				count++;
				value >>= 8;
			}
			return 1 + count;
		}
		return sl_false;
	}


	sl_size Asn1Body::getSize(const Memory& input) noexcept
	{
		return input.getSize();
	}

	sl_size Asn1Body::getSize(const MemoryData& input) noexcept
	{
		return input.size;
	}

	sl_size Asn1Body::getSize(MemoryBuffer& input) noexcept
	{
		return input.getSize();
	}

	sl_bool Asn1Body::serialize(MemoryBuffer* output, MemoryBuffer& input) noexcept
	{
		output->link(input);
		return sl_true;
	}

}
