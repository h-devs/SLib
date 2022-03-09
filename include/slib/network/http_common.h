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

#ifndef CHECKHEADER_SLIB_NETWORK_HTTP_COMMON
#define CHECKHEADER_SLIB_NETWORK_HTTP_COMMON

/****************************************************************************
	
				Hypertext Transfer Protocol -- HTTP/1.1

	http://ietf.org/rfc/rfc2616.txt

	and replaced by

	http://tools.ietf.org/html/rfc7230 (Message Syntax and Routing)
	http://tools.ietf.org/html/rfc7231 (Semantics and Content)
	http://tools.ietf.org/html/rfc7232 (Conditional Requests)
	http://tools.ietf.org/html/rfc7233 (Range Requests)
	http://tools.ietf.org/html/rfc7234 (Caching)
	http://tools.ietf.org/html/rfc7235 (Authentication)


				Hypertext Transfer Protocol Version 2 (HTTP/2)

	https://tools.ietf.org/html/rfc7540 (HTTP/2)
	https://tools.ietf.org/html/rfc7541 (HPACK: Header Compression for HTTP/2)

*****************************************************************************/

#include "url.h"

#include "../core/time.h"
#include "../core/content_type.h"
#include "../core/hash_map.h"
#include "../core/string_buffer.h"
#include "../core/string_cast.h"
#include "../core/variant_def.h"

namespace slib
{

	class MemoryBuffer;
	
	typedef HashMap< String, String, HashIgnoreCase<String>, CompareIgnoreCase<String> > HttpHeaderMap;
	typedef HashMap< String, String, HashIgnoreCase<String>, CompareIgnoreCase<String> > HttpHeaderValueMap;

	enum class HttpStatus
	{
		Unknown = 0,
		
		// Informational
		Continue = 100,
		SwitchingProtocols = 101,
		
		// Success
		OK = 200,
		Created = 201,
		Accepted = 202,
		NonAuthInfo = 203,
		NoContent = 204,
		ResetContent = 205,
		PartialContent = 206,
		MultiStatus = 207, // WebDAV

		// Redirection
		MultipleChoices = 300,
		MovedPermanently = 301,
		Found = 302,
		SeeOther = 303,
		NotModified = 304,
		UseProxy = 305,
		SwitchProxy = 306,
		TemporaryRedirect = 307,
		PermanentRedirect = 308,
		
		// Client Error
		BadRequest = 400,
		Unauthorized = 401,
		PaymentRequired = 402,
		Forbidden = 403,
		NotFound = 404,
		MethodNotAllowed = 405,
		NotAcceptable = 406,
		ProxyAuthenticationRequired = 407,
		RequestTimeout = 408,
		Conflict = 409,
		Gone = 410,
		LengthRequired = 411,
		PreconditionFailed = 412,
		RequestEntityTooLarge = 413,
		RequestUriTooLarge = 414,
		UnsupportedMediaType = 415,
		RequestRangeNotSatisfiable = 416,
		ExpectationFailed = 417,
		
		// Server Error
		InternalServerError = 500,
		NotImplemented = 501,
		BadGateway = 502,
		ServiceUnavailable = 503,
		GatewayTimeout = 504,
		HttpVersionNotSupported = 505
		
	};
	
	class SLIB_EXPORT HttpStatusHelper
	{
	public:
		static String toString(HttpStatus status);
		
	};
	
	
#ifdef DELETE
#undef DELETE
#endif
#ifdef TRACE
#undef TRACE
#endif

	enum class HttpMethod
	{
		Unknown = 0,
		GET,
		HEAD,
		POST,
		PUT,
		DELETE,
		CONNECT,
		OPTIONS,
		TRACE,
		PATCH, // https://tools.ietf.org/html/rfc5789
		PROPFIND // WebDAV: https://tools.ietf.org/html/rfc4918
	};
	
	class SLIB_EXPORT HttpMethodHelper
	{
	public:
		static String toString(HttpMethod method);
		
		static HttpMethod fromString(const String& method);
		
	};
	
	class SLIB_EXPORT HttpHeader
	{
	public:
		// General Headers
		static const String& Connection;
		static const String& KeepAlive;
		static const String& CacheControl;
		static const String& ContentDisposition;
		static const String& Authorization;

		// Entity Headers
		static const String& ContentLength;
		static const String& ContentType;
		static const String& ContentEncoding;

