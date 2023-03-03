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

#include "slib/crypto/oauth_server.h"

#include "slib/core/safe_static.h"

namespace slib
{

	namespace {

		SLIB_STATIC_STRING(g_string_space, " ")

		SLIB_STATIC_STRING(g_field_tokenType, "t")
		SLIB_STATIC_STRING(g_field_clientId, "cid")
		SLIB_STATIC_STRING(g_field_scope, "scp")
		SLIB_STATIC_STRING(g_field_user, "usr")
		SLIB_STATIC_STRING(g_field_redirectUri, "dir")
		SLIB_STATIC_STRING(g_field_codeChallenge, "cc")
		SLIB_STATIC_STRING(g_field_codeChallengeMode, "cm")

		SLIB_STATIC_STRING(g_tokenType_access, "access")
		SLIB_STATIC_STRING(g_tokenType_refresh, "refresh")
		SLIB_STATIC_STRING(g_tokenType_code, "code")

	}


	OAuthServer::ClientEntity::ClientEntity()
	{
	}

	OAuthServer::ClientEntity::~ClientEntity()
	{
	}

	sl_bool OAuthServer::ClientEntity::validateSecret(const String& clientSecret)
	{
		return sl_true;
	}

	sl_bool OAuthServer::ClientEntity::validateRedirectUri(String& redirectUri)
	{
		return sl_true;
	}

	sl_bool OAuthServer::ClientEntity::validateScopes(List<String>& scopes)
	{
		return sl_true;
	}


	SLIB_DEFINE_NESTED_CLASS_DEFAULT_MEMBERS(OAuthServer, TokenPayload)

	OAuthServer::TokenPayload::TokenPayload()
	{
		grantType = GrantType::None;
		codeChallengeMethod = CodeChallengeMethod::Plain;
	}


	OAuthServer::TokenRepository::TokenRepository()
	{
	}

	OAuthServer::TokenRepository::~TokenRepository()
	{
	}


	OAuthServer::MemoryTokenRepository::MemoryTokenRepository()
	{
	}

	OAuthServer::MemoryTokenRepository::~MemoryTokenRepository()
	{
	}

	void OAuthServer::MemoryTokenRepository::registerToken(const String& code, const Json& data)
	{
		m_tokens.put(code, data);
	}

	void OAuthServer::MemoryTokenRepository::revokeToken(const String& code)
	{
		m_tokens.remove(code);
	}

	sl_bool OAuthServer::MemoryTokenRepository::isValid(const String& code)
	{
		return m_tokens.find(code);
	}

	Json OAuthServer::MemoryTokenRepository::getTokenData(const String& code)
	{
		return m_tokens.getValue(code);
	}


	SLIB_DEFINE_NESTED_CLASS_DEFAULT_MEMBERS(OAuthServer, AuthorizationRequest)

	OAuthServer::AuthorizationRequest::AuthorizationRequest()
	{
	}


	SLIB_DEFINE_OBJECT(OAuthServer, Object)

	OAuthServer::OAuthServer()
	{
		setSupportedImplicitGrant(sl_true);
		setSupportedAuthorizationCodeGrant(sl_true);
		setSupportedClientCredentialsGrant(sl_true);
		setSupportedPasswordGrant(sl_true);
		setSupportedRefreshToken(sl_true);

		setAccessTokenExpirySeconds(50000);
		setRefreshTokenExpirySeconds(1000000);
		setAuthorizationCodeExpirySeconds(3600);
	}

	OAuthServer::~OAuthServer()
	{
	}

