/*
 *   Copyright (c) 2008-2024 SLIBIO <https://github.com/SLIBIO>
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

#include "slib/network/os.h"

#include "slib/network/socket.h"
#include "slib/system/system.h"
#include "slib/core/endian.h"
#include "slib/core/hash_map.h"
#include "slib/core/stringx.h"

#if defined(SLIB_PLATFORM_IS_WIN32)

#include "slib/system/process.h"
#include "slib/core/search.h"
#include "slib/core/scoped_buffer.h"
#include "slib/core/variant.h"
#include "slib/platform.h"
#include "slib/platform/win32/registry.h"
#include "slib/platform/win32/wmi.h"
#include "slib/dl/win32/iphlpapi.h"

#include <winsock2.h>
#include <ws2ipdef.h>
#include <ws2tcpip.h>
#include <netioapi.h>

#pragma comment(lib, "ws2_32.lib")

#elif defined(SLIB_PLATFORM_IS_UNIX)

#include "slib/io/file.h"
#if defined(SLIB_PLATFORM_IS_ANDROID)
#	include "slib/platform.h"
#endif

#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <net/if.h>
#if defined(SLIB_PLATFORM_IS_APPLE)
#	define	IFT_ETHER	0x6		/* Ethernet CSMACD */
#	include <net/if_dl.h>
#endif
#if defined(SLIB_PLATFORM_IS_LINUX)
#	include <linux/if_packet.h>
#endif

#endif

namespace slib
{

	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(NetworkInterfaceInfo)

	NetworkInterfaceInfo::NetworkInterfaceInfo(): index(0), flagUp(sl_false), flagLoopback(sl_false)
	{
	}


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(NetworkAdapterInfo)

	NetworkAdapterInfo::NetworkAdapterInfo(): interfaceIndex(0), flagPhysical(sl_false)
	{
	}

	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(NetworkSetAddressParam)

	NetworkSetAddressParam::NetworkSetAddressParam(): index(0)
	{
	}


	List<IPv4Address> Network::findAllIPv4Addresses()
	{
		List<IPv4Address> ret;
		ListElements<NetworkInterfaceInfo> devices(Network::getInterfaces());
		for (sl_size i = 0; i < devices.count; i++) {
			NetworkInterfaceInfo& device = devices[i];
			if (!(device.flagLoopback)) {
				ListElements<IPv4AddressInfo> addresses(device.addresses_IPv4);
				for (sl_size k = 0; k < addresses.count; k++) {
					IPv4Address& ip = addresses[k].address;
					if (ip.isHost()) {
						ret.add_NoLock(ip);
					}
				}
			}
		}
		return ret;
	}

	List<IPv4AddressInfo> Network::findAllIPv4AddressInfos()
	{
		List<IPv4AddressInfo> list;
		ListElements<NetworkInterfaceInfo> devices(Network::getInterfaces());
		for (sl_size i = 0; i < devices.count; i++) {
			NetworkInterfaceInfo& device = devices[i];
			if (!(device.flagLoopback)) {
				ListElements<IPv4AddressInfo> addrs(device.addresses_IPv4);
				for (sl_size k = 0; k < addrs.count; k++) {
					if (addrs[k].address.isHost()) {
						list.add_NoLock(addrs[k]);
					}
				}
			}
		}
		return list;
	}

	List<IPv6Address> Network::findAllIPv6Addresses()
	{
		List<IPv6Address> list;
		ListElements<NetworkInterfaceInfo> devices(Network::getInterfaces());
		for (sl_size i = 0; i < devices.count; i++) {
			NetworkInterfaceInfo& device = devices[i];
			if (!(device.flagLoopback)) {
				ListElements<IPv6Address> addrs(device.addresses_IPv6);
				for (sl_size k = 0; k < addrs.count; k++) {
					if (addrs[k].isNotZero() && !(addrs[k].isLoopback()) && !(addrs[k].isIPv4Transition())) {
						list.add_NoLock(addrs[k]);
					}
				}
			}
		}
		return list;
	}

	List<MacAddress> Network::findAllMacAddresses()
	{
		List<MacAddress> list;
		ListElements<NetworkInterfaceInfo> devices(Network::getInterfaces());
		for (sl_size i = 0; i < devices.count; i++) {
			if (devices[i].macAddress.isNotZero()) {
				list.add_NoLock(devices[i].macAddress);
			}
		}
		return list;
	}


