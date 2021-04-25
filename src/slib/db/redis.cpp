/*
 *   Copyright (c) 2008-2018 SLIBIO <https://github.com/SLIBIO>
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

#include "hiredis/hiredis.h"

#include "slib/db/redis.h"

#include "slib/core/system.h"
#include "slib/core/variant.h"
#include "slib/core/log.h"

#define TAG "Redis"

namespace slib
{

	namespace priv
	{
		namespace redis
		{

			class DatabaseImpl : public Redis
			{
			public:
				redisContext* m_context;

				DatabaseImpl()
				{
					m_context = sl_null;
				}

				~DatabaseImpl()
				{
					redisFree(m_context);
				}

			public:
				static Ref<DatabaseImpl> connect(const StringParam& _ip, sl_uint16 port)
				{
					StringCstr ip(_ip);
					redisContext* context = redisConnect(ip.getData(), port);
					if (context) {
						Ref<DatabaseImpl> ret = new DatabaseImpl();
						if (ret.isNotNull()) {
							ret->m_context = context;
							return ret;
						}
						redisFree(context);
					}
					return sl_null;
				}
				
			public:				
				Variant _parseReply(redisReply* reply)
				{
					clearErrorMessage();
					if (reply) {
						switch (reply->type) {
							case REDIS_REPLY_INTEGER:
								return reply->integer;
							case REDIS_REPLY_STRING:
							case REDIS_REPLY_STATUS:
								return String(reply->str);
							case REDIS_REPLY_ARRAY:
								{
									List<Variant> list;
									for (size_t i = 0; i < reply->elements; i++) {
										list.add(_parseReply(reply->element[i]));
									}
									return list;
								}
							case REDIS_REPLY_ERROR:
								processError(reply->str);
								break;
						}
					}
					return Variant();
				}
				
				Variant _processReply(redisReply* reply)
				{
					if (reply) {
						Variant ret = _parseReply(reply);
						freeReplyObject(reply);
						return ret;
					} else {
						processError("Cannot connect to the server");
					}
					return Variant();
				}

				sl_bool _processCheckReply(redisReply* reply, Variant check)
				{
					return _processReply(reply) == check;
				}
				
				sl_bool _processIntReply(redisReply* reply, sl_int64* pValue)
				{
					return _processReply(reply).getInt64(pValue);
				}
				
				Variant get(const StringParam& _key) override
				{
					StringCstr key(_key);
					ObjectLocker lock(this);
					redisReply* reply = (redisReply*)(redisCommand(m_context, "GET %s", key.getData()));
					return _processReply(reply);
				}

				sl_bool put(const StringParam& _key, const Variant& value) override
				{
					StringCstr key(_key);
					StringCstr str = value.getString();
					ObjectLocker lock(this);
					redisReply* reply = (redisReply*)(redisCommand(m_context, "SET %s %s", key.getData(), str.getData()));
					return _processCheckReply(reply, "OK");
				}

				sl_bool remove(const StringParam& _key) override
				{
					StringCstr key(_key);
					ObjectLocker lock(this);
					redisReply* reply = (redisReply*)(redisCommand(m_context, "DEL %s", key.getData()));
					return _processCheckReply(reply, 1);
				}

				Ref<KeyValueWriteBatch> createWriteBatch() override
				{
					return sl_null;
				}
				
				Ref<KeyValueIterator> createIterator() override
				{
					return sl_null;
				}

				Ref<KeyValueSnapshot> createSnapshot() override
				{
					return sl_null;
				}

				Variant execute(const StringParam& command) override
				{
					ObjectLocker lock(this);
					String s = command.toString().replaceAll("%", "%%").toNullTerminated();
					redisReply* reply = (redisReply*)(redisCommand(m_context, s.getData()));
					return _processReply(reply);
				}
				
				sl_bool incr(const StringParam& _key, sl_int64* pValue) override
				{
					StringCstr key(_key);
					ObjectLocker lock(this);
					redisReply* reply = (redisReply*)(redisCommand(m_context, "INCR %s", key.getData()));
					return _processIntReply(reply, pValue);
				}
				
				sl_bool decr(const StringParam& _key, sl_int64* pValue) override
				{
					StringCstr key(_key);
					ObjectLocker lock(this);
					redisReply* reply = (redisReply*)(redisCommand(m_context, "DECR %s", key.getData()));
					return _processIntReply(reply, pValue);
				}
				
				sl_bool incrby(const StringParam& _key, sl_int64 n, sl_int64* pValue) override
				{
					StringCstr key(_key);
					ObjectLocker lock(this);
					redisReply* reply = (redisReply*)(redisCommand(m_context, "INCRBY %s %lld", key.getData(), n));
					return _processIntReply(reply, pValue);
				}
				
				sl_bool decrby(const StringParam& _key, sl_int64 n, sl_int64* pValue) override
				{
					StringCstr key(_key);
					ObjectLocker lock(this);
					redisReply* reply = (redisReply*)(redisCommand(m_context, "DECRBY %s %lld", key.getData(), n));
					return _processIntReply(reply, pValue);
				}
				
				sl_bool llen(const StringParam& _key, sl_int64* pValue) override
				{
					StringCstr key(_key);
					ObjectLocker lock(this);
					redisReply* reply = (redisReply*)(redisCommand(m_context, "LLEN %s", key.getData()));
					return _processIntReply(reply, pValue);
				}
				
				sl_int64 lpush(const StringParam& _key, const Variant& value) override
				{
					StringCstr key(_key);
					StringCstr str = value.getString();
					ObjectLocker lock(this);
					redisReply* reply = (redisReply*)(redisCommand(m_context, "LPUSH %s %s", key.getData(), str.getData()));
					sl_int64 count = 0;
					_processIntReply(reply, &count);
					return count;
				}

				sl_int64 rpush(const StringParam& _key, const Variant& value) override
				{
					StringCstr key(_key);
					StringCstr str = value.getString();
					ObjectLocker lock(this);
					redisReply* reply = (redisReply*)(redisCommand(m_context, "RPUSH %s %s", key.getData(), str.getData()));
					sl_int64 count = 0;
					_processIntReply(reply, &count);
					return count;
				}

				Variant lindex(const StringParam& _key, sl_int64 index) override
				{
					StringCstr key(_key);
					ObjectLocker lock(this);
					redisReply* reply = (redisReply*)(redisCommand(m_context, "LINDEX %s %lld", key.getData(), index));
					return _processReply(reply);
				}
				
				sl_bool lset(const StringParam& _key, sl_int64 index, const Variant& value) override
				{
					StringCstr key(_key);
					StringCstr str = value.getString();
					ObjectLocker lock(this);
					redisReply* reply = (redisReply*)(redisCommand(m_context, "LSET %s %lld %s", key.getData(), index, str.getData()));
					return _processCheckReply(reply, "OK");
				}
				
				sl_bool ltrm(const StringParam& _key, sl_int64 start, sl_int64 stop) override
				{
					StringCstr key(_key);
					ObjectLocker lock(this);
					redisReply* reply = (redisReply*)(redisCommand(m_context, "LSET %s %lld %lld", key.getData(), start, stop));
					return _processCheckReply(reply, "OK");
				}
				
				Variant lpop(const StringParam& _key) override
				{
					StringCstr key(_key);
					ObjectLocker lock(this);
					redisReply* reply = (redisReply*)(redisCommand(m_context, "LPOP %s", key.getData()));
					return _processReply(reply);
				}
				
				Variant rpop(const StringParam& _key) override
				{
					StringCstr key(_key);
					ObjectLocker lock(this);
					redisReply* reply = (redisReply*)(redisCommand(m_context, "RPOP %s", key.getData()));
					return _processReply(reply);
				}
				
				VariantList lrange(const StringParam& _key, sl_int64 start, sl_int64 stop) override
				{
					StringCstr key(_key);
					ObjectLocker lock(this);
					redisReply* reply = (redisReply*)(redisCommand(m_context, "LRANGE %s %lld %lld", key.getData(), start, stop));
					return _processReply(reply).getVariantList();
				}

			};

		}
	}
	
	SLIB_DEFINE_OBJECT(Redis, KeyValueStore)

	Redis::Redis()
	{
		m_flagLogErrors = sl_false;
	}

	Redis::~Redis()
	{
	}
	
	sl_bool Redis::get(const void* key, sl_size sizeKey, MemoryData* pOutValue)
	{
		Variant var = get(StringView((sl_char8*)key, sizeKey));
		if (var.isNotUndefined()) {
			if (pOutValue) {
				String str = var.toString();
				sl_size n = str.getLength();
				if (n <= pOutValue->size) {
					Base::copyMemory(pOutValue->data, str.getData(), n);
					pOutValue->size = n;
				} else {
					Memory mem = str.toMemory();
					if (mem.isNull()) {
						return sl_false;
					}
					*pOutValue = Move(mem);
				}
			}
			return sl_true;
		} else {
			return sl_false;
		}
	}

	sl_int64 Redis::incr(const StringParam& key, sl_int64 def)
	{
		sl_int64 val;
		if (incr(key, &val)) {
			return val;
		}
		return def;
	}

	sl_int64 Redis::decr(const StringParam& key, sl_int64 def)
	{
		sl_int64 val;
		if (decr(key, &val)) {
			return val;
		}
		return def;
	}

	sl_int64 Redis::incrby(const StringParam& key, sl_int64 n, sl_int64 def)
	{
		sl_int64 val;
		if (incrby(key, n, &val)) {
			return val;
		}
		return def;
	}
	
	sl_int64 Redis::decrby(const StringParam& key, sl_int64 n, sl_int64 def)
	{
		sl_int64 val;
		if (decrby(key, n, &val)) {
			return val;
		}
		return def;
	}

	sl_int64 Redis::llen(const StringParam& key)
	{
		sl_int64 val;
		if (llen(key, &val)) {
			return val;
		}
		return 0;
	}
	
	sl_bool Redis::isLoggingErrors()
	{
		return m_flagLogErrors;
	}
	
	void Redis::setLoggingErrors(sl_bool flag)
	{
		m_flagLogErrors = flag;
	}

	String Redis::getErrorMessage()
	{
		return m_errorMessage;
	}
	
	void Redis::processError(const String& error)
	{
		m_errorMessage = error;
		if (m_flagLogErrors) {
			LogError(TAG, "%s", error);
		}
	}

	void Redis::clearErrorMessage()
	{
		m_errorMessage.setNull();
	}

	Ref<Redis> Redis::connect(const StringParam& ip, sl_uint16 port)
	{
		return priv::redis::DatabaseImpl::connect(ip, port);
	}

}
