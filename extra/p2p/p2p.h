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

#ifndef CHECKHEADER_SLIB_EXTRA_P2P
#define CHECKHEADER_SLIB_EXTRA_P2P

#include <slib/network/socket_address.h>
#include <slib/data/json.h>
#include <slib/io/async.h>

#define SLIB_P2P_DEFAULT_PORT 39000
#define SLIB_P2P_NODE_ID_SIZE 16

namespace slib
{

	enum class P2PConnectionType
	{
		Unknown = 0,
		Direct = 1
	};

	class SLIB_EXPORT P2PNodeId : public Bytes<SLIB_P2P_NODE_ID_SIZE>
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

	public:
		P2PConnectionType connectionType;

		// For receiver (Broadcast, Datagram)
		sl_uint32 interfaceIndex;
		SocketAddress remoteAddress;

	private:
		Ref<CRef> ref;
		Memory mem;
		String str;
		Json json;
		sl_bool flagNotJson;

	public:
		P2PMessage();

		P2PMessage(const void* data, sl_size size, CRef* ref = sl_null);

		template <class T>
		P2PMessage(T&& value): data(sl_null), size(0), connectionType(P2PConnectionType::Unknown)
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

		void setContent(const void* data, sl_uint32 size, CRef* ref = sl_null);

		void setContent(const Variant& var);

		void setContent(P2PMessage& content);

		Memory getMemory();

		void setMemory(const Memory& mem);

		String getString();

		void setString(const String& str);

		Json getJson();

		void setJson(const Json& json);

		void setJson(const Json& json, const Memory& mem);

	};

	class SLIB_EXPORT P2PRequest : public P2PMessage
	{
	public:
		P2PNodeId senderId; // For receiver

	public:
		P2PRequest();

		P2PRequest(const void* data, sl_uint32 size, CRef* ref = sl_null);

		template <class T>
		P2PRequest(T&& value): P2PMessage(Forward<T>(value)) {}

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(P2PRequest)

	};

	class SLIB_EXPORT P2PResponse : public P2PMessage
	{
	public:
		P2PResponse();

		P2PResponse(const void* data, sl_uint32 size, CRef* ref = sl_null);

		template <class T>
		P2PResponse(T&& value): P2PMessage(Forward<T>(value)) {}

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(P2PResponse)

	};

	class P2PSocket;

	class SLIB_EXPORT P2PSocketParam
	{
	public:
		Memory key; // [In, Out] If not initialized, socket will generate new key
		sl_bool flagGeneratedKey; // [Out]

		IPAddress bindAddress;
		sl_uint16 port; // [In] Lobby port. We recommend you don't change `port` or `portCount`
		sl_uint16 portCount; // [In] Socket will search unbound port from [port + 1, port+portCount]
		sl_uint16 boundPort; // [Out] Bound port (UDP/TCP)
		List< Pair<sl_uint32, IPv4Address> > broadcasters; // Pairs of interface index and address

		MemoryView helloPrefix; // [In] Hello Prefix
		P2PMessage helloMessage; // [In] Hello Message
		P2PMessage connectMessage; // [In] Connect Message

		sl_uint32 helloInterval; // [In, Out] In milliseconds
		sl_uint32 connectionTimeout; // [In, Out] In milliseconds
		sl_uint32 findTimeout; // [In, Out] In milliseconds
		sl_uint32 maximumMessageSize; // [In, Out] In bytes
		sl_uint32 messageBufferSize; // [In, Out] In bytes
		sl_uint32 ephemeralKeyDuration; // [In, Out] In milliseconds

		Function<void(P2PSocket*, P2PRequest&)> onReceiveHello;
		Function<void(P2PSocket*, P2PRequest&)> onConnectNode;
		Function<void(P2PSocket*, P2PRequest&, P2PResponse&)> onReceiveMessage;
		Function<void(P2PSocket*, P2PRequest&)> onReceiveBroadcast;
		Function<void(P2PSocket*, P2PRequest&)> onReceiveDatagram;
		Function<void(P2PSocket*, P2PRequest&)> onReceiveEncryptedDatagram;

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
		virtual sl_bool isOpened() = 0;

		virtual void close() = 0;

		virtual sl_bool start() = 0;

		virtual P2PNodeId getLocalNodeId() = 0;

		virtual sl_uint16 getLocalPort() = 0;

		virtual void setHelloMessage(const P2PMessage& msg) = 0;

		virtual void setConnectMessage(const P2PMessage& msg) = 0;

		virtual void connectNode(const P2PNodeId& nodeId, const SocketAddress* address) = 0;

		void connectNode(const P2PNodeId& nodeId, const SocketAddress& address);

		void connectNode(const P2PNodeId& nodeId);

		// `outKey`: 32 Bytes
		virtual sl_bool getEncryptionKeyForNode(const P2PNodeId& nodeId, void* outKey) = 0;

		virtual void sendMessage(const P2PNodeId& nodeId, const SocketAddress* address, const P2PRequest& msg, const Function<void(P2PResponse&)>& callback, sl_uint32 timeoutMillis = 0) = 0;

		void sendMessage(const P2PNodeId& nodeId, const SocketAddress& address, const P2PRequest& msg, const Function<void(P2PResponse&)>& callback, sl_uint32 timeoutMillis = 0);

		void sendMessage(const P2PNodeId& nodeId, const P2PRequest& msg, const Function<void(P2PResponse&)>& callback, sl_uint32 timeoutMillis = 0);

		void sendMessage(const P2PNodeId& nodeId, const SocketAddress* address, const P2PRequest& msg, P2PResponse& response, sl_uint32 timeoutMillis = 0);

		void sendMessage(const P2PNodeId& nodeId, const SocketAddress& address, const P2PRequest& msg, P2PResponse& response, sl_uint32 timeoutMillis = 0);

		void sendMessage(const P2PNodeId& nodeId, const P2PRequest& msg, P2PResponse& response, sl_uint32 timeoutMillis = 0);

		virtual void sendBroadcast(sl_uint32 interfaceIndex, const P2PRequest& msg) = 0;

		void sendBroadcast(const P2PRequest& msg);

		virtual void sendDatagram(const P2PNodeId& nodeId, const SocketAddress& address, const P2PRequest& msg) = 0;

		// Call after connected
		virtual void sendEncryptedDatagram(const P2PNodeId& nodeId, const SocketAddress& address, const P2PRequest& msg) = 0;

	};

}

#endif
