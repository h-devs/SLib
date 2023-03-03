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

#ifndef CHECKHEADER_SLIB_EXTRA_SOCIAL_WECHAT
#define CHECKHEADER_SLIB_EXTRA_SOCIAL_WECHAT

#include <slib/crypto/oauth.h>
#include <slib/data/xml.h>

namespace slib
{

	class WeChatUser
	{
	public:
		String openid;
		String nickname;
		sl_uint32 sex;
		String province;
		String city;
		String country;
		String headimgurl;
		List<String> privilege;
		String unionid;

		Json json;

	public:
		WeChatUser();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(WeChatUser)

		SLIB_DECLARE_JSON

	};

	typedef OAuthApiResult WeChatResult;

	class WeChatParam : public OAuth2_Param
	{
	public:
		WeChatParam();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(WeChatParam)

	};

	class WeChat : public OAuth2
	{
		SLIB_DECLARE_OBJECT

	public:
		class PaymentOrder
		{
		public:
			String partnerId;
			String prepayId;
			String package;
			String nonce;
			sl_uint64 timeStamp;
			String sign;

		public:
			PaymentOrder();
			SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(PaymentOrder)
		};

		class AppResult
		{
		public:
			sl_bool flagSuccess;
			sl_bool flagCancel;
			String error;

		public:
			AppResult();
			SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(AppResult)
		};

	protected:
		WeChat(const WeChatParam& param);

		~WeChat();

	public:
		static Ref<WeChat> create(const WeChatParam& param);

		static void initialize(const WeChatParam& param);

		static void initialize();

		static Ref<WeChat> create(const String& appId, const String& appSecret, const String& redirectUrl);

		static void initialize(const String& appId, const String& appSecret, const String& redirectUrl);

		static Ref<WeChat> create(const String& appId, const String& redirectUrl);

		static void initialize(const String& appId, const String& redirectUrl);

		static Ref<WeChat> createWithAccessToken(const String& accessToken);

		static Ref<WeChat> getInstance();

	public:
		String getOpenId();

		void authorizeRequest(UrlRequestParam& param, const AccessToken& token) override;
		using OAuth2::authorizeRequest;

		String getRequestUrl(const String& path);

		void getUser(const String& openId, const Function<void(WeChatResult&, WeChatUser&)>& onComplete);

		void getUser(const Function<void(WeChatResult&, WeChatUser&)>& onComplete);

		class CreateOrderResult
		{
		public:
			sl_bool flagSuccess;

			UrlRequest* request;
			Ref<XmlDocument> response;
			String responseText;

			String returnCode;
			String returnMessage;
			String resultCode;
			String errorCode;
			String errorDescription;

			PaymentOrder order;

		public:
			CreateOrderResult();
			SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(CreateOrderResult)
		};

		class CreateOrderParam
		{
		public:
			String apiKey; // required
			String appId; // required
			String businessId; // required
			String orderId; // required
			sl_uint64 amount; // required, unit: yuan/100
			String currency;
			String deviceId;
			String body; // required
			String detail;
			String attach;
			String ip; // required
			String notifyUrl; // required
			String nonce;
			Time timeStart;
			Time timeExpire;

			Function<void(CreateOrderResult&)> onComplete;

		public:
			CreateOrderParam();
			SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(CreateOrderParam)
		};

		static void createOrder(const CreateOrderParam& param);

		class PaymentResult : public AppResult
		{
		public:
			PaymentResult();
			SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(PaymentResult)
		};

		class PaymentRequest
		{
		public:
			PaymentOrder order;
			Function<void(PaymentResult&)> onComplete;

		public:
			PaymentRequest();
			SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(PaymentRequest)
		};

	protected:
		void onReceiveAccessToken(AccessTokenResult& result) override;

	protected:
		String m_currentOpenId;

	};

	class WeChatSDK
	{
	public:
		static void initialize(const String& appId, const String& universalURL);

	public:
		static void login(const WeChat::LoginParam& param);

		static void login(const Function<void(WeChat::LoginResult& result)>& onComplete);

		static void pay(const WeChat::PaymentRequest& req);

	};

}

#endif
