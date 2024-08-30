/*
 *   Copyright (c) 2008-2024 SLIBIO <https://github.com/SLIBIO>
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

#include "slib/core/object.h"

#include "slib/core/variant.h"
#include "slib/core/string_buffer.h"
#include "slib/core/stringx.h"
#include "slib/core/priv/map_iterator.h"
#include "slib/data/serialize.h"

namespace slib
{

	Lockable::Lockable() noexcept
	{
	}

	Lockable::~Lockable() noexcept
	{
	}

	Mutex* Lockable::getLocker() const noexcept
	{
		return const_cast<Mutex*>(&m_locker);
	}

	void Lockable::lock() const noexcept
	{
		m_locker.lock();
	}

	void Lockable::unlock() const noexcept
	{
		m_locker.unlock();
	}

	sl_bool Lockable::tryLock() const noexcept
	{
		return m_locker.tryLock();
	}


	SLIB_DEFINE_ROOT_OBJECT(Object)

	Object::Object() noexcept: m_onFree(sl_null), m_properties(sl_null)
	{
	}

	Object::~Object() noexcept
	{
		if (m_onFree) {
			((Callable<void()>*)m_onFree)->decreaseReference();
		}
		if (m_properties) {
			delete (CHashMap<String, Variant>*)m_properties;
		}
	}

	void Object::free()
	{
		if (m_onFree) {
			((Callable<void()>*)m_onFree)->invoke();
		}
		CRef::free();
	}

	const Function<void()>& Object::getOnFree()
	{
		return *((Function<void()>*)((void*)&m_onFree));
	}

	void Object::setOnFree(const Function<void()>& callback)
	{
		m_onFree = callback.ref.ptr;
		((Callable<void()>*)m_onFree)->increaseReference_NoSync();
	}

	Variant Object::getProperty(const String& name)
	{
		ObjectLocker lock(this);
		if (m_properties) {
			CHashMap<String, Variant>* map = static_cast<CHashMap<String, Variant>*>(m_properties);
			return map->getValue_NoLock(name);
		}
		return Variant();
	}

	sl_bool Object::setProperty(const String& name, const Variant& value)
	{
		ObjectLocker lock(this);
		CHashMap<String, Variant>* map;
		if (m_properties) {
			map = static_cast<CHashMap<String, Variant>*>(m_properties);
		} else {
			map = new CHashMap<String, Variant>;
			if (map) {
				m_properties = map;
			} else {
				return sl_false;
			}
		}
		return map->put_NoLock(name, value) != sl_null;
	}

	sl_bool Object::clearProperty(const String& name)
	{
		ObjectLocker lock(this);
		if (m_properties) {
			CHashMap<String, Variant>* map = static_cast<CHashMap<String, Variant>*>(m_properties);
			return map->remove_NoLock(name);
		}
		return sl_false;
	}

	PropertyIterator Object::getPropertyIterator()
	{
		ObjectLocker lock(this);
		if (m_properties) {
			CHashMap<String, Variant>* map = static_cast<CHashMap<String, Variant>*>(m_properties);
			return new MapIterator< CHashMap<String, Variant> >(map);
		}
		return sl_null;
	}

	sl_bool Object::toJsonString(StringBuffer& buf)
	{
		ObjectLocker lock(this);
		if (m_properties) {
			CHashMap<String, Variant>* map = static_cast<CHashMap<String, Variant>*>(m_properties);
			if (!(buf.addStatic("{"))) {
				return sl_false;
			}
			sl_bool flagFirst = sl_true;
			auto node = map->getFirstNode();
			while (node) {
				Variant& v = node->value;
				if (v.isNotUndefined()) {
					if (flagFirst) {
						flagFirst = sl_false;
					} else {
						if (!(buf.addStatic(", "))) {
							return sl_false;
						}
					}
					if (!(buf.add(Stringx::applyBackslashEscapes(node->key)))) {
						return sl_false;
					}
					if (!(buf.addStatic(": "))) {
						return sl_false;
					}
					if (!(v.toJsonString(buf))) {
						return sl_false;
					}
				}
				node = node->getNext();
			}
			if (!(buf.addStatic("}"))) {
				return sl_false;
			}
			return sl_true;
		} else {
			PropertyIterator iterator = getPropertyIterator();
			if (!(buf.addStatic("{"))) {
				return sl_false;
			}
			sl_bool flagFirst = sl_true;
			while (iterator.moveNext()) {
				Variant v = iterator.getValue();
				if (v.isNotUndefined()) {
					if (flagFirst) {
						flagFirst = sl_false;
					} else {
						if (!(buf.addStatic(", "))) {
							return sl_false;
						}
					}
					String name = iterator.getKey();
					if (!(buf.add(Stringx::applyBackslashEscapes(name)))) {
						return sl_false;
					}
					if (!(buf.addStatic(": "))) {
						return sl_false;
					}
					if (!(v.toJsonString(buf))) {
						return sl_false;
					}
				}
			}
			if (!(buf.addStatic("}"))) {
				return sl_false;
			}
			return sl_true;
		}
	}

	sl_bool Object::toJsonBinary(MemoryBuffer& buf)
	{
		if (!(SerializeByte(&buf, (sl_uint8)(VariantType::Object)))) {
			return sl_false;
		}
		ObjectLocker lock(this);
		if (m_properties) {
			HashMap<String, Variant>& map = *((HashMap<String, Variant>*)(void*)(&m_properties));
			return Serialize(&buf, map);
		} else {
			PropertyIterator iterator = getPropertyIterator();
			MemoryBuffer queue;
			sl_size count = 0;
			while (iterator.moveNext()) {
				Variant v = iterator.getValue();
				if (v.isNotUndefined()) {
					String name = iterator.getKey();
					if (!(Serialize(&queue, name))) {
						return sl_false;
					}
					if (!(Serialize(&queue, v))) {
						return sl_false;
					}
					count++;
				}
			}
			if (!(CVLI::serialize(&buf, count))) {
				return sl_false;
			}
			buf.link(queue);
			return sl_true;
		}
	}


	ObjectLocker::ObjectLocker() noexcept
	{
	}

	ObjectLocker::ObjectLocker(const Lockable* object) noexcept: MutexLocker(object ? object->getLocker(): sl_null)
	{
	}

	ObjectLocker::~ObjectLocker() noexcept
	{
	}

	void ObjectLocker::lock(const Lockable* object) noexcept
	{
		if (object) {
			MutexLocker::lock(object->getLocker());
		}
	}


	MultipleObjectsLocker::MultipleObjectsLocker() noexcept
	{
	}

	MultipleObjectsLocker::MultipleObjectsLocker(const Lockable* object) noexcept: MultipleMutexLocker(object ? object->getLocker(): sl_null)
	{
	}

	MultipleObjectsLocker::MultipleObjectsLocker(const Lockable* object1, const Lockable* object2) noexcept: MultipleMutexLocker(object1 ? object1->getLocker() : sl_null, object2 ? object2->getLocker() : sl_null)
	{
	}

	MultipleObjectsLocker::~MultipleObjectsLocker() noexcept
	{
	}

	void MultipleObjectsLocker::lock(const Lockable* object) noexcept
	{
		if (object) {
			MultipleMutexLocker::lock(object->getLocker());
		}
	}

	void MultipleObjectsLocker::lock(const Lockable* object1, const Lockable* object2) noexcept
	{
		if (object1) {
			if (object2) {
				MultipleMutexLocker::lock(object1->getLocker(), object2->getLocker());
			} else {
				MultipleMutexLocker::lock(object1->getLocker());
			}
		} else {
			if (object2) {
				MultipleMutexLocker::lock(object2->getLocker());
			}
		}
	}

}
