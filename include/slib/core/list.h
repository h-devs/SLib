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

#ifndef CHECKHEADER_SLIB_CORE_LIST
#define CHECKHEADER_SLIB_CORE_LIST

#include "array.h"
#include "lockable.h"

#ifdef SLIB_SUPPORT_STD_TYPES
#include <initializer_list>
#endif

namespace slib
{
	
	template <class T>
	class List;
	
	template <class T>
	using AtomicList = Atomic< List<T> >;
	
	namespace priv
	{
		namespace list
		{
			sl_bool setCapacity(void* pData, sl_size elementSize, sl_size* pCapacity, sl_size* pCount, sl_size newCapacity) noexcept;
			
			sl_bool adjustCapacity(void* pData, sl_size elementSize, sl_size* pCapacity, sl_size* pCount, sl_size newCount) noexcept;

			sl_bool growCapacity(void* pData, sl_size elementSize, sl_size* pCapacity, sl_size newCount) noexcept;

			sl_bool shrinkCapacity(void* pData, sl_size elementSize, sl_size* pCapacity, sl_size newCount) noexcept;
		}
	}

	class SLIB_EXPORT CListBase : public Referable, public Lockable
	{
		SLIB_DECLARE_OBJECT

	public:
		CListBase();

		~CListBase();

	};
	
	template <class T>
	class SLIB_EXPORT CList : public CListBase
	{
	public:
		typedef T ELEMENT_TYPE;
		
	protected:
		T* m_data;
		sl_size m_count;
		sl_size m_capacity;

	public:
		CList() noexcept: m_data(sl_null), m_count(0), m_capacity(0) {}

		CList(sl_size count) noexcept
		{
			if (count > 0) {
				T* data = (T*)(Base::createMemory(count * sizeof(T)));
				if (data) {
					ArrayTraits<T>::construct(data, count);
					m_data = data;
					m_count = count;
					m_capacity = count;
					return;
				}
			}
			m_data = sl_null;
			m_count = 0;
			m_capacity = 0;
		}

		CList(sl_size count, sl_size capacity) noexcept
		{
			if (capacity < count) {
				capacity = count;
			}
			if (capacity > 0) {
				T* data = (T*)(Base::createMemory(capacity * sizeof(T)));
				if (data) {
					ArrayTraits<T>::construct(data, count);
					m_data = data;
					m_count = count;
					m_capacity = capacity;
					return;
				}
			}
			m_data = sl_null;
			m_count = 0;
			m_capacity = 0;
		}
		
		CList(sl_size count, sl_size capacity, const T& initialValue) noexcept
		{
			if (capacity < count) {
				capacity = count;
			}
			if (capacity > 0) {
				T* data = (T*)(Base::createMemory(capacity * sizeof(T)));
				if (data) {
					for (sl_size i = 0; i < count; i++) {
						new (data + i) T(initialValue);
					}
					m_data = data;
					m_count = count;
					m_capacity = capacity;
					return;
				}
			}
			m_data = sl_null;
			m_count = 0;
			m_capacity = 0;
		}

		template <class VALUE>
		CList(const VALUE* values, sl_size count) noexcept
		{
			if (count > 0) {
				T* data = (T*)(Base::createMemory(count * sizeof(T)));
				if (data) {
					ArrayTraits<T>::copy_construct(data, values, count);
					m_data = data;
					m_count = count;
					m_capacity = count;
					return;
				}
			}
			m_data = sl_null;
			m_count = 0;
			m_capacity = 0;
		}
		
#ifdef SLIB_SUPPORT_STD_TYPES
		CList(const std::initializer_list<T>& l) noexcept: CList(l.begin(), l.size()) {}
#endif

		~CList() noexcept
		{
			T* data = m_data;
			if (data) {
				ArrayTraits<T>::free(data, m_count);
				Base::freeMemory((void*)data);
			}
		}
		
	public:
		CList(const CList& other) = delete;
		
		CList(CList&& other) noexcept
		{
			m_data = other.m_data;
			m_count = other.m_count;
			m_capacity = other.m_capacity;
			other.m_data = sl_null;
			other.m_count = 0;
			other.m_capacity = 0;
		}
		
		CList& operator=(const CList& other) = delete;
		
		CList& operator=(CList&& other) noexcept
		{
			T* data = m_data;
			if (data) {
				ArrayTraits<T>::free(data, m_count);
				Base::freeMemory((void*)data);
			}
			m_data = other.m_data;
			m_count = other.m_count;
			m_capacity = other.m_capacity;
			other.m_data = sl_null;
			other.m_count = 0;
			other.m_capacity = 0;
			return *this;
		}

	public:
		static CList<T>* create() noexcept
		{
			return new CList<T>;
		}

		static CList<T>* create(sl_size count) noexcept
		{
			if (count > 0) {
				CList<T>* ret = new CList<T>(count);
				if (ret) {
					if (ret->m_count > 0) {
						return ret;
					}
					delete ret;
				}
			} else {
				return new CList<T>;
			}
			return sl_null;
		}

		static CList<T>* create(sl_size count, sl_size capacity) noexcept
		{
			if (count > 0 || capacity > 0) {
				CList<T>* ret = new CList<T>(count, capacity);
				if (ret) {
					if (ret->m_capacity > 0) {
						return ret;
					}
					delete ret;
				}
			} else {
				return new CList<T>;
			}
			return sl_null;
		}

		static CList<T>* create(sl_size count, sl_size capacity, const T& initialValue) noexcept
		{
			if (count > 0 || capacity > 0) {
				CList<T>* ret = new CList<T>(count, capacity, initialValue);
				if (ret) {
					if (ret->m_capacity > 0) {
						return ret;
					}
					delete ret;
				}
			} else {
				return new CList<T>;
			}
			return sl_null;
		}
		
		template <class VALUE>
		static CList<T>* create(const VALUE* values, sl_size count) noexcept
		{
			if (count > 0) {
				CList<T>* ret = new CList<T>(values, count);
				if (ret) {
					if (ret->m_count > 0) {
						return ret;
					}
					delete ret;
				}
			} else {
				return new CList<T>;
			}
			return sl_null;
		}
		
		template <class VALUE>
		static CList<T>* create(const Array<VALUE>& array) noexcept
		{
			return create(array.getData(), array.getCount());
		}

#ifdef SLIB_SUPPORT_STD_TYPES
		static CList<T>* create(const std::initializer_list<T>& l) noexcept
		{
			return create(l.begin(), l.size());
		}
#endif
		
		template <class VALUE>
		static CList<T>* createFromElement(VALUE&& value) noexcept
		{
			CList<T>* ret = new CList<T>();
			if (ret) {
				T* data = (T*)(Base::createMemory(sizeof(T)));
				if (data) {
					new (data) T(Forward<VALUE>(value));
					ret->m_data = data;
					ret->m_count = 1;
					ret->m_capacity = 1;
					return ret;
				}
				delete ret;
			}
			return sl_null;
		}
		
		static CList<T>* createFromElement(const T& value, sl_size count) noexcept
		{
			return create(count, count, value);
		}
		
		template <class... ARGS>
		static CList<T>* createFromElements(ARGS&&... _values) noexcept
		{
			T values[] = {Forward<ARGS>(_values)...};
			return create(values, sizeof...(_values));
		}

		template <class VALUE>
		static CList<T>* createCopy(CList<VALUE>* other) noexcept
		{
			if (other) {
				ObjectLocker lock(other);
				return create(other->getData(), other->getCount());
			}
			return sl_null;
		}

	public:
		sl_size getCount() const noexcept
		{
			return m_count;
		}

		sl_size getCapacity() const noexcept
		{
			return m_capacity;
		}

		T* getData() const noexcept
		{
			return m_data;
		}

	public:
		/* unsynchronized function */
		T* getPointerAt(sl_size index) const noexcept
		{
			if (index < m_count) {
				return m_data + index;
			}
			return sl_null;
		}

		sl_bool getAt_NoLock(sl_size index, T* _out = sl_null) const noexcept
		{
			if (index < m_count) {
				if (_out) {
					*_out = m_data[index];
				}
				return sl_true;
			}
			return sl_false;
		}

		sl_bool getAt(sl_size index, T* _out = sl_null) const noexcept
		{
			ObjectLocker lock(this);
			if (index < m_count) {
				if (_out) {
					*_out = m_data[index];
				}
				return sl_true;
			}
			return sl_false;
		}

		T getValueAt_NoLock(sl_size index) const noexcept
		{
			if (index < m_count) {
				return m_data[index];
			} else {
				return T();
			}
		}

		T getValueAt(sl_size index) const noexcept
		{
			ObjectLocker lock(this);
			if (index < m_count) {
				return m_data[index];
			} else {
				return T();
			}
		}

		T getValueAt_NoLock(sl_size index, const T& def) const noexcept
		{
			if (index < m_count) {
				return m_data[index];
			}
			return def;
		}

		T getValueAt(sl_size index, const T& def) const noexcept
		{
			ObjectLocker lock(this);
			if (index < m_count) {
				return m_data[index];
			}
			return def;
		}

		sl_bool getFirst_NoLock(T* _out = sl_null) const noexcept
		{
			if (m_count) {
				if (_out) {
					*_out = m_data[0];
				}
				return sl_true;
			}
			return sl_false;
		}
		
		sl_bool getFirst(T* _out = sl_null) const noexcept
		{
			ObjectLocker lock(this);
			if (m_count) {
				if (_out) {
					*_out = m_data[0];
				}
				return sl_true;
			}
			return sl_false;
		}
		
		T getFirstValue_NoLock() const noexcept
		{
			if (m_count) {
				return m_data[0];
			} else {
				return T();
			}
		}
		
		T getFirstValue() const noexcept
		{
			ObjectLocker lock(this);
			if (m_count) {
				return m_data[0];
			} else {
				return T();
			}
		}
		
		T getFirstValue_NoLock(const T& def) const noexcept
		{
			if (m_count) {
				return m_data[0];
			}
			return def;
		}
		
		T getFirstValue(const T& def) const noexcept
		{
			ObjectLocker lock(this);
			if (m_count) {
				return m_data[0];
			}
			return def;
		}

		sl_bool getLast_NoLock(T* _out = sl_null) const noexcept
		{
			sl_size count = m_count;
			if (count) {
				if (_out) {
					*_out = m_data[count - 1];
				}
				return sl_true;
			}
			return sl_false;
		}
		
		sl_bool getLast(T* _out = sl_null) const noexcept
		{
			ObjectLocker lock(this);
			sl_size count = m_count;
			if (count) {
				if (_out) {
					*_out = m_data[count - 1];
				}
				return sl_true;
			}
			return sl_false;
		}
		
		T getLastValue_NoLock() const noexcept
		{
			sl_size count = m_count;
			if (count) {
				return m_data[count - 1];
			} else {
				return T();
			}
		}
		
		T getLastValue() const noexcept
		{
			ObjectLocker lock(this);
			sl_size count = m_count;
			if (count) {
				return m_data[count - 1];
			} else {
				return T();
			}
		}
		
		T getLastValue_NoLock(const T& def) const noexcept
		{
			sl_size count = m_count;
			if (count) {
				return m_data[count - 1];
			}
			return def;
		}
		
		T getLastValue(const T& def) const noexcept
		{
			ObjectLocker lock(this);
			sl_size count = m_count;
			if (count) {
				return m_data[count - 1];
			}
			return def;
		}

		template <class VALUE>
		sl_bool setAt_NoLock(sl_size index, VALUE&& value) const noexcept
		{
			if (index < m_count) {
				m_data[index] = Forward<VALUE>(value);
				return sl_true;
			}
			return sl_false;
		}

		template <class VALUE>
		sl_bool setAt(sl_size index, VALUE&& value) const noexcept
		{
			ObjectLocker lock(this);
			if (index < m_count) {
				m_data[index] = Forward<VALUE>(value);
				return sl_true;
			}
			return sl_false;
		}

		/* unsynchronized function */
		T const& operator[](sl_size_t index) const noexcept
		{
			return m_data[index];
		}

		/* unsynchronized function */
		T& operator[](sl_size_t index) noexcept
		{
			return m_data[index];
		}

