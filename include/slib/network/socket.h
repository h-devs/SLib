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

#ifndef CHECKHEADER_SLIB_NETWORK_SOCKET
#define CHECKHEADER_SLIB_NETWORK_SOCKET

#include "constants.h"
#include "socket_address.h"
#include "mac_address.h"

typedef int sl_socket;
#define SLIB_SOCKET_INVALID_HANDLE (-1)

namespace slib
{

	enum class L2PacketType
	{
		Host = 0,
		Broadcast = 1,
		Multicast = 2,
		OtherHost = 3,
		OutGoing = 4,
		Loopback = 5,
		FastRoute = 6
	};
	
	class SLIB_EXPORT L2PacketInfo
	{
	public:
		NetworkLinkProtocol protocol; // physical layer protocol
		sl_uint32 iface; // interface number
		
		L2PacketType type;
		sl_uint32 lenHardwareAddress;
		sl_uint8 hardwareAddress[8];
		
	public:
		void setMacAddress(const MacAddress& address) noexcept;
		
		sl_bool getMacAddress(MacAddress* address = sl_null) noexcept;
		
		void clearAddress() noexcept;
		
	};
	
	enum class SocketType
	{
		None = 0,

		MASK_ADDRESS_TYPE = 0x0F,
		MASK_ADDRESS_FAMILY = 0xF0,
		ADDRESS_FAMILY_IPv4 = 0x00,
		ADDRESS_FAMILY_IPv6 = 0x10,
		ADDRESS_FAMILY_DOMAIN = 0x20,
		ADDRESS_FAMILY_PACKET = 0x30,

		Stream = ADDRESS_FAMILY_IPv4,
		Datagram = ADDRESS_FAMILY_IPv4 + 1,
		Raw = ADDRESS_FAMILY_IPv4 + 2,
		StreamIPv6 = ADDRESS_FAMILY_IPv6,
		DatagramIPv6 = ADDRESS_FAMILY_IPv6 + 1,
		RawIPv6 = ADDRESS_FAMILY_IPv6 + 2,
		DomainStream = ADDRESS_FAMILY_DOMAIN,
		DomainDatagram = ADDRESS_FAMILY_DOMAIN + 1,
		PacketDatagram = ADDRESS_FAMILY_PACKET + 1,
		PacketRaw = ADDRESS_FAMILY_PACKET + 2

	};
	
	enum class SocketShutdownMode
	{
		Receive = 0,
		Send = 1,
		Both = 2
	};
	
	enum class SocketError
	{
		None = 0,
		WouldBlock = 1,
		NetworkDown = 2,
		NetworkReset = 3,
		ConnectionReset = 4,
		ConnectionAbort = 5,
		ConnectionRefused = 6,
		Timeout = 7,
		NotSocket = 8,
		AddressAlreadyInUse = 9,
		NoBufs = 10,
		NoMem = 11,
		InProgress = 12,
		DestinationAddressRequired = 13, // sendTo
		ProtocolFamilyNotSupported = 14,
		AddressFamilyNotSupported = 15,
		AddressNotAvailable = 16,
		NotConnected = 17,
		Shutdown = 18,
		Access = 19, // ex. broadcast error
		NotPermitted = 20,
		Invalid = 21,
		Fault = 22,

		Closed = 101,
		UnexpectedResult = 102,

		Unknown = 10000
		
	};
	
	
	class SLIB_EXPORT Socket : public Referable
	{
		SLIB_DECLARE_OBJECT
		
	private:
		Socket() noexcept;
		
		~Socket();
		
	public:
		static Ref<Socket> open(SocketType type, sl_uint32 protocol = 0) noexcept;
		
		static Ref<Socket> openStream(NetworkInternetProtocol internetProtocol) noexcept;

		static Ref<Socket> openTcp() noexcept;

		static Ref<Socket> openDatagram(NetworkInternetProtocol internetProtocol) noexcept;

		static Ref<Socket> openUdp() noexcept;
		
		static Ref<Socket> openRaw(NetworkInternetProtocol internetProtocol) noexcept;
		
		static Ref<Socket> openStream_IPv6(NetworkInternetProtocol internetProtocol) noexcept;

		static Ref<Socket> openTcp_IPv6() noexcept;
		
