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

#include "slib/crypto/oauth.h"

#include "slib/crypto/hmac.h"
#include "slib/crypto/sha1.h"
#include "slib/crypto/sha2.h"
#include "slib/data/base64.h"
#include "slib/core/preference.h"
#include "slib/core/log.h"

#define TAG "OAuth"

namespace slib
{

	SLIB_DEFINE_NESTED_CLASS_DEFAULT_MEMBERS(OAuth1, AccessToken)

	SLIB_DEFINE_JSON_MEMBERS(OAuth1::AccessToken, token, secret)

	OAuth1::AccessToken::AccessToken()
	{
	}

	OAuth1::AccessToken::AccessToken(const String& _token, const String& _tokenSecret): token(_token), secret(_tokenSecret)
	{
	}

	sl_bool OAuth1::AccessToken::isValid() const
	{
		return token.isNotEmpty() && secret.isNotEmpty();
	}

	void OAuth1::AccessToken::setResponse(const HashMap<String, String>& params)
	{
		token = params["oauth_token"];
		secret = params["oauth_token_secret"];
	}


	SLIB_DEFINE_NESTED_CLASS_DEFAULT_MEMBERS(OAuth1, AuthorizationRequestParam)

	OAuth1::AuthorizationRequestParam::AuthorizationRequestParam()
	{
	}


	SLIB_DEFINE_NESTED_CLASS_DEFAULT_MEMBERS(OAuth1, AccessTokenResult)

	OAuth1::AccessTokenResult::AccessTokenResult()
	{
		flagSuccess = sl_false;
	}

	void OAuth1::AccessTokenResult::setResponse(const HashMap<String, String>& params)
	{
		response = params;
		accessToken.setResponse(params);
		flagSuccess = accessToken.isValid();
	}


	SLIB_DEFINE_NESTED_CLASS_DEFAULT_MEMBERS(OAuth1, LoginResult)

	OAuth1::LoginResult::LoginResult()
	{
		flagCancel = sl_false;
		flagCache = sl_false;
	}

	void OAuth1::LoginResult::parseRedirectUrl(const String& _url)
	{
		Url url(_url);

		HashMap<String, String> params = HttpRequest::parseQueryParameters(url.query);

		requestToken = params.getValue_NoLock("oauth_token");
		verifier  = params.getValue_NoLock("oauth_verifier");

		response = params;

		flagSuccess = requestToken.isNotEmpty() && verifier.isNotEmpty();
	}


	SLIB_DEFINE_NESTED_CLASS_DEFAULT_MEMBERS(OAuth1, Param)

	OAuth1::Param::Param()
	{
		requestTokenMethod = HttpMethod::POST;
		accessTokenMethod = HttpMethod::POST;
	}


	SLIB_DEFINE_OBJECT(OAuth1, Object)

	OAuth1::OAuth1(const Param& param)
	{
		m_preferenceName = param.preferenceName;
		restore();

		m_consumerKey = param.consumerKey;
		m_consumerSecret = param.consumerSecret;
		if (param.accessToken.isValid()) {
			m_accessToken = param.accessToken;
		}

		m_requestTokenMethod = param.requestTokenMethod;
		m_requestTokenUrl = param.requestTokenUrl;
		m_authenticateUrl = param.authenticateUrl;
		m_accessTokenMethod = param.accessTokenMethod;
		m_accessTokenUrl = param.accessTokenUrl;
		m_callbackUrl = param.callbackUrl;
		m_flagLogErrors = sl_true;
	}

	OAuth1::~OAuth1()
	{
	}

	Shared<OAuth1::AccessToken> OAuth1::getAccessToken()
	{
		return m_accessToken;
	}

	void OAuth1::setAccessToken(const AccessToken& accessToken)
	{
		if (accessToken.isValid()) {
			m_accessToken = accessToken;
		} else {
			m_accessToken.setNull();
		}
		save();
	}

	void OAuth1::setAccessToken(const String& token, const String& secret)
	{
		if (token.isNotEmpty() && secret.isNotEmpty()) {
			m_accessToken = AccessToken(token, secret);
		} else {
			m_accessToken.setNull();
		}
		save();
	}

