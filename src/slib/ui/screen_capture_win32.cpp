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

#include "slib/ui/definition.h"

#if defined(SLIB_UI_IS_WIN32)

#include "slib/ui/screen_capture.h"

#include "slib/core/safe_static.h"
#include "slib/graphics/platform.h"
#include "slib/dl/win32/shcore.h"

namespace slib
{

	namespace {

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
			Ref<Image> getImage(HDC hdcSource, sl_int32 x, sl_int32 y, sl_int32 _width, sl_int32 _height)
			{
				if (_width < 1 || _height < 1) {
					return sl_null;
				}
				sl_uint32 width = (sl_uint32)_width;
				sl_uint32 height = (sl_uint32)_height;
				if (!m_hdcCache || m_widthCache < width || m_heightCache < height) {
					do {
						HDC hdc = CreateCompatibleDC(hdcSource);
						if (hdc) {
							HBITMAP hbm = CreateCompatibleBitmap(hdcSource, (int)width, (int)height);
							if (hbm) {
								freeCache();
								m_hdcCache = hdc;
								m_hbmCache = hbm;
								m_hbmCacheOld = (HBITMAP)(SelectObject(hdc, hbm));
								m_widthCache = width;
								m_heightCache = height;
								break;
							}
							DeleteDC(hdc);
						}
					} while (0);
				}
				if (m_hdcCache && m_hbmCache) {
					BitBlt(m_hdcCache, 0, 0, (int)width, (int)height, hdcSource, x, y, SRCCOPY);
					Ref<Bitmap> bitmap = GraphicsPlatform::createBitmap(m_hbmCache);
					if (bitmap.isNotNull()) {
						return Image::createCopyBitmap(bitmap, 0, 0, width, height);
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

		static Ref<Image> CaptureScreen(HMONITOR hMonitor)
		{
			Helper* helper = GetHelper();
			if (!helper) {
				return sl_null;
			}
			MutexLocker lock(&(helper->m_lock));
			MONITORINFOEXW info;
			Base::zeroMemory(&info, sizeof(info));
			info.cbSize = sizeof(info);
			if (GetMonitorInfoW(hMonitor, &info)) {
				HDC hDC = CreateDCW(L"DISPLAY", info.szDevice, NULL, NULL);
				if (hDC) {
					sl_uint32 width, height;
					DEVMODEW dm;
					Base::zeroMemory(&dm, sizeof(dm));
					dm.dmSize = sizeof(dm);
					if (EnumDisplaySettingsW(info.szDevice, ENUM_CURRENT_SETTINGS, &dm)) {
						width = (sl_uint32)(dm.dmPelsWidth);
						height = (sl_uint32)(dm.dmPelsHeight);
					} else {
						width = (sl_uint32)(GetDeviceCaps(hDC, HORZRES));
						height = (sl_uint32)(GetDeviceCaps(hDC, VERTRES));
					}
					Ref<Image> image = helper->getImage(hDC, 0, 0, width, height);
					DeleteDC(hDC);
					return image;
				}
			}
			return sl_null;
		}

	}

	Ref<Image> ScreenCapture::takeScreenshot()
	{
		POINT pt = { 0, 0 };
		HMONITOR hMonitor = MonitorFromPoint(pt, MONITOR_DEFAULTTOPRIMARY);
		if (hMonitor) {
			return CaptureScreen(hMonitor);
		}
		return sl_null;
	}

	Ref<Image> ScreenCapture::takeScreenshotFromCurrentMonitor()
	{
		POINT pt;
		GetCursorPos(&pt);
		HMONITOR hMonitor = MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);
		if (hMonitor) {
			return CaptureScreen(hMonitor);
		}
		return sl_null;
	}

	namespace {
		struct CaptureScreensContext
		{
			List< Ref<Image> > list;
		};

		static BOOL CALLBACK EnumDisplayMonitorsCallbackForCaptureScreens(HMONITOR hMonitor, HDC hDC, LPRECT pClip, LPARAM lParam)
		{
			CaptureScreensContext& context = *((CaptureScreensContext*)lParam);
			Ref<Image> image = CaptureScreen(hMonitor);
			if (image.isNotNull()) {
				context.list.add_NoLock(Move(image));
			}
			return TRUE;
		}
	}

	List< Ref<Image> > ScreenCapture::takeScreenshotsFromAllMonitors()
	{
		CaptureScreensContext context;
		EnumDisplayMonitors(NULL, NULL, EnumDisplayMonitorsCallbackForCaptureScreens, (LPARAM)&context);
		return context.list;
	}

	namespace {
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

}

#endif