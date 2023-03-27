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

#include "slib/device/definition.h"

#if defined(SLIB_PLATFORM_IS_TIZEN)

#include "slib/device/device.h"

#include <system_info.h>
#include <stdlib.h>

namespace slib
{

	String Device::getDeviceId()
	{
		char *value = NULL;
		int ret = system_info_get_platform_string("http://tizen.org/system/tizenid", &value);
		if (ret == SYSTEM_INFO_ERROR_NONE) {
			String deviceId = value;
			free(value);
			return deviceId;
		}
		return sl_null;
	}

	SizeI Device::getScreenSize()
	{
		SizeI size;
		int value;
		int ret = system_info_get_platform_int("http://tizen.org/feature/screen.height", &value);
		if (ret != SYSTEM_INFO_ERROR_NONE) {
			return size;
		}
		size.x = value;
		ret = system_info_get_platform_int("http://tizen.org/feature/screen.width", &value);
		if (ret != SYSTEM_INFO_ERROR_NONE) {
			return size;
		}
		size.y = value;
		return size;
	}

	double Device::getScreenPPI()
	{
		int value;
		int ret = system_info_get_platform_int("http://tizen.org/feature/screen.dpi", &value);
		if (ret != SYSTEM_INFO_ERROR_NONE) {
			return -1;
		}
		return value;
	}

}

#endif
