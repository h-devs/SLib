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

#ifndef CHECKHEADER_SLIB_MATH_JSON_VECTOR
#define CHECKHEADER_SLIB_MATH_JSON_VECTOR

#include "../vector.h"

#include "../../data/json/core.h"

namespace slib
{

	template <sl_uint32 N, class T, class FT>
	static void FromJson(const Json& json, VectorT<N, T, FT>& _out)
	{
		if (json.isUndefined()) {
			return;
		}
		ListElements<Json> list(json.getJsonList());
		if (list.count != N) {
			return;
		}
		for (sl_uint32 i = 0; i < N; i++) {
			FromJson(list[i], _out.m[i]);
		}
	}

	template <sl_uint32 N, class T, class FT>
	static Json ToJson(const VectorT<N, T, FT>& _in)
	{
		JsonList ret;
		for (sl_uint32 i = 0; i < N; i++) {
			ret.add_NoLock(Json(_in.m[i]));
		}
		return ret;
	}

}

#endif
