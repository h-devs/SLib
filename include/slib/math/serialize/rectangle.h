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

#ifndef CHECKHEADER_SLIB_MATH_SERIALIZE_RECTANGLE
#define CHECKHEADER_SLIB_MATH_SERIALIZE_RECTANGLE

#include "../rectangle.h"

#include "../../data/serialize/io.h"

namespace slib
{

	template <class OUTPUT, class T>
	static sl_bool Serialize(OUTPUT* output, const RectangleT<T>& _in)
	{
		if (!(Serialize(output, _in.left))) {
			return sl_false;
		}
		if (!(Serialize(output, _in.top))) {
			return sl_false;
		}
		if (!(Serialize(output, _in.right))) {
			return sl_false;
		}
		if (!(Serialize(output, _in.bottom))) {
			return sl_false;
		}
		return sl_true;
	}

	template <class INPUT, class T>
	static sl_bool Deserialize(INPUT* input, RectangleT<T>& _out)
	{
		if (!(Deserialize(input, _out.left))) {
			return sl_false;
		}
		if (!(Deserialize(input, _out.top))) {
			return sl_false;
		}
		if (!(Deserialize(input, _out.right))) {
			return sl_false;
		}
		if (!(Deserialize(input, _out.bottom))) {
			return sl_false;
		}
		return sl_true;
	}

}

#endif