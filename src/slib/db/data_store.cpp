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

#include "slib/db/data_package.h"
#include "slib/db/data_store.h"

#include "slib/db/key_value_store.h"
#include "slib/crypto/sha3.h"
#include "slib/core/serialize.h"
#include "slib/core/scoped_buffer.h"
#include "slib/core/safe_static.h"

/*

			Data Package File Format
	
	---------------- Header -----------------------
	Version (4 bytes)
	Flags (4 bytes)
	First Item Position (8 Bytes)
	Position After Last Item (8 Bytes)
	---------------- Items ------------------------


			Data Package Item Format

	---------------- Header -----------------------
	Flags (CVLI)
	Total Size (Description + Data, except Header) (CVLI)
	Flags & Data ? Description Size (CVLI)
	Flags & Data ? SHA-256 Hash (32 Bytes)
	----------------Description -------------------
	Description (Serialized Json)
	----------------- Data ------------------------
	Flags & Data ? Data

*/

#define FILE_HEADER_MIN_SIZE 256

namespace slib
{

	namespace priv
	{
		namespace data_store
		{

			class SLIB_EXPORT DataPackageFileHeader
			{
			public:
				sl_uint32 version;
				sl_uint32 flags;
				sl_uint64 firstItemPosition;
				sl_uint64 endingPosition;

			public:
				DataPackageFileHeader()
				{
					version = 1;
					flags = 0;
					firstItemPosition = 0;
					endingPosition = 0;
				}

			public:
				sl_bool read(const File& file)
				{
					if (!(file.readUint32(&version))) {
						return sl_false;
					}
					if (version != 1) {
						return sl_false;
					}
					if (!(file.readUint32(&flags))) {
						return sl_false;
					}
					if (!(file.readUint64(&firstItemPosition))) {
						return sl_false;
					}
					if (firstItemPosition < FILE_HEADER_MIN_SIZE) {
						return sl_false;
					}
					if (!(file.readUint64(&endingPosition))) {
						return sl_false;
					}
					if (endingPosition < firstItemPosition) {
						return sl_false;
					}
					return sl_true;
				}

				sl_bool write(const File& file)
				{
					if (!(file.writeUint32(version))) {
						return sl_false;
					}
					if (!(file.writeUint32(flags))) {
						return sl_false;
					}
					if (!(file.writeUint64(firstItemPosition))) {
						return sl_false;
					}
					if (!(file.writeUint64(endingPosition))) {
						return sl_false;
					}
					return sl_true;
				}

			};

			class SLIB_EXPORT DataPackageItemHeader
			{
			public:
				sl_uint32 flags;
				sl_uint64 totalSize;
				sl_uint32 descriptionSize;
				sl_uint8 hash[32];
				Json description;

			public:
				sl_bool read(const File& file)
				{
					if (!(file.readCVLI32(&flags))) {
						return sl_false;
					}
					if (!(file.readCVLI64(&totalSize))) {
						return sl_false;
					}
					if (flags & DataPackageItemFlags::Data) {
						if (!(file.readCVLI32(&descriptionSize))) {
							return sl_false;
						}
						if (file.readFully(hash, sizeof(hash)) != sizeof(hash)) {
							return sl_false;
						}
						if (descriptionSize >= totalSize) {
							return sl_false;
						}
					} else {
						descriptionSize = (sl_uint32)totalSize;
					}
					if (!descriptionSize) {
						return sl_true;
					}
					SLIB_SCOPED_BUFFER(sl_uint8, 1024, header, descriptionSize)
					if (!header) {
						return sl_false;
					}
					if (file.readFully(header, descriptionSize) != descriptionSize) {
						return sl_false;
					}
					DeserializeBuffer buf(header, descriptionSize);
					return description.deserialize(&buf);
				}

			};


			class SLIB_EXPORT DataPackageReaderImpl : public DataPackageReader
			{
			public:
				File m_file;
				DataPackageFileHeader m_header;

