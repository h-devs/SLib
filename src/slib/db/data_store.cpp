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

#include "slib/db/leveldb.h"
#include "slib/crypto/sha3.h"
#include "slib/core/file.h"
#include "slib/core/lender.h"

/*

			Data Package File Format
	
	---------------- Header -----------------------
	Version (4 bytes)
	Flags (4 bytes)
	Package ID (12 bytes)
	Creation Time (8 Bytes)
	First Item Position (8 Bytes)
	Position After Last Item (8 Bytes)
	Modified Time (8 Bytes)
	---------------- Items ------------------------


			Data Package Item Format

	----------------- Header ----------------------
	Flags (CVLI)
	Type (CVLI)
	Data Size (CVLI)
	SHA3-256 Hash (32 Bytes)
	------------------ Data -----------------------
	Data

*/

#define FILE_HEADER_MIN_SIZE 256

namespace slib
{

	namespace priv
	{
		namespace data_store
		{

			class DataPackageFileHeader
			{
			public:
				sl_uint32 version;
				sl_uint32 flags;
				sl_uint8 packageId[12];
				Time creationTime;
				sl_uint64 firstItemPosition;
				sl_uint64 endingPosition;
				Time modifiedTime;

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
					if (file.readFully(packageId, sizeof(packageId)) != sizeof(packageId)) {
						return sl_false;
					}
					sl_uint64 _creationTime;
					if (!(file.readUint64(&_creationTime))) {
						return sl_false;
					}
					creationTime = _creationTime;
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
					sl_uint64 _modifiedTime;
					if (!(file.readUint64(&_modifiedTime))) {
						return sl_false;
					}
					modifiedTime = _modifiedTime;
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
					if (file.writeFully(packageId, sizeof(packageId)) != sizeof(packageId)) {
						return sl_false;
					}
					if (!(file.writeUint64(creationTime.toInt()))) {
						return sl_false;
					}
					if (!(file.writeUint64(firstItemPosition))) {
						return sl_false;
					}
					if (!(file.writeUint64(endingPosition))) {
						return sl_false;
					}
					if (!(file.writeUint64(modifiedTime.toInt()))) {
						return sl_false;
					}
					return sl_true;
				}

			};

			class DataPackageItemHeader
			{
			public:
				sl_uint32 flags;
				DataStoreItemType type;
				sl_uint64 size;
				sl_uint8 hash[32];

			public:
				sl_bool read(const File& file)
				{
					if (!(file.readCVLI32(&flags))) {
						return sl_false;
					}
					sl_uint32 _type;
					if (!(file.readCVLI32(&_type))) {
						return sl_false;
					}
					type = (DataStoreItemType)_type;
					if (!(file.readCVLI64(&size))) {
						return sl_false;
					}
					if (!size) {
						return sl_false;
					}
					if (file.readFully(hash, sizeof(hash)) != sizeof(hash)) {
						return sl_false;
					}
					return sl_true;
				}

			};


			class DataPackageReaderImpl : public DataPackageReader
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
				sl_bool getItemAt(sl_uint64 position, DataPackageItem& _out, Memory* outData, sl_size sizeRead) override
				{
					if (!position) {
						if (m_header.firstItemPosition >= m_header.endingPosition) {
							return  sl_false;
						}
						position = m_header.firstItemPosition;
					}
					if (m_file.seek(position, SeekPosition::Begin)) {
						DataPackageItemHeader header;
						if (header.read(m_file)) {
							sl_uint64 dataPosition = m_file.getPosition();
							_out.position = position;
							_out.flags = header.flags;
							_out.type = header.type;
							_out.dataPosition = dataPosition;
							_out.dataSize = header.size;
							Base::copyMemory(_out.dataHash, header.hash, sizeof(_out.dataHash));
							if (dataPosition < m_header.endingPosition) {
								_out.nextItemPosition = dataPosition + header.size;
							} else {
								_out.nextItemPosition = 0;
							}
							if (outData) {
								if (sizeRead) {
									if (sizeRead > header.size) {
										sizeRead = (sl_size)(header.size);
									}
								} else {
									sizeRead = SLIB_SIZE_FROM_UINT64(header.size);
								}
								Memory mem = Memory::create(sizeRead);
								if (mem.isNull()) {
									return sl_false;
								}
								if (m_file.readFully(mem.getData(), sizeRead) != sizeRead) {
									return sl_false;
								}
								*outData = Move(mem);
							}
							return sl_true;
						}
					}
					return sl_false;
				}

