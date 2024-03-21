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
#include "slib/io/chunk.h"
#include "slib/io/file.h"
#include "slib/core/system.h"
#include "slib/core/thread.h"

namespace slib
{

	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(IPCRequestParam)

	IPCRequestParam::IPCRequestParam()
	{
		timeout = -1;
		flagSelfAlive = sl_true;
		maximumMessageSize = 0x7fffffff;
		messageSegmentSize = 0;
	}


	SLIB_DEFINE_OBJECT(IPCRequest, Object)

	IPCRequest::IPCRequest()
	{
		m_nCountFinish = 0;
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
		m_tickEnd = GetTickFromTimeout(param.timeout);
		m_maximumResponseSize = param.maximumMessageSize;
		m_messageSegmentSize = param.messageSegmentSize;
		m_dispatcher = param.dispatcher;
		m_onResponse = param.onResponse;
		return sl_true;
	}

	void IPCRequest::onError()
	{
		IPCResponseMessage err;
		onResponse(err);
	}

	void IPCRequest::onResponse(IPCResponseMessage& response)
	{
		if (Base::interlockedIncrement32(&m_nCountFinish) == 1) {
			m_onResponse(response);
			if (m_flagSelfAlive) {
				decreaseReference();
			}
		}
	}

	void IPCRequest::sendRequest()
	{
		ChunkIO::writeAsync(m_stream, m_requestData, SLIB_FUNCTION_WEAKREF(this, onSentRequest), GetTimeoutFromTick(m_tickEnd));
	}

	void IPCRequest::onSentRequest(AsyncStream*, sl_bool flagError)
	{
		if (flagError) {
			onError();
			return;
		}
		receiveResponse();
	}

	void IPCRequest::receiveResponse()
	{
		ChunkIO::readAsync(m_stream, SLIB_FUNCTION_WEAKREF(this, onReceiveResponse), m_maximumResponseSize, m_messageSegmentSize, GetTimeoutFromTick(m_tickEnd));
	}

	void IPCRequest::onReceiveResponse(AsyncStream*, Memory& data, sl_bool flagError)
	{
		if (flagError) {
			onError();
			return;
		}
		IPCResponseMessage response(data);
		onResponse(response);
	}


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(IPCServerParam)

	IPCServerParam::IPCServerParam()
	{
		maximumMessageSize = 0x7fffffff;
		messageSegmentSize = 0;
		flagAcceptOtherUsers = sl_true;
	}


	SLIB_DEFINE_OBJECT(IPCServer, Object)

	IPCServer::IPCServer()
	{
	}

	IPCServer::~IPCServer()
	{
	}

	sl_bool IPCServer::initialize(const IPCServerParam& param)
	{
		m_ioLoop = param.ioLoop;
		if (m_ioLoop.isNull()) {
			m_ioLoop = AsyncIoLoop::create(sl_false);
			if (m_ioLoop.isNull()) {
				return sl_null;
			}
		}
		m_dispatcher = param.dispatcher;
		m_maximumMessageSize = param.maximumMessageSize;
		m_messageSegmentSize = param.messageSegmentSize;
		m_onReceiveMessage = param.onReceiveMessage;
	}

	void IPCServer::startStream(AsyncStream* stream)
	{
		m_streams.put(stream, stream);
		receiveRequest(stream);
	}

	void IPCServer::receiveRequest(AsyncStream* stream)
	{
		ChunkIO::readAsync(stream, SLIB_FUNCTION_WEAKREF(this, onReceiveRequest), m_maximumMessageSize, m_messageSegmentSize);
	}

	void IPCServer::onReceiveRequest(AsyncStream* stream, Memory& data, sl_bool flagError)
	{
		if (flagError) {
			if (stream) {
				m_streams.remove(stream);
			}
			return;
		}
		if (m_dispatcher.isNotNull()) {
			Ref<AsyncStream> refStream = stream;
			WeakRef<IPCServer> thiz = this;
			m_dispatcher->dispatch([this, thiz, refStream, stream, data]() {
				Ref<IPCServer> ref = thiz;
				if (ref.isNull()) {
					return;
				}
				processRequest(stream, data);
			});
		} else {
			processRequest(stream, data);
		}
	}

