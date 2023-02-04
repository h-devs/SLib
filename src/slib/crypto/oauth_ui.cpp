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

#include "slib/ui/mobile_app.h"
#include "slib/ui/view_page.h"
#include "slib/ui/web_view.h"
#include "slib/ui/button.h"
#include "slib/ui/resource.h"
#include "slib/core/log.h"

#include "../resources.h"

#define TAG "OAuth"

namespace slib
{

	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(OAuthWebRedirectDialogOptions)

	OAuthWebRedirectDialogOptions::OAuthWebRedirectDialogOptions():
		width(800),
		height(600)
	{
	}

	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(OAuthWebRedirectDialogParam)

	OAuthWebRedirectDialogParam::OAuthWebRedirectDialogParam()
	{
	}

	OAuthWebRedirectDialog::OAuthWebRedirectDialog()
	{
	}

	OAuthWebRedirectDialog::~OAuthWebRedirectDialog()
	{
	}

	namespace {
		class DefaultOAuthWebRedirectDialogImpl : public ViewPage, public OAuthWebRedirectDialog
		{
		public:
			Ref<WebView> m_webView;
			WeakRef<Window> m_window;

		public:
			void init() override
			{
				ViewPage::init();

#ifdef SLIB_PLATFORM_IS_MOBILE
				sl_real fontSize = (sl_real)(UIResource::getScreenMinimum() / 20);
				Ref<Button> btnCancel = new Button;
				btnCancel->setCancelOnClick();
				btnCancel->setText(::slib::string::cancel::get(), UIUpdateMode::Init);
				btnCancel->setAlignParentLeft(UIUpdateMode::Init);
				btnCancel->setAlignParentTop(UIUpdateMode::Init);
				btnCancel->setWidthWrapping(UIUpdateMode::Init);
				btnCancel->setHeightWrapping(UIUpdateMode::Init);
				btnCancel->setMarginTop(UIResource::getSafeAreaInsetTop(), UIUpdateMode::Init);
				btnCancel->setFontSize(fontSize, UIUpdateMode::Init);
				btnCancel->setPadding((sl_ui_pos)(fontSize / 3), UIUpdateMode::Init);
				addChild(btnCancel, UIUpdateMode::Init);
#endif

				m_webView = new WebView;
				m_webView->setAlignParentLeft(UIUpdateMode::Init);
#ifdef SLIB_PLATFORM_IS_MOBILE
				m_webView->setBelow(btnCancel, UIUpdateMode::Init);
#else
				m_webView->setAlignParentTop(UIUpdateMode::Init);
#endif
				m_webView->setWidthFilling(1, UIUpdateMode::Init);
				m_webView->setHeightFilling(1, UIUpdateMode::Init);
				addChild(m_webView, UIUpdateMode::Init);

				setWidthFilling(1, UIUpdateMode::Init);
				setHeightFilling(1, UIUpdateMode::Init);
			}

		public:
			Ref<WebView> getWebView() override
			{
				return m_webView;
			}

			void show(const OAuthWebRedirectDialogParam& param) override
			{
				auto onRedirect = param.onRedirect;

				m_webView->setOnStartLoad([onRedirect](WebView*, const String& url) {
					onRedirect(url);
				});
				setOnBack([onRedirect](View* view, UIEvent* ev) {
					onRedirect(sl_null);
				});

				m_webView->loadURL(param.url);

#ifdef SLIB_PLATFORM_IS_MOBILE
				Ref<MobileApp> app = MobileApp::getApp();
				if (app.isNotNull()) {
					Transition transition;
					transition.type = TransitionType::Cover;
					transition.direction = TransitionDirection::FromBottomToTop;
					transition.duration = 0.2f;
					app->popupPage(this, transition);
				}
#else
				setCenterInParent(UIUpdateMode::Init);
				Ref<Window> window = popupWindow(param.options.parentWindow, (sl_ui_len)(param.options.width), (sl_ui_len)(param.options.height));
				if (window.isNull()) {
					onRedirect(sl_null);
					return;
				}
				window->setTitle(param.options.title);
				window->increaseReference();
				m_window = window;
#endif
			}

			void close() override
			{
				ViewPage::close();
				auto window = ToRef(m_window);
				if (window.isNotNull()) {
					window->decreaseReference();
				}
			}

		};
	}

