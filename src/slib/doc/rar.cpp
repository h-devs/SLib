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

#include "slib/doc/rar.h"

#include "slib/core/file.h"
#include "slib/core/list.h"
#include "slib/core/memory_reader.h"

namespace slib
{

	RarBlockBaseHeader::RarBlockBaseHeader()
	{
	}

	RarBlockBaseHeader::~RarBlockBaseHeader()
	{
	}

	sl_bool RarBlockBaseHeader::readHeaderContent(IReader* reader, sl_size size, Memory* pMemory)
	{
		if (pMemory) {
			if (pMemory->getSize() < size) {
				rawHeader = reader->readToMemory(size);
				if (rawHeader.getSize() != size) {
					return sl_false;
				}
				*pMemory = rawHeader;
			} else {
				if (reader->readFully(pMemory->getData(), size) != size) {
					return sl_false;
				}
				rawHeader = pMemory->sub(0, size);
			}
		} else {
			rawHeader = reader->readToMemory(size);
			if (rawHeader.getSize() != size) {
				return sl_false;
			}
		}
		return sl_true;
	}

	RarBlockHeader4::RarBlockHeader4() : headerCRC(0), headerSize(0), type(RarBlockType4::Unknown)
	{
	}

	RarBlockHeader4::~RarBlockHeader4()
	{
	}

	sl_bool RarBlockHeader4::read(IReader* readerBlock, Memory* pMemory)
	{
		if (!(readerBlock->readUint16(&headerCRC))) {
			return sl_false;
		}
		sl_uint8 _type;
		if (!(readerBlock->readUint8(&_type))) {
			return sl_false;
		}
		type = (RarBlockType4)_type;
		sl_uint16 _flags;
		if (!(readerBlock->readUint16(&_flags))) {
			return sl_false;
		}
		flags = (int)_flags;
		if (!(readerBlock->readUint16(&headerSize))) {
			return sl_false;
		}
		if (!headerSize) {
			return sl_false;
		}
		if (headerSize < 7) {
			return sl_false;
		}
		sl_size n = headerSize - 7;
		if (n) {
			if (!(readHeaderContent(readerBlock, n, pMemory))) {
				return sl_false;
			}
		}
		return sl_true;
	}


	RarBlockHeader5::RarBlockHeader5() : headerCRC(0), headerSize(0), type(RarBlockType5::Unknown), extraAreaSize(0), dataSize(0), customHeaderPosition(0)
	{
	}

	RarBlockHeader5::~RarBlockHeader5()
	{
	}

	sl_bool RarBlockHeader5::read(IReader* readerBlock, Memory* pMemory)
	{
		if (!(readerBlock->readUint32(&headerCRC))) {
			return sl_false;
		}
		if (!(readerBlock->readCVLI32(&headerSize))) {
			return sl_false;
		}
		if (!headerSize) {
			return sl_false;
		}
		if (!(readHeaderContent(readerBlock, headerSize, pMemory))) {
			return sl_false;
		}
		MemoryReader readerHeader(rawHeader);
		sl_uint32 _type;
		if (!(readerHeader.readCVLI32(&_type))) {
			return sl_false;
		}
		if (_type < (sl_uint32)(RarBlockType5::Min)) {
			return sl_false;
		}
		if (_type > (sl_uint32)(RarBlockType5::Max)) {
			return sl_false;
		}
		type = (RarBlockType5)_type;
		if (!(readerHeader.readCVLI32((sl_uint32*)&(flags.value)))) {
			return sl_false;
		}
		if (flags & RarBlockFlags5::ExtraArea) {
			if (!(readerHeader.readCVLI64(&extraAreaSize))) {
				return sl_false;
			}
		} else {
			extraAreaSize = 0;
		}
		if (flags & RarBlockFlags5::DataArea) {
			if (!(readerHeader.readCVLI64(&dataSize))) {
				return sl_false;
			}
		} else {
			dataSize = 0;
		}
		customHeaderPosition = readerHeader.getPosition();
		return sl_true;
	}


