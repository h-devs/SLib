/*
 *   Copyright (c) 2008-2023 SLIBIO <https://github.com/SLIBIO>
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

#define SLIB_SUPPORT_STD_TYPES

#include "slib/db/leveldb.h"

#include "slib/io/file.h"
#include "slib/math/math.h"
#include "slib/core/memory.h"
#include "slib/core/mio.h"
#include "slib/core/unique_ptr.h"
#include "slib/core/safe_static.h"

#include "leveldb/db.h"
#include "leveldb/write_batch.h"
#include "leveldb/env.h"

namespace slib
{

	namespace {

		class StdStringContainer : public CRef
		{
		public:
			std::string string;

		public:
			StdStringContainer(std::string&& other): string(Move(other)) {}

		};

		static sl_bool DecodeValue(std::string&& str, MemoryData* value)
		{
			sl_size size = (sl_size)(str.size());
			if (!size) {
				value->size = 0;
				return sl_true;
			}
			if (size <= value->size) {
				Base::copyMemory(value->data, str.data(), size);
				value->size = size;
				return sl_true;
			} else {
				StdStringContainer* container = new StdStringContainer(Move(str));
				if (container) {
					value->data = (void*)(container->string.data());
					value->ref = container;
					value->size = size;
					return sl_true;
				}
			}
			return sl_false;
		}

		class DefaultEnvironmentManager : public CRef
		{
		public:
			DefaultEnvironmentManager()
			{
			}

			~DefaultEnvironmentManager()
			{
#ifdef SLIB_PLATFORM_IS_WIN32
				leveldb::Env* env = leveldb::Env::Default();
				if (env) {
					env->~Env();
				}
#endif
			}

		};

		SLIB_SAFE_STATIC_GETTER(Ref<DefaultEnvironmentManager>, GetDefaultEnironmentManager, new DefaultEnvironmentManager)

		class EncryptionSequentialFile : public leveldb::SequentialFile
		{
		public:
			UniquePtr<leveldb::SequentialFile> m_base;
			Ref<FileEncryption> m_enc;
			sl_uint64 m_offset;
			sl_uint64 m_iv;

		public:
			EncryptionSequentialFile(leveldb::SequentialFile* base, const Ref<FileEncryption>& enc): m_base(base), m_enc(enc), m_offset(0) {}

		public:
			sl_bool init()
			{
				char iv[8];
				leveldb::Slice slice;
				if ((m_base->Read(sizeof(iv), &slice, iv)).ok()) {
					if (slice.size() == sizeof(iv)) {
						m_iv = MIO::readUint64LE(iv);
						return sl_true;
					}
				}
				return sl_false;
			}

			leveldb::Status Read(size_t n, leveldb::Slice* result, char* scratch) override
			{
				leveldb::Status status = m_base->Read(n, result, scratch);
				if (status.ok()) {
					const char* data = result->data();
					sl_size size = result->size();
					m_enc->decrypt(m_iv + m_offset, data, scratch, size);
					m_offset += size;
					if (data != scratch) {
						*result = leveldb::Slice(scratch, size);
					}
				}
				return status;
			}

			leveldb::Status Skip(uint64_t n) override
			{
				leveldb::Status status = m_base->Skip(n);
				if (status.ok()) {
					m_offset += n;
				}
				return status;
			}
		};

		class EncryptionRandomAccessFile : public leveldb::RandomAccessFile
		{
		public:
			UniquePtr<leveldb::RandomAccessFile> m_base;
			Ref<FileEncryption> m_enc;
			sl_uint64 m_iv;

		public:
			EncryptionRandomAccessFile(leveldb::RandomAccessFile* base, const Ref<FileEncryption>& enc): m_base(base), m_enc(enc) {}

		public:
			sl_bool init()
			{
				char iv[8];
				leveldb::Slice slice;
				if ((m_base->Read(0, sizeof(iv), &slice, iv)).ok()) {
					if (slice.size() == sizeof(iv)) {
						m_iv = MIO::readUint64LE(iv);
						return sl_true;
					}
				}
				return sl_false;
			}

			leveldb::Status Read(uint64_t offset, size_t n, leveldb::Slice* result, char* scratch) const override
			{
				leveldb::Status status = m_base->Read(8 + offset, n, result, scratch);
				if (status.ok()) {
					const char* data = result->data();
					sl_size size = result->size();
					m_enc->decrypt(m_iv + offset, data, scratch, size);
					if (data != scratch) {
						*result = leveldb::Slice(scratch, size);
					}
				}
				return status;
			}

		};

		class EncryptionWritableFile : public leveldb::WritableFile
		{
		public:
			UniquePtr<leveldb::WritableFile> m_base;
			Ref<FileEncryption> m_enc;
			sl_uint64 m_iv;
			sl_uint64 m_offset;

		public:
			EncryptionWritableFile(leveldb::WritableFile* base, const Ref<FileEncryption>& enc) : m_base(base), m_enc(enc), m_offset(0) {}

		public:
			sl_bool init()
			{
				char iv[8];
				Math::randomMemory(iv, sizeof(iv));
				m_iv = MIO::readUint64LE(iv);
				return m_base->Append(leveldb::Slice(iv, sizeof(iv))).ok();
			}

			void init(sl_uint64 iv)
			{
				m_iv = iv;
			}

			leveldb::Status Append(const leveldb::Slice& slice) override
			{
				char buf[1024];
				sl_size size = slice.size();
				sl_size pos = 0;
				const char* data = slice.data();
				while (pos < size) {
					sl_size n = size - pos;
					if (n > sizeof(buf)) {
						n = sizeof(buf);
					}
					m_enc->encrypt(m_iv + m_offset, data + pos, buf, n);
					leveldb::Status status = m_base->Append(leveldb::Slice(buf, n));
					if (status.ok()) {
						pos += n;
						m_offset += n;
					} else {
						return status;
					}
				}
				return leveldb::Status::OK();
			}

			leveldb::Status Close() override
			{
				return m_base->Close();
			}

			leveldb::Status Flush() override
			{
				return m_base->Flush();
			}

			leveldb::Status Sync() override
			{
				return m_base->Sync();
			}

		};

		class EncryptionEnv : public leveldb::EnvWrapper
		{
		private:
			Ref<FileEncryption> m_enc;

		public:
			EncryptionEnv(const Ref<FileEncryption>& enc): EnvWrapper(leveldb::Env::Default()), m_enc(enc) {}

		public:
			leveldb::Status NewSequentialFile(const std::string& fname, leveldb::SequentialFile** result) override
			{
				leveldb::Status status = target()->NewSequentialFile(fname, result);
				if (status.ok()) {
					EncryptionSequentialFile* file = new EncryptionSequentialFile(*result, m_enc);
					if (file->init()) {
						*result = file;
					} else {
						delete file;
						return status_FailedToReadIV();
					}
				}
				return status;
			}

			leveldb::Status NewRandomAccessFile(const std::string& fname, leveldb::RandomAccessFile** result) override
			{
				leveldb::Status status = target()->NewRandomAccessFile(fname, result);
				if (status.ok()) {
					EncryptionRandomAccessFile* file = new EncryptionRandomAccessFile(*result, m_enc);
					if (file->init()) {
						*result = file;
					} else {
						delete file;
						return status_FailedToReadIV();
					}
				}
				return status;
			}

			leveldb::Status NewWritableFile(const std::string& fname, leveldb::WritableFile** result) override
			{
				leveldb::Status status = target()->NewWritableFile(fname, result);
				if (status.ok()) {
					EncryptionWritableFile* file = new EncryptionWritableFile(*result, m_enc);
					if (file->init()) {
						*result = file;
					} else {
						delete file;
						return status_FailedToWriteIV();
					}
				}
				return status;
			}

			leveldb::Status NewAppendableFile(const std::string& fname, leveldb::WritableFile** result) override
			{
				leveldb::Env* env = target();

				uint64_t size = 0;
				env->GetFileSize(fname, &size);
				if (!size) {
					return NewWritableFile(fname, result);
				}

				char iv[8];
				{
					if (size < sizeof(iv)) {
						return status_FailedToReadIV();
					}
					leveldb::SequentialFile* reader;
					leveldb::Status status = env->NewSequentialFile(fname, &reader);
					if (!(status.ok())) {
						return status;
					}
					leveldb::Slice slice;
					status = reader->Read(sizeof(iv), &slice, iv);
					if (!(status.ok()) || slice.size() != sizeof(iv)) {
						delete reader;
						return status_FailedToReadIV();
					}
					const char* data = slice.data();
					if (iv != data) {
						Base::copyMemory(iv, data, sizeof(iv));
					}
					delete reader;
				}
				leveldb::Status status = env->NewAppendableFile(fname, result);
				if (status.ok()) {
					EncryptionWritableFile* file = new EncryptionWritableFile(*result, m_enc);
					file->init(MIO::readUint64LE(iv));
					*result = file;
				}
				return status;
			}

			leveldb::Status GetFileSize(const std::string& fname, uint64_t* file_size) override
			{
				leveldb::Status status = target()->GetFileSize(fname, file_size);
				if (status.ok()) {
					sl_uint32 headerSize = m_enc->getHeaderSize();
					if (*file_size >= headerSize) {
						*file_size -= headerSize;
					} else {
						*file_size = 0;
						return leveldb::Status::Corruption("Too small encryption header");
					}
				}
				return status;
			}

			leveldb::Status status_FailedToReadIV()
			{
				return leveldb::Status::Corruption("Failed to read IV");
			}

			leveldb::Status status_FailedToWriteIV()
			{
				return leveldb::Status::Corruption("Failed to write IV");
			}

		};

		class LevelDBImpl : public LevelDB
		{
		public:
			leveldb::DB* m_db;
			EncryptionEnv* m_encryptionEnv;

			leveldb::ReadOptions m_optionsRead;
			leveldb::WriteOptions m_optionsWrite;

			Ref<DefaultEnvironmentManager> m_defaultEnvironemtManager;

		public:
			LevelDBImpl(): m_encryptionEnv(sl_null)
			{
			}

			~LevelDBImpl()
			{
				delete m_db;
				if (m_encryptionEnv) {
					delete m_encryptionEnv;
				}
			}

		public:
			static Ref<LevelDBImpl> open(LevelDB_Param& param)
			{
				Ref<DefaultEnvironmentManager>* p = GetDefaultEnironmentManager();
				if (!p) {
					return sl_null;
				}

				StringCstr path(param.path);
				if (path.isEmpty()) {
					SLIB_STATIC_STRING(error, "Empty path")
					param.errorText = error;
					return sl_null;
				}

				leveldb::Options options;
				options.create_if_missing = (bool)(param.flagCreateIfMissing);
				options.write_buffer_size = (size_t)(param.writeBufferSize);
				options.block_size = (size_t)(param.blockSize);
				options.max_open_files = (int)(param.maxOpenFile);
				options.max_file_size = (size_t)(param.maxFileSize);
				options.compression = (leveldb::CompressionType)(param.compression);

				EncryptionEnv* encryptionEnv = sl_null;
				if (param.encryption.isNotNull()) {
					sl_uint32 headerSize = param.encryption->getHeaderSize();
					Memory header = Memory::create(headerSize);
					if (header.isNull()) {
						SLIB_STATIC_STRING(error, "Failed to allocate enryption header")
						param.errorText = error;
						return sl_null;
					}
					do {
						SLIB_STATIC_STRING(filenameENC, "ENC")
						String pathENC = File::concatPath(path, filenameENC);
						File file = File::openForRead(pathENC);
						if (file.isOpened()) {
							if (file.readFully(header.getData(), headerSize) == headerSize) {
								if (param.encryption->open(header.getData())) {
									break;
								} else {
									SLIB_STATIC_STRING(error, "Invalid encryption key")
									param.errorText = error;
									return sl_null;
								}
							}
							SLIB_STATIC_STRING(error, "Failed to read encryption header")
							param.errorText = error;
							return sl_null;
						}
						do {
							SLIB_STATIC_STRING(filenameCURRENT, "CURRENT")
							if (!(File::isFile(File::concatPath(path, filenameCURRENT)))) {
								if (param.flagCreateIfMissing) {
									if (!(File::isDirectory(path))) {
										File::createDirectories(path);
									}
									file = File::openForWrite(pathENC);
									if (file.isOpened()) {
										break;
									}
								}
							}
							SLIB_STATIC_STRING(error, "Failed to open ENC file")
							param.errorText = error;
							return sl_null;
						} while (0);

						param.encryption->generateHeader(header.getData());
						if (!(file.writeFully(header.getData(), headerSize) == headerSize)) {
							SLIB_STATIC_STRING(error, "Failed to write encryption header")
							param.errorText = error;
							return sl_null;
						}
					} while (0);
					encryptionEnv = new EncryptionEnv(param.encryption);
					if (!encryptionEnv) {
						SLIB_STATIC_STRING(error, "Failed to allocate encryption environment")
						param.errorText = error;
						return sl_null;
					}
					options.env = encryptionEnv;
				}

				leveldb::DB* db = sl_null;
				leveldb::Status status = leveldb::DB::Open(options, path.getData(), &db);
				if (status.ok()) {
					Ref<LevelDBImpl> ret = new LevelDBImpl;
					if (ret.isNotNull()) {
						ret->m_db = db;
						ret->m_encryptionEnv = encryptionEnv;
						ret->m_defaultEnvironemtManager = *p;
						return ret;
					} else {
						SLIB_STATIC_STRING(error, "Failed to allocate database object")
						param.errorText = error;
					}
				} else {
					param.errorText = status.ToString();
					delete db;
				}
				if (encryptionEnv) {
					delete encryptionEnv;
				}
				return sl_null;
			}

		public:
			Ref<KeyValueWriteBatch> createWriteBatch() override;

			Ref<KeyValueIterator> getIterator() override;

			Ref<KeyValueSnapshot> getSnapshot() override;

			sl_bool get(const void* key, sl_size sizeKey, MemoryData* value) override
			{
				std::string str;
				leveldb::Status status = m_db->Get(m_optionsRead, leveldb::Slice((char*)key, sizeKey), &str);
				if (status.ok()) {
					return DecodeValue(Move(str), value);
				}
				return sl_false;
			}

			sl_bool put(const void* key, sl_size sizeKey, const void* value, sl_size sizeValue) override
			{
				leveldb::Status status = m_db->Put(m_optionsWrite, leveldb::Slice((char*)key, sizeKey), leveldb::Slice((char*)value, sizeValue));
				return status.ok();
			}

			sl_bool remove(const void* key, sl_size sizeKey) override
			{
				leveldb::Status status = m_db->Delete(m_optionsWrite, leveldb::Slice((char*)key, sizeKey));
				return status.ok();
			}

			sl_bool compact(const void* from, sl_size sizeFrom, const void* end, sl_size sizeEnd) override
			{
				if (from) {
					if (end) {
						leveldb::Slice f((char*)from, sizeFrom);
						leveldb::Slice e((char*)end, sizeEnd);
						m_db->CompactRange(&f, &e);
					} else {
						leveldb::Slice f((char*)from, sizeFrom);
						m_db->CompactRange(&f, sl_null);
					}
				} else {
					if (end) {
						leveldb::Slice e((char*)end, sizeEnd);
						m_db->CompactRange(sl_null, &e);
					} else {
						m_db->CompactRange(sl_null, sl_null);
					}
				}
				return sl_true;
			}

		};

		class LevelDBWriteBatch : public KeyValueWriteBatch
		{
		public:
			Ref<LevelDBImpl> m_instance;
			leveldb::DB* m_db;
			leveldb::WriteBatch m_updates;

		public:
			LevelDBWriteBatch(LevelDBImpl* instance): m_instance(instance)
			{
				m_db = instance->m_db;
			}

			~LevelDBWriteBatch()
			{
				discard();
			}

		public:
			sl_bool put(const void* key, sl_size sizeKey, const void* value, sl_size sizeValue) override
			{
				m_updates.Put(leveldb::Slice((char*)key, sizeKey), leveldb::Slice((char*)value, sizeValue));
				return sl_true;
			}

			sl_bool remove(const void* key, sl_size sizeKey) override
			{
				m_updates.Delete(leveldb::Slice((char*)key, sizeKey));
				return sl_true;
			}

			sl_bool _commit() override
			{
				leveldb::Status status = m_db->Write(m_instance->m_optionsWrite, &m_updates);
				if (status.ok()) {
					m_updates.Clear();
					return sl_true;
				} else {
					return sl_false;
				}
			}

			void _discard() override
			{
				m_updates.Clear();
			}

		};

		Ref<KeyValueWriteBatch> LevelDBImpl::createWriteBatch()
		{
			return new LevelDBWriteBatch(this);
		}

		class LevelDBIterator : public KeyValueIterator
		{
		public:
			Ref<LevelDBImpl> m_instance;
			leveldb::Iterator* m_iterator;

		public:
			LevelDBIterator(LevelDBImpl* instance, leveldb::Iterator* iterator): m_instance(instance), m_iterator(iterator) {}

			~LevelDBIterator()
			{
				delete m_iterator;
			}

		public:
			sl_reg getKey(void* value, sl_size sizeValue) override
			{
				if (m_iterator->Valid()) {
					leveldb::Slice slice = m_iterator->key();
					sl_size sizeRequired = (sl_size)(slice.size());
					sl_size n = SLIB_MIN(sizeRequired, sizeValue);
					if (n) {
						Base::copyMemory(value, slice.data(), n);
					}
					return sizeRequired;
				}
				return -1;
			}

			sl_reg getValue(void* value, sl_size sizeValue) override
			{
				if (m_iterator->Valid()) {
					leveldb::Slice slice = m_iterator->value();
					sl_size sizeRequired = (sl_size)(slice.size());
					sl_size n = SLIB_MIN(sizeRequired, sizeValue);
					if (n) {
						Base::copyMemory(value, slice.data(), n);
					}
					return sizeRequired;
				}
				return -1;
			}

			sl_bool moveFirst() override
			{
				m_iterator->SeekToFirst();
				return m_iterator->Valid();
			}

			sl_bool moveLast() override
			{
				m_iterator->SeekToLast();
				return m_iterator->Valid();
			}

			sl_bool movePrevious() override
			{
				if (m_iterator->Valid()) {
					m_iterator->Prev();
				} else {
					m_iterator->SeekToLast();
				}
				return m_iterator->Valid();
			}

			sl_bool moveNext() override
			{
				if (m_iterator->Valid()) {
					m_iterator->Next();
				} else {
					m_iterator->SeekToFirst();
				}
				return m_iterator->Valid();
			}

			sl_bool seek(const void* key, sl_size sizeKey) override
			{
				m_iterator->Seek(leveldb::Slice((char*)key, sizeKey));
				return m_iterator->Valid();
			}

		};

		Ref<KeyValueIterator> LevelDBImpl::getIterator()
		{
			leveldb::Iterator* iterator = m_db->NewIterator(m_optionsRead);
			if (iterator) {
				Ref<KeyValueIterator> ret = new LevelDBIterator(this, iterator);
				if (ret.isNotNull()) {
					return ret;
				}
				delete iterator;
			}
			return sl_null;
		}

		class LevelDBSnapshot : public KeyValueSnapshot
		{
		public:
			Ref<LevelDBImpl> m_instance;
			leveldb::DB* m_db;
			const leveldb::Snapshot* m_snapshot;
			leveldb::ReadOptions m_optionsRead;

		public:
			LevelDBSnapshot(LevelDBImpl* instance, const leveldb::Snapshot* snapshot): m_instance(instance), m_snapshot(snapshot), m_optionsRead(instance->m_optionsRead)
			{
				m_db = instance->m_db;
				m_optionsRead.snapshot = snapshot;
			}

			~LevelDBSnapshot()
			{
				m_db->ReleaseSnapshot(m_snapshot);
			}

		public:
			sl_bool get(const void* key, sl_size sizeKey, MemoryData* value) override
			{
				std::string str;
				leveldb::Status status = m_db->Get(m_optionsRead, leveldb::Slice((char*)key, sizeKey), &str);
				if (status.ok()) {
					return DecodeValue(Move(str), value);
				}
				return sl_false;
			}

			Ref<KeyValueIterator> getIterator() override
			{
				leveldb::Iterator* iterator = m_db->NewIterator(m_optionsRead);
				if (iterator) {
					Ref<KeyValueIterator> ret = new LevelDBIterator(m_instance.get(), iterator);
					if (ret.isNotNull()) {
						return ret;
					}
					delete iterator;
				}
				return sl_null;
			}

		};

		Ref<KeyValueSnapshot> LevelDBImpl::getSnapshot()
		{
			const leveldb::Snapshot* snapshot = m_db->GetSnapshot();
			if (snapshot) {
				Ref<KeyValueSnapshot> ret = new LevelDBSnapshot(this, snapshot);
				if (ret.isNotNull()) {
					return ret;
				}
				m_db->ReleaseSnapshot(snapshot);
			}
			return sl_null;
		}

	}

	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(LevelDB_Param)

	LevelDB_Param::LevelDB_Param()
	{
		flagCreateIfMissing = sl_true;

		writeBufferSize = 4 * 1024 * 1024;
		blockSize = 4096;
		maxOpenFile = 1000;
		maxFileSize = 2 * 1024 * 1024;

		compression = LevelDB_CompressionType::None;
	}


	SLIB_DEFINE_OBJECT(LevelDB, KeyValueStore)

	LevelDB::LevelDB()
	{
	}

	LevelDB::~LevelDB()
	{
	}

	Ref<LevelDB> LevelDB::open(LevelDB_Param& param)
	{
		return Ref<LevelDB>::from(LevelDBImpl::open(param));
	}

	Ref<LevelDB> LevelDB::open(const StringParam& path)
	{
		LevelDB_Param param;
		param.path = path.toString();
		return open(param);
	}

	void LevelDB::freeDefaultEnvironment()
	{
		Ref<DefaultEnvironmentManager>* p = GetDefaultEnironmentManager();
		if (p) {
			p->setNull();
		}
	}

}
