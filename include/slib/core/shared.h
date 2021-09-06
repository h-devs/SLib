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
#include "unique_ptr.h"

#define PRIV_SLIB_DEFINE_SHARED_CLASS_MEMBERS \
public: \
	Container* container; \
public: \
	SLIB_CONSTEXPR Shared(): container(sl_null) {} \
	SLIB_CONSTEXPR Shared(sl_null_t): container(sl_null) {} \
	Shared(Shared&& other) noexcept \
	{ \
		container = other.container; \
		other.container = sl_null; \
	} \
	Shared(const Shared& other) noexcept \
	{ \
		Container* o = other.container; \
		if (o) { \
			o->increaseReference(); \
		} \
		container = o; \
	} \
	~Shared() \
	{ \
		if (container) { \
			container->decreaseReference(); \
			container = sl_null; \
		} \
	} \
public: \
	static const Shared& null() noexcept \
	{ \
		return *(reinterpret_cast<Shared const*>(&(priv::shared::g_shared_null))); \
	} \
	SLIB_CONSTEXPR sl_bool isNull() const \
	{ \
		return !container; \
	} \
	SLIB_CONSTEXPR sl_bool isNotNull() const \
	{ \
		return container != sl_null; \
	} \
	void setNull() \
	{ \
		_replace(sl_null); \
	} \
public: \
	Shared& operator=(Shared&& other) \
	{ \
		if (this != &other) { \
			_replace(other.container); \
			other.container = sl_null; \
		} \
		return *this; \
	} \
	Shared& operator=(const Shared& other) \
	{ \
		Container* o = other.container; \
		if (container != o) { \
			if (o) { \
				o->increaseReference(); \
			} \
			_replace(o); \
		} \
		return *this; \
	} \
	SLIB_CONSTEXPR explicit operator sl_bool() const \
	{ \
		return container != sl_null; \
	} \
	SLIB_CONSTEXPR T* operator->() const \
	{ \
		return get(); \
	} \
	template <class OTHER> \
	sl_bool equals(const Shared<OTHER>& other) const noexcept \
	{ \
		return *((void**)this) == *((void**)&other); \
	} \
	template <class OTHER> \
	sl_bool equals(const AtomicShared<OTHER>& other) const noexcept \
	{ \
		return *((void**)this) == *((void**)&other); \
	} \
	template <class OTHER> \
	sl_compare_result compare(const Shared<OTHER>& other) const noexcept \
	{ \
		return ComparePrimitiveValues(*((void**)this), *((void**)&other)); \
	} \
	template <class OTHER> \
	sl_compare_result compare(const AtomicShared<T>& other) const noexcept \
	{ \
		return ComparePrimitiveValues(*((void**)this), *((void**)&other)); \
	} \
	SLIB_DEFINE_CLASS_DEFAULT_COMPARE_OPERATORS_CONSTEXPR \
private: \
	Shared(Container* _container) noexcept: container(_container) {} \
	void _replace(Container* other) \
	{ \
		if (container) { \
			container->decreaseReference(); \
		} \
		container = other; \
	}

#define PRIV_SLIB_DEFINE_ATOMIC_SHARED_CLASS_MEMBERS \
public: \
	Container* _container; \
private: \
	SpinLock m_lock; \
public: \
	SLIB_CONSTEXPR Atomic(): _container(sl_null) {} \
	SLIB_CONSTEXPR Atomic(sl_null_t): _container(sl_null) {} \
	Atomic(const Atomic& other) noexcept \
	{ \
		_container = other._retain(); \
	} \
	Atomic(typename RemoveAtomic<Atomic>::Type && other) noexcept \
	{ \
		_container = other.container; \
		other.container = sl_null; \
	} \
	Atomic(typename RemoveAtomic<Atomic>::Type const& other) noexcept \
	{ \
		Container* o = other.container; \
		if (o) { \
			o->increaseReference(); \
		} \
		_container = o; \
	} \
	~Atomic() \
	{ \
		if (_container) { \
			_container->decreaseReference(); \
			_container = sl_null; \
		} \
	} \
