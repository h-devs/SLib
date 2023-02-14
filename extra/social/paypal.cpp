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

#include "paypal.h"

#include <slib/core/safe_static.h>

namespace slib
{

	namespace {

		SLIB_GLOBAL_ZERO_INITIALIZED(AtomicRef<PayPal>, g_instance)

		static PayPal::OrderStatus ParseOrderStatus(const String& strStatus)
		{
			if (strStatus == "CREATED") {
				return PayPal::OrderStatus::CREATED;
			} else if (strStatus == "SAVED") {
				return PayPal::OrderStatus::SAVED;
			} else if (strStatus == "APPROVED") {
				return PayPal::OrderStatus::APPROVED;
			} else if (strStatus == "VOIDED") {
				return PayPal::OrderStatus::VOIDED;
			} else if (strStatus == "COMPLETED") {
				return PayPal::OrderStatus::COMPLETED;
			} else if (strStatus == "SAVED") {
				return PayPal::OrderStatus::SAVED;
			} else {
				return PayPal::OrderStatus::None;
			}
		}

	}

	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(PayPalParam)

	PayPalParam::PayPalParam(sl_bool flagSandbox)
	{
		setSandbox(flagSandbox);
		flagUseBasicAuthorizationForAccessToken = sl_true;
	}

	sl_bool PayPalParam::isSandbox() const
	{
		return m_flagSandbox;
	}

	void PayPalParam::setSandbox(sl_bool flag)
	{
		m_flagSandbox = flag;
		if (flag) {
			accessTokenUrl = "https://api.sandbox.paypal.com/v1/oauth2/token";
		} else {
			accessTokenUrl = "https://api.paypal.com/v1/oauth2/token";
		}
	}


	SLIB_DEFINE_OBJECT(PayPal, OAuth2)

	PayPal::PayPal(const PayPalParam& param) : OAuth2(param)
	{
		m_flagSandbox = param.isSandbox();
	}

	PayPal::~PayPal()
	{
	}

	Ref<PayPal> PayPal::create(const PayPalParam& param)
	{
		return new PayPal(param);
	}

	void PayPal::initialize(const PayPalParam& param)
	{
		if (SLIB_SAFE_STATIC_CHECK_FREED(g_instance)) {
			return;
		}
		g_instance = create(param);
	}

	void PayPal::initialize()
	{
		PayPalParam param(sl_false);
		initialize(param);
	}

	void PayPal::initializeSandbox()
	{
		PayPalParam param(sl_true);
		initialize(param);
	}

	Ref<PayPal> PayPal::create(const String& clientId, const String& clientSecret)
	{
		PayPalParam param(sl_false);
		param.clientId = clientId;
		param.clientSecret = clientSecret;
		return create(param);
	}

	Ref<PayPal> PayPal::createSandbox(const String& clientId, const String& clientSecret)
	{
		PayPalParam param(sl_true);
		param.clientId = clientId;
		param.clientSecret = clientSecret;
		return create(param);
	}

	void PayPal::initialize(const String& clientId, const String& clientSecret)
	{
		PayPalParam param(sl_false);
		param.clientId = clientId;
		param.clientSecret = clientSecret;
		initialize(param);
	}

	void PayPal::initializeSandbox(const String& clientId, const String& clientSecret)
	{
		PayPalParam param(sl_true);
		param.clientId = clientId;
		param.clientSecret = clientSecret;
		initialize(param);
	}

	Ref<PayPal> PayPal::createWithAccessToken(const String& accessToken)
	{
		PayPalParam param(sl_false);
		param.accessToken.token = accessToken;
		return create(param);
	}

	Ref<PayPal> PayPal::createSandboxWithAccessToken(const String& accessToken)
	{
		PayPalParam param(sl_true);
		param.accessToken.token = accessToken;
		return create(param);
	}

	Ref<PayPal> PayPal::getInstance()
	{
		if (SLIB_SAFE_STATIC_CHECK_FREED(g_instance)) {
			return sl_null;
		}
		return g_instance;
	}

	String PayPal::getRequestUrl(const String& path)
	{
		if (m_flagSandbox) {
			return "https://api.sandbox.paypal.com/" + path;
		} else {
			return "https://api.paypal.com/" + path;
		}
	}

	String PayPal::getRequestUrl_v2(const String& path)
	{
		return getRequestUrl("v2/" + path);
	}
	
