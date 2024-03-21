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

#include "p2p.h"

#define USE_CURVE448

#include <slib/network/socket.h>
#include <slib/network/event.h>
#include <slib/network/async.h>
#include <slib/network/os.h>
#include <slib/core/dispatch_loop.h>
#include <slib/core/system.h>
#include <slib/core/scoped_buffer.h>
#include <slib/core/log.h>
#include <slib/crypto/aes.h>
#include <slib/crypto/hkdf.h>
#ifdef USE_CURVE448
#include <slib/crypto/curve448.h>
#else
#include <slib/crypto/curve25519.h>
#endif
#include <slib/data/expiring_map.h>
#include <slib/io/chunk.h>

#ifdef SLIB_DEBUG
#define LOG_COMMANDS
#endif
#ifdef LOG_COMMANDS
#define LOG_RECEIVE_COMMAND(COMMAND, ADDRESS) _logReceiveCommand(COMMAND, ADDRESS);
#else
#define LOG_RECEIVE_COMMAND(COMMAND, ADDRESS)
#endif

#define DURATION_VALID_BROADCASTERS 10000

/*
	P2P Socket Protocol

Port = Uint16 (Little Endian)
TickCount = Uint32 (Little Endian)
SharedKey = HKDF(ECDH(LocalNode's Ephemeral Private Key, RemoteNode's Ephemeral Public Key), 32)
DH_KL = 32 (for X25519), or 56 (for X448)
DSA_KL = 32 (for Ed25519), or 57 (for Ed448)
DSA_SL = 64 (for Ed25519), or 114 (for Ed448)
Encryption = IV(12 Bytes) | Tag(16 Bytes) | Content (AES-GCM, key=SharedKey)

- Hello
Length(B)	Type			Value
-----------------------------------------------
1			Command			0 (Hello)
16			NodeId			LocalNode
1			Boolean			Need Reply
*			Prefix
*			Message Content

- ReplyHello
Length(B)	Type			Value
-----------------------------------------------
1			Command			1 (ReplyHello)
16			NodeId			LocalNode
*			Prefix
*			Message Content

- FindNode
Length(B)	Type			Value
-----------------------------------------------
1			Command			2 (FindNode)
16			NodeId			RemoteNode

- ReplyFindNode
Length(B)	Type			Value
-----------------------------------------------
1			Command			1 (ReplyFindNode)
16			NodeId			LocalNode

- ConnectNode
Length(B)	Type			Value
-----------------------------------------------
1			Command			4 (ConnectNode)
16			NodeId			RemoteNode
16			NodeId			LocalNode
DH_KL		Bytes			LocalNode's Ephemeral Public Key
4			TickCount		LocalNode

- ReplyConnectNode
Length(B)	Type			Value
-----------------------------------------------
1			Command			5 (ReplyConnectNode)
16			NodeId			RemoteNode
DSA_KL		Bytes			LocalNode's Public Key
DH_KL		Bytes			LocalNode's Ephemeral Public Key
*			Bytes			Encryption
-----	Encrypted Content ---------------------
DSA_SL		Bytes			EdDSA(LocalNode's Private Key, LocalNode's Ephemeral Public Key | RemoteNode's Ephemeral Public Key)
4			TickCount		RemoteNode
*			Message Content

- Ping
Length(B)	Type			Value
-----------------------------------------------
1			Command			6 (Ping)
4			ShortNodeId		RemoteNode
4			TickCount		LocalNode

- ReplyPing
Length(B)	Type			Value
-----------------------------------------------
1			Command			7 (ReplyPing)
16			NodeId			LocalNode
8			Bytes			LocalNode's Ephemeral Public Key (Prefix)
4			TickCount		RemoteNode

- Broadcast
Length(B)	Type			Value
-----------------------------------------------
1			Command			8 (Broadcast)
16			NodeId			LocalNode
*			Content

- Datagram
Length(B)	Type			Value
-----------------------------------------------
1			Command			9 (Datagram)
4			ShortNodeId		RemoteNode
16			NodeId			LocalNode
*			Content

- EncryptedDatagram
Length(B)	Type			Value
-----------------------------------------------
1			Command			9 (Datagram)
4			ShortNodeId		RemoteNode
16			NodeId			LocalNode
*			Bytes			Encryption


---------------- TCP Commands -----------------

- Init
Length(B)	Type			Value
-----------------------------------------------
1			Command			0 (Init)
4			ShortNodeId		RemoteNode
16			NodeId			LocalNode
2			Port			LocalNode

- ReplyInit
Length(B)	Type			Value
-----------------------------------------------
1			Command			1 (ReplyInit)

- Message
Length(B)	Type			Value
-----------------------------------------------
1			Command			2 (Message)
*			Bytes			Encryption

- ReplyMessage
Length(B)	Type			Value
-----------------------------------------------
1			Command			3 (ReplyMessage)
*			Bytes			Encryption

*/

#ifdef USE_CURVE448
#define EdDSA Ed448
#define EdDH X448
#else
#define EdDSA Ed25519
#define EdDH X25519
#endif

namespace slib
{

	namespace {

		enum class Command
		{
			Unknown = -1,
			Hello = 0,
			ReplyHello = 1,
			FindNode = 2,
			ReplyFindNode = 3,
			ConnectNode = 4,
			ReplyConnectNode = 5,
			Ping = 6,
			ReplyPing = 7,
			Broadcast = 8,
			Datagram = 9,
			EncryptedDatagram = 10
		};

		enum class TcpCommand
		{
			Unknown = -1,
			Init = 0,
			ReplyInit = 1,
			Message = 2,
			ReplyMessage = 3
		};

		static void DeriveKey(const void* localPrivateKey, const void* remotePublicKey, sl_uint8 out[32])
		{
			auto key = EdDH::getSharedKey(localPrivateKey, remotePublicKey);
			HKDF_SHA256::generateKey(key.data, sizeof(key), out, 32);
		}

		SLIB_INLINE static sl_uint32 GetCurrentTick()
		{
			return (sl_uint32)(System::getHighResolutionTickCount());
		}

		SLIB_INLINE static sl_bool CheckDelay(sl_uint32 tickOld, sl_uint32 tickNew, sl_uint32 timeout)
		{
			return tickNew >= tickOld && tickNew < tickOld + timeout;
		}

		static void ReplyErrorResponse(const Function<void(P2PResponse&)>& callback)
		{
			P2PResponse response;
			callback(response);
		}

		class P2PSocketImpl;
		class Node;

		typedef Function<void(Node*, void*)> NodeCallback;

		class NodeCallbackContainer
		{
		public:
			NodeCallbackContainer() {}

			NodeCallbackContainer(NodeCallback&& callback) : m_callback(Move(callback)) {}

			~NodeCallbackContainer()
			{
				m_callback(sl_null, sl_null);
			}

			SLIB_DEFINE_CLASS_DEFAULT_MEMBERS_INLINE(NodeCallbackContainer)

		public:
			void success(Node* node, void* param)
			{
				m_callback(node, param);
				m_callback.setNull();
			}

		protected:
			NodeCallback m_callback;

		};
		
		class Connection : public CRef
		{
		public:
			P2PConnectionType m_type;
			sl_uint32 m_timeLastPing = 0;
			sl_uint32 m_delayLastPing = 0;

		public:
			Connection(P2PConnectionType type): m_type(type) {}

		};

		class DirectConnection;

		class Node : public CRef
		{
		public:
			P2PNodeId m_id;
			Bytes<EdDSA::KeySize> m_publicKey;
			Bytes<EdDH::KeySize> m_remoteEphemeralPublicKey;
			Bytes<EdDH::KeySize> m_localEphemeralPublicKey;
			sl_uint8 m_encryptionKey[32];
			sl_bool m_flagInvalidEncryptionKey = sl_true;

			AtomicRef<Connection> m_connectionDefault;
			CHashMap< IPv4Address, Ref<DirectConnection> > m_connectionsDirect;

		public:
			Node(const sl_uint8 publicKey[EdDSA::KeySize]): m_publicKey(publicKey), m_id(publicKey) {}

		public:
			void updateRemoteEphemeralKey(const sl_uint8 key[EdDH::KeySize])
			{
				if (Base::equalsMemory(m_remoteEphemeralPublicKey.data, key, EdDH::KeySize)) {
					return;
				}
				m_remoteEphemeralPublicKey.setData(key);
				m_flagInvalidEncryptionKey = sl_true;
			}

			void updateEncryptionKey(const sl_uint8 localEphemeralPrivateKey[EdDH::KeySize], const sl_uint8 localEphemeralPublicKey[EdDH::KeySize])
			{
				sl_bool flagUpdate = sl_false;
				if (m_flagInvalidEncryptionKey) {
					m_flagInvalidEncryptionKey = sl_false;
					flagUpdate = sl_true;
				}
				if (!(Base::equalsMemory(m_localEphemeralPublicKey.data, localEphemeralPublicKey, EdDH::KeySize))) {
					m_localEphemeralPublicKey.setData(localEphemeralPublicKey);
					flagUpdate = sl_true;
				}
				if (flagUpdate) {
					DeriveKey(localEphemeralPrivateKey, m_remoteEphemeralPublicKey.data, m_encryptionKey);
				}
			}

		};

		class DirectConnection : public Connection
		{
		public:
			SocketAddress m_address;

		public:
			DirectConnection(const SocketAddress& address): Connection(P2PConnectionType::Direct), m_address(address) {}

		};

		class TcpSocket : public CRef
		{
		public:
			Ref<AsyncSocketStream> m_stream;

		public:
			template <class STREAM>
			TcpSocket(STREAM&& _stream): m_stream(Forward<STREAM>(_stream)) {}

		};

		class TcpServerSocket : public TcpSocket
		{
		public:
			SocketAddress m_remoteAddress;
			P2PNodeId m_remoteId;
			sl_uint16 m_remoteActor = 0;

		public:
			template <class STREAM>
			TcpServerSocket(STREAM&& stream, const SocketAddress& _remoteAddress): TcpSocket(Forward<STREAM>(stream)), m_remoteAddress(_remoteAddress) {}

		};

