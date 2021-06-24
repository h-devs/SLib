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

#include "slib/network/ipc.h"

#include "slib/network/socket.h"
#include "slib/network/event.h"
#include "slib/core/memory_output.h"
#include "slib/core/system.h"
#include "slib/core/file.h"
#include "slib/core/thread.h"
#include "slib/core/moving_container.h"
#include "slib/core/serialize/variable_length_integer.h"
#include "slib/core/serialize/buffer.h"

namespace slib
{

	namespace priv
	{
		namespace ipc
		{

#if defined(SLIB_PLATFORM_IS_WIN32)
			static String GetDomainName(const StringParam& name)
			{
				return String::join(System::getTempDirectory(), "/", name);
			}
#endif

			class DomainSocketIPCImpl : public IPC
			{
			public:
				Ref<Socket> m_socketServer;
				Ref<Thread> m_threadListen;
				CList< Ref<Thread> > m_threads;

				sl_uint32 m_maxThreadsCount;
				sl_uint32 m_maxReceivingMessageSize;

				Function<void(sl_uint8* data, sl_uint32 size, MemoryOutput* output)> m_onReceiveMessage;
				
			public:
				DomainSocketIPCImpl()
				{
				}

				~DomainSocketIPCImpl()
				{
					if (m_threadListen.isNotNull()) {
						m_threadListen->finishAndWait();
					}
					List< Ref<Thread> > threads = m_threads.duplicate();
					for (auto& item : threads) {
						item->finish();
					}
					for (auto& item : threads) {
						item->finishAndWait();
					}
				}

			public:
				static Ref<DomainSocketIPCImpl> create(const IPCParam& param)
				{
					Ref<Socket> socket = Socket::openDomainStream();
					if (socket.isNotNull()) {
#if defined(SLIB_PLATFORM_IS_WIN32)
						String path = GetDomainName(param.name);
						File::deleteFile(path);
						if (socket->bindDomain(path)) {
#else
						if (socket->bindAbstractDomain(param.name))) {
#endif
							if (socket->setNonBlockingMode(sl_true)) {
								if (socket->listen()) {
									Ref<DomainSocketIPCImpl> ret = new DomainSocketIPCImpl;
									if (ret.isNotNull()) {
										ret->m_maxThreadsCount = param.maxThreadsCount;
										ret->m_maxReceivingMessageSize = param.maxReceivingMessageSize;
										ret->m_socketServer = Move(socket);
										Ref<Thread> thread = Thread::start(SLIB_FUNCTION_MEMBER(DomainSocketIPCImpl, runListen, ret.get()));
										if (thread.isNotNull()) {
											ret->m_threadListen = Move(thread);
											ret->m_onReceiveMessage = param.onReceiveMessage;
											return ret;
										}
									}
								}
							}
						}
					}
					return sl_null;
				}

				void sendMessage(const StringParam& _targetName, const Memory& data, const Function<void(sl_uint8* packet, sl_uint32 size)>& callbackResponse) override
				{
					String targetName = _targetName.toString();
					if (targetName.isNotEmpty()) {
						if (data.isNotNull()) {
							if (m_threads.getCount() < m_maxThreadsCount) {
								Ref<Socket> socket = Socket::openDomainStream();
								if (socket.isNotNull()) {
									MovingContainer< Ref<Socket> > _socket(Move(socket));
									auto thiz = ToWeakRef(this);
									Ref<Thread> thread = Thread::create([_socket, thiz, this, targetName, data, callbackResponse]() {
										auto ref = ToRef(thiz);
										if (ref.isNull()) {
											callbackResponse(sl_null, 0);
											return;
										}
										Ref<Socket> socket = Move(_socket.value);
										processSending(socket, targetName, data, callbackResponse);
									});
									if (thread.isNotNull()) {
										m_threads.add(thread);
										thread->start();
										return;
									}
								}
							}
						}
					}
					callbackResponse(sl_null, 0);
				}