	sl_bool OAuthServer::validateAuthorizationRequest(HttpServerContext* context, AuthorizationRequest& request)
	{
		String strResponseType = context->getParameter("response_type");
		sl_bool flagResponseTypeError = sl_false;
		if (strResponseType == "token") {
			request.responseType = ResponseType::Token;
		} else if (strResponseType == "code") {
			request.responseType = ResponseType::Code;
		} else {
			flagResponseTypeError = sl_true;
		}
		request.redirectUri = context->getParameter("redirect_uri");
		request.clientId = context->getParameter("client_id");
		request.scopes = context->getParameter("scope").split(" ");
		request.state = context->getParameter("state");

		request.codeChallenge = context->getParameter("code_challenge");
		String strCodeChallengeMethod = context->getParameter("code_challenge_method");
		sl_bool flagCodeChallengeMethodError = sl_false;
		if (strCodeChallengeMethod.isEmpty() || strCodeChallengeMethod == "plain") {
			request.codeChallengeMethod = CodeChallengeMethod::Plain;
		} else if (strCodeChallengeMethod == "S256") {
			request.codeChallengeMethod = CodeChallengeMethod::S256;
		} else {
			flagCodeChallengeMethodError = sl_true;
		}

		if (request.clientId.isEmpty()) {
			completeAuthorizationRequestWithError(context, request, ErrorCode::InvalidRequest, "client_id is not found");
			return sl_false;
		}
		if (flagResponseTypeError) {
			completeAuthorizationRequestWithError(context, request, ErrorCode::InvalidRequest, "response_type is invalid");
			return sl_false;
		}
		if (flagCodeChallengeMethodError) {
			completeAuthorizationRequestWithError(context, request, ErrorCode::InvalidRequest, "code_challenge_method is invalid");
			return sl_false;
		}
		if (request.codeChallenge.isNotEmpty()) {
			if (!(OAuth2::checkCodeVerifier(request.codeChallenge))) {
				completeAuthorizationRequestWithError(context, request, ErrorCode::InvalidRequest, "Code challenge must follow the specifications of RFC-7636.");
				return sl_false;
			}
		}

		if (request.responseType == ResponseType::Code) {
			if (!(isSupportedAuthorizationCodeGrant())) {
				completeAuthorizationRequestWithError(context, request, ErrorCode::UnsupportedGrantType);
				return sl_false;
			}
		} else {
			if (!(isSupportedImplicitGrant())) {
				completeAuthorizationRequestWithError(context, request, ErrorCode::UnsupportedGrantType);
				return sl_false;
			}
		}

		request.client = getClientEntity(request.clientId);
		ClientEntity* client = request.client.get();
		if (!client) {
			completeAuthorizationRequestWithError(context, request, ErrorCode::InvalidClient, "Client entity is not found");
			return sl_false;
		}

		if (!(validateRedirectUri(client, request.redirectUri))) {
			completeAuthorizationRequestWithError(context, request, ErrorCode::InvalidClient, "redirect_uri is not authorized for the client entity");
			return sl_false;
		}
		if (!(validateScopes(client, request.scopes))) {
			completeAuthorizationRequestWithError(context, request, ErrorCode::InvalidScope);
			return sl_false;
		}

		return sl_true;
	}

	void OAuthServer::completeAuthorizationRequest(HttpServerContext* context, const AuthorizationRequest& request, const Json& userEntity)
	{
		if (request.responseType == ResponseType::Token) {

			TokenPayload payload;
			payload.grantType = GrantType::Implicit;
			payload.client = request.client;
			payload.clientId = request.clientId;
			payload.user = userEntity;
			payload.scopes = request.scopes;

			setExpirySeconds(payload);
			issueAccessToken(payload);

			if (payload.accessToken.isEmpty()) {
				completeAuthorizationRequestWithError(context, request, ErrorCode::ServerError, "Failed to generate access token");
				return;
			}

			registerAccessToken(payload);
			if (payload.refreshToken.isNotEmpty()) {
				registerRefreshToken(payload);
			}

			HashMap<String, String> params = generateAccessTokenResponseParams(payload);
			if (request.state.isNotEmpty()) {
				params.add_NoLock("state", request.state);
			}
			context->setResponseRedirect(request.redirectUri, params);

		} else if (request.responseType == ResponseType::Code) {

			TokenPayload payload;
			payload.client = request.client;
			payload.clientId = request.clientId;
			payload.user = userEntity;
			payload.scopes = request.scopes;
			payload.redirectUri = request.redirectUri;
			payload.codeChallenge = request.codeChallenge;
			payload.codeChallengeMethod = request.codeChallengeMethod;

			setExpirySeconds(payload);
			issueAuthorizationCode(payload);

			if (payload.authorizationCode.isEmpty()) {
				completeAuthorizationRequestWithError(context, request, ErrorCode::ServerError, "Failed to generate authorization code");
				return;
			}

			registerAuthorizationCode(payload);

			CHashMap<String, String> params;
			params.add_NoLock("code", payload.authorizationCode);
			if (request.state.isNotEmpty()) {
				params.add_NoLock("state", request.state);
			}
			context->setResponseRedirect(request.redirectUri, params);
		}
	}

