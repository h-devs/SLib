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

#include "slib/device/definition.h"

#if defined(SLIB_UI_IS_GTK)

#include "slib/device/device.h"

#include "slib/io/file.h"
#include "slib/core/safe_static.h"
#include "slib/dl/linux/gdk.h"
#include "slib/dl/linux/gtk.h"

namespace slib
{

	double Device::getScreenPPI()
	{
		gtk_init_check(NULL, NULL);
		GdkScreen* screen = gdk_screen_get_default();
		if (screen) {
			return gdk_screen_get_resolution(screen);
		}
		return 96;
	}

	SizeI Device::getScreenSize()
	{
		gtk_init_check(NULL, NULL);
		GdkScreen* screen = gdk_screen_get_default();
		if (screen) {
			SizeI ret;
			ret.x = (int)(gdk_screen_get_width(screen));
			ret.y = (int)(gdk_screen_get_height(screen));
			return ret;
		}
		return SizeI::zero();
	}

	// Requires root privilege
	String Device::getBoardSerialNumber() 
	{
        SLIB_SAFE_LOCAL_STATIC(String, ret, File::readAllTextUTF8("/sys/devices/virtual/dmi/id/chassis_serial"))
        return ret;
	}

}

#endif