	sl_bool Network::findInterface(const StringParam& _name, NetworkInterfaceInfo* pInfo)
	{
		StringData name(_name);
		ListElements<NetworkInterfaceInfo> devices(Network::getInterfaces());
		for (sl_size i = 0; i < devices.count; i++) {
			if (devices[i].name == name || devices[i].displayName == name) {
				if(pInfo) {
					*pInfo = devices[i];
				}
				return sl_true;
			}
		}
		return sl_false;
	}

#if defined(SLIB_PLATFORM_IS_WIN32)
	template <>
	class Compare<MIB_IPADDRROW, IPv4Address>
	{
	public:
		sl_compare_result operator()(const MIB_IPADDRROW& a, const IPv4Address& b) const
		{
			return Compare<sl_uint32>()(a.dwAddr, Endian::swap32LE(b.toInt()));
		}
	};

	template <>
	class Equals<MIB_IPADDRROW, IPv4Address>
	{
	public:
		sl_bool operator()(const MIB_IPADDRROW& a, const IPv4Address& b) const
		{
			return Equals<sl_uint32>()(a.dwAddr, Endian::swap32LE(b.toInt()));
		}
	};

	List<NetworkInterfaceInfo> Network::getInterfaces()
	{
		auto funcGetIpAddrTable = iphlpapi::getApi_GetIpAddrTable();
		if (!funcGetIpAddrTable) {
			return sl_null;
		}
		auto funcGetAdaptersAddresses = iphlpapi::getApi_GetAdaptersAddresses();
		if (!funcGetAdaptersAddresses) {
			return sl_null;
		}

		Socket::initializeSocket();

		ULONG ulOutBufLen = 0;
		if (funcGetAdaptersAddresses(AF_UNSPEC, 0, NULL, sl_null, &ulOutBufLen) != ERROR_BUFFER_OVERFLOW) {
			return sl_null;
		}
		SLIB_SCOPED_BUFFER(sl_uint8, 4096, bufAdapter, (sl_size)ulOutBufLen)
		if (!bufAdapter) {
			return sl_null;
		}
		IP_ADAPTER_ADDRESSES* adapter = (IP_ADAPTER_ADDRESSES*)bufAdapter;
		if (funcGetAdaptersAddresses(AF_UNSPEC, 0, NULL, adapter, &ulOutBufLen) != NO_ERROR) {
			return sl_null;
		}

		sl_bool flagVista = Win32::getVersion().majorVersion >= WindowsVersion::Vista_MajorVersion;
		ulOutBufLen = 0;
		if (!flagVista) {
			if (funcGetIpAddrTable(sl_null, &ulOutBufLen, TRUE) != ERROR_INSUFFICIENT_BUFFER) {
				return sl_null;
			}
		}
		SLIB_SCOPED_BUFFER(sl_uint8, 1024, bufIptable, (sl_size)ulOutBufLen);
		MIB_IPADDRTABLE* iptable = (MIB_IPADDRTABLE*)bufIptable;
		if (!flagVista) {
			if (!iptable) {
				return sl_null;
			}
			if (funcGetIpAddrTable(iptable, &ulOutBufLen, TRUE) != NO_ERROR) {
				return sl_null;
			}
		}

		List<NetworkInterfaceInfo> ret;
		do {
			NetworkInterfaceInfo device;
			device.index = (sl_uint32)(adapter->IfIndex);
			device.name = adapter->AdapterName;
			device.displayName = String::create(adapter->FriendlyName);
			device.description = String::create(adapter->Description);
			device.flagUp = adapter->OperStatus == IfOperStatusUp;
			device.flagLoopback = adapter->IfType == IF_TYPE_SOFTWARE_LOOPBACK;
			IP_ADAPTER_UNICAST_ADDRESS* pip = adapter->FirstUnicastAddress;
			while (pip) {
				SocketAddress sa;
				sa.setSystemSocketAddress(pip->Address.lpSockaddr, pip->Address.iSockaddrLength);
				if (sa.ip.isIPv4()) {
					IPv4AddressInfo a4;
					a4.address = sa.ip.getIPv4();
					sl_uint32 networkPrefixLength = 0;
					if (flagVista) {
						networkPrefixLength = ((IP_ADAPTER_UNICAST_ADDRESS_LH*)pip)->OnLinkPrefixLength;
					} else {
						sl_size indexTable = 0;
						if (BinarySearch::search(iptable->table, iptable->dwNumEntries, a4.address, &indexTable)) {
							networkPrefixLength = IPv4Address(Endian::swap32LE(iptable->table[indexTable].dwMask)).getNetworkPrefixLengthFromMask();
						}
					}
					a4.networkPrefixLength = networkPrefixLength;
					device.addresses_IPv4.add_NoLock(a4);
				} else if (sa.ip.isIPv6()) {
					IPv6Address a6;
					a6 = sa.ip.getIPv6();
					device.addresses_IPv6.add_NoLock(a6);
				}
				pip = pip->Next;
			}
			if (adapter->PhysicalAddressLength == 6) {
				device.macAddress.setBytes(adapter->PhysicalAddress);
			} else {
				device.macAddress.setZero();
			}
			ret.add_NoLock(device);
			adapter = adapter->Next;
		} while (adapter);

		return ret;
	}

