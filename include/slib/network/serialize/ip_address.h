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

#ifndef CHECKHEADER_SLIB_NETWORK_SERIALIZE_IP_ADDRESS
#define CHECKHEADER_SLIB_NETWORK_SERIALIZE_IP_ADDRESS

#include "../ip_address.h"

#include "../../core/serialize/primitive.h"
#include "../../core/serialize/generic.h"

namespace slib
{

	template <class OUTPUT>
	SLIB_INLINE sl_bool IPv4Address::serialize(OUTPUT* output) const
	{
		return SerializeRaw(output, this, sizeof(IPv4Address));
	}

	template <class INPUT>
	SLIB_INLINE sl_bool IPv4Address::deserialize(INPUT* input)
	{
		return DeserializeRaw(input, this, sizeof(IPv4Address));
	}


	template <class OUTPUT>
	SLIB_INLINE sl_bool IPv6Address::serialize(OUTPUT* output) const
	{
		return SerializeRaw(output, this, sizeof(IPv6Address));
	}

	template <class INPUT>
	SLIB_INLINE sl_bool IPv6Address::deserialize(INPUT* input)
	{
		return DeserializeRaw(input, this, sizeof(IPv6Address));
	}


	template <class OUTPUT>
	SLIB_INLINE sl_bool IPAddress::serialize(OUTPUT* output) const
	{
		if (!(Serialize(output, (sl_uint8)type))) {
			return sl_false;
		}
		if (type == IPAddressType::IPv4) {
			return getIPv4().serialize(output);
		} else if (type == IPAddressType::IPv4) {
			return getIPv6().serialize(output);
		} else {
			return sl_true;
		}
	}

	template <class INPUT>
	SLIB_INLINE sl_bool IPAddress::deserialize(INPUT* input)
	{
		sl_uint8 _type;
		if (!(Deserialize(input, _type))) {
			return sl_false;
		}
		if (_type == (sl_uint8)(IPAddressType::IPv4)) {
			IPv4Address v;
			if (v.deserialize(input)) {
				setIPv4(v);
				return sl_true;
			}
		} else if (_type == (sl_uint8)(IPAddressType::IPv6)) {
			IPv6Address v;
			if (v.deserialize(input)) {
				setIPv6(v);
				return sl_true;
			}
		} else {
			setNone();
			return sl_true;
		}
		return sl_false;
	}

}

#endif
