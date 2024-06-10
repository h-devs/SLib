/*
 *   Copyright (c) 2008-2023 SLIBIO <https://github.com/SLIBIO>
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
#include "slib/core/safe_static.h"

#pragma warning(disable: 4091)
#include <mmdeviceapi.h>
#include <endpointvolume.h>

namespace slib
{

	namespace
	{
		static IAudioEndpointVolume* GetEndpointVolume(EDataFlow dataFlow)
		{
			CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
			IMMDeviceEnumerator* enumerator = NULL;
			HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_INPROC_SERVER, __uuidof(IMMDeviceEnumerator), (void**)&enumerator);
			if (!SUCCEEDED(hr)) {
				return NULL;
			}
			IAudioEndpointVolume* volume = NULL;
			IMMDevice* device = sl_null;
			hr = enumerator->GetDefaultAudioEndpoint(dataFlow, eConsole, &device);
			if (SUCCEEDED(hr)) {
				device->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, NULL, (void**)&volume);
				device->Release();
			}
			enumerator->Release();
			return volume;
		}
	}

	float Device::getVolume(AudioStreamType stream)
	{
		// Only works for Windows Vista and later
		IAudioEndpointVolume* volume = GetEndpointVolume(eRender);
		if (volume) {
			float level = 0;
			volume->GetMasterVolumeLevelScalar(&level);
			volume->Release();
			return level;
		}
		return 0;
	}

	void Device::setVolume(AudioStreamType stream, float level, const DeviceSetVolumeFlags& flags)
	{
		// Only works for Windows Vista and later
		IAudioEndpointVolume* volume = GetEndpointVolume(eRender);
		if (volume) {
			volume->SetMasterVolumeLevelScalar(level, NULL);
			volume->Release();
		}
	}

	sl_bool Device::isMute(AudioStreamType stream)
	{
		// Only works for Windows Vista and later
		IAudioEndpointVolume* volume = GetEndpointVolume(eRender);
		if (volume) {
			BOOL f = FALSE;
			volume->GetMute(&f);
			volume->Release();
			return f;
		}
		return sl_false;
	}

	void Device::setMute(AudioStreamType stream, sl_bool flag, const DeviceSetVolumeFlags& flags)
	{
		// Only works for Windows Vista and later
		IAudioEndpointVolume* volume = GetEndpointVolume(eRender);
		if (volume) {
			volume->SetMute(flag ? TRUE : FALSE, NULL);
			volume->Release();
		}
	}

	float Device::getMicrophoneVolume()
	{
		// Only works for Windows Vista and later
		IAudioEndpointVolume* volume = GetEndpointVolume(eCapture);
		if (volume) {
			float level = 0;
			volume->GetMasterVolumeLevelScalar(&level);
			volume->Release();
			return level;
		}
		return 0;
	}

	void Device::setMicrophoneVolume(float level)
	{
		// Only works for Windows Vista and later
		IAudioEndpointVolume* volume = GetEndpointVolume(eCapture);
		if (volume) {
			volume->SetMasterVolumeLevelScalar(level, NULL);
			volume->Release();
		}
	}

	sl_bool Device::isMicrophoneMute()
	{
		// Only works for Windows Vista and later
		IAudioEndpointVolume* volume = GetEndpointVolume(eCapture);
		if (volume) {
			BOOL f = FALSE;
			volume->GetMute(&f);
			volume->Release();
			return f;
		}
		return sl_false;
	}

	void Device::setMicrophoneMute(sl_bool flag)
	{
		// Only works for Windows Vista and later
		IAudioEndpointVolume* volume = GetEndpointVolume(eCapture);
		if (volume) {
			volume->SetMute(flag ? TRUE : FALSE, NULL);
			volume->Release();
		}
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
		ListElements<VariantMap> items(win32::Wmi::getQueryResponseRecords(L"SELECT * FROM Win32_SoundDevice", L"Name", L"PNPDeviceID"));
		for (sl_size i = 0; i < items.count; i++) {
			SoundDeviceInfo dev;
			VariantMap& item = items[i];
			dev.name = item.getValue("Name").getString();
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
