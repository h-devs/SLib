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

#include "ref.h"

#include <new>

namespace slib
{
	
	namespace priv
	{
		namespace ptr
		{

			struct ConstStruct;
			extern const ConstStruct g_null;
			
			template <class... TYPES>
			static const Ref<Referable>& GetRef(const Ref<TYPES...>& ref) noexcept
			{
				return Ref<Referable>::from(ref);
			}

			template <class... TYPES>
			static Ref<Referable>&& GetRef(Ref<TYPES...>&& ref) noexcept
			{
				return Move(Ref<Referable>::from(ref));
			}

			template <class OTHER>
			static const AtomicRef<Referable>& GetRef(const AtomicRef<OTHER>& ref) noexcept
			{
				return AtomicRef<Referable>::from(ref);
			}

			template <class OTHER>
			static const Ref<Referable>& GetRef(const WeakRef<OTHER>& weak) noexcept
			{
				return Ref<Referable>::from(weak._weak);
			}

			template <class OTHER>
			static Ref<Referable>&& GetRef(WeakRef<OTHER>&& weak) noexcept
			{
				return Move(Ref<Referable>::from(weak._weak));
			}

			template <class OTHER>
			static const AtomicRef<Referable>& GetRef(const AtomicWeakRef<OTHER>& weak) noexcept
			{
				return AtomicRef<Referable>::from(weak._weak);
			}

			template <class T>
			class ManagedPtrContainer : public Referable
			{
			public:
				T* ptr;
				
			public:
				ManagedPtrContainer(T* _ptr): ptr(_ptr) {}
				
				~ManagedPtrContainer()
				{
					delete ptr;
				}

			};

			template <class T, class Deleter>
			class ManagedPtrContainerWithDeleter : public Referable
			{
			public:
				T* ptr;
				Deleter deleter;
				
			public:
				template <class DELETER>
				ManagedPtrContainerWithDeleter(T* _ptr, const DELETER& _deleter): ptr(_ptr), deleter(Forward<DELETER>(_deleter)) {}

				~ManagedPtrContainerWithDeleter()
				{
					deleter(ptr);
				}
				
			};
			
			template <class T>
			class ManagedObjectContainer : public Referable
			{
			public:
				T object;
				
			public:
				template <class... ARGS>
				ManagedObjectContainer(ARGS&&... args): object(Forward<ARGS>(args)...) {}
				
			};
			
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
		constexpr Ptr(): ptr(sl_null) {}

		Ptr(Ptr&& other) noexcept
		{
			ptr = other.ptr;
			_move_init(&other);
		}
	
		Ptr(const Ptr& other) noexcept: ptr(other.ptr), ref(other.ref) {}

		template <class OTHER>
		Ptr(Ptr<OTHER>&& other) noexcept
		{
			SLIB_TRY_CONVERT_TYPE(OTHER*, T*)
			ptr = other.ptr;
			_move_init(&other);
		}
	
		template <class OTHER>
		Ptr(const Ptr<OTHER>& other) noexcept : ptr(other.ptr), ref(other.ref)
		{
			SLIB_TRY_CONVERT_TYPE(OTHER*, T*)
		}

		template <class OTHER>
		Ptr(const AtomicPtr<OTHER>& other) noexcept
		{
			SLIB_TRY_CONVERT_TYPE(OTHER*, T*)
			ptr = other._retain(ref);
		}

		constexpr Ptr(sl_null_t): ptr(sl_null) {}

		constexpr Ptr(T* pointer): ptr(pointer) {}

		template <class OTHER>
		Ptr(const Ref<OTHER>& reference) noexcept: ptr(reference.ptr), ref(reference)
		{
			SLIB_TRY_CONVERT_TYPE(OTHER*, T*)
		}

		template <class OTHER>
		Ptr(Ref<OTHER>&& reference) noexcept: ptr(reference.ptr), ref(Move(reference))
		{
			SLIB_TRY_CONVERT_TYPE(OTHER*, T*)
		}

		template <class OTHER>
		Ptr(const AtomicRef<OTHER>& reference) noexcept: ref(reference)
		{
			SLIB_TRY_CONVERT_TYPE(OTHER*, T*)
			ptr = ref.ptr;
		}

		template <class OTHER>
		Ptr(const WeakRef<OTHER>& weak) noexcept
		{
			SLIB_TRY_CONVERT_TYPE(OTHER*, T*)
			Ref<OTHER> o(weak);
			if (o.isNotNull()) {
				ptr = o.ptr;
				ref = weak._weak;
			} else {
				ptr = sl_null;
			}
		}