		static Ref<Socket> openDatagram_IPv6(NetworkInternetProtocol internetProtocol) noexcept;
		
		static Ref<Socket> openUdp_IPv6() noexcept;
		
		static Ref<Socket> openRaw_IPv6(NetworkInternetProtocol internetProtocol) noexcept;

		static Ref<Socket> openDomainStream() noexcept;

		static Ref<Socket> openDomainDatagram() noexcept;

		static Ref<Socket> openPacketRaw(NetworkLinkProtocol linkProtocol = NetworkLinkProtocol::All) noexcept;
		
		static Ref<Socket> openPacketDatagram(NetworkLinkProtocol linkProtocol = NetworkLinkProtocol::All) noexcept;

	public:
		static String getTypeText(SocketType type) noexcept;

		static sl_bool isStreamType(SocketType type) noexcept;

		static sl_bool isDatagramType(SocketType type) noexcept;

		static sl_bool isRawType(SocketType type) noexcept;

		static sl_bool isIPv4Type(SocketType type) noexcept;

		static sl_bool isIPv6Type(SocketType type) noexcept;

		static sl_bool isDomainType(SocketType type) noexcept;

		static sl_bool isPacketType(SocketType type) noexcept;

	public:
		void close() noexcept;

		sl_bool isOpened() const noexcept;

		sl_socket getHandle() const noexcept;

	public:
		sl_bool shutdown(SocketShutdownMode shutMode) noexcept;
		
		sl_bool bind(const SocketAddress& addr) noexcept;

		sl_bool bindDomain(const StringParam& path, sl_bool flagAbstract = sl_false) noexcept;

		sl_bool bindAbstractDomain(const StringParam& name) noexcept;

		sl_bool listen() noexcept;
		
		sl_bool accept(Ref<Socket>& socket, SocketAddress& address) noexcept;

		Ref<Socket> accept(SocketAddress& address) noexcept;

		sl_bool acceptDomain(Ref<Socket>& socket, char* outPath, sl_uint32& inOutLenPath, sl_bool* pOutFlagAbstract = sl_null) noexcept;

		Ref<Socket> acceptDomain(char* outPath, sl_uint32& inOutLenPath, sl_bool* pOutFlagAbstract = sl_null) noexcept;

		sl_bool acceptDomain(Ref<Socket>& socket, String& outPath, sl_bool* pOutFlagAbstract = sl_null) noexcept;

		Ref<Socket> acceptDomain(String& outPath, sl_bool* pOutFlagAbstract = sl_null) noexcept;

		sl_bool connect(const SocketAddress& address) noexcept;
		
		sl_bool connectAndWait(const SocketAddress& address, sl_int32 timeout = -1) noexcept;

		sl_bool connectDomain(const StringParam& path, sl_bool flagAbstract = sl_false) noexcept;

		sl_bool connectDomainAndWait(const StringParam& address, sl_int32 timeout = -1) noexcept;

		sl_bool connectAbstractDomain(const StringParam& name) noexcept;

		sl_bool connectAbstractDomainAndWait(const StringParam& name, sl_int32 timeout = -1) noexcept;

		sl_int32 send(const void* buf, sl_size size) noexcept;
		
		sl_int32 receive(void* buf, sl_size size) noexcept;
		
		sl_int32 sendTo(const SocketAddress& address, const void* buf, sl_size size) noexcept;

		sl_int32 sendToDomain(const StringParam& path, const void* buf, sl_size size, sl_bool flagAbstract = sl_false) noexcept;

		sl_int32 sendToAbstractDomain(const StringParam& name, const void* buf, sl_size size) noexcept;

		sl_int32 receiveFrom(SocketAddress& address, void* buf, sl_size size) noexcept;

		sl_int32 receiveFromDomain(void* buf, sl_size size, char* outPath, sl_uint32& inOutLenPath, sl_bool* pOutFlagAbstract = sl_null) noexcept;

		sl_int32 receiveFromDomain(void* buf, sl_size size, String& outPath, sl_bool* pOutFlagAbstract = sl_null) noexcept;

		sl_int32 sendPacket(const void* buf, sl_size size, const L2PacketInfo& info) noexcept;
		
		sl_int32 receivePacket(const void* buf, sl_size size, L2PacketInfo& info) noexcept;
		
