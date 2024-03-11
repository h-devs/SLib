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

#if defined(SLIB_UI_IS_IOS)

#include "slib/ui/core.h"

#include "slib/ui/screen.h"
#include "slib/ui/mobile_app.h"
#include "slib/ui/notification.h"
#include "slib/core/log.h"
#include "slib/core/safe_static.h"
#include "slib/ui/platform.h"

@interface SLIBAppDelegate : UIResponder <UIApplicationDelegate>
	@property (strong, nonatomic) UIWindow *window;
@end

namespace slib
{

	namespace priv
	{
		void ResetOrientation();
	}

	using namespace priv;

	namespace {
		class ScreenImpl : public Screen
		{
		public:
			UIScreen* m_screen;

		public:
			static Ref<ScreenImpl> create(UIScreen* screen)
			{
				Ref<ScreenImpl> ret;
				if (screen != nil) {
					ret = new ScreenImpl();
					if (ret.isNotNull()) {
						ret->m_screen = screen;
					}
				}
				return ret;
			}

			static UIScreen* getPrimaryScreen()
			{
				NSArray* arr = [UIScreen screens];
				sl_size n = [arr count];
				if (n == 0) {
					return nil;
				}
				UIScreen* primary = [arr objectAtIndex:0];
				return primary;
			}

			UIRect getRegion() override
			{
				CGRect rect = [m_screen bounds];
				CGFloat f = UIPlatform::getGlobalScaleFactor();
				UIRect region;
				region.left = (sl_ui_pos)(rect.origin.x * f);
				region.top = (sl_ui_pos)(rect.origin.y * f);
				region.setWidth((sl_ui_pos)(rect.size.width * f));
				region.setHeight((sl_ui_pos)(rect.size.height * f));
				return region;
			}
		};
	}

	List< Ref<Screen> > UI::getScreens()
	{
		List< Ref<Screen> > ret;
		NSArray* arr = [UIScreen screens];
		sl_size n = [arr count];
		if (n == 0) {
			return ret;
		}
		for (sl_size i = 0; i < n; i++) {
			UIScreen* _screen = [arr objectAtIndex:i];
			ret.add_NoLock(ScreenImpl::create(_screen));
		}
		return ret;
	}

	Ref<Screen> UI::getPrimaryScreen()
	{
		UIScreen* screen = ScreenImpl::getPrimaryScreen();
		return UIPlatform::createScreen(screen);
	}

	sl_bool UI::isUiThread()
	{
		return [NSThread isMainThread];
	}

	void UI::dispatchToUiThread(const Function<void()>& _callback, sl_uint32 delayMillis)
	{
		Function<void()> callback = _callback;
		if (callback.isNotNull()) {
			if (delayMillis == 0) {
				dispatch_async(dispatch_get_main_queue(), ^{
					callback();
				});
			} else {
				dispatch_time_t t = dispatch_time(DISPATCH_TIME_NOW, (sl_int64)(delayMillis) * NSEC_PER_MSEC);
				dispatch_after(t, dispatch_get_main_queue(), ^{
					callback();
				});
			}
		}
	}

	void UI::openUrl(const StringParam& _url)
	{
		if (_url.isNotEmpty()) {
			if (![NSThread isMainThread]) {
				String url = _url.toString();
				dispatch_async(dispatch_get_main_queue(), ^{
					UI::openUrl(url);
				});
			} else {
				NSString* s = Apple::getNSStringFromString(_url);
				NSURL* url = [NSURL URLWithString:s];
#ifdef SLIB_PLATFORM_IS_IOS_CATALYST
				[[UIApplication sharedApplication] openURL:url options:@{} completionHandler:nil];
#else
				[[UIApplication sharedApplication] openURL:url];
#endif
			}
		}
	}

	void UI::dismissKeyboard()
	{
		UIWindow* window = UIPlatform::getKeyWindow();
		if (window != nil) {
			if (![NSThread isMainThread]) {
				dispatch_async(dispatch_get_main_queue(), ^{
					[window endEditing:YES];
				});
			} else {
				[window endEditing:YES];
			}
		}
	}

	Ref<Screen> UIPlatform::createScreen(UIScreen* screen)
	{
		return ScreenImpl::create(screen);
	}

	UIScreen* UIPlatform::getScreenHandle(Screen* _screen)
	{
		ScreenImpl* screen = (ScreenImpl*)_screen;
		if (screen) {
			return screen->m_screen;
		}
		return nil;
	}

	void UIPlatform::runLoop(sl_uint32 level)
	{
		CFRunLoopRun();
	}

	void UIPlatform::quitLoop()
	{
		CFRunLoopStop(CFRunLoopGetCurrent());
	}

	void UIPlatform::initApp()
	{
	}

	void UIPlatform::runApp()
	{
		char* args[] = {sl_null};
		@autoreleasepool {
			UIApplicationMain(0, args, nil, NSStringFromClass([SLIBAppDelegate class]));
		}
	}

