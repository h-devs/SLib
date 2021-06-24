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

#ifndef CHECKHEADER_SLIB_CORE_COMMON_MEMBERS
#define CHECKHEADER_SLIB_CORE_COMMON_MEMBERS

#include "compare.h"
#include "parse.h"

#define SLIB_DECLARE_CLASS_COMPARE_MEMBERS_NO_OP(CLASS) \
	sl_bool equals(const CLASS& other) const noexcept; \
	sl_compare_result compare(const CLASS& other) const noexcept;

#define SLIB_DECLARE_CLASS_COMPARE_MEMBERS(CLASS) \
	SLIB_DECLARE_CLASS_COMPARE_MEMBERS_NO_OP(CLASS) \
	SLIB_DEFINE_CLASS_DEFAULT_COMPARE_OPERATORS

#define SLIB_DECLARE_CLASS_HASH_MEMBERS \
	sl_size getHashCode() const noexcept;

#define SLIB_DECLARE_CLASS_COMPARE_HASH_MEMBERS_NO_OP(CLASS) \
	SLIB_DECLARE_CLASS_COMPARE_MEMBERS_NO_OP(CLASS) \
	SLIB_DECLARE_CLASS_HASH_MEMBERS

#define SLIB_DECLARE_CLASS_COMPARE_HASH_MEMBERS(CLASS) \
	SLIB_DECLARE_CLASS_COMPARE_MEMBERS(CLASS) \
	SLIB_DECLARE_CLASS_HASH_MEMBERS

#define SLIB_DECLARE_CLASS_JSON_MEMBERS \
	slib::Json toJson() const noexcept; \
	sl_bool setJson(const slib::Json& json) noexcept;

#define SLIB_DECLARE_CLASS_SERIALIZE_MEMBERS \
	template <class OUTPUT> sl_bool serialize(OUTPUT* output) const; \
	template <class INPUT> sl_bool deserialize(INPUT* input);

#define SLIB_DECLARE_CLASS_JSON_SERIALIZE_MEMBERS \
	SLIB_DECLARE_CLASS_JSON_MEMBERS \
	SLIB_DECLARE_CLASS_SERIALIZE_MEMBERS

#define SLIB_DECLARE_CLASS_STRING_MEMBERS(CLASS) \
	slib::String toString() const noexcept; \
	SLIB_DECLARE_CLASS_PARSE_MEMBERS(CLASS)

#define SLIB_DECLARE_CLASS_COMMON_MEMBERS(CLASS) \
	SLIB_DECLARE_CLASS_COMPARE_HASH_MEMBERS(CLASS) \
	SLIB_DECLARE_CLASS_JSON_SERIALIZE_MEMBERS \
	SLIB_DECLARE_CLASS_STRING_MEMBERS(CLASS)

namespace slib
{

	class Json;
	class String;

}

#endif
