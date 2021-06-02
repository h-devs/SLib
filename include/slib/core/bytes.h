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

#ifndef CHECKHEADER_SLIB_CORE_BYTES
#define CHECKHEADER_SLIB_CORE_BYTES

#include "base.h"
#include "string.h"

namespace slib
{

	template <sl_size N>
	class SLIB_EXPORT Bytes
	{
	public:
		sl_uint8 data[N];

	public:
		Bytes() noexcept {}

		Bytes(sl_null_t) noexcept
		{
			Base::zeroMemory(data, N);
		}

		Bytes(const StringParam& _id) noexcept
		{
			if (!(parse(_id))) {
				Base::zeroMemory(data, N);
			}
		}

		Bytes(const sl_uint8* other) noexcept
		{
			Base::copyMemory(data, other, N);
		}

	public:
		sl_bool isZero() const
		{
			return Base::equalsMemoryZero(data, N);
		}

		sl_bool isNotZero() const
		{
			return !(Base::equalsMemoryZero(data, N));
		}

		void setZero()
		{
			Base::zeroMemory(data, N);
		}

	public:
		sl_bool equals(const Bytes& other) const noexcept
		{
			return Base::equalsMemory(data, other.data, N);
		}

		sl_compare_result compare(const Bytes& other) const noexcept
		{
			return Base::compareMemory(data, other.data, N);
		}

		sl_size getHashCode() const noexcept
		{
			return HashBytes(data, N);
		}

		String toString() const noexcept
		{
			return String::makeHexString(data, N);
		}

		sl_bool parse(const StringParam& _str) noexcept
		{
			if (_str.is16()) {
				StringData16 str(_str);
				if (str.getLength() == (N << 1)) {
					return str.parseHexString(data);
				}
			} else {
				StringData str(_str);
				if (str.getLength() == (N << 1)) {
					return str.parseHexString(data);
				}
			}
			return sl_false;
		}

	public:
		SLIB_DECLARE_CLASS_JSON_SERIALIZE_MEMBERS

	};


	template <sl_size N>
	SLIB_INLINE static sl_bool operator==(const Bytes<N>& a, const Bytes<N>& b) noexcept
	{
		return Base::equalsMemory(a.data, b.data, N);
	}

	template <sl_size N>
	SLIB_INLINE static sl_bool operator!=(const Bytes<N>& a, const Bytes<N>& b) noexcept
	{
		return !(Base::equalsMemory(a.data, b.data, N));
	}

	template <sl_size N>
	SLIB_INLINE static sl_bool operator>=(const Bytes<N>& a, const Bytes<N>& b) noexcept
	{
		return Base::compareMemory(a.data, b.data, N) >= 0;
	}

	template <sl_size N>
	SLIB_INLINE static sl_bool operator>(const Bytes<N>& a, const Bytes<N>& b) noexcept
	{
		return Base::compareMemory(a.data, b.data, N) > 0;
	}

	template <sl_size N>
	SLIB_INLINE static sl_bool operator<=(const Bytes<N>& a, const Bytes<N>& b) noexcept
	{
		return Base::compareMemory(a.data, b.data, N) <= 0;
	}

	template <sl_size N>
	SLIB_INLINE static sl_bool operator<(const Bytes<N>& a, const Bytes<N>& b) noexcept
	{
		return Base::compareMemory(a.data, b.data, N) < 0;
	}

}

#endif