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

#define _WIN32_WINNT 0x0601

#include "slib/device/definition.h"

#if defined(SLIB_PLATFORM_IS_WIN32)

#include "slib/device/device.h"
#include "slib/device/cpu.h"
#include "slib/device/physical_memory.h"
#include "slib/device/disk.h"

#include "slib/platform/win32/windows.h"
#include "slib/platform/win32/wmi.h"
#include "slib/platform/win32/registry.h"
#include "slib/core/safe_static.h"

#pragma warning(disable: 4091)
#include <mmdeviceapi.h>
#include <endpointvolume.h>

namespace slib
{

	namespace
	{
		static IMMDeviceEnumerator* GetDeviceEnumerator()
		{
			CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
			IMMDeviceEnumerator* enumerator = NULL;
			HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_INPROC_SERVER, __uuidof(IMMDeviceEnumerator), (void**)&enumerator);
			SLIB_UNUSED(hr)
			return enumerator;
		}

		static IMMDeviceCollection* GetDevices(EDataFlow dataFlow)
		{
			IMMDeviceEnumerator* enumerator = GetDeviceEnumerator();
			if (!enumerator) {
				return sl_null;
			}
			IMMDeviceCollection* devices = sl_null;
			HRESULT hr = enumerator->EnumAudioEndpoints(dataFlow, DEVICE_STATE_ACTIVE, &devices);
			SLIB_UNUSED(hr)
			enumerator->Release();
			return devices;
		}

