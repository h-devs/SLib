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

#include "slib/ui/screen_capture.h"

#include "slib/core/safe_static.h"
#include "slib/graphics/platform.h"

#include <AppKit/AppKit.h>
#include <libproc.h>

namespace slib
{

	namespace {

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
					return bitmap->toImage();
				}
				return sl_null;
			}

		private:
			Ref<Bitmap> m_bitmapCache;
			Ref<Canvas> m_canvasCache;
			CGContextRef m_contextCache;

		public:
			Mutex m_lock;

		};

		SLIB_SAFE_STATIC_GETTER(Helper, GetHelper)

		static Ref<Image> TakeScreenshot(Helper* helper, NSScreen* screen)
		{
			CGDirectDisplayID display = (CGDirectDisplayID)([[[screen deviceDescription] objectForKey:@"NSScreenNumber"] unsignedIntValue]);
			if (display) {
				CGImageRef cgImage = CGDisplayCreateImage(display);
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
		for (NSScreen* screen in [NSScreen screens]) {
			Ref<Image> image = TakeScreenshot(helper, screen);
			if (image.isNotNull()) {
				ret.add_NoLock(Move(image));
			}
		}
		return ret;
	}

	sl_uint32 ScreenCapture::getScreenCount()
	{
		sl_uint32 count = (sl_uint32)([[NSScreen screens] count]);
		return count;
	}

	sl_bool ScreenCapture::isScreenRecordingEnabled()
	{
		if (@available(macOS 10.15, *)) {
			return CGPreflightScreenCaptureAccess();
		}
		return YES;
	}

	void ScreenCapture::openSystemPreferencesForScreenRecording()
	{
		if (@available(macos 10.15, *)) {
			[[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString:@"x-apple.systempreferences:com.apple.preference.security?Privacy_ScreenCapture"]];
		}
	}

	void ScreenCapture::requestScreenRecordingAccess()
	{
		if (@available(macos 10.15, *)) {
			ScreenCapture::takeScreenshot();
		}
	}

}
#endif
