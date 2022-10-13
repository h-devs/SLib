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

#ifndef CHECKHEADER_SLIB_CORE_LENDER
#define CHECKHEADER_SLIB_CORE_LENDER

#include "list.h"

namespace slib
{
	
	template <class TYPE>
	class SLIB_EXPORT SingleLender
	{
	protected:
		TYPE m_object;
		sl_bool m_flagAbsence;
		SpinLock m_lock;
		
	public:
		SingleLender(): m_flagAbsence(sl_true) {}

	public:
		sl_bool lend(TYPE& _out)
		{
			SpinLocker locker(&m_lock);
			if (m_flagAbsence) {
				locker.unlock();
				return create(_out);
			} else {
				_out = Move(m_object);
				m_flagAbsence = sl_true;
				return sl_true;
			}
		}

		void collect(TYPE&& object)
		{
			SpinLocker locker(&m_lock);
			if (m_flagAbsence) {
				TYPE old = Move(m_object);
				m_object = Move(object);
				m_flagAbsence = sl_false;
				locker.unlock();
			}
		}

		virtual sl_bool create(TYPE& _out) = 0;
		
	};

	template <class TYPE>
	class SLIB_EXPORT Lender
	{
	protected:
		List<TYPE> m_list;
		sl_size m_nMaxStock;

	public:
		Lender(sl_size nMaxStock = 1) : m_nMaxStock(nMaxStock) {}

	public:
		sl_size getMaxStockCount() const noexcept
		{
			return m_nMaxStock;
		}

		void setMaxStockCount(sl_size n) noexcept
		{
			m_nMaxStock = n;
		}

		sl_size getStockCount() const noexcept
		{
			return m_list.getCount();
		}

		sl_bool lend(TYPE& _out)
		{
			if (m_list.popBack(&_out)) {
				return sl_true;
			} else {
				return create(_out);
			}
		}

		void collect(TYPE&& object)
		{
			if (m_list.getCount() < m_nMaxStock) {
				m_list.add(Move(object));
			}
		}

		virtual sl_bool create(TYPE& _out) = 0;

	};

	template <class TYPE, sl_size STOCK_SIZE>
	class SLIB_EXPORT StaticArrayLender
	{
	protected:
		TYPE m_list[STOCK_SIZE];
		sl_size m_count;
		SpinLock m_lock;

	public:
		sl_bool lend(TYPE& _out)
		{
			SpinLocker locker(&m_lock);
			sl_size n = m_count;
			if (n) {
				n--;
				_out = Move(m_list[n]);
				m_count = n;
				return sl_true;
			} else {
				locker.unlock();
				return create(_out);
			}
		}

		void collect(TYPE&& object)
		{
			SpinLocker locker(&m_lock);
			sl_size n = m_count;
			if (n < STOCK_SIZE) {
				m_list[n] = Move(object);
				m_count++;
			}
		}

		virtual sl_bool create(TYPE& _out) = 0;

	};

	template <class TYPE, class LENDER>
	class SLIB_EXPORT Borrower
	{
	public:
		TYPE value;
		LENDER* lender;

	public:
		Borrower(): lender(sl_null) {}

		~Borrower()
		{
			if (lender) {
				lender->collect(Move(value));
			}
		}

	public:
		sl_bool borrow(LENDER* _lender)
		{
			if (_lender->lend(value)) {
				lender = _lender;
				return sl_true;
			}
			return sl_false;
		}

	};

}

#endif
