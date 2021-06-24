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

#ifndef CHECKHEADER_SLIB_CORE_SHARED
#define CHECKHEADER_SLIB_CORE_SHARED

#include "atomic.h"
#include "base.h"

namespace slib
{

	template <class T>
	class Shared;

	template <class T>
	using AtomicShared = Atomic< Shared<T> >;

	namespace priv
	{
		namespace shared
		{

			extern void* const g_shared_null;

			template <class T>
			class SharedContainer
			{
			public:
				T object;
				sl_reg refCount;

			public:
				template <class... ARGS>
				SharedContainer(ARGS&&... args) noexcept: object(Forward<ARGS>(args)...), refCount(1) {}

			public:
				sl_reg increaseReference() noexcept
				{
					return Base::interlockedIncrement(&refCount);
				}

				sl_reg decreaseReference()
				{
					sl_reg nRef = Base::interlockedDecrement(&refCount);
					if (!nRef) {
						delete this;
					}
					return nRef;
				}

			};

		}
	}

	template <class T>
	class SLIB_EXPORT Shared
	{
		typedef typename priv::shared::SharedContainer<T> Container;

	public:
		constexpr Shared(): container(sl_null) {}

		constexpr Shared(sl_null_t): container(sl_null) {}

		Shared(Shared&& other) noexcept
		{
			container = other.container;
			other.container = sl_null;
		}

		Shared(const Shared& other) noexcept
		{
			Container* o = other.container;
			if (o) {
				o->increaseReference();
			}
			container = o;
		}

		Shared(const AtomicShared<T>& other) noexcept;

		Shared(const T& t) noexcept: container(new Container(t)) {}

		Shared(T&& t) noexcept: container(new Container(Move(t))) {}

		~Shared()
		{
			if (container) {
				container->decreaseReference();
				container = sl_null;
			}
		}

	private:
		Shared(Container* _container) noexcept: container(_container) {}

	public:
		template <class... Args>
		static Shared create(Args&&... args)
		{
			return new Container(Forward<Args>(args)...);
		}

		static const Shared& null() noexcept
		{
			return *(reinterpret_cast<Shared const*>(&(priv::shared::g_shared_null)));
		}
	
		constexpr sl_bool isNull() const
		{
			return !container;
		}

		constexpr sl_bool isNotNull() const
		{
			return container != sl_null;
		}

		void setNull()
		{
			_replace(sl_null);
		}

		constexpr T* get() const&
		{
			return &(container->object);
		}

	public:
		Shared& operator=(Shared&& other)
		{
			_move_assign(&other);
			return *this;
		}

		Shared& operator=(const Shared& other)
		{
			Container* o = other.container;
			if (container != o) {
				if (o) {
					o->increaseReference();
				}
				_replace(o);
			}
			return *this;
		}

		Shared& operator=(const AtomicShared<T>& other);

		Shared& operator=(const T& other)
		{
			_replace(new Container(other));
			return *this;
		}

		Shared& operator=(T&& other)
		{
			_replace(new Container(Move(other)));
			return *this;
		}

		T& operator*() const noexcept
		{
			return container->object;
		}

		constexpr T* operator->() const
		{
			return &(container->object);
		}

		constexpr explicit operator sl_bool() const
		{
			return container != sl_null;
		}

	public:
		constexpr sl_bool equals(const Shared& other) const
		{
			return container == other.container;
		}

		sl_bool equals(const AtomicShared<T>& other) const noexcept
		{
			return container == ((Shared*)((void*)&other))->container;
		}

		constexpr sl_compare_result compare(const Shared& other) const
		{
			return ComparePrimitiveValues(container, other.container);
		}

		sl_compare_result compare(const AtomicShared<T>& other) const
		{
			return ComparePrimitiveValues(container, ((Shared*)((void*)&other))->container);
		}
		
		SLIB_DEFINE_CLASS_DEFAULT_COMPARE_OPERATORS_CONSTEXPR

