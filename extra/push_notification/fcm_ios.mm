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

#include <slib/core/definition.h>

#if defined(SLIB_UI_IS_IOS)

#include "fcm.h"

#include <slib/data/json.h>
#include <slib/core/safe_static.h>
#include <slib/ui/platform.h>

#include "external/firebase/iOS/FirebaseCore/FIRApp.h"
#include "external/firebase/iOS/FirebaseMessaging/FIRMessaging.h"

@interface SLIBFirebaseMessagingDelegate : NSObject<FIRMessagingDelegate> {}
@end

namespace slib
{

	namespace {

		static SLIBFirebaseMessagingDelegate* g_delegate;

		class FCMImpl : public FCM
		{
		public:
			void onStart() override
			{
				[FIRApp configure];

				g_delegate = [SLIBFirebaseMessagingDelegate new];
				[FIRMessaging messaging].delegate = g_delegate;

				APNs::getInstance()->start();
			}

		};

	}

	Ref<FCM> FCM::getInstance()
	{
		SLIB_SAFE_LOCAL_STATIC(Mutex, lock)
		MutexLocker locker(&lock);

		SLIB_LOCAL_STATIC_ZERO_INITIALIZED(Ref<FCM>, instance);
		if (instance.isNotNull()) {
			return instance;
		}

		instance = new FCMImpl;

		UIPlatform::registerDidReceiveRemoteNotificationCallback([&instance](NSDictionary* _userInfo) {
			PushNotificationMessage message;
			if (UIPlatform::parseRemoteNotificationInfo(_userInfo, message)) {
				instance->invokeReceiveMessage(message);
			}
		});

		return instance;
	}

}

using namespace slib;

@implementation SLIBFirebaseMessagingDelegate

- (void)messaging:(FIRMessaging *)messaging didReceiveRegistrationToken:(NSString *)fcmToken
{
	Ref<FCM> instance = FCM::getInstance();
	if (instance.isNull()) {
		return;
	}
	instance->invokeRefreshToken(Apple::getStringFromNSString(fcmToken));
}

@end

#endif
