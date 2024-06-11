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

#if defined(SLIB_UI_IS_MACOS)

#include "slib/ui/core.h"

#include "slib/ui/screen.h"
#include "slib/ui/window.h"
#include "slib/ui/app.h"
#include "slib/core/safe_static.h"
#include "slib/ui/platform.h"

#include "ui_core_common.h"

@interface SLIBAppDelegate : NSObject <NSApplicationDelegate>
@end

namespace slib
{

	using namespace priv;

	namespace {

		class ScreenImpl : public Screen
		{
		public:
			NSScreen* m_screen;

		public:
			static Ref<ScreenImpl> create(NSScreen* screen)
			{
				if (screen != nil) {
					Ref<ScreenImpl> ret = new ScreenImpl();
					if (ret.isNotNull()) {
						ret->m_screen = screen;
						return ret;
					}
				}
				return sl_null;
			}

			UIRect getRegion() override
			{
				NSRect rect = [m_screen frame];
				return convertRect(rect);
			}

			UIRect getWorkingRegion() override
			{
				NSRect rect = [m_screen visibleFrame];
				return convertRect(rect);
			}

			static UIRect convertRect(const NSRect& rect)
			{
				sl_ui_pos leftBottom = 0;
				NSScreen* primary = getPrimaryScreen();
				if (primary != nil) {
					NSRect rect = [primary frame];
					leftBottom = (sl_ui_pos)(rect.origin.y + rect.size.height);
				}
				UIRect region;
				region.left = (sl_ui_pos)(rect.origin.x);
				region.top = leftBottom - (sl_ui_pos)(rect.origin.y + rect.size.height);
				region.setWidth((sl_ui_pos)(rect.size.width));
				region.setHeight((sl_ui_pos)(rect.size.height));
				return region;
			}
			
			static NSScreen* getPrimaryScreen()
			{
				NSArray* arr = [NSScreen screens];
				NSUInteger n = [arr count];
				if (!n) {
					return nil;
				}
				return [arr objectAtIndex:0];
			}

		};

	}

	Ref<Screen> UIPlatform::createScreen(NSScreen* screen)
	{
		return ScreenImpl::create(screen);
	}

	NSScreen* UIPlatform::getScreenHandle(Screen* _screen)
	{
		ScreenImpl* screen = (ScreenImpl*)_screen;
		if (screen) {
			return screen->m_screen;
		}
		return nil;
	}

	float UIPlatform::getDpiForScreen(NSScreen* screen)
	{
		NSDictionary *description = [screen deviceDescription];
		NSSize displayPixelSize = [[description objectForKey:NSDeviceSize] sizeValue];
		CGSize displayPhysicalSize = CGDisplayScreenSize([[description objectForKey:@"NSScreenNumber"] unsignedIntValue]);
		return displayPixelSize.width / displayPhysicalSize.width * 25.4f;
	}

	List< Ref<Screen> > UI::getScreens()
	{
		NSArray* arr = [NSScreen screens];
		sl_size n = [arr count];
		if (!n) {
			return sl_null;
		}
		List< Ref<Screen> > ret;
		for (sl_size i = 0; i < n; i++) {
			NSScreen* screen = [arr objectAtIndex:i];
			ret.add_NoLock(ScreenImpl::create(screen));
		}
		return ret;
	}

	Ref<Screen> UI::getPrimaryScreen()
	{
		NSScreen* screen = ScreenImpl::getPrimaryScreen();
		return UIPlatform::createScreen(screen);
	}

	UIRect UI::getScreenWorkingRegion()
	{
		NSScreen* screen = [NSScreen mainScreen];
		if (screen != nil) {
			return ScreenImpl::convertRect(screen.visibleFrame);
		}
		return UIRect::zero();
	}

	sl_bool UI::isUiThread()
	{
		return [NSThread isMainThread];
	}

	namespace {
		class DispatchContext
		{
		public:
			NSEvent* dispatchEvent;

