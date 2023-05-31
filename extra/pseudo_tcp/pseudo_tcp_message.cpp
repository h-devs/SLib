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

#include "pseudo_tcp_message.h"

#include <slib/core/thread.h>
#include <slib/core/event.h>
#include <slib/core/time.h>
#include <slib/core/system.h>
#include <slib/core/mio.h>
#include <slib/core/shared.h>

#define DEFAULT_MTU 1024
#define DEFAULT_TIMEOUT 30000
#define MESSAGE_SIZE_MAX 0x7fffffff

namespace slib
{

	struct PseudoTcpMessage::Packet
	{
		WeakRef<Connection> connection;
		Memory content;
	};

	class PseudoTcpMessage::Address
	{
	public:
		String host;
		sl_uint32 conversationNo;

	public:
		SLIB_INLINE sl_size getHashCode() const noexcept
		{
			return Rehash(host.getHashCode() ^ conversationNo);
		}

		SLIB_INLINE sl_compare_result compare(const PseudoTcpMessage::Address& other) const noexcept
		{
			sl_compare_result r = host.compare(other.host);
			if (r) {
				return r;
			}
			return Compare<sl_uint32>()(conversationNo, other.conversationNo);
		}

	};

	class PseudoTcpMessage::Connection : public CRef, public IPseudoTcpNotify
	{
	public:
		PseudoTcp tcp;
		Function<void(Connection* connection)> onUpdate;

		Memory dataSend;
		sl_bool flagCalledReceiveCallback;
		sl_bool flagError;
		sl_bool flagEnd;

	protected:
		sl_uint32 m_timeout;
		Function<void(sl_uint8* packet, sl_uint32 size)> m_callbackSendPacket;

		sl_uint32 m_offsetWrite;
		MemoryOutput m_dataReceive;
		sl_uint8 m_bufReceiveHeader[4];

		sl_uint32 m_timeStart;

	public:
		Connection(sl_uint32 conversationNo, const Function<void(Connection* connection)>& _onUpdate, const Function<void(sl_uint8* packet, sl_uint32 size)>& callbackSendPacket, sl_uint32 timeout): tcp(this, conversationNo), onUpdate(_onUpdate), m_callbackSendPacket(callbackSendPacket)
		{
			tcp.notifyMTU(DEFAULT_MTU);
			flagCalledReceiveCallback = sl_false;
			flagError = sl_false;
			flagEnd = sl_false;

			m_timeout = timeout;

			m_offsetWrite = 0;
			m_timeStart = PseudoTcp::now();
		}

	public:
		void onTcpOpen(PseudoTcp* tcp) override
		{
			onTcpReadable(tcp);
			onTcpWriteable(tcp);
		}

		void onTcpReadable(PseudoTcp*) override
		{
			sl_uint8 buf[65536];
			for (;;) {
				sl_int32 n = tcp.receive(buf, sizeof(buf));
				if (n < 0) {
					if (n != SLIB_IO_WOULD_BLOCK) {
						flagError = sl_true;
						onUpdate(this);
					}
					return;
				}
				sl_uint32 m = (sl_uint32)(m_dataReceive.getSize());
				if (m < 4) {
					sl_uint32 k = 4 - m;
					if (k > (sl_uint32)n) {
						k = n;
					}
					Base::copyMemory(m_bufReceiveHeader + m, buf, k);
				}
				if (!(m_dataReceive.write(buf, n))) {
					flagError = sl_true;
					onUpdate(this);
					return;
				}
				if (isReadComplete()) {
					onUpdate(this);
					return;
				}
			}
		}

		void onTcpWriteable(PseudoTcp*) override
		{
			if (dataSend.isNull()) {
				return;
			}
			for (;;) {
				sl_uint32 total = (sl_uint32)(dataSend.getSize());
				if (m_offsetWrite >= total) {
					return;
				}
				sl_int32 n = tcp.send((sl_uint8*)(dataSend.getData()) + m_offsetWrite, total - m_offsetWrite);
				if (n < 0) {
					if (n != SLIB_IO_WOULD_BLOCK) {
						flagError = sl_true;
						onUpdate(this);
					}
					return;
				}
				m_offsetWrite += n;
				if (isWriteComplete()) {
					onUpdate(this);
					return;
				}
			}
		}