		class TcpClientSocket : public TcpSocket
		{
		public:
			template <class STREAM>
			TcpClientSocket(STREAM&& stream): TcpSocket(Forward<STREAM>(stream)) {}

		};

		class P2PSocketImpl : public P2PSocket
		{
		public:
			typedef P2PSocketImpl This;

			P2PNodeId m_localNodeId;
			Bytes<EdDSA::KeySize> m_localKey;
			Bytes<EdDSA::KeySize> m_localPublicKey;

			sl_uint32 m_connectionTimeout = 0;
			sl_uint32 m_findTimeout = 0;
			sl_uint32 m_messageSegmentSize = 0;
			sl_uint32 m_maximumMessageSize = 0;

			Memory m_helloPrefix;

			Function<void(P2PSocket*, P2PRequest&)> m_onReceiveHello;
			Function<void(P2PSocket*, P2PRequest&)> m_onConnectNode;
			Function<void(P2PSocket*, P2PRequest&, P2PResponse&)> m_onReceiveMessage;
			Function<void(P2PSocket*, P2PRequest&)> m_onReceiveBroadcast;
			Function<void(P2PSocket*, P2PRequest&)> m_onReceiveDatagram;
			Function<void(P2PSocket*, P2PRequest&)> m_onReceiveEncryptedDatagram;

			Bytes<EdDH::KeySize> m_ephemeralKey;
			Bytes<EdDH::KeySize> m_ephemeralPublicKey;

			sl_uint8 m_helloMessage[1024];
			sl_uint32 m_sizeHelloMessage;
			sl_uint8 m_connectMessage[2048];
			sl_uint32 m_sizeConnectMessage;

			sl_bool m_flagClosed = sl_false;

			IPAddress m_bindAddress;
			sl_uint16 m_portLobby = 0;
			sl_uint16 m_portActor = 0;
			sl_uint16 m_portActorMax = 0;
			List< Pair<sl_uint32, IPv4Address> > m_broadcasters;

			Ref<AsyncUdpSocket> m_socketUdpLobby;
			Ref<AsyncUdpSocket> m_socketUdpActor;
			Ref<AsyncTcpServer> m_serverTcp;

			ExpiringMap< TcpSocket*, Ref<TcpSocket> > m_mapTcpSockets;
			ExpiringMap< DirectConnection*, Ref<AsyncSocketStream> > m_mapIdleTcpStreams;

			Ref<AsyncIoLoop> m_ioLoop;
			Ref<DispatchLoop> m_dispatchLoop;
			Ref<Timer> m_timerHello;
			Ref<Timer> m_timerUpdateEphemeralKey;

			ExpiringMap< P2PNodeId, Ref<Node> > m_mapNodes;
			ExpiringMap<P2PNodeId, NodeCallbackContainer> m_mapFindCallbacks;

			sl_uint16 m_portLocalhostMax = 0;
			AtomicList< Pair<sl_uint32, IPv4Address> > m_lastBroadcasters;
			sl_uint32 m_lastTickUpdateBroadcasters = 0;

		public:
			P2PSocketImpl()
			{
			}

			~P2PSocketImpl()
			{
				close();
			}

		public:
			static Ref<This> open(P2PSocketParam& param)
			{
				if (!(param.port) || !(param.portCount)) {
					SLIB_STATIC_STRING(err, "port or portCount is invalid")
					param.errorText = err;
					return sl_null;
				}

				if (param.key.isNull() || param.key.getSize() != EdDSA::KeySize) {
					param.key = Memory::create(EdDSA::KeySize);
					if (param.key.isNull()) {
						SLIB_STATIC_STRING(err, "Lack of memory")
						param.errorText = err;
						return sl_null;
					}
					Math::randomMemory(param.key.getData(), EdDSA::KeySize);
					param.flagGeneratedKey = sl_true;
				}

				SocketAddress bindAddress;
				bindAddress.ip = param.bindAddress;
				bindAddress.port = param.port;
				Socket socketLobby = _openLobby(bindAddress);
				if (socketLobby.isNone()) {
					SLIB_STATIC_STRING(err, "Failed to bind lobby socket")
					param.errorText = err;
					return sl_null;
				}
				Socket socketUdp, socketTcp;
				{
					param.boundPort = 0;
					for (sl_uint16 i = 1; i <= param.portCount; i++) {
						bindAddress.port = param.port + i;
						if (_openPorts(bindAddress, socketUdp, socketTcp)) {
							param.boundPort = param.port + i;
							break;
						}
					}
					if (!(param.boundPort)) {
						SLIB_STATIC_STRING(err, "Failed to bind the actor sockets")
						param.errorText = err;
						return sl_null;
					}
				}

				Ref<AsyncIoLoop> ioLoop = AsyncIoLoop::create(sl_false);
				if (ioLoop.isNull()) {
					SLIB_STATIC_STRING(err, "Failed to create I/O` loop")
					param.errorText = err;
					return sl_null;
				}
				Ref<DispatchLoop> dispatchLoop = DispatchLoop::create(sl_false);
				if (dispatchLoop.isNull()) {
					SLIB_STATIC_STRING(err, "Failed to create dispatch loop")
					param.errorText = err;
					return sl_null;
				}

				Ref<This> ret = new This;
				if (ret.isNotNull()) {
					ret->m_ioLoop = Move(ioLoop);
					ret->m_dispatchLoop = Move(dispatchLoop);
					if (!(ret->_initialize(param, socketLobby, socketUdp, socketTcp))) {
						return sl_null;
					}
					if (param.flagAutoStart) {
						if (ret->start()) {
							return ret;
						} else {
							SLIB_STATIC_STRING(err, "Failed to start P2P socket")
							param.errorText = err;
						}
					} else {
						return ret;
					}
				} else {
					SLIB_STATIC_STRING(err, "Failed to create P2P socket")
					param.errorText = err;
				}
				return sl_null;
			}

		public:
			sl_bool _initialize(P2PSocketParam& param, Socket& socketLobby, Socket& socketUdp, Socket& socketTcp)
			{
				m_localKey.setData(param.key.getData());
				m_localPublicKey = EdDSA::getPublicKey(m_localKey.data);
				m_localNodeId.setData(m_localPublicKey.data);

				if (param.helloInterval) {
					if (param.helloInterval < 100) {
						param.helloInterval = 100;
					}
				}
				if (param.ephemeralKeyDuration) {
					if (param.ephemeralKeyDuration < 60000) {
						param.ephemeralKeyDuration = 60000;
					}
				}

				if (param.findTimeout < 1000) {
					param.findTimeout = 1000;
				}
				m_findTimeout = param.findTimeout;

				if (param.connectionTimeout < 10000) {
					param.connectionTimeout = 10000;
				}
				m_connectionTimeout = param.connectionTimeout;

				m_messageSegmentSize = param.messageSegmentSize;
				if (param.maximumMessageSize < 1) {
					param.maximumMessageSize = 1;
				}
				m_maximumMessageSize = param.maximumMessageSize;

				if (param.helloPrefix.size) {
					m_helloPrefix = Memory::create(param.helloPrefix.data, param.helloPrefix.size);
				}
				m_onReceiveHello = param.onReceiveHello;
				m_onConnectNode = param.onConnectNode;
				m_onReceiveMessage = param.onReceiveMessage;
				m_onReceiveBroadcast = param.onReceiveBroadcast;
				m_onReceiveDatagram = param.onReceiveDatagram;
				m_onReceiveEncryptedDatagram = param.onReceiveEncryptedDatagram;

				setHelloMessage(param.helloMessage);
				setConnectMessage(param.connectMessage);

				if (param.bindAddress.isNone() && param.broadcasters.isNull()) {
					_updateBroadcasters();
				}

				m_mapNodes.setupTimer(param.connectionTimeout, m_dispatchLoop);
				m_mapFindCallbacks.setupTimer(param.findTimeout, m_dispatchLoop);

				// Initialize UDP Socket
				{
					m_bindAddress = param.bindAddress;
					m_portLobby = param.port;
					m_portActor = param.boundPort;
					m_portActorMax = param.port + param.portCount;
					m_portLocalhostMax = param.boundPort - 1;
					m_broadcasters = param.broadcasters;

					AsyncUdpSocketParam udpParam;
					udpParam.ioLoop = m_ioLoop;
					udpParam.flagSendingBroadcast = sl_true;
					udpParam.socket = Move(socketUdp);
					udpParam.onReceive = [this](AsyncUdpSocket*, sl_uint32 ifIndex, IPAddress& dst, SocketAddress& src, void* data, sl_uint32 size) {
						_processReceivedUdp(ifIndex, src, (sl_uint8*)data, size);
					};
					m_socketUdpActor = AsyncUdpSocket::create(udpParam);
					if (m_socketUdpActor.isNull()) {
						return sl_false;
					}

					udpParam.socket = Move(socketLobby);
					udpParam.onReceive = [this](AsyncUdpSocket*, sl_uint32 ifIndex, IPAddress& dst, SocketAddress& src, void* data, sl_uint32 size) {
						_processReceivedUdp(ifIndex, src, (sl_uint8*)data, size);
					};
					m_socketUdpLobby = AsyncUdpSocket::create(udpParam);
					if (m_socketUdpLobby.isNull()) {
						return sl_false;
					}
				}

				// Initialize TCP Server
				{
					m_mapTcpSockets.setupTimer(param.connectionTimeout, m_dispatchLoop);
					m_mapIdleTcpStreams.setupTimer(param.connectionTimeout, m_dispatchLoop);
					AsyncTcpServerParam serverParam;
					serverParam.ioLoop = m_ioLoop;
					serverParam.onAccept = [this](AsyncTcpServer*, Socket& socket, const SocketAddress& address) {
						_onAcceptTcpConnection(socket, address);
					};
					serverParam.socket = Move(socketTcp);
					m_serverTcp = AsyncTcpServer::create(serverParam);
					if (m_serverTcp.isNull()) {
						return sl_false;
					}
				}

				// Initialize Hello Timer
				if (param.helloInterval) {
					m_timerHello = Timer::createWithDispatcher(m_dispatchLoop, [this](Timer*) {
						_sendHello(sl_null, sl_false);
					}, param.helloInterval);
					if (m_timerHello.isNull()) {
						return sl_false;
					}
					m_dispatchLoop->dispatch([this]() {
						_sendHello(sl_null, sl_true);
					});
				}

				// Initialize Update Ephemeral Key Timer
				if (param.ephemeralKeyDuration) {
					m_timerUpdateEphemeralKey = Timer::createWithDispatcher(m_dispatchLoop, [this](Timer*) {
						_updateEphemeralKey();
					}, param.ephemeralKeyDuration);
					if (m_timerUpdateEphemeralKey.isNull()) {
						return sl_false;
					}
				}

				return sl_true;
			}

