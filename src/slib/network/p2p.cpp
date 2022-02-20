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
#include "slib/core/system.h"
#include "slib/core/scoped_buffer.h"
#include "slib/crypto/chacha.h"
#include "slib/crypto/serialize/ecc.h"

/*
			P2P Socket Protocol

- Hello
Offset	Length(B)	Type			Value
-------------------------------------------------------
0		1			Command			0 (Hello)
1		16			NodeId			LocalNode
17		1			Boolean			Need Reply

- ReplyHello
Offset	Length(B)	Type			Value
-------------------------------------------------------
0		1			Command			1 (ReplyHello)
1		16			NodeId			LocalNode

- FindNode
Offset	Length(B)	Type			Value
-------------------------------------------------------
0		1			Command			2 (FindNode) 
1		16			NodeId			RemoteNode

- ReplyFindNode
Offset	Length(B)	Type			Value
-------------------------------------------------------
0		1			Command			3 (ReplyFindNode)
1		Serialize	PublicKey		LocalNode

- VerifyNode
Offset	Length(B)	Type			Value
-------------------------------------------------------
0		1			Command			4 (VerifyNode)
1		16			NodeId			RemoteNode
17		4			Uint32			LocalNode's Current Tick Count (Little Endian)
21		4			IPv4Address		RemoteNode
25		2			Port			RemoteNode's Port (Little Endian)
27		Serialize	PublicKey		LocalNode

- ReplyVerifyNode
Offset	Length(B)	Type			Value
-------------------------------------------------------
0		1			Command			5 (ReplyVerifyNode)
1		16			NodeId			RemoteNode
17		12			Encryption IV	Random
29		10			Encryption Content: By ChaCha20-Poly1305: key=ECDH(PrivKey(LocalNode), PubKey(RemoteNode))
39		16			Encryption Tag
55		Serialize	PublicKey		LocalNode
-------------	Encrypted Content ---------------------
0		4			Uint32			RemoteNode's Current Tick Count (Little Endian))
4		4			IPv4Address		LocalNode
8		2			Port			LocalNode's Port (Little Endian)

- Ping
Offset	Length(B)	Type			Value
-------------------------------------------------------
0		1			Command			6 (Ping)
1		4			ShortNodeId		RemoteNode
5		4			Uint32			LocalNode's Current Milliseconds (Little Endian))

- ReplyPing
Offset	Length(B)	Type			Value
-------------------------------------------------------
0		1			Command			7 (ReplyPing)
1		16			NodeId			LocalNode
17		4			Uint32			RemoteNode's Current Milliseconds (Little Endian))

- Broadcast
Offset	Length(B)	Type			Value
-------------------------------------------------------
0		1			Command			8 (Broadcast)
1		16			NodeId			LocalNode
17		*			Content

- InitTcp
Length(B)	Type			Value
-----------------------------------------------
1			Command			100 (InitTcp)
CVLI		Uint32			Total Length (From next field)
4			ShortNodeId		RemoteNode
Serialize	PublicKey		LocalNode

- ReplyInitTcp
Length(B)	Type			Value
-----------------------------------------------
1			Command			101 (ReplyInitTcp)

- Message
Length(B)	Type			Value
-----------------------------------------------
1			Command			102 (Message)
CVLI		Uint32			Total Length (From next field)
12			Encryption IV	Random
16			Encryption Tag
*			Encryption Content: By ChaCha20-Poly1305: key=ECDH(PrivKey(LocalNode), PubKey(RemoteNode))

- ReplyMessage
Length(B)	Type			Value
-----------------------------------------------
1			Command			103 (ReplyMessage)
CVLI		Uint32			Total Length (From next field)
12			Encryption IV	Random
16			Encryption Tag
*			Encryption Content: By ChaCha20-Poly1305: key=ECDH(PrivKey(LocalNode), PubKey(RemoteNode))

*/

#define TIMEOUT_PING_DIRECT_CONNECTION 10000
#define TIMEOUT_VALID_DIRECT_CONNECTION 30000
#define INTERVAL_HELLO_DIRECT_CONNECTION 10000
#define EXPIRE_DURATION_FIND_DIRECT_CONNECTION 3000
#define EXPIRE_DURATION_IDLE_TCP_SOCKET 30000
#define BUFFER_SIZE_TCP_STREAM 0x10000

namespace slib
{

	namespace priv
	{
		namespace p2p
		{

			enum class Command
			{
				Unknown = -1,