	List<NetworkAdapterInfo> Network::getAdapters()
	{
		List<NetworkAdapterInfo> ret;
		ListElements<VariantMap> items(win32::Wmi::getQueryResponseRecords(L"SELECT * FROM Win32_NetworkAdapter", L"Name", L"InterfaceIndex", L"NetConnectionID", L"MACAddress", L"PhysicalAdapter", L"PNPDeviceID"));
		for (sl_size i = 0; i < items.count; i++) {
			NetworkAdapterInfo adapter;
			VariantMap& item = items[i];
			adapter.interfaceIndex = item.getValue("InterfaceIndex").getUint32();
			adapter.interfaceName = item.getValue("NetConnectionID").getString();
			adapter.deviceName = item.getValue("Name").getString();
			adapter.macAddress.parse(item.getValue("MACAddress").getString());
			adapter.flagPhysical = item.getValue("PhysicalAdapter").getBoolean();
			adapter.pnpDeviceId = item.getValue("PNPDeviceID").getString();
			ret.add_NoLock(Move(adapter));
		}
		return ret;
	}

#elif defined(SLIB_PLATFORM_IS_ANDROID)
	namespace 
	{
		SLIB_JNI_BEGIN_CLASS(JNetworkDevice, "slib/android/network/NetworkDevice")
			SLIB_JNI_INT_FIELD(index);
			SLIB_JNI_STRING_FIELD(name);
			SLIB_JNI_STRING_FIELD(macAddress);
			SLIB_JNI_OBJECT_FIELD(addresses_IPv4, "[Ljava/lang/String;");
			SLIB_JNI_OBJECT_FIELD(addresses_IPv6, "[Ljava/lang/String;");
			SLIB_JNI_BOOLEAN_FIELD(flagUp);
			SLIB_JNI_BOOLEAN_FIELD(flagLoopback);
		SLIB_JNI_END_CLASS

		SLIB_JNI_BEGIN_CLASS(JNetworkAddress, "slib/android/network/Network")
			SLIB_JNI_STATIC_METHOD(getAllDevices, "getAllDevices", "()[Lslib/android/network/NetworkDevice;");
		SLIB_JNI_END_CLASS
	}

