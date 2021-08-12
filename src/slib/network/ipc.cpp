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
#include "slib/core/time_counter.h"
#include "slib/core/serialize/variable_length_integer.h"
#include "slib/core/serialize/buffer.h"

namespace slib
{

	namespace priv
	{
		namespace ipc
		{

#if !defined(SLIB_PLATFORM_IS_LINUX)
			static String GetDomainName(const StringParam& name)
			{
#if defined(SLIB_PLATFORM_IS_WIN32)
				return String::join(System::getWindowsDirectory(), "/Temp/IPC__", name);
#else
				return String::join("/var/tmp/IPC__", name);
#endif
			}
#endif
		
			class DomainSocketIPC : public IPC
			{
			public:
				CList< Ref<Thread> > m_threads;
				
			public:
				DomainSocketIPC()
				{
				}
				
				~DomainSocketIPC()
				{
					List< Ref<Thread> > threads = m_threads.duplicate();
					for (auto& item : threads) {
						item->finish();
					}
					for (auto& item : threads) {
						item->finishAndWait();
					}
				}
				
			public:
				static Ref<DomainSocketIPC> create(const IPCParam& param)
				{
					Ref<DomainSocketIPC> ret = new DomainSocketIPC;
					if (ret.isNotNull()) {
						ret->_init(param);
						return ret;
					}
					return sl_null;
				}
				
