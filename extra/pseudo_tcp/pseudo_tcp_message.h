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

	class PseudoTcpMessage : public Object
	{
	public:
		PseudoTcpMessage();

		~PseudoTcpMessage();

	public:
		sl_uint32 getTimeout();

		void setTimeout(sl_uint32 timeout);

	public:
		void sendMessage(const void* data, sl_size size, const Function<void(sl_uint8* data, sl_int32 size)>& callbackResponse, const Function<void(sl_uint8* packet, sl_uint32 size)>& callbackSendPacket, sl_uint32 timeout = 0);

		void notifyPacketForSendingMessage(const void* data, sl_size size);

		void notifyPacketForListeningMessage(const String& host, const void* data, sl_size size, const Function<Promise<sl_bool>(sl_uint8* data, sl_uint32 size, MemoryOutput* output)>& callbackMessage, const Function<void(sl_uint8* packet, sl_uint32 size)>& callbackSendPacket);

	protected:
		class Address;
		class Connection;

		sl_uint32 generateConversationNo();

		void process();

		void dispatch(const Function<void()>& callback);

		void endSendingConnection(sl_uint32 conversationNo, Connection* connection);

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