			static Socket _openLobby(const SocketAddress& bindAddress)
			{
				Socket socket = Socket::openUdp();
				if (socket.isNotNone()) {
					socket.setReusingAddress();
					socket.setReusingPort();
					if (socket.bind(bindAddress)) {
						return socket;
					}
				}
				return SLIB_SOCKET_INVALID_HANDLE;
			}

			static sl_bool _openPorts(const SocketAddress& bindAddress, Socket& udp, Socket& tcp)
			{
				udp = Socket::openUdp(bindAddress);
				if (udp.isNone()) {
					return sl_false;
				}
				tcp = Socket::openTcp(bindAddress);
				if (tcp.isNone()) {
					return sl_false;
				}
				return sl_true;
			}

		public:
#ifdef LOG_COMMANDS
			void _logReceiveCommand(Command command, const SocketAddress& address)
			{
				const char* szCommand = sl_null;
				switch (command) {
					case Command::Hello:
						szCommand = "Hello";
						break;
					case Command::ReplyHello:
						szCommand = "ReplyHello";
						break;
					case Command::FindNode:
						szCommand = "FindNode";
						break;
					case Command::ReplyFindNode:
						szCommand = "ReplyFindNode";
						break;
					case Command::ConnectNode:
						szCommand = "ConnectNode";
						break;
					case Command::ReplyConnectNode:
						szCommand = "ReplyConnectNode";
						break;
					case Command::Ping:
						szCommand = "Ping";
						break;
					case Command::ReplyPing:
						szCommand = "ReplyPing";
						break;
					case Command::Broadcast:
						szCommand = "Broadcast";
						break;
					case Command::Datagram:
						szCommand = "Datagram";
						break;
					default:
						szCommand = "Unknown";
						break;
				}
				Log("P2P", "Received Command: %s, Sender=%s", szCommand, address.toString());
			}
#endif

			void _sendUdp(const SocketAddress& address, sl_uint8* buf, sl_size size)
			{
				m_socketUdpActor->sendTo(address, buf, size);
			}

			void _sendBroadcast(sl_uint32 ifIndex, const IPv4Address& local, sl_uint8* buf, sl_size size)
			{
				SocketAddress targetAddress;
				targetAddress.ip.setIPv4(IPv4Address::Broadcast);
				targetAddress.port = m_portLobby;
				if (m_socketUdpActor->sendTo(ifIndex, local, targetAddress, buf, size)) {
					return;
				}
				if (Socket::getLastError() == SocketError::NotSupported) {
					auto socket = Socket::openUdp();
					SocketAddress bindAddress;
					bindAddress.port = m_portActor;
					bindAddress.ip = local;
					if (socket.bind(bindAddress)) {
						socket.setSendingBroadcast();
						socket.sendTo(targetAddress, buf, size);
					}
				}
			}

			void _sendBroadcast(sl_uint32 ifIndex, sl_uint8* buf, sl_size size)
			{
				if (m_bindAddress.isNotNone()) {
					if (m_bindAddress.isIPv4()) {
						IPv4Address ip = m_bindAddress.getIPv4();
						if (ip.isHost()) {
							SocketAddress targetAddress;
							targetAddress.ip.setIPv4(IPv4Address::Broadcast);
							targetAddress.port = m_portLobby;
							_sendUdp(targetAddress, buf, size);
						}
					}
				} else if (m_broadcasters.isNotNull()) {
					ListElements< Pair<sl_uint32, IPv4Address> > items(m_broadcasters);
					for (sl_size i = 0; i < items.count; i++) {
						Pair<sl_uint32, IPv4Address>& item = items[i];
						if (ifIndex) {
							if (ifIndex == item.first) {
								_sendBroadcast(item.first, item.second, buf, size);
							}
						} else {
							_sendBroadcast(item.first, item.second, buf, size);
						}
					}
				} else {
					_updateBroadcasters();
					{
						ListElements< Pair<sl_uint32, IPv4Address> > items(m_lastBroadcasters);
						for (sl_size i = 0; i < items.count; i++) {
							Pair<sl_uint32, IPv4Address>& item = items[i];
							if (ifIndex) {
								if (ifIndex == item.first) {
									_sendBroadcast(item.first, item.second, buf, size);
								}
							} else {
								_sendBroadcast(item.first, item.second, buf, size);
							}
						}
					}
				}
				// Send to localhost sockets
				{
#if defined(SLIB_PLATFORM_IS_WIN32) || defined(SLIB_PLATFORM_IS_LINUX)
					_sendUdp(SocketAddress(IPv4Address(127, 255, 255, 255), m_portLobby), buf, size);
#else
					SocketAddress address;
					address.ip = IPv4Address::Loopback;
					for (sl_uint16 i = m_portLobby + 1; i <= m_portLocalhostMax; i++) {
						if (i != m_portActor) {
							address.port = i;
							_sendUdp(address, buf, size);
						}
					}
#endif
				}
			}

			void _updateBroadcasters()
			{
				sl_uint32 now = GetCurrentTick();
				if (CheckDelay(m_lastTickUpdateBroadcasters, now, DURATION_VALID_BROADCASTERS)) {
					return;
				}
				List< Pair<sl_uint32, IPv4Address> > broadcasters;
				ListElements<NetworkInterfaceInfo> interfaces(Network::getInterfaces());
				for (sl_size i = 0; i < interfaces.count; i++) {
					NetworkInterfaceInfo& iface = interfaces[i];
					if (iface.flagUp && !(iface.flagLoopback)) {
						ListElements<IPv4AddressInfo> addresses(iface.addresses_IPv4);
						for (sl_size j = 0; j < addresses.count; j++) {
							IPv4Address& ip = addresses[j].address;
							broadcasters.add_NoLock(iface.index, ip);
						}
					}
				}
				m_lastBroadcasters = Move(broadcasters);
				m_lastTickUpdateBroadcasters = now;
			}

			sl_bool _isValidBroadcastSender(sl_uint32 ifIndex, const SocketAddress& address)
			{
				IPv4Address ip = address.ip.getIPv4();
				if (ip.isZero()) {
					return sl_false;
				}
				if (ip.isLoopback()) {
					return m_portActor != address.port;
				}
				if (m_bindAddress.isNotNone()) {
					return m_bindAddress.getIPv4() != ip;
				}
				if (m_broadcasters.isNotNull()) {
					ListElements< Pair<sl_uint32, IPv4Address> > items(m_broadcasters);
					for (sl_size i = 0; i < items.count; i++) {
						Pair<sl_uint32, IPv4Address>& item = items[i];
						if (ifIndex) {
							if (item.first == ifIndex && item.second == ip) {
								return sl_false;
							}
						} else {
							if (item.second == ip) {
								return sl_false;
							}
						}
					}
				}
				if (CheckDelay(m_lastTickUpdateBroadcasters, GetCurrentTick(), DURATION_VALID_BROADCASTERS * 2)) {
					ListElements< Pair<sl_uint32, IPv4Address> > items(m_lastBroadcasters);
					for (sl_size i = 0; i < items.count; i++) {
						Pair<sl_uint32, IPv4Address>& item = items[i];
						if (ifIndex) {
							if (item.first == ifIndex && item.second == ip) {
								return sl_false;
							}
						} else {
							if (item.second == ip) {
								return sl_false;
							}
						}
					}
				}
				return sl_true;
			}

			void _processReceivedUdp(sl_uint32 ifIndex, const SocketAddress& address, sl_uint8* packet, sl_uint32 sizePacket)
			{
				if (!sizePacket) {
					return;
				}
				Command cmd = (Command)(*packet);
				switch (cmd) {
					case Command::Hello:
					case Command::FindNode:
					case Command::Broadcast:
						if (!(_isValidBroadcastSender(ifIndex, address))) {
							return;
						}
						break;
					default:
						break;
				};
				LOG_RECEIVE_COMMAND(cmd, address)
				switch (cmd) {
					case Command::Hello:
						_onReceiveHello(ifIndex, address, packet, sizePacket);
						break;
					case Command::ReplyHello:
						_onReceiveReplyHello(ifIndex, address, packet, sizePacket);
						break;
					case Command::FindNode:
						_onReceiveFindNode(ifIndex, address, packet, sizePacket);
						break; 
					case Command::ReplyFindNode:
						_onReceiveReplyFindNode(ifIndex, address, packet, sizePacket);
						break;
					case Command::ConnectNode:
						_onReceiveConnectNode(ifIndex, address, packet, sizePacket);
						break;
					case Command::ReplyConnectNode:
						_onReceiveReplyConnectNode(ifIndex, address, packet, sizePacket);
						break;
					case Command::Ping:
						_onReceivePing(ifIndex, address, packet, sizePacket);
						break;
					case Command::ReplyPing:
						_onReceiveReplyPing(ifIndex, address, packet, sizePacket);
						break;
					case Command::Broadcast:
						_onReceiveBroadcast(ifIndex, address, packet, sizePacket);
						break;
					case Command::Datagram:
						_onReceiveDatagram(ifIndex, address, packet, sizePacket);
						break;
					case Command::EncryptedDatagram:
						_onReceiveEncryptedDatagram(ifIndex, address, packet, sizePacket);
						break;
					default:
						break;
				}
			}