	List<NetworkInterfaceInfo> Network::getInterfaces()
	{
		List<NetworkInterfaceInfo> ret;
		if (JNetworkAddress::get() && JNetworkDevice::get()) {
			JniLocal<jobjectArray> jarr = JNetworkAddress::getAllDevices.callObject(sl_null);
			if (jarr.isNotNull()) {
				sl_uint32 n = Jni::getArrayLength(jarr);
				for (sl_uint32 i = 0; i < n; i++) {
					JniLocal<jobject> jdev = Jni::getObjectArrayElement(jarr, i);
					if (jdev.isNotNull()) {
						NetworkInterfaceInfo dev;
						dev.index = JNetworkDevice::index.get(jdev);
						dev.name = JNetworkDevice::name.get(jdev);
						dev.displayName = dev.name;
						dev.macAddress.setZero();
						dev.macAddress.parse(JNetworkDevice::macAddress.get(jdev));
						JniLocal<jobjectArray> jarrIPv4 = JNetworkDevice::addresses_IPv4.get(jdev);
						if (jarrIPv4.isNotNull()) {
							sl_uint32 nAddr = Jni::getArrayLength(jarrIPv4);
							for (sl_uint32 k = 0; k < nAddr; k++) {
								String saddr = Jni::getStringArrayElement(jarrIPv4, k);
								sl_int32 indexPrefix = saddr.indexOf("/");
								if (indexPrefix > 0) {
									IPv4AddressInfo ip;
									if (ip.address.parse(saddr.substring(0, indexPrefix))) {
										ip.networkPrefixLength = saddr.substring(indexPrefix+1).parseUint32();
										dev.addresses_IPv4.add_NoLock(ip);
									}
								}
							}
						}
						JniLocal<jobjectArray> jarrIPv6 = JNetworkDevice::addresses_IPv6.get(jdev);
						if (jarrIPv6.isNotNull()) {
							sl_uint32 nAddr = Jni::getArrayLength(jarrIPv6);
							for (sl_uint32 k = 0; k < nAddr; k++) {
								String saddr = Jni::getStringArrayElement(jarrIPv6, k);
								sl_int32 indexPrefix = saddr.indexOf("/");
								if (indexPrefix > 0) {
									IPv6Address ip;
									if (ip.parse(saddr.substring(0, indexPrefix))) {
										dev.addresses_IPv6.add_NoLock(ip);
									}
								}
							}
						}
						dev.flagUp = JNetworkDevice::flagUp.get(jdev);
						dev.flagLoopback = JNetworkDevice::flagLoopback.get(jdev);
						ret.add_NoLock(dev);
					}
				}
			}
		}
		return ret;
	}

#elif defined(SLIB_PLATFORM_IS_UNIX)

	List<NetworkInterfaceInfo> Network::getInterfaces()
	{
		HashMap<String, NetworkInterfaceInfo> ret;

		struct ifaddrs* adapters = 0;
		getifaddrs(&adapters);

		if (adapters) {

			struct ifaddrs* adapter = adapters;

			while (adapter) {

				String name = adapter->ifa_name;

				NetworkInterfaceInfo* pdev = ret.getItemPointer(name);
				if (!pdev) {
					NetworkInterfaceInfo dev;
					dev.index = if_nametoindex(adapter->ifa_name);
					dev.name = name;
					dev.displayName = name;
					dev.macAddress.setZero();
					dev.flagUp = (adapter->ifa_flags & (IFF_UP | IFF_RUNNING)) != 0;
					dev.flagLoopback = (adapter->ifa_flags & IFF_LOOPBACK) != 0;
					ret.put_NoLock(name, dev);
					pdev = ret.getItemPointer(name);
				}
				if (pdev && adapter->ifa_addr) {
					if (adapter->ifa_addr->sa_family == AF_INET) {
						// ipv4 address
						sockaddr_in* addr = (sockaddr_in*)(adapter->ifa_addr);
						sockaddr_in* mask = (sockaddr_in*)(adapter->ifa_netmask);
						IPv4AddressInfo ip;
						ip.address = IPv4Address(Endian::swap32LE(addr->sin_addr.s_addr));
						ip.networkPrefixLength = IPv4Address(Endian::swap32LE(mask->sin_addr.s_addr)).getNetworkPrefixLengthFromMask();
						pdev->addresses_IPv4.add_NoLock(ip);
					} else if (adapter->ifa_addr->sa_family == AF_INET6) {
						// ipv6 address
						SocketAddress s;
						s.setSystemSocketAddress(adapter->ifa_addr);
						IPv6Address ip;
						ip = s.ip.getIPv6();
						pdev->addresses_IPv6.add_NoLock(ip);
					}
#	if defined(SLIB_PLATFORM_IS_APPLE)
					else if (adapter->ifa_addr->sa_family == AF_LINK) {
						sockaddr_dl* addr = (sockaddr_dl*)(adapter->ifa_addr);
						if (addr->sdl_type == IFT_ETHER) {
							sl_uint8* base = (sl_uint8*)(addr->sdl_data + addr->sdl_nlen);
							pdev->macAddress.setBytes(base);
						}
					}
#	endif
#	if defined(SLIB_PLATFORM_IS_LINUX)
					else if (adapter->ifa_addr->sa_family == AF_PACKET) {
						sockaddr_ll* addr = (sockaddr_ll*)(adapter->ifa_addr);
						if (addr->sll_halen == 6) {
							pdev->macAddress.setBytes((sl_uint8*)(addr->sll_addr));
						}
					}
#	endif
				}
				adapter = adapter->ifa_next;
			}
			freeifaddrs(adapters);
		}

		return ret.getAllValues();
	}

#endif

