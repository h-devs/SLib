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
#include "slib/core/thread.h"

namespace slib
{

	namespace priv
	{
		namespace p2p
		{

			class P2PSocketImpl : public P2PSocket
			{
			private:
				Socket m_socketUdp;
				sl_bool m_flagUdpHost;

				Ref<Thread> m_threadUdp;
				

			public:
				static Ref<P2PSocketImpl> open(P2PSocketParam& param)
				{
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
							SLIB_STATIC_STRING(err, "Failed to bind the UDP socket!")
							param.errorText = err;
							return sl_null;
						}
					}
					Ref<P2PSocketImpl> ret = new P2PSocketImpl;
					if (ret.isNotNull()) {
						ret->m_socketUdp = Move(socket);
						ret->m_flagUdpHost = flagUdpHost;
						ret->_initialize();
						if (param.flagAutoStart) {
							if (ret->start()) {
								return ret;
							}
						} else {
							return ret;
						}
					}
					return sl_null;
				}

			private:
				void _initialize()
				{
					m_socketUdp.setNonBlockingMode();
				}

				void runUdpHost()
				{
					Thread* thread = Thread::getCurrent();
					if (!thread) {
						return;
					}
					Socket& socket = m_socketUdp;
					while (thread->isNotStopping()) {
						
					}
				}

			public:
				sl_bool start() override
				{
					if (m_threadUdp.isNotNull()) {
						return sl_true;
					}
					ObjectLocker locker(this);
					if (m_threadUdp.isNotNull()) {
						return sl_true;
					}
					m_threadUdp = Thread::start(SLIB_FUNCTION_MEMBER(P2PSocketImpl, runUdpHost, this));
					return m_threadUdp.isNotNull();
				}

				sl_bool getLocalNodeAddress(const P2PNodeId& nodeId, SocketAddress* pAddress) override
				{
					return sl_false;
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

	sl_bool P2PSocket::isLocalNode(const P2PNodeId& nodeId)
	{
		return getLocalNodeAddress(nodeId);
	}

}
