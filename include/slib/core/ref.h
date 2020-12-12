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

#ifndef CHECKHEADER_SLIB_CORE_REF
#define CHECKHEADER_SLIB_CORE_REF

#include "definition.h"

#include "base.h"
#include "atomic.h"
#include "macro.h"

typedef const void* sl_object_type;

namespace slib
{
	namespace priv
	{
		namespace ref
		{
			struct ConstStruct;

			extern const ConstStruct g_null;
		}
	}

	class CWeakRef;
	
	class SLIB_EXPORT Referable
	{
	public:
		Referable() noexcept;

		Referable(const Referable& other) noexcept;
		
		Referable(Referable&& other) noexcept;

		virtual ~Referable();

	public:
		sl_reg increaseReference() noexcept;

		sl_reg decreaseReference() noexcept;

		sl_reg decreaseReferenceNoFree() noexcept;
		
		sl_reg getReferenceCount() noexcept;
		
	protected:
		virtual void init();

	public:
		static sl_object_type ObjectType() noexcept;
		
		static sl_bool isDerivedFrom(sl_object_type type) noexcept;
		
		virtual sl_object_type getObjectType() const noexcept;

		virtual sl_bool isInstanceOf(sl_object_type type) const noexcept;

	private:
		void _clearWeak() noexcept;

	public:
		sl_bool _isWeakRef() const noexcept;

		CWeakRef* _getWeakObject() noexcept;

		void _free() noexcept;
		
	public:
		Referable& operator=(const Referable& other);
		
		Referable& operator=(Referable&& other);

	private:
		sl_reg m_nRefCount;
		CWeakRef* m_weak;

		friend class CWeakRef;
		
	};

	
	template <class... TYPES>
	class Ref;
	
	template <class T>
	class WeakRef;
	
	template <class T>
	using AtomicRef = Atomic< Ref<T> >;
	
	template <class T>
	using AtomicWeakRef = Atomic< WeakRef<T> >;


	template <class... TYPES>
	class Pointer;
	

	template <class T>
	class SLIB_EXPORT Ref<T>
	{
	public:
		constexpr Ref() noexcept : ptr(sl_null) {}

		constexpr Ref(sl_null_t) noexcept : ptr(sl_null) {}

		Ref(T* other) noexcept
		{
			if (other) {
				other->increaseReference();
			}
			ptr = other;
		}

		Ref(Ref<T>&& other) noexcept
		{
			_move_init(&other);
		}

		Ref(const Ref<T>& other) noexcept
		{
			T* o = other.ptr;
			if (o) {
				o->increaseReference();
			}
			ptr = o;
		}

		template <class OTHER>
		Ref(Ref<OTHER>&& other) noexcept
		{
			SLIB_TRY_CONVERT_TYPE(OTHER*, T*)
			_move_init(&other);
		}

		template <class OTHER>
		Ref(const Ref<OTHER>& other) noexcept
		{
			SLIB_TRY_CONVERT_TYPE(OTHER*, T*)
			T* o = other.ptr;
			if (o) {
				o->increaseReference();
			}
			ptr = o;
		}

		template <class OTHER>
		Ref(const AtomicRef<OTHER>& other) noexcept
		{
			SLIB_TRY_CONVERT_TYPE(OTHER*, T*)
			T* o = other._retainObject();
			ptr = o;
		}

		template <class OTHER>
		Ref(const WeakRef<OTHER>& _other) noexcept
		{
			SLIB_TRY_CONVERT_TYPE(OTHER*, T*)
			Ref<OTHER> other(_other.lock());
			T* o = other.ptr;
			if (o) {
				o->increaseReference();
			}
			ptr = o;
		}

		template <class OTHER>
		Ref(const AtomicWeakRef<OTHER>& _other) noexcept
		{
			SLIB_TRY_CONVERT_TYPE(OTHER*, T*)
			Ref<OTHER> other(_other.lock());
			T* o = other.ptr;
			if (o) {
				o->increaseReference();
			}
			ptr = o;
		}

		template <class T1, class T2, class... TYPES>
		Ref(const Ref<T1, T2, TYPES...>& other) noexcept;

		template <class T1, class T2, class... TYPES>
		Ref(Ref<T1, T2, TYPES...>&& other) noexcept;

		template <class... TYPES>
		Ref(const Pointer<TYPES...>& other) noexcept;

		~Ref() noexcept
		{
			SLIB_TRY_CONVERT_TYPE(T*, Referable*)
			if (ptr) {
				ptr->decreaseReference();
				ptr = sl_null;
			}
		}
	
	public:
		static const Ref<T>& null() noexcept
		{
			return *(reinterpret_cast<Ref<T> const*>(&(priv::ref::g_null)));
		}

