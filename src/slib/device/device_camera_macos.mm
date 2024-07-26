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

#include "slib/media/definition.h"

#if defined(SLIB_PLATFORM_IS_MACOS)

#include "slib/device/device.h"

#include "slib/core/scoped_buffer.h"

#import <CoreMediaIO/CoreMediaIO.h>

namespace slib
{

	sl_bool Device::isUsingCamera()
	{
		CMIOObjectPropertyAddress address;
		address.mSelector = kCMIOHardwarePropertyDevices;
		address.mScope = kCMIOObjectPropertyScopeGlobal;
		address.mElement = kCMIOObjectPropertyElementMain;
		UInt32 nDataSize = 0;
		CMIOObjectGetPropertyDataSize(kCMIOObjectSystemObject, &address, 0, nil, &nDataSize);
		if (!nDataSize) {
			return sl_false;
		}
		sl_uint32 nDevices = nDataSize / sizeof(CMIOObjectID);
		SLIB_SCOPED_BUFFER(CMIOObjectID, 16, deviceIds, nDevices)
		if (!deviceIds) {
			return sl_false;
		}
		UInt32 nDataUsed = 0;
		CMIOObjectGetPropertyData(CMIOObjectID(kCMIOObjectSystemObject), &address, 0, nil, nDataSize, &nDataUsed, deviceIds);
		sl_bool bRet = sl_false;
		nDevices = nDataUsed / sizeof(CMIOObjectID);
		for (sl_uint32 i = 0; i < nDevices; i++) {
			CMIOObjectID deviceId = deviceIds[i];
			address.mSelector = kCMIODevicePropertyDeviceIsRunningSomewhere;
			address.mScope = kCMIOObjectPropertyScopeWildcard;
			address.mElement = kCMIOObjectPropertyElementWildcard;
			nDataSize = 0;
			CMIOObjectGetPropertyDataSize(deviceId, &address, 0, nil, &nDataSize);
			SLIB_SCOPED_BUFFER(sl_uint8, 64, data, nDataSize)
			if (data) {
				nDataUsed = 0;
				CMIOObjectGetPropertyData(deviceId, &address, 0, nil, nDataSize, &nDataUsed, data);
				if (nDataUsed) {
					if (data[0]) {
						bRet = sl_true;
					}
				}
			}
		}
		return bRet;
	}

}

#endif