		public:
			DispatchContext()
			{
				dispatchEvent = [NSEvent otherEventWithType:NSEventTypeApplicationDefined location:NSMakePoint(0, 0) modifierFlags:0 timestamp:0 windowNumber:0 context:nil subtype:0 data1:0 data2:0];
			}

		};

		SLIB_SAFE_STATIC_GETTER(DispatchContext, GetDispatchContext)
	}

	void UI::dispatchToUiThread(const Function<void()>& callback, sl_uint32 delayMillis)
	{
		if (callback.isNull()) {
			return;
		}
		DispatchContext* context = GetDispatchContext();
		if (!context) {
			return;
		}
		if (delayMillis == 0) {
			if (UIDispatcher::addCallback(callback)) {
				if ([NSThread isMainThread]) {
					[NSApp postEvent:context->dispatchEvent atStart:YES];
				} else {
					dispatch_async(dispatch_get_main_queue(), ^{
						[NSApp postEvent:context->dispatchEvent atStart:YES];
					});
				}
			}
		} else {
			Function<void()> refCallback(callback);
			dispatch_time_t t = dispatch_time(DISPATCH_TIME_NOW, (sl_int64)(delayMillis) * NSEC_PER_MSEC);
			dispatch_after(t, dispatch_get_main_queue(), ^{
				if (UIDispatcher::addCallback(refCallback)) {
					[NSApp postEvent:context->dispatchEvent atStart:YES];
				}
			});
		}
	}

	void UI::dispatchToUiThreadUrgently(const Function<void()>& callback, sl_uint32 delayMillis)
	{
		Function<void()> refCallback(callback);
		if (delayMillis == 0) {
			dispatch_async(dispatch_get_main_queue(), ^{
				refCallback();
			});
		} else {
			dispatch_time_t t = dispatch_time(DISPATCH_TIME_NOW, (sl_int64)(delayMillis) * NSEC_PER_MSEC);
			dispatch_after(t, dispatch_get_main_queue(), ^{
				refCallback();
			});
		}
	}

	void UI::openUrl(const StringParam& _url)
	{
		if (_url.isNotEmpty()) {
			NSString* s = Apple::getNSStringFromString(_url);
			NSURL* url = [NSURL URLWithString:s];
			[[NSWorkspace sharedWorkspace] openURL:url];
		}
	}

	String UI::getActiveApplicationName()
	{
		NSRunningApplication* app = [[NSWorkspace sharedWorkspace] frontmostApplication];
		if (app != nil) {
			return Apple::getStringFromNSString(app.localizedName);
		}
		return sl_null;
	}

	String UI::getActiveWindowTitle(sl_int32 timeout)
	{
		String ret;
		NSRunningApplication* app = [[NSWorkspace sharedWorkspace] frontmostApplication];
		if (app != nil) {
			AXUIElementRef axApp = AXUIElementCreateApplication([app processIdentifier]);
			if (axApp) {
				AXUIElementRef axWindow = NULL;
				AXUIElementCopyAttributeValue(axApp, kAXFocusedWindowAttribute, (CFTypeRef*)&axWindow);
				if (axWindow) {
					CFStringRef title = NULL;
					AXUIElementCopyAttributeValue(axWindow, kAXTitleAttribute, (CFTypeRef*)&title);
					if (title) {
						ret = Apple::getStringFromNSString((__bridge NSString*)title);
						CFRelease(title);
					}
					CFRelease(axWindow);
				}
				CFRelease(axApp);
			}
		}
		return ret;
	}

	void UIPlatform::runLoop(sl_uint32 level)
	{
		for (;;) {
			@autoreleasepool {
				NSDate* date = [NSDate dateWithTimeIntervalSinceNow:1000];
				NSEvent* ev = [NSApp nextEventMatchingMask:NSAnyEventMask untilDate:date inMode:NSDefaultRunLoopMode dequeue:YES];
				if (ev != nil) {
					if ([ev type] == NSApplicationDefined && [ev subtype] == NSPowerOffEventType) {
						break;
					}
					[NSApp sendEvent:ev];
				}
			}
		}
	}

