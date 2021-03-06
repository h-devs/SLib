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

namespace slib
{

	class KeyValueStore;

	class SLIB_EXPORT DataStoreParam
	{
	public:
		// Path to root directory
		String path;

	public:
		DataStoreParam();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(DataStoreParam)

	};

	class SLIB_EXPORT DataStoreItem : public Object
	{
		SLIB_DECLARE_OBJECT

	public:
		DataStoreItem();

		~DataStoreItem();

	public:
		Json getDescription();

		sl_uint64 getDataSize();

	public:
		virtual sl_reg read(sl_uint64 offset, void* buf, sl_size size) = 0;

	protected:
		sl_uint64 m_sizeData;
		Json m_desc;

	};

	class SLIB_EXPORT DataStore : public Object
	{
		SLIB_DECLARE_OBJECT

	public:
		DataStore();

		~DataStore();

	public:
		static Ref<DataStore> open(const DataStoreParam& param);

	public:
		// `hash`: 32 bytes hash
		virtual Ref<DataStoreItem> getItem(const void* hash) = 0;

	};

}

#endif