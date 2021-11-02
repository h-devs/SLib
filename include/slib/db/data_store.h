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

#ifndef CHECKHEADER_SLIB_DB_DATA_STORE
#define CHECKHEADER_SLIB_DB_DATA_STORE

#include "definition.h"

#include "../core/json.h"
#include "../core/file.h"
#include "../crypto/sha2.h"

namespace slib
{

	SLIB_DEFINE_FLAGS(DataPackageItemFlags, {
		Deleted = 0x1,
		Data = 0x2
	})

	class DataPackageReader;

	class SLIB_EXPORT DataPackageItem : public Object
	{
		SLIB_DECLARE_OBJECT

	public:
		DataPackageItem();

		~DataPackageItem();

	public:
		sl_uint64 getPosition();

		sl_uint64 getSize();

		// 32 bytes
		sl_uint8* getHash();

		Ref<DataPackageItem> getNext();

		sl_reg read(sl_uint64 offset, void* buf, sl_size size);

		Json getDescription();

	protected:
		Ref<DataPackageReader> m_package;

		sl_uint32 m_flags;
		sl_uint64 m_position;
		sl_uint32 m_offsetData;
		sl_uint64 m_sizeData;
		sl_uint8 m_hash[32];
		sl_uint64 m_next;
		Json m_desc;

		friend class DataPackageReader;

	};

	class SLIB_EXPORT DataPackageFileHeader
	{
	public:
		sl_uint32 version;
		sl_uint32 flags;
		sl_uint64 firstItemPosition;
		sl_uint64 endingPosition;

	public:
		DataPackageFileHeader();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(DataPackageFileHeader)

	public:
		sl_bool read(const File& file);
		
		sl_bool write(const File& file);

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
		DataPackageItemHeader();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(DataPackageItemHeader)

	public:
		sl_bool read(const File& file);

	};
	
	class SLIB_EXPORT DataPackageReader : public Object
	{
		SLIB_DECLARE_OBJECT

	protected:
		DataPackageReader();

		~DataPackageReader();

	public:
		static Ref<DataPackageReader> open(const StringParam& path);

	public:
		Ref<DataPackageItem> getItemAt(sl_uint64 position);

		Ref<DataPackageItem> getFirstItem();

	protected:
		File m_file;
		DataPackageFileHeader m_header;

		friend class DataPackageItem;

	};

	class SLIB_EXPORT DataPackageWriter : public Object
	{
		SLIB_DECLARE_OBJECT

	protected:
		DataPackageWriter();

		~DataPackageWriter();

	public:
		static Ref<DataPackageWriter> open(const StringParam& path);

		sl_bool writeHeader(sl_uint64 dataSize);

		sl_bool writeHeader(sl_uint64 dataSize, const Json& desc);

		sl_bool writeData(const void* data, sl_size size);

		sl_bool endItem();

		// call after `end()`
		sl_uint8* getItemDataHash();

	protected:
#ifdef SLIB_PLATFORM_IS_UNIX
		String m_path;
#endif
		File m_file;
		DataPackageFileHeader m_headerFile;
		sl_bool m_flagWrittenItemHeader;
		sl_uint64 m_positionItemDataHash;
		sl_uint64 m_positionEndingItem;
		sl_uint64 m_sizeItemData;
		sl_uint64 m_sizeItemDataWritten;
		sl_uint8 m_hashItemData[32];
		SHA256 m_hasher;

	};

}

#endif