	private:
		void _replace(Container* other)
		{
			if (container) {
				container->decreaseReference();
			}
			container = other;
		}
		
		void _move_assign(void* _other)
		{
			if ((void*)this != _other) {
				Shared& other = *(reinterpret_cast<Shared*>(_other));
				_replace(other.container);
				other.container = sl_null;
			}
		}

	public:
		Container* container;

	};

	template <class T>
	class SLIB_EXPORT Atomic< Shared<T> >
	{
		typedef typename priv::shared::SharedContainer<T> Container;

	public:
		constexpr Atomic(): _container(sl_null) {}

		constexpr Atomic(sl_null_t): _container(sl_null) {}

		Atomic(const Atomic& other) noexcept
		{
			_container = other._retain();
		}

		Atomic(Shared<T>&& other) noexcept
		{
			_container = other.container;
			other.container = sl_null;
		}

		Atomic(const Shared<T>& other) noexcept
		{
			Container* o = other.container;
			if (o) {
				o->increaseReference();
			}
			_container = o;
		}

		Atomic(T&& t) noexcept: _container(new Container(Move(t))) {}

		~Atomic()
		{
			if (_container) {
				_container->decreaseReference();
				_container = sl_null;
			}
		}

	public:
		constexpr sl_bool isNull() const
		{
			return !_container;
		}

		constexpr sl_bool isNotNull() const
		{
			return _container != sl_null;
		}

		void setNull()
		{
			_replace(sl_null);
		}

	public:
		Atomic& operator=(const Atomic& other)
		{
			if (_container != other._container) {
				_replace(other._retain());
			}
			return *this;
		}
		
		Atomic& operator=(Shared<T>&& other)
		{
			_move_assign(&other);
			return *this;
		}

		Atomic& operator=(const Shared<T>& other)
		{
			Container* o = other.container;
			if (_container != o) {
				if (o) {
					o->increaseReference();
				}
				_replace(o);
			}
			return *this;
		}

		Atomic& operator=(T&& other) noexcept
		{
			_replace(new Container(Move(other)));
			return *this;
		}

		Shared<T> operator*() const noexcept
		{
			return *this;
		}

		explicit constexpr operator sl_bool() const
		{
			return _container != sl_null;
		}

	public:
		constexpr sl_bool equals(const Shared<T>& other) const
		{
			return _container == other.container;
		}

		constexpr sl_bool equals(const Atomic& other) const
		{
			return _container == other._container;
		}

		constexpr sl_compare_result compare(const Shared<T>& other) const
		{
			return ComparePrimitiveValues(_container, other.container);
		}

		constexpr sl_compare_result compare(const Atomic& other) const
		{
			return ComparePrimitiveValues(_container, other._container);
		}

		SLIB_DEFINE_CLASS_DEFAULT_COMPARE_OPERATORS_CONSTEXPR

	public:
		Container* _retain() const noexcept
		{
			if (!_container) {
				return sl_null;
			}
			m_lock.lock();
			Container* p = _container;
			if (p) {
				p->increaseReference();
			}
			m_lock.unlock();
			return p;
		}

		void _replace(Container* other)
		{
			m_lock.unlock();
			Container* before = _container;
			_container = other;
			m_lock.unlock();
			if (before) {
				before->decreaseReference();
			}
		}

		void _move_assign(void* _other)
		{
			if ((void*)this != _other) {
				Shared<T>& other = *(reinterpret_cast<Shared<T>*>(_other));
				_replace(other.container);
				other.container = sl_null;
			}
		}

	public:
		Container* _container;

	private:
		SpinLock m_lock;

	};
	
	template <class T>
	Shared<T>::Shared(const AtomicShared<T>& other) noexcept
	{
		container = other._retain();
	}

	template <class T>
	Shared<T>& Shared<T>::operator=(const AtomicShared<T>& other)
	{
		if (container != other._container) {
			_replace(other._retain());
		}
		return *this;
	}

}

#endif