	Ptr<OAuthWebRedirectDialog> OAuthWebRedirectDialog::getDefault()
	{
		SLIB_GLOBAL_ZERO_INITIALIZED(AtomicPtr<OAuthWebRedirectDialog>, dlg);
		if (SLIB_SAFE_STATIC_CHECK_FREED(dlg)) {
			return sl_null;
		}
		if (dlg.isNull()) {
			dlg = ToRef(new DefaultOAuthWebRedirectDialogImpl);
		}
		return dlg;
	}

	void OAuthWebRedirectDialog::showDefault(const OAuthWebRedirectDialogParam& param)
	{
		auto dialog = OAuthWebRedirectDialog::getDefault();
		if (dialog.isNotNull()) {
			dialog->show(param);
		}
	}


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(OAuth1_LoginParam)

	OAuth1_LoginParam::OAuth1_LoginParam()
	{
		flagIgnoreExistingAccessToken = sl_false;
	}

	void OAuth1::login(const OAuth1_LoginParam& param)
	{
		String callbackUrl = param.authorization.callbackUrl;
		if (callbackUrl.isEmpty()) {
			callbackUrl = m_callbackUrl;
		}

		if (param.url.isNotEmpty()) {

			auto dialog = param.dialog;
			if (dialog.isNull()) {
				dialog = OAuthWebRedirectDialog::getDefault();
			}

			OAuthWebRedirectDialogParam dialogParam;
			dialogParam.url = param.url;
			dialogParam.options = param.dialogOptions;

			auto thiz = ToRef(this);
			auto onComplete = param.onComplete;
			auto weakDialog = dialog.toWeak();

			dialogParam.onRedirect = [thiz, weakDialog, callbackUrl, onComplete](const String& url) {
				if (url.isEmpty()) {
					OAuth1_LoginResult result;
					result.flagCancel = sl_true;
					onComplete(result);
					return;
				}
				if (url.startsWith(callbackUrl)) {
					Log(TAG, "Redirected to Callback URL: %s", url);
					auto dialog = weakDialog.lock();
					if (dialog.isNotNull()) {
						dialog->close();
					}
					OAuth1_LoginResult result;
					result.parseRedirectUrl(url);
					onComplete(result);
				}
			};

			dialog->show(dialogParam);
			return;
		}

		if (!(param.flagIgnoreExistingAccessToken)) {
			Shared<OAuth1_AccessToken> accessToken = m_accessToken;
			if (accessToken.isNotNull()) {
				if (accessToken->isValid()) {
					OAuth1_LoginResult result;
					result.flagSuccess = sl_true;
					result.flagCache = sl_true;
					result.accessToken = *accessToken;
					param.onComplete(result);
					return;
				}
			}
		}

		OAuth1_AuthorizationRequestParam authParam = param.authorization;
		authParam.callbackUrl = callbackUrl;
		auto thiz = ToRef(this);
		getLoginUrl(authParam, [thiz, param, callbackUrl](const String& url, const String& requestToken, const String& requestTokenSecret) {
			auto onComplete = param.onComplete;
			if (url.isEmpty() || requestToken.isEmpty() || requestTokenSecret.isEmpty()) {
				OAuth1_LoginResult result;
				onComplete(result);
				return;
			}
			OAuth1_LoginParam _param = param;
			_param.url = url;
			_param.authorization.callbackUrl = callbackUrl;
			_param.onComplete = [thiz, onComplete, requestToken, requestTokenSecret](OAuth1_LoginResult& result) {
				if (!(result.flagSuccess) || result.requestToken != requestToken || result.verifier.isEmpty()) {
					onComplete(result);
					return;
				}
				thiz->requestAccessToken(result.verifier, requestToken, requestTokenSecret, [onComplete](OAuth1_AccessTokenResult& _result) {
					OAuth1_LoginResult result;
					*((OAuth1_AccessTokenResult*)&result) = _result;
					onComplete(result);
				});
			};
			thiz->login(_param);
		});
	}

	void OAuth1::login(const Function<void(OAuth1_LoginResult& result)>& onComplete)
	{
		OAuth1_LoginParam param;
		param.onComplete = onComplete;
		login(param);
	}


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(OAuthLoginParam)

	OAuthLoginParam::OAuthLoginParam()
	{
		flagIgnoreExistingAccessToken = sl_false;
		flagAlwaysRequireAccessToken = sl_false;
	}