	public:
		sl_bool setCount_NoLock(sl_size count) noexcept
		{
			sl_size oldCount = m_count;
			if (oldCount == count) {
				return sl_true;
			}
			if (count < oldCount) {
				ArrayTraits<T>::free(m_data + count, oldCount - count);
				m_count = count;
				adjustCapacity_NoLock(count);
				return sl_true;
			} else {
				if (adjustCapacity_NoLock(count)) {
					ArrayTraits<T>::construct(m_data + oldCount, count - oldCount);
					m_count = count;
					return sl_true;
				}
			}
			return sl_false;
		}

		sl_bool setCount(sl_size count) noexcept
		{
			ObjectLocker lock(this);
			return setCount_NoLock(count);
		}

		sl_bool setCapacity_NoLock(sl_size capacity) noexcept
		{
			return priv::list::setCapacity(reinterpret_cast<void*>(&m_data), sizeof(T), &m_capacity, &m_count, capacity);
		}
		
		sl_bool setCapacity(sl_size capacity) noexcept
		{
			ObjectLocker lock(this);
			return priv::list::setCapacity(reinterpret_cast<void*>(&m_data), sizeof(T), &m_capacity, &m_count, capacity);
		}
		
		sl_bool adjustCapacity_NoLock(sl_size count) noexcept
		{
			return priv::list::adjustCapacity(reinterpret_cast<void*>(&m_data), sizeof(T), &m_capacity, &m_count, count);
		}
		
		sl_bool adjustCapacity(sl_size count) noexcept
		{
			ObjectLocker lock(this);
			return priv::list::adjustCapacity(reinterpret_cast<void*>(&m_data), sizeof(T), &m_capacity, &m_count, count);
		}
		
		sl_bool growCapacity_NoLock(sl_size newCount) noexcept
		{
			return priv::list::growCapacity(reinterpret_cast<void*>(&m_data), sizeof(T), &m_capacity, newCount);
		}
		
		sl_bool growCapacity(sl_size newCount) noexcept
		{
			ObjectLocker lock(this);
			return priv::list::growCapacity(reinterpret_cast<void*>(&m_data), sizeof(T), &m_capacity, newCount);
		}
		
		sl_bool shrinkCapacity_NoLock() noexcept
		{
			return priv::list::shrinkCapacity(reinterpret_cast<void*>(&m_data), sizeof(T), &m_capacity, m_count);
		}
		
		sl_bool shrinkCapacity() noexcept
		{
			ObjectLocker lock(this);
			return priv::list::shrinkCapacity(reinterpret_cast<void*>(&m_data), sizeof(T), &m_capacity, m_count);
		}
		
		sl_bool shrinkToFit_NoLock() noexcept
		{
			return priv::list::setCapacity(reinterpret_cast<void*>(&m_data), sizeof(T), &m_capacity, &m_count, m_count);
		}
		
		sl_bool shrinkToFit() noexcept
		{
			ObjectLocker lock(this);
			return priv::list::setCapacity(reinterpret_cast<void*>(&m_data), sizeof(T), &m_capacity, &m_count, m_count);
		}

		template <class... ARGS>
		sl_bool insert_NoLock(sl_size index, ARGS&&... args) noexcept
		{
			sl_size oldCount = m_count;
			if (index > oldCount) {
				index = oldCount;
			}
			sl_size newCount = oldCount + 1;
			if (growCapacity_NoLock(newCount)) {
				T* data = m_data + index;
				if (index < oldCount) {
					Base::moveMemory(data + 1, data, (oldCount - index) * sizeof(T));
				}
				new (data) T(Forward<ARGS>(args)...);
				m_count = newCount;
				return sl_true;
			}
			return sl_false;
		}
		
		template <class... ARGS>
		sl_bool insert(sl_size index, ARGS&&... args) noexcept
		{
			ObjectLocker lock(this);
			return insert_NoLock(index, Forward<ARGS>(args)...);
		}

		template <class VALUE>
		sl_bool insertElements_NoLock(sl_size index, const VALUE* values, sl_size nValues) noexcept
		{
			if (!nValues) {
				return sl_true;
			}
			sl_size oldCount = m_count;
			if (index > oldCount) {
				index = oldCount;
			}
			sl_size newCount = oldCount + nValues;
			if (growCapacity_NoLock(newCount)) {
				T* data = m_data + index;
				if (index < oldCount) {
					Base::moveMemory(data + nValues, data, (oldCount - index) * sizeof(T));
				}
				ArrayTraits<T>::copy_construct(data, values, nValues);
				m_count = newCount;
				return sl_true;
			}
			return sl_false;
		}

		template <class VALUE>
		sl_bool insertElements(sl_size index, const VALUE* values, sl_size count) noexcept
		{
			if (!count) {
				return sl_true;
			}
			ObjectLocker lock(this);
			return insertElements_NoLock(index, values, count);
		}
		
		sl_bool insertElements_NoLock(sl_size index, sl_size nValues, const T& value) noexcept
		{
			if (!nValues) {
				return sl_true;
			}
			sl_size oldCount = m_count;
			if (index > oldCount) {
				index = oldCount;
			}
			sl_size newCount = oldCount + nValues;
			if (growCapacity_NoLock(newCount)) {
				T* data = m_data + index;
				if (index < oldCount) {
					Base::moveMemory(data + nValues, data, (oldCount - index) * sizeof(T));
				}
				for (sl_size i = 0; i < nValues; i++) {
					new (data + i) T(value);
				}
				m_count = newCount;
				return sl_true;
			}
			return sl_false;
		}
		
		sl_bool insertElements(sl_size index, sl_size count, const T& value) noexcept
		{
			if (!count) {
				return sl_true;
			}
			ObjectLocker lock(this);
			return insertElements_NoLock(index, count, value);
		}

#ifdef SLIB_SUPPORT_STD_TYPES
		sl_bool insertElements_NoLock(sl_size index, const std::initializer_list<T>& l) noexcept
		{
			return insertElements_NoLock(index, l.begin(), l.size());
		}
		
		sl_bool insertElements(sl_size index, const std::initializer_list<T>& l) noexcept
		{
			return insertElements(index, l.begin(), l.size());
		}
#endif
		
		template <class VALUE>
		sl_bool insertAll_NoLock(sl_size index, const CList<VALUE>* other) noexcept
		{
			if (!other) {
				return sl_true;
			}
			if (this == other) {
				return sl_false;
			}
			return insertElements_NoLock(index, other->getData(), other->getCount());
		}
		
		template <class VALUE>
		sl_bool insertAll(sl_size index, const CList<VALUE>* other) noexcept
		{
			if (!other) {
				return sl_true;
			}
			if (this == other) {
				return sl_false;
			}
			MultipleObjectsLocker lock(this, other);
			return insertElements_NoLock(index, other->getData(), other->getCount());
		}

		template <class... ARGS>
		sl_bool add_NoLock(ARGS&&... args) noexcept
		{
			sl_size oldCount = m_count;
			sl_size newCount = oldCount + 1;
			if (growCapacity_NoLock(newCount)) {
				new (m_data + oldCount) T(Forward<ARGS>(args)...);
				m_count = newCount;
				return sl_true;
			}
			return sl_false;
		}

		template <class... ARGS>
		sl_bool add(ARGS&&... args) noexcept
		{
			ObjectLocker lock(this);
			return add_NoLock(Forward<ARGS>(args)...);
		}
	
		template <class VALUE>
		sl_bool addElements_NoLock(const VALUE* values, sl_size nValues) noexcept
		{
			if (!nValues) {
				return sl_true;
			}
			sl_size oldCount = m_count;
			sl_size newCount = oldCount + nValues;
			if (growCapacity_NoLock(newCount)) {
				ArrayTraits<T>::copy_construct(m_data + oldCount, values, nValues);
				m_count = newCount;
				return sl_true;
			}
			return sl_false;
		}

		template <class VALUE>
		sl_bool addElements(const VALUE* values, sl_size count) noexcept
		{
			if (!count) {
				return sl_true;
			}
			ObjectLocker lock(this);
			return addElements_NoLock(values, count);
		}
		
		sl_bool addElements_NoLock(sl_size nValues, const T& value) noexcept
		{
			if (!nValues) {
				return sl_true;
			}
			sl_size oldCount = m_count;
			sl_size newCount = oldCount + nValues;
			if (growCapacity_NoLock(newCount)) {
				T* data = m_data + oldCount;
				for (sl_size i = 0; i < nValues; i++) {
					new (data + i) T(value);
				}
				m_count = newCount;
				return sl_true;
			}
			return sl_false;
		}
		
		sl_bool addElements(sl_size count, const T& value) noexcept
		{
			if (!count) {
				return sl_true;
			}
			ObjectLocker lock(this);
			return addElements_NoLock(count, value);
		}

#ifdef SLIB_SUPPORT_STD_TYPES
		sl_bool addElements_NoLock(const std::initializer_list<T>& l) noexcept
		{
			return addElements_NoLock(l.begin(), l.size());
		}
		
		sl_bool addElements(const std::initializer_list<T>& l) noexcept
		{
			return addElements(l.begin(), l.size());
		}
#endif
		
		template <class VALUE>
		sl_bool addAll_NoLock(const CList<VALUE>* other) noexcept
		{
			if (!other) {
				return sl_true;
			}
			if ((void*)this == (void*)other) {
				return sl_false;
			}
			return addElements_NoLock(other->getData(), other->getCount());
		}
	
		template <class VALUE>
		sl_bool addAll(const CList<VALUE>* other) noexcept
		{
			if (!other) {
				return sl_true;
			}
			if ((void*)this == (void*)other) {
				return sl_false;
			}
			MultipleObjectsLocker lock(this, other);
			return addElements_NoLock(other->getData(), other->getCount());
		}

		template < class VALUE, class EQUALS = Equals<T, typename RemoveConstReference<VALUE>::Type> >
		sl_bool addIfNotExist_NoLock(VALUE&& value, const EQUALS& equals = EQUALS()) noexcept
		{
			if (indexOf_NoLock(value, equals) < 0) {
				return add_NoLock(value);
			}
			return sl_false;
		}

		template < class VALUE, class EQUALS = Equals<T, typename RemoveConstReference<VALUE>::Type> >
		sl_bool addIfNotExist(VALUE&& value, const EQUALS& equals = EQUALS()) noexcept
		{
			ObjectLocker lock(this);
			if (indexOf_NoLock(value, equals) < 0) {
				return add_NoLock(value);
			}
			return sl_false;
		}

		sl_bool removeAt_NoLock(sl_size index, T* outValue = sl_null) noexcept
		{
			sl_size count = m_count;
			if (index < count) {
				T* m = m_data + index;
				if (outValue) {
					*outValue = Move(*m);
				}
				m->~T();
				if (index + 1 < count) {
					Base::moveMemory(m, m+1, (count - index - 1) * sizeof(T));
				}
				m_count = count - 1;
				shrinkCapacity_NoLock();
				return sl_true;
			}
			return sl_false;
		}

		sl_bool removeAt(sl_size index, T* outValue = sl_null) noexcept
		{
			ObjectLocker lock(this);
			return removeAt_NoLock(index, outValue);
		}
	
		sl_size removeRange_NoLock(sl_size index, sl_size nValues) noexcept
		{
			sl_size count = m_count;
			if (nValues > 0 && index < count) {
				if (nValues > count - index) {
					nValues = count - index;
				}
				T* dst = m_data + index;
				ArrayTraits<T>::free(dst, nValues);
				if (index + nValues < count) {
					Base::moveMemory(dst, dst + nValues, (count - index - nValues) * sizeof(T));
				}
				m_count = count - nValues;
				shrinkCapacity_NoLock();
				return nValues;
			}
			return 0;
		}

		sl_size removeRange(sl_size index, sl_size nValues) noexcept
		{
			ObjectLocker lock(this);
			return removeRange_NoLock(index, nValues);
		}

		template < class VALUE, class EQUALS = Equals<T, VALUE> >
		sl_bool remove_NoLock(const VALUE& value, const EQUALS& equals = EQUALS()) noexcept
		{
			if (m_data) {
				T* data = m_data;
				sl_size count = m_count;
				for (sl_size i = 0; i < count; i++) {
					if (equals(data[i], value)) {
						(data + i)->~T();
						if (i + 1 < count) {
							Base::moveMemory(data + i, data + i + 1, (count - i - 1) * sizeof(T));
						}
						m_count = count - 1;
						shrinkCapacity_NoLock();
						return sl_true;
					}
				}
			}
			return sl_false;
		}

