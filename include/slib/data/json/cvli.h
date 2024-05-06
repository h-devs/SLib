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

#ifndef CHECKHEADER_SLIB_DATA_JSON_CVLI
#define CHECKHEADER_SLIB_DATA_JSON_CVLI

#include "core.h"

#include "../cvli.h"

namespace slib
{

	template <class T>
	static void FromJson(const Json& json, CVLType<T>& _out)
	{
		FromJson(json, _out.value);
	}

	template <class T>
	static Json ToJson(const CVLType<T>& _in)
	{
		T n = _in.value;
		sl_uint8 n8 = (sl_uint8)n;
		if (n == n8) {
			return n8;
		}
		sl_uint16 n16 = (sl_uint16)n;
		if (n == n16) {
			return n16;
		}
		sl_uint32 n32 = (sl_uint32)n;
		if (n == n32) {
			return n32;
		}
		return n;
	}

}

#endif
