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

#ifndef CHECKHEADER_SLIB_CORE_LIST_COLLECTION
#define CHECKHEADER_SLIB_CORE_LIST_COLLECTION

#include "variant.h"
#include "string_buffer.h"

#include "serialize/variant.h"

namespace slib
{

	template <class T>
	class ListCollection_NoLocking : public Collection
	{
	public:
		ListCollection_NoLocking(const List<T>& list): m_list(list.ref) {}

		ListCollection_NoLocking(CList<T>* list): m_list(list) {}

	public:
		sl_uint64 getElementCount() override
		{
			return m_list->getCount();
		}

		Variant getElement(sl_uint64 index) override
		{
			return m_list->getAt_NoLock((sl_size)index);
		}

		sl_bool setElement(sl_uint64 index, const Variant& item) override
		{
			CList<T>* list = m_list.get();
			if (item.isNotUndefined()) {
				T* p = list->getPointerAt((sl_size)index);
				if (p) {
					item.get(*p);
					return sl_true;
				}
				return sl_false;
			} else {
				return list->removeAt_NoLock((sl_size)index);
			}
		}

		sl_bool addElement(const Variant& item) override
		{
			T v;
			item.get(v);
			return m_list->add_NoLock(Move(v));
		}

		sl_bool toJsonString(StringBuffer& buf) override
		{
			ListElements<T> list(*m_list);
			if (!(buf.addStatic("["))) {
				return sl_false;
			}
			for (sl_size i = 0; i < list.count; i++) {
				if (i) {
					if (!(buf.addStatic(", "))) {
						return sl_false;
					}
				}
				Variant v(list[i]);
				if (!(v.toJsonString(buf))) {
					return sl_false;
				}
			}
			if (!(buf.addStatic("]"))) {
				return sl_false;
			}
			return sl_true;
		}

		sl_bool toJsonBinary(MemoryBuffer& buf) override
		{
			ListElements<T> list(*m_list);
			if (!(SerializeByte(&buf, (sl_uint8)(VariantType::Collection)))) {
				return sl_false;
			}
			if (!(CVLI::serialize(&buf, list.count))) {
				return sl_false;
			}
			for (sl_size i = 0; i < list.count; i++) {
				if (!(Serialize(&buf, Variant(list[i])))) {
					return sl_false;
				}
			}
			return sl_true;
		}

	protected:
		Ref< CList<T> > m_list;

	};

	template <class T>
	class ListCollection : public ListCollection_NoLocking<T>
	{
		typedef ListCollection_NoLocking<T> Base;

	public:
		ListCollection(const List<T>& list): Base(list) {}

		ListCollection(CList<T>* list): Base(list) {}

	public:
		Variant getElement(sl_uint64 index) override
		{
			return m_list->getAt((sl_size)index);
		}

		sl_bool setElement(sl_uint64 index, const Variant& item) override
		{
			ObjectLocker lock(m_list.get());
			return Base::setElement(index, item);
		}

		sl_bool addElement(const Variant& item) override
		{
			T v;
			item.get(v);
			return m_list->add(Move(v));
		}

		sl_bool toJsonString(StringBuffer& buf) override
		{
			ObjectLocker lock(m_list.get());
			return Base::toJsonString(buf);
		}

		sl_bool toJsonBinary(MemoryBuffer& buf) override
		{
			ObjectLocker lock(m_list.get());
			return Base::toJsonBinary(buf);
		}

	protected:
		using Base::m_list;

	};

	template <class T>
	Ref<Collection> CList<T>::toCollection() noexcept
	{
		return new ListCollection<T>(this);
	}

	template <class T>
	Ref<Collection> CList<T>::toCollection_NoLocking() noexcept
	{
		return new ListCollection_NoLocking<T>(this);
	}

	template <class T>
	List<T> List<T>::create(Collection* collection)
	{
		return priv::variant::CreateListFromCollection< List<T> >(collection);
	}

	template <class T>
	Variant::Variant(const List<T>& list)
	{
		Ref<Collection> collection(list.toCollection_NoLocking());
		_constructorMoveRef(&collection, VariantType::Collection);
	}

}

#endif
