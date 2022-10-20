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

#include "slib/db/key_value_store.h"

#include "slib/core/variant.h"
#include "slib/core/serialize.h"

#define VALUE_BUFFER_SIZE 1024

namespace slib
{

	namespace priv
	{
		namespace kvs
		{

			static Variant DeserializeValue(MemoryData&& mem)
			{
				if (mem.size) {
					sl_char8* buf = (sl_char8*)(mem.data);
					if (*buf) {
						Referable* ref = mem.ref.get();
						if (ref) {
							return String::fromRef(ref, buf, mem.size);
						} else {
							return String(buf, mem.size);
						}
					} else {
						if (mem.size > 1) {
							mem.size--;
							mem.data = buf + 1;
							Variant ret;
							ret.deserialize(Move(mem));
							return ret;
						} else {
							return String::getEmpty();
						}
					}
				} else {
					return sl_null;
				}
			}

		}
	}

	using namespace priv::kvs;


	KeyValueReader::KeyValueReader()
	{
	}

	KeyValueReader::~KeyValueReader()
	{
	}

	sl_bool KeyValueReader::get(const void* key, sl_size sizeKey, MemoryData* pOutValue)
	{
		if (!pOutValue) {
			return get(key, sizeKey, sl_null, 0) >= 0;
		}
		sl_reg n = get(key, sizeKey, pOutValue->data, pOutValue->size);
		if (n < 0) {
			return sl_false;
		}
		if ((sl_size)n <= pOutValue->size) {
			pOutValue->size = n;
			return sl_true;
		}
		Memory mem = Memory::create(n);
		if (mem.isNull()) {
			return sl_false;
		}
		*pOutValue = Move(mem);
		n = get(key, sizeKey, pOutValue->data, n);
		if (n >= 0) {
			pOutValue->size = n;
			return sl_true;
		} else {
			return sl_false;
		}
	}

	sl_reg KeyValueReader::get(const void* key, sl_size sizeKey, void* value, sl_size sizeValue)
	{
		char buf[VALUE_BUFFER_SIZE];
		MemoryData mem(buf, sizeof(buf));
		if (get(key, sizeKey, &mem)) {
			sl_size n = SLIB_MIN(sizeValue, mem.size);
			if (n) {
				Base::copyMemory(value, mem.data, n);
			}
			return mem.size;
		} else {
			return -1;
		}
	}

	Variant KeyValueReader::get(const StringParam& _key)
	{
		StringData key(_key);
		char buf[VALUE_BUFFER_SIZE];
		MemoryData mem(buf, sizeof(buf));
		if (get(key.getUnsafeData(), key.getLength(), &mem)) {
			return DeserializeValue(Move(mem));
		} else {
			return Variant();
		}
	}

	Variant KeyValueReader::deserialize(const MemoryData& data)
	{
		return DeserializeValue(MemoryData(data));
	}

	Variant KeyValueReader::deserialize(MemoryData&& data)
	{
		return DeserializeValue(Move(data));
	}


	KeyValueWriter::KeyValueWriter()
	{
	}

	KeyValueWriter::~KeyValueWriter()
	{
	}

	sl_bool KeyValueWriter::put(const void* key, sl_size sizeKey, const void* value, sl_size sizeValue)
	{
		return put(StringView((sl_char8*)key, sizeKey), StringView((sl_char8*)value, sizeValue));
	}

	sl_bool KeyValueWriter::put(const StringParam& _key, const Variant& value)
	{
		if (value.isUndefined()) {
			return remove(_key);
		} else if (value.isNull()) {
			StringData key(_key);
			return put(key.getUnsafeData(), key.getLength(), sl_null, 0);
		} else if (value.isStringType()) {
			StringData key(_key);
			String str = value.getString();
			if (str.isNotEmpty()) {
				return put(key.getUnsafeData(), key.getLength(), str.getData(), str.getLength());
			} else {
				return put(key.getUnsafeData(), key.getLength(), "", 1);
			}
		} else {
			sl_uint8 buf[1024];
			Memory mem;
			sl_size nWrite = SerializeVariant(value, buf, sizeof(buf), &mem, "", 1);
			Memory temp = value.getMemory();
			if (nWrite) {
				StringData key(_key);
				void* p = buf;
				if (mem.isNotNull()) {
					p = mem.getData();
				}
				//return put(key.getUnsafeData(), key.getLength(), temp.getData(), nWrite); // nWrite가 수상함.
				return put(key.getUnsafeData(), key.getLength(), temp.getData(), temp.getSize());
			}
		}
		return sl_false;
	}

	sl_bool KeyValueWriter::remove(const StringParam& _key)
	{
		StringData key(_key);
		return remove(key.getUnsafeData(), key.getLength());
	}


	SLIB_DEFINE_OBJECT(KeyValueWriteBatch, Object)

	KeyValueWriteBatch::KeyValueWriteBatch(): m_flagClosed(sl_false)
	{
	}

	KeyValueWriteBatch::~KeyValueWriteBatch()
	{
	}

