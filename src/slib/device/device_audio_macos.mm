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

#include "slib/device/definition.h"

#if defined(SLIB_PLATFORM_IS_MACOS)

#include "slib/device/device.h"

#include <CoreAudio/CoreAudio.h>
#include <AudioToolbox/AudioToolbox.h>

namespace slib
{

	namespace
	{
		static AudioDeviceID GetDefaultDeviceID(AudioObjectPropertySelector selector)
		{
			AudioObjectPropertyAddress address;
			address.mSelector = selector;
			address.mScope = kAudioObjectPropertyScopeGlobal;
			address.mElement = kAudioObjectPropertyElementMain;
			if (!(AudioObjectHasProperty(kAudioObjectSystemObject, &address))) {
				return kAudioObjectUnknown;
			}
			AudioDeviceID defaultDeviceID = kAudioObjectUnknown;
			UInt32 size = sizeof(defaultDeviceID);
			OSStatus ret = AudioObjectGetPropertyData(kAudioObjectSystemObject, &address, 0, nil, &size, &defaultDeviceID);
			if (ret == noErr) {
				return defaultDeviceID;
			}
			return kAudioObjectUnknown;
		}

		static AudioDeviceID GetDefaultOutputDeviceID()
		{
			return GetDefaultDeviceID(kAudioHardwarePropertyDefaultOutputDevice);
		}

		static AudioDeviceID GetDefaultInputDeviceID()
		{
			return GetDefaultDeviceID(kAudioHardwarePropertyDefaultInputDevice);
		}
	
		static float GetDeviceVolume(AudioDeviceID deviceID, AudioObjectPropertyScope scope)
		{
			if (deviceID == kAudioObjectUnknown) {
				return 0;
			}
			AudioObjectPropertyAddress address;
			address.mSelector = kAudioHardwareServiceDeviceProperty_VirtualMainVolume;
			address.mScope = scope;
			address.mElement = kAudioObjectPropertyElementMain;
			if (!(AudioObjectHasProperty(deviceID, &address))) {
				return 0;
			}
			Float32 volume = 0;
			UInt32 size = sizeof(volume);
			OSStatus ret = AudioObjectGetPropertyData(deviceID, &address, 0, nil, &size, &volume);
			if (ret == noErr) {
				return (float)volume;
			}
			return 0;
		}

		static void SetDeviceVolume(AudioDeviceID deviceID, AudioObjectPropertyScope scope, float _volume)
		{
			if (deviceID == kAudioObjectUnknown) {
				return;
			}
			AudioObjectPropertyAddress address;
			address.mSelector = kAudioHardwareServiceDeviceProperty_VirtualMainVolume;
			address.mScope = scope;
			address.mElement = kAudioObjectPropertyElementMain;
			if (!(AudioObjectHasProperty(deviceID, &address))) {
				return;
			}
			Float32 volume = (Float32)(Math::clamp(_volume, 0.0f, 1.0f));
			UInt32 size = sizeof(volume);
			AudioObjectSetPropertyData(deviceID, &address, 0, nil, size, &volume);
		}

		static sl_bool IsDeviceMute(AudioDeviceID deviceID, AudioObjectPropertyScope scope)
		{
			if (deviceID == kAudioObjectUnknown) {
				return sl_false;
			}
			AudioObjectPropertyAddress address;
			address.mSelector = kAudioDevicePropertyMute;
			address.mScope = scope;
			address.mElement = kAudioObjectPropertyElementMain;
			if (!(AudioObjectHasProperty(deviceID, &address))) {
				return sl_false;
			}
			UInt32 flag = 0;
			UInt32 size = sizeof(flag);
			OSStatus ret = AudioObjectGetPropertyData(deviceID, &address, 0, nil, &size, &flag);
			if (ret == noErr) {
				return flag == 1;
			}
			return sl_false;
		}

		static void SetDeviceMute(AudioDeviceID deviceID, AudioObjectPropertyScope scope, sl_bool flagMute)
		{
			if (deviceID == kAudioObjectUnknown) {
				return;
			}
			AudioObjectPropertyAddress address;
			address.mSelector = kAudioDevicePropertyMute;
			address.mScope = kAudioDevicePropertyScopeOutput;
			address.mElement = kAudioObjectPropertyElementMain;
			if (!(AudioObjectHasProperty(deviceID, &address))) {
				return;
			}
			UInt32 flag = flagMute ? 1 : 0;
			UInt32 size = sizeof(flag);
			AudioObjectSetPropertyData(deviceID, &address, 0, nil, size, &flag);
		}
	}

	float Device::getVolume(AudioStreamType stream)
	{
		AudioDeviceID deviceID = GetDefaultOutputDeviceID();
		return GetDeviceVolume(deviceID, kAudioDevicePropertyScopeOutput);
	}

	void Device::setVolume(AudioStreamType stream, float volume, const DeviceSetVolumeFlags& flags)
	{
		AudioDeviceID deviceID = GetDefaultOutputDeviceID();
		SetDeviceVolume(deviceID, kAudioDevicePropertyScopeOutput, volume);
	}

	sl_bool Device::isMute(AudioStreamType stream)
	{
		AudioDeviceID deviceID = GetDefaultOutputDeviceID();
		return IsDeviceMute(deviceID, kAudioDevicePropertyScopeOutput);
	}

	void Device::setMute(AudioStreamType stream, sl_bool flagMute, const DeviceSetVolumeFlags& flags)
	{
		AudioDeviceID deviceID = GetDefaultOutputDeviceID();
		SetDeviceMute(deviceID, kAudioDevicePropertyScopeOutput, flagMute);
	}

	float Device::getMicrophoneVolume()
	{
		AudioDeviceID deviceID = GetDefaultInputDeviceID();
		return GetDeviceVolume(deviceID, kAudioDevicePropertyScopeInput);
	}

	void Device::setMicrophoneVolume(float volume)
	{
		AudioDeviceID deviceID = GetDefaultInputDeviceID();
		SetDeviceVolume(deviceID, kAudioDevicePropertyScopeInput, volume);
	}

	sl_bool Device::isMicrophoneMute()
	{
		return getMicrophoneVolume() < SLIB_EPSILON;
	}

	void Device::setMicrophoneMute(sl_bool flag)
	{
		static float savedVolume = 1.0f;
		if (flag) {
			float volume = getMicrophoneVolume();
			if (volume >= SLIB_EPSILON) {
				savedVolume = volume;
				setMicrophoneVolume(0);
			}
		} else {
			float volume = getMicrophoneVolume();
			if (volume < SLIB_EPSILON) {
				setMicrophoneVolume(savedVolume);
			}
		}
	}

}

#endif