				sl_reg readFile(sl_uint64 offset, void* buf, sl_size size) override
				{
					return m_file.readAt(offset, buf, size);
				}

				void getId(void* outId) override
				{
					Base::copyMemory(outId, m_header.packageId, sizeof(m_header.packageId));
				}
				
				Time getCreationTime() override
				{
					return m_header.creationTime;
				}
				
				Time getModifiedTime() override
				{
					return m_header.modifiedTime;
				}

				sl_uint64 getFirstItemPosition() override
				{
					return m_header.firstItemPosition;
				}

				sl_uint64 getEndingPosition() override
				{
					return m_header.endingPosition;
				}

			};

			class DataPackageWriterImpl : public DataPackageWriter
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

				sl_bool writeHeader(const DataPackageWriteParam& param) override
				{
					if (m_flagWrittenItemHeader) {
						return sl_false;
					}
					if (!(param.dataSize)) {
						return sl_false;
					}

					// Write File Header
					if (!(m_file.getSize())) {
						if (!(m_file.seekToBegin())) {
							return sl_false;
						}
						m_headerFile.creationTime = m_headerFile.modifiedTime = Time::now();
						ObjectId packageId = ObjectId::generate();
						Base::copyMemory(m_headerFile.packageId, packageId.data, sizeof(packageId));
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
					if (!(m_file.writeCVLI32((sl_uint32)(param.flags)))) {
						return sl_false;
					}
					if (!(m_file.writeCVLI32((sl_uint32)(param.type)))) {
						return sl_false;
					}
					if (!(m_file.writeCVLI64(param.dataSize))) {
						return sl_false;
					}

					m_positionItemDataHash = m_file.getPosition();
					sl_uint8 hash[32] = { 0 };
					if (m_file.writeFully(hash, sizeof(hash)) != sizeof(hash)) {
						return sl_false;
					}
					m_hasher.start();
					m_positionEndingItem = m_positionItemDataHash + sizeof(hash) + param.dataSize;

					m_sizeItemData = param.dataSize;
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
					// Seek to offset of `positionEndingItem`
					if (!(m_file.seek(36, SeekPosition::Begin))) {
						return sl_false;
					}
					if (!(m_file.writeUint64(m_positionEndingItem))) {
						return sl_false;
					}
					Time time = Time::now();
					// Modified Time
					if (!(m_file.writeUint64(time.toInt()))) {
						return sl_false;
					}
					m_headerFile.endingPosition = m_positionEndingItem;
					m_flagWrittenItemHeader = sl_false;
					return sl_true;
				}

				void getId(void* outId) override
				{
					Base::copyMemory(outId, m_headerFile.packageId, sizeof(m_headerFile.packageId));
				}

			};

			class PackageReaderLender : public SingleLender< Ref<DataPackageReader> >
			{
			private:
				String m_path;

			public:
				PackageReaderLender(const String& path): m_path(path) {}

			public:
				sl_bool create(Ref<DataPackageReader>& reader) override
				{
					reader = DataPackage::openReader(m_path);
					return reader.isNotNull();
				}

			};

			class DataStoreImpl : public DataStore
			{
			public:
				Ref<LevelDB> m_dbHash;
				String m_pathPackage;
				CHashMap<ObjectId, String> m_mapPackagePath;
				CHashMap<ObjectId, PackageReaderLender> m_mapPackageReaders;
				Ref<DataPackageWriter> m_writer;

			public:
				static Ref<DataStoreImpl> open(const DataStoreParam& param)
				{
					String pathHash = File::joinPath(param.path, "hash");
					if (!(File::isDirectory(pathHash))) {
						if (!(File::createDirectory(pathHash))) {
							return sl_null;
						}
					}
					String pathPackage = File::joinPath(param.path, "package");
					if (!(File::isDirectory(pathPackage))) {
						if (!(File::createDirectory(pathPackage))) {
							return sl_null;
						}
					}
					LevelDB::Param paramHash;
					paramHash.path = pathHash;
					Ref<LevelDB> dbHash = LevelDB::open(paramHash);
					if (dbHash.isNotNull()) {
						Ref<DataStoreImpl> ret = new DataStoreImpl;
						if (ret.isNotNull()) {
							ret->m_dbHash = Move(dbHash);
							ret->m_pathPackage = Move(pathPackage);
							ret->_initialize(param);
							return ret;
						}
					}
					return sl_null;
				}

