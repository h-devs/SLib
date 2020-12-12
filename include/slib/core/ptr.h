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

#ifndef CHECKHEADER_SLIB_CORE_PTR
#define CHECKHEADER_SLIB_CORE_PTR

#include "definition.h"

#include "ref.h"

namespace slib
{
	namespace priv
	{
		namespace ptr
		{
			struct ConstStruct;
			extern const ConstStruct g_null;
		}
	}

	template <class... TYPES>
	class Ptr;
	
	template <class T>
	using AtomicPtr = Atomic< Ptr<T> >;
		
	template <class T>
	class SLIB_EXPORT Ptr<T>
	{
	public:
		T* ptr;
		Ref<Referable> ref;
	
	public:
		Ptr() noexcept : ptr(sl_null)
	 	{}

		Ptr(Ptr<T>&& other) noexcept
		{
			ptr = other.ptr;
			_move_init(&other);
		}
	
		Ptr(const Ptr<T>& other) noexcept : ptr(other.ptr), ref(other.ref)
		{}

		template <class O>
		Ptr(Ptr<O>&& other) noexcept
		{
			SLIB_TRY_CONVERT_TYPE(O*, T*)
			ptr = other.ptr;
			_move_init(&other);
		}
	
		template <class O>
		Ptr(const Ptr<O>& other) noexcept : ptr(other.ptr), ref(other.ref)
		{
			SLIB_TRY_CONVERT_TYPE(O*, T*)
		}

		template <class O>
		Ptr(const AtomicPtr<O>& other) noexcept
		{
			SLIB_TRY_CONVERT_TYPE(O*, T*)
			ptr = other._retain(ref);
		}

		Ptr(sl_null_t) noexcept	: ptr(sl_null)
		{}

		Ptr(T* pointer) noexcept : ptr(pointer)
		{}

		template <class O>
		Ptr(const Ref<O>& reference) noexcept
		: ptr(reference.ptr), ref(reference)
		{
			SLIB_TRY_CONVERT_TYPE(O*, T*)
		}

		template <class O>
		Ptr(const AtomicRef<O>& reference) noexcept
		: ref(reference)
		{
			SLIB_TRY_CONVERT_TYPE(O*, T*)
			ptr = ref.ptr;
		}

		template <class O>
		Ptr(const WeakRef<O>& weak) noexcept
		{
			SLIB_TRY_CONVERT_TYPE(O*, T*)
			Ref<O> o(weak);
			if (o.isNotNull()) {
				ptr = o.ptr;
				ref = weak._weak;
			} else {
				ptr = sl_null;
			}
		}

		template <class O>
		Ptr(const AtomicWeakRef<O>& _weak) noexcept
		{
			SLIB_TRY_CONVERT_TYPE(O*, T*)
			WeakRef<O> weak(_weak);
			Ref<O> o(weak);
			if (o.isNotNull()) {
				ptr = o.ptr;
				ref = weak._weak;
			} else {
				ptr = sl_null;
			}
		}

		template <class T1, class T2, class... TYPES>
		Ptr(const Ptr<T1, T2, TYPES...>& other) noexcept;

		template <class T1, class T2, class... TYPES>
		Ptr(Ptr<T1, T2, TYPES...>&& other) noexcept;

		template <class T1, class T2, class... TYPES>
		Ptr(const Ref<T1, T2, TYPES...>& other) noexcept;

		template <class... TYPES>
		Ptr(const Pointer<TYPES...>& other) noexcept;

		template <class REF>
		Ptr(T* pointer, REF&& r) noexcept: ptr(pointer), ref(_getRef(Forward<REF>(r)))
		{
		}

		~Ptr()
		{
			ptr = sl_null;
		}

	public:
		static const Ptr<T>& null() noexcept
		{
			return *(reinterpret_cast<Ptr<T> const*>(&(priv::ptr::g_null)));
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
			ptr = sl_null;
			ref.setNull();
		}

		template <class... TYPES>
		static const Ptr<T>& from(const Ptr<TYPES...>& other) noexcept
		{
			return *(reinterpret_cast<Ptr<T> const*>(&other));
		}

		template <class... TYPES>
		static Ptr<T>& from(Ptr<TYPES...>& other) noexcept
		{
			return *(reinterpret_cast<Ptr<T>*>(&other));
		}

