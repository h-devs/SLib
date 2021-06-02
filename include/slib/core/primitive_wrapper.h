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

#ifndef CHECKHEADER_SLIB_CORE_PRIMITIVE_WRAPPER
#define CHECKHEADER_SLIB_CORE_PRIMITIVE_WRAPPER

#include "common_members.h"
#include "hash.h"

#define SLIB_MEMBERS_OF_PRIMITIVE_WRAPPER(CLASS, TYPE, VALUE) \
public: \
	constexpr CLASS(TYPE _value): VALUE(_value) {} \
	constexpr CLASS(const CLASS& other): VALUE(other.VALUE) {} \
	constexpr operator TYPE() const { return VALUE; } \
	CLASS& operator=(const CLASS& other) noexcept { VALUE = other.VALUE; return *this; } \
	CLASS& operator=(TYPE _value) noexcept { VALUE = _value; return *this; } \
	constexpr sl_bool equals(const CLASS& other) const { return VALUE == other.VALUE; } \
	constexpr sl_bool equals(TYPE _value) const { return VALUE == _value; } \
	constexpr sl_compare_result compare(const CLASS& other) const { return slib::ComparePrimitiveValues(VALUE, other.VALUE); } \
	constexpr sl_compare_result compare(TYPE _value) const { return slib::ComparePrimitiveValues(VALUE, _value); } \
	constexpr sl_size getHashCode() const { return HashPrimitiveValue(VALUE); } \
	SLIB_DEFINE_CLASS_DEFAULT_COMPARE_OPERATORS_CONSTEXPR

#endif
