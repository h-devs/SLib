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

#ifndef CHECKHEADER_SLIB_NETWORK_HTTP_SERVER
#define CHECKHEADER_SLIB_NETWORK_HTTP_SERVER

#include "http_common.h"
#include "http_io.h"
#include "socket_address.h"

#include "../core/property.h"
#include "../core/shared.h"
#include "../io/io.h"
#include "../crypto/tls.h"

namespace slib
{

	class HttpServer;
	class HttpServerConnection;

	class SLIB_EXPORT HttpServerContext : public Object, public HttpRequest, public HttpResponse, public HttpOutputBuffer
	{
		SLIB_DECLARE_OBJECT

	protected:
		HttpServerContext();

		~HttpServerContext();

	public:
		static Ref<HttpServerContext> create(const Ref<HttpServerConnection>& connection);

	public:
		Memory getRawRequestHeader() const;

		sl_uint64 getRequestContentLength() const;

		Memory getRequestBody() const;

		Json getRequestBodyAsJson() const;

		void applyRequestBodyAsFormUrlEncoded();

		sl_uint64 getResponseContentLength() const;

		Ref<HttpServer> getServer() const;

		Ref<HttpServerConnection> getConnection() const;

		Ref<AsyncStream> getIO() const;

		Ref<AsyncIoLoop> getAsyncIoLoop() const;

		Ref<Dispatcher> getDispatcher() const;

		void setDispatcher(const Ref<Dispatcher>& dispatcher);

		const SocketAddress& getLocalAddress();

		const SocketAddress& getRemoteAddress();

		sl_bool isProcessed() const;

		void setProcessed(sl_bool flag = sl_true);

		sl_bool isClosingConnection() const;

		void setClosingConnection(sl_bool flag = sl_true);

		sl_bool isKeepAlive() const;

		void setKeepAlive(sl_bool flag = sl_true);

	protected:
		HttpHeaderReader m_requestHeaderReader;
		AtomicMemory m_requestHeader;
		sl_uint64 m_requestContentLength;
		MemoryQueue m_requestBodyBuffer;
		AtomicMemory m_requestBody;
		AtomicRef<Dispatcher> m_dispatcher;

		sl_bool m_flagProcessed;
		sl_bool m_flagClosingConnection;
		sl_bool m_flagKeepAlive;

		sl_bool m_flagBeganProcessing;

	private:
		WeakRef<HttpServerConnection> m_connection;

		friend class HttpServerConnection;

	};

	class SLIB_EXPORT HttpServerConnection : public Object, public IClosable
	{
		SLIB_DECLARE_OBJECT

	protected:
		HttpServerConnection();

		~HttpServerConnection();

	public:
		static Ref<HttpServerConnection> create(HttpServer* server, AsyncStream* io);

	public:
		void close() override;

		void start();

		Ref<AsyncStream> getIO();

		Ref<HttpServer> getServer();

		Ref<HttpServerContext> getCurrentContext();

		void sendResponseAndRestart(const Memory& mem);

		void sendResponseAndClose(const Memory& mem);

		void sendResponseAndClose_BadRequest();

		void sendResponseAndClose_ServerError();

		void sendConnectResponse_Successed();

		void sendConnectResponse_Failed();

		void sendProxyResponse_Failed();

	public:
		SLIB_PROPERTY(SocketAddress, LocalAddress)
		SLIB_PROPERTY(SocketAddress, RemoteAddress)
		SLIB_PROPERTY(AtomicRef<CRef>, ProxyObject)
		SLIB_PROPERTY(AtomicRef<CRef>, UserObject)

	protected:
		WeakRef<HttpServer> m_server;
		Ref<AsyncStream> m_io;
		Ref<AsyncOutput> m_output;

		AtomicRef<HttpServerContext> m_contextCurrent;

		sl_bool m_flagFreed;
		sl_bool m_flagClosed;
		Memory m_bufRead;
		sl_bool m_flagReading;
		sl_bool m_flagKeepAlive;
		List<char> m_bufReadUnprocessed;
		sl_uint64 m_timeLastRead;

	protected:
		void _free();

		void _read(AsyncStreamResult* result);

		void _processInput(AsyncStreamResult* result);

		void _processContext(const Ref<HttpServerContext>& context);

	public:
		void completeContext(HttpServerContext* context);

	protected:
		void onReadStream(AsyncStreamResult& result);

		void onAsyncOutputEnd(AsyncOutput* output, sl_bool flagError);