	void OAuth1::clearAccessToken()
	{
		m_accessToken.setNull();
		save();
	}

	String OAuth1::getAccessTokenKey()
	{
		Shared<AccessToken> accessToken = m_accessToken;
		if (accessToken.isNotNull()) {
			return accessToken->token;
		}
		return sl_null;
	}

	String OAuth1::getAccessTokenSecret()
	{
		Shared<AccessToken> accessToken = m_accessToken;
		if (accessToken.isNotNull()) {
			return accessToken->secret;
		}
		return sl_null;
	}

	String OAuth1::getCallbackUrl()
	{
		return m_callbackUrl;
	}

	void OAuth1::setCallbackUrl(const String& url)
	{
		m_callbackUrl = url;
	}

	void OAuth1::setLoggingErrors(sl_bool flag)
	{
		m_flagLogErrors = flag;
	}

	String OAuth1::generateAuthorization(HttpMethod _method, const String& _url, HashMap<String, String>& parameters, const String& nonce, sl_int64 _timestamp, const String& token, const String& tokenSecret, const String& callbackUrl)
	{
		String consumerKey = Url::encodePercent(m_consumerKey);
		String timestamp = String::fromInt64(_timestamp);

		Map< Pair<String, String>, sl_bool > mapParams;
		String method = HttpMethodHelper::toString(_method);
		String url = _url;
		sl_reg indexQuery = url.indexOf('?');
		if (indexQuery > 0) {
			for (auto& item : HttpRequest::parseQueryParameters(url.substring(indexQuery + 1))) {
				mapParams.add_NoLock(Pair<String, String>(Url::encodePercent(item.key), Url::encodePercent(item.value)), sl_true);
			}
			url = url.substring(0, indexQuery);
		}
		for (auto& item : parameters) {
			mapParams.add_NoLock(Pair<String, String>(Url::encodePercent(item.key), Url::encodePercent(item.value)), sl_true);
		}
		mapParams.add_NoLock(Pair<String, String>("oauth_consumer_key", consumerKey), sl_true);
		mapParams.add_NoLock(Pair<String, String>("oauth_nonce", nonce), sl_true);
		mapParams.add_NoLock(Pair<String, String>("oauth_signature_method", "HMAC-SHA1"), sl_true);
		mapParams.add_NoLock(Pair<String, String>("oauth_timestamp", timestamp), sl_true);
		mapParams.add_NoLock(Pair<String, String>("oauth_version", "1.0"), sl_true);
		if (token.isNotEmpty()) {
			mapParams.add_NoLock(Pair<String, String>("oauth_token", Url::encodePercent(token)), sl_true);
		}
		if (callbackUrl.isNotEmpty()) {
			mapParams.add_NoLock(Pair<String, String>("oauth_callback", Url::encodePercent(callbackUrl)), sl_true);
		}

		StringBuffer sbParams;
		{
			sl_bool flagFirstParam = sl_true;
			for (auto& item : mapParams) {
				if (flagFirstParam) {
					flagFirstParam = sl_false;
				} else {
					sbParams.addStatic("&");
				}
				sbParams.add(item.key.first);
				sbParams.addStatic("=");
				sbParams.add(item.key.second);
			}
		}

		StringBuffer sbSignature;
		sbSignature.add(method);
		sbSignature.addStatic("&");
		sbSignature.add(Url::encodePercent(url));
		sbSignature.addStatic("&");
		sbSignature.add(Url::encodePercent(sbParams.merge()));
		String signatureBase = sbSignature.merge();
		String signKey = Url::encodePercent(m_consumerSecret) + "&";
		if (tokenSecret.isNotEmpty()) {
			signKey += Url::encodePercent(tokenSecret);
		}
		char bufSignature[SHA1::HashSize];
		HMAC<SHA1>::execute(signKey.getData(), signKey.getLength(), signatureBase.getData(), signatureBase.getLength(), bufSignature);
		String signature = Url::encodePercent(Base64::encode(bufSignature, SHA1::HashSize));

		mapParams.add_NoLock(Pair<String, String>("oauth_signature", signature), sl_true);
		StringBuffer sbAuthorization;
		{
			sbAuthorization.addStatic("OAuth ");
			sl_bool flagFirstParam = sl_true;
			for (auto& item : mapParams) {
				if (item.key.first.startsWith("oauth_")) {
					if (flagFirstParam) {
						flagFirstParam = sl_false;
					} else {
						sbAuthorization.addStatic(", ");
					}
					sbAuthorization.add(item.key.first);
					sbAuthorization.addStatic("=\"");
					sbAuthorization.add(item.key.second);
					sbAuthorization.addStatic("\"");
				}
			}
		}
		return sbAuthorization.merge();
	}