				void processSending(const Ref<Socket>& socket, const String& serverName, const Memory& data, const Function<void(sl_uint8* packet, sl_uint32 size)>& callbackResponse)
				{
					Thread* thread = Thread::getCurrent();
					if (thread) {
#if defined(SLIB_PLATFORM_IS_WIN32)
						if (socket->connectDomainAndWait(GetDomainName(serverName))) {
#else
						if (socket->connectAbstractDomainAndWait(serverName)) {
#endif
							if (writeMessage(thread, socket, data.getData(), (sl_uint32)(data.getSize()))) {
								if (thread->isNotStoppingCurrent()) {
									Memory mem = readMessage(thread, socket);
									callbackResponse((sl_uint8*)(mem.getData()), (sl_uint32)(mem.getSize()));
									m_threads.remove(thread);
									return;
								}
							}
						}
					}
					callbackResponse(sl_null, 0);
					m_threads.remove(thread);
				}

				void runListen()
				{
					Thread* thread = Thread::getCurrent();
					if (!thread) {
						return;
					}
					Ref<SocketEvent> event = SocketEvent::createRead(m_socketServer);
					if (event.isNull()) {
						return;
					}
					while (thread->isNotStopping()) {
						String address;
						Ref<Socket> socket = m_socketServer->acceptDomain(address);
						if (socket.isNotNull()) {
							if (m_threads.getCount() < m_maxThreadsCount) {
								MovingContainer< Ref<Socket> > _socket(Move(socket));
								auto thiz = ToWeakRef(this);
								Ref<Thread> thread = Thread::create([_socket, thiz, this]() {
									auto ref = ToRef(thiz);
									if (ref.isNull()) {
										return;
									}
									Ref<Socket> socket = Move(_socket.value);
									processReceiving(socket);
								});
								if (thread.isNotNull()) {
									m_threads.add(thread);
									thread->start();
								}
							}
						} else {
							event->wait();
						}
					}
				}

				void processReceiving(const Ref<Socket>& socket)
				{
					Thread* thread = Thread::getCurrent();
					if (!thread) {
						return;
					}
					Memory mem = readMessage(thread, socket);
					if (mem.isNotNull() && thread->isNotStoppingCurrent()) {
						MemoryOutput response;
						m_onReceiveMessage((sl_uint8*)(mem.getData()), (sl_uint32)(mem.getSize()), &response);
						Memory output = response.getData();
						writeMessage(thread, socket, output.getData(), (sl_uint32)(output.getSize()));
					}
					m_threads.remove(thread);
				}

				Memory readMessage(Thread* thread, const Ref<Socket>& socket)
				{
					Ref<SocketEvent> event = SocketEvent::createRead(socket);
					if (event.isNull()) {
						return sl_null;
					}
					sl_uint32 sizeContent = 0;
					MemoryBuffer bufRead;
					sl_uint8 bufHeader[16];
					sl_uint32 nReadHeader = 0;
					while (thread->isNotStopping()) {
						sl_int32 n = socket->receive(bufHeader + nReadHeader, sizeof(bufHeader) - nReadHeader);
						if (n < 0) {
							return sl_null;
						}
						if (n) {
							nReadHeader += n;
							sl_uint32 nSizeHeader = CVLI::deserialize(bufHeader, nReadHeader, sizeContent);
							if (nSizeHeader) {
								bufRead.addStatic(bufHeader + nSizeHeader, nReadHeader - nSizeHeader);
								break;
							}
							if (nReadHeader >= sizeof(bufHeader)) {
								return sl_null;
							}
						} else {
							event->wait();
						}
					}
					if (!sizeContent) {
						return sl_null;
					}
					if (sizeContent > m_maxReceivingMessageSize) {
						return sl_null;
					}
					char buf[1024];
					while (thread->isNotStopping()) {
						sl_int32 n = socket->receive(buf, sizeof(buf));
						if (n < 0) {
							return sl_null;
						}
						if (n) {
							bufRead.add(Memory::create(buf, n));
							if (bufRead.getSize() >= sizeContent) {
								return bufRead.merge();
							}
						} else {
							event->wait();
						}
					}
					return sl_null;
				}

				sl_bool writeMessage(Thread* thread, const Ref<Socket>& socket, const void* _data, sl_uint32 size)
				{
					sl_uint8* data = (sl_uint8*)_data;
					Ref<SocketEvent> event = SocketEvent::createWrite(socket);
					if (event.isNull()) {
						return sl_false;
					}
					sl_uint8 bufHeader[16];
					sl_uint32 nHeader = CVLI::serialize(bufHeader, size);
					sl_uint32 nWriteHeader = 0;
					while (thread->isNotStopping()) {
						sl_int32 n = socket->send(bufHeader + nWriteHeader, nHeader - nWriteHeader);
						if (n < 0) {
							return sl_false;
						}
						if (n) {
							nWriteHeader += n;
							if (nWriteHeader >= nHeader) {
								break;
							}
						} else {
							event->wait();
						}
					}
					if (!size) {
						return sl_true;
					}
					sl_uint32 posWrite = 0;
					while (thread->isNotStopping()) {
						sl_int32 n = socket->send(data + posWrite, size - posWrite);
						if (n < 0) {
							return sl_false;
						}
						if (n) {
							posWrite += n;
							if (posWrite >= size) {
								break;
							}
						} else {
							event->wait();
						}
					}
					return sl_true;
				}

			};

		}
	}

	using namespace priv::ipc;


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(IPCParam)

	IPCParam::IPCParam()
	{
		maxThreadsCount = 16;
		maxReceivingMessageSize = 64 << 20;
	}


	SLIB_DEFINE_OBJECT(IPC, Object)

	IPC::IPC()
	{
	}

	IPC::~IPC()
	{
	}

#if defined(SLIB_PLATFORM_IS_UNIX)
	Ref<IPC> IPC::create(const IPCParam& param)
	{
		return createDomainSocket(param);
	}
#endif

	Ref<IPC> IPC::createDomainSocket(const IPCParam& param)
	{
		return Ref<IPC>::from(DomainSocketIPCImpl::create(param));
	}

}
