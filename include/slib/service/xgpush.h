/*
 *   Copyright (c) 2008-2019 SLIBIO <https://github.com/SLIBIO>
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

#ifndef CHECKHEADER_SLIB_SERVICE_XGPUSH
#define CHECKHEADER_SLIB_SERVICE_XGPUSH

#include "push_notification.h"

namespace slib
{

	enum class XgPushPlatform
	{
		All = 0,
		iOS = 1,
		Android = 2
	};

	enum class XgPushEnvironment
	{
		Production = 0,
		Development = 1
	};

	class XgPushSendParam
	{
	public:
		String appId;
		String secretKey;
		XgPushPlatform platform;
		XgPushEnvironment environment;

		List<String> receiverDeviceTokens;
		PushNotificationMessage message;

		Json customMessage;

		Function<void(sl_bool, String)> callback;

	public:
		XgPushSendParam();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(XgPushSendParam)

	};

	class XgPushService
	{
	public:
		static void sendNotification(const XgPushSendParam& param);

		static String getPlatformString(XgPushPlatform platform);

		static String getEnvironmentString(XgPushEnvironment environment);

	};

}

#endif