	RarExtraArea5::RarExtraArea5() : size(0), type(RarExtraType5::Unknown), dataPosition(0), dataSize(0)
	{
	}

	RarExtraArea5::~RarExtraArea5()
	{
	}

	sl_bool RarExtraArea5::read(MemoryReader& reader)
	{
		if (!(reader.readCVLI32(&size))) {
			return sl_false;
		}
		sl_size pos = reader.getPosition();
		sl_size end = pos + size;
		sl_uint32 _type;
		if (!(reader.readCVLI32(&_type))) {
			return sl_false;
		}
		type = (RarExtraType5)_type;
		dataPosition = reader.getPosition();
		if (dataPosition > end) {
			return sl_false;
		}
		dataSize = end - dataPosition;
		if (!(reader.seek(dataSize, SeekPosition::Current))) {
			return sl_false;
		}
		return sl_true;
	}


	RarMainBlock4::RarMainBlock4(): posAV(0)
	{
	}

	RarMainBlock4::~RarMainBlock4()
	{
	}

	sl_bool RarMainBlock4::readHeader(const RarBlockHeader4& header)
	{
		MemoryReader reader(header.rawHeader);
		sl_uint16 _highPosAV;
		sl_uint32 _posAV;
		if (!(reader.readUint16(&_highPosAV))) {
			return sl_false;
		}
		if (!(reader.readUint32(&_posAV))) {
			return sl_false;
		}
		posAV = _posAV | (((sl_uint64)_highPosAV) << 32);
		return sl_true;
	}


	RarMainBlock5::RarMainBlock5(): volumeNumber(0)
	{
	}

	RarMainBlock5::~RarMainBlock5()
	{
	}

	sl_bool RarMainBlock5::readHeader(const RarBlockHeader5& header)
	{
		MemoryReader reader(header.rawHeader);
		if (!(reader.seek(header.customHeaderPosition, SeekPosition::Begin))) {
			return sl_false;
		}
		if (!(reader.readCVLI32((sl_uint32*)&(flags.value)))) {
			return sl_false;
		}
		if (flags & RarArchiveFlags5::VolumeNumber) {
			if (!(reader.readCVLI64(&volumeNumber))) {
				return sl_false;
			}
		}
		return sl_true;
	}


	RarEncryptionBlock5::RarEncryptionBlock5() : version(0), countKDF(0)
	{
	}

	RarEncryptionBlock5::~RarEncryptionBlock5()
	{
	}

	sl_bool RarEncryptionBlock5::readHeader(const RarBlockHeader5& header)
	{
		MemoryReader reader(header.rawHeader);
		if (!(reader.seek(header.customHeaderPosition, SeekPosition::Begin))) {
			return sl_false;
		}
		if (!(reader.readCVLI32(&version))) {
			return sl_false;
		}
		if (!(reader.readCVLI32((sl_uint32*)&(flags.value)))) {
			return sl_false;
		}
		if (!(reader.readUint8(&countKDF))) {
			return sl_false;
		}
		if (reader.readFully(salt, 16) != 16) {
			return sl_false;
		}
		if (flags & RarEncryptionFlags5::PasswordCheck) {
			if (reader.readFully(checkValue, 12) != 12) {
				return sl_false;
			}
		}
		return sl_true;
	}


	RarCompressionInformation5::RarCompressionInformation5() : version(0), flagSolid(sl_false), method(0), dictionarySize(0)
	{
	}

	RarCompressionInformation5::~RarCompressionInformation5()
	{
	}

	void RarCompressionInformation5::setValue(sl_uint32 value)
	{
		version = (sl_uint8)(value & 0x3f);
		flagSolid = (value & 0x40) != 0;
		method = (sl_uint8)((value & 0x380) >> 7);
		dictionarySize = (sl_uint8)((value & 0x3c00) >> 10);
	}