		sl_bool isNull() const noexcept
		{
			return ptr == sl_null;
		}
	
		sl_bool isNotNull() const noexcept
		{
			return ptr != sl_null;
		}
	
		void setNull() noexcept
		{
			_replaceObject(sl_null);
		}

		T* get() const& noexcept
		{
			return ptr;
		}

		const Ref<Referable>& getReference() const noexcept
		{
			return *(reinterpret_cast<Ref<Referable> const*>(this));
		}

		template <class... TYPES>
		static const Ref<T>& from(const Ref<TYPES...>& other) noexcept
		{
			return *(reinterpret_cast<Ref<T> const*>(&other));
		}

		template <class... TYPES>
		static Ref<T>& from(Ref<TYPES...>& other) noexcept
		{
			return *(reinterpret_cast<Ref<T>*>(&other));
		}

		template <class... TYPES>
		static Ref<T>&& from(Ref<TYPES...>&& other) noexcept
		{
			return static_cast<Ref<T>&&>(*(reinterpret_cast<Ref<T>*>(&other)));
		}

	public:
		Ref<T>& operator=(sl_null_t) noexcept
		{
			_replaceObject(sl_null);
			return *this;
		}

		Ref<T>& operator=(T* other) noexcept
		{
			if (ptr != other) {
				if (other) {
					other->increaseReference();
				}
				_replaceObject(other);
			}
			return *this;
		}
	
		Ref<T>& operator=(Ref<T>&& other) noexcept
		{
			_move_assign(&other);
			return *this;
		}

		Ref<T>& operator=(const Ref<T>& other) noexcept
		{
			T* o = other.ptr;
			if (ptr != o) {
				if (o) {
					o->increaseReference();
				}
				_replaceObject(o);
			}
			return *this;
		}

		template <class OTHER>
		Ref<T>& operator=(Ref<OTHER>&& other) noexcept
		{
			SLIB_TRY_CONVERT_TYPE(OTHER*, T*)
			_move_assign(&other);
			return *this;
		}

		template <class OTHER>
		Ref<T>& operator=(const Ref<OTHER>& other) noexcept
		{
			SLIB_TRY_CONVERT_TYPE(OTHER*, T*)
			T* o = other.ptr;
			if (ptr != o) {
				if (o) {
					o->increaseReference();
				}
				_replaceObject(o);
			}
			return *this;
		}

		template <class OTHER>
		Ref<T>& operator=(const AtomicRef<OTHER>& other) noexcept
		{
			SLIB_TRY_CONVERT_TYPE(OTHER*, T*)
			if (ptr != other._ptr) {
				T* o = other._retainObject();
				_replaceObject(o);
			}
			return *this;
		}

		template <class OTHER>
		Ref<T>& operator=(const WeakRef<OTHER>& _other) noexcept
		{
			SLIB_TRY_CONVERT_TYPE(OTHER*, T*)
			Ref<OTHER> other(_other.lock());
			T* o = other.ptr;
			if (ptr != o) {
				if (o) {
					o->increaseReference();
				}
				_replaceObject(o);
			}
			return *this;
		}

		template <class OTHER>
		Ref<T>& operator=(const AtomicWeakRef<OTHER>& _other) noexcept
		{
			SLIB_TRY_CONVERT_TYPE(OTHER*, T*)
			Ref<OTHER> other(_other.lock());
			T* o = other.ptr;
			if (ptr != o) {
				if (o) {
					o->increaseReference();
				}
				_replaceObject(o);
			}
			return *this;
		}
	
		template <class T1, class T2, class... TYPES>
		Ref<T>& operator=(const Ref<T1, T2, TYPES...>& other) noexcept;

		template <class T1, class T2, class... TYPES>
		Ref<T>& operator=(Ref<T1, T2, TYPES...>&& other) noexcept;

		template <class... TYPES>
		Ref<T>& operator=(const Pointer<TYPES...>& other) noexcept;

	public:
		sl_bool operator==(sl_null_t) const noexcept
		{
			return ptr == sl_null;
		}

		sl_bool operator==(T* other) const noexcept
		{
			return ptr == other;
		}

		template <class OTHER>
		sl_bool operator==(const Ref<OTHER>& other) const noexcept
		{
			return (void*)ptr == (void*)(other.ptr);
		}

		template <class OTHER>
		sl_bool operator==(const AtomicRef<OTHER>& other) const noexcept
		{
			return (void*)ptr == (void*)(other._ptr);
		}

		sl_bool operator!=(sl_null_t) const noexcept
		{
			return ptr != sl_null;
		}
	
		sl_bool operator!=(T* other) const noexcept
		{
			return ptr != other;
		}

		template <class OTHER>
		sl_bool operator!=(const Ref<OTHER>& other) const noexcept
		{
			return (void*)ptr != (void*)(other.ptr);
		}

