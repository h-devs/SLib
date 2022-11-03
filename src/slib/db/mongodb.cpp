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

#include "slib/db/mongodb.h"

#include "slib/math/decimal128.h"
#include "slib/math/json/decimal128.h"
#include "slib/core/safe_static.h"
#include "slib/core/log.h"

#define BSON_STATIC
#define MONGOC_STATIC
#include "mongoc/mongoc.h"

#ifdef SLIB_PLATFORM_IS_WIN32
#pragma comment(lib, "dnsapi.lib")
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "bcrypt.lib")
#pragma comment(lib, "secur32.lib")
#pragma comment(lib, "crypt32.lib")
#endif

namespace slib
{

	namespace priv
	{
		namespace mongodb
		{

			typedef HashMap<String, sl_bool> ExtendedKeyMap;
			static ExtendedKeyMap GetExtendedKeys()
			{
				SLIB_SAFE_LOCAL_STATIC(ExtendedKeyMap, ret, ExtendedKeyMap::create());
				if (SLIB_SAFE_STATIC_CHECK_FREED(ret)) {
					return sl_null;
				}
				if (ret.isEmpty()) {
					MutexLocker lock(ret.getLocker());
					ret.put_NoLock("$binary", sl_true);
					ret.put_NoLock("$date", sl_true);
					ret.put_NoLock("$numberDecimal", sl_true);
					ret.put_NoLock("$numberDouble", sl_true);
					ret.put_NoLock("$numberLong", sl_true);
					ret.put_NoLock("$numberInt", sl_true);
					ret.put_NoLock("$maxKey", sl_true);
					ret.put_NoLock("$minKey", sl_true);
					ret.put_NoLock("$oid", sl_true);
					ret.put_NoLock("$regularExpression", sl_true);
					ret.put_NoLock("$timestamp", sl_true);
				}
				return ret;
			}

			static void AppendJsonToBson(bson_t* bson, const StringData& key, const Json& _json);

			static void AppendBsonDocumentContent(bson_t* bson, const Json& json)
			{
				if (json.isJsonMap()) {
					JsonMap map = json.getJsonMap();
					if (map.isNotNull()) {
						auto node = map.getFirstNode();
						while (node) {
							AppendJsonToBson(bson, node->key, node->value);
							node = node->getNext();
						}
					}
				} else {
					Ref<Object> object = json.getObject();
					if (object.isNotNull()) {
						PropertyIterator iterator = object->getPropertyIterator();
						while (iterator.moveNext()) {
							AppendJsonToBson(bson, iterator.getKey(), iterator.getValue());
						}
					}
				}
			}

			static void AppendBsonArrayContent(bson_t* bson, const Json& json)
			{
				if (json.isJsonList()) {
					JsonList list = json.getJsonList();
					if (list.isNotNull()) {
						Json* data = list.getData();
						sl_uint32 n = (sl_uint32)(list.getCount());
						char buf[16];
						for (sl_uint32 i = 0; i < n; i++) {
							const char* key;
							size_t lenKey = bson_uint32_to_string(i, &key, buf, sizeof(buf));
							AppendJsonToBson(bson, StringData(key, lenKey), data[i]);
						}
					}
				} else {
					Ref<Collection> collection = json.getCollection();
					if (collection.isNotNull()) {
						sl_uint32 n = (sl_uint32)(collection->getElementCount());
						char buf[16];
						for (sl_uint32 i = 0; i < n; i++) {
							const char* key;
							size_t lenKey = bson_uint32_to_string(i, &key, buf, sizeof(buf));
							AppendJsonToBson(bson, StringData(key, lenKey), collection->getElement(i));
						}
					}
				}
			}

#define PTR_VAR(TYPE, x) ((TYPE*)((void*)(&(x))))
#define REF_VAR(TYPE, x) (*PTR_VAR(TYPE, x))

