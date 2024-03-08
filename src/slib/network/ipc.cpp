/*
 *   Copyright (c) 2008-2024 SLIBIO <https://github.com/SLIBIO>
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

#include "slib/network/async.h"
#include "slib/network/event.h"
#include "slib/core/system.h"
#include "slib/core/thread.h"
#include "slib/io/file.h"
#include "slib/data/serialize/variable_length_integer.h"
#include "slib/data/serialize/buffer.h"
#include "slib/device/cpu.h"

namespace slib
{

	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(IPCRequestParam)

	IPCRequestParam::IPCRequestParam()
	{
		timeout = -1;
		flagSelfAlive = sl_true;
		maximumMessageSize = -1;
	}


	SLIB_DEFINE_OBJECT(IPCRequest, Object)

	IPCRequest::IPCRequest()
	{
	}

	IPCRequest::~IPCRequest()
	{
	}

	sl_bool IPCRequest::initialize(Ref<AsyncStream>&& stream, const IPCRequestParam& param)
	{
		m_stream = Move(stream);
		if (param.message.isNotEmpty()) {
			Memory content = param.message.getMemory();
			if (content.isNull()) {
				return sl_false;
			}
			m_requestData = Move(content);
		}
		if (param.flagSelfAlive) {
			increaseReference();
		}
		sl_int32 timeout = param.timeout;
		if (timeout >= 0) {
			m_tickEnd = GetCurrentTick() + timeout;
		} else {
			m_tickEnd = 0;
		}
		m_maximumResponseSize = param.maximumMessageSize;
		m_dispatcher = param.dispatcher;
		m_onResponse = param.onResponse;
		return sl_true;
	}

	sl_uint64 IPCRequest::getCurrentTick()
	{
		return System::getTickCount64();
	}

	void IPCRequest::onError()
	{
		Function<void(IPCResponseMessage&)> callback = m_onResponse.release();
		if (callback.isNotNull()) {
			IPCResponseMessage err;
			callback(err);
		}
	}


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(IPCServerParam)

	IPCServerParam::IPCServerParam()
	{
		maximumMessageSize = -1;
		flagAcceptOtherUsers = sl_true;

		maximumThreadCount = Cpu::getCoreCount();
		if (!maximumThreadCount) {
			maximumThreadCount = 1;
		}
		minimumThreadCount = maximumThreadCount / 2;
		flagProcessByThreads = sl_false;
	}


	SLIB_DEFINE_OBJECT(IPCServer, Object)

	IPCServer::IPCServer()
	{
	}

	IPCServer::~IPCServer()
	{
	}

#ifdef SLIB_PLATFORM_IS_UNIX
	Ref<IPC::Request> IPC::sendMessage(const RequestParam& param)
	{
		return SocketIPC::sendMessage(param);
	}
#endif

	Ref<IPC::Request> IPC::sendMessage(const StringParam& targetName, const RequestMessage& message, const Function<void(ResponseMessage&)>& callbackResponse)
	{
		RequestParam param;
		param.targetName = targetName;
		param.message = message;
		param.onResponse = callbackResponse;
		return sendMessage(param);
	}

#ifdef SLIB_PLATFORM_IS_UNIX
	sl_bool IPC::sendMessageSynchronous(const RequestParam& param, ResponseMessage& response)
	{
		return SocketIPC::sendMessageSynchronous(param, response);
	}
#endif

	sl_bool IPC::sendMessageSynchronous(const StringParam& targetName, const RequestMessage& request, ResponseMessage& response, sl_int32 timeout)
	{
		RequestParam param;
		param.targetName = targetName;
		param.message = request;
		param.timeout = timeout;
		return sendMessageSynchronous(param, response);
	}

#ifdef SLIB_PLATFORM_IS_UNIX
	Ref<IPC::Server> IPC::createServer(const ServerParam& param)
	{
		return SocketIPC::createServer(param);
	}
#endif

	// IPC by domain socket
	namespace
	{

#if defined(SLIB_PLATFORM_IS_LINUX)
#	define DOMAIN_PATH(NAME) AbstractDomainSocketPath(NAME)
#else
		static String GetDomainName(const StringParam& name)
		{
#if defined(SLIB_PLATFORM_IS_WIN32)
			return String::concat(System::getWindowsDirectory(), "/Temp/IPC__", name);
#else
			return String::concat("/var/tmp/IPC__", name);
#endif
		}
#	define DOMAIN_PATH(NAME) DomainSocketPath(GetDomainName(NAME))
#endif

		SLIB_INLINE static sl_uint64 GetCurrentTick()
		{
			return IPCRequest::getCurrentTick();
		}

		static sl_bool WriteDataSynchronous(Thread* thread, const Socket& socket, const Ref<SocketEvent>& event, const void* _data, sl_uint32 size, sl_uint64 tickEnd)
		{
			sl_uint8* data = (sl_uint8*)_data;
			sl_uint32 nWrite = 0;
			for (;;) {
				if (thread) {
					if (thread->isStopping()) {
						return sl_false;
					}
				}
				sl_int32 n = socket.send(data + nWrite, size - nWrite);
				if (n >= 0) {
					nWrite += n;
					if (nWrite >= size) {
						break;
					}
				} else if (n == SLIB_IO_WOULD_BLOCK) {
					if (tickEnd) {
						event->wait(10);
						if (GetCurrentTick() > tickEnd) {
							return sl_false;
						}
					} else {
						event->wait();
					}
				} else {
					return sl_false;
				}
			}
			return sl_true;
		}

		static sl_bool WriteMessageSynchronous(Thread* thread, const Socket& socket, const void* data, sl_uint32 size, sl_uint64 tickEnd)
		{
			Ref<SocketEvent> event = SocketEvent::createWrite(socket);
			if (event.isNull()) {
				return sl_false;
			}
			sl_uint8 bufHeader[16];
			sl_uint32 nHeader = CVLI::serialize(bufHeader, size);
			if (!(WriteDataSynchronous(thread, socket, event, bufHeader, nHeader, tickEnd))) {
				return sl_false;
			}
			if (!size) {
				return sl_true;
			}
			return WriteDataSynchronous(thread, socket, event, data, size, tickEnd);
		}

		static sl_bool ReadMessageSynchronous(Thread* thread, const Socket& socket, IPC::ResponseMessage& response, sl_uint64 tickEnd, sl_int32 maxMessageSize)
		{
			Ref<SocketEvent> event = SocketEvent::createRead(socket);
			if (event.isNull()) {
				return sl_false;
			}
			sl_uint8 bufHeader[16];
			sl_uint32 nReadHeader = 0;
			Memory memContent;
			sl_uint32 sizeContent = 0;
			sl_uint8* bufContent;
			for (;;) {
				if (thread) {
					if (thread->isStopping()) {
						return sl_false;
					}
				}
				sl_int32 n = socket.receive(bufHeader + nReadHeader, sizeof(bufHeader) - nReadHeader);
				if (n > 0) {
					nReadHeader += n;
					sl_uint32 nSizeHeader = CVLI::deserialize(bufHeader, nReadHeader, sizeContent);
					if (nSizeHeader) {
						if (!sizeContent) {
							response.clear();
							response.data = "";
							return sl_true;
						}
						if (maxMessageSize > 0 && sizeContent > (sl_uint32)maxMessageSize) {
							return sl_false;
						}
						sl_uint32 nReadContent = nReadHeader - nSizeHeader;
						if (nReadContent >= sizeContent) {
							response.setMemory(Memory::create(bufHeader + nSizeHeader, sizeContent));
							return response.isNotEmpty();
						}
						memContent = Memory::create(sizeContent);
						if (memContent.isNull()) {
							return sl_false;
						}
						bufContent = (sl_uint8*)(memContent.getData());
						Base::copyMemory(bufContent, bufHeader + nSizeHeader, nReadContent);
						sizeContent -= nReadContent;
						bufContent += nReadContent;
						break;
					}
					if (nReadHeader >= sizeof(bufHeader)) {
						return sl_false;
					}
				} else if (n == SLIB_IO_WOULD_BLOCK) {
					if (tickEnd) {
						event->wait(10);
						if (GetCurrentTick() > tickEnd) {
							return sl_false;
						}
					} else {
						event->wait();
					}
				} else {
					return sl_false;
				}
			}
			while (thread->isNotStopping()) {
				sl_int32 n = socket.receive(bufContent, sizeContent);
				if (n > 0) {
					if ((sl_uint32)n >= sizeContent) {
						response.setMemory(memContent);
						return sl_true;
					}
					sizeContent -= n;
					bufContent += n;
				} else if (n == SLIB_IO_WOULD_BLOCK) {
					if (tickEnd) {
						event->wait(10);
						if (GetCurrentTick() > tickEnd) {
							return sl_false;
						}
					} else {
						event->wait();
					}
				} else {
					return sl_false;
				}
			}
			return sl_false;
		}

		class SocketRequest : public IPC::Request
		{
		public:
			sl_uint64 tickEnd;
			Memory content;

		public:
			void onConnect(AsyncDomainSocket* socket, sl_bool flagError)
			{
				if (flagError) {
					onError();
					return;
				}
			}
		};

	}

	Ref<SocketIPC::Request> SocketIPC::sendMessage(const RequestParam& param)
	{
		Ref<SocketRequest> request = new SocketRequest;
		if (request.isNotNull()) {
			request->socket = AsyncDomainSocket::create(param.ioLoop);
			if (request->socket.isNotNull()) {
				if (request->initialize(param)) {
					request->socket->connect(DOMAIN_PATH(param.targetName), SLIB_FUNCTION_WEAKREF(request, onConnect));
					return request;
				}
			}
		}
		ResponseMessage errorMsg;
		param.onResponse(errorMsg);
		return sl_null;
	}

	Ref<SocketIPC::Request> SocketIPC::sendMessage(const StringParam& targetName, const RequestMessage& message, const Function<void(ResponseMessage&)>& callbackResponse)
	{
		RequestParam param;
		param.targetName = targetName;
		param.message = message;
		param.onResponse = callbackResponse;
		return sendMessage(param);
	}

	sl_bool SocketIPC::sendMessageSynchronous(const RequestParam& param, ResponseMessage& response)
	{
		Socket socket = Socket::openDomainStream();
		if (socket.isNone()) {
			return sl_false;
		}
		sl_int32 timeout = param.timeout;
		sl_uint64 tickEnd;
		if (timeout >= 0) {
			tickEnd = GetCurrentTick() + timeout;
		} else {
			tickEnd = 0;
		}
		if (!(socket.connectAndWait(DOMAIN_PATH(param.targetName), timeout))) {
			return sl_false;
		}
		if (GetCurrentTick() > tickEnd) {
			return sl_false;
		}
		Thread* thread = Thread::getCurrent();
		if (!(WriteMessageSynchronous(thread, socket, param.message.data, (sl_uint32)(param.message.size), tickEnd))) {
			return sl_false;
		}
		if (thread) {
			if (thread->isStopping()) {
				return sl_false;
			}
		}
		if (!(ReadMessageSynchronous(thread, socket, response, tickEnd, param.maximumMessageSize))) {
			return sl_false;
		}
		socket.writeUint8(0); // Close Signal
		return sl_true;
	}

	sl_bool SocketIPC::sendMessageSynchronous(const StringParam& targetName, const RequestMessage& request, ResponseMessage& response, sl_int32 timeout)
	{
		RequestParam param;
		param.targetName = targetName;
		param.message = request;
		param.timeout = timeout;
		return sendMessageSynchronous(param, response);
	}

	Ref<SocketIPC::Server> SocketIPC::createServer(const ServerParam& param)
	{
	}

	namespace {

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
			void sendMessage(const StringParam& _targetName, const Memory& data, const Function<void(sl_uint8* packet, sl_uint32 size)>& callbackResponse) override
			{
				String targetName = _targetName.toString();
				if (targetName.isNotEmpty()) {
					if (data.isNotNull()) {
						if (m_threads.getCount() < m_maxThreadCount) {
							MoveT<Socket> socket = Socket::openDomainStream();
							if (socket.isOpened()) {
								auto thiz = ToWeakRef(this);
								Ref<Thread> thread = Thread::create([socket, thiz, this, targetName, data, callbackResponse]() {
									auto ref = ToRef(thiz);
									if (ref.isNull()) {
										callbackResponse(sl_null, 0);
										return;
									}
									processSending(socket, targetName, data, callbackResponse, sl_true);
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

			Memory processSending(const Socket& socket, const String& serverName, const MemoryView& data, const Function<void(sl_uint8* packet, sl_uint32 size)>& callbackResponse, sl_bool flagAsync)
			{
				Thread* thread = Thread::getCurrent();
				if (thread) {
#ifdef SLIB_PLATFORM_IS_LINUX
					if (socket.connectAbstractDomainAndWait(serverName)) {
#else
					if (socket.connectDomainAndWait(GetDomainName(serverName))) {
#endif
						if (writeMessage(thread, socket, data.data, (sl_uint32)(data.size))) {
							if (thread->isNotStoppingCurrent()) {
								Memory mem = readMessage(thread, socket);
								writeMessage(thread, socket, sl_null, 0); // Dummy message to safely close
								if (flagAsync) {
									callbackResponse((sl_uint8*)(mem.getData()), (sl_uint32)(mem.getSize()));
									m_threads.remove(thread);
								}
								return mem;
							}
						}
					}
				}
				if (flagAsync) {
					callbackResponse(sl_null, 0);
					m_threads.remove(thread);
				}
				return sl_null;
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
						if (socket.setNonBlockingMode()) {
							if (socket.listen()) {
								Ref<DomainSocketServer> ret = new DomainSocketServer;
								if (ret.isNotNull()) {
									ret->_init(param);
									ret->m_socketServer = Move(socket);
									Ref<Thread> thread = Thread::start(SLIB_FUNCTION_MEMBER(ret.get(), runListen));
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
					if (m_threads.getCount() < m_maxThreadCount) {
						String address;
						MoveT<Socket> socket = m_socketServer.acceptDomain(address);
						if (socket.isOpened()) {
							if (m_flagReceiveOnNewThread) {
								auto thiz = ToWeakRef(this);
								Ref<Thread> threadNew = Thread::create([socket, thiz, this]() {
									auto ref = ToRef(thiz);
									if (ref.isNull()) {
										return;
									}
									Thread* thread = Thread::getCurrent();
									if (!thread) {
										return;
									}
									processReceiving(socket, thread, sl_true);
								});
								if (threadNew.isNotNull()) {
									m_threads.add(threadNew);
									threadNew->start();
								}
							} else {
								processReceiving(socket, thread, sl_false);
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

			void processReceiving(const Socket& socket, Thread* thread, sl_bool flagAsync)
			{
				Memory mem = readMessage(thread, socket);
				if (mem.isNotNull() && thread->isNotStoppingCurrent()) {
					MemoryOutput response;
					m_onReceiveMessage((sl_uint8*)(mem.getData()), (sl_uint32)(mem.getSize()), &response);
					Memory output = response.merge();
					writeMessage(thread, socket, output.getData(), (sl_uint32)(output.getSize()));
					readMessage(thread, socket); // Dummy message to safely close
				}
				if (flagAsync) {
					m_threads.remove(thread);
				}
			}

		};
	}

	Ref<IPC> IPC::createDomainSocket(const IPCParam& param)
	{
		if (param.name.isEmpty()) {
			return Ref<IPC>::from(DomainSocketIPC::create(param));
		} else {
			return Ref<IPC>::from(DomainSocketServer::create(param));
		}
	}

}