public: \
	SLIB_CONSTEXPR sl_bool isNull() const \
	{ \
		return !_container; \
	} \
	SLIB_CONSTEXPR sl_bool isNotNull() const \
	{ \
		return _container != sl_null; \
	} \
	void setNull() \
	{ \
		_replace(sl_null); \
	} \
	void swap(typename RemoveAtomic<Atomic>::Type& other) \
	{ \
		m_lock.lock(); \
		Container* before = _container; \
		_container = other.container; \
		other.container = before; \
		m_lock.unlock(); \
	} \
	sl_bool compareExchange(typename RemoveAtomic<Atomic>::Type& expected, typename RemoveAtomic<Atomic>::Type&& desired) \
	{ \
		m_lock.lock(); \
		Container* before = _container; \
		if (before == expected.container) { \
			_container = desired.container; \
			m_lock.unlock(); \
			desired.container = sl_null; \
			if (before) { \
				before->decreaseReference(); \
			} \
			return sl_true; \
		} else { \
			if (before) { \
				before->increaseReference(); \
			} \
			m_lock.unlock(); \
			if (expected.container) { \
				expected.container->decreaseReference(); \
			} \
			expected.container = before; \
			return sl_false; \
		} \
	} \
public: \
	Atomic& operator=(const Atomic& other) \
	{ \
		if (_container != other._container) { \
			_replace(other._retain()); \
		} \
		return *this; \
	} \
	Atomic& operator=(typename RemoveAtomic<Atomic>::Type&& other) \
	{ \
		if ((void*)this != &other) { \
			_replace(other.container); \
			other.container = sl_null; \
		} \
		return *this; \
	} \
	Atomic& operator=(typename RemoveAtomic<Atomic>::Type const& other) \
	{ \
		Container* o = other.container; \
		if (_container != o) { \
			if (o) { \
				o->increaseReference(); \
			} \
			_replace(o); \
		} \
		return *this; \
	} \
	explicit SLIB_CONSTEXPR operator sl_bool() const \
	{ \
		return _container != sl_null; \
	} \
	template <class OTHER> \
	sl_bool equals(const Shared<OTHER>& other) const noexcept \
	{ \
		return *((void**)this) == *((void**)&other); \
	} \
	template <class OTHER> \
	sl_bool equals(const AtomicShared<OTHER>& other) const noexcept \
	{ \
		return *((void**)this) == *((void**)&other); \
	} \
	template <class OTHER> \
	sl_compare_result compare(const Shared<OTHER>& other) const noexcept \
	{ \
		return ComparePrimitiveValues(*((void**)this), *((void**)&other)); \
	} \
	template <class OTHER> \
	sl_compare_result compare(const AtomicShared<T>& other) const noexcept \
	{ \
		return ComparePrimitiveValues(*((void**)this), *((void**)&other)); \
	} \
	SLIB_DEFINE_CLASS_DEFAULT_COMPARE_OPERATORS_CONSTEXPR \
private: \
	void _replace(Container* other) \
	{ \
		m_lock.lock(); \
		Container* before = _container; \
		_container = other; \
		m_lock.unlock(); \
		if (before) { \
			before->decreaseReference(); \
		} \
	} \
public: \
	Container* _retain() const noexcept \
	{ \
		if (!_container) { \
			return sl_null; \
		} \
		m_lock.lock(); \
		Container* p = _container; \
		if (p) { \
			p->increaseReference(); \
		} \
		m_lock.unlock(); \
		return p; \
	}

namespace slib
{

	template <class T>
	class Shared;

	template <class T>
	using AtomicShared = Atomic< Shared<T> >;

	template <class T>
	using SharedPtr = Shared<T*>;

	template <class T>
	using AtomicSharedPtr = Atomic< Shared<T*> >;

	namespace priv
	{
		namespace shared
		{

			extern void* const g_shared_null;

			template <class T>
			class SharedContainer
			{
			public:
				T value;
				sl_reg refCount;

			public:
				template <class... ARGS>
				SharedContainer(ARGS&&... args) noexcept: value(Forward<ARGS>(args)...), refCount(1) {}

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

			template <class T, sl_bool CanBeBool = __is_class(T) && SLIB_IS_CONVERTIBLE(T, sl_bool)>
			class SharedContainerHelper;

			template <class T>
			class SharedContainerHelper<T, sl_true>
			{
			public:
				template <class VALUE>
				static SharedContainer<T>* create(VALUE&& value)
				{
					if (value) {
						return new SharedContainer<T>(Forward<VALUE>(value));
					} else {
						return sl_null;
					}
				}
			};

			template <class T>
			class SharedContainerHelper<T, sl_false>
			{
			public:
				template <class VALUE>
				static SharedContainer<T>* create(VALUE&& value)
				{
					return new SharedContainer<T>(Forward<VALUE>(value));
				}
			};

			class PtrContainer
			{
			public:
				sl_reg refCount;
				void* ptr;

			public:
				PtrContainer() noexcept: refCount(1) {}

				virtual ~PtrContainer();

			public:
				sl_reg increaseReference() noexcept;

				sl_reg decreaseReference();

			};
			
			template <class T>
			class GenericContainer : public PtrContainer
			{
			public:
				T* ptr2;