			void _sendHello(const SocketAddress* address, sl_bool flagNeedReply)
			{
				sl_uint8 packet[18 + sizeof(m_helloMessage)];
				*packet = (sl_uint8)(Command::Hello);
				Base::copyMemory(packet + 1, m_localNodeId.data, sizeof(P2PNodeId));
				packet[17] = flagNeedReply ? 1 : 0;
				sl_uint32 sizeMessage = m_sizeHelloMessage;
				if (sizeMessage > sizeof(m_helloMessage)) {
					sizeMessage = sizeof(m_helloMessage);
				}
				Base::copyMemory(packet + 18, m_helloMessage, sizeMessage);
				if (address) {
					_sendUdp(*address, packet, 18 + sizeMessage);
				} else {
					_sendBroadcast(0, packet, 18 + sizeMessage);
				}
			}

			void _onReceiveHello(sl_uint32 ifIndex, const SocketAddress& address, sl_uint8* packet, sl_uint32 sizePacket)
			{
				if (sizePacket < 18) {
					return;
				}
				if (Base::equalsMemory(m_localNodeId.data, packet + 1, sizeof(P2PNodeId))) {
					return;
				}
				P2PRequest message(packet + 18, sizePacket - 18);
				message.senderId = P2PNodeId(packet + 1);
				message.connectionType = P2PConnectionType::Direct;
				message.interfaceIndex = ifIndex;
				message.remoteAddress = address;
				_onReceiveHello(message, packet[17] != 0);
			}

			void _onReceiveHello(P2PRequest& message, sl_bool flagNeedReply)
			{
				_onReceiveHelloMessage(message);
				if (m_timerHello.isNull()) {
					_sendHello(&(message.remoteAddress), sl_false);
				} else if (flagNeedReply) {
					_sendReplyHello(message.remoteAddress);
				}
				Ref<Node> node = _getNode(message.senderId);
				if (node.isNotNull()) {
					if (_findConnection(node.get(), message.remoteAddress).isNotNull()) {
						_sendPing(message.remoteAddress, message.senderId);
					}
				}
			}

			void _sendReplyHello(const SocketAddress& address)
			{
				sl_uint8 packet[17 + sizeof(m_helloMessage)];
				*packet = (sl_uint8)(Command::ReplyHello);
				Base::copyMemory(packet + 1, m_localNodeId.data, sizeof(P2PNodeId));
				sl_uint32 sizeMessage = m_sizeHelloMessage;
				if (sizeMessage > sizeof(m_helloMessage)) {
					sizeMessage = sizeof(m_helloMessage);
				}
				Base::copyMemory(packet + 17, m_helloMessage, sizeMessage);
				_sendUdp(address, packet, 17 + sizeMessage);
			}

			void _onReceiveReplyHello(sl_uint32 ifIndex, const SocketAddress& address, sl_uint8* packet, sl_uint32 sizePacket)
			{
				if (sizePacket < 17) {
					return;
				}
				if (Base::equalsMemory(m_localNodeId.data, packet + 1, sizeof(P2PNodeId))) {
					return;
				}
				P2PRequest message(packet + 17, sizePacket - 17);
				message.senderId = P2PNodeId(packet + 1);
				message.connectionType = P2PConnectionType::Direct;
				message.interfaceIndex = ifIndex;
				message.remoteAddress = address;
				_onReceiveHelloMessage(message);
			}

			void _onReceiveHelloMessage(P2PRequest& message)
			{
				if (message.remoteAddress.ip.getIPv4().isLoopback()) {
					if (message.remoteAddress.port > m_portLocalhostMax && message.remoteAddress.port <= m_portActorMax) {
						m_portLocalhostMax = message.remoteAddress.port;
					}
				}
				if (m_helloPrefix) {
					MemoryView prefix(m_helloPrefix);
					if (prefix.size > message.size) {
						return;
					}
					if (Base::equalsMemory(prefix.data, message.data, prefix.size)) {
						message.data = ((sl_uint8*)(message.data)) + prefix.size;
						message.size -= (sl_uint32)(prefix.size);
					} else {
						return;
					}
				}
				m_onReceiveHello(this, message);
			}

			void _sendFindNode(const SocketAddress* address, const P2PNodeId& nodeId)
			{
				if (address) {
					_sendConnectNode(*address, nodeId);
				} else {
					sl_uint8 packet[17];
					*packet = (sl_uint8)(Command::FindNode);
					Base::copyMemory(packet + 1, nodeId.data, sizeof(P2PNodeId));
					_sendBroadcast(0, packet, sizeof(packet));
				}
			}

			void _onReceiveFindNode(sl_uint32 ifIndex, const SocketAddress& address, sl_uint8* packet, sl_uint32 sizePacket)
			{
				if (sizePacket != 17) {
					return;
				}
				if (!(Base::equalsMemory(m_localNodeId.data, packet + 1, sizeof(P2PNodeId)))) {
					return;
				}
				_sendReplyFindNode(address);
			}

			void _sendReplyFindNode(const SocketAddress& address)
			{
				sl_uint8 packet[17];
				*packet = (sl_uint8)(Command::ReplyFindNode);
				Base::copyMemory(packet + 1, m_localNodeId.data, sizeof(P2PNodeId));
				_sendUdp(address, packet, sizeof(packet));
			}

			void _onReceiveReplyFindNode(sl_uint32 ifIndex, const SocketAddress& address, sl_uint8* packet, sl_uint32 sizePacket)
			{
				if (sizePacket != 17) {
					return;
				}
				if (Base::equalsMemory(m_localNodeId.data, packet + 1, sizeof(P2PNodeId))) {
					return;
				}
				P2PNodeId targetId(packet + 1);
				_sendConnectNode(address, targetId);
			}

			void _sendConnectNode(const SocketAddress& address, const P2PNodeId& remoteId)
			{
				sl_uint8 packet[37 + EdDH::KeySize];
				*packet = (sl_uint8)(Command::ConnectNode);
				Base::copyMemory(packet + 1, remoteId.data, sizeof(P2PNodeId));
				Base::copyMemory(packet + 17, m_localNodeId.data, sizeof(P2PNodeId));
				Base::copyMemory(packet + 33, m_ephemeralPublicKey.data, EdDH::KeySize);
				MIO::writeUint32LE(packet + (33 + EdDH::KeySize), GetCurrentTick());
				_sendUdp(address, packet, sizeof(packet));
			}

			void _onReceiveConnectNode(sl_uint32 ifIndex, const SocketAddress& address, sl_uint8* packet, sl_uint32 sizePacket)
			{
				if (sizePacket != 37 + EdDH::KeySize) {
					return;
				}
				if (!(Base::equalsMemory(m_localNodeId.data, packet + 1, sizeof(P2PNodeId)))) {
					return;
				}
				if (Base::equalsMemory(m_localNodeId.data, packet + 17, sizeof(P2PNodeId))) {
					return;
				}
				P2PNodeId remoteId(packet + 17);
				Bytes<EdDH::KeySize> remoteEphemeralKey(packet + 33);
				sl_uint32 remoteTick = MIO::readUint32LE(packet + (33 + EdDH::KeySize));
				_sendReplyConnectNode(address, remoteId, remoteEphemeralKey.data, remoteTick);
			}

			void _sendReplyConnectNode(const SocketAddress& address, const P2PNodeId& remoteId, const sl_uint8 remoteEphemeralKey[EdDH::KeySize], sl_uint32 remoteTick)
			{
				const sl_uint32 sizeHeader = 17 + EdDSA::KeySize + EdDH::KeySize;
				const sl_uint32 sizeContentHeader = EdDSA::SignatureSize + 4;
				sl_uint8 packet[sizeHeader + 28 + sizeContentHeader + sizeof(m_connectMessage)];
				*packet = (sl_uint8)(Command::ReplyConnectNode);
				Base::copyMemory(packet + 1, remoteId.data, sizeof(P2PNodeId));
				Base::copyMemory(packet + 17, m_localPublicKey.data, EdDSA::KeySize);
				Base::copyMemory(packet + (17 + EdDSA::KeySize), m_ephemeralPublicKey.data, EdDH::KeySize);
				Math::randomMemory(packet + sizeHeader, 12);
				const sl_uint32 posContent = sizeHeader + 28;
				sl_uint8 sts[EdDH::KeySize * 2];
				Base::copyMemory(sts, m_ephemeralPublicKey.data, EdDH::KeySize);
				Base::copyMemory(sts + EdDH::KeySize, remoteEphemeralKey, EdDH::KeySize);
				EdDSA::sign(m_localKey.data, m_localPublicKey.data, sts, sizeof(sts), packet + posContent);
				MIO::writeUint32LE(packet + (posContent + EdDSA::SignatureSize), remoteTick);
				sl_uint32 sizeMessage = m_sizeConnectMessage;
				if (sizeMessage > sizeof(m_connectMessage)) {
					sizeMessage = sizeof(m_connectMessage);
				}
				Base::copyMemory(packet + (posContent + sizeContentHeader), m_connectMessage, sizeMessage);
				AES_GCM cryptor;
				sl_uint8 key[32];
				_deriveEncryptionKey(remoteEphemeralKey, key);
				cryptor.setKey(key, sizeof(key));
				cryptor.start(packet + sizeHeader, 12);
				cryptor.encrypt(packet + posContent, packet + posContent, sizeContentHeader + sizeMessage);
				cryptor.finish(packet + (sizeHeader + 12));
				_sendUdp(address, packet, sizeHeader + 28 + sizeContentHeader + sizeMessage);
			}

