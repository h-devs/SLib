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

#ifndef CHECKHEADER_SLIB_EXTRA_PSEUDO_TCP_MESSAGE
#define CHECKHEADER_SLIB_EXTRA_PSEUDO_TCP_MESSAGE

#include "pseudo_tcp.h"

#include <slib/core/string.h>
#include <slib/core/hash_map.h>
#include <slib/core/queue.h>
#include <slib/core/function.h>
#include <slib/core/promise.h>
#include <slib/io/memory_output.h>

namespace slib
{

	class Thread;
	class Event;

	class PseudoTcpConnection : public CRef
	{
	public:
		PseudoTcpConnection();

		~PseudoTcpConnection();

	};

	class PseudoTcpMessageParam
	{
	public:
		sl_uint32 timeout;
		sl_bool flagAutoStart;

	public:
		PseudoTcpMessageParam();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(PseudoTcpMessageParam)

	};

	class PseudoTcpMessage : public Object
	{
	protected:
		PseudoTcpMessage();

		~PseudoTcpMessage();

	public:
		static Ref<PseudoTcpMessage> create(const PseudoTcpMessageParam& param);

		static Ref<PseudoTcpMessage> create();

	public:
		sl_bool start();

		void release();

		static Memory createMessageChunk(const void* data, sl_size size);

		Ref<PseudoTcpConnection> sendMessageChunk(const Memory& chunk, const Function<void(Memory&)>& callbackResponse, const Function<void(sl_uint8* packet, sl_uint32 size)>& callbackSendPacket, sl_uint32 timeout = 0);

		Ref<PseudoTcpConnection> sendMessage(const void* data, sl_size size, const Function<void(Memory&)>& callbackResponse, const Function<void(sl_uint8* packet, sl_uint32 size)>& callbackSendPacket, sl_uint32 timeout = 0);

		void endConnection(PseudoTcpConnection* connection);

		void notifyPacketForSendingMessage(const void* data, sl_size size);

		void startListeningMessage(const String& host, const void* data, sl_size size, const Function<Promise<Memory>(const Memory& input)>& callbackMessage, const Function<void(sl_uint8* packet, sl_uint32 size)>& callbackSendPacket);

		sl_bool continueListeningMessage(const String& host, const void* data, sl_size size);

		void notifyPacketForListeningMessage(const String& host, const void* data, sl_size size, const Function<Promise<Memory>(const Memory& input)>& callbackMessage, const Function<void(sl_uint8* packet, sl_uint32 size)>& callbackSendPacket);

	protected:
		class Address;
		class Connection;

		sl_bool initialize(const PseudoTcpMessageParam& param);

		sl_uint32 generateConversationNo();

		void process();

		void dispatch(const Function<void()>& callback);

		void endSendingConnection(Connection* connection);

		void endListeningConnection(const Address& address, Connection* connection);

	protected:
		sl_uint32 m_timeout;

		volatile sl_uint32 m_conversationNoLastSent;
		HashMap< sl_uint32, Ref<Connection> > m_mapSend;
		HashMap< Address, Ref<Connection> > m_mapListen;
		LinkedList<sl_uint32> m_queueEndSend;
		LinkedList<Address> m_queueEndListen;

		Ref<Thread> m_threadProcess;
		Ref<Event> m_eventProcess;
		struct Packet;
		LinkedList<Packet> m_queuePackets;
		LinkedList< Function<void()> > m_queueDispatch;

	};

}

#endif
