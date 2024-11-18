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

#include "slib/core/scoped_buffer.h"

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
#ifdef __MAC_13_0
			address.mElement = kAudioObjectPropertyElementMain;
#else
			address.mElement = kAudioObjectPropertyElementMaster;
#endif
			if (!(AudioObjectHasProperty(kAudioObjectSystemObject, &address))) {
				return kAudioObjectUnknown;
			}
			AudioDeviceID defaultDeviceID = kAudioObjectUnknown;
			UInt32 size = sizeof(defaultDeviceID);
			OSStatus ret = AudioObjectGetPropertyData(kAudioObjectSystemObject, &address, 0, sl_null, &size, &defaultDeviceID);
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

		static Array<AudioDeviceID> GetAllDeviceIDs()
		{
			AudioObjectPropertyAddress address;
			address.mSelector = kAudioHardwarePropertyDevices;
			address.mScope = kAudioObjectPropertyScopeGlobal;
			address.mElement = kAudioObjectPropertyElementWildcard;
			if (!(AudioObjectHasProperty(kAudioObjectSystemObject, &address))) {
				return sl_null;
			}
			UInt32 size = 0;
			AudioObjectGetPropertyDataSize(kAudioObjectSystemObject, &address, 0, sl_null, &size);
			if (!size) {
				return sl_null;
			}
			sl_uint32 n = (sl_uint32)(size / sizeof(AudioDeviceID));
			if (!n) {
				return sl_null;
			}
			Array<AudioDeviceID> ret = Array<AudioDeviceID>::create(n);
			if (ret.isNull()) {
				return sl_null;
			}
			if (AudioObjectGetPropertyData(kAudioObjectSystemObject, &address, 0, sl_null, &size, ret.getData()) == noErr) {
				return ret;
			}
			return sl_null;
		}

		static sl_bool IsDeviceSupportingScope(AudioDeviceID deviceID, AudioObjectPropertyScope scope)
		{
			AudioObjectPropertyAddress address;
			address.mSelector = kAudioDevicePropertyStreamConfiguration;
			address.mScope = scope;
			address.mElement = kAudioObjectPropertyElementWildcard;
			if (!(AudioObjectHasProperty(deviceID, &address))) {
				return sl_false;
			}
			UInt32 size = 0;
			AudioObjectGetPropertyDataSize(deviceID, &address, 0, sl_null, &size);
			if (!size) {
				return sl_false;
			}
			SLIB_SCOPED_BUFFER(sl_uint8, 1024, buf, size)
			if (!buf) {
				return sl_false;
			}
			AudioBufferList* bufferList = (AudioBufferList*)buf;
			if (AudioObjectGetPropertyData(deviceID, &address, 0, sl_null, &size, bufferList) == noErr) {
				return bufferList->mNumberBuffers > 0;
			} else {
				return sl_false;
			}
		}

		static float GetDeviceVolume(AudioDeviceID deviceID, AudioObjectPropertyScope scope)
		{
			if (deviceID == kAudioObjectUnknown) {
				return 0;
			}
			AudioObjectPropertyAddress address;
			address.mScope = scope;
#ifdef __MAC_13_0
			address.mSelector = kAudioHardwareServiceDeviceProperty_VirtualMainVolume;
			address.mElement = kAudioObjectPropertyElementMain;
#else
			address.mSelector = kAudioHardwareServiceDeviceProperty_VirtualMasterVolume;
			address.mElement = kAudioObjectPropertyElementMaster;
#endif
			if (!(AudioObjectHasProperty(deviceID, &address))) {
				return 0;
			}
			Float32 volume = 0;
			UInt32 size = sizeof(volume);
			OSStatus ret = AudioObjectGetPropertyData(deviceID, &address, 0, sl_null, &size, &volume);
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
			address.mScope = scope;
#ifdef __MAC_13_0
			address.mSelector = kAudioHardwareServiceDeviceProperty_VirtualMainVolume;
			address.mElement = kAudioObjectPropertyElementMain;
#else
			address.mSelector = kAudioHardwareServiceDeviceProperty_VirtualMasterVolume;
			address.mElement = kAudioObjectPropertyElementMaster;
#endif
			if (!(AudioObjectHasProperty(deviceID, &address))) {
				return;
			}
			Float32 volume = (Float32)(Math::clamp(_volume, 0.0f, 1.0f));
			UInt32 size = sizeof(volume);
			AudioObjectSetPropertyData(deviceID, &address, 0, sl_null, size, &volume);
		}

		static sl_bool IsDeviceMute(AudioDeviceID deviceID, AudioObjectPropertyScope scope)
		{
			if (deviceID == kAudioObjectUnknown) {
				return sl_false;
			}
			AudioObjectPropertyAddress address;
			address.mSelector = kAudioDevicePropertyMute;
			address.mScope = scope;
#ifdef __MAC_13_0
			address.mElement = kAudioObjectPropertyElementMain;
#else
			address.mElement = kAudioObjectPropertyElementMaster;
#endif
			if (!(AudioObjectHasProperty(deviceID, &address))) {
				return sl_false;
			}
			UInt32 flag = 0;
			UInt32 size = sizeof(flag);
			OSStatus ret = AudioObjectGetPropertyData(deviceID, &address, 0, sl_null, &size, &flag);
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
			address.mScope = scope;
#ifdef __MAC_13_0
			address.mElement = kAudioObjectPropertyElementMain;
#else
			address.mElement = kAudioObjectPropertyElementMaster;
#endif
			if (!(AudioObjectHasProperty(deviceID, &address))) {
				return;
			}
			UInt32 flag = flagMute ? 1 : 0;
			UInt32 size = sizeof(flag);
			AudioObjectSetPropertyData(deviceID, &address, 0, sl_null, size, &flag);
		}
	}

	float Device::getVolume(AudioStreamType stream)
	{
		AudioDeviceID deviceID = GetDefaultOutputDeviceID();
		return GetDeviceVolume(deviceID, kAudioDevicePropertyScopeOutput);
	}

	void Device::setVolume(AudioStreamType stream, float volume, const DeviceSetVolumeFlags& flags)
	{
		if (flags & DeviceSetVolumeFlags::AllDevices) {
			Array<AudioDeviceID> devices = GetAllDeviceIDs();
			AudioDeviceID* pDevices = devices.getData();
			sl_size nDevices = devices.getCount();
			for (sl_size i = 0; i < nDevices; i++) {
				AudioDeviceID& deviceID = pDevices[i];
				if (IsDeviceSupportingScope(deviceID, kAudioDevicePropertyScopeOutput)) {
					SetDeviceVolume(deviceID, kAudioDevicePropertyScopeOutput, volume);
				}
			}
		} else {
			AudioDeviceID deviceID = GetDefaultOutputDeviceID();
			SetDeviceVolume(deviceID, kAudioDevicePropertyScopeOutput, volume);
		}
	}

	sl_bool Device::isMute(AudioStreamType stream)
	{
		AudioDeviceID deviceID = GetDefaultOutputDeviceID();
		return IsDeviceMute(deviceID, kAudioDevicePropertyScopeOutput);
	}

	sl_bool Device::isMuteAll()
	{
		Array<AudioDeviceID> devices = GetAllDeviceIDs();
		AudioDeviceID* pDevices = devices.getData();
		sl_size nDevices = devices.getCount();
		for (sl_size i = 0; i < nDevices; i++) {
			AudioDeviceID& deviceID = pDevices[i];
			if (IsDeviceSupportingScope(deviceID, kAudioDevicePropertyScopeOutput)) {
				if (!(IsDeviceMute(deviceID, kAudioDevicePropertyScopeOutput))) {
					return sl_false;
				}
			}
		}
		return sl_true;
	}

	void Device::setMute(AudioStreamType stream, sl_bool flagMute, const DeviceSetVolumeFlags& flags)
	{
		if (flags & DeviceSetVolumeFlags::AllDevices) {
			Array<AudioDeviceID> devices = GetAllDeviceIDs();
			AudioDeviceID* pDevices = devices.getData();
			sl_size nDevices = devices.getCount();
			for (sl_size i = 0; i < nDevices; i++) {
				AudioDeviceID& deviceID = pDevices[i];
				if (IsDeviceSupportingScope(deviceID, kAudioDevicePropertyScopeOutput)) {
					SetDeviceMute(deviceID, kAudioDevicePropertyScopeOutput, flagMute);
				}
			}
		} else {
			AudioDeviceID deviceID = GetDefaultOutputDeviceID();
			SetDeviceMute(deviceID, kAudioDevicePropertyScopeOutput, flagMute);
		}
	}

	float Device::getMicrophoneVolume()
	{
		AudioDeviceID deviceID = GetDefaultInputDeviceID();
		return GetDeviceVolume(deviceID, kAudioDevicePropertyScopeInput);
	}

	void Device::setMicrophoneVolume(float volume, const DeviceSetVolumeFlags& flags)
	{
		if (flags & DeviceSetVolumeFlags::AllDevices) {
			Array<AudioDeviceID> devices = GetAllDeviceIDs();
			AudioDeviceID* pDevices = devices.getData();
			sl_size nDevices = devices.getCount();
			for (sl_size i = 0; i < nDevices; i++) {
				AudioDeviceID& deviceID = pDevices[i];
				if (IsDeviceSupportingScope(deviceID, kAudioDevicePropertyScopeInput)) {
					SetDeviceVolume(deviceID, kAudioDevicePropertyScopeInput, volume);
				}
			}
		} else {
			AudioDeviceID deviceID = GetDefaultInputDeviceID();
			SetDeviceVolume(deviceID, kAudioDevicePropertyScopeInput, volume);
		}
	}

	sl_bool Device::isMicrophoneMute()
	{
		AudioDeviceID deviceID = GetDefaultInputDeviceID();
		return IsDeviceMute(deviceID, kAudioDevicePropertyScopeInput);
	}

	sl_bool Device::isMicrophoneMuteAll()
	{
		Array<AudioDeviceID> devices = GetAllDeviceIDs();
		AudioDeviceID* pDevices = devices.getData();
		sl_size nDevices = devices.getCount();
		for (sl_size i = 0; i < nDevices; i++) {
			AudioDeviceID& deviceID = pDevices[i];
			if (IsDeviceSupportingScope(deviceID, kAudioDevicePropertyScopeInput)) {
				if (!(IsDeviceMute(deviceID, kAudioDevicePropertyScopeInput))) {
					return sl_false;
				}
			}
		}
		return sl_true;
	}

	void Device::setMicrophoneMute(sl_bool flagMute, const DeviceSetVolumeFlags& flags)
	{
		if (flags & DeviceSetVolumeFlags::AllDevices) {
			Array<AudioDeviceID> devices = GetAllDeviceIDs();
			AudioDeviceID* pDevices = devices.getData();
			sl_size nDevices = devices.getCount();
			for (sl_size i = 0; i < nDevices; i++) {
				AudioDeviceID& deviceID = pDevices[i];
				if (IsDeviceSupportingScope(deviceID, kAudioDevicePropertyScopeInput)) {
					SetDeviceMute(deviceID, kAudioDevicePropertyScopeInput, flagMute);
				}
			}
		} else {
			AudioDeviceID deviceID = GetDefaultInputDeviceID();
			SetDeviceMute(deviceID, kAudioDevicePropertyScopeInput, flagMute);
		}
	}

	sl_bool Device::isUsingMicrophone()
	{
		AudioObjectPropertyAddress address;
		address.mSelector = kAudioHardwarePropertyDevices;
		address.mScope = kAudioObjectPropertyScopeGlobal;
#ifdef __MAC_13_0
		address.mElement = kAudioObjectPropertyElementMain;
#else
		address.mElement = kAudioObjectPropertyElementMaster;
#endif
		UInt32 nDataSize = 0;
		AudioObjectGetPropertyDataSize(kAudioObjectSystemObject, &address, 0, sl_null, &nDataSize);
		sl_uint32 nDevices = nDataSize / sizeof(AudioDeviceID);
		SLIB_SCOPED_BUFFER(AudioDeviceID, 64, deviceIds, nDevices)
		if (!deviceIds) {
			return sl_false;
		}
		sl_bool bRet = sl_false;
		if (AudioObjectGetPropertyData(kAudioObjectSystemObject, &address, 0, sl_null, &nDataSize, deviceIds) == noErr) {
			for (sl_uint32 i = 0; i < nDevices; i++) {
				AudioDeviceID deviceId = deviceIds[i];
				address.mSelector = kAudioDevicePropertyStreams;
				address.mScope = kAudioDevicePropertyScopeInput;
				nDataSize = 0;
				AudioObjectGetPropertyDataSize(deviceId, &address, 0, sl_null, &nDataSize);
				if (nDataSize) {
					UInt32 flagRunning = 0;
					address.mSelector = kAudioDevicePropertyDeviceIsRunningSomewhere;
					nDataSize = sizeof(flagRunning);
					if (AudioObjectGetPropertyData(deviceId, &address, 0, sl_null, &nDataSize, &flagRunning) == noErr) {
						if (flagRunning) {
							bRet = sl_true;
						}
					}
				}
			}
		}
		return bRet;
	}

}

#endif