		// Request Headers
		static const String& Host;
		static const String& AcceptEncoding;
		static const String& Origin;
		static const String& Cookie;
		static const String& Range;
		static const String& IfModifiedSince;
		static const String& Depth;

		// Response Headers
		static const String& TransferEncoding;
		static const String& AccessControlAllowOrigin;
		static const String& AccessControlAllowHeaders;
		static const String& AccessControlAllowMethods;
		static const String& AccessControlExposeHeaders;
		static const String& AccessControlRequestMethod;
		static const String& AccessControlRequestHeaders;
		static const String& SetCookie;
		static const String& AcceptRanges;
		static const String& ContentRange;
		static const String& LastModified;
		static const String& Location;
		static const String& DAV; // WebDAV

	};

	class SLIB_EXPORT HttpHeaderHelper
	{
	public:
		
		/*
		 Returns
		 <0: error
		 =0: incomplete packet
		 >0: size of the headers (ending with [CR][LF][CR][LF])
		 not thread-safe
		 */
		static sl_reg parseHeaders(HttpHeaderMap& outMap, const void* headers, sl_size size);
		
		// not thread-safe
		static void splitValue(const String& value, List<String>* values, HttpHeaderValueMap* map, HashMap<String, String>* mapCaseSensitive, sl_char8 delimiter=',');
		
		static List<String> splitValueToList(const String& value, sl_char8 delimiter=',');
		
		static HttpHeaderValueMap splitValueToMap(const String& value, sl_char8 delimiter=',');
		
		// value con't contain quots
		static String makeSafeValue(const String& value, sl_char8 delimiter=',');

		static String mergeValues(const List<String>& list, sl_char8 delimiter=',');

		template <class MAP>
		static String mergeValueMap(const MAP& map, sl_char8 delimiter=',')
		{
			MutexLocker lock(map.getLocker());
			StringBuffer sb;
			for (auto& item: map) {
				if (item.key.isNotEmpty()) {
					if (sb.getLength() > 0) {
						sb.addStatic(&delimiter, 1);
						sb.addStatic(" ");
					}
					String key = Cast<typename MAP::KEY_TYPE, String>()(item.key);
					String value = Cast<typename MAP::VALUE_TYPE, String>()(item.value);
					if (value.isNull()) {
						sb.add(makeSafeValue(key));
					} else {
						sb.add(makeSafeValue(key));
						sb.addStatic("=");
						sb.add(makeSafeValue(value));
					}
				}
			}
			return sb.merge();
		}

	};
	
	
	struct SLIB_EXPORT HttpCacheControlRequest
	{
		Nullable<sl_int32> max_age;
		Nullable<sl_int32> max_stale;
		Nullable<sl_int32> min_fresh;
		sl_bool no_cache = sl_false;
		sl_bool no_store = sl_false;
		sl_bool no_transform = sl_false;
		sl_bool only_if_cached = sl_false;
	};
	
	struct SLIB_EXPORT HttpCacheControlResponse
	{
		sl_bool must_revalidate = sl_false;
		sl_bool no_cache = sl_false;
		sl_bool no_store = sl_false;
		sl_bool no_transform = sl_false;
		sl_bool public_ = sl_false;
		sl_bool private_ = sl_false;
		sl_bool proxy_revalidate = sl_false;
		Nullable<sl_int32> max_age;
		Nullable<sl_int32> s_maxage;

		sl_bool immutable = sl_false;
		Nullable<sl_int32> stale_while_revalidate;
		Nullable<sl_int32> stale_if_error;
	};
	
	class SLIB_EXPORT HttpCookie
	{
	public:
		String name;
		String value;
		
		Time expires;
		Nullable<sl_int32> max_age;
		String domain;
		String path;
		sl_bool secure = sl_false;
		sl_bool http_only = sl_false;
		String same_site;
		
	public:
		HttpCookie();
		
		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(HttpCookie)
		
	public:
		String toHeaderValue() const;
		
		void parseHeaderValue(const String& value);
		
	};
	
	// not thread-safe
	class SLIB_EXPORT HttpUploadFile : public Referable
	{
		SLIB_DECLARE_OBJECT
		
	public:
		HttpUploadFile();
		
		HttpUploadFile(const String& fileName, const HttpHeaderMap& headers, void* data, sl_size size, const Ref<Referable>& ref);
		
		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(HttpUploadFile)
		
