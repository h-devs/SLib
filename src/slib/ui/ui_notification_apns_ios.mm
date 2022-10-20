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

#include "slib/ui/definition.h"

#if defined(SLIB_PLATFORM_IS_IOS)

#include "slib/ui/notification.h"

#include "slib/ui/core.h"
#include "slib/ui/platform.h"

#include "slib/core/safe_static.h"

#import <UserNotifications/UserNotifications.h>

namespace slib
{
	
	Ref<APNs> APNs::getInstance()
	{
		SLIB_SAFE_LOCAL_STATIC(Mutex, lock)
		MutexLocker locker(&lock);
		
		SLIB_LOCAL_STATIC_ZERO_INITIALIZED(Ref<APNs>, instance);
		if (instance.isNotNull()) {
			return instance;
		}
		
		instance = new APNs;
		
		UIPlatform::registerDidRegisterForRemoteNotifications([&instance](NSData* deviceToken, NSError* error) {
			String token = String::makeHexString([deviceToken bytes], [deviceToken length]);
			instance->dispatchRefreshToken(token);
		});
		UIPlatform::registerDidReceiveRemoteNotificationCallback([&instance](NSDictionary* _userInfo) {
			PushNotificationMessage message;
			if (UIPlatform::parseRemoteNotificationInfo(_userInfo, message)) {
				instance->dispatchReceiveMessage(message);
			}
		});
		return instance;
	}
	
	void APNs::onStart()
	{
		UIApplication* application = [UIApplication sharedApplication];
		if (@available(iOS 10.0, *)) {
			[[UNUserNotificationCenter currentNotificationCenter] requestAuthorizationWithOptions:(UNAuthorizationOptionAlert + UNAuthorizationOptionSound + UNAuthorizationOptionBadge) completionHandler:^(BOOL granted, NSError * _Nullable error) {
				if (!granted) {
					if (error != nil) {
						NSLog(@"Error on requesting notification authroization: %@", [error localizedDescription]);
					} else {
						NSLog(@"Not granted: notification authroization");
					}
				}
			}];
		} else {
#ifndef SLIB_PLATFORM_IS_IOS_CATALYST
			UIUserNotificationSettings* notificationSettings = [UIUserNotificationSettings settingsForTypes:UIUserNotificationTypeBadge | UIUserNotificationTypeSound | UIUserNotificationTypeAlert categories:nil];
			[application registerUserNotificationSettings:notificationSettings];
#endif
		}
		[application registerForRemoteNotifications];
	}
	
	sl_bool UIPlatform::parseRemoteNotificationInfo(NSDictionary* _userInfo, PushNotificationMessage& message)
	{
		NSError* error;
		NSData* dataUserInfo = [NSJSONSerialization dataWithJSONObject:_userInfo options:0 error:&error];
		String strUserInfo = Apple::getStringFromNSString([[NSString alloc] initWithData:dataUserInfo encoding:NSUTF8StringEncoding]);
		Json userInfo = Json::parse(strUserInfo);
		if (userInfo.isNotNull()) {
			Json aps = userInfo["aps"];
			Json alert = aps["alert"];
			if (alert.isStringType()) {
				message.title.setNull();
				message.content = alert.getString();
			} else {
				message.title = alert["title"].getString();
				message.content = alert["body"].getString();
			}
			message.badge = aps["badge"].getInt32(message.badge);
			message.data = userInfo;
			return sl_true;
		}
		return sl_false;
	}
	
}

#endif
