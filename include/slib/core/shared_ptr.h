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

#ifndef CHECKHEADER_SLIB_CORE_SHARED_PTR
#define CHECKHEADER_SLIB_CORE_SHARED_PTR

#include "atomic.h"

namespace slib
{

	template <class T>
	class SharedPtr;

	template <class T>
	using AtomicSharedPtr = Atomic< SharedPtr<T> >;


	class SLIB_EXPORT CSharedPtrBase
	{
	public:
		constexpr CSharedPtrBase() noexcept: refCount(1) {}

		virtual ~CSharedPtrBase();

	public:
		sl_reg increaseReference() noexcept;

		sl_reg decreaseReference() noexcept;

	public:
		sl_reg refCount;

	};

	template <class T>
	class SLIB_EXPORT CSharedPtr : public CSharedPtrBase
	{
	public:
		T* ptr;

	};
	
	namespace priv
	{
		namespace ptr
		{

			extern void* const g_shared_null;

			template <class T>
			class SharedPtrContainer : public CSharedPtr<T>
			{
			public:
				SharedPtrContainer(T* _ptr)
				{
					this->ptr = _ptr;
				}

				~SharedPtrContainer()
				{
					delete this->ptr;
				}

			};

			template <class T, class Deleter>
			class SharedPtrContainerWithDeleter : public CSharedPtr<T>
			{
			public:
				Deleter deleter;

			public:
				template <class DELETER>
				SharedPtrContainerWithDeleter(T* _ptr, DELETER&& _deleter): deleter(Forward<DELETER>(_deleter))
				{
					this->ptr = _ptr;
				}

				~SharedPtrContainerWithDeleter()
				{
					deleter(this->ptr);
				}

			};

			template <class T>
			class SharedObjectContainer : public CSharedPtr<T>
			{
			public:
				T object;

			public:
				template <class... ARGS>
				SharedObjectContainer(ARGS&&... args): object(Forward<ARGS>(args)...)
				{
					this->ptr = &object;
				}

			};

			template <class T>
			struct ValueType
			{
				typedef T Type;
			};
			
			template <>
			struct ValueType<void>
			{
				typedef int Type;
			};

		}
	}

	template <class T>
	class SLIB_EXPORT SharedPtr
	{
		typedef CSharedPtr<T> Container;
		typedef typename priv::ptr::ValueType<T>::Type ValueType;

	public:
		SharedPtr() noexcept: container(sl_null) {}

		SharedPtr(SharedPtr&& other) noexcept
		{
			container = other.container;
			other.container = sl_null;
		}

		SharedPtr(const SharedPtr& other) noexcept
		{
			Container* o = other.container;
			if (o) {
				o->increaseReference();
			}
			container = o;
		}

		template <class OTHER>
		SharedPtr(SharedPtr<OTHER>&& other) noexcept
		{
			container = reinterpret_cast<Container*>(other.container);
			other.container = sl_null;
		}

		template <class OTHER>
		SharedPtr(const SharedPtr<OTHER>& other) noexcept
		{
			Container* o = reinterpret_cast<Container*>(other.container);
			if (o) {
				o->increaseReference();
			}
			container = o;
		}

		template <class OTHER>
		SharedPtr(const AtomicSharedPtr<OTHER>& other) noexcept
		{
			container = reinterpret_cast<Container*>(other._retain());
		}

		SharedPtr(sl_null_t) noexcept: container(sl_null) {}

		SharedPtr(const T* ptr) noexcept
		{
			if (ptr) {
				container = new priv::ptr::SharedPtrContainer<T>((T*)ptr);
			} else {
				container = sl_null;
			}
		}

		template <class DELETER>
		SharedPtr(const T* ptr, DELETER&& deleter) noexcept
		{
			if (ptr) {
				container = new priv::ptr::SharedPtrContainerWithDeleter<T, typename RemoveConstReference<DELETER>::Type>((T*)ptr, Forward<DELETER>(deleter));
			} else {
				container = sl_null;
			}
		}

		SharedPtr(ValueType&& t) noexcept: container(new priv::ptr::SharedObjectContainer<ValueType>(Move(t))) {}

		SharedPtr(const ValueType& t) noexcept: container(new priv::ptr::SharedObjectContainer<ValueType>(t)) {}

		~SharedPtr()
		{
			if (container) {
				container->decreaseReference();
				container = sl_null;
			}
		}

	private:
		SharedPtr(Container* _container) noexcept: container(_container) {}

	public:
		template <class... Args>
		static SharedPtr create(Args&&... args)
		{
			return new priv::ptr::SharedObjectContainer<ValueType>(Forward<Args>(args)...);
		}

