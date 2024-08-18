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

#if defined(SLIB_UI_IS_WIN32)

#include "slib/ui/screen_capture.h"

#include "slib/core/safe_static.h"
#include "slib/graphics/util.h"
#include "slib/graphics/platform.h"
#include "slib/dl/win32/shcore.h"

namespace slib
{

	namespace
	{
		class Helper
		{
		private:
			HDC m_hdcCache;
			HBITMAP m_hbmCache;
			HBITMAP m_hbmCacheOld;
			sl_uint32 m_widthCache;
			sl_uint32 m_heightCache;

		public:
			Helper()
			{
				m_hdcCache = NULL;
				m_hbmCache = NULL;
				m_hbmCacheOld = NULL;
				m_widthCache = 0;
				m_heightCache = 0;
			}

			~Helper()
			{
				freeCache();
			}

		public:
			Ref<Image> getImage(sl_uint32 dstWidth, sl_uint32 dstHeight, HDC hdcSource, sl_int32 srcX, sl_int32 srcY, sl_int32 srcWidth, sl_int32 srcHeight)
			{
				if (!dstWidth || !dstHeight || !srcWidth || !srcHeight) {
					return sl_null;
				}
				if (!m_hdcCache || m_widthCache < dstWidth || m_heightCache < dstHeight) {
					do {
						HDC hdc = CreateCompatibleDC(hdcSource);
						if (hdc) {
							HBITMAP hbm = CreateCompatibleBitmap(hdcSource, (int)dstWidth, (int)dstHeight);
							if (hbm) {
								freeCache();
								SetStretchBltMode(hdc, HALFTONE);
								SetBrushOrgEx(hdc, 0, 0, NULL);
								m_hdcCache = hdc;
								m_hbmCache = hbm;
								m_hbmCacheOld = (HBITMAP)(SelectObject(hdc, hbm));
								m_widthCache = dstWidth;
								m_heightCache = dstHeight;
								break;
							}
							DeleteDC(hdc);
						}
					} while (0);
				}
				if (m_hdcCache && m_hbmCache) {
					if (srcWidth == dstWidth && srcHeight == dstHeight) {
						BitBlt(m_hdcCache, 0, 0, (int)dstWidth, (int)dstHeight, hdcSource, (int)srcX, (int)srcY, SRCCOPY);
					} else {
						StretchBlt(m_hdcCache, 0, 0, (int)dstWidth, (int)dstHeight, hdcSource, (int)srcX, (int)srcY, (int)srcWidth, (int)srcHeight, SRCCOPY);
					}
					Ref<Bitmap> bitmap = GraphicsPlatform::createBitmap(m_hbmCache);
					if (bitmap.isNotNull()) {
						return Image::createCopyBitmap(bitmap, 0, 0, dstWidth, dstHeight);
					}
				}
				return sl_null;
			}

			void freeCache()
			{
				if (m_hdcCache) {
					SelectObject(m_hdcCache, m_hbmCacheOld);
					DeleteDC(m_hdcCache);
					m_hdcCache = NULL;
					m_hbmCacheOld = NULL;
				}
				if (m_hbmCache) {
					DeleteObject(m_hbmCache);
					m_hbmCache = NULL;
				}
				m_widthCache = 0;
				m_heightCache = 0;
			}

		public:
			Mutex m_lock;
		};

		SLIB_SAFE_STATIC_GETTER(Helper, GetHelper)

		static sl_bool CaptureScreen(Screenshot& _out, HDC hDC, HMONITOR hMonitor, sl_uint32 maxWidth, sl_uint32 maxHeight)
		{
			Helper* helper = GetHelper();
			if (!helper) {
				return sl_false;
			}
			MutexLocker lock(&(helper->m_lock));
			MONITORINFOEXW info = {};
			info.cbSize = sizeof(info);
			if (!(GetMonitorInfoW(hMonitor, &info))) {
				return sl_false;
			}
			sl_int32 x, y;
			sl_uint32 screenWidth, screenHeight;
			DEVMODEW dm;
			Base::zeroMemory(&dm, sizeof(dm));
			dm.dmSize = sizeof(dm);
			if (EnumDisplaySettingsW(info.szDevice, ENUM_CURRENT_SETTINGS, &dm)) {
				x = (sl_int32)(dm.dmPosition.x);
				y = (sl_int32)(dm.dmPosition.y);
				screenWidth = (sl_uint32)(dm.dmPelsWidth);
				screenHeight = (sl_uint32)(dm.dmPelsHeight);
			} else {
				x = 0;
				y = 0;
				screenWidth = (sl_uint32)(GetDeviceCaps(hDC, HORZRES));
				screenHeight = (sl_uint32)(GetDeviceCaps(hDC, VERTRES));
			}
			_out.screenWidth = screenWidth;
			_out.screenHeight = screenHeight;
			sl_uint32 dstWidth = screenWidth;
			sl_uint32 dstHeight = screenHeight;
			GraphicsUtil::toSmallSize(dstWidth, dstHeight, maxWidth, maxHeight);
			_out.image = helper->getImage(dstWidth, dstHeight, hDC, x, y, screenWidth, screenHeight);
			return _out.image.isNotNull();
		}
	}