		template <class... TYPES>
		static Ptr<T>&& from(Ptr<TYPES...>&& other) noexcept
		{
			return static_cast<Ptr<T>&&>(*(reinterpret_cast<Ptr<T>*>(&other)));
		}
	
		sl_bool isWeak() const noexcept
		{
			Referable* obj = ref.ptr;
			return obj && obj->_isWeakRef();
		}

		sl_bool lockRef(Ref<Referable>& outRef) const noexcept
		{
			Referable* obj = ref.ptr;
			if (obj) {
				if (obj->_isWeakRef()) {
					CWeakRef* weak = static_cast<CWeakRef*>(obj);
					Ref<Referable> r(weak->lock());
					if (r.isNotNull()) {
						outRef = Move(r);
						return sl_true;
					} else {
						return sl_false;
					}
				} else {
					outRef = obj;
				}
			}
			return sl_true;
		}
		
		Ptr<T> lock() const noexcept
		{
			Referable* obj = ref.ptr;
			if (obj && obj->_isWeakRef()) {
				CWeakRef* weak = static_cast<CWeakRef*>(obj);
				Ref<Referable> r(weak->lock());
				if (r.isNotNull()) {
					return Ptr<T>(ptr, r);
				} else {
					return sl_null;
				}
			} else {
				return *this;
			}
		}
		
		Ptr<T> toWeak() const noexcept
		{
			Referable* obj = ref.ptr;
			if (obj && !(obj->_isWeakRef())) {
				return Ptr<T>(ptr, WeakRef<Referable>(obj));
			} else {
				return *this;
			}
		}

	public:
		T* get() const noexcept
		{
			return ptr;
		}
		
		void set(T* pointer) noexcept
		{
			ptr = pointer;
			ref.setNull();
		}

		void set(sl_null_t) noexcept
		{
			ptr = sl_null;
			ref.setNull();
		}

		template <class O>
		void set(Ptr<O>&& other) noexcept
		{
			SLIB_TRY_CONVERT_TYPE(O*, T*)
			ptr = other.ptr;
			_move_assign(&other);
		}
	
		template <class O>
		void set(const Ptr<O>& other) noexcept
		{
			SLIB_TRY_CONVERT_TYPE(O*, T*)
			ptr = other.ptr;
			ref = other.ref;
		}

		template <class O>
		void set(const AtomicPtr<O>& other) noexcept
		{
			SLIB_TRY_CONVERT_TYPE(O*, T*)
			ptr = other._retain(ref);
		}

		template <class O>
		void set(const Ref<O>& reference) noexcept
		{
			SLIB_TRY_CONVERT_TYPE(O*, T*)
			ptr = reference.ptr;
			ref = reference;
		}

		template <class O>
		void set(const AtomicRef<O>& reference) noexcept
		{
			SLIB_TRY_CONVERT_TYPE(O*, T*)
			ref = reference;
			ptr = ref.ptr;
		}

		template <class O>
		void set(const WeakRef<O>& weak) noexcept
		{
			SLIB_TRY_CONVERT_TYPE(O*, T*)
			Ref<O> o(weak);
			if (o.isNotNull()) {
				ptr = o.ptr;
				ref = weak._weak;
			} else {
				ptr = sl_null;
				ref.setNull();
			}
		}

		template <class O>
		void set(const AtomicWeakRef<O>& _weak) noexcept
		{
			SLIB_TRY_CONVERT_TYPE(O*, T*)
			WeakRef<O> weak(_weak);
			Ref<O> o(weak);
			if (o.isNotNull()) {
				ptr = o.ptr;
				ref = weak._weak;
			} else {
				ptr = sl_null;
				ref.setNull();
			}
		}

		template <class T1, class T2, class... TYPES>
		void set(const Ptr<T1, T2, TYPES...>& other) noexcept;

		template <class T1, class T2, class... TYPES>
		void set(Ptr<T1, T2, TYPES...>&& other) noexcept;

		template <class T1, class T2, class... TYPES>
		void set(const Ref<T1, T2, TYPES...>& other) noexcept;

		template <class... TYPES>
		void set(const Pointer<TYPES...>& other) noexcept;