			static void AppendJsonToBson(bson_t* bson, const StringData& key, const Json& json)
			{
				switch (json._type) {
					case VariantType::Boolean:
						bson_append_bool(bson, key.getData(), (int)(key.getLength()), REF_VAR(sl_bool, json._value));
						return;
					case VariantType::Int32:
						bson_append_int32(bson, key.getData(), (int)(key.getLength()), REF_VAR(sl_int32, json._value));
						return;
					case VariantType::Uint32:
						{
							sl_uint32 n = REF_VAR(sl_uint32, json._value);
							if (n & 0x80000000) {
								bson_append_int64(bson, key.getData(), (int)(key.getLength()), n);
							} else {
								bson_append_int32(bson, key.getData(), (int)(key.getLength()), n);
							}
							return;
						}
					case VariantType::Int64:
					case VariantType::Uint64:
						bson_append_int64(bson, key.getData(), (int)(key.getLength()), REF_VAR(sl_int64, json._value));
						return;
					case VariantType::Double:
						bson_append_double(bson, key.getData(), (int)(key.getLength()), REF_VAR(double, json._value));
						return;
					case VariantType::Float:
						bson_append_double(bson, key.getData(), (int)(key.getLength()), REF_VAR(float, json._value));
						return;
					case VariantType::Time:
						bson_append_date_time(bson, key.getData(), (int)(key.getLength()), REF_VAR(Time, json._value).getMillisecondCount());
						return;
					case VariantType::ObjectId:
						bson_append_oid(bson, key.getData(), (int)(key.getLength()), PTR_VAR(bson_oid_t, json._value));
						return;
					case VariantType::Null:
						if (json._value) {
							bson_append_null(bson, key.getData(), (int)(key.getLength()));
						} else {
							bson_append_undefined(bson, key.getData(), (int)(key.getLength()));
						}
						return;
				}

				if (json.isStringType()) {
					String str = json.getString();
					bson_append_utf8(bson, key.getData(), (int)(key.getLength()), str.getData(), (int)(str.getLength()));
					return;
				}
				Memory mem = json.getMemory();
				if (mem.isNotNull()) {
					bson_append_binary(bson, key.getData(), (int)(key.getLength()), BSON_SUBTYPE_BINARY, (uint8_t*)(mem.getData()), (uint32_t)(mem.getSize()));
					return;
				}
				if (json.isCollection()) {
					bson_t child;
					bson_append_array_begin(bson, key.getData(), (int)(key.getLength()), &child);
					AppendBsonArrayContent(&child, json);
					bson_append_array_end(bson, &child);
					return;
				}
				if (json.isObject()) {
					sl_bool flagExtendedKey = sl_false;
					{
						PropertyIterator iterator = json.getItemIterator();
						// only check one item
						if (iterator.moveNext()) {
							if (GetExtendedKeys().find(iterator.getKey())) {
								flagExtendedKey = sl_true;
							}
						}
					}
					if (flagExtendedKey) {
						JsonMap map;
						map.put_NoLock(key, json);
						String strJson = Json(map).toJsonString();
						bson_t child;
						bson_error_t error;
						if (bson_init_from_json(&child, strJson.getData(), strJson.getLength(), &error)) {
							bson_concat(bson, &child);
							bson_destroy(&child);
							return;
						}
					}
					bson_t child;
					bson_append_document_begin(bson, key.getData(), (int)(key.getLength()), &child);
					AppendBsonDocumentContent(&child, json);
					bson_append_document_end(bson, &child);
					return;
				}
			}

			static bson_t* GetBsonFromJson(const Json& json)
			{
				if (json.isStringType()) {
					StringParam param = json.getStringParam();
					StringCstr str(param);
					sl_size len = str.getLength();
					if (len) {
						bson_error_t error;
						return bson_new_from_json((uint8_t*)(str.getData()), len, &error);
					}
				} else {
					bson_t* bson = bson_new();
					if (bson) {
						AppendBsonDocumentContent(bson, json);
						return bson;
					}
				}
				return sl_null;
			}

			static Json GetJsonItemFromBson(bson_iter_t* iter);

			static Json GetJsonFromBsonDocument(bson_iter_t* iter)
			{
				JsonMap ret;
				while (bson_iter_next(iter)) {
					ret.put_NoLock(String(bson_iter_key(iter), bson_iter_key_len(iter)), GetJsonItemFromBson(iter));
				}
				return ret;
			}

			static Json GetJsonFromBsonArray(bson_iter_t* iter)
			{
				JsonList ret;
				while (bson_iter_next(iter)) {
					ret.add_NoLock(GetJsonItemFromBson(iter));
				}
				return ret;
			}