		sl_bool setNonBlockingMode(sl_bool flagEnable) noexcept;
		
		sl_bool setPromiscuousMode(const StringParam& deviceName, sl_bool flagEnable) noexcept;
		
		sl_bool getLocalAddress(SocketAddress& _out) noexcept;
		
		sl_bool getRemoteAddress(SocketAddress& _out) noexcept;

		sl_bool getLocalDomain(char* outPath, sl_uint32& inOutLenPath, sl_bool* pOutFlagAbstract = sl_null) noexcept;

		String getLocalDomain(sl_bool* pOutFlagAbstract = sl_null) noexcept;

		sl_bool getRemoteDomain(char* outPath, sl_uint32& inOutLenPath, sl_bool* pOutFlagAbstract = sl_null) noexcept;

		String getRemoteDomain(sl_bool* pOutFlagAbstract = sl_null) noexcept;

		sl_uint32 getOption_Error() const noexcept;
		
		sl_bool setOption_Broadcast(sl_bool flagEnable) noexcept;
		
		sl_bool getOption_Broadcast() const noexcept;
		
		sl_bool setOption_ReuseAddress(sl_bool flagEnable) noexcept;
		
		sl_bool getOption_ReuseAddress() const noexcept;
		
		sl_bool setOption_ReusePort(sl_bool flagEnable) noexcept;
		
		sl_bool getOption_ReusePort() const noexcept;
		
		sl_bool setOption_SendBufferSize(sl_uint32 size) noexcept;
		
		sl_uint32 getOption_SendBufferSize() const noexcept;
		
		sl_bool setOption_ReceiveBufferSize(sl_uint32 size) noexcept;
		
		sl_uint32 getOption_ReceiveBufferSize() const noexcept;
		
		sl_bool setOption_SendTimeout(sl_uint32 size) noexcept; // write-only
		
		sl_bool setOption_ReceiveTimeout(sl_uint32 size) noexcept; // write-only
		
		sl_bool setOption_IPv6Only(sl_bool flagEnable) noexcept;
		
		sl_bool getOption_IPv6Only() const noexcept;
		
		sl_bool setOption_TcpNoDelay(sl_bool flagEnable) noexcept;
		
		sl_bool getOption_TcpNoDelay() const noexcept;
		
		sl_bool setOption_IpTTL(sl_uint32 ttl) noexcept; // max - 255
		
		sl_uint32 getOption_IpTTL() const noexcept;
		
		sl_bool getOption_IsListening() const noexcept; // read-only
		
		sl_bool setOption_IncludeIpHeader(sl_bool flagEnable) noexcept;
		
		sl_bool getOption_IncludeIpHeader() const noexcept;
		
		sl_bool setOption_bindToDevice(const StringParam& ifname) noexcept;
		
		// multicast
		// interface address may be null
		sl_bool setOption_IpAddMembership(const IPv4Address& ipMulticast, const IPv4Address& ipInterface) noexcept;
		
		sl_bool setOption_IpDropMembership(const IPv4Address& ipMulticast, const IPv4Address& ipInterface) noexcept;
		
		sl_bool setOption_IpMulticastLoop(sl_bool flag) noexcept;
		
		sl_bool getOption_IpMulticastLoop() const noexcept;
		
		sl_bool setOption_IpMulticastTTL(sl_uint32 ttl = 1) noexcept;
		
		sl_uint32 getOption_IpMulticastTTL() const noexcept;

		sl_bool isListening() const noexcept;

	public:
		static void initializeSocket() noexcept;

		static SocketError getLastError() noexcept;

		static String getLastErrorMessage() noexcept;

		static String getErrorMessage(SocketError error) noexcept;
		
		static void clearError() noexcept;
		
	private:
		static SocketError _setError(SocketError code) noexcept;
		static SocketError _checkError() noexcept;
		static sl_int32 _processResult(sl_int32 result) noexcept;
		
		sl_bool setOption(int level, int option, const void* buf, sl_uint32 bufSize) noexcept;
		sl_bool getOption(int level, int option, void* buf, sl_uint32 bufSize) const noexcept;
		sl_bool setOption(int level, int option, sl_uint32 value) noexcept;
		sl_uint32 getOption(int level, int option) const noexcept;
		
	protected:
		sl_socket m_socket;
		
	};

}

#endif