		void onTcpClosed(PseudoTcp*, PseudoTcpError) override
		{
			flagError = sl_true;
			onUpdate(this);
		}

		PseudoTcpWriteResult writeTcpPacket(PseudoTcp*, const void* buf, sl_size len) override
		{
			m_callbackSendPacket((sl_uint8*)buf, (sl_uint32)len);
			return PseudoTcpWriteResult::Success;
		}

		sl_bool setSendingData(const void* data, sl_size size)
		{
			if (size <= MESSAGE_SIZE_MAX) {
				Memory mem = Memory::create(4 + size);
				if (mem.isNotNull()) {
					sl_uint8* p = (sl_uint8*)(mem.getData());
					MIO::writeUint32LE(p, (sl_uint32)size);
					if (size) {
						Base::copyMemory(p + 4, data, size);
					}
					dataSend = Move(mem);
					m_offsetWrite = 0;
					return sl_true;
				}
			}
			return sl_false;
		}

		Memory getReceivedData()
		{
			Memory mem = m_dataReceive.merge();
			sl_size n = mem.getSize();
			if (n > 4) {
				return mem.sub(4, MIO::readUint32LE(m_bufReceiveHeader));
			}
			return sl_null;
		}

		sl_bool isReadComplete()
		{
			sl_size m = m_dataReceive.getSize();
			return m >= 4 && m >= 4 + MIO::readUint32LE(m_bufReceiveHeader);
		}

		sl_bool isReadCompleteOver()
		{
			sl_size m = m_dataReceive.getSize();
			return m > 4 && m > 4 + MIO::readUint32LE(m_bufReceiveHeader);
		}

		sl_bool isWriteComplete()
		{
			return m_offsetWrite >= dataSend.getSize();
		}

		sl_bool isTimeout(sl_uint32 now)
		{
			return now - m_timeStart > m_timeout;
		}

	};


	PseudoTcpMessage::PseudoTcpMessage()
	{
		m_timeout = DEFAULT_TIMEOUT;
		m_conversationNoLastSent = (sl_uint32)(Time::now().getMillisecondCount());
		m_eventProcess = Event::create();
		m_threadProcess = Thread::start(SLIB_FUNCTION_MEMBER(this, process));
	}

	PseudoTcpMessage::~PseudoTcpMessage()
	{
		m_threadProcess->finishAndWait();
	}

	sl_uint32 PseudoTcpMessage::getTimeout()
	{
		return m_timeout;
	}

	void PseudoTcpMessage::setTimeout(sl_uint32 timeout)
	{
		m_timeout = timeout;
	}