	RarFileBlock4::RarFileBlock4() : dataSize(0), fileSize(0), flagUnknownFileSize(sl_false), flagDirectory(sl_false), hostOS(RarHostOS4::Unix), fileCRC(0), modifiedTime(0), compressionVersion(0), compressionMethod(0), attributes(0)
	{
	}

	RarFileBlock4::~RarFileBlock4()
	{
	}

	sl_bool RarFileBlock4::readHeader(MemoryReader& reader, const RarBlockHeader4& header, sl_bool flagReadName)
	{
		flagDirectory = (header.flags & RarBlockFlags4::File_Window_Mask) == RarBlockFlags4::File_Window_Directory;
		sl_uint32 dataSizeLow;
		if (!(reader.readUint32(&dataSizeLow))) {
			return sl_false;
		}
		sl_uint32 fileSizeLow;
		if (!(reader.readUint32(&fileSizeLow))) {
			return sl_false;
		}
		sl_uint8 _hostOS;
		if (!(reader.readUint8(&_hostOS))) {
			return sl_false;
		}
		hostOS = (RarHostOS4)_hostOS;
		if (!(reader.readUint32(&fileCRC))) {
			return sl_false;
		}
		if (!(reader.readUint32(&modifiedTime))) {
			return sl_false;
		}
		if (!(reader.readUint8(&compressionVersion))) {
			return sl_false;
		}
		if (!(reader.readUint8(&compressionMethod))) {
			return sl_false;
		}
		sl_uint16 lenName;
		if (!(reader.readUint16(&lenName))) {
			return sl_false;
		}
		if (!(reader.readUint32(&attributes))) {
			return sl_false;
		}
		if (compressionVersion < 20 && (attributes & 0x10)) {
			flagDirectory = sl_true;
		}
		flagUnknownFileSize = sl_false;
		if (header.flags & RarBlockFlags4::File_Large) {
			sl_uint32 dataSizeHigh;
			if (!(reader.readUint32(&dataSizeHigh))) {
				return sl_false;
			}
			sl_uint32 fileSizeHigh;
			if (!(reader.readUint32(&fileSizeHigh))) {
				return sl_false;
			}
			dataSize = SLIB_MAKE_QWORD4(dataSizeHigh, dataSizeLow);
			fileSize = SLIB_MAKE_QWORD4(fileSizeHigh, fileSizeLow);
			if (fileSize == SLIB_UINT64(0xffffffffffffffff)) {
				flagUnknownFileSize = sl_true;
			}
		} else {
			dataSize = dataSizeLow;
			fileSize = fileSizeLow;
			if (fileSizeLow == 0xffffffff) {
				flagUnknownFileSize = sl_true;
			}
		}
		if (flagUnknownFileSize) {
			fileSize = 0;
		}
		if (lenName) {
			if (flagReadName) {
				String _name = String::allocate(lenName);
				if (_name.isEmpty()) {
					return sl_false;
				}
				if (reader.readFully(_name.getData(), lenName) != lenName) {
					return sl_false;
				}
				name = _name;
			} else {
				if (!(reader.seek(lenName, SeekPosition::Current))) {
					return sl_false;
				}
			}
		}
		if (header.flags & RarBlockFlags4::File_Salt) {
			if (reader.readFully(salt, 8) != 8) {
				return sl_false;
			}
		}
		if (header.flags & RarBlockFlags4::File_ExtTime) {
			// createdTime, accessTime
		}
		return sl_true;
	}

	sl_bool RarFileBlock4::readHeader(const RarBlockHeader4& header, sl_bool flagReadName)
	{
		MemoryReader reader(header.rawHeader);
		return readHeader(reader, header, flagReadName);
	}

	sl_bool RarFileBlock4::isDirectory()
	{
		return flagDirectory;
	}


	RarFileBlock5::RarFileBlock5() : fileSize(0), attributes(0), modifiedTime(0), fileCRC(0), hostOS(RarHostOS5::Unix)
	{
	}

	RarFileBlock5::~RarFileBlock5()
	{
	}