		template <class OTHER>
		sl_bool operator!=(const AtomicRef<OTHER>& other) const noexcept
		{
			return (void*)ptr != (void*)(other._ptr);
		}
	
	public:
		T& operator*() const noexcept
		{
			return *ptr;
		}

		T* operator->() const noexcept
		{
			return ptr;
		}

		operator T*() const noexcept
		{
			return ptr;
		}
	
		explicit operator sl_bool() const noexcept
		{
			return ptr != sl_null;
		}
	
	public:
		void _replaceObject(T* other) noexcept
		{
			if (ptr) {
				ptr->decreaseReference();
			}
			ptr = other;
		}

		void _move_init(void* _other) noexcept
		{
			if ((void*)this != _other) {
				Ref<T>& other = *(reinterpret_cast<Ref<T>*>(_other));
				ptr = other.ptr;
				other.ptr = sl_null;
			}
		}
	
		void _move_assign(void* _other) noexcept
		{
			if ((void*)this != _other) {
				Ref<T>& other = *(reinterpret_cast<Ref<T>*>(_other));
				_replaceObject(other.ptr);
				other.ptr = sl_null;
			}
		}

	public:
		T* ptr;

	};
	
	template <class T>
	class SLIB_EXPORT Atomic< Ref<T> >
	{
	public:
		constexpr Atomic() noexcept : _ptr(sl_null) {}

		constexpr Atomic(sl_null_t) noexcept : _ptr(sl_null) {}

		Atomic(T* other) noexcept
		{
			if (other) {
				other->increaseReference();
			}
			_ptr = other;
		}

		Atomic(const AtomicRef<T>& other) noexcept
		{
			T* o = other._retainObject();
			_ptr = o;
		}

		template <class OTHER>
		Atomic(const AtomicRef<OTHER>& other) noexcept
		{
			SLIB_TRY_CONVERT_TYPE(OTHER*, T*)
			T* o = other._retainObject();
			_ptr = o;
		}

		template <class OTHER>
		Atomic(Ref<OTHER>&& other) noexcept
		{
			SLIB_TRY_CONVERT_TYPE(OTHER*, T*)
			_move_init(&other);
		}
	
		template <class OTHER>
		Atomic(const Ref<OTHER>& other) noexcept
		{
			SLIB_TRY_CONVERT_TYPE(OTHER*, T*)
			T* o = other.ptr;
			if (o) {
				o->increaseReference();
			}
			_ptr = o;
		}

		template <class OTHER>
		Atomic(const WeakRef<OTHER>& _other) noexcept
		{
			SLIB_TRY_CONVERT_TYPE(OTHER*, T*)
			Ref<OTHER> other(_other.lock());
			T* o = other.ptr;
			if (o) {
				o->increaseReference();
			}
			_ptr = o;
		}

		template <class OTHER>
		Atomic(const AtomicWeakRef<OTHER>& _other) noexcept
		{
			SLIB_TRY_CONVERT_TYPE(OTHER*, T*)
			Ref<OTHER> other(_other.lock());
			T* o = other.ptr;
			if (o) {
				o->increaseReference();
			}
			_ptr = o;
		}
	
		template <class T1, class T2, class... TYPES>
		Atomic(const Ref<T1, T2, TYPES...>& other) noexcept;

		template <class T1, class T2, class... TYPES>
		Atomic(Ref<T1, T2, TYPES...>&& other) noexcept;

		template <class... TYPES>
		Atomic(const Pointer<TYPES...>& other) noexcept;

		~Atomic() noexcept
		{
			SLIB_TRY_CONVERT_TYPE(T*, Referable*)
			T* o = _ptr;
			if (o) {
				o->decreaseReference();
				_ptr = sl_null;
			}
		}
	
	public:
		static const AtomicRef<T>& null() noexcept
		{
			return *(reinterpret_cast<AtomicRef<T> const*>(&(priv::ref::g_null)));
		}
	
		sl_bool isNull() const noexcept
		{
			return _ptr == sl_null;
		}
	
		sl_bool isNotNull() const noexcept
		{
			return _ptr != sl_null;
		}
	
		void setNull() noexcept
		{
			_replaceObject(sl_null);
		}
	
		template <class OTHER>
		static const AtomicRef<T>& from(const AtomicRef<OTHER>& other) noexcept
		{
			return *(reinterpret_cast<AtomicRef<T> const*>(&other));
		}

		template <class OTHER>
		static AtomicRef<T>& from(AtomicRef<OTHER>& other) noexcept
		{
			return *(reinterpret_cast<AtomicRef<T>*>(&other));
		}

	public:
		AtomicRef<T>& operator=(sl_null_t) noexcept
		{
			_replaceObject(sl_null);
			return *this;
		}