			public:
				void _initialize(const DataStoreParam& param)
				{
					// enumerate package files
					ListElements<String> files = File::getFiles(m_pathPackage);
					for (sl_size i = 0; i < files.count; i++) {
						String& fileName = files[i];
						if (fileName.endsWith(".pkg")) {
							String path = File::joinPath(m_pathPackage, fileName);
							File file = File::open(path, FileMode::Read | FileMode::ShareReadWrite);
							if (file.isOpened()) {
								DataPackageFileHeader header;
								if (header.read(file)) {
									m_mapPackagePath.put_NoLock(ObjectId(header.packageId), Move(path));
								}
							}
						}
					}
				}

			public:
				Memory getItem(const void* hash, DataStoreItemType* pOutType) override
				{
					sl_uint8 buf[20];
					sl_reg n = m_dbHash->get(hash, 32, buf, sizeof(buf));
					if (n == sizeof(buf)) {
						PackageReaderLender* lender = getReaderLender(ObjectId(buf));
						if (lender) {
							Borrower< Ref<DataPackageReader>, PackageReaderLender > borrower;
							if (borrower.borrow(lender)) {
								Ref<DataPackageReader>& reader = borrower.value;
								sl_uint64 offset = MIO::readUint64LE(buf + 12);
								DataPackageItem item;
								Memory mem;
								if (reader->getItemAt(offset, item, &mem)) {
									if (pOutType) {
										*pOutType = item.type;
										return mem;
									}
								}
							}
						}
					}
					return sl_null;
				}

				sl_bool putItem(DataStoreItemType type, const void* hash, const void* data, sl_size size) override
				{
					ObjectLocker lock(this);
					return sl_false;
				}

			private:
				Ref<PackageReaderLender> getReaderLender(const ObjectId& packageId)
				{
					ObjectLocker locker(&m_mapPackageReaders);
					PackageReaderLender* lender = m_mapPackageReaders.getItemPointer(packageId);
					if (lender) {
						return lender;
					}
					String path = m_mapPackagePath.getValue(packageId);
					if (path.isNull()) {
						return sl_null;
					}
					auto node = m_mapPackageReaders.put_NoLock(packageId, path);
					if (node) {
						return &(node->value);
					}
					return sl_null;
				}

			};

		}
	}

	using namespace priv::data_store;

	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(DataPackageItem)

	DataPackageItem::DataPackageItem()
	{
	}


	SLIB_DEFINE_OBJECT(DataPackageReader, Object)

	DataPackageReader::DataPackageReader()
	{
	}

	DataPackageReader::~DataPackageReader()
	{
	}

	sl_bool DataPackageReader::getFirstItem(DataPackageItem& _out, Memory* outData, sl_size sizeRead)
	{
		return getItemAt(0, _out, outData, sizeRead);
	}


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(DataPackageWriteParam)

	DataPackageWriteParam::DataPackageWriteParam(): type(DataStoreItemType::Data), dataSize(0)
	{
	}


	SLIB_DEFINE_OBJECT(DataPackageWriter, Object)

	DataPackageWriter::DataPackageWriter()
	{
	}

	DataPackageWriter::~DataPackageWriter()
	{
	}

	sl_bool DataPackageWriter::writeItem(const DataPackageItemFlags& flags, DataStoreItemType type, const void* data, sl_size size, void* outHash)
	{
		DataPackageWriteParam param;
		param.flags = flags;
		param.type = type;
		param.dataSize = size;
		if (writeHeader(param)) {
			if (writeData(data, size)) {
				return endItem(outHash);
			}
		}
		return sl_false;
	}

	sl_bool DataPackageWriter::writeItem(DataStoreItemType type, const void* data, sl_size size, void* outHash)
	{
		return writeItem(0, type, data, size, outHash);
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


	SLIB_DEFINE_OBJECT(DataStore, Object)

	DataStore::DataStore()
	{
	}

	DataStore::~DataStore()
	{
	}

	Ref<DataStore> DataStore::open(const DataStoreParam& param)
	{
		return Ref<DataStore>::from(DataStoreImpl::open(param));
	}
	
}
