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

#ifndef CHECKHEADER_SLIB_EXTRA_SOCIAL_PAYPAL
#define CHECKHEADER_SLIB_EXTRA_SOCIAL_PAYPAL

#include <slib/crypto/oauth.h>

namespace slib
{

	typedef OAuthApiResult PayPalResult;

	class PayPalParam : public OAuth2_Param
	{
	public:
		PayPalParam(sl_bool flagSandbox = sl_false);

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(PayPalParam)

	public:
		sl_bool isSandbox() const;

		void setSandbox(sl_bool flag);

	private:
		sl_bool m_flagSandbox;

	};

	class PayPal : public OAuth2
	{
		SLIB_DECLARE_OBJECT

	public:
		enum class OrderIntent
		{
			CAPTURE,
			AUTHORIZE
		};

		enum class OrderStatus
		{
			None,
			CREATED,
			SAVED,
			APPROVED,
			VOIDED,
			COMPLETED
		};

		enum class LandingPage
		{
			Default,
			LOGIN,
			BILLING
		};

		enum class ShippingPreference
		{
			Default,
			GET_FROM_FILE,
			NO_SHIPPING,
			SET_PROVIDED_ADDRESS
		};

		enum class UserAction
		{
			Default,
			CONTINUE,
			PAY_NOW
		};

	protected:
		PayPal(const PayPalParam& param);

		~PayPal();

	public:
		static Ref<PayPal> create(const PayPalParam& param);

		static void initialize(const PayPalParam& param);

		static void initialize();

		static void initializeSandbox();

		static Ref<PayPal> create(const String& clientId, const String& clientSecret);

		static Ref<PayPal> createSandbox(const String& clientId, const String& clientSecret);

		static void initialize(const String& clientId, const String& clientSecret);

		static void initializeSandbox(const String& clientId, const String& clientSecret);

		static Ref<PayPal> createWithAccessToken(const String& accessToken);

		static Ref<PayPal> createSandboxWithAccessToken(const String& accessToken);

		static Ref<PayPal> getInstance();

	public:
		String getRequestUrl(const String& path);

		String getRequestUrl_v2(const String& path);

		class CreateOrderResult : public PayPalResult
		{
		public:
			String orderId;
			OrderStatus status;
			String approveLink;

		public:
			CreateOrderResult(UrlRequest*);
			SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(CreateOrderResult)
		};

		class CreateOrderParam
		{
		public:
			OrderIntent intent;
			double amount;
			String currencyCode;
			String description;

			String brandName;
			String locale;
			LandingPage landingPage;
			ShippingPreference shippingPreference;
			UserAction userAction;
			String returnUrl;
			String cancelUrl;

			Function<void(CreateOrderResult&)> onComplete;

		public:
			CreateOrderParam();
			SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(CreateOrderParam)
		};

		void createOrder(const CreateOrderParam& param);


		class CheckoutResult
		{
		public:
			sl_bool flagSuccess;
			sl_bool flagCancel;

			String orderId;

		public:
			CheckoutResult();
			SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(CheckoutResult)
		};

		class CheckoutParam : public CreateOrderParam
		{
		public:
			OAuthWebRedirectDialogOptions dialogOptions;
			Ptr<OAuthWebRedirectDialog> dialog;

			Function<void(CheckoutResult&)> onComplete;

		public:
			CheckoutParam();
			SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(CheckoutParam)
		};

		// Note that you should initialize PayPal instance with `accessToken` (which is granted on your secure server containing `clientSecret`) on mobile/desktop apps.
		void checkout(const CheckoutParam& param);

	public:
		sl_bool m_flagSandbox;

	};



}

#endif