		AtomicRef<T>& operator=(T* other) noexcept
		{
			if (_ptr != other) {
				if (other) {
					other->increaseReference();
				}
				_replaceObject(other);
			}
			return *this;
		}

		AtomicRef<T>& operator=(const AtomicRef<T>& other) noexcept
		{
			if (_ptr != other._ptr) {
				T* o = other._retainObject();
				_replaceObject(o);
			}
			return *this;
		}

		template <class OTHER>
		AtomicRef<T>& operator=(const AtomicRef<OTHER>& other) noexcept
		{
			SLIB_TRY_CONVERT_TYPE(OTHER*, T*)
			if (_ptr != other._ptr) {
				T* o = other._retainObject();
				_replaceObject(o);
			}
			return *this;
		}

		template <class OTHER>
		AtomicRef<T>& operator=(Ref<OTHER>&& other) noexcept
		{
			SLIB_TRY_CONVERT_TYPE(OTHER*, T*)
			_move_assign(&other);
			return *this;
		}
	
		template <class OTHER>
		AtomicRef<T>& operator=(const Ref<OTHER>& other) noexcept
		{
			SLIB_TRY_CONVERT_TYPE(OTHER*, T*)
			T* o = other.ptr;
			if (_ptr != o) {
				if (o) {
					o->increaseReference();
				}
				_replaceObject(o);
			}
			return *this;
		}

		template <class OTHER>
		AtomicRef<T>& operator=(const WeakRef<OTHER>& _other) noexcept
		{
			SLIB_TRY_CONVERT_TYPE(OTHER*, T*)
			Ref<OTHER> other(_other.lock());
			T* o = other.ptr;
			if (_ptr != o) {
				if (o) {
					o->increaseReference();
				}
				_replaceObject(o);
			}
			return *this;
		}

		template <class OTHER>
		AtomicRef<T>& operator=(const AtomicWeakRef<OTHER>& _other) noexcept
		{
			SLIB_TRY_CONVERT_TYPE(OTHER*, T*)
			Ref<OTHER> other(_other.lock());
			T* o = other.ptr;
			if (_ptr != o) {
				if (o) {
					o->increaseReference();
				}
				_replaceObject(o);
			}
			return *this;
		}
	
		template <class T1, class T2, class... TYPES>
		AtomicRef<T>& operator=(const Ref<T1, T2, TYPES...>& other) noexcept;

		template <class T1, class T2, class... TYPES>
		AtomicRef<T>& operator=(Ref<T1, T2, TYPES...>&& other) noexcept;

		template <class... TYPES>
		AtomicRef<T>& operator=(const Pointer<TYPES...>& other) noexcept;

	public:
		sl_bool operator==(sl_null_t) const noexcept
		{
			return _ptr == sl_null;
		}

		sl_bool operator==(T* other) const noexcept
		{
			return _ptr == other;
		}

		template <class OTHER>
		sl_bool operator==(const AtomicRef<OTHER>& other) const noexcept
		{
			return (void*)_ptr == (void*)(other._ptr);
		}

		template <class OTHER>
		sl_bool operator==(const Ref<OTHER>& other) const noexcept
		{
			return (void*)_ptr == (void*)(other.ptr);
		}

		sl_bool operator!=(sl_null_t) const noexcept
		{
			return _ptr != sl_null;
		}

		sl_bool operator!=(T* other) const noexcept
		{
			return _ptr != other;
		}

		template <class OTHER>
		sl_bool operator!=(const AtomicRef<OTHER>& other) const noexcept
		{
			return (void*)_ptr != (void*)(other._ptr);
		}

		template <class OTHER>
		sl_bool operator!=(const Ref<OTHER>& other) const noexcept
		{
			return (void*)_ptr != (void*)(other.ptr);
		}

	public:
		Ref<T> operator*() const noexcept
		{
			return *this;
		}
		
		explicit operator sl_bool() const noexcept
		{
			return _ptr != sl_null;
		}
	
	public:
		T* _retainObject() const noexcept
		{
			if (!_ptr) {
				return sl_null;
			}
			m_lock.lock();
			T* o = _ptr;
			if (o) {
				o->increaseReference();
			}
			m_lock.unlock();
			return o;
		}

		void _replaceObject(T* other) noexcept
		{
			m_lock.lock();
			T* before = _ptr;
			_ptr = other;
			m_lock.unlock();
			if (before) {
				before->decreaseReference();
			}
		}

		void _move_init(void* _other) noexcept
		{
			if ((void*)this != _other) {
				AtomicRef<T>& other = *(reinterpret_cast<AtomicRef<T>*>(_other));
				_ptr = other._ptr;
				other._ptr = sl_null;
			}
		}