			public:
				void sendMessage(const StringParam& _targetName, const Memory& data, const Function<void(sl_uint8* packet, sl_uint32 size)>& callbackResponse) override
				{
					String targetName = _targetName.toString();
					if (targetName.isNotEmpty()) {
						if (data.isNotNull()) {
							if (m_threads.getCount() < m_maxThreadsCount) {
								MoveT<Socket> socket = Socket::openDomainStream();
								if (socket.isOpened()) {
									auto thiz = ToWeakRef(this);
									Ref<Thread> thread = Thread::create([socket, thiz, this, targetName, data, callbackResponse]() {
										auto ref = ToRef(thiz);
										if (ref.isNull()) {
											callbackResponse(sl_null, 0);
											return;
										}
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

				void processSending(const Socket& socket, const String& serverName, const Memory& data, const Function<void(sl_uint8* packet, sl_uint32 size)>& callbackResponse)
				{
					Thread* thread = Thread::getCurrent();
					if (thread) {
#ifdef SLIB_PLATFORM_IS_LINUX
						if (socket.connectAbstractDomainAndWait(serverName)) {
#else
						if (socket.connectDomainAndWait(GetDomainName(serverName))) {
#endif
							if (writeMessage(thread, socket, data.getData(), (sl_uint32)(data.getSize()))) {
								if (thread->isNotStoppingCurrent()) {
									Memory mem = readMessage(thread, socket);
									writeMessage(thread, socket, sl_null, 0); // Dummy message to safely close
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

				Memory readMessage(Thread* thread, const Socket& socket)
				{
					Ref<SocketEvent> event = SocketEvent::createRead(socket);
					if (event.isNull()) {
						return sl_null;
					}
					TimeCounter tc;
					sl_uint32 sizeContent = 0;
					MemoryBuffer bufRead;
					sl_uint8 bufHeader[16];
					sl_uint32 nReadHeader = 0;
					while (thread->isNotStopping()) {
						sl_int32 n = socket.receive(bufHeader + nReadHeader, sizeof(bufHeader) - nReadHeader);
						if (n > 0) {
							nReadHeader += n;
							sl_uint32 nSizeHeader = CVLI::deserialize(bufHeader, nReadHeader, sizeContent);
							if (nSizeHeader) {
								if (nReadHeader - nSizeHeader >= sizeContent) {
									return Memory::create(bufHeader + nSizeHeader, nReadHeader - nSizeHeader);
								}
								bufRead.addStatic(bufHeader + nSizeHeader, nReadHeader - nSizeHeader);
								break;
							}
							if (nReadHeader >= sizeof(bufHeader)) {
								return sl_null;
							}
						} else if (n == SLIB_IO_WOULD_BLOCK) {
							event->wait(10);
							if (tc.getElapsedMilliseconds() > m_timeout) {
								return sl_null;
							}
						} else {
							return sl_null;
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
						sl_int32 n = socket.receive(buf, sizeof(buf));
						if (n > 0) {
							bufRead.addNew(buf, n);
							if (bufRead.getSize() >= sizeContent) {
								return bufRead.merge();
							}
						} else if (n == SLIB_IO_WOULD_BLOCK) {
							event->wait(10);
							if (tc.getElapsedMilliseconds() > m_timeout) {
								return sl_null;
							}
						} else {
							return sl_null;
						}
					}
					return sl_null;
				}

				sl_bool writeMessage(Thread* thread, const Socket& socket, const void* _data, sl_uint32 size)
				{
					sl_uint8* data = (sl_uint8*)_data;
					Ref<SocketEvent> event = SocketEvent::createWrite(socket);
					if (event.isNull()) {
						return sl_false;
					}
					TimeCounter tc;
					sl_uint8 bufHeader[16];
					sl_uint32 nHeader = CVLI::serialize(bufHeader, size);
					sl_uint32 nWriteHeader = 0;
					while (thread->isNotStopping()) {
						sl_int32 n = socket.send(bufHeader + nWriteHeader, nHeader - nWriteHeader);
						if (n >= 0) {
							nWriteHeader += n;
							if (nWriteHeader >= nHeader) {
								break;
							}
						} else if (n == SLIB_IO_WOULD_BLOCK) {
							event->wait(10);
							if (tc.getElapsedMilliseconds() > m_timeout) {
								return sl_false;
							}
						} else {
							return sl_false;
						}
					}
					if (!size) {
						return sl_true;
					}
					sl_uint32 posWrite = 0;
					while (thread->isNotStopping()) {
						sl_int32 n = socket.send(data + posWrite, size - posWrite);
						if (n >= 0) {
							posWrite += n;
							if (posWrite >= size) {
								break;
							}
						} else if (n == SLIB_IO_WOULD_BLOCK) {
							event->wait(10);
							if (tc.getElapsedMilliseconds() > m_timeout) {
								return sl_false;
							}
						} else {
							return sl_false;
						}
					}
					return sl_true;
				}

			};
				
			class DomainSocketServer : public DomainSocketIPC
			{
			public:
				Socket m_socketServer;
				Ref<Thread> m_threadListen;
				
			public:
				DomainSocketServer()
				{
				}

				~DomainSocketServer()
				{
					if (m_threadListen.isNotNull()) {
						m_threadListen->finishAndWait();
					}
				}

			public:
				static Ref<DomainSocketServer> create(const IPCParam& param)
				{
					Socket socket = Socket::openDomainStream();
					if (socket.isOpened()) {
#ifdef SLIB_PLATFORM_IS_LINUX
						if (socket.bindAbstractDomain(param.name)) {
#else
						String path = GetDomainName(param.name);
						File::deleteFile(path);
						if (socket.bindDomain(path)) {
#if !defined(SLIB_PLATFORM_IS_WINDOWS)
							if (param.flagAcceptOtherUsers) {
								File::setAttributes(path, FileAttributes::AllAccess);
							}
#endif
#endif
							if (socket.setNonBlockingMode(sl_true)) {
								if (socket.listen()) {
									Ref<DomainSocketServer> ret = new DomainSocketServer;
									if (ret.isNotNull()) {
										ret->_init(param);
										ret->m_socketServer = Move(socket);
										Ref<Thread> thread = Thread::start(SLIB_FUNCTION_MEMBER(DomainSocketServer, runListen, ret.get()));
										if (thread.isNotNull()) {
											ret->m_threadListen = Move(thread);
											return ret;
										}
									}
								}
							}
						}
					}
					return sl_null;
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
						if (m_threads.getCount() < m_maxThreadsCount) {
							String address;
							MoveT<Socket> socket = m_socketServer.acceptDomain(address);
							if (socket.isOpened()) {
								auto thiz = ToWeakRef(this);
								Ref<Thread> threadNew = Thread::create([socket, thiz, this]() {
									auto ref = ToRef(thiz);
									if (ref.isNull()) {
										return;
									}
									processReceiving(socket);
								});
								if (threadNew.isNotNull()) {
									m_threads.add(threadNew);
									threadNew->start();
								}
							} else {
								if (Socket::getLastError() == SocketError::WouldBlock) {
									event->wait();
								} else {
									return;
								}
							}
						} else {
							thread->wait(10);
						}
					}
				}

				void processReceiving(const Socket& socket)
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
						readMessage(thread, socket); // Dummy message to safely close
					}
					m_threads.remove(thread);
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
		timeout = 10000;
		flagAcceptOtherUsers = sl_true;
	}


	SLIB_DEFINE_OBJECT(IPC, Object)

	IPC::IPC()
	{
	}

	IPC::~IPC()
	{
	}

	Ref<IPC> IPC::create()
	{
		IPCParam param;
		return create(param);
	}

#if defined(SLIB_PLATFORM_IS_UNIX)
	Ref<IPC> IPC::create(const IPCParam& param)
	{
		return createDomainSocket(param);
	}
#endif

	Ref<IPC> IPC::createDomainSocket(const IPCParam& param)
	{
		if (param.name.isEmpty()) {
			return Ref<IPC>::from(DomainSocketIPC::create(param));
		} else {
			return Ref<IPC>::from(DomainSocketServer::create(param));
		}
	}

	void IPC::_init(const IPCParam& param) noexcept
	{
		m_maxThreadsCount = param.maxThreadsCount;
		m_maxReceivingMessageSize = param.maxReceivingMessageSize;
		m_timeout = param.timeout;
		m_flagAcceptOtherUsers = param.flagAcceptOtherUsers;
		m_onReceiveMessage = param.onReceiveMessage;
	}

}