		template < class VALUE, class EQUALS = Equals<T, VALUE> >
		sl_bool remove(const VALUE& value, const EQUALS& equals = EQUALS()) noexcept
		{
			ObjectLocker lock(this);
			return remove_NoLock(value, equals);
		}

		template < class VALUE, class EQUALS = Equals<T, VALUE> >
		sl_size removeValues_NoLock(const VALUE& value, const EQUALS& equals = EQUALS()) noexcept
		{
			if (m_data) {
				T* data = m_data;
				sl_size count = m_count;
				sl_size t = 0;
				for (sl_size i = 0; i < count; i++) {
					if (!(equals(data[i], value))) {
						if (t != i) {
							data[t] = Move(data[i]);
						}
						t++;
					}
				}
				if (t < count) {
					setCount_NoLock(t);
					return count - t;
				}
			}
			return 0;
		}

		template < class VALUE, class EQUALS = Equals<T, VALUE> >
		sl_size removeValues(const VALUE& value, const EQUALS& equals = EQUALS()) noexcept
		{
			ObjectLocker lock(this);
			return removeValues_NoLock(value, equals);
		}
		
		template <class PREDICATE>
		sl_bool removeIf_NoLock(const PREDICATE& p) noexcept
		{
			if (m_data) {
				T* data = m_data;
				sl_size count = m_count;
				for (sl_size i = 0; i < count; i++) {
					if (p(data[i])) {
						(data + i)->~T();
						if (i + 1 < count) {
							Base::moveMemory(data + i, data + i + 1, (count - i - 1) * sizeof(T));
						}
						m_count = count - 1;
						shrinkCapacity_NoLock();
						return sl_true;
					}
				}
			}
			return sl_false;
		}
		
		template <class PREDICATE>
		sl_bool removeIf(const PREDICATE& p) noexcept
		{
			ObjectLocker lock(this);
			return removeIf_NoLock(p);
		}
		
		template <class PREDICATE>
		sl_size removeElementsIf_NoLock(const PREDICATE& p) noexcept
		{
			if (m_data) {
				T* data = m_data;
				sl_size count = m_count;
				sl_size t = 0;
				for (sl_size i = 0; i < count; i++) {
					if (!(p(data[i]))) {
						if (t != i) {
							data[t] = Move(data[i]);
						}
						t++;
					}
				}
				if (t < count) {
					setCount_NoLock(t);
					return count - t;
				}
			}
			return 0;
		}
		
		template <class PREDICATE>
		sl_size removeElementsIf(const PREDICATE& p) noexcept
		{
			ObjectLocker lock(this);
			return removeElementsIf_NoLock(p);
		}

		sl_size removeAll_NoLock() noexcept
		{
			T* data = m_data;
			sl_size count = m_count;
			if (data) {
				ArrayTraits<T>::free(data, count);
				Base::freeMemory((void*)data);
				m_data = sl_null;
			}
			m_count = 0;
			m_capacity = 0;
			return count;
		}

		sl_size removeAll() noexcept
		{
			T* data;
			sl_size count;
			{
				ObjectLocker lock(this);
				data = m_data;
				count = m_count;
				m_data = sl_null;
				m_count = 0;
				m_capacity = 0;
			}
			if (data) {
				ArrayTraits<T>::free(data, count);
				Base::freeMemory((void*)data);
			}
			return count;
		}

		sl_bool popFront_NoLock(T* _out = sl_null) noexcept
		{
			sl_size count = m_count;
			if (count > 0) {
				T* data = m_data;
				if (_out) {
					*_out = Move(*data);
				}
				data->~T();
				count--;
				Base::moveMemory(data, data + 1, count * sizeof(T));
				m_count = count;
				shrinkCapacity_NoLock();
				return sl_true;
			}
			return sl_false;
		}

		sl_bool popFront(T* _out = sl_null) noexcept
		{
			ObjectLocker lock(this);
			return popFront_NoLock(_out);
		}

		sl_size popFrontElements_NoLock(sl_size nElements) noexcept
		{
			sl_size count = m_count;
			if (nElements > 0 && count > 0) {
				if (nElements > count) {
					nElements = count;
				}
				T* data = m_data;
				ArrayTraits<T>::free(data, nElements);
				count -= nElements;
				Base::moveMemory(data, data + nElements, count * sizeof(T));
				m_count = count;
				shrinkCapacity_NoLock();
				return nElements;
			}
			return 0;
		}

		sl_size popFrontElements(sl_size count) noexcept
		{
			ObjectLocker lock(this);
			return popFrontElements_NoLock(count);
		}

		sl_bool popBack_NoLock(T* _out = sl_null) noexcept
		{
			sl_size count = m_count;
			if (count > 0) {
				T* data = m_data + count - 1;
				if (_out) {
					*_out = Move(*data);
				}
				data->~T();
				m_count = count - 1;
				shrinkCapacity_NoLock();
				return sl_true;
			}
			return sl_false;
		}

		sl_bool popBack(T* _out = sl_null) noexcept
		{
			ObjectLocker lock(this);
			return popBack_NoLock(_out);
		}

		sl_size popBackElements_NoLock(sl_size nElements) noexcept
		{
			sl_size count = m_count;
			if (nElements > 0 && count > 0) {
				if (nElements > count) {
					nElements = count;
				}
				ArrayTraits<T>::free(m_data + count - nElements, nElements);
				m_count = count - nElements;
				shrinkCapacity_NoLock();
				return nElements;
			}
			return 0;
		}

		sl_size popBackElements(sl_size count) noexcept
		{
			ObjectLocker lock(this);
			return popBackElements_NoLock(count);
		}
		
		template < class VALUE, class ARG = Equals<T, VALUE> >
		sl_reg indexOf_NoLock(const VALUE& value, const ARG& arg = ARG()) const noexcept
		{
			return ArrayTraits<T>::indexOf(m_data, m_count, value, arg);
		}
		
		template < class VALUE, class ARG = Equals<T, VALUE> >
		sl_reg indexOf(const VALUE& value, const ARG& arg = ARG()) const noexcept
		{
			ObjectLocker lock(this);
			return ArrayTraits<T>::indexOf(m_data, m_count, value, arg);
		}
		
		template <class VALUE, class EQUALS>
		sl_reg indexOf_NoLock(const VALUE& value, const EQUALS& equals, sl_reg startIndex) const noexcept
		{
			return ArrayTraits<T>::indexOf(m_data, m_count, value, equals, startIndex);
		}
		
		template <class VALUE, class EQUALS>
		sl_reg indexOf(const VALUE& value, const EQUALS& equals, sl_reg startIndex) const noexcept
		{
			ObjectLocker lock(this);
			return ArrayTraits<T>::indexOf(m_data, m_count, value, equals, startIndex);
		}
		
		template < class VALUE, class ARG = Equals<T, VALUE> >
		sl_reg lastIndexOf_NoLock(const VALUE& value, const ARG& arg = ARG()) const noexcept
		{
			return ArrayTraits<T>::lastIndexOf(m_data, m_count, value, arg);
		}
		
		template < class VALUE, class ARG = Equals<T, VALUE> >
		sl_reg lastIndexOf(const VALUE& value, const ARG& arg = ARG()) const noexcept
		{
			ObjectLocker lock(this);
			return ArrayTraits<T>::lastIndexOf(m_data, m_count, value, arg);
		}
		
		template <class VALUE, class EQUALS>
		sl_reg lastIndexOf_NoLock(const VALUE& value, const EQUALS& equals, sl_reg startIndex) const noexcept
		{
			return ArrayTraits<T>::lastIndexOf(m_data, m_count, value, equals, startIndex);
		}
			
		template <class VALUE, class EQUALS>
		sl_reg lastIndexOf(const VALUE& value, const EQUALS& equals, sl_reg startIndex) const noexcept
		{
			ObjectLocker lock(this);
			return ArrayTraits<T>::lastIndexOf(m_data, m_count, value, equals, startIndex);
		}

		template < class VALUE, class EQUALS = Equals<T, VALUE> >
		sl_bool contains_NoLock(const VALUE& value, const EQUALS& equals = EQUALS()) const noexcept
		{
			return ArrayTraits<T>::indexOf(m_data, m_count, value, equals) >= 0;
		}

		template < class VALUE, class EQUALS = Equals<T, VALUE> >
		sl_bool contains(const VALUE& value, const EQUALS& equals = EQUALS()) const noexcept
		{
			ObjectLocker lock(this);
			return ArrayTraits<T>::indexOf(m_data, m_count, value, equals) >= 0;
		}

		CList<T>* duplicate_NoLock() const noexcept
		{
			if (m_count > 0) {
				return create(m_data, m_count);
			}
			return sl_null;
		}

		CList<T>* duplicate() const noexcept
		{
			ObjectLocker lock(this);
			return duplicate_NoLock();
		}

		Array<T> toArray_NoLock() const noexcept
		{
			return Array<T>::create(m_data, m_count);
		}

		Array<T> toArray() const noexcept
		{
			ObjectLocker lock(this);
			return toArray_NoLock();
		}

		template < class COMPARE = Compare<T> >
		void sort_NoLock(const COMPARE& compare = COMPARE()) const noexcept
		{
			QuickSort::sortAsc(m_data, m_count, compare);
		}

		template < class COMPARE = Compare<T> >
		void sort(const COMPARE& compare = COMPARE()) const noexcept
		{
			ObjectLocker lock(this);
			QuickSort::sortAsc(m_data, m_count, compare);
		}
		
		template < class COMPARE = Compare<T> >
		void sortDesc_NoLock(const COMPARE& compare = COMPARE()) const noexcept
		{
			QuickSort::sortDesc(m_data, m_count, compare);
		}
		
		template < class COMPARE = Compare<T> >
		void sortDesc(const COMPARE& compare = COMPARE()) const noexcept
		{
			ObjectLocker lock(this);
			QuickSort::sortDesc(m_data, m_count, compare);
		}
		
		void reverse_NoLock() const noexcept
		{
			ArrayTraits<T>::reverse(m_data, m_count);
		}
		
		void reverse() const noexcept
		{
			ObjectLocker lock(this);
			ArrayTraits<T>::reverse(m_data, m_count);
		}

		CList<T>* slice_NoLock(sl_size index, sl_size count = SLIB_SIZE_MAX) const noexcept
		{
			if (count > 0 && index < m_count) {
				sl_size n = m_count - index;
				if (count > n) {
					count = n;
				}
				return create(m_data + index, count);
			}
			return sl_null;
		}
		
		CList<T>* slice(sl_size index, sl_size count = SLIB_SIZE_MAX) const noexcept
		{
			ObjectLocker lock(this);
			return slice_NoLock(index, count);
		}

		Ref<Collection> toCollection() noexcept;

		// range-based for loop
		T* begin() noexcept
		{
			return m_data;
		}

		T const* begin() const noexcept
		{
			return m_data;
		}

		T* end() noexcept
		{
			return m_data + m_count;
		}

		T const* end() const noexcept
		{
			return m_data + m_count;
		}

	};
	
	
	template <class T>
	class SLIB_EXPORT List
	{
	public:
		typedef T ELEMENT_TYPE;
		
	public:
		Ref< CList<T> > ref;
		SLIB_REF_WRAPPER(List, CList<T>)

	public:
		List(sl_size count) noexcept: ref(CList<T>::create(count)) {}
		
		List(sl_size count, sl_size capacity) noexcept: ref(CList<T>::create(count, capacity)) {}
		
		List(sl_size count, sl_size capacity, const T& initialValue) noexcept: ref(CList<T>::create(count, capacity, initialValue)) {}
		
		template <class VALUE>
		List(const VALUE* values, sl_size count) noexcept: ref(CList<T>::create(values, count)) {}

#ifdef SLIB_SUPPORT_STD_TYPES
		List(const std::initializer_list<T>& l) noexcept: ref(CList<T>::create(l.begin(), l.size())) {}
#endif
		
	public:
		static List<T> create() noexcept
		{
			return CList<T>::create();
		}

		static List<T> create(sl_size count) noexcept
		{
			return CList<T>::create(count);
		}

		static List<T> create(sl_size count, sl_size capacity) noexcept
		{
			return CList<T>::create(count, capacity);
		}
		
