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

#include "slib/network/mac_address.h"

#include "slib/core/string_buffer.h"

namespace slib
{

	SLIB_ALIGN(8) const sl_uint8 MacAddress::_zero[6] = { 0 };
	SLIB_ALIGN(8) const sl_uint8 MacAddress::_broadcast[6] = { 255, 255, 255, 255, 255, 255 };

	MacAddress::MacAddress(const sl_uint8* _m) noexcept
	{
		m[0] = _m[0];
		m[1] = _m[1];
		m[2] = _m[2];
		m[3] = _m[3];
		m[4] = _m[4];
		m[5] = _m[5];
	}

	MacAddress::MacAddress(sl_uint8 m0, sl_uint8 m1, sl_uint8 m2, sl_uint8 m3, sl_uint8 m4, sl_uint8 m5) noexcept
	{
		m[0] = m0;
		m[1] = m1;
		m[2] = m2;
		m[3] = m3;
		m[4] = m4;
		m[5] = m5;
	}

	MacAddress::MacAddress(const StringParam& address) noexcept
	{
		if (!(parse(address))) {
			setZero();
		}
	}

	void MacAddress::setZero() noexcept
	{
		m[0] = 0;
		m[1] = 0;
		m[2] = 0;
		m[3] = 0;
		m[4] = 0;
		m[5] = 0;
	}

	void MacAddress::setBroadcast() noexcept
	{
		m[0] = 255;
		m[1] = 255;
		m[2] = 255;
		m[3] = 255;
		m[4] = 255;
		m[5] = 255;
	}

	void MacAddress::makeMulticast(const IPv4Address& addrMulticast) noexcept
	{
		m[0] = 1;
		m[1] = 0;
		m[2] = 0x5e;
		m[3] = addrMulticast.b & 0x7F;
		m[4] = addrMulticast.c;
		m[5] = addrMulticast.d;
	}

	void MacAddress::makeMulticast(const IPv6Address& addrMulticast) noexcept
	{
		m[0] = 0x33;
		m[1] = 0x33;
		m[2] = addrMulticast.m[12];
		m[3] = addrMulticast.m[13];
		m[4] = addrMulticast.m[14];
		m[5] = addrMulticast.m[15];
	}

	void MacAddress::getBytes(sl_uint8* _m) const noexcept
	{
		_m[0] = m[0];
		_m[1] = m[1];
		_m[2] = m[2];
		_m[3] = m[3];
		_m[4] = m[4];
		_m[5] = m[5];
	}

	void MacAddress::setBytes(const sl_uint8* _m) noexcept
	{
		m[0] = _m[0];
		m[1] = _m[1];
		m[2] = _m[2];
		m[3] = _m[3];
		m[4] = _m[4];
		m[5] = _m[5];
	}

	String MacAddress::toString(sl_char8 sep) const noexcept
	{
		StringBuffer ret;
		for (int i = 0; i < 6; i++) {
			if (i) {
				ret.addStatic(&sep, 1);
			}
			ret.add(String::fromUint32(m[i], 16, 2, sl_true));
		}
		return ret.merge();
	}

	namespace priv
	{
		namespace mac_address
		{
			template <class CT>
			SLIB_INLINE static sl_reg Parse(MacAddress* obj, const CT* sz, sl_size i, sl_size n) noexcept
			{
				int v[6];
				for (int k = 0; k < 6; k++) {
					int t = 0;
					int s = 0;
					for (; i < n; i++) {
						int h = sz[i];
						if (h >= '0' && h <= '9') {
							s = (s << 4) | (h - '0');
							if (s > 255) {
								return SLIB_PARSE_ERROR;
							}
							t++;
						} else if (h >= 'A' && h <= 'F') {
							s = (s << 4) | (h - 'A' + 10);
							if (s > 255) {
								return SLIB_PARSE_ERROR;
							}
							t++;
						} else if (h >= 'a' && h <= 'f') {
							s = (s << 4) | (h - 'a' + 10);
							if (s > 255) {
								return SLIB_PARSE_ERROR;
							}
							t++;
						} else {
							break;
						}
					}
					if (k < 5) {
						if (i >= n || (sz[i] != '-' && sz[i] != ':')) {
							return SLIB_PARSE_ERROR;
						}
						i++;
					}
					if (t == 0) {
						return SLIB_PARSE_ERROR;
					}
					v[k] = s;
				}
				if (obj) {
					obj->m[0] = (sl_uint8)(v[0]);
					obj->m[1] = (sl_uint8)(v[1]);
					obj->m[2] = (sl_uint8)(v[2]);
					obj->m[3] = (sl_uint8)(v[3]);
					obj->m[4] = (sl_uint8)(v[4]);
					obj->m[5] = (sl_uint8)(v[5]);
				}
				return i;
			}
		}
	}

	SLIB_DEFINE_CLASS_PARSE_MEMBERS(MacAddress, priv::mac_address::Parse)

	MacAddress& MacAddress::operator=(const StringParam& address) noexcept
	{
		if (!(parse(address))) {
			setZero();
		}
		return *this;
	}

}
