/*
*   Copyright (c) 2008-2023 SLIBIO <https://github.com/SLIBIO>
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

//#define USE_CURVE448

#include <slib/network/socket.h>
#include <slib/network/event.h>
#include <slib/network/async.h>
#include <slib/network/os.h>
#include <slib/core/thread_pool.h>
#include <slib/core/dispatch_loop.h>
#include <slib/core/system.h>
#include <slib/core/scoped_buffer.h>
#include <slib/crypto/aes.h>
#include <slib/crypto/hkdf.h>
#ifdef USE_CURVE448
#include <slib/crypto/curve448.h>
#else
#include <slib/crypto/curve25519.h>
#endif
#include <slib/data/expiring_map.h>
#include <slib/device/cpu.h>

/*
	P2P Socket Protocol

Port = Uint16 (Little Endian)
TickCount = Uint32 (Little Endian)
SharedKey = HKDF(ECDH(LocalNode's Ephemeral Private Key, RemoteNode's Ephemeral Public Key), 32)
DH_KL = 32 (for X25519), or 56 (for X448)
DSA_KL = 32 (for Ed25519), or 57 (for Ed448)
Encryption = IV(12 Bytes) | Tag(16 Bytes) | Content (AES-GCM, key=SharedKey)

- Hello
Length(B)	Type			Value
-----------------------------------------------
1			Command			0 (Hello)
16			NodeId			LocalNode
1			Boolean			Need Reply
*			Message Content

- ReplyHello
Length(B)	Type			Value
-----------------------------------------------
1			Command			1 (ReplyHello)
16			NodeId			LocalNode
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

- VerifyNode
Offset	Length(B)	Type			Value
-----------------------------------------------
1			Command			4 (VerifyNode)
16			NodeId			RemoteNode
16			NodeId			LocalNode
DH_KL		Bytes			LocalNode's Ephemeral Public Key
4			TickCount		LocalNode

- ReplyVerifyNode
Length(B)	Type			Value
-----------------------------------------------
1			Command			5 (ReplyVerifyNode)
16			NodeId			RemoteNode
DSA_KL		Bytes			LocalNode's Public Key
DH_KL		Bytes			LocalNode's Ephemeral Public Key
*			Bytes			Encryption
-----	Encrypted Content ---------------------
2*DSA_KL	Bytes			EdDSA(LocalNode's Private Key, LocalNode's Ephemeral Public Key | RemoteNode's Ephemeral Public Key)
4			TickCount		RemoteNode
4			TickCount		LocalNode (0: Completed, 1: Need Complete, Otherwise: Need Verify)
*			Bytes			LocalNode's Description

- CompleteVerifyNode
Offset	Length(B)	Type			Value
-----------------------------------------------
1			Command			6 (CompleteVerifyNode)
16			NodeId			RemoteNode
16			NodeId			LocalNode

- Ping
Length(B)	Type			Value
-----------------------------------------------
1			Command			7 (Ping)
4			ShortNodeId		RemoteNode
4			TickCount		LocalNode

- ReplyPing
Length(B)	Type			Value
-----------------------------------------------
1			Command			8 (ReplyPing)
16			NodeId			LocalNode
8			Bytes			LocalNode's Ephemeral Public Key (Prefix)
4			TickCount		RemoteNode

- Broadcast
Length(B)	Type			Value
-----------------------------------------------
1			Command			9 (Broadcast)
16			NodeId			LocalNode
*			Content

- Datagram
Length(B)	Type			Value
-----------------------------------------------
1			Command			10 (Datagram)
16			NodeId			LocalNode
*			Content


---------------- TCP Commands -----------------

- Init
Length(B)	Type			Value
-----------------------------------------------
1			Command			0 (Init)
4			ShortNodeId		RemoteNode
16			NodeId			LocalNode

- ReplyInit
Length(B)	Type			Value
-----------------------------------------------
1			Command			1 (ReplyInit)

- Message
Length(B)	Type			Value
-----------------------------------------------
1			Command			2 (Message)
CVLI		Uint32			Total Length (From next field)
*			Bytes			Encryption

- ReplyMessage
Length(B)	Type			Value
-----------------------------------------------
1			Command			3 (ReplyMessage)
CVLI		Uint32			Total Length (From next field)
*			Bytes			Encryption

*/