	sl_uint32 Network::getInterfaceIndexFromName(const StringParam& _name)
	{
		StringCstr name(_name);
#if defined(SLIB_PLATFORM_IS_WIN32)
		auto func = iphlpapi::getApi_if_nametoindex();
		if (func) {
			Socket::initializeSocket();
			return func(name.getData());
		}
		return 0;
#else
		return if_nametoindex(name.getData());
#endif
	}

	String Network::getInterfaceNameFromIndex(sl_uint32 index)
	{
		char buf[256];
		char* s;
#if defined(SLIB_PLATFORM_IS_WIN32)
		auto func = iphlpapi::getApi_if_indextoname();
		if (func) {
			Socket::initializeSocket();
			s = func(index, buf);
		} else {
			return sl_null;
		}
#else
		s = if_indextoname(index, buf);
#endif
		if (s) {
			return String(s);
		} else {
			return sl_null;
		}
	}

	List<IPAddress> Network::getIPAddressesFromHostName(const StringParam& _hostName)
	{
#if defined(SLIB_PLATFORM_IS_WIN32)
		Socket::initializeSocket();
#endif

		StringCstr hostName(_hostName);
		List<IPAddress> ret;

		addrinfo *addrs = sl_null;

		SocketAddress sa;
		int iRet = getaddrinfo(hostName.getData(), sl_null, sl_null, &addrs);
		if (iRet == 0) {
			addrinfo* addr = addrs;
			while (addr) {
				sl_int32 lenAddr = (sl_int32)(addr->ai_addrlen);
				if (lenAddr > 0) {
					sa.ip.setNone();
					sa.setSystemSocketAddress(addr->ai_addr, lenAddr);
					if (sa.ip.isNotNone()) {
						ret.add_NoLock(sa.ip);
					}
				}
				addr = addr->ai_next;
			}
			if (addrs) {
				freeaddrinfo(addrs);
			}
		}
		return ret;
	}

	IPAddress Network::getIPAddressFromHostName(const StringParam& hostName)
	{
		ListElements<IPAddress> list(getIPAddressesFromHostName(hostName));
		sl_size i;
		for (i = 0; i < list.count; i++) {
			if (list[i].isIPv4()) {
				return list[i];
			}
		}
		for (i = 0; i < list.count; i++) {
			if (list[i].isIPv6()) {
				return list[i];
			}
		}
		return IPAddress::none();
	}

	IPv4Address Network::getIPv4AddressFromHostName(const StringParam& hostName)
	{
		ListElements<IPAddress> list(getIPAddressesFromHostName(hostName));
		for (sl_size i = 0; i < list.count; i++) {
			if (list[i].isIPv4()) {
				return list[i].getIPv4();
			}
		}
		return IPv4Address::zero();
	}

	IPv6Address Network::getIPv6AddressFromHostName(const StringParam& hostName)
	{
		ListElements<IPAddress> list(getIPAddressesFromHostName(hostName));
		for (sl_size i = 0; i < list.count; i++) {
			if (list[i].isIPv6()) {
				return list[i].getIPv6();
			}
		}
		return IPv6Address::zero();
	}

	sl_bool Network::setAddress(const NetworkSetAddressParam& param)
	{
#if defined(SLIB_PLATFORM_IS_WIN32)
		if (!(Process::isCurrentProcessAdmin())) {
			return sl_false;
		}
		SLIB_STATIC_STRING(strNone, "none")
		System::execute(String::format("netsh interface ipv4 set address %s static %s %s %s", param.index, param.address.toString(), param.subnetMask.toString(), param.gateway.isNotZero() ? param.gateway.toString() : strNone), sl_true);
		IPv4Address dns1 = param.dns1;
		IPv4Address dns2 = param.dns2;
		if (dns1.isZero()) {
			dns1 = dns2;
			dns2.setZero();
		}
		System::execute(String::format("netsh interface ipv4 set dnsservers %s static %s validate=no", param.index, dns1.isNotZero() ? dns1.toString() : strNone), sl_true);
		if (dns2.isNotZero()) {
			System::execute(String::format("netsh interface ipv4 add dnsservers %s %s validate=no", param.index, dns2.toString()), sl_true);
		}
		return sl_true;
#else
		return sl_false;
#endif
	}

