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
#include "slib/data/xml.h"
#include "slib/core/safe_static.h"
#include "slib/platform/apple/iokit.h"

#import <sys/sysctl.h>

namespace slib
{

	namespace
	{
		static String GetPlatformEntryValue(CFStringRef key)
		{
			return apple::IOKit::getServiceProperty("IOPlatformExpertDevice", key).getString();
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
		static Ref<XmlElement> GetDictValueElement(const Ref<XmlElement>& dict, const StringView& name)
		{
			ListElements< Ref<XmlElement> > children(dict->getChildElements());
			for (sl_size i = 0; i < children.count; i++) {
				Ref<XmlElement>& child = children[i];
				if (child.isNotNull()) {
					if (child->getName() == StringView::literal("key")) {
						if (child->getText() == name) {
							return children[i+1];
						}
					}
				}
			}
			return sl_null;
		}

		static String GetDictValueString(const Ref<XmlElement>& dict, const StringView& name)
		{
			Ref<XmlElement> xml = GetDictValueElement(dict, name);
			if (xml.isNotNull()) {
				return xml->getText();
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
		String strXml = System::getCommandOutput(StringView::literal("system_profiler -xml SPDisplaysDataType"));
		Ref<XmlDocument> xml = Xml::parse(strXml);
		if (xml.isNull()) {
			return sl_null;
		}
		Ref<XmlElement> xmlPlist = xml->getFirstChildElement("plist");
		if (xmlPlist.isNull()) {
			return sl_null;
		}
		Ref<XmlElement> xmlArray = xmlPlist->getFirstChildElement("array");
		if (xmlArray.isNull()) {
			return sl_null;
		}
		Ref<XmlElement> xmlDict = xmlArray->getFirstChildElement("dict");
		if (xmlDict.isNull()) {
			return sl_null;
		}
		Ref<XmlElement> xmlItems = GetDictValueElement(xmlDict, "_items");
		if (xmlItems.isNull()) {
			return sl_null;
		}
		List<VideoControllerInfo> ret;
		ListElements< Ref<XmlElement> > xmlVideos(xmlItems->getChildElements("dict"));
		for (sl_size i = 0; i < xmlVideos.count; i++) {
			VideoControllerInfo controller;
			Ref<XmlElement>& xmlVideo = xmlVideos[i];
			controller.name = GetDictValueString(xmlVideo, "sppci_model");
			controller.memorySize = ParseCapacity(GetDictValueString(xmlVideo, "spdisplays_vram"));
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
			String strXml = System::getCommandOutput(StringView::literal("system_profiler -xml SPMemoryDataType"));
			Ref<XmlDocument> xml = Xml::parse(strXml);
			if (xml.isNull()) {
				return sl_null;
			}
			Ref<XmlElement> xmlPlist = xml->getFirstChildElement("plist");
			if (xmlPlist.isNull()) {
				return sl_null;
			}
			Ref<XmlElement> xmlArray = xmlPlist->getFirstChildElement("array");
			if (xmlArray.isNull()) {
				return sl_null;
			}
			Ref<XmlElement> xmlDict = xmlArray->getFirstChildElement("dict");
			if (xmlDict.isNull()) {
				return sl_null;
			}
			Ref<XmlElement> xmlItems = GetDictValueElement(xmlDict, "_items");
			if (xmlItems.isNull()) {
				return sl_null;
			}
			xmlDict = xmlItems->getFirstChildElement("dict");
			if (xmlDict.isNull()) {
				return sl_null;
			}
			xmlItems = GetDictValueElement(xmlDict, "_items");
			if (xmlItems.isNull()) {
				return sl_null;
			}
			List<PhysicalMemorySlotInfo> ret;
			ListElements< Ref<XmlElement> > xmlSlots(xmlItems->getChildElements("dict"));
			for (sl_size i = 0; i < xmlSlots.count; i++) {
				PhysicalMemorySlotInfo slot;
				Ref<XmlElement>& xmlSlot = xmlSlots[i];
				slot.capacity = ParseCapacity(GetDictValueString(xmlSlot, "dimm_size"));
				String strSpeed = GetDictValueString(xmlSlot, "dimm_speed");
				if (strSpeed.endsWith_IgnoreCase("MHz")) {
					String::parseUint32(10, &(slot.speed), strSpeed.getData(), 0, strSpeed.getLength());
				}
				slot.bank = GetDictValueString(xmlSlot, "_name");
				slot.serialNumber = GetDictValueString(xmlSlot, "dimm_serial_number");
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
		String strXml = System::getCommandOutput(StringView::literal("system_profiler -xml SPEthernetDataType"));
		Ref<XmlDocument> xml = Xml::parse(strXml);
		if (xml.isNull()) {
			return sl_null;
		}
		Ref<XmlElement> xmlPlist = xml->getFirstChildElement("plist");
		if (xmlPlist.isNull()) {
			return sl_null;
		}
		Ref<XmlElement> xmlArray = xmlPlist->getFirstChildElement("array");
		if (xmlArray.isNull()) {
			return sl_null;
		}
		Ref<XmlElement> xmlDict = xmlArray->getFirstChildElement("dict");
		if (xmlDict.isNull()) {
			return sl_null;
		}
		Ref<XmlElement> xmlItems = GetDictValueElement(xmlDict, "_items");
		if (xmlItems.isNull()) {
			return sl_null;
		}
		List<NetworkAdapterInfo> ret;
		ListElements< Ref<XmlElement> > xmlDevices(xmlItems->getChildElements("dict"));
		for (sl_size i = 0; i < xmlDevices.count; i++) {
			NetworkAdapterInfo adapter;
			Ref<XmlElement>& xmlDevice = xmlDevices[i];
			adapter.deviceName = GetDictValueString(xmlDevice, "_name");
			adapter.interfaceName = GetDictValueString(xmlDevice, "spethernet_BSD_Device_Name");
			adapter.interfaceIndex = Network::getInterfaceIndexFromName(adapter.interfaceName);
			adapter.macAddress.parse(GetDictValueString(xmlDevice, "spethernet_mac_address"));
			adapter.flagPhysical = sl_true;
			ret.add_NoLock(Move(adapter));
		}
		return ret;
	}

	List<SoundDeviceInfo> Device::getSoundDevices()
	{
		String strXml = System::getCommandOutput(StringView::literal("system_profiler -xml SPAudioDataType"));
		Ref<XmlDocument> xml = Xml::parse(strXml);
		if (xml.isNull()) {
			return sl_null;
		}
		Ref<XmlElement> xmlPlist = xml->getFirstChildElement("plist");
		if (xmlPlist.isNull()) {
			return sl_null;
		}
		Ref<XmlElement> xmlArray = xmlPlist->getFirstChildElement("array");
		if (xmlArray.isNull()) {
			return sl_null;
		}
		Ref<XmlElement> xmlDict = xmlArray->getFirstChildElement("dict");
		if (xmlDict.isNull()) {
			return sl_null;
		}
		Ref<XmlElement> xmlItems = GetDictValueElement(xmlDict, "_items");
		if (xmlItems.isNull()) {
			return sl_null;
		}
		xmlDict = xmlItems->getFirstChildElement("dict");
		if (xmlDict.isNull()) {
			return sl_null;
		}
		xmlItems = GetDictValueElement(xmlDict, "_items");
		if (xmlItems.isNull()) {
			return sl_null;
		}
		List<SoundDeviceInfo> ret;
		ListElements< Ref<XmlElement> > xmlDevices(xmlItems->getChildElements("dict"));
		for (sl_size i = 0; i < xmlDevices.count; i++) {
			SoundDeviceInfo dev;
			Ref<XmlElement>& xmlDevice = xmlDevices[i];
			dev.name = GetDictValueString(xmlDevice, "_name");
			dev.manufacturer = GetDictValueString(xmlDevice, "coreaudio_device_manufacturer");
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