			void _onReceiveReplyConnectNode(sl_uint32 ifIndex, const SocketAddress& address, sl_uint8* packet, sl_uint32 sizePacket)
			{
				const sl_uint32 sizeHeader = 17 + EdDSA::KeySize + EdDH::KeySize;
				const sl_uint32 sizeContentHeader = EdDSA::SignatureSize + 4;
				if (sizePacket < sizeHeader + 28 + sizeContentHeader) {
					return;
				}
				if (!(Base::equalsMemory(m_localNodeId.data, packet + 1, sizeof(P2PNodeId)))) {
					return;
				}
				Bytes<EdDSA::KeySize> remoteKey(packet + 17);
				Bytes<EdDH::KeySize> remoteEphemeralKey(packet + (17 + EdDSA::KeySize));
				sl_uint8 key[32];
				_deriveEncryptionKey(remoteEphemeralKey.data, key);
				AES_GCM decryptor;
				decryptor.setKey(key, sizeof(key));
				const sl_uint32 posContent = sizeHeader + 28;
				decryptor.start(packet + sizeHeader, 12); // iv
				decryptor.decrypt(packet + posContent, packet + posContent, sizePacket - posContent);
				if (!(decryptor.finishAndCheckTag(packet + (sizeHeader + 12)))) {
					return;
				}
				Bytes<EdDSA::SignatureSize> signature(packet + posContent);
				sl_uint32 timeOld = MIO::readUint32LE(packet + (posContent + EdDSA::SignatureSize));
				sl_uint32 timeNew = GetCurrentTick();
				if (!(CheckDelay(timeOld, timeNew, m_findTimeout))) {
					return;
				}
				MemoryView msg(packet + (posContent + sizeContentHeader), sizePacket - (posContent + sizeContentHeader));
				_onReceiveReplyConnectDirectConnection(ifIndex, address, remoteKey.data, remoteEphemeralKey.data, signature.data, msg, timeNew, timeNew - timeOld);
			}

			void _onReceiveReplyConnectDirectConnection(sl_uint32 ifIndex, const SocketAddress& address, const sl_uint8 remoteKey[EdDSA::KeySize], const sl_uint8 remoteEphemeralKey[EdDH::KeySize], const sl_uint8 signature[EdDSA::SignatureSize], const MemoryView& msg, sl_uint32 tick, sl_uint32 delay)
			{
				sl_uint8 sts[EdDH::KeySize * 2];
				Base::copyMemory(sts, remoteEphemeralKey, EdDH::KeySize);
				Base::copyMemory(sts + EdDH::KeySize, m_ephemeralPublicKey.data, EdDH::KeySize);
				if (!(EdDSA::verify(remoteKey, sts, sizeof(sts), signature))) {
					return;
				}
				Ref<Node> node = _createNode(remoteKey);
				if (node.isNotNull()) {
					node->updateRemoteEphemeralKey(remoteEphemeralKey);
					P2PRequest message(msg.data, msg.size);
					message.senderId.setData(remoteKey);
					message.connectionType = P2PConnectionType::Direct;
					message.interfaceIndex = ifIndex;
					message.remoteAddress = address;
					m_onConnectNode(this, message);
					Ref<DirectConnection> connection = _createDirectConnection(node.get(), address);
					if (connection.isNotNull()) {
						connection->m_timeLastPing = tick;
						connection->m_delayLastPing = delay;
						_selectDefaultConnectionIfBetter(node.get(), connection.get());
					} else {
						node.setNull();
					}
				}
				_completeFindNodeCallbacks(node->m_id, node.get());
			}

			void _sendPing(const SocketAddress& address, const P2PNodeId& nodeId)
			{
				sl_uint8 packet[9];
				*packet = (sl_uint8)(Command::Ping);
				Base::copyMemory(packet + 1, nodeId.data, 4);
				MIO::writeUint32LE(packet + 5, GetCurrentTick());
				_sendUdp(address, packet, sizeof(packet));
			}

			void _onReceivePing(sl_uint32 ifIndex, const SocketAddress& address, sl_uint8* packet, sl_uint32 sizePacket)
			{
				if (sizePacket != 9) {
					return;
				}
				if (!(Base::equalsMemory(m_localNodeId.data, packet + 1, 4))) {
					return;
				}
				sl_uint32 remoteTick = MIO::readUint32LE(packet + 5);
				_sendReplyPing(address, remoteTick);
			}

			void _sendReplyPing(const SocketAddress& address, sl_uint32 remoteTick)
			{
				sl_uint8 packet[29];
				*packet = (sl_uint8)(Command::ReplyPing);
				Base::copyMemory(packet + 1, m_localNodeId.data, sizeof(P2PNodeId));
				Base::copyMemory(packet + 17, m_ephemeralPublicKey.data, 8);
				MIO::writeUint32LE(packet + 25, remoteTick);
				_sendUdp(address, packet, sizeof(packet));
			}

			void _onReceiveReplyPing(sl_uint32 ifIndex, const SocketAddress& address, sl_uint8* packet, sl_uint32 sizePacket)
			{
				if (sizePacket != 29) {
					return;
				}
				P2PNodeId remoteId(packet + 1);
				sl_uint32 timeOld = MIO::readUint32LE(packet + 25);
				sl_uint32 timeNew = (sl_uint32)(GetCurrentTick());
				if (!CheckDelay(timeOld, timeNew, m_connectionTimeout)) {
					return;
				}
				_onReceiveReplyPing(ifIndex, address, remoteId, packet + 17, timeNew, timeNew - timeOld);
			}

			void _onReceiveReplyPing(sl_uint32 ifIndex, const SocketAddress& address, const P2PNodeId& nodeId, sl_uint8 ephemeralKeyPrefix[8], sl_uint32 time, sl_uint32 delay)
			{
				Ref<Node> node = _getNode(nodeId);
				if (node.isNull()) {
					return;
				}
				if (!(Base::equalsMemory(node->m_remoteEphemeralPublicKey.data, ephemeralKeyPrefix, 8))) {
					_sendConnectNode(address, nodeId);
					return;
				}
				IPv4Address ip = address.ip.getIPv4();
				if (ip.isZero()) {
					return;
				}
				Ref<DirectConnection> connection = node->m_connectionsDirect.getValue(ip);
				if (connection.isNull()) {
					return;
				}
				connection->m_timeLastPing = time;
				connection->m_delayLastPing = delay;
				_selectDefaultConnectionIfBetter(node.get(), connection.get());
			}

			void _onReceiveBroadcast(sl_uint32 ifIndex, const SocketAddress& address, sl_uint8* packet, sl_uint32 sizePacket)
			{
				if (sizePacket <= 17) {
					return;
				}
				if (Base::equalsMemory(m_localNodeId.data, packet + 1, sizeof(P2PNodeId))) {
					return;
				}
				P2PRequest request(packet + 17, sizePacket - 17);
				request.senderId = P2PNodeId(packet + 1);
				request.connectionType = P2PConnectionType::Direct;
				request.interfaceIndex = ifIndex;
				request.remoteAddress = address;
				m_onReceiveBroadcast(this, request);
			}

			void _onReceiveDatagram(sl_uint32 ifIndex, const SocketAddress& address, sl_uint8* packet, sl_uint32 sizePacket)
			{
				if (sizePacket <= 21) {
					return;
				}
				if (!(Base::equalsMemory(m_localNodeId.data, packet + 1, 4))) {
					return;
				}
				if (Base::equalsMemory(m_localNodeId.data, packet + 5, sizeof(P2PNodeId))) {
					return;
				}
				P2PRequest request(packet + 21, sizePacket - 21);
				request.senderId = P2PNodeId(packet + 5);
				request.connectionType = P2PConnectionType::Direct;
				request.interfaceIndex = ifIndex;
				request.remoteAddress = address;
				m_onReceiveDatagram(this, request);
			}

			void _onReceiveEncryptedDatagram(sl_uint32 ifIndex, const SocketAddress& address, sl_uint8* packet, sl_uint32 sizePacket)
			{
				if (sizePacket <= 49) {
					return;
				}
				if (!(Base::equalsMemory(m_localNodeId.data, packet + 1, 4))) {
					return;
				}
				P2PNodeId senderId(packet + 5);
				if (Base::equalsMemory(m_localNodeId.data, senderId.data, sizeof(P2PNodeId))) {
					return;
				}
				Ref<Node> node = m_mapNodes.getValue(senderId, sl_null, sl_true);
				if (node.isNull()) {
					return;
				}
				AES_GCM decryptor;
				decryptor.setKey(node->m_encryptionKey, 32);
				decryptor.start(packet + 21, 12);
				sl_uint8* content = packet + 49;
				sl_uint32 sizeContent = sizePacket - 49;
				decryptor.decrypt(content, content, sizeContent);
				if (!(decryptor.finishAndCheckTag(packet + 33))) {
					return;
				}
				P2PRequest request(content, sizeContent);
				request.senderId = senderId;
				request.connectionType = P2PConnectionType::Direct;
				request.interfaceIndex = ifIndex;
				request.remoteAddress = address;
				m_onReceiveEncryptedDatagram(this, request);
			}

		public:
			void _onAcceptTcpConnection(Socket& _socket, const SocketAddress& address)
			{
				Ref<AsyncSocketStream> client = AsyncSocketStream::create(Move(_socket), m_ioLoop);
				if (client.isNotNull()) {
					Ref<TcpServerSocket> socket = new TcpServerSocket(Move(client), address);
					if (socket.isNotNull()) {
						m_mapTcpSockets.put(socket.get(), socket);
						_receiveTcpRequestPacket(socket);
					}
				}
			}

			void _receiveTcpRequestPacket(const Ref<TcpServerSocket>& socket)
			{
				WeakRef<TcpServerSocket> weakSocket = socket;
				ChunkIO::readAsync(socket->m_stream.get(), [this, weakSocket](AsyncStream*, Memory& content, sl_bool flagError) {
					Ref<TcpServerSocket> socket = weakSocket;
					if (socket.isNull()) {
						return;
					}
					sl_uint8* data = (sl_uint8*)(content.getData());
					sl_size size = content.getSize();
					if (!flagError && size) {
						TcpCommand command = (TcpCommand)(*data);
						data++;
						size--;
						switch (command) {
							case TcpCommand::Init:
								if (_onReceiveTcpInit(socket, data, size)) {
									return;
								}
								break;
							case TcpCommand::Message:
								if (_onReceiveTcpMessage(socket, content.sub(1))) {
									return;
								}
								break;
							default:
								break;
						}
					}
					m_mapTcpSockets.remove(socket.get());
				}, m_maximumMessageSize, m_messageSegmentSize);
			}