		friend class HttpServerContext;
		friend class HttpServer;

	};


	class SLIB_EXPORT HttpServerConnectionProvider : public Object
	{
		SLIB_DECLARE_OBJECT

	public:
		HttpServerConnectionProvider();

		~HttpServerConnectionProvider();

	public:
		virtual void release() = 0;

	public:
		Ref<HttpServer> getServer();

		void setServer(const Ref<HttpServer>& server);

	private:
		WeakRef<HttpServer> m_server;

	};

	class SLIB_EXPORT HttpServerRoute
	{
	public:
		Function<Variant(HttpServerContext*)> onRequest;
		HashMap<String, HttpServerRoute> routes;
		Shared<HttpServerRoute> defaultRoute;
		Shared<HttpServerRoute> ellipsisRoute;
		List< Pair<String, HttpServerRoute> > parameterRoutes;

	public:
		HttpServerRoute();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(HttpServerRoute)

	public:
		HttpServerRoute* createRoute(const String& path);

		HttpServerRoute* getRoute(const String& path, HashMap<String, String>& parameters);

		void add(const String& path, const HttpServerRoute& route);

		void add(const String& path, const Function<Variant(HttpServerContext*)>& onRequest);

		Variant processRequest(const String& path, HttpServerContext* context);

	};

	class SLIB_EXPORT HttpServerRouter
	{
	public:
		HashMap<HttpMethod, HttpServerRoute> routes;
		HashMap<HttpMethod, HttpServerRoute> preRoutes;
		HashMap<HttpMethod, HttpServerRoute> postRoutes;

	public:
		HttpServerRouter();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(HttpServerRouter)

	public:
		Variant processRequest(const String& path, HttpServerContext* context);

		Variant preProcessRequest(const String& path, HttpServerContext* context);

		Variant postProcessRequest(const String& path, HttpServerContext* context);

		void add(HttpMethod method, const String& path, const HttpServerRoute& route);

		void add(HttpMethod method, const String& path, const Function<Variant(HttpServerContext*)>& onRequest);

		void before(HttpMethod method, const String& path, const HttpServerRoute& route);

		void before(HttpMethod method, const String& path, const Function<Variant(HttpServerContext*)>& onRequest);

		void after(HttpMethod method, const String& path, const HttpServerRoute& route);

		void after(HttpMethod method, const String& path, const Function<Variant(HttpServerContext*)>& onRequest);

		void add(const String& path, const HttpServerRouter& router);

		void GET(const String& path, const HttpServerRoute& route);

		void GET(const String& path, const Function<Variant(HttpServerContext*)>& onRequest);

		void POST(const String& path, const HttpServerRoute& route);

		void POST(const String& path, const Function<Variant(HttpServerContext*)>& onRequest);

		void PUT(const String& path, const HttpServerRoute& route);

		void PUT(const String& path, const Function<Variant(HttpServerContext*)>& onRequest);

		void DELETE(const String& path, const HttpServerRoute& route);

		void DELETE(const String& path, const Function<Variant(HttpServerContext*)>& onRequest);

		void ALL(const String& path, const HttpServerRoute& route);

		void ALL(const String& path, const Function<Variant(HttpServerContext*)>& onRequest);

	};

	class SLIB_EXPORT WebDavItemProperty
	{
	public:
		String displayName;
		sl_bool flagCollection;
		Time creationTime;
		Time lastModifiedTime;
		sl_uint64 contentLength;
		String contentType;

	public:
		WebDavItemProperty();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(WebDavItemProperty)

	public:
		sl_bool setFromFile(const StringParam& path);

		static HashMap<String, WebDavItemProperty> getFiles(const StringParam& path);

	};

	class SLIB_EXPORT HttpServerParam
	{
	public:
		IPAddress bindAddress;
		sl_uint16 port;

		Ref<Dispatcher> dispatcher; // usually ThreadPool

		sl_bool flagUseWebRoot;
		String webRootPath;

		sl_bool flagUseAsset;
		String prefixAsset;

		sl_uint64 maxRequestHeadersSize;
		sl_uint64 maxRequestBodySize;

		sl_bool flagAllowCrossOrigin;

		List<String> allowedFileExtensions;
		List<String> blockedFileExtensions;

		sl_bool flagUseCacheControl;
		sl_bool flagCacheControlNoCache;
		sl_uint32 cacheControlMaxAge;

		sl_bool flagSupportWebDAV;