		static const SharedPtr& null() noexcept
		{
			return *(reinterpret_cast<SharedPtr const*>(&(priv::ptr::g_shared_null)));
		}
	
		sl_bool isNull() const noexcept
		{
			return !container;
		}

		sl_bool isNotNull() const noexcept
		{
			return container != sl_null;
		}

		void setNull() noexcept
		{
			_replace(sl_null);
		}

		T* get() const& noexcept
		{
			if (container) {
				return container->ptr;
			} else {
				return sl_null;
			}
		}

		template <class OTHER>
		static const SharedPtr& from(const SharedPtr<OTHER>& other) noexcept
		{
			return *(reinterpret_cast<SharedPtr const*>(&other));
		}

		template <class OTHER>
		static SharedPtr& from(SharedPtr<OTHER>& other) noexcept
		{
			return *(reinterpret_cast<SharedPtr*>(&other));
		}

		template <class OTHER>
		static SharedPtr&& from(SharedPtr<OTHER>&& other) noexcept
		{
			return static_cast<SharedPtr&&>(*(reinterpret_cast<SharedPtr*>(&other)));
		}

	public:
		SharedPtr& operator=(SharedPtr&& other) noexcept
		{
			_move_assign(&other);
			return *this;
		}

		SharedPtr& operator=(const SharedPtr& other) noexcept
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

		template <class OTHER>
		SharedPtr& operator=(SharedPtr<OTHER>&& other) noexcept
		{
			_move_assign(&other);
			return *this;
		}

		template <class OTHER>
		SharedPtr& operator=(const SharedPtr<OTHER>& other) noexcept
		{
			Container* o = reinterpret_cast<Container*>(other.container);
			if (container != o) {
				if (o) {
					o->increaseReference();
				}
				_replace(o);
			}
			return *this;
		}

		template <class OTHER>
		SharedPtr& operator=(const AtomicSharedPtr<OTHER>& other) noexcept
		{
			if ((void*)container != (void*)(other.container)) {
				_replace(other._retain());
			}
			return *this;
		}

		SharedPtr& operator=(ValueType&& other) noexcept
		{
			_replace(new priv::ptr::SharedObjectContainer<ValueType>(Move(other)));
			return *this;
		}

		SharedPtr& operator=(const ValueType& other) noexcept
		{
			_replace(new priv::ptr::SharedObjectContainer<ValueType>(other));
			return *this;
		}

		template <class OTHER>
		sl_bool operator==(const SharedPtr<OTHER>& other) const noexcept
		{
			return (const void*)(get()) == (const void*)(other.get());
		}

		template <class OTHER>
		sl_bool operator==(const AtomicSharedPtr<OTHER>& other) const noexcept
		{
			return *this == SharedPtr(other);
		}

		template <class OTHER>
		sl_bool operator!=(const SharedPtr<OTHER>& other) const noexcept
		{
			return (const void*)(get()) != (const void*)(other.get());
		}

		template <class OTHER>
		sl_bool operator!=(const AtomicSharedPtr<OTHER>& other) const noexcept
		{
			return *this != SharedPtr(other);
		}

		ValueType& operator*() const noexcept
		{
			return *reinterpret_cast<ValueType*>(container->ptr);
		}

		T* operator->() const noexcept
		{
			return container->ptr;
		}

		explicit operator sl_bool() const noexcept
		{
			return container != sl_null;
		}
		
	private:
		void _replace(Container* other) noexcept
		{
			if (container) {
				container->decreaseReference();
			}
			container = other;
		}
		
		void _move_assign(void* _other) noexcept
		{
			if ((void*)this != _other) {
				SharedPtr& other = *(reinterpret_cast<SharedPtr*>(_other));
				_replace(other.container);
				other.container = sl_null;
			}
		}

	public:
		Container * container;

	};

	template <class T>
	class SLIB_EXPORT Atomic< SharedPtr<T> >
	{
		typedef CSharedPtr<T> Container;
		typedef typename priv::ptr::ValueType<T>::Type ValueType;

	public:
		Atomic() noexcept : _container(sl_null) {}

		Atomic(const Atomic& other) noexcept
		{
			_container = other._retain();
		}

		template <class OTHER>
		Atomic(const AtomicSharedPtr<OTHER>& other) noexcept
		{
			_container = reinterpret_cast<Container*>(other._retain());
		}

		template <class OTHER>
		Atomic(SharedPtr<OTHER>&& other) noexcept
		{
			_container = other.container;
			other.container = sl_null;
		}