				Hello = 0,
				ReplyHello = 1,
				FindNode = 2,
				ReplyFindNode = 3,
				VerifyNode = 4,
				ReplyVerifyNode = 5,
				Ping = 6,
				ReplyPing = 7,
				Broadcast = 8,

				InitTcp = 100,
				ReplyInitTcp = 101,
				Message = 102,
				ReplyMessage = 103
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

			enum class ConnectionType
			{
				Direct = 0
			};

			class Connection : public Referable
			{
			public:
				ConnectionType m_type;
				sl_uint32 m_timeLastPing = 0;
				sl_uint32 m_delayLastPing = 0;

			public:
				Connection(ConnectionType type) : m_type(type) {}

			};

			class DirectConnection;

			class Node : public Referable
			{
			public:
				P2PPublicKey m_key;
				P2PNodeId m_id;
				sl_uint8 m_encryptionKey[32];

				sl_uint16 m_portBound = 0;

				AtomicRef<Connection> m_connectionDefault;
				CHashMap< IPv4Address, Ref<DirectConnection> > m_connectionsDirect;

			public:
				Node(const P2PPublicKey& key): m_key(key)
				{
					GetNodeId(key, m_id.data);
				}
				
			};
			
			class DirectConnection : public Connection
			{
			public:
				IPv4Address m_ip;
				
			public:
				DirectConnection(const IPv4Address& ip): Connection(ConnectionType::Direct), m_ip(ip) {}

			};
			
			class TcpCommandContentReceiver
			{
			public:
				sl_uint8* content = sl_null;
				sl_size sizeContent = 0;