			static Json GetJsonItemFromBson(bson_iter_t* iter)
			{
				bson_type_t type = bson_iter_type(iter);
				switch (type) {
					case BSON_TYPE_UNDEFINED:
						return Json();
					case BSON_TYPE_NULL:
						return sl_null;
					case BSON_TYPE_DOCUMENT:
						{
							bson_iter_t child;
							if (bson_iter_recurse(iter, &child)) {
								return GetJsonFromBsonDocument(&child);
							}
							return Json();
						}
					case BSON_TYPE_ARRAY:
						{
							bson_iter_t child;
							if (bson_iter_recurse(iter, &child)) {
								return GetJsonFromBsonArray(&child);
							}
							return Json();
						}
					case BSON_TYPE_BOOL:
						return bson_iter_bool(iter);
					case BSON_TYPE_INT32:
						return bson_iter_int32(iter);
					case BSON_TYPE_INT64:
						return bson_iter_int64(iter);
					case BSON_TYPE_DOUBLE:
						return bson_iter_double(iter);
					case BSON_TYPE_DATE_TIME:
						return Time::withMilliseconds(bson_iter_date_time(iter));
					case BSON_TYPE_OID:
						return *((ObjectId*)(void*)(bson_iter_oid(iter)));
					case BSON_TYPE_DECIMAL128:
						{
							bson_decimal128_t dec;
							if (bson_iter_decimal128(iter, &dec)) {
								return Decimal128(dec.high, dec.low);
							}
							return sl_null;
						}
					case BSON_TYPE_UTF8:
						{
							uint32_t len = 0;
							const char* sz = bson_iter_utf8(iter, &len);
							return String(sz, len);
						}
					case BSON_TYPE_BINARY:
						{
							const uint8_t* data = sl_null;
							uint32_t size = 0;
							bson_subtype_t subtype;
							bson_iter_binary(iter, &subtype, &size, &data);
							if (subtype == BSON_SUBTYPE_BINARY) {
								return Memory::create(data, size);
							}
							break;
						}
					default:
						break;
				}
				const bson_value_t* value = bson_iter_value(iter);
				if (!value) {
					return Json();
				}
				Json ret;
				bson_t bson;
				bson_init(&bson);
				if (bson_append_value(&bson, "a", 1, value)) {
					size_t size;
					char* sz = bson_as_canonical_extended_json(&bson, &size);
					if (sz) {
						Json json = Json::parse(StringView(sz, size));
						SLIB_STATIC_STRING(a, "a")
						ret = json.getItem(a);
						bson_free(sz);
					}
				}
				bson_destroy(&bson);
				return ret;
			}

			static Json GetJsonFromBson(const bson_t* bson)
			{
				if (!bson) {
					return sl_null;
				}
				bson_iter_t iter;
				bson_iter_init(&iter, bson);
				return GetJsonFromBsonDocument(&iter);
			}

			class MongoDBEnv : public Referable
			{
			public:
				MongoDBEnv()
				{
					mongoc_init();
				}

				~MongoDBEnv()
				{
					mongoc_cleanup();
				}

			};

			SLIB_SAFE_STATIC_GETTER(Ref<MongoDBEnv>, GetEnv, new MongoDBEnv)

			class PoolImpl : public DocumentStorePool
			{
			public:
				mongoc_client_pool_t * m_pool;

			public:
				PoolImpl()
				{
				}

				~PoolImpl()
				{
					mongoc_client_pool_destroy(m_pool);
				}

			public:
				Ref<DocumentStore> getStore() override;

			};

			class MongoDBImpl : public MongoDB
			{
			public:
				Ref<MongoDBEnv> m_env;
				mongoc_client_t* m_client;
				Ref<PoolImpl> m_pool;

			public:
				MongoDBImpl()
				{
				}

				~MongoDBImpl()
				{
					if (m_pool.isNotNull()) {
						mongoc_client_pool_push(m_pool->m_pool, m_client);
					} else {
						mongoc_client_destroy(m_client);
					}
				}