		static IAudioEndpointVolume* GetEndpointVolume(EDataFlow dataFlow)
		{
			IMMDeviceEnumerator* enumerator = GetDeviceEnumerator();
			if (!enumerator) {
				return sl_null;
			}
			IAudioEndpointVolume* volume = sl_null;
			IMMDevice* device = sl_null;
			HRESULT hr = enumerator->GetDefaultAudioEndpoint(dataFlow, eConsole, &device);
			SLIB_UNUSED(hr)
			if (device) {
				hr = device->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, sl_null, (void**)&volume);
				device->Release();
			}
			enumerator->Release();
			return volume;
		}

		static float GetVolumeDefault(EDataFlow dataFlow)
		{
			IAudioEndpointVolume* volume = GetEndpointVolume(dataFlow);
			if (volume) {
				float level = 0;
				HRESULT hr = volume->GetMasterVolumeLevelScalar(&level);
				SLIB_UNUSED(hr)
				volume->Release();
				return level;
			}
			return 0.0f;
		}

		static void SetVolumeDefault(EDataFlow dataFlow, float level)
		{
			IAudioEndpointVolume* volume = GetEndpointVolume(dataFlow);
			if (volume) {
				HRESULT hr = volume->SetMasterVolumeLevelScalar(level, sl_null);
				SLIB_UNUSED(hr)
				volume->Release();
			}
		}
		
		static void SetVolumeAll(EDataFlow dataFlow, float level)
		{
			IMMDeviceCollection* devices = GetDevices(dataFlow);
			if (!devices) {
				return;
			}
			sl_bool bRet = sl_true;
			UINT nDevices = 0;
			HRESULT hr = devices->GetCount(&nDevices);
			SLIB_UNUSED(hr)
			for (UINT iDevice = 0; iDevice < nDevices; iDevice++) {
				IMMDevice* device = sl_null;
				hr = devices->Item(iDevice, &device);
				if (device) {
					IAudioEndpointVolume* volume = sl_null;
					hr = device->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, sl_null, (void**)&volume);
					device->Release();
					if (volume) {
						HRESULT hr = volume->SetMasterVolumeLevelScalar(level, sl_null);
						volume->Release();
					}
				}
			}
			devices->Release();
		}

		static sl_bool IsMuteDefault(EDataFlow dataFlow)
		{
			IAudioEndpointVolume* volume = GetEndpointVolume(dataFlow);
			if (volume) {
				BOOL f = TRUE;
				HRESULT hr = volume->GetMute(&f);
				SLIB_UNUSED(hr)
				volume->Release();
				return f ? sl_true : sl_false;
			}
			return sl_true;
		}

		static void SetMuteDefault(EDataFlow dataFlow, sl_bool flagMute)
		{
			IAudioEndpointVolume* volume = GetEndpointVolume(dataFlow);
			if (volume) {
				HRESULT hr = volume->SetMute(flagMute ? TRUE : FALSE, sl_null);
				SLIB_UNUSED(hr)
				volume->Release();
			}
		}

		static sl_bool IsMuteAll(EDataFlow dataFlow)
		{
			IMMDeviceCollection* devices = GetDevices(dataFlow);
			if (!devices) {
				return sl_true;
			}
			sl_bool bRet = sl_true;
			UINT nDevices = 0;
			HRESULT hr = devices->GetCount(&nDevices);
			SLIB_UNUSED(hr)
			for (UINT iDevice = 0; iDevice < nDevices; iDevice++) {
				IMMDevice* device = sl_null;
				hr = devices->Item(iDevice, &device);
				if (device) {
					IAudioEndpointVolume* volume = sl_null;
					hr = device->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, sl_null, (void**)&volume);
					device->Release();
					if (volume) {
						BOOL f = TRUE;
						hr = volume->GetMute(&f);
						volume->Release();
						if (!f) {
							bRet = sl_false;
						}
					}
				}
			}
			devices->Release();
			return bRet;
		}

		static void SetMuteAll(EDataFlow dataFlow, sl_bool flagMute)
		{
			IMMDeviceCollection* devices = GetDevices(dataFlow);
			if (!devices) {
				return;
			}
			sl_bool bRet = sl_true;
			UINT nDevices = 0;
			HRESULT hr = devices->GetCount(&nDevices);
			SLIB_UNUSED(hr)
			for (UINT iDevice = 0; iDevice < nDevices; iDevice++) {
				IMMDevice* device = sl_null;
				hr = devices->Item(iDevice, &device);
				if (device) {
					IAudioEndpointVolume* volume = sl_null;
					hr = device->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, sl_null, (void**)&volume);
					device->Release();
					if (volume) {
						HRESULT hr = volume->SetMute(flagMute ? TRUE : FALSE, sl_null);
						volume->Release();
					}
				}
			}
			devices->Release();
		}
	}

	float Device::getVolume(AudioStreamType stream)
	{
		// Only works for Windows Vista and later
		return GetVolumeDefault(eRender);
	}

	void Device::setVolume(AudioStreamType stream, float level, const DeviceSetVolumeFlags& flags)
	{
		// Only works for Windows Vista and later
		if (flags & DeviceSetVolumeFlags::AllDevices) {
			SetVolumeAll(eRender, level);
		} else {
			SetVolumeDefault(eRender, level);
		}
	}

	sl_bool Device::isMute(AudioStreamType stream)
	{
		// Only works for Windows Vista and later
		return IsMuteDefault(eRender);
	}

	sl_bool Device::isMuteAll()
	{
		// Only works for Windows Vista and later
		return IsMuteAll(eRender);
	}

	void Device::setMute(AudioStreamType stream, sl_bool flagMute, const DeviceSetVolumeFlags& flags)
	{
		// Only works for Windows Vista and later
		if (flags & DeviceSetVolumeFlags::AllDevices) {
			SetMuteAll(eRender, flagMute);
		} else {
			SetMuteDefault(eRender, flagMute);
		}
	}

	float Device::getMicrophoneVolume()
	{
		// Only works for Windows Vista and later
		return GetVolumeDefault(eCapture);
	}

	void Device::setMicrophoneVolume(float level, const DeviceSetVolumeFlags& flags)
	{
		// Only works for Windows Vista and later
		if (flags & DeviceSetVolumeFlags::AllDevices) {
			SetVolumeAll(eCapture, level);
		} else {
			SetVolumeDefault(eCapture, level);
		}
	}

	sl_bool Device::isMicrophoneMute()
	{
		// Only works for Windows Vista and later
		return IsMuteDefault(eCapture);
	}

	sl_bool Device::isMicrophoneMuteAll()
	{
		// Only works for Windows Vista and later
		return IsMuteAll(eCapture);
	}

	void Device::setMicrophoneMute(sl_bool flagMute, const DeviceSetVolumeFlags& flags)
	{
		// Only works for Windows Vista and later
		if (flags & DeviceSetVolumeFlags::AllDevices) {
			SetMuteAll(eCapture, flagMute);
		} else {
			SetMuteDefault(eCapture, flagMute);
		}
	}

	namespace
	{
		static sl_bool IsUsingConsentSub(win32::Registry& key)
		{
			ListElements<String16> subkeys(key.getSubkeys());
			for (sl_size i = 0; i < subkeys.count; i++) {
				String16& subkey = subkeys[i];
				Variant value = win32::Registry::getValue(key.get(), subkey, u"LastUsedTimeStop");
				if (value.isIntegerType() && !(value.getUint32())) {
					return sl_true;
				}
			}
			return sl_false;
		}

		static sl_bool IsUsingConsent(const StringView16& name)
		{
			win32::Registry root = HKEY_USERS;
			ListElements<String16> users(root.getSubkeys());
			for (sl_size i = 0; i < users.count; i++) {
				win32::Registry key = win32::Registry::open(HKEY_USERS, users[i] + u"\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\" + name, KEY_READ);
				if (key.isNotNone()) {
					if (IsUsingConsentSub(key)) {
						return sl_true;
					}
					key = win32::Registry::open(key.get(), "NonPackaged", KEY_READ);
					if (key.isNotNone()) {
						if (IsUsingConsentSub(key)) {
							return sl_true;
						}
					}
				}
			}
			return sl_false;
		}

		static void GetAppsUsingConsentSub(win32::Registry& key, List<String>& ret)
		{
			ListElements<String16> subkeys(key.getSubkeys());
			for (sl_size i = 0; i < subkeys.count; i++) {
				String16& subkey = subkeys[i];
				Variant value = win32::Registry::getValue(key.get(), subkey, u"LastUsedTimeStop");
				if (value.isIntegerType() && !(value.getUint32())) {
					ret.add_NoLock(String::from(subkey));
				}
			}
		}

		static List<String> GetAppsUsingConsent(const StringView16& name)
		{
			List<String> ret;
			win32::Registry root = HKEY_USERS;
			ListElements<String16> users(root.getSubkeys());
			for (sl_size i = 0; i < users.count; i++) {
				win32::Registry key = win32::Registry::open(HKEY_USERS, users[i] + u"\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\" + name, KEY_READ);
				if (key.isNotNone()) {
					GetAppsUsingConsentSub(key, ret);
					key = win32::Registry::open(key.get(), "NonPackaged", KEY_READ);
					if (key.isNotNone()) {
						GetAppsUsingConsentSub(key, ret);
					}
				}
			}
			return ret;
		}
	}

	sl_bool Device::isUsingCamera()
	{
		return IsUsingConsent(StringView16::literal(u"webcam"));
	}

	sl_bool Device::isUsingMicrophone()
	{
		return IsUsingConsent(StringView16::literal(u"microphone"));
	}

	List<String> Device::getApplicationsUsingCamera()
	{
		return GetAppsUsingConsent(StringView16::literal(u"webcam"));
	}

	List<String> Device::getApplicationsUsingMicrophone()
	{
		return GetAppsUsingConsent(StringView16::literal(u"microphone"));
	}

	double Device::getScreenPPI()
	{
		return 96;
	}

	SizeI Device::getScreenSize()
	{
		SizeI ret;
		ret.x = (int)(GetSystemMetrics(SM_CXSCREEN));
		ret.y = (int)(GetSystemMetrics(SM_CYSCREEN));
		return ret;
	}

	String Device::getManufacturer()
	{
		SLIB_SAFE_LOCAL_STATIC(String, ret, win32::Wmi::getQueryResponseValue(L"SELECT * FROM Win32_ComputerSystem", L"Manufacturer").getString())
		return ret;
	}

	String Device::getModel()
	{
		SLIB_SAFE_LOCAL_STATIC(String, ret, win32::Wmi::getQueryResponseValue(L"SELECT * FROM Win32_ComputerSystem", L"Model").getString())
		return ret;
	}

	String Device::getBoardSerialNumber()
	{
		SLIB_SAFE_LOCAL_STATIC(String, ret, win32::Wmi::getQueryResponseValue(L"SELECT * FROM Win32_BIOS", L"SerialNumber").getString().trim())
		return ret;
	}

	List<VideoControllerInfo> Device::getVideoControllers()
	{
		List<VideoControllerInfo> ret;
		ListElements<VariantMap> items(win32::Wmi::getQueryResponseRecords(L"SELECT * FROM Win32_VideoController", L"Name", L"AdapterRAM"));
		for (sl_size i = 0; i < items.count; i++) {
			VideoControllerInfo controller;
			VariantMap& item = items[i];
			controller.name = item.getValue("Name").getString();
			controller.memorySize = item.getValue("AdapterRAM").getUint32();
			ret.add_NoLock(Move(controller));
		}
		return ret;
	}

	List<SoundDeviceInfo> Device::getSoundDevices()
	{
		List<SoundDeviceInfo> ret;
		ListElements<VariantMap> items(win32::Wmi::getQueryResponseRecords(L"SELECT * FROM Win32_SoundDevice", L"Name", L"Manufacturer", L"PNPDeviceID"));
		for (sl_size i = 0; i < items.count; i++) {
			SoundDeviceInfo dev;
			VariantMap& item = items[i];
			dev.name = item.getValue("Name").getString();
			dev.manufacturer = item.getValue("Manufacturer").getString();
			dev.pnpDeviceId = item.getValue("PNPDeviceID").getString();
			ret.add_NoLock(Move(dev));
		}
		return ret;
	}

	String Cpu::getName()
	{
		SLIB_SAFE_LOCAL_STATIC(String, ret, String::from(win32::Wmi::getQueryResponseValue(L"SELECT * FROM Win32_Processor", L"Name")))
		return ret;
	}

	namespace
	{
		static List<PhysicalMemorySlotInfo> GetMemorySlots()
		{
			List<PhysicalMemorySlotInfo> ret;
			ListElements<VariantMap> items(win32::Wmi::getQueryResponseRecords(L"SELECT * FROM Win32_PhysicalMemory", L"Capacity", L"Speed", L"BankLabel", L"SerialNumber"));
			for (sl_size i = 0; i < items.count; i++) {
				PhysicalMemorySlotInfo slot;
				VariantMap& item = items[i];
				slot.capacity = item.getValue("Capacity").getUint64();
				slot.speed = item.getValue("Speed").getUint32();
				slot.bank = item.getValue("BankLabel").getString();
				slot.serialNumber = item.getValue("SerialNumber").getString();
				ret.add_NoLock(Move(slot));
			}
			return ret;
		}
	}

	List<PhysicalMemorySlotInfo> PhysicalMemory::getSlots()
	{
		SLIB_SAFE_LOCAL_STATIC(List<PhysicalMemorySlotInfo>, ret, GetMemorySlots())
		return ret;
	}

}

#endif