	void PseudoTcpMessage::sendMessage(const void* data, sl_size size, const Function<void(sl_uint8* data, sl_int32 size)>& callbackResponse, const Function<void(sl_uint8* packet, sl_uint32 size)>& callbackSendPacket, sl_uint32 timeout)
	{
		if (!(data && size && size <= MESSAGE_SIZE_MAX)) {
			callbackResponse(sl_null, -1);
			return;
		}
		if (!timeout) {
			timeout = m_timeout;
		}
		sl_uint32 conversationNo = generateConversationNo();
		WeakRef<PseudoTcpMessage> thiz = this;
		auto callbackUpdate = [thiz, this, conversationNo, callbackResponse](Connection* connection) {
			if (connection->flagEnd) {
				return;
			}
			Ref<PseudoTcpMessage> ref = thiz;
			if (ref.isNull()) {
				connection->flagEnd = sl_true;
				callbackResponse(sl_null, -1);
				return;
			}
			if (connection->flagError) {
				endSendingConnection(conversationNo, connection);
				callbackResponse(sl_null, -1);
				return;
			}
			if (connection->isWriteComplete()) {
				if (connection->isReadComplete()) {
					if (!(connection->flagCalledReceiveCallback)) {
						connection->flagCalledReceiveCallback = sl_true;
						Ref<Connection> refConnection = connection;
						dispatch([connection, refConnection]() {
							connection->tcp.send("", 1);
						});
						endSendingConnection(conversationNo, connection);
						Memory mem = connection->getReceivedData();
						callbackResponse((sl_uint8*)(mem.getData()), (sl_uint32)(mem.getSize()));
					}
				}
			}
		};
		Ref<Connection> connection = new Connection(conversationNo, callbackUpdate, callbackSendPacket, timeout);
		if (connection.isNotNull()) {
			if (!(connection->setSendingData(data, size))) {
				callbackResponse(sl_null, -1);
				return;
			}
			m_mapSend.put(conversationNo, connection);
			dispatch([connection]() {
				connection->tcp.connect();
			});
			m_eventProcess->set();
		}
	}

	void PseudoTcpMessage::notifyPacketForSendingMessage(const void* data, sl_size size)
	{
		if (size < 4) {
			return;
		}
		sl_uint32 conversationNo = MIO::readUint32BE(data);
		Ref<Connection> connection = m_mapSend.getValue(conversationNo);
		if (connection.isNotNull()) {
			Packet packet;
			packet.connection = connection;
			packet.content = Memory::create(data, size);
			m_queuePackets.pushBack(packet);
			m_eventProcess->set();
		}
	}

	void PseudoTcpMessage::notifyPacketForListeningMessage(const String& host, const void* data, sl_size size, const Function<Promise<sl_bool>(sl_uint8* data, sl_uint32 size, MemoryOutput* output)>& callbackMessage, const Function<void(sl_uint8* packet, sl_uint32 size)>& callbackSendPacket)
	{
		if (size < 4) {
			return;
		}
		sl_uint32 conversationNo = MIO::readUint32BE(data);
		Address address;
		address.host = host;
		address.conversationNo = conversationNo;
		Ref<Connection> connection = m_mapListen.getValue(address);
		if (connection.isNotNull()) {
			Packet packet;
			packet.connection = connection;
			packet.content = Memory::create(data, size);
			m_queuePackets.pushBack(packet);
			m_eventProcess->set();
			return;
		}
		WeakRef<PseudoTcpMessage> thiz = this;
		auto callbackUpdate = [this, thiz, address, callbackMessage](Connection* connection) {
			if (connection->flagEnd) {
				return;
			}
			Ref<PseudoTcpMessage> ref = thiz;
			if (ref.isNull()) {
				connection->flagEnd = sl_true;
				return;
			}
			if (connection->flagError) {
				endListeningConnection(address, connection);
				return;
			}
			if (connection->isReadComplete()) {
				if (!(connection->flagCalledReceiveCallback)) {
					connection->flagCalledReceiveCallback = sl_true;
					Shared<MemoryOutput> output = Shared<MemoryOutput>::create();
					if (output.isNull()) {
						endListeningConnection(address, connection);
						return;
					}
					Memory mem = connection->getReceivedData();
					Promise<sl_bool> promise = callbackMessage((sl_uint8*)(mem.getData()), (sl_uint32)(mem.getSize()), output.get());
					if (promise.isNotNull()) {
						Ref<Connection> refConnection = connection;
						promise.then([this, thiz, address, mem, output, refConnection](sl_bool flag) {
							Ref<PseudoTcpMessage> ref = thiz;
							if (ref.isNull()) {
								refConnection->flagEnd = sl_true;
								return;
							}
							dispatch([this, address, output, refConnection, flag]() {
								if (flag) {
									Memory mem = output->merge();
									if (refConnection->setSendingData(mem.getData(), mem.getSize())) {
										refConnection->onTcpWriteable(sl_null);
										return;
									}
								}
								endListeningConnection(address, refConnection);
							});
						});
					} else {
						mem = output->merge();
						if (connection->setSendingData(mem.getData(), mem.getSize())) {
							connection->onTcpWriteable(sl_null);
						} else {
							endListeningConnection(address, connection);
							return;
						}
					}
				}
				if (connection->isWriteComplete()) {
					if (connection->isReadCompleteOver()) {
						endListeningConnection(address, connection);
					}
				}
			}
		};
		connection = new Connection(conversationNo, callbackUpdate, callbackSendPacket, m_timeout);
		if (connection.isNotNull()) {
			m_mapListen.put(address, connection);
			Packet packet;
			packet.connection = connection;
			packet.content = Memory::create(data, size);
			m_queuePackets.pushBack(packet);
			m_eventProcess->set();
		}
	}