	void OAuth1::authorizeRequest(UrlRequestParam& param, const String& token, const String& tokenSecret, const String& callbackUrl)
	{
		char bufNonce[32];
		Math::randomMemory(bufNonce, 32);
		String nonce = String::makeHexString(bufNonce, 32);
		sl_int64 timestamp = Time::now().toUnixTime();

		HashMap<String, String> parameters;
		for (auto& item : param.parameters) {
			parameters.add_NoLock(item.key, item.value.getString());
		}

		if (param.method == HttpMethod::POST) {
			String type = param.requestHeaders.getValue(HttpHeader::ContentType);
			sl_reg index = type.indexOf(';');
			if (index >= 0) {
				type = type.substring(0, index);
			}
			if (!(type.trim().equals_IgnoreCase(ContentType::MultipartFormData))) {
				if (param.requestBody.isNotNull()) {
					for (auto& item : HttpRequest::parseFormUrlEncoded(param.requestBody.getData(), param.requestBody.getSize())) {
						parameters.add_NoLock(item.key, item.value);
					}
				}
			}
		}

		String authorization = generateAuthorization(param.method, param.url, parameters, nonce, timestamp, token, tokenSecret, callbackUrl);
		param.requestHeaders.add_NoLock(HttpHeader::Authorization, authorization);
	}

	void OAuth1::authorizeRequest(UrlRequestParam& param)
	{
		Shared<AccessToken> token = m_accessToken;
		if (token.isNotNull()) {
			authorizeRequest(param, token->token, token->secret, sl_null);
		}
	}

	void OAuth1::getLoginUrl(const AuthorizationRequestParam& param, const Function<void(const String& url, const String& requestToken, const String& requestTokenSecret)>& onComplete)
	{
		String callbackUrl = param.callbackUrl;
		if (callbackUrl.isEmpty()) {
			callbackUrl = m_callbackUrl;
		}
		UrlRequestParam rp;
		rp.method = m_requestTokenMethod;
		rp.url = m_requestTokenUrl;
		if (rp.method == HttpMethod::POST) {
			rp.setFormData(param.customParameters);
		} else {
			rp.parameters = param.customParameters;
		}
		auto thiz = ToRef(this);
		rp.onComplete = [thiz, onComplete](UrlRequest* request) {
			if (request->isError()) {
				thiz->logUrlRequestError(request);
				onComplete(sl_null, sl_null, sl_null);
				return;
			}
			String response = request->getResponseContentAsString();
			auto params = HttpRequest::parseFormUrlEncoded(response);
			String requestToken = params["oauth_token"];
			String requestTokenSecret = params["oauth_token_secret"];
			String loginUrl = params["login_url"];
			if (loginUrl.isEmpty()) {
				loginUrl = String::format(thiz->m_authenticateUrl, Url::encodePercent(requestToken));
			}
			onComplete(loginUrl, requestToken, requestTokenSecret);
		};
		authorizeRequest(rp, sl_null, sl_null, callbackUrl);
		UrlRequest::send(rp);
	}

	void OAuth1::getLoginUrl(const Function<void(const String& url, const String& requestToken, const String& requestTokenSecret)>& onComplete)
	{
		AuthorizationRequestParam param;
		getLoginUrl(param, onComplete);
	}

