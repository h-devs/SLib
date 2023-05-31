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

#include "slib/doc/zip.h"

#include "slib/core/mio.h"
#include "slib/io/file.h"
#include "slib/io/memory_output.h"
#include "slib/data/crc32.h"
#include "slib/data/zlib.h"
#include "slib/data/zstd.h"

#define ZIP_VERSION 64 //6.4

#define ZIP_LOCAL_FILE_HEADER_SIZE 30
#define ZIP_CENTRAL_DIR_HEADER_SIZE 46
#define ZIP_END_OF_CENTRAL_DIR_SIZE 22

#define ZIP_LOCAL_FILE_HEADER_SIG 0x04034b50
#define ZIP_CENTRAL_DIR_HEADER_SIG 0x02014b50
#define ZIP_END_OF_CENTRAL_DIR_SIG 0x06054b50

namespace slib
{

	namespace {

		struct ZipLocalFileHeader
		{
			sl_uint8 signature[4];
			sl_uint8 version[2];
			sl_uint8 generalFlags[2];
			sl_uint8 compressionMethod[2];
			sl_uint8 lastModifiedTime[2];
			sl_uint8 lastModifiedDate[2];
			sl_uint8 crc32[4];
			sl_uint8 compressedSize[4];
			sl_uint8 uncompressedSize[4];
			sl_uint8 lenFileName[2];
			sl_uint8 lenExtraField[2];
		};

		struct ZipCentralDirHeader
		{
			sl_uint8 signature[4];
			sl_uint8 versionMadeBy[2];
			sl_uint8 versionNeededToExtract[2];
			sl_uint8 generalFlags[2];
			sl_uint8 compressionMethod[2];
			sl_uint8 lastModifiedTime[2];
			sl_uint8 lastModifiedDate[2];
			sl_uint8 crc32[4];
			sl_uint8 compressedSize[4];
			sl_uint8 uncompressedSize[4];
			sl_uint8 lenFileName[2];
			sl_uint8 lenExtraField[2];
			sl_uint8 lenComment[2];
			sl_uint8 diskNumberStart[2];
			sl_uint8 interalFileAttrs[2];
			sl_uint8 exteralFileAttrs[4];
			sl_uint8 relativeOffsetOfLocalHeader[4];
		};

		struct ZipEndOfCentralDirRecord
		{
			sl_uint8 signature[4];
			sl_uint8 diskNumber[2]; // number of this disk
			sl_uint8 diskNumberStart[2]; // number of the disk with the start of the central directory
			sl_uint8 totalFilesOnDisk[2]; // total number of entries in the central directory on this disk
			sl_uint8 totalFiles[2]; // total number of entries in the central directory
			sl_uint8 size[4]; // size of the central directory
			sl_uint8 offsetStart[4]; // offset of start of central directory with respect to the starting disk number
			sl_uint8 lenComment[2];
		};

		template <class HEADER>
		static void FillModifiedTime(HEADER* header, const Time& time)
		{
			if (time.isNotZero()) {
				TimeComponents c;
				time.get(c);
				MIO::writeUint16LE(header->lastModifiedTime, ((sl_uint16)(c.hour) << 11) + ((sl_uint16)(c.minute) << 5) +
					(c.second >> 1));
				MIO::writeUint16LE(header->lastModifiedDate, ((sl_uint16)(c.year - 1980) << 9) + ((sl_uint16)(c.month) << 5) + c.day);
			} else {
				MIO::writeUint16LE(header->lastModifiedTime, 0);
				MIO::writeUint16LE(header->lastModifiedDate, 0);
			}
		}

		class ZipArchiver
		{
		public:
			sl_uint32 m_nTotalFiles;
			sl_uint64 m_offsetCurrent;
			MemoryBuffer m_bufCenteralDir;

		public:
			ZipArchiver()
			{
				m_nTotalFiles = 0;
				m_offsetCurrent = 0;
			}

