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
	Ptr<T>::Ptr(const Ptr<T1, T2, TYPES...>& other) noexcept
	 : ref(other.ref), ptr(other)
	{
	}

	template <class T>
	template <class T1, class T2, class... TYPES>
	Ptr<T>::Ptr(Ptr<T1, T2, TYPES...>&& other) noexcept
	 : ref(Move(other.ref)), ptr(other)
	{
	}

	template <class T>
	template <class T1, class T2, class... TYPES>
	Ptr<T>::Ptr(const Ref<T1, T2, TYPES...>& other) noexcept
	 : ref(other), ptr(other)
	{
	}

	template <class T>
	template <class... TYPES>
	Ptr<T>::Ptr(const Pointer<TYPES...>& other) noexcept
	 : ptr(other)
	{
	}

	template <class T>
	template <class T1, class T2, class... TYPES>
	void Ptr<T>::set(const Ptr<T1, T2, TYPES...>& other) noexcept
	{
		ref = other.ref;
		ptr = other;
	}

	template <class T>
	template <class T1, class T2, class... TYPES>
	void Ptr<T>::set(Ptr<T1, T2, TYPES...>&& other) noexcept
	{
		ref = Move(other.ref);
		ptr = other;
	}
	
	template <class T>
	template <class T1, class T2, class... TYPES>
	void Ptr<T>::set(const Ref<T1, T2, TYPES...>& other) noexcept
	{
		ref = other;
		ptr = other;
	}

	template <class T>
	template <class... TYPES>
	void Ptr<T>::set(const Pointer<TYPES...>& other) noexcept
	{
		ptr = other;
		ref.setNull();
	}


	template <class T>
	template <class T1, class T2, class... TYPES>
	Atomic< Ptr<T> >::Atomic(const Ptr<T1, T2, TYPES...>& other) noexcept
	 : _ptr(other), _ref(other.ref)
	{
	}

	template <class T>
	template <class T1, class T2, class... TYPES>
	Atomic< Ptr<T> >::Atomic(Ptr<T1, T2, TYPES...>&& other) noexcept
	 : _ptr(other), _ref(Move(other.ref))
	{
	}

	template <class T>
	template <class T1, class T2, class... TYPES>
	Atomic< Ptr<T> >::Atomic(const Ref<T1, T2, TYPES...>& other) noexcept
	 : _ptr(other), _ref(other)
	{
	}

	template <class T>
	template <class... TYPES>
	Atomic< Ptr<T> >::Atomic(const Pointer<TYPES...>& other) noexcept
	 : _ptr(other)
	{
	}

	template <class T>
	template <class T1, class T2, class... TYPES>
	void Atomic< Ptr<T> >::set(const Ptr<T1, T2, TYPES...>& other) noexcept
	{
		_replace(other, other.ref);
	}
	
	template <class T>
	template <class T1, class T2, class... TYPES>
	void Atomic< Ptr<T> >::set(Ptr<T1, T2, TYPES...>&& other) noexcept
	{
		_ptr = other;
		_move_assign(&other);
	}
	
	template <class T>
	template <class T1, class T2, class... TYPES>
	void Atomic< Ptr<T> >::set(const Ref<T1, T2, TYPES...>& other) noexcept
	{
		_replace(other, Ref<Referable>::from(other));
	}
	
	template <class T>
	template <class... TYPES>
	void Atomic< Ptr<T> >::set(const Pointer<TYPES...>& other) noexcept
	{
		_replace(other, Ref<Referable>::null());
	}

}
