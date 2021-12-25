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

#include "../core/json.h"
#include "../core/async.h"
#include "../crypto/ecc.h"

#define SLIB_P2P_DEFAULT_PORT 39000

namespace slib
{

	typedef ECPrivateKey_secp256k1 P2PPrivateKey;
	typedef ECPublicKey_secp256k1 P2PPublicKey;

	class SLIB_EXPORT P2PNodeId : public Bytes<16>
	{
	public:
		P2PNodeId() noexcept;

		P2PNodeId(sl_null_t) noexcept;

		P2PNodeId(const StringParam& _id) noexcept;

		P2PNodeId(const sl_uint8* other) noexcept;

	public:
		sl_size getHashCode() const noexcept;

	};

	class SLIB_EXPORT P2PMessage
	{
	public:
		const void* data;
		sl_uint32 size;

	private:
		Ref<Referable> ref;
		Memory mem;
		String str;
		Json json;
		sl_bool flagNotJson;

	public:
		P2PMessage();

		P2PMessage(const void* data, sl_uint32 size, Referable* ref = sl_null);

		template <class T>
		P2PMessage(T&& value): data(sl_null), size(0)
		{
			setContent(Forward<T>(value));
		}

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(P2PMessage)

	public:
		sl_bool isEmpty()
		{
			return !size;
		}

		sl_bool isNotEmpty()
		{
			return size > 0;
		}

		void clear();

		void setContent(const void* data, sl_uint32 size, Referable* ref = sl_null);

		void setContent(const Variant& var);

		void setContent(P2PMessage& content);

		Memory getMemory();

		void setMemory(const Memory& mem);

		String getString();

		void setString(const String& str);

		Json getJson();

		void setJson(const Json& json);

		void setJson(const Json& json, const String& str);

		void makeSafe();

	};

	class SLIB_EXPORT P2PResponse : public P2PMessage
	{
	public:
		P2PResponse();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(P2PResponse)

	};

	class SLIB_EXPORT P2PSocketParam
	{
	public:
		P2PPrivateKey key; // [In, Out] If not initialized, socket will generate new key

		sl_uint16 port; // [In] Host port. We recommend you don't change `port` and `portCount`
		sl_uint16 portCount; // [In] Socket will search unbind guest port from [port, port+portCount)
		sl_uint16 boundUdpPort; // [Out] Bound UDP port
		sl_uint16 boundTcpPort; // [Out] Bound TCP port

		sl_bool flagAutoStart; // [In] Automatically start the socket

		String errorText; // [Out] Error text during creation

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

		virtual void sendMessage(const P2PNodeId& nodeId, P2PMessage& msg, const Function<void(P2PResponse& response)>& callback) = 0;

	};

}

#endif
