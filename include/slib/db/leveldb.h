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

#ifndef CHECKHEADER_SLIB_DB_LEVELDB
#define CHECKHEADER_SLIB_DB_LEVELDB

#include "key_value_store.h"

#include "../core/string.h"
#include "../crypto/file_encryption.h"

namespace slib
{

	enum class LevelDB_CompressionType
	{
		// NOTE: do not change the values of existing entries, as these are
		// part of the persistent format on disk.
		None = 0x0,
		Snappy = 0x1
	};

	class LevelDB_Param
	{
	public:
		StringParam path;
		Ref<FileEncryption> encryption;

		sl_bool flagCreateIfMissing;

		sl_uint64 writeBufferSize;
		sl_uint64 blockSize;
		sl_uint64 maxOpenFile;
		sl_uint64 maxFileSize;
		LevelDB_CompressionType compression;

		// output
		String errorText;

	public:
		LevelDB_Param();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(LevelDB_Param)

	};

	class SLIB_EXPORT LevelDB : public KeyValueStore
	{
		SLIB_DECLARE_OBJECT

	public:
		LevelDB();

		~LevelDB();

	public:
		typedef LevelDB_Param Param;

		static Ref<LevelDB> open(LevelDB_Param& param);

		static Ref<LevelDB> open(const StringParam& path);

		static void freeDefaultEnvironment();

	};

}

#endif
