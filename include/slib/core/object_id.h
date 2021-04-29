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

#ifndef CHECKHEADER_SLIB_CORE_OBJECT_ID
#define CHECKHEADER_SLIB_CORE_OBJECT_ID

#include "compare.h"
#include "hash.h"
#include "default_members.h"

namespace slib
{

	class SLIB_EXPORT ObjectId
	{
	public:
		sl_uint8 data[12];

	public:
		ObjectId() noexcept;

		ObjectId(sl_null_t) noexcept;

		ObjectId(const StringParam& _id) noexcept;

		ObjectId(const sl_uint8* other) noexcept;

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(ObjectId)

	public:
		sl_bool operator==(const ObjectId& other) const noexcept;

		sl_bool operator!=(const ObjectId& other) const noexcept;

		sl_bool operator>=(const ObjectId& other) const noexcept;

		sl_bool operator>(const ObjectId& other) const noexcept;

		sl_bool operator<=(const ObjectId& other) const noexcept;

		sl_bool operator<(const ObjectId& other) const noexcept;

	public:
		static ObjectId generate() noexcept;

		String toString() const noexcept;

		sl_bool parse(const StringParam& str) noexcept;

		Json toJson() const noexcept;

		sl_bool fromJson(const Json& json) noexcept;

		sl_bool equals(const ObjectId& other) const noexcept;

		sl_compare_result compare(const ObjectId& other) const noexcept;

		sl_size getHashCode() const noexcept;

	};

	template <>
	class Compare<ObjectId>
	{
	public:
		sl_compare_result operator()(const ObjectId &a, const ObjectId &b) const noexcept;
	};

	template <>
	class Equals<ObjectId>
	{
	public:
		sl_bool operator()(const ObjectId &a, const ObjectId &b) const noexcept;
	};

	template <>
	class Hash<ObjectId>
	{
	public:
		sl_size operator()(const ObjectId &a) const noexcept;
	};

}

#endif
