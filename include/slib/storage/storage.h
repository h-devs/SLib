/*
 *   Copyright (c) 2008-2021 SLIBIO <https://github.com/SLIBIO>
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

#ifndef CHECKHEADER_SLIB_STORAGE_STORAGE
#define CHECKHEADER_SLIB_STORAGE_STORAGE

#include "definition.h"

#include "../core/string.h"
#include "../core/function.h"

namespace slib
{

	// equivalent to Win32's `STORAGE_BUS_TYPE`
	enum class StorageBusType
	{
		Unknown = 0,
		Scsi,
		Atapi,
		Ata,
		IEEE1394,
		Ssa,
		Fibre,
		Usb,
		RAID,
		iScsi,
		Sas,
		Sata,
		Sd,
		Mmc,
		Virtual,
		FileBackedVirtual,
		Max
	};

	class SLIB_EXPORT StorageVolumeDescription
	{
	public:
		sl_bool flagRemovable;
		StorageBusType busType;

	public:
		StorageVolumeDescription();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(StorageVolumeDescription)

	};

	typedef Function<void(const String& path)> VolumeArrivalCallback;
	typedef Function<void(const String& path)> VolumeRemovalCallback;


	class SLIB_EXPORT Storage
	{
	public:
		// returns the list of volume device path
		static List<String> getAllVolumes();

		static String getVolumePath(const StringParam& devicePath);

		static sl_bool getVolumeDescription(const StringParam& path, StorageVolumeDescription& _out);

		static sl_bool isRemovableVolume(const StringParam& path);

		static sl_bool isUsbVolume(const StringParam& path);

		static sl_bool isCdromVolume(const StringParam& path);

		static sl_bool removeDevice(const StringParam& volumPath);


		static sl_bool getVolumnSize(const StringParam& path, sl_uint64* pTotalSize = sl_null, sl_uint64* pFreeSize = sl_null);

		static sl_uint64 getVolumnTotalSize(const StringParam& path);

		static sl_uint64 getVolumnFreeSize(const StringParam& path);


		static sl_bool disableUsbMassStorage();

		static sl_bool enableUsbMassStorage();

		static sl_bool isUsbMassStorageEnabled();


		static void addOnVolumeArrival(const VolumeArrivalCallback& callback);

		static void removeOnVolumeArrival(const VolumeArrivalCallback& callback);

		static void setOnVolumeArrival(const VolumeArrivalCallback& callback);

		static void addOnVolumeRemoval(const VolumeRemovalCallback& callback);

		static void removeOnVolumeRemoval(const VolumeRemovalCallback& callback);

		static void setOnVolumeRemoval(const VolumeRemovalCallback& callback);

	};

}

#endif
