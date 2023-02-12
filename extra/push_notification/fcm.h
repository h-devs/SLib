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

#ifndef CHECKHEADER_SLIB_EXTRA_PUSH_NOTIFICATION_FCM
#define CHECKHEADER_SLIB_EXTRA_PUSH_NOTIFICATION_FCM

#include <slib/ui/notification.h>

namespace slib
{

	// Firebase Cloud Messaging
	class FCM : public PushNotificationClient
	{
		SLIB_DECLARE_OBJECT

	public:
		FCM();

		~FCM();

	public:
		void onStart() override;

	public:
		static Ref<FCM> getInstance();

	};

	class UrlRequest;

	class FCM_Service
	{
	public:
		class SendResult
		{
		public:
			String message_id;
			String registration_id;
			String error;

		public:
			SendResult();

			SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(SendResult)
			SLIB_DECLARE_JSON
		};

		class SendResponse
		{
		public:
			sl_bool flagSuccess;
			UrlRequest* request;

			String multicast_id;
			sl_uint32 success;
			sl_uint32 failure;
			List<SendResult> results;

		public:
			SendResponse();

			SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(SendResponse)
			SLIB_DECLARE_JSON
		};

		class SendParam
		{
		public:
			String legacyServerKey;

			List<String> receiverDeviceTokens;
			String receiverDeviceToken;
			PushNotificationMessage message;

			Json customMessage;

			Function<void(SendResponse&)> callback;

		public:
			SendParam();
			SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(SendParam)
		};

		static void sendNotification(const SendParam& param);

	};

}

#endif