			public:
				static Ref<DocumentStorePool> createPool(const MongoDB_Param& param)
				{
					Ref<MongoDBEnv>* env = GetEnv();
					if (!env) {
						return sl_null;
					}
					if (env->isNull()) {
						return sl_null;
					}
					StringCstr connectionString(param.connectionString);
					mongoc_uri_t *uri = mongoc_uri_new(connectionString.getData());
					if (!uri) {
						return sl_null;
					}
					mongoc_client_pool_t* pool = mongoc_client_pool_new(uri);
					if (pool) {
						Ref<PoolImpl> ret = new PoolImpl;
						if (ret.isNotNull()) {
							ret->m_pool = pool;
							return ret;
						}
						mongoc_client_pool_destroy(pool);
					}
					return sl_null;
				}

				static Ref<MongoDBImpl> connect(const MongoDB_Param& param)
				{
					Ref<MongoDBEnv>* env = GetEnv();
					if (!env) {
						return sl_null;
					}
					if (env->isNull()) {
						return sl_null;
					}
					StringCstr connectionString(param.connectionString);
					mongoc_client_t* client = mongoc_client_new(connectionString.getData());
					if (client) {
						Ref<MongoDBImpl> ret = new MongoDBImpl;
						if (ret.isNotNull()) {
							ret->m_client = client;
							return ret;
						}
						mongoc_client_destroy(client);
					}
					return sl_null;
				}

			public:
				Ref<DocumentDatabase> createDatabase(const StringParam& name) override
				{
					return getDatabase(name);
				}

				Ref<DocumentDatabase> getDatabase(const StringParam& _name) override
				{
					MutexLocker lock(getClientLocker());
					StringCstr name(_name);
					mongoc_database_t* db = mongoc_client_get_database(m_client, name.getData());
					return getDatabase(db);
				}

				Ref<DocumentDatabase> getDefaultDatabase() override
				{
					MutexLocker lock(getClientLocker());
					mongoc_database_t* db = mongoc_client_get_default_database(m_client);
					return getDatabase(db);
				}

				sl_bool dropDatabase(const StringParam& _name) override
				{
					MutexLocker lock(getClientLocker());
					StringCstr name(_name);
					mongoc_database_t* db = mongoc_client_get_database(m_client, name.getData());
					if (db) {
						bson_error_t error;
						return mongoc_database_drop(db, &error);
					}
					return sl_false;
				}

				List<String> getDatabaseNames() override
				{
					MutexLocker lock(getClientLocker());
					bson_error_t error;
					char** names = mongoc_client_get_database_names_with_opts(m_client, sl_null, &error);
					if (names) {
						List<String> ret;
						for (sl_size i = 0; names[i]; i++) {
							ret.add_NoLock(names[i]);
						}
						bson_strfreev(names);
						return ret;
					}
					return sl_null;
				}

			public:
				Ref<DocumentDatabase> getDatabase(mongoc_database_t* db);

				Mutex* getClientLocker()
				{
					return getLocker();
				}

			};

			Ref<DocumentStore> PoolImpl::getStore()
			{
				mongoc_client_t* client = mongoc_client_pool_pop(m_pool);
				if (client) {
					Ref<MongoDBImpl> ret = new MongoDBImpl;
					if (ret.isNotNull()) {
						ret->m_pool = this;
						ret->m_client = client;
						return Ref<DocumentStore>::from(ret);
					}
					mongoc_client_pool_push(m_pool, client);
				}
				return sl_null;
			}

			class DatabaseImpl : public DocumentDatabase
			{
			public:
				Ref<MongoDBImpl> m_client;
				mongoc_database_t* m_db;

			public:
				DatabaseImpl()
				{
				}

				~DatabaseImpl()
				{
					MutexLocker lock(getClientLocker());
					mongoc_database_destroy(m_db);
				}

			public:
				Ref<DocumentStore> getStore() override
				{
					return m_client.get();
				}

				Ref<DocumentCollection> createCollection(const StringParam& _name, const Json& options) override
				{
					bson_t* bsonOptions = sl_null;
					if (options.isNotNull()) {
						bsonOptions = GetBsonFromJson(options);
						if (!bsonOptions) {
							return sl_null;
						}
					}
					StringCstr name(_name);
					MutexLocker lock(getClientLocker());
					bson_error_t error;
					mongoc_collection_t* collection = mongoc_database_create_collection(m_db, name.getData(), bsonOptions, &error);
					if (bsonOptions) {
						bson_destroy(bsonOptions);
					}
					return getCollection(collection);
				}

