/*
 *   Copyright (c) 2008-2020 SLIBIO <https://github.com/SLIBIO>
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

#ifndef CHECKHEADER_SLIB_STORAGE_NTFS
#define CHECKHEADER_SLIB_STORAGE_NTFS

#include "definition.h"

#include "../core/mio.h"

namespace slib
{


#pragma pack(push, 1)
	struct NtfsBootSector_LittleEndian
	{
		sl_uint8 jmp[3]; // EB 52 90
		sl_uint8 oemID[8]; // "NTFS    "
		sl_uint16 bytesPerSector; // 0x200
		sl_uint8 sectorsPerCluster; // 0x08
		sl_uint8 reserved1[2];
		sl_uint8 reserved2[3];
		sl_uint8 reserved3[2];
		sl_uint8 mediaDescriptor; // 0xF8: Hard Disk
		sl_uint8 reserved4[2];
		sl_uint16 sectorsPerTrack;
		sl_uint16 numberOfHeads;
		sl_uint32 hiddenSectors;
		sl_uint8 reserved5[4];
		sl_uint8 reserved6[4];
		sl_uint64 totalSectors;
		sl_uint64 mftClusterNumber;
		sl_uint64 mftMirrorClusterNumber;
		sl_uint8 clustersPerFileRecordSegment;
		sl_uint8 reserved7[3];
		sl_uint8 clustersPerIndexBuffer; // 0x01
		sl_uint8 reserved8[3];
		sl_uint8 serialNumber[8];
		sl_uint8 reserved9[4];
		sl_uint8 bootstrapCode[426];
		sl_uint16 endMarker; // 0xAA55
	};
#pragma pack(pop)

#define SLIB_NTFS_JMP_CODE1 0xEB
#define SLIB_NTFS_JMP_CODE2 0x52
#define SLIB_NTFS_JMP_CODE3 0x90
#define SLIB_NTFS_OEMID "NTFS    "
#define SLIB_NTFS_END_OF_SECTOR_MARKER 0xAA55

	class SLIB_EXPORT NtfsBootSector
	{
	public:
		sl_uint8 jmp[3]; // EB 52 90
		sl_uint8 oemID[8]; // "NTFS    "
		sl_uint8 bytesPerSector[2]; // 512
		sl_uint8 sectorsPerCluster; // 0x08
		sl_uint8 reserved1[2];
		sl_uint8 reserved2[3];
		sl_uint8 reserved3[2];
		sl_uint8 mediaDescriptor; // 0xF8: Hard Disk
		sl_uint8 reserved4[2];
		sl_uint8 sectorsPerTrack[2];
		sl_uint8 numberOfHeads[2];
		sl_uint8 hiddenSectors[4];
		sl_uint8 reserved5[4];
		sl_uint8 reserved6[4];
		sl_uint8 totalSectors[8];
		sl_uint8 mftClusterNumber[8];
		sl_uint8 mftMirrorClusterNumber[8];
		sl_uint8 clustersPerFileRecordSegment;
		sl_uint8 reserved7[3];
		sl_uint8 clustersPerIndexBuffer; // 0x01
		sl_uint8 reserved8[3];
		sl_uint8 serialNumber[8];
		sl_uint8 reserved9[4];
		sl_uint8 bootstrapCode[426];
		sl_uint8 endMarker[2]; // 55 AA

	public:
		// 3 bytes
		sl_uint8* getJmpCode() const
		{
			return (sl_uint8*)jmp;
		}


		// 8 bytes
		sl_uint8* getOemId() const
		{
			return (sl_uint8*)oemID;
		}


		sl_uint16 getBytesPerSector() const
		{
			return MIO::readUint16LE(bytesPerSector);
		}

		void setBytesPerSector(sl_uint16 value)
		{
			MIO::writeUint16LE(bytesPerSector, value);
		}


		sl_uint8 getSectorsPerCluster() const
		{
			return sectorsPerCluster;
		}

		void setSectorsPerCluster(sl_uint8 value)
		{
			sectorsPerCluster = value;
		}

		
		sl_uint16 getSectorsPerTrack() const
		{
			return MIO::readUint16LE(sectorsPerTrack);
		}

		void setSectorsPerTrack(sl_uint16 value)
		{
			MIO::writeUint16LE(sectorsPerTrack, value);
		}


		sl_uint16 getNumberOfHeads() const
		{
			return MIO::readUint16LE(numberOfHeads);
		}

		void setNumberOfHeads(sl_uint16 value)
		{
			MIO::writeUint16LE(numberOfHeads, value);
		}


		sl_uint32 getHiddenSectors() const
		{
			return MIO::readUint32LE(hiddenSectors);
		}

		void setHiddenSectors(sl_uint32 value)
		{
			MIO::writeUint32LE(hiddenSectors, value);
		}


		sl_uint64 getTotalSectors() const
		{
			return MIO::readUint64LE(totalSectors);
		}

		void setTotalSectors(sl_uint64 value)
		{
			MIO::writeUint64LE(totalSectors, value);
		}


		sl_uint64 getMftClusterNumber() const
		{
			return MIO::readUint64LE(mftClusterNumber);
		}

		void setMftClusterNumber(sl_uint64 value)
		{
			MIO::writeUint64LE(mftClusterNumber, value);
		}


		sl_uint64 getMftMirrorClusterNumber() const
		{
			return MIO::readUint64LE(mftMirrorClusterNumber);
		}

		void setMftMirrorClusterNumber(sl_uint64 value)
		{
			MIO::writeUint64LE(mftMirrorClusterNumber, value);
		}


		sl_uint8 getClustersPerFileRecordSegment() const
		{
			return clustersPerFileRecordSegment;
		}

		void setClustersPerFileRecordSegment(sl_uint8 value)
		{
			clustersPerFileRecordSegment = value;
		}


		sl_uint8 getClustersPerIndexBuffer() const
		{
			return clustersPerIndexBuffer;
		}

		void setClustersPerIndexBuffer(sl_uint8 value)
		{
			clustersPerIndexBuffer = value;
		}


		// 8 bytes
		sl_uint8* getSerialNumber() const
		{
			return (sl_uint8*)serialNumber;
		}


		// 426 bytes
		sl_uint8* getBootstrapCode() const
		{
			return (sl_uint8*)bootstrapCode;
		}


		sl_uint16 getEndMarker() const
		{
			return MIO::readUint16LE(endMarker);
		}

		void setEndMarker(sl_uint16 value)
		{
			MIO::writeUint16LE(endMarker, value);
		}

	};

}

#endif
