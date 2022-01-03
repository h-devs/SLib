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

#include "slib/network/p2p.h"

#include "slib/network/socket.h"
#include "slib/network/event.h"
#include "slib/network/async.h"
#include "slib/network/os.h"
#include "slib/core/thread.h"
#include "slib/core/serialize.h"
#include "slib/core/dispatch_loop.h"
#include "slib/core/expiring_map.h"
#include "slib/core/lender.h"
#include "slib/crypto/chacha.h"

/*
			P2P Socket Protocol

- FindNode
Offset	Length(B)	Type			Value
-------------------------------------------------------
0		1			Command			0 (FindNode) 
1		16			NodeId			RemoteNode

- ReplyFindNode
Offset	Length(B)	Type			Value
-------------------------------------------------------
0		1			Command			1 (ReplyFindNode)
1		Serialize	PublicKey		LocalNode

- VerifyNode
Offset	Length(B)	Type			Value
-------------------------------------------------------
0		1			Command			2 (VerifyNode)
1		16			NodeId			RemoteNode
17		8			Time			LocalNode's Current Milliseconds (Little Endian)
25		4			IPv4Address		RemoteNode
29		2			Port			RemoteNode's UDP Port (Little Endian)
31		Serialize	PublicKey		LocalNode

- ReplyVerifyNode
Offset	Length(B)	Type			Value
-------------------------------------------------------
0		1			Command			3 (ReplyVerifyNode)
1		16			NodeId			RemoteNode
17		12			Encryption IV	Random
29		16			Encryption Content: By ChaCha20-Poly1305: key=ECDH(PrivKey(LocalNode), PubKey(RemoteNode))
45		16			Encryption Tag
61		Serialize	PublicKey		LocalNode
-------------	Encrypted Content ---------------------
0		8			Time			RemoteNode's Current Milliseconds (Little Endian))
8		4			IPv4Address		LocalNode
12		2			Port			LocalNode's UDP Port (Little Endian)
14		2			Port			LocalNode's TCP Port (Little Endian)

- Ping
Offset	Length(B)	Type			Value
-------------------------------------------------------
0		1			Command			4 (Ping)
1		16			NodeId			RemoteNode

- ReplyPing
Offset	Length(B)	Type			Value
-------------------------------------------------------
0		1			Command			5 (ReplyPing)
1		16			NodeId			LocalNode

*/

#define TIMEOUT_VERIFY_LAN_NODE 10000
#define TIMEOUT_VALID_LAN_NODE 5000

namespace slib
{

	namespace priv
	{
		namespace p2p
		{

			enum class Command
			{
				FindNode = 0,
				ReplyFindNode = 1,
				VerifyNode = 2,
				ReplyVerifyNode = 3,
				Ping = 4,
				ReplyPing = 5
			};

			// `out`: 16 bytes
			static void GetNodeId(const ECPublicKey& key, sl_uint8* out)
			{
				key.Q.x.getBytesBE(out, 16);
			}

			// `out`: 32 bytes
			static void DeriveKey(const ECPrivateKey& local, const ECPublicKey& remote, sl_uint8* out)
			{
				ECDH::getSharedKey(EllipticCurve::secp256k1(), local, remote).getBytesBE(out, 32);
			}

