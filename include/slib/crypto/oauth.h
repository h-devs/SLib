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

#ifndef CHECKHEADER_SLIB_CRYPTO_OAUTH
#define CHECKHEADER_SLIB_CRYPTO_OAUTH

/**************************************************************************
 
 							OAuth 1.0
 				https://tools.ietf.org/html/rfc5849
 
 							OAuth 2.0
				https://tools.ietf.org/html/rfc6749
				https://tools.ietf.org/html/rfc6750
 
**************************************************************************/

#include "definition.h"

#include "../network/url_request.h"
#include "../core/shared.h"

namespace slib
{

	class Window;
	class WebView;

	class SLIB_EXPORT OAuthWebRedirectDialogOptions
	{
	public:
		Ref<Window> parentWindow;

		String title;
		sl_uint32 width;
		sl_uint32 height;

	public:
		OAuthWebRedirectDialogOptions();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(OAuthWebRedirectDialogOptions)

	};

	class SLIB_EXPORT OAuthWebRedirectDialogParam
	{
	public:
		String url;
		OAuthWebRedirectDialogOptions options;

		Function<void(const String& url)> onRedirect;

	public:
		OAuthWebRedirectDialogParam();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(OAuthWebRedirectDialogParam)

	};

	class SLIB_EXPORT OAuthWebRedirectDialog
	{
	public:
		OAuthWebRedirectDialog();

		~OAuthWebRedirectDialog();

	public:
		virtual Ref<WebView> getWebView() = 0;

		virtual void show(const OAuthWebRedirectDialogParam& param) = 0;

		virtual void close() = 0;

	public:
		static Ptr<OAuthWebRedirectDialog> getDefault();

		static void showDefault(const OAuthWebRedirectDialogParam& param);

	};


	/*
	 	`OAuth1` authentication is not recommended to be used in unconfidental clients
	 	(such as mobile/desktop app), because you can't keep secret the `consumer secret` in such clients.
	 	`TwitterKit`(now discontinued) also saves `consumer secret` in client app, but this is not
	 	good practice.
	 	Instead, you should use the authentication functions in your own API server.
	 	You can get `login url` from your server using `getLoginUrl()`.
	 	After getting `login url`, you can call `login()` function in client app (put the `login url`
	 	in the param), and then you can get `verifier` in callback after user-web-login process.
	 	After getting `verifier`, you should send it to your server, and you can call
	 	`requestAccessToken` in the server. Your server should return the `access token` to the client,
	 	and then your client app can call Twitter APIs after setting `access token` by `setAccessToken()`.
	 	Btw, `OAuth1` supports complete authentication flow, but you should follow above instruction for security.
	 	your Twitter
	*/
	class SLIB_EXPORT OAuth1 : public Object
	{
		SLIB_DECLARE_OBJECT

	public:
		class AccessToken
		{
		public:
			String token;
			String secret;

		public:
			AccessToken();
			AccessToken(const String& token, const String& tokenSecret);

			SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(AccessToken)
			SLIB_DECLARE_JSON

		public:
			sl_bool isValid() const;

			void setResponse(const HashMap<String, String>& params);
		};

		class Param
		{
		public:
			String consumerKey;
			String consumerSecret;

			AccessToken accessToken;

			HttpMethod requestTokenMethod;
			String requestTokenUrl;
			String authenticateUrl;
			HttpMethod accessTokenMethod;
			String accessTokenUrl;
			String callbackUrl;

			String preferenceName;

		public:
			Param();
			SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(Param)
		};

	public:
		OAuth1(const Param& param);

		~OAuth1();

	public:
		Shared<AccessToken> getAccessToken();

		void setAccessToken(const AccessToken& accessToken);

		void setAccessToken(const String& token, const String& tokenSecret);

		void clearAccessToken();

		String getAccessTokenKey();

		String getAccessTokenSecret();

		String getCallbackUrl();

		void setCallbackUrl(const String& url);

		void setLoggingErrors(sl_bool flag);


