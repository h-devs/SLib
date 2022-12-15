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

#include "slib/ui/dl/linux/gdk.h"
#include "slib/ui/dl/linux/gtk.h"

#include <stdio.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <linux/hdreg.h>

using namespace std;

#define CHASSIS_SERIAL "/sys/devices/virtual/dmi/id/chassis_serial"

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

	Sizei Device::getScreenSize()
	{
		gtk_init_check(NULL, NULL);
		GdkScreen* screen = gdk_screen_get_default();
		if (screen) {
			Sizei ret;
			ret.x = (int)(gdk_screen_get_width(screen));
			ret.y = (int)(gdk_screen_get_height(screen));
			return ret;
		}
		return Sizei::zero();
	}

	String Device::getBoardSerialNumber() 
	{
		String serial;
		FILE *fChassis;
		char line[100] = { 0 };

		if (fChassis = fopen(CHASSIS_SERIAL, "r"))
		{
			if (fgets(line, sizeof(line), fChassis) && strlen(line) > 0)
			{
				serial = String::from(line);
			}
			fclose(fChassis);
		}
		else
		{
			printf("Cannot read this board serial number.\n");
		}
		return serial;
	}
}

#endif
