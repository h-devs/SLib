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

#if defined(SLIB_PLATFORM_IS_IOS)

#include "wechat.h"

#include <slib/core/time.h>
#include <slib/core/safe_static.h>
#include <slib/ui/core.h>
#include <slib/ui/platform.h>

#import "external/wechat/iOS/WXApi.h"

@interface SLIBWeChatSDKDelegate : NSObject<WXApiDelegate> {}
@end

namespace slib
{

	namespace {

		class StaticContext
		{
		public:
			SLIBWeChatSDKDelegate* delegate;
			Mutex lock;
			Function<void(WeChat::LoginResult&)> callbackLogin;
			Function<void(WeChat::PaymentResult&)> callbackPay;

		public:
			StaticContext()
			{
				SLIBWeChatSDKDelegate* delegate = [SLIBWeChatSDKDelegate new];
				this->delegate = delegate;
				UIPlatform::registerOpenUrlCallback([delegate](NSURL* url, NSDictionary*) {
					return [WXApi handleOpenURL:url delegate:delegate];
				});
			}

		public:
			void setLoginCallback(const Function<void(WeChat::LoginResult&)>& callback)
			{
				MutexLocker locker(&lock);
				if (callbackPay.isNotNull()) {
					WeChat::LoginResult result;
					result.flagCancel = sl_true;
					callbackLogin(result);
				}
				callbackLogin = callback;
			}

			void onLoginResult(WeChat::LoginResult& result)
			{
				MutexLocker locker(&lock);
				callbackLogin(result);
				callbackLogin.setNull();
			}

			void setPayCallback(const Function<void(WeChat::PaymentResult&)>& callback)
			{
				MutexLocker locker(&lock);
				if (callbackPay.isNotNull()) {
					WeChat::PaymentResult result;
					result.flagCancel = sl_true;
					callbackPay(result);
				}
				callbackPay = callback;
			}

			void onPayResult(WeChat::PaymentResult& result)
			{
				MutexLocker locker(&lock);
				callbackPay(result);
				callbackPay.setNull();
			}

		};

		SLIB_SAFE_STATIC_GETTER(StaticContext, GetStaticContext)

	}

	void WeChatSDK::initialize(const String& appId, const String& universalLink)
	{
		GetStaticContext();
		[WXApi registerApp:Apple::getNSStringFromString(appId) universalLink:Apple::getNSStringFromString(universalLink)];
	}

	void WeChatSDK::login(const WeChat::LoginParam& param)
	{
		if (!(UI::isUiThread())) {
			void (*f)(const WeChat::LoginParam&) = &WeChatSDK::login;
			UI::dispatchToUiThread(Function<void()>::bind(f, param));
			return;
		}

		GetStaticContext()->setLoginCallback(param.onComplete);

		SendAuthReq* req = [SendAuthReq new];
		req.scope = @"snsapi_userinfo";
		req.state = [NSString stringWithFormat:@"%d", (int)(Time::now().toInt())];
		[WXApi sendReq:req completion:^(BOOL success) {
			if (!success) {
				WeChat::LoginResult result;
				GetStaticContext()->onLoginResult(result);
			}
		}];
	}

	void WeChatSDK::pay(const WeChat::PaymentRequest& param)
	{
		if (!(UI::isUiThread())) {
			UI::dispatchToUiThread(Function<void()>::bind(&WeChatSDK::pay, param));
			return;
		}

		GetStaticContext()->setPayCallback(param.onComplete);

		PayReq* req = [PayReq new];
		req.partnerId = Apple::getNSStringFromString(param.order.partnerId);
		req.prepayId = Apple::getNSStringFromString(param.order.prepayId);
		req.nonceStr = Apple::getNSStringFromString(param.order.nonce);
		req.timeStamp = (UInt32)(param.order.timeStamp);
		req.package = Apple::getNSStringFromString(param.order.package);
		req.sign = Apple::getNSStringFromString(param.order.sign);
		[WXApi sendReq:req completion:^(BOOL success) {
			if (!success) {
				WeChat::PaymentResult result;
				GetStaticContext()->onPayResult(result);
			}
		}];
	}

}

using namespace slib;

@implementation SLIBWeChatSDKDelegate

-(void)onResp:(BaseResp*)resp
{
	if ([resp isKindOfClass:[SendAuthResp class]]) {
		SendAuthResp* response = (SendAuthResp*)resp;
		WeChat::LoginResult result;
		switch(response.errCode){
			case WXSuccess:
				result.flagSuccess = sl_true;
				break;
			case WXErrCodeUserCancel:
				result.flagCancel = sl_true;
			default:
				break;
		}
		result.code = Apple::getStringFromNSString(response.code);
		result.error = Apple::getStringFromNSString(response.errStr);
		GetStaticContext()->onLoginResult(result);
	} else if ([resp isKindOfClass:[PayResp class]]) {
		PayResp* response = (PayResp*)resp;
		WeChat::PaymentResult result;
		switch(response.errCode){
			case WXSuccess:
				result.flagSuccess = sl_true;
				break;
			case WXErrCodeUserCancel:
				result.flagCancel = sl_true;
			default:
				break;
		}
		result.error = Apple::getStringFromNSString(response.errStr);
		GetStaticContext()->onPayResult(result);
	}
}

@end

#endif