	void OAuth1::requestAccessToken(const String& verifier, const String& requestToken, const String& requestTokenSecret, const Function<void(AccessTokenResult&)>& onComplete)
	{
		UrlRequestParam rp;
		rp.method = m_accessTokenMethod;
		rp.url = m_accessTokenUrl;
		if (rp.method == HttpMethod::POST) {
			rp.setRequestBodyAsString("oauth_verifier=" + Url::encodePercent(verifier));
		} else {
			rp.parameters.put_NoLock("oauth_verifier", verifier);
		}
		auto thiz = ToRef(this);
		rp.onComplete = [thiz, onComplete](UrlRequest* request) {
			AccessTokenResult result;
			result.setResponse(HttpRequest::parseFormUrlEncoded(request->getResponseContentAsString()));
			if (request->isError()) {
				thiz->logUrlRequestError(request);
				result.flagSuccess = sl_false;
			}
			if (result.flagSuccess) {
				thiz->setAccessToken(result.accessToken);
			}
			onComplete(result);
		};

		authorizeRequest(rp, requestToken, requestTokenSecret, sl_null);
		UrlRequest::send(rp);
	}

	void OAuth1::logUrlRequestError(UrlRequest* request)
	{
		if (m_flagLogErrors) {
			LogError(TAG, "Error: %s, Status: %s, Response: %s", request->getErrorMessage(), HttpStatusHelper::toString(request->getResponseStatus()), request->getResponseContentAsString());
		}
	}

	void OAuth1::save()
	{
		if (m_preferenceName.isEmpty()) {
			return;
		}
		String key = "oauth1_" + m_preferenceName;
		Json value;
		Shared<AccessToken> accessToken = m_accessToken;
		if (accessToken.isNotNull()) {
			value.set(*accessToken);
		}
		Preference::setValue(key, value);
	}

	void OAuth1::restore()
	{
		if (m_preferenceName.isEmpty()) {
			return;
		}
		String key = "oauth1_" + m_preferenceName;
		Json value = Preference::getValue(key);
		if (value.isNotNull()) {
			AccessToken accessToken;
			FromJson(value, accessToken);
			m_accessToken = accessToken;
		}
	}


	SLIB_DEFINE_NESTED_CLASS_DEFAULT_MEMBERS(OAuth2, AccessToken)

	SLIB_DEFINE_JSON_MEMBERS(OAuth2::AccessToken, token, refreshToken, tokenType, scopes, expirationTime, refreshTime)

	OAuth2::AccessToken::AccessToken(): expirationTime(0), refreshTime(0)
	{
	}

	OAuth2::AccessToken::AccessToken(const String& _token): token(_token), expirationTime(0), refreshTime(Time::now())
	{
	}

	sl_bool OAuth2::AccessToken::isValid() const
	{
		return token.isNotEmpty() && (expirationTime.isZero() || Time::now() <= expirationTime);
	}

	sl_bool OAuth2::AccessToken::isValid(const List<String>& _requiredScopes) const
	{
		if (!(isValid())) {
			return sl_false;
		}
		ListElements<String> requiredScopes(_requiredScopes);
		for (sl_size i = 0; i < requiredScopes.count; i++) {
			if (!(scopes.contains(requiredScopes[i]))) {
				return sl_false;
			}
		}
		return sl_true;
	}

	void OAuth2::AccessToken::setResponse(const Json& json)
	{
		token = json.getItem("access_token").getString();
		if (token.isEmpty()) {
			return;
		}
		tokenType = json.getItem("token_type").getString();
		refreshTime = Time::now();
		Json jsonExpireIn = json.getItem("expires_in");
		if (jsonExpireIn.isNotNull()) {
			sl_uint32 nSecondsExpiresIn = jsonExpireIn.getUint32();
			expirationTime = refreshTime;
			expirationTime.addSeconds(nSecondsExpiresIn);
		} else {
			expirationTime.setZero();
		}
		refreshToken = json.getItem("refresh_token").getString();
		scopes = json.getItem("scope").getString().split(" ");
	}


	SLIB_DEFINE_NESTED_CLASS_DEFAULT_MEMBERS(OAuth2, AuthorizationRequest)

	OAuth2::AuthorizationRequest::AuthorizationRequest()
	{
		responseType = ResponseType::Token;
		codeChallengeMethod = CodeChallengeMethod::S256;
	}


	SLIB_DEFINE_NESTED_CLASS_DEFAULT_MEMBERS(OAuth2, Result)

	OAuth2::Result::Result()
	{
		flagSuccess = sl_false;
		errorCode = ErrorCode::None;
	}