	void OAuthServer::completeAuthorizationRequestWithError(HttpServerContext* context, const AuthorizationRequest& request, ErrorCode err, const String& errorDescription, const String& errorUri)
	{
		String redirectUri = getRedirectUri(request);
		if (request.redirectUri.isNotEmpty()) {
			CHashMap<String, String> params;
			params.add_NoLock("error", getErrorCodeText(err));
			if (errorDescription.isNotEmpty()) {
				params.add_NoLock("error_description", errorDescription);
			}
			if (errorUri.isNotEmpty()) {
				params.add_NoLock("error_uri", errorUri);
			}
			if (request.state.isNotEmpty()) {
				params.add_NoLock("state", request.state);
			}
			context->setResponseRedirect(request.redirectUri, params);
		} else {
			respondError(context, err, errorDescription, errorUri, request.state);
		}
	}

	void OAuthServer::respondToAccessTokenRequest(HttpServerContext* context)
	{
		if (context->getRequestContentType().isEmpty()) {
			context->applyRequestBodyAsFormUrlEncoded();
		}
		String grantType = context->getParameter("grant_type");
		String clientId = getParameter(context, "client_id");
		String clientSecret = getParameter(context, "client_secret");
		List<String> scopes = context->getParameter("scope").split(" ");

		if (clientId.isEmpty()) {
			respondError(context, ErrorCode::InvalidRequest, "client_id is not found");
			return;
		}
		Ref<ClientEntity> refClient = getClientEntity(clientId);
		ClientEntity* client = refClient.get();
		if (!client) {
			respondError(context, ErrorCode::InvalidClient, "Client entity is not found");
			return;
		}
		if (clientSecret.isEmpty() || !(validateClientSecret(client, clientSecret))) {
			respondError(context, ErrorCode::InvalidClient, "Client authentication failed");
			return;
		}
		if (!(validateScopes(client, scopes))) {
			respondError(context, ErrorCode::InvalidScope);
			return;
		}

		TokenPayload payload;
		payload.client = client;
		payload.clientId = clientId;

		if (grantType == "authorization_code") {

			if (!(isSupportedAuthorizationCodeGrant())) {
				respondError(context, ErrorCode::UnsupportedGrantType);
				return;
			}

			String code = context->getParameter("code");
			if (code.isEmpty()) {
				respondError(context, ErrorCode::InvalidRequest, "Authorization code is not found");
				return;
			}

			Ref<TokenRepository> repo = getAuthorizationCodeRepository();
			if (repo.isNotNull()) {
				if (!(repo->isValid(code))) {
					respondError(context, ErrorCode::InvalidRequest, "Authorization code is revoked or invalid");
					return;
				}
			}

			payload.grantType = GrantType::AuthorizationCode;
			payload.authorizationCode = code;

			if (!(getAuthorizationCodePayload(payload))) {
				respondError(context, ErrorCode::InvalidRequest, "Authorization code is invalid");
				return;
			}

			if (payload.authorizationCodeExpirationTime.isNotZero() && payload.authorizationCodeExpirationTime < Time::now()) {
				respondError(context, ErrorCode::InvalidRequest, "Authorization code has expired");
				revokeAuthorizationCode(payload);
				return;
			}
			if (payload.clientId != clientId) {
				respondError(context, ErrorCode::InvalidRequest, "Authorization code was not issued to this client");
				return;
			}
			if (payload.redirectUri != context->getParameter("redirect_uri")) {
				respondError(context, ErrorCode::InvalidRequest, "redirect_uri is invalid");
				return;
			}
			ListElements<String> requiredScopes(scopes);
			for (sl_size i = 0; i < requiredScopes.count; i++) {
				if (!(payload.scopes.contains(requiredScopes[i]))) {
					respondError(context, ErrorCode::InvalidScope);
					return;
				}
			}

			String codeVerifier = context->getParameter("code_verifier");
			if (payload.codeChallenge.isNotEmpty() || codeVerifier.isNotEmpty()) {
				if (codeVerifier.isEmpty()) {
					respondError(context, ErrorCode::InvalidRequest, "code_verifier is not found");
					return;
				}
				if (payload.codeChallenge.isEmpty()) {
					respondError(context, ErrorCode::InvalidRequest, "Failed to verify code_verifier");
					return;
				}
				if (!(OAuth2::checkCodeVerifier(codeVerifier))) {
					respondError(context, ErrorCode::InvalidRequest, "code_verifier must follow the specifications of RFC-7636.");
					return;
				}
				if (payload.codeChallenge != OAuth2::generateCodeChallenge(codeVerifier, payload.codeChallengeMethod)) {
					respondError(context, ErrorCode::InvalidRequest, "Failed to verify code_verifier");
					return;
				}

			}

		} else if (grantType == "client_credentials") {

			if (!(isSupportedClientCredentialsGrant())) {
				respondError(context, ErrorCode::UnsupportedGrantType);
				return;
			}
			payload.grantType = GrantType::ClientCredentials;
			if (!(onClientCredentialsGrant(payload))) {
				respondError(context, ErrorCode::AccessDenied);
				return;
			}

		} else if (grantType == "password") {

			if (!(isSupportedPasswordGrant())) {
				respondError(context, ErrorCode::UnsupportedGrantType);
				return;
			}
			payload.grantType = GrantType::Password;
			String username = context->getParameter("username");
			if (username.isEmpty()) {
				respondError(context, ErrorCode::InvalidRequest, "username is not found");
				return;
			}
			String password = context->getParameter("password");
			if (password.isEmpty()) {
				respondError(context, ErrorCode::InvalidRequest, "password is not found");
				return;
			}
			if (!(onPasswordGrant(username, password, payload))) {
				respondError(context, ErrorCode::AccessDenied, "username or password is not match");
				return;
			}

		} else if (grantType == "refresh_token") {

			if (!(isSupportedRefreshToken())) {
				respondError(context, ErrorCode::UnsupportedGrantType);
				return;
			}

			payload.grantType = GrantType::RefreshToken;
			payload.refreshToken = context->getParameter("refresh_token");
			if (payload.refreshToken.isEmpty()) {
				respondError(context, ErrorCode::InvalidRequest, "refresh_token is not found");
				return;
			}

			Ref<TokenRepository> repo = getRefreshTokenRepository();
			if (repo.isNotNull()) {
				if (!(repo->isValid(payload.refreshToken))) {
					respondError(context, ErrorCode::InvalidRequest, "refresh_token is revoked or invalid");
					return;
				}
			}

			if (!(getRefreshTokenPayload(payload))) {
				respondError(context, ErrorCode::InvalidRequest, "refresh_token is invalid");
				return;
			}

			if (payload.refreshTokenExpirationTime.isNotZero() && payload.refreshTokenExpirationTime < Time::now()) {
				respondError(context, ErrorCode::InvalidRequest, "refresh_token has expired");
				revokeRefreshToken(payload);
				return;
			}
			if (payload.clientId != clientId) {
				respondError(context, ErrorCode::InvalidRequest, "refresh_token was not issued to this client");
				return;
			}
			ListElements<String> requiredScopes(scopes);
			for (sl_size i = 0; i < requiredScopes.count; i++) {
				if (!(payload.scopes.contains(requiredScopes[i]))) {
					respondError(context, ErrorCode::InvalidScope);
					return;
				}
			}

		} else {
			respondError(context, ErrorCode::InvalidGrant);
			return;
		}

		payload.scopes = scopes;

		setExpirySeconds(payload);
		issueAccessToken(payload);

		if (payload.accessToken.isEmpty()) {
			respondError(context, ErrorCode::ServerError, "Failed to generate access token");
			return;
		}

		registerAccessToken(payload);
		if (payload.refreshToken.isNotEmpty()) {
			registerRefreshToken(payload);
		}

		HashMap<String, String> params = generateAccessTokenResponseParams(payload);
		context->write(Json(params).toJsonString());
	}

