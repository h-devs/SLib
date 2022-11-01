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

#ifndef CHECKHEADER_SLIB_CORE_JSON_BYTES
#define CHECKHEADER_SLIB_CORE_JSON_BYTES

#include "core.h"

namespace slib
{

	template <sl_size N>
	Json Bytes<N>::toJson() const noexcept
	{
		if (isZero()) {
			return sl_null;
		} else {
			return toString();
		}
	}

	template <sl_size N>
	sl_bool Bytes<N>::setJson(const Json& json) noexcept
	{
		if (json.isUndefined()) {
			return sl_false;
		}
		if (json.isNull()) {
			setZero();
			return sl_true;
		}
		if (json.isStringType()) {
			return parse(json.getStringParam());
		} else if (json.isMemory()) {
			Memory mem = json.getMemory();
			if (mem.getSize() == N) {
				Base::copyMemory(data, mem.getData(), N);
				return sl_true;
			}
		}
		return sl_false;
	}

}

#endif