	void OAuth2::Result::setResponse(const Json& json)
	{
		flagSuccess = sl_false;
		response = json;
		error = json.getItem("error").getString();
		errorDescription = json.getItem("error_description").getString();
		errorUri = json.getItem("error_uri").getString();
		if (error.isNotEmpty()) {
			if (error == "invalid_request") {
				errorCode = ErrorCode::InvalidRequest;
			} else if (error == "unauthorized_client") {
				errorCode = ErrorCode::UnauthorizedClient;
			} else if (error == "access_denied") {
				errorCode = ErrorCode::AccessDenied;
			} else if (error == "unsupported_response_type") {
				errorCode = ErrorCode::UnsupportedResponseType;
			} else if (error == "invalid_scope") {
				errorCode = ErrorCode::InvalidScope;
			} else if (error == "server_error") {
				errorCode = ErrorCode::ServerError;
			} else if (error == "temporarily_unavailable") {
				errorCode = ErrorCode::TemporarilyUnavailable;
			} else if (error == "invalid_client") {
				errorCode = ErrorCode::InvalidClient;
			} else if (error == "invalid_grant") {
				errorCode = ErrorCode::InvalidGrant;
			} else if (error == "unsupported_grant_type") {
				errorCode = ErrorCode::UnsupportedGrantType;
			} else {
				errorCode = ErrorCode::None;
			}
		} else {
			flagSuccess = sl_true;
		}
	}

	void OAuth2::Result::setResult(UrlRequest* req)
	{
		setResponse(req->getResponseContentAsJson());
	}


	SLIB_DEFINE_NESTED_CLASS_DEFAULT_MEMBERS(OAuth2, AccessTokenResult)

	OAuth2::AccessTokenResult::AccessTokenResult()
	{
	}

	void OAuth2::AccessTokenResult::setResult(UrlRequest* req)
	{
		Result::setResult(req);
		accessToken.setResponse(response);
		if (!(accessToken.isValid())) {
			flagSuccess = sl_false;
		}
	}


	SLIB_DEFINE_NESTED_CLASS_DEFAULT_MEMBERS(OAuth2, LoginResult)

	OAuth2::LoginResult::LoginResult()
	{
		flagCancel = sl_false;
		flagCache = sl_false;
		errorCode = ErrorCode::None;
	}

	void OAuth2::LoginResult::parseRedirectUrl(const String& _url)
	{
		Url url(_url);

		HashMap<String, String> params;
		params.addAll_NoLock(HttpRequest::parseQueryParameters(url.query));
		params.addAll_NoLock(HttpRequest::parseQueryParameters(url.fragment));

		Result::setResponse(params);
		flagCancel = errorCode == ErrorCode::AccessDenied;

		accessToken.setResponse(params);

		state = params.getValue_NoLock("state");
		code = params.getValue_NoLock("code");

		response = params;

		if (code.isEmpty() && !(accessToken.isValid())) {
			flagSuccess = sl_false;
		}
	}


	SLIB_DEFINE_NESTED_CLASS_DEFAULT_MEMBERS(OAuth2, Param)

	OAuth2::Param::Param()
	{
		accessTokenMethod = HttpMethod::POST;
		flagUseBasicAuthorizationForAccessToken = sl_false;
		flagSupportImplicitGrantType = sl_true;
		SLIB_STATIC_STRING(strClientId,  "client_id")
		clientIdFieldName = strClientId;
		SLIB_STATIC_STRING(strClientSecret,  "client_secret")
		clientSecretFieldName = strClientSecret;
		flagLoggingErrors = sl_true;
	}


	SLIB_DEFINE_OBJECT(OAuth2, Object)

	OAuth2::OAuth2(const Param& param)
	{
		m_preferenceName = param.preferenceName;
		restore();

		m_clientId = param.clientId;
		m_clientSecret = param.clientSecret;
		if (param.accessToken.isValid()) {
			m_accessToken = param.accessToken;
		}

		m_authorizeUrl = param.authorizeUrl;
		m_accessTokenUrl = param.accessTokenUrl;
		m_accessTokenMethod = param.accessTokenMethod;
		m_flagUseBasicAuthorizationForAccessToken = param.flagUseBasicAuthorizationForAccessToken;
		m_redirectUri = param.redirectUri;
		m_loginRedirectUri = param.loginRedirectUri;
		m_defaultScopes = param.defaultScopes;
		m_flagSupportImplicitGrantType = param.flagSupportImplicitGrantType;
		m_clientIdFieldName = param.clientIdFieldName;
		m_clientSecretFieldName = param.clientSecretFieldName;

		m_flagLogErrors = param.flagLoggingErrors;
	}

