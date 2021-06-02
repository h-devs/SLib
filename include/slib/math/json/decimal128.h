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

#ifndef CHECKHEADER_SLIB_MATH_JSON_DECIMAL128
#define CHECKHEADER_SLIB_MATH_JSON_DECIMAL128

#include "../decimal128.h"

#include "../../core/json/core.h"

namespace slib
{

	SLIB_INLINE Json Decimal128::toJson() const noexcept
	{
		JsonMap ret;
		SLIB_STATIC_STRING(numberDecimal, "$numberDecimal");
		if (ret.put_NoLock(numberDecimal, toString())) {
			return ret;
		}
		return sl_null;
	}

	SLIB_INLINE sl_bool Decimal128::setJson(const Json& json) noexcept
	{
		if (json.isString()) {
			return parse(json.getStringParam());
		} else {
			SLIB_STATIC_STRING(numberDecimal, "$numberDecimal");
			return parse(json.getItem(numberDecimal).getString());
		}
		return sl_false;
	}

}

#endif
