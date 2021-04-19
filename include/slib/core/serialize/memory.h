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

#ifndef CHECKHEADER_SLIB_CORE_SERIALIZE_MEMORY
#define CHECKHEADER_SLIB_CORE_SERIALIZE_MEMORY

#include "primitive.h"
#include "variable_length_integer.h"

#include "../memory.h"

namespace slib
{

	template <class OUTPUT>
	static sl_bool Serialize(OUTPUT* output, const Memory& _in)
	{
		sl_size size = _in.getSize();
		if (!(CVLI::serialize(output, size))) {
			return sl_false;
		}
		if (size) {
			return SerializeRaw(output, _in.getData(), size);
		} else {
			return sl_true;
		}
	}

	template <class INPUT>
	static sl_bool Deserialize(INPUT* input, Memory& _out)
	{
		sl_size size;
		if (!(CVLI::deserialize(input, size))) {
			return sl_false;
		}
		if (size) {
			Memory ret = Memory::create(size);
			if (ret.isNull()) {
				return sl_false;
			}
			if (DeserializeRaw(input, ret.getData(), size)) {
				_out = Move(ret);
				return sl_true;
			}
			return sl_false;
		} else {
			_out.setNull();
			return sl_true;
		}
	}

}

#endif
