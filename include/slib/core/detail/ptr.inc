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

#include <new>

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

	template <class T>
	SLIB_INLINE Ptr<T>::Ptr() noexcept
	 : ptr(sl_null)
	 {}

	template <class T>
	SLIB_INLINE Ptr<T>::~Ptr()
	{
		ptr = sl_null;
	}

	template <class T>
	SLIB_INLINE Ptr<T>::Ptr(Ptr<T>&& other) noexcept
	{
		ptr = other.ptr;
		_move_init(&other);
	}

	template <class T>
	SLIB_INLINE Ptr<T>::Ptr(const Ptr<T>& other) noexcept
	 : ptr(other.ptr), ref(other.ref)
	 {}
	
	template <class T>
	template <class O>
	SLIB_INLINE Ptr<T>::Ptr(Ptr<O>&& other) noexcept
	{
		SLIB_TRY_CONVERT_TYPE(O*, T*)
		ptr = other.ptr;
		_move_init(&other);
	}

	template <class T>
	template <class O>
	SLIB_INLINE Ptr<T>::Ptr(const Ptr<O>& other) noexcept
	 : ptr(other.ptr), ref(other.ref)
	{
		SLIB_TRY_CONVERT_TYPE(O*, T*)
	}
	
	template <class T>
	template <class O>
	Ptr<T>::Ptr(const AtomicPtr<O>& other) noexcept
	{
		SLIB_TRY_CONVERT_TYPE(O*, T*)
		ptr = other._retain(ref);
	}
	
	template <class T>
	SLIB_INLINE Ptr<T>::Ptr(sl_null_t) noexcept
	 : ptr(sl_null)
	 {}
	
	template <class T>
	SLIB_INLINE Ptr<T>::Ptr(T* pointer) noexcept
	 : ptr(pointer)
	 {}
	
	template <class T>
	template <class O>
	SLIB_INLINE Ptr<T>::Ptr(const Ref<O>& reference) noexcept
	 : ptr(reference.ptr), ref(reference)
	{
		SLIB_TRY_CONVERT_TYPE(O*, T*)
	}
	
	template <class T>
	template <class O>
	Ptr<T>::Ptr(const AtomicRef<O>& reference) noexcept
	 : ref(reference)
	{
		SLIB_TRY_CONVERT_TYPE(O*, T*)
		ptr = ref.ptr;
	}
	
	template <class T>
	template <class O>
	Ptr<T>::Ptr(const WeakRef<O>& weak) noexcept
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
	
	template <class T>
	template <class O>
	Ptr<T>::Ptr(const AtomicWeakRef<O>& _weak) noexcept
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
	
	template <class T>
	template <class REF>
	Ptr<T>::Ptr(T* pointer, REF&& r) noexcept: ref(_getRef(Forward<REF>(r)))
	{
		if (ref.isNull()) {
			ptr = sl_null;
		} else {
			ptr = pointer;
		}
	}
	
	template <class T>
	SLIB_INLINE const Ptr<T>& Ptr<T>::null() noexcept
	{
		return *(reinterpret_cast<Ptr<T> const*>(&(priv::ptr::g_null)));
	}
	
	template <class T>
	SLIB_INLINE sl_bool Ptr<T>::isNull() const noexcept
	{
		return ptr == sl_null;
	}
	
	template <class T>
	SLIB_INLINE sl_bool Ptr<T>::isNotNull() const noexcept
	{
		return ptr != sl_null;
	}
	
	template <class T>
	void Ptr<T>::setNull() noexcept
	{
		ptr = sl_null;
		ref.setNull();
	}
	
	template <class T>
	template <class... TYPES>
	SLIB_INLINE const Ptr<T>& Ptr<T>::from(const Ptr<TYPES...>& other) noexcept
	{
		return *(reinterpret_cast<Ptr<T> const*>(&other));
	}
	
	template <class T>
	template <class... TYPES>
	SLIB_INLINE Ptr<T>& Ptr<T>::from(Ptr<TYPES...>& other) noexcept
	{
		return *(reinterpret_cast<Ptr<T>*>(&other));
	}
	
	template <class T>
	template <class... TYPES>
	SLIB_INLINE Ptr<T>&& Ptr<T>::from(Ptr<TYPES...>&& other) noexcept
	{
		return static_cast<Ptr<T>&&>(*(reinterpret_cast<Ptr<T>*>(&other)));
	}

	template <class T>
	sl_bool Ptr<T>::isWeak() const noexcept
	{
		Referable* obj = ref.ptr;
		return obj && obj->_isWeakRef();
	}
	
	template <class T>
	sl_bool Ptr<T>::lockRef(Ref<Referable>& ref) const noexcept
	{
		Referable* obj = ref.ptr;
		if (obj && obj->_isWeakRef()) {
			CWeakRef* weak = static_cast<CWeakRef*>(obj);
			Ref<Referable> r(weak->lock());
			if (r.isNotNull()) {
				ref = Move(r);
				return sl_true;
			} else {
				return sl_false;
			}
		} else {
			return sl_true;
		}
	}

	template <class T>
	Ptr<T> Ptr<T>::lock() const noexcept
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

	template <class T>
	Ptr<T> Ptr<T>::toWeak() const noexcept
	{
		Referable* obj = ref.ptr;
		if (obj && !(obj->_isWeakRef())) {
			return Ptr<T>(ptr, WeakRef<Referable>(obj));
		} else {
			return *this;
		}
	}

	template <class T>
	SLIB_INLINE T* Ptr<T>::get() const noexcept
	{
		return ptr;
	}

	template <class T>
	void Ptr<T>::set(T* pointer) noexcept
	{
		ptr = pointer;
		ref.setNull();
	}
	
	template <class T>
	void Ptr<T>::set(sl_null_t) noexcept
	{
		ptr = sl_null;
		ref.setNull();
	}
	
	template <class T>
	template <class O>
	void Ptr<T>::set(Ptr<O>&& other) noexcept
	{
		SLIB_TRY_CONVERT_TYPE(O*, T*)
		ptr = other.ptr;
		_move_assign(&other);
	}

	template <class T>
	template <class O>
	void Ptr<T>::set(const Ptr<O>& other) noexcept
	{
		SLIB_TRY_CONVERT_TYPE(O*, T*)
		ptr = other.ptr;
		ref = other.ref;
	}
	
	template <class T>
	template <class O>
	void Ptr<T>::set(const AtomicPtr<O>& other) noexcept
	{
		SLIB_TRY_CONVERT_TYPE(O*, T*)
		ptr = other._retain(ref);
	}

	template <class T>
	template <class O>
	void Ptr<T>::set(const Ref<O>& reference) noexcept
	{
		SLIB_TRY_CONVERT_TYPE(O*, T*)
		ptr = reference.ptr;
		ref = reference;
	}
	
	template <class T>
	template <class O>
	void Ptr<T>::set(const AtomicRef<O>& reference) noexcept
	{
		SLIB_TRY_CONVERT_TYPE(O*, T*)
		ref = reference;
		ptr = ref.ptr;
	}
	
	template <class T>
	template <class O>
	void Ptr<T>::set(const WeakRef<O>& weak) noexcept
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
	
	template <class T>
	template <class O>
	void Ptr<T>::set(const AtomicWeakRef<O>& _weak) noexcept
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
	
	template <class T>
	template <class REF>
	void Ptr<T>::set(T* pointer, REF&& r) noexcept
	{
		ref = _getRef(Forward<REF>(r));
		if (ref.isNotNull()) {
			ptr = pointer;
		} else {
			ptr = sl_null;
		}
	}
	
	template <class T>
	Ptr<T>& Ptr<T>::operator=(Ptr<T>&& other) noexcept
	{
		ptr = other.ptr;
		_move_assign(&other);
		return *this;
	}
	
	template <class T>
	Ptr<T>& Ptr<T>::operator=(const Ptr<T>& other) noexcept
	{
		ptr = other.ptr;
		ref = other.ref;
		return *this;
	}
	
	template <class T>
	template <class OTHER>
	Ptr<T>& Ptr<T>::operator=(OTHER&& other) noexcept
	{
		set(Forward<OTHER>(other));
		return *this;
	}

	template <class T>
	SLIB_INLINE sl_bool Ptr<T>::operator==(sl_null_t) const noexcept
	{
		return ptr == sl_null;
	}

	template <class T>
	SLIB_INLINE sl_bool Ptr<T>::operator==(T* other) const noexcept
	{
		return ptr == other;
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
	template <class O>
	SLIB_INLINE sl_bool Ptr<T>::operator==(const Ptr<O>& other) const noexcept
	{
		return ptr == other.ptr;
	}
	
	template <class T>
	template <class O>
	SLIB_INLINE sl_bool Ptr<T>::operator==(const AtomicPtr<O>& other) const noexcept
	{
		return ptr == other._ptr;
	}
	
	template <class T>
	SLIB_INLINE sl_bool Ptr<T>::operator!=(sl_null_t) const noexcept
	{
		return ptr != sl_null;
	}

	template <class T>
	SLIB_INLINE sl_bool Ptr<T>::operator!=(T* other) const noexcept
	{
		return ptr != other;
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
	template <class O>
	SLIB_INLINE sl_bool Ptr<T>::operator!=(const Ptr<O>& other) const noexcept
	{
		return ptr != other.ptr;
	}
	
	template <class T>
	template <class O>
	SLIB_INLINE sl_bool Ptr<T>::operator!=(const AtomicPtr<O>& other) const noexcept
	{
		return ptr != other._ptr;
	}
	
	template <class T>
	SLIB_INLINE T& Ptr<T>::operator*() const noexcept
	{
		return *ptr;
	}
	
	template <class T>
	SLIB_INLINE T* Ptr<T>::operator->() const noexcept
	{
		return ptr;
	}
	
	template <class T>
	SLIB_INLINE Ptr<T>::operator T*() const noexcept
	{
		return ptr;
	}

	template <class T>
	SLIB_INLINE Ptr<T>::operator sl_bool() const noexcept
	{
		return ptr != sl_null;
	}

	template <class T>
	SLIB_INLINE void Ptr<T>::_move_init(void* _other) noexcept
	{
		Ptr<T>& other = *(reinterpret_cast<Ptr<T>*>(_other));
		ref._move_init(&(other.ref));
	}
	
	template <class T>
	SLIB_INLINE void Ptr<T>::_move_assign(void* _other) noexcept
	{
		if ((void*)this != _other) {
			Ptr<T>& other = *(reinterpret_cast<Ptr<T>*>(_other));
			ref._move_assign(&(other.ref));
		}
	}
	
	template <class T>
	template <class... TYPES>
	SLIB_INLINE const Ref<Referable>& Ptr<T>::_getRef(const Ref<TYPES...>& ref) noexcept
	{
		return Ref<Referable>::from(ref);
	}

	template <class T>
	template <class... TYPES>
	SLIB_INLINE Ref<Referable>&& Ptr<T>::_getRef(Ref<TYPES...>&& ref) noexcept
	{
		return Move(Ref<Referable>::from(ref));
	}

	template <class T>
	template <class O>
	SLIB_INLINE const AtomicRef<Referable>& Ptr<T>::_getRef(const AtomicRef<O>& ref) noexcept
	{
		return AtomicRef<Referable>::from(ref);
	}

	template <class T>
	template <class O>
	SLIB_INLINE const Ref<Referable>& Ptr<T>::_getRef(const WeakRef<O>& weak) noexcept
	{
		return Ref<Referable>::from(weak._weak);
	}

	template <class T>
	template <class O>
	SLIB_INLINE Ref<Referable>&& Ptr<T>::_getRef(WeakRef<O>&& weak) noexcept
	{
		return Move(Ref<Referable>::from(weak._weak));
	}

	template <class T>
	template <class O>
	SLIB_INLINE const AtomicRef<Referable>& Ptr<T>::_getRef(const AtomicWeakRef<O>& weak) noexcept
	{
		return AtomicRef<Referable>::from(weak._weak);
	}

	
	template <class T>
	Atomic< Ptr<T> >::Atomic() noexcept
	 : _ptr(sl_null)
	 {}
	
	template <class T>
	Atomic< Ptr<T> >::~Atomic()
	{
		_ptr = sl_null;
	}
	
	template <class T>
	Atomic< Ptr<T> >::Atomic(const AtomicPtr<T>& other) noexcept
	{
		_ptr = other._retain(_ref);
	}
	
	template <class T>
	template <class O>
	Atomic< Ptr<T> >::Atomic(const AtomicPtr<O>& other) noexcept
	{
		SLIB_TRY_CONVERT_TYPE(O*, T*)
		_ptr = other._retain(_ref);
	}
	
	template <class T>
	template <class O>
	Atomic< Ptr<T> >::Atomic(Ptr<O>&& other) noexcept
	{
		SLIB_TRY_CONVERT_TYPE(O*, T*)
		_ptr = other.ptr;
		_move_init(&other);
	}

	template <class T>
	template <class O>
	Atomic< Ptr<T> >::Atomic(const Ptr<O>& other) noexcept
	 : _ptr(other.ptr), _ref(other.ref)
	{
		SLIB_TRY_CONVERT_TYPE(O*, T*)
	}
	
	template <class T>
	Atomic< Ptr<T> >::Atomic(sl_null_t) noexcept
	 : _ptr(sl_null)
	 {}
	
	template <class T>
	Atomic< Ptr<T> >::Atomic(T* pointer) noexcept
	 : _ptr(pointer)
	 {}
	
	template <class T>
	template <class O>
	Atomic< Ptr<T> >::Atomic(const Ref<O>& reference) noexcept
	 : _ptr(reference.ptr), _ref(reference)
	{
		SLIB_TRY_CONVERT_TYPE(O*, T*)
	}
	
	template <class T>
	template <class O>
	Atomic< Ptr<T> >::Atomic(const AtomicRef<O>& reference) noexcept
	 : _ref(reference)
	{
		SLIB_TRY_CONVERT_TYPE(O*, T*)
		_ptr = _ref.ptr;
	}
	
	template <class T>
	template <class O>
	Atomic< Ptr<T> >::Atomic(const WeakRef<O>& weak) noexcept
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
	
	template <class T>
	template <class O>
	Atomic< Ptr<T> >::Atomic(const AtomicWeakRef<O>& _weak) noexcept
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
	
	template <class T>
	template <class REF>
	Atomic< Ptr<T> >::Atomic(T* pointer, REF&& r) noexcept
	 : _ref(Ptr<T>::_getRef(Forward<REF>(r)))
	{
		if (_ref.isNull()) {
			_ptr = sl_null;
		} else {
			_ptr = pointer;
		}
	}
	
	template <class T>
	SLIB_INLINE const AtomicPtr<T>& Atomic< Ptr<T> >::null() noexcept
	{
		return *(reinterpret_cast<AtomicPtr<T> const*>(&(priv::ptr::g_null)));
	}
	
	template <class T>
	sl_bool Atomic< Ptr<T> >::isNull() const noexcept
	{
		return _ptr == sl_null;
	}
	
	template <class T>
	sl_bool Atomic< Ptr<T> >::isNotNull() const noexcept
	{
		return _ptr != sl_null;
	}
	
	template <class T>
	void Atomic< Ptr<T> >::setNull() noexcept
	{
		_replace(sl_null, Ref<Referable>::null());
	}
	
	template <class T>
	template <class OTHER>
	SLIB_INLINE const AtomicPtr<T>& Atomic< Ptr<T> >::from(const AtomicPtr<OTHER>& other) noexcept
	{
		return *(reinterpret_cast<AtomicPtr<OTHER> const*>(&other));
	}
	
	template <class T>
	template <class OTHER>
	SLIB_INLINE AtomicPtr<T>& Atomic< Ptr<T> >::from(AtomicPtr<OTHER>& other) noexcept
	{
		return *(reinterpret_cast<AtomicPtr<OTHER>*>(&other));
	}
	
	template <class T>
	Ptr<T> Atomic< Ptr<T> >::lock() const noexcept
	{
		Ptr<T> p(*this);
		return p.lock();
	}
	
	template <class T>
	void Atomic< Ptr<T> >::set(T* pointer) noexcept
	{
		_replace(pointer, Ref<Referable>::null());
	}
	
	template <class T>
	void Atomic< Ptr<T> >::set(sl_null_t) noexcept
	{
		_replace(sl_null, Ref<Referable>::null());
	}
	
	template <class T>
	template <class O>
	void Atomic< Ptr<T> >::set(const AtomicPtr<O>& other) noexcept
	{
		SLIB_TRY_CONVERT_TYPE(O*, T*)
		Ref<Referable> reference;
		T* pointer = other._retain(reference);
		_replace(pointer, reference);
	}
	
	template <class T>
	template <class O>
	void Atomic< Ptr<T> >::set(Ptr<O>&& other) noexcept
	{
		SLIB_TRY_CONVERT_TYPE(O*, T*)
		_ptr = other.ptr;
		_move_assign(&other);
	}
	
	template <class T>
	template <class O>
	void Atomic< Ptr<T> >::set(const Ptr<O>& other) noexcept
	{
		SLIB_TRY_CONVERT_TYPE(O*, T*)
		_replace(other.ptr, other.ref);
	}
	
	template <class T>
	template <class O>
	void Atomic< Ptr<T> >::set(const Ref<O>& reference) noexcept
	{
		SLIB_TRY_CONVERT_TYPE(O*, T*)
		_replace(reference.ptr, Ref<Referable>::from(reference));
	}
	
	template <class T>
	template <class O>
	void Atomic< Ptr<T> >::set(const AtomicRef<O>& _reference) noexcept
	{
		SLIB_TRY_CONVERT_TYPE(O*, T*)
		Ref<O> reference(_reference);
		_replace(reference.ptr, Ref<Referable>::from(reference));
	}

	template <class T>
	template <class O>
	void Atomic< Ptr<T> >::set(const WeakRef<O>& weak) noexcept
	{
		SLIB_TRY_CONVERT_TYPE(O*, T*)
		Ref<O> o(weak);
		if (o.isNotNull()) {
			_replace(o.ptr, Ref<Referable>::from(weak._weak));
		} else {
			_replace(sl_null, Ref<Referable>::null());
		}
	}
	
	template <class T>
	template <class O>
	void Atomic< Ptr<T> >::set(const AtomicWeakRef<O>& _weak) noexcept
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
	
	template <class T>
	template <class REF>
	void Atomic< Ptr<T> >::set(T* pointer, REF&& r) noexcept
	{
		_replace(pointer, Ptr<T>::_getRef(Forward<REF>(r)));
	}
	
	template <class T>
	AtomicPtr<T>& Atomic< Ptr<T> >::operator=(const AtomicPtr<T>& other) noexcept
	{
		Ref<Referable> reference;
		T* pointer = other._retain(reference);
		_replace(pointer, reference);
		return *this;
	}
	
	template <class T>
	template <class OTHER>
	AtomicPtr<T>& Atomic< Ptr<T> >::operator=(OTHER&& other) noexcept
	{
		set(Forward<OTHER>(other));
		return *this;
	}

	template <class T>
	SLIB_INLINE sl_bool Atomic< Ptr<T> >::operator==(sl_null_t) const noexcept
	{
		return _ptr == sl_null;
	}

	template <class T>
	SLIB_INLINE sl_bool Atomic< Ptr<T> >::operator==(T* other) const noexcept
	{
		return _ptr == other;
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
	template <class O>
	SLIB_INLINE sl_bool Atomic< Ptr<T> >::operator==(const AtomicPtr<O>& other) const noexcept
	{
		return _ptr == other._ptr;
	}
	
	template <class T>
	template <class O>
	SLIB_INLINE sl_bool Atomic< Ptr<T> >::operator==(const Ptr<O>& other) const noexcept
	{
		return _ptr == other.ptr;
	}
	
	template <class T>
	SLIB_INLINE sl_bool Atomic< Ptr<T> >::operator!=(sl_null_t) const noexcept
	{
		return _ptr != sl_null;
	}

	template <class T>
	SLIB_INLINE sl_bool Atomic< Ptr<T> >::operator!=(T* other) const noexcept
	{
		return _ptr != other;
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

	template <class T>
	template <class O>
	SLIB_INLINE sl_bool Atomic< Ptr<T> >::operator!=(const AtomicPtr<O>& other) const noexcept
	{
		return _ptr != other._ptr;
	}
	
	template <class T>
	template <class O>
	SLIB_INLINE sl_bool Atomic< Ptr<T> >::operator!=(const Ptr<O>& other) const noexcept
	{
		return _ptr != other.ptr;
	}
	
	template <class T>
	SLIB_INLINE T* Atomic< Ptr<T> >::_retain(Ref<Referable>& reference) const noexcept
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

	template <class T>
	SLIB_INLINE void Atomic< Ptr<T> >::_replace(T* pointer, const Ref<Referable>& reference) noexcept
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

	template <class T>
	SLIB_INLINE void Atomic< Ptr<T> >::_move_init(void* _other) noexcept
	{
		AtomicPtr<T>& other = *(reinterpret_cast<AtomicPtr<T>*>(_other));
		_ref._move_init(&(other._ref));
	}

	template <class T>
	SLIB_INLINE void Atomic< Ptr<T> >::_move_assign(void* _other) noexcept
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


	template <class T>
	SLIB_INLINE PtrLocker<T>::PtrLocker(const Ptr<T>& ptr) noexcept
	 : m_ptr(ptr.lock())
	 {}

	template <class T>
	SLIB_INLINE PtrLocker<T>::PtrLocker(const AtomicPtr<T>& ptr) noexcept
	 : m_ptr(ptr.lock())
	 {}

	template <class T>
	SLIB_INLINE void PtrLocker<T>::unlock() noexcept
	{
		m_ptr.setNull();
	}

	template <class T>
	SLIB_INLINE T* PtrLocker<T>::get() noexcept
	{
		return m_ptr.ptr;
	}

	template <class T>
	SLIB_INLINE sl_bool PtrLocker<T>::isNull() noexcept
	{
		return m_ptr.isNull();
	}

	template <class T>
	SLIB_INLINE sl_bool PtrLocker<T>::isNotNull() noexcept
	{
		return m_ptr.isNotNull();
	}

	template <class T>
	SLIB_INLINE T& PtrLocker<T>::operator*() const noexcept
	{
		return *(m_ptr.ptr);
	}

	template <class T>
	SLIB_INLINE T* PtrLocker<T>::operator->() const noexcept
	{
		return m_ptr.ptr;
	}

	template <class T>
	SLIB_INLINE PtrLocker<T>::operator T*() const noexcept
	{
		return m_ptr.ptr;
	}

	template <class T>
	SLIB_INLINE PtrLocker<T>::operator sl_bool() const noexcept
	{
		return m_ptr.ptr;
	}


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
				SLIB_INLINE SharedPtrContainer(T* _ptr) : ptr(_ptr)
				{					
				}
				
				SLIB_INLINE ~SharedPtrContainer()
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
				SLIB_INLINE SharedPtrContainerWithDeleter(T* _ptr, const DELETER& _deleter): ptr(_ptr), deleter(_deleter)
				{
				}

				SLIB_INLINE ~SharedPtrContainerWithDeleter()
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
				SLIB_INLINE SharedPtrObjectContainer(ARGS&&... args): object(Forward<ARGS>(args)...)
				{
				}
				
			};
			
		}
	}

	template <class T>
	SLIB_INLINE Ptr<T> SharedPtr(T* _ptr)
	{
		if (_ptr) {
			return Ptr<T>(_ptr, ToRef(new priv::ptr::SharedPtrContainer<T>(_ptr)));
		} else {
			return sl_null;
		}
	}

	template <class T, class DELETER>
	SLIB_INLINE Ptr<T> SharedPtr(T* _ptr, const DELETER& deleter)
	{
		if (_ptr) {
			return Ptr<T>(_ptr, ToRef(new priv::ptr::SharedPtrContainerWithDeleter<T, DELETER>(_ptr, deleter)));
		} else {
			return sl_null;
		}
	}
		
	template<class T, class... ARGS>
	SLIB_INLINE Ptr<T> MakeShared(ARGS&&... args)
	{
		priv::ptr::SharedPtrObjectContainer<T>* ptr = new priv::ptr::SharedPtrObjectContainer<T>(Forward<ARGS>(args)...);
		if (ptr) {
			return Ptr<T>(&(ptr->object), ToRef(ptr));
		} else {
			return sl_null;
		}
	}

}
