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

			public:
				static Ref<P2PSocketImpl> open(P2PSocketParam& param)
				{
					sl_bool flagUdpHost = sl_false;
					Socket socket = Socket::openUdp(param.port);
					if (socket.isNotNone()) {
						flagUdpHost = sl_true;
					} else {
						for (sl_uint16 i = 1; i < 1000; i++) {
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

					return sl_null;
				}

			private:
				void runUdpHost()
				{
					Thread* thread = Thread::getCurrent();
					if (!thread) {
						return;
					}
					Socket& socket = m_socketUdp;
					while (thread->isNotStopping()) {
						socket.receiveFrom();
					}
				}

			};

		}
	}

	using namespace priv::p2p;

	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(P2PSocketParam)

	P2PSocketParam::P2PSocketParam(): port(SLIB_P2P_DEFAULT_PORT)
	{
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
