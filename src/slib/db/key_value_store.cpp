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

namespace slib
{

	namespace priv
	{
		namespace kvs
		{

			static Variant GetValue(MemoryData& value)
			{
				if (value.size) {
					sl_char8* buf = (sl_char8*)(value.data);
					if (*buf) {
						if (value.ref.isNotNull()) {
							return String::fromRef(value.ref.get(), buf, value.size);
						} else {
							return String(buf, value.size);
						}
					} else {
						if (value.size > 1) {
							value.size--;
							value.data = buf + 1;
							Variant ret;
							ret.deserialize(value);
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

	SLIB_DEFINE_OBJECT(KeyValueWriter, Object)

	KeyValueWriter::KeyValueWriter()
	{
	}

	KeyValueWriter::~KeyValueWriter()
	{
	}

	sl_bool KeyValueWriter::set(const void* key, sl_size sizeKey, const void* value, sl_size sizeValue)
	{
		return set(StringView((sl_char8*)key, sizeKey), StringView((sl_char8*)value, sizeValue));
	}

	sl_bool KeyValueWriter::remove(const void* key, sl_size sizeKey)
	{
		return remove(StringView((sl_char8*)key, sizeKey));
	}

	sl_bool KeyValueWriter::set(const StringParam& _key, const Variant& value)
	{
		if (value.isUndefined()) {
			return remove(_key);
		} else if (value.isNull()) {
			StringData key(_key);
			return set(key.getUnsafeData(), key.getLength(), sl_null, 0);
		} else if (value.isString()) {
			StringData key(_key);
			String str = value.getString();
			if (str.isNotEmpty()) {
				return set(key.getUnsafeData(), key.getLength(), str.getData(), str.getLength());
			} else {
				return set(key.getUnsafeData(), key.getLength(), "", 1);
			}
		} else {
			sl_uint8 buf[1024];
			Memory mem;
			sl_size nWrite = SerializeVariant(value, buf, sizeof(buf), &mem);
			if (nWrite) {
				StringData key(_key);
				void* p = buf;
				if (mem.isNotNull()) {
					p = mem.getData();
				}
				return set(key.getUnsafeData(), key.getLength(), p, nWrite);
			}
		}
		return sl_false;
	}

	sl_bool KeyValueWriter::remove(const StringParam& _key)
	{
		StringData key(_key);
		return remove(key.getUnsafeData(), key.getLength());
	}


	SLIB_DEFINE_OBJECT(KeyValueIO, KeyValueWriter)

	KeyValueIO::KeyValueIO()
	{
	}

	KeyValueIO::~KeyValueIO()
	{
	}

	sl_bool KeyValueIO::get(const void* key, sl_size sizeKey, MemoryData* pOutValue)
	{
		sl_reg n = get(key, sizeKey, sl_null, 0);
		if (n < 0) {
			return sl_false;
		}
		if (!pOutValue) {
			return sl_true;
		}
		if (!n) {
			pOutValue->size = 0;
			return sl_true;
		}
		if (pOutValue->size < (sl_size)n) {
			Memory mem = Memory::create(n);
			if (mem.isNull()) {
				return sl_false;
			}
			*pOutValue = Move(mem);
		}
		n = get(key, sizeKey, pOutValue->data, n);
		if (n >= 0) {
			pOutValue->size = n;
			return sl_true;
		} else {
			return sl_false;
		}
	}

	sl_reg KeyValueIO::get(const void* key, sl_size sizeKey, void* value, sl_size sizeValue)
	{
		MemoryData data;
		if (get(key, sizeKey, &data)) {
			if (sizeValue) {
				if (data.size) {
					sl_size n = SLIB_MIN(sizeValue, data.size);
					Base::copyMemory(value, data.data, n);
					return n;
				} else {
					return 0;
				}
			} else {
				return data.size;
			}
		} else {
			return -1;
		}
	}

	Variant KeyValueIO::get(const StringParam& _key)
	{
		StringData key(_key);
		MemoryData value;
		if (get(key.getUnsafeData(), key.getLength(), &value)) {
			return GetValue(value);
		} else {
			return Variant();
		}
	}


	SLIB_DEFINE_OBJECT(KeyValueTransation, KeyValueWriter)

	KeyValueTransation::KeyValueTransation(): m_flagClosed(sl_false)
	{
	}

	KeyValueTransation::~KeyValueTransation()
	{
	}

	void KeyValueTransation::commit()
	{
		ObjectLocker lock(this);
		if (m_flagClosed) {
			return;
		}
		m_flagClosed = sl_true;
		_commit();
	}

	void KeyValueTransation::discard()
	{
		ObjectLocker lock(this);
		if (m_flagClosed) {
			return;
		}
		m_flagClosed = sl_true;
		_discard();
	}


	SLIB_DEFINE_OBJECT(KeyValueCursor, Object)

	KeyValueCursor::KeyValueCursor()
	{
	}

	KeyValueCursor::~KeyValueCursor()
	{
	}

	sl_bool KeyValueCursor::getValue(MemoryData* pOutValue)
	{
		sl_reg n = getValue(sl_null, 0);
		if (n < 0) {
			return sl_false;
		}
		if (!pOutValue) {
			return sl_true;
		}
		if (!n) {
			pOutValue->size = 0;
			return sl_true;
		}
		if (pOutValue->size < (sl_size)n) {
			Memory mem = Memory::create(n);
			if (mem.isNull()) {
				return sl_false;
			}
			*pOutValue = Move(mem);
		}
		n = getValue(pOutValue->data, n);
		if (n >= 0) {
			pOutValue->size = n;
			return sl_true;
		} else {
			return sl_false;
		}
	}

	sl_reg KeyValueCursor::getValue(void* value, sl_size sizeValue)
	{
		MemoryData data;
		if (getValue(&data)) {
			if (sizeValue) {
				if (data.size) {
					sl_size n = SLIB_MIN(sizeValue, data.size);
					Base::copyMemory(value, data.data, n);
					return n;
				} else {
					return 0;
				}
			} else {
				return data.size;
			}
		} else {
			return -1;
		}
	}

	Variant KeyValueCursor::getValue()
	{
		MemoryData value;
		if (getValue(&value)) {
			return GetValue(value);
		} else {
			return Variant();
		}
	}


	SLIB_DEFINE_OBJECT(KeyValueStore, KeyValueIO)

	KeyValueStore::KeyValueStore()
	{
	}

	KeyValueStore::~KeyValueStore()
	{
	}

}