	OAuth2::~OAuth2()
	{
	}

	Shared<OAuth2::AccessToken> OAuth2::getAccessToken()
	{
		return m_accessToken;
	}

	void OAuth2::setAccessToken(const AccessToken& accessToken)
	{
		if (accessToken.isValid()) {
			m_accessToken = accessToken;
		} else {
			m_accessToken.setNull();
		}
		save();
	}

	void OAuth2::setAccessToken(const String& accessToken)
	{
		if (accessToken.isNotEmpty()) {
			m_accessToken = accessToken;
		} else {
			m_accessToken.setNull();
		}
		save();
	}

	void OAuth2::clearAccessToken()
	{
		m_accessToken.setNull();
		save();
	}

	String OAuth2::getAccessTokenKey()
	{
		Shared<AccessToken> accessToken = m_accessToken;
		if (accessToken.isNotNull()) {
			return accessToken->token;
		}
		return sl_null;
	}

	List<String> OAuth2::getAccessTokenScopes()
	{
		Shared<AccessToken> accessToken = m_accessToken;
		if (accessToken.isNotNull()) {
			return accessToken->scopes;
		}
		return sl_null;
	}

	void OAuth2::setLoggingErrors(sl_bool flag)
	{
		m_flagLogErrors = flag;
	}

	void OAuth2::authorizeRequest(UrlRequestParam& param, const AccessToken& token)
	{
		if (token.tokenType.isEmpty() || token.tokenType.equals_IgnoreCase("bearer")) {
			param.requestHeaders.put_NoLock(HttpHeader::Authorization, "Bearer " + token.token);
		}
	}

	void OAuth2::authorizeRequest(UrlRequestParam& param)
	{
		Shared<AccessToken> token = m_accessToken;
		if (token.isNotNull()) {
			authorizeRequest(param, *token);
		}
	}

	String OAuth2::getLoginUrl(const AuthorizationRequest& param)
	{
		Url url(m_authorizeUrl);
		auto oldParameters = url.getQueryParameters();
		HashMap<String, String> params;
		if (param.responseType == ResponseType::Token) {
			params.put_NoLock("response_type", "token");
		} else {
			params.put_NoLock("response_type", "code");
		}
		if (param.clientId.isNotEmpty()) {
			params.put_NoLock(m_clientIdFieldName, param.clientId);
		} else {
			params.put_NoLock(m_clientIdFieldName, m_clientId);
		}
		String redirectUri = param.redirectUri;
		if (redirectUri.isEmpty()) {
			redirectUri = m_redirectUri;
		}
		if (redirectUri.isNotEmpty()) {
			params.put_NoLock("redirect_uri", redirectUri);
		}
		List<String> listScopes = param.scopes;
		if (listScopes.isNull()) {
			listScopes = m_defaultScopes;
		}
		String scope = String::join(listScopes, " ").trim();
		if (scope.isNotEmpty()) {
			params.put_NoLock("scope", scope);
		}
		if (param.state.isNotEmpty()) {
			params.put_NoLock("state", param.state);
		}
		{
			MutexLocker lock(param.customParameters.getLocker());
			for (auto& item: param.customParameters) {
				params.put_NoLock(item.key, item.value.getString());
			}
		}
		if (param.responseType == ResponseType::Code) {
			String codeChallenge;
			if (param.codeChallenge.isNotEmpty()) {
				codeChallenge = param.codeChallenge;
			} else if (param.codeVerifier.isNotEmpty()) {
				codeChallenge = generateCodeChallenge(param.codeVerifier, param.codeChallengeMethod);
			}
			if (codeChallenge.isNotEmpty()) {
				params.put_NoLock("code_challenge", codeChallenge);
				if (param.codeChallengeMethod == CodeChallengeMethod::S256) {
					params.put_NoLock("code_challenge_method", "S256");
				} else {
					params.put_NoLock("code_challenge_method", "plain");
				}
			}
		}
		params.putAll_NoLock(oldParameters);
		url.setQueryParameters(params);
		return url.toString();
	}