	void UIPlatform::quitLoop()
	{
		NSEvent* ev = [NSEvent otherEventWithType: NSApplicationDefined
											location: NSMakePoint(0,0)
									modifierFlags: 0
										timestamp: 0.0
										windowNumber: 0
											context: nil
											subtype: NSPowerOffEventType
											data1: 0
											data2: 0];
		[NSApp postEvent:ev atStart:YES];
	}

	void UIPlatform::initApp()
	{
		[NSApplication sharedApplication];

		[[NSBundle mainBundle] loadNibNamed:@"MainMenu" owner:NSApp topLevelObjects:nil];

		SLIBAppDelegate * delegate = [[SLIBAppDelegate alloc] init];
		[NSApp setDelegate:delegate];

		[NSEvent addLocalMonitorForEventsMatchingMask:(NSApplicationDefinedMask | NSKeyDownMask) handler:^(NSEvent* event){
			NSEventType type = [event type];
			if (type == NSKeyDown) {
				NSEventModifierFlags modifiers = [event modifierFlags] & NSDeviceIndependentModifierFlagsMask;
				NSString* key = [event charactersIgnoringModifiers];
				if (modifiers == NSCommandKeyMask) {
					if ([key isEqualToString:@"x"]) {
						if ([NSApp sendAction:NSSelectorFromString(@"cut:") to:nil from:NSApp]) {
							return (NSEvent*)nil;
						}
					} else if ([key isEqualToString:@"c"]) {
						if ([NSApp sendAction:NSSelectorFromString(@"copy:") to:nil from:NSApp]) {
							return (NSEvent*)nil;
						}
					} else if ([key isEqualToString:@"v"]) {
						if ([NSApp sendAction:NSSelectorFromString(@"paste:") to:nil from:NSApp]) {
							return (NSEvent*)nil;
						}
					} else if ([key isEqualToString:@"z"]) {
						if ([NSApp sendAction:NSSelectorFromString(@"undo:") to:nil from:NSApp]) {
							return (NSEvent*)nil;
						}
					} else if ([key isEqualToString:@"a"]) {
						if ([NSApp sendAction:NSSelectorFromString(@"selectAll:") to:nil from:NSApp]) {
							return (NSEvent*)nil;
						}
					}
				} else if (modifiers == (NSCommandKeyMask | NSShiftKeyMask)) {
					if ([key isEqualToString:@"Z"]) {
						if ([NSApp sendAction:NSSelectorFromString(@"redo:") to:nil from:NSApp]) {
							return (NSEvent*)nil;
						}
					}
				}
			} else if (type == NSApplicationDefined) {
				UIDispatcher::processCallbacks();
				return (NSEvent*)nil;
			}
			return event;
		}];
	}

	namespace {
		SLIB_GLOBAL_ZERO_INITIALIZED(Function<void()>, g_customMessageLoop)
		SLIB_GLOBAL_ZERO_INITIALIZED(Function<void()>, g_customQuitApp)
	}

	namespace priv
	{
		void SetCustomMessageLoop(const Function<void()>& func)
		{
			g_customMessageLoop = func;
		}

		void SetCustomQuitApp(const Function<void()>& func)
		{
			g_customQuitApp = func;
		}
	}

	void UIPlatform::runApp()
	{
		if (g_customMessageLoop.isNotNull()) {
			g_customMessageLoop();
		} else {
			@autoreleasepool {
				[NSApp run];
			}
		}
	}

	void UIPlatform::quitApp()
	{
		Ref<Application> app = Application::getApp();
		if (app.isNotNull()) {
			app->dispatchQuitApp();
		}
		dispatch_async(dispatch_get_main_queue(), ^{
			if (g_customQuitApp.isNotNull()) {
				g_customQuitApp();
			} else {
				[NSApp terminate:nil];
			}
		});
	}

	namespace {
		SLIB_GLOBAL_ZERO_INITIALIZED(AtomicFunction<void(NSNotification*)>, g_callbackDidFinishLaunching);
	}

	void UIPlatform::registerDidFinishLaunchingCallback(const Function<void(NSNotification*)>& callback)
	{
		g_callbackDidFinishLaunching.add(callback);
	}