		void _move_assign(void* _other) noexcept
		{
			if ((void*)this != _other) {
				AtomicRef<T>& other = *(reinterpret_cast<AtomicRef<T>*>(_other));
				_replaceObject(other._ptr);
				other._ptr = sl_null;
			}
		}

	public:
		T* _ptr;
	private:
		SpinLock m_lock;
	
	};

	
	template <class T>
	class SLIB_EXPORT WeakRef
	{
	public:
		WeakRef() noexcept = default;

		WeakRef(sl_null_t) noexcept {}

		WeakRef(T* _other) noexcept
		{
			_set(_other);
		}
	
		WeakRef(WeakRef<T>&& other) noexcept
		{
			_weak._move_init(&other);
		}

		WeakRef(const WeakRef<T>& other) noexcept : _weak(other._weak)
		{
		}

		template <class OTHER>
		WeakRef(WeakRef<OTHER>&& other) noexcept
		{
			SLIB_TRY_CONVERT_TYPE(OTHER*, T*)
			_weak._move_init(&other);
		}
	
		template <class OTHER>
		WeakRef(const WeakRef<OTHER>& other) noexcept : _weak(other._weak)
		{
			SLIB_TRY_CONVERT_TYPE(OTHER*, T*)
		}

		template <class OTHER>
		WeakRef(const AtomicWeakRef<OTHER>& other) noexcept : _weak(other._weak)
		{
			SLIB_TRY_CONVERT_TYPE(OTHER*, T*)
		}

		template <class OTHER>
		WeakRef(const Ref<OTHER>& other) noexcept
		{
			SLIB_TRY_CONVERT_TYPE(OTHER*, T*)
			_set(other.ptr);
		}
	
		template <class OTHER>
		WeakRef(const AtomicRef<OTHER>& _other) noexcept
		{
			SLIB_TRY_CONVERT_TYPE(OTHER*, T*)
			Ref<OTHER> other(_other);
			_set(other.ptr);
		}

		template <class T1, class T2, class... TYPES>
		WeakRef(const Ref<T1, T2, TYPES...>& other) noexcept;

		template <class... TYPES>
		WeakRef(const Pointer<TYPES...>& other) noexcept;

		~WeakRef() noexcept
		{
			SLIB_TRY_CONVERT_TYPE(T*, Referable*)
		}

	public:
		static const WeakRef<T>& null() noexcept
		{
			return *(reinterpret_cast<WeakRef<T> const*>(&(priv::ref::g_null)));
		}

		sl_bool isNull() const noexcept
		{
			return _weak.isNull();
		}

		sl_bool isNotNull() const noexcept
		{
			return _weak.isNotNull();
		}

		void setNull() noexcept
		{
			_weak.setNull();
		}

		template <class OTHER>
		static const WeakRef<T>& from(const WeakRef<OTHER>& other) noexcept
		{
			return *(reinterpret_cast<WeakRef<T> const*>(&other));
		}

		template <class OTHER>
		static WeakRef<T>& from(WeakRef<OTHER>& other) noexcept
		{
			return *(reinterpret_cast<WeakRef<T>*>(&other));
		}

		template <class OTHER>
		static WeakRef<T>&& from(WeakRef<OTHER>&& other) noexcept
		{
			return static_cast<WeakRef<T>&&>(*(reinterpret_cast<WeakRef<T>*>(&other)));
		}

		Ref<T> lock() const noexcept
		{
			if (_weak.isNotNull()) {
				return Ref<T>::from(_weak->lock());
			}
			return sl_null;
		}

		static WeakRef<T> fromReferable(Referable* referable) noexcept
		{
			if (referable) {
				WeakRef<T> ret;
				if (referable->_isWeakRef()) {
					ret._weak = static_cast<CWeakRef*>(referable);
				} else {
					ret._weak = referable->_getWeakObject();
				}
				return ret;
			} else {
				return sl_null;
			}
		}

	public:
		WeakRef<T>& operator=(sl_null_t) noexcept
		{
			_weak.setNull();
			return *this;
		}

		WeakRef<T>& operator=(T* _other) noexcept
		{
			_set(_other);
			return *this;
		}
	
		WeakRef<T>& operator=(WeakRef<T>&& other) noexcept
		{
			_weak._move_assign(&other);
			return *this;
		}

		WeakRef<T>& operator=(const WeakRef<T>& other) noexcept
		{
			_weak = other._weak;
			return *this;
		}

		template <class OTHER>
		WeakRef<T>& operator=(WeakRef<OTHER>&& other) noexcept
		{
			SLIB_TRY_CONVERT_TYPE(OTHER*, T*)
			_weak._move_assign(&other);
			return *this;
		}
	