	sl_bool RarFileBlock5::readHeader(MemoryReader& reader, sl_bool flagReadName)
	{
		if (!(reader.readCVLI32((sl_uint32*)&(flags.value)))) {
			return sl_false;
		}
		if (!(reader.readCVLI64(&fileSize))) {
			return sl_false;
		}
		if (!(reader.readCVLI64(&attributes))) {
			return sl_false;
		}
		if (flags & RarFileFlags5::Time) {
			if (!(reader.readUint32(&modifiedTime))) {
				return sl_false;
			}
		}
		if (flags & RarFileFlags5::CRC32) {
			if (!(reader.readUint32(&fileCRC))) {
				return sl_false;
			}
		}
		sl_uint32 _compression;
		if (!(reader.readCVLI32(&_compression))) {
			return sl_false;
		}
		compression.setValue(_compression);
		sl_uint32 _hostOS;
		if (!(reader.readCVLI32(&_hostOS))) {
			return sl_false;
		}
		hostOS = (RarHostOS5)_hostOS;
		sl_uint32 lenName;
		if (!(reader.readCVLI32(&lenName))) {
			return sl_false;
		}
		if (lenName) {
			if (flagReadName) {
				String _name = String::allocate(lenName);
				if (_name.isEmpty()) {
					return sl_false;
				}
				if (reader.readFully(_name.getData(), lenName) != lenName) {
					return sl_false;
				}
				name = _name;
			} else {
				return reader.seek(lenName, SeekPosition::Current);
			}
		}
		return sl_true;
	}

	sl_bool RarFileBlock5::readHeader(const RarBlockHeader5& header, sl_bool flagReadName)
	{
		MemoryReader reader(header.rawHeader);
		if (!(reader.seek(header.customHeaderPosition, SeekPosition::Begin))) {
			return sl_false;
		}
		return readHeader(reader, flagReadName);
	}

	sl_bool RarFileBlock5::isDirectory()
	{
		return flags & RarFileFlags5::Directory;
	}


	RarFileEncryptionRecord5::RarFileEncryptionRecord5(): version(0), countKDF(0)
	{
	}

	RarFileEncryptionRecord5::~RarFileEncryptionRecord5()
	{
	}

	sl_bool RarFileEncryptionRecord5::read(const void* data, sl_size size)
	{
		MemoryReader reader(data, size);
		if (!(reader.readCVLI32(&version))) {
			return sl_false;
		}
		if (!(reader.readCVLI32((sl_uint32*)&(flags.value)))) {
			return sl_false;
		}
		if (!(reader.readUint8(&countKDF))) {
			return sl_false;
		}
		if (reader.readFully(salt, 16) != 16) {
			return sl_false;
		}
		if (reader.readFully(IV, 16) != 16) {
			return sl_false;
		}
		if (flags & RarEncryptionFlags5::PasswordCheck) {
			if (reader.readFully(checkValue, 12) != 12) {
				return sl_false;
			}
		}
		return sl_true;
	}


	RarFile::RarFile(): flagRAR5(sl_false), flagEncryptedHeaders(sl_false)
	{
	}

	RarFile::~RarFile()
	{
	}

	sl_bool RarFile::setReader(const Ptrx<IReader, ISeekable>& reader)
	{
		return m_reader.setReader(reader);
	}

	sl_bool RarFile::readSignature()
	{
		sl_uint8 signature[7];
		if (!(m_reader.readFully(signature, 7))) {
			return sl_false;
		}
		if (signature[0] != 0x52 || signature[1] != 0x61 || signature[2] != 0x72 || signature[3] != 0x21 || signature[4] != 0x1A || signature[5] != 0x07) {
			return sl_false;
		}
		if (signature[6] == 0) {
			flagRAR5 = sl_false; // RAR 4.x
		} else if (signature[6] == 0x01) {
			if (m_reader.readUint8() == 0) {
				flagRAR5 = sl_true; // RAR 5.0
			} else {
				return sl_false;
			}
		} else {
			return sl_false;
		}
		return sl_true;
	}

