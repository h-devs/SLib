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

#ifndef CHECKHEADER_SLIB_EXTRA_PUSH_NOTIFICATION_XGPUSH
#define CHECKHEADER_SLIB_EXTRA_PUSH_NOTIFICATION_XGPUSH

#include <slib/ui/notification.h>

namespace slib
{

	class XgPush : public PushNotificationClient
	{
		SLIB_DECLARE_OBJECT

	public:
		enum class Platform
		{
			All = 0,
			iOS = 1,
			Android = 2
		};

		enum class Environment
		{
			Production = 0,
			Development = 1
		};

	public:
		XgPush();

		~XgPush();

	public:
		// used on iOS
		static void initialize(sl_uint32 accessId, const String& accessKey);

		static void setEnableDebug(sl_bool flag);

	public:
		void onStart() override;

	public:
		static Ref<XgPush> getInstance();

	};

	class XgPushService
	{
	public:
		typedef XgPush::Platform Platform;
		typedef XgPush::Environment Environment;

		class SendParam
		{
		public:
			String appId;
			String secretKey;
			Platform platform;
			Environment environment;

			List<String> receiverDeviceTokens;
			PushNotificationMessage message;

			Json customMessage;

			Function<void(sl_bool, String)> callback;

		public:
			SendParam();

			SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(SendParam)

		};

		static void sendNotification(const SendParam& param);

		static String getPlatformString(Platform platform);

		static String getEnvironmentString(Environment environment);

	};

}

#endif