			public:
				GenericContainer(void* _ptr, T* _ptr2) noexcept
				{
					ptr = _ptr;
					ptr2 = _ptr2;
				}

				~GenericContainer()
				{
					delete ptr2;
				}

			};

			template <class T, class Deleter>
			class DeleterContainer : public PtrContainer
			{
			public:
				T* ptr2;
				Deleter deleter;

			public:
				template <class DELETER>
				DeleterContainer(void* _ptr, const T* _ptr2, DELETER&& _deleter) noexcept: deleter(Forward<DELETER>(_deleter))
				{
					ptr = _ptr;
					ptr2 = _ptr2;
				}

				~DeleterContainer()
				{
					deleter(ptr2);
				}

			};

			template <class OBJECT>
			class ObjectContainer : public PtrContainer
			{
			public:
				OBJECT object;

			public:
				template <class T, class... ARGS>
				ObjectContainer(T*, ARGS&&... args) noexcept: object(Forward<ARGS>(args)...)
				{
					ptr = (T*)(&object);
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
	class SLIB_EXPORT Shared
	{
	public:
		typedef typename priv::shared::SharedContainer<T> Container;
		typedef T ValueType;
		PRIV_SLIB_DEFINE_SHARED_CLASS_MEMBERS

	public:
		Shared(const AtomicShared<T>& other) noexcept;

		Shared(const T& t) noexcept: container(priv::shared::SharedContainerHelper<ValueType>::create(t)) {}

		Shared(T&& t) noexcept: container(priv::shared::SharedContainerHelper<ValueType>::create(Move(t))) {}

	public:
		template <class... Args>
		static Shared create(Args&&... args)
		{
			return new Container(Forward<Args>(args)...);
		}

		SLIB_CONSTEXPR T* get() const&
		{
			return &(container->value);
		}

	public:
		Shared& operator=(const AtomicShared<T>& other);

		Shared& operator=(const T& other)
		{
			_replace(priv::shared::SharedContainerHelper<ValueType>::create(other));
			return *this;
		}

		Shared& operator=(T&& other)
		{
			_replace(priv::shared::SharedContainerHelper<ValueType>::create(Move(other)));
			return *this;
		}

		T& operator*() const noexcept
		{
			return *(get());
		}

		operator T&() const noexcept
		{
			return *(get());
		}
		
	};

	template <class T>
	class SLIB_EXPORT Atomic< Shared<T> >
	{
	public:
		typedef typename priv::shared::SharedContainer<T> Container;
		typedef T ValueType;
		PRIV_SLIB_DEFINE_ATOMIC_SHARED_CLASS_MEMBERS

	public:
		Atomic(const T& t) noexcept: _container(priv::shared::SharedContainerHelper<ValueType>::create(t)) {}

		Atomic(T&& t) noexcept: _container(priv::shared::SharedContainerHelper<ValueType>::create(Move(t))) {}

	public:
		Atomic& operator=(const T& other)
		{
			_replace(priv::shared::SharedContainerHelper<ValueType>::create(other));
			return *this;
		}

		Atomic& operator=(T&& other)
		{
			_replace(priv::shared::SharedContainerHelper<ValueType>::create(Move(other)));
			return *this;
		}

	};
	
	template <class T>
	class SLIB_EXPORT Shared<T*>
	{
	public:
		typedef priv::shared::PtrContainer Container;
		typedef typename priv::shared::ValueType<T>::Type ValueType;
		PRIV_SLIB_DEFINE_SHARED_CLASS_MEMBERS

	public:
		Shared(const AtomicShared<T*>& other) noexcept;
		
		template <class OTHER>
		Shared(OTHER* ptr) noexcept
		{
			if (ptr) {
				container = new priv::shared::GenericContainer<OTHER>((T*)ptr, ptr);
			} else {
				container = sl_null;
			}
		}

		template <class OTHER, class DELETER>
		Shared(OTHER* ptr, DELETER&& deleter) noexcept
		{
			if (ptr) {
				container = new priv::shared::DeleterContainer<OTHER, typename RemoveConstReference<DELETER>::Type>((T*)ptr, ptr, Forward<DELETER>(deleter));
			} else {
				container = sl_null;
			}
		}

		template <class OTHER>
		Shared(UniquePtr<OTHER>&& other) noexcept: Shared(other.release()) {}

		Shared(ValueType&& t): container(new priv::shared::ObjectContainer<ValueType>((T*)0, Move(t))) {}

		Shared(const ValueType& t): container(new priv::shared::ObjectContainer<ValueType>((T*)0, t)) {}

	public:
		template <class... Args>
		static Shared create(Args&&... args)
		{
			return new priv::shared::ObjectContainer<ValueType>((T*)0, Forward<Args>(args)...);
		}

		SLIB_CONSTEXPR T* get() const&
		{
			return container ? (T*)(container->ptr) : sl_null;
		}

		template <class OTHER>
		static const Shared& from(const Shared<OTHER*>& other) noexcept
		{
			return *(reinterpret_cast<Shared const*>(&other));
		}

		template <class OTHER>
		static Shared& from(SharedPtr<OTHER*>& other) noexcept
		{
			return *(reinterpret_cast<Shared*>(&other));
		}

		template <class OTHER>
		static Shared&& from(Shared<OTHER*>&& other) noexcept
		{
			return static_cast<Shared&&>(*(reinterpret_cast<Shared*>(&other)));
		}

	public:
		Shared& operator=(const AtomicShared<T*>& other);

		template <class OTHER>
		Shared& operator=(OTHER* ptr) noexcept
		{
			if (ptr) {
				_replace(new priv::shared::GenericContainer<OTHER>((T*)ptr, ptr));
			} else {
				_replace(sl_null);
			}
			return *this;
		}

		template <class OTHER>
		Shared& operator=(UniquePtr<OTHER>&& other)
		{
			return *this = other.release();
		}

		Shared& operator=(ValueType&& other)
		{
			_replace(new priv::shared::ObjectContainer<ValueType>((T*)0, Move(other)));
			return *this;
		}

		Shared& operator=(const ValueType& other)
		{
			_replace(new priv::shared::ObjectContainer<ValueType>((T*)0, other));
			return *this;
		}

		ValueType& operator*() const noexcept
		{
			return *(get());
		}

	};

	template <class T>
	class SLIB_EXPORT Atomic< Shared<T*> >
	{
	public:
		typedef priv::shared::PtrContainer Container;
		typedef typename priv::shared::ValueType<T>::Type ValueType;
		PRIV_SLIB_DEFINE_ATOMIC_SHARED_CLASS_MEMBERS

	public:
		template <class OTHER>
		Atomic(OTHER* ptr) noexcept
		{
			if (ptr) {
				_container = new priv::shared::GenericContainer<OTHER>((T*)ptr, ptr);
			} else {
				_container = sl_null;
			}
		}

		template <class OTHER, class DELETER>
		Atomic(OTHER* ptr, DELETER&& deleter) noexcept
		{
			if (ptr) {
				_container = new priv::shared::DeleterContainer<OTHER, typename RemoveConstReference<DELETER>::Type>((T*)ptr, ptr, Forward<DELETER>(deleter));
			} else {
				_container = sl_null;
			}
		}

		template <class OTHER>
		Atomic(UniquePtr<OTHER>&& other) noexcept: Atomic(other.release()) {}

		Atomic(ValueType&& t): _container(new priv::shared::ObjectContainer<ValueType>((T*)0, Move(t))) {}

		Atomic(const ValueType& t): _container(new priv::shared::ObjectContainer<ValueType>((T*)0, t)) {}

	public:
		template <class OTHER>
		static const Atomic& from(const AtomicShared<OTHER*>& other) noexcept
		{
			return *(reinterpret_cast<Atomic const*>(&other));
		}

		template <class OTHER>
		static Atomic& from(AtomicShared<OTHER*>& other) noexcept
		{
			return *(reinterpret_cast<Atomic*>(&other));
		}

	public:
		template <class OTHER>
		Atomic& operator=(OTHER* ptr) noexcept
		{
			if (ptr) {
				_replace(new priv::shared::GenericContainer<OTHER>((T*)ptr, ptr));
			} else {
				_replace(sl_null);
			}
			return *this;
		}

		template <class OTHER>
		Atomic& operator=(UniquePtr<OTHER>&& other)
		{
			return *this = other.release();
		}

		Atomic& operator=(ValueType&& other)
		{
			_replace(new priv::shared::ObjectContainer<ValueType>((T*)0, Move(other)));
			return *this;
		}

		Atomic& operator=(const ValueType& other)
		{
			_replace(new priv::shared::ObjectContainer<ValueType>((T*)0, other));
			return *this;
		}

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

	template <class T>
	Shared<T*>::Shared(const AtomicShared<T*>& other) noexcept
	{
		container = other._retain();
	}

	template <class T>
	Shared<T*>& Shared<T*>::operator=(const AtomicShared<T*>& other)
	{
		if (container != other._container) {
			_replace(other._retain());
		}
		return *this;
	}


	template <class T>
	SLIB_INLINE static Shared<typename RemoveConstReference<T>::Type> ToShared(T&& t)
	{
		return Shared<typename RemoveConstReference<T>::Type>::create(Forward<T>(t));
	}

}

#endif
