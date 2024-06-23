/*
 *   Copyright (c) 2008-2018 SLIBIO <https://github.com/SLIBIO>
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

#ifndef CHECKHEADER_SLIB_NETWORK_OS
#define CHECKHEADER_SLIB_NETWORK_OS

#include "ip_address.h"
#include "mac_address.h"

#include "../core/string.h"
#include "../core/list.h"
#include "../core/hash_map.h"

namespace slib
{

	class SLIB_EXPORT NetworkInterfaceInfo
	{
	public:
		sl_uint32 index;
		String name;
		String displayName;
		String description;
		MacAddress macAddress;
		List<IPv4AddressInfo> addresses_IPv4;
		List<IPv6Address> addresses_IPv6;
		sl_bool flagUp;
		sl_bool flagLoopback;

	public:
		NetworkInterfaceInfo();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(NetworkInterfaceInfo)

	};

	class SLIB_EXPORT NetworkAdapterInfo
	{
	public:
		sl_uint32 index; // Interface Index
		String name;
		MacAddress macAddress;
		sl_bool flagPhysical;
		String pnpDeviceId;

	public:
		NetworkAdapterInfo();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(NetworkAdapterInfo)

	};

	class SLIB_EXPORT NetworkSetAddressParam
	{
	public:
		sl_uint32 index; // Interface index
		IPv4Address address;
		IPv4Address subnetMask;
		IPv4Address gateway;
		IPv4Address dns1;
		IPv4Address dns2;

	public:
		NetworkSetAddressParam();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(NetworkSetAddressParam)

	};

	class SLIB_EXPORT Network
	{
	public:
		static sl_bool findInterface(const StringParam& nameOrDisplayName, NetworkInterfaceInfo* pInfo);

		static List<NetworkInterfaceInfo> getInterfaces();

		static List<NetworkAdapterInfo> getAdapters();


		// 0 is returned on error (wrapper of if_nametoindex call)
		static sl_uint32 getInterfaceIndexFromName(const StringParam& name);

		// wrapper of if_indextoname
		static String getInterfaceNameFromIndex(sl_uint32 index);


		static List<IPv4Address> findAllIPv4Addresses();

		static List<IPv4AddressInfo> findAllIPv4AddressInfos();

		static List<IPv6Address> findAllIPv6Addresses();

		static List<MacAddress> findAllMacAddresses();


		static List<IPAddress> getIPAddressesFromHostName(const StringParam& hostName);

		static IPAddress getIPAddressFromHostName(const StringParam& hostName);

		static IPv4Address getIPv4AddressFromHostName(const StringParam& hostName);

		static IPv6Address getIPv6AddressFromHostName(const StringParam& hostName);


		static sl_bool setAddress(const NetworkSetAddressParam& param);

		static IPv4Address getDefaultGateway(const StringParam& interfaceName);


		static HashMap<IPv4Address, MacAddress> getArpTable();

	};

}

#endif