			public:
				static Ref<DataPackageReaderImpl> _open(const StringParam& path)
				{
					File file = File::open(path, FileMode::Read | FileMode::ShareReadWrite);
					if (file.isOpened()) {
						DataPackageFileHeader header;
						if (header.read(file)) {
							Ref<DataPackageReaderImpl> ret = new DataPackageReaderImpl;
							if (ret.isNotNull()) {
								ret->m_file = Move(file);
								ret->m_header = header;
								return ret;
							}
						}
					}
					return sl_null;
				}

			public:
				Ref<DataPackageItem> getItemAt(sl_uint64 position) override;

				Ref<DataPackageItem> getFirstItem() override
				{
					if (m_header.firstItemPosition < m_header.endingPosition) {
						return getItemAt(m_header.firstItemPosition);
					}
					return sl_null;
				}

			};


			class SLIB_EXPORT DataPackageItemImpl : public DataPackageItem
			{
			public:
				friend class DataPackageReaderImpl;
				Ref<DataPackageReaderImpl> m_package;
				sl_uint32 m_offsetData;
				sl_uint64 m_next;

			public:
				sl_reg read(sl_uint64 offset, void* buf, sl_size size) override
				{
					if (offset > m_sizeData) {
						return SLIB_IO_ERROR;
					}
					sl_uint64 sizeRemain = m_sizeData - offset;
					if (size > sizeRemain) {
						size = (sl_size)sizeRemain;
					}
					if (!size) {
						return 0;
					}
					File& file = m_package->m_file;
					if (file.seek(m_position + m_offsetData + offset, SeekPosition::Begin)) {
						return file.read(buf, size);
					}
					return SLIB_IO_ERROR;
				}

				Ref<DataPackageItem> getNext() override
				{
					if (m_next) {
						return m_package->getItemAt(m_next);
					}
					return sl_null;
				}

			};

			Ref<DataPackageItem> DataPackageReaderImpl::getItemAt(sl_uint64 offset)
			{
				if (m_file.seek(offset, SeekPosition::Begin)) {
					DataPackageItemHeader header;
					if (header.read(m_file)) {
						Ref<DataPackageItemImpl> ret = new DataPackageItemImpl;
						if (ret.isNotNull()) {
							ret->m_package = this;
							ret->m_position = offset;
							ret->m_flags = header.flags;
							sl_uint64 pos = m_file.getPosition();
							ret->m_offsetData = (sl_uint32)(pos - offset);
							ret->m_sizeData = header.totalSize - header.descriptionSize;
							pos += ret->m_sizeData;
							if (pos < m_header.endingPosition) {
								ret->m_next = pos;
							} else {
								ret->m_next = 0;
							}
							Base::copyMemory(ret->m_hashData, header.hash, sizeof(ret->m_hashData));
							ret->m_desc = Move(header.description);
							return ret;
						}
					}
				}
				return sl_null;
			}


			class SLIB_EXPORT DataPackageWriterImpl : public DataPackageWriter
			{
			public:
				File m_file;
				sl_bool m_flagLockFile;

				DataPackageFileHeader m_headerFile;
				sl_bool m_flagWrittenItemHeader;
				sl_uint64 m_positionItemDataHash;
				sl_uint64 m_positionEndingItem;
				sl_uint64 m_sizeItemData;
				sl_uint64 m_sizeItemDataWritten;
				SHA3_256 m_hasher;

			public:
				DataPackageWriterImpl()
				{
					m_flagWrittenItemHeader = sl_false;
					m_flagLockFile = sl_false;
				}

				~DataPackageWriterImpl()
				{
					if (m_flagLockFile) {
						m_file.unlock(0, 1);
					}
				}

