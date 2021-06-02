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

#include "spin_lock.h"
#include "compare.h"
#include "hash.h"
#include "new_helper.h"

namespace slib
{
	
	template <class T>
	class SLIB_EXPORT Atomic
	{
	public:
		Atomic()
		{
			new ((T*)m_value) T();
		}

		Atomic(const T& value)
		{
			new ((T*)m_value) T(value);
		}

		Atomic(T&& value) noexcept
		{
			new ((T*)m_value) T(Move(value));
		}

		Atomic(const Atomic<T>& value)
		{
			value._retain_construct(m_value);
		}

		~Atomic()
		{
			((T*)m_value)->~T();
		}

	public:
		Atomic<T>& operator=(const T& other)
		{
			_assign_copy(&other);
			return *this;
		}

		Atomic<T>& operator=(T&& other)
		{
			_assign_move(&other);
			return *this;
		}

		Atomic<T>& operator=(const Atomic<T>& _other)
		{
			SLIB_ALIGN(8) char other[sizeof(T)];
			_other._retain_construct(other);
			_assign_move(other);
			((T*)other)->~T();
			return *this;
		}

		operator T() const
		{
			T value;
			_retain_assign(&value);
			return value;
		}

	public:
		void _retain_construct(void* other) const
		{
			m_lock.lock();
			new ((T*)other) T(*((T*)m_value));
			m_lock.unlock();
		}

		void _retain_assign(void* other) const
		{
			m_lock.lock();
			*((T*)other) = *((T*)m_value);
			m_lock.unlock();
		}

		void _assign_copy(const void* other)
		{
			SLIB_ALIGN(8) char old[sizeof(T)];
			m_lock.lock();
			new ((T*)old) T(Move(*((T*)m_value)));
			*((T*)m_value) = *((T*)other);
			m_lock.unlock();
			((T*)old)->~T();
		}

		void _assign_move(void* other)
		{
			SLIB_ALIGN(8) char old[sizeof(T)];
			m_lock.lock();
			new ((T*)old) T(Move(*((T*)m_value)));
			*((T*)m_value) = Move(*((T*)other));
			m_lock.unlock();
			((T*)old)->~T();
		}

	protected:
		char m_value[sizeof(T)];
		SpinLock m_lock;

	};
	
	
	template <>
	class SLIB_EXPORT Atomic<sl_int32>
	{
	public:
		Atomic() noexcept;

		Atomic(sl_int32 value) noexcept;

	public:
		sl_int32 operator=(sl_int32 value) noexcept;

		operator sl_int32() const noexcept;

	public:
		sl_int32 increase() noexcept;

		sl_int32 decrease() noexcept;

		sl_int32 add(sl_int32 other) noexcept;

		sl_bool waitZero(sl_int32 timeout = -1) noexcept;

	private:
		volatile sl_int32 m_value;

	};
	
	typedef Atomic<sl_int32> AtomicInt32;
	
	
	template <class T>
	struct RemoveAtomic;
	
	template <class T>
	struct RemoveAtomic< Atomic<T> > { typedef T Type; };
	
	
	template <class T>
	class Compare< Atomic<T>, Atomic<T> >
	{
	public:
		sl_compare_result operator()(const T& a, const T& b) const
		{
			return Compare<T>()(a, b);
		}
	};
	
	template <class T>
	class Equals< Atomic<T>, Atomic<T> >
	{
	public:
		sl_bool operator()(const T& a, const T& b) const
		{
			return Equals<T>()(a, b);
		}
	};
	
	template <class T>
	class Hash< Atomic<T> >
	{
	public:
		sl_size operator()(const T& a) const
		{
			return Hash<T>()(a);
		}
	};
	
}

#endif