	sl_uint32 PseudoTcpMessage::generateConversationNo()
	{
		return Base::interlockedIncrement32((sl_int32*)&m_conversationNoLastSent);
	}

	void PseudoTcpMessage::process()
	{
		Thread* thread = Thread::getCurrent();
		if (!thread) {
			return;
		}
		while (thread->isNotStopping()) {
			Function<void()> callback;
			while (m_queueDispatch.popFront(&callback)) {
				callback();
			}
			Packet packet;
			while (m_queuePackets.popFront(&packet)) {
				Ref<Connection> connection = packet.connection;
				if (connection.isNotNull()) {
					connection->tcp.notifyPacket(packet.content.getData(), packet.content.getSize());
				}
			}
			sl_uint32 now = PseudoTcp::now();
			sl_uint32 timeout = 60000;
			{
				MutexLocker lock(m_mapSend.getLocker());
				auto node = m_mapSend.getFirstNode();
				while (node) {
					Ref<Connection>& connection = node->value;
					if (connection->isTimeout(now)) {
						connection->flagError = sl_true;
						connection->onUpdate(connection);
						m_queueEndSend.pushBack(node->key);
					} else {
						connection->tcp.notifyClock(now);
						sl_uint32 t;
						if (connection->tcp.getNextClock(now, t)) {
							if (t < timeout) {
								timeout = t;
							}
						}
					}
					node = node->getNext();
				}
				sl_uint32 key;
				while (m_queueEndSend.popFront(&key)) {
					m_mapSend.remove_NoLock(key);
				}
			}
			{
				MutexLocker lock(m_mapListen.getLocker());
				auto node = m_mapListen.getFirstNode();
				while (node) {
					Ref<Connection>& connection = node->value;
					if (connection->isTimeout(now)) {
						connection->flagError = sl_true;
						connection->onUpdate(connection);
						m_queueEndListen.pushBack(node->key);
					} else {
						connection->tcp.notifyClock(now);
						sl_uint32 t;
						if (connection->tcp.getNextClock(now, t)) {
							if (t < timeout) {
								timeout = t;
							}
						}
					}
					node = node->getNext();
				}
				Address key;
				while (m_queueEndListen.popFront(&key)) {
					m_mapListen.remove_NoLock(key);
				}
			}
			if (m_queueDispatch.isEmpty() && m_queuePackets.isEmpty()) {
				m_eventProcess->wait(timeout);
			}
		}
	}

	void PseudoTcpMessage::dispatch(const Function<void()>& callback)
	{
		if (callback.isNull()) {
			return;
		}
		m_queueDispatch.pushBack(callback);
		m_eventProcess->set();
	}

	void PseudoTcpMessage::endSendingConnection(sl_uint32 conversationNo, Connection* connection)
	{
		connection->flagEnd = sl_true;
		m_queueEndSend.pushBack(conversationNo);
		m_eventProcess->set();
	}

	void PseudoTcpMessage::endListeningConnection(const Address& address, Connection* connection)
	{
		connection->flagEnd = sl_true;
		m_queueEndListen.pushBack(address);
		m_eventProcess->set();
	}

}