	sl_bool OAuthServer::validateAccessToken(HttpServerContext* context, TokenPayload& payload)
	{
		String token = getAccessToken(context);
		if (token.isNotEmpty()) {
			Ref<TokenRepository> repo = getAccessTokenRepository();
			if (repo.isNotNull()) {
				if (!(repo->isValid(token))) {
					return sl_false;
				}
			}
			payload.accessToken = token;
			if (getAccessTokenPayload(payload)) {
				if (payload.accessTokenExpirationTime.isNotZero() && payload.accessTokenExpirationTime < Time::now()) {
					revokeAccessToken(payload);
					return sl_false;
				}
				return sl_true;
			}
		}
		return sl_false;
	}

	void OAuthServer::respondError(HttpServerContext* context, ErrorCode err, const StringView& errorDescription, const StringView& errorUri, const StringView& state)
	{
		HttpStatus status = HttpStatus::BadRequest;
		if (err == ErrorCode::InvalidClient || err == ErrorCode::UnauthorizedClient || err == ErrorCode::AccessDenied) {
			status = HttpStatus::Unauthorized;
		} else if (err == ErrorCode::ServerError) {
			status = HttpStatus::InternalServerError;
		} else if (err == ErrorCode::TemporarilyUnavailable) {
			status = HttpStatus::ServiceUnavailable;
		}

		context->setResponseCode(status);
		context->setResponseContentType(ContentType::Json);

		Json json;
		json.putItem("error", getErrorCodeText(err));
		if (errorDescription.isNotEmpty()) {
			json.putItem("error_description", errorDescription);
		}
		if (errorUri.isNotEmpty()) {
			json.putItem("error_uri", errorUri);
		}
		if (state.isNotEmpty()) {
			json.putItem("state", state);
		}
		context->write(json.toJsonString());
	}