	void UIPlatform::quitApp()
	{
	}

	UIWindow* UIPlatform::getMainWindow()
	{
		SLIBAppDelegate* app = (SLIBAppDelegate*)([[UIApplication sharedApplication] delegate]);
		if (app != nil) {
			return app.window;
		}
		return nil;
	}

	UIViewController* UIPlatform::getCurrentViewController()
	{
		return getCurrentViewController(Ref<Window>::null());
	}

	UIViewController* UIPlatform::getCurrentViewController(const Ref<Window>& _parentWindow)
	{
		Ref<Window> parentWindow = _parentWindow;
		UIWindow* window = nil;
		if (parentWindow.isNull()) {
			Ref<MobileApp> app = MobileApp::getApp();
			if (app.isNotNull()) {
				parentWindow = app->getMainWindow();
			}
		}
		if (parentWindow.isNotNull()) {
			Ref<WindowInstance> instance = parentWindow->getWindowInstance();
			UIView* view = UIPlatform::getWindowHandle(instance.get());
			if ([view isKindOfClass:[UIWindow class]]) {
				window = (UIWindow*)view;
			}
		}
		if (window == nil) {
			window = UIPlatform::getKeyWindow();
		}
		if (window != nil) {
			return [window rootViewController];
		}
		return nil;
	}

	UIView* UIPlatform::findFirstResponder(UIView* root)
	{
		if (root.isFirstResponder) {
			return root;
		}
		for (UIView* subView in root.subviews) {
			UIView* v = findFirstResponder(subView);
			if (v != nil) {
				return v;
			}
		}
		return nil;
	}

	namespace {

		static CGFloat g_fGlobalScaleFactor = 0;

		SLIB_GLOBAL_ZERO_INITIALIZED(AtomicFunction<void(NSDictionary*)>, g_callbackDidFinishLaunching);
		SLIB_GLOBAL_ZERO_INITIALIZED(AtomicFunction<void(NSData*, NSError*)>, g_callbackDidRegisterForRemoteNotifications);
		SLIB_GLOBAL_ZERO_INITIALIZED(AtomicFunction<void(NSDictionary*)>, g_callbackDidReceiveRemoteNotification);
		SLIB_GLOBAL_ZERO_INITIALIZED(AtomicList< Function<BOOL(NSURL*, NSDictionary*)> >, g_callbackOpenURL);

		static BOOL UIPlatform_onOpenUrl(NSURL* url, NSDictionary* options)
		{
			List< Function<BOOL(NSURL*, NSDictionary*)> > callbacks(g_callbackOpenURL);
			for (auto&& callback : callbacks) {
				if (callback(url, options)) {
					return YES;
				}
			}
			return MobileApp::Current::invokeOpenUrl(Apple::getStringFromNSString(url.absoluteString));
		}

	}

	CGFloat UIPlatform::getGlobalScaleFactor()
	{
		if (g_fGlobalScaleFactor == 0) {
			UIScreen* screen = ScreenImpl::getPrimaryScreen();
			if (screen != nil) {
				CGFloat f = screen.scale;
				g_fGlobalScaleFactor = f;
				return f;
			} else {
				return 1;
			}
		}
		return g_fGlobalScaleFactor;
	}

	void UIPlatform::setGlobalScaleFactor(CGFloat factor)
	{
		g_fGlobalScaleFactor = factor;
	}

	void UIPlatform::registerDidFinishLaunchingCallback(const Function<void(NSDictionary*)>& callback)
	{
		g_callbackDidFinishLaunching.add(callback);
	}

	void UIPlatform::registerDidRegisterForRemoteNotifications(const Function<void(NSData*, NSError*)>& callback)
	{
		g_callbackDidRegisterForRemoteNotifications.add(callback);
	}

	void UIPlatform::registerDidReceiveRemoteNotificationCallback(const Function<void(NSDictionary*)>& callback)
	{
		g_callbackDidReceiveRemoteNotification.add(callback);
	}

	void UIPlatform::registerOpenUrlCallback(const Function<BOOL(NSURL*, NSDictionary*)>& callback)
	{
		g_callbackOpenURL.add(callback);
	}


	sl_ui_len MobileApp::getStatusBarHeight()
	{
#ifndef SLIB_PLATFORM_IS_IOS_CATALYST
		CGRect rectOfStatusbar = [[UIApplication sharedApplication] statusBarFrame];
		return (sl_ui_len)(rectOfStatusbar.size.height * UIPlatform::getGlobalScaleFactor());
#else
		return 0;
#endif
	}