	sl_bool KeyValueWriteBatch::commit()
	{
		ObjectLocker lock(this);
		if (m_flagClosed) {
			return sl_false;
		}
		if (_commit()) {
			m_flagClosed = sl_true;
			return sl_true;
		} else {
			return sl_false;
		}
	}

	void KeyValueWriteBatch::discard()
	{
		ObjectLocker lock(this);
		if (m_flagClosed) {
			return;
		}
		m_flagClosed = sl_true;
		_discard();
	}


	SLIB_DEFINE_ROOT_OBJECT(KeyValueSnapshot)

	KeyValueSnapshot::KeyValueSnapshot()
	{
	}

	KeyValueSnapshot::~KeyValueSnapshot()
	{
	}


	SLIB_DEFINE_OBJECT(KeyValueIterator, CPropertyIterator)

	KeyValueIterator::KeyValueIterator()
	{
	}

	KeyValueIterator::~KeyValueIterator()
	{
	}

	sl_bool KeyValueIterator::getKey(MemoryData* pOutValue)
	{
		if (!pOutValue) {
			return getKey(sl_null, 0) >= 0;
		}
		sl_reg n = getKey(pOutValue->data, pOutValue->size);
		if (n < 0) {
			return sl_false;
		}
		if ((sl_size)n <= pOutValue->size) {
			pOutValue->size = n;
			return sl_true;
		}
		Memory mem = Memory::create(n);
		if (mem.isNull()) {
			return sl_false;
		}
		*pOutValue = Move(mem);
		n = getKey(pOutValue->data, n);
		if (n >= 0) {
			pOutValue->size = n;
			return sl_true;
		} else {
			return sl_false;
		}
	}

	sl_reg KeyValueIterator::getKey(void* value, sl_size sizeValue)
	{
		char buf[VALUE_BUFFER_SIZE];
		MemoryData mem(buf, sizeof(buf));
		if (getKey(&mem)) {
			sl_size n = SLIB_MIN(sizeValue, mem.size);
			if (n) {
				Base::copyMemory(value, mem.data, n);
			}
			return mem.size;
		} else {
			return -1;
		}
	}

	String KeyValueIterator::getKey()
	{
		char buf[VALUE_BUFFER_SIZE];
		MemoryData mem(buf, sizeof(buf));
		if (getKey(&mem)) {
			Referable* ref = mem.ref.get();
			if (ref) {
				return String::fromRef(ref, (sl_char8*)(mem.data), mem.size);
			} else {
				return String((sl_char8*)(mem.data), mem.size);
			}
		} else {
			return sl_null;
		}
	}

	sl_bool KeyValueIterator::getValue(MemoryData* pOutValue)
	{
		if (!pOutValue) {
			return getValue(sl_null, 0) >= 0;
		}
		sl_reg n = getValue(pOutValue->data, pOutValue->size);
		if (n < 0) {
			return sl_false;
		}
		if ((sl_size)n <= pOutValue->size) {
			pOutValue->size = n;
			return sl_true;
		}
		Memory mem = Memory::create(n);
		if (mem.isNull()) {
			return sl_false;
		}
		*pOutValue = Move(mem);
		n = getValue(pOutValue->data, n);
		if (n >= 0) {
			pOutValue->size = n;
			return sl_true;
		} else {
			return sl_false;
		}
	}

	sl_reg KeyValueIterator::getValue(void* value, sl_size sizeValue)
	{
		char buf[VALUE_BUFFER_SIZE];
		MemoryData mem(buf, sizeof(buf));
		if (getValue(&mem)) {
			sl_size n = SLIB_MIN(sizeValue, mem.size);
			if (n) {
				Base::copyMemory(value, mem.data, n);
			}
			return mem.size;
		} else {
			return -1;
		}
	}

	Variant KeyValueIterator::getValue()
	{
		char buf[VALUE_BUFFER_SIZE];
		MemoryData mem(buf, sizeof(buf));
		if (getValue(&mem)) {
			return DeserializeValue(Move(mem));
		} else {
			return Variant();
		}
	}

	sl_bool KeyValueIterator::seek(const String& key)
	{
		return seek(key.getData(), key.getLength());
	}

	sl_bool KeyValueIterator::seek(const StringParam& _key)
	{
		StringData key(_key);
		return seek(key.getUnsafeData(), key.getLength());
	}


	SLIB_DEFINE_OBJECT(KeyValueStore, Object)

	KeyValueStore::KeyValueStore()
	{
	}

	KeyValueStore::~KeyValueStore()
	{
	}

	sl_bool KeyValueStore::compact(const void* from, sl_size sizeFrom, const void* end, sl_size sizeEnd)
	{
		return sl_false;
	}

	sl_bool KeyValueStore::compact()
	{
		return compact(sl_null, 0, sl_null, 0);
	}

	sl_bool KeyValueStore::compactFrom(const void* from, sl_size sizeFrom)
	{
		return compact(from, sizeFrom, sl_null, 0);
	}

	sl_bool KeyValueStore::compactTo(const void* end, sl_size sizeEnd)
	{
		return compact(sl_null, 0, end, sizeEnd);
	}

}