	String OAuth2::getLoginUrl(ResponseType type, const List<String>& scopes, const String& state)
	{
		AuthorizationRequest param;
		param.responseType = type;
		param.scopes = scopes;
		param.state = state;
		return getLoginUrl(param);
	}

	String OAuth2::getLoginUrl(const List<String>& scopes, const String& state)
	{
		AuthorizationRequest param;
		param.scopes = scopes;
		param.state = state;
		return getLoginUrl(param);
	}

	void OAuth2::requestAccessToken(VariantMap& params, const Function<void(AccessTokenResult&)>& onComplete)
	{
		if (m_clientId.isEmpty() || m_clientSecret.isEmpty()) {
			AccessTokenResult result;
			onComplete(result);
			return;
		}
		UrlRequestParam rp;
		rp.method = m_accessTokenMethod;
		rp.url = m_accessTokenUrl;
		if (m_flagUseBasicAuthorizationForAccessToken) {
			rp.requestHeaders.put_NoLock(HttpHeader::Authorization, "Basic " + Base64::encode(m_clientId + ":" + m_clientSecret));
		} else {
			params.put_NoLock(m_clientIdFieldName, m_clientId);
			params.put_NoLock(m_clientSecretFieldName, m_clientSecret);
		}
		if (rp.method == HttpMethod::POST) {
			rp.setFormData(params);
		} else {
			rp.parameters = params;
		}
		auto thiz = ToRef(this);
		rp.onComplete = [thiz, onComplete](UrlRequest* request) {
			AccessTokenResult result;
			result.setResult(request);
			if (request->isError()) {
				thiz->logUrlRequestError(request);
				result.flagSuccess = sl_false;
			}
			if (result.flagSuccess) {
				thiz->setAccessToken(result.accessToken);
				thiz->onReceiveAccessToken(result);
			}
			onComplete(result);
		};
		UrlRequest::send(rp);
	}

	void OAuth2::requestAccessTokenFromCode(const String& code, const String& redirectUri, const String& codeVerifier, const List<String>& scopes, const Function<void(AccessTokenResult&)>& onComplete)
	{
		VariantMap params;
		params.put_NoLock("grant_type", "authorization_code");
		params.put_NoLock("code", code);
		if (redirectUri.isNotEmpty()) {
			params.put_NoLock("redirect_uri", redirectUri);
		} else {
			params.put_NoLock("redirect_uri", m_redirectUri);
		}
		if (codeVerifier.isNotEmpty()) {
			params.put_NoLock("code_verifier", codeVerifier);
		}
		if (scopes.isNotNull()) {
			String s = String::join(scopes, " ").trim();
			if (s.isNotEmpty()) {
				params.put_NoLock("scope", s);
			}
		}
		requestAccessToken(params, onComplete);
	}

	void OAuth2::requestAccessTokenFromCode(const String& code, const String& redirectUri, const String& codeVerifier, const Function<void(AccessTokenResult&)>& onComplete)
	{
		requestAccessTokenFromCode(code, redirectUri, sl_null, sl_null, onComplete);
	}

	void OAuth2::requestAccessTokenFromCode(const String& code, const String& redirectUri, const Function<void(AccessTokenResult&)>& onComplete)
	{
		requestAccessTokenFromCode(code, redirectUri, sl_null, sl_null, onComplete);
	}

	void OAuth2::requestAccessTokenFromCode(const String& code, const Function<void(AccessTokenResult&)>& onComplete)
	{
		requestAccessTokenFromCode(code, sl_null, sl_null, sl_null, onComplete);
	}

	void OAuth2::requestAccessTokenFromClientCredentials(const List<String>& scopes, const Function<void(AccessTokenResult&)>& onComplete)
	{
		VariantMap params;
		params.put_NoLock("grant_type", "client_credentials");
		if (scopes.isNotNull()) {
			String s = String::join(scopes, " ").trim();
			if (s.isNotEmpty()) {
				params.put_NoLock("scope", s);
			}
		}
		requestAccessToken(params, onComplete);
	}

