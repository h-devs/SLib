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

#include "definition.h"

#include "../core/json.h"
#include "../core/flags.h"

namespace slib
{

	SLIB_DEFINE_FLAGS(DataPackageItemFlags, {
		Deleted = 0x1,
		Data = 0x2
	})

	class File;
	class DataPackageReader;

	class SLIB_EXPORT DataPackageItem : public Object
	{
		SLIB_DECLARE_OBJECT

	public:
		DataPackageItem();

		~DataPackageItem();

	public:
		sl_uint64 getPosition();

		Json getDescription();

		sl_uint64 getDataSize();

		// 32 bytes
		sl_uint8* getDataHash();

	public:
		virtual sl_reg read(sl_uint64 offset, void* buf, sl_size size) = 0;

		virtual Ref<DataPackageItem> getNext() = 0;

	protected:
		sl_uint32 m_flags;
		sl_uint64 m_position;
		sl_uint64 m_sizeData;
		sl_uint8 m_hashData[32];
		Json m_desc;

	};

	class SLIB_EXPORT DataPackageReader : public Object
	{
		SLIB_DECLARE_OBJECT

	public:
		DataPackageReader();

		~DataPackageReader();

	public:
		virtual Ref<DataPackageItem> getItemAt(sl_uint64 position) = 0;

		virtual Ref<DataPackageItem> getFirstItem() = 0;

	};

	class SLIB_EXPORT DataPackageWriter : public Object
	{
		SLIB_DECLARE_OBJECT

	public:
		DataPackageWriter();

		~DataPackageWriter();

	public:
		sl_bool writeHeader(sl_uint64 dataSize);

		virtual sl_bool writeHeader(sl_uint64 dataSize, const Json& desc) = 0;

		virtual sl_bool writeData(const void* data, sl_size size) = 0;

		// `outHash`: 32 bytes
		virtual sl_bool endItem(void* outHash = sl_null) = 0;
		
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