			public:
				static Ref<DataPackageWriterImpl> _open(const StringParam& _path, sl_bool flagLockFile)
				{
					File file = File::open(_path, FileMode::ReadWrite | FileMode::NotTruncate | FileMode::ShareReadWrite);
					if (file.isOpened()) {
						if (flagLockFile) {
							if (!(file.lock(0, 1))) {
								return sl_null;
							}
						}
						if (file.getSize()) {
							DataPackageFileHeader header;
							if (header.read(file)) {
								Ref<DataPackageWriterImpl> writer = new DataPackageWriterImpl;
								if (writer.isNotNull()) {
									writer->m_file = Move(file);
									writer->m_flagLockFile = flagLockFile;
									writer->m_headerFile = header;
									return writer;
								}
							}
						} else {
							Ref<DataPackageWriterImpl> writer = new DataPackageWriterImpl;
							if (writer.isNotNull()) {
								writer->m_file = Move(file);
								writer->m_flagLockFile = flagLockFile;
								writer->m_headerFile.firstItemPosition = FILE_HEADER_MIN_SIZE;
								writer->m_headerFile.endingPosition = FILE_HEADER_MIN_SIZE;
								return writer;
							}
						}
					}
					return sl_null;
				}

				sl_bool writeHeader(sl_uint64 sizeData, const Json& desc) override
				{
					if (m_flagWrittenItemHeader) {
						return sl_false;
					}

					// Write File Header
					if (!(m_file.getSize())) {
						if (!(m_file.seekToBegin())) {
							return sl_false;
						}
						if (!(m_headerFile.write(m_file))) {
							return sl_false;
						}
						if (!(m_file.setSize(m_headerFile.endingPosition))) {
							return sl_false;
						}
					}
					if (!(m_file.seek(m_headerFile.endingPosition, SeekPosition::Begin))) {
						return sl_false;
					}

					// Write Item Header
					Memory memDesc;
					sl_uint32 sizeDesc;
					if (desc.isNotNull()) {
						memDesc = desc.serialize();
						sizeDesc = (sl_uint32)(memDesc.getSize());
					} else {
						sizeDesc = 0;
					}

					sl_uint32 flags = 0;
					if (sizeData) {
						flags |= DataPackageItemFlags::Data;
					}
					if (!(m_file.writeCVLI32(flags))) {
						return sl_false;
					}
					if (!(m_file.writeCVLI64(sizeDesc + sizeData))) {
						return sl_false;
					}
					if (sizeData) {
						if (!(m_file.writeCVLI32(sizeDesc))) {
							return sl_false;
						}
						m_positionItemDataHash = m_file.getPosition();
						sl_uint8 hash[32] = { 0 };
						if (m_file.writeFully(hash, sizeof(hash)) != sizeof(hash)) {
							return sl_false;
						}
						m_hasher.start();
						m_positionEndingItem = m_positionItemDataHash + sizeof(hash) + sizeDesc + sizeData;
					} else {
						m_positionItemDataHash = 0;
						m_positionEndingItem = m_file.getPosition() + sizeDesc + sizeData;
					}
					if (sizeDesc) {
						if (m_file.writeFully(memDesc.getData(), sizeDesc) != sizeDesc) {
							return sl_false;
						}
					}
					m_sizeItemData = sizeData;
					m_sizeItemDataWritten = 0;
					m_flagWrittenItemHeader = sl_true;
					return sl_true;
				}

				sl_bool writeData(const void* data, sl_size size) override
				{
					if (!m_flagWrittenItemHeader) {
						return sl_false;
					}
					if (m_sizeItemDataWritten + size > m_sizeItemData) {
						return sl_false;
					}
					if (m_file.writeFully(data, size) != size) {
						return sl_false;
					}
					m_sizeItemDataWritten += size;
					m_hasher.update(data, size);
					return sl_true;
				}