	public:
		String getFileName();
		
		void setFileName(const String& fileName);
		
		const HttpHeaderMap& getHeaders();
		
		String getHeader(const String& name);
		
		void setHeader(const String& name, const String& value);
		
		String getContentType();
		
		void setContentType(const String& contentType);
		
		void* getData();
		
		sl_size getSize();
		
		Memory getDataMemory();

		void setData(const void* data, sl_size size);

		void setData(const Memory& data);
		
		sl_bool saveToFile(const String& path);
		
	public:
		String m_fileName;
		HttpHeaderMap m_headers;
		void* m_data;
		sl_size m_size;
		Ref<Referable> m_ref;
		
	};
	
	
	class SLIB_EXPORT HttpRequest
	{
	public:
		HttpRequest();
		
		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(HttpRequest)
		
	public:
		HttpMethod getMethod() const;
		
		const String& getMethodText() const;
		
		const String& getMethodTextUppercase() const;
		
		void setMethod(HttpMethod method);
		
		void setMethod(const String& method);
		
		const String& getPath() const;
		
		void setPath(const String& path);
		
		const String& getQuery() const;
		
		void setQuery(const String& query);
		
		const String& getRequestVersion() const;
		
		void setRequestVersion(const String& version);
		
		
		const HttpHeaderMap& getRequestHeaders() const;
		
		String getRequestHeader(const String& name) const;
		
		void setRequestHeader(const String& name, const String& value);
		
		void addRequestHeader(const String& name, const String& value);
		
		sl_bool containsRequestHeader(const String& name) const;
		
		void removeRequestHeader(const String& name);
		
		List<String> getRequestHeaderValues(const String& name) const;
		
		void setRequestHeaderValues(const String& name, const List<String>& list);
		
		void addRequestHeaderValues(const String& name, const List<String>& list);
		
		HttpHeaderValueMap getRequestHeaderValueMap(const String& name) const;
		
		void setRequestHeaderValueMap(const String& name, const HttpHeaderValueMap& map);
		
		void addRequestHeaderValueMap(const String& name, const HttpHeaderValueMap& map);
		
		void clearRequestHeaders();
		
		
		sl_uint64 getRequestContentLengthHeader() const;
		
		void setRequestContentLengthHeader(sl_uint64 size);
		
		String getRequestContentType() const;
		
		String getRequestContentTypeNoParams() const;
		
		void setRequestContentType(const String& type);
				
		sl_bool isRequestMultipartFormData() const;
		
		String getRequestMultipartFormDataBoundary() const;
		
		String getRequestContentEncoding() const;
		
		void setRequestContentEncoding(const String& type);
		
		String getRequestTransferEncoding() const;
		
		void setRequestTransferEncoding(const String& type);
		
		sl_bool isChunkedRequest() const;
		
		String getHost() const;
		
		void setHost(const String& type);
		
		sl_bool isRequestKeepAlive() const;
		
		void setRequestKeepAlive();
		
		String getRequestRange() const;
		
		void setRequestRange(const String& range);
		
		void setRequestRange(sl_uint64 start, sl_uint64 last);
		
		void setRequestRangeFrom(sl_uint64 start);
		
		void setRequestRangeSuffix(sl_uint64 length);
		
		String getRequestOrigin() const;
		
		void setRequestOrigin(const String& origin);
		
		Time getRequestIfModifiedSince() const;
		
		void setRequestIfModifiedSince(const Time& time);
		
		HttpCacheControlRequest getRequestCacheControl() const;
		
		void setRequestCacheControl(const HttpCacheControlRequest&);
		
		HashMap<String, String> getRequestCookies() const;
		
		template <class MAP>
		void setRequestCookies(const MAP& cookies)
		{
			String value = HttpHeaderHelper::mergeValueMap(cookies, ';');
			setRequestHeader(HttpHeader::Cookie, value);
		}
		
		String getRequestCookie(const String& cookie) const;


		const HashMap<String, String>& getParameters() const;
		
		HashMap<String, String>& getParameters();

		String getParameter(const String& name) const;
		
		List<String> getParameterValues(const String& name) const;
		
		sl_bool containsParameter(const String& name) const;
		
		const HashMap<String, String>& getQueryParameters() const;
		