		template <class REF>
		void set(T* pointer, REF&& r) noexcept
		{
			ref = _getRef(Forward<REF>(r));
			if (ref.isNotNull()) {
				ptr = pointer;
			} else {
				ptr = sl_null;
			}
		}

	public:
		Ptr<T>& operator=(Ptr<T>&& other) noexcept
		{
			ptr = other.ptr;
			_move_assign(&other);
			return *this;
		}

		Ptr<T>& operator=(const Ptr<T>& other) noexcept
		{
			ptr = other.ptr;
			ref = other.ref;
			return *this;
		}

		template <class OTHER>
		Ptr<T>& operator=(OTHER&& other) noexcept
		{
			set(Forward<OTHER>(other));
			return *this;
		}


		sl_bool operator==(sl_null_t) const noexcept
		{
			return ptr == sl_null;
		}

		sl_bool operator==(T* other) const noexcept
		{
			return ptr == other;
		}

		template <class O>
		sl_bool operator==(const Ptr<O>& other) const noexcept
		{
			return ptr == other.ptr;
		}
	
		template <class O>
		sl_bool operator==(const AtomicPtr<O>& other) const noexcept
		{
			return ptr == other._ptr;
		}

		sl_bool operator!=(sl_null_t) const noexcept
		{
			return ptr != sl_null;
		}
	
		sl_bool operator!=(T* other) const noexcept
		{
			return ptr != other;
		}

		template <class O>
		sl_bool operator!=(const Ptr<O>& other) const noexcept
		{
			return ptr != other.ptr;
		}

		template <class O>
		sl_bool operator!=(const AtomicPtr<O>& other) const noexcept
		{
			return ptr != other._ptr;
		}

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
		void _move_init(void* _other) noexcept
		{
			Ptr<T>& other = *(reinterpret_cast<Ptr<T>*>(_other));
			ref._move_init(&(other.ref));
		}

		void _move_assign(void* _other) noexcept
		{
			if ((void*)this != _other) {
				Ptr<T>& other = *(reinterpret_cast<Ptr<T>*>(_other));
				ref._move_assign(&(other.ref));
			}
		}

		template <class... TYPES>
		static const Ref<Referable>& _getRef(const Ref<TYPES...>& ref) noexcept
		{
			return Ref<Referable>::from(ref);
		}

		template <class... TYPES>
		static Ref<Referable>&& _getRef(Ref<TYPES...>&& ref) noexcept
		{
			return Move(Ref<Referable>::from(ref));
		}

		template <class O>
		static const AtomicRef<Referable>& _getRef(const AtomicRef<O>& ref) noexcept
		{
			return AtomicRef<Referable>::from(ref);
		}

		template <class O>
		static const Ref<Referable>& _getRef(const WeakRef<O>& weak) noexcept
		{
			return Ref<Referable>::from(weak._weak);
		}

		template <class O>
		static Ref<Referable>&& _getRef(WeakRef<O>&& weak) noexcept
		{
			return Move(Ref<Referable>::from(weak._weak));
		}

		template <class O>
		static const AtomicRef<Referable>& _getRef(const AtomicWeakRef<O>& weak) noexcept
		{
			return AtomicRef<Referable>::from(weak._weak);
		}

	};
	
	
	template <class T>
	class SLIB_EXPORT Atomic< Ptr<T> >
	{
	public:
		T* _ptr;
		Ref<Referable> _ref;
	
	public:
		Atomic() noexcept
		: _ptr(sl_null)
		{}

		Atomic(const AtomicPtr<T>& other) noexcept
		{
			_ptr = other._retain(_ref);
		}

		template <class O>
		Atomic(const AtomicPtr<O>& other) noexcept
		{
			SLIB_TRY_CONVERT_TYPE(O*, T*)
			_ptr = other._retain(_ref);
		}

		template <class O>
		Atomic(Ptr<O>&& other) noexcept
		{
			SLIB_TRY_CONVERT_TYPE(O*, T*)
			_ptr = other.ptr;
			_move_init(&other);
		}
	
		template <class O>
		Atomic(const Ptr<O>& other) noexcept
		: _ptr(other.ptr), _ref(other.ref)
		{
			SLIB_TRY_CONVERT_TYPE(O*, T*)
		}

		Atomic(sl_null_t) noexcept	: _ptr(sl_null)
		{}

		Atomic(T* pointer) noexcept	: _ptr(pointer)
		{}