			public:
				TcpCommandContentReceiver(sl_size maxSize): m_maxContentSize(maxSize) {}

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
						sl_uint32 m = CVLI::deserialize(m_bufHeader, m_sizeHeader, sizeContent);
						if (m) {
							if (sizeContent > m_maxContentSize) {
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
							if (size > sizeContent) {
								return -1;
							}
							if (size == sizeContent) {
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
						if (sizeNew > sizeContent) {
							return -1;
						}
						if (sizeNew == sizeContent) {
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
				sl_bool m_flagParsingHeader = sl_true;
				sl_uint8 m_bufHeader[10];
				sl_uint32 m_sizeHeader = 0;
				MemoryBuffer m_bufContent;
				Memory m_memContent;
			};

			class TcpStream : public Referable
			{
			public:
				Ref<AsyncTcpSocket> m_socket;
				Command m_currentCommand = Command::Unknown;

			public:
				TcpStream(Ref<AsyncTcpSocket>&& _socket, sl_size maximumMessageSize) : m_socket(Move(_socket)), m_maximumMessageSize(maximumMessageSize) {}

			public:
				sl_int32 processReceivedData(sl_uint8* data, sl_size size)
				{
					if (!size) {
						return sl_false;
					}
					if (m_receiver.isNull()) {
						m_currentCommand = (Command)(data[0]);
						data++;
						size--;
						if (!size) {
							return sl_true;
						}
						m_receiver = Shared<TcpCommandContentReceiver>::create(m_maximumMessageSize + 1024);
						if (m_receiver.isNull()) {
							return sl_false;
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
						return m_receiver->sizeContent;
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
				sl_bool m_flagWriting = sl_false;
				P2PPublicKey m_remoteNodeKey;
				sl_uint8 m_encryptionKey[32];

			public:
				TcpServerStream(Ref<AsyncTcpSocket>&& _socket, sl_size maximumMessageSize): TcpStream(Move(_socket), maximumMessageSize) {}

			};

			class TcpClientStream : public TcpStream
			{
			public:
				TcpClientStream(Ref<AsyncTcpSocket>&& _socket, sl_size maximumMessageSize): TcpStream(Move(_socket), maximumMessageSize) {}

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

				P2PPrivateKey m_key;
				P2PNodeId m_localNodeId;
				sl_bool m_flagClosed = sl_false;

				sl_uint16 m_portLobby = 0;
				sl_uint16 m_portActor = 0;
				sl_uint16 m_portActorMax = 0;

				Ref<AsyncUdpSocket> m_socketUdpLobby;
				Ref<AsyncUdpSocket> m_socketUdpActor;
				Ref<AsyncTcpServer> m_serverTcp;

				ExpiringMap< TcpStream*, Ref<TcpStream> > m_mapTcpStreams;
				ExpiringMap< DirectConnection*, Ref<AsyncTcpSocket> > m_mapIdleTcpSockets;

				Ref<AsyncIoLoop> m_ioLoop;
				Ref<DispatchLoop> m_dispatchLoop;
				Ref<Timer> m_timerHello;

				CHashMap< P2PNodeId, Ref<Node> > m_mapNodes;
				CHashMap< P2PNodeId, Bytes<32> > m_mapEncryptionKey;
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
					m_key = param.key;
					GetNodeId(param.key, m_localNodeId.data);

					m_listLocalIPAddresses = Network::findAllIPv4Addresses();

					m_mapFindCallbacks.setupTimer(EXPIRE_DURATION_FIND_DIRECT_CONNECTION, m_dispatchLoop);

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
						m_mapTcpStreams.setupTimer(param.tcpConnectionTimeout, m_dispatchLoop);
						m_mapIdleTcpSockets.setupTimer(EXPIRE_DURATION_IDLE_TCP_SOCKET, m_dispatchLoop);

						AsyncTcpServerParam serverParam;
						serverParam.ioLoop = m_ioLoop;
						serverParam.onAccept = [this](AsyncTcpServer*, Socket& socket, const SocketAddress& address) {
							AsyncTcpSocketParam param;
							param.socket = Move(socket);
							param.ioLoop = m_ioLoop;
							Ref<AsyncTcpSocket> client = AsyncTcpSocket::create(param);
							if (client.isNotNull()) {
								Ref<TcpServerStream> stream = new TcpServerStream(Move(client), m_param.maximumMessageSize);
								if (stream.isNotNull()) {
									WeakRef<TcpServerStream> weakStream = stream;
									if (stream->m_socket->receive(Memory::create(BUFFER_SIZE_TCP_STREAM), [this, weakStream](AsyncStreamResult& result) {
										Ref<TcpServerStream> stream = weakStream;
										if (stream.isNull()) {
											return;
										}
										if (result.isSuccess() && !(stream->m_flagWriting)) {
											sl_int32 iRet = stream->processReceivedData((sl_uint8*)(result.data), result.size);
											if (iRet >= 0) {
												sl_bool flagSuccess = sl_true;
												if (iRet > 0) {
													if (_onReceivedTcpServerStream(stream.get(), stream->m_currentCommand, stream->getContent(), stream->getContentSize())) {
														stream->clear();
														m_mapTcpStreams.get(stream.get());
													} else {
														flagSuccess = sl_false;
													}
												}
												if (flagSuccess) {
													if (stream->m_socket->receive(result.data, result.requestSize, result.callback, result.userObject)) {
														return;
													}
												}
											}
										}
										m_mapTcpStreams.remove(stream.get());
									})) {
										m_mapTcpStreams.put(stream.get(), stream);
									}
								}
							}
						};
						serverParam.socket = Move(socketTcp);
						m_serverTcp = AsyncTcpServer::create(serverParam);
						if (m_serverTcp.isNull()) {
							return sl_false;
						}
					}

					// Initialize Hello Timer
					{
						m_timerHello = Timer::createWithDispatcher(m_dispatchLoop, SLIB_FUNCTION_WEAKREF(This, _onTimerHello, this), INTERVAL_HELLO_DIRECT_CONNECTION);
						if (m_timerHello.isNull()) {
							return sl_false;
						}
						m_dispatchLoop->dispatch([this]() {
							_sendHello(sl_true);
						});
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
				void _processReceivedUdp(const SocketAddress& address, sl_uint8* packet, sl_uint32 sizePacket)
				{
					if (!(_isValidSenderAddress(address))) {
						return;
					}
					Command cmd = (Command)(packet[0]);
					switch (cmd) {
					case Command::Hello:
						if (sizePacket == 18) {
							_onReceivedHelloDirectConnection(P2PNodeId(packet + 1), address, packet[17] != 0);
						}
						break;
					case Command::ReplyHello:
						if (sizePacket == 17) {
							_processHelloNode(P2PNodeId(packet + 1), address);
						}
						break;
					case Command::FindNode:
						if (sizePacket == 17) {
							_onReceivedFindNodeDirectConnection(address, P2PNodeId(packet + 1));
						}
						break; 
					case Command::ReplyFindNode:
						if (sizePacket > 1) {
							DeserializeBuffer bufRead(packet + 1, sizePacket - 1);
							P2PPublicKey remoteKey;
							if (remoteKey.deserialize(&bufRead)) {
								_sendVerifyDirectConnection(address, remoteKey);
							}
						}
						break;
					case Command::VerifyNode:
						if (sizePacket > 27) {
							if (Base::equalsMemory(m_localNodeId.data, packet + 1, sizeof(P2PNodeId))) {
								sl_uint8 contentToEncrypt[10];
								Base::copyMemory(contentToEncrypt, packet + 17, 10);
								DeserializeBuffer bufRead(packet + 27, sizePacket - 27);
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
									encryptor.encrypt(contentToEncrypt, packet + 29, sizeof(contentToEncrypt));
									encryptor.finish(packet + 39);
									SerializeBuffer bufWrite(packet + 55, 1024);
									if (m_key.toPublicKey().serialize(&bufWrite)) {
										_sendUdp(address, packet, bufWrite.current - packet);
									}
								}
							}
						}
						break;
					case Command::ReplyVerifyNode:
						if (sizePacket > 55) {
							if (Base::equalsMemory(m_localNodeId.data, packet + 1, sizeof(P2PNodeId))) {
								DeserializeBuffer bufRead(packet + 55, sizePacket - 55);
								P2PPublicKey remoteKey;
								if (remoteKey.deserialize(&bufRead)) {
									ChaCha20_Poly1305 decryptor;
									sl_uint8 key[32];
									_deriveEncryptionKey(remoteKey, key);
									decryptor.setKey(key);
									decryptor.start(packet + 17); // iv
									sl_uint8 content[10];
									decryptor.decrypt(packet + 29, content, sizeof(content));
									if (decryptor.finishAndCheckTag(packet + 39)) {
										IPv4Address remoteIP = address.ip.getIPv4();
										if (IPv4Address(content[4], content[5], content[6], content[7]) == remoteIP) {
											if (MIO::readUint16LE(content + 8) == address.port) {
												sl_uint32 timeOld = MIO::readUint32LE(content);
												sl_uint32 timeNew = GetCurrentTick();
												if (CheckDelay(timeOld, timeNew, TIMEOUT_PING_DIRECT_CONNECTION)) {
													_onVerifyDirectConnection(remoteKey, remoteIP, address.port, timeNew, timeNew - timeOld);
												}
											}
										}
									}
								}
							}
						}
						break;
					case Command::Ping:
						if (sizePacket == 9) {
							if (Base::equalsMemory(m_localNodeId.data, packet + 1, 4)) {
								sl_uint32 time = MIO::readUint32LE(packet + 5);
								packet[0] = (sl_uint8)(Command::ReplyPing);
								Base::copyMemory(packet + 1, m_localNodeId.data, sizeof(P2PNodeId));
								MIO::writeUint32LE(packet + 17, time);
								_sendUdp(address, packet, 21);
							}
						}
						break;
					case Command::ReplyPing:
						if (sizePacket == 21) {
							sl_uint32 timeOld = MIO::readUint32LE(packet + 17);
							sl_uint32 timeNew = (sl_uint32)(GetCurrentTick());
							if (CheckDelay(timeOld, timeNew, TIMEOUT_PING_DIRECT_CONNECTION)) {
								_onPingDirectConnection(P2PNodeId(packet + 1), address, timeNew, timeNew - timeOld);
							}
						}
						break;
					case Command::Broadcast:
						if (sizePacket > 17) {
							P2PNodeId nodeId(packet + 1);
							P2PMessage msg(packet + 17, sizePacket - 17);
							m_param.onReceiveBroadcast(this, nodeId, msg);
						}
						break;
					default:
						break;
					}
				}

				void _onTimerHello(Timer*)
				{
					_sendHello(sl_false);
				}

				void _onReceivedHelloDirectConnection(const P2PNodeId& nodeId, const SocketAddress& address, sl_bool flagNeedReply)
				{
					_processHelloNode(nodeId, address);
					if (flagNeedReply) {
						sl_uint8 packet[17];
						packet[0] = (sl_uint8)(Command::ReplyHello);
						Base::copyMemory(packet + 1, m_localNodeId.data, sizeof(P2PNodeId));
						_sendUdp(address, packet, sizeof(packet));
					}
					Ref<Node> node = m_mapNodes.getValue(nodeId);
					if (node.isNotNull()) {
						if (node->m_portBound == address.port) {
							if (node->m_connectionsDirect.find(address.ip.getIPv4())) {
								sl_uint8 packet[9];
								packet[0] = (sl_uint8)(Command::Ping);
								Base::copyMemory(packet + 1, node->m_id.data, 4);
								MIO::writeUint32(packet + 5, GetCurrentTick());
								_sendUdp(address, packet, sizeof(packet));
								return;
							}
						}
						_sendVerifyDirectConnection(address, node->m_key);
					}
				}

				void _processHelloNode(const P2PNodeId& nodeId, const SocketAddress& address)
				{
					if (address.ip.getIPv4().isLoopback()) {
						if (address.port > m_portLocalhostMax && address.port <= m_portActorMax) {
							m_portLocalhostMax = address.port;
						}
					}
				}

				void _onReceivedFindNodeDirectConnection(const SocketAddress& senderAddress, const P2PNodeId& targetId)
				{
					if (m_localNodeId == targetId) {
						sl_uint8 packet[1024];
						packet[0] = (sl_uint8)(Command::ReplyFindNode);
						SerializeBuffer bufWrite(packet + 1, 1024);
						if (m_key.toPublicKey().serialize(&bufWrite)) {
							_sendUdp(senderAddress, packet, bufWrite.current - packet);
						}
					}
				}

				void _onVerifyDirectConnection(const P2PPublicKey& key, const IPv4Address& ip, sl_uint16 port, sl_uint32 time, sl_uint32 delay)
				{
					P2PNodeId nodeId;
					GetNodeId(key, nodeId.data);
					Ref<Node> node = _createNode(key, port);
					if (node.isNotNull()) {
						Ref<DirectConnection> connection = _createDirectConnection(node.get(), ip);
						if (connection.isNotNull()) {
							connection->m_timeLastPing = time;
							connection->m_delayLastPing = delay;
							_selectDefaultConnectionIfBetter(node.get(), connection.get());
						} else {
							node.setNull();
						}
					}
					NodeCallbackContainer container;
					while (m_mapFindCallbacks.remove(nodeId, &container)) {
						container.success(node.get(), sl_null);
					}
				}

				void _onPingDirectConnection(const P2PNodeId& nodeId, const SocketAddress& address, sl_uint32 time, sl_uint32 delay)
				{
					Ref<Node> node = m_mapNodes.getValue(nodeId);
					if (node.isNotNull()) {
						if (node->m_portBound == address.port) {
							Ref<DirectConnection> connection = node->m_connectionsDirect.getValue(address.ip.getIPv4());
							if (connection.isNotNull()) {
								connection->m_timeLastPing = time;
								connection->m_delayLastPing = delay;
								_selectDefaultConnectionIfBetter(node.get(), connection.get());
							}
						}
					}
				}

				sl_bool _onReceivedTcpServerStream(TcpServerStream* stream, Command command, sl_uint8* packet, sl_size sizePacket)
				{
					switch (command) {
					case Command::InitTcp:
						if (sizePacket > 4) {
							if (Base::equalsMemory(m_localNodeId.data, packet, 4)) {
								DeserializeBuffer bufRead(packet + 4, sizePacket - 4);
								if (stream->m_remoteNodeKey.deserialize(&bufRead)) {
									_deriveEncryptionKey(stream->m_remoteNodeKey, stream->m_encryptionKey);
									sl_uint8 c = (sl_uint8)(Command::ReplyInitTcp);
									_sendTcpServerStream(stream, Memory::create(&c, 1));
									return sl_true;
								}
							}
						}
						break;
					case Command::Message:
						{
							P2PResponse response;
							P2PNodeId nodeId;
							GetNodeId(stream->m_remoteNodeKey, nodeId.data);
							if (sizePacket > 28) {
								ChaCha20_Poly1305 decryptor;
								decryptor.setKey(stream->m_encryptionKey);
								sl_uint8* header = packet;
								packet += 28;
								sizePacket -= 28;
								decryptor.start(header); // iv
								decryptor.decrypt(packet, packet, sizePacket);
								if (decryptor.finishAndCheckTag(header + 12)) {
									P2PMessage msg(packet, (sl_uint32)sizePacket);
									m_param.onReceiveMessage(this, nodeId, msg, response);
								} else {
									return sl_false;
								}
							} else {
								P2PMessage msg;
								m_param.onReceiveMessage(this, nodeId, msg, response);
							}
							if (response.size) {
								sl_uint8 bufSize[16];
								sl_uint32 nSize = CVLI::serialize(bufSize, response.size + 28);
								Memory memPacket = Memory::create(29 + nSize + response.size);
								if (memPacket.isNull()) {
									return sl_false;
								}
								ChaCha20_Poly1305 encryptor;
								encryptor.setKey(stream->m_encryptionKey);
								packet = (sl_uint8*)(memPacket.getData());
								*(packet++) = (sl_uint8)(Command::ReplyMessage);
								Base::copyMemory(packet, bufSize, nSize);
								packet += nSize;
								Math::randomMemory(packet, 12); // iv
								encryptor.start(packet);
								packet += 12;
								encryptor.encrypt(response.data, packet + 16, response.size);
								encryptor.finish(packet);
								return _sendTcpServerStream(stream, memPacket);
							} else {
								sl_uint8 buf[2];
								buf[0] = (sl_uint8)(Command::ReplyMessage);
								buf[1] = 0;
								return _sendTcpServerStream(stream, Memory::create(buf, 2));
							}
						}
						break;
					default:
						break;
					}
					return sl_false;
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

				void _sendHello(sl_bool flagNeedReply)
				{
					sl_uint8 packet[18];
					packet[0] = (sl_uint8)(Command::Hello);
					Base::copyMemory(packet + 1, m_localNodeId.data, sizeof(P2PNodeId));
					packet[17] = flagNeedReply ? 1 : 0;
					_sendBroadcast(packet, sizeof(packet));
				}

				void _sendVerifyDirectConnection(const SocketAddress& address, const P2PPublicKey& key)
				{
					sl_uint8 packet[1024];
					packet[0] = (sl_uint8)(Command::VerifyNode);
					GetNodeId(key, packet + 1);
					MIO::writeUint32LE(packet + 17, GetCurrentTick());
					IPv4Address remoteIP = address.ip.getIPv4();
					packet[21] = remoteIP.a;
					packet[22] = remoteIP.b;
					packet[23] = remoteIP.c;
					packet[24] = remoteIP.d;
					MIO::writeUint16LE(packet + 25, address.port);
					SerializeBuffer bufWrite(packet + 27, 1024);
					if (m_key.toPublicKey().serialize(&bufWrite)) {
						_sendUdp(address, packet, bufWrite.current - packet);
					}
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

				Ref<Node> _createNode(const P2PPublicKey& key, sl_uint16 port)
				{
					P2PNodeId nodeId;
					GetNodeId(key, nodeId.data);
					Ref<Node> node = m_mapNodes.getValue(nodeId);
					if (node.isNotNull()) {
						if (node->m_key == key && node->m_portBound == port) {
							return node;
						}
					}
					node = new Node(key);
					if (node.isNotNull()) {
						_deriveEncryptionKey(key, node->m_encryptionKey);
						node->m_portBound = port;
						m_mapNodes.put(nodeId, node);
						return node;
					}
					return sl_null;
				}

				Ref<DirectConnection> _createDirectConnection(Node* node, const IPv4Address& ip)
				{
					Ref<DirectConnection> connection = node->m_connectionsDirect.getValue(ip);
					if (connection.isNull()) {
						connection = new DirectConnection(ip);
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
							if (connectionDefault->m_type == ConnectionType::Direct) {
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
					return CheckDelay(connection->m_timeLastPing, GetCurrentTick(), TIMEOUT_VALID_DIRECT_CONNECTION);
				}

				void _getTcpClientStream(Node* node, DirectConnection* connection, const Function<void(This*, Node*, DirectConnection*, TcpClientStream*)>& callback)
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
						Ref<Node> refNode = node;
						Ref<DirectConnection> refConnection = connection;
						Ref<AsyncTcpSocket> socket = AsyncTcpSocket::create(param);
						if (socket.isNotNull()) {
							Ref<TcpClientStream> stream = new TcpClientStream(Move(socket), m_param.maximumMessageSize);
							if (stream.isNotNull()) {
								WeakRef<TcpClientStream> weakStream = stream;
								if (stream->m_socket->connect(SocketAddress(connection->m_ip, node->m_portBound), [this, refNode, refConnection, weakStream, callback](AsyncTcpSocket* socket, sl_bool flagError) {
									Node* node = refNode.get();
									Ref<TcpClientStream> stream = weakStream;
									if (stream.isNotNull()) {
										if (!flagError) {
											sl_uint8 bufKey[1024];
											SerializeBuffer sbKey(bufKey, sizeof(bufKey));
											if (m_key.toPublicKey().serialize(&sbKey)) {
												sl_uint32 nKey = (sl_uint32)(sbKey.current - sbKey.begin);
												sl_uint8 bufPacket[1024];
												sl_uint8* packet = bufPacket;
												*(packet++) = (sl_uint8)(Command::InitTcp);
												sl_uint32 n = CVLI::serialize(packet, 4 + nKey);
												packet += n;
												Base::copyMemory(packet, node->m_id.data, 4);
												packet += 4;
												Base::copyMemory(packet, bufKey, nKey);
												packet += nKey;
												if (socket->send(Memory::create(bufPacket, packet - bufPacket), [this, refNode, refConnection, weakStream, callback](AsyncStreamResult& result) {
													Ref<TcpClientStream> stream = weakStream;
													if (stream.isNotNull()) {
														if (result.isSuccess()) {
															if (result.stream->read(Memory::create(16), [this, refNode, refConnection, weakStream, callback](AsyncStreamResult& result) {
																Ref<TcpClientStream> stream = weakStream;
																if (stream.isNotNull()) {
																	if (result.isSuccess()) {
																		if (result.size == 1 && *((sl_uint8*)(result.data)) == (sl_uint8)(Command::ReplyInitTcp)) {
																			callback(this, refNode.get(), refConnection.get(), stream.get());
																			return;
																		}
																	}
																	m_mapTcpStreams.remove(stream.get());
																}
																callback(this, refNode.get(), refConnection.get(), sl_null);
															})) {
																return;
															}
														}
														m_mapTcpStreams.remove(stream.get());
													}
													callback(this, refNode.get(), refConnection.get(), sl_null);
												})) {
													return;
												}
											}
										}
										m_mapTcpStreams.remove(stream.get());
									}
									callback(this, refNode.get(), refConnection.get(), sl_null);
								})) {
									m_mapTcpStreams.put(stream.get(), stream);
									return;
								}
							}
						}
						callback(this, node, connection, sl_null);
					}
				}
				
				void _findNode(const P2PNodeId& nodeId, const NodeCallback& callback, sl_uint32 nRetry)
				{
					Ref<Node> node = m_mapNodes.getValue(nodeId);
					if (node.isNotNull()) {
						callback(node.get(), sl_null);
						return;
					}
					WeakRef<This> weakThis = this;
					m_mapFindCallbacks.add(nodeId, [weakThis, nodeId, callback, nRetry](Node* node, void*) {
						Ref<This> thiz = weakThis;
						if (thiz.isNull()) {
							callback(sl_null, sl_null);
							return;
						}
						if (node) {
							callback(node, sl_null);
						} else {
							if (nRetry) {
								thiz->_findNode(nodeId, callback, nRetry - 1);
							} else {
								callback(sl_null, sl_null);
							}
						}
					});
					sl_uint8 buf[1024];
					buf[0] = (sl_uint8)(Command::FindNode);
					Base::copyMemory(buf + 1, nodeId.data, sizeof(nodeId));
					_sendBroadcast(buf, 17);
				}

				void _sendMessage(Node* node, const MessageBody& body, const Function<void(P2PResponse&)>& callback)
				{
					Ref<Connection> connection = node->m_connectionDefault;
					if (connection.isNotNull()) {
						if (_isValidConnection(connection.get())) {
							if (connection->m_type == ConnectionType::Direct) {
								_sendMessageDirectConnection(node, (DirectConnection*)(connection.get()), body, callback);
								return;
							}
						}
					}
					ReplyErrorResponse(callback);
				}

				void _sendMessageDirectConnection(Node* node, DirectConnection* connection, const MessageBody& body, const Function<void(P2PResponse&)>& callback)
				{
					_getTcpClientStream(node, connection, [this, body, callback](This* thiz, Node* node, DirectConnection* connection, TcpClientStream* stream) {
						if (stream) {
							Memory memPacket;
							sl_uint8* packet;
							if (body.lengthOfSize) {
								memPacket = body.packet;
								packet = (sl_uint8*)(memPacket.getData());
								Math::randomMemory(packet + 1 + body.lengthOfSize, 28);
								ChaCha20_Poly1305 enc;
								enc.setKey(node->m_encryptionKey);
								enc.start(packet + 1 + body.lengthOfSize);
								sl_uint8* content = packet + 29 + body.lengthOfSize;
								enc.encrypt(content, content, memPacket.getSize() - 29 - body.lengthOfSize);
								enc.finish(packet + 13 + body.lengthOfSize);
							} else {
								memPacket = Memory::create(2);
								if (memPacket.isNull()) {
									ReplyErrorResponse(callback);
									m_mapTcpStreams.remove(stream);
									return;
								}
								packet = (sl_uint8*)(memPacket.getData());
								packet[1] = 0;
							}
							packet[0] = (sl_uint8)(Command::Message);
							WeakRef<TcpClientStream> weakStream = stream;
							Ref<DirectConnection> refConnection = connection;
							Ref<Node> refNode = node;
							if (stream->m_socket->send(memPacket, [this, refNode, refConnection, weakStream, callback](AsyncStreamResult& result) {
								Ref<TcpClientStream> stream = weakStream;
								if (stream.isNotNull()) {
									if (result.isSuccess()) {
										if (stream->m_socket->receive(Memory::create(BUFFER_SIZE_TCP_STREAM), [this, refNode, refConnection, weakStream, callback](AsyncStreamResult& result) {
											Ref<TcpClientStream> stream = weakStream;
											DirectConnection* connection = refConnection.get();
											if (stream.isNotNull()) {
												if (result.isSuccess()) {
													sl_int32 iRet = stream->processReceivedData((sl_uint8*)(result.data), result.size);
													if (iRet >= 0) {
														if (iRet > 0) {
															if (stream->m_currentCommand == Command::ReplyMessage) {
																sl_uint8* buf = stream->getContent();
																sl_uint32 size = (sl_uint32)(stream->getContentSize());
																sl_bool flagSuccess = sl_true;
																if (size) {
																	ChaCha20_Poly1305 enc;
																	enc.setKey(refNode->m_encryptionKey);
																	enc.start(buf);
																	sl_uint8* content = buf + 28;
																	size -= 28;
																	enc.decrypt(content, content, size);
																	if (enc.finishAndCheckTag(buf + 12)) {
																		P2PResponse response(content, size);
																		callback(response);
																	} else {
																		flagSuccess = sl_false;
																	}
																} else {
																	P2PResponse response;
																	callback(response);
																}
																stream->clear();
																if (flagSuccess) {
																	m_mapTcpStreams.get(stream.get());
																	m_mapIdleTcpSockets.add(connection, stream->m_socket);
																	return;
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
											ReplyErrorResponse(callback);
										})) {
											return;
										}
									}
									m_mapTcpStreams.remove(stream.get());
								}
								ReplyErrorResponse(callback);
							})) {
								return;
							}
							m_mapTcpStreams.remove(stream);
						}
						ReplyErrorResponse(callback);
					});
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

					if (m_serverTcp.isNotNull()) {
						m_serverTcp->close();
						m_serverTcp.setNull();
					}
					if (m_timerHello.isNotNull()) {
						m_timerHello->stopAndWait();
						m_timerHello.setNull();
					}
					if (m_dispatchLoop.isNotNull()) {
						m_dispatchLoop->release();
						m_dispatchLoop.setNull();
					}

					m_mapTcpStreams.removeAll();
					m_mapIdleTcpSockets.removeAll();
					m_mapNodes.removeAll();

					if (m_socketUdpActor.isNotNull()) {
						m_socketUdpActor->close();
					}
					if (m_socketUdpLobby.isNotNull()) {
						m_socketUdpLobby->close();
					}
					if (m_serverTcp.isNotNull()) {
						m_serverTcp->close();
					}
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

				void sendMessage(const P2PNodeId& nodeId, const P2PMessage& msg, const Function<void(P2PResponse&)>& callback) override
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
					WeakRef<This> weakThis = this;
					_findNode(nodeId, [weakThis, body, callback](Node* node, void*) {
						Ref<This> thiz = weakThis;
						if (thiz.isNotNull() && node) {
							thiz->_sendMessage(node, body, callback);
						} else {
							ReplyErrorResponse(callback);
						}
					}, 3);
				}

				void sendBroadcast(const P2PMessage& msg) override
				{
					if (m_flagClosed) {
						return;
					}
					SLIB_SCOPED_BUFFER(sl_uint8, 1024, packet, 17 + msg.size);
					packet[0] = (sl_uint8)(Command::Broadcast);
					Base::copyMemory(packet + 1, m_localNodeId.data, sizeof(P2PNodeId));
					Base::copyMemory(packet + 17, msg.data, msg.size);
					_sendBroadcast(packet, 17 + msg.size);
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


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(P2PResponse)
	
	P2PResponse::P2PResponse()
	{
	}

	P2PResponse::P2PResponse(const void* data, sl_uint32 size, Referable* ref): P2PMessage(data, size, ref)
	{
	}


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(P2PSocketParam)

	P2PSocketParam::P2PSocketParam()
	{
		port = SLIB_P2P_DEFAULT_PORT;
		portCount = 1000;
		boundPort = 0;

		tcpConnectionTimeout = 300000; // 5 minutes
		maximumMessageSize = 100 * 1024 * 1024;

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
