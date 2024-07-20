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

#include "slib/device/device.h"

namespace slib
{

	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(SoundDeviceInfo)

	SoundDeviceInfo::SoundDeviceInfo()
	{
	}


#if !defined(SLIB_PLATFORM_IS_IOS)
	void Device::setAudioCategory(DeviceAudioCategory category)
	{
	}
#endif

#if !defined(SLIB_PLATFORM_IS_ANDROID)
	DeviceAudioMode Device::getAudioMode()
	{
		return DeviceAudioMode::Default;
	}

	void Device::setAudioMode(DeviceAudioMode mode)
	{
	}

	DeviceRingerMode Device::getRingerMode()
	{
		return DeviceRingerMode::Normal;
	}

	void Device::setRingerMode(DeviceRingerMode mode)
	{
	}

	sl_bool Device::isSpeakerphoneOn()
	{
		return sl_false;
	}

	void Device::setSpeakerphoneOn(sl_bool flag)
	{
	}

	sl_bool Device::isBluetoothScoOn()
	{
		return sl_false;
	}

	void Device::setBluetoothScoOn(sl_bool flag)
	{
	}
#endif

#if !defined(SLIB_PLATFORM_IS_ANDROID) && !defined(SLIB_PLATFORM_IS_WIN32) && !defined(SLIB_PLATFORM_IS_MACOS) && !defined(SLIB_PLATFORM_IS_LINUX_DESKTOP)
	float Device::getVolume(AudioStreamType stream)
	{
		return 0;
	}

	void Device::setVolume(AudioStreamType stream, float volume, const DeviceSetVolumeFlags& flags)
	{
	}

	sl_bool Device::isMute(AudioStreamType stream)
	{
		return sl_false;
	}

	void Device::setMute(AudioStreamType stream, sl_bool flagMute, const DeviceSetVolumeFlags& flags)
	{
	}

	sl_bool Device::isMicrophoneMute()
	{
		return sl_false;
	}

	void Device::setMicrophoneMute(sl_bool flag)
	{
	}
#endif

#if !defined(SLIB_PLATFORM_IS_WIN32) && !defined(SLIB_PLATFORM_IS_MACOS) && !defined(SLIB_PLATFORM_IS_LINUX_DESKTOP)
	float Device::getMicrophoneVolume()
	{
		return 0;
	}

	void Device::setMicrophoneVolume(float volume)
	{
	}
#endif

	void Device::setVolume(float volume, const DeviceSetVolumeFlags& flags)
	{
		setVolume(AudioStreamType::Default, volume, flags);
	}

	void Device::setMute(sl_bool flagMute, const DeviceSetVolumeFlags& flags)
	{
		setMute(AudioStreamType::Default, flagMute, flags);
	}

}