		template <class OTHER>
		WeakRef<T>& operator=(const WeakRef<OTHER>& other) noexcept
		{
			SLIB_TRY_CONVERT_TYPE(OTHER*, T*)
			_weak = other._weak;
			return *this;
		}

		template <class OTHER>
		WeakRef<T>& operator=(const AtomicWeakRef<OTHER>& other) noexcept
		{
			SLIB_TRY_CONVERT_TYPE(OTHER*, T*)
			_weak = other._weak;
			return *this;
		}
	
		template <class OTHER>
		WeakRef<T>& operator=(const Ref<OTHER>& other) noexcept
		{
			SLIB_TRY_CONVERT_TYPE(OTHER*, T*)
			_set(other.ptr);
			return *this;
		}
	
		template <class OTHER>
		WeakRef<T>& operator=(const AtomicRef<OTHER>& _other) noexcept
		{
			SLIB_TRY_CONVERT_TYPE(OTHER*, T*)
			Ref<OTHER> other(_other);
			_set(other.ptr);
			return *this;
		}

		template <class T1, class T2, class... TYPES>
		WeakRef<T>& operator=(const Ref<T1, T2, TYPES...>& other) noexcept;

		template <class... TYPES>
		WeakRef<T>& operator=(const Pointer<TYPES...>& other) noexcept;

	public:
		template <class OTHER>
		sl_bool operator==(const WeakRef<OTHER>& other) const noexcept
		{
			return _weak == other._weak;
		}
	
		template <class OTHER>
		sl_bool operator==(const AtomicWeakRef<OTHER>& other) const noexcept
		{
			return _weak == other._weak;
		}

		template <class OTHER>
		sl_bool operator!=(const WeakRef<OTHER>& other) const noexcept
		{
			return _weak != other._weak;
		}

		template <class OTHER>
		sl_bool operator!=(const AtomicWeakRef<OTHER>& other) const noexcept
		{
			return _weak != other._weak;
		}
		
	public:
		explicit operator sl_bool() const noexcept
		{
			return _weak.isNotNull();
		}
	
	private:
		void _set(T* object) noexcept
		{
			if (object) {
				_weak = object->_getWeakObject();
			} else {
				_weak.setNull();
			}
		}

	public:
		Ref<CWeakRef> _weak;

	};
	
	template <class T>
	class SLIB_EXPORT Atomic< WeakRef<T> >
	{
	public:
		Atomic() noexcept = default;

		Atomic(sl_null_t) noexcept {}

		Atomic(T* _other) noexcept
		{
			_set(_other);
		}

		Atomic(const AtomicWeakRef<T>& other) noexcept : _weak(other._weak)
		{
		}

		template <class OTHER>
		Atomic(const AtomicWeakRef<OTHER>& other) noexcept : _weak(other._weak)
		{
			SLIB_TRY_CONVERT_TYPE(OTHER*, T*)
		}

		template <class OTHER>
		Atomic(WeakRef<OTHER>&& other) noexcept
		{
			SLIB_TRY_CONVERT_TYPE(OTHER*, T*)
			_weak._move_init(&other);
		}

		template <class OTHER>
		Atomic(const WeakRef<OTHER>& other) noexcept : _weak(other._weak)
		{
			SLIB_TRY_CONVERT_TYPE(OTHER*, T*)
		}

		template <class OTHER>
		Atomic(const Ref<OTHER>& other) noexcept
		{
			SLIB_TRY_CONVERT_TYPE(OTHER*, T*)
			_set(other.ptr);
		}

		template <class OTHER>
		Atomic(const AtomicRef<OTHER>& _other) noexcept
		{
			SLIB_TRY_CONVERT_TYPE(OTHER*, T*)
			Ref<OTHER> other(_other);
			_set(other.ptr);
		}

		template <class T1, class T2, class... TYPES>
		Atomic(const Ref<T1, T2, TYPES...>& other) noexcept;

		template <class... TYPES>
		Atomic(const Pointer<TYPES...>& other) noexcept;

		~Atomic() noexcept
		{
			SLIB_TRY_CONVERT_TYPE(T*, Referable*)
		}

	public:
		static const AtomicWeakRef<T>& null() noexcept
		{
			return *(reinterpret_cast<AtomicWeakRef<T> const*>(&(priv::ref::g_null)));
		}

		sl_bool isNull() const noexcept
		{
			return _weak.isNull();
		}

		sl_bool isNotNull() const noexcept
		{
			return _weak.isNotNull();
		}

		void setNull() noexcept
		{
			_weak.setNull();
		}

		template <class OTHER>
		static const AtomicWeakRef<T>& from(const AtomicWeakRef<OTHER>& other) noexcept
		{
			return *(reinterpret_cast<AtomicWeakRef<T> const*>(&other));
		}