				Ref<DocumentCollection> getCollection(const StringParam& _name) override
				{
					StringCstr name(_name);
					MutexLocker lock(getClientLocker());
					mongoc_collection_t* collection = mongoc_database_get_collection(m_db, name.getData());
					return getCollection(collection);
				}

				sl_bool dropCollection(const StringParam& _name) override
				{
					StringCstr name(_name);
					MutexLocker lock(getClientLocker());
					mongoc_collection_t* collection = mongoc_database_get_collection(m_db, name.getData());
					if (collection) {
						bson_error_t error;
						return mongoc_collection_drop(collection, &error);
					}
					return sl_false;
				}

				List<String> getCollectionNames() override
				{
					MutexLocker lock(getClientLocker());
					bson_error_t error;
					char** names = mongoc_database_get_collection_names_with_opts(m_db, sl_null, &error);
					if (names) {
						List<String> ret;
						for (sl_size i = 0; names[i]; i++) {
							ret.add_NoLock(names[i]);
						}
						bson_strfreev(names);
						return ret;
					}
					return sl_null;
				}

				sl_bool hasCollection(const StringParam& _name) override
				{
					StringCstr name(_name);
					MutexLocker lock(getClientLocker());
					return mongoc_database_has_collection(m_db, name.getData(), sl_null);
				}

				Json execute(const Json& _command) override
				{
					Json ret;
					bson_t* command = GetBsonFromJson(_command);
					if (command) {
						bson_error_t error;
						bson_t reply;
						bool bRet = mongoc_database_command_simple(m_db, command, sl_null, &reply, &error);
						if (bRet) {
							ret = GetJsonFromBson(&reply);
						} else {
							SLIB_LOG_ERROR("MongoDB", "Execute: %s", String::from(error.message));
						}
						bson_destroy(&reply);
						bson_destroy(command);
					}
					return ret;
				}

			public:
				Ref<DocumentCollection> getCollection(mongoc_collection_t* collection);

				Mutex* getClientLocker()
				{
					return m_client->getClientLocker();
				}

			};

			class CollectionImpl : public DocumentCollection
			{
			public:
				Ref<DatabaseImpl> m_db;
				mongoc_collection_t* m_collection;

			public:
				CollectionImpl()
				{
				}

				~CollectionImpl()
				{
					mongoc_collection_destroy(m_collection);
				}

			public:
				Ref<DocumentDatabase> getDatabase() override
				{
					return m_db.get();
				}

				sl_int64 getDocumentCount(const Json& filter) override
				{
					bson_t* bson = sl_null;
					if (filter.isNotNull()) {
						bson = GetBsonFromJson(filter);
					} else {
						bson = bson_new();
					}
					if (!bson) {
						return -1;
					}
					bson_error_t error;
					int64_t n = mongoc_collection_count_documents(m_collection, bson, sl_null, sl_null, sl_null, &error);
					bson_destroy(bson);
					return n;
				}

				Ref<DocumentCursor> find(const Json& filter, const Json& options) override;

				sl_bool insert(const Json& doc) override
				{
					bson_t* bson = GetBsonFromJson(doc);
					if (bson) {
						bson_error_t error;
						bool bRet = mongoc_collection_insert_one(m_collection, bson, sl_null, sl_null, &error);
						if (!bRet) {
							SLIB_LOG_ERROR("MongoDB", "Insert: %s", String::from(error.message));
						}
						bson_destroy(bson);
						return bRet;
					}
					return sl_false;
				}

				sl_bool replace(const Json& selector, const Json& document, sl_bool flagUpsert) override
				{
					if (selector.isNull()) {
						return sl_false;
					}
					if (document.isNull()) {
						return sl_false;
					}
					sl_bool bRet = sl_false;
					bson_t* bsonSelector = GetBsonFromJson(selector);
					if (bsonSelector) {
						bson_t* bsonDocument = GetBsonFromJson(document);
						if (bsonDocument) {
							bson_t opts;
							bson_init(&opts);
							if (flagUpsert) {
								bson_append_bool(&opts, "upsert", 6, true);
							}
							bson_error_t error;
							if (mongoc_collection_replace_one(m_collection, bsonSelector, bsonDocument, &opts, sl_null, &error)) {
								bRet = sl_true;
							} else {
								SLIB_LOG_ERROR("MongoDB", "%s: %s", flagUpsert ? "Upsert" : "Replace", String::from(error.message));
							}
							bson_destroy(&opts);
							bson_destroy(bsonDocument);
						}
						bson_destroy(bsonSelector);
					}
					return bRet;
				}

