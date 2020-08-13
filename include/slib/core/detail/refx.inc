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

namespace slib
{

	template <class T>
	template <class T1, class T2, class... TYPES>
	Ref<T>::Ref(const Ref<T1, T2, TYPES...>& other) noexcept
	{
		SLIB_TRY_CONVERT_TYPE(T1*, T*)
		T* o = other.ptr;
		if (o) {
			o->increaseReference();
		}
		ptr = o;
	}

	template <class T>
	template <class T1, class T2, class... TYPES>
	Ref<T>::Ref(Ref<T1, T2, TYPES...>&& other) noexcept
	{
		SLIB_TRY_CONVERT_TYPE(T1*, T*)
		_move_init(&other);
	}

	template <class T>
	template <class... TYPES>
	Ref<T>::Ref(const Pointer<TYPES...>& other) noexcept
	{
		T* o = other;
		if (o) {
			o->increaseReference();
		}
		ptr = o;
	}

	template <class T>
	template <class T1, class T2, class... TYPES>
	Ref<T>& Ref<T>::operator=(const Ref<T1, T2, TYPES...>& other) noexcept
	{
		SLIB_TRY_CONVERT_TYPE(T1*, T*)
		T* o = other.ptr;
		if (ptr != o) {
			if (o) {
				o->increaseReference();
			}
			_replaceObject(o);
		}
		return *this;
	}

	template <class T>
	template <class T1, class T2, class... TYPES>
	Ref<T>& Ref<T>::operator=(Ref<T1, T2, TYPES...>&& other) noexcept
	{
		SLIB_TRY_CONVERT_TYPE(T1*, T*)
		_move_assign(&other);
		return *this;
	}

	template <class T>
	template <class... TYPES>
	Ref<T>& Ref<T>::operator=(const Pointer<TYPES...>& other) noexcept
	{
		T* o = other;
		if (ptr != o) {
			if (o) {
				o->increaseReference();
			}
			_replaceObject(o);
		}
		return *this;
	}


	template <class T>
	template <class T1, class T2, class... TYPES>
	Atomic< Ref<T> >::Atomic(const Ref<T1, T2, TYPES...>& other) noexcept
	{
		SLIB_TRY_CONVERT_TYPE(T1*, T*)
		T* o = other.ptr;
		if (o) {
			o->increaseReference();
		}
		_ptr = o;
	}

	template <class T>
	template <class T1, class T2, class... TYPES>
	Atomic< Ref<T> >::Atomic(Ref<T1, T2, TYPES...>&& other) noexcept
	{
		SLIB_TRY_CONVERT_TYPE(T1*, T*)
		_move_init(&other);
	}

	template <class T>
	template <class... TYPES>
	Atomic< Ref<T> >::Atomic(const Pointer<TYPES...>& other) noexcept
	{
		T* o = other;
		if (o) {
			o->increaseReference();
		}
		_ptr = o;
	}

	template <class T>
	template <class T1, class T2, class... TYPES>
	Atomic< Ref<T> >& Atomic< Ref<T> >::operator=(const Ref<T1, T2, TYPES...>& other) noexcept
	{
		SLIB_TRY_CONVERT_TYPE(T1*, T*)
		T* o = other.ptr;
		if (_ptr != o) {
			if (o) {
				o->increaseReference();
			}
			_replaceObject(o);
		}
		return *this;
	}

	template <class T>
	template <class T1, class T2, class... TYPES>
	Atomic< Ref<T> >& Atomic< Ref<T> >::operator=(Ref<T1, T2, TYPES...>&& other) noexcept
	{
		SLIB_TRY_CONVERT_TYPE(T1*, T*)
		_move_assign(&other);
		return *this;
	}

	template <class T>
	template <class... TYPES>
	Atomic< Ref<T> >& Atomic< Ref<T> >::operator=(const Pointer<TYPES...>& other) noexcept
	{
		T* o = other;
		if (_ptr != o) {
			if (o) {
				o->increaseReference();
			}
			_replaceObject(o);
		}
		return *this;
	}


	template <class T>
	template <class T1, class T2, class... TYPES>
	WeakRef<T>::WeakRef(const Ref<T1, T2, TYPES...>& other) noexcept
	{
		SLIB_TRY_CONVERT_TYPE(T1*, T*)
		_set(other.ptr);
	}
	
	template <class T>
	template <class... TYPES>
	WeakRef<T>::WeakRef(const Pointer<TYPES...>& other) noexcept
	{
		_set(other);
	}
	
	template <class T>
	template <class T1, class T2, class... TYPES>
	WeakRef<T>& WeakRef<T>::operator=(const Ref<T1, T2, TYPES...>& other) noexcept
	{
		SLIB_TRY_CONVERT_TYPE(T1*, T*)
		_set(other.ptr);
		return *this;
	}
	
	template <class T>
	template <class... TYPES>
	WeakRef<T>& WeakRef<T>::operator=(const Pointer<TYPES...>& other) noexcept
	{
		_set(other);
		return *this;
	}


	template <class T>
	template <class T1, class T2, class... TYPES>
	Atomic< WeakRef<T> >::Atomic(const Ref<T1, T2, TYPES...>& other) noexcept
	{
		SLIB_TRY_CONVERT_TYPE(T1*, T*)
		_set(other.ptr);
	}
	
	template <class T>
	template <class... TYPES>
	Atomic< WeakRef<T> >::Atomic(const Pointer<TYPES...>& other) noexcept
	{
		_set(other);
	}
	
	template <class T>
	template <class T1, class T2, class... TYPES>
	AtomicWeakRef<T>& Atomic< WeakRef<T> >::operator=(const Ref<T1, T2, TYPES...>& other) noexcept
	{
		SLIB_TRY_CONVERT_TYPE(T1*, T*)
		_set(other.ptr);
		return *this;
	}
	
	template <class T>
	template <class... TYPES>
	AtomicWeakRef<T>& Atomic< WeakRef<T> >::operator=(const Pointer<TYPES...>& other) noexcept
	{
		_set(other);
		return *this;
	}
	
}
