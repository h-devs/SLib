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

#ifndef CHECKHEADER_SLIB_NETWORK_MAC_ADDRESS
#define CHECKHEADER_SLIB_NETWORK_MAC_ADDRESS

#include "ip_address.h"

#include "../core/common_members.h"
#include "../core/hash.h"

namespace slib
{

	class SLIB_EXPORT MacAddress
	{
	public:
		sl_uint8 m[6];
		
	public:
		MacAddress() = default;
		
		MacAddress(const MacAddress& other) = default;
		
		MacAddress(const sl_uint8* m) noexcept;
		
		MacAddress(sl_uint8 m0, sl_uint8 m1, sl_uint8 m2, sl_uint8 m3, sl_uint8 m4, sl_uint8 m5) noexcept;

		MacAddress(sl_uint64 v) noexcept;

		MacAddress(const StringParam& address) noexcept;
		
	public:
		static const MacAddress& zero() noexcept
		{
			return *(reinterpret_cast<MacAddress const*>(&_zero));
		}

		SLIB_CONSTEXPR sl_uint64 getInt() const
		{
			return SLIB_MAKE_QWORD(0, 0, m[0], m[1], m[2], m[3], m[4], m[5]);
		}

		SLIB_CONSTEXPR sl_bool isZero() const
		{
			return !(getInt());
		}
		
		SLIB_CONSTEXPR sl_bool isNotZero() const
		{
			return getInt() != 0;
		}
		
		void setZero() noexcept;
		
		static const MacAddress& getBroadcast() noexcept
		{
			return *(reinterpret_cast<MacAddress const*>(&_broadcast));
		}
		
		SLIB_CONSTEXPR sl_bool isBroadcast() const
		{
			return m[0] == 255 && m[1] == 255 && m[2] == 255 && m[3] == 255 && m[4] == 255 && m[5] == 255;
		}
		
		SLIB_CONSTEXPR sl_bool isNotBroadcast() const
		{
			return m[0] != 255 || m[1] != 255 || m[2] != 255 || m[3] != 255 || m[4] != 255 || m[5] != 255;
		}
		
		void setBroadcast() noexcept;
		
		SLIB_CONSTEXPR sl_bool isMulticast() const
		{
			return (m[0] & 1);
		}
		
		SLIB_CONSTEXPR sl_bool isNotMulticast() const
		{
			return (m[0] & 1) == 0;
		}
		
		void makeMulticast(const IPv4Address& addrMulticast) noexcept;
		
		void makeMulticast(const IPv6Address& addrMulticast) noexcept;
		
		void getBytes(sl_uint8* _m) const noexcept;
		
		void setBytes(const sl_uint8* _m) noexcept;
		
	public:
		SLIB_CONSTEXPR sl_compare_result compare(const MacAddress& other) const
		{
			return ComparePrimitiveValues(getInt(), other.getInt());
		}
		
		SLIB_CONSTEXPR sl_bool equals(const MacAddress& other) const
		{
			return getInt() == other.getInt();
		}

		SLIB_CONSTEXPR sl_size getHashCode() const
		{
			return HashPrimitiveValue(getInt());
		}

		// m0-m1-m2-m3-m4-m5, m0:m1:m2:m3:m4:m5
		String toString(sl_char8 sep = '-') const noexcept;

		SLIB_DEFINE_CLASS_DEFAULT_COMPARE_OPERATORS
		SLIB_DECLARE_CLASS_PARSE_MEMBERS(MacAddress)
		SLIB_DECLARE_CLASS_JSON_SERIALIZE_MEMBERS

	public:
		MacAddress& operator=(const MacAddress& other) = default;
		MacAddress& operator=(const StringParam& address) noexcept;
		
	private:
		static const sl_uint8 _zero[6];
		static const sl_uint8 _broadcast[6];
		
	};

}

#endif