				sl_int64 update(const Json& selector, const Json& update) override
				{
					if (selector.isNull()) {
						return -1;
					}
					if (update.isNull()) {
						return -1;
					}
					sl_int64 n = -1;
					bson_t* bsonSelector = GetBsonFromJson(selector);
					if (bsonSelector) {
						bson_t* bsonUpdate = GetBsonFromJson(update);
						if (bsonUpdate) {
							bson_error_t error;
							bson_t reply;
							if (mongoc_collection_update_many(m_collection, bsonSelector, bsonUpdate, sl_null, &reply, &error)) {
								SLIB_STATIC_STRING(strModifiedCount, "modifiedCount")
								n = GetJsonFromBson(&reply).getItem(strModifiedCount).getUint64();
							} else {
								SLIB_LOG_ERROR("MongoDB", "Update: %s", String::from(error.message));
							}
							bson_destroy(&reply);
							bson_destroy(bsonUpdate);
						}
						bson_destroy(bsonSelector);
					}
					return n;
				}

				sl_int64 remove(const Json& filter) override
				{
					if (filter.isNull()) {
						return -1;
					}
					bson_t* bson = GetBsonFromJson(filter);
					if (!bson) {
						return -1;
					}
					sl_int64 n = -1;
					bson_t reply;
					bson_error_t error;
					if (mongoc_collection_delete_many(m_collection, bson, sl_null, &reply, &error)) {
						SLIB_STATIC_STRING(strDeletedCount, "deletedCount")
						n = GetJsonFromBson(&reply).getItem(strDeletedCount).getUint64();
					} else {
						SLIB_LOG_ERROR("MongoDB", "Remove: %s", String::from(error.message));
					}
					bson_destroy(&reply);
					bson_destroy(bson);
					return n;
				}

				Ref<DocumentCursor> aggregate(const Json& pipeline, const Json& options) override;

			public:
				Mutex* getClientLocker()
				{
					return m_db->getClientLocker();
				}

			};

			class CursorImpl : public DocumentCursor
			{
			public:
				Ref<CollectionImpl> m_collection;
				mongoc_cursor_t* m_cursor;

				const bson_t* m_current;

			public:
				CursorImpl()
				{
					m_current = sl_null;
				}

				~CursorImpl()
				{
					mongoc_cursor_destroy(m_cursor);
				}

			public:
				Ref<DocumentCollection> getCollection() override
				{
					return m_collection.get();
				}

				sl_bool moveNext() override
				{
					return mongoc_cursor_next(m_cursor, &m_current);
				}

				Json getDocument() override
				{
					return GetJsonFromBson(m_current);
				}

			};

			Ref<DocumentCursor> CollectionImpl::find(const Json& filter, const Json& options)
			{
				bson_t* bsonFilter = sl_null;
				if (filter.isNotNull()) {
					bsonFilter = GetBsonFromJson(filter);
				} else {
					bsonFilter = bson_new();
				}
				if (!bsonFilter) {
					return sl_null;
				}
				bson_t* bsonOptions = sl_null;
				if (options.isNotNull()) {
					bsonOptions = GetBsonFromJson(options);
					if (!bsonOptions) {
						bson_destroy(bsonFilter);
						return sl_null;
					}
				}
				Ref<CursorImpl> ret;
				mongoc_cursor_t* cursor = mongoc_collection_find_with_opts(m_collection, bsonFilter, bsonOptions, sl_null);
				if (cursor) {
					ret = new CursorImpl;
					if (ret.isNotNull()) {
						ret->m_collection = this;
						ret->m_cursor = cursor;
					} else {
						mongoc_cursor_destroy(cursor);
					}
				}
				bson_destroy(bsonFilter);
				if (bsonOptions) {
					bson_destroy(bsonOptions);
				}
				return ret;
			}

