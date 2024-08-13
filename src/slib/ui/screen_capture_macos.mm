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

#include "slib/ui/definition.h"

#if defined(SLIB_UI_IS_MACOS)

#include "slib/ui/screen_capture.h"

#include "slib/system/system.h"
#include "slib/core/event.h"
#include "slib/core/shared.h"
#include "slib/core/safe_static.h"
#include "slib/graphics/platform.h"

#include <AppKit/AppKit.h>
#ifdef __MAC_14_0
#include <ScreenCaptureKit/ScreenCaptureKit.h>
#define USE_SCREEN_CAPTURE_KIT
#endif

namespace slib
{

	namespace
	{
		class Helper
		{
		public:
			Ref<Image> getImage(CGImageRef cgImage, CGFloat scale)
			{
				sl_uint32 srcWidth = (sl_uint32)(CGImageGetWidth(cgImage));
				sl_uint32 srcHeight = (sl_uint32)(CGImageGetHeight(cgImage));
				sl_uint32 dstWidth = (sl_uint32)(srcWidth * scale);
				sl_uint32 dstHeight = (sl_uint32)(srcHeight * scale);
				Ref<Bitmap>& bitmap = m_bitmapCache;
				Ref<Canvas>& canvas = m_canvasCache;
				CGContextRef& context = m_contextCache;
				if (bitmap.isNull() || canvas.isNull() || bitmap->getWidth() < dstWidth || bitmap->getHeight() < dstHeight) {
					bitmap = Bitmap::create(dstWidth, dstHeight);
					if (bitmap.isNull()) {
						return sl_null;
					}
					canvas = bitmap->getCanvas();
					if (canvas.isNull()) {
						return sl_null;
					}
					context = GraphicsPlatform::getCanvasHandle(canvas.get());
					CGContextTranslateCTM(context, 0, (CGFloat)dstHeight);
					CGContextScaleCTM(context, 1, -1);
				}
				if (context) {
					CGContextDrawImage(context, CGRectMake(0, 0, (CGFloat)dstWidth, (CGFloat)dstHeight), cgImage);
					return Image::createCopyBitmap(bitmap, 0, bitmap->getHeight() - dstHeight, dstWidth, dstHeight);
				}
				return sl_null;
			}

		private:
			Ref<Bitmap> m_bitmapCache;
			Ref<Canvas> m_canvasCache;
			CGContextRef m_contextCache;

		public:
			Mutex m_lock;
#ifdef USE_SCREEN_CAPTURE_KIT
			Mutex m_lockShareableContent;
			id m_shareableContent;
#endif
		};

		SLIB_SAFE_STATIC_GETTER(Helper, GetHelper)

#ifdef USE_SCREEN_CAPTURE_KIT
		API_AVAILABLE(macos(14.0))
		static Ref<Image> TakeScreenshot(Helper* helper, SCDisplay* display)
		{
			SCContentFilter* filter = [[SCContentFilter alloc] initWithDisplay:display excludingWindows:@[]];
			SCStreamConfiguration* config = [[SCStreamConfiguration alloc] init];
			[config setWidth:(sl_uint32)([display width])];
			[config setHeight:(sl_uint32)([display height])];
			Ref<Event> ev = Event::create();
			if (ev.isNull()) {
				return nil;
			}
			Shared< AtomicRef<Image> > ret = Shared< AtomicRef<Image> >::create();
			if (ret.isNull()) {
				return nil;
			}
			[SCScreenshotManager captureImageWithFilter:filter configuration:config completionHandler:^(CGImageRef cgImage, NSError* error) {
				if (cgImage) {
					Ref<Image> image = helper->getImage(cgImage, 1.0);
					CGImageRelease(cgImage);
					*ret = Move(image);
				}
				ev->set();
			}];
			ev->wait(5000);
			return ret->release();
		}

		API_AVAILABLE(macos(14.0))
		static SCShareableContent* GetShareableContent(Helper* helper)
		{
			Ref<Event> ev = Event::create();
			if (ev.isNull()) {
				return nil;
			}
			[SCShareableContent getShareableContentWithCompletionHandler:^(SCShareableContent* content, NSError* err) {
				{
					MutexLocker lock(&(helper->m_lockShareableContent));
					helper->m_shareableContent = content;
				}
				ev->set();
			}];
			ev->wait(3000);
			MutexLocker lock(&(helper->m_lockShareableContent));
			return helper->m_shareableContent;
		}

