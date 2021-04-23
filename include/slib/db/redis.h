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

#ifndef CHECKHEADER_SLIB_DB_REDIS
#define CHECKHEADER_SLIB_DB_REDIS

#include "key_value_store.h"

#include "../core/string.h"

namespace slib
{

	class SLIB_EXPORT RedisDatabase : public KeyValueStore
	{
		SLIB_DECLARE_OBJECT

	protected:
		RedisDatabase();

		~RedisDatabase();

	public:
		static Ref<RedisDatabase> connect(const StringParam& ip, sl_uint16 port);
		
	public:
		using KeyValueStore::get;
		sl_bool get(const void* key, sl_size sizeKey, MemoryData* pOutValue = sl_null) override;
		
		virtual Variant execute(const StringParam& command) = 0;

		virtual sl_bool incr(const StringParam& key, sl_int64* pValue) = 0;
		
		sl_int64 incr(const StringParam& key, sl_int64 def = 0);

		virtual sl_bool decr(const StringParam& key, sl_int64* pValue) = 0;
		
		sl_int64 decr(const StringParam& key, sl_int64 def = 0);

		virtual sl_bool incrby(const StringParam& key, sl_int64 n, sl_int64* pValue) = 0;
		
		sl_int64 incrby(const StringParam& key, sl_int64 n, sl_int64 def = 0);
		
		virtual sl_bool decrby(const StringParam& key, sl_int64 n, sl_int64* pValue) = 0;
		
		sl_int64 decrby(const StringParam& key, sl_int64 n, sl_int64 def = 0);
		
		virtual sl_bool llen(const StringParam& key, sl_int64* pValue) = 0;
		
		sl_int64 llen(const StringParam& key);
		
		virtual sl_int64 lpush(const StringParam& key, const Variant& value) = 0;

		virtual sl_int64 rpush(const StringParam& key, const Variant& value) = 0;

		virtual Variant lindex(const StringParam& key, sl_int64 index) = 0;
		
		virtual sl_bool lset(const StringParam& key, sl_int64 index, const Variant& value) = 0;

		virtual sl_bool ltrm(const StringParam& key, sl_int64 start, sl_int64 stop) = 0;
		
		virtual Variant lpop(const StringParam& key) = 0;
		
		virtual Variant rpop(const StringParam& key) = 0;
		
		virtual List<Variant> lrange(const StringParam& key, sl_int64 start = 0, sl_int64 stop = -1);
		
	public:
		sl_bool isLoggingErrors();
		
		void setLoggingErrors(sl_bool flag);

		String getErrorMessage();

	protected:
		void processError(const String& error);

		void clearErrorMessage();

	protected:
		sl_bool m_flagLogErrors;
		AtomicString m_errorMessage;

	};

}

#endif