	sl_int32 UIApp::onExistingInstance()
	{
		String uid = getApplicationId();
		if (uid.isEmpty()) {
			return -1;
		}
		NSArray* arr = [NSRunningApplication runningApplicationsWithBundleIdentifier:[[NSBundle mainBundle] bundleIdentifier]];
		if (arr.count > 0) {
			NSRunningApplication* app = [arr objectAtIndex:0];
			if ([app.bundleURL.absoluteString isEqualToString:([NSRunningApplication currentApplication].bundleURL.absoluteString)]) {
				[app activateWithOptions:NSApplicationActivateIgnoringOtherApps];
			} else {
				[[NSWorkspace sharedWorkspace] launchApplication:app.bundleURL.path];
			}
			return 0;
		} else {
			NSLog(@"Running application is not found! bundleId=%@", [[NSBundle mainBundle] bundleIdentifier]);
			return -1;
		}
	}

	sl_bool UIApp::isMenuBarVisible()
	{
		return [NSMenu menuBarVisible] == YES;
	}

	void UIApp::setMenuBarVisible(sl_bool flagVisible)
	{
		[NSMenu setMenuBarVisible:flagVisible ? YES : NO];
	}

	void UIApp::setVisibleOnDock(sl_bool flagVisible)
	{
		ProcessSerialNumber psn = { 0, kCurrentProcess };
		if (flagVisible) {
			TransformProcessType(&psn, kProcessTransformToForegroundApplication);
		} else {
			TransformProcessType(&psn, kProcessTransformToUIElementApplication);
		}
	}

	void UIApp::activate(sl_bool flagIgnoreOtherApps)
	{
		[NSApp activateIgnoringOtherApps:(flagIgnoreOtherApps ? YES : NO)];
	}

	void UIApp::setBadgeNumber(sl_uint32 number)
	{
		if (number) {
			[[NSApp dockTile] setBadgeLabel:[NSString stringWithFormat:@"%d", number]];
		} else {
			[[NSApp dockTile] setBadgeLabel:nil];
		}
	}

}

using namespace slib;

@implementation SLIBAppDelegate

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification
{
	UIApp::Current::invokeStart();
	g_callbackDidFinishLaunching(aNotification);
}

- (void)applicationWillTerminate:(NSNotification *)aNotification
{
	UIApp::Current::invokeExit();
}

- (void)application:(NSApplication *)application openURLs:(NSArray<NSURL*>*)urls
{
	List<String> list;
	for (NSURL* url : urls) {
		list.add_NoLock(Apple::getStringFromNSString(url.absoluteString));
	}
	if (list.getCount() == 1) {
		UIApp::Current::invokeOpenUrl(list[0]);
	} else {
		UIApp::Current::invokeOpenUrls(list);
	}
}

- (BOOL)application:(NSApplication *)sender openFile:(NSString *)filename
{
	return UIApp::Current::invokeOpenFile(Apple::getStringFromNSString(filename));
}

- (void)application:(NSApplication *)sender openFiles:(NSArray<NSString*>*)filenames
{
	List<String> list;
	for (NSString* path : filenames) {
		list.add_NoLock(Apple::getStringFromNSString(path));
	}
	UIApp::Current::invokeOpenUrls(list);
}

- (BOOL)application:(NSApplication *)sender openTempFile:(NSString *)filename
{
	return UIApp::Current::invokeOpenTempFile(Apple::getStringFromNSString(filename));
}

- (BOOL)applicationOpenUntitledFile:(NSApplication *)sender
{
	return UIApp::Current::invokeOpenUntitledFile();
}

- (BOOL)applicationShouldOpenUntitledFile:(NSApplication *)sender
{
	Ref<UIApp> app = UIApp::getApp();
	if (app.isNotNull()) {
		return app->shouldOpenUntitledFile();
	}
	return NO;
}

- (BOOL)applicationShouldHandleReopen:(NSApplication *)sender hasVisibleWindows:(BOOL)flag
{
	return UIApp::Current::invokeReopen(sl_null, flag);
}

@end

#endif
