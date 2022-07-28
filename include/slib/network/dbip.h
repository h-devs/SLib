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

#ifndef CHECKHEADER_SLIB_NETWORK_DBIP
#define CHECKHEADER_SLIB_NETWORK_DBIP

/*************************************************
	DB-IP

Get Country code from ip address using the database updated from https://db-ip.com/db/download/country

For example,

   211.106.66.5 => CN

*************************************************/

#include "ip_address.h"

#include "../core/list.h"

namespace slib
{

	class DbIp
	{
	public:
		struct IPv4Item
		{
			sl_uint32 start;
			sl_uint32 end;
			char code[4];
		};

		struct IPv6Item
		{
			IPv6Address start;
			IPv6Address end;
			char code[4];
		};

	public:
		DbIp();

		~DbIp();

	public:
		// Not thread safe
		sl_bool parse(const void* csv, sl_size size);

		// Not thread safe
		sl_bool parseFile(const StringParam& pathToCSVFile);

		void clearAll();

	public:
		// `depth`: reverse loopup level for overlapped case. 0 means exact match
		const char* getCountryCode(const IPv4Address& ipv4, sl_size depth = 1);
		
		const char* getCountryCode(const IPv6Address& ipv6, sl_size depth = 1);

		List<IPv4Item> getIPv4Items(const StringParam& code);

		List<IPv6Item> getIPv6Items(const StringParam& code);

	private:
		List<IPv4Item> m_listIPv4;
		IPv4Item* m_ipv4;
		sl_uint32 m_countIPv4;

		List<IPv6Item> m_listIPv6;
		IPv6Item* m_ipv6;
		sl_uint32 m_countIPv6;
		
	};

}

#endif