		template <class OTHER>
		Atomic(const SharedPtr<OTHER>& other) noexcept
		{
			Container* o = reinterpret_cast<Container*>(other.container);
			if (o) {
				o->increaseReference();
			}
			_container = o;
		}

		Atomic(sl_null_t) noexcept: _container(sl_null) {}

		Atomic(const T* ptr) noexcept
		{
			if (ptr) {
				_container = new priv::ptr::SharedPtrContainer<T>((T*)ptr);
			} else {
				_container = sl_null;
			}
		}

		template <class DELETER>
		Atomic(const T* ptr, DELETER&& deleter) noexcept
		{
			if (ptr) {
				_container = new priv::ptr::SharedPtrContainerWithDeleter<T, typename RemoveConstReference<DELETER>::Type>((T*)ptr, Forward<DELETER>(deleter));
			} else {
				_container = sl_null;
			}
		}

		Atomic(ValueType&& t) noexcept: _container(new priv::ptr::SharedObjectContainer<ValueType>(Move(t))) {}

		Atomic(const ValueType& t) noexcept: _container(new priv::ptr::SharedObjectContainer<ValueType>(t)) {}

		~Atomic()
		{
			if (_container) {
				_container->decreaseReference();
				_container = sl_null;
			}
		}

	public:
		sl_bool isNull() const noexcept
		{
			return !_container;
		}

		sl_bool isNotNull() const noexcept
		{
			return _container != sl_null;
		}

		void setNull() noexcept
		{
			_replace(sl_null);
		}

		template <class OTHER>
		static const Atomic& from(const AtomicSharedPtr<OTHER>& other) noexcept
		{
			return *(reinterpret_cast<Atomic const*>(&other));
		}

		template <class OTHER>
		static Atomic& from(AtomicSharedPtr<OTHER>& other) noexcept
		{
			return *(reinterpret_cast<Atomic*>(&other));
		}

	public:
		Atomic & operator=(const Atomic& other) noexcept
		{
			if (_container != other._container) {
				_replace(other._retain());
			}
			return *this;
		}

		template <class OTHER>
		Atomic& operator=(const AtomicSharedPtr<OTHER>& other) noexcept
		{
			if (_container != reinterpret_cast<Container*>(other._container)) {
				_replace(other._retain());
			}
			return *this;
		}

		template <class OTHER>
		Atomic& operator=(SharedPtr<OTHER>&& other) noexcept
		{
			_move_assign(&other);
			return *this;
		}

		template <class OTHER>
		Atomic& operator=(const SharedPtr<OTHER>& other) noexcept
		{
			Container* o = reinterpret_cast<Container*>(other.container);
			if (_container != o) {
				if (o) {
					o->increaseReference();
				}
				_replace(o);
			}
			return *this;
		}

		Atomic& operator=(ValueType&& other) noexcept
		{
			_replace(new priv::ptr::SharedObjectContainer<ValueType>(Move(other)));
			return *this;
		}

		Atomic& operator=(const ValueType& other) noexcept
		{
			_replace(new priv::ptr::SharedObjectContainer<ValueType>(other));
			return *this;
		}

		template <class OTHER>
		sl_bool operator==(const AtomicSharedPtr<OTHER>& other) const noexcept
		{
			return SharedPtr<T>(*this) == SharedPtr<OTHER>(other);
		}

		template <class OTHER>
		sl_bool operator==(const SharedPtr<OTHER>& other) const noexcept
		{
			return SharedPtr<T>(*this) == other;
		}

		template <class OTHER>
		sl_bool operator!=(const AtomicSharedPtr<OTHER>& other) const noexcept
		{
			return SharedPtr<T>(*this) != SharedPtr<OTHER>(other);
		}

		template <class OTHER>
		sl_bool operator!=(const SharedPtr<OTHER>& other) const noexcept
		{
			return SharedPtr<T>(*this) != other;
		}

		SharedPtr<T> operator*() const noexcept
		{
			return *this;
		}

		explicit operator sl_bool() const noexcept
		{
			return _container != sl_null;
		}

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

		void _replace(Container* other) noexcept
		{
			m_lock.unlock();
			Container* before = _container;
			_container = other;
			m_lock.unlock();
			if (before) {
				before->decreaseReference();
			}
		}

		void _move_assign(void* _other) noexcept
		{
			if ((void*)this != _other) {
				SharedPtr<T>& other = *(reinterpret_cast<SharedPtr<T>*>(_other));
				_replace(other.container);
				other.container = sl_null;
			}
		}

	public:
		Container* _container;

	private:
		SpinLock m_lock;

	};
	
}

#endif