			Ref<DocumentCursor> CollectionImpl::aggregate(const Json& pipeline, const Json& options)
			{
				if (pipeline.isNull()) {
					return sl_null;
				}
				bson_t* bsonPipeline = GetBsonFromJson(pipeline);
				if (!bsonPipeline) {
					return sl_null;
				}
				bson_t* bsonOptions = sl_null;
				if (options.isNotNull()) {
					bsonOptions = GetBsonFromJson(options);
					if (!bsonOptions) {
						bson_destroy(bsonPipeline);
						return sl_null;
					}
				}
				Ref<CursorImpl> ret;
				mongoc_cursor_t* cursor = mongoc_collection_aggregate(m_collection, MONGOC_QUERY_NONE, bsonPipeline, bsonOptions, sl_null);
				if (cursor) {
					ret = new CursorImpl;
					if (ret.isNotNull()) {
						ret->m_collection = this;
						ret->m_cursor = cursor;
					} else {
						mongoc_cursor_destroy(cursor);
					}
				}
				bson_destroy(bsonPipeline);
				if (bsonOptions) {
					bson_destroy(bsonOptions);
				}
				return ret;
			}

			Ref<DocumentCollection> DatabaseImpl::getCollection(mongoc_collection_t* collection)
			{
				if (collection) {
					Ref<CollectionImpl> ret = new CollectionImpl;
					if (ret.isNotNull()) {
						ret->m_db = this;
						ret->m_collection = collection;
						return ret;
					}
					mongoc_collection_destroy(collection);
				}
				return sl_null;
			}

			Ref<DocumentDatabase> MongoDBImpl::getDatabase(mongoc_database_t* db)
			{
				if (db) {
					Ref<DatabaseImpl> ret = new DatabaseImpl;
					if (ret.isNotNull()) {
						ret->m_client = this;
						ret->m_db = db;
						return ret;
					}
					mongoc_database_destroy(db);
				}
				return sl_null;
			}

		}
	}

	using namespace priv::mongodb;


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(MongoDB_Param)

	MongoDB_Param::MongoDB_Param()
	{
	}


	SLIB_DEFINE_OBJECT(MongoDB, DocumentStore)

	MongoDB::MongoDB()
	{
	}

	MongoDB::~MongoDB()
	{
	}

	Ref<DocumentStorePool> MongoDB::createPool(const MongoDB_Param& param)
	{
		return MongoDBImpl::createPool(param);
	}

	Ref<DocumentStorePool> MongoDB::createPool(const StringParam& connectionString)
	{
		MongoDB_Param param;
		param.connectionString = connectionString;
		return createPool(param);
	}

	Ref<MongoDB> MongoDB::connect(const MongoDB_Param& param)
	{
		return Ref<MongoDB>::from(MongoDBImpl::connect(param));
	}

	Ref<MongoDB> MongoDB::connect(const StringParam& connectionString)
	{
		MongoDB_Param param;
		param.connectionString = connectionString;
		return connect(param);
	}

	Ref<DocumentDatabase> MongoDB::connectDatabase(const MongoDB_Param& param)
	{
		Ref<MongoDB> store = connect(param);
		if (store.isNotNull()) {
			return store->getDefaultDatabase();
		}
		return sl_null;
	}

	Ref<DocumentDatabase> MongoDB::connectDatabase(const StringParam& connectionString)
	{
		Ref<MongoDB> store = connect(connectionString);
		if (store.isNotNull()) {
			return store->getDefaultDatabase();
		}
		return sl_null;
	}

	Ref<DocumentCollection> MongoDB::connectCollection(const MongoDB_Param& param, const StringParam& collectionName)
	{
		Ref<MongoDB> store = connect(param);
		if (store.isNotNull()) {
			return store->getCollection(collectionName);
		}
		return sl_null;
	}

	Ref<DocumentCollection> MongoDB::connectCollection(const StringParam& connectionString, const StringParam& collectionName)
	{
		Ref<MongoDB> store = connect(connectionString);
		if (store.isNotNull()) {
			return store->getCollection(collectionName);
		}
		return sl_null;
	}

	void MongoDB::freeEnvironment()
	{
		Ref<MongoDBEnv>* env = GetEnv();
		if (env) {
			env->setNull();
		}
	}

}
