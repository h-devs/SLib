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

#ifndef CHECKHEADER_SLIB_CRYPTO_OAUTH_SERVER
#define CHECKHEADER_SLIB_CRYPTO_OAUTH_SERVER

#include "oauth.h"
#include "jwt.h"

#include "../network/http_server.h"
#include "../core/property.h"

namespace slib
{

	class SLIB_EXPORT OAuthServer : public Object
	{
		SLIB_DECLARE_OBJECT

	public:
		typedef OAuth2::GrantType GrantType;
		typedef OAuth2::CodeChallengeMethod CodeChallengeMethod;
		typedef OAuth2::ResponseType ResponseType;
		typedef OAuth2::ErrorCode ErrorCode;

		class ClientEntity : public Object
		{
		public:
			ClientEntity();
			~ClientEntity();

		public:
			String clientId;

		public:
			virtual sl_bool validateSecret(const String& clientSecret);
			virtual sl_bool validateRedirectUri(String& redirectUri);
			virtual sl_bool validateScopes(List<String>& scopes);
		};

		class TokenPayload
		{
		public:
			GrantType grantType;
			Ref<ClientEntity> client;
			String clientId;

			Json user;

			List<String> scopes;

			String accessToken;
			Time accessTokenExpirationTime;

			String refreshToken;
			Time refreshTokenExpirationTime;

			// payload for authorization code
			String redirectUri;
			String codeChallenge;
			CodeChallengeMethod codeChallengeMethod;

			String authorizationCode;
			Time authorizationCodeExpirationTime;

		public:
			TokenPayload();
			SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(TokenPayload)
		};

		class TokenRepository : public Object
		{
		public:
			TokenRepository();
			~TokenRepository();

		public:
			virtual void registerToken(const String& token, const Json& data) = 0;
			virtual void revokeToken(const String& token) = 0;
			virtual sl_bool isValid(const String& token) = 0;
			virtual Json getTokenData(const String& token) = 0;
		};

		class MemoryTokenRepository : public TokenRepository
		{
		public:
			MemoryTokenRepository();
			~MemoryTokenRepository();

		public:
			void registerToken(const String& token, const Json& data) override;
			void revokeToken(const String& token) override;
			sl_bool isValid(const String& token) override;
			Json getTokenData(const String& token) override;

		protected:
			HashMap<String, Json> m_tokens;
		};

	public:
		OAuthServer();

		~OAuthServer();

	public:
		class SLIB_EXPORT AuthorizationRequest : public OAuth2::AuthorizationRequest
		{
		public:
			Ref<ClientEntity> client;

		public:
			AuthorizationRequest();
			SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(AuthorizationRequest)
		};

		sl_bool validateAuthorizationRequest(HttpServerContext* context, AuthorizationRequest& _out);

		void completeAuthorizationRequest(HttpServerContext* context, const AuthorizationRequest& request, const Json& userEntity);

		void completeAuthorizationRequestWithError(HttpServerContext* context, const AuthorizationRequest& request, ErrorCode err, const String& errorDescription = String::null(), const String& errorUri = String::null());

		void respondToAccessTokenRequest(HttpServerContext* context);

		sl_bool validateAccessToken(HttpServerContext* context, TokenPayload& payload);

		static void respondError(HttpServerContext* context, ErrorCode err, const StringView& errorDescription = sl_null, const StringView& errorUri = sl_null, const StringView& state = sl_null);

	public:
		virtual Ref<ClientEntity> getClientEntity(const String& clientId);

		virtual sl_bool validateClientSecret(ClientEntity* client, const String& clientSecret);

		virtual sl_bool validateRedirectUri(ClientEntity* client, String& redirectUri);

		virtual sl_bool validateScopes(ClientEntity* client, List<String>& scopes);


		virtual void issueAccessToken(TokenPayload& payload) = 0;

		virtual sl_bool getAccessTokenPayload(TokenPayload& payload) = 0;

		virtual sl_bool getRefreshTokenPayload(TokenPayload& payload) = 0;

		virtual void issueAuthorizationCode(TokenPayload& payload) = 0;

		virtual sl_bool getAuthorizationCodePayload(TokenPayload& payload) = 0;


		virtual void registerAccessToken(TokenPayload& payload);

