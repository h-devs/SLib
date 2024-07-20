/*
 *   Copyright (c) 2008-2022 SLIBIO <https://github.com/SLIBIO>
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

#if defined(SLIB_PLATFORM_IS_LINUX)

#include "slib/device/disk.h"

#include "slib/system/system.h"
#include "slib/io/file.h"
#include "slib/core/stringx.h"

namespace slib
{

	namespace
	{
		static String GetDeviceInfo(const StringView& name)
		{
			String cmd = StringView::literal("udevadm info --query=all --name=/dev/") + name;
			return System::getCommandOutput(cmd);
		}

		static String GetDiskInfoValue(const StringView& output, const StringView& field)
		{
			sl_reg index = output.indexOf(field);
			if (index > 0) {
				index += field.getLength();
				sl_reg index2 = Stringx::indexOfLine(output, index);
				if (index2 > 0) {
					return output.substring(index, index2);
				}
			}
			return sl_null;
		}

		static String GetSerialNumber(const StringView& output)
		{
			String s = GetDiskInfoValue(output, StringView::literal("ID_SCSI_SERIAL="));
			if (s.isNotEmpty()) {
				return s;
			}
			return GetDiskInfoValue(output, StringView::literal("ID_SERIAL_SHORT="));
		}

		static DiskInterface GetDiskInterface(const StringView& output)
		{
			String s = GetDiskInfoValue(output, StringView::literal("ID_BUS="));
			if (s.isNotEmpty()) {
				if (s == StringView::literal("ata")) {
					return DiskInterface::IDE;
				} else if (s == StringView::literal("scsi")) {
					return DiskInterface::SCSI;
				} else if (s == StringView::literal("usb")) {
					return DiskInterface::USB;
				}
			}
			return DiskInterface::Unknown;
		}

		static DiskType GetDiskType(const StringView& output)
		{
			String s = GetDiskInfoValue(output, StringView::literal("ID_DRIVE_THUMB="));
			if (s.isNotEmpty()) {
				if (s == StringView::literal("1")) {
					return DiskType::Removable;
				}
			}
			return DiskType::Fixed;
		}
	}

	String Disk::getSerialNumber(sl_uint32 diskNo)
	{
		char name[4] = {'s', 'd', 0, 0};
		name[2] = 'a' + diskNo;
		String info = GetDeviceInfo(name);
		return GetSerialNumber(info);
	}

	List<DiskInfo> Disk::getDevices()
	{
		List<DiskInfo> ret;
		ListElements<String> names(File::getFiles(StringView::literal("/dev")));
		for (sl_size i = 0; i < names.count; i++) {
			String& name = names[i];
			if (name.startsWith(StringView::literal("sd")) && name.getLength() == 3) {
				char c = (name.getData())[2];
				if (c >= 'a' && c <= 'z') {
					String infos = GetDeviceInfo(name);
					if (infos.isNotEmpty()) {
						DiskInfo info;
						info.index = c - 'a';
						info.path = StringView::literal("/dev/") + name;
						info.interface = GetDiskInterface(infos);
						info.type = GetDiskType(infos);
						info.model = GetDiskInfoValue(infos, StringView::literal("ID_MODEL="));
						info.serialNumber = GetSerialNumber(infos);
						info.capacity = File::getDiskSize(info.path);
						if (!(ret.add_NoLock(Move(info)))) {
							return sl_null;
						}
					}
				}
			}
		}
		return ret;
	}

}

#endif