		static List<T> create(sl_size count, sl_size capacity, const T& initialValue) noexcept
		{
			return CList<T>::create(count, capacity, initialValue);
		}
		
		template <class VALUE>
		static List<T> create(const VALUE* values, sl_size count) noexcept
		{
			return CList<T>::create(values, count);
		}

		template <class VALUE>
		static List<T> create(const Array<VALUE>& array) noexcept
		{
			return CList<T>::create(array.getData(), array.getCount());
		}

		static List<T> create(Collection* collection);

#ifdef SLIB_SUPPORT_STD_TYPES
		static List<T> create(const std::initializer_list<T>& l) noexcept
		{
			return create(l.begin(), l.size());
		}
#endif

		template <class VALUE>
		static List<T> createFromElement(VALUE&& e) noexcept
		{
			return CList<T>::createFromElement(Forward<VALUE>(e));
		}
		
		static List<T> createFromElement(const T& e, sl_size count) noexcept
		{
			return CList<T>::createFromElement(e, count);
		}
		
		template <class... ARGS>
		static List<T> createFromElements(ARGS&&... args) noexcept
		{
			T values[] = {Forward<ARGS>(args)...};
			return CList<T>::create(values, sizeof...(args));
		}
		
		template <class VALUE>
		static List<T> createCopy(const List<VALUE>& other) noexcept
		{
			return CList<T>::createCopy(other.ref.ptr);
		}
		
		template <class VALUE>
		static List<T>& from(const List<VALUE>& other) noexcept
		{
			return *(const_cast<List<T>*>(reinterpret_cast<List<T> const*>(&other)));
		}

	public:
		sl_size getCount() const noexcept
		{
			CList<T>* obj = ref.ptr;
			if (obj) {
				return obj->getCount();
			}
			return 0;
		}

		sl_size getCapacity() const noexcept
		{
			CList<T>* obj = ref.ptr;
			if (obj) {
				return obj->getCapacity();
			}
			return 0;
		}

		T* getData() const noexcept
		{
			CList<T>* obj = ref.ptr;
			if (obj) {
				return obj->getData();
			}
			return 0;
		}

		sl_bool isEmpty() const noexcept
		{
			CList<T>* obj = ref.ptr;
			if (obj) {
				return !(obj->getCount());
			}
			return sl_true;
		}

		sl_bool isNotEmpty() const noexcept
		{
			CList<T>* obj = ref.ptr;
			if (obj) {
				return obj->getCount() != 0;
			}
			return sl_false;
		}
		
	public:
		/* unsynchronized function */
		T* getPointerAt(sl_size index) const noexcept
		{
			CList<T>* obj = ref.ptr;
			if (obj) {
				return obj->getPointerAt(index);
			}
			return sl_null;
		}

		sl_bool getAt_NoLock(sl_size index, T* _out = sl_null) const noexcept
		{
			CList<T>* obj = ref.ptr;
			if (obj) {
				return obj->getAt_NoLock(index, _out);
			}
			return sl_false;
		}

		sl_bool getAt(sl_size index, T* _out = sl_null) const noexcept
		{
			CList<T>* obj = ref.ptr;
			if (obj) {
				return obj->getAt(index, _out);
			}
			return sl_false;
		}

		T getValueAt_NoLock(sl_size index) const noexcept
		{
			CList<T>* obj = ref.ptr;
			if (obj) {
				return obj->getValueAt_NoLock(index);
			} else {
				return T();
			}
		}

		T getValueAt(sl_size index) const noexcept
		{
			CList<T>* obj = ref.ptr;
			if (obj) {
				return obj->getValueAt(index);
			} else {
				return T();
			}
		}
	
		T getValueAt_NoLock(sl_size index, const T& def) const noexcept
		{
			CList<T>* obj = ref.ptr;
			if (obj) {
				return obj->getValueAt_NoLock(index, def);
			}
			return def;
		}

		T getValueAt(sl_size index, const T& def) const noexcept
		{
			CList<T>* obj = ref.ptr;
			if (obj) {
				return obj->getValueAt(index, def);
			}
			return def;
		}

		sl_bool getFirst_NoLock(T* _out = sl_null) const noexcept
		{
			CList<T>* obj = ref.ptr;
			if (obj) {
				return obj->getFirst_NoLock(_out);
			}
			return sl_false;
		}
		
		sl_bool getFirst(T* _out = sl_null) const noexcept
		{
			CList<T>* obj = ref.ptr;
			if (obj) {
				return obj->getFirst(_out);
			}
			return sl_false;
		}
		
		T getFirstValue_NoLock() const noexcept
		{
			CList<T>* obj = ref.ptr;
			if (obj) {
				return obj->getFirstValue_NoLock();
			} else {
				return T();
			}
		}
		
		T getFirstValue() const noexcept
		{
			CList<T>* obj = ref.ptr;
			if (obj) {
				return obj->getFirstValue();
			} else {
				return T();
			}
		}
		
		T getFirstValue_NoLock(const T& def) const noexcept
		{
			CList<T>* obj = ref.ptr;
			if (obj) {
				return obj->getFirstValue_NoLock(def);
			}
			return def;
		}
		
		T getFirstValue(const T& def) const noexcept
		{
			CList<T>* obj = ref.ptr;
			if (obj) {
				return obj->getFirstValue(def);
			}
			return def;
		}
		
		sl_bool getLast_NoLock(T* _out = sl_null) const noexcept
		{
			CList<T>* obj = ref.ptr;
			if (obj) {
				return obj->getLast_NoLock(_out);
			}
			return sl_false;
		}
		
		sl_bool getLast(T* _out = sl_null) const noexcept
		{
			CList<T>* obj = ref.ptr;
			if (obj) {
				return obj->getLast(_out);
			}
			return sl_false;
		}
		
		T getLastValue_NoLock() const noexcept
		{
			CList<T>* obj = ref.ptr;
			if (obj) {
				return obj->getLastValue_NoLock();
			} else {
				return T();
			}
		}
		
		T getLastValue() const noexcept
		{
			CList<T>* obj = ref.ptr;
			if (obj) {
				return obj->getLastValue();
			} else {
				return T();
			}
		}
		
		T getLastValue_NoLock(const T& def) const noexcept
		{
			CList<T>* obj = ref.ptr;
			if (obj) {
				return obj->getLastValue_NoLock(def);
			}
			return def;
		}
		
		T getLastValue(const T& def) const noexcept
		{
			CList<T>* obj = ref.ptr;
			if (obj) {
				return obj->getLastValue(def);
			}
			return def;
		}

		template <class VALUE>
		sl_bool setAt_NoLock(sl_size index, VALUE&& value) const noexcept
		{
			CList<T>* obj = ref.ptr;
			if (obj) {
				return obj->setAt_NoLock(index, Forward<VALUE>(value));
			}
			return sl_false;
		}

		template <class VALUE>
		sl_bool setAt(sl_size index, VALUE&& value) const noexcept
		{
			CList<T>* obj = ref.ptr;
			if (obj) {
				return obj->setAt(index, Forward<VALUE>(value));
			}
			return sl_false;
		}

		/* unsynchronized function */
		T& operator[](sl_size_t index) const noexcept
		{
			return (ref->getData())[index];
		}

#ifdef SLIB_SUPPORT_STD_TYPES
		List<T>& operator=(const std::initializer_list<T>& l) noexcept
		{
			ref = CList<T>::create(l.begin(), l.size());
			return *this;
		}
#endif

	public:
		sl_bool setCount_NoLock(sl_size count) noexcept
		{
			CList<T>* obj = ref.ptr;
			if (obj) {
				return obj->setCount_NoLock(count);
			} else {
				if (!count) {
					return sl_true;
				}
				obj = CList<T>::create(count);
				if (obj) {
					ref = obj;
					return sl_true;
				}
			}
			return sl_false;
		}

		sl_bool setCount(sl_size count) noexcept
		{
			CList<T>* obj = ref.ptr;
			if (obj) {
				return obj->setCount(count);
			} else {
				if (!count) {
					return sl_true;
				}
				obj = CList<T>::create(count);
				if (obj) {
					ref = obj;
					return sl_true;
				}
			}
			return sl_false;
		}
		
		sl_bool setCapacity_NoLock(sl_size capacity) noexcept
		{
			CList<T>* obj = ref.ptr;
			if (obj) {
				return obj->setCapacity_NoLock(capacity);
			} else {
				if (!capacity) {
					return sl_true;
				}
				obj = CList<T>::create(0, capacity);
				if (obj) {
					ref = obj;
					return sl_true;
				}
			}
			return sl_false;
		}
		
		sl_bool setCapacity(sl_size capacity) noexcept
		{
			CList<T>* obj = ref.ptr;
			if (obj) {
				return obj->setCount(capacity);
			} else {
				if (!capacity) {
					return sl_true;
				}
				obj = CList<T>::create(0, capacity);
				if (obj) {
					ref = obj;
					return sl_true;
				}
			}
			return sl_false;
		}
				
		sl_bool shrinkToFit_NoLock() const noexcept
		{
			CList<T>* obj = ref.ptr;
			if (obj) {
				return obj->shrinkToFit_NoLock();
			}
			return sl_true;
		}
		
		sl_bool shrinkToFit() const noexcept
		{
			CList<T>* obj = ref.ptr;
			if (obj) {
				return obj->shrinkToFit();
			}
			return sl_true;
		}

		template <class... ARGS>
		sl_bool insert_NoLock(sl_size index, ARGS&&... args) noexcept
		{
			CList<T>* obj = ref.ptr;
			if (obj) {
				return obj->insert_NoLock(index, Forward<ARGS>(args)...);
			} else {
				obj = CList<T>::createFromElement(T(Forward<ARGS>(args)...));
				if (obj) {
					ref = obj;
					return sl_true;
				}
			}
			return sl_false;
		}

		template <class... ARGS>
		sl_bool insert(sl_size index, ARGS&&... args) noexcept
		{
			CList<T>* obj = ref.ptr;
			if (obj) {
				return obj->insert(index, Forward<ARGS>(args)...);
			} else {
				SpinLocker lock(SpinLockPoolForList::get(this));
				obj = ref.ptr;
				if (obj) {
					lock.unlock();
					return obj->insert(index, Forward<ARGS>(args)...);
				}
				obj = CList<T>::create();
				if (obj) {
					ref = obj;
					lock.unlock();
					return obj->insert(index, Forward<ARGS>(args)...);
				}
			}
			return sl_false;
		}
	
		template <class VALUE>
		sl_bool insertElements_NoLock(sl_size index, const VALUE* values, sl_size count) noexcept
		{
			CList<T>* obj = ref.ptr;
			if (obj) {
				return obj->insertElements_NoLock(index, values, count);
			} else {
				obj = CList<T>::create(values, count);
				if (obj) {
					ref = obj;
					return sl_true;
				}
			}
			return sl_false;
		}

		template <class VALUE>
		sl_bool insertElements(sl_size index, const VALUE* values, sl_size count) noexcept
		{
			CList<T>* obj = ref.ptr;
			if (obj) {
				return obj->insertElements(index, values, count);
			} else {
				SpinLocker lock(SpinLockPoolForList::get(this));
				obj = ref.ptr;
				if (obj) {
					lock.unlock();
					return obj->insertElements(index, values, count);
				}
				obj = CList<T>::create();
				if (obj) {
					ref = obj;
					lock.unlock();
					return obj->insertElements(index, values, count);
				}
			}
			return sl_false;
		}
		
		sl_bool insertElements_NoLock(sl_size index, sl_size count, const T& value) noexcept
		{
			CList<T>* obj = ref.ptr;
			if (obj) {
				return obj->insertElements_NoLock(index, count, value);
			} else {
				obj = CList<T>::createFromElement(value, count);
				if (obj) {
					ref = obj;
					return sl_true;
				}
			}
			return sl_false;
		}
		
