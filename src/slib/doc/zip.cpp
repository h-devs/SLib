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

#include "slib/doc/zip.h"

#include "slib/io/file.h"
#include "slib/io/memory_reader.h"
#include "slib/io/memory_output.h"
#include "slib/data/crc32.h"
#include "slib/data/zlib.h"
#include "slib/data/zstd.h"
#include "slib/core/mio.h"

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

		template <class HEADER>
		static Time ParseModifiedTime(HEADER* header)
		{
			sl_uint16 lastModifiedTime = MIO::readUint16LE(header->lastModifiedTime);
			sl_uint16 lastModifiedDate = MIO::readUint16LE(header->lastModifiedDate);
			if (lastModifiedTime || lastModifiedDate) {
				sl_uint16 hour = lastModifiedTime >> 11;
				sl_uint16 minute = (lastModifiedTime >> 5) & 63;
				sl_uint16 second = (lastModifiedTime << 1) & 63;
				sl_uint16 year = 1980 + (lastModifiedDate >> 9);
				sl_uint16 month = (lastModifiedTime >> 5) & 15;
				sl_uint16 day = lastModifiedTime & 31;
				return Time(year, month, day, hour, minute, second);
			} else {
				return Time::zero();
			}
		}

		class ZipArchiver
		{
		public:
			sl_uint32 m_nTotalFiles = 0;
			sl_uint64 m_offsetCurrent = 0;
			MemoryBuffer m_bufCenteralDir;

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
					MIO::writeUint32LE(header.exteralFileAttrs, element.attributes);
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

		class ZipUnarchiver
		{
		public:
			sl_uint32 m_nTotalFiles = 0;
			sl_uint32 m_offsetDir = 0;
			sl_uint32 m_endDir = 0;

		public:
			sl_bool start(IReader* reader, ISeekable* seeker)
			{
				// End of Central Dir Header
				{
					if (!(seeker->seek(-ZIP_END_OF_CENTRAL_DIR_SIZE, SeekPosition::End))) {
						return sl_false;
					}
					ZipEndOfCentralDirRecord header;
					if (reader->readFully(&header, ZIP_END_OF_CENTRAL_DIR_SIZE) != ZIP_END_OF_CENTRAL_DIR_SIZE) {
						return sl_false;
					}
					if (MIO::readUint32LE(header.signature) != ZIP_END_OF_CENTRAL_DIR_SIG) {
						return sl_false;
					}
					m_nTotalFiles = MIO::readUint16LE(header.totalFilesOnDisk);
					m_offsetDir = MIO::readUint32LE(header.offsetStart);
					m_endDir = m_offsetDir + MIO::readUint32LE(header.size);
				}
				return sl_true;
			}

			sl_bool readEntry(IReader* reader, ISeekable* seeker, ZipElement& element)
			{
				sl_uint32 crc = 0;
				sl_uint32 sizeCompressed = 0;
				sl_uint32 offsetLocalHeader = 0;

				// Central Dir Header
				{
					if (m_offsetDir + sizeof(ZipCentralDirHeader) > m_endDir) {
						return sl_false;
					}
					if (!(seeker->seek(m_offsetDir, SeekPosition::Begin))) {
						return sl_false;
					}
					ZipCentralDirHeader header;
					if (reader->readFully(&header, sizeof(header)) != sizeof(header)) {
						return sl_false;
					}
					if (MIO::readUint32LE(header.signature) != ZIP_CENTRAL_DIR_HEADER_SIG) {
						return sl_false;
					}
					element.compressionMethod = (ZipCompressionMethod)(MIO::readUint16LE(header.compressionMethod));
					element.lastModifiedTime = ParseModifiedTime(&header);
					element.attributes = MIO::readUint32LE(header.exteralFileAttrs);
					crc = MIO::readUint32LE(header.crc32);
					sizeCompressed = MIO::readUint32LE(header.compressedSize);
					offsetLocalHeader = MIO::readUint32LE(header.relativeOffsetOfLocalHeader);
					m_offsetDir += sizeof(header);

					sl_uint16 lenFilePath = MIO::readUint16LE(header.lenFileName);
					if (lenFilePath) {
						if (m_offsetDir + lenFilePath > m_endDir) {
							return sl_false;
						}
						element.filePath = String::allocate(lenFilePath);
						if (element.filePath.isNull()) {
							return sl_false;
						}
						if (reader->readFully(element.filePath.getData(), lenFilePath) != lenFilePath) {
							return sl_false;
						}
						m_offsetDir += lenFilePath;
					}
					sl_uint16 lenExtra = MIO::readUint16LE(header.lenExtraField);
					if (lenExtra) {
						if (m_offsetDir + lenExtra > m_endDir) {
							return sl_false;
						}
						if (!(seeker->seek(lenExtra, SeekPosition::Current))) {
							return sl_false;
						}
						m_offsetDir += lenExtra;
					}
					sl_uint16 lenComment = MIO::readUint16LE(header.lenComment);
					if (lenComment) {
						if (m_offsetDir + lenComment > m_endDir) {
							return sl_false;
						}
						if (!(seeker->seek(lenComment, SeekPosition::Current))) {
							return sl_false;
						}
						m_offsetDir += lenComment;
					}
				}

				// Local File Header
				{
					if (!(seeker->seek(offsetLocalHeader, SeekPosition::Begin))) {
						return sl_false;
					}
					ZipLocalFileHeader header;
					if (reader->readFully(&header, ZIP_LOCAL_FILE_HEADER_SIZE) != ZIP_LOCAL_FILE_HEADER_SIZE) {
						return sl_false;
					}
					sl_uint32 signature = MIO::readUint32LE(header.signature);
					if (signature != ZIP_LOCAL_FILE_HEADER_SIG) {
						return sl_false;
					}
					sl_uint16 lenFilePath = MIO::readUint16LE(header.lenFileName);
					if (lenFilePath) {
						if (!(seeker->seek(lenFilePath, SeekPosition::Current))) {
							return sl_false;
						}
					}
					sl_uint16 lenExtra = MIO::readUint16LE(header.lenExtraField);
					if (lenExtra) {
						if (!(seeker->seek(lenExtra, SeekPosition::Current))) {
							return sl_false;
						}
					}
				}

				// Content
				if (sizeCompressed) {
					Memory memCompressed = reader->readFully(sizeCompressed);
					if (memCompressed.isNull()) {
						return sl_false;
					}
					if (element.compressionMethod == ZipCompressionMethod::Store) {
						element.content = Move(memCompressed);
					} else if (element.compressionMethod == ZipCompressionMethod::Deflated) {
						element.content = Zlib::decompressRaw(memCompressed.getData(), memCompressed.getSize());
					} else if (element.compressionMethod == ZipCompressionMethod::Zstandard) {
						element.content = Zstd::decompress(memCompressed.getData(), memCompressed.getSize());
					} else {
						return sl_false;
					}
					if (element.content.isNull()) {
						return sl_false;
					}
				}
				element.flagValidCrc = crc == Crc32::get(element.content);
				element.flagDirectory = element.filePath.endsWith('/') || element.filePath.endsWith('\\');
				return sl_true;
			}

		};

	}


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(ZipFileInfo)

	ZipFileInfo::ZipFileInfo()
	{
		compressionMethod = ZipCompressionMethod::Deflated;
		attributes = 0;
		flagValidCrc = sl_false;
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

	List<ZipElement> Zip::unarchive(const MemoryView& zip)
	{
		MemoryReader input(zip.data, zip.size);
		ZipUnarchiver unarchiver;
		if (!(unarchiver.start(&input, &input))) {
			return sl_null;
		}
		List<ZipElement> ret;
		for (;;) {
			ZipElement element;
			if (!(unarchiver.readEntry(&input, &input, element))) {
				break;
			}
			if (!(ret.add_NoLock(Move(element)))) {
				break;
			}
		}
		return ret;
	}

}