		String generateAuthorization(HttpMethod method, const String& url, HashMap<String, String>& parameters, const String& nonce, sl_int64 timestamp, const String& token, const String& tokenSecret, const String& callbackUrl);

		virtual void authorizeRequest(UrlRequestParam& param, const String& token, const String& tokenSecret, const String& callbackUrl);

		void authorizeRequest(UrlRequestParam& param);

		void getLoginUrl(const Function<void(const String& url, const String& requestToken, const String& requestTokenSecret)>& onComplete);

		class AccessTokenResult
		{
		public:
			sl_bool flagSuccess;
			AccessToken accessToken;
			HashMap<String, String> response;

		public:
			AccessTokenResult();
			SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(AccessTokenResult)

		public:
			void setResponse(const HashMap<String, String>& params);
		};

		void requestAccessToken(const String& verifier, const String& requestToken, const String& requestTokenSecret, const Function<void(AccessTokenResult&)>& onComplete);

		class AuthorizationRequestParam
		{
		public:
			String callbackUrl; // If empty, uses instance's callbackUrl attribute
			VariantMap customParameters;

		public:
			AuthorizationRequestParam();
			SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(AuthorizationRequestParam)
		};

		virtual void getLoginUrl(const AuthorizationRequestParam& param, const Function<void(const String& url, const String& requestToken, const String& requestTokenSecret)>& onComplete);

		class LoginResult : public AccessTokenResult
		{
		public:
			sl_bool flagCancel;
			sl_bool flagCache; // True if the `access token` is from cache

			String verifier;
			String requestToken;

		public:
			LoginResult();
			SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(LoginResult)

		public:
			void parseRedirectUrl(const String& url);
		};

		class LoginParam
		{
		public:
			String url; // Should be defined in unconfidental clients (such as mobile/desktop app). See class description for detail.

			AuthorizationRequestParam authorization;

			OAuthWebRedirectDialogOptions dialogOptions;
			Ptr<OAuthWebRedirectDialog> dialog;

			sl_bool flagIgnoreExistingAccessToken; // Ignored if `url` is not empty

			Function<void(LoginResult&)> onComplete;

		public:
			LoginParam();
			SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(LoginParam)
		};

		void login(const LoginParam& param);

		void login(const Function<void(LoginResult& result)>& onComplete);

	protected:
		void logUrlRequestError(UrlRequest* request);

		void save();

		void restore();

	protected:
		String m_consumerKey;
		String m_consumerSecret;

		AtomicShared<AccessToken> m_accessToken;

		HttpMethod m_requestTokenMethod;
		String m_requestTokenUrl;
		String m_authenticateUrl;
		HttpMethod m_accessTokenMethod;
		String m_accessTokenUrl;
		AtomicString m_callbackUrl;

		sl_bool m_flagLogErrors;

		String m_preferenceName;

	};

	typedef OAuth1::Param OAuth1_Param;


	class SLIB_EXPORT OAuth2 : public Object
	{
		SLIB_DECLARE_OBJECT

	public:
		enum class GrantType
		{
			None = 0,
			Implicit = 1,
			AuthorizationCode = 2,
			ClientCredentials = 3,
			Password = 4,
			RefreshToken = 5
		};

		enum class ResponseType
		{
			Token = 0,
			Code = 1
		};

		enum class CodeChallengeMethod
		{
			Plain = 0,
			S256 = 1 // SHA256
		};

		enum class ErrorCode
		{
			None = 0,

			InvalidRequest = 0x0001, // The request is missing a required parameter, includes an invalid parameter value, includes a parameter more than once, or is otherwise malformed.
			UnauthorizedClient = 0x0002, // The client is not authorized to request an authorization code using this method.
			AccessDenied = 0x0003, // The resource owner or authorization server denied the request.
			UnsupportedResponseType = 0x0004, // The authorization server does not support obtaining an authorization code using this method.
			InvalidScope = 0x0005, // The requested scope is invalid, unknown, or malformed.
			ServerError = 0x0006, // The authorization server encountered an unexpected condition that prevented it from fulfilling the request. (This error code is needed because a 500 Internal Server Error HTTP status code cannot be returned to the client via an HTTP redirect.)
			TemporarilyUnavailable = 0x0007, // The authorization server is currently unable to handle the request due to a temporary overloading or maintenance of the server. (This error code is needed because a 503 Service Unavailable HTTP status code cannot be returned to the client via an HTTP redirect.)