		sl_bool insertElements(sl_size index, sl_size count, const T& value) noexcept
		{
			CList<T>* obj = ref.ptr;
			if (obj) {
				return obj->insertElements(index, count, value);
			} else {
				SpinLocker lock(SpinLockPoolForList::get(this));
				obj = ref.ptr;
				if (obj) {
					lock.unlock();
					return obj->insertElements(index, count, value);
				}
				obj = CList<T>::create();
				if (obj) {
					ref = obj;
					lock.unlock();
					return obj->insertElements(index, count, value);
				}
			}
			return sl_false;
		}

#ifdef SLIB_SUPPORT_STD_TYPES
		sl_bool insertElements_NoLock(sl_size index, const std::initializer_list<T>& l) noexcept
		{
			return insertElements_NoLock(index, l.begin(), l.size());
		}
		
		sl_bool insertElements(sl_size index, const std::initializer_list<T>& l) noexcept
		{
			return insertElements(index, l.begin(), l.size());
		}
#endif
		
		template <class VALUE>
		sl_bool insertAll_NoLock(sl_size index, const List<VALUE>& _other) noexcept
		{
			CList<VALUE>* other = _other.ref.ptr;
			if (!other) {
				return sl_true;
			}
			CList<T>* obj = ref.ptr;
			if (obj) {
				return obj->insertAll_NoLock(index, other);
			} else {
				obj = CList<T>::create(other->getData(), other->getCount());
				if (obj) {
					ref = obj;
					return sl_true;
				}
			}
			return sl_false;
		}
		
		template <class VALUE>
		sl_bool insertAll_NoLock(sl_size index, const AtomicList<VALUE>& other) noexcept
		{
			return insertAll_NoLock(index, List<VALUE>(other));
		}

		template <class VALUE>
		sl_bool insertAll(sl_size index, const List<VALUE>& _other) noexcept
		{
			CList<VALUE>* other = _other.ref.ptr;
			if (!other) {
				return sl_true;
			}
			CList<T>* obj = ref.ptr;
			if (obj) {
				return obj->insertAll(index, other);
			} else {
				SpinLocker lock(SpinLockPoolForList::get(this));
				obj = ref.ptr;
				if (obj) {
					lock.unlock();
					return obj->insertAll(index, other);
				}
				obj = CList<T>::create();
				if (obj) {
					ref = obj;
					lock.unlock();
					return obj->insertAll(index, other);
				}
			}
			return sl_false;
		}

		template <class VALUE>
		sl_bool insertAll(sl_size index, const AtomicList<VALUE>& other) noexcept
		{
			return insertAll(index, List<VALUE>(other));
		}

		template <class... ARGS>
		sl_bool add_NoLock(ARGS&&... args) noexcept
		{
			CList<T>* obj = ref.ptr;
			if (obj) {
				return obj->add_NoLock(Forward<ARGS>(args)...);
			} else {
				obj = CList<T>::createFromElement(T(Forward<ARGS>(args)...));
				if (obj) {
					ref = obj;
					return sl_true;
				}
			}
			return sl_false;
		}

		template <class... ARGS>
		sl_bool add(ARGS&&... args) noexcept
		{
			CList<T>* obj = ref.ptr;
			if (obj) {
				return obj->add(Forward<ARGS>(args)...);
			} else {
				SpinLocker lock(SpinLockPoolForList::get(this));
				obj = ref.ptr;
				if (obj) {
					lock.unlock();
					return obj->add(Forward<ARGS>(args)...);
				}
				obj = CList<T>::create();
				if (obj) {
					ref = obj;
					lock.unlock();
					return obj->add(Forward<ARGS>(args)...);
				}
			}
			return sl_false;
		}
	
		template <class VALUE>
		sl_bool addElements_NoLock(const VALUE* values, sl_size count) noexcept
		{
			if (!count) {
				return sl_true;
			}
			CList<T>* obj = ref.ptr;
			if (obj) {
				return obj->addElements_NoLock(values, count);
			} else {
				obj = CList<T>::create(values, count);
				if (obj) {
					ref = obj;
					return sl_true;
				}
			}
			return sl_false;
		}

		template <class VALUE>
		sl_bool addElements(const VALUE* values, sl_size count) noexcept
		{
			if (!count) {
				return sl_true;
			}
			CList<T>* obj = ref.ptr;
			if (obj) {
				return obj->addElements(values, count);
			} else {
				SpinLocker lock(SpinLockPoolForList::get(this));
				obj = ref.ptr;
				if (obj) {
					lock.unlock();
					return obj->addElements(values, count);
				}
				obj = CList<T>::create();
				if (obj) {
					ref = obj;
					lock.unlock();
					return obj->addElements(values, count);
				}
			}
			return sl_false;
		}
		
		sl_bool addElements_NoLock(sl_size count, const T& value) noexcept
		{
			if (!count) {
				return sl_true;
			}
			CList<T>* obj = ref.ptr;
			if (obj) {
				return obj->addElements_NoLock(count, value);
			} else {
				obj = CList<T>::createFromElement(value, count);
				if (obj) {
					ref = obj;
					return sl_true;
				}
			}
			return sl_false;
		}
		
		sl_bool addElements(sl_size count, const T& value) noexcept
		{
			if (!count) {
				return sl_true;
			}
			CList<T>* obj = ref.ptr;
			if (obj) {
				return obj->addElements(count, value);
			} else {
				SpinLocker lock(SpinLockPoolForList::get(this));
				obj = ref.ptr;
				if (obj) {
					lock.unlock();
					return obj->addElements(count, value);
				}
				obj = CList<T>::create();
				if (obj) {
					ref = obj;
					lock.unlock();
					return obj->addElements(count, value);
				}
			}
			return sl_false;
		}

#ifdef SLIB_SUPPORT_STD_TYPES
		sl_bool addElements_NoLock(const std::initializer_list<T>& l) noexcept
		{
			return addElements_NoLock(l.begin(), l.size());
		}
		
		sl_bool addElements(const std::initializer_list<T>& l) noexcept
		{
			return addElements(l.begin(), l.size());
		}
#endif

		template <class VALUE>
		sl_bool addAll_NoLock(const List<VALUE>& _other) noexcept
		{
			CList<VALUE>* other = _other.ref.ptr;
			if (!other) {
				return sl_true;
			}
			CList<T>* obj = ref.ptr;
			if (obj) {
				return obj->addAll_NoLock(other);
			} else {
				obj = CList<T>::create(other->getData(), other->getCount());
				if (obj) {
					ref = obj;
					return sl_true;
				}
			}
			return sl_false;
		}

		template <class VALUE>
		sl_bool addAll_NoLock(const AtomicList<VALUE>& _other) noexcept
		{
			return addAll_NoLock(List<VALUE>(_other));
		}

		template <class VALUE>
		sl_bool addAll(const List<VALUE>& _other) noexcept
		{
			CList<VALUE>* other = _other.ref.ptr;
			if (!other) {
				return sl_true;
			}
			CList<T>* obj = ref.ptr;
			if (obj) {
				return obj->addAll(other);
			} else {
				SpinLocker lock(SpinLockPoolForList::get(this));
				obj = ref.ptr;
				if (obj) {
					lock.unlock();
					return obj->addAll(other);
				}
				obj = CList<T>::create();
				if (obj) {
					ref = obj;
					lock.unlock();
					return obj->addAll(other);
				}
			}
			return sl_false;
		}

		template <class VALUE>
		sl_bool addAll(const AtomicList<VALUE>& _other) noexcept
		{
			return addAll(List<VALUE>(_other));
		}

		template < class VALUE, class EQUALS = Equals<T, typename RemoveConstReference<VALUE>::Type> >
		sl_bool addIfNotExist_NoLock(VALUE&& value, const EQUALS& equals = EQUALS()) noexcept
		{
			CList<T>* obj = ref.ptr;
			if (obj) {
				return obj->addIfNotExist_NoLock(Forward<VALUE>(value), equals);
			} else {
				obj = CList<T>::createFromElement(Forward<VALUE>(value));
				if (obj) {
					ref = obj;
					return sl_true;
				}
			}
			return sl_false;
		}

		template < class VALUE, class EQUALS = Equals<T, typename RemoveConstReference<VALUE>::Type> >
		sl_bool addIfNotExist(VALUE&& value, const EQUALS& equals = EQUALS()) noexcept
		{
			CList<T>* obj = ref.ptr;
			if (obj) {
				return obj->addIfNotExist(Forward<VALUE>(value), equals);
			} else {
				SpinLocker lock(SpinLockPoolForList::get(this));
				obj = ref.ptr;
				if (obj) {
					lock.unlock();
					return obj->addIfNotExist(Forward<VALUE>(value), equals);
				}
				obj = CList<T>::create();
				if (obj) {
					ref = obj;
					lock.unlock();
					return obj->addIfNotExist(Forward<VALUE>(value), equals);
				}
			}
			return sl_false;
		}

		sl_bool removeAt_NoLock(sl_size index, T* outValue = sl_null) const noexcept
		{
			CList<T>* obj = ref.ptr;
			if (obj) {
				return obj->removeAt_NoLock(index, outValue);
			}
			return sl_false;
		}

		sl_bool removeAt(sl_size index, T* outValue = sl_null) const noexcept
		{
			CList<T>* obj = ref.ptr;
			if (obj) {
				return obj->removeAt(index, outValue);
			}
			return sl_false;
		}

		sl_size removeRange_NoLock(sl_size index, sl_size count) const noexcept
		{
			CList<T>* obj = ref.ptr;
			if (obj) {
				return obj->removeRange_NoLock(index, count);
			}
			return 0;
		}

		sl_size removeRange(sl_size index, sl_size count) const noexcept
		{
			CList<T>* obj = ref.ptr;
			if (obj) {
				return obj->removeRange(index, count);
			}
			return 0;
		}

		template < class VALUE, class EQUALS = Equals<T, VALUE> >
		sl_bool remove_NoLock(const VALUE& value, const EQUALS& equals = EQUALS()) const noexcept
		{
			CList<T>* obj = ref.ptr;
			if (obj) {
				return obj->remove_NoLock(value, equals);
			}
			return sl_false;
		}

		template < class VALUE, class EQUALS = Equals<T, VALUE> >
		sl_bool remove(const VALUE& value, const EQUALS& equals = EQUALS()) const noexcept
		{
			CList<T>* obj = ref.ptr;
			if (obj) {
				return obj->remove(value, equals);
			}
			return sl_false;
		}

		template < class VALUE, class EQUALS = Equals<T, VALUE> >
		sl_size removeValues_NoLock(const VALUE& value, const EQUALS& equals = EQUALS()) const noexcept
		{
			CList<T>* obj = ref.ptr;
			if (obj) {
				return obj->removeValues_NoLock(value, equals);
			}
			return 0;
		}

		template < class VALUE, class EQUALS = Equals<T, VALUE> >
		sl_size removeValues(const VALUE& value, const EQUALS& equals = EQUALS()) const noexcept
		{
			CList<T>* obj = ref.ptr;
			if (obj) {
				return obj->removeValues(value, equals);
			}
			return 0;
		}

		template <class PREDICATE>
		sl_bool removeIf_NoLock(const PREDICATE& p) noexcept
		{
			CList<T>* obj = ref.ptr;
			if (obj) {
				return obj->removeIf_NoLock(p);
			}
			return sl_false;
		}
		
		template <class PREDICATE>
		sl_bool removeIf(const PREDICATE& p) noexcept
		{
			CList<T>* obj = ref.ptr;
			if (obj) {
				return obj->removeIf(p);
			}
			return sl_false;
		}
		
		template <class PREDICATE>
		sl_size removeElementsIf_NoLock(const PREDICATE& p) noexcept
		{
			CList<T>* obj = ref.ptr;
			if (obj) {
				return obj->removeElementsIf_NoLock(p);
			}
			return 0;
		}
		
		template <class PREDICATE>
		sl_size removeElementsIf(const PREDICATE& p) noexcept
		{
			CList<T>* obj = ref.ptr;
			if (obj) {
				return obj->removeElementsIf(p);
			}
			return 0;
		}

		sl_size removeAll_NoLock() const noexcept
		{
			CList<T>* obj = ref.ptr;
			if (obj) {
				return obj->removeAll_NoLock();
			}
			return 0;
		}

		sl_size removeAll() const noexcept
		{
			CList<T>* obj = ref.ptr;
			if (obj) {
				return obj->removeAll();
			}
			return 0;
		}

		sl_bool popFront_NoLock(T* _out = sl_null) const noexcept
		{
			CList<T>* obj = ref.ptr;
			if (obj) {
				return obj->popFront_NoLock(_out);
			}
			return sl_false;
		}

