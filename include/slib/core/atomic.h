/*
 *   Copyright (c) 2008-2018 SLIBIO <https://github.com/SLIBIO>
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

#ifndef CHECKHEADER_SLIB_CORE_ATOMIC
#define CHECKHEADER_SLIB_CORE_ATOMIC

#include "definition.h"

#include "cpp.h"
#include "spin_lock.h"
#include "compare.h"
#include "hash.h"

namespace slib
{
	
	template <class T>
	class SLIB_EXPORT Atomic
	{
	public:
		Atomic()
		{
		}

		Atomic(const T& value)
		: m_value(value)
		{
		}

		Atomic(T&& value)
		: m_value(Move(value))
		{
		}

	public:
		Atomic<T>& operator=(const T& other)
		{
			SLIB_ALIGN(8) char t[sizeof(T)];
			m_lock.lock();
			new ((T*)t) T(Move(m_value));
			m_value = other;
			m_lock.unlock();
			((T*)t)->~T();
			return *this;
		}

		Atomic<T>& operator=(T&& other)
		{
			SLIB_ALIGN(8) char t[sizeof(T)];
			m_lock.lock();
			new ((T*)t) T(Move(m_value));
			m_value = Move(other);
			m_lock.unlock();
			((T*)t)->~T();
			return *this;
		}

		operator T() const
		{
			SLIB_ALIGN(8) char t[sizeof(T)];
			m_lock.lock();
			new ((T*)t) T(m_value);
			m_lock.unlock();
			return *((T*)t);
		}

	protected:
		T m_value;
		SpinLock m_lock;

	};
	
	
	template <>
	class SLIB_EXPORT Atomic<sl_int32>
	{
	public:
		Atomic();

		Atomic(sl_int32 value);

	public:
		sl_int32 operator=(sl_int32 value);

		operator sl_int32 () const;

	public:
		sl_int32 increase();

		sl_int32 decrease();

		sl_int32 add(sl_int32 other);

		sl_bool waitZero(sl_int32 timeout = -1);

	private:
		volatile sl_int32 m_value;

	};
	
	typedef Atomic<sl_int32> AtomicInt32;
	
	
	template <class T>
	struct RemoveAtomic;
	
	template <class T>
	struct RemoveAtomic< Atomic<T> > { typedef T Type; };
	
	template <class T>
	struct PropertyTypeHelper< Atomic<T> >
	{
		typedef typename PropertyTypeHelper<T>::ArgType ArgType;
		typedef typename RemoveConstReference< typename PropertyTypeHelper<T>::RetType >::Type RetType;
	};
	
	
	template <class T>
	class Compare< Atomic<T>, Atomic<T> >
	{
	public:
		SLIB_INLINE sl_compare_result operator()(const T& a, const T& b) const noexcept
		{
			return Compare<T>()(a, b);
		}
	};
	
	template <class T>
	class Equals< Atomic<T>, Atomic<T> >
	{
	public:
		SLIB_INLINE sl_bool operator()(const T& a, const T& b) const noexcept
		{
			return Equals<T>()(a, b);
		}
	};
	
	template <class T>
	class Hash< Atomic<T>, sl_false >
	{
	public:
		SLIB_INLINE sl_size operator()(const T& a) const noexcept
		{
			return Hash<T>()(a);
		}
	};
	
}

#endif
