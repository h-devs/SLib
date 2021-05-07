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

#ifndef CHECKHEADER_SLIB_DB_MONGODB
#define CHECKHEADER_SLIB_DB_MONGODB

#include "document_store.h"

#include "../core/string.h"

namespace slib
{

	class MongoDB_Param
	{
	public:
		StringParam connectionString;

	public:
		MongoDB_Param();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(MongoDB_Param)

	};

	class MongoDB : public DocumentStore
	{
		SLIB_DECLARE_OBJECT

	public:
		MongoDB();

		~MongoDB();

	public:
		typedef MongoDB_Param Param;

		static Ref<DocumentStorePool> createPool(const MongoDB_Param& param);

		static Ref<DocumentStorePool> createPool(const StringParam& connectionString);

		static Ref<MongoDB> connect(const MongoDB_Param& param);

		static Ref<MongoDB> connect(const StringParam& connectionString);

		static Ref<DocumentDatabase> connectDatabase(const MongoDB_Param& param);

		static Ref<DocumentDatabase> connectDatabase(const StringParam& connectionString);

		static Ref<DocumentCollection> connectCollection(const MongoDB_Param& param, const StringParam& collectionName);

		static Ref<DocumentCollection> connectCollection(const StringParam& connectionString, const StringParam& collectionName);

		static void freeEnvironment();

	};

}

#endif