		sl_bool popFront(T* _out = sl_null) const noexcept
		{
			CList<T>* obj = ref.ptr;
			if (obj) {
				return obj->popFront(_out);
			}
			return sl_false;
		}

		sl_size popFrontElements_NoLock(sl_size count) const noexcept
		{
			CList<T>* obj = ref.ptr;
			if (obj) {
				return obj->popFrontElements_NoLock(count);
			}
			return 0;
		}

		sl_size popFrontElements(sl_size count) const noexcept
		{
			CList<T>* obj = ref.ptr;
			if (obj) {
				return obj->popFrontElements(count);
			}
			return 0;
		}

		sl_bool popBack_NoLock(T* _out = sl_null) const noexcept
		{
			CList<T>* obj = ref.ptr;
			if (obj) {
				return obj->popBack_NoLock(_out);
			}
			return sl_false;
		}

		sl_bool popBack(T* _out = sl_null) const noexcept
		{
			CList<T>* obj = ref.ptr;
			if (obj) {
				return obj->popBack(_out);
			}
			return sl_false;
		}

		sl_size popBackElements_NoLock(sl_size count) const noexcept
		{
			CList<T>* obj = ref.ptr;
			if (obj) {
				return obj->popBackElements_NoLock(count);
			}
			return 0;
		}

		sl_size popBackElements(sl_size count) const noexcept
		{
			CList<T>* obj = ref.ptr;
			if (obj) {
				return obj->popBackElements(count);
			}
			return 0;
		}
		
		template < class VALUE, class ARG = Equals<T, VALUE> >
		sl_reg indexOf_NoLock(const VALUE& value, const ARG& arg = ARG()) const noexcept
		{
			CList<T>* obj = ref.ptr;
			if (obj) {
				return obj->indexOf_NoLock(value, arg);
			}
			return -1;
		}
		
		template < class VALUE, class ARG = Equals<T, VALUE> >
		sl_reg indexOf(const VALUE& value, const ARG& arg = ARG()) const noexcept
		{
			CList<T>* obj = ref.ptr;
			if (obj) {
				return obj->indexOf(value, arg);
			}
			return -1;
		}
		
		template <class VALUE, class EQUALS>
		sl_reg indexOf_NoLock(const VALUE& value, const EQUALS& equals, sl_reg startIndex) const noexcept
		{
			CList<T>* obj = ref.ptr;
			if (obj) {
				return obj->indexOf_NoLock(value, equals, startIndex);
			}
			return -1;
		}
		
		template <class VALUE, class EQUALS>
		sl_reg indexOf(const VALUE& value, const EQUALS& equals, sl_reg startIndex) const noexcept
		{
			CList<T>* obj = ref.ptr;
			if (obj) {
				return obj->indexOf(value, equals, startIndex);
			}
			return -1;
		}
		
		template < class VALUE, class ARG = Equals<T, VALUE> >
		sl_reg lastIndexOf_NoLock(const VALUE& value, const ARG& arg = ARG()) const noexcept
		{
			CList<T>* obj = ref.ptr;
			if (obj) {
				return obj->lastIndexOf_NoLock(value, arg);
			}
			return -1;
		}
		
		template < class VALUE, class ARG = Equals<T, VALUE> >
		sl_reg lastIndexOf(const VALUE& value, const ARG& arg = ARG()) const noexcept
		{
			CList<T>* obj = ref.ptr;
			if (obj) {
				return obj->lastIndexOf(value, arg);
			}
			return -1;
		}
		
		template <class VALUE, class EQUALS>
		sl_reg lastIndexOf_NoLock(const VALUE& value, const EQUALS& equals, sl_reg startIndex) const noexcept
		{
			CList<T>* obj = ref.ptr;
			if (obj) {
				return obj->lastIndexOf_NoLock(value, equals, startIndex);
			}
			return -1;
		}
		
		template <class VALUE, class EQUALS>
		sl_reg lastIndexOf(const VALUE& value, const EQUALS& equals, sl_reg startIndex) const noexcept
		{
			CList<T>* obj = ref.ptr;
			if (obj) {
				return obj->lastIndexOf(value, equals, startIndex);
			}
			return -1;
		}

		template < class VALUE, class EQUALS = Equals<T, VALUE> >
		sl_bool contains_NoLock(const VALUE& value, const EQUALS& equals = EQUALS()) const noexcept
		{
			CList<T>* obj = ref.ptr;
			if (obj) {
				return obj->contains_NoLock(value, equals);
			}
			return sl_false;
		}

		template < class VALUE, class EQUALS = Equals<T, VALUE> >
		sl_bool contains(const VALUE& value, const EQUALS& equals = EQUALS()) const noexcept
		{
			CList<T>* obj = ref.ptr;
			if (obj) {
				return obj->contains(value, equals);
			}
			return sl_false;
		}

		List<T> duplicate_NoLock() const noexcept
		{
			CList<T>* obj = ref.ptr;
			if (obj) {
				return obj->duplicate_NoLock();
			}
			return sl_null;
		}

		List<T> duplicate() const noexcept
		{
			CList<T>* obj = ref.ptr;
			if (obj) {
				return obj->duplicate();
			}
			return sl_null;
		}

		Array<T> toArray_NoLock() const noexcept
		{
			CList<T>* obj = ref.ptr;
			if (obj) {
				return obj->toArray_NoLock();
			}
			return sl_null;
		}

		Array<T> toArray() const noexcept
		{
			CList<T>* obj = ref.ptr;
			if (obj) {
				return obj->toArray();
			}
			return sl_null;
		}
		
		template < class COMPARE = Compare<T> >
		void sort_NoLock(const COMPARE& compare = COMPARE()) const noexcept
		{
			CList<T>* obj = ref.ptr;
			if (obj) {
				obj->sort_NoLock(compare);
			}
		}
		
		template < class COMPARE = Compare<T> >
		void sort(const COMPARE& compare = COMPARE()) const noexcept
		{
			CList<T>* obj = ref.ptr;
			if (obj) {
				obj->sort(compare);
			}
		}
		
		template < class COMPARE = Compare<T> >
		void sortDesc_NoLock(const COMPARE& compare = COMPARE()) const noexcept
		{
			CList<T>* obj = ref.ptr;
			if (obj) {
				obj->sortDesc_NoLock(compare);
			}
		}
		
		template < class COMPARE = Compare<T> >
		void sortDesc(const COMPARE& compare = COMPARE()) const noexcept
		{
			CList<T>* obj = ref.ptr;
			if (obj) {
				obj->sortDesc(compare);
			}
		}
		
		void reverse_NoLock() const noexcept
		{
			CList<T>* obj = ref.ptr;
			if (obj) {
				obj->reverse_NoLock();
			}
		}
		
		void reverse() const noexcept
		{
			CList<T>* obj = ref.ptr;
			if (obj) {
				obj->reverse();
			}
		}
		
		List<T> slice_NoLock(sl_size index, sl_size count = SLIB_SIZE_MAX) const noexcept
		{
			CList<T>* obj = ref.ptr;
			if (obj) {
				return obj->slice_NoLock(index, count);
			}
			return sl_null;
		}
		
		List<T> slice(sl_size index, sl_size count = SLIB_SIZE_MAX) const noexcept
		{
			CList<T>* obj = ref.ptr;
			if (obj) {
				return obj->slice(index, count);
			}
			return sl_null;
		}

		Ref<Collection> toCollection() const noexcept
		{
			CList<T>* obj = ref.ptr;
			if (obj) {
				return obj->toCollection();
			}
			return sl_null;
		}

		const Mutex* getLocker() const noexcept
		{
			CList<T>* obj = ref.ptr;
			if (obj) {
				return obj->getLocker();
			} else {
				return sl_null;
			}
		}

		// range-based for loop
		T* begin() const noexcept
		{
			CList<T>* obj = ref.ptr;
			if (obj) {
				return obj->begin();
			}
			return sl_null;
		}

		T* end() const noexcept
		{
			CList<T>* obj = ref.ptr;
			if (obj) {
				return obj->end();
			}
			return sl_null;
		}

	};
	
	template <class T>
	class SLIB_EXPORT Atomic< List<T> >
	{
	public:
		typedef T ELEMENT_TYPE;
		
	public:
		AtomicRef< CList<T> > ref;
		SLIB_ATOMIC_REF_WRAPPER(CList<T>)
		
	public:
		Atomic(sl_size count) noexcept: ref(CList<T>::create(count)) {}
		
		Atomic(sl_size count, sl_size capacity) noexcept: ref(CList<T>::create(count, capacity)) {}
		
		Atomic(sl_size count, sl_size capacity, const T& initialValue) noexcept: ref(CList<T>::create(count, capacity, initialValue)) {}
		
		template <class VALUE>
		Atomic(const VALUE* values, sl_size count) noexcept: ref(CList<T>::create(values, count)) {}
		
#ifdef SLIB_SUPPORT_STD_TYPES
		Atomic(const std::initializer_list<T>& l) noexcept: ref(CList<T>::create(l.begin(), l.size())) {}
#endif

	public:
		template <class VALUE>
		static Atomic< List<T> >& from(const Atomic< List<VALUE> >& other) noexcept
		{
			return *(const_cast<Atomic< List<T> >*>(reinterpret_cast<Atomic< List<T> > const*>(&other)));
		}

		sl_size getCount() const noexcept
		{
			Ref< CList<T> > obj(ref);
			if (obj.isNotNull()) {
				return obj->getCount();
			}
			return 0;
		}

		sl_bool isEmpty() const noexcept
		{
			Ref< CList<T> > obj(ref);
			if (obj.isNotNull()) {
				return !(obj->getCount());
			}
			return sl_true;
		}

		sl_bool isNotEmpty() const noexcept
		{
			Ref< CList<T> > obj(ref);
			if (obj.isNotNull()) {
				return obj->getCount() != 0;
			}
			return sl_false;
		}

	public:
		sl_bool getAt(sl_size index, T* _out = sl_null) const noexcept
		{
			Ref< CList<T> > obj(ref);
			if (obj.isNotNull()) {
				return obj->getAt(index, _out);
			}
			return sl_false;
		}

		T getValueAt(sl_size index) const noexcept
		{
			Ref< CList<T> > obj(ref);
			if (obj.isNotNull()) {
				return obj->getValueAt(index);
			} else {
				return T();
			}
		}

		T getValueAt(sl_size index, const T& def) const noexcept
		{
			Ref< CList<T> > obj(ref);
			if (obj.isNotNull()) {
				return obj->getValueAt(index, def);
			}
			return def;
		}

		sl_bool getFirst(T* _out = sl_null) const noexcept
		{
			Ref< CList<T> > obj(ref);
			if (obj.isNotNull()) {
				return obj->getFirst(_out);
			}
			return sl_false;
		}
		
		T getFirstValue() const noexcept
		{
			Ref< CList<T> > obj(ref);
			if (obj.isNotNull()) {
				return obj->getFirstValue();
			} else {
				return T();
			}
		}
		
		T getFirstValue(const T& def) const noexcept
		{
			Ref< CList<T> > obj(ref);
			if (obj.isNotNull()) {
				return obj->getFirstValue(def);
			}
			return def;
		}
		
		sl_bool getLast(T* _out = sl_null) const noexcept
		{
			Ref< CList<T> > obj(ref);
			if (obj.isNotNull()) {
				return obj->getLast(_out);
			}
			return sl_false;
		}
		
		T getLastValue() const noexcept
		{
			Ref< CList<T> > obj(ref);
			if (obj.isNotNull()) {
				return obj->getLastValue();
			} else {
				return T();
			}
		}
		
		T getLastValue(const T& def) const noexcept
		{
			Ref< CList<T> > obj(ref);
			if (obj.isNotNull()) {
				return obj->getLastValue(def);
			}
			return def;
		}

		template <class VALUE>
		sl_bool setAt(sl_size index, VALUE&& value) const noexcept
		{
			Ref< CList<T> > obj(ref);
			if (obj.isNotNull()) {
				return obj->setAt(index, Forward<VALUE>(value));
			}
			return sl_false;
		}