			static void PrepareMessageContent(const Variant& var, P2PMessage& content)
			{
				if (var.isNotNull()) {
					if (var.isMemory()) {
						content.setMemory(var.getMemory());
					} else if (var.isObject() || var.isCollection()) {
						Json json(var);
						String str = json.toJsonString();
						content.setJson(Move(json), Move(str));
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

			typedef Function<void(sl_bool flagSuccess)> BoolCallback;

			class BoolCallbackContainer
			{
			public:
				BoolCallbackContainer() {}

				BoolCallbackContainer(BoolCallback&& callback): m_callback(Move(callback)) {}

				~BoolCallbackContainer()
				{
					m_callback(sl_false);
				}

				SLIB_DEFINE_CLASS_DEFAULT_MEMBERS_INLINE(BoolCallbackContainer)

			public:
				void success()
				{
					m_callback(sl_true);
					m_callback.setNull();
				}

			protected:
				BoolCallback m_callback;

			};

			typedef Function<void(Node*)> NodeCallback;

			class NodeCallbackContainer
			{
			public:
				NodeCallbackContainer() {}

				NodeCallbackContainer(NodeCallback&& callback) : m_callback(Move(callback)) {}

				~NodeCallbackContainer()
				{
					m_callback(sl_null);
				}

				SLIB_DEFINE_CLASS_DEFAULT_MEMBERS_INLINE(NodeCallbackContainer)

			public:
				void success(Node* node)
				{
					m_callback(node);
					m_callback.setNull();
				}

			protected:
				NodeCallback m_callback;

			};

			typedef Function<void(P2PResponse& response)> ResponseCallback;

			enum class ConnectionType
			{
				Lan = 0
			};

			class Connection : public Referable
			{
			public:
				WeakRef<Node> m_node;
				ConnectionType m_type;
				Time m_timeLastPing;

			public:
				Connection(Node* node, ConnectionType type);

			public:
				void setLastPingTime();

			};

			class LanConnection;

			class Node : public Referable
			{
			public:
				WeakRef<P2PSocketImpl> m_socket;
				P2PPublicKey m_key;
				P2PNodeId m_id;

				AtomicRef<LanConnection> m_connectionDefault;
				AtomicRef<LanConnection> m_connectionLan;

				Time m_timeLastPing;
				Time m_timeLastPingDefault;

			public:
				Node(P2PSocketImpl* socket, const P2PPublicKey& key): m_socket(socket), m_key(key)
				{
					GetNodeId(key, m_id.data);
					m_timeLastPing = Time::now();
				}
				
			public:
				Ref<LanConnection> createLanConnection(const IPv4Address& ip, sl_uint16 portUdp, sl_uint16 portTcp)
				{
					Ref<LanConnection> connection = m_connectionLan;
					if (connection.isNotNull()) {
						if (connection->m_ip != ip || connection->m_portUdp != portUdp || connection->m_portTcp != portTcp) {
							connection.setNull();
						}
					}
					if (connection.isNull()) {
						connection = new LanConnection(this, ip, portUdp, portTcp);
					}
					if (connection.isNotNull()) {
						m_connectionDefault = connection;
						m_connectionLan = connection;
						connection->setLastPingTime();
						return connection;
					}
					return sl_null;
				}

			};

			Connection::Connection(Node* node, ConnectionType type): m_node(node), m_type(type)
			{
				setLastPingTime();
			}

			void Connection::setLastPingTime()
			{
				Time now = Time::now();
				m_timeLastPing = now;
				Ref<Node> node = m_node;
				if (node.isNotNull()) {
					node->m_timeLastPing = now;
					if (node->m_connectionDefault == this) {
						node->m_timeLastPingDefault = now;
					}
				}
			}

			class LanConnection : public Connection
			{
			public:
				IPv4Address m_ip;
				sl_uint16 m_portUdp;
				sl_uint16 m_portTcp;
				
			public:
				LanConnection(Node* node, const IPv4Address& ip, sl_uint16 portUdp, sl_uint16 portTcp): Connection(node, ConnectionType::Lan), m_ip(ip), m_portUdp(portUdp), m_portTcp(portTcp)
				{
				}

			public:
				sl_bool checkValid() override
				{
					return (Time::now() - m_timeReceivedPing).getMillisecondsCount() < TIMEOUT_VALID_LAN_NODE;
				}

				void sendPing() override;

				void sendMessage(const Memory& mem, const ResponseCallback& callback) override;

			};

			class P2PSocketImpl : public P2PSocket
			{
			public:
				P2PPrivateKey m_key;
				P2PNodeId m_localNodeId;

				Socket m_socketUdp;
				Ref<SocketEvent> m_eventUdp;
				sl_uint16 m_portUdp;
				sl_uint16 m_portUdpHost;
				sl_bool m_flagUdpHost;

				Ref<AsyncTcpServer> m_serverTcp;
				sl_uint16 m_portTcp;

				Ref<Thread> m_threadReceiveUdp;
				sl_uint8 m_bufReceiveUdp[65536];

				Ref<DispatchLoop> m_dispatcher;

				CHashMap< P2PNodeId, Ref<Node> > m_mapNodes;
				CHashMap< P2PNodeId, Bytes<32> > m_mapEncryptionKey;
				ExpiringMap<P2PNodeId, NodeCallbackContainer> m_mapVerifyCallbacks;
				ExpiringMap<Node*, BoolCallbackContainer> m_mapPingCallbacks;

			public:
				P2PSocketImpl()
				{
					m_flagUdpHost = sl_false;
					m_portUdp = 0;
					m_portTcp = 0;
				}

				~P2PSocketImpl()
				{
					if (m_serverTcp.isNotNull()) {
						m_serverTcp->close();
					}
				}

			public:
				static Ref<P2PSocketImpl> open(P2PSocketParam& param)
				{
					if (param.key.isNull()) {
						if (!(param.key.generate())) {
							SLIB_STATIC_STRING(err, "Failed to generate private key")
							param.errorText = err;
							return sl_null;
						}
					} else {
						if (!(param.key.checkValid())) {
							SLIB_STATIC_STRING(err, "key is invalid")
							param.errorText = err;
							return sl_null;
						}
					}
					sl_bool flagUdpHost = sl_false;
					Socket socket = Socket::openUdp(param.port);
					if (socket.isNotNone()) {
						flagUdpHost = sl_true;
						param.boundUdpPort = param.port;
					} else {
						for (sl_uint16 i = 1; i < param.portCount; i++) {
							socket = Socket::openUdp(param.port + i);
							if (socket.isNotNone()) {
								param.boundUdpPort = param.port + i;
								break;
							}
						}
						if (socket.isNone()) {
							SLIB_STATIC_STRING(err, "Failed to bind the UDP socket")
							param.errorText = err;
							return sl_null;
						}
					}
					Ref<SocketEvent> ev = SocketEvent::createRead(socket);
					if (ev.isNull()) {
						SLIB_STATIC_STRING(err, "Failed to create socket event")
						param.errorText = err;
						return sl_null;
					}
					Ref<DispatchLoop> dispatcher = DispatchLoop::create(sl_false);
					if (dispatcher.isNull()) {
						SLIB_STATIC_STRING(err, "Failed to create dispatch loop")
						param.errorText = err;
						return sl_null;
					}
					Ref<P2PSocketImpl> ret = new P2PSocketImpl;
					if (ret.isNotNull()) {
						ret->m_socketUdp = Move(socket);
						ret->m_eventUdp = Move(ev);
						ret->m_flagUdpHost = flagUdpHost;
						ret->m_dispatcher = Move(dispatcher);
						ret->_initialize(param);
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
				void _initialize(P2PSocketParam& param)
				{
					m_key = param.key;
					GetNodeId(param.key, m_localNodeId.data);

					m_mapPingCallbacks.setupTimer(1000, m_dispatcher);
					m_mapVerifyCallbacks.setupTimer(3000, m_dispatcher);

					// Initialize UDP Socket
					{
						m_socketUdp.setNonBlockingMode();
						m_socketUdp.setOption_Broadcast();
						m_portUdp = param.boundUdpPort;
						m_portUdpHost = param.port;
					}

					// Initialize TCP Server
					{
						AsyncTcpServerParam serverParam;
						serverParam.onAccept = [this](AsyncTcpServer*, Socket& socket, const SocketAddress& address) {
							
						};
						for (sl_uint16 i = 0; i < param.portCount; i++) {
							serverParam.bindAddress.port = param.port + i;
							m_serverTcp = AsyncTcpServer::create(serverParam);
							if (m_serverTcp.isNotNull()) {
								param.boundTcpPort = param.port + i;
								m_portTcp = param.port + i;
								break;
							}
						}
					}
				}

				void _runReceiveUdp()
				{
					Thread* thread = Thread::getCurrent();
					if (!thread) {
						return;
					}
					Socket& socket = m_socketUdp;
					Ref<SocketEvent>& ev = m_eventUdp;
					sl_uint8* buf = m_bufReceiveUdp;
					sl_uint32 sizeBuf = sizeof(m_bufReceiveUdp);
					SocketAddress address;
					while (thread->isNotStopping()) {
						sl_int32 nReceive = socket.receiveFrom(address, buf, sizeBuf);
						if (nReceive > 0) {
							_processReceivedUdp(address, buf, nReceive);
						} else {
							ev->wait();
						}
					}
				}

				void _processReceivedUdp(SocketAddress& address, sl_uint8* packet, sl_uint32 sizePacket)
				{
					Command cmd = (Command)(packet[0]);
					switch (cmd) {
					case Command::FindNode:
						if (sizePacket == 17) {
							if (Base::equalsMemory(m_localNodeId.data, packet + 1, sizeof(P2PNodeId))) {
								packet[0] = (sl_uint8)(Command::ReplyFindNode);
								SerializeBuffer bufWrite(packet + 1, 1024);
								if (m_key.toPublicKey().serialize(&bufWrite)) {
									_sendUdp(address, packet, bufWrite.current - packet);
								}
							}
						}
						break;
					case Command::ReplyFindNode:
						if (sizePacket > 1) {
							DeserializeBuffer bufRead(packet + 1, sizePacket - 1);
							P2PPublicKey remoteKey;
							if (remoteKey.deserialize(&bufRead)) {
								packet[0] = (sl_uint8)(Command::VerifyNode);
								GetNodeId(remoteKey, packet + 1);
								sl_uint64 time = Time::now().getMillisecondsCount();
								MIO::writeUint64LE(packet + 17, time);
								IPv4Address remoteIP = address.ip.getIPv4();
								packet[25] = remoteIP.a;
								packet[26] = remoteIP.b;
								packet[27] = remoteIP.c;
								packet[28] = remoteIP.d;
								MIO::writeUint16LE(packet + 29, address.port);
								SerializeBuffer bufWrite(packet + 31, 1024);
								if (m_key.toPublicKey().serialize(&bufWrite)) {
									_sendUdp(address, packet, bufWrite.current - packet);
								}
							}
						}
						break;
					case Command::VerifyNode:
						if (sizePacket > 31) {
							if (Base::equalsMemory(m_localNodeId.data, packet + 1, sizeof(P2PNodeId))) {
								sl_uint8 contentToEncrypt[16];
								Base::copyMemory(contentToEncrypt, packet + 17, 14);
								DeserializeBuffer bufRead(packet + 31, sizePacket - 31);
								P2PPublicKey remoteKey;
								if (remoteKey.deserialize(&bufRead)) {
									ChaCha20_Poly1305 encryptor;
									sl_uint8 key[32];
									_deriveEncryptionKey(remoteKey, key);
									encryptor.setKey(key);
									packet[0] = (sl_uint8)(Command::ReplyVerifyNode);
									GetNodeId(remoteKey, packet + 1);
									sl_uint8* iv = packet + 17;
									Math::randomMemory(iv, 12);
									encryptor.start(iv);
									MIO::writeUint16LE(contentToEncrypt + 14, m_portTcp);
									encryptor.encrypt(contentToEncrypt, packet + 29, sizeof(contentToEncrypt));
									encryptor.finish(packet + 45);
									SerializeBuffer bufWrite(packet + 61, 1024);
									if (m_key.toPublicKey().serialize(&bufWrite)) {
										_sendUdp(address, packet, bufWrite.current - packet);
									}
								}
							}
						}
						break;
					case Command::ReplyVerifyNode:
						if (sizePacket > 61) {
							if (Base::equalsMemory(m_localNodeId.data, packet + 1, sizeof(P2PNodeId))) {
								DeserializeBuffer bufRead(packet + 61, sizePacket - 61);
								P2PPublicKey remoteKey;
								if (remoteKey.deserialize(&bufRead)) {
									ChaCha20_Poly1305 decryptor;
									sl_uint8 key[32];
									_deriveEncryptionKey(remoteKey, key);
									decryptor.setKey(key);
									decryptor.start(packet + 17); // iv
									sl_uint8 content[16];
									decryptor.decrypt(packet + 29, content, sizeof(content));
									if (decryptor.finishAndCheckTag(packet + 45)) {
										IPv4Address remoteIP = address.ip.getIPv4();
										if (IPv4Address(content[8], content[9], content[10], content[11]) == remoteIP) {
											if (MIO::readUint16LE(content + 12) == address.port) {
												sl_uint64 timeNew = Time::now().getMillisecondsCount();
												sl_uint64 timeOld = MIO::readUint64LE(content);
												if (timeNew >= timeOld && timeNew < timeOld + TIMEOUT_VERIFY_LAN_NODE) {
													sl_uint16 portTcp = MIO::readUint16LE(content + 14);
													_onVerifyLanConnection(remoteKey, remoteIP, address.port, portTcp);
												}
											}
										}
									}
								}
							}
						}
						break;
					case Command::Ping:
						if (sizePacket == 17) {
							if (Base::equalsMemory(m_localNodeId.data, packet + 1, sizeof(P2PNodeId))) {
								packet[0] = (sl_uint8)(Command::ReplyPing);
								_sendUdp(address, packet, 17);
							}
						}
						break;
					case Command::ReplyPing:
						if (sizePacket == 17) {
							_onPingLanNode(P2PNodeId(packet + 1));
						}
						break;
					default:
						break;
					}
				}

				Ref<Node> _createNode(const P2PPublicKey& key)
				{
					P2PNodeId nodeId;
					GetNodeId(key, nodeId.data);
					Ref<Node> node = m_mapNodes.getValue(nodeId);
					if (node.isNotNull()) {
						if (node->m_key == key) {
							return node;
						}
					}
					Ref<Node> node = new Node(this, key);
					if (node.isNotNull()) {
						m_mapNodes.put(nodeId, node);
						return node;
					}
					return sl_null;
				}

				void _onVerifyLanConnection(const P2PPublicKey& key, const IPv4Address& ip, sl_uint16 portUdp, sl_uint16 portTcp)
				{
					P2PNodeId nodeId;
					GetNodeId(key, nodeId.data);
					Ref<Node> node = _createNode(key);
					if (node.isNull()) {
						_notifyVerifyCallbacks(nodeId, sl_null);
						return;
					}
					Ref<LanConnection> connection = node->createLanConnection(ip, portUdp, portTcp);
					if (connection.isNotNull()) {
						_notifyVerifyCallbacks(nodeId, node.get());
					} else {
						_notifyVerifyCallbacks(nodeId, sl_null);
					}
				}

				void _notifyVerifyCallbacks(const P2PNodeId& nodeId, Node* node)
				{
					NodeCallbackContainer container;
					while (m_mapVerifyCallbacks.remove(nodeId, &container)) {
						container.success(node);
					}
				}

				void _onPingLanNode(const P2PNodeId& nodeId)
				{
					Ref<Node> node = m_mapNodes.getValue(nodeId);
					if (node.isNotNull()) {
						node->m_timeReceivedPing = Time::now();
						BoolCallbackContainer container;
						while (m_mapPingCallbacks.remove(node.get(), &container)) {
							container.success();
						}
					}
				}

				void _deriveEncryptionKey(const ECPublicKey& remoteKey, sl_uint8* key)
				{
					P2PNodeId remoteId;
					GetNodeId(remoteKey, remoteId.data);
					// Check saved key
					{
						MutexLocker lock(m_mapEncryptionKey.getLocker());
						Bytes<32>* savedKey = m_mapEncryptionKey.getItemPointer(remoteId);
						if (savedKey) {
							Base::copyMemory(key, savedKey->data, 32);
							return;
						}
					}
					DeriveKey(m_key, remoteKey, key);
					if (m_mapEncryptionKey.getCount() > 10000) {
						m_mapEncryptionKey.removeAll();
					}
					m_mapEncryptionKey.put(remoteId, key);
				}

				void _sendUdp(const SocketAddress& address, sl_uint8* buf, sl_size size)
				{
					m_socketUdp.sendTo(address, buf, size);
				}

				void _sendBroadcast(sl_uint8* buf, sl_size size)
				{
					SocketAddress address;
					address.port = m_portUdpHost;
					ListElements<IPv4AddressInfo> addrs(Network::findAllIPv4AddressInfos());
					for (sl_size j = 0; j < addrs.count; j++) {
						sl_uint32 m = addrs[j].address.getInt();
						sl_uint32 n = addrs[j].networkPrefixLength;
						if (n < 32) {
							m = m | ((1 << (32 - n)) - 1);
							address.ip = m;
							_sendUdp(address, buf, size);
						}
					}
				}

				void _findNode(const P2PNodeId& nodeId, const NodeCallback& callback)
				{
					Ref<Node> node = m_mapNodes.getValue(nodeId);
					if (node.isNotNull()) {
						if (node->checkValid()) {
							callback(node.get());
							return;
						}
						_sendPing(node.get(), [callback, nodeId, node](sl_bool flagSuccess) {
							if (flagSuccess) {
								callback(node.get());
							} else {
								Ref<P2PSocketImpl> socket = node->m_socket;
								if (socket.isNull()) {
									callback(sl_null);
									return;
								}
								socket->_findNode1(nodeId, callback, 3);
							}
						}, 3);
					} else {
						_findNode1(nodeId, callback, 3);
					}
				}

				void _sendPing(Node* node, const BoolCallback& callback, sl_uint32 nRetry)
				{
					Ref<Node> refNode = node;
					m_mapPingCallbacks.add(node, [refNode, callback, nRetry](sl_bool flagSuccess) {
						Node* node = refNode.get();
						if (flagSuccess) {
							callback(sl_true);
						} else {
							if (nRetry) {
								Ref<P2PSocketImpl> socket = node->m_socket;
								if (socket.isNull()) {
									callback(sl_false);
									return;
								}
								socket->_sendPing(node, callback, nRetry - 1);
							} else {
								callback(sl_false);
							}
						}
					});
					node->sendPing();
				}

				void LanNode::sendPing()
				{
					Ref<P2PSocketImpl> socket = m_socket;
					if (socket.isNull()) {
						return;
					}
					sl_uint8 buf[1024];
					buf[0] = (sl_uint8)(Command::FindNode);
					Base::copyMemory(buf + 1, m_id.data, sizeof(m_id));
					socket->_sendUdp(SocketAddress(m_ip, m_portUdp), buf, 17);
				}

				void _findNode1(const P2PNodeId& nodeId, const NodeCallback& callback, sl_uint32 nRetry)
				{
					m_mapVerifyCallbacks.add(nodeId, [nodeId, callback, nRetry](Node* node) {
						if (node) {
							callback(node);
						} else {
							if (nRetry) {
								Ref<P2PSocketImpl> socket = node->m_socket;
								if (socket.isNull()) {
									callback(sl_null);
									return;
								}
								socket->_findNode1(nodeId, callback, nRetry - 1);
							} else {
								callback(sl_null);
							}
						}
					});
					sl_uint8 buf[1024];
					buf[0] = (sl_uint8)(Command::FindNode);
					Base::copyMemory(buf + 1, nodeId.data, sizeof(nodeId));
					_sendBroadcast(buf, 17);
				}

			public:
				sl_bool start() override
				{
					if (m_threadReceiveUdp.isNotNull()) {
						return sl_true;
					}
					ObjectLocker locker(this);
					if (m_threadReceiveUdp.isNotNull()) {
						return sl_true;
					}
					m_threadReceiveUdp = Thread::start(SLIB_FUNCTION_MEMBER(P2PSocketImpl, _runReceiveUdp, this));
					if (m_threadReceiveUdp.isNull()) {
						return sl_false;
					}
					m_dispatcher->start();
					return sl_true;
				}

				void sendMessage(const P2PNodeId& nodeId, P2PMessage& msg, const ResponseCallback& callback) override
				{
					Memory content = msg.getMemory();
					_findNode(nodeId, [content, callback](Node* node) {
						if (!node) {
							ReplyErrorResponse(callback);
							return;
						}
						node->sendMessage(content, callback);
					});
				}

			};


		}
	}

	using namespace priv::p2p;

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

	P2PMessage::P2PMessage() : data(sl_null), size(0), flagNotJson(sl_false)
	{
	}

	P2PMessage::P2PMessage(const void* _data, sl_uint32 _size, Referable* _ref) : data(_data), size(_size), ref(_ref), flagNotJson(sl_false)
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

	void P2PMessage::setContent(const void* _data, sl_uint32 _size, Referable* _ref)
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
		json = Json::parseJson(getString());
		if (json.isNotNull()) {
			return json;
		}
		flagNotJson = sl_true;
		return sl_null;
	}

	void P2PMessage::setJson(const Json& _json)
	{
		clear();
		if (_json.isNotNull()) {
			String _str = _json.toJsonString();
			if (_str.isNotNull()) {
				setString(_str);
				json = _json;
			}
		}
	}

	void P2PMessage::setJson(const Json& _json, const String& _str)
	{
		clear();
		if (_json.isNotNull()) {
			if (_str.isNotNull()) {
				setString(_str);
				json = _json;
			}
		}
	}

	void P2PMessage::makeSafe()
	{
		if (data && ref.isNull()) {
			setMemory(Memory::create(data, size));
		}
	}


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(P2PResponse)
	
	P2PResponse::P2PResponse()
	{
	}


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(P2PSocketParam)

	P2PSocketParam::P2PSocketParam()
	{
		port = SLIB_P2P_DEFAULT_PORT;
		portCount = 1000;
		boundUdpPort = 0;
		boundTcpPort = 0;
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
