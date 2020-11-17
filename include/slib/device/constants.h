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

#ifndef CHECKHEADER_SLIB_DEVICE_CONSTANTS
#define CHECKHEADER_SLIB_DEVICE_CONSTANTS

#include "definition.h"

#include "../core/macro.h"

namespace slib
{

	enum class DeviceAudioCategory
	{
		Default = 0,
		Playback = 1,
		Record = 2,
		PlayAndRecord = 3,
		PlayVideo = 4,
		RecordVideo = 5,
		VideoChat = 6
	};
	
	enum class DeviceAudioMode
	{
		Default = 0,
		Ringtone = 1,
		InCall = 2,
		InCommunication = 3,
		
		Current = -1,
		Invalid = -2
	};

	enum class DeviceRingerMode
	{
		Silent = 0,
		Vibrate = 1,
		Normal = 2
	};

	SLIB_DEFINE_FLAGS(DeviceSetVolumeFlags, {
		ShowUI = 1,
		PlaySound = 4
	})

}

#endif