			void _sendTcpResponsePacket(const Ref<TcpServerSocket>& socket, const Memory& response)
			{
				WeakRef<TcpServerSocket> weakSocket = socket;
				ChunkIO::writeAsync(socket->m_stream.get(), response, [this, weakSocket](AsyncStream*, sl_bool flagError) {
					Ref<TcpServerSocket> socket = weakSocket;
					if (socket.isNull()) {
						return;
					}
					if (!flagError) {
						_receiveTcpRequestPacket(socket);
						return;
					}
					m_mapTcpSockets.remove(socket.get());
				});
			}

			sl_bool _onReceiveTcpInit(const Ref<TcpServerSocket>& socket, sl_uint8* data, sl_size size)
			{
				if (size != 22) {
					return sl_false;
				}
				if (Base::equalsMemory(m_localNodeId.data, data, 4)) {
					socket->m_remoteId = P2PNodeId(data + 4);
					socket->m_remoteActor = MIO::readUint16LE(data + 20);
					SocketAddress address(socket->m_remoteAddress.ip, socket->m_remoteActor);
					_findNode(&address, socket->m_remoteId, [this, socket](Node* node, void*) {
						if (node) {
							if (_sendTcpReplyInit(socket)) {
								return;
							}
						}
						m_mapTcpSockets.remove(socket.get());
					});
					return;
				}
			}

			sl_bool _sendTcpReplyInit(const Ref<TcpServerSocket>& socket)
			{
				sl_uint8 c = (sl_uint8)(TcpCommand::ReplyInit);
				Memory mem = Memory::create(&c, 1);
				if (mem.isNull()) {
					return sl_false;
				}
				_sendTcpResponsePacket(socket, mem);
				return sl_true;
			}

			sl_bool _onReceiveTcpMessage(const Ref<TcpServerSocket>& socket, const Memory& content)
			{
				SocketAddress address(socket->m_remoteAddress.ip, socket->m_remoteActor);
				_findNode(&address, socket->m_remoteId, [this, socket, content](Node* node, void*) {
					if (node) {
						Memory response = _processTcpMessage(node, content);
						if (response.isNotNull()) {
							_sendTcpResponsePacket(socket, response);
							return;
						}
					}
					m_mapTcpSockets.remove(socket.get());
				});
			}

			Memory _processTcpMessage(Node* node, const Memory& content)
			{
				sl_size size = content.getSize();
				node->updateEncryptionKey(m_ephemeralKey.data, m_ephemeralPublicKey.data);
				P2PResponse response;
				if (size >= 28) {
					AES_GCM decryptor;
					decryptor.setKey(node->m_encryptionKey, 32);
					sl_uint8* data = (sl_uint8*)(content.getData());
					decryptor.start(data, 12);
					data += 28;
					size -= 28;
					decryptor.decrypt(data, data, size);
					if (decryptor.finishAndCheckTag(data - 16)) {
						P2PRequest request(data, (sl_uint32)size, content.ref.get());
						request.senderId = node->m_id;
						request.connectionType = P2PConnectionType::Direct;
						m_onReceiveMessage(this, request, response);
					} else {
						return sl_null;
					}
				} else if (!size) {
					P2PRequest request;
					request.senderId = node->m_id;
					m_onReceiveMessage(this, request, response);
				} else {
					return sl_null;
				}
				if (response.size) {
					Memory memResponse = Memory::create(29 + response.size);
					if (memResponse.isNull()) {
						return sl_null;
					}
					AES_GCM encryption;
					encryption.setKey(node->m_encryptionKey, 32);
					sl_uint8* data = (sl_uint8*)(memResponse.getData());
					*(data++) = (sl_uint8)(TcpCommand::ReplyMessage);
					Math::randomMemory(data, 12); // iv
					encryption.start(data, 12);
					data += 12;
					encryption.encrypt(response.data, data + 16, response.size);
					encryption.finish(data);
					return memResponse;
				} else {
					sl_uint8 c = (sl_uint8)(TcpCommand::ReplyMessage);
					return Memory::create(&c, 1);
				}
			}

		public:
			sl_bool _sendTcpRequestPacket(const Ref<TcpClientSocket>& socket, const Memory& request, const Function<void(TcpCommand command, sl_uint8* data, sl_size size, CRef* refData)>& callback, sl_int64 tickEnd)
			{
				WeakRef<TcpServerSocket> weakSocket = socket;
				ChunkIO::writeAsync(socket->m_stream.get(), request, [this, weakSocket, callback, tickEnd](AsyncStream*, sl_bool flagError) {
					Ref<TcpServerSocket> socket = weakSocket;
					if (socket.isNotNull()) {
						if (!flagError) {
							_receiveTcpResponsePacket(socket, callback, tickEnd);
							return;
						}
						m_mapTcpSockets.remove(socket.get());
					}
					callback(TcpCommand::Unknown, sl_null, 0, sl_null);
				}, GetTimeoutFromTick(tickEnd));
			}

			void _receiveTcpResponsePacket(const Ref<TcpClientSocket>& socket, const Function<void(TcpCommand command, sl_uint8* data, sl_size size, CRef* refData)>& callback, sl_int64 tickEnd)
			{
				WeakRef<TcpServerSocket> weakSocket = socket;
				ChunkIO::readAsync(socket->m_stream.get(), [this, weakSocket, callback, tickEnd](AsyncStream*, Memory& content, sl_bool flagError) {
					Ref<TcpServerSocket> socket = weakSocket;
					if (socket.isNotNull()) {
						if (!flagError) {
							sl_size size = content.getSize();
							if (size) {
								sl_uint8* data = (sl_uint8*)(content.getData());
								callback((TcpCommand)*data, data + 1, size - 1, content.ref.get());
								return;
							}
						}
						m_mapTcpSockets.remove(socket.get());
					}
					callback(TcpCommand::Unknown, sl_null, 0, sl_null);
				}, m_maximumMessageSize, m_messageSegmentSize, GetTimeoutFromTick(tickEnd));
			}

			void _getTcpClientSocket(Node* node, DirectConnection* connection, const Function<void(This*, Node*, DirectConnection*, TcpClientSocket*)>& callback, sl_uint64 tickEnd)
			{
				Ref<AsyncSocketStream> oldStream;
				if (m_mapIdleTcpStreams.remove(connection, &oldStream)) {
					Ref<TcpClientSocket> socket = new TcpClientSocket(Move(oldStream));
					if (socket.isNotNull()) {
						m_mapTcpSockets.put(socket.get(), socket);
					}
					callback(this, node, connection, socket.get());
					return;
				}
				Ref<AsyncTcpSocket> stream = AsyncTcpSocket::create(m_ioLoop);
				if (stream.isNotNull()) {
					Ref<TcpClientSocket> socket = new TcpClientSocket(stream);
					if (stream.isNotNull()) {
						TcpInitContext context;
						context.callback = callback;
						context.node = node;
						context.connection = connection;
						context.socket = socket;
						m_mapTcpSockets.put(socket.get(), socket);
						stream->connect(connection->m_address, [this, context, tickEnd](AsyncTcpSocket*, sl_bool flagError) {
							Ref<TcpClientSocket> socket = context.socket;
							if (socket.isNotNull()) {
								if (!flagError) {
									if (_sendTcpInit(socket, context, tickEnd)) {
										return;
									}
								}
								m_mapTcpSockets.remove(socket.get());
							}
							context.callback(this, context.node.get(), context.connection.get(), sl_null);
						}, GetTimeoutFromTick(tickEnd));
						return;
					}
				}
				callback(this, node, connection, sl_null);
			}

			struct TcpInitContext
			{
				Ref<Node> node;
				Ref<DirectConnection> connection;
				WeakRef<TcpClientSocket> socket;
				Function<void(This*, Node*, DirectConnection*, TcpClientSocket*)> callback;
			};

			sl_bool _sendTcpInit(const Ref<TcpClientSocket>& socket, const TcpInitContext& context, sl_int64 tickEnd)
			{
				sl_uint8 packet[23];
				*packet = (sl_uint8)(TcpCommand::Init);
				Base::copyMemory(packet + 1, context.node->m_id.data, 4);
				Base::copyMemory(packet + 5, m_localNodeId.data, sizeof(P2PNodeId));
				MIO::writeUint16LE(packet + 21, m_portActor);
				Memory mem = Memory::create(packet, sizeof(packet));
				if (mem.isNull()) {
					return sl_false;
				}
				_sendTcpRequestPacket(socket, mem, [this, context](TcpCommand command, sl_uint8* data, sl_size size, CRef* refData) {
					Ref<TcpClientSocket> socket = context.socket;
					if (socket.isNotNull()) {
						if (command == TcpCommand::ReplyInit && !size) {
							context.callback(this, context.node.get(), context.connection.get(), socket.get());
							return;
						}
						m_mapTcpSockets.remove(socket.get());
					}
					context.callback(this, context.node.get(), context.connection.get(), sl_null);
				}, tickEnd);
				return sl_true;
			}

			struct TcpMessageContext
			{
				Ref<Node> node;
				Ref<DirectConnection> connection;
				WeakRef<TcpClientSocket> socket;
				Function<void(P2PResponse&)> callback;
			};