	IPv4Address Network::getDefaultGateway(const StringParam& _interfaceName)
	{
#if defined(SLIB_PLATFORM_IS_WIN32)
		// Reference: PcapPlusPlus (https://github.com/seladb/PcapPlusPlus/blob/master/Pcap%2B%2B/src/PcapLiveDevice.cpp#setDefaultGateway)
		auto funcGetAdaptersInfo = iphlpapi::getApi_GetAdaptersInfo();
		if (!funcGetAdaptersInfo) {
			return IPv4Address::zero();
		}

		Socket::initializeSocket();

		ULONG ulOutBufLen = 0;
		if (funcGetAdaptersInfo(sl_null, &ulOutBufLen) != ERROR_BUFFER_OVERFLOW) {
			return IPv4Address::zero();
		}
		SLIB_SCOPED_BUFFER(sl_uint8, 1024, bufInfo, (sl_size)ulOutBufLen)
		if (!bufInfo) {
			return IPv4Address::zero();
		}
		IP_ADAPTER_INFO* info = (IP_ADAPTER_INFO*)bufInfo;
		if (funcGetAdaptersInfo(info, &ulOutBufLen) != NO_ERROR) {
			return IPv4Address::zero();
		}

		StringData interfaceName(_interfaceName);
		do {
			if (interfaceName.equals(info->AdapterName) || interfaceName.equals(info->Description)) {
				return IPv4Address(info->GatewayList.IpAddress.String);
			}
			info = info->Next;
		} while (info);
#elif defined(SLIB_PLATFORM_IS_MACOS) || defined(SLIB_PLATFORM_IS_FREEBSD)
		String command = String::concat(StringView::literal("netstat -nr | grep default | grep "), _interfaceName);
		char buf[1024];
		int n = System::getCommandOutput(command.getData(), buf, sizeof(buf) - 1);
		if (n > 0) {
			buf[n] = 0;
			if (Base::equalsMemory(buf, "default ", 8)) {
				char* p = buf + 8;
				while (*p == ' ') {
					p++;
				}
				IPv4Address ret;
				sl_reg iRet = IPv4Address::parse(&ret, buf, p - buf, sizeof(buf) - 1);
				if (iRet > 0 && buf[iRet] == ' ') {
					return ret;
				}
			}
		}
#elif defined(SLIB_PLATFORM_IS_LINUX)
		File file = File::openForRead("/proc/net/route");
		if (file.isNone()) {
			return IPv4Address::zero();
		}
		StringData interfaceName(_interfaceName);
		for (;;) {
			String line = file.readLine();
			if (line.isNull()) {
				break;
			}
			sl_reg index = line.indexOf('\t');
			if (index > 0) {
				StringView name = StringView(line).substring(0, index);
				if (interfaceName.equals(name)) {
					StringView remain = StringView(line).substring(index + 1);
					if (remain.startsWith("00000000\t")) {
						remain = remain.substring(9);
						sl_reg index = remain.indexOf('\t');
						if (index > 0) {
							StringView gateway = remain.substring(0, index);
							sl_uint32 nip = gateway.parseUint32(16);
							return IPv4Address(SLIB_GET_BYTE0(nip), SLIB_GET_BYTE1(nip), SLIB_GET_BYTE2(nip), SLIB_GET_BYTE3(nip));
						}
					}
				}
			}
		}
#endif
		return IPv4Address::zero();
	}

	void Network::disableIPv6()
	{
#if defined(SLIB_PLATFORM_IS_WIN32)
		System::execute(StringView::literal("powershell.exe -command Disable-NetAdapterBinding -Name * -ComponentID ms_tcpip6"), sl_true);
		win32::Registry::setValue(HKEY_LOCAL_MACHINE, StringView16::literal(u"SYSTEM\\CurrentControlSet\\Services\\Tcpip6\\Parameters"), StringView16::literal(u"DisabledComponents"), (sl_uint32)0xFF);
#elif defined(SLIB_PLATFORM_IS_LINUX)
		System::execute(StringView::literal("sysctl -w net.ipv6.conf.all.disable_ipv6=1"));
		System::execute(StringView::literal("sysctl -w net.ipv6.conf.default.disable_ipv6=1"));
#elif defined(SLIB_PLATFORM_IS_MACOS)
		for (auto&& service : Stringx::splitLines(System::getCommandOutput(StringView::literal("networksetup -listallnetworkservices")))) {
			if (!service.contains('*')) {
				System::execute(String::concat("networksetup -setv6off ", service));
			}
		}
#endif
	}

