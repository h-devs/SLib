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

#include <slib/core/definition.h>

#if defined(SLIB_PLATFORM_IS_ANDROID)

#include "facebook.h"

#include <slib/core/safe_static.h>
#include <slib/ui/core.h>
#include <slib/ui/platform.h>

namespace slib
{

	namespace {

		SLIB_JNI_BEGIN_CLASS(JToken, "slib/android/facebook/FacebookToken")
			SLIB_JNI_STRING_FIELD(token)
			SLIB_JNI_STRING_FIELD(scopes)
			SLIB_JNI_LONG_FIELD(expirationTime)
			SLIB_JNI_LONG_FIELD(refreshTime)
		SLIB_JNI_END_CLASS

		static void OnLoginResult(JNIEnv* env, jobject _this, jobject token, jboolean flagCancel);
		static void OnShareResult(JNIEnv* env, jobject _this, jboolean flagSuccess, jboolean flagCancel);

		SLIB_JNI_BEGIN_CLASS(JFacebook, "slib/android/facebook/Facebook")
			SLIB_JNI_STATIC_METHOD(initialize, "initialize", "()V");
			SLIB_JNI_STATIC_METHOD(getCurrentToken, "getCurrentToken", "()Lslib/android/facebook/FacebookToken;");
			SLIB_JNI_STATIC_METHOD(clearAccessToken, "clearAccessToken", "()V");

			SLIB_JNI_STATIC_METHOD(login, "login", "(Landroid/app/Activity;Ljava/lang/String;)V");
			SLIB_JNI_NATIVE(nativeOnLoginResult, "nativeOnLoginResult", "(Lslib/android/facebook/FacebookToken;Z)V", OnLoginResult);

			SLIB_JNI_STATIC_METHOD(share, "share", "(Landroid/app/Activity;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V");
			SLIB_JNI_NATIVE(nativeOnShareResult, "nativeOnShareResult", "(ZZ)V", OnShareResult);
		SLIB_JNI_END_CLASS

		class FacebookSDKContext
		{
		public:
			Function<void(Facebook::LoginResult&)> onLoginResult;
			Function<void(Facebook::ShareResult&)> onShareResult;

		public:
			static FacebookSDKContext* get()
			{
				SLIB_SAFE_LOCAL_STATIC(FacebookSDKContext, s)
				if (SLIB_SAFE_STATIC_CHECK_FREED(s)) {
					return sl_null;
				}
				return &s;
			}

			static void getToken(OAuth2::AccessToken& _out, jobject _in)
			{
				_out.token = JToken::token.get(_in);
				_out.scopes = String(JToken::scopes.get(_in)).split(",");
				_out.expirationTime = Time::withMilliseconds(JToken::expirationTime.get(_in));
				_out.refreshTime = Time::withMilliseconds(JToken::refreshTime.get(_in));
			}

		};

		void OnLoginResult(JNIEnv* env, jobject _this, jobject token, jboolean flagCancel)
		{
			auto p = FacebookSDKContext::get();
			if (p) {
				Facebook::LoginResult result;
				if (token) {
					FacebookSDKContext::getToken(result.accessToken, token);
					result.flagSuccess = sl_true;
				} else {
					if (flagCancel) {
						result.flagCancel = sl_true;
					}
				}
				p->onLoginResult(result);
				p->onLoginResult.setNull();
			}
		}

		void OnShareResult(JNIEnv* env, jobject _this, jboolean flagSuccess, jboolean flagCancel)
		{
			auto p = FacebookSDKContext::get();
			if (p) {
				Facebook::ShareResult result;
				if (flagSuccess) {
					result.flagSuccess = flagSuccess;
				} else {
					if (flagCancel) {
						result.flagCancel = sl_true;
					}
				}
				p->onShareResult(result);
				p->onShareResult.setNull();
			}
		}

	}

	void FacebookSDK::initialize()
	{
		JFacebook::initialize.call(sl_null);
	}

	void FacebookSDK::_updateCurrentToken(Facebook* instance)
	{
		auto p = FacebookSDKContext::get();
		if (p) {
			JniLocal<jobject> token = JFacebook::getCurrentToken.callObject(sl_null);
			if (token.get()) {
				OAuth2::AccessToken oauthToken;
				FacebookSDKContext::getToken(oauthToken, token.get());
				instance->setAccessToken(oauthToken);
			}
		}
	}

	void FacebookSDK::login(const Facebook::LoginParam& param)
	{
		if (!(UI::isUiThread())) {
			UI::dispatchToUiThread([param]() {
				FacebookSDK::login(param);
			});
			return;
		}
		auto p = FacebookSDKContext::get();
		if (p) {
			jobject context = Android::getCurrentContext();
			if (context) {
				if (p->onLoginResult.isNotNull()) {
					Facebook::LoginResult result;
					p->onLoginResult(result);
				}
				p->onLoginResult = param.onComplete;
				String scopes = String::join(param.authorization.scopes, ",");
				JniLocal<jstring> jscopes = Jni::getJniString(scopes);
				JFacebook::login.call(sl_null, context, jscopes.get());
				return;
			}
		}
		Facebook::LoginResult result;
		param.onComplete(result);
	}

	void FacebookSDK::share(const Facebook::ShareParam& param)
	{
		auto p = FacebookSDKContext::get();
		if (!p) {
			Facebook::ShareResult result;
			param.onComplete(result);
			return;
		}
		if (param.url.isEmpty()) {
			Facebook::ShareResult result;
			param.onComplete(result);
			return;
		}
		if (!(UI::isUiThread())) {
			UI::dispatchToUiThread([param]() {
				FacebookSDK::share(param);
			});
			return;
		}
		jobject context = Android::getCurrentContext();
		if (!context) {
			Facebook::ShareResult result;
			param.onComplete(result);
			return;
		}
		if (p->onShareResult.isNotNull()) {
			Facebook::ShareResult result;
			p->onShareResult(result);
		}
		p->onShareResult = param.onComplete;

		JniLocal<jstring> jurl = Jni::getJniString(param.url);
		JniLocal<jstring> jquote = Jni::getJniString(param.quote);
		JniLocal<jstring> jhashTag = Jni::getJniString(param.hashTag);
		JFacebook::share.call(sl_null, context, jurl.get(), jquote.get(), jhashTag.get());
	}

	void FacebookSDK::clearAccessToken()
	{
		JFacebook::clearAccessToken.call(sl_null);
	}

}

#endif
