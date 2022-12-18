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

#include "slib/ui/definition.h"

#if defined(SLIB_UI_IS_GTK)

#include "slib/ui/screen_capture.h"

#include "slib/io/file.h"
#include "slib/ui/platform.h"

namespace slib
{

	namespace priv
	{
		namespace screen_capture
		{

			static Ref<Image> DoCaptureFromGnomeShell()
			{
				auto funcCallSync = gio::getApi_g_dbus_connection_call_sync();
				if (!funcCallSync) {
					return sl_null;
				}
				GDBusConnection* connection = UIPlatform::getDefaultDBusConnection();
				if (!connection) {
					return sl_null;
				}
				Ref<Image> ret;
				const char* szTmpFilePath = "/tmp/.gnome_screenshot.png";
				GVariant* varRet = funcCallSync(
						connection,
						"org.gnome.Shell.Screenshot", // bus_name
						"/org/gnome/Shell/Screenshot", // object_path
						"org.gnome.Shell.Screenshot", // interface
						"Screenshot", // method
						g_variant_new(
							"(bbs)",
							TRUE, // include mouse pointer
							FALSE, // flash
							szTmpFilePath
						),
						sl_null, // reply_type
						G_DBUS_CALL_FLAGS_NONE,
						3000, // timeout
						sl_null, sl_null);
				if (varRet) {
					ret = Image::loadFromFile(szTmpFilePath);
					File::deleteFile(szTmpFilePath);
					g_variant_unref(varRet);
				}
				g_object_unref(connection);
				return ret;
			}

			static Ref<Image> GetImageFromPixbuf(GdkPixbuf* pixbuf)
			{
				GdkColorspace colorspace = gdk_pixbuf_get_colorspace(pixbuf);
				if (colorspace == GDK_COLORSPACE_RGB) {
					int bitsPerSample = gdk_pixbuf_get_bits_per_sample(pixbuf);
					if (bitsPerSample == 8) {
						gboolean flagHasAlpha = gdk_pixbuf_get_has_alpha(pixbuf);
						int nChannelsExpected = flagHasAlpha ? 4 : 3;
						int nChannels = gdk_pixbuf_get_n_channels(pixbuf);
						if (nChannels == nChannelsExpected) {
							int width = gdk_pixbuf_get_width(pixbuf);
							int height = gdk_pixbuf_get_height(pixbuf);
							if (width > 0 && height > 0) {
								sl_uint8* rowSrc = (sl_uint8*)(gdk_pixbuf_get_pixels(pixbuf));
								if (rowSrc) {
									int strideSrc = gdk_pixbuf_get_rowstride(pixbuf);
									Ref<Image> ret = Image::create((sl_uint32)width, (sl_uint32)height);
									if (ret.isNotNull()) {
										Color* dst = ret->getColors();
										for (int i = 0; i < height; i++) {
											sl_uint8* src = rowSrc;
											for (int j = 0; j < width; j++) {
												dst->r = *(src++);
												dst->g = *(src++);
												dst->b = *(src++);
												if (flagHasAlpha) {
													dst->a = *(src++);
												} else {
													dst->a = 255;
												}
												dst++;
											}
											rowSrc += strideSrc;
										}
										return ret;
									}
								}
							}
						}
					}
				}
				return sl_null;
			}

			static Ref<Image> DoCapture(GdkScreen* screen)
			{
				GdkWindow* root = gdk_screen_get_root_window(screen);
				if (root) {
					gint width = gdk_screen_get_width(screen);
					gint height = gdk_screen_get_height(screen);
					if (width > 0 & height > 0) {
						GdkPixbuf* pixbuf;
						if (UIPlatform::isSupportedGtk(3)) {
							pixbuf = gdk::getApi_gdk_pixbuf_get_from_window()(root, 0, 0, width, height);
						} else {
							pixbuf = gdk_pixbuf_get_from_drawable(sl_null, root, sl_null, 0, 0, 0, 0, width, height);
						}
						if (pixbuf) {
							Ref<Image> ret = GetImageFromPixbuf(pixbuf);
							g_object_unref(pixbuf);
							return ret;
						}
					}
				}
				return sl_null;
			}

		}
	}

	using namespace priv::screen_capture;

	Ref<Image> ScreenCapture::takeScreenshot()
	{
		Ref<Image> image = DoCaptureFromGnomeShell();
		if (image.isNotNull()) {
			return image;
		}
		gtk_init_check(sl_null, sl_null);
		GdkScreen* screen = gdk_screen_get_default();
		if (screen) {
			return DoCapture(screen);
		}
		return sl_null;
	}

	Ref<Image> ScreenCapture::takeScreenshotFromCurrentMonitor()
	{
		return takeScreenshot();
	}

	List< Ref<Image> > ScreenCapture::takeScreenshotsFromAllMonitors()
	{
		return List< Ref<Image> >::createFromElement(takeScreenshot());
	}

	sl_uint32 ScreenCapture::getScreenCount()
	{
		return (sl_uint32)1;
	}

}

#endif