		template <class OTHER>
		Ptr(const AtomicWeakRef<OTHER>& _weak) noexcept
		{
			SLIB_TRY_CONVERT_TYPE(OTHER*, T*)
			WeakRef<OTHER> weak(_weak);
			Ref<OTHER> o(weak);
			if (o.isNotNull()) {
				ptr = o.ptr;
				ref = Move(weak._weak);
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
		Ptr(T* pointer, REF&& r) noexcept: ptr(pointer), ref(priv::ptr::GetRef(Forward<REF>(r))) {}

	public:
		static Ptr createManaged(const T* ptr)
		{
			if (ptr) {
				Ref<Referable> ref = new priv::ptr::ManagedPtrContainer<T>(ptr);
				if (ref.isNotNull()) {
					return Ptr((T*)ptr, Move(ref));
				}
			}
			return sl_null;
		}

		template <class DELETER>
		static Ptr createManagedWithDeleter(const T* ptr, DELETER&& deleter)
		{
			if (ptr) {
				Ref<Referable> ref = new priv::ptr::ManagedPtrContainerWithDeleter<T, typename RemoveConstReference<DELETER>::Type>(ptr, deleter);
				if (ref.isNotNull()) {
					return Ptr((T*)ptr, Move(ref));
				}
			}
			return sl_null;
		}

		template <class... Args>
		static Ptr createManagedObject(Args&&... args)
		{
			Ref< priv::ptr::ManagedObjectContainer<T> > ref = new priv::ptr::ManagedObjectContainer<T>(Forward<Args>(args)...);
			if (ref.isNotNull()) {
				return Ptr(&(ref->object), Move(ref));
			} else {
				return sl_null;
			}
		}

		static const Ptr& null() noexcept
		{
			return *(reinterpret_cast<Ptr const*>(&(priv::ptr::g_null)));
		}
	
		constexpr sl_bool isNull() const
		{
			return !ptr;
		}

		constexpr sl_bool isNotNull() const
		{
			return ptr != sl_null;
		}

		void setNull() noexcept
		{
			ptr = sl_null;
			ref.setNull();
		}

		template <class... TYPES>
		static const Ptr& from(const Ptr<TYPES...>& other) noexcept
		{
			return *(reinterpret_cast<Ptr const*>(&other));
		}

		template <class... TYPES>
		static Ptr& from(Ptr<TYPES...>& other) noexcept
		{
			return *(reinterpret_cast<Ptr*>(&other));
		}

		template <class... TYPES>
		static Ptr&& from(Ptr<TYPES...>&& other) noexcept
		{
			return static_cast<Ptr&&>(*(reinterpret_cast<Ptr*>(&other)));
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
		
		Ptr lock() const noexcept
		{
			Referable* obj = ref.ptr;
			if (obj && obj->_isWeakRef()) {
				CWeakRef* weak = static_cast<CWeakRef*>(obj);
				Ref<Referable> r(weak->lock());
				if (r.isNotNull()) {
					return Ptr(ptr, Move(r));
				} else {
					return sl_null;
				}
			} else {
				return *this;
			}
		}
		
		Ptr toWeak() const noexcept
		{
			Referable* obj = ref.ptr;
			if (obj && !(obj->_isWeakRef())) {
				return Ptr(ptr, WeakRef<Referable>(obj));
			} else {
				return *this;
			}
		}

	public:
		constexpr T* get() const
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

		template <class OTHER>
		void set(Ptr<OTHER>&& other) noexcept
		{
			SLIB_TRY_CONVERT_TYPE(OTHER*, T*)
			ptr = other.ptr;
			_move_assign(&other);
		}
	
		template <class OTHER>
		void set(const Ptr<OTHER>& other) noexcept
		{
			SLIB_TRY_CONVERT_TYPE(OTHER*, T*)
			ptr = other.ptr;
			ref = other.ref;
		}

		template <class OTHER>
		void set(const AtomicPtr<OTHER>& other) noexcept
		{
			SLIB_TRY_CONVERT_TYPE(OTHER*, T*)
			ptr = other._retain(ref);
		}

		template <class OTHER>
		void set(const Ref<OTHER>& reference) noexcept
		{
			SLIB_TRY_CONVERT_TYPE(OTHER*, T*)
			ptr = reference.ptr;
			ref = reference;
		}

		template <class OTHER>
		void set(const AtomicRef<OTHER>& reference) noexcept
		{
			SLIB_TRY_CONVERT_TYPE(OTHER*, T*)
			ref = reference;
			ptr = ref.ptr;
		}

		template <class OTHER>
		void set(const WeakRef<OTHER>& weak) noexcept
		{
			SLIB_TRY_CONVERT_TYPE(OTHER*, T*)
			Ref<OTHER> o(weak);
			if (o.isNotNull()) {
				ptr = o.ptr;
				ref = weak._weak;
			} else {
				ptr = sl_null;
				ref.setNull();
			}
		}

		template <class OTHER>
		void set(const AtomicWeakRef<OTHER>& _weak) noexcept
		{
			SLIB_TRY_CONVERT_TYPE(OTHER*, T*)
			WeakRef<OTHER> weak(_weak);
			Ref<OTHER> o(weak);
			if (o.isNotNull()) {
				ptr = o.ptr;
				ref = Move(weak._weak);
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
			ref = priv::ptr::GetRef(Forward<REF>(r));
			if (ref.isNotNull()) {
				ptr = pointer;
			} else {
				ptr = sl_null;
			}
		}

	public:
		Ptr& operator=(Ptr&& other) noexcept
		{
			ptr = other.ptr;
			_move_assign(&other);
			return *this;
		}

		Ptr& operator=(const Ptr& other) noexcept
		{
			ptr = other.ptr;
			ref = other.ref;
			return *this;
		}

		template <class OTHER>
		Ptr& operator=(OTHER&& other) noexcept
		{
			set(Forward<OTHER>(other));
			return *this;
		}

		constexpr T& operator*() const
		{
			return *ptr;
		}

		constexpr T* operator->() const
		{
			return ptr;
		}

		constexpr operator T*() const
		{
			return ptr;
		}

		constexpr explicit operator sl_bool() const
		{
			return ptr != sl_null;
		}

	public:
		constexpr sl_bool equals(sl_null_t) const
		{
			return !ptr;
		}

		template <class OTHER>
		constexpr sl_bool equals(const OTHER* other) const
		{
			return ptr == other;
		}

		template <class OTHER>
		constexpr sl_bool equals(const Ptr<OTHER>& other) const
		{
			return ptr == other.ptr;
		}
	
		template <class OTHER>
		constexpr sl_bool equals(const AtomicPtr<OTHER>& other) const
		{
			return ptr == other._ptr;
		}

		constexpr sl_compare_result compare(sl_null_t) const
		{
			return ptr != sl_null;
		}
	
		template <class OTHER>
		constexpr sl_compare_result compare(const OTHER* other) const
		{
			return ComparePrimitiveValues(ptr, other);
		}

		template <class OTHER>
		constexpr sl_compare_result compare(const Ptr<OTHER>& other) const
		{
			return ComparePrimitiveValues(ptr, other.ptr);
		}

		template <class OTHER>
		constexpr sl_compare_result compare(const AtomicPtr<OTHER>& other) const
		{
			return ComparePrimitiveValues(ptr, other._ptr);
		}

		SLIB_DEFINE_CLASS_DEFAULT_COMPARE_OPERATORS_CONSTEXPR

	public:
		void _move_init(void* _other) noexcept
		{
			Ptr& other = *(reinterpret_cast<Ptr*>(_other));
			ref.ptr = other.ref.ptr;
			other.ref.ptr = sl_null;
		}

		void _move_assign(void* _other) noexcept
		{
			if ((void*)this != _other) {
				Ptr& other = *(reinterpret_cast<Ptr*>(_other));
				ref._move_assign(&(other.ref));
			}
		}

	};
	
	
	template <class T>
	class SLIB_EXPORT Atomic< Ptr<T> >
	{
	public:
		T* _ptr;
		Ref<Referable> _ref;
	
	public:
		constexpr Atomic(): _ptr(sl_null) {}

		Atomic(const Atomic& other) noexcept
		{
			_ptr = other._retain(_ref);
		}

		template <class OTHER>
		Atomic(const AtomicPtr<OTHER>& other) noexcept
		{
			SLIB_TRY_CONVERT_TYPE(OTHER*, T*)
			_ptr = other._retain(_ref);
		}

		template <class OTHER>
		Atomic(Ptr<OTHER>&& other) noexcept
		{
			SLIB_TRY_CONVERT_TYPE(OTHER*, T*)
			_ptr = other.ptr;
			_move_init(&other);
		}
	
		template <class OTHER>
		Atomic(const Ptr<OTHER>& other) noexcept: _ptr(other.ptr), _ref(other.ref)
		{
			SLIB_TRY_CONVERT_TYPE(OTHER*, T*)
		}

		constexpr Atomic(sl_null_t): _ptr(sl_null) {}

		Atomic(T* pointer) noexcept: _ptr(pointer) {}

		template <class OTHER>
		Atomic(const Ref<OTHER>& reference) noexcept: _ptr(reference.ptr), _ref(reference)
		{
			SLIB_TRY_CONVERT_TYPE(OTHER*, T*)
		}

		template <class OTHER>
		Atomic(const AtomicRef<OTHER>& reference) noexcept: _ref(reference)
		{
			SLIB_TRY_CONVERT_TYPE(OTHER*, T*)
			_ptr = _ref.ptr;
		}

		template <class OTHER>
		Atomic(const WeakRef<OTHER>& weak) noexcept
		{
			SLIB_TRY_CONVERT_TYPE(OTHER*, T*)
			Ref<OTHER> o(weak);
			if (o.isNotNull()) {
				_ptr = o.ptr;
				_ref = weak._weak;
			} else {
				_ptr = sl_null;
			}
		}

		template <class OTHER>
		Atomic(const AtomicWeakRef<OTHER>& _weak) noexcept
		{
			SLIB_TRY_CONVERT_TYPE(OTHER*, T*)
			WeakRef<OTHER> weak(_weak);
			Ref<OTHER> o(weak);
			if (o.isNotNull()) {
				_ptr = o.ptr;
				_ref = Move(weak._weak);
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
		Atomic(T* pointer, REF&& r) noexcept: _ptr(pointer), _ref(priv::ptr::GetRef(Forward<REF>(r))) {}

	public:
		constexpr sl_bool isNull() const
		{
			return _ptr == sl_null;
		}

		constexpr sl_bool isNotNull() const
		{
			return _ptr != sl_null;
		}

		void setNull() noexcept
		{
			_replace(sl_null, Ref<Referable>::null());
		}

		template <class OTHER>
		static const Atomic& from(const AtomicPtr<OTHER>& other) noexcept
		{
			return *(reinterpret_cast<Atomic const*>(&other));
		}

		template <class OTHER>
		static Atomic& from(AtomicPtr<OTHER>& other) noexcept
		{
			return *(reinterpret_cast<Atomic*>(&other));
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

		template <class OTHER>
		void set(const AtomicPtr<OTHER>& other) noexcept
		{
			SLIB_TRY_CONVERT_TYPE(OTHER*, T*)
			Ref<Referable> reference;
			T* pointer = other._retain(reference);
			_replace(pointer, Move(reference));
		}

		template <class OTHER>
		void set(Ptr<OTHER>&& other) noexcept
		{
			SLIB_TRY_CONVERT_TYPE(OTHER*, T*)
			_ptr = other.ptr;
			_move_assign(&other);
		}
	
		template <class OTHER>
		void set(const Ptr<OTHER>& other) noexcept
		{
			SLIB_TRY_CONVERT_TYPE(OTHER*, T*)
			_replace(other.ptr, other.ref);
		}

		template <class OTHER>
		void set(const Ref<OTHER>& reference) noexcept
		{
			SLIB_TRY_CONVERT_TYPE(OTHER*, T*)
			_replace(reference.ptr, Ref<Referable>::from(reference));
		}

		template <class OTHER>
		void set(const AtomicRef<OTHER>& _reference) noexcept
		{
			SLIB_TRY_CONVERT_TYPE(OTHER*, T*)
			Ref<OTHER> reference(_reference);
			_replace(reference.ptr, Ref<Referable>::from(reference));
		}

		template <class OTHER>
		void set(const WeakRef<OTHER>& weak) noexcept
		{
			SLIB_TRY_CONVERT_TYPE(OTHER*, T*)
			Ref<OTHER> o(weak);
			if (o.isNotNull()) {
				_replace(o.ptr, Ref<Referable>::from(weak._weak));
			} else {
				_replace(sl_null, Ref<Referable>::null());
			}
		}

		template <class OTHER>
		void set(const AtomicWeakRef<OTHER>& _weak) noexcept
		{
			SLIB_TRY_CONVERT_TYPE(OTHER*, T*)
			WeakRef<OTHER> weak(_weak);
			Ref<OTHER> o(weak);
			if (o.isNotNull()) {
				_replace(o.ptr, Ref<Referable>::from(Move(weak._weak)));
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
			_replace(pointer, priv::ptr::GetRef(Forward<REF>(r)));
		}

	public:
		Atomic& operator=(const Atomic& other) noexcept
		{
			Ref<Referable> reference;
			T* pointer = other._retain(reference);
			_replace(pointer, Move(reference));
			return *this;
		}

		template <class REF>
		Atomic& operator=(REF&& other) noexcept
		{
			set(Forward<REF>(other));
			return *this;
		}

	public:
		constexpr sl_bool equals(sl_null_t) const
		{
			return !_ptr;
		}

		template <class OTHER>
		constexpr sl_bool equals(const OTHER* other) const
		{
			return _ptr == other;
		}

		template <class OTHER>
		constexpr sl_bool equals(const Ptr<OTHER>& other) const
		{
			return _ptr == other.ptr;
		}
	
		template <class OTHER>
		constexpr sl_bool equals(const AtomicPtr<OTHER>& other) const
		{
			return _ptr == other._ptr;
		}

		constexpr sl_compare_result compare(sl_null_t) const
		{
			return _ptr != sl_null;
		}
	
		template <class OTHER>
		constexpr sl_compare_result compare(const OTHER* other) const
		{
			return ComparePrimitiveValues(_ptr, other);
		}

		template <class OTHER>
		constexpr sl_compare_result compare(const Ptr<OTHER>& other) const
		{
			return ComparePrimitiveValues(_ptr, other.ptr);
		}

		template <class OTHER>
		constexpr sl_compare_result compare(const AtomicPtr<OTHER>& other) const
		{
			return ComparePrimitiveValues(_ptr, other._ptr);
		}

		SLIB_DEFINE_CLASS_DEFAULT_COMPARE_OPERATORS_CONSTEXPR

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

		void _replace(T* pointer, Ref<Referable>&& reference) noexcept
		{
			m_lock.lock();
			_ptr = pointer;
			Referable* refOld = _ref.ptr;
			new (&_ref) Ref<Referable>(Move(reference));
			m_lock.unlock();
			if (refOld) {
				refOld->decreaseReference();
			}
		}

		void _move_init(void* _other) noexcept
		{
			Ptr<T>& other = *(reinterpret_cast<Ptr<T>*>(_other));
			_ref.ptr = other.ref.ptr;
			other.ref.ptr = sl_null;
		}

		void _move_assign(void* _other) noexcept
		{
			if ((void*)this != _other) {
				Ptr<T>& other = *(reinterpret_cast<Ptr<T>*>(_other));
				m_lock.lock();
				Referable* refOld = _ref.ptr;
				_ref.ptr = other.ref.ptr;
				other.ref.ptr = sl_null;
				m_lock.unlock();
				if (refOld) {
					refOld->decreaseReference();
				}
			}
		}
	
	private:
		SpinLock m_lock;
	
	};
	

	template <class T>
	constexpr sl_bool operator==(sl_null_t, const Ptr<T>& o)
	{
		return !(o.ptr);
	}

	template <class T>
	constexpr sl_bool operator==(T* other, const Ptr<T>& o)
	{
		return o.ptr == other;
	}

	template <class T>
	constexpr sl_bool operator!=(sl_null_t, const Ptr<T>& o)
	{
		return o.ptr != sl_null;
	}

	template <class T>
	constexpr sl_bool operator!=(T* other, const Ptr<T>& o)
	{
		return o.ptr != other;
	}
	
	template <class T>
	constexpr sl_bool operator==(sl_null_t, const Atomic< Ptr<T> >& p)
	{
		return !(p._ptr);
	}

	template <class T>
	constexpr sl_bool operator==(T* other, const Atomic< Ptr<T> >& p)
	{
		return p._ptr == other;
	}

	template <class T>
	constexpr sl_bool operator!=(sl_null_t, const Atomic< Ptr<T> >& p)
	{
		return p._ptr != sl_null;
	}

	template <class T>
	constexpr sl_bool operator!=(T* other, const Atomic< Ptr<T> >& p)
	{
		return p._ptr != other;
	}


	template <class... TYPES>
	class SLIB_EXPORT PtrLocker;

	template <class T>
	class SLIB_EXPORT PtrLocker<T>
	{
	private:
		Ptr<T> m_ptr;

	public:
		PtrLocker(const Ptr<T>& ptr) noexcept: m_ptr(ptr.lock()) {}

		PtrLocker(const AtomicPtr<T>& ptr) noexcept: m_ptr(ptr.lock()) {}

	public:
		void unlock() noexcept
		{
			m_ptr.setNull();
		}
	
		constexpr T* get() const
		{
			return m_ptr.ptr;
		}

		constexpr sl_bool isNull() const
		{
			return m_ptr.isNull();
		}
	
		constexpr sl_bool isNotNull() const
		{
			return m_ptr.isNotNull();
		}
	
		constexpr T& operator*() const
		{
			return *(m_ptr.ptr);
		}
	
		constexpr T* operator->() const
		{
			return m_ptr.ptr;
		}

		constexpr operator T*() const
		{
			return m_ptr.ptr;
		}

		constexpr explicit operator sl_bool() const
		{
			return m_ptr.ptr;
		}

	};
	
}

#endif
