/*
 *   Copyright (c) 2008-2020 SLIBIO <https://github.com/SLIBIO>
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

#ifndef CHECKHEADER_SLIB_CORE_FLAGS
#define CHECKHEADER_SLIB_CORE_FLAGS

#include "primitive_wrapper.h"

#define SLIB_DEFINE_FLAGS(CLASS, ...) \
	class CLASS \
	{ \
	public: \
		SLIB_DEFINE_PRIMITIVE_WRAPPER_MEMBERS(CLASS, int, value) \
		SLIB_CONSTEXPR CLASS(): value(0) {} \
		CLASS& operator|=(int _value) { value |= _value; return *this; } \
		CLASS& operator&=(int _value) { value &= _value; return *this; } \
	public: \
		enum __VA_ARGS__; \
	};

#endif
