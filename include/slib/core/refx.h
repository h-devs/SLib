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

#ifndef CHECKHEADER_SLIB_CORE_REFX
#define CHECKHEADER_SLIB_CORE_REFX

#include "definition.h"

#include "ref.h"
#include "pointer.h"

namespace slib
{

    template <class... TYPES>
	class Refx;
	
	template <class T>
	class Refx<T> : public Ref<T>
	{
	public:
        SLIB_INLINE constexpr Refx() noexcept {}

		template <class T>
		SLIB_INLINE constexpr Refx(T&& v) noexcept: Ref<T>(Forward<T>(v)) {}

		SLIB_INLINE constexpr Refx(Refx const& other) noexcept = default;

		SLIB_INLINE constexpr Refx(Refx&& other) noexcept = default;

	public:
		SLIB_INLINE Refx& operator=(Refx const& other) noexcept = default;

		SLIB_INLINE Refx& operator=(Refx&& other) noexcept = default;

	public:
		SLIB_INLINE operator T*() const noexcept
		{
			return ptr;
		}

	};

	template <class T1, class T2>
	class Refx<T1, T2> : public Ref<T1>
	{
	public:
		T2* ptr2;

	public:
        SLIB_INLINE constexpr Refx() noexcept: ptr2(sl_null) {}

		template <class T>
		SLIB_INLINE constexpr Refx(T&& v1, T2* v2) noexcept: Ref<T1>(Forward<T>(v1)), ptr2(v2) {}

		template <class T>
		SLIB_INLINE constexpr Refx(const Ref<T>& v) noexcept: Ref<T1>(v), ptr2(v.ptr) {}

		template <class T>
		SLIB_INLINE constexpr Refx(Ref<T>&& v) noexcept: Ref<T1>(Move(v))
		{
			ptr2 = (T*)ptr;
		}

		template <class T>
		SLIB_INLINE constexpr Refx(const AtomicRef<T>& v) noexcept: Refx(Ref<T>(v))
	
		template <class T>
		SLIB_INLINE constexpr Refx(AtomicRef<T>&& v) noexcept: Ref<T1>(Move(v))
		{
			ptr2 = (T*)ptr;
		}

		template <class T>
		SLIB_INLINE constexpr Refx(T* v) noexcept: Ref<T1>(v), ptr2(v) {}

		SLIB_INLINE constexpr Refx(Refx const& other) noexcept = default;

		SLIB_INLINE constexpr Refx(Refx&& other) noexcept = default;

		template <class T, class... TYPES>
		SLIB_INLINE constexpr Refx(const Refx<T, TYPES...>& v) noexcept: Ref<T1>(*((Ref<T>*)&v)), ptr2(v) {}

		template <class... TYPES>
		SLIB_INLINE constexpr Refx(const Pointer<TYPES...>& v) noexcept: Ref<T1>((T1*)v), ptr2(v) {}

	public:
		SLIB_INLINE Refx& operator=(Refx const& other) noexcept = default;

		SLIB_INLINE Refx& operator=(Refx&& other) noexcept = default;

	public:
		SLIB_INLINE operator T1*() const noexcept
		{
			return ptr;
		}

		SLIB_INLINE operator T2*() const noexcept
		{
			return ptr2;
		}

	};

	template <class T1, class T2, class T3>
	class Refx<T1, T2, T3> : public Ref<T1>
	{
	public:
		T2* ptr2;
		T3* ptr3;

	public:
        SLIB_INLINE constexpr Refx() noexcept: ptr2(sl_null), ptr3(sl_null) {}

		template <class T>
		SLIB_INLINE constexpr Refx(T&& v1, T2* v2, T3* v3) noexcept: Ref<T1>(Forward<T>(v1)), ptr2(v2), ptr3(v3) {}

		template <class T>
		SLIB_INLINE constexpr Refx(const Ref<T>& v) noexcept: Ref<T1>(v), ptr2(v.ptr), ptr3(v.ptr) {}

		template <class T>
		SLIB_INLINE constexpr Refx(Ref<T>&& v) noexcept: Ref<T1>(Move(v))
		{
			T* _p = (T*)ptr;
			ptr2 = _p;
			ptr3 = _p;
		}

		template <class T>
		SLIB_INLINE constexpr Refx(const AtomicRef<T>& v) noexcept: Refx(Ref<T>(v)) {}

