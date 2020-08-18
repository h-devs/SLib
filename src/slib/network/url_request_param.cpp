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

#include "slib/network/url_request.h"

#include "slib/core/json.h"
#include "slib/core/safe_static.h"

namespace slib
{
	
	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(UrlRequestParam)

	UrlRequestParam::UrlRequestParam()
	{
		method = HttpMethod::GET;
		flagUseBackgroundSession = sl_false;
		flagSelfAlive = sl_true;
		flagStoreResponseContent = sl_true;
		flagSynchronous = sl_false;
		timeout = UrlRequest::getDefaultTimeout();
		flagAllowInsecureConnection = UrlRequest::isDefaultAllowInsecureConnection();
		dispatcher = UrlRequest::getDefaultDispatcher();
	}
	
	void UrlRequestParam::setContentType(const String& contentType)
	{
		requestHeaders.put_NoLock(HttpHeader::ContentType, contentType);
	}
	
	void UrlRequestParam::setRequestBodyAsMemory(const Memory &mem)
	{
		requestBody = mem;
	}
	
	void UrlRequestParam::setRequestBodyAsString(const String& str)
	{
		requestBody = str.toMemory();
	}
	
	void UrlRequestParam::setRequestBodyAsJson(const Json& json)
	{
		requestBody = json.toJsonString().toMemory();
	}
	
	void UrlRequestParam::setRequestBodyAsXml(const Ref<XmlDocument>& xml)
	{
		if (xml.isNotNull()) {
			requestBody = xml->toString().toMemory();
		} else {
			requestBody.setNull();
		}
	}
	
	void UrlRequestParam::setRequestBody(const Variant& varBody)
	{
		if (varBody.isNotNull()) {
			if (varBody.isObject()) {
				Ref<Referable> obj = varBody.getObject();
				if (obj.isNotNull()) {
					if (CMap<String, Variant>* map = CastInstance< CMap<String, Variant> >(obj.get())) {
						if (ContentTypeHelper::equalsContentTypeExceptParams(requestHeaders.getValue(HttpHeader::ContentType), ContentType::Json)) {
							requestBody = varBody.toJsonString().toMemory();
						} else {
							setFormData(Map<String, Variant>(map));
						}
					} else if (CHashMap<String, Variant>* hashMap = CastInstance< CHashMap<String, Variant> >(obj.get())) {
						if (ContentTypeHelper::equalsContentTypeExceptParams(requestHeaders.getValue(HttpHeader::ContentType), ContentType::Json)) {
							requestBody = varBody.toJsonString().toMemory();
						} else {
							setFormData(HashMap<String, Variant>(hashMap));
						}
					} else if (IsInstanceOf< CList<Variant> >(obj.get()) || IsInstanceOf< CList< HashMap<String, Json> > >(obj.get()) || IsInstanceOf< CList< Map<String, Json> > >(obj.get())) {
						requestBody = varBody.toJsonString().toMemory();
					} else if (XmlDocument* xml = CastInstance<XmlDocument>(obj.get())) {
						requestBody = xml->toString().toMemory();
					} else if (CMemory* mem = CastInstance<CMemory>(obj.get())) {
						requestBody = mem;
					} else {
						requestBody = varBody.getString().toMemory();
					}
				} else {
					requestBody.setNull();
				}
			} else {
				requestBody = varBody.getString().toMemory();
			}
		} else {
			requestBody.setNull();
		}
	}

	void UrlRequestParam::setRequestHeader(const String& header, const String& value)
	{
		requestHeaders.put_NoLock(header, value);
	}
	
	void UrlRequestParam::addRequestHeader(const String& header, const String& value)
	{
		requestHeaders.add_NoLock(header, value);
	}

	void UrlRequestParam::setMultipartFormData(const HashMap<String, Variant>& params)
	{
		for (;;) {
			char memBoundary[32];
			Math::randomMemory(memBoundary, 32);
			String boundary = String::makeHexString(memBoundary, 32);
			Memory data = HttpRequest::buildMultipartFormData(boundary, params);
			if (data.isNotNull()) {
				setContentType("multipart/form-data; boundary=" + boundary);
				setRequestBodyAsMemory(data);
				return;
			}
		}
	}
	
	void UrlRequestParam::setJsonData(const Json& json)
	{
		setContentType(ContentType::Json);
		setRequestBodyAsJson(json);
	}
	
	namespace priv
	{
		namespace url_request
		{
			
			sl_uint32 g_default_timeout = 60000;
			sl_bool g_default_allowInsecureConnection = sl_false;
			
			SLIB_GLOBAL_ZERO_INITIALIZED(AtomicRef<Dispatcher>, g_default_dispatcher)
			
		}
	}
	
	using namespace priv::url_request;
	
	sl_uint32 UrlRequest::getDefaultTimeout()
	{
		return g_default_timeout;
	}
	
	void UrlRequest::setDefaultTimeout(sl_uint32 ms)
	{
		g_default_timeout = ms;
	}
	
	sl_bool UrlRequest::isDefaultAllowInsecureConnection()
	{
		return g_default_allowInsecureConnection;
	}
	
	void UrlRequest::setDefaultAllowInsecureConnection(sl_bool flag)
	{
		g_default_allowInsecureConnection = flag;
	}
	
	Ref<Dispatcher> UrlRequest::getDefaultDispatcher()
	{
		if (SLIB_SAFE_STATIC_CHECK_FREED(g_default_dispatcher)) {
			return sl_null;
		}
		return g_default_dispatcher;
	}
	
	void UrlRequest::setDefaultDispatcher(const Ref<Dispatcher>& dispatcher)
	{
		if (SLIB_SAFE_STATIC_CHECK_FREED(g_default_dispatcher)) {
			return;
		}
		g_default_dispatcher = dispatcher;
	}
	
}