			sl_bool _sendTcpMessage(const Ref<TcpClientSocket>& socket, const TcpMessageContext& context, const Memory& _memPacket, sl_int64 tickEnd)
			{
				Memory memPacket = _memPacket;
				if (memPacket.isNotNull()) {
					context.node->updateEncryptionKey(m_ephemeralKey.data, m_ephemeralPublicKey.data);
					AES_GCM enc;
					enc.setKey(context.node->m_encryptionKey, 32);
					sl_size size = memPacket.getSize();
					sl_uint8* packet = (sl_uint8*)(memPacket.getData());
					*packet = (sl_uint8)(TcpCommand::Message);
					enc.start(packet + 1, 12);
					sl_uint8* content = packet + 29;
					enc.encrypt(content, content, size - 29);
					enc.finish(packet + 13);
				} else {
					sl_uint8 c = (sl_uint8)(TcpCommand::Message);
					memPacket = Memory::create(&c, 1);
					if (memPacket.isNull()) {
						return sl_false;
					}
				}
				_sendTcpRequestPacket(socket, memPacket, [this, context](TcpCommand command, sl_uint8* data, sl_size size, CRef* refData) {
					Ref<TcpClientSocket> socket = context.socket;
					if (socket.isNotNull()) {
						if (command == TcpCommand::ReplyMessage) {
							if (size > 28) {
								AES_GCM enc;
								enc.setKey(context.node->m_encryptionKey, 32);
								enc.start(data, 12);
								sl_uint8* content = data + 28;
								size -= 28;
								enc.decrypt(content, content, size);
								if (enc.finishAndCheckTag(data + 12)) {
									P2PResponse response(content, size, refData);
									response.connectionType = P2PConnectionType::Direct;
									context.callback(response);
									return;
								}
							} else if (!size) {
								P2PResponse response;
								response.connectionType = P2PConnectionType::Direct;
								context.callback(response);
								return;
							}
						}
						m_mapTcpSockets.remove(socket.get());
					}
					ReplyErrorResponse(context.callback);
				}, tickEnd);
				return sl_true;
			}

		public:
			void _deriveEncryptionKey(const sl_uint8 remoteEphemeralKey[EdDH::KeySize], sl_uint8 key[32])
			{
				DeriveKey(m_ephemeralKey.data, remoteEphemeralKey, key);
			}

			Ref<Node> _getNode(const P2PNodeId& nodeId)
			{
				return m_mapNodes.getValue(nodeId, sl_null, sl_false);
			}

			Ref<Node> _createNode(const sl_uint8 remoteKey[EdDSA::KeySize])
			{
				P2PNodeId nodeId(remoteKey);
				Ref<Node> node = m_mapNodes.getValue(nodeId, sl_null, sl_true);
				if (node.isNotNull()) {
					if (Base::equalsMemory(node->m_publicKey.data, remoteKey, EdDSA::KeySize)) {
						return node;
					}
				}
				node = new Node(remoteKey);
				if (node.isNotNull()) {
					m_mapNodes.put(nodeId, node);
					return node;
				}
				return sl_null;
			}

			Ref<DirectConnection> _createDirectConnection(Node* node, const SocketAddress& remoteAddress)
			{
				IPv4Address ip = remoteAddress.ip.getIPv4();
				if (ip.isZero()) {
					return sl_null;
				}
				Ref<DirectConnection> connection = node->m_connectionsDirect.getValue(ip);
				if (connection.isNull()) {
					connection = new DirectConnection(remoteAddress);
					if (connection.isNull()) {
						return sl_null;
					}
					node->m_connectionsDirect.put(ip, connection);
				}
				return connection;
			}

			void _selectDefaultConnectionIfBetter(Node* node, Connection* connection)
			{
				if (node->m_connectionDefault == connection) {
					return;
				}
				if (node->m_connectionDefault.isNotNull()) {
					Ref<Connection> connectionDefault = node->m_connectionDefault;
					if (connectionDefault.isNotNull()) {
						if (connectionDefault->m_type == P2PConnectionType::Direct) {
							if (_isValidConnection(connectionDefault.get())) {
								if (connectionDefault->m_delayLastPing <= connection->m_delayLastPing) {
									return;
								}
							}
						}
					}
				}
				node->m_connectionDefault = connection;
			}

			sl_bool _isValidConnection(Connection* connection)
			{
				return CheckDelay(connection->m_timeLastPing, GetCurrentTick(), m_connectionTimeout);
			}

			void _findNode(const SocketAddress* address, const P2PNodeId& nodeId, const NodeCallback& callback, sl_int64 tickEnd = -1)
			{
				Ref<Node> node = m_mapNodes.getValue(nodeId, sl_null, sl_true);
				if (node.isNotNull()) {
					if (address) {
						if (_findConnection(node.get(), *address).isNotNull()) {
							callback(node.get(), sl_null);
							return;
						}
					} else {
						callback(node.get(), sl_null);
						return;
					}
				}
				sl_bool flagShortTimeout = sl_false;
				sl_uint32 nShortTimeout = 0;
				if (tickEnd >= 0) {
					sl_uint64 cur = GetCurrentTick();
					if (tickEnd <= cur) {
						callback(sl_null, sl_null);
						return;
					}
					if (tickEnd < cur + m_findTimeout) {
						flagShortTimeout = sl_true;
						nShortTimeout = (sl_uint32)(tickEnd - cur);
					}
				} else {
					tickEnd = GetCurrentTick() + 5 * m_findTimeout;
				}
				WeakRef<This> thiz = this;
				if (flagShortTimeout) {
					Shared<sl_int32> counter = Shared<sl_int32>::create(0);
					if (counter.isNull()) {
						callback(sl_null, sl_null);
						return;
					}
					m_mapFindCallbacks.add(nodeId, [thiz, nodeId, callback, counter](Node* node, void*) {
						if (Base::interlockedIncrement32(counter.get()) == 1) {
							Ref<This> ref = thiz;
							callback(ref.isNotNull() ? node : sl_null, sl_null);
						}
					});
					m_dispatchLoop->dispatch([callback, counter]() {
						if (Base::interlockedIncrement32(counter.get()) == 1) {
							callback(sl_null, sl_null);
						}
					}, nShortTimeout);
				} else {
					SocketAddress rAddress;
					if (address) {
						rAddress = *address;
					}
					m_mapFindCallbacks.add(nodeId, [this, thiz, address, rAddress, nodeId, callback, tickEnd](Node* node, void*) {
						Ref<This> ref = thiz;
						if (ref.isNull()) {
							callback(sl_null, sl_null);
							return;
						}
						if (node) {
							callback(node, sl_null);
						} else {
							_findNode(address ? &rAddress : sl_null, nodeId, callback, tickEnd);
						}
					});
				}
				_sendFindNode(address, nodeId);
			}

			void _completeFindNodeCallbacks(const P2PNodeId& nodeId, Node* node)
			{
				NodeCallbackContainer container;
				while (m_mapFindCallbacks.remove(nodeId, &container)) {
					container.success(node, sl_null);
				}
			}

			Ref<Connection> _findConnection(Node* node, const SocketAddress& address)
			{
				IPv4Address ip = address.ip.getIPv4();
				if (ip.isNotZero()) {
					Ref<DirectConnection> connection = node->m_connectionsDirect.getValue(ip);
					if (connection.isNotNull()) {
						if (connection->m_address.port == address.port) {
							return Ref<Connection>::from(connection);
						}
					}
				}
				return sl_null;
			}

			void _sendMessage(Node* node, const SocketAddress* address, const Memory& packet, const Function<void(P2PResponse&)>& callback, sl_int64 tickEnd)
			{
				Ref<Connection> connection;
				if (address) {
					connection = _findConnection(node, *address);
				} else {
					connection = node->m_connectionDefault;
				}
				if (connection.isNotNull()) {
					if (_isValidConnection(connection.get())) {
						if (connection->m_type == P2PConnectionType::Direct) {
							_sendMessageDirectConnection(node, (DirectConnection*)(connection.get()), packet, callback, tickEnd);
							return;
						}
					}
				}
				ReplyErrorResponse(callback);
			}

			void _sendMessageDirectConnection(Node* node, DirectConnection* connection, const Memory& packet, const Function<void(P2PResponse&)>& callback, sl_int64 tickEnd)
			{
				_getTcpClientSocket(node, connection, [this, packet, callback, tickEnd](This* thiz, Node* node, DirectConnection* connection, TcpClientSocket* socket) {
					if (socket) {
						TcpMessageContext context;
						context.node = node;
						context.connection = connection;
						context.socket = socket;
						context.callback = callback;
						if (_sendTcpMessage(socket, context, packet, tickEnd)) {
							return;
						}
						m_mapTcpSockets.remove(socket);
					}
					ReplyErrorResponse(callback);
				}, tickEnd);
			}

		public:
			sl_bool isOpened() override
			{
				return !m_flagClosed;
			}

			void close() override
			{
				if (m_flagClosed) {
					return;
				}

				ObjectLocker locker(this);

				if (m_flagClosed) {
					return;
				}
				m_flagClosed = sl_true;

				if (m_timerHello.isNotNull()) {
					m_timerHello->stopAndWait();
					m_timerHello.setNull();
				}
				if (m_timerUpdateEphemeralKey.isNotNull()) {
					m_timerUpdateEphemeralKey->stopAndWait();
					m_timerUpdateEphemeralKey.setNull();
				}

				if (m_serverTcp.isNotNull()) {
					m_serverTcp->close();
					m_serverTcp.setNull();
				}
				if (m_socketUdpActor.isNotNull()) {
					m_socketUdpActor->close();
				}
				if (m_socketUdpLobby.isNotNull()) {
					m_socketUdpLobby->close();
				}

				if (m_dispatchLoop.isNotNull()) {
					m_dispatchLoop->release();
				}
				if (m_ioLoop.isNotNull()) {
					m_ioLoop->release();
					m_ioLoop.setNull();
				}

				m_mapTcpSockets.removeAll();
				m_mapIdleTcpStreams.removeAll();
				m_mapNodes.removeAll();

			}

			sl_bool start() override
			{
				if (m_flagClosed) {
					return sl_false;
				}

				ObjectLocker locker(this);

				if (m_flagClosed) {
					return sl_false;
				}

				m_ioLoop->start();
				m_dispatchLoop->start();

				if (m_timerHello.isNotNull()) {
					m_timerHello->start();
				}
				if (m_timerUpdateEphemeralKey.isNotNull()) {
					m_timerUpdateEphemeralKey->start();
				}

				return sl_true;
			}

			P2PNodeId getLocalNodeId() override
			{
				return m_localNodeId;
			}

			sl_uint16 getLocalPort() override
			{
				return m_portActor;
			}

