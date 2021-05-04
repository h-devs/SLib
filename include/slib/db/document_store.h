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

#ifndef CHECKHEADER_SLIB_DB_DOCUMENT_STORE
#define CHECKHEADER_SLIB_DB_DOCUMENT_STORE

#include "definition.h"

#include "../core/json.h"

namespace slib
{

	class DocumentStore;
	class DocumentDatabase;
	class DocumentCollection;

	class SLIB_EXPORT DocumentCursor : public Object
	{
		SLIB_DECLARE_OBJECT

	public:
		DocumentCursor();

		~DocumentCursor();

	public:
		virtual Ref<DocumentCollection> getCollection() = 0;

		Ref<DocumentDatabase> getDatabase();

		Ref<DocumentStore> getStore();

		virtual sl_bool moveNext() = 0;

		virtual Json getDocument() = 0;

	};

	class SLIB_EXPORT DocumentCollection : public Object
	{
		SLIB_DECLARE_OBJECT

	public:
		DocumentCollection();

		~DocumentCollection();

	public:
		virtual Ref<DocumentDatabase> getDatabase() = 0;

		Ref<DocumentStore> getStore();

		virtual sl_int64 getDocumentsCount(const Json& filter) = 0;

		sl_int64 getDocumentsCount();

		virtual Ref<DocumentCursor> find(const Json& filter, const Json& options) = 0;

		Ref<DocumentCursor> find(const Json& filter);

		Ref<DocumentCursor> find();

		virtual sl_bool insert(const Json& document) = 0;

		virtual sl_bool replace(const Json& selector, const Json& document, sl_bool flagUpsert = sl_false) = 0;

		virtual sl_int64 update(const Json& selector, const Json& update) = 0;

		virtual sl_int64 remove(const Json& filter) = 0;

	public:
		template <class... ARGS>
		Json getFirstDocument(ARGS&&... args)
		{
			Ref<DocumentCursor> cursor = find(Forward<ARGS>(args)...);
			if (cursor.isNotNull()) {
				if (cursor->moveNext()) {
					return cursor->getDocument();
				}
			}
			return sl_null;
		}

		template <class... ARGS>
		JsonList getDocuments(ARGS&&... args)
		{
			Ref<DocumentCursor> cursor = find(Forward<ARGS>(args)...);
			if (cursor.isNotNull()) {
				JsonList list;
				if (cursor->moveNext()) {
					list.add_NoLock(cursor->getDocument());
				}
				return list;
			}
			return sl_null;
		}

	};

	class SLIB_EXPORT DocumentDatabase : public Object
	{
		SLIB_DECLARE_OBJECT

	public:
		DocumentDatabase();

		~DocumentDatabase();

	public:
		virtual Ref<DocumentStore> getStore() = 0;

		virtual Ref<DocumentCollection> createCollection(const StringParam& name) = 0;

		virtual Ref<DocumentCollection> getCollection(const StringParam& name) = 0;

		virtual sl_bool dropCollection(const StringParam& name) = 0;

		virtual List<String> getCollectionNames() = 0;

	public:
		template <class... ARGS>
		Json getFirstDocument(const StringParam& collectionName, ARGS&&... args)
		{
			Ref<DocumentCollection> collection = getCollection(collectionName);
			if (collection.isNotNull()) {
				return collection->getFirstDocument(Forward<ARGS>(args)...);
			}
			return sl_null;
		}

		template <class... ARGS>
		JsonList getDocuments(const StringParam& collectionName, ARGS&&... args)
		{
			Ref<DocumentCollection> collection = getCollection(collectionName);
			if (collection.isNotNull()) {
				return collection->getDocuments(Forward<ARGS>(args)...);
			}
			return sl_null;
		}

	};

	class SLIB_EXPORT DocumentStore : public Object
	{
		SLIB_DECLARE_OBJECT

	public:
		DocumentStore();

		~DocumentStore();

	public:
		virtual Ref<DocumentDatabase> createDatabase(const StringParam& name) = 0;

		virtual Ref<DocumentDatabase> getDatabase(const StringParam& name) = 0;

		virtual Ref<DocumentDatabase> getDefaultDatabase() = 0;

		virtual sl_bool dropDatabase(const StringParam& name) = 0;

		virtual List<String> getDatabaseNames() = 0;

		Ref<DocumentCollection> getCollection(const StringParam& dbName, const StringParam& collectionName);

		Ref<DocumentCollection> getCollection(const StringParam& collectionName);

	};

}

#endif