	sl_bool RarFile::readMainHeader()
	{
		flagEncryptedHeaders = sl_false;
		if (flagRAR5) {
			RarBlockHeader5 header;
			while (readBlockHeaderAndSkipData(header)) {
				if (header.type == RarBlockType5::Main) {
					return mainBlock5.readHeader(header);
				} else if (header.type == RarBlockType5::Encryption) {
					if (encryption5.readHeader(header)) {
						flagEncryptedHeaders = sl_true;
					}
					return sl_false;
				}
			}
		} else {
			RarBlockHeader4 header;
			while (header.read(m_reader, &m_bufferHeader)) {
				if (header.type == RarBlockType4::Main) {
					if (mainBlock4.readHeader(header)) {
						if (header.flags & RarBlockFlags4::Main_Password) {
							flagEncryptedHeaders = sl_true;
						}
						return sl_true;
					}
				}
			}
		}
		return sl_false;
	}

	sl_bool RarFile::readFromSignatureToMainHeader()
	{
		if (readSignature()) {
			return readMainHeader();
		}
		return sl_false;
	}

	List<String> RarFile::readFileNames()
	{
		if (flagEncryptedHeaders) {
			return sl_null;
		}
		List<String> list;
		if (flagRAR5) {
			RarBlockHeader5 header;
			RarFileBlock5 fileBlock;
			while (readBlockHeaderAndSkipData(header)) {
				if (header.type == RarBlockType5::File) {
					if (fileBlock.readHeader(header)) {
						if (!(fileBlock.isDirectory())) {
							list.add_NoLock(fileBlock.name);
						}
					}
				}
			}
		} else {
			RarBlockHeader4 header;
			RarFileBlock4 fileBlock;
			while (readBlockHeader(header)) {
				if (header.type == RarBlockType4::File) {
					if (fileBlock.readHeader(header)) {
						if (!(fileBlock.isDirectory())) {
							list.add_NoLock(fileBlock.name);
						}
						if (m_reader.skip(fileBlock.dataSize) != fileBlock.dataSize) {
							break;
						}
					} else {
						break;
					}
				} else {
					if (!(skipData(header))) {
						break;
					}
				}
			}
		}
		return list;
	}

	sl_bool RarFile::isEncrypted(sl_int32 maxCheckFileCount)
	{
		if (flagEncryptedHeaders) {
			return sl_true;
		}
		if (!maxCheckFileCount) {
			return sl_false;
		}
		sl_int32 nCheckCount = 0;
		sl_bool flagCheckLimit = maxCheckFileCount > 0;
		if (flagRAR5) {
			RarBlockHeader5 header;
			RarFileBlock5 fileBlock;
			RarExtraArea5 extra;
			while (readBlockHeaderAndSkipData(header)) {
				if (header.type == RarBlockType5::File) {
					if (header.flags & RarBlockFlags5::ExtraArea) {
						MemoryReader reader(header.rawHeader);
						if (reader.seek(header.customHeaderPosition, SeekPosition::Begin)) {
							if (fileBlock.readHeader(reader, sl_false)) {
								while (extra.read(reader)) {
									if (extra.type == RarExtraType5::FileEncryption) {
										return sl_true;
									}
								}
							}
						}
					}
					if (flagCheckLimit) {
						nCheckCount++;
						if (nCheckCount >= maxCheckFileCount) {
							break;
						}
					}
				}
			}
		} else {
			RarBlockHeader4 header;
			RarFileBlock4 fileBlock;
			while (readBlockHeader(header)) {
				if (header.type == RarBlockType4::File) {
					if (header.flags & RarBlockFlags4::File_Password) {
						return sl_true;
					}
					if (flagCheckLimit) {
						nCheckCount++;
						if (nCheckCount >= maxCheckFileCount) {
							break;
						}
					}
				}
				if (!(skipData(header))) {
					break;
				}
			}
		}
		return sl_false;
	}