		template <class OTHER>
		static AtomicWeakRef<T>& from(AtomicWeakRef<OTHER>& other) noexcept
		{
			return *(reinterpret_cast<AtomicWeakRef<T>*>(&other));
		}

		Ref<T> lock() const noexcept
		{
			Ref<CWeakRef> weak(_weak);
			if (weak.isNotNull()) {
				return Ref<T>::from(weak->lock());
			}
			return sl_null;
		}
	
	public:
		AtomicWeakRef<T>& operator=(sl_null_t) noexcept
		{
			_weak.setNull();
			return *this;
		}

		AtomicWeakRef<T>& operator=(T* _other) noexcept
		{
			_set(_other);
			return *this;
		}
	
		AtomicWeakRef<T>& operator=(const AtomicWeakRef<T>& other) noexcept
		{
			_weak = other._weak;
			return *this;
		}

		template <class OTHER>
		AtomicWeakRef<T>& operator=(const AtomicWeakRef<OTHER>& other) noexcept
		{
			SLIB_TRY_CONVERT_TYPE(OTHER*, T*)
			_weak = other._weak;
			return *this;
		}

		template <class OTHER>
		AtomicWeakRef<T>& operator=(WeakRef<OTHER>&& other) noexcept
		{
			SLIB_TRY_CONVERT_TYPE(OTHER*, T*)
			_weak._move_assign(&other);
			return *this;
		}

		template <class OTHER>
		AtomicWeakRef<T>& operator=(const WeakRef<OTHER>& other) noexcept
		{
			SLIB_TRY_CONVERT_TYPE(OTHER*, T*)
			_weak = other._weak;
			return *this;
		}

		template <class OTHER>
		AtomicWeakRef<T>& operator=(const Ref<OTHER>& other) noexcept
		{
			SLIB_TRY_CONVERT_TYPE(OTHER*, T*)
			_set(other.ptr);
			return *this;
		}

		template <class OTHER>
		AtomicWeakRef<T>& operator=(const AtomicRef<OTHER>& _other) noexcept
		{
			SLIB_TRY_CONVERT_TYPE(OTHER*, T*)
			Ref<OTHER> other(_other);
			_set(other.ptr);
			return *this;
		}

		template <class T1, class T2, class... TYPES>
		AtomicWeakRef<T>& operator=(const Ref<T1, T2, TYPES...>& other) noexcept;

		template <class... TYPES>
		AtomicWeakRef<T>& operator=(const Pointer<TYPES...>& other) noexcept;

	public:
		template <class OTHER>
		sl_bool operator==(const AtomicWeakRef<OTHER>& other) const noexcept
		{
			return _weak == other._weak;
		}
		

		template <class OTHER>
		sl_bool operator==(const WeakRef<OTHER>& other) const noexcept
		{
			return _weak == other._weak;
		}

		template <class OTHER>
		sl_bool operator!=(const AtomicWeakRef<OTHER>& other) const noexcept
		{
			return _weak != other._weak;
		}

		template <class OTHER>
		sl_bool operator!=(const WeakRef<OTHER>& other) const noexcept
		{
			return _weak != other._weak;
		}

	public:
		WeakRef<T> operator*() const noexcept
		{
			return *this;
		}
		
		explicit operator sl_bool() const noexcept
		{
			return _weak.isNotNull();
		}
	
	private:
		void _set(T* object) noexcept
		{
			if (object) {
				_weak = object->_getWeakObject();
			} else {
				_weak.setNull();
			}
		}

	public:
		AtomicRef<CWeakRef> _weak;
	
	};
	
	template <class T>
	struct PropertyTypeHelper< WeakRef<T> >
	{
		typedef Ref<T> const& ArgType;
		typedef Ref<T> RetType;
	};

	
	class SLIB_EXPORT CWeakRef : public Referable
	{
		SLIB_DECLARE_OBJECT
		
	public:
		CWeakRef() noexcept;

		~CWeakRef() noexcept;

	public:
		Referable* m_object;
		SpinLock m_lock;

	public:
		static CWeakRef* create(Referable* object) noexcept;

	public:
		Ref<Referable> lock() noexcept;

		void release() noexcept;
		
	};

	
	template <class T>
	SLIB_INLINE sl_bool operator==(sl_null_t, const Ref<T>& b) noexcept
	{
		return sl_null == b.ptr;
	}
	
	template <class T>
	SLIB_INLINE sl_bool operator==(T* a, const Ref<T>& b) noexcept
	{
		return a == b.ptr;
	}

	template <class T>
	SLIB_INLINE sl_bool operator!=(sl_null_t, const Ref<T>& b) noexcept
	{
		return sl_null != b.ptr;
	}
	
	template <class T>
	SLIB_INLINE sl_bool operator!=(T* a, const Ref<T>& b) noexcept
	{
		return a != b.ptr;
	}