		template <class O>
		Atomic(const Ref<O>& reference) noexcept : _ptr(reference.ptr), _ref(reference)
		{
			SLIB_TRY_CONVERT_TYPE(O*, T*)
		}

		template <class O>
		Atomic(const AtomicRef<O>& reference) noexcept : _ref(reference)
		{
			SLIB_TRY_CONVERT_TYPE(O*, T*)
			_ptr = _ref.ptr;
		}

		template <class O>
		Atomic(const WeakRef<O>& weak) noexcept
		{
			SLIB_TRY_CONVERT_TYPE(O*, T*)
			Ref<O> o(weak);
			if (o.isNotNull()) {
				_ptr = o.ptr;
				_ref = weak._weak;
			} else {
				_ptr = sl_null;
			}
		}

		template <class O>
		Atomic(const AtomicWeakRef<O>& _weak) noexcept
		{
			SLIB_TRY_CONVERT_TYPE(O*, T*)
			WeakRef<O> weak(_weak);
			Ref<O> o(weak);
			if (o.isNotNull()) {
				_ptr = o.ptr;
				_ref = weak._weak;
			} else {
				_ptr = sl_null;
			}
		}

		template <class T1, class T2, class... TYPES>
		Atomic(const Ptr<T1, T2, TYPES...>& other) noexcept;

		template <class T1, class T2, class... TYPES>
		Atomic(Ptr<T1, T2, TYPES...>&& other) noexcept;

		template <class T1, class T2, class... TYPES>
		Atomic(const Ref<T1, T2, TYPES...>& other) noexcept;

		template <class... TYPES>
		Atomic(const Pointer<TYPES...>& other) noexcept;

		template <class REF>
		Atomic(T* pointer, REF&& r) noexcept
		{
		}

		~Atomic()
		{
			_ptr = sl_null;
		}

	public:
		static const AtomicPtr<T>& null() noexcept
		{
			return *(reinterpret_cast<AtomicPtr<T> const*>(&(priv::ptr::g_null)));
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
			_replace(sl_null, Ref<Referable>::null());
		}

		template <class OTHER>
		static const AtomicPtr<T>& from(const AtomicPtr<OTHER>& other) noexcept
		{
			return *(reinterpret_cast<AtomicPtr<OTHER> const*>(&other));
		}

		template <class OTHER>
		static AtomicPtr<T>& from(AtomicPtr<OTHER>& other) noexcept
		{
			return *(reinterpret_cast<AtomicPtr<OTHER>*>(&other));
		}

		Ptr<T> lock() const noexcept
		{
			Ptr<T> p(*this);
			return p.lock();
		}

	public:
		void set(T* pointer) noexcept
		{
			_replace(pointer, Ref<Referable>::null());
		}

		void set(sl_null_t) noexcept
		{
			_replace(sl_null, Ref<Referable>::null());
		}

		template <class O>
		void set(const AtomicPtr<O>& other) noexcept
		{
			SLIB_TRY_CONVERT_TYPE(O*, T*)
			Ref<Referable> reference;
			T* pointer = other._retain(reference);
			_replace(pointer, reference);
		}

		template <class O>
		void set(Ptr<O>&& other) noexcept
		{
			SLIB_TRY_CONVERT_TYPE(O*, T*)
			_ptr = other.ptr;
			_move_assign(&other);
		}
	
		template <class O>
		void set(const Ptr<O>& other) noexcept
		{
			SLIB_TRY_CONVERT_TYPE(O*, T*)
			_replace(other.ptr, other.ref);
		}

		template <class O>
		void set(const Ref<O>& reference) noexcept
		{
			SLIB_TRY_CONVERT_TYPE(O*, T*)
			_replace(reference.ptr, Ref<Referable>::from(reference));
		}

		template <class O>
		void set(const AtomicRef<O>& _reference) noexcept
		{
			SLIB_TRY_CONVERT_TYPE(O*, T*)
			Ref<O> reference(_reference);
			_replace(reference.ptr, Ref<Referable>::from(reference));
		}

		template <class O>
		void set(const WeakRef<O>& weak) noexcept
		{
			SLIB_TRY_CONVERT_TYPE(O*, T*)
			Ref<O> o(weak);
			if (o.isNotNull()) {
				_replace(o.ptr, Ref<Referable>::from(weak._weak));
			} else {
				_replace(sl_null, Ref<Referable>::null());
			}
		}