	SLIB_DEFINE_NESTED_CLASS_DEFAULT_MEMBERS(PayPal, CreateOrderResult)

	PayPal::CreateOrderResult::CreateOrderResult(UrlRequest* request) : PayPalResult(request)
	{
		status = OrderStatus::None;
		if (!request) {
			return;
		}
		if (request->isError()) {
			return;
		}
		orderId = response["id"].getString();
		status = ParseOrderStatus(response["status"].getString());
		for (auto& item : response["links"].getJsonList()) {
			if (item["rel"].getString() == "approve") {
				approveLink = item["href"].getString();
				break;
			}
		}
		if (orderId.isNotEmpty() && status == OrderStatus::CREATED && approveLink.isNotEmpty()) {
			flagSuccess = sl_true;
		}
	}

	SLIB_DEFINE_NESTED_CLASS_DEFAULT_MEMBERS(PayPal, CreateOrderParam)

	PayPal::CreateOrderParam::CreateOrderParam()
	{
		intent = OrderIntent::CAPTURE;
		amount = 0;
		currencyCode = "USD";

		landingPage = LandingPage::Default;
		shippingPreference = ShippingPreference::Default;
		userAction = UserAction::Default;
		returnUrl = "https://localhost/return";
		cancelUrl = "https://localhost/cancel";
	}

	void PayPal::createOrder(const PayPal::CreateOrderParam& param)
	{
		UrlRequestParam rp;
		rp.method = HttpMethod::POST;
		rp.url = getRequestUrl_v2("checkout/orders");
		rp.requestHeaders.put_NoLock("Content-Type", "application/json");
		const char* strIntent;
		if (param.intent == OrderIntent::CAPTURE) {
			strIntent = "CAPTURE";
		} else {
			strIntent = "AUTHORIZE";
		}
		Json json;
		json.putItem("intent", strIntent);
		Json amount;
		amount.putItem("currency_code", param.currencyCode);
		amount.putItem("value", String::format("%.2f", param.amount));
		Json purchase_unit;
		purchase_unit.putItem("amount", amount);
		String description = param.description.trim();
		if (description.isNotEmpty()) {
			purchase_unit.putItem("description", description.substring(0, 126));
		}
		json.putItem("purchase_units", JsonList::createFromElement(purchase_unit));
		Json application_context;
		if (param.brandName.isNotEmpty()) {
			application_context.putItem("brand_name", param.brandName);
		}
		if (param.locale.isNotEmpty()) {
			application_context.putItem("locale", param.locale);
		}
		if (param.landingPage != LandingPage::Default) {
			const char* s;
			if (param.landingPage == LandingPage::LOGIN) {
				s = "LOGIN";
			} else {
				s = "BILLING";
			}
			application_context.putItem("landing_page", s);
		}
		if (param.shippingPreference != ShippingPreference::Default) {
			const char* s;
			if (param.shippingPreference == ShippingPreference::GET_FROM_FILE) {
				s = "GET_FROM_FILE";
			} else if (param.shippingPreference == ShippingPreference::NO_SHIPPING) {
				s = "NO_SHIPPING";
			} else {
				s = "SET_PROVIDED_ADDRESS";
			}
			application_context.putItem("shipping_preference", s);
		}
		if (param.userAction != UserAction::Default) {
			const char* s;
			if (param.userAction == UserAction::CONTINUE) {
				s = "CONTINUE";
			} else {
				s = "PAY_NOW";
			}
			application_context.putItem("user_action", s);
		}
		application_context.putItem("return_url", param.returnUrl);
		application_context.putItem("cancel_url", param.cancelUrl);
		json.putItem("application_context", application_context);
		rp.setRequestBodyAsJson(json);

		auto onComplete = param.onComplete;
		rp.onComplete = [onComplete](UrlRequest* request) {
			CreateOrderResult result(request);
			onComplete(result);
		};
		authorizeRequest(rp);
		UrlRequest::send(rp);
	}
	
	
	SLIB_DEFINE_NESTED_CLASS_DEFAULT_MEMBERS(PayPal, CheckoutResult)

	PayPal::CheckoutResult::CheckoutResult()
	{
		flagSuccess = sl_false;
		flagCancel = sl_false;
	}


	SLIB_DEFINE_NESTED_CLASS_DEFAULT_MEMBERS(PayPal, CheckoutParam)

	PayPal::CheckoutParam::CheckoutParam()
	{
	}


}