	template <class T>
	SLIB_INLINE sl_bool operator==(sl_null_t, const AtomicRef<T>& b) noexcept
	{
		return sl_null == b._ptr;
	}
	
	template <class T>
	SLIB_INLINE sl_bool operator==(T* a, const AtomicRef<T>& b) noexcept
	{
		return a == b._ptr;
	}

	template <class T>
	SLIB_INLINE sl_bool operator!=(sl_null_t, const AtomicRef<T>& b) noexcept
	{
		return sl_null != b._ptr;
	}
	
	template <class T>
	SLIB_INLINE sl_bool operator!=(T* a, const AtomicRef<T>& b) noexcept
	{
		return a != b._ptr;
	}

	template <class T>
	SLIB_INLINE sl_bool operator>(const Ref<T>& a, const Ref<T>& b) noexcept
	{
		return a.ptr > b.ptr;
	}

	template <class T>
	SLIB_INLINE sl_bool operator<(const Ref<T>& a, const Ref<T>& b) noexcept
	{
		return a.ptr < b.ptr;
	}


	template <class T>
	Ref<T> New() noexcept
	{
		return new T;
	}

	template <class T, class ARG, class... ARGS>
	Ref<T> New(ARG&& arg, ARGS&&... args) noexcept
	{
		Ref<T> o = new T;
		if (o.isNotNull()) {
			o->init(Forward<ARG>(arg), Forward<ARGS>(args)...);
			return o;
		}
		return sl_null;
	}

	template <class T, class OTHER>
	sl_bool IsInstanceOf(const OTHER* object) noexcept
	{
		SLIB_TRY_CONVERT_TYPE(OTHER*, Referable*)
		if (object) {
			return object->isInstanceOf(T::ObjectType());
		}
		return sl_false;
	}

	template <class T, class OTHER>
	sl_bool IsInstanceOf(const Ref<OTHER>& object) noexcept
	{
		SLIB_TRY_CONVERT_TYPE(OTHER*, Referable*)
		if (object.isNotNull()) {
			return object->isInstanceOf(T::ObjectType());
		}
		return sl_false;
	}
	
	template <class T, class OTHER>
	T* CastInstance(OTHER* object) noexcept
	{
		SLIB_TRY_CONVERT_TYPE(OTHER*, Referable*)
		if (object) {
			if (object->isInstanceOf(T::ObjectType())) {
				return static_cast<T*>(object);
			}
		}
		return sl_null;
	}

	template <class T, class OTHER>
	const Ref<T>& CastRef(const Ref<OTHER>& object) noexcept
	{
		SLIB_TRY_CONVERT_TYPE(OTHER*, Referable*)
		if (object.isNotNull()) {
			if (object->isInstanceOf(T::ObjectType())) {
				return *(reinterpret_cast<Ref<T> const*>(&object));
			}
		}
		return Ref<T>::null();
	}

	template <class T, class OTHER>
	const Ref<T>& CastRef(const Ref<OTHER>& object, const Ref<T>& def) noexcept
	{
		SLIB_TRY_CONVERT_TYPE(OTHER*, Referable*)
		if (object.isNotNull()) {
			if (object->isInstanceOf(T::ObjectType())) {
				return *(reinterpret_cast<Ref<T> const*>(&object));
			}
		}
		return def;
	}

	template <class T>
	Ref<T> ToRef(T* other) noexcept
	{
		return Ref<T>(other);
	}

	template <class T>
	const Ref<T>& ToRef(const Ref<T>& other) noexcept
	{
		return other;
	}

	template <class T>
	Ref<T> ToRef(const AtomicRef<T>& other) noexcept
	{
		return Ref<T>(other);
	}

	template <class T>
	Ref<T> ToRef(const WeakRef<T>& other) noexcept
	{
		return Ref<T>(other);
	}

	template <class T>
	Ref<T> ToRef(const AtomicWeakRef<T>& other) noexcept
	{
		return Ref<T>(other);
	}

	template <class T>
	WeakRef<T> ToWeakRef(T* other) noexcept
	{
		return WeakRef<T>(other);
	}

	template <class T>
	WeakRef<T> ToWeakRef(const Ref<T>& other) noexcept
	{
		return WeakRef<T>(other);
	}

	template <class T>
	WeakRef<T> ToWeakRef(const AtomicRef<T>& other) noexcept
	{
		return WeakRef<T>(other);
	}

	template <class T>
	const WeakRef<T>& ToWeakRef(const WeakRef<T>& other) noexcept
	{
		return other;
	}

	template <class T>
	WeakRef<T> ToWeakRef(const AtomicWeakRef<T>& other) noexcept
	{
		return WeakRef<T>(other);
	}

}

#endif
