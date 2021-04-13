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

#ifndef CHECKHEADER_SLIB_CORE_ARRAY_COLLECTION
#define CHECKHEADER_SLIB_CORE_ARRAY_COLLECTION

#include "variant.h"
#include "string_buffer.h"
#include "serialize.h"

namespace slib
{

	template <class T>
	class ArrayCollection : public Collection
	{
	public:
		ArrayCollection(const Array<T>& array): m_array(array.ref) {}

		ArrayCollection(CArray<T>* array) : m_array(array) {}

	public:
		sl_uint64 getElementsCount() override
		{
			return m_array->getCount();
		}

		Variant getElement(sl_uint64 index) override
		{
			T* p = m_array->getPointerAt((sl_size)index);
			if (p) {
				return Variant(*p);
			}
			return Variant();
		}

		sl_bool setElement(sl_uint64 index, const Variant& item) override
		{
			T* p = m_array->getPointerAt((sl_size)index);
			if (p) {
				item.get(*p);
				return sl_true;
			}
			return sl_false;
		}
		
		sl_bool toJsonString(StringBuffer& buf) override
		{
			sl_size n = m_array->getCount();
			T* data = m_array->getData();
			if (!(buf.addStatic("["))) {
				return sl_false;
			}
			for (sl_size i = 0; i < n; i++) {
				Variant v(data[i]);
				if (i) {
					if (!(buf.addStatic(", "))) {
						return sl_false;
					}
				}
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
			if (!(SerializeByte(&buf, (sl_uint8)(VariantType::Collection)))) {
				return sl_false;
			}
			sl_size n = m_array->getCount();
			if (!(CVLI::serialize(&buf, n))) {
				return sl_false;
			}
			T* data = m_array->getData();
			for (sl_size i = 0; i < n; i++) {
				if (!(Serialize(&buf, Variant(data[i])))) {
					return sl_false;
				}
			}
			return sl_true;
		}

	protected:
		Ref< CArray<T> > m_array;

	};

	template <class T>
	Ref<Collection> CArray<T>::toCollection() noexcept
	{
		return new ArrayCollection<T>(this);
	}

	template <class T>
	Array<T> Array<T>::create(Collection* collection)
	{
		return priv::variant::CreateListFromCollection< Array<T> >(collection);
	}

	template <class T>
	Variant::Variant(const Array<T>& arr)
	{
		Ref<Collection> collection(arr.toCollection());
		_constructorMoveRef(&collection, VariantType::Collection);
	}

}

#endif