	Ref<OAuthServer::ClientEntity> OAuthServer::getClientEntity(const String& clientId)
	{
		Ref<ClientEntity> client = new ClientEntity;
		if (client.isNotNull()) {
			client->clientId = clientId;
		}
		return client;
	}

	sl_bool OAuthServer::validateClientSecret(ClientEntity* client, const String& clientSecret)
	{
		return client->validateSecret(clientSecret);
	}

	sl_bool OAuthServer::validateRedirectUri(ClientEntity* client, String& redirectUri)
	{
		return client->validateRedirectUri(redirectUri);
	}

	sl_bool OAuthServer::validateScopes(ClientEntity* client, List<String>& scopes)
	{
		return client->validateScopes(scopes);
	}

	void OAuthServer::registerAccessToken(TokenPayload& payload)
	{
		if (payload.accessToken.isEmpty()) {
			return;
		}
		Ref<TokenRepository> repo = getAccessTokenRepository();
		if (repo.isNotNull()) {
			repo->registerToken(payload.accessToken, sl_null);
		}
	}

	void OAuthServer::revokeAccessToken(TokenPayload& payload)
	{
		if (payload.accessToken.isEmpty()) {
			return;
		}
		Ref<TokenRepository> repo = getAccessTokenRepository();
		if (repo.isNotNull()) {
			repo->revokeToken(payload.accessToken);
		}
	}

	void OAuthServer::registerRefreshToken(TokenPayload& payload)
	{
		if (payload.refreshToken.isEmpty()) {
			return;
		}
		Ref<TokenRepository> repo = getRefreshTokenRepository();
		if (repo.isNotNull()) {
			repo->registerToken(payload.refreshToken, sl_null);
		}
	}

	void OAuthServer::revokeRefreshToken(TokenPayload& payload)
	{
		if (payload.refreshToken.isEmpty()) {
			return;
		}
		Ref<TokenRepository> repo = getRefreshTokenRepository();
		if (repo.isNotNull()) {
			repo->revokeToken(payload.refreshToken);
		}
	}

	void OAuthServer::registerAuthorizationCode(TokenPayload& payload)
	{
		if (payload.authorizationCode.isEmpty()) {
			return;
		}
		Ref<TokenRepository> repo = getAuthorizationCodeRepository();
		if (repo.isNotNull()) {
			repo->registerToken(payload.authorizationCode, sl_null);
		}
	}

