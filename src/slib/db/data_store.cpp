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
#include "slib/crypto/chacha.h"
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
				DataPackageFileFlags flags;
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
				template <class READER>
				sl_bool read(READER* reader)
				{
					if (!(reader->readUint32(&version))) {
						return sl_false;
					}
					if (version != 1) {
						return sl_false;
					}
					sl_uint32 _flags;
					if (!(reader->readUint32(&_flags))) {
						return sl_false;
					}
					flags = _flags;
					if (reader->readFully(packageId, sizeof(packageId)) != sizeof(packageId)) {
						return sl_false;
					}
					sl_uint64 _creationTime;
					if (!(reader->readUint64(&_creationTime))) {
						return sl_false;
					}
					creationTime = _creationTime;
					if (!(reader->readUint64(&firstItemPosition))) {
						return sl_false;
					}
					if (firstItemPosition < FILE_HEADER_MIN_SIZE) {
						return sl_false;
					}
					if (!(reader->readUint64(&endingPosition))) {
						return sl_false;
					}
					if (endingPosition < firstItemPosition) {
						return sl_false;
					}
					sl_uint64 _modifiedTime;
					if (!(reader->readUint64(&_modifiedTime))) {
						return sl_false;
					}
					modifiedTime = _modifiedTime;
					return sl_true;
				}

				template <class WRITER>
				sl_bool write(WRITER* writer)
				{
					if (!(writer->writeUint32(version))) {
						return sl_false;
					}
					if (!(writer->writeUint32(flags.value))) {
						return sl_false;
					}
					if (writer->writeFully(packageId, sizeof(packageId)) != sizeof(packageId)) {
						return sl_false;
					}
					if (!(writer->writeUint64(creationTime.toInt()))) {
						return sl_false;
					}
					if (!(writer->writeUint64(firstItemPosition))) {
						return sl_false;
					}
					if (!(writer->writeUint64(endingPosition))) {
						return sl_false;
					}
					if (!(writer->writeUint64(modifiedTime.toInt()))) {
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
				template <class READER>
				sl_bool read(READER* reader)
				{
					if (!(reader->readCVLI32(&flags))) {
						return sl_false;
					}
					sl_uint32 _type;
					if (!(reader->readCVLI32(&_type))) {
						return sl_false;
					}
					type = (DataStoreItemType)_type;
					if (!(reader->readCVLI64(&size))) {
						return sl_false;
					}
					if (!size) {
						return sl_false;
					}
					if (reader->readFully(hash, sizeof(hash)) != sizeof(hash)) {
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

				sl_bool m_flagEncrypted;
				ChaCha20_IO m_ioEncrypted;
				sl_uint8 m_maskHash[32];

			public:
				DataPackageReaderImpl(File&& file): m_file(Move(file)) {}

			public:
				static Ref<DataPackageReaderImpl> _open(const DataPackageReaderParam& param)
				{
					File file = File::open(param.path, FileMode::Read | FileMode::ShareReadWrite);
					if (file.isOpened()) {
						DataPackageFileHeader header;
						if (header.read(&file)) {
							if (param.encryptionKey && param.encryptionIV) {
								if (!(header.flags & DataPackageFileFlags::Encrypted)) {
									return sl_null;
								}
							} else {
								if (header.flags & DataPackageFileFlags::Encrypted) {
									return sl_null;
								}
							}
							Ref<DataPackageReaderImpl> ret = new DataPackageReaderImpl(Move(file));
							if (ret.isNotNull()) {
								ret->m_header = header;
								ret->_initialize(param);
								return ret;
							}
						}
					}
					return sl_null;
				}

			public:
				void _initialize(const DataPackageReaderParam& param)
				{
					if (param.encryptionKey && param.encryptionIV) {
						m_flagEncrypted = sl_true;
						m_ioEncrypted.setKey(param.encryptionKey);
						m_ioEncrypted.setIV(param.encryptionIV);
						SHA3_256::hash(param.encryptionKey, 32, m_maskHash);
					} else {
						m_flagEncrypted = sl_false;
					}
				}

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
						if (header.read(&m_file)) {
							sl_uint64 dataPosition = m_file.getPosition();
							_out.position = position;
							_out.flags = header.flags;
							_out.type = header.type;
							_out.dataPosition = dataPosition;
							_out.dataSize = header.size;
							if (m_flagEncrypted) {
								for (sl_uint32 i = 0; i < sizeof(header.hash); i++) {
									_out.dataHash[i] = header.hash[i] ^ m_maskHash[i];
								}
							} else {
								Base::copyMemory(_out.dataHash, header.hash, sizeof(_out.dataHash));
							}
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
								void* data = mem.getData();
								if (m_file.readFully(data, sizeRead) != sizeRead) {
									return sl_false;
								}
								if (m_flagEncrypted) {
									m_ioEncrypted.decrypt(dataPosition, data, data, sizeRead);
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
					if (m_flagEncrypted) {
						sl_reg n = m_file.readAt(offset, buf, size);
						if (n > 0) {
							m_ioEncrypted.decrypt(offset, buf, buf, size);
							return n;
						} else {
							return n;
						}
					} else {
						return m_file.readAt(offset, buf, size);
					}
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

				sl_bool m_flagEncrypted;
				ChaCha20_IO m_ioEncrypted;
				sl_uint8 m_maskHash[32];

			public:
				DataPackageWriterImpl(File&& file): m_file(Move(file))
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
				static Ref<DataPackageWriterImpl> _open(const DataPackageWriterParam& param)
				{
					File file = File::open(param.path, FileMode::ReadWrite | FileMode::NotTruncate | FileMode::ShareReadWrite);
					if (file.isOpened()) {
						if (param.flagLockFile) {
							if (!(file.lock(0, 1))) {
								return sl_null;
							}
						}
						DataPackageFileHeader header;
						if (file.getSize()) {
							if (!(header.read(&file))) {
								return sl_null;
							}
							if (param.encryptionKey && param.encryptionIV) {
								if (!(header.flags & DataPackageFileFlags::Encrypted)) {
									return sl_null;
								}
							} else {
								if (header.flags & DataPackageFileFlags::Encrypted) {
									return sl_null;
								}
							}
						} else {
							header.firstItemPosition = FILE_HEADER_MIN_SIZE;
							header.endingPosition = FILE_HEADER_MIN_SIZE;
							if (param.encryptionKey && param.encryptionIV) {
								header.flags |= DataPackageFileFlags::Encrypted;
							}
						}
						Ref<DataPackageWriterImpl> writer = new DataPackageWriterImpl(Move(file));
						if (writer.isNotNull()) {
							writer->m_headerFile = header;
							writer->_initialize(param);
							return writer;
						}
					}
					return sl_null;
				}

			public:
				void _initialize(const DataPackageWriterParam& param)
				{
					m_flagLockFile = param.flagLockFile;
					if (param.encryptionKey && param.encryptionIV) {
						m_flagEncrypted = sl_true;
						m_ioEncrypted.setKey(param.encryptionKey);
						m_ioEncrypted.setIV(param.encryptionIV);
						SHA3_256::hash(param.encryptionKey, 32, m_maskHash);
					} else {
						m_flagEncrypted = sl_false;
					}
				}

				sl_bool writeHeader(const DataPackageItemFlags& flags, DataStoreItemType type, sl_uint64 dataSize) override
				{
					if (m_flagWrittenItemHeader) {
						return sl_false;
					}
					if (!dataSize) {
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
						if (!(m_headerFile.write(&m_file))) {
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
					if (!(m_file.writeCVLI32((sl_uint32)(flags.value)))) {
						return sl_false;
					}
					if (!(m_file.writeCVLI32((sl_uint32)type))) {
						return sl_false;
					}
					if (!(m_file.writeCVLI64(dataSize))) {
						return sl_false;
					}

					m_positionItemDataHash = m_file.getPosition();
					sl_uint8 hash[32] = { 0 };
					if (m_file.writeFully(hash, sizeof(hash)) != sizeof(hash)) {
						return sl_false;
					}
					m_hasher.start();
					m_positionEndingItem = m_positionItemDataHash + sizeof(hash) + dataSize;

					m_sizeItemData = dataSize;
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
					if (m_flagEncrypted) {
						sl_uint64 position;
						if (!(m_file.getPosition(position))) {
							return sl_false;
						}
						sl_uint8* input = (sl_uint8*)data;
						sl_size sizeRemained = size;
						while (sizeRemained) {
							sl_uint8 buf[4096];
							sl_size n = sizeRemained;
							if (n > sizeof(buf)) {
								n = sizeof(buf);
							}
							m_ioEncrypted.encrypt(position, input, buf, n);
							if (m_file.writeFully(buf, n) != n) {
								return sl_false;
							}
							input += n;
							position += n;
							sizeRemained -= n;
						}
					} else {
						if (m_file.writeFully(data, size) != size) {
							return sl_false;
						}
					}
					m_hasher.update(data, size);
					m_sizeItemDataWritten += size;
					return sl_true;
				}

				sl_bool endItem(void* _outHash) override
				{
					sl_uint8* h = (sl_uint8*)_outHash;
					sl_uint8 _h[32];
					if (!h) {
						h = _h;
					}
					if (!m_flagWrittenItemHeader) {
						return sl_false;
					}
					if (m_sizeItemDataWritten != m_sizeItemData) {
						return sl_false;
					}
					if (m_positionItemDataHash) {
						m_hasher.finish(h);
						if (!(m_file.seek(m_positionItemDataHash, SeekPosition::Begin))) {
							return sl_false;
						}
						if (m_flagEncrypted) {
							sl_uint8 t[32];
							for (sl_size i = 0; i < sizeof(t); i++) {
								t[i] = h[i] ^ m_maskHash[i];
							}
							if (m_file.writeFully(t, sizeof(t)) != sizeof(t)) {
								return sl_false;
							}
						} else {
							if (m_file.writeFully(h, sizeof(_h)) != sizeof(_h)) {
								return sl_false;
							}
						}
					} else {
						Base::zeroMemory(h, sizeof(_h));
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

				sl_uint64 getCurrentPosition() override
				{
					return m_headerFile.endingPosition;
				}

			};

			class PackageReaderLender : public SingleLender< Ref<DataPackageReader> >
			{
			public:
				const void* encryptionKey;
				const void* encryptionIV;

			public:
				PackageReaderLender(const String& path): m_path(path), encryptionKey(sl_null), encryptionIV(sl_null) {}

			public:
				sl_bool create(Ref<DataPackageReader>& reader) override
				{
					DataPackageReaderParam param;
					param.path = m_path;
					param.encryptionKey = encryptionKey;
					param.encryptionIV = encryptionIV;
					reader = DataPackage::openReader(param);
					return reader.isNotNull();
				}

			private:
				String m_path;

			};

			class DataStoreImpl : public DataStore
			{
			public:
				Ref<LevelDB> m_dbHash;
				String m_pathPackage;
				CHashMap<ObjectId, String> m_mapPackagePath;
				CHashMap<ObjectId, PackageReaderLender> m_mapPackageReaders;
				Ref<DataPackageWriter> m_writer;
				Time m_timeCreationWriter;
				
				sl_bool m_flagEncrypted;
				sl_uint8 m_encryptionKey[32];
				sl_uint8 m_encryptionIV[16];
				sl_uint8 m_maskHash[32];

			public:
				static Ref<DataStoreImpl> open(const DataStoreParam& param)
				{
					String pathHash = File::concatPath(param.path, "hash");
					if (!(File::isDirectory(pathHash))) {
						if (!(File::createDirectory(pathHash))) {
							return sl_null;
						}
					}
					String pathPackage = File::concatPath(param.path, "package");
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
							if (ret->_initialize(param)) {
								return ret;
							}
						}
					}
					return sl_null;
				}

			public:
				sl_bool _initialize(const DataStoreParam& param)
				{
					// Check encryption
					{
						ChaCha20_FileEncryptor enc;
						String pathEnc = File::concatPath(param.path, "ENC");
						Memory mem = File::readAllBytes(pathEnc);
						sl_size size = mem.getSize();
						if (size) {
							sl_uint8* data = (sl_uint8*)(mem.getData());
							if (size == 1) {
								if (param.encrytionKey.isNotNull()) {
									return sl_false;
								}
								if (*data != 0) {
									return sl_false;
								}
								m_flagEncrypted = sl_false;
							} else {
								if (param.encrytionKey.isNull()) {
									return sl_false;
								}
								if (size != ChaCha20_FileEncryptor::HeaderSize) {
									return sl_false;
								}
								StringData key(param.encrytionKey);
								if (!(enc.open(data, key.getData(), key.getLength()))) {
									return sl_false;
								}
								m_flagEncrypted = sl_true;
							}
						} else {
							m_flagEncrypted = param.encrytionKey.isNotNull();
							if (m_flagEncrypted) {
								sl_uint8 header[ChaCha20_FileEncryptor::HeaderSize];
								StringData key(param.encrytionKey);
								enc.create(header, key.getData(), key.getLength());
								if (!(File::writeAllBytes(pathEnc, header, sizeof(header)))) {
									return sl_false;
								}
							} else {
								sl_uint8 zero = 0;
								if (!(File::writeAllBytes(pathEnc, &zero, 1))) {
									return sl_false;
								}
							}
						}
						if (m_flagEncrypted) {
							enc.getKey(m_encryptionKey);
							enc.getIV(m_encryptionIV);
							SHA3_256::hash(m_encryptionKey, 32, m_maskHash);
						}
					}
					// enumerate package files
					ListElements<String> files = File::getFiles(m_pathPackage);
					for (sl_size i = 0; i < files.count; i++) {
						String& fileName = files[i];
						if (fileName.endsWith(".pkg")) {
							String path = File::concatPath(m_pathPackage, fileName);
							File file = File::open(path, FileMode::Read | FileMode::ShareReadWrite);
							if (file.isOpened()) {
								DataPackageFileHeader header;
								if (header.read(&file)) {
									m_mapPackagePath.put_NoLock(ObjectId(header.packageId), Move(path));
								}
							}
						}
					}
					return sl_true;
				}

			public:
				Memory getItem(const void* hash, DataStoreItemType* pOutType) override
				{
					sl_uint8 buf[28];
					sl_reg n;
					if (m_flagEncrypted) {
						sl_uint8* h = (sl_uint8*)hash;
						sl_uint8 t[32];
						for (sl_size i = 0; i < sizeof(t); i++) {
							t[i] = h[i] ^ m_maskHash[i];
						}
						n = m_dbHash->get(t, 32, buf, sizeof(buf));
					} else {
						n = m_dbHash->get(hash, 32, buf, sizeof(buf));
					}
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
									sl_uint64 size = MIO::readUint64(buf + 20);
									if (mem.getSize() == size) {
										if (pOutType) {
											*pOutType = item.type;
										}
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
					sl_uint8 buf[28];
					sl_uint8 hashMasked[32];
					if (m_flagEncrypted) {
						sl_uint8* h = (sl_uint8*)hash;
						for (sl_size i = 0; i < sizeof(hashMasked); i++) {
							hashMasked[i] = h[i] ^ m_maskHash[i];
						}
						if (m_dbHash->get(hashMasked, 32, buf, sizeof(buf)) == sizeof(buf)) {
							return MIO::readUint64LE(buf + 20) == size;
						}
					} else {
						if (m_dbHash->get(hash, 32, buf, sizeof(buf)) == sizeof(buf)) {
							return MIO::readUint64LE(buf + 20) == size;
						}
					}
					Time time = Time::now();
					time = Time(time.getYear(), time.getMonth(), 1);
					if (m_writer.isNull() || time != m_timeCreationWriter) {
						String path = File::concatPath(m_pathPackage, String::concat(String::fromInt32(time.getYear()), String::fromInt32(time.getMonth(), 10, 2), ".pkg"));
						DataPackageWriterParam param;
						param.path = path;
						param.flagLockFile = sl_true;
						if (m_flagEncrypted) {
							param.encryptionKey = m_encryptionKey;
							param.encryptionIV = m_encryptionIV;
						}
						m_writer = DataPackage::openWriter(param);
						if (m_writer.isNull()) {
							return sl_false;
						}
						m_timeCreationWriter = time;
					}
					sl_uint8 hashResult[32];
					sl_uint64 position = m_writer->getCurrentPosition();
					if (m_writer->writeItem(type, data, size, hashResult)) {
						if (Base::equalsMemory(hash, hashResult, sizeof(hashResult))) {
							m_writer->getId(buf);
							MIO::writeUint64LE(buf + 12, position);
							MIO::writeUint64LE(buf + 20, size);
							if (m_flagEncrypted) {
								return m_dbHash->put(hashMasked, 32, buf, sizeof(buf));
							} else {
								return m_dbHash->put(hash, 32, buf, sizeof(buf));
							}
						}
					}
					return sl_false;
				}

			private:
				PackageReaderLender* getReaderLender(const ObjectId& packageId)
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
						if (m_flagEncrypted) {
							node->value.encryptionKey = m_encryptionKey;
							node->value.encryptionIV = m_encryptionIV;
						}
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


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(DataPackageReaderParam)

	DataPackageReaderParam::DataPackageReaderParam(): encryptionKey(sl_null), encryptionIV(sl_null)
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


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(DataPackageWriterParam)

	DataPackageWriterParam::DataPackageWriterParam(): flagLockFile(sl_false)
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
		if (writeHeader(flags, type, size)) {
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


	Ref<DataPackageReader> DataPackage::openReader(const DataPackageReaderParam& param)
	{
		return Ref<DataPackageReader>::from(DataPackageReaderImpl::_open(param));
	}

	Ref<DataPackageReader> DataPackage::openReader(const StringParam& path)
	{
		DataPackageReaderParam param;
		param.path = path;
		return openReader(param);
	}

	Ref<DataPackageWriter> DataPackage::openWriter(const DataPackageWriterParam& param)
	{
		return Ref<DataPackageWriter>::from(DataPackageWriterImpl::_open(param));
	}

	Ref<DataPackageWriter> DataPackage::openWriter(const StringParam& path)
	{
		DataPackageWriterParam param;
		param.path = path;
		return openWriter(param);
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