	void Network::renewDhcp(const StringView& interfaceName)
	{
#if defined(SLIB_PLATFORM_IS_WIN32)
		System::execute(StringView::literal("ipconfig /renew"), sl_true);
#elif defined(SLIB_PLATFORM_IS_LINUX)
		System::execute(StringView::literal("dhclient -r"));
#elif defined(SLIB_PLATFORM_IS_MACOS)
		System::execute(String::concat(StringView::literal("ifconfig "), interfaceName, StringView::literal(" down")));
		System::execute(String::concat(StringView::literal("ifconfig "), interfaceName, StringView::literal(" up")));
#endif
	}

	HashMap<IPv4Address, MacAddress> Network::getArpTable()
	{
		HashMap<IPv4Address, MacAddress> ret;
#if defined(SLIB_PLATFORM_IS_WIN32)
		auto funcGetIpNetTable = iphlpapi::getApi_GetIpNetTable();
		if (!funcGetIpNetTable) {
			return ret;
		}
		MIB_IPNETTABLE _table = { 0 };
		DWORD size = 0;
		funcGetIpNetTable(&_table, &size, FALSE);
		if (size) {
			MIB_IPNETTABLE* table = (MIB_IPNETTABLE*)(Base::createMemory(size));
			if (!table) {
				return ret;
			}
			if (funcGetIpNetTable(table, &size, FALSE) == NO_ERROR) {
				for (DWORD i = 0; i < table->dwNumEntries; i++) {
					MIB_IPNETROW& row = table->table[i];
					if (row.dwPhysAddrLen == 6 && (row.dwType == MIB_IPNET_TYPE_DYNAMIC || row.dwType == MIB_IPNET_TYPE_STATIC)) {
						IPv4Address ip((sl_uint32)(Endian::swap32(row.dwAddr)));
						MacAddress mac(row.bPhysAddr);
						ret.add_NoLock(ip, mac);
					}
				}
			}
			Base::freeMemory(table);
		}
#endif
		return ret;
	}

	sl_bool Network::removeArpEntry(sl_uint32 index, const IPv4Address& ip)
	{
#if defined(SLIB_PLATFORM_IS_WIN32)
		auto funcDeleteIpNetEntry = iphlpapi::getApi_DeleteIpNetEntry();
		if (funcDeleteIpNetEntry) {
			MIB_IPNETROW row = { 0 };
			row.dwIndex = index;
			row.dwAddr = (sl_uint32)(Endian::swap32(ip.toInt()));
			if (funcDeleteIpNetEntry(&row) == NO_ERROR) {
				return sl_true;
			}
		}
#endif
		return sl_false;
	}

	void Network::flushArpTable()
	{
#if defined(SLIB_PLATFORM_IS_WIN32)
		System::execute(StringView::literal("arp -d *"), sl_true);
#elif defined(SLIB_PLATFORM_IS_MACOS)
		System::execute(StringView::literal("arp -d -a"));
#elif defined(SLIB_PLATFORM_IS_LINUX)
		System::execute(StringView::literal("arp -a | egrep -o '\\(.+\\)' | egrep -o '[0-9\\.]+' | xargs -n1 arp -d"));
#endif
	}

	void Network::flushDnsCache()
	{
#if defined(SLIB_PLATFORM_IS_WIN32)
		System::execute(StringView::literal("ipconfig /flushdns"), sl_true);
#elif defined(SLIB_PLATFORM_IS_MACOS)
		System::execute(StringView::literal("dscacheutil -flushcache"));
		System::execute(StringView::literal("killall -HUP mDNSResponder"));
#elif defined(SLIB_PLATFORM_IS_LINUX)
		System::execute(StringView::literal("service nscd restart"));
		System::execute(StringView::literal("systemd-resolve --flush-caches"));
		System::execute(StringView::literal("resolvectl flush-caches"));
#endif
	}

}
