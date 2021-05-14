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

#ifndef CHECKHEADER_SLIB_DB_LMDB
#define CHECKHEADER_SLIB_DB_LMDB

#include "key_value_store.h"

#include "../core/string.h"

namespace slib
{

	class LMDB_Param
	{
	public:
		StringParam path;

		sl_bool flagCreateIfMissing;
		sl_uint32 mode; // The UNIX permissions to set on created files and semaphores

	public:
		LMDB_Param();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(LMDB_Param)

	};

	class SLIB_EXPORT LMDB : public KeyValueStore
	{
		SLIB_DECLARE_OBJECT

	public:
		LMDB();

		~LMDB();

	public:
		typedef LMDB_Param Param;

		static Ref<LMDB> open(const LMDB_Param& param);

		static Ref<LMDB> open(const StringParam& path);

	};

}

#endif
