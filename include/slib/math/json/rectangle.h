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

#ifndef CHECKHEADER_SLIB_MATH_JSON_RECTANGLE
#define CHECKHEADER_SLIB_MATH_JSON_RECTANGLE

#include "../rectangle.h"

#include "../../data/json/core.h"

namespace slib
{

	template <class T>
	static void FromJson(const Json& json, RectangleT<T>& _out)
	{
		if (json.isUndefined()) {
			return;
		}
		ListLocker<Json> list(json.getJsonList());
		if (list.count != 4) {
			return;
		}
		FromJson(list[0], _out.left);
		FromJson(list[1], _out.top);
		FromJson(list[2], _out.right);
		FromJson(list[3], _out.bottom);
	}

	template <class T>
	static Json ToJson(const RectangleT<T>& _in)
	{
		JsonList ret;
		ret.add_NoLock(Json(_in.left));
		ret.add_NoLock(Json(_in.top));
		ret.add_NoLock(Json(_in.right));
		ret.add_NoLock(Json(_in.bottom));
		return ret;
	}

}

#endif