				sl_bool endItem(void* outHash) override
				{
					char hash[32];
					if (!outHash) {
						outHash = hash;
					}
					if (!m_flagWrittenItemHeader) {
						return sl_false;
					}
					if (m_sizeItemDataWritten != m_sizeItemData) {
						return sl_false;
					}
					if (m_positionItemDataHash) {
						m_hasher.finish(outHash);
						if (!(m_file.seek(m_positionItemDataHash, SeekPosition::Begin))) {
							return sl_false;
						}
						if (m_file.writeFully(outHash, sizeof(hash)) != sizeof(hash)) {
							return sl_false;
						}
					} else {
						Base::zeroMemory(outHash, sizeof(hash));
					}
					if (!(m_file.seek(16, SeekPosition::Begin))) {
						return sl_false;
					}
					if (!(m_file.writeUint64(m_positionEndingItem))) {
						return sl_false;
					}
					m_headerFile.endingPosition = m_positionEndingItem;
					m_flagWrittenItemHeader = sl_false;
					return sl_true;
				}

			};

		}
	}

	using namespace priv::data_store;

	SLIB_DEFINE_OBJECT(DataPackageItem, Object)

	DataPackageItem::DataPackageItem()
	{
		m_position = 0;
		m_sizeData = 0;
	}

	DataPackageItem::~DataPackageItem()
	{
	}

	sl_uint64 DataPackageItem::getPosition()
	{
		return m_position;
	}

	Json DataPackageItem::getDescription()
	{
		return m_desc;
	}

	sl_uint64 DataPackageItem::getDataSize()
	{
		return m_sizeData;
	}

	sl_uint8* DataPackageItem::getDataHash()
	{
		return m_hashData;
	}


	SLIB_DEFINE_OBJECT(DataPackageReader, Object)

	DataPackageReader::DataPackageReader()
	{
	}

	DataPackageReader::~DataPackageReader()
	{
	}


	SLIB_DEFINE_OBJECT(DataPackageWriter, Object)

	DataPackageWriter::DataPackageWriter()
	{
	}

	DataPackageWriter::~DataPackageWriter()
	{
	}

	sl_bool DataPackageWriter::writeHeader(sl_uint64 dataSize)
	{
		return writeHeader(dataSize, Json::null());
	}


	Ref<DataPackageReader> DataPackage::openReader(const StringParam& path)
	{
		return Ref<DataPackageReader>::from(DataPackageReaderImpl::_open(path));
	}

	Ref<DataPackageWriter> DataPackage::openWriter(const StringParam& path, sl_bool flagLockFile)
	{
		return Ref<DataPackageWriter>::from(DataPackageWriterImpl::_open(path, flagLockFile));
	}

	sl_bool DataPackage::deleteItemAt(const StringParam& path, sl_uint64 offset)
	{
		File file = File::open(path, FileMode::ReadWrite | FileMode::NotTruncate | FileMode::ShareReadWrite);
		if (file.isOpened()) {
			if (file.seek(offset, SeekPosition::Begin)) {
				sl_uint8 flags;
				if (file.readUint8(&flags)) {
					flags |= DataPackageItemFlags::Deleted;
					if (file.seek(offset, SeekPosition::Begin)) {
						if (file.writeUint8(flags)) {
							return sl_true;
						}
					}
				}
			}
		}
		return sl_false;
	}


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(DataStoreParam)

	DataStoreParam::DataStoreParam()
	{
	}


	SLIB_DEFINE_OBJECT(DataStoreItem, Object)

	DataStoreItem::DataStoreItem()
	{

	}

	DataStoreItem::~DataStoreItem()
	{
	}

	Json DataStoreItem::getDescription()
	{
		return m_desc;
	}

	sl_uint64 DataStoreItem::getDataSize()
	{
		return m_sizeData;
	}


	SLIB_DEFINE_OBJECT(DataStore, Object)

	DataStore::DataStore()
	{
	}

	DataStore::~DataStore()
	{
	}

	Ref<DataStore> DataStore::open(const DataStoreParam& param)
	{
		return sl_null;
	}
	
}