		API_AVAILABLE(macos(14.0))
		static SCDisplay* GetShareableDisplay(Helper* helper, CGDirectDisplayID displayId)
		{
			SCShareableContent* content = GetShareableContent(helper);
			if (content == nil) {
				return nil;
			}
			for (SCDisplay* display in [content displays]) {
				if ([display displayID] == displayId) {
					return display;
				}
			}
			return nil;
		}
#endif

		static sl_bool GetDisplayID(NSScreen* screen, CGDirectDisplayID& ret)
		{
			NSDictionary* desc = [screen deviceDescription];
			if (desc == nil) {
				return sl_false;
			}
			NSNumber* num = [desc objectForKey:@"NSScreenNumber"];
			if (num == nil) {
				return sl_false;
			}
			ret = (CGDirectDisplayID)([num unsignedIntValue]);
			return sl_true;
		}

		static Ref<Image> TakeScreenshot(Helper* helper, NSScreen* screen)
		{
			CGDirectDisplayID displayId;
			if (!(GetDisplayID(screen, displayId))) {
				return sl_null;
			}
#ifdef USE_SCREEN_CAPTURE_KIT
			if (@available(macos 14.0, *)) {
				SCDisplay* display = GetShareableDisplay(helper, displayId);
				if (display != nil) {
					return TakeScreenshot(helper, display);
				}
			} else
#endif
			{
				CGImageRef cgImage = CGDisplayCreateImage(displayId);
				if (cgImage) {
					Ref<Image> image = helper->getImage(cgImage, 1 / [screen backingScaleFactor]);
					CGImageRelease(cgImage);
					return image;
				}
			}
			return sl_null;
		}

	}

	Ref<Image> ScreenCapture::takeScreenshot()
	{
		Helper* helper = GetHelper();
		if (!helper) {
			return sl_null;
		}
		MutexLocker lock(&(helper->m_lock));
		return TakeScreenshot(helper, [[NSScreen screens] objectAtIndex:0]);
	}

	Ref<Image> ScreenCapture::takeScreenshotFromCurrentMonitor()
	{
		Helper* helper = GetHelper();
		if (!helper) {
			return sl_null;
		}
		MutexLocker lock(&(helper->m_lock));
		return TakeScreenshot(helper, [NSScreen mainScreen]);
	}

	List< Ref<Image> > ScreenCapture::takeScreenshotsFromAllMonitors()
	{
		Helper* helper = GetHelper();
		if (!helper) {
			return sl_null;
		}
		MutexLocker lock(&(helper->m_lock));
		List< Ref<Image> > ret;
#ifdef USE_SCREEN_CAPTURE_KIT
		if (@available(macos 14.0, *)) {
			SCShareableContent* content = GetShareableContent(helper);
			for (SCDisplay* display in content.displays) {
				Ref<Image> image = TakeScreenshot(helper, display);
				if (image.isNotNull()) {
					ret.add_NoLock(Move(image));
				}
			}
		} else
#endif
		{
			for (NSScreen* screen in [NSScreen screens]) {
				Ref<Image> image = TakeScreenshot(helper, screen);
				if (image.isNotNull()) {
					ret.add_NoLock(Move(image));
				}
			}
		}
		return ret;
	}

	sl_uint32 ScreenCapture::getScreenCount()
	{
		sl_uint32 count = (sl_uint32)([[NSScreen screens] count]);
		return count;
	}

	sl_bool ScreenCapture::isEnabled()
	{
		if (@available(macOS 10.15, *)) {
			return CGPreflightScreenCaptureAccess();
		} else {
			return sl_true;
		}
	}

	void ScreenCapture::openSystemPreferences()
	{
		if (@available(macos 10.15, *)) {
			[[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString:@"x-apple.systempreferences:com.apple.preference.security?Privacy_ScreenCapture"]];
		}
	}

	void ScreenCapture::requestAccess()
	{
#ifdef USE_SCREEN_CAPTURE_KIT
		if (@available(macos 14.0, *)) {
			[SCShareableContent getShareableContentWithCompletionHandler:^(SCShareableContent*, NSError*) {}];
		} else
#endif
		if (@available(macos 10.15, *)) {
			ScreenCapture::takeScreenshot();
		}
	}

	void ScreenCapture::resetAccess(const StringParam& appBundleId)
	{
		if (@available(macos 10.15, *)) {
			System::execute(String::concat(StringView::literal("tccutil reset ScreenCapture "), appBundleId));
		}
	}

}

#endif
