/*
 *   Copyright (c) 2008-2019 SLIBIO <https://github.com/SLIBIO>
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

#include "slib/social/ebay.h"

#include "slib/core/safe_static.h"

namespace slib
{
	
	namespace priv
	{
		namespace ebay
		{
			
			SLIB_GLOBAL_ZERO_INITIALIZED(AtomicRef<Ebay>, g_instance)

			static String GetSimpleXMLValue(const String& xml, const String& tagName)
			{
				sl_reg index1 = xml.indexOf("<" + tagName + ">");
				if (index1 < 0) {
					return sl_null;
				}
				sl_reg index2 = xml.indexOf("</" + tagName + ">", index1 + 1);
				if (index2 < 0) {
					return sl_null;
				}
				return xml.substring(index1 + tagName.getLength() + 2, index2);
			}
			
			/*
			static List<String> GetSimpleXMLValues(const String& xml, const String& tagName) {
				List<String> ret;
				sl_reg index = 0;
				while (true) {
					sl_reg index1 = xml.indexOf("<" + tagName + ">", index);
					if (index1 < 0) {
						return sl_null;
					}
					sl_reg index2 = xml.indexOf("</" + tagName + ">", index1 + 1);
					if (index2 < 0) {
						return sl_null;
					}
					ret.add(xml.substring(index1 + tagName.getLength() + 2, index2));
					index = index2 + 1;
				}
				return ret;
			}
			*/
			
		}
	}
	
	using namespace priv::ebay;
	
	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(EbayUser)
	
	EbayUser::EbayUser()
	{
	}
	
	String EbayUser::getPublicProfileURL(const String& userId)
	{
		if (userId.isNotEmpty()) {
			return "http://www.ebay.com/usr/" + userId;
		}
		return sl_null;
	}
	
	String EbayUser::getPublicProfileURL() const
	{
		return getPublicProfileURL(userId);
	}
	
	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(EbayResult)
	
	EbayResult::EbayResult(UrlRequest* _request)
	{
		flagSuccess = sl_false;
		request = _request;
		if (_request) {
			response = _request->getResponseContentAsString();
		}
	}

	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(EbayParam)
	
	EbayParam::EbayParam(sl_bool flagSandbox)
	{
		setSandbox(flagSandbox);
		defaultScopes.add_NoLock("https://api.ebay.com/oauth/api_scope");
	}

	sl_bool EbayParam::isSandbox() const
	{
		return m_flagSandbox;
	}

	void EbayParam::setSandbox(sl_bool flag)
	{
		m_flagSandbox = flag;
		if (flag) {
			authorizeUrl = "https://auth.sandbox.ebay.com/oauth2/authorize";
			accessTokenUrl = "https://api.sandbox.ebay.com/identity/v1/oauth2/token";
		} else {
			authorizeUrl = "https://auth.ebay.com/oauth2/authorize";
			accessTokenUrl = "https://api.ebay.com/identity/v1/oauth2/token";
		}
	}

	void EbayParam::setRedirectUrl(const String& ruName, const String& _loginRedirectUri)
	{
		redirectUri = ruName;
		loginRedirectUri = _loginRedirectUri;
	}

	SLIB_DEFINE_OBJECT(Ebay, OAuth2)
	
	Ebay::Ebay(const EbayParam& param) : OAuth2(param)
	{
		m_flagSandbox = param.isSandbox();
	}
	
	Ebay::~Ebay()
	{
	}
	
	Ref<Ebay> Ebay::create(const EbayParam& param)
	{
		return new Ebay(param);
	}

	void Ebay::initialize(const EbayParam& param)
	{
		if (SLIB_SAFE_STATIC_CHECK_FREED(g_instance)) {
			return;
		}
		g_instance = create(param);
	}

	void Ebay::initialize()
	{
		EbayParam param(sl_false);
		param.preferenceName = "ebay";
		initialize(param);
	}

	void Ebay::initializeSandbox()
	{
		EbayParam param(sl_true);
		param.preferenceName = "ebay";
		initialize(param);
	}

	Ref<Ebay> Ebay::create(const String& appId, const String& appSecret, const String& ruName, const String& loginRedirectUri)
	{
		EbayParam param(sl_false);
		param.clientId = appId;
		param.clientSecret = appSecret;
		param.setRedirectUrl(ruName, loginRedirectUri);
		return create(param);
	}

	Ref<Ebay> Ebay::createSandbox(const String& appId, const String& appSecret, const String& ruName, const String& loginRedirectUri)
	{
		EbayParam param(sl_true);
		param.clientId = appId;
		param.clientSecret = appSecret;
		param.setRedirectUrl(ruName, loginRedirectUri);
		return create(param);
	}

	void Ebay::initialize(const String& appId, const String& appSecret, const String& ruName, const String& loginRedirectUri)
	{
		EbayParam param(sl_false);
		param.preferenceName = "ebay";
		param.clientId = appId;
		param.clientSecret = appSecret;
		param.setRedirectUrl(ruName, loginRedirectUri);
		initialize(param);
	}

	void Ebay::initializeSandbox(const String& appId, const String& appSecret, const String& ruName, const String& loginRedirectUri)
	{
		EbayParam param(sl_true);
		param.preferenceName = "ebay_sandbox";
		param.clientId = appId;
		param.clientSecret = appSecret;
		param.setRedirectUrl(ruName, loginRedirectUri);
		initialize(param);
	}

	Ref<Ebay> Ebay::create(const String& appId, const String& ruName, const String& loginRedirectUri)
	{
		return create(appId, String::null(), ruName, loginRedirectUri);
	}

	Ref<Ebay> Ebay::createSandbox(const String& appId, const String& ruName, const String& loginRedirectUri)
	{
		return createSandbox(appId, String::null(), ruName, loginRedirectUri);
	}

	void Ebay::initialize(const String& appId, const String& ruName, const String& loginRedirectUri)
	{
		initialize(appId, String::null(), ruName, loginRedirectUri);
	}

	void Ebay::initializeSandbox(const String& appId, const String& ruName, const String& loginRedirectUri)
	{
		initializeSandbox(appId, String::null(), ruName, loginRedirectUri);
	}

	Ref<Ebay> Ebay::createWithAccessToken(const String& accessToken)
	{
		EbayParam param(sl_false);
		param.accessToken.token = accessToken;
		return create(param);
	}

	Ref<Ebay> Ebay::createSandboxWithAccessToken(const String& accessToken)
	{
		EbayParam param(sl_true);
		param.accessToken.token = accessToken;
		return create(param);
	}

	Ref<Ebay> Ebay::getInstance()
	{
		if (SLIB_SAFE_STATIC_CHECK_FREED(g_instance)) {
			return sl_null;
		}
		return g_instance;
	}
	
	String Ebay::getRequestUrl(const String& path)
	{
		if (m_flagSandbox) {
			return "https://api.sandbox.ebay.com/" + path;
		} else {
			return "https://api.ebay.com/" + path;
		}
	}
	
	void Ebay::callTraditionalApi(const String& callName, const String& request, const Function<void(UrlRequest*)>& onComplete)
	{
		UrlRequestParam rp;
		rp.url = getRequestUrl("ws/api.dll");
		rp.method = HttpMethod::POST;
		rp.requestHeaders.put_NoLock("X-EBAY-API-COMPATIBILITY-LEVEL", "1085");
		rp.requestHeaders.put_NoLock("X-EBAY-API-SITEID", "0");
		rp.requestHeaders.put_NoLock("X-EBAY-API-CALL-NAME", callName);
		rp.requestHeaders.put_NoLock("X-EBAY-API-IAF-TOKEN", getAccessTokenKey());
		rp.requestHeaders.put_NoLock("Content-Type", "text/xml");
		rp.setRequestBodyAsString(request);
		rp.onComplete = onComplete;
		UrlRequest::send(rp);
	}
	
	void Ebay::getUser(const Function<void(EbayResult&, EbayUser&)>& onComplete)
	{
		String request = SLIB_STRINGIFY(
			<?xml version="1.0" encoding="utf-8"?>
			<GetUserRequest xmlns="urn:ebay:apis:eBLBaseComponents" />
		);
		callTraditionalApi("GetUser", request, [onComplete](UrlRequest* request) {
			EbayResult result(request);
			EbayUser user;
			if (!(request->isError())) {
				user.userId = GetSimpleXMLValue(result.response, "UserID");
				user.email = GetSimpleXMLValue(result.response, "Email");
				result.flagSuccess = user.userId.isNotEmpty();
			}
			onComplete(result, user);
		});
	}
	
}
