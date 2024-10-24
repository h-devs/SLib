/*
 *   Copyright (c) 2008-2024 SLIBIO <https://github.com/SLIBIO>
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

#include "slib/system/setting.h"

#include "slib/system/system.h"
#include "slib/platform.h"

namespace slib
{

	void Setting::openAccessibility()
	{
		[[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString:@"x-apple.systempreferences:com.apple.preference.security?Privacy_Accessibility"]];
	}

	void Setting::openScreenRecording()
	{
		if (@available(macos 10.15, *)) {
			[[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString:@"x-apple.systempreferences:com.apple.preference.security?Privacy_ScreenCapture"]];
		}
	}

	void Setting::openNotifications()
	{
		if (@available(macOS 10.14, *)) {
			[[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString:@"x-apple.systempreferences:com.apple.preference.notifications"]];
		}
	}

	void Setting::resetAccessibility(const StringParam& appBundleId)
	{
		System::execute(String::concat(StringView::literal("tccutil reset Accessibility "), appBundleId));
	}

	void Setting::resetAutomation(const StringParam& appBundleId)
	{
		System::execute(String::concat(StringView::literal("tccutil reset AppleEvents "), appBundleId));
	}

	void Setting::resetScreenRecording(const StringParam& appBundleId)
	{
		if (@available(macos 10.15, *)) {
			System::execute(String::concat(StringView::literal("tccutil reset ScreenCapture "), appBundleId));
		}
	}

	void Setting::resetMicrophone(const StringParam& appBundleId)
	{
		if (@available(macos 10.14, *)) {
			System::execute(String::concat(StringView::literal("tccutil reset Microphone "), appBundleId));
		}
	}

	void Setting::resetCamera(const StringParam& appBundleId)
	{
		if (@available(macos 10.14, *)) {
			System::execute(String::concat(StringView::literal("tccutil reset Camera "), appBundleId));
		}
	}

	void Setting::resetInputMonitoring(const StringParam& appBundleId)
	{
		if (@available(macos 10.15, *)) {
			System::execute(String::concat(StringView::literal("tccutil reset ListenEvent "), appBundleId));
		}
	}

}

#endif