		template <class O>
		void set(const AtomicWeakRef<O>& _weak) noexcept
		{
			SLIB_TRY_CONVERT_TYPE(O*, T*)
			WeakRef<O> weak(_weak);
			Ref<O> o(weak);
			if (o.isNotNull()) {
				_replace(o.ptr, Ref<Referable>::from(weak._weak));
			} else {
				_replace(sl_null, Ref<Referable>::null());
			}
		}

		template <class T1, class T2, class... TYPES>
		void set(const Ptr<T1, T2, TYPES...>& other) noexcept;

		template <class T1, class T2, class... TYPES>
		void set(Ptr<T1, T2, TYPES...>&& other) noexcept;

		template <class T1, class T2, class... TYPES>
		void set(const Ref<T1, T2, TYPES...>& other) noexcept;

		template <class... TYPES>
		void set(const Pointer<TYPES...>& other) noexcept;

		template <class REF>
		void set(T* pointer, REF&& r) noexcept
		{
			_replace(pointer, Ptr<T>::_getRef(Forward<REF>(r)));
		}

	public:
		AtomicPtr<T>& operator=(const AtomicPtr<T>& other) noexcept
		{
			Ref<Referable> reference;
			T* pointer = other._retain(reference);
			_replace(pointer, reference);
			return *this;
		}

		template <class REF>
		AtomicPtr<T>& operator=(REF&& other) noexcept
		{
			set(Forward<REF>(other));
			return *this;
		}


		sl_bool operator==(sl_null_t) const noexcept
		{
			return _ptr == sl_null;
		}

		sl_bool operator==(T* other) const noexcept
		{
			return _ptr == other;
		}

		template <class O>
		sl_bool operator==(const AtomicPtr<O>& other) const noexcept
		{
			return _ptr == other._ptr;
		}

		template <class O>
		sl_bool operator==(const Ptr<O>& other) const noexcept
		{
			return _ptr == other.ptr;
		}

		sl_bool operator!=(sl_null_t) const noexcept
		{
			return _ptr != sl_null;
		}

		sl_bool operator!=(T* other) const noexcept
		{
			return _ptr != other;
		}

		template <class O>
		sl_bool operator!=(const AtomicPtr<O>& other) const noexcept
		{
			return _ptr != other._ptr;
		}

		template <class O>
		sl_bool operator!=(const Ptr<O>& other) const noexcept
		{
			return _ptr != other.ptr;
		}

	public:
		T* _retain(Ref<Referable>& reference) const noexcept
		{
			if ((void*)this == (void*)(&(priv::ptr::g_null))) {
				return sl_null;
			} else {			
				m_lock.lock();
				reference = _ref;
				T* ret = _ptr;
				m_lock.unlock();
				return ret;
			}
		}

		void _replace(T* pointer, const Ref<Referable>& reference) noexcept
		{
			m_lock.lock();
			_ptr = pointer;
			Referable* refOld = _ref.ptr;
			new (&_ref) Ref<Referable>(reference);
			m_lock.unlock();
			if (refOld) {
				refOld->decreaseReference();
			}
		}

		void _move_init(void* _other) noexcept
		{
			AtomicPtr<T>& other = *(reinterpret_cast<AtomicPtr<T>*>(_other));
			_ref._move_init(&(other._ref));
		}

		void _move_assign(void* _other) noexcept
		{
			if ((void*)this != _other) {
				AtomicPtr<T>& other = *(reinterpret_cast<AtomicPtr<T>*>(_other));
				m_lock.lock();
				Referable* refOld = _ref.ptr;
				_ref._move_init(&(other._ref));
				m_lock.unlock();
				if (refOld) {
					refOld->decreaseReference();
				}
			}
		}
	
	private:
		SpinLock m_lock;
	
	};
	

	template <class... TYPES>
	class SLIB_EXPORT PtrLocker;

	template <class T>
	class SLIB_EXPORT PtrLocker<T>
	{
	private:
		Ptr<T> m_ptr;

	public:
		PtrLocker(const Ptr<T>& ptr) noexcept : m_ptr(ptr.lock())
		{}

		PtrLocker(const AtomicPtr<T>& ptr) noexcept	: m_ptr(ptr.lock())
		{}

