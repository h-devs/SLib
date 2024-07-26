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

#include "slib/device/disk.h"

#include "slib/platform/apple/iokit.h"

namespace slib
{

	namespace
	{

		static void FindSataDevices(const Function<sl_bool(DiskInfo&)>& callback)
		{
			apple::IOKit::findServices("AppleAHCIDiskDriver", [callback](io_service_t disk) {
				io_registry_entry_t media = apple::IOKit::findRegistryEntry(disk, kIOServicePlane, "IOMedia");
				if (!media) {
					return sl_true;
				}
				DiskInfo info;
				info.index = apple::IOKit::getRegistryEntryProperty(media, CFSTR("BSD Unit")).getUint32();
				info.path = apple::IOKit::getRegistryEntryProperty(media, CFSTR("BSD Name")).getString();
				if (info.path.isNotEmpty()) {
					info.path = "/dev/" + info.path;
				}
				info.interface = DiskInterface::IDE;
				info.type = DiskType::Fixed;
				info.model = apple::IOKit::getRegistryEntryProperty(disk, CFSTR("Model")).getString().trim();
				info.serialNumber = Disk::normalizeSerialNumber(apple::IOKit::getRegistryEntryProperty(disk, CFSTR("Serial Number")).getString());
				info.capacity = apple::IOKit::getRegistryEntryProperty(media, CFSTR("Size")).getUint64();
				IOObjectRelease(media);
				return callback(info);
			});
		}

		static void FindNvmeDevices(const Function<sl_bool(DiskInfo&)>& callback)
		{
			apple::IOKit::findServices("IONVMeController", [callback](io_service_t disk) {
				io_registry_entry_t media = apple::IOKit::findRegistryEntry(disk, kIOServicePlane, "IOMedia");
				if (!media) {
					return sl_true;
				}
				DiskInfo info;
				info.index = apple::IOKit::getRegistryEntryProperty(media, CFSTR("BSD Unit")).getUint32();
				info.path = apple::IOKit::getRegistryEntryProperty(media, CFSTR("BSD Name")).getString();
				if (info.path.isNotEmpty()) {
					info.path = "/dev/" + info.path;
				}
				info.interface = DiskInterface::SCSI;
				info.type = DiskType::Fixed;
				info.model = apple::IOKit::getRegistryEntryProperty(disk, CFSTR("Model Number")).getString().trim();
				info.serialNumber = Disk::normalizeSerialNumber(apple::IOKit::getRegistryEntryProperty(disk, CFSTR("Serial Number")).getString());
				info.capacity = apple::IOKit::getRegistryEntryProperty(media, CFSTR("Size")).getUint64();
				IOObjectRelease(media);
				return callback(info);
			});
		}

		static void FindUsbDevices(const Function<sl_bool(DiskInfo&)>& callback)
		{
			apple::IOKit::findServices("IOUSBMassStorageInterfaceNub", [callback](io_service_t disk) {
				io_registry_entry_t media = apple::IOKit::findRegistryEntry(disk, kIOServicePlane, "IOMedia");
				if (!media) {
					return sl_true;
				}
				DiskInfo info;
				info.index = apple::IOKit::getRegistryEntryProperty(media, CFSTR("BSD Unit")).getUint32();
				info.path = apple::IOKit::getRegistryEntryProperty(media, CFSTR("BSD Name")).getString();
				if (info.path.isNotEmpty()) {
					info.path = "/dev/" + info.path;
				}
				info.interface = DiskInterface::USB;
				info.type = apple::IOKit::getRegistryEntryProperty(media, CFSTR("Removable")).getBoolean() ? DiskType::Removable : DiskType::External;
				info.model = apple::IOKit::getRegistryEntryProperty(disk, CFSTR("USB Product Name")).getString().trim();
				info.serialNumber = Disk::normalizeSerialNumber(apple::IOKit::getRegistryEntryProperty(disk, CFSTR("kUSBSerialNumberString")).getString());
				info.capacity = apple::IOKit::getRegistryEntryProperty(media, CFSTR("Size")).getUint64();
				sl_bool flagContinue = callback(info);
				IOObjectRelease(media);
				return flagContinue;
			});
		}

	}

	String Disk::getSerialNumber(sl_uint32 diskNo)
	{
		String ret;
		FindSataDevices([diskNo, &ret](DiskInfo& info) {
			if (info.index == diskNo) {
				ret = info.serialNumber.getNotNull();
				return sl_false;
			}
			return sl_true;
		});
		if (ret.isNotNull()) {
			return ret;
		}
		FindNvmeDevices([diskNo, &ret](DiskInfo& info) {
			if (info.index == diskNo) {
				ret = info.serialNumber.getNotNull();
				return sl_false;
			}
			return sl_true;
		});
		if (ret.isNotNull()) {
			return ret;
		}
		FindUsbDevices([diskNo, &ret](DiskInfo& info) {
			if (info.index == diskNo) {
				ret = info.serialNumber.getNotNull();
				return sl_false;
			}
			return sl_true;
		});
		return ret;
	}

	List<DiskInfo> Disk::getDevices()
	{
		List<DiskInfo> ret;
		FindSataDevices([&ret](DiskInfo& info) {
			ret.add_NoLock(Move(info));
			return sl_true;
		});
		FindNvmeDevices([&ret](DiskInfo& info) {
			ret.add_NoLock(Move(info));
			return sl_true;
		});
		FindUsbDevices([&ret](DiskInfo& info) {
			ret.add_NoLock(Move(info));
			return sl_true;
		});
		return ret;
	}

}

#endif
