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

#include "slib/db/document_store.h"

namespace slib
{

	SLIB_DEFINE_OBJECT(DocumentCursor, Object)

	DocumentCursor::DocumentCursor()
	{
	}

	DocumentCursor::~DocumentCursor()
	{
	}

	Ref<DocumentDatabase> DocumentCursor::getDatabase()
	{
		Ref<DocumentCollection> collection = getCollection();
		if (collection.isNotNull()) {
			return collection->getDatabase();
		}
		return sl_null;
	}

	Ref<DocumentStore> DocumentCursor::getStore()
	{
		Ref<DocumentCollection> collection = getCollection();
		if (collection.isNotNull()) {
			return collection->getStore();
		}
		return sl_null;
	}

	JsonList DocumentCursor::toList()
	{
		JsonList list;
		while (moveNext()) {
			list.add_NoLock(getDocument());
		}
		return list;
	}


	SLIB_DEFINE_OBJECT(DocumentCollection, Object)

	DocumentCollection::DocumentCollection()
	{
	}

	DocumentCollection::~DocumentCollection()
	{
	}

	Ref<DocumentStore> DocumentCollection::getStore()
	{
		Ref<DocumentDatabase> db = getDatabase();
		if (db.isNotNull()) {
			return db->getStore();
		}
		return sl_null;
	}

	sl_int64 DocumentCollection::getDocumentsCount()
	{
		return getDocumentsCount(sl_null);
	}

	Ref<DocumentCursor> DocumentCollection::find(const Json& filter)
	{
		return find(filter, sl_null);
	}

	Ref<DocumentCursor> DocumentCollection::find()
	{
		return find(sl_null, sl_null);
	}

	Ref<DocumentCursor> DocumentCollection::aggregate(const Json& pipeline)
	{
		return aggregate(pipeline, sl_null);
	}


	SLIB_DEFINE_OBJECT(DocumentDatabase, Object)

	DocumentDatabase::DocumentDatabase()
	{
	}

	DocumentDatabase::~DocumentDatabase()
	{
	}

	Ref<DocumentCollection> DocumentDatabase::createCollection(const StringParam& name)
	{
		return createCollection(name, sl_null);
	}

	sl_int64 DocumentDatabase::getDocumentsCount(const StringParam& collectionName, const Json& filter)
	{
		Ref<DocumentCollection> collection = getCollection(collectionName);
		if (collection.isNotNull()) {
			return collection->getDocumentsCount(filter);
		}
		return -1;
	}

	sl_int64 DocumentDatabase::getDocumentsCount(const StringParam& collectionName)
	{
		return getDocumentsCount(collectionName, sl_null);
	}

	Ref<DocumentCursor> DocumentDatabase::find(const StringParam& collectionName, const Json& filter, const Json& options)
	{
		Ref<DocumentCollection> collection = getCollection(collectionName);
		if (collection.isNotNull()) {
			return collection->find(filter, options);
		}
		return sl_null;
	}

	Ref<DocumentCursor> DocumentDatabase::find(const StringParam& collectionName, const Json& filter)
	{
		return find(collectionName, sl_null, sl_null);
	}

	Ref<DocumentCursor> DocumentDatabase::find(const StringParam& collectionName)
	{
		return find(collectionName, sl_null, sl_null);
	}

	sl_bool DocumentDatabase::insert(const StringParam& collectionName, const Json& document)
	{
		Ref<DocumentCollection> collection = getCollection(collectionName);
		if (collection.isNotNull()) {
			return collection->insert(document);
		}
		return sl_false;
	}

	sl_bool DocumentDatabase::replace(const StringParam& collectionName, const Json& selector, const Json& document, sl_bool flagUpsert)
	{
		Ref<DocumentCollection> collection = getCollection(collectionName);
		if (collection.isNotNull()) {
			return collection->replace(selector, document, flagUpsert);
		}
		return sl_false;
	}

	sl_int64 DocumentDatabase::update(const StringParam& collectionName, const Json& selector, const Json& update)
	{
		Ref<DocumentCollection> collection = getCollection(collectionName);
		if (collection.isNotNull()) {
			return collection->update(selector, update);
		}
		return -1;
	}

	sl_int64 DocumentDatabase::remove(const StringParam& collectionName, const Json& filter)
	{
		Ref<DocumentCollection> collection = getCollection(collectionName);
		if (collection.isNotNull()) {
			return collection->remove(filter);
		}
		return -1;
	}

	Ref<DocumentCursor> DocumentDatabase::aggregate(const StringParam& collectionName, const Json& pipeline, const Json& options)
	{
		Ref<DocumentCollection> collection = getCollection(collectionName);
		if (collection.isNotNull()) {
			return collection->aggregate(pipeline, options);
		}
		return sl_null;
	}

	Ref<DocumentCursor> DocumentDatabase::aggregate(const StringParam& collectionName, const Json& pipeline)
	{
		return aggregate(collectionName, pipeline, sl_null);
	}


	SLIB_DEFINE_OBJECT(DocumentStore, Object)

	DocumentStore::DocumentStore()
	{
	}

	DocumentStore::~DocumentStore()
	{
	}

	Ref<DocumentCollection> DocumentStore::getCollection(const StringParam& dbName, const StringParam& collectionName)
	{
		Ref<DocumentDatabase> db = getDatabase(dbName);
		if (db.isNotNull()) {
			return db->getCollection(collectionName);
		}
		return sl_null;
	}

	Ref<DocumentCollection> DocumentStore::getCollection(const StringParam& collectionName)
	{
		Ref<DocumentDatabase> db = getDefaultDatabase();
		if (db.isNotNull()) {
			return db->getCollection(collectionName);
		}
		return sl_null;
	}


	SLIB_DEFINE_OBJECT(DocumentStorePool, Object)

	DocumentStorePool::DocumentStorePool()
	{
	}

	DocumentStorePool::~DocumentStorePool()
	{
	}

	Ref<DocumentDatabase> DocumentStorePool::getDatabase(const StringParam& name)
	{
		Ref<DocumentStore> store = getStore();
		if (store.isNotNull()) {
			return store->getDatabase(name);
		}
		return sl_null;
	}

	Ref<DocumentDatabase> DocumentStorePool::getDefaultDatabase()
	{
		Ref<DocumentStore> store = getStore();
		if (store.isNotNull()) {
			return store->getDefaultDatabase();
		}
		return sl_null;
	}

	List<String> DocumentStorePool::getDatabaseNames()
	{
		Ref<DocumentStore> store = getStore();
		if (store.isNotNull()) {
			return store->getDatabaseNames();
		}
		return sl_null;
	}

	Ref<DocumentCollection> DocumentStorePool::getCollection(const StringParam& dbName, const StringParam& collectionName)
	{
		Ref<DocumentStore> store = getStore();
		if (store.isNotNull()) {
			return store->getCollection(dbName, collectionName);
		}
		return sl_null;
	}

	Ref<DocumentCollection> DocumentStorePool::getCollection(const StringParam& collectionName)
	{
		Ref<DocumentStore> store = getStore();
		if (store.isNotNull()) {
			return store->getCollection(collectionName);
		}
		return sl_null;
	}

}
