/*
 *   Copyright (c) 2008-2018 SLIBIO <https://github.com/SLIBIO>
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

#ifndef CHECKHEADER_SLIB_SOCIAL_ETSY
#define CHECKHEADER_SLIB_SOCIAL_ETSY

#include "oauth.h"

namespace slib
{

	class SLIB_EXPORT EtsyUserFeedbackInfo
	{
	public:
		int count;
		int score;

	public:
		EtsyUserFeedbackInfo();
		
		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(EtsyUserFeedbackInfo)
		
		SLIB_DECLARE_JSON
		
	};
	
	class SLIB_EXPORT EtsyUser
	{
	public:
		String user_id;
		String login_name;
		String primary_email;
		float creation_tsz;
		String user_pub_key;
		int referred_by_user_id;
		EtsyUserFeedbackInfo feedback_info;
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
	
	typedef OAuth1_LoginResult EtsyLoginResult;
	
	class SLIB_EXPORT EtsyLoginParam : public OAuth1_LoginParam
	{
	public:
		List<String> scopes;
		
	public:
		EtsyLoginParam();
		
		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(EtsyLoginParam)
		
	};
	
	class SLIB_EXPORT EtsyParam : public OAuth1_Param
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
		void login(const EtsyLoginParam& param);
		
		void login(const Function<void(EtsyLoginResult& result)>& onComplete);
		
		void login(const List<String>& scopes, const Function<void(EtsyLoginResult& result)>& onComplete);
		
	public:
		String getRequestUrl(const String& path);
		
		void getUser(const String& userId, const Function<void(EtsyResult&, EtsyUser&)>& onComplete);
		
	};
	
}

#endif
