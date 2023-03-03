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

#ifndef CHECKHEADER_SLIB_EXTRA_SOCIAL_ETSY
#define CHECKHEADER_SLIB_EXTRA_SOCIAL_ETSY

#include <slib/crypto/oauth.h>

namespace slib
{

	class EtsyUser
	{
	public:
		String user_id;
		String login_name;
		String primary_email;
		float creation_tsz;
		String user_pub_key;
		int referred_by_user_id;
		
		class FeedbackInfo
		{
		public:
			int count;
			int score;

		public:
			FeedbackInfo();

			SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(FeedbackInfo)
			SLIB_DECLARE_JSON
		} feedback_info;

		int awaiting_feedback_count;
		sl_bool use_new_inventory_endpoints;

	public:
		EtsyUser();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(EtsyUser)

		SLIB_DECLARE_JSON

	public:
		static String getPublicProfileURL(const String& userId);

		String getPublicProfileURL() const;

	};

	typedef OAuthApiResult EtsyResult;

	class EtsyParam : public OAuth1_Param
	{
	public:
		EtsyParam();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(EtsyParam)

	};

	class Etsy : public OAuth1
	{
		SLIB_DECLARE_OBJECT

	protected:
		Etsy(const EtsyParam& param);

		~Etsy();

	public:
		static Ref<Etsy> create(const EtsyParam& param);

		static void initialize(const EtsyParam& param);

		static void initialize();

		static Ref<Etsy> create(const String& consumerKey, const String& consumerSecret, const String& callbackUrl);

		static void initialize(const String& consumerKey, const String& consumerSecret, const String& callbackUrl);

		static Ref<Etsy> createWithAccessToken(const String& token, const String tokenSecret);

		static Ref<Etsy> getInstance();

	public:
		class LoginParam : public OAuth1::LoginParam
		{
		public:
			List<String> scopes;

		public:
			LoginParam();
			SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(LoginParam)
		};

		void login(const LoginParam& param);

		void login(const Function<void(LoginResult& result)>& onComplete);

		void login(const List<String>& scopes, const Function<void(LoginResult& result)>& onComplete);

	public:
		String getRequestUrl(const String& path);

		void getUser(const String& userId, const Function<void(EtsyResult&, EtsyUser&)>& onComplete);

	};

}

#endif
