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

#ifndef CHECKHEADER_SLIB_NETWORK_P2P
#define CHECKHEADER_SLIB_NETWORK_P2P

#include "socket_address.h"

#include "../core/object.h"
#include "../crypto/ecc.h"

#define SLIB_P2P_DEFAULT_PORT 39000

namespace slib
{

	typedef ECPrivateKey_secp256k1 P2P_PrivateKey;
	
	class SLIB_EXPORT P2PNodeId : public Bytes<32>
	{
	public:
		P2PNodeId() noexcept;

		P2PNodeId(sl_null_t) noexcept;

		P2PNodeId(const StringParam& _id) noexcept;

		P2PNodeId(const sl_uint8* other) noexcept;

	public:
		sl_size getHashCode() const noexcept;

	};

	class SLIB_EXPORT P2PSocketParam
	{
	public:
		P2P_PrivateKey key;

		sl_uint16 port; // UDP host port. We recommend you don't change `port` and `portCount`
		sl_uint16 portCount; // Socket will search unbind guest port from [port, port+portCount]

		sl_bool flagAutoStart;

		String errorText;

	public:
		P2PSocketParam();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(P2PSocketParam)

	};

	class P2PSocket : public Object
	{
		SLIB_DECLARE_OBJECT

	protected:
		P2PSocket();

		~P2PSocket();

	public:
		static Ref<P2PSocket> open(P2PSocketParam& param);

	public:
		virtual sl_bool start() = 0;

		virtual sl_bool getLocalNodeAddress(const P2PNodeId& nodeId, SocketAddress* pOut = sl_null) = 0;

		sl_bool isLocalNode(const P2PNodeId& nodeId);

	};

}

#endif
