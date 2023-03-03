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

#ifndef CHECKHEADER_SLIB_EXTRA_SOCIAL_PINTEREST
#define CHECKHEADER_SLIB_EXTRA_SOCIAL_PINTEREST

#include <slib/crypto/oauth.h>

namespace slib
{

	class PinterestUser
	{
	public:
		String id;
		String url;
		String first_name;
		String last_name;

		Json json;

	public:
		PinterestUser();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(PinterestUser)

		SLIB_DECLARE_JSON

	};

	class PinterestBoard
	{
	public:
		String id;
		String name;
		String url;

		Json json;

	public:
		PinterestBoard();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(PinterestBoard)

		SLIB_DECLARE_JSON

	};

	typedef OAuthApiResult PinterestResult;

	class PinterestParam : public OAuth2_Param
	{
	public:
		PinterestParam();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(PinterestParam)

	};

	class Pinterest : public OAuth2
	{
		SLIB_DECLARE_OBJECT

	protected:
		Pinterest(const PinterestParam& param);

		~Pinterest();

	public:
		static Ref<Pinterest> create(const PinterestParam& param);

		static void initialize(const PinterestParam& param);

		static void initialize();

		static Ref<Pinterest> create(const String& appId, const String& appSecret, const String& redirectUrl);

		static void initialize(const String& appId, const String& appSecret, const String& redirectUrl);

		static Ref<Pinterest> create(const String& appId, const String& redirectUrl);

		static void initialize(const String& appId, const String& redirectUrl);

		static Ref<Pinterest> createWithAccessToken(const String& accessToken);

		static Ref<Pinterest> getInstance();

	public:
		using OAuth2::authorizeRequest;

		void authorizeRequest(UrlRequestParam& param, const AccessToken& token) override;

	public:
		String getRequestUrl(const String& path);

		void getUser(const String& userId, const Function<void(PinterestResult&, PinterestUser&)>& onComplete);

		void getMyBoards(const Function<void(PinterestResult&, List<PinterestBoard>& boards)>& onComplete);

		class CreateBoardResult : public PinterestResult
		{
		public:
			PinterestBoard createdBoard;

		public:
			CreateBoardResult(UrlRequest* request);
			SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(CreateBoardResult)
		};

		class CreateBoardParam
		{
		public:
			String name; // required
			String description;

			Function<void(CreateBoardResult& result)> onComplete;

		public:
			CreateBoardParam();
			SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(CreateBoardParam)
		};

		void createBoard(const CreateBoardParam& param);

		class CreatePinResult : public PinterestResult
		{
		public:
			CreatePinResult(UrlRequest* request);
			SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(CreatePinResult)
		};

		class CreatePinParam
		{
		public:
			String board; // required
			String note; // required
			String link;
			String imageUrl;

			Function<void(CreatePinResult& result)> onComplete;

		public:
			CreatePinParam();
			SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(CreatePinParam)
		};

		void createPin(const CreatePinParam& param);

		class LoginParam : public OAuth2::LoginParam
		{
		public:
			LoginParam();
			SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(LoginParam)

		public:
			void addScopeForWritingPublic();
		};

	};

}

#endif