		sl_uint32 connectionExpiringDuration;

		sl_bool flagLogDebug;

		HttpServerRouter router;

		Function<Variant(HttpServerContext*)> onRequest;
		Function<Variant(HttpServerContext*)> onPreRequest;
		Function<void(HttpServerContext*)> onPostRequest;

		// WebDav
		Function<sl_bool(HttpServerContext*, const String& path, WebDavItemProperty& prop)> onGetWebDavItem;
		Function<HashMap<String, WebDavItemProperty>(HttpServerContext*, const String& path)> onGetWebDavItems;

		sl_bool flagAutoStart;

	public:
		HttpServerParam();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(HttpServerParam)

	public:
		void setJson(const Json& json);

		sl_bool parseJsonFile(const String& filePath);

	};

	class SLIB_EXPORT HttpServer : public Object
	{
		SLIB_DECLARE_OBJECT

	protected:
		HttpServer();

		~HttpServer();

	public:
		static Ref<HttpServer> create(const HttpServerParam& param);

	public:
		sl_bool start();

		void release();

		sl_bool isReleased();

		sl_bool isRunning();

		Ref<AsyncIoLoop> getAsyncIoLoop();

		const HttpServerParam& getParam();

	public:
		// called before processing body, returns true if the server is trying to process the connection itself.
		virtual sl_bool preprocessRequest(HttpServerContext* context);

		// called after inputing body
		void processRequest(HttpServerContext* context, HttpServerConnection* connection);

		void processRequest(HttpServerContext* context, HttpServerConnection* connection, const Variant& response);

		sl_bool processResource(HttpServerContext* context);

		sl_bool processResource(HttpServerContext* context, const String& path);

		virtual sl_bool processAsset(HttpServerContext* context, const String& path);

		sl_bool processFile(HttpServerContext* context, const String& path);

		sl_bool processRangeRequest(HttpServerContext* context, sl_uint64 totalLength, const String& range, sl_uint64& outStart, sl_uint64& outLength);

		sl_bool processWebDav_PROPFIND(HttpServerContext* context);

		void processWebDav_PROPFIND_Response(HttpServerContext* context, const String& path, const String& name, const WebDavItemProperty& prop);

		virtual sl_bool getWebDavItem(HttpServerContext* context, const String& path, WebDavItemProperty& prop);

		virtual HashMap<String, WebDavItemProperty> getWebDavItems(HttpServerContext* context, const String& path);


		virtual Ref<HttpServerConnection> addConnection(AsyncStream* stream, const SocketAddress& remoteAddress, const SocketAddress& localAddress);

		virtual void closeConnection(HttpServerConnection* connection);

	protected:
		virtual Variant onRequest(HttpServerContext* context);

		Variant handleRequest(HttpServerContext* context);

		virtual void onPostRequest(HttpServerContext* context);

		void handlePostRequest(HttpServerContext* context);

	public:
		void addConnectionProvider(const Ref<HttpServerConnectionProvider>& provider);

		void removeConnectionProvider(const Ref<HttpServerConnectionProvider>& provider);


		sl_bool addHttpBinding(const SocketAddress& addr);

		sl_bool addHttpBinding(sl_uint16 port = 80);

		sl_bool addHttpBinding(const IPAddress& addr, sl_uint16 port = 80);

		sl_bool addHttpsBinding(const TlsAcceptStreamParam& param, const SocketAddress& addr);

		sl_bool addHttpsBinding(const TlsAcceptStreamParam& param, sl_uint16 port = 443);

		sl_bool addHttpsBinding(const TlsAcceptStreamParam& param, const IPAddress& addr, sl_uint16 port = 443);


		static Variant fileResponse(const String& path);

	protected:
		sl_bool _init(const HttpServerParam& param);

		void _processCacheControl(HttpServerContext* context);

		void _onTimerExpireConnections(Timer*);

		sl_bool _isConnectionExpiring(HttpServerConnection* connection, sl_uint64 currentTick);

	protected:
		AtomicRef<AsyncIoLoop> m_ioLoop;
		AtomicRef<DispatchLoop> m_dispatchLoop;
		sl_bool m_flagReleased;
		sl_bool m_flagRunning;

		CHashMap< HttpServerConnection*, Ref<HttpServerConnection> > m_connections;
		Ref<Timer> m_timerExpireConnections;

		CList< Ref<HttpServerConnectionProvider> > m_connectionProviders;

		HttpServerParam m_param;

	};

}

#endif

