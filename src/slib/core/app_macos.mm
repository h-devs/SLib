/*
 *   Copyright (c) 2008-2021 SLIBIO <https://github.com/SLIBIO>
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

#include "slib/core/definition.h"

#if defined(SLIB_PLATFORM_IS_MACOS)

#include "slib/core/app.h"

#include "slib/system/system.h"
#include "slib/platform.h"

#include <AppKit/AppKit.h>

namespace slib
{

	sl_bool Application::isAccessibilityEnabled()
	{
		return AXIsProcessTrustedWithOptions(NULL) != FALSE;
	}

	void Application::authenticateAccessibility()
	{
		NSDictionary *options = @{(__bridge NSString*)kAXTrustedCheckOptionPrompt: @YES};
		AXIsProcessTrustedWithOptions((CFDictionaryRef)options);
	}

	void Application::openSystemPreferencesForAccessibility()
	{
		[[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString:@"x-apple.systempreferences:com.apple.preference.security?Privacy_Accessibility"]];
	}

	namespace {
		static void SetBundleLoginItemEnabled(const StringParam& path, sl_bool flagEnabled)
		{
			if (path.isEmpty()) {
				return;
			}

			NSURL *itemURL = [NSURL fileURLWithPath:(Apple::getNSStringFromString(path))];
			LSSharedFileListItemRef existingItem = NULL;

			LSSharedFileListRef loginItems = LSSharedFileListCreate(NULL, kLSSharedFileListSessionLoginItems, NULL);

			if(loginItems) {
				UInt32 seed = 0U;
				NSArray *currentLoginItems = CFBridgingRelease(LSSharedFileListCopySnapshot(loginItems, &seed));
				for (id itemObject in currentLoginItems) {
					LSSharedFileListItemRef item = (__bridge LSSharedFileListItemRef)itemObject;
					UInt32 resolutionFlags = kLSSharedFileListNoUserInteraction | kLSSharedFileListDoNotMountVolumes;
					CFURLRef URL = NULL;
					OSStatus err = LSSharedFileListItemResolve(item, resolutionFlags, &URL, NULL);
					if (err == noErr) {
						Boolean foundIt = CFEqual(URL, (__bridge CFTypeRef)(itemURL));
						CFRelease(URL);
						if (foundIt) {
							existingItem = item;
							break;
						}
					}
				}
				if (flagEnabled) {
					if (existingItem == NULL) {
						LSSharedFileListInsertItemURL(loginItems, kLSSharedFileListItemBeforeFirst, NULL, NULL, (__bridge CFURLRef)itemURL, NULL, NULL);
					}
				} else {
					if (existingItem != NULL) {
						LSSharedFileListItemRemove(loginItems, existingItem);
					}
				}
				CFRelease(loginItems);
			}
		}
	}

	void Application::registerRunAtStartup(const StringParam& path)
	{
		SetBundleLoginItemEnabled(path, sl_true);
	}

	void Application::registerRunAtStartup()
	{
		SetBundleLoginItemEnabled(System::getMainBundlePath(), sl_true);
	}

	void Application::unregisterRunAtStartup(const StringParam& path)
	{
		SetBundleLoginItemEnabled(path, sl_false);
	}

	void Application::unregisterRunAtStartup()
	{
		SetBundleLoginItemEnabled(System::getMainBundlePath(), sl_false);
	}

}

#endif