			void setHelloMessage(const P2PMessage& msg) override
			{
				sl_size size = m_helloPrefix.getSize();
				if (size > sizeof(m_helloMessage)) {
					return;
				}
				Base::copyMemory(m_helloMessage, m_helloPrefix.getData(), size);
				sl_uint32 n = (sl_uint32)(sizeof(m_helloMessage) - size);
				sl_uint8* d = m_helloMessage + size;
				size = msg.size;
				if (size > n) {
					size = n;
				}
				Base::copyMemory(d, msg.data, size);
				m_sizeHelloMessage = (sl_uint32)size;
			}

			void setConnectMessage(const P2PMessage& msg) override
			{
				sl_uint32 size = msg.size;
				if (size > sizeof(m_connectMessage)) {
					size = sizeof(m_connectMessage);
				}
				Base::copyMemory(m_connectMessage, msg.data, size);
				m_sizeConnectMessage = size;
				_updateEphemeralKey();
			}

			void _updateEphemeralKey()
			{
				Math::randomMemory(m_ephemeralKey.data, EdDH::KeySize);
				m_ephemeralPublicKey = EdDH::getPublicKey(m_ephemeralKey.data);
			}

			void connectNode(const P2PNodeId& nodeId, const SocketAddress* address) override
			{
				if (m_flagClosed) {
					return;
				}
				Ref<Node> node = m_mapNodes.getValue(nodeId, sl_null, sl_true);
				if (node.isNotNull()) {
					if (address) {
						if (_findConnection(node.get(), *address).isNotNull()) {
							return;
						}
					}
				}
				_sendFindNode(address, nodeId);
			}

			sl_bool getEncryptionKeyForNode(const P2PNodeId& nodeId, void* outKey) override
			{
				Ref<Node> node = m_mapNodes.getValue(nodeId, sl_null, sl_true);
				if (node.isNotNull()) {
					Base::copyMemory(outKey, node->m_encryptionKey, 32);
					return sl_true;
				}
				return sl_false;
			}

			void sendMessage(const P2PNodeId& nodeId, const SocketAddress* address, const P2PRequest& msg, const Function<void(P2PResponse&)>& callback, sl_int32 timeout) override
			{
				if (m_flagClosed) {
					ReplyErrorResponse(callback);
					return;
				}
				Memory memPacket;
				if (msg.size) {
					memPacket = Memory::create(33 + msg.size);
					if (memPacket.isNull()) {
						ReplyErrorResponse(callback);
						return;
					}
					sl_uint8* packet = (sl_uint8*)(memPacket.getData());
					MIO::writeUint32LE(packet + 1, msg.size);
					Base::copyMemory(packet + 33, msg.data, msg.size);
				}
				sl_int64 tickEnd = GetTickFromTimeout(timeout);
				WeakRef<This> weakThis = this;
				SocketAddress rAddress;
				if (address) {
					rAddress = *address;
				}
				_findNode(address, nodeId, [weakThis, address, rAddress, memPacket, callback, tickEnd](Node* node, void*) {
					Ref<This> thiz = weakThis;
					if (thiz.isNotNull() && node) {
						thiz->_sendMessage(node, address ? &rAddress : sl_null, memPacket, callback, tickEnd);
					} else {
						ReplyErrorResponse(callback);
					}
				}, tickEnd);
			}

			void sendBroadcast(sl_uint32 ifIndex, const P2PRequest& msg) override
			{
				if (m_flagClosed) {
					return;
				}
				SLIB_SCOPED_BUFFER(sl_uint8, 4096, packet, 17 + msg.size)
				if (!packet) {
					return;
				}
				packet[0] = (sl_uint8)(Command::Broadcast);
				Base::copyMemory(packet + 1, m_localNodeId.data, sizeof(P2PNodeId));
				Base::copyMemory(packet + 17, msg.data, msg.size);
				_sendBroadcast(ifIndex, packet, 17 + msg.size);
			}

			void sendDatagram(const P2PNodeId& targetId, const SocketAddress& address, const P2PRequest& msg) override
			{
				if (m_flagClosed) {
					return;
				}
				SLIB_SCOPED_BUFFER(sl_uint8, 4096, packet, 21 + msg.size)
				if (!packet) {
					return;
				}
				packet[0] = (sl_uint8)(Command::Datagram);
				Base::copyMemory(packet + 1, targetId.data, 4);
				Base::copyMemory(packet + 5, m_localNodeId.data, sizeof(P2PNodeId));
				Base::copyMemory(packet + 21, msg.data, msg.size);
				_sendUdp(address, packet, 21 + msg.size);
			}

			void sendEncryptedDatagram(const P2PNodeId& targetId, const SocketAddress& address, const P2PRequest& msg) override
			{
				if (m_flagClosed) {
					return;
				}
				Ref<Node> node = m_mapNodes.getValue(targetId, sl_null, sl_true);
				if (node.isNull()) {
					return;
				}
				SLIB_SCOPED_BUFFER(sl_uint8, 4096, packet, 49 + msg.size)
				if (!packet) {
					return;
				}
				packet[0] = (sl_uint8)(Command::EncryptedDatagram);
				Base::copyMemory(packet + 1, targetId.data, 4);
				Base::copyMemory(packet + 5, m_localNodeId.data, sizeof(P2PNodeId));
				AES_GCM encryption;
				encryption.setKey(node->m_encryptionKey, 32);
				Math::randomMemory(packet + 21, 12); // iv
				encryption.start(packet + 21, 12);
				encryption.encrypt(msg.data, packet + 49, msg.size);
				encryption.finish(packet + 33);
				_sendUdp(address, packet, 49 + msg.size);
			}

		};

	}

	P2PNodeId::P2PNodeId() noexcept
	{
	}

	P2PNodeId::P2PNodeId(sl_null_t) noexcept: Bytes(sl_null)
	{
	}

	P2PNodeId::P2PNodeId(const StringParam& _id) noexcept: Bytes(_id)
	{
	}

	P2PNodeId::P2PNodeId(const sl_uint8* other) noexcept : Bytes(other)
	{
	}

	sl_size P2PNodeId::getHashCode() const noexcept
	{
#ifdef SLIB_ARCH_IS_X64
		return SLIB_MAKE_QWORD(data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7]);
#else
		return SLIB_MAKE_DWORD(data[0], data[1], data[2], data[3]);
#endif
	}


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(P2PMessage)

	P2PMessage::P2PMessage(): connectionType(P2PConnectionType::Unknown), interfaceIndex(0)
	{
	}

	P2PMessage::P2PMessage(const void* data, sl_size size, CRef* ref): DataContainer(data, size, ref), connectionType(P2PConnectionType::Unknown), interfaceIndex(0)
	{
	}


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(P2PRequest)

	P2PRequest::P2PRequest()
	{
	}

	P2PRequest::P2PRequest(const void* data, sl_uint32 size, CRef* ref): P2PMessage(data, size, ref)
	{
	}


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(P2PResponse)

	P2PResponse::P2PResponse()
	{
	}

	P2PResponse::P2PResponse(const void* data, sl_uint32 size, CRef* ref): P2PMessage(data, size, ref)
	{
	}


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(P2PSocketParam)

	P2PSocketParam::P2PSocketParam()
	{
		flagGeneratedKey = sl_false;

		port = SLIB_P2P_DEFAULT_PORT;
		portCount = 1000;
		boundPort = 0;

		helloInterval = 10000; // 10 seconds
		connectionTimeout = 60000; // 1 minutes
		findTimeout = 10000; // 10 seconds
		maximumMessageSize = 104857600; // 100MB
		messageSegmentSize = 0x10000;
		ephemeralKeyDuration = 86400000; // 24 hours

		flagAutoStart = sl_true;
	}


	SLIB_DEFINE_OBJECT(P2PSocket, Object)

	P2PSocket::P2PSocket()
	{
	}

	P2PSocket::~P2PSocket()
	{
	}

	Ref<P2PSocket> P2PSocket::open(P2PSocketParam& param)
	{
		return Ref<P2PSocket>::from(P2PSocketImpl::open(param));
	}

	void P2PSocket::connectNode(const P2PNodeId& nodeId, const SocketAddress& address)
	{
		connectNode(nodeId, &address);
	}

	void P2PSocket::connectNode(const P2PNodeId& nodeId)
	{
		connectNode(nodeId, sl_null);
	}

	void P2PSocket::sendMessage(const P2PNodeId& nodeId, const SocketAddress& address, const P2PRequest& msg, const Function<void(P2PResponse&)>& callback, sl_int32 timeout)
	{
		sendMessage(nodeId, &address, msg, callback, timeout);
	}

	void P2PSocket::sendMessage(const P2PNodeId& nodeId, const P2PRequest& msg, const Function<void(P2PResponse&)>& callback, sl_int32 timeout)
	{
		sendMessage(nodeId, sl_null, msg, callback, timeout);
	}

	void P2PSocket::sendMessage(const P2PNodeId& nodeId, const SocketAddress* address, const P2PRequest& msg, P2PResponse& response, sl_int32 timeout)
	{
		if (!(isOpened())) {
			return;
		}
		Ref<Event> ev = Event::create();
		if (ev.isNull()) {
			return;
		}
		Shared< Atomic<P2PResponse> > ret = Shared< Atomic<P2PResponse> >::create();
		if (ret.isNull()) {
			return;
		}
		sendMessage(nodeId, address, msg, [ret, ev](P2PResponse& response) {
			*ret = Move(response);
			ev->set();
		}, timeout);
		ev->wait();
		ret->release(response);
	}

	void P2PSocket::sendMessage(const P2PNodeId& nodeId, const SocketAddress& address, const P2PRequest& msg, P2PResponse& response, sl_int32 timeout)
	{
		sendMessage(nodeId, &address, msg, response, timeout);
	}

	void P2PSocket::sendMessage(const P2PNodeId& nodeId, const P2PRequest& msg, P2PResponse& response, sl_int32 timeout)
	{
		sendMessage(nodeId, sl_null, msg, response, timeout);
	}

	void P2PSocket::sendBroadcast(const P2PRequest& msg)
	{
		sendBroadcast(0, msg);
	}

}
