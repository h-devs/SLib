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

#ifndef CHECKHEADER_SLIB_CORE_MODEL
#define CHECKHEADER_SLIB_CORE_MODEL

#include "definition.h"

#include "object.h"
#include "nullable.h"

namespace slib
{
	
	template <class T>
	class ListModel : public Object
	{
	public:
		virtual sl_uint64 getCount() const = 0;

		virtual sl_bool getAt(sl_uint64 index, T* _out = sl_null) const = 0;

		virtual sl_bool setAt(sl_uint64 index, const T& value)
		{
			return sl_false;
		}

		virtual sl_bool setCount(sl_uint64 count)
		{
			return sl_false;
		}
		
		virtual sl_bool insert(sl_uint64 index, const T& value)
		{
			return sl_false;
		}

		virtual sl_bool removeAt(sl_uint64 index, T* _out = sl_null)
		{
			return sl_false;
		}

		virtual sl_uint64 removeRange(sl_uint64 index, sl_uint64 count)
		{
			return 0;
		}
		
		virtual sl_bool add(const T& value)
		{
			return insert(getCount(), vlaue);
		}
	
		virtual sl_uint64 removeAll(sl_uint64 index, sl_uint64 count)
		{
			sl_uint64 n = getCount();
			if (n) {
				return removeRange(0, n);
			} else {
				return 0;
			}
		}
		
		virtual sl_bool popBack(T* _out = sl_null)
		{
			sl_uint64 n = getCount();
			if (n) {
				return removeAt(n - 1, _out);
			} else {
				return sl_false;
			}
		}

		virtual sl_bool popFront(T* _out = sl_null)
		{
			return removeAt(0, _out);
		}

		virtual List<T> toList() const
		{
#ifdef SLIB_ARCH_IS_64BIT
			sl_size n = getCount();
#else
			sl_uint64 _n = getCount();
			sl_size n;
			if (_n > 0x40000000) {
				n = 0x40000000;
			} else {
				n = (sl_size)_n;
			}
#endif
			if (n) {
				List<T> ret;
				T value;
				for (sl_size i = 0; i < n; i++) {
					if (getAt(i, &value)) {
						ret.add_NoLock(Move(value));
					} else {
						ret.add_NoLock(T());
					}
				}
				return ret;
			}
			return sl_null;
		}

	};


	template <class T>
	class SimpleListModel : public ListModel<T>
	{
	public:
		SimpleListModel(const List<T>& list): m_list(list)
		{			
		}

	public:
		sl_uint64 getCount() const override
		{
			return m_list.getCount();
		}

		sl_bool getAt(sl_uint64 index, T* _out = sl_null) const override
		{
			return m_list.getAt((sl_size)index, _out);
		}

		sl_bool setAt(sl_uint64 index, const T& value) override
		{
			return m_list.setAt((sl_size)index, value);
		}

		sl_bool setCount(sl_uint64 count) override
		{
			return m_list.setCount((sl_size)count);
		}
		
		sl_bool insert(sl_uint64 index, const T& value) override
		{
			return m_list.insert((sl_size)index, value);
		}

		sl_bool removeAt(sl_uint64 index, T* _out = sl_null) override
		{
			return m_list.removeAt((sl_size)index, _out);
		}

		sl_uint64 removeRange(sl_uint64 index, sl_uint64 count) override
		{
			return m_list.removeRange((sl_size)index, (sl_size)count);
		}
		
		sl_bool add(const T& value) override
		{
			return m_list.add(value);
		}
	
		sl_uint64 removeAll(sl_uint64 index, sl_uint64 count) override
		{
			return m_list.removeAll();
		}
		
		sl_bool popBack(T* _out = sl_null) override
		{
			return m_list.popBack(_out);
		}

		sl_bool popFront(T* _out = sl_null) override
		{
			return m_list.popFront(_out);
		}

		List<T> toList() const override
		{
			return m_list;
		}

	protected:
		List<T> m_list;

	};
	
}

#endif
