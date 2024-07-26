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
#include "slib/device/cpu.h"
#include "slib/device/physical_memory.h"
#include "slib/device/printer.h"
#include "slib/network/os.h"

#include "slib/system/system.h"
#include "slib/data/json.h"
#include "slib/core/thread.h"
#include "slib/core/safe_static.h"
#include "slib/platform/apple/iokit.h"

#import <sys/sysctl.h>

namespace slib
{

	namespace
	{
		static String GetPlatformEntryValue(CFStringRef key)
		{
			return apple::IOKit::getServiceProperty("IOPlatformExpertDevice", key).getString().trim();
		}

		static String GetDeviceManufacturer()
		{
			return GetPlatformEntryValue(CFSTR("manufacturer"));
		}

		static String GetDeviceModel()
		{
			return GetPlatformEntryValue(CFSTR("model"));
		}

		static String GetBoardSerialNumber()
		{
			return GetPlatformEntryValue(CFSTR(kIOPlatformSerialNumberKey));
		}

		static String GetDeviceId()
		{
			return GetPlatformEntryValue(CFSTR(kIOPlatformUUIDKey));
		}
	}

	String Device::getManufacturer()
	{
		SLIB_SAFE_LOCAL_STATIC(String, ret, GetDeviceManufacturer())
		return ret;
	}

	String Device::getModel()
	{
		SLIB_SAFE_LOCAL_STATIC(String, ret, GetDeviceModel())
		return ret;
	}

	String Device::getBoardSerialNumber()
	{
		SLIB_SAFE_LOCAL_STATIC(String, ret, GetBoardSerialNumber());
		return ret;
	}

	String Device::getDeviceId()
	{
		SLIB_SAFE_LOCAL_STATIC(String, ret, GetDeviceId());
		return ret;
	}

	namespace
	{
		static String GetCommandOutput(const StringView& command, sl_uint32 nRetry = 3)
		{
			for (sl_uint32 i = 0; i < nRetry; i++) {
				String output = System::getCommandOutput(command);
				if (output.isNotEmpty()) {
					return output;
				}
				Thread::sleep(10);
			}
			return sl_null;
		}

		static sl_uint64 ParseCapacity(const String& text)
		{
			sl_uint64 capacity = 0;
			if (String::parseUint64(10, &capacity, text.getData(), 0, text.getLength()) > 0) {
				if (text.endsWith("GB")) {
					capacity *= SLIB_UINT64(1073741824);
				} else if (text.endsWith("TB")) {
					capacity *= SLIB_UINT64(1099511627776);
				} else if (text.endsWith("MB")) {
					capacity *= 1048576;
				} else if (text.endsWith("KB")) {
					capacity *= 1024;
				}
			}
			return capacity;
		}
	}

	List<VideoControllerInfo> Device::getVideoControllers()
	{
		String strJson = GetCommandOutput(StringView::literal("system_profiler -json SPDisplaysDataType"));
		Json json = Json::parse(strJson);
		List<VideoControllerInfo> ret;
		ListElements<Json> items(json["SPDisplaysDataType"].getJsonList());
		for (sl_size i = 0; i < items.count; i++) {
			VideoControllerInfo controller;
			Json& item = items[i];
			controller.name = item["sppci_model"].getString().trim();
			controller.memorySize = ParseCapacity(item["spdisplays_vram"].getString());
			ret.add_NoLock(Move(controller));
		}
		return ret;
	}

	namespace
	{
		static String GetCpuName()
		{
			char buf[128] = {0};
			size_t size = sizeof(buf) - 1;
			sysctlbyname("machdep.cpu.brand_string", buf, &size, sl_null, 0);
			return String::fromUtf8(buf);
		}
	}

	String Cpu::getName()
	{
		SLIB_SAFE_LOCAL_STATIC(String, ret, GetCpuName());
		return ret;
	}

	namespace
	{
		static List<PhysicalMemorySlotInfo> GetMemorySlots()
		{
			String strJson = GetCommandOutput(StringView::literal("system_profiler -json SPMemoryDataType"));
			Json json = Json::parse(strJson);
			List<PhysicalMemorySlotInfo> ret;
			ListElements<Json> items(json["SPMemoryDataType"][0]["_items"].getJsonList());
			for (sl_size i = 0; i < items.count; i++) {
				PhysicalMemorySlotInfo slot;
				Json& item = items[i];
				slot.capacity = ParseCapacity(item["dimm_size"].getString());
				String strSpeed = item["dimm_speed"].getString();
				if (strSpeed.endsWith_IgnoreCase("MHz")) {
					String::parseUint32(10, &(slot.speed), strSpeed.getData(), 0, strSpeed.getLength());
				}
				slot.bank = item["_name"].getString().trim();
				slot.serialNumber = item["dimm_serial_number"].getString().trim();
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

	List<NetworkAdapterInfo> Network::getAdapters()
	{
		String strJson = GetCommandOutput(StringView::literal("system_profiler -json SPEthernetDataType"));
		Json json = Json::parse(strJson);
		List<NetworkAdapterInfo> ret;
		ListElements<Json> items(json["SPEthernetDataType"].getJsonList());
		for (sl_size i = 0; i < items.count; i++) {
			NetworkAdapterInfo adapter;
			Json& item = items[i];
			adapter.deviceName = item["_name"].getString().trim();
			adapter.interfaceName = item["spethernet_BSD_Device_Name"].getString();
			adapter.interfaceIndex = Network::getInterfaceIndexFromName(adapter.interfaceName);
			adapter.macAddress.parse(item["spethernet_mac_address"].getString());
			adapter.flagPhysical = sl_true;
			ret.add_NoLock(Move(adapter));
		}
		return ret;
	}

	List<SoundDeviceInfo> Device::getSoundDevices()
	{
		String strJson = GetCommandOutput(StringView::literal("system_profiler -json SPAudioDataType"));
		Json json = Json::parse(strJson);
		List<SoundDeviceInfo> ret;
		ListElements<Json> items(json["SPAudioDataType"][0]["_items"].getJsonList());
		for (sl_size i = 0; i < items.count; i++) {
			SoundDeviceInfo dev;
			Json& item = items[i];
			dev.name = item["_name"].getString().trim();
			dev.manufacturer = item["coreaudio_device_manufacturer"].getString().trim();
			ret.add_NoLock(Move(dev));
		}
		return ret;
	}

	List<PrinterInfo> Printer::getDevices()
	{
		return sl_null;
	}

}
#endif