	sl_bool RarFile::readBlockHeader(RarBlockHeader4& header)
	{
		return header.read(m_reader, &m_bufferHeader);
	}

	sl_bool RarFile::readBlockHeader(RarBlockHeader5& header)
	{
		return header.read(m_reader, &m_bufferHeader);
	}

	sl_bool RarFile::skipData(const RarBlockHeader4& header)
	{
		sl_uint64 size = 0;
		switch (header.type) {
		case RarBlockType4::Main:
		case RarBlockType4::End:
		case RarBlockType4::Comment:
			break;
		case RarBlockType4::File:
		case RarBlockType4::Service:
			{
				MemoryReader reader(header.rawHeader);
				sl_uint32 low;
				if (!(reader.readUint32(&low))) {
					return sl_false;
				}
				if (!(reader.seek(21, SeekPosition::Current))) {
					return sl_false;
				}
				if (header.flags & RarBlockFlags4::File_Large) {
					sl_uint32 high;
					if (!(reader.readUint32(&high))) {
						return sl_false;
					}
					size = SLIB_MAKE_QWORD4(high, low);
				} else {
					size = low;
				}
			}
			break;
		case RarBlockType4::Protect:
		case RarBlockType4::OldService:
			{
				sl_uint32 n;
				MemoryReader reader(header.rawHeader);
				if (reader.readUint32(&n)) {
					size = n;
				} else {
					return sl_false;
				}
			}
			break;
		default:
			if (header.flags & RarBlockFlags4::LongBlock) {
				sl_uint32 n;
				MemoryReader reader(header.rawHeader);
				if (reader.readUint32(&n)) {
					size = n;
				} else {
					return sl_false;
				}
			}
			break;
		}
		if (size) {
			return m_reader.skip(size) == size;
		}
		return sl_true;
	}

	sl_bool RarFile::skipData(const RarBlockHeader5& header)
	{
		return m_reader.skip(header.dataSize) == header.dataSize;
	}

	sl_bool RarFile::readBlockHeaderAndSkipData(RarBlockHeader5& header)
	{
		if (!(readBlockHeader(header))) {
			return sl_false;
		}
		return skipData(header);
	}

	sl_uint32 RarFile::getVersion(const Ptrx<IReader, ISeekable>& reader)
	{
		RarFile rar;
		if (rar.setReader(reader)) {
			if (rar.readSignature()) {
				if (rar.flagRAR5) {
					return 5;
				} else {
					return 4;
				}
			}
		}
		return 0;
	}

	sl_uint32 RarFile::getFileVersion(const StringParam& path)
	{
		Ref<File> file = File::openForRead(path);
		if (file.isNotNull()) {
			return getVersion(file);
		}
		return 0;
	}

	List<String> RarFile::getFileNames(const Ptrx<IReader, ISeekable>& reader)
	{
		RarFile rar;
		if (rar.setReader(reader)) {
			if (rar.readFromSignatureToMainHeader()) {
				return rar.readFileNames();
			}
		}
		return sl_null;
	}

	List<String> RarFile::getFileNamesInFile(const StringParam& path)
	{
		Ref<File> file = File::openForRead(path);
		if (file.isNotNull()) {
			return getFileNames(file);
		}
		return sl_null;
	}

	sl_bool RarFile::isEncrypted(const Ptrx<IReader, ISeekable>& reader, sl_int32 maxCheckFileCount)
	{
		RarFile rar;
		if (rar.setReader(reader)) {
			if (rar.readSignature()) {
				if (rar.readMainHeader()) {
					return rar.isEncrypted(maxCheckFileCount);
				} else {
					return rar.flagEncryptedHeaders;
				}
			}
		}
		return sl_false;
	}

	sl_bool RarFile::isEncryptedFile(const StringParam& path, sl_int32 maxCheckFileCount)
	{
		Ref<File> file = File::openForRead(path);
		if (file.isNotNull()) {
			return isEncrypted(file, maxCheckFileCount);
		}
		return sl_false;
	}

}