		T operator[](sl_size_t index) const noexcept
		{
			Ref< CList<T> > obj(ref);
			if (obj.isNotNull()) {
				return obj->getValueAt(index);
			} else {
				return T();
			}
		}
		
#ifdef SLIB_SUPPORT_STD_TYPES
		Atomic& operator=(const std::initializer_list<T>& l) noexcept
		{
			ref = CList<T>::create(l.begin(), l.size());
			return *this;
		}
#endif

	public:
		sl_bool setCount(sl_size count) noexcept
		{
			Ref< CList<T> > obj(ref);
			if (obj.isNotNull()) {
				return obj->setCount(count);
			} else {
				SpinLocker lock(SpinLockPoolForList::get(this));
				obj = ref;
				if (obj.isNotNull()) {
					lock.unlock();
					return obj->setCount(count);
				}
				obj = CList<T>::create();
				if (obj.isNotNull()) {
					ref = obj;
					lock.unlock();
					return obj->setCount(count);
				}
			}
			return sl_false;
		}
		
		sl_bool setCapacity(sl_size capacity) noexcept
		{
			Ref< CList<T> > obj(ref);
			if (obj.isNotNull()) {
				return obj->setCapacity(capacity);
			} else {
				SpinLocker lock(SpinLockPoolForList::get(this));
				obj = ref;
				if (obj.isNotNull()) {
					lock.unlock();
					return obj->setCapacity(capacity);
				}
				obj = CList<T>::create();
				if (obj.isNotNull()) {
					ref = obj;
					lock.unlock();
					return obj->setCapacity(capacity);
				}
			}
			return sl_false;
		}
		
		sl_bool shrinkToFit() const noexcept
		{
			Ref< CList<T> > obj(ref);
			if (obj.isNotNull()) {
				return obj->shrinkToFit();
			}
			return sl_true;
		}

		template <class... ARGS>
		sl_bool insert(sl_size index, ARGS&&... args) noexcept
		{
			Ref< CList<T> > obj(ref);
			if (obj.isNotNull()) {
				return obj->insert(index, Forward<ARGS>(args)...);
			} else {
				SpinLocker lock(SpinLockPoolForList::get(this));
				obj = ref;
				if (obj.isNotNull()) {
					lock.unlock();
					return obj->insert(index, Forward<ARGS>(args)...);
				}
				obj = CList<T>::create();
				if (obj.isNotNull()) {
					ref = obj;
					lock.unlock();
					return obj->insert(index, Forward<ARGS>(args)...);
				}
			}
			return sl_false;
		}

		template <class VALUE>
		sl_bool insertElements(sl_size index, const VALUE* values, sl_size count) noexcept
		{
			Ref< CList<T> > obj(ref);
			if (obj.isNotNull()) {
				return obj->insertElements(index, values, count);
			} else {
				SpinLocker lock(SpinLockPoolForList::get(this));
				obj = ref;
				if (obj.isNotNull()) {
					lock.unlock();
					return obj->insertElements(index, values, count);
				}
				obj = CList<T>::create();
				if (obj.isNotNull()) {
					ref = obj;
					lock.unlock();
					return obj->insertElements(index, values, count);
				}
			}
			return sl_false;
		}
		
		sl_bool insertElements(sl_size index, sl_size count, const T& value) noexcept
		{
			Ref< CList<T> > obj(ref);
			if (obj.isNotNull()) {
				return obj->insertElements(index, count, value);
			} else {
				SpinLocker lock(SpinLockPoolForList::get(this));
				obj = ref;
				if (obj.isNotNull()) {
					lock.unlock();
					return obj->insertElements(index, count, value);
				}
				obj = CList<T>::create();
				if (obj.isNotNull()) {
					ref = obj;
					lock.unlock();
					return obj->insertElements(index, count, value);
				}
			}
			return sl_false;
		}

#ifdef SLIB_SUPPORT_STD_TYPES
		sl_bool insertElements(sl_size index, const std::initializer_list<T>& l) noexcept
		{
			return insertElements(index, l.begin(), l.size());
		}
#endif
		
		template <class VALUE>
		sl_bool insertAll(sl_size index, const List<VALUE>& _other) noexcept
		{
			CList<VALUE>* other = _other.ref.ptr;
			if (!other) {
				return sl_true;
			}
			Ref< CList<T> > obj(ref);
			if (obj.isNotNull()) {
				return obj->insertAll(index, other);
			} else {
				SpinLocker lock(SpinLockPoolForList::get(this));
				obj = ref;
				if (obj.isNotNull()) {
					lock.unlock();
					return obj->insertAll(index, other);
				}
				obj = CList<T>::create();
				if (obj.isNotNull()) {
					ref = obj;
					lock.unlock();
					return obj->insertAll(index, other);
				}
			}
			return sl_false;
		}

		template <class VALUE>
		sl_bool insertAll(sl_size index, const AtomicList<VALUE>& other) noexcept
		{
			return insertAll(index, List<VALUE>(other));
		}

		template <class... ARGS>
		sl_bool add(ARGS&&... args) noexcept
		{
			Ref< CList<T> > obj(ref);
			if (obj.isNotNull()) {
				return obj->add(Forward<ARGS>(args)...);
			} else {
				SpinLocker lock(SpinLockPoolForList::get(this));
				obj = ref;
				if (obj.isNotNull()) {
					lock.unlock();
					return obj->add(Forward<ARGS>(args)...);
				}
				obj = CList<T>::create();
				if (obj.isNotNull()) {
					ref = obj;
					lock.unlock();
					return obj->add(Forward<ARGS>(args)...);
				}
			}
			return sl_false;
		}

		template <class VALUE>
		sl_bool addElements(const VALUE* values, sl_size count) noexcept
		{
			if (!count) {
				return sl_true;
			}
			Ref< CList<T> > obj(ref);
			if (obj.isNotNull()) {
				return obj->addElements(values, count);
			} else {
				SpinLocker lock(SpinLockPoolForList::get(this));
				obj = ref;
				if (obj.isNotNull()) {
					lock.unlock();
					return obj->addElements(values, count);
				}
				obj = CList<T>::create();
				if (obj.isNotNull()) {
					ref = obj;
					lock.unlock();
					return obj->addElements(values, count);
				}
			}
			return sl_false;
		}
		
		sl_bool addElements(sl_size count, const T& value) noexcept
		{
			if (!count) {
				return sl_true;
			}
			Ref< CList<T> > obj(ref);
			if (obj.isNotNull()) {
				return obj->addElements(count, value);
			} else {
				SpinLocker lock(SpinLockPoolForList::get(this));
				obj = ref;
				if (obj.isNotNull()) {
					lock.unlock();
					return obj->addElements(count, value);
				}
				obj = CList<T>::create();
				if (obj.isNotNull()) {
					ref = obj;
					lock.unlock();
					return obj->addElements(count, value);
				}
			}
			return sl_false;
		}

#ifdef SLIB_SUPPORT_STD_TYPES
		sl_bool addElements(const std::initializer_list<T>& l) noexcept
		{
			return addElements(l.begin(), l.size());
		}
#endif

		template <class VALUE>
		sl_bool addAll(const List<VALUE>& _other) noexcept
		{
			CList<VALUE>* other = _other.ref.ptr;
			if (!other) {
				return sl_true;
			}
			Ref< CList<T> > obj(ref);
			if (obj.isNotNull()) {
				return obj->addAll(other);
			} else {
				SpinLocker lock(SpinLockPoolForList::get(this));
				obj = ref;
				if (obj.isNotNull()) {
					lock.unlock();
					return obj->addAll(other);
				}
				obj = CList<T>::create();
				if (obj.isNotNull()) {
					ref = obj;
					lock.unlock();
					return obj->addAll(other);
				}
			}
			return sl_false;
		}

		template <class VALUE>
		sl_bool addAll(const AtomicList<VALUE>& _other) noexcept
		{
			return addAll(List<VALUE>(_other));
		}

		template < class VALUE, class EQUALS = Equals<T, typename RemoveConstReference<VALUE>::Type> >
		sl_bool addIfNotExist(VALUE&& value, const EQUALS& equals = EQUALS()) noexcept
		{
			Ref< CList<T> > obj(ref);
			if (obj.isNotNull()) {
				return obj->addIfNotExist(Forward<VALUE>(value), equals);
			} else {
				SpinLocker lock(SpinLockPoolForList::get(this));
				obj = ref;
				if (obj.isNotNull()) {
					lock.unlock();
					return obj->addIfNotExist(Forward<VALUE>(value), equals);
				}
				obj = CList<T>::create();
				if (obj.isNotNull()) {
					ref = obj;
					lock.unlock();
					return obj->addIfNotExist(Forward<VALUE>(value), equals);
				}
			}
			return sl_false;
		}

		sl_bool removeAt(sl_size index, T* outValue = sl_null) const noexcept
		{
			Ref< CList<T> > obj(ref);
			if (obj.isNotNull()) {
				return obj->removeAt(index, outValue);
			}
			return sl_false;
		}

		sl_size removeRange(sl_size index, sl_size count) const noexcept
		{
			Ref< CList<T> > obj(ref);
			if (obj.isNotNull()) {
				return obj->removeRange(index, count);
			}
			return 0;
		}

		template < class VALUE, class EQUALS = Equals<T, VALUE> >
		sl_bool remove(const VALUE& value, const EQUALS& equals = EQUALS()) const noexcept
		{
			Ref< CList<T> > obj(ref);
			if (obj.isNotNull()) {
				return obj->remove(value, equals);
			}
			return sl_false;
		}

		template < class VALUE, class EQUALS = Equals<T, VALUE> >
		sl_size removeValues(const VALUE& value, const EQUALS& equals = EQUALS()) const noexcept
		{
			Ref< CList<T> > obj(ref);
			if (obj.isNotNull()) {
				return obj->removeValues(value, equals);
			}
			return 0;
		}

		template <class PREDICATE>
		sl_bool removeIf(const PREDICATE& p) noexcept
		{
			Ref< CList<T> > obj(ref);
			if (obj.isNotNull()) {
				return obj->removeIf(p);
			}
			return sl_false;
		}
		
		template <class PREDICATE>
		sl_size removeElementsIf(const PREDICATE& p) noexcept
		{
			Ref< CList<T> > obj(ref);
			if (obj.isNotNull()) {
				return obj->removeElementsIf(p);
			}
			return sl_false;
		}
		
		sl_size removeAll() const noexcept
		{
			Ref< CList<T> > obj(ref);
			if (obj.isNotNull()) {
				return obj->removeAll();
			}
			return 0;
		}

		sl_bool popFront(T* _out = sl_null) const noexcept
		{
			Ref< CList<T> > obj(ref);
			if (obj.isNotNull()) {
				return obj->popFront(_out);
			}
			return sl_false;
		}

		sl_size popFrontElements(sl_size count) const noexcept
		{
			Ref< CList<T> > obj(ref);
			if (obj.isNotNull()) {
				return obj->popFrontElements(count);
			}
			return 0;
		}

		sl_bool popBack(T* _out = sl_null) const noexcept
		{
			Ref< CList<T> > obj(ref);
			if (obj.isNotNull()) {
				return obj->popBack(_out);
			}
			return sl_false;
		}

		sl_size popBackElements(sl_size count) const noexcept
		{
			Ref< CList<T> > obj(ref);
			if (obj.isNotNull()) {
				return obj->popBackElements(count);
			}
			return 0;
		}
		
		
		template < class VALUE, class ARG = Equals<T, VALUE> >
		sl_reg indexOf(const VALUE& value, const ARG& arg = ARG()) const noexcept
		{
			Ref< CList<T> > obj(ref);
			if (obj.isNotNull()) {
				return obj->indexOf(value, arg);
			}
			return -1;
		}
		
		template <class VALUE, class EQUALS>
		sl_reg indexOf(const VALUE& value, const EQUALS& equals, sl_reg startIndex) const noexcept
		{
			Ref< CList<T> > obj(ref);
			if (obj.isNotNull()) {
				return obj->indexOf(value, equals, startIndex);
			}
			return -1;
		}
		
		template < class VALUE, class ARG = Equals<T, VALUE> >
		sl_reg lastIndexOf(const VALUE& value, const ARG& arg = ARG()) const noexcept
		{
			Ref< CList<T> > obj(ref);
			if (obj.isNotNull()) {
				return obj->lastIndexOf(value, arg);
			}
			return -1;
		}
		
