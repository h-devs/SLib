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

	enum class DataStoreItemType
	{
		Data = 0,
		List = 1,
		Document = 2
	};

	class SLIB_EXPORT DataStoreParam
	{
	public:
		// Path to root directory
		StringParam path;

	public:
		DataStoreParam();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(DataStoreParam)

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
		// `hash`: SHA3-256 Hash
		virtual Memory getItem(const void* hash, DataStoreItemType* pOutType = sl_null) = 0;

		// `hash`: SHA3-256 Hash
		virtual sl_bool putItem(DataStoreItemType type, const void* hash, const void* data, sl_size size) = 0;

	};

}

#endif