	UIEdgeInsets MobileApp::getSafeAreaInsets()
	{
		if (@available(iOS 12.0, *)) {
			UIWindow* window = UIPlatform::getMainWindow();
			if (window != nil) {
				::UIEdgeInsets insets = window.safeAreaInsets;
				UIEdgeInsets ret;
				ret.left = (sl_ui_len)(insets.left * UIPlatform::getGlobalScaleFactor());
				ret.top = (sl_ui_len)(insets.top * UIPlatform::getGlobalScaleFactor());
				ret.right = (sl_ui_len)(insets.right * UIPlatform::getGlobalScaleFactor());
				ret.bottom = (sl_ui_len)(insets.bottom * UIPlatform::getGlobalScaleFactor());
				return ret;
			}
		}
		UIEdgeInsets ret;
		ret.left = 0;
		ret.top = getStatusBarHeight();
		ret.right = 0;
		ret.bottom = 0;
		return ret;
	}

}

using namespace slib;
using namespace slib::priv;

@implementation SLIBAppDelegate

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions {

	Log("App", "Finished Launching");

	ResetOrientation();

	MobileApp::Current::invokeStart();

	g_callbackDidFinishLaunching(launchOptions);

	MobileApp::Current::invokeCreateActivity();

	return YES;
}

- (void)applicationWillResignActive:(UIApplication *)application {
	// Sent when the application is about to move from active to inactive state. This can occur for certain types of temporary interruptions (such as an incoming phone call or SMS message) or when the user quits the application and it begins the transition to the background state.
	// Use this method to pause ongoing tasks, disable timers, and throttle down OpenGL ES frame rates. Games should use this method to pause the game.

	Log("App", "Resign Active");

	MobileApp::Current::invokePause();

}

- (void)applicationDidEnterBackground:(UIApplication *)application {
	// Use this method to release shared resources, save user data, invalidate timers, and store enough application state information to restore your application to its current state in case it is terminated later.
	// If your application supports background execution, this method is called instead of applicationWillTerminate: when the user quits.
	Log("App", "Enter Background");

}

- (void)applicationWillEnterForeground:(UIApplication *)application {
	// Called as part of the transition from the background to the inactive state; here you can undo many of the changes made on entering the background.

	Log("App", "Enter Foreground");

	ResetOrientation();

}

- (void)applicationDidBecomeActive:(UIApplication *)application {
	// Restart any tasks that were paused (or not yet started) while the application was inactive. If the application was previously in the background, optionally refresh the user interface.

	Log("App", "Become Active");

	ResetOrientation();

	MobileApp::Current::invokeResume();
}

- (void)applicationWillTerminate:(UIApplication *)application {
	// Called when the application is about to terminate. Save data if appropriate. See also applicationDidEnterBackground:.

	Log("App", "Terminate");

	MobileApp::Current::invokeDestroyActivity();

	MobileApp::Current::invokeExit();
}

- (UIInterfaceOrientationMask)application:(UIApplication *)application supportedInterfaceOrientationsForWindow:(nullable UIWindow *)window
{
	UIInterfaceOrientationMask mask = 0;
	for (ScreenOrientation value : MobileApp::getAvailableScreenOrientations()) {
		switch (value) {
			case ScreenOrientation::Portrait:
				mask |= UIInterfaceOrientationMaskPortrait;
				break;
			case ScreenOrientation::LandscapeRight:
				mask |= UIInterfaceOrientationMaskLandscapeRight;
				break;
			case ScreenOrientation::PortraitUpsideDown:
				mask |= UIInterfaceOrientationMaskPortraitUpsideDown;
				break;
			case ScreenOrientation::LandscapeLeft:
				mask |= UIInterfaceOrientationMaskLandscapeLeft;
				break;
		}
	}
	if (mask) {
		return mask;
	} else {
		return UIInterfaceOrientationMaskAll;
	}
}

- (void)application:(UIApplication *)application didRegisterForRemoteNotificationsWithDeviceToken:(NSData *)deviceToken
{
	g_callbackDidRegisterForRemoteNotifications(deviceToken, nil);
}

- (void)application:(UIApplication *)application didFailToRegisterForRemoteNotificationsWithError:(NSError *)error
{
	g_callbackDidRegisterForRemoteNotifications(nil, error);
}

- (void)application:(UIApplication *)application didReceiveRemoteNotification:(NSDictionary *)userInfo fetchCompletionHandler:(nonnull void (^)(UIBackgroundFetchResult))completionHandler {

	g_callbackDidReceiveRemoteNotification(userInfo);
	completionHandler(UIBackgroundFetchResultNewData);
}

- (BOOL)application:(UIApplication *)application openURL:(NSURL *)url options:(NSDictionary *)options {
	return UIPlatform_onOpenUrl(url, options);
}

- (BOOL)application:(UIApplication *)application continueUserActivity:(NSUserActivity *)userActivity restorationHandler:(void (^)(NSArray<id<UIUserActivityRestoring>> * _Nullable))restorationHandler
{
	if ([userActivity.activityType isEqualToString:NSUserActivityTypeBrowsingWeb]) {
		NSURL* url = userActivity.webpageURL;
		return UIPlatform_onOpenUrl(url, @{});
	}
	return NO;
}

@end

#endif
