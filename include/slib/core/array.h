/*
 *   Copyright (c) 2008-2020 SLIBIO <https://github.com/SLIBIO>
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

#ifndef CHECKHEADER_SLIB_CORE_ARRAY
#define CHECKHEADER_SLIB_CORE_ARRAY

#include "array_traits.h"
#include "ref.h"
#include "sort.h"
#include "collection.h"

#ifdef SLIB_SUPPORT_STD_TYPES
#include <initializer_list>
#endif

namespace slib
{
	
	template <class T>
	class SLIB_EXPORT ArrayData
	{
	public:
		T* data;
		sl_size count;
		Ref<Referable> ref;

	public:
		T& operator[](sl_reg index) const noexcept
		{
			return data[index];
		}

	};
	
	template <class T>
	class Array;
	
	template <class T>
	using AtomicArray = Atomic< Array<T> >;

	class SLIB_EXPORT CArrayBase : public Referable
	{
		SLIB_DECLARE_OBJECT

	public:
		CArrayBase();

		~CArrayBase();

	};
	
	template <class T>
	class SLIB_EXPORT CArray : public CArrayBase
	{
	public:
		typedef T ELEMENT_TYPE;
		
	protected:
		T* m_data;
		sl_size m_count;
		Ref<Referable> m_ref;
		sl_bool m_flagStatic;

	public:
		CArray() noexcept: m_data(sl_null), m_count(0), m_flagStatic(sl_true) {}

		CArray(sl_size count) noexcept
		{
			if (count) {
				T* data = (T*)(Base::createMemory(count * sizeof(T)));
				if (data) {
					ArrayTraits<T>::construct(data, count);
					m_flagStatic = sl_false;
					m_data = data;
					m_count = count;
					return;
				}
			}
			m_flagStatic = sl_true;
			m_data = sl_null;
			m_count = 0;
		}

		template <class VALUE>
		CArray(const VALUE* src, sl_size count) noexcept
		{
			if (count) {
				T* data = (T*)(Base::createMemory(count * sizeof(T)));
				if (data) {
					ArrayTraits<T>::copy_construct(data, src, count);
					m_flagStatic = sl_false;
					m_data = data;
					m_count = count;
					return;
				}
			}
			m_flagStatic = sl_true;
			m_data = sl_null;
			m_count = 0;
		}

		CArray(const T* data, sl_size count, Referable* ref) noexcept: m_ref(ref)
		{
			m_data = const_cast<T*>(data);
			m_count = count;
			m_flagStatic = sl_true;
		}

#ifdef SLIB_SUPPORT_STD_TYPES
		CArray(const std::initializer_list<T>& l) noexcept: CArray(l.begin(), l.size()) {}
#endif

		~CArray() noexcept
		{
			if (!m_flagStatic) {
				T* data = m_data;
				if (data) {
					ArrayTraits<T>::free(data, m_count);
					Base::freeMemory((void*)data);
				}
			}
		}
		
	public:
		CArray(const CArray& other) = delete;
		
		CArray(CArray&& other) noexcept: m_ref(Move(other.m_ref))
		{
			m_flagStatic = other.m_flagStatic;
			m_data = other.m_data;
			m_count = other.m_count;
			other.m_flagStatic = sl_true;
			other.m_data = sl_null;
			other.m_count = 0;
		}
		
		CArray& operator=(const CArray& other) = delete;
		
		CArray& operator=(CArray&& other) noexcept
		{
			if (!m_flagStatic) {
				T* data = m_data;
				if (data) {
					ArrayTraits<T>::free(data, m_count);
					Base::freeMemory((void*)data);
				}
			}
			m_flagStatic = other.m_flagStatic;
			m_data = other.m_data;
			m_count = other.m_count;
			other.m_flagStatic = sl_true;
			other.m_data = sl_null;
			other.m_count = 0;
			m_ref = Move(other.m_ref);
			return *this;
		}

	public:	
		static CArray<T>* create(sl_size count) noexcept
		{
			if (count) {
				CArray<T>* ret = new CArray<T>(count);
				if (ret) {
					if (ret->m_data) {
						return ret;
					}
					delete ret;
				}
			}
			return sl_null;
		}

		template <class VALUE>
		static CArray<T>* create(const VALUE* data, sl_size count) noexcept
		{
			if (count) {
				CArray<T>* ret = new CArray<T>(data, count);
				if (ret) {
					if (ret->m_data) {
						return ret;
					}
					delete ret;
				}
			}
			return sl_null;
		}

		static CArray<T>* createStatic(const T* data, sl_size count, Referable* ref = sl_null) noexcept
		{
			if (data && count) {
				return new CArray<T>(data, count, ref);
			}
			return sl_null;
		}

#ifdef SLIB_SUPPORT_STD_TYPES
		static CArray<T>* create(const std::initializer_list<T>& l) noexcept
		{
			return create(l.begin(), l.size());
		}
#endif

	public:
		T* getData() const noexcept
		{
			return m_data;
		}

		sl_size getCount() const noexcept
		{
			return m_count;
		}

		sl_bool isStatic() const noexcept
		{
			return m_flagStatic;
		}

		const Ref<Referable>& getRef() const noexcept
		{
			return m_ref;
		}

	public:
		T* getPointerAt(sl_size index) const noexcept
		{
			if (index < m_count) {
				return m_data + index;
			}
			return sl_null;
		}

		sl_bool getAt(sl_size index, T* _out = sl_null) const noexcept
		{
			if (index < m_count) {
				*_out = m_data[index];
				return sl_true;
			}
			return sl_false;
		}

		T getValueAt(sl_size index) const noexcept
		{
			if (index < m_count) {
				return m_data[index];
			} else {
				return T();
			}
		}

		T getValueAt(sl_size index, const T& def) const noexcept
		{
			if (index < m_count) {
				return m_data[index];
			}
			return def;
		}

		sl_bool setAt(sl_size index, const T& value) const noexcept
		{
			if (index < m_count) {
				m_data[index] = value;
				return sl_true;
			}
			return sl_false;
		}

		T const& operator[](sl_size_t index) const noexcept
		{
			return m_data[index];
		}

		T& operator[](sl_size_t index) noexcept
		{
			return m_data[index];
		}

	public:
		CArray<T>* sub(sl_size start, sl_size count = SLIB_SIZE_MAX) const noexcept
		{
			sl_size countParent = m_count;
			if (start < countParent) {
				sl_size n = countParent - start;
				if (count > n) {
					count = n;
				}
				if (count) {
					if (countParent == count) {
						return (CArray<T>*)this;
					}
					if (m_flagStatic) {
						return createStatic(m_data + start, count, m_ref.ptr);
					} else {
						return createStatic(m_data + start, count, (Referable*)this);
					}
				}
			}
			return sl_null;
		}
		
		template < class VALUE, class ARG = Equals<T, VALUE> >
		sl_reg indexOf(const VALUE& value, const ARG& arg = ARG()) const noexcept
		{
			return ArrayTraits<T>::indexOf(m_data, m_count, value, arg);
		}

		template <class VALUE, class EQUALS>
		sl_reg indexOf(const VALUE& value, const EQUALS& equals, sl_reg startIndex) const noexcept
		{
			return ArrayTraits<T>::indexOf(m_data, m_count, value, equals, startIndex);
		}

		template < class VALUE, class ARG = Equals<T, VALUE> >
		sl_reg lastIndexOf(const VALUE& value, const ARG& arg = ARG()) const noexcept
		{
			return ArrayTraits<T>::lastIndexOf(m_data, m_count, value, arg);
		}

		template <class VALUE, class EQUALS>
		sl_reg lastIndexOf(const VALUE& value, const EQUALS& equals, sl_reg startIndex) const noexcept
		{
			return ArrayTraits<T>::lastIndexOf(m_data, m_count, value, equals, startIndex);
		}

		template < class VALUE, class EQUALS = Equals<T, VALUE> >
		sl_bool contains(const VALUE& value, const EQUALS& equals = EQUALS()) const noexcept
		{
			return ArrayTraits<T>::indexOf(m_data, m_count, value, equals) >= 0;
		}

		template <class VALUE>
		sl_size read(sl_size startSource, sl_size len, VALUE* pDst) const noexcept
		{
			T* pSrc = m_data;
			if (pDst && pSrc) {
				sl_size countSrc = m_count;
				if (startSource < countSrc) {
					sl_size lenSrc = countSrc - startSource;
					if (len > lenSrc) {
						len = lenSrc;
					}
					ArrayTraits<T>::copy(pDst, pSrc + startSource, len);
					return len;
				}
			}
			return 0;
		}

		template <class VALUE>
		sl_size write(sl_size startTarget, sl_size len, const VALUE* pSrc) const noexcept
		{
			T* pDst = m_data;
			if (pSrc && pDst) {
				sl_size countDst = m_count;
				if (startTarget < countDst) {
					sl_size lenDst = countDst - startTarget;
					if (len > lenDst) {
						len = lenDst;
					}
					ArrayTraits<T>::copy(pDst + startTarget, pSrc, len);
					return len;
				}
			}
			return 0;
		}

		template <class VALUE>
		sl_size copy(sl_size startTarget, const CArray<VALUE>* source, sl_size startSource = 0, sl_size len = SLIB_SIZE_MAX) const noexcept
		{
			if (source) {
				VALUE* pSrc = source->getData();
				if (pSrc) {
					sl_size countSrc = source->getCount();
					if (startSource < countSrc) {
						sl_size lenSrc = countSrc - startSource;
						if (len > lenSrc) {
							len = lenSrc;
						}
						return write<VALUE>(startTarget, len, pSrc + startSource);
					}
				}
			}
			return 0;
		}

		template <class VALUE>
		sl_size copy(const CArray<VALUE>* source, sl_size start = 0, sl_size len = SLIB_SIZE_MAX) const noexcept
		{
			return copy(0, source, start, len);
		}

		CArray<T>* duplicate() const noexcept
		{
			return create(m_data, m_count);
		}

		template < class COMPARE = Compare<T> >
		void sort(const COMPARE& compare = COMPARE()) const noexcept
		{
			QuickSort::sortAsc(m_data, m_count, compare);
		}

		template < class COMPARE = Compare<T> >
		void sortDesc(const COMPARE& compare = COMPARE()) const noexcept
		{
			QuickSort::sortDesc(m_data, m_count, compare);
		}
		
		void reverse() const noexcept
		{
			ArrayTraits<T>::reverse(m_data, m_count);
		}

		Ref<Collection> toCollection() noexcept;

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
	class SLIB_EXPORT ArrayPosition
	{
	public:
		ArrayPosition() noexcept
		{
			pos = sl_null;
		}

		ArrayPosition(T* _pos, sl_size _count, Referable* _ref) noexcept: ref(_ref)
		{
			count = _count;
			if (_count) {
				pos = _pos;
			} else {
				pos = sl_null;
			}
		}


		ArrayPosition(ArrayPosition&& other) = default;

	public:
		ArrayPosition& operator=(const ArrayPosition& other) = default;
		
		ArrayPosition& operator=(ArrayPosition&& other) = default;
		
		T& operator*() const noexcept
		{
			return *pos;
		}

		sl_bool operator==(const ArrayPosition& other) const noexcept
		{
			return pos == other.pos;
		}

		sl_bool operator!=(const ArrayPosition& other) const noexcept
		{
			return pos != other.pos;
		}

		explicit operator sl_bool() const noexcept
		{
			return count > 0;
		}

		ArrayPosition& operator++() noexcept
		{
			pos++;
			count--;
			if (count == 0) {
				pos = sl_null;
			}
			return *this;
		}

	private:
		T* pos;
		sl_size count;
		Ref<Referable> ref;

	};
	
	template <class T>
	class SLIB_EXPORT Array
	{
	public:
		typedef T ELEMENT_TYPE;
		
	public:
		Ref< CArray<T> > ref;
		SLIB_REF_WRAPPER(Array, CArray<T>)
	
	public:
		Array(sl_size count) noexcept: ref(CArray<T>::create(count)) {}
		
		template <class VALUE>
		Array(const VALUE* data, sl_size count) noexcept: ref(CArray<T>::create(data, count)) {}

		Array(const T* data, sl_size count, Referable* ref) noexcept: ref(CArray<T>::createStatic(data, count, ref)) {}

#ifdef SLIB_SUPPORT_STD_TYPES
		Array(const std::initializer_list<T>& l) noexcept: ref(CArray<T>::create(l.begin(), l.size())) {}
#endif
		
	public:
		static Array<T> create(sl_size count) noexcept
		{
			return CArray<T>::create(count);
		}

		template <class VALUE>
		static Array<T> create(const VALUE* data, sl_size count) noexcept
		{
			return CArray<T>::create(data, count);
		}

		static Array<T> createStatic(const T* data, sl_size count, Referable* ref = sl_null) noexcept
		{
			return CArray<T>::createStatic(data, count, ref);
		}

		static Array<T> create(Collection* collection);

#ifdef SLIB_SUPPORT_STD_TYPES
		static Array<T> create(const std::initializer_list<T>& l) noexcept
		{
			return create(l.begin(), l.size());
		}
#endif

		template <class VALUE>
		static Array<T>& from(const Array<VALUE>& other) noexcept
		{
			return *(const_cast<Array<T>*>(reinterpret_cast<Array<T> const*>(&other)));
		}

	public:
		T* getData() const noexcept
		{
			CArray<T>* obj = ref.ptr;
			if (obj) {
				return obj->getData();
			}
			return sl_null;
		}

		sl_size getCount() const noexcept
		{
			CArray<T>* obj = ref.ptr;
			if (obj) {
				return obj->getCount();
			}
			return 0;
		}

	public:
		T* getPointerAt(sl_size index) const noexcept
		{
			CArray<T>* obj = ref.ptr;
			if (obj) {
				return obj->getPointerAt(index);
			}
			return sl_null;
		}

		sl_bool getAt(sl_size index, T* _out = sl_null) const noexcept
		{
			CArray<T>* obj = ref.ptr;
			if (obj) {
				return obj->getAt(index, _out);
			}
			return sl_false;
		}

		T getValueAt(sl_size index) const noexcept
		{
			CArray<T>* obj = ref.ptr;
			if (obj) {
				return obj->getValueAt(index);
			} else {
				return T();
			}
		}

		T getValueAt(sl_size index, const T& def) const noexcept
		{
			CArray<T>* obj = ref.ptr;
			if (obj) {
				return obj->getValueAt(index, def);
			}
			return def;
		}

		sl_bool setAt(sl_size index, const T& value) const noexcept
		{
			CArray<T>* obj = ref.ptr;
			if (obj) {
				return obj->setAt(index, value);
			}
			return sl_false;
		}

		T& operator[](sl_size_t index) const noexcept
		{
			return (ref->getData())[index];
		}
		
#ifdef SLIB_SUPPORT_STD_TYPES
		Array<T>& operator=(const std::initializer_list<T>& l) noexcept
		{
			ref = CArray<T>::create(l.begin(), l.size());
			return *this;
		}
#endif

	public:
		Array<T> sub(sl_size start, sl_size count = SLIB_SIZE_MAX) const noexcept
		{
			CArray<T>* obj = ref.ptr;
			if (obj) {
				return obj->sub(start, count);
			}
			return sl_null;
		}
		
		template < class VALUE, class ARG = Equals<T, VALUE> >
		sl_reg indexOf(const VALUE& value, const ARG& arg = ARG()) const noexcept
		{
			CArray<T>* obj = ref.ptr;
			if (obj) {
				return obj->indexOf(value, arg);
			}
			return -1;
		}
		
		template <class VALUE, class EQUALS>
		sl_reg indexOf(const VALUE& value, const EQUALS& equals, sl_reg startIndex) const noexcept
		{
			CArray<T>* obj = ref.ptr;
			if (obj) {
				return obj->indexOf(value, equals, startIndex);
			}
			return -1;
		}
		
		template < class VALUE, class ARG = Equals<T, VALUE> >
		sl_reg lastIndexOf(const VALUE& value, const ARG& arg = ARG()) const noexcept
		{
			CArray<T>* obj = ref.ptr;
			if (obj) {
				return obj->lastIndexOf(value, arg);
			}
			return -1;
		}

		template <class VALUE, class EQUALS>
		sl_reg lastIndexOf(const VALUE& value, const EQUALS& equals, sl_reg startIndex) const noexcept
		{
			CArray<T>* obj = ref.ptr;
			if (obj) {
				return obj->lastIndexOf(value, equals, startIndex);
			}
			return -1;
		}

		template < class VALUE, class EQUALS = Equals<T, VALUE> >
		sl_bool contains(const VALUE& value, const EQUALS& equals = EQUALS()) const noexcept
		{
			return indexOf(value, equals) >= 0;
		}

		template <class VALUE>
		sl_size read(sl_size startSource, sl_size len, VALUE* dataDst) const noexcept
		{
			CArray<T>* obj = ref.ptr;
			if (obj) {
				return obj->read(startSource, len, dataDst);
			}
			return 0;
		}

		template <class VALUE>
		sl_size write(sl_size startTarget, sl_size len, const VALUE* dataSrc) const noexcept
		{
			CArray<T>* obj = ref.ptr;
			if (obj) {
				return obj->write(startTarget, len, dataSrc);
			}
			return 0;
		}

		template <class VALUE>
		sl_size copy(sl_size startTarget, const Array<VALUE>& source, sl_size startSource = 0, sl_size len = SLIB_SIZE_MAX) const noexcept
		{
			CArray<T>* obj = ref.ptr;
			if (obj) {
				return obj->copy(startTarget, source.ref.ptr, startSource, len);
			}
			return 0;
		}

		template <class VALUE>
		sl_size copy(const Array<VALUE>& source, sl_size start = 0, sl_size len = SLIB_SIZE_MAX) const noexcept
		{
			return copy(0, source, start, len);
		}

		Array<T> duplicate() const noexcept
		{
			CArray<T>* obj = ref.ptr;
			if (obj) {
				return obj->duplicate();
			}
			return sl_null;
		}

		sl_bool getData(ArrayData<T>& data) const noexcept
		{
			CArray<T>* obj = ref.ptr;
			if (obj) {
				data.data = obj->getData();
				data.count = obj->getCount();
				if (obj->isStatic()) {
					data.ref = obj->getRef();
				} else {
					data.ref = obj;
				}
				return sl_true;
			} else {
				data.data = sl_null;
				data.count = 0;
				data.ref.setNull();
				return sl_false;
			}
		}

		template < class COMPARE = Compare<T> >
		void sort(const COMPARE& compare = COMPARE()) const noexcept
		{
			CArray<T>* obj = ref.ptr;
			if (obj) {
				obj->sort(compare);
			}
		}

		template < class COMPARE = Compare<T> >
		void sortDesc(const COMPARE& compare = COMPARE()) const noexcept
		{
			CArray<T>* obj = ref.ptr;
			if (obj) {
				obj->sortDesc(compare);
			}
		}

		void reverse() const noexcept
		{
			CArray<T>* obj = ref.ptr;
			if (obj) {
				obj->reverse();
			}
		}

		Ref<Collection> toCollection() const noexcept
		{
			CArray<T>* obj = ref.ptr;
			if (obj) {
				return obj->toCollection();
			}
			return sl_null;
		}

		// range-based for loop
		T* begin() const noexcept
		{
			CArray<T>* obj = ref.ptr;
			if (obj) {
				return obj->getData();
			}
			return sl_null;
		}

		T* end() const noexcept
		{
			CArray<T>* obj = ref.ptr;
			if (obj) {
				return obj->getData() + obj->getCount();
			}
			return sl_null;
		}

	};
	
	
	template <class T>
	class SLIB_EXPORT Atomic< Array<T> >
	{
	public:
		typedef T ELEMENT_TYPE;
		
	public:
		AtomicRef< CArray<T> > ref;
		SLIB_ATOMIC_REF_WRAPPER(CArray<T>)
		
	public:	
		Atomic(const T* data, sl_size count, Referable* ref, sl_bool flagStatic = sl_true) noexcept: ref(CArray<T>::create(data, count, ref, flagStatic)) {}
		
		Atomic(sl_size count) noexcept: ref(CArray<T>::create(count)) {}
		
		template <class VALUE>
		Atomic(const VALUE* data, sl_size count) noexcept: ref(CArray<T>::create(data, count)) {}
		
#ifdef SLIB_SUPPORT_STD_TYPES
		Atomic(const std::initializer_list<T>& l) noexcept: ref(CArray<T>::create(l.begin(), l.size())) {}
#endif
		
	public:
		template <class VALUE>
		static Atomic< Array<T> >& from(const Atomic< Array<VALUE> >& other) noexcept
		{
			return *(const_cast<Atomic< Array<T> >*>(reinterpret_cast<Atomic< Array<T> > const*>(&other)));
		}

	public:
		
#ifdef SLIB_SUPPORT_STD_TYPES
		Atomic& operator=(const std::initializer_list<T>& l) noexcept
		{
			ref = CArray<T>::create(l.begin(), l.size());
			return *this;
		}
#endif

	};
	

	template <class T>
	class SLIB_EXPORT ArrayElements
	{
	public:
		T* data;
		sl_size count;
		Array<T> array;

	public:
		ArrayElements(Array<T>&& _array) noexcept: array(Move(_array))
		{
			data = array.getData();
			count = array.getCount();
		}

		ArrayElements(const Array<T>& _array) noexcept: array(_array)
		{
			data = array.getData();
			count = array.getCount();
		}

		ArrayElements(const AtomicArray<T>& _array) noexcept: array(_array)
		{
			data = array.getData();
			count = array.getCount();
		}

		template <class LIST>
		ArrayElements(LIST&& array, sl_size startIndex) noexcept: ArrayElements(Forward<LIST>(array))
		{
			if (startIndex >= count) {
				data = sl_null;
				count = 0;
			} else {
				data += startIndex;
				count -= startIndex;
			}
		}

		template <class LIST>
		ArrayElements(LIST&& array, sl_size startIndex, sl_size _count) noexcept: ArrayElements(Forward<LIST>(array))
		{
			if (!_count || startIndex >= count) {
				data = sl_null;
				count = 0;
			} else {
				data += startIndex;
				sl_size limit = count - startIndex;
				if (_count <= limit) {
					count = _count;
				} else {
					count = limit;
				}
			}
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

}

#endif