	void OAuth2::requestAccessTokenFromClientCredentials(const Function<void(AccessTokenResult&)>& onComplete)
	{
		requestAccessTokenFromClientCredentials(sl_null, onComplete);
	}

	void OAuth2::requestAccessTokenFromUserPassword(const String& username, const String& password, const List<String>& scopes, const Function<void(AccessTokenResult&)>& onComplete)
	{
		VariantMap params;
		params.put_NoLock("grant_type", "password");
		params.put_NoLock("username", username);
		params.put_NoLock("password", password);
		if (scopes.isNotNull()) {
			String s = String::join(scopes, " ").trim();
			if (s.isNotEmpty()) {
				params.put_NoLock("scope", s);
			}
		}
		requestAccessToken(params, onComplete);
	}

	void OAuth2::requestAccessTokenFromUserPassword(const String& username, const String& password, const Function<void(AccessTokenResult&)>& onComplete)
	{
		requestAccessTokenFromUserPassword(username, password, sl_null, onComplete);
	}

	void OAuth2::refreshAccessToken(const String& refreshToken, const List<String>& scopes, const Function<void(AccessTokenResult&)>& onComplete)
	{
		VariantMap params;
		params.put_NoLock("grant_type", "refresh_token");
		params.put_NoLock("refresh_token", refreshToken);
		if (scopes.isNotNull()) {
			String s = String::join(scopes, " ").trim();
			if (s.isNotEmpty()) {
				params.put_NoLock("scope", s);
			}
		}
		requestAccessToken(params, onComplete);
	}

	void OAuth2::refreshAccessToken(const String& refreshToken, const Function<void(AccessTokenResult&)>& onComplete)
	{
		refreshAccessToken(refreshToken, sl_null, onComplete);
	}

	sl_bool OAuth2::checkCodeVerifier(const String& challenge)
	{
		// Validate code_challenge according to RFC-7636
		// https://tools.ietf.org/html/rfc7636#section-4.2
		sl_size len = challenge.getLength();
		if (len < 43 || len > 128) {
			return sl_false;
		}
		sl_char8* sz = challenge.getData();
		for (sl_size i = 0; i < len; i++) {
			sl_char8 ch = sz[i];
			if (!(SLIB_CHAR_IS_ALNUM(ch) || ch == '-' || ch == '_' || ch == '.' || ch == '~')) {
				return sl_false;
			}
		}
		return sl_true;
	}

	String OAuth2::generateCodeChallenge(const String& verifier, CodeChallengeMethod method)
	{
		if (method == CodeChallengeMethod::S256) {
			char hash[32];
			SHA256::hash(verifier, hash);
			return Base64::encodeUrl(hash, 32);
		} else {
			return verifier;
		}
	}

	void OAuth2::logUrlRequestError(UrlRequest* request)
	{
		if (m_flagLogErrors) {
			LogError(TAG, "Error: %s, Status: %s, Response: %s", request->getErrorMessage(), HttpStatusHelper::toString(request->getResponseStatus()), request->getResponseContentAsString());
		}
	}

	void OAuth2::save()
	{
		if (m_preferenceName.isEmpty()) {
			return;
		}
		String key = "oauth2_" + m_preferenceName;
		Json value;
		Shared<AccessToken> accessToken = m_accessToken;
		if (accessToken.isNotNull()) {
			value.set(*accessToken);
		}
		Preference::setValue(key, value);
	}

	void OAuth2::restore()
	{
		if (m_preferenceName.isEmpty()) {
			return;
		}
		String key = "oauth2_" + m_preferenceName;
		Json value = Preference::getValue(key);
		if (value.isNotNull()) {
			AccessToken accessToken;
			FromJson(value, accessToken);
			m_accessToken = accessToken;
		}
	}

	void OAuth2::onReceiveAccessToken(AccessTokenResult& result)
	{
	}


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(OAuthApiResult)

	OAuthApiResult::OAuthApiResult(UrlRequest* _request)
	{
		flagSuccess = sl_false;
		request = _request;
		if (_request) {
			response = _request->getResponseContentAsJson();
		}
	}

}
