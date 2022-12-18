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

#include "facebook.h"

#include <slib/core/safe_static.h>

namespace slib
{

	namespace priv
	{
		namespace facebook
		{
			SLIB_GLOBAL_ZERO_INITIALIZED(AtomicRef<Facebook>, g_instance)
		}
	}

	using namespace priv::facebook;

	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(FacebookUser)

	SLIB_DEFINE_JSON(FacebookUser)
	{
		if (isFromJson) {
			this->json = json;
		}
		SLIB_JSON_ADD_MEMBERS(id, email, name, name_format, first_name, middle_name, last_name, short_name, gender, birthday, quotes, profile_pic)
	}

	FacebookUser::FacebookUser()
	{
	}


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(FacebookShareResult)

	FacebookShareResult::FacebookShareResult()
	{
		flagSuccess = sl_false;
		flagCancel = sl_false;
	}

	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(FacebookShareParam)

	FacebookShareParam::FacebookShareParam()
	{
	}


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(FacebookParam)

	FacebookParam::FacebookParam() : FacebookParam(sl_null)
	{
	}

	FacebookParam::FacebookParam(const String& _version)
	{
		if (_version.isNotEmpty()) {
			version = _version;
			authorizeUrl = String::format("https://www.facebook.com/%s/dialog/oauth", _version);
			accessTokenUrl = String::format("https://graph.facebook.com/%s/oauth/access_token", _version);
		} else {
			authorizeUrl = "https://www.facebook.com/dialog/oauth";
			accessTokenUrl = "https://graph.facebook.com/oauth/access_token";
		}
		accessTokenMethod = HttpMethod::GET;
		defaultScopes.add_NoLock("public_profile");
		defaultScopes.add_NoLock("email");
	}

	SLIB_DEFINE_OBJECT(Facebook, OAuth2)

	Facebook::Facebook(const FacebookParam& param) : OAuth2(param)
	{
		m_version = param.version;
	}

	Facebook::~Facebook()
	{
	}

	Ref<Facebook> Facebook::create(const FacebookParam& param)
	{
		return new Facebook(param);
	}

	void Facebook::initialize(const FacebookParam& param)
	{
		if (SLIB_SAFE_STATIC_CHECK_FREED(g_instance)) {
			return;
		}
		g_instance = create(param);
	}

	void Facebook::initialize()
	{
		FacebookParam param;
		param.preferenceName = "facebook";
		initialize(param);
	}

	Ref<Facebook> Facebook::create(const String& appId, const String& appSecret, const String& redirectUri)
	{
		FacebookParam param;
		param.clientId = appId;
		param.clientSecret = appSecret;
		param.redirectUri = redirectUri;
		return create(param);
	}

	void Facebook::initialize(const String& appId, const String& appSecret, const String& redirectUri)
	{
		FacebookParam param;
		param.preferenceName = "facebook";
		param.clientId = appId;
		param.clientSecret = appSecret;
		param.redirectUri = redirectUri;
		initialize(param);
	}

	Ref<Facebook> Facebook::create(const String& appId, const String& redirectUri)
	{
		return create(appId, String::null(), redirectUri);
	}

	void Facebook::initialize(const String& appId, const String& redirectUri)
	{
		initialize(appId, String::null(), redirectUri);
	}

	Ref<Facebook> Facebook::createWithAccessToken(const String& accessToken)
	{
		FacebookParam param;
		param.accessToken.token = accessToken;
		return create(param);
	}

	Ref<Facebook> Facebook::getInstance()
	{
		if (SLIB_SAFE_STATIC_CHECK_FREED(g_instance)) {
			return sl_null;
		}
		return g_instance;
	}

	String Facebook::getRequestUrl(const String& path)
	{
		if (m_version.isNotEmpty()) {
			return String::format("https://graph.facebook.com/%s/%s", m_version, path);
		} else {
			return "https://graph.facebook.com/" + path;
		}
	}

	void Facebook::getUser(const String& personId, const String& fields, const Function<void(FacebookResult&, FacebookUser&)>& onComplete)
	{
		UrlRequestParam rp;
		if (personId.isNotEmpty()) {
			rp.url = getRequestUrl(String::format("%s", personId));
		} else {
			SLIB_STATIC_STRING(me, "me")
			rp.url = getRequestUrl(me);
		}
		if (fields.isNotEmpty()) {
			rp.parameters.put_NoLock("fields", fields);
		}
		rp.onComplete = [onComplete](UrlRequest* request) {
			FacebookResult result(request);
			FacebookUser user;
			if (!(request->isError())) {
				FromJson(result.response, user);
				result.flagSuccess = user.id.isNotEmpty();
			}
			onComplete(result, user);
		};
		authorizeRequest(rp);
		UrlRequest::send(rp);
	}

	void Facebook::getUser(const String& personId, const List<String>& fields, const Function<void(FacebookResult&, FacebookUser&)>& onComplete)
	{
		getUser(personId, String::join(fields, ","), onComplete);
	}

	void Facebook::getUser(const String& personId, const Function<void(FacebookResult&, FacebookUser&)>& onComplete)
	{
		SLIB_STATIC_STRING(defaultFields, "id,name,name_format,first_name,last_name,middle_name,email")
		getUser(personId, defaultFields, onComplete);
	}

}