	void IPCServer::processRequest(AsyncStream* stream, const Memory& data)
	{
		IPCRequestMessage request(data);
		IPCResponseMessage response;
		m_onReceiveMessage(request, response);
		sendResponse(stream, response.getMemory());
	}

	void IPCServer::sendResponse(AsyncStream* stream, const Memory& data)
	{
		ChunkIO::writeAsync(stream, data, SLIB_FUNCTION_WEAKREF(this, onSentResponse));
	}

	void IPCServer::onSentResponse(AsyncStream* stream, sl_bool flagError)
	{
		if (flagError) {
			if (stream) {
				m_streams.remove(stream);
			}
			return;
		}
		receiveRequest(stream);
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

		class SocketRequest : public IPCRequest
		{
		public:
			void connect(const IPCRequestParam& param)
			{
				((AsyncDomainSocket*)(m_stream.get()))->connect(DOMAIN_PATH(param.targetName), SLIB_FUNCTION_WEAKREF(this, onConnect), param.timeout);
			}

			void onConnect(AsyncDomainSocket* socket, sl_bool flagError)
			{
				if (flagError) {
					onError();
					return;
				}
				sendRequest();
			}
		};

		class SocketServer : public IPCServer
		{
		public:
			Ref<AsyncDomainSocketServer> m_server;

		public:
			static Ref<SocketServer> create(const IPCServerParam& param)
			{
				AsyncDomainSocketServerParam serverParam;
#if defined(SLIB_PLATFORM_IS_LINUX)
				serverParam.bindPath = AbstractDomainSocketPath(param.name);
#else
				String path = GetDomainName(param.name);
				File::deleteFile(path);
				serverParam.bindPath = DomainSocketPath(path);
#if !defined(SLIB_PLATFORM_IS_WINDOWS)
				if (param.flagAcceptOtherUsers) {
					File::setAttributes(path, FileAttributes::AllAccess);
				}
#endif
#endif
				Ref<SocketServer> ret = new SocketServer;
				if (ret.isNotNull()) {
					if (ret->initialize(param)) {
						serverParam.ioLoop = ret->m_ioLoop;
						serverParam.onAccept = SLIB_FUNCTION_WEAKREF(ret, onAccept);
						Ref<AsyncDomainSocketServer> server = AsyncDomainSocketServer::create(serverParam);
						if (server.isNotNull()) {
							ret->m_server = server;
							serverParam.ioLoop->start();
							return ret;
						}
					}
				}
				return sl_null;
			}

		public:
			void onAccept(AsyncDomainSocketServer*, Socket& socket, DomainSocketPath& path)
			{
				Ref<AsyncSocketStream> stream = AsyncSocketStream::create(Move(socket), m_ioLoop);
				if (stream.isNotNull()) {
					startStream(stream.get());
				}
			}

		};

	}

	Ref<SocketIPC::Request> SocketIPC::sendMessage(const RequestParam& param)
	{
		Ref<SocketRequest> request = new SocketRequest;
		if (request.isNotNull()) {
			Ref<AsyncDomainSocket> socket = AsyncDomainSocket::create(param.ioLoop);
			if (socket.isNotNull()) {
				if (request->initialize(Move(socket), param)) {
					request->connect(param);
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
		sl_int64 tickEnd = GetTickFromTimeout(timeout);
		if (!(socket.connectAndWait(DOMAIN_PATH(param.targetName), timeout))) {
			return sl_false;
		}
		if (!(ChunkIO::write(&socket, MemoryView(param.message.data, param.message.size), GetTimeoutFromTick(tickEnd)))) {
			return sl_false;
		}
		CurrentThread thread;
		if (thread.isStopping()) {
			return sl_false;
		}
		Nullable<Memory> ret = ChunkIO::read(&socket, param.maximumMessageSize, param.messageSegmentSize, GetTimeoutFromTick(tickEnd));
		if (ret.isNull()) {
			return sl_false;
		}
		response.setMemory(ret.value);
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
		return Ref<SocketIPC::Server>::from(SocketServer::create(param));
	}

}