	void OAuthServer::revokeAuthorizationCode(TokenPayload& payload)
	{
		if (payload.authorizationCode.isEmpty()) {
			return;
		}
		Ref<TokenRepository> repo = getAuthorizationCodeRepository();
		if (repo.isNotNull()) {
			repo->revokeToken(payload.authorizationCode);
		}
	}

	sl_bool OAuthServer::onClientCredentialsGrant(TokenPayload& payload)
	{
		return sl_false;
	}

	sl_bool OAuthServer::onPasswordGrant(const String& username, const String& password, TokenPayload& payload)
	{
		return sl_false;
	}

	String OAuthServer::getRedirectUri(const AuthorizationRequest& request)
	{
		if (request.redirectUri.isNotEmpty()) {
			return request.redirectUri;
		} else {
			return getDefaultRedirectUri();
		}
	}

	void OAuthServer::setExpirySeconds(TokenPayload& payload)
	{
		payload.accessTokenExpirationTime = getExpiryTime(getAccessTokenExpirySeconds());
		payload.refreshTokenExpirationTime = getExpiryTime(getRefreshTokenExpirySeconds());
		payload.authorizationCodeExpirationTime = getExpiryTime(getAuthorizationCodeExpirySeconds());
	}

	HashMap<String, String> OAuthServer::generateAccessTokenResponseParams(const TokenPayload& payload)
	{
		HashMap<String, String> params;
		params.add_NoLock("access_token", payload.accessToken);
		params.add_NoLock("token_type", "Bearer");
		sl_int64 exp = (payload.accessTokenExpirationTime - Time::now()).getSecondCount();
		if (exp < 0) {
			exp = 0;
		}
		params.add_NoLock("expires_in", String::fromInt64(exp));
		if (payload.refreshToken.isNotEmpty()) {
			params.add_NoLock("refresh_token", payload.refreshToken);
		}
		params.add_NoLock("scope", String::join(payload.scopes, " "));
		return Move(params);
	}

	String OAuthServer::getErrorCodeText(ErrorCode err)
	{
		switch (err) {
			case ErrorCode::InvalidRequest:
				return "invalid_request";
			case ErrorCode::UnauthorizedClient:
				return "unauthorized_client";
			case ErrorCode::AccessDenied:
				return "access_denied";
			case ErrorCode::UnsupportedResponseType:
				return "unsupported_response_type";
			case ErrorCode::InvalidScope:
				return "invalid_scope";
			case ErrorCode::ServerError:
				return "server_error";
			case ErrorCode::TemporarilyUnavailable:
				return "temporarily_unavailable";
			case ErrorCode::InvalidClient:
				return "invalid_client";
			case ErrorCode::InvalidGrant:
				return "invalid_grant";
			case ErrorCode::UnsupportedGrantType:
				return "unsupported_grant_type";
			default:
				return "unknown";
		}
	}

	String OAuthServer::getParameter(HttpServerContext* context, const String& name)
	{
		if (context->containsRequestHeader(name)) {
			return context->getRequestHeader(name);
		}
		return context->getParameter(name);
	}

	String OAuthServer::getAccessToken(HttpServerContext* context)
	{
		String auth = getParameter(context, HttpHeader::Authorization);
		if (auth.startsWith("Bearer ")) {
			return auth.substring(7);
		}
		return sl_null;
	}

	Time OAuthServer::getExpiryTime(sl_uint32 seconds)
	{
		if (seconds) {
			return Time::now() + Time::withSeconds(seconds);
		} else {
			return Time::zero();
		}
	}


	SLIB_DEFINE_OBJECT(OAuthServerWithJwt, OAuthServer)

	OAuthServerWithJwt::OAuthServerWithJwt()
	{
		setAlgorithm(JwtAlgorithm::HS256);
	}

	OAuthServerWithJwt::~OAuthServerWithJwt()
	{
	}

	Memory& OAuthServerWithJwt::getMasterKey()
	{
		return m_masterKey;
	}

	void OAuthServerWithJwt::setMasterKey(const Memory& key)
	{
		m_masterKey = key;
	}

	void OAuthServerWithJwt::setMasterKey(const void* key, sl_size len)
	{
		m_masterKey = Memory::create(key, len);
	}