#ifdef USE_CURVE448
#define EdDSA Ed448
#define EdDSA_KEY_LEN 57
#define EdDH X448
#define EdDH_KEY_LEN 56
#else
#define EdDSA Ed25519
#define EdDSA_KEY_LEN 32
#define EdDH X25519
#define EdDH_KEY_LEN 32
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
			VerifyNode = 4,
			ReplyVerifyNode = 5,
			CompleteVerifyNode = 6,
			Ping = 7,
			ReplyPing = 8,
			Broadcast = 9,
			Datagram = 10
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

		static void PrepareMessageContent(const Variant& var, P2PMessage& content)
		{
			if (var.isNotNull()) {
				if (var.isMemory()) {
					content.setMemory(var.getMemory());
				} else if (var.isObject() || var.isCollection()) {
					Json json(var);
					Memory mem = json.serialize();
					content.setJson(Move(json), Move(mem));
				} else {
					String str = var.getString();
					content.setString(Move(str));
				}
			}
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
			Connection(P2PConnectionType type) : m_type(type) {}

		};

		class DirectConnection;

		class Node : public CRef
		{
		public:
			P2PNodeId m_id;
			Bytes<EdDSA_KEY_LEN> m_publicKey;
			Bytes<EdDH_KEY_LEN> m_remoteEphemeralKey;
			Bytes<EdDH_KEY_LEN> m_localEphemeralKey;
			sl_uint8 m_encryptionKey[32];
			sl_bool m_flagInvalidEncryptionKey = sl_true;
			AtomicMemory m_description;

			AtomicRef<Connection> m_connectionDefault;
			CHashMap< IPv4Address, Ref<DirectConnection> > m_connectionsDirect;

		public:
			Node(const sl_uint8 publicKey[EdDSA_KEY_LEN]): m_publicKey(publicKey), m_id(publicKey) {}

		public:
			void updateEphemeralKey(const sl_uint8 key[EdDH_KEY_LEN], const Memory& description)
			{
				if (Base::equalsMemory(m_remoteEphemeralKey.data, key, EdDH_KEY_LEN)) {
					return;
				}
				m_remoteEphemeralKey.setData(key);
				m_description = description;
				m_flagInvalidEncryptionKey = sl_true;
			}

			void updateEncryptionKey(const sl_uint8 localEphemeralPrivateKey[EdDH_KEY_LEN], const sl_uint8 localEphemeralPublicKey[EdDH_KEY_LEN])
			{
				sl_bool flagUpdate = sl_false;
				if (m_flagInvalidEncryptionKey) {
					m_flagInvalidEncryptionKey = sl_false;
					flagUpdate = sl_true;
				}
				if (!(Base::equalsMemory(m_localEphemeralKey.data, localEphemeralPublicKey, EdDH_KEY_LEN))) {
					m_localEphemeralKey.setData(localEphemeralPublicKey);
					flagUpdate = sl_true;
				}
				if (flagUpdate) {
					DeriveKey(localEphemeralPrivateKey, m_remoteEphemeralKey.data, m_encryptionKey);
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

		class TcpCommandContentReceiver
		{
		public:
			sl_uint8* content = sl_null;
			sl_size contentSize;

		public:
			TcpCommandContentReceiver(sl_uint32 _contentSize, sl_size maxSize): contentSize(_contentSize), m_flagParsingHeader(!_contentSize), m_maxContentSize(maxSize) {}

		public:
			// returns -1 on error, 1 on success, 0 on incomplete
			sl_int32 put(sl_uint8* data, sl_size size)
			{
				if (m_flagParsingHeader) {
					sl_uint32 n = sizeof(m_bufHeader) - m_sizeHeader;
					if (n > size) {
						n = (sl_uint32)size;
					}
					if (n) {
						Base::copyMemory(m_bufHeader, data, n);
						m_sizeHeader += n;
					}
					sl_uint32 m = CVLI::deserialize(m_bufHeader, m_sizeHeader, contentSize);
					if (m) {
						if (contentSize > m_maxContentSize) {
							return -1;
						}
						if (m >= 8) {
							return -1;
						}
						if (m <= m_sizeHeader - n) {
							return -1;
						}
						sl_uint32 l = m - (m_sizeHeader - n);
						data += l;
						size -= l;
						m_flagParsingHeader = sl_false;
						m_sizeHeader = m;
						if (size > contentSize) {
							return -1;
						}
						if (size == contentSize) {
							content = data;
							return 1;
						}
						if (size) {
							if (!(m_bufContent.addNew(data, size))) {
								return -1;
							}
						}
					} else {
						if (m_sizeHeader >= 8) {
							return -1;
						}
					}
				} else {
					sl_size sizeOld = m_bufContent.getSize();
					sl_size sizeNew = sizeOld + size;
					if (sizeNew > contentSize) {
						return -1;
					}
					if (sizeNew == contentSize) {
						if (sizeOld) {
							if (m_bufContent.addNew(data, size)) {
								m_memContent = m_bufContent.merge();
								if (m_memContent.isNotNull()) {
									content = (sl_uint8*)(m_memContent.getData());
									return 1;
								}
							}
							return -1;
						} else {
							content = data;
							return 1;
						}
					} else {
						if (!(m_bufContent.addNew(data, size))) {
							return -1;
						}
					}
				}
				return 0;
			}

		private:
			sl_size m_maxContentSize;
			sl_bool m_flagParsingHeader;
			sl_uint8 m_bufHeader[10];
			sl_uint32 m_sizeHeader = 0;
			MemoryBuffer m_bufContent;
			Memory m_memContent;
		};

		class TcpStream : public CRef
		{
		public:
			Ref<AsyncTcpSocket> m_socket;
			TcpCommand m_currentCommand = TcpCommand::Unknown;

		public:
			TcpStream(Ref<AsyncTcpSocket>&& _socket, sl_size maximumMessageSize) : m_socket(Move(_socket)), m_maximumMessageSize(maximumMessageSize) {}

		public:
			sl_int32 processReceivedData(sl_uint8* data, sl_size size)
			{
				if (!size) {
					return 0;
				}
				if (m_receiver.isNull()) {
					m_currentCommand = (TcpCommand)(*data);
					sl_uint32 contentSize = 0;
					switch (m_currentCommand) {
						case TcpCommand::Init:
							contentSize = 20;
							break;
						default:
							break;
					}
					m_receiver = Shared<TcpCommandContentReceiver>::create(contentSize, m_maximumMessageSize + 1024);
					if (m_receiver.isNull()) {
						return -1;
					}
					data++;
					size--;
					if (!size) {
						return 0;
					}
				}
				return m_receiver->put(data, size);
			}

			sl_uint8* getContent()
			{
				if (m_receiver.isNotNull()) {
					return m_receiver->content;
				}
				return sl_null;
			}

			sl_size getContentSize()
			{
				if (m_receiver.isNotNull()) {
					return m_receiver->contentSize;
				}
				return 0;
			}

			void clear()
			{
				m_receiver.setNull();
			}

		private:
			sl_size m_maximumMessageSize;
			Shared<TcpCommandContentReceiver> m_receiver;

		};

		class TcpServerStream : public TcpStream
		{
		public:
			P2PNodeId m_remoteId;
			sl_bool m_flagWriting = sl_false;

		public:
			TcpServerStream(Ref<AsyncTcpSocket>&& _socket, sl_size maximumMessageSize): TcpStream(Move(_socket), maximumMessageSize) {}

		};

		class TcpClientStream : public TcpStream
		{
		public:
			TcpClientStream(Ref<AsyncTcpSocket>&& _socket, sl_size maximumMessageSize): TcpStream(Move(_socket), maximumMessageSize) {}

		};

		class TimeoutMonitor
		{
		public:
			sl_bool tryFinish() noexcept
			{
				return !(Base::interlockedDecrement32(&m_counter));
			}

			sl_bool isFinished() noexcept
			{
				return m_counter != 1;
			}

		public:
			static sl_bool create(Shared<TimeoutMonitor>& monitor, sl_uint64 tickEnd) noexcept
			{
				if (tickEnd) {
					monitor = Shared<TimeoutMonitor>::create();
					return monitor.isNotNull();
				} else {
					return sl_true;
				}
			}

			static void dispatchTimeout(const Shared<TimeoutMonitor>& monitor, const Ref<DispatchLoop>& loop, const Function<void()>& callbackTimeout, sl_uint64 tickEnd) noexcept
			{
				sl_uint64 cur = GetCurrentTick();
				if (cur < tickEnd) {
					loop->dispatch([monitor, callbackTimeout]() {
						if (monitor->tryFinish()) {
							callbackTimeout();
						}
					}, tickEnd - cur);
				} else {
					if (monitor->tryFinish()) {
						callbackTimeout();
					}
				}
			}

			static sl_bool isFinished(const Shared<TimeoutMonitor>& monitor) noexcept
			{
				if (monitor.isNull()) {
					return sl_false;
				}
				return monitor->isFinished();
			}

			static sl_bool tryFinish(const Shared<TimeoutMonitor>& monitor) noexcept
			{
				if (monitor.isNull()) {
					return sl_true;
				}
				return monitor->tryFinish();
			}

		private:
			volatile sl_int32 m_counter = 1;
		};

		class MessageBody
		{
		public:
			Memory packet; // Same layout with Message command
			sl_uint32 lengthOfSize = 0; // Size of CVLI

		};

		class P2PSocketImpl : public P2PSocket
		{
		public:
			typedef P2PSocketImpl This;

			P2PSocketParam m_param;

			P2PNodeId m_localNodeId;
			Bytes<EdDSA_KEY_LEN> m_localKey;
			Bytes<EdDSA_KEY_LEN> m_localPublicKey;
			Bytes<EdDH_KEY_LEN> m_ephemeralKey;
			Bytes<EdDH_KEY_LEN> m_ephemeralPublicKey;

			sl_uint8 m_helloMessage[1024];
			sl_uint32 m_sizeHelloMessage;
			sl_uint8 m_localNodeDescription[2048];
			sl_uint32 m_sizeLocalNodeDescription;

			sl_bool m_flagClosed = sl_false;

			sl_uint16 m_portLobby = 0;
			sl_uint16 m_portActor = 0;
			sl_uint16 m_portActorMax = 0;

			Ref<AsyncUdpSocket> m_socketUdpLobby;
			Ref<AsyncUdpSocket> m_socketUdpActor;
			Ref<AsyncTcpServer> m_serverTcp;

			ExpiringMap< TcpStream*, Ref<TcpStream> > m_mapTcpStreams;
			ExpiringMap< DirectConnection*, Ref<AsyncTcpSocket> > m_mapIdleTcpSockets;

			Ref<ThreadPool> m_threadPool;
			Ref<AsyncIoLoop> m_ioLoop;
			Ref<DispatchLoop> m_dispatchLoop;
			Ref<Timer> m_timerHello;
			Ref<Timer> m_timerUpdateEphemeralKey;

			ExpiringMap< P2PNodeId, Ref<Node> > m_mapNodes;
			ExpiringMap<P2PNodeId, NodeCallbackContainer> m_mapFindCallbacks;

			AtomicList<IPv4Address> m_listLocalIPAddresses;
			sl_uint16 m_portLocalhostMax = 0;

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

				if (param.key.isNull() || param.key.getSize() != EdDSA_KEY_LEN) {
					param.key = Memory::create(EdDSA_KEY_LEN);
					if (param.key.isNull()) {
						SLIB_STATIC_STRING(err, "Lack of memory")
						param.errorText = err;
						return sl_null;
					}
					Math::randomMemory(param.key.getData(), EdDSA_KEY_LEN);
					param.flagGeneratedKey = sl_true;
				}

				Socket socketLobby = _openLobby(param.port);
				if (socketLobby.isNone()) {
					SLIB_STATIC_STRING(err, "Failed to bind lobby socket")
					param.errorText = err;
					return sl_null;
				}
				Socket socketUdp, socketTcp;
				{
					param.boundPort = 0;
					for (sl_uint16 i = 1; i <= param.portCount; i++) {
						if (_openPorts(param.port + i, socketUdp, socketTcp)) {
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

				Ref<ThreadPool> threadPool = ThreadPool::create(0, Cpu::getCoreCount());
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
					ret->m_threadPool = Move(threadPool);
					ret->m_ioLoop = Move(ioLoop);
					ret->m_dispatchLoop = Move(dispatchLoop);
					ret->m_param = param;
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
				if (param.connectionTimeout < 10000) {
					param.connectionTimeout = 10000;
				}
				if (param.messageBufferSize < 64) {
					param.messageBufferSize = 64;
				}
				if (param.maximumMessageSize < 1) {
					param.maximumMessageSize = 1;
				}

				m_localKey.setData(param.key.getData());
				m_localPublicKey = EdDSA::getPublicKey(m_localKey.data);
				m_localNodeId.setData(m_localPublicKey.data);

				setHelloMessage(param.helloMessage);
				setLocalNodeDescription(param.localNodeDescription);

				m_listLocalIPAddresses = Network::findAllIPv4Addresses();

				m_mapNodes.setupTimer(param.connectionTimeout, m_dispatchLoop);
				m_mapFindCallbacks.setupTimer(param.findTimeout, m_dispatchLoop);

				// Initialize UDP Socket
				{
					m_portLobby = param.port;
					m_portActor = param.boundPort;
					m_portActorMax = param.port + param.portCount;
					m_portLocalhostMax = param.boundPort - 1;

					AsyncUdpSocketParam udpParam;
					udpParam.ioLoop = m_ioLoop;
					udpParam.flagBroadcast = sl_true;
					udpParam.socket = Move(socketUdp);
					udpParam.onReceiveFrom = [this](AsyncUdpSocket*, const SocketAddress& address, void* data, sl_uint32 size) {
						_processReceivedUdp(address, (sl_uint8*)data, size);
					};
					m_socketUdpActor = AsyncUdpSocket::create(udpParam);
					if (m_socketUdpActor.isNull()) {
						return sl_false;
					}

					udpParam.socket = Move(socketLobby);
					udpParam.onReceiveFrom = [this](AsyncUdpSocket*, const SocketAddress& address, void* data, sl_uint32 size) {
						_processReceivedUdp(address, (sl_uint8*)data, size);
					};
					m_socketUdpLobby = AsyncUdpSocket::create(udpParam);
					if (m_socketUdpLobby.isNull()) {
						return sl_false;
					}
				}

				// Initialize TCP Server
				{
					m_mapTcpStreams.setupTimer(param.connectionTimeout, m_dispatchLoop);
					m_mapIdleTcpSockets.setupTimer(param.connectionTimeout, m_dispatchLoop);
					AsyncTcpServerParam serverParam;
					serverParam.ioLoop = m_ioLoop;
					serverParam.onAccept = [this](AsyncTcpServer*, Socket& socket, const SocketAddress& address) {
						_onAcceptTcpServerConnection(socket);
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
						_sendHello(sl_false);
					}, param.helloInterval);
					if (m_timerHello.isNull()) {
						return sl_false;
					}
					m_dispatchLoop->dispatch([this]() {
						_sendHello(sl_true);
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

			static Socket _openLobby(sl_uint16 port)
			{
				Socket socket = Socket::openUdp();
				if (socket.isNotNone()) {
					socket.setOption_ReuseAddress();
					socket.setOption_ReusePort();
					if (socket.bind(port)) {
						return socket;
					}
				}
				return SLIB_SOCKET_INVALID_HANDLE;
			}

			static sl_bool _openPorts(sl_uint16 port, Socket& udp, Socket& tcp)
			{
				udp = Socket::openUdp(port);
				if (udp.isNone()) {
					return sl_false;
				}
				tcp = Socket::openTcp(port);
				if (tcp.isNone()) {
					return sl_false;
				}
				return sl_true;
			}

		public:
			void _sendUdp(const SocketAddress& address, sl_uint8* buf, sl_size size)
			{
				m_socketUdpActor->sendTo(address, buf, size);
			}

			void _sendBroadcast(sl_uint8* buf, sl_size size)
			{
				// Send to other hosts
				{
					SocketAddress address;
					address.port = m_portLobby;
					List<IPv4Address> listIP;
					ListElements<IPv4AddressInfo> addrs(Network::findAllIPv4AddressInfos());
					for (sl_size i = 0; i < addrs.count; i++) {
						IPv4Address& addr = addrs[i].address;
						if (!(addr.isLoopback())) {
							listIP.add_NoLock(addr);
							sl_uint32 m = addr.getInt();
							sl_uint32 n = addrs[i].networkPrefixLength;
							if (n < 32) {
								m = m | ((1 << (32 - n)) - 1);
								address.ip = m;
								_sendUdp(address, buf, size);
							}
						}
					}
					m_listLocalIPAddresses = listIP;
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

			void _processReceivedUdp(const SocketAddress& address, sl_uint8* packet, sl_uint32 sizePacket)
			{
				if (!sizePacket) {
					return;
				}
				if (!(_isValidSenderAddress(address))) {
					return;
				}
				Command cmd = (Command)(*packet);
				switch (cmd) {
					case Command::Hello:
						_onReceiveHello(address, packet, sizePacket);
						break;
					case Command::ReplyHello:
						_onReceiveReplyHello(address, packet, sizePacket);
						break;
					case Command::FindNode:
						_onReceiveFindNode(address, packet, sizePacket);
						break; 
					case Command::ReplyFindNode:
						_onReceiveReplyFindNode(address, packet, sizePacket);
						break;
					case Command::VerifyNode:
						_onReceiveVerifyNode(address, packet, sizePacket);
						break;
					case Command::ReplyVerifyNode:
						_onReceiveReplyVerifyNode(address, packet, sizePacket);
						break;
					case Command::CompleteVerifyNode:
						_onReceiveCompleteVerifyNode(address, packet, sizePacket);
						break;
					case Command::Ping:
						_onReceivePing(address, packet, sizePacket);
						break;
					case Command::ReplyPing:
						_onReceiveReplyPing(address, packet, sizePacket);
						break;
					case Command::Broadcast:
						_onReceiveBroadcast(address, packet, sizePacket);
						break;
					case Command::Datagram:
						_onReceiveDatagram(address, packet, sizePacket);
						break;
					default:
						break;
				}
			}

			void _sendHello(sl_bool flagNeedReply)
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
				_sendBroadcast(packet, 18 + sizeMessage);
			}

			void _onReceiveHello(const SocketAddress& address, sl_uint8* packet, sl_uint32 sizePacket)
			{
				if (sizePacket < 18) {
					return;
				}
				P2PRequest message(packet + 18, sizePacket - 18);
				message.senderId = P2PNodeId(packet + 1);
				message.connectionType = P2PConnectionType::Direct;
				message.remoteAddress = address;
				_onReceiveHello(message, packet[17] != 0);
			}

			void _onReceiveHello(P2PRequest& message, sl_bool flagNeedReply)
			{
				_onReceiveHelloMessage(message);
				if (flagNeedReply) {
					_sendReplyHello(message.remoteAddress);
				}
				Ref<Node> node = _getNode(message.senderId);
				if (node.isNotNull()) {
					IPv4Address ip = message.remoteAddress.ip.getIPv4();
					if (ip.isNotZero()) {
						if (node->m_connectionsDirect.find(ip)) {
							_sendPing(message.remoteAddress, message.senderId);
							return;
						}
					}
					_sendVerifyNode(message.remoteAddress, message.senderId);
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

			void _onReceiveReplyHello(const SocketAddress& address, sl_uint8* packet, sl_uint32 sizePacket)
			{
				if (sizePacket < 17) {
					return;
				}
				P2PRequest message(packet + 17, sizePacket - 17);
				message.senderId = P2PNodeId(packet + 1);
				message.connectionType = P2PConnectionType::Direct;
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
				m_param.onReceiveHello(this, message);
			}

			void _sendFindNode(const P2PNodeId& nodeId)
			{
				sl_uint8 packet[17];
				*packet = (sl_uint8)(Command::FindNode);
				Base::copyMemory(packet + 1, nodeId.data, sizeof(P2PNodeId));
				_sendBroadcast(packet, sizeof(packet));
			}

			void _onReceiveFindNode(const SocketAddress& address, sl_uint8* packet, sl_uint32 sizePacket)
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

			void _onReceiveReplyFindNode(const SocketAddress& address, sl_uint8* packet, sl_uint32 sizePacket)
			{
				if (sizePacket != 17) {
					return;
				}
				P2PNodeId targetId(packet + 1);
				_sendVerifyNode(address, targetId);
			}

			void _sendVerifyNode(const SocketAddress& address, const P2PNodeId& remoteId)
			{
				sl_uint8 packet[37 + EdDH_KEY_LEN];
				*packet = (sl_uint8)(Command::VerifyNode);
				Base::copyMemory(packet + 1, remoteId.data, sizeof(P2PNodeId));
				Base::copyMemory(packet + 17, m_localNodeId.data, sizeof(P2PNodeId));
				Base::copyMemory(packet + 33, m_ephemeralPublicKey.data, EdDH_KEY_LEN);
				MIO::writeUint32LE(packet + (33 + EdDH_KEY_LEN), GetCurrentTick());
				_sendUdp(address, packet, sizeof(packet));
			}

			void _onReceiveVerifyNode(const SocketAddress& address, sl_uint8* packet, sl_uint32 sizePacket)
			{
				if (sizePacket != 37 + EdDH_KEY_LEN) {
					return;
				}
				if (!(Base::equalsMemory(m_localNodeId.data, packet + 1, sizeof(P2PNodeId)))) {
					return;
				}
				P2PNodeId remoteId(packet + 17);
				Bytes<EdDH_KEY_LEN> remoteEphemeralKey(packet + 33);
				sl_uint32 remoteTick = MIO::readUint32LE(packet + (33 + EdDH_KEY_LEN));
				m_threadPool->addTask([this, address, remoteId, remoteEphemeralKey, remoteTick]() {
					_onReceiveVerifyNode(address, remoteId, remoteEphemeralKey.data, remoteTick);
				});
			}

			void _onReceiveVerifyNode(const SocketAddress& address, const P2PNodeId& remoteId, const sl_uint8 remoteEphemeralKey[EdDH_KEY_LEN], sl_uint32 remoteTick)
			{
				sl_uint32 localTick = GetCurrentTick();
				if (localTick < 2) {
					localTick = 2;
				}
				Ref<Node> node = _getNode(remoteId);
				if (node.isNotNull()) {
					if (Base::equalsMemory(node->m_remoteEphemeralKey.data, remoteEphemeralKey, EdDH_KEY_LEN)) {
						localTick = 0; // Completed
					}
				}
				_sendReplyVerifyNode(address, remoteId, remoteEphemeralKey, remoteTick, localTick);
			}

			void _sendReplyVerifyNode(const SocketAddress& address, const P2PNodeId& remoteId, const sl_uint8 remoteEphemeralKey[EdDH_KEY_LEN], sl_uint32 remoteTick, sl_uint32 localTick)
			{
				const sl_uint32 sizeHeader = 17 + EdDSA_KEY_LEN + EdDH_KEY_LEN;
				const sl_uint32 sizeContentHeader = EdDSA_KEY_LEN * 2 + 8;
				sl_uint8 packet[sizeHeader + 28 + sizeContentHeader + sizeof(m_localNodeDescription)];
				*packet = (sl_uint8)(Command::ReplyVerifyNode);
				Base::copyMemory(packet + 1, remoteId.data, sizeof(P2PNodeId));
				Base::copyMemory(packet + 17, m_localPublicKey.data, EdDSA_KEY_LEN);
				Base::copyMemory(packet + (17 + EdDSA_KEY_LEN), m_ephemeralPublicKey.data, EdDH_KEY_LEN);
				Math::randomMemory(packet + sizeHeader, 12);
				const sl_uint32 posContent = sizeHeader + 28;
				sl_uint8 sts[EdDH_KEY_LEN * 2];
				Base::copyMemory(sts, m_ephemeralPublicKey.data, EdDH_KEY_LEN);
				Base::copyMemory(sts + EdDH_KEY_LEN, remoteEphemeralKey, EdDH_KEY_LEN);
				EdDSA::sign(m_localKey.data, m_localPublicKey.data, sts, sizeof(sts), packet + posContent);
				MIO::writeUint32LE(packet + (posContent + EdDSA_KEY_LEN * 2), remoteTick);
				MIO::writeUint32LE(packet + (posContent + EdDSA_KEY_LEN * 2 + 4), localTick);
				sl_uint32 sizeDesc = m_sizeLocalNodeDescription;
				if (sizeDesc > sizeof(m_localNodeDescription)) {
					sizeDesc = sizeof(m_localNodeDescription);
				}
				Base::copyMemory(packet + (posContent + sizeContentHeader), m_localNodeDescription, sizeDesc);
				AES_GCM cryptor;
				sl_uint8 key[32];
				_deriveEncryptionKey(remoteEphemeralKey, key);
				cryptor.setKey(key, sizeof(key));
				cryptor.start(packet + sizeHeader, 12);
				cryptor.encrypt(packet + posContent, packet + posContent, sizeContentHeader + sizeDesc);
				cryptor.finish(packet + (sizeHeader + 12));
				_sendUdp(address, packet, sizeHeader + 28 + sizeContentHeader + sizeDesc);
			}

			void _onReceiveReplyVerifyNode(const SocketAddress& address, sl_uint8* packet, sl_uint32 sizePacket)
			{
				const sl_uint32 sizeHeader = 17 + EdDSA_KEY_LEN + EdDH_KEY_LEN;
				const sl_uint32 sizeContentHeader = EdDSA_KEY_LEN * 2 + 8;
				if (sizePacket < sizeHeader + 28 + sizeContentHeader) {
					return;
				}
				if (!(Base::equalsMemory(m_localNodeId.data, packet + 1, sizeof(P2PNodeId)))) {
					return;
				}
				Bytes<EdDSA_KEY_LEN> remoteKey(packet + 17);
				Bytes<EdDH_KEY_LEN> remoteEphemeralKey(packet + (17 + EdDSA_KEY_LEN));
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
				Bytes<EdDSA_KEY_LEN * 2> signature(packet + posContent);
				sl_uint32 timeOld = MIO::readUint32LE(packet + (posContent + EdDSA_KEY_LEN * 2));
				sl_uint32 timeNew = GetCurrentTick();
				if (!(CheckDelay(timeOld, timeNew, m_param.findTimeout))) {
					return;
				}
				sl_uint32 localTick = MIO::readUint32LE(packet + (posContent + EdDSA_KEY_LEN * 2 + 4));
				Memory desc = Memory::create(packet + (posContent + sizeContentHeader), sizePacket - (posContent + sizeContentHeader));
				m_threadPool->addTask([this, address, remoteKey, remoteEphemeralKey, signature, desc, timeOld, timeNew, localTick]() {
					_onReceiveReplyVerifyDirectConnection(address, remoteKey.data, remoteEphemeralKey.data, signature.data, desc, timeNew, timeNew - timeOld, localTick);
				});
			}

			void _onReceiveReplyVerifyDirectConnection(const SocketAddress& address, const sl_uint8 remoteKey[EdDSA_KEY_LEN], const sl_uint8 remoteEphemeralKey[EdDH_KEY_LEN], const sl_uint8 signature[EdDSA_KEY_LEN * 2], const Memory& desc, sl_uint32 tick, sl_uint32 delay, sl_uint32 localTick)
			{
				sl_uint8 sts[EdDH_KEY_LEN * 2];
				Base::copyMemory(sts, remoteEphemeralKey, EdDH_KEY_LEN);
				Base::copyMemory(sts + EdDH_KEY_LEN, m_ephemeralPublicKey.data, EdDH_KEY_LEN);
				if (!(EdDSA::verify(remoteKey, sts, sizeof(sts), signature))) {
					return;
				}
				Ref<Node> node = _createNode(remoteKey);
				if (node.isNotNull()) {
					node->updateEphemeralKey(remoteEphemeralKey, desc);
					Ref<DirectConnection> connection = _createDirectConnection(node.get(), address);
					if (connection.isNotNull()) {
						connection->m_timeLastPing = tick;
						connection->m_delayLastPing = delay;
						_selectDefaultConnectionIfBetter(node.get(), connection.get());
					} else {
						node.setNull();
					}
				}
				if (node.isNull()) {
					if (localTick != 1) {
						_completeFindNodeCallbacks(P2PNodeId(remoteKey), sl_null);
					}
					return;
				}
				if (localTick == 0) { // Completed
					_completeFindNodeCallbacks(node->m_id, node.get());
				} else if (localTick == 1) { // Need Complete
					_sendCompleteVerifyNode(address, node->m_id);
				} else { // Need Verify
					_sendReplyVerifyNode(address, node->m_id, remoteEphemeralKey, localTick, 1); // Need Complete
				}
			}

			void _sendCompleteVerifyNode(const SocketAddress& address, const P2PNodeId& remoteId)
			{
				sl_uint8 packet[33];
				*packet = (sl_uint8)(Command::CompleteVerifyNode);
				Base::copyMemory(packet + 1, remoteId.data, sizeof(P2PNodeId));
				Base::copyMemory(packet + 17, m_localNodeId.data, sizeof(P2PNodeId));
				_sendUdp(address, packet, sizeof(packet));
			}

			void _onReceiveCompleteVerifyNode(const SocketAddress& address, sl_uint8* packet, sl_uint32 sizePacket)
			{
				if (sizePacket != 33) {
					return;
				}
				if (!(Base::equalsMemory(m_localNodeId.data, packet + 1, sizeof(P2PNodeId)))) {
					return;
				}
				P2PNodeId nodeId(packet + 17);
				Ref<Node> node = _getNode(nodeId);
				if (node.isNotNull()) {
					_completeFindNodeCallbacks(node->m_id, node.get());
				}
			}

			void _sendPing(const SocketAddress& address, const P2PNodeId& nodeId)
			{
				sl_uint8 packet[9];
				*packet = (sl_uint8)(Command::Ping);
				Base::copyMemory(packet + 1, nodeId.data, 4);
				MIO::writeUint32LE(packet + 5, GetCurrentTick());
				_sendUdp(address, packet, sizeof(packet));
			}

			void _onReceivePing(const SocketAddress& address, sl_uint8* packet, sl_uint32 sizePacket)
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

			void _onReceiveReplyPing(const SocketAddress& address, sl_uint8* packet, sl_uint32 sizePacket)
			{
				if (sizePacket != 29) {
					return;
				}
				P2PNodeId remoteId(packet + 1);
				sl_uint32 timeOld = MIO::readUint32LE(packet + 25);
				sl_uint32 timeNew = (sl_uint32)(GetCurrentTick());
				if (!CheckDelay(timeOld, timeNew, m_param.connectionTimeout)) {
					return;
				}
				_onReceiveReplyPing(address, remoteId, packet + 17, timeNew, timeNew - timeOld);
			}

			void _onReceiveReplyPing(const SocketAddress& address, const P2PNodeId& nodeId, sl_uint8 ephemeralKeyPrefix[8], sl_uint32 time, sl_uint32 delay)
			{
				Ref<Node> node = _getNode(nodeId);
				if (node.isNull()) {
					return;
				}
				if (!(Base::equalsMemory(node->m_remoteEphemeralKey.data, ephemeralKeyPrefix, 8))) {
					_sendVerifyNode(address, nodeId);
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

			void _onReceiveBroadcast(const SocketAddress& address, sl_uint8* packet, sl_uint32 sizePacket)
			{
				if (sizePacket <= 17) {
					return;
				}
				P2PRequest request(packet + 17, sizePacket - 17);
				request.senderId = P2PNodeId(packet + 1);
				request.connectionType = P2PConnectionType::Direct;
				request.remoteAddress = address;
				m_param.onReceiveBroadcast(this, request);
			}

			void _onReceiveDatagram(const SocketAddress& address, sl_uint8* packet, sl_uint32 sizePacket)
			{
				if (sizePacket <= 17) {
					return;
				}
				P2PRequest request(packet + 17, sizePacket - 17);
				request.senderId = P2PNodeId(packet + 1);
				request.connectionType = P2PConnectionType::Direct;
				request.remoteAddress = address;
				m_param.onReceiveDatagram(this, request);
			}

		public:
			void _onAcceptTcpServerConnection(Socket& socket)
			{
				AsyncTcpSocketParam param;
				param.socket = Move(socket);
				param.ioLoop = m_ioLoop;
				Ref<AsyncTcpSocket> client = AsyncTcpSocket::create(param);
				if (client.isNotNull()) {
					Ref<TcpServerStream> stream = new TcpServerStream(Move(client), m_param.maximumMessageSize);
					if (stream.isNotNull()) {
						if (_receiveTcpServerConnection(stream)) {
							m_mapTcpStreams.put(stream.get(), stream);
						}
					}
				}
			}

			sl_bool _receiveTcpServerConnection(const Ref<TcpServerStream>& stream)
			{
				WeakRef<TcpServerStream> weakStream = stream;
				return stream->m_socket->receive(Memory::create(m_param.messageBufferSize), [this, weakStream](AsyncStreamResult& result) {
					Ref<TcpServerStream> stream = weakStream;
					if (stream.isNull()) {
						return;
					}
					if (result.isSuccess() && !(stream->m_flagWriting)) {
						sl_int32 iRet = stream->processReceivedData((sl_uint8*)(result.data), result.size);
						if (iRet >= 0) {
							if (iRet > 0) {
								if (_onReceiveTcpServerStream(stream, result)) {
									return;
								}
							} else {
								if (stream->m_socket->receive(result.data, result.requestSize, result.callback, result.userObject)) {
									return;
								}
							}
						}
					}
					m_mapTcpStreams.remove(stream.get());
				});
			}

			sl_bool _onReceiveTcpServerStream(const Ref<TcpServerStream>& stream, AsyncStreamResult& result)
			{
				if (stream->m_currentCommand == TcpCommand::Init) {
					if (stream->getContentSize() != 20) {
						return sl_false;
					}
					sl_uint8* packet = stream->getContent();
					if (!(Base::equalsMemory(m_localNodeId.data, packet, 4))) {
						return sl_false;
					}
					stream->m_remoteId = P2PNodeId(packet + 4);
				}
				Memory buf = (CMemory*)(result.userObject);
				Function<void(AsyncStreamResult&)> callback = result.callback;
				_findNode(stream->m_remoteId, [this, stream, buf, callback](Node* node, void*) {
					if (node) {
						Memory response = _onReceiveTcpServerStream(node, stream->m_currentCommand, stream->getContent(), stream->getContentSize());
						if (response.isNotNull()) {
							stream->clear();
							m_mapTcpStreams.get(stream.get());
							if (_sendTcpServerStream(stream, response)) {
								if (stream->m_socket->receive(buf, callback)) {
									return;
								}
							}
						}
					}
					m_mapTcpStreams.remove(stream.get());
				}, 0);
				return sl_true;
			}

			Memory _onReceiveTcpServerStream(Node* node, TcpCommand command, sl_uint8* packet, sl_size sizePacket)
			{
				switch (command) {
					case TcpCommand::Init:
						{
							sl_uint8 c = (sl_uint8)(TcpCommand::ReplyInit);
							return Memory::create(&c, 1);
						}
						break;
					case TcpCommand::Message:
						{
							node->updateEncryptionKey(m_ephemeralKey.data, m_ephemeralPublicKey.data);
							P2PResponse response;
							if (sizePacket > 28) {
								AES_GCM decryptor;
								decryptor.setKey(node->m_encryptionKey, 32);
								decryptor.start(packet, 12);
								packet += 28;
								sizePacket -= 28;
								decryptor.decrypt(packet, packet, sizePacket);
								if (decryptor.finishAndCheckTag(packet - 16)) {
									P2PRequest request(packet, (sl_uint32)sizePacket);
									request.senderId = node->m_id;
									request.connectionType = P2PConnectionType::Direct;
									m_param.onReceiveMessage(this, request, response);
								} else {
									return sl_null;
								}
							} else if (!sizePacket) {
								P2PRequest request;
								request.senderId = node->m_id;
								m_param.onReceiveMessage(this, request, response);
							} else {
								return sl_null;
							}
							if (response.size) {
								sl_uint8 bufSize[16];
								sl_uint32 nSize = CVLI::serialize(bufSize, response.size + 28);
								Memory memPacket = Memory::create(29 + nSize + response.size);
								if (memPacket.isNull()) {
									return sl_null;
								}
								AES_GCM encryption;
								encryption.setKey(node->m_encryptionKey, 32);
								packet = (sl_uint8*)(memPacket.getData());
								*(packet++) = (sl_uint8)(TcpCommand::ReplyMessage);
								Base::copyMemory(packet, bufSize, nSize);
								packet += nSize;
								Math::randomMemory(packet, 12); // iv
								encryption.start(packet, 12);
								packet += 12;
								encryption.encrypt(response.data, packet + 16, response.size);
								encryption.finish(packet);
								return memPacket;
							} else {
								sl_uint8 buf[2];
								buf[0] = (sl_uint8)(TcpCommand::ReplyMessage);
								buf[1] = 0;
								return Memory::create(buf, 2);
							}
						}
						break;
					default:
						break;
				}
				return sl_null;
			}

			sl_bool _sendTcpServerStream(TcpServerStream* stream, const Memory& content)
			{
				stream->m_flagWriting = sl_true;
				WeakRef<TcpServerStream> weakStream = stream;
				return stream->m_socket->send(content, [this, weakStream](AsyncStreamResult& result) {
					Ref<TcpServerStream> stream = weakStream;
					if (stream.isNull()) {
						return;
					}
					stream->m_flagWriting = sl_false;
					if (!(result.isSuccess())) {
						m_mapTcpStreams.remove(stream.get());
					}
				});
			}

		public:
			struct TcpClientStream_InitContext
			{
				Ref<Node> node;
				Ref<DirectConnection> connection;
				WeakRef<TcpClientStream> stream;
				Shared<TimeoutMonitor> timeoutMonitor;
				Function<void(This*, Node*, DirectConnection*, TcpClientStream*)> callback;
			};

			void _getTcpClientStream(Node* node, DirectConnection* connection, const Function<void(This*, Node*, DirectConnection*, TcpClientStream*)>& callback, sl_uint64 tickEnd)
			{
				Ref<AsyncTcpSocket> socket;
				if (m_mapIdleTcpSockets.remove(connection, &socket)) {
					Ref<TcpClientStream> stream = new TcpClientStream(Move(socket), m_param.maximumMessageSize);
					if (stream.isNotNull()) {
						m_mapTcpStreams.put(stream.get(), stream);
					}
					callback(this, node, connection, stream.get());
				} else {
					AsyncTcpSocketParam param;
					param.ioLoop = m_ioLoop;
					Ref<AsyncTcpSocket> socket = AsyncTcpSocket::create(param);
					if (socket.isNotNull()) {
						TcpClientStream_InitContext context;
						context.callback = callback;
						context.node = node;
						context.connection = connection;
						Ref<TcpClientStream> stream = new TcpClientStream(Move(socket), m_param.maximumMessageSize);
						if (stream.isNotNull() && TimeoutMonitor::create(context.timeoutMonitor, tickEnd)) {
							context.stream = stream;
							if (stream->m_socket->connect(connection->m_address, [this, context](AsyncTcpSocket* socket, sl_bool flagError) {
								if (TimeoutMonitor::isFinished(context.timeoutMonitor)) {
									return;
								}
								Ref<TcpClientStream> stream = context.stream;
								if (stream.isNotNull()) {
									if (!flagError) {
										if (_sendTcpClientStream_Init(socket, context)) {
											return;
										}
									}
									m_mapTcpStreams.remove(stream.get());
								}
								if (TimeoutMonitor::tryFinish(context.timeoutMonitor)) {
									context.callback(this, context.node.get(), context.connection.get(), sl_null);
								}
							})) {
								m_mapTcpStreams.put(stream.get(), stream);
								if (context.timeoutMonitor.isNotNull()) {
									TimeoutMonitor::dispatchTimeout(context.timeoutMonitor, m_dispatchLoop, [this, context]() {
										Ref<TcpStream> stream = context.stream;
										if (stream.isNotNull()) {
											m_mapTcpStreams.remove(stream.get());
										}
										context.callback(this, context.node.get(), context.connection.get(), sl_null);
									}, tickEnd);
								}
								return;
							}
						}
					}
					callback(this, node, connection, sl_null);
				}
			}

			sl_bool _sendTcpClientStream_Init(AsyncTcpSocket* socket, const TcpClientStream_InitContext& context)
			{
				sl_uint8 packet[21];
				*packet = (sl_uint8)(TcpCommand::Init);
				Base::copyMemory(packet + 1, context.node->m_id.data, 4);
				Base::copyMemory(packet + 5, m_localNodeId.data, sizeof(P2PNodeId));
				return socket->send(Memory::create(packet, sizeof(packet)), [this, context](AsyncStreamResult& result) {
					if (TimeoutMonitor::isFinished(context.timeoutMonitor)) {
						return;
					}
					Ref<TcpClientStream> stream = context.stream;
					if (stream.isNotNull()) {
						if (result.isSuccess()) {
							if (_receiveTcpClientStream_ReplyInit(result.stream, context)) {
								return;
							}
						}
						m_mapTcpStreams.remove(stream.get());
					}
					if (TimeoutMonitor::tryFinish(context.timeoutMonitor)) {
						context.callback(this, context.node.get(), context.connection.get(), sl_null);
					}
				});
			}

			sl_bool _receiveTcpClientStream_ReplyInit(AsyncStream* stream, const TcpClientStream_InitContext& context)
			{
				return stream->read(Memory::create(16), [this, context](AsyncStreamResult& result) {
					if (TimeoutMonitor::isFinished(context.timeoutMonitor)) {
						return;
					}
					Ref<TcpClientStream> stream = context.stream;
					if (stream.isNotNull()) {
						if (result.isSuccess()) {
							if (result.size == 1 && *((sl_uint8*)(result.data)) == (sl_uint8)(TcpCommand::ReplyInit)) {
								if (TimeoutMonitor::tryFinish(context.timeoutMonitor)) {
									context.callback(this, context.node.get(), context.connection.get(), stream.get());
								}
								return;
							}
						}
						m_mapTcpStreams.remove(stream.get());
					}
					if (TimeoutMonitor::tryFinish(context.timeoutMonitor)) {
						context.callback(this, context.node.get(), context.connection.get(), sl_null);
					}
				});
			}

			struct TcpClientStream_MessageContext
			{
				Ref<Node> node;
				Ref<DirectConnection> connection;
				WeakRef<TcpClientStream> stream;
				Shared<TimeoutMonitor> timeoutMonitor;
				Function<void(P2PResponse&)> callback;
			};

			sl_bool _sendTcpClientStream_Message(TcpClientStream* stream, const TcpClientStream_MessageContext& context, const MessageBody& body)
			{
				Memory memPacket;
				sl_uint8* packet;
				if (body.lengthOfSize) {
					memPacket = body.packet;
					packet = (sl_uint8*)(memPacket.getData());
					Math::randomMemory(packet + 1 + body.lengthOfSize, 12);
					context.node->updateEncryptionKey(m_ephemeralKey.data, m_ephemeralPublicKey.data);
					AES_GCM enc;
					enc.setKey(context.node->m_encryptionKey, 32);
					enc.start(packet + 1 + body.lengthOfSize, 12);
					sl_uint8* content = packet + 29 + body.lengthOfSize;
					enc.encrypt(content, content, memPacket.getSize() - 29 - body.lengthOfSize);
					enc.finish(packet + 13 + body.lengthOfSize);
				} else {
					memPacket = Memory::create(2);
					if (memPacket.isNull()) {
						return sl_false;
					}
					packet = (sl_uint8*)(memPacket.getData());
					packet[1] = 0;
				}
				packet[0] = (sl_uint8)(TcpCommand::Message);
				return stream->m_socket->send(memPacket, [this, context](AsyncStreamResult& result) {
					if (TimeoutMonitor::isFinished(context.timeoutMonitor)) {
						return;
					}
					Ref<TcpClientStream> stream = context.stream;
					if (stream.isNotNull()) {
						if (result.isSuccess()) {
							if (_receiveTcpClientStream_ReplyMessage(stream.get(), context)) {
								return;
							}
						}
						m_mapTcpStreams.remove(stream.get());
					}
					if (TimeoutMonitor::tryFinish(context.timeoutMonitor)) {
						ReplyErrorResponse(context.callback);
					}
				});
			}

			sl_bool _receiveTcpClientStream_ReplyMessage(TcpClientStream* stream, const TcpClientStream_MessageContext& context)
			{
				return stream->m_socket->receive(Memory::create(m_param.messageBufferSize), [this, context](AsyncStreamResult& result) {
					if (TimeoutMonitor::isFinished(context.timeoutMonitor)) {
						return;
					}
					Ref<TcpClientStream> stream = context.stream;
					if (stream.isNotNull()) {
						if (result.isSuccess()) {
							sl_int32 iRet = stream->processReceivedData((sl_uint8*)(result.data), result.size);
							if (iRet >= 0) {
								if (iRet > 0) {
									if (TimeoutMonitor::tryFinish(context.timeoutMonitor)) {
										if (stream->m_currentCommand == TcpCommand::ReplyMessage) {
											sl_uint8* packet = stream->getContent();
											sl_uint32 size = (sl_uint32)(stream->getContentSize());
											sl_bool flagSuccess = sl_true;
											if (size > 28) {
												AES_GCM enc;
												enc.setKey(context.node->m_encryptionKey, 32);
												enc.start(packet, 12);
												sl_uint8* content = packet + 28;
												size -= 28;
												enc.decrypt(content, content, size);
												if (enc.finishAndCheckTag(packet + 12)) {
													P2PResponse response(content, size);
													response.connectionType = P2PConnectionType::Direct;
													context.callback(response);
												} else {
													flagSuccess = sl_false;
												}
											} else if (!size) {
												P2PResponse response;
												response.connectionType = P2PConnectionType::Direct;
												context.callback(response);
											} else {
												flagSuccess = sl_false;
											}
											stream->clear();
											if (flagSuccess) {
												m_mapTcpStreams.get(stream.get());
												m_mapIdleTcpSockets.add(context.connection.get(), stream->m_socket);
												return;
											}
										}
									}
								} else {
									if (stream->m_socket->receive(result.data, result.requestSize, result.callback, result.userObject)) {
										return;
									}
								}
							}
						}
						m_mapTcpStreams.remove(stream.get());
					}
					if (TimeoutMonitor::tryFinish(context.timeoutMonitor)) {
						ReplyErrorResponse(context.callback);
					}
				});
			}

		public:
			sl_bool _isLocalAddress(const IPv4Address& ip)
			{
				return List<IPv4Address>(m_listLocalIPAddresses).contains(ip);
			}

			sl_bool _isValidSenderAddress(const SocketAddress& address)
			{
				IPv4Address ip = address.ip.getIPv4();
				if (ip.isLoopback()) {
					return m_portActor != address.port;
				}
				if (_isLocalAddress(ip)) {
					return sl_false;
				}
				return sl_true;
			}

			void _deriveEncryptionKey(const sl_uint8 remoteEphemeralKey[EdDH_KEY_LEN], sl_uint8 key[32])
			{
				DeriveKey(m_ephemeralKey.data, remoteEphemeralKey, key);
			}

			Ref<Node> _getNode(const P2PNodeId& nodeId)
			{
				return m_mapNodes.getValue(nodeId, sl_null, sl_false);
			}

			Ref<Node> _createNode(const sl_uint8 remoteKey[EdDSA_KEY_LEN])
			{
				P2PNodeId nodeId(remoteKey);
				Ref<Node> node = m_mapNodes.getValue(nodeId, sl_null, sl_true);
				if (node.isNotNull()) {
					if (Base::equalsMemory(node->m_publicKey.data, remoteKey, EdDSA_KEY_LEN)) {
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
				return CheckDelay(connection->m_timeLastPing, GetCurrentTick(), m_param.connectionTimeout);
			}

			void _findNode(const P2PNodeId& nodeId, const NodeCallback& callback, sl_uint64 tickEnd)
			{
				Ref<Node> node = m_mapNodes.getValue(nodeId, sl_null, sl_true);
				if (node.isNotNull()) {
					callback(node.get(), sl_null);
					return;
				}
				sl_bool flagShortTimeout = sl_false;
				sl_uint32 timeout = 0;
				if (tickEnd) {
					sl_uint64 cur = GetCurrentTick();
					if (tickEnd <= cur) {
						callback(sl_null, sl_null);
						return;
					}
					if (tickEnd < cur + m_param.findTimeout) {
						flagShortTimeout = sl_true;
						timeout = (sl_uint32)(tickEnd - cur);
					}
				} else {
					tickEnd = GetCurrentTick() + 5 * m_param.findTimeout;
				}
				WeakRef<This> weakThis = this;
				if (flagShortTimeout) {
					Shared<TimeoutMonitor> monitorResult = Shared<TimeoutMonitor>::create();
					if (monitorResult.isNull()) {
						callback(sl_null, sl_null);
						return;
					}
					m_mapFindCallbacks.add(nodeId, [weakThis, nodeId, callback, monitorResult](Node* node, void*) {
						if (monitorResult->tryFinish()) {
							Ref<This> thiz = weakThis;
							callback(thiz.isNotNull() ? node : sl_null, sl_null);
						}
					});
					m_dispatchLoop->dispatch([callback, monitorResult]() {
						if (monitorResult->tryFinish()) {
							callback(sl_null, sl_null);
						}
					}, timeout);
				} else {
					m_mapFindCallbacks.add(nodeId, [weakThis, nodeId, callback, tickEnd](Node* node, void*) {
						Ref<This> thiz = weakThis;
						if (thiz.isNull()) {
							callback(sl_null, sl_null);
							return;
						}
						if (node) {
							callback(node, sl_null);
						} else {
							thiz->_findNode(nodeId, callback, tickEnd);
						}
					});
				}
				_sendFindNode(nodeId);
			}

			void _completeFindNodeCallbacks(const P2PNodeId& nodeId, Node* node)
			{
				NodeCallbackContainer container;
				while (m_mapFindCallbacks.remove(nodeId, &container)) {
					container.success(node, sl_null);
				}
			}

			void _sendMessage(Node* node, const MessageBody& body, const Function<void(P2PResponse&)>& callback, sl_uint64 tickEnd)
			{
				Ref<Connection> connection = node->m_connectionDefault;
				if (connection.isNotNull()) {
					if (_isValidConnection(connection.get())) {
						if (connection->m_type == P2PConnectionType::Direct) {
							_sendMessageDirectConnection(node, (DirectConnection*)(connection.get()), body, callback, tickEnd);
							return;
						}
					}
				}
				ReplyErrorResponse(callback);
			}

			void _sendMessageDirectConnection(Node* node, DirectConnection* connection, const MessageBody& body, const Function<void(P2PResponse&)>& callback, sl_uint64 tickEnd)
			{
				_getTcpClientStream(node, connection, [this, body, callback, tickEnd](This* thiz, Node* node, DirectConnection* connection, TcpClientStream* stream) {
					TcpClientStream_MessageContext context;
					context.node = node;
					context.connection = connection;
					context.stream = stream;
					context.callback = callback;
					if (stream) {
						if (TimeoutMonitor::create(context.timeoutMonitor, tickEnd)) {
							if (_sendTcpClientStream_Message(stream, context, body)) {
								if (context.timeoutMonitor.isNotNull()) {
									TimeoutMonitor::dispatchTimeout(context.timeoutMonitor, m_dispatchLoop, [this, context]() {
										Ref<TcpStream> stream = context.stream;
										if (stream.isNotNull()) {
											m_mapTcpStreams.remove(stream.get());
										}
										ReplyErrorResponse(context.callback);
									}, tickEnd);
								}
								return;
							}
						}
						m_mapTcpStreams.remove(stream);
					}
					ReplyErrorResponse(callback);
				}, tickEnd);
			}

		public:
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
				if (m_threadPool.isNotNull()) {
					m_threadPool->release();
				}

				m_mapTcpStreams.removeAll();
				m_mapIdleTcpSockets.removeAll();
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
				m_timerHello->start();

				return sl_true;
			}

			void setHelloMessage(const P2PMessage& msg) override
			{
				sl_uint32 size = msg.size;
				if (size > sizeof(m_helloMessage)) {
					size = sizeof(m_helloMessage);
				}
				Base::copyMemory(m_helloMessage, msg.data, size);
				m_sizeHelloMessage = size;
			}

			void setLocalNodeDescription(const P2PMessage& msg) override
			{
				sl_uint32 size = msg.size;
				if (size > sizeof(m_localNodeDescription)) {
					size = sizeof(m_localNodeDescription);
				}
				Base::copyMemory(m_localNodeDescription, msg.data, size);
				m_sizeLocalNodeDescription = size;
				_updateEphemeralKey();
			}

			void _updateEphemeralKey()
			{
				Math::randomMemory(m_ephemeralKey.data, EdDH_KEY_LEN);
				m_ephemeralPublicKey = EdDH::getPublicKey(m_ephemeralKey.data);
			}

			void sendMessage(const P2PNodeId& nodeId, const P2PRequest& msg, const Function<void(P2PResponse&)>& callback, sl_uint32 timeoutMillis) override
			{
				if (m_flagClosed) {
					ReplyErrorResponse(callback);
					return;
				}
				MessageBody body;
				if (msg.size) {
					sl_uint8 bufSize[16];
					body.lengthOfSize = CVLI::serialize(bufSize, msg.size + 28);
					body.packet = Memory::create(29 + body.lengthOfSize + msg.size);
					if (body.packet.isNull()) {
						ReplyErrorResponse(callback);
						return;
					}
					sl_uint8* packet = (sl_uint8*)(body.packet.getData());
					Base::copyMemory(packet + 1, bufSize, body.lengthOfSize);
					Base::copyMemory(packet + 29 + body.lengthOfSize, msg.data, msg.size);
				}
				sl_uint64 tickEnd;
				if (timeoutMillis) {
					tickEnd = GetCurrentTick() + timeoutMillis;
				} else {
					tickEnd = 0;
				}
				WeakRef<This> weakThis = this;
				_findNode(nodeId, [weakThis, body, callback, tickEnd](Node* node, void*) {
					Ref<This> thiz = weakThis;
					if (thiz.isNotNull() && node) {
						thiz->_sendMessage(node, body, callback, tickEnd);
					} else {
						ReplyErrorResponse(callback);
					}
				}, tickEnd);
			}

			void sendMessage(const P2PNodeId& nodeId, const P2PRequest& msg, P2PResponse& response, sl_uint32 timeoutMillis) override
			{
				if (m_flagClosed) {
					return;
				}
				Ref<Event> ev = Event::create();
				if (ev.isNull()) {
					return;
				}
				Shared<TimeoutMonitor> timeoutMonitor = Shared<TimeoutMonitor>::create();
				if (timeoutMonitor.isNull()) {
					return;
				}
				P2PResponse* ret = &response;
				sendMessage(nodeId, msg, [timeoutMonitor, ret, ev](P2PResponse& response) {
					if (timeoutMonitor->tryFinish()) {
						*ret = Move(response);
						ev->set();
					}
				}, timeoutMillis);
				ev->wait();
				timeoutMonitor->tryFinish();
			}

			void sendBroadcast(const P2PRequest& msg) override
			{
				if (m_flagClosed) {
					return;
				}
				SLIB_SCOPED_BUFFER(sl_uint8, 1024, packet, 17 + msg.size);
				if (!packet) {
					return;
				}
				packet[0] = (sl_uint8)(Command::Broadcast);
				Base::copyMemory(packet + 1, m_localNodeId.data, sizeof(P2PNodeId));
				Base::copyMemory(packet + 17, msg.data, msg.size);
				_sendBroadcast(packet, 17 + msg.size);
			}

			void sendDatagram(const SocketAddress& address, const P2PRequest& msg) override
			{
				if (m_flagClosed) {
					return;
				}
				SLIB_SCOPED_BUFFER(sl_uint8, 1024, packet, 17 + msg.size);
				if (!packet) {
					return;
				}
				packet[0] = (sl_uint8)(Command::Datagram);
				Base::copyMemory(packet + 1, m_localNodeId.data, sizeof(P2PNodeId));
				Base::copyMemory(packet + 17, msg.data, msg.size);
				_sendUdp(address, packet, 17 + msg.size);
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

	P2PMessage::P2PMessage() : data(sl_null), size(0), connectionType(P2PConnectionType::Unknown), flagNotJson(sl_false)
	{
	}

	P2PMessage::P2PMessage(const void* _data, sl_uint32 _size, CRef* _ref) : data(_data), size(_size), ref(_ref), connectionType(P2PConnectionType::Unknown), flagNotJson(sl_false)
	{
	}

	void P2PMessage::clear()
	{
		data = sl_null;
		size = 0;
		ref.setNull();
		mem.setNull();
		str.setNull();
		json.setNull();
		flagNotJson = sl_false;
	}

	void P2PMessage::setContent(const void* _data, sl_uint32 _size, CRef* _ref)
	{
		clear();
		data = _data;
		size = _size;
		ref = _ref;
	}

	void P2PMessage::setContent(const Variant& var)
	{
		clear();
		PrepareMessageContent(var, *this);
	}

	void P2PMessage::setContent(P2PMessage& content)
	{
		data = content.data;
		size = content.size;
		ref = content.ref;
		mem = content.mem;
		str = content.str;
		json = content.json;
		flagNotJson = content.flagNotJson;
	}

	Memory P2PMessage::getMemory()
	{
		if (data && size) {
			if (mem.isNotNull()) {
				if (data == mem.getData() && size == mem.getSize()) {
					return mem;
				} else {
					return Memory::createStatic(data, size, mem.ref.get());
				}
			} else {
				if (ref.isNotNull()) {
					mem = Memory::createStatic(data, size, ref.get());
				} else {
					mem = Memory::create(data, size);
				}
			}
			return mem;
		}
		return sl_null;
	}

	void P2PMessage::setMemory(const Memory& _mem)
	{
		clear();
		data = _mem.getData();
		size = (sl_uint32)(_mem.getSize());
		mem = _mem;
	}

	String P2PMessage::getString()
	{
		if (data && size) {
			if (str.isNotNull()) {
				if (data == str.getData() && size == str.getLength()) {
					return str;
				}
			}
			str = String::fromUtf8(data, size);
			return str;
		}
		return sl_null;
	}

	void P2PMessage::setString(const String& _str)
	{
		clear();
		data = _str.getData();
		size = (sl_uint32)(_str.getLength());
		str = _str;
	}

	Json P2PMessage::getJson()
	{
		if (flagNotJson) {
			return sl_null;
		}
		if (json.isNotNull()) {
			return json;
		}
		if (json.deserialize(getMemory())) {
			return json;
		}
		flagNotJson = sl_true;
		return sl_null;
	}

	void P2PMessage::setJson(const Json& _json)
	{
		clear();
		if (_json.isNotNull()) {
			Memory _mem = _json.serialize();
			if (_mem.isNotNull()) {
				setMemory(_mem);
				json = _json;
			}
		}
	}

	void P2PMessage::setJson(const Json& _json, const Memory& _mem)
	{
		clear();
		if (_json.isNotNull()) {
			if (_mem.isNotNull()) {
				setMemory(_mem);
				json = _json;
			}
		}
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
		messageBufferSize = 0x10000;
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

}