	public:
		void unlock() noexcept
		{
			m_ptr.setNull();
		}
	
		T* get() noexcept
		{
			return m_ptr.ptr;
		}

		sl_bool isNull() noexcept
		{
			return m_ptr.isNull();
		}
	
		sl_bool isNotNull() noexcept
		{
			return m_ptr.isNotNull();
		}
	
		T& operator*() const noexcept
		{
			return *(m_ptr.ptr);
		}
	
		T* operator->() const noexcept
		{
			return m_ptr.ptr;
		}

		operator T*() const noexcept
		{
			return m_ptr.ptr;
		}

		explicit operator sl_bool() const noexcept
		{
			return m_ptr.ptr;
		}

	};
	
	namespace priv
	{
		namespace ptr
		{

			template <class T>
			class SharedPtrContainer : public Referable
			{
			public:
				T* ptr;
				
			public:
				SharedPtrContainer(T* _ptr) : ptr(_ptr)
				{					
				}
				
				~SharedPtrContainer()
				{
					delete ptr;
				}
			};

			template <class T, class DELETER>
			class SharedPtrContainerWithDeleter : public Referable
			{
			public:
				T* ptr;
				DELETER deleter;
				
			public:
				SharedPtrContainerWithDeleter(T* _ptr, const DELETER& _deleter): ptr(_ptr), deleter(_deleter)
				{
				}

				~SharedPtrContainerWithDeleter()
				{
					deleter(ptr);
				}
				
			};
			
			template <class T>
			class SharedPtrObjectContainer : public Referable
			{
			public:
				T object;
				
			public:
				template <class... ARGS>
				SharedPtrObjectContainer(ARGS&&... args): object(Forward<ARGS>(args)...)
				{
				}
				
			};
			
		}
	}

	template <class T>
	Ptr<T> SharedPtr(T* _ptr)
	{
		if (_ptr) {
			return Ptr<T>(_ptr, ToRef(new priv::ptr::SharedPtrContainer<T>(_ptr)));
		} else {
			return sl_null;
		}
	}
	
	template <class T, class Deleter>
	Ptr<T> SharedPtr(T* _ptr, const Deleter& deleter)
	{
		if (_ptr) {
			return Ptr<T>(_ptr, ToRef(new priv::ptr::SharedPtrContainerWithDeleter<T, DELETER>(_ptr, deleter)));
		} else {
			return sl_null;
		}
	}

	template <class T, class... Args>
	Ptr<T> MakeShared(Args&&... args)
	{
		Ref< priv::ptr::SharedPtrObjectContainer<T> > ptr = new priv::ptr::SharedPtrObjectContainer<T>(Forward<Args>(args)...);
		if (ptr.isNotNull()) {
			return Ptr<T>(&(ptr->object), ptr);
		} else {
			return sl_null;
		}
	}

	template <class T>
	SLIB_INLINE sl_bool operator==(sl_null_t, const Ptr<T>& o) noexcept
	{
		return o.ptr == sl_null;
	}

	template <class T>
	SLIB_INLINE sl_bool operator==(T* other, const Ptr<T>& o) noexcept
	{
		return o.ptr == other;
	}

	template <class T>
	SLIB_INLINE sl_bool operator!=(sl_null_t, const Ptr<T>& o) noexcept
	{
		return o.ptr != sl_null;
	}

	template <class T>
	SLIB_INLINE sl_bool operator!=(T* other, const Ptr<T>& o) noexcept
	{
		return o.ptr != other;
	}
	
	template <class T>
	SLIB_INLINE sl_bool operator==(sl_null_t, const Atomic< Ptr<T> >& p) noexcept
	{
		return p._ptr == sl_null;
	}

	template <class T>
	SLIB_INLINE sl_bool operator==(T* other, const Atomic< Ptr<T> >& p) noexcept
	{
		return p._ptr == other;
	}

	template <class T>
	SLIB_INLINE sl_bool operator!=(sl_null_t, const Atomic< Ptr<T> >& p) noexcept
	{
		return p._ptr != sl_null;
	}

	template <class T>
	SLIB_INLINE sl_bool operator!=(T* other, const Atomic< Ptr<T> >& p) noexcept
	{
		return p._ptr != other;
	}

}

#endif
