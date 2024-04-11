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

#include "dbip.h"

#include <slib/io/file.h>
#include <slib/core/memory.h>
#include <slib/core/search.h>
#include <slib/core/string.h>

namespace slib
{

	template<>
	class Compare<DbIp::IPv4Item, sl_uint32>
	{
	public:
		int operator()(const DbIp::IPv4Item& a, const sl_uint32& b) const
		{
			return Compare<sl_uint32>()(a.start, b);
		}
	};

	DbIp::DbIp()
	{
		clearAll();
	}

	DbIp::~DbIp()
	{
	}

	sl_bool DbIp::parse(const void* _data, sl_size _size)
	{
		if (_size == 0 || _size >= 0x80000000) {
			return sl_false;
		}
		char* sz = (char*)_data;
		sl_uint32 len = (sl_uint32)_size;

		List<IPv4Item> list4 = List<IPv4Item>::create(0, len / 64);
		List<IPv6Item> list6 = List<IPv6Item>::create(0, len / 128);

		sl_size pos = 0;
		sl_reg resultParse;
		while (pos < len) {
			IPv4Item item4;
			IPv6Item item6;
			do {
				if (pos >= len) {
					break;
				}
				if (sz[pos] == '"') {
					pos++;
				}
				IPAddress ip;
				resultParse = IPAddress::parse(&ip, sz, pos, len);
				if (resultParse == SLIB_PARSE_ERROR) {
					break;
				}
				if (ip.isIPv4()) {
					item4.start = ip.getIPv4().getInt();
				} else {
					item6.start = ip.getIPv6();
				}
				pos = resultParse;
				if (pos >= len) {
					break;
				}
				if (sz[pos] == '"') {
					pos++;
				}
				if (pos >= len || sz[pos] != ',') {
					break;
				}
				pos++;
				if (pos >= len) {
					break;
				}
				if (sz[pos] == '"') {
					pos++;
				}
				if (ip.isIPv4()) {
					IPv4Address ip4;
					resultParse = IPv4Address::parse(&ip4, sz, pos, len);
					if (resultParse == SLIB_PARSE_ERROR) {
						break;
					}
					item4.end = ip4.getInt();
				} else {
					IPv6Address ip6;
					resultParse = IPv6Address::parse(&ip6, sz, pos, len);
					if (resultParse == SLIB_PARSE_ERROR) {
						break;
					}
					item6.end = ip6;
				}
				pos = resultParse;
				if (pos >= len) {
					break;
				}
				if (sz[pos] == '"') {
					pos++;
				}
				if (pos >= len || sz[pos] != ',') {
					break;
				}
				pos++;
				if (pos >= len) {
					break;
				}
				if (sz[pos] == '"') {
					pos++;
				}
				if (pos + 1 >= len) {
					break;
				}
				if (ip.isIPv4()) {
					item4.code[0] = sz[pos];
					item4.code[1] = sz[pos + 1];
					item4.code[2] = 0;
					item4.code[3] = 0;
					if (!(list4.add(item4))) {
						return sl_false;
					}
				} else {
					item6.code[0] = sz[pos];
					item6.code[1] = sz[pos + 1];
					item6.code[2] = 0;
					item6.code[3] = 0;
					if (!(list6.add(item6))) {
						return sl_false;
					}
				}
				pos += 2;
				if (pos >= len) {
					break;
				}
				if (sz[pos] == '"') {
					pos++;
				}
			} while (0);
			while (pos < len && sz[pos] != '\r' && sz[pos] != '\n') {
				pos++;
			}
			while (pos < len && (sz[pos] == '\r' || sz[pos] == '\n')) {
				pos++;
			}
		}
		if (list4.getCount() == 0 && list6.getCount() == 0) {
			return sl_false;
		}

		m_ipv4 = list4.getData();
		m_countIPv4 = (sl_uint32)(list4.getCount());
		m_listIPv4 = Move(list4);

		m_ipv6 = list6.getData();
		m_countIPv6 = (sl_uint32)(list6.getCount());
		m_listIPv6 = Move(list6);

		return sl_true;
	}

	sl_bool DbIp::parseFile(const StringParam& pathToCSVFile)
	{
		Memory mem = File::readAllBytes(pathToCSVFile);
		if (mem.isNotNull()) {
			return parse(mem.getData(), mem.getSize());
		}
		return sl_false;
	}

	void DbIp::clearAll()
	{
		m_ipv4 = sl_null;
		m_countIPv4 = 0;
		m_listIPv4.setNull();

		m_ipv6 = sl_null;
		m_countIPv6 = 0;
		m_listIPv6.setNull();
	}


	const char* DbIp::getCountryCode(const IPv4Address& _ipv4, sl_size depth)
	{
		sl_size index = 0;
		sl_uint32 ipv4 = _ipv4.getInt();
		if (BinarySearch::search(m_ipv4, m_countIPv4, ipv4, &index)) {
			return m_ipv4[index].code;
		} else {
			// reverse search for overlapped ip ranges
			for (sl_size i = 0; i < depth; i++) {
				if (index > i) {
					if (ipv4 >= m_ipv4[index - i - 1].start && ipv4 <= m_ipv4[index - i - 1].end) {
						return m_ipv4[index - i - 1].code;
					}
				}
			}
		}
		return sl_null;
	}

	List<DbIp::IPv4Item> DbIp::getIPv4Items(const StringParam& _code)
	{
		List<IPv4Item> ret;
		String code = _code.toString();
		for (auto&& item : m_listIPv4) {
			if (code.equals(item.code)) {
				ret.add_NoLock(item);
			}
		}
		return ret;
	}

	template<>
	class Compare<DbIp::IPv6Item, IPv6Address>
	{
	public:
		int operator()(const DbIp::IPv6Item& a, const IPv6Address& b) const
		{
			return Compare<IPv6Address>()(a.start, b);
		}
	};

	const char* DbIp::getCountryCode(const IPv6Address& ipv6, sl_size depth)
	{
		sl_size index = 0;
		if (BinarySearch::search(m_ipv6, m_countIPv6, ipv6, &index)) {
			return m_ipv6[index].code;
		} else {
			// reverse search for overlapped ip ranges
			for (sl_size i = 0; i < depth; i++) {
				if (index > i) {
					if (ipv6 >= m_ipv6[index - i - 1].start && ipv6 <= m_ipv6[index - i - 1].end) {
						return m_ipv6[index - i - 1].code;
					}
				}
			}
		}
		return sl_null;
	}

	List<DbIp::IPv6Item> DbIp::getIPv6Items(const StringParam& _code)
	{
		List<IPv6Item> ret;
		String code = _code.toString();
		for (auto&& item : m_listIPv6) {
			if (code.equals(item.code)) {
				ret.add_NoLock(item);
			}
		}
		return ret;
	}

}
