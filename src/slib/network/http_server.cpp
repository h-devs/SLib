/*
 *   Copyright (c) 2008-2018 SLIBIO <https://github.com/SLIBIO>
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

#include "slib/network/http_server.h"

#include "slib/network/url.h"
#include "slib/core/app.h"
#include "slib/core/asset.h"
#include "slib/core/thread_pool.h"
#include "slib/core/dispatch_loop.h"
#include "slib/core/timer.h"
#include "slib/core/content_type.h"
#include "slib/core/system.h"
#include "slib/core/log.h"
#include "slib/io/async_file.h"
#include "slib/io/file_util.h"
#include "slib/data/json.h"
#include "slib/data/xml.h"
#include "slib/device/cpu.h"

#define SERVER_TAG "HTTP SERVER"

namespace slib
{

	SLIB_DEFINE_OBJECT(HttpServerContext, Object)

	HttpServerContext::HttpServerContext()
	{
		m_requestContentLength = 0;

		m_flagProcessed = sl_false;
		m_flagClosingConnection = sl_false;
		m_flagProcessingByThread = sl_true;
		m_flagKeepAlive = sl_true;

		m_flagBeganProcessing = sl_false;
	}

	HttpServerContext::~HttpServerContext()
	{
	}

	Ref<HttpServerContext> HttpServerContext::create(const Ref<HttpServerConnection>& connection)
	{
		if (connection.isNotNull()) {
			Ref<HttpServerContext> ret = new HttpServerContext;
			if (ret.isNotNull()) {
				ret->m_connection = connection;
				return ret;
			}
		}
		return sl_null;
	}

	Memory HttpServerContext::getRawRequestHeader() const
	{
		return m_requestHeader;
	}

	sl_uint64 HttpServerContext::getRequestContentLength() const
	{
		return m_requestContentLength;
	}

	Memory HttpServerContext::getRequestBody() const
	{
		return m_requestBody;
	}

	Json HttpServerContext::getRequestBodyAsJson() const
	{
		Memory body = m_requestBody;
		return Json::parse((sl_char8*)(body.getData()), body.getSize());
	}

	void HttpServerContext::applyRequestBodyAsFormUrlEncoded()
	{
		Memory body = getRequestBody();
		if (body.isNotNull()) {
			applyFormUrlEncoded(body.getData(), body.getSize());
		}
	}

	sl_uint64 HttpServerContext::getResponseContentLength() const
	{
		return getOutputLength();
	}

	Ref<HttpServer> HttpServerContext::getServer()
	{
		Ref<HttpServerConnection> connection = m_connection;
		if (connection.isNotNull()) {
			return connection->getServer();
		}
		return sl_null;
	}

	Ref<HttpServerConnection> HttpServerContext::getConnection()
	{
		return m_connection;
	}

	Ref<AsyncStream> HttpServerContext::getIO()
	{
		Ref<HttpServerConnection> connection = m_connection;
		if (connection.isNotNull()) {
			return connection->getIO();
		}
		return sl_null;
	}

	Ref<AsyncIoLoop> HttpServerContext::getAsyncIoLoop()
	{
		Ref<HttpServer> server = getServer();
		if (server.isNotNull()) {
			return server->getAsyncIoLoop();
		}
		return sl_null;
	}

	const SocketAddress& HttpServerContext::getLocalAddress()
	{
		Ref<HttpServerConnection> connection = getConnection();
		if (connection.isNotNull()) {
			return connection->getLocalAddress();
		} else {
			return SocketAddress::none();
		}
	}

	const SocketAddress& HttpServerContext::getRemoteAddress()
	{
		Ref<HttpServerConnection> connection = getConnection();
		if (connection.isNotNull()) {
			return connection->getRemoteAddress();
		} else {
			return SocketAddress::none();
		}
	}

	sl_bool HttpServerContext::isProcessed() const
	{
		return m_flagProcessed;
	}

	void HttpServerContext::setProcessed(sl_bool flag)
	{
		m_flagProcessed = flag;
	}

	sl_bool HttpServerContext::isClosingConnection() const
	{
		return m_flagClosingConnection;
	}

	void HttpServerContext::setClosingConnection(sl_bool flag)
	{
		m_flagClosingConnection = flag;
	}

	sl_bool HttpServerContext::isProcessingByThread() const
	{
		return m_flagProcessingByThread;
	}

	void HttpServerContext::setProcessingByThread(sl_bool flag)
	{
		m_flagProcessingByThread = flag;
	}

	sl_bool HttpServerContext::isKeepAlive() const
	{
		return m_flagKeepAlive;
	}

	void HttpServerContext::setKeepAlive(sl_bool flag)
	{
		m_flagKeepAlive = flag;
	}


#define SIZE_READ_BUF 0x10000
#define SIZE_COPY_BUF 0x10000

	SLIB_DEFINE_OBJECT(HttpServerConnection, Object)

	HttpServerConnection::HttpServerConnection()
	{
		m_flagFreed = sl_true;
		m_flagClosed = sl_false;
		m_flagReading = sl_false;
		m_flagKeepAlive = sl_true;
		m_timeLastRead = System::getTickCount64();
	}

	HttpServerConnection::~HttpServerConnection()
	{
		_free();
	}

	Ref<HttpServerConnection> HttpServerConnection::create(HttpServer* server, AsyncStream* io)
	{
		if (server && io) {
			Memory bufRead = Memory::create(SIZE_READ_BUF);
			if (bufRead.isNotNull()) {
				Ref<HttpServerConnection> ret = new HttpServerConnection;
				if (ret.isNotNull()) {
					AsyncOutputParam op;
					op.stream = io;
					op.onEnd = SLIB_FUNCTION_WEAKREF(ret, onAsyncOutputEnd);
					op.bufferSize = SIZE_COPY_BUF;
					Ref<AsyncOutput> output = AsyncOutput::create(op);
					if (output.isNotNull()) {
						ret->m_server = server;
						ret->m_io = io;
						ret->m_output = output;
						ret->m_bufRead = bufRead;
						ret->m_flagFreed = sl_false;
						return ret;
					}
				}
			}
		}
		return sl_null;
	}

	void HttpServerConnection::close()
	{
		if (m_flagClosed) {
			return;
		}
		ObjectLocker lock(this);
		if (m_flagClosed) {
			return;
		}
		m_flagClosed = sl_true;
		Ref<HttpServer> server = m_server;
		if (server.isNotNull()) {
			server->closeConnection(this);
		}
		m_bufReadUnprocessed.setNull();
		_free();
	}

	void HttpServerConnection::_free()
	{
		if (m_flagFreed) {
			return;
		}
		m_flagFreed = sl_true;
		m_io->close();
		m_output->close();
	}

	void HttpServerConnection::start()
	{
		m_contextCurrent.setNull();
		if (m_bufReadUnprocessed.isNotEmpty()) {
			_processInput(sl_null);
		} else {
			_read(sl_null);
		}
	}

	Ref<AsyncStream> HttpServerConnection::getIO()
	{
		return m_io;
	}

	Ref<HttpServer> HttpServerConnection::getServer()
	{
		return m_server;
	}

	Ref<HttpServerContext> HttpServerConnection::getCurrentContext()
	{
		return m_contextCurrent;
	}

	void HttpServerConnection::_read(AsyncStreamResult* result)
	{
		ObjectLocker lock(this);
		if (m_flagClosed) {
			return;
		}
		if (m_flagReading) {
			return;
		}
		m_flagReading = sl_true;
		sl_bool bSuccess;
		if (result) {
			bSuccess = m_io->read(result->data, result->requestSize, result->callback, result->userObject);
		} else {
			bSuccess = m_io->read(m_bufRead, SLIB_FUNCTION_WEAKREF(this, onReadStream));
		}
		if (!bSuccess) {
			m_flagReading = sl_false;
			close();
		}
	}

	void HttpServerConnection::_processInput(AsyncStreamResult* result)
	{
		Ref<HttpServer> server = m_server;
		if (server.isNull()) {
			return;
		}
		if (server->isReleased()) {
			return;
		}

		ObjectLocker lock(this);

		if (m_flagClosed) {
			return;
		}

		char* data;
		sl_size size;
		if (result) {
			data = (char*)(result->data);
			size = result->size;
		} else {
			data = sl_null;
			size = 0;
		}

		if (m_bufReadUnprocessed.isNotEmpty()) {
			if (size) {
				if (!(m_bufReadUnprocessed.addElements_NoLock((char*)data, size))) {
					close();
					return;
				}
			}
			data = m_bufReadUnprocessed.getData();
			size = m_bufReadUnprocessed.getCount();
		}

		if (!size) {
			_read(result);
			return;
		}

		const HttpServerParam& param = server->getParam();
		sl_uint64 maxRequestHeadersSize = param.maxRequestHeadersSize;
		sl_uint64 maxRequestBodySize = param.maxRequestBodySize;

		Ref<HttpServerContext> _context = m_contextCurrent;
		if (_context.isNull()) {
			_context = HttpServerContext::create(this);
			if (_context.isNull()) {
				close();
				return;
			}
			m_contextCurrent = _context;
			_context->setProcessingByThread(param.flagProcessByThreads);
		}
		HttpServerContext* context = _context.get();
		if (context->m_requestHeader.isNull()) {
			sl_size posBody;
			if (context->m_requestHeaderReader.add(data, size, posBody)) {
				context->m_requestHeader = context->m_requestHeaderReader.mergeHeader();
				if (context->m_requestHeader.isNull()) {
					sendResponseAndClose_ServerError();
					return;
				}
				if (posBody > size) {
					sendResponseAndClose_ServerError();
					return;
				}
				context->m_requestHeaderReader.clear();
				Memory header = context->getRawRequestHeader();
				sl_reg iRet = context->parseRequestPacket(header.getData(), header.getSize());
				if (iRet != (sl_reg)(Memory(context->m_requestHeader).getSize())) {
					sendResponseAndClose_BadRequest();
					return;
				}
				context->m_requestContentLength = context->getRequestContentLengthHeader();
				if (context->m_requestContentLength > maxRequestBodySize) {
					sendResponseAndClose_BadRequest();
					return;
				}
				context->setKeepAlive(context->isRequestKeepAlive());
				if (size > posBody) {
					sl_size sizeRemain = size - posBody;
					sl_size sizeRequired = (sl_size)(context->m_requestContentLength);
					if (sizeRequired) {
						if (sizeRequired < sizeRemain) {
							context->m_requestBody = Memory::create(data + posBody, sizeRequired);
							m_bufReadUnprocessed = List<char>::create(data + posBody + sizeRequired, sizeRemain - sizeRequired);
						} else {
							context->m_requestBody = Memory::create(data + posBody, sizeRemain);
							m_bufReadUnprocessed.setNull();
						}
						if (!(context->m_requestBodyBuffer.add(context->m_requestBody))) {
							sendResponseAndClose_ServerError();
							return;
						}
					} else {
						m_bufReadUnprocessed = List<char>::create(data + posBody, sizeRemain);
					}
				} else {
					m_bufReadUnprocessed.setNull();
				}
				context->applyQueryToParameters();
				if (server->preprocessRequest(context)) {
					return;
				}
			} else {
				m_bufReadUnprocessed.setNull();
				if (context->m_requestHeaderReader.getHeaderSize() > maxRequestHeadersSize) {
					sendResponseAndClose_BadRequest();
					return;
				}
			}
		} else {
			sl_size sizeBody = (sl_size)(context->m_requestContentLength);
			sl_size sizeCurrent = context->m_requestBodyBuffer.getSize();
			if (sizeCurrent < sizeBody) {
				sl_size sizeRemain = sizeBody - sizeCurrent;
				if (sizeRemain < size) {
					if (!(context->m_requestBodyBuffer.addNew(data, sizeRemain))) {
						sendResponseAndClose_ServerError();
						return;
					}
					m_bufReadUnprocessed = List<char>::create(data + sizeRemain, size - sizeRemain);
				} else {
					if (!(context->m_requestBodyBuffer.addNew(data, size))) {
						sendResponseAndClose_ServerError();
						return;
					}
					m_bufReadUnprocessed.setNull();
				}
			}
		}

		if (server->isReleased()) {
			return;
		}

		if (!(context->m_flagBeganProcessing)) {

			if (context->m_requestHeader.isNotNull()) {

				sl_size sizeBody = (sl_size)(context->m_requestContentLength);
				sl_size sizeCurrent = context->m_requestBodyBuffer.getSize();

				if (sizeCurrent >= sizeBody) {

					context->m_flagBeganProcessing = sl_true;

					if (sizeBody) {
						if (Memory(context->m_requestBody).getSize() < sizeBody) {
							context->m_requestBody = context->m_requestBodyBuffer.merge();
							if (context->m_requestBody.isNull()) {
								sendResponseAndClose_ServerError();
								return;
							}
						}
					}
					context->m_requestBodyBuffer.clear();

					String multipartBoundary = context->getRequestMultipartFormDataBoundary();
					if (multipartBoundary.isNotEmpty()) {
						Memory body = context->getRequestBody();
						context->applyMultipartFormData(multipartBoundary, body);
					} else if (context->getMethod() == HttpMethod::POST) {
						String reqContentType = context->getRequestContentTypeNoParams();
						if (reqContentType == ContentType::WebForm) {
							Memory body = context->getRequestBody();
							context->applyFormUrlEncoded(body.getData(), body.getSize());
						}
					}
					if (context->isProcessingByThread()) {
						Ref<ThreadPool> threadPool = server->getThreadPool();
						if (threadPool.isNotNull()) {
							threadPool->addTask(SLIB_BIND_WEAKREF(void(), this, _processContext, _context));
						} else {
							sendResponseAndClose_ServerError();
						}
					} else {
						_processContext(context);
					}
					return;
				}
			}

		}

		if (server->isReleased()) {
			return;
		}
		_read(result);
	}

	void HttpServerConnection::_processContext(const Ref<HttpServerContext>& context)
	{
		Ref<HttpServer> server = getServer();
		if (server.isNull()) {
			return;
		}
		if (context->getMethod() == HttpMethod::CONNECT) {
			sendConnectResponse_Failed();
			return;
		}
		server->processRequest(context.get(), this);
	}

	void HttpServerConnection::completeContext(HttpServerContext* context)
	{
		Memory header = context->makeResponsePacket();
		if (header.isNull()) {
			close();
			return;
		}
		if (!(m_output->write(header))) {
			close();
			return;
		}
		m_output->mergeBuffer(&(context->m_bufferOutput));
		if (context->isKeepAlive()) {
			m_output->startWriting();
			start();
		} else {
			m_contextCurrent.setNull();
			m_flagKeepAlive = sl_false;
			m_output->startWriting();
		}
	}

	void HttpServerConnection::onReadStream(AsyncStreamResult& result)
	{
		m_flagReading = sl_false;
		if (!(result.isSuccess())) {
			close();
		} else {
			m_timeLastRead = System::getTickCount64();
			_processInput(&result);
		}
	}

	void HttpServerConnection::onAsyncOutputEnd(AsyncOutput* output, sl_bool flagError)
	{
		if (flagError || !m_flagKeepAlive) {
			close();
		}
	}

	void HttpServerConnection::sendResponseAndRestart(const Memory& mem)
	{
		if (mem.isNotNull()) {
			if (m_io->write(mem, sl_null)) {
				start();
				return;
			}
		}
		close();
	}

	namespace {
		class SendResponseAndCloseListener : public CRef
		{
		public:
			WeakRef<HttpServerConnection> m_connection;

		public:
			SendResponseAndCloseListener(HttpServerConnection* connection)
			{
				m_connection = connection;
			}

			void onWriteStream(AsyncStreamResult& result)
			{
				Ref<HttpServerConnection> connection = m_connection;
				if (connection.isNotNull()) {
					connection->close();
				}
			}
		};
	}

	void HttpServerConnection::sendResponseAndClose(const Memory& mem)
	{
		if (mem.isNotNull()) {
			Ref<SendResponseAndCloseListener> listener(new SendResponseAndCloseListener(this));
			if (m_io->write(mem, SLIB_FUNCTION_REF(listener, onWriteStream))) {
				return;
			}
		}
		close();
	}

	void HttpServerConnection::sendResponseAndClose_BadRequest()
	{
		static char s[] = "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n\r\n";
		sendResponseAndClose(Memory::createStatic(s, sizeof(s) - 1));
	}

	void HttpServerConnection::sendResponseAndClose_ServerError()
	{
		static char s[] = "HTTP/1.1 500 Internal Server Error\r\nContent-Length: 0\r\n\r\n";
		sendResponseAndClose(Memory::createStatic(s, sizeof(s) - 1));
	}

	void HttpServerConnection::sendConnectResponse_Successed()
	{
		static char s[] = "HTTP/1.1 200 Connection established\r\n\r\n";
		sendResponseAndRestart(Memory::createStatic(s, sizeof(s) - 1));
	}

	void HttpServerConnection::sendConnectResponse_Failed()
	{
		static char s[] = "HTTP/1.1 500 Tunneling is not supported\r\n\r\n";
		sendResponseAndClose(Memory::createStatic(s, sizeof(s) - 1));
	}

	void HttpServerConnection::sendProxyResponse_Failed()
	{
		static char s[] = "HTTP/1.1 500 Internal Error\r\nContent-Length: 0\r\n\r\n";
		sendResponseAndClose(Memory::createStatic(s, sizeof(s) - 1));
	}


	SLIB_DEFINE_OBJECT(HttpServerConnectionProvider, Object)

	HttpServerConnectionProvider::HttpServerConnectionProvider()
	{
	}

	HttpServerConnectionProvider::~HttpServerConnectionProvider()
	{
	}

	Ref<HttpServer> HttpServerConnectionProvider::getServer()
	{
		return m_server;
	}

	void HttpServerConnectionProvider::setServer(const Ref<HttpServer>& server)
	{
		m_server = server;
	}


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(HttpServerRoute)

	HttpServerRoute::HttpServerRoute()
	{
	}

	HttpServerRoute* HttpServerRoute::createRoute(const String& path)
	{
		sl_reg indexStart = 0;
		if (path.startsWith('/')) {
			indexStart = 1;
		}
		if (indexStart == path.getLength()) {
			return this;
		}
		sl_reg indexSubpath = path.indexOf('/', indexStart);
		String name;
		String subPath;
		if (indexSubpath < 0) {
			name = path.substring(indexStart);
		} else {
			name = path.substring(indexStart, indexSubpath);
			subPath = path.substring(indexSubpath);
		}
		HttpServerRoute* route = sl_null;
		if (name.getLength() >= 2 && name.startsWith(':')) {
			name = name.substring(1);
			ListElements< Pair<String, HttpServerRoute> > list(parameterRoutes);
			for (sl_size i = 0; i < list.count; i++) {
				if (list[i].first == name) {
					route = &(list[i].second);
					break;
				}
			}
			if (!route) {
				parameterRoutes.add_NoLock(name, HttpServerRoute());
				Pair<String, HttpServerRoute>* p = parameterRoutes.getPointerAt(parameterRoutes.getCount() - 1);
				if (p && p->first == name) {
					route = &(p->second);
				}
			}
		} else if (name == "*") {
			if (defaultRoute.isNotNull()) {
				route = defaultRoute.get();
			} else {
				defaultRoute = Shared<HttpServerRoute>::create();
				route = defaultRoute.get();
			}
		} else if (name == "**") {
			if (ellipsisRoute.isNotNull()) {
				route = ellipsisRoute.get();
			} else {
				ellipsisRoute = Shared<HttpServerRoute>::create();
				route = ellipsisRoute.get();
			}
		} else {
			route = routes.getItemPointer(name);
			if (!route) {
				route = &(routes.emplace_NoLock(name).node->value);
			}
		}
		if (route) {
			return route->createRoute(subPath);
		}
		return sl_null;
	}

	HttpServerRoute* HttpServerRoute::getRoute(const String& path, HashMap<String, String>& parameters)
	{
		sl_reg indexStart = 0;
		if (path.startsWith('/')) {
			indexStart = 1;
		}
		if (indexStart == path.getLength()) {
			return this;
		}
		sl_reg indexSubpath = path.indexOf('/', indexStart);
		String name;
		String subPath;
		if (indexSubpath < 0) {
			name = path.substring(indexStart);
		} else {
			name = path.substring(indexStart, indexSubpath);
			subPath = path.substring(indexSubpath);
		}
		HttpServerRoute* route = routes.getItemPointer(name);
		if (route) {
			HashMap<String, String> subParams;
			route = route->getRoute(subPath, subParams);
			if (route) {
				if (subParams.isNotNull()) {
					parameters.putAll_NoLock(subParams);
				}
				return route;
			}
		}
		{
			ListElements< Pair<String, HttpServerRoute> > list(parameterRoutes);
			for (sl_size i = 0; i < list.count; i++) {
				route = &(list[i].second);
				HashMap<String, String> subParams;
				route = route->getRoute(subPath, subParams);
				if (route) {
					parameters.put_NoLock(list[i].first, Url::decodePercent(name));
					if (subParams.isNotNull()) {
						parameters.putAll_NoLock(subParams);
					}
					return route;
				}
			}
		}
		if (defaultRoute.isNotNull()) {
			HashMap<String, String> subParams;
			route = defaultRoute->getRoute(subPath, subParams);
			if (route) {
				if (subParams.isNotNull()) {
					parameters.putAll_NoLock(subParams);
				}
				return route;
			}
		}
		if (ellipsisRoute.isNotNull()) {
			for (;;) {
				HashMap<String, String> subParams;
				route = ellipsisRoute->getRoute(subPath, subParams);
				if (route) {
					if (subParams.isNotNull()) {
						parameters.putAll_NoLock(subParams);
					}
					return route;
				}
				indexSubpath = subPath.indexOf('/', 1);
				if (indexSubpath < 0) {
					return ellipsisRoute.get();
				}
				subPath = subPath.substring(indexSubpath);
			}
		}
		return sl_null;
	}

	void HttpServerRoute::add(const String& path, const HttpServerRoute& _route)
	{
		HttpServerRoute* route = createRoute(path);
		*route = _route;
	}

	void HttpServerRoute::add(const String& path, const Function<Variant(HttpServerContext*)>& onRequest)
	{
		HttpServerRoute* route = createRoute(path);
		route->onRequest = onRequest;
	}

	Variant HttpServerRoute::processRequest(const String& path, HttpServerContext* context)
	{
		HashMap<String, String> params;
		HttpServerRoute* route = getRoute(path, params);
		if (route) {
			if (route->onRequest.isNotNull()) {
				if (params.isNotNull()) {
					context->getParameters().addAll_NoLock(params);
				}
				return route->onRequest(context);
			}
		}
		return sl_false;
	}


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(HttpServerRouter)

	HttpServerRouter::HttpServerRouter()
	{
	}

	Variant HttpServerRouter::processRequest(const String& path, HttpServerContext* context)
	{
		if (routes.isNull()) {
			return sl_false;
		}
		HttpMethod method = context->getMethod();
		HttpServerRoute* route = routes.getItemPointer(method);
		if (route) {
			Variant result = route->processRequest(path, context);
			if (!(result.isFalse())) {
				return result;
			}
		}
		route = routes.getItemPointer(HttpMethod::Unknown);
		if (route) {
			Variant result = route->processRequest(path, context);
			if (!(result.isFalse())) {
				return result;
			}
		}
		return sl_false;
	}

	Variant HttpServerRouter::preProcessRequest(const String& path, HttpServerContext* context)
	{
		if (preRoutes.isNull()) {
			return sl_false;
		}
		HttpMethod method = context->getMethod();
		HttpServerRoute* route = preRoutes.getItemPointer(method);
		if (route) {
			Variant result = route->processRequest(path, context);
			if (!(result.isFalse())) {
				return result;
			}
		}
		route = preRoutes.getItemPointer(HttpMethod::Unknown);
		if (route) {
			Variant result = route->processRequest(path, context);
			if (!(result.isFalse())) {
				return result;
			}
		}
		return sl_false;
	}

	Variant HttpServerRouter::postProcessRequest(const String& path, HttpServerContext* context)
	{
		if (postRoutes.isNull()) {
			return sl_false;
		}
		HttpMethod method = context->getMethod();
		HttpServerRoute* route = postRoutes.getItemPointer(method);
		if (route) {
			Variant result = route->processRequest(path, context);
			if (!(result.isFalse())) {
				return result;
			}
		}
		route = postRoutes.getItemPointer(HttpMethod::Unknown);
		if (route) {
			Variant result = route->processRequest(path, context);
			if (!(result.isFalse())) {
				return result;
			}
		}
		return sl_false;
	}

	void HttpServerRouter::add(HttpMethod method, const String& path, const HttpServerRoute& _route)
	{
		HttpServerRoute* route = routes.getItemPointer(method);
		if (!route) {
			route = &(routes.emplace_NoLock(method).node->value);
		}
		route->add(path, _route);
	}

	void HttpServerRouter::add(HttpMethod method, const String& path, const Function<Variant(HttpServerContext*)>& onRequest)
	{
		HttpServerRoute* route = routes.getItemPointer(method);
		if (!route) {
			route = &(routes.emplace_NoLock(method).node->value);
		}
		route->add(path, onRequest);
	}

	void HttpServerRouter::before(HttpMethod method, const String& path, const HttpServerRoute& _route)
	{
		HttpServerRoute* route = preRoutes.getItemPointer(method);
		if (!route) {
			route = &(preRoutes.emplace_NoLock(method).node->value);
		}
		route->add(path, _route);
	}

	void HttpServerRouter::before(HttpMethod method, const String& path, const Function<Variant(HttpServerContext*)>& onRequest)
	{
		HttpServerRoute* route = preRoutes.getItemPointer(method);
		if (!route) {
			route = &(preRoutes.emplace_NoLock(method).node->value);
		}
		route->add(path, onRequest);
	}

	void HttpServerRouter::after(HttpMethod method, const String& path, const HttpServerRoute& _route)
	{
		HttpServerRoute* route = postRoutes.getItemPointer(method);
		if (!route) {
			route = &(postRoutes.emplace_NoLock(method).node->value);
		}
		route->add(path, _route);
	}

	void HttpServerRouter::after(HttpMethod method, const String& path, const Function<Variant(HttpServerContext*)>& onRequest)
	{
		HttpServerRoute* route = postRoutes.getItemPointer(method);
		if (!route) {
			route = &(postRoutes.emplace_NoLock(method).node->value);
		}
		route->add(path, onRequest);
	}

	void HttpServerRouter::add(const String& path, const HttpServerRouter& router)
	{
		for (auto& item : router.routes) {
			add(item.key, path, item.value);
		}
		for (auto& item : router.preRoutes) {
			before(item.key, path, item.value);
		}
		for (auto& item : router.postRoutes) {
			after(item.key, path, item.value);
		}
	}

	void HttpServerRouter::GET(const String& path, const HttpServerRoute& route)
	{
		add(HttpMethod::GET, path, route);
	}

	void HttpServerRouter::GET(const String& path, const Function<Variant(HttpServerContext*)>& onRequest)
	{
		add(HttpMethod::GET, path, onRequest);
	}

	void HttpServerRouter::POST(const String& path, const HttpServerRoute& route)
	{
		add(HttpMethod::POST, path, route);
	}

	void HttpServerRouter::POST(const String& path, const Function<Variant(HttpServerContext*)>& onRequest)
	{
		add(HttpMethod::POST, path, onRequest);
	}

	void HttpServerRouter::PUT(const String& path, const HttpServerRoute& route)
	{
		add(HttpMethod::PUT, path, route);
	}

	void HttpServerRouter::PUT(const String& path, const Function<Variant(HttpServerContext*)>& onRequest)
	{
		add(HttpMethod::PUT, path, onRequest);
	}

	void HttpServerRouter::DELETE(const String& path, const HttpServerRoute& route)
	{
		add(HttpMethod::DELETE, path, route);
	}

	void HttpServerRouter::DELETE(const String& path, const Function<Variant(HttpServerContext*)>& onRequest)
	{
		add(HttpMethod::DELETE, path, onRequest);
	}

	void HttpServerRouter::ALL(const String& path, const HttpServerRoute& route)
	{
		add(HttpMethod::Unknown, path, route);
	}

	void HttpServerRouter::ALL(const String& path, const Function<Variant(HttpServerContext*)>& onRequest)
	{
		add(HttpMethod::Unknown, path, onRequest);
	}


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(WebDavItemProperty)

	WebDavItemProperty::WebDavItemProperty()
	{
		flagCollection = sl_false;
		contentLength = 0;
	}

	sl_bool WebDavItemProperty::setFromFile(const StringParam& path)
	{
		FileAttributes attrs = File::getAttributes(path);
		if (attrs & FileAttributes::NotExist) {
			return sl_false;
		}
		if (attrs & FileAttributes::Directory) {
			flagCollection = sl_true;
		} else {
			flagCollection = sl_false;
			contentLength = File::getSize(path);
			contentType = ContentTypeHelper::getFromFilePath(path, ContentType::OctetStream);
		}
		creationTime = File::getCreatedTime(path);
		lastModifiedTime = File::getModifiedTime(path);
		return sl_true;
	}

	HashMap<String, WebDavItemProperty> WebDavItemProperty::getFiles(const StringParam& path)
	{
		HashMap<String, WebDavItemProperty> ret;
		for (auto& file : File::getFileInfos(path)) {
			WebDavItemProperty prop;
			if (file.value.attributes & FileAttributes::Directory) {
				prop.flagCollection = sl_true;
			} else {
				prop.contentLength = file.value.size;
				prop.contentType = ContentTypeHelper::getFromFilePath(file.key, ContentType::OctetStream);
			}
			prop.creationTime = file.value.createdAt;
			prop.lastModifiedTime = file.value.modifiedAt;
			ret.add_NoLock(file.key, Move(prop));
		}
		return ret;
	}


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(HttpServerParam)

	HttpServerParam::HttpServerParam()
	{
		port = 8080;

		maximumThreadCount = Cpu::getCoreCount();
		if (!maximumThreadCount) {
			maximumThreadCount = 1;
		}
		minimumThreadCount = maximumThreadCount / 2;
		flagProcessByThreads = sl_true;

		flagUseWebRoot = sl_false;
		flagUseAsset = sl_false;

		maxRequestHeadersSize = 0x10000; // 64KB
		maxRequestBodySize = 0x2000000; // 32MB

		flagAllowCrossOrigin = sl_false;

		flagUseCacheControl = sl_true;
		flagCacheControlNoCache = sl_false;
		cacheControlMaxAge = 600;

		flagSupportWebDAV = sl_false;

		connectionExpiringDuration = 43200000; // 12 hours

		flagLogDebug = sl_false;

		flagAutoStart = sl_true;
	}

	void HttpServerParam::setJson(const Json& conf)
	{
		port = (sl_uint16)(conf["port"].getUint32(port));
		{
			String s = conf["root"].getString();
			if (s.isNotNull()) {
				webRootPath = s;
				flagUseWebRoot = sl_true;
			}
		}
		{
			List<String> s;
			FromJson(conf["allowed_file_extensions"], s);
			if (s.isNotNull()) {
				allowedFileExtensions = s;
			}
		}
		{
			List<String> s;
			FromJson(conf["blocked_file_extensions"], s);
			if (s.isNotNull()) {
				blockedFileExtensions = s;
			}
		}

		Json cacheControl = conf["cache_control"];
		if (cacheControl.isNotNull()) {
			flagUseCacheControl = sl_true;
			flagCacheControlNoCache = cacheControl["no_cache"].getBoolean(flagCacheControlNoCache);
			cacheControlMaxAge = cacheControl["max_age"].getUint32(cacheControlMaxAge);
		}

		{
			sl_uint32 n;
			if (conf["max_request_body"].getString().parseUint32(10, &n)) {
				maxRequestBodySize = n * 1024 * 1024;
			}
		}
	}

	sl_bool HttpServerParam::parseJsonFile(const String& filePath)
	{
		Json::ParseParam param;
		param.flagLogError = sl_true;
		Json json = Json::parseTextFile(filePath);
		if (json.isNotNull()) {
			setJson(json);
			return sl_true;
		} else {
			return sl_false;
		}
	}


	SLIB_DEFINE_OBJECT(HttpServer, Object)

	HttpServer::HttpServer()
	{
		m_flagRunning = sl_false;
		m_flagReleased = sl_false;
	}

	HttpServer::~HttpServer()
	{
		release();
	}

	Ref<HttpServer> HttpServer::create(const HttpServerParam& param)
	{
		Ref<HttpServer> ret = new HttpServer;
		if (ret.isNotNull()) {
			if (ret->_init(param)) {
				return ret;
			}
		}
		return sl_null;
	}

	sl_bool HttpServer::_init(const HttpServerParam& param)
	{
		m_param = param;
		if (param.webRootPath.isEmpty()) {
			m_param.webRootPath = Application::getApplicationDirectory();
		} else {
			String path = File::concatPath(Application::getApplicationDirectory(), param.webRootPath);
			if (File::isDirectory(path)) {
				m_param.webRootPath = path;
			}
		}
		Ref<AsyncIoLoop> ioLoop = AsyncIoLoop::create(sl_false);
		if (ioLoop.isNull()) {
			return sl_false;
		}
		m_ioLoop = Move(ioLoop);
		if (param.port) {
			if (!(addHttpBinding(param.bindAddress, param.port))) {
				return sl_false;
			}
		}
		if (param.flagAutoStart) {
			if (!(start())) {
				return sl_false;
			}
		}
		return sl_true;
	}

	void HttpServer::_onTimerExpireConnections(Timer*)
	{
		ObjectLocker lock(&m_connections);
		sl_uint64 now = System::getTickCount64();
		auto node = m_connections.getFirstNode();
		while (node) {
			auto next = node->next;
			Ref<HttpServerConnection>& connection = node->value;
			if (_isConnectionExpiring(connection.get(), now)) {
				m_connections.removeAt(node);
			}
			node = next;
		}
	}

	sl_bool HttpServer::_isConnectionExpiring(HttpServerConnection* connection, sl_uint64 now)
	{
		if (connection->m_output->isWriting()) {
			return sl_false;
		}
		sl_uint64 tick = connection->m_timeLastRead;
		return !(now >= tick && now - tick < m_param.connectionExpiringDuration);
	}

	sl_bool HttpServer::start()
	{
		ObjectLocker lock(this);
		if (m_flagReleased) {
			return sl_false;
		}
		if (m_flagRunning) {
			return sl_true;
		}

		Ref<AsyncIoLoop> ioLoop = m_ioLoop;
		if (ioLoop.isNull()) {
			return sl_false;
		}
		Ref<DispatchLoop> dispatchLoop = DispatchLoop::create(sl_false);
		if (dispatchLoop.isNull()) {
			return sl_false;
		}
		if (!(m_param.maximumThreadCount)) {
			m_param.maximumThreadCount = 1;
		}
		if (m_param.minimumThreadCount >= m_param.maximumThreadCount) {
			m_param.minimumThreadCount = m_param.maximumThreadCount / 2;
		}
		Ref<ThreadPool> threadPool = ThreadPool::create(m_param.minimumThreadCount, m_param.maximumThreadCount);
		if (threadPool.isNull()) {
			return sl_false;
		}

		dispatchLoop->start();
		ioLoop->start();

		if (m_param.connectionExpiringDuration) {
			m_timerExpireConnections = Timer::startWithLoop(dispatchLoop, SLIB_FUNCTION_WEAKREF(this, _onTimerExpireConnections), m_param.connectionExpiringDuration);
		}

		m_dispatchLoop = Move(dispatchLoop);
		m_threadPool = Move(threadPool);

		m_flagRunning = sl_true;

		return sl_true;
	}

	void HttpServer::release()
	{
		ObjectLocker lock(this);

		if (m_flagReleased) {
			return;
		}

		m_flagReleased = sl_true;
		m_flagRunning = sl_false;

		Ref<ThreadPool> threadPool = m_threadPool;
		if (threadPool.isNotNull()) {
			threadPool->release();
			m_threadPool.setNull();
		}

		Ref<DispatchLoop> dispatchLoop = m_dispatchLoop;
		if (dispatchLoop.isNotNull()) {
			dispatchLoop->release();
			m_dispatchLoop.setNull();
		}

		Ref<AsyncIoLoop> ioLoop = m_ioLoop;
		if (ioLoop.isNotNull()) {
			ioLoop->release();
			m_ioLoop.setNull();
		}

		m_connections.removeAll();

		{
			ListLocker< Ref<HttpServerConnectionProvider> > cp(m_connectionProviders);
			for (sl_size i = 0; i < cp.count; i++) {
				cp[i]->release();
			}
		}
		m_connectionProviders.removeAll();

	}

	sl_bool HttpServer::isReleased()
	{
		return m_flagReleased;
	}

	sl_bool HttpServer::isRunning()
	{
		return m_flagRunning;
	}

	Ref<AsyncIoLoop> HttpServer::getAsyncIoLoop()
	{
		return m_ioLoop;
	}

	Ref<ThreadPool> HttpServer::getThreadPool()
	{
		return m_threadPool;
	}

	const HttpServerParam& HttpServer::getParam()
	{
		return m_param;
	}

	sl_bool HttpServer::preprocessRequest(HttpServerContext* context)
	{
		return sl_false;
	}

	void HttpServer::processRequest(HttpServerContext* context, HttpServerConnection* connection)
	{
		if (m_param.flagLogDebug) {
			Log(SERVER_TAG, "[%s] %s Method=%s Path=%s Query=%s Host=%s",
				String::fromPointerValue(connection),
				context->getRequestVersion(),
				context->getMethodText(),
				context->getPath(),
				context->getQuery(),
				context->getHost());
		}

		Variant result = handleRequest(context);
		if (result.isVariantPromise()) {
			Promise<Variant> promise = result.getVariantPromise();
			if (promise.isNotNull()) {
				Ref<HttpServerContext> refContext(context);
				Ref<HttpServerConnection> refConnection(connection);
				WeakRef<HttpServer> weakThis(this);
				promise.then([this, weakThis, refContext, refConnection](Variant& response) {
					Ref<HttpServer> refThis = weakThis;
					if (refThis.isNotNull()) {
						processRequest(refContext.get(), refConnection.get(), response);
					}
				});
				return;
			}
		}
		processRequest(context, connection, result);
	}

	void HttpServer::processRequest(HttpServerContext* context, HttpServerConnection* connection, const Variant& response)
	{
		if (response.isFalse()) {
			HttpMethod method = context->getMethod();
			if (method == HttpMethod::GET) {
				if (m_param.flagUseWebRoot || m_param.flagUseAsset) {
					if (processResource(context)) {
						context->setProcessed();
					}
				}
			} else if (method == HttpMethod::PROPFIND) {
				if (m_param.flagSupportWebDAV) {
					if (processWebDav_PROPFIND(context)) {
						context->setProcessed();
					}
				}
			} else if (method == HttpMethod::OPTIONS) {
				if (m_param.flagSupportWebDAV || m_param.flagAllowCrossOrigin) {
					if (m_param.flagSupportWebDAV) {
						// compliance-class = 1
						context->setResponseHeader(HttpHeader::DAV, "1");
					}
					context->setResponseCode(HttpStatus::OK);
					context->setProcessed();
				}
			}
		} else {
			do {
				if (response.isBoolean()) {
					context->setResponseContentTypeIfEmpty(ContentType::TextHtml_Utf8);
					break;
				} else if (response.isStringType()) {
					context->setResponseContentTypeIfEmpty(ContentType::TextHtml_Utf8);
					context->write(response.getString());
					break;
				} else if (response.isRef()) {
					if (response.isMemory()) {
						context->setResponseContentTypeIfEmpty(ContentType::OctetStream);
						context->write(response.getMemory());
						break;
					} else if (response.isObject() || response.isCollection()) {
						context->setResponseContentTypeIfEmpty(ContentType::Json);
						context->write(Json(response).toJsonString());
						break;
					} else {
						Ref<CRef> ref = response.getRef();
						if (IsInstanceOf<XmlDocument>(ref)) {
							context->setResponseContentTypeIfEmpty(ContentType::TextXml);
							context->write(((XmlDocument*)(ref.get()))->toString());
							break;
						}
					}
				}
				context->setResponseContentTypeIfEmpty(ContentType::TextHtml_Utf8);
				context->write(response.toString());
			} while (0);
			context->setProcessed();
		}
		handlePostRequest(context);
		connection->completeContext(context);
	}

	sl_bool HttpServer::processResource(HttpServerContext* context)
	{
		String path = Url::decodeUri(context->getPath());
		FilePathSegments segments;
		segments.parsePath(path);
		if (segments.parentLevel) {
			return sl_false;
		}
		return processResource(context, path);
	}

	sl_bool HttpServer::processResource(HttpServerContext* context, const String& path)
	{
		if (m_param.allowedFileExtensions.isNotEmpty() || m_param.blockedFileExtensions.isNotEmpty()) {
			String ext = File::getFileExtension(path).trim();
			if (m_param.blockedFileExtensions.isNotEmpty()) {
				if (m_param.blockedFileExtensions.contains(ext)) {
					return sl_false;
				}
			} else if (m_param.allowedFileExtensions.isNotEmpty()) {
				if (!(m_param.allowedFileExtensions.contains(ext))) {
					return sl_false;
				}
			}
		}
		if (m_param.flagUseWebRoot) {
			String pathFile = File::concatPath(m_param.webRootPath, path);
			if (processFile(context, pathFile)) {
				return sl_true;
			}
			if (path.endsWith('/')) {
				if (processFile(context, pathFile + "index.html")) {
					return sl_true;
				}
				if (processFile(context, pathFile + "index.htm")) {
					return sl_true;
				}
			}
		}
		if (m_param.flagUseAsset) {
			String pathAsset = File::concatPath(m_param.prefixAsset, path);
			if (processAsset(context, pathAsset)) {
				return sl_true;
			}
			if (path.endsWith('/')) {
				if (processAsset(context, pathAsset + "index.html")) {
					return sl_true;
				}
				if (processAsset(context, pathAsset + "index.htm")) {
					return sl_true;
				}
			}
		}
		return sl_false;
	}

	sl_bool HttpServer::processAsset(HttpServerContext* context, const String& path)
	{
		if (Assets::isBasedOnFileSystem()) {
			String filePath = Assets::getFilePath(path);
			return processFile(context, filePath);
		} else {
			Memory mem = Assets::readAllBytes(path);
			if (mem.isNotNull()) {
				context->setResponseContentTypeFromFilePath(path, ContentType::OctetStream);
				_processCacheControl(context);
				context->write(mem);
				return sl_true;
			}
		}
		return sl_false;
	}

	sl_bool HttpServer::processFile(HttpServerContext* context, const String& path)
	{
		if (File::exists(path) && !(File::isDirectory(path))) {

			sl_uint64 totalSize = File::getSize(path);

			context->setResponseContentTypeFromFilePath(path, ContentType::OctetStream);
			context->setResponseAcceptRanges(sl_true);

			_processCacheControl(context);

			Time lastModifiedTime = File::getModifiedTime(path);
			context->setResponseLastModified(lastModifiedTime);
			Time ifModifiedSince = context->getRequestIfModifiedSince();
			if (ifModifiedSince.isNotZero() && ifModifiedSince == lastModifiedTime) {
				context->setResponseCode(HttpStatus::NotModified);
				return sl_true;
			}

			String rangeHeader = context->getRequestRange();
			if (rangeHeader.isNotEmpty()) {
				sl_uint64 start;
				sl_uint64 len;
				if (processRangeRequest(context, totalSize, rangeHeader, start, len)) {
					Ref<AsyncStream> file = AsyncFile::openStream(path, FileMode::Read, m_ioLoop, m_threadPool);
					if (file.isNotNull()) {
						if (file->seek(start)) {
							return context->copyFrom(file.get(), len);
						}
					}
				} else {
					return sl_true;
				}
			} else {
				if (totalSize > 100000) {
					return context->copyFromFile(path, m_ioLoop, m_threadPool);
				} else {
					Memory mem = File::readAllBytes(path);
					if (mem.isNotNull()) {
						return context->write(mem);
					} else {
						return sl_true;
					}
				}
			}
		}
		return sl_false;
	}

	void HttpServer::_processCacheControl(HttpServerContext* context)
	{
		if (m_param.flagUseCacheControl) {
			HttpCacheControlResponse cc;
			if (m_param.flagCacheControlNoCache) {
				cc.no_cache = sl_true;
			} else {
				cc.max_age = m_param.cacheControlMaxAge;
			}
			context->setResponseCacheControl(cc);
		}
	}

	sl_bool HttpServer::processRangeRequest(HttpServerContext* context, sl_uint64 totalLength, const String& range, sl_uint64& outStart, sl_uint64& outLength)
	{
		if (range.getLength() < 2 || !(range.startsWith("bytes="))) {
			context->setResponseCode(HttpStatus::BadRequest);
			return sl_false;
		}
		sl_reg indexSplit = range.indexOf('-');
		if (indexSplit < 0) {
			context->setResponseCode(HttpStatus::BadRequest);
			return sl_false;
		}
		String s1 = range.substring(6, indexSplit);
		String s2 = range.substring(indexSplit+1);
		sl_uint64 n1 = 0;
		sl_uint64 n2 = 0;
		if (s1.isNotEmpty()) {
			if (!(s1.parseUint64(10, &n1))) {
				context->setResponseCode(HttpStatus::BadRequest);
				return sl_false;
			}
		}
		if (s2.isNotEmpty()) {
			if (!(s2.parseUint64(10, &n2))) {
				context->setResponseCode(HttpStatus::BadRequest);
				return sl_false;
			}
		}
		if (indexSplit == 0) {
			if (n2 == 0) {
				context->setResponseCode(HttpStatus::NoContent);
				return sl_false;
			}
			if (n2 > totalLength) {
				context->setResponseCode(HttpStatus::RequestRangeNotSatisfiable);
				context->setResponseContentRangeUnsatisfied(totalLength);
				return sl_false;
			}
			outStart = totalLength - n2;
			outLength = totalLength - 1;
		} else {
			if (n1 >= totalLength) {
				context->setResponseCode(HttpStatus::RequestRangeNotSatisfiable);
				context->setResponseContentRangeUnsatisfied(totalLength);
				return sl_false;
			}
			if (indexSplit == (sl_reg)(range.getLength()) - 1) {
				outLength = totalLength - n1;
			} else {
				if (n2 >= totalLength) {
					context->setResponseCode(HttpStatus::RequestRangeNotSatisfiable);
					context->setResponseContentRangeUnsatisfied(totalLength);
					return sl_false;
				}
				outLength = n2 - n1 + 1;
			}
			outStart = n1;
		}
		context->setResponseContentRange(outStart, outStart + outLength - 1, totalLength);
		context->setResponseCode(HttpStatus::PartialContent);
		return sl_true;
	}

	sl_bool HttpServer::processWebDav_PROPFIND(HttpServerContext* context)
	{
		String strDepth = context->getRequestHeader(HttpHeader::Depth);
		if (strDepth.getLength() == 1) {
			sl_char8 chDepth = (strDepth.getData())[0];
			if (chDepth == '0' || chDepth == '1') {
				String path = Url::decodeUri(context->getPath());
				WebDavItemProperty prop;
				if (getWebDavItem(context, path, prop)) {
					context->setResponseContentType(ContentType::TextXml);
					context->setResponseCode(HttpStatus::MultiStatus);
					context->write(R"(<?xml version="1.0" encoding="utf-8"?><D:multistatus xmlns:D="DAV:">)");
					processWebDav_PROPFIND_Response(context, path, sl_null, prop);
					if (chDepth == '1') {
						if (prop.flagCollection) {
							for (auto& item : getWebDavItems(context, path)) {
								processWebDav_PROPFIND_Response(context, path, item.key, item.value);
							}
						}
					}
					context->write("</D:multistatus>");
					return sl_true;
				}
			}
		}
		// Not support infinite depth
		return sl_false;
	}

	void HttpServer::processWebDav_PROPFIND_Response(HttpServerContext* context, const String& path, const String& name, const WebDavItemProperty& prop)
	{
		context->write("<D:response><D:href>");
		context->write(path);
		if (!(path.endsWith('/'))) {
			context->write("/");
		}
		if (name.isNotNull()) {
			context->write(Url::encodeUriComponent(name));
			if (prop.flagCollection) {
				context->write("/");
			}
		}
		context->write("</D:href><D:propstat><D:status>HTTP/1.1 200 OK</D:status><D:prop><D:displayname>");
		if (prop.displayName.isNotNull()) {
			context->write(prop.displayName);
		} else {
			if (name.isNotNull()) {
				context->write(name);
			} else {
				if (path == "/") {
					context->write("/");
				} else if (path.endsWith('/')) {
					context->write(File::getFileName(path.substring(0, path.getLength() - 1)));
				} else {
					context->write(File::getFileName(path));
				}
			}
		}
		context->write("</D:displayname>");
		if (prop.flagCollection) {
			context->write("<D:resourcetype><D:collection/></D:resourcetype><D:iscollection>1</D:iscollection>");
		} else {
			context->write("<D:iscollection>0</D:iscollection>");
		}
		context->write("<D:creationdate>");
		context->write(prop.creationTime.toISOString());
		context->write("</D:creationdate><D:getlastmodified>");
		context->write(prop.lastModifiedTime.toHttpDate());
		context->write("</D:getlastmodified><D:getcontentlength>");
		context->write(String::fromUint64(prop.contentLength));
		context->write("</D:getcontentlength><D:getcontenttype>");
		context->write(prop.contentType);
		context->write("</D:getcontenttype></D:prop></D:propstat></D:response>");
	}

	sl_bool HttpServer::getWebDavItem(HttpServerContext* context, const String& path, WebDavItemProperty& prop)
	{
		if (m_param.onGetWebDavItem.isNotNull()) {
			return m_param.onGetWebDavItem(context, path, prop);
		}
		if (m_param.flagUseWebRoot) {
			FilePathSegments segments;
			segments.parsePath(path);
			if (segments.parentLevel) {
				return sl_false;
			}
			return prop.setFromFile(File::concatPath(m_param.webRootPath, path));
		}
		return sl_false;
	}

	HashMap<String, WebDavItemProperty> HttpServer::getWebDavItems(HttpServerContext* context, const String& path)
	{
		if (m_param.onGetWebDavItems.isNotNull()) {
			return m_param.onGetWebDavItems(context, path);
		}
		if (m_param.flagUseWebRoot) {
			return WebDavItemProperty::getFiles(File::concatPath(m_param.webRootPath, path));
		}
		return sl_null;
	}

	Variant HttpServer::onRequest(HttpServerContext* context)
	{
		return sl_false;
	}

	Variant HttpServer::handleRequest(HttpServerContext* context)
	{
		if (m_param.onPreRequest.isNotNull()) {
			Variant result = m_param.onPreRequest(context);
			if (!(result.isFalse())) {
				return result;
			}
		}
		{
			Variant result = m_param.router.preProcessRequest(context->getPath(), context);
			if (!(result.isFalse())) {
				return result;
			}
		}
		{
			Variant result = m_param.router.processRequest(context->getPath(), context);
			if (!(result.isFalse())) {
				return result;
			}
		}
		if (m_param.onRequest.isNotNull()) {
			Variant result = m_param.onRequest(context);
			if (!(result.isFalse())) {
				return result;
			}
		}
		return onRequest(context);
	}

	void HttpServer::onPostRequest(HttpServerContext* context)
	{
	}

	void HttpServer::handlePostRequest(HttpServerContext* context)
	{
		if (m_param.flagAllowCrossOrigin) {
			SLIB_STATIC_STRING(s, "*")
			context->setResponseAccessControlAllowOrigin(s);
			context->setResponseAccessControlAllowHeaders(s);
			context->setResponseAccessControlAllowMethods(s);
		}

		m_param.router.postProcessRequest(context->getPath(), context);
		m_param.onPostRequest(context);
		onPostRequest(context);

		if (!(context->isProcessed())) {
			context->write(StringView("Not Found"));
			context->setResponseCode(HttpStatus::NotFound);
		}
		if (context->isKeepAlive() && !(context->containsResponseHeader(HttpHeader::KeepAlive))) {
			context->setResponseKeepAlive();
		}
		context->setResponseContentTypeIfEmpty(ContentType::TextHtml_Utf8);
		context->setResponseContentLengthHeader(context->getResponseContentLength());
	}

	Ref<HttpServerConnection> HttpServer::addConnection(AsyncStream* stream, const SocketAddress& remoteAddress, const SocketAddress& localAddress)
	{
		Ref<HttpServerConnection> connection = HttpServerConnection::create(this, stream);
		if (connection.isNotNull()) {
			if (m_param.flagLogDebug) {
				Log(SERVER_TAG, "[%s] Connection Created - Address: %s",
					String::fromPointerValue(connection.get()),
					remoteAddress.toString());
			}
			connection->setRemoteAddress(remoteAddress);
			connection->setLocalAddress(localAddress);
			m_connections.put(connection.get(), connection);
			connection->start();
		}
		return connection;
	}

	void HttpServer::closeConnection(HttpServerConnection* connection)
	{
		if (m_param.flagLogDebug) {
			Log(SERVER_TAG, "[%s] Connection Closed", String::fromPointerValue(connection));
		}
		m_connections.remove(connection);
	}

	void HttpServer::addConnectionProvider(const Ref<HttpServerConnectionProvider>& provider)
	{
		m_connectionProviders.add(provider);
	}

	void HttpServer::removeConnectionProvider(const Ref<HttpServerConnectionProvider>& provider)
	{
		m_connectionProviders.remove(provider);
	}

	namespace {
		class DefaultConnectionProvider : public HttpServerConnectionProvider
		{
		public:
			Ref<AsyncTcpServer> m_server;
			Ref<AsyncIoLoop> m_loop;

		public:
			DefaultConnectionProvider()
			{
			}

			~DefaultConnectionProvider()
			{
				release();
			}

		public:
			static Ref<HttpServerConnectionProvider> create(HttpServer* server, const SocketAddress& addressListen)
			{
				Ref<AsyncIoLoop> loop = server->getAsyncIoLoop();
				if (loop.isNotNull()) {
					Ref<DefaultConnectionProvider> ret = new DefaultConnectionProvider;
					if (ret.isNotNull()) {
						ret->m_loop = loop;
						ret->setServer(server);
						AsyncTcpServerParam sp;
						sp.bindAddress = addressListen;
						sp.onAccept = SLIB_FUNCTION_WEAKREF(ret, onAccept);
						sp.ioLoop = loop;
						Ref<AsyncTcpServer> server = AsyncTcpServer::create(sp);
						if (server.isNotNull()) {
							ret->m_server = server;
							return ret;
						}
					}
				}
				return sl_null;
			}

			void release() override
			{
				ObjectLocker lock(this);
				if (m_server.isNotNull()) {
					m_server->close();
				}
			}

			void onAccept(AsyncTcpServer* socketListen, Socket& socketAccept, const SocketAddress& address)
			{
				Ref<HttpServer> server = getServer();
				if (server.isNotNull()) {
					Ref<AsyncIoLoop> loop = m_loop;
					if (loop.isNull()) {
						return;
					}
					SocketAddress addrLocal;
					socketAccept.getLocalAddress(addrLocal);
					Ref<AsyncSocketStream> stream = AsyncSocketStream::create(Move(socketAccept), loop);
					if (stream.isNotNull()) {
						server->addConnection(stream.get(), address, addrLocal);
					}
				}
			}

		};
	}

	sl_bool HttpServer::addHttpBinding(const SocketAddress& addr)
	{
		Ref<HttpServerConnectionProvider> provider = DefaultConnectionProvider::create(this, addr);
		if (provider.isNotNull()) {
			addConnectionProvider(provider);
			return sl_true;
		}
		return sl_false;
	}

	sl_bool HttpServer::addHttpBinding(sl_uint16 port)
	{
		return addHttpBinding(SocketAddress(port));
	}

	sl_bool HttpServer::addHttpBinding(const IPAddress& addr, sl_uint16 port)
	{
		return addHttpBinding(SocketAddress(addr, port));
	}

}
