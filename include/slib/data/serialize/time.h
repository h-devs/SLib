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

#ifndef CHECKHEADER_SLIB_DATA_SERIALIZE_TIME
#define CHECKHEADER_SLIB_DATA_SERIALIZE_TIME

#include "primitive.h"

#include "../../core/time.h"

namespace slib
{

	template <class OUTPUT>
	static sl_bool Serialize(OUTPUT* output, const Time& _in)
	{
		sl_uint8 buf[8];
		MIO::writeInt64LE(buf, _in.toInt());
		return SerializeRaw(output, buf, 8);
	}

	template <class INPUT>
	static sl_bool Deserialize(INPUT* input, Time& _out)
	{
		sl_uint8 buf[8];
		if (DeserializeRaw(input, buf, 8)) {
			_out = MIO::readInt64LE(buf);
			return sl_true;
		}
		return sl_false;
	}

}

#endif