		String getQueryParameter(const String& name) const;
		
		List<String> getQueryParameterValues(const String& name) const;
		
		sl_bool containsQueryParameter(const String& name) const;
		
		const HashMap<String, String>& getPostParameters() const;
		
		String getPostParameter(const String& name) const;
		
		List<String> getPostParameterValues(const String& name) const;
		
		sl_bool containsPostParameter(const String& name) const;
		
		void applyFormUrlEncoded(const void* data, sl_size size);
		
		void applyFormUrlEncoded(const String& str);
		
		void applyQueryToParameters();
		
		static HashMap<String, String> parseQueryParameters(const void* data, sl_size size);
		
		static HashMap<String, String> parseQueryParameters(const String& str);
		
		static HashMap<String, String> parseFormUrlEncoded(const void* data, sl_size size);
		
		static HashMap<String, String> parseFormUrlEncoded(const String& str);
		
		const HashMap< String, Ref<HttpUploadFile> >& getUploadFiles() const;
		
		Ref<HttpUploadFile> getUploadFile(const String& name) const;
		
		List< Ref<HttpUploadFile> > getUploadFiles(const String& name) const;
		
		sl_bool containsUploadFile(const String& name) const;
		
		void applyMultipartFormData(const String& boundary, const Memory& body);
		
		Memory makeRequestPacket() const;
		
		/*
		 Returns
		 <0: error
		 =0: incomplete packet
		 >0: size of the HTTP header section (ending with [CR][LF][CR][LF])
		 */
		sl_reg parseRequestPacket(const void* packet, sl_size size);
		
		template <class MAP>
		static String buildQuery(const MAP& params)
		{
			StringBuffer sb;
			sl_bool flagFirst = sl_true;
			for (auto& pair : params) {
				if (!flagFirst) {
					sb.addStatic("&");
				}
				flagFirst = sl_false;
				sb.add(Cast<typename MAP::KEY_TYPE, String>()(pair.key));
				sb.addStatic("=");
				sb.add(Url::encodePercent(Cast<typename MAP::VALUE_TYPE, String>()(pair.value)));
			}
			return sb.merge();
		}
		
		template <class MAP>
		static String buildFormUrlEncoded(const MAP& params)
		{
			StringBuffer sb;
			sl_bool flagFirst = sl_true;
			for (auto& pair : params) {
				if (!flagFirst) {
					sb.addStatic("&");
				}
				flagFirst = sl_false;
				sb.add(Cast<typename MAP::KEY_TYPE, String>()(pair.key));
				sb.addStatic("=");
				sb.add(Url::encodeForm(Cast<typename MAP::VALUE_TYPE, String>()(pair.value)));
			}
			return sb.merge();
		}
		
		static sl_bool buildMultipartFormData(MemoryBuffer& output, const String& boundary, VariantMap& parameters);
		
		static Memory buildMultipartFormData(const String& boundary, const VariantMap& parameters);
		
	protected:
		HttpMethod m_method;
		String m_methodText;
		String m_methodTextUpper;
		String m_path;
		String m_query;
		String m_requestVersion;
		
		HttpHeaderMap m_requestHeaders;
		HashMap<String, String> m_parameters;
		HashMap<String, String> m_queryParameters;
		HashMap<String, String> m_postParameters;
		HashMap< String, Ref<HttpUploadFile> > m_uploadFiles;
		
	};
	
	class SLIB_EXPORT HttpResponse
	{
	public:
		HttpResponse();
		
		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(HttpResponse)
		
	public:
		HttpStatus getResponseCode() const;
		
		void setResponseCode(HttpStatus code);
		
		String getResponseMessage() const;
		
		void setResponseMessage(const String& message);
		
		String getResponseVersion() const;
		
		void setResponseVersion(const String& version);
		
		
		const HttpHeaderMap& getResponseHeaders() const;
		
		String getResponseHeader(const String& name) const;
		
		void setResponseHeader(const String& name, const String& value);
		
		void addResponseHeader(const String& name, const String& value);
		
		sl_bool containsResponseHeader(const String& name) const;
		
		void removeResponseHeader(const String& name);
		
		List<String> getResponseHeaderValues(const String& name) const;
		
		void setResponseHeaderValues(const String& name, const List<String>& list);
		
		void addResponseHeaderValues(const String& name, const List<String>& list);
		
