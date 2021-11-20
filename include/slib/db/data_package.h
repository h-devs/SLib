#/*
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

#ifndef CHECKHEADER_SLIB_DB_DATA_PACKAGE
#define CHECKHEADER_SLIB_DB_DATA_PACKAGE

#include "data_store.h"

#include "../core/flags.h"

namespace slib
{

	SLIB_DEFINE_FLAGS(DataPackageItemFlags, {
		Deleted = 0x1
	})

	class File;
	class DataPackageReader;

	class SLIB_EXPORT DataPackageItem
	{
	public:
		DataPackageItemFlags flags;
		DataStoreItemType type;
		sl_uint64 position;
		sl_uint64 nextItemPosition;
		sl_uint64 dataPosition;
		sl_uint64 dataSize;
		sl_uint8 dataHash[32]; // SHA3-256

	public:
		DataPackageItem();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(DataPackageItem)

	};

	class SLIB_EXPORT DataPackageReader : public Object
	{
		SLIB_DECLARE_OBJECT

	public:
		DataPackageReader();

		~DataPackageReader();

	public:
		virtual sl_bool getItemAt(sl_uint64 position, DataPackageItem& _out, Memory* outData = sl_null, sl_size sizeRead = 0) = 0;

		sl_bool getFirstItem(DataPackageItem& _out, Memory* outData = sl_null, sl_size sizeRead = 0);

		virtual sl_reg readFile(sl_uint64 offset, void* buf, sl_size size) = 0;

	public:
		// `outId`: 12 Bytes
		virtual void getId(void* outId) = 0;

		virtual Time getCreationTime() = 0;

		virtual Time getModifiedTime() = 0;

		virtual sl_uint64 getFirstItemPosition() = 0;

		virtual sl_uint64 getEndingPosition() = 0;

	};

	class SLIB_EXPORT DataPackageWriteParam
	{
	public:
		DataPackageItemFlags flags;
		DataStoreItemType type;
		sl_uint64 dataSize;

	public:
		DataPackageWriteParam();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(DataPackageWriteParam)

	};

	class SLIB_EXPORT DataPackageWriter : public Object
	{
		SLIB_DECLARE_OBJECT

	public:
		DataPackageWriter();

		~DataPackageWriter();

	public:
		virtual sl_bool writeHeader(const DataPackageWriteParam& param) = 0;

		virtual sl_bool writeData(const void* data, sl_size size) = 0;

		// `outHash`: 32 bytes (SHA3-256)
		virtual sl_bool endItem(void* outHash = sl_null) = 0;

		// `outHash`: 32 bytes (SHA3-256)
		sl_bool writeItem(const DataPackageItemFlags& flags, DataStoreItemType type, const void* data, sl_size size, void* outHash = sl_null);

		// `outHash`: 32 bytes (SHA3-256)
		sl_bool writeItem(DataStoreItemType type, const void* data, sl_size size, void* outHash = sl_null);

	public:
		// `outId`: 12 Bytes
		virtual void getId(void* outId) = 0;
		
	};

	class SLIB_EXPORT DataPackage
	{
	public:
		static Ref<DataPackageReader> openReader(const StringParam& path);
		
		static Ref<DataPackageWriter> openWriter(const StringParam& path, sl_bool flagLockFile = sl_false);

		static sl_bool deleteItemAt(const StringParam& path, sl_uint64 offset);

	};

}

#endif