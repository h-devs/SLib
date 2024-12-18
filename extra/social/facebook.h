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

#ifndef CHECKHEADER_SLIB_EXTRA_SOCIAL_FACEBOOK
#define CHECKHEADER_SLIB_EXTRA_SOCIAL_FACEBOOK

#include <slib/crypto/oauth.h>

namespace slib
{

	class FacebookUser
	{
	public:
		String id;
		String email;
		String name;
		String name_format;
		String first_name;
		String middle_name;
		String last_name;
		String short_name;
		String gender;
		String birthday;
		String quotes;
		String profile_pic;

		Json json;

	public:
		FacebookUser();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(FacebookUser)

		SLIB_DECLARE_JSON

	};

	typedef OAuthApiResult FacebookResult;

	class FacebookParam : public OAuth2_Param
	{
	public:
		String version;

	public:
		FacebookParam();

		FacebookParam(const String& version);

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(FacebookParam)

	};

	class Facebook : public OAuth2
	{
		SLIB_DECLARE_OBJECT

	protected:
		Facebook(const FacebookParam& param);

		~Facebook();

	public:
		static Ref<Facebook> create(const FacebookParam& param);

		static void initialize(const FacebookParam& param);

		static void initialize();

		static Ref<Facebook> create(const String& appId, const String& appSecret, const String& redirectUrl);

		static void initialize(const String& appId, const String& appSecret, const String& redirectUrl);

		static Ref<Facebook> create(const String& appId, const String& redirectUrl);

		static void initialize(const String& appId, const String& redirectUrl);

		static Ref<Facebook> createWithAccessToken(const String& accessToken);

		static Ref<Facebook> getInstance();

	public:
		class ResolveUserUrlParam
		{
		public:
			OAuthWebRedirectDialogOptions dialogOptions;
			Ptr<OAuthWebRedirectDialog> dialog;

			Function<void(const String& url)> onComplete;

		public:
			ResolveUserUrlParam();
			SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(ResolveUserUrlParam)
		};

		static void resolveUserUrl(const ResolveUserUrlParam& param);

		static void resolveUserUrl(const Function<void(const String& url)>& onComplete);

		class ShareResult
		{
		public:
			sl_bool flagSuccess;
			sl_bool flagCancel;

		public:
			ShareResult();
			SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(ShareResult)
		};

		class ShareParam
		{
		public:
			String url;
			String quote;
			String hashTag;

			Function<void(ShareResult&)> onComplete;

		public:
			ShareParam();
			SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(ShareParam)
		};

	public:
		String getRequestUrl(const String& path);

		void getUser(const String& personId, const String& fields, const Function<void(FacebookResult&, FacebookUser&)>& onComplete);

		void getUser(const String& personId, const List<String>& fields, const Function<void(FacebookResult&, FacebookUser&)>& onComplete);

		void getUser(const String& personId, const Function<void(FacebookResult&, FacebookUser&)>& onComplete);

	protected:
		String m_version;

		friend class FacebookSDK;
	};

	class FacebookSDK
	{
	public:
		static void initialize(); // Must be called in `Application::onStart()` override

	public:
		static void login(const Facebook::LoginParam& param);

		static void login(const Function<void(Facebook::LoginResult& result)>& onComplete);

		static void share(const Facebook::ShareParam& param);

	public:
		static void clearAccessToken();

		static Ref<Facebook> getInstance();

	protected:
		static void _updateCurrentToken(Facebook* instance);

	};

}

#endif
