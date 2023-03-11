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

#ifndef CHECKHEADER_SLIB_DB_KEY_VALUE_STORE
#define CHECKHEADER_SLIB_DB_KEY_VALUE_STORE

#include "definition.h"

#include "../core/object.h"

namespace slib
{

	class KeyValueStore;
	class KeyValueIterator;
	class MemoryData;

	class SLIB_EXPORT KeyValueReader
	{
	public:
		KeyValueReader();

		~KeyValueReader();

	public:
		virtual sl_bool get(const void* key, sl_size sizeKey, MemoryData* pOutValue = sl_null);

		// returns the required size when `sizeValue` is zero
		virtual sl_reg get(const void* key, sl_size sizeKey, void* value, sl_size sizeValue);

		virtual Variant get(const StringParam& key);

		virtual Ref<KeyValueIterator> getIterator() = 0;

	public:
		static Variant deserialize(const MemoryData& data);
		static Variant deserialize(MemoryData&& data);

	};

	class SLIB_EXPORT KeyValueWriter
	{
	public:
		KeyValueWriter();

		~KeyValueWriter();

	public:
		virtual sl_bool put(const void* key, sl_size sizeKey, const void* value, sl_size sizeValue);

		virtual sl_bool put(const StringParam& key, const Variant& value);

		virtual sl_bool remove(const void* key, sl_size sizeKey) = 0;

		sl_bool remove(const StringParam& key);

	};

	class SLIB_EXPORT KeyValueWriteBatch : public Object, public KeyValueWriter
	{
		SLIB_DECLARE_OBJECT

	public:
		KeyValueWriteBatch();

		~KeyValueWriteBatch();

	public:
		sl_bool commit();

		void discard();

	public:
		virtual sl_bool _commit() = 0;

		virtual void _discard() = 0;

	protected:
		sl_bool m_flagClosed;

	};

	class SLIB_EXPORT KeyValueSnapshot : public CRef, public KeyValueReader
	{
		SLIB_DECLARE_OBJECT

	public:
		KeyValueSnapshot();

		~KeyValueSnapshot();

	};

	class SLIB_EXPORT KeyValueIterator : public CPropertyIterator
	{
		SLIB_DECLARE_OBJECT

	public:
		KeyValueIterator();

		~KeyValueIterator();

	public:
		virtual sl_bool getKey(MemoryData* pOutKey);

		// returns the required size when `sizeValue` is zero
		virtual sl_reg getKey(void* value, sl_size sizeValue);

		String getKey() override;

		virtual sl_bool getValue(MemoryData* pOutValue);

		// returns the required size when `sizeValue` is zero
		virtual sl_reg getValue(void* value, sl_size sizeValue);

		Variant getValue() override;

		virtual sl_bool moveFirst() = 0;

		virtual sl_bool moveLast() = 0;

		virtual sl_bool movePrevious() = 0;

		virtual sl_bool seek(const void* key, sl_size sizeKey) = 0;

		sl_bool seek(const String& key) override;

		sl_bool seek(const StringParam& key);

		template <class T>
		sl_bool seek(const T& key)
		{
			return seek(StringParam(key));
		}

	};

	class SLIB_EXPORT KeyValueStore : public Object, public KeyValueReader, public KeyValueWriter
	{
		SLIB_DECLARE_OBJECT

	public:
		KeyValueStore();

		~KeyValueStore();

	public:
		virtual Ref<KeyValueWriteBatch> createWriteBatch() = 0;

		virtual Ref<KeyValueSnapshot> getSnapshot() = 0;

		virtual sl_bool compact(const void* from, sl_size sizeFrom, const void* end, sl_size sizeEnd);

		sl_bool compact();

		sl_bool compactFrom(const void* from, sl_size sizeFrom);

		sl_bool compactTo(const void* end, sl_size sizeEnd);

	};

}

#endif