	void OAuth2::login(const OAuthLoginParam& param)
	{
		String redirectUri = param.authorization.redirectUri;
		if (redirectUri.isEmpty()) {
			redirectUri = m_redirectUri;
		}
		List<String> scopes = param.authorization.scopes;
		if (scopes.isNull()) {
			scopes = m_defaultScopes;
		}
		String state = param.authorization.state;

		if (param.url.isNotEmpty()) {

			auto dialog = param.dialog;
			if (dialog.isNull()) {
				dialog = OAuthWebRedirectDialog::getDefault();
			}

			OAuthWebRedirectDialogParam dialogParam;
			dialogParam.url = param.url;
			dialogParam.options = param.dialogOptions;

			String loginRedirectUri = param.loginRedirectUri;
			if (loginRedirectUri.isEmpty()) {
				loginRedirectUri = m_loginRedirectUri;
				if (loginRedirectUri.isEmpty()) {
					loginRedirectUri = redirectUri;
				}
			}

			auto thiz = ToRef(this);
			auto onComplete = param.onComplete;
			auto weakDialog = dialog.toWeak();

			dialogParam.onRedirect = [thiz, weakDialog, loginRedirectUri, scopes, state, onComplete](const String& url) {

				if (url.isEmpty()) {
					OAuthLoginResult result;
					result.flagCancel = sl_true;
					onComplete(result);
					return;
				}

				sl_bool flagRedirected = sl_false;
				ListElements<String> urls(loginRedirectUri.split(","));
				for (sl_size i = 0; i < urls.count; i++) {
					String pattern = urls[i].trim();
					if (pattern.isNotEmpty() && url.startsWith(pattern)) {
						flagRedirected = sl_true;
						break;
					}
				}
				if (flagRedirected) {
					Log(TAG, "Redirected to URI: %s", url);
					OAuthLoginResult result;
					result.parseRedirectUrl(url);
					if (state.isEmpty() || result.state == state) {
						auto dialog = weakDialog.lock();
						if (dialog.isNotNull()) {
							dialog->close();
						}
						if (result.flagSuccess && result.accessToken.isValid()) {
							if (result.accessToken.scopes.isNull()) {
								result.accessToken.scopes = scopes;
							}
							thiz->setAccessToken(result.accessToken);
						}
						onComplete(result);
					}
				}

			};

			dialog->show(dialogParam);
			return;
		}

		if (!(param.flagIgnoreExistingAccessToken)) {
			Shared<OAuthAccessToken> accessToken = m_accessToken;
			if (accessToken.isNotNull()) {
				if (accessToken->isValid(scopes)) {
					OAuthLoginResult result;
					result.flagSuccess = sl_true;
					result.flagCache = sl_true;
					result.accessToken = *accessToken;
					param.onComplete(result);
					return;
				}
			}
		}

		OAuthLoginParam _param = param;
		if (!m_flagSupportImplicitGrantType && _param.authorization.responseType == OAuthResponseType::Token) {
			_param.authorization.responseType = OAuthResponseType::Code;
			_param.flagAlwaysRequireAccessToken = sl_true;
		}
		_param.authorization.redirectUri = redirectUri;
		_param.authorization.scopes = scopes;
		if (state.isEmpty()) {
			state = String::fromInt64(Time::now().toUnixTime());
		}
		_param.authorization.state = state;
		_param.url = getLoginUrl(_param.authorization);

		if (_param.authorization.responseType == OAuthResponseType::Code) {
			if (_param.flagAlwaysRequireAccessToken) {
				auto onComplete = _param.onComplete;
				auto thiz = ToRef(this);
				_param.onComplete = [thiz, redirectUri, scopes, onComplete](OAuthLoginResult& result) {
					if (!(result.flagSuccess) || result.code.isEmpty()) {
						onComplete(result);
						return;
					}
					thiz->requestAccessTokenFromCode(result.code, redirectUri, [thiz, scopes, onComplete](OAuthAccessTokenResult& _result) {
						OAuthLoginResult result;
						*((OAuthAccessTokenResult*)&result) = _result;
						if (result.flagSuccess) {
							if (result.accessToken.scopes.isNull()) {
								result.accessToken.scopes = scopes;
							}
							thiz->setAccessToken(result.accessToken);
						}
						onComplete(result);
					});
				};
			}
		}
		login(_param);
	}

	void OAuth2::login(const Function<void(OAuthLoginResult& result)>& onComplete)
	{
		OAuthLoginParam param;
		param.onComplete = onComplete;
		login(param);
	}

}
