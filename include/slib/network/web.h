/*
 *   Copyright (c) 2008-2022 SLIBIO <https://github.com/SLIBIO>
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

#ifndef CHECKHEADER_SLIB_NETWORK_WEB
#define CHECKHEADER_SLIB_NETWORK_WEB

#include "http_server.h"

#include "../core/service.h"
#include "../core/function.h"
#include "../core/variant.h"

namespace slib
{

#define SWEB_HANDLER_PARAMS_LIST const slib::Ref<slib::HttpServerContext>& context, HttpMethod method, const slib::String& path

	typedef Function<Variant(SWEB_HANDLER_PARAMS_LIST)> WebHandler;

	class WebController : public Object
	{
		SLIB_DECLARE_OBJECT

	protected:
		WebController();

	public:
		static Ref<WebController> create();

	public:
		void registerHandler(HttpMethod method, const String& path, const WebHandler& handler);

		sl_bool processHttpRequest(HttpServerContext* context);

	protected:
		static String _getHandlerSignature(HttpMethod method, const String& path);

	protected:
		CMap<String, WebHandler> m_handlers;

		friend class WebModule;

	};

	class WebModule
	{
	public:
		WebModule(const String& path);

	public:
		void registerToController();

		void addHandler(HttpMethod method, const String& path, const WebHandler& handler);

	protected:
		String m_path;
		struct Handler
		{
			HttpMethod method;
			String path;
			WebHandler handler;
		};
		CList<Handler> m_handlers;

	};

	class SLIB_EXPORT WebService : public Service
	{
		SLIB_DECLARE_OBJECT

	public:
		WebService();

		~WebService();

	public:
		static Ref<WebService> getApp();

	public:
		HttpServerParam& getHttpParam();

		sl_uint16 getHttpPort();

		void setHttpPort(sl_uint16 port);

		void useAsset(const String& prefixForAssetPath);

		const Ref<WebController>& getController();

	protected:
		sl_bool onStartService() override;

		void onStopService() override;

		sl_bool onHttpRequest(HttpServerContext*);

	protected:
		Ref<HttpServer> m_http;
		HttpServerParam m_httpParam;
		Ref<WebController> m_controller;

	};

}

#define SWEB_DECALRE_MODULE(NAME) namespace SWEB_MODULE_##NAME { void registerModule(); }

#define SWEB_REGISTER_MODULE(NAME) SWEB_MODULE_##NAME::registerModule();

#define SWEB_BEGIN_MODULE(NAME, PATH) \
namespace SWEB_MODULE_##NAME { \
	SLIB_SAFE_STATIC_GETTER(slib::WebModule, getModule, PATH) \
	void registerModule() { getModule()->registerToController(); }

#define SWEB_END_MODULE }

#define SWEB_HANDLER(METHOD, PATH, NAME) \
	slib::Variant NAME(SWEB_HANDLER_PARAMS_LIST); \
	class WebHandlerRegisterer_##NAME { public: WebHandlerRegisterer_##NAME() { getModule()->addHandler(slib::HttpMethod::METHOD, PATH, &NAME); } } WebHandlerRegisterer_instance_##NAME; \
	slib::Variant NAME(SWEB_HANDLER_PARAMS_LIST)

#define SWEB_STRING_PARAM(NAME) slib::String NAME = context->getParameter(#NAME);
#define SWEB_INT_PARAM(NAME, ...) sl_int32 NAME = context->getParameter(#NAME).parseInt32(10, ##__VA_ARGS__);
#define SWEB_INT64_PARAM(NAME, ...) sl_int64 NAME = context->getParameter(#NAME).parseInt64(10, ##__VA_ARGS__);
#define SWEB_FLOAT_PARAM(NAME, ...) float NAME = context->getParameter(#NAME).parseFloat(##__VA_ARGS__);
#define SWEB_DOUBLE_PARAM(NAME, ...) double NAME = context->getParameter(#NAME).parseDouble(##__VA_ARGS__);

#endif