	void OAuthServerWithJwt::issueAccessToken(TokenPayload& payload)
	{
		payload.accessToken = generateToken(TokenType::Access, payload);
		if (isSupportedRefreshToken()) {
			payload.refreshToken = generateToken(TokenType::Refresh, payload);
		}
	}

	sl_bool OAuthServerWithJwt::getAccessTokenPayload(TokenPayload& payload)
	{
		return parseToken(payload.accessToken, payload) == TokenType::Access;
	}

	sl_bool OAuthServerWithJwt::getRefreshTokenPayload(TokenPayload& payload)
	{
		return parseToken(payload.refreshToken, payload) == TokenType::Refresh;
	}

	void OAuthServerWithJwt::issueAuthorizationCode(TokenPayload& payload)
	{
		payload.authorizationCode = generateToken(TokenType::AuthorizationCode, payload);
	}

	sl_bool OAuthServerWithJwt::getAuthorizationCodePayload(TokenPayload& payload)
	{
		return parseToken(payload.authorizationCode, payload) == TokenType::AuthorizationCode;
	}

	String OAuthServerWithJwt::generateToken(OAuthServerWithJwt::TokenType type, TokenPayload& payload)
	{
		Jwt jwt;
		jwt.setAlgorithm(getAlgorithm());
		if (type == TokenType::Refresh) {
			jwt.payload.putItem(g_field_tokenType, g_tokenType_refresh);
			jwt.setExpirationTime(payload.refreshTokenExpirationTime);
		} else if (type == TokenType::AuthorizationCode) {
			jwt.payload.putItem(g_field_tokenType, g_tokenType_code);
			jwt.setExpirationTime(payload.authorizationCodeExpirationTime);
			if (payload.codeChallenge.isNotEmpty()) {
				jwt.payload.putItem(g_field_codeChallenge, payload.codeChallenge);
				jwt.payload.putItem(g_field_codeChallengeMode, (int)(payload.codeChallengeMethod));
			}
			if (payload.redirectUri.isNotEmpty()) {
				jwt.payload.putItem(g_field_redirectUri, payload.redirectUri);
			}
		} else {
			jwt.payload.putItem(g_field_tokenType, g_tokenType_access);
			jwt.setExpirationTime(payload.accessTokenExpirationTime);
		}
		jwt.payload.putItem(g_field_clientId, payload.clientId);
		jwt.payload.putItem(g_field_user, payload.user);
		if (payload.scopes.isNotEmpty()) {
			jwt.payload.putItem(g_field_scope, String::join(payload.scopes, g_string_space));
		}
		return encrypt(jwt);
	}

	OAuthServerWithJwt::TokenType OAuthServerWithJwt::parseToken(const String& token, TokenPayload& payload)
	{
		Jwt jwt;
		if (decrypt(token, jwt)) {
			String strType = jwt.payload.getItem(g_field_tokenType).getString();
			payload.clientId = jwt.payload.getItem(g_field_clientId).getString();
			payload.user = jwt.payload.getItem(g_field_user);
			payload.scopes = jwt.payload.getItem(g_field_scope).getString().split(g_string_space);
			if (strType == g_tokenType_access) {
				payload.accessTokenExpirationTime = jwt.getExpirationTime();
				return TokenType::Access;
			} else if (strType == g_tokenType_refresh) {
				payload.refreshTokenExpirationTime = jwt.getExpirationTime();
				return TokenType::Refresh;
			} else if (strType == g_tokenType_code) {
				payload.authorizationCodeExpirationTime = jwt.getExpirationTime();
				payload.redirectUri = jwt.payload.getItem(g_field_redirectUri).getString();
				payload.codeChallenge = jwt.payload.getItem(g_field_codeChallenge).getString();
				payload.codeChallengeMethod = (CodeChallengeMethod)(jwt.payload.getItem(g_field_codeChallengeMode).getUint32());
				return TokenType::AuthorizationCode;
			}
		}
		return TokenType::None;
	}

	String OAuthServerWithJwt::encrypt(const Jwt& jwt)
	{
		return jwt.encode(m_masterKey);
	}

	sl_bool OAuthServerWithJwt::decrypt(const String& str, Jwt& jwt)
	{
		return jwt.decode(m_masterKey, str);
	}

}