		template <class VALUE, class EQUALS>
		sl_reg lastIndexOf(const VALUE& value, const EQUALS& equals, sl_reg startIndex) const noexcept
		{
			Ref< CList<T> > obj(ref);
			if (obj.isNotNull()) {
				return obj->lastIndexOf(value, equals, startIndex);
			}
			return -1;
		}

		template < class VALUE, class EQUALS = Equals<T, VALUE> >
		sl_bool contains(const VALUE& value, const EQUALS& equals = EQUALS()) const noexcept
		{
			Ref< CList<T> > obj(ref);
			if (obj.isNotNull()) {
				return obj->contains(value);
			}
			return sl_false;
		}

		List<T> duplicate() const noexcept
		{
			Ref< CList<T> > obj(ref);
			if (obj.isNotNull()) {
				return obj->duplicate();
			}
			return sl_null;
		}

		Array<T> toArray() const noexcept
		{
			Ref< CList<T> > obj(ref);
			if (obj.isNotNull()) {
				return obj->toArray();
			}
			return sl_null;
		}
		
		template < class COMPARE = Compare<T> >
		void sort(const COMPARE& compare = COMPARE()) const noexcept
		{
			Ref< CList<T> > obj(ref);
			if (obj.isNotNull()) {
				obj->sort(compare);
			}
		}
		
		template < class COMPARE = Compare<T> >
		void sortDesc(const COMPARE& compare = COMPARE()) const noexcept
		{
			Ref< CList<T> > obj(ref);
			if (obj.isNotNull()) {
				obj->sortDesc(compare);
			}
		}
		
		void reverse() const noexcept
		{
			Ref< CList<T> > obj(ref);
			if (obj.isNotNull()) {
				obj->reverse();
			}
		}
		
		List<T> slice(sl_size index, sl_size count = SLIB_SIZE_MAX) const noexcept
		{
			Ref< CList<T> > obj(ref);
			if (obj.isNotNull()) {
				return obj->slice(index, count);
			}
			return sl_null;
		}

		Ref<Collection> toCollection() const noexcept
		{
			Ref< CList<T> > obj(ref);
			if (obj.isNotNull()) {
				return obj->toCollection();
			}
			return sl_null;
		}

		// range-based for loop
		ArrayPosition<T> begin() const noexcept
		{
			Ref< CList<T> > obj(ref);
			if (obj.isNotNull()) {
				return ArrayPosition<T>(obj->getData(), obj->getCount(), obj.get());
			}
			return ArrayPosition<T>();
		}

		ArrayPosition<T> end() const noexcept
		{
			return ArrayPosition<T>();
		}

	};
	

	template <class T>
	class ListParam;

	template <class T>
	class SLIB_EXPORT ListLocker : public ObjectLocker
	{
	public:
		T* data;
		sl_size count;
		List<T> list;

	public:
		ListLocker(List<T>&& _list) noexcept: ObjectLocker(_list.ref.ptr), list(Move(_list))
		{
			data = list.getData();
			count = list.getCount();
		}

		ListLocker(const List<T>& _list) noexcept: ObjectLocker(_list.ref.ptr), list(_list)
		{
			data = list.getData();
			count = list.getCount();
		}

		ListLocker(const AtomicList<T>& _list) noexcept: ListLocker(List<T>(_list))
		{
		}

		ListLocker(const CList<T>& _list) noexcept: ObjectLocker(&_list)
		{
			data = _list.getData();
			count = _list.getCount();
		}

		ListLocker(const ListParam<T>& _list) noexcept: ObjectLocker(_list.getObject()), list(_list._getList())
		{
			data = _list.getData();
			count = _list.getCount();
		}

		~ListLocker() noexcept
		{
			unlock();
		}

	public:
		T& operator[](sl_reg index) const noexcept
		{
			return data[index];
		}

		// range-based for loop
		T* begin() const noexcept
		{
			return data;
		}

		T* end() const noexcept
		{
			return data + count;
		}

	};
	

	template <class T>
	class SLIB_EXPORT ListElements
	{
	public:
		T* data;
		sl_size count;
		List<T> list;

	public:
		ListElements(List<T>&& _list) noexcept: list(Move(_list))
		{
			data = list.getData();
			count = list.getCount();
		}

		ListElements(const List<T>& _list) noexcept: list(_list)
		{
			data = list.getData();
			count = list.getCount();
		}

		ListElements(const AtomicList<T>& _list) noexcept: list(_list)
		{
			data = list.getData();
			count = list.getCount();
		}

		ListElements(const CList<T>& _list) noexcept
		{
			data = _list.getData();
			count = _list.getCount();
		}

		ListElements(const ListParam<T>& _list) noexcept: list(_list._getList())
		{
			data = _list.getData();
			count = _list.getCount();
		}

	public:
		T& operator[](sl_reg index) const noexcept
		{
			return data[index];
		}

		// range-based for loop
		T* begin() const noexcept
		{
			return data;
		}

		T* end() const noexcept
		{
			return data + count;
		}

	};
	
	enum class ListType : sl_reg
	{
		List_Ref = (sl_reg)-1,
		List_NoRef = (sl_reg)-2,
		CList = (sl_reg)-3
	};

	template <class T>
	class SLIB_EXPORT ListParam
	{
	private:
		union {
			void* _value;
			
			const T* _data;
			const CList<T>* _list;
		};
		union {
			ListType _type;
			sl_reg _count;
		};

	public:
		ListParam() noexcept: _value(sl_null), _count(0)
		{
		}

		ListParam(sl_null_t) noexcept: _value(sl_null), _count(0)
		{
		}

		ListParam(ListParam&& other) noexcept
		{
			_value = other._value;
			_count = other._count;
			other._value = sl_null;
			other._count = 0;
		}

		ListParam(const ListParam& other) noexcept
		{
			_count = other._count;
			_value = other._value;
			if (_count == (sl_reg)(ListType::List_Ref)) {
				_count = (sl_reg)(ListType::List_NoRef);
			}
		}

		ListParam(List<T>&& list) noexcept
		{
			if (list.isNotNull()) {
				_count = (sl_reg)(ListType::List_Ref);
				new (reinterpret_cast<List<T>*>(&_value)) List<T>(Move(list));
			} else {
				_value = sl_null;
				_count = 0;
			}
		}

		ListParam(const List<T>& list) noexcept
		{
			_value = list.ref.ptr;
			if (_value) {
				_count = (sl_reg)(ListType::List_NoRef);
			} else {
				_count = 0;
			}
		}
		
		ListParam(AtomicList<T>&& list) noexcept: ListParam(List<T>(Move(list)))
		{
		}

		ListParam(const AtomicList<T>& list) noexcept: ListParam(List<T>(list))
		{
		}

		ListParam(const CList<T>& list) noexcept
		{
			_count = (sl_reg)(ListType::CList);
			_value = (void*)(&list);
		}

		ListParam(const T* data, sl_size count) noexcept
		{
			if (data && count) {
				_count = count & SLIB_SIZE_MASK_NO_SIGN_BITS;
				_value = (void*)data;
			} else {
				_count = 0;
				_value = sl_null;
			}
		}
		
		template <sl_size N>
		ListParam(T(&data)[N]) noexcept
		{
			_count = N & SLIB_SIZE_MASK_NO_SIGN_BITS;
			_value = data;
		}

		ListParam(const ListLocker<T>& list) noexcept
		{
			_count = list.count & SLIB_SIZE_MASK_NO_SIGN_BITS;
			_value = list.data;
		}

		ListParam(const ListElements<T>& list) noexcept
		{
			_count = list.count & SLIB_SIZE_MASK_NO_SIGN_BITS;
			_value = list.data;
		}

		~ListParam()
		{
			_free();
		}

	public:
		ListParam& operator=(sl_null_t) noexcept
		{
			_free();
			_value = sl_null;
			_count = 0;
			return *this;
		}

		ListParam& operator=(ListParam&& other) noexcept
		{
			_value = other._value;
			_count = other._count;
			other._value = sl_null;
			other._count = 0;
			return *this;
		}

		ListParam& operator=(const ListParam& other) noexcept
		{
			_free();
			_count = other._count;
			_value = other._value;
			if (_count == (sl_reg)(ListType::List_Ref)) {
				_count = (sl_reg)(ListType::List_NoRef);
			}
			return *this;
		}
		
		ListParam& operator=(List<T>&& list) noexcept
		{
			_free();
			if (list.isNotNull()) {
				_count = (sl_reg)(ListType::List_Ref);
				new (reinterpret_cast<List<T>*>(&_value)) List<T>(Move(list));
			} else {
				_value = sl_null;
				_count = 0;
			}
			return *this;
		}

		ListParam& operator=(const List<T>& list) noexcept
		{
			_free();
			_value = list.ref.ptr;
			if (_value) {
				_count = (sl_reg)(ListType::List_NoRef);
			} else {
				_count = 0;
			}
			return *this;
		}
		
		ListParam& operator=(AtomicList<T>&& list) noexcept
		{
			return *this = List<T>(Move(list));
		}

		ListParam& operator=(const AtomicList<T>& list) noexcept
		{
			return *this = List<T>(list);
		}

		ListParam& operator=(const CList<T>& list) noexcept
		{
			_free();
			_count = (sl_reg)(ListType::CList);
			_value = (void*)(&list);
			return *this;
		}

		template <sl_size N>
		ListParam& operator=(T(&data)[N]) noexcept
		{
			_free();
			_count = N & SLIB_SIZE_MASK_NO_SIGN_BITS;
			_value = (void*)data;
			return *this;
		}
		
		ListParam& operator=(const ListLocker<T>& list) noexcept
		{
			_free();
			_count = list.count & SLIB_SIZE_MASK_NO_SIGN_BITS;
			_value = (void*)(list.data);
			return *this;
		}

		ListParam& operator=(const ListElements<T>& list) noexcept
		{
			_free();
			_count = list.count & SLIB_SIZE_MASK_NO_SIGN_BITS;
			_value = (void*)(list.data);
			return *this;
		}

		T& operator[](sl_reg index) const noexcept
		{
			return (getData())[index];
		}

	public:
		void setNull() noexcept
		{
			_free();
			_value = sl_null;
			_count = 0;
		}
		
		sl_bool isNull() const noexcept
		{
			return !_value;
		}
		
		sl_bool isNotNull() const noexcept
		{
			return _value != sl_null;
		}

		sl_size getCount() const noexcept
		{
			if (_count < 0) {
				return ((CList<T>*)_value)->getCount();
			} else {
				return _count;
			}
		}

		T* getData() const noexcept
		{
			if (_count < 0) {
				return ((CList<T>*)_value)->getData();
			} else {
				return (T*)_value;
			}
		}

		CList<T>* getObject() const noexcept
		{
			if (_count < 0) {
				return (CList<T>*)_value;
			} else {
				return sl_null;
			}
		}

		List<T> toList() const noexcept
		{
			if (_count == (sl_reg)(ListType::List_Ref) || _count == (sl_reg)(ListType::List_NoRef)) {
				return (CList<T>*)_value;
			} else if (_count == (sl_reg)(ListType::CList)) {
				return ((CList<T>*)_value)->duplicate();
			} else {
				return List<T>::create((T*)_value, _count);
			}
		}

		template <class... ARGS>
		sl_bool add(ARGS&&... args) noexcept
		{
			if (_count < 0) {
				return ((CList<T>*)_value)->add_NoLock(Forward<ARGS>(args)...);
			} else {
				if (_count) {
					return sl_false;
				}
				_count = (sl_reg)(ListType::List_Ref);
				return (reinterpret_cast<List<T>*>(&_value))->add_NoLock(Forward<ARGS>(args)...);
			}
		}

		// range-based for loop
		T* begin() const noexcept
		{
			return getData();
		}

		T* end() const noexcept
		{
			return getData() + getCount();
		}

	private:
		const List<T>& _getList() const noexcept
		{
			if (_count == (sl_reg)(ListType::List_Ref)) {
				return *(reinterpret_cast<const List<T>*>(&_value));
			} else {
				return List<T>::null();
			}
		}

		void _free() noexcept
		{
			if (_count == (sl_reg)(ListType::List_Ref)) {
				(*(reinterpret_cast<List<T>*>(&_value))).List<T>::~List();
			}
		}

		friend class ListLocker<T>;
		friend class ListElements<T>;

	};
	
}

#endif