		public:
			sl_bool writeEntry(IWriter* writer, const ZipElement& element)
			{
				StringCstr path(element.filePath);
				sl_size lenFilePath = path.getLength();
				if (lenFilePath >> 16) {
					return sl_false;
				}
				Memory memCompressed;
				sl_uint16 versionNeeded;
				ZipCompressionMethod method = element.compressionMethod;
				if (element.content.isNull()) {
					versionNeeded = 0;
					method = ZipCompressionMethod::Store;
				} else {
					if (method == ZipCompressionMethod::Deflated) {
						versionNeeded = 20; // 2.0
						sl_int32 level;
						if (element.compressionLevel.isNotNull()) {
							level = element.compressionLevel;
						} else {
							level = 6;
						}
						memCompressed = Zlib::compressRaw(element.content.getData(), element.content.getSize(), level);
					} else if (method == ZipCompressionMethod::Store) {
						versionNeeded = 20; // 2.0
						memCompressed = element.content;
					} else if (method == ZipCompressionMethod::Zstandard) {
						versionNeeded = 63; // 6.3
						sl_int32 level;
						if (element.compressionLevel.isNotNull()) {
							level = element.compressionLevel;
						} else {
							level = 3;
						}
						memCompressed = Zstd::compress(element.content.getData(), element.content.getSize(), level);
					} else {
						return sl_false;
					}
					if (memCompressed.isNull()) {
						return sl_false;
					}
				}

				sl_size sizeCompressed = memCompressed.getSize();
				sl_uint64 offsetLocalHeader = m_offsetCurrent;
				sl_uint32 crc = Crc32::get(element.content);

				// Local File Header
				{
					ZipLocalFileHeader header;
					MIO::writeUint32LE(header.signature, ZIP_LOCAL_FILE_HEADER_SIG);
					MIO::writeUint16LE(header.version, versionNeeded);
					MIO::writeUint16LE(header.generalFlags, 0);
					MIO::writeUint16LE(header.compressionMethod, (sl_uint16)(method));
					FillModifiedTime(&header, element.lastModifiedTime);
					MIO::writeUint32LE(header.crc32, crc);
					MIO::writeUint32LE(header.compressedSize, (sl_uint32)sizeCompressed);
					MIO::writeUint32LE(header.uncompressedSize, (sl_uint32)(element.content.getSize()));
					MIO::writeUint16LE(header.lenFileName, (sl_uint16)lenFilePath);
					MIO::writeUint16LE(header.lenExtraField, 0);

					if (writer->writeFully(&header, ZIP_LOCAL_FILE_HEADER_SIZE) != ZIP_LOCAL_FILE_HEADER_SIZE) {
						return sl_false;
					}
					m_offsetCurrent += ZIP_LOCAL_FILE_HEADER_SIZE;
				}

				// File path
				if (lenFilePath) {
					if (writer->writeFully(path.getData(), lenFilePath) != lenFilePath) {
						return sl_false;
					}
					m_offsetCurrent += lenFilePath;
				}

				// Content
				if (sizeCompressed) {
					if (writer->writeFully(memCompressed.getData(), sizeCompressed) != sizeCompressed) {
						return sl_false;
					}
					m_offsetCurrent += sizeCompressed;
				}

				// Pushing Central Dir Header
				{
					ZipCentralDirHeader header;
					Base::zeroMemory(&header, sizeof(header));
					MIO::writeUint32LE(header.signature, ZIP_CENTRAL_DIR_HEADER_SIG);
					MIO::writeUint16LE(header.versionMadeBy, ZIP_VERSION);
					MIO::writeUint16LE(header.versionNeededToExtract, versionNeeded);
					MIO::writeUint16LE(header.generalFlags, 0);
					MIO::writeUint16LE(header.compressionMethod, (sl_uint16)(method));
					FillModifiedTime(&header, element.lastModifiedTime);
					MIO::writeUint32LE(header.crc32, crc);
					MIO::writeUint32LE(header.compressedSize, (sl_uint32)sizeCompressed);
					MIO::writeUint32LE(header.uncompressedSize, (sl_uint32)(element.content.getSize()));
					MIO::writeUint16LE(header.lenFileName, (sl_uint16)lenFilePath);
					MIO::writeUint16LE(header.lenExtraField, 0);
					MIO::writeUint16LE(header.lenComment, 0);
					MIO::writeUint32LE(header.exteralFileAttrs, 0);
					MIO::writeUint32LE(header.relativeOffsetOfLocalHeader, (sl_uint32)offsetLocalHeader);

					if (!(m_bufCenteralDir.addNew(&header, ZIP_CENTRAL_DIR_HEADER_SIZE))) {
						return sl_false;
					}
					if (lenFilePath) {
						if (!(m_bufCenteralDir.addNew(path.getData(), lenFilePath))) {
							return sl_false;
						}
					}
				}

				m_nTotalFiles++;
				return sl_true;
			}

			sl_bool end(IWriter* writer)
			{
				sl_uint64 offset = m_offsetCurrent;
				sl_size sizeCentralDir = m_bufCenteralDir.getSize();
				// Central Dir Header
				{
					MemoryData data;
					while (m_bufCenteralDir.pop(data)) {
						if (writer->writeFully(data.data, data.size) != data.size) {
							return sl_false;
						}
					}
				}
				// End of Central Dir Header
				{
					ZipEndOfCentralDirRecord header;
					Base::zeroMemory(&header, sizeof(header));
					MIO::writeUint32LE(header.signature, ZIP_END_OF_CENTRAL_DIR_SIG);
					MIO::writeUint16LE(header.totalFilesOnDisk, (sl_uint16)m_nTotalFiles);
					MIO::writeUint16LE(header.totalFiles, (sl_uint16)m_nTotalFiles);
					MIO::writeUint32LE(header.size, (sl_uint32)sizeCentralDir);
					MIO::writeUint32LE(header.offsetStart, (sl_uint32)offset);
					if (writer->writeFully(&header, ZIP_END_OF_CENTRAL_DIR_SIZE) != ZIP_END_OF_CENTRAL_DIR_SIZE) {
						return sl_false;
					}
				}
				return sl_true;
			}

		};

	}


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(ZipFileInfo)

	ZipFileInfo::ZipFileInfo()
	{
		compressionMethod = ZipCompressionMethod::Deflated;
	}


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(ZipElement)

	ZipElement::ZipElement()
	{
	}


	Memory Zip::archive(const ListParam<ZipElement>& _elements)
	{
		MemoryOutput output;
		ZipArchiver archiver;
		ListLocker<ZipElement> elements(_elements);
		for (sl_size i = 0; i < elements.count; i++) {
			if (!(archiver.writeEntry(&output, elements[i]))) {
				return sl_null;
			}
		}
		if (!(archiver.end(&output))) {
			return sl_null;
		}
		return output.merge();
	}

}