			InvalidClient = 0x0011, // Client authentication failed (e.g., unknown client, no client authentication included, or unsupported authentication method).
			InvalidGrant = 0x0012, // The provided authorization grant (e.g., authorization code, resource owner credentials) or refresh token is invalid, expired, revoked, does not match the redirection URI used in the authorization request, or was issued to another client.
			UnsupportedGrantType = 0x0013 // The authorization grant type is not supported by the authorization server.
		};

		class AccessToken
		{
		public:
			String token;
			String refreshToken;
			String tokenType;
			List<String> scopes;
			Time expirationTime;
			Time refreshTime;

		public:
			AccessToken();
			AccessToken(const String& token);

			SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(AccessToken)
			SLIB_DECLARE_JSON

		public:
			sl_bool isValid() const;
			sl_bool isValid(const List<String>& requiredScopes) const;

			void setResponse(const Json& json);
		};

		class Result
		{
		public:
			sl_bool flagSuccess;

			String error;
			String errorDescription;
			String errorUri;
			ErrorCode errorCode;

			Json response;

		public:
			Result();
			SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(Result)

		public:
			void setResponse(const Json& json);
			void setResult(UrlRequest* req);
		};

		class Param
		{
		public:
			String clientId;
			String clientSecret;

			AccessToken accessToken;

			String authorizeUrl;
			HttpMethod accessTokenMethod;
			sl_bool flagUseBasicAuthorizationForAccessToken;
			String accessTokenUrl;
			String redirectUri;
			String loginRedirectUri;
			List<String> defaultScopes;
			sl_bool flagSupportImplicitGrantType;
			String clientIdFieldName;
			String clientSecretFieldName;

			String preferenceName;

			sl_bool flagLoggingErrors;

		public:
			Param();
			SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(Param)
		};

	public:
		OAuth2(const Param& param);

		~OAuth2();

	public:
		Shared<AccessToken> getAccessToken();

		void setAccessToken(const AccessToken& accessToken);

		void setAccessToken(const String& accessToken);

		void clearAccessToken();

		String getAccessTokenKey();

		List<String> getAccessTokenScopes();

		Json getAccessTokenAttribute(const String& key);

		void setLoggingErrors(sl_bool flag);


		virtual void authorizeRequest(UrlRequestParam& param, const AccessToken& token);

		void authorizeRequest(UrlRequestParam& param);

		class AuthorizationRequest
		{
		public:
			String clientId;
			ResponseType responseType;
			List<String> scopes;
			String state;
			String redirectUri; // If empty, uses instance's `redirectUri` attribute

			String codeVerifier;
			String codeChallenge;
			CodeChallengeMethod codeChallengeMethod;

			VariantMap customParameters;

		public:
			AuthorizationRequest();
			SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(AuthorizationRequest)
		};

		virtual String getLoginUrl(const AuthorizationRequest& param);

		String getLoginUrl(ResponseType type, const List<String>& scopes = List<String>::null(), const String& state = String::null());

		String getLoginUrl(const List<String>& scopes = List<String>::null(), const String& state = String::null());

		class AccessTokenResult : public Result
		{
		public:
			AccessToken accessToken;

		public:
			AccessTokenResult();
			SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(AccessTokenResult)

		public:
			void setResult(UrlRequest* req);
		};

		void requestAccessToken(VariantMap& params, const Function<void(AccessTokenResult&)>& onComplete);