		virtual void revokeAccessToken(TokenPayload& payload);

		virtual void registerRefreshToken(TokenPayload& payload);

		virtual void revokeRefreshToken(TokenPayload& payload);

		virtual void registerAuthorizationCode(TokenPayload& payload);

		virtual void revokeAuthorizationCode(TokenPayload& payload);


		virtual sl_bool onClientCredentialsGrant(TokenPayload& payload);

		virtual sl_bool onPasswordGrant(const String& username, const String& password, TokenPayload& payload);

	public:
		String getRedirectUri(const AuthorizationRequest& request);

		void setExpirySeconds(TokenPayload& payload);

		static HashMap<String, String> generateAccessTokenResponseParams(const TokenPayload& payload);

		static String getErrorCodeText(ErrorCode err);

		static String getParameter(HttpServerContext* context, const String& name);

		static String getAccessToken(HttpServerContext* context);

		static Time getExpiryTime(sl_uint32 seconds);

	public:
		SLIB_PROPERTY(AtomicString, DefaultRedirectUri)

		SLIB_BOOLEAN_PROPERTY(SupportedImplicitGrant)
		SLIB_BOOLEAN_PROPERTY(SupportedAuthorizationCodeGrant)
		SLIB_BOOLEAN_PROPERTY(SupportedClientCredentialsGrant)
		SLIB_BOOLEAN_PROPERTY(SupportedPasswordGrant)
		SLIB_BOOLEAN_PROPERTY(SupportedRefreshToken)

		SLIB_PROPERTY(sl_uint32, AccessTokenExpirySeconds)
		SLIB_PROPERTY(sl_uint32, RefreshTokenExpirySeconds)
		SLIB_PROPERTY(sl_uint32, AuthorizationCodeExpirySeconds)

		SLIB_PROPERTY(AtomicRef<TokenRepository>, AccessTokenRepository)
		SLIB_PROPERTY(AtomicRef<TokenRepository>, AuthorizationCodeRepository)
		SLIB_PROPERTY(AtomicRef<TokenRepository>, RefreshTokenRepository)

	};

	class SLIB_EXPORT OAuthServerWithJwt : public OAuthServer
	{
		SLIB_DECLARE_OBJECT

	public:
		OAuthServerWithJwt();

		~OAuthServerWithJwt();

	public:
		Memory& getMasterKey();

		void setMasterKey(const Memory& key);

		void setMasterKey(const void* key, sl_size len);

		void issueAccessToken(TokenPayload& payload) override;

		sl_bool getAccessTokenPayload(TokenPayload& payload) override;

		sl_bool getRefreshTokenPayload(TokenPayload& payload) override;

		void issueAuthorizationCode(TokenPayload& payload) override;

		sl_bool getAuthorizationCodePayload(TokenPayload& payload) override;

	protected:
		enum class TokenType {
			None, Access, Refresh, AuthorizationCode
		};

		String generateToken(TokenType type, TokenPayload& payload);

		TokenType parseToken(const String& token, TokenPayload& payload);

		virtual String encrypt(const Jwt& jwt);

		virtual sl_bool decrypt(const String& str, Jwt& jwt);

	public:
		SLIB_PROPERTY(JwtAlgorithm, Algorithm)

	protected:
		Memory m_masterKey;

	};

	class SLIB_EXPORT OAuthServerWithJwtAndOpenSSL : public OAuthServerWithJwt
	{
		SLIB_DECLARE_OBJECT

	public:
		OAuthServerWithJwtAndOpenSSL();

		~OAuthServerWithJwtAndOpenSSL();

	public:
		Ref<OpenSSL_Key>& getPrivateKey();

		void setPrivateKey(const Ref<OpenSSL_Key>& key);

		void setPrivateKey(const String& pem);

		Ref<OpenSSL_Key>& getPublicKey();

		void setPublicKey(const Ref<OpenSSL_Key>& key);

		void setPublicKey(const String& pem);

	protected:
		String encrypt(const Jwt& jwt) override;

		sl_bool decrypt(const String& str, Jwt& jwt) override;

	protected:
		Ref<OpenSSL_Key> m_publicKey;
		Ref<OpenSSL_Key> m_privateKey;

	};

}

#endif