	sl_bool ScreenCapture::takeScreenshot(Screenshot& _out, sl_uint32 maxWidth, sl_uint32 maxHeight)
	{
		sl_bool bRet = sl_false;
		HDC hDC = CreateDCW(L"DISPLAY", NULL, NULL, NULL);
		if (hDC) {
			POINT pt = {0, 0};
			HMONITOR hMonitor = MonitorFromPoint(pt, MONITOR_DEFAULTTOPRIMARY);
			if (hMonitor) {
				bRet = CaptureScreen(_out, hDC, hMonitor, maxWidth, maxHeight);
			}
			DeleteDC(hDC);
		}
		return bRet;
	}

	sl_bool ScreenCapture::takeScreenshotFromCurrentMonitor(Screenshot& _out, sl_uint32 maxWidth, sl_uint32 maxHeight)
	{
		sl_bool bRet = sl_false;
		HDC hDC = CreateDCW(L"DISPLAY", NULL, NULL, NULL);
		if (hDC) {
			POINT pt;
			GetCursorPos(&pt);
			HMONITOR hMonitor = MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);
			if (hMonitor) {
				bRet = CaptureScreen(_out, hDC, hMonitor, maxWidth, maxHeight);
			}
			DeleteDC(hDC);
		}
		return bRet;
	}

	namespace
	{
		struct CaptureScreensContext
		{
			HDC hDC;
			sl_uint32 maxWidth;
			sl_uint32 maxHeight;
			List<Screenshot> list;
		};

		static BOOL CALLBACK EnumDisplayMonitorsCallbackForCaptureScreens(HMONITOR hMonitor, HDC hDC, LPRECT pClip, LPARAM lParam)
		{
			CaptureScreensContext& context = *((CaptureScreensContext*)lParam);
			Screenshot screenshot;
			if (CaptureScreen(screenshot, context.hDC, hMonitor, context.maxWidth, context.maxHeight)) {
				context.list.add_NoLock(Move(screenshot));
			}
			return TRUE;
		}
	}

	List<Screenshot> ScreenCapture::takeScreenshotsFromAllMonitors(sl_uint32 maxWidth, sl_uint32 maxHeight)
	{
		CaptureScreensContext context;
		context.hDC = CreateDCW(L"DISPLAY", NULL, NULL, NULL);
		if (!(context.hDC)) {
			return sl_null;
		}
		context.maxWidth = maxWidth;
		context.maxHeight = maxHeight;
		EnumDisplayMonitors(NULL, NULL, EnumDisplayMonitorsCallbackForCaptureScreens, (LPARAM)&context);
		DeleteDC(context.hDC);
		return context.list;
	}

	namespace
	{
		static BOOL CALLBACK EnumDisplayMonitorsCallbackForGetScreenCount(HMONITOR hMonitor, HDC hDC, LPRECT pClip, LPARAM lParam)
		{
			sl_uint32 *count = (sl_uint32*)lParam;
			(*count)++;
			return TRUE;
		}
	}

	sl_uint32 ScreenCapture::getScreenCount()
	{
		sl_uint32 count = 0;
		EnumDisplayMonitors(NULL, NULL, EnumDisplayMonitorsCallbackForGetScreenCount, (LPARAM)&count);
		return count;
	}

	void ScreenCapture::switchToCurrentDesktop()
	{
		String16 inputDesktopName = Win32::getInputDesktopName();
		if (!inputDesktopName) {
			return;
		}
		if (inputDesktopName != Win32::getCurrentDesktopName()) {
			Win32::switchToInputDesktop();
		}
	}

}

#endif