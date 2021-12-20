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
#include "slib/core/thread.h"
#include "slib/core/serialize.h"
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
17		4			IPv4Address		RemoteNode
21		2			Port			RemoteNode
23		8			Time			Current Milliseconds (Local)
31		Serialize	PublicKey		LocalNode

- ReplyVerifyNode
Offset	Length(B)	Type			Value
-------------------------------------------------------
0		1			Command			3 (ReplyVerifyNode)
1		16			NodeId			RemoteNode
17		12			Encryption IV	Random
29		14			Encryption Content: By ChaCha20-Poly1305: key=ECDH(PrivKey(LocalNode), PubKey(RemoteNode))
43		16			Encryption Tag
59		Serialize	PublicKey		LocalNode
-------------	Encrypted Content ---------------------
0		4			IPv4Address		LocalNode
4		2			Port			LocalNode
6		8			Time			Current Milliseconds (Remote)
*/

#define TIMEOUT_VERIFY_NODE 10000

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
				Datagram = 16,

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

			class P2PSocketImpl : public P2PSocket
			{
			private:
				P2PPrivateKey m_key;
				P2PNodeId m_localNodeId;

				Socket m_socketUdp;
				Ref<SocketEvent> m_eventUdp;
				sl_bool m_flagUdpHost;

				Ref<Thread> m_threadReceiveUdp;
				sl_uint8 m_bufReceiveUdp[65536];

				CHashMap< P2PNodeId, Bytes<32> > m_mapEncryptionKey;

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
					} else {
						for (sl_uint16 i = 1; i <= param.portCount; i++) {
							socket = Socket::openUdp(param.port + i);
							if (socket.isNotNone()) {
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
					Ref<P2PSocketImpl> ret = new P2PSocketImpl;
					if (ret.isNotNull()) {
						ret->m_socketUdp = Move(socket);
						ret->m_eventUdp = Move(ev);
						ret->m_flagUdpHost = flagUdpHost;
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

			private:
				void _initialize(P2PSocketParam& param)
				{
					m_key = param.key;
					GetNodeId(param.key, m_localNodeId.data);
					m_socketUdp.setNonBlockingMode();
					m_socketUdp.setOption_Broadcast();
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
								IPv4Address remoteIP = address.ip.getIPv4();
								packet[17] = remoteIP.a;
								packet[18] = remoteIP.b;
								packet[19] = remoteIP.c;
								packet[20] = remoteIP.d;
								MIO::writeUint16LE(packet + 21, address.port);
								sl_uint64 time = Time::now().getMillisecondsCount();
								MIO::writeUint64LE(packet + 23, time);
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
								sl_uint8 contentToEncrypt[14];
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
									encryptor.encrypt(contentToEncrypt, packet + 29, sizeof(contentToEncrypt));
									encryptor.finish(packet + 43);
									SerializeBuffer bufWrite(packet + 59, 1024);
									if (m_key.toPublicKey().serialize(&bufWrite)) {
										_sendUdp(address, packet, bufWrite.current - packet);
									}
								}
							}
						}
						break;
					case Command::ReplyVerifyNode:
						if (sizePacket > 59) {
							if (Base::equalsMemory(m_localNodeId.data, packet + 1, sizeof(P2PNodeId))) {
								DeserializeBuffer bufRead(packet + 59, sizePacket - 59);
								P2PPublicKey remoteKey;
								if (remoteKey.deserialize(&bufRead)) {
									ChaCha20_Poly1305 decryptor;
									sl_uint8 key[32];
									_deriveEncryptionKey(remoteKey, key);
									decryptor.setKey(key);
									decryptor.start(packet + 17); // iv
									sl_uint8 content[14];
									decryptor.decrypt(packet + 29, content, sizeof(content));
									if (decryptor.finishAndCheckTag(packet + 43)) {
										IPv4Address remoteIP = address.ip.getIPv4();
										if (IPv4Address(content[0], content[1], content[2], content[3]) == remoteIP) {
											if (MIO::readUint16LE(content + 4) == address.port) {
												sl_uint64 timeNew = Time::now().getMillisecondsCount();
												sl_uint64 timeOld = MIO::readUint64LE(content + 6);
												if (timeNew >= timeOld && timeNew < timeOld + TIMEOUT_VERIFY_NODE) {
													_registerLanNode(remoteKey, address, timeOld, timeNew);
												}
											}
										}
									}
								}
							}
						}
						break;
					default:
						break;
					}
				}

				void _sendUdp(const SocketAddress& address, sl_uint8* buf, sl_size size)
				{
					m_socketUdp.sendTo(address, buf, size);
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

				void _registerLanNode(const P2PPublicKey& key, const SocketAddress& address, const Time& timeSentVerify, const Time& timeReceivedVerify)
				{
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
					return m_threadReceiveUdp.isNotNull();
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


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(P2PSocketParam)

	P2PSocketParam::P2PSocketParam()
	{
		port = SLIB_P2P_DEFAULT_PORT;
		portCount = 1000;
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
