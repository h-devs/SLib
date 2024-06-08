/*
 *   Copyright (c) 2008-2024 SLIBIO <https://github.com/SLIBIO>
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

#ifndef CHECKHEADER_SLIB_DATA_SERIALIZE_PAIR
#define CHECKHEADER_SLIB_DATA_SERIALIZE_PAIR

#include "io.h"

#include "../../core/pair.h"

namespace slib
{

	template <class OUTPUT, class T1, class T2>
	static sl_bool Serialize(OUTPUT* output, const Pair<T1, T2>& _in)
	{
		if (!(Serialize(output, _in.first))) {
			return sl_false;
		}
		return Serialize(output, _in.second);
	}

	template <class INPUT, class T1, class T2>
	static sl_bool Deserialize(INPUT* input, Pair<T1, T2>& _out)
	{
		if (!(Deserialize(input, _out.first))) {
			return sl_false;
		}
		return Deserialize(input, _out.second);
	}

}

#endif