		// grant_type=authorization_code
		void requestAccessTokenFromCode(const String& code, const String& redirectUri, const String& codeVerifier, const List<String>& scopes, const Function<void(AccessTokenResult&)>& onComplete);

		// grant_type=authorization_code
		void requestAccessTokenFromCode(const String& code, const String& redirectUri, const String& codeVerifier, const Function<void(AccessTokenResult&)>& onComplete);

		// grant_type=authorization_code
		void requestAccessTokenFromCode(const String& code, const String& redirectUri, const Function<void(AccessTokenResult&)>& onComplete);

		// grant_type=authorization_code
		void requestAccessTokenFromCode(const String& code, const Function<void(AccessTokenResult&)>& onComplete);

		// grant_type=client_credentials
		void requestAccessTokenFromClientCredentials(const List<String>& scopes, const Function<void(AccessTokenResult&)>& onComplete);

		// grant_type=client_credentials
		void requestAccessTokenFromClientCredentials(const Function<void(AccessTokenResult&)>& onComplete);

		// grant_type=password
		void requestAccessTokenFromUserPassword(const String& username, const String& password, const List<String>& scopes, const Function<void(AccessTokenResult&)>& onComplete);

		// grant_type=client_credentials
		void requestAccessTokenFromUserPassword(const String& username, const String& password, const Function<void(AccessTokenResult&)>& onComplete);

		// grant_type=refresh_token
		void refreshAccessToken(const String& refreshToken, const List<String>& scopes, const Function<void(AccessTokenResult&)>& onComplete);

		// grant_type=refresh_token
		void refreshAccessToken(const String& refreshToken, const Function<void(AccessTokenResult&)>& onComplete);

		class LoginResult : public AccessTokenResult
		{
		public:
			sl_bool flagCancel;
			sl_bool flagCache; // Used in `Token` Grant Type. True if the `access token` is from cache

			String code; // Used in `Code` Grant Type
			String state;

		public:
			LoginResult();
			SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(LoginResult)

		public:
			void parseRedirectUrl(const String& url);
		};

		class LoginParam
		{
		public:
			String url; // Used in process of `Code` grant type

			AuthorizationRequest authorization;

			OAuthWebRedirectDialogOptions dialogOptions;
			Ptr<OAuthWebRedirectDialog> dialog;

			sl_bool flagIgnoreExistingAccessToken; // Ignored if `url` is not empty
			sl_bool flagAlwaysRequireAccessToken; // Should not be set on unconfidental clients (Mobile/Desktop apps)
			String loginRedirectUri; // If empty, uses `authorization`'s `redirectUri`

			Function<void(LoginResult&)> onComplete;

		public:
			LoginParam();
			SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(LoginParam)
		};

		void login(const LoginParam& param);

		void login(const Function<void(LoginResult& result)>& onComplete);


		static sl_bool checkCodeVerifier(const String& str);

		static String generateCodeChallenge(const String& verifier, CodeChallengeMethod method);

	protected:
		void logUrlRequestError(UrlRequest* request);

		void save();

		void restore();

		virtual void onReceiveAccessToken(AccessTokenResult& result);

	public:
		String m_clientId;
		String m_clientSecret;
		String m_preferenceName;

		AtomicShared<AccessToken> m_accessToken;

		String m_authorizeUrl;
		String m_accessTokenUrl;
		HttpMethod m_accessTokenMethod;
		sl_bool m_flagUseBasicAuthorizationForAccessToken;
		String m_redirectUri;
		String m_loginRedirectUri;
		List<String> m_defaultScopes;
		sl_bool m_flagSupportImplicitGrantType;
		String m_clientIdFieldName;
		String m_clientSecretFieldName;

		sl_bool m_flagLogErrors;

	};

	typedef OAuth2::Param OAuth2_Param;

	class SLIB_EXPORT OAuthApiResult
	{
	public:
		sl_bool flagSuccess;
		UrlRequest* request;
		Json response;

	public:
		OAuthApiResult(UrlRequest*);

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(OAuthApiResult)

	};

}

#endif