		template <class T>
		SLIB_INLINE constexpr Refx(AtomicRef<T>&& v) noexcept: Ref<T1>(Move(v))
		{
			T* _p = (T*)ptr;
			ptr2 = _p;
			ptr3 = _p;
		}

		template <class T>
		SLIB_INLINE constexpr Refx(T* v) noexcept: Ref<T1>(v), ptr2(v), ptr3(v) {}

		SLIB_INLINE constexpr Refx(Refx const& other) noexcept = default;

		SLIB_INLINE constexpr Refx(Refx&& other) noexcept = default;

		template <class T, class... TYPES>
		SLIB_INLINE constexpr Refx(const Refx<T, TYPES...>& v) noexcept: Ref<T1>(*((Ref<T>*)&v)), ptr2(v), ptr3(v) {}

		template <class... TYPES>
		SLIB_INLINE constexpr Refx(const Pointer<TYPES...>& v) noexcept: Ref<T1>((T1*)v), ptr2(v), ptr3(v) {}

	public:
		SLIB_INLINE Refx& operator=(Refx const& other) noexcept = default;

		SLIB_INLINE Refx& operator=(Refx&& other) noexcept = default;

	public:
		SLIB_INLINE operator T1*() const noexcept
		{
			return ptr;
		}

		SLIB_INLINE operator T2*() const noexcept
		{
			return ptr2;
		}

		SLIB_INLINE operator T3*() const noexcept
		{
			return ptr3;
		}

	};

	template <class T1, class T2, class T3, class T4>
	class Refx<T1, T2, T3, T4> : public Ref<T1>
	{
	public:
		T2* ptr2;
		T3* ptr3;
		T4* ptr4;

	public:
        SLIB_INLINE constexpr Refx() noexcept: ptr2(sl_null), ptr3(sl_null), ptr4(sl_null) {}

		template <class T>
		SLIB_INLINE constexpr Refx(T&& v1, T2* v2, T3* v3, T4* v4) noexcept: Ref<T1>(Forward<T>(v1)), ptr2(v2), ptr3(v3), ptr4(v4) {}

		template <class T>
		SLIB_INLINE constexpr Refx(const Ref<T>& v) noexcept: Ref<T1>(v), ptr2(v.ptr), ptr3(v.ptr), ptr4(v.ptr) {}

		template <class T>
		SLIB_INLINE constexpr Refx(Ref<T>&& v) noexcept: Ref<T1>(Move(v))
		{
			T* _p = (T*)ptr;
			ptr2 = _p;
			ptr3 = _p;
		}

		template <class T>
		SLIB_INLINE constexpr Refx(const AtomicRef<T>& v) noexcept: Refx(Ref<T>(v)) {}

		template <class T>
		SLIB_INLINE constexpr Refx(AtomicRef<T>&& v) noexcept: Ref<T1>(Move(v))
		{
			T* _p = (T*)ptr;
			ptr2 = _p;
			ptr3 = _p;
		}

		template <class T>
		SLIB_INLINE constexpr Refx(T* v) noexcept: Ref<T1>(v), ptr2(v), ptr3(v), ptr4(v) {}

		SLIB_INLINE constexpr Refx(Refx const& other) noexcept = default;

		SLIB_INLINE constexpr Refx(Refx&& other) noexcept = default;

		template <class T, class... TYPES>
		SLIB_INLINE constexpr Refx(const Refx<T, TYPES...>& v) noexcept: Ref<T1>(*((Ref<T>*)&v)), ptr2(v), ptr3(v), ptr4(v) {}

		template <class... TYPES>
		SLIB_INLINE constexpr Refx(const Pointer<TYPES...>& v) noexcept: Ref<T1>((T1*)v), ptr2(v), ptr3(v), ptr4(v) {}

	public:
		SLIB_INLINE Refx& operator=(Refx const& other) noexcept = default;

		SLIB_INLINE Refx& operator=(Refx&& other) noexcept = default;

	public:
		SLIB_INLINE operator T1*() const noexcept
		{
			return ptr;
		}

		SLIB_INLINE operator T2*() const noexcept
		{
			return ptr2;
		}

		SLIB_INLINE operator T3*() const noexcept
		{
			return ptr3;
		}

		SLIB_INLINE operator T4*() const noexcept
		{
			return ptr4;
		}

	};

}

#endif