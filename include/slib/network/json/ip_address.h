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

#ifndef CHECKHEADER_SLIB_NETWORK_JSON_IP_ADDRESS
#define CHECKHEADER_SLIB_NETWORK_JSON_IP_ADDRESS

#include "../ip_address.h"

#include "../../core/json/core.h"

namespace slib
{

	SLIB_INLINE Json IPv4Address::toJson() const noexcept
	{
		return toString();
	}

	SLIB_INLINE sl_bool IPv4Address::setJson(const Json& json) noexcept
	{
		return parse(json.getStringParam());
	}


	SLIB_INLINE Json IPv6Address::toJson() const noexcept
	{
		return toString();
	}

	SLIB_INLINE sl_bool IPv6Address::setJson(const Json& json) noexcept
	{
		return parse(json.getStringParam());
	}


	SLIB_INLINE Json IPAddress::toJson() const noexcept
	{
		return toString();
	}

	SLIB_INLINE sl_bool IPAddress::setJson(const Json& json) noexcept
	{
		return parse(json.getStringParam());
	}

}

#endif