		HttpHeaderValueMap getResponseHeaderValueMap(const String& name) const;
		
		void setResponseHeaderValueMap(const String& name, const HttpHeaderValueMap& map);
		
		void addResponseHeaderValueMap(const String& name, const HttpHeaderValueMap& map);
		
		void clearResponseHeaders();
		
		
		sl_bool isResponseKeepAlive() const;
		
		sl_bool getResponseKeepAliveParameters(sl_uint32& timeout, sl_uint32& max) const;
		
		void setResponseKeepAlive(sl_uint32 timeout = 0, sl_uint32 max = 0);
		
		sl_uint64 getResponseContentLengthHeader() const;
		
		void setResponseContentLengthHeader(sl_uint64 size);
		
		String getResponseContentType() const;
		
		void setResponseContentType(const String& type);

		void setResponseContentTypeIfEmpty(const String& type);

		void setResponseContentTypeFromFilePath(const String& path, const String& defaultType);

		String getResponseContentEncoding() const;
		
		void setResponseContentEncoding(const String& type);
		
		String getResponseTransferEncoding() const;
		
		void setResponseTransferEncoding(const String& type);
		
		sl_bool isChunkedResponse() const;
		
		sl_bool isAttachmentResponse() const;
		
		String getResponseAttachmentFileName() const;
		
		void setResponseInline();
		
		void setResponseAttachment(const String& fileName);
		
		String getResponseContentRange() const;
		
		void setResponseContentRange(const String& range);
		
		void setResponseContentRange(sl_uint64 start, sl_uint64 last, sl_uint64 total);
		
		void setResponseContentRangeUnknownTotal(sl_uint64 start, sl_uint64 last);
		
		void setResponseContentRangeUnsatisfied(sl_uint64 total);
		
		String getResponseAcceptRanges() const;
		
		void setResponseAcceptRanges(sl_bool flagAcceptRanges);
		
		void setResponseAcceptRangesIfNotDefined(sl_bool flagAcceptRanges);
		
		String getResponseAccessControlAllowOrigin() const;
		
		void setResponseAccessControlAllowOrigin(const String& origin);

		String getResponseAccessControlAllowHeaders() const;

		void setResponseAccessControlAllowHeaders(const String& headers);

		String getResponseAccessControlAllowMethods() const;

		void setResponseAccessControlAllowMethods(const String& methods);

		String getResponseAccessControlExposeHeaders() const;

		void setResponseAccessControlExposeHeaders(const String& headers);

		Time getResponseLastModified() const;
		
		void setResponseLastModified(const Time& time);
		
		HttpCacheControlResponse getResponseCacheControl() const;
		
		void setResponseCacheControl(const HttpCacheControlResponse&);

		List<HttpCookie> getResponseCookies() const;
		
		void setResponseCookies(const List<HttpCookie>&);
		
		HashMap<String, HttpCookie> getResponseCookieMap() const;
		
		void setResponseCookieMap(const HashMap<String, HttpCookie>&);
		
		sl_bool getResponseCookie(const String& name, HttpCookie* cookie);
		
		String getResponseCookie(const String& name);
		
		void setResponseCookie(const HttpCookie& cookie);
		
		void addResponseCookie(const HttpCookie& cookie);
		
		String getResponseRedirectLocation();
		
		void setResponseRedirectLocation(const String& location);

		void setResponseRedirect(const String& location, HttpStatus status = HttpStatus::Found);
		
		template <class MAP>
		void setResponseRedirect(const String& location, const MAP& queryParams, HttpStatus status = HttpStatus::Found)
		{
			String url = location;
			String query = HttpRequest::buildQuery(queryParams);
			if (query.isNotEmpty()) {
				if (url.indexOf('?') >= 0) {
					url += "&";
				} else {
					url += "?";
				}
				url += query;
			}
			setResponseRedirect(url, status);
		}

		
		Memory makeResponsePacket() const;
		
		/*
		 Returns
		 <0: error
		 =0: incomplete packet
		 >0: size of the HTTP header section (ending with [CR][LF][CR][LF])
		 */
		sl_reg parseResponsePacket(const void* packet, sl_size size);
		
	protected:
		HttpStatus m_responseCode;
		String m_responseMessage;
		String m_responseVersion;
		
		HttpHeaderMap m_responseHeaders;
		
	};
	
}

#endif

