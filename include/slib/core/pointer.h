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

#ifndef CHECKHEADER_SLIB_CORE_POINTER
#define CHECKHEADER_SLIB_CORE_POINTER

#include "definition.h"

#include "macro.h"

#define PRIV_SLIB_DEFINE_POINTER_COMMON_FUNCTIONS \
		SLIB_DEFINE_CLASS_DEFAULT_MEMBERS_INLINE(Pointer) \
	public: \
		template <class TYPE> \
		SLIB_INLINE Pointer(const TYPE& v) noexcept { _init(v); } \
		SLIB_INLINE Pointer& operator=(sl_null_t) noexcept { _init(sl_null); return *this; } \
		template <class TYPE> \
		SLIB_INLINE Pointer& operator=(const TYPE& v) noexcept { _init(v); return *this; }

namespace slib
{

    template <class... TYPES>
	class Pointer;
	
	template <class T>
	class Pointer<T>
	{
		PRIV_SLIB_DEFINE_POINTER_COMMON_FUNCTIONS

	public:
		T* ptr;

	public:
        constexpr Pointer() noexcept: ptr(sl_null) {}

        constexpr Pointer(sl_null_t) noexcept: ptr(sl_null) {}

	public:
		SLIB_INLINE operator T*() const noexcept
		{
			return ptr;
		}
		
		SLIB_INLINE T& operator*() const noexcept
		{
			return *ptr;
		}

		SLIB_INLINE T* operator->() const noexcept
		{
			return ptr;
		}

		SLIB_INLINE explicit operator sl_bool() const noexcept
		{
			return ptr != sl_null;
		}

	private:
		template <class TYPE>
		SLIB_INLINE void _init(const TYPE& p)
		{
			ptr = p;
		}

	};

	template <class T1, class T2>
	class Pointer<T1, T2>
	{
		PRIV_SLIB_DEFINE_POINTER_COMMON_FUNCTIONS

	public:
		T1* ptr;
		T2* ptr2;

	public:
		constexpr Pointer() noexcept: ptr(sl_null), ptr2(sl_null) {}

		constexpr Pointer(sl_null_t) noexcept: ptr(sl_null), ptr2(sl_null) {}

		SLIB_INLINE Pointer(T1* v1, T2* v2) noexcept: ptr(v1), ptr2(v2) {}

	public:
		SLIB_INLINE operator T1*() const noexcept
		{
			return ptr;
		}

		SLIB_INLINE operator T2*() const noexcept
		{
			return ptr2;
		}

	private:
		template <class TYPE>
		SLIB_INLINE void _init(const TYPE& p)
		{
			ptr = p;
			ptr2 = p;
		}

	};

	template <class T1, class T2, class T3>
	class Pointer<T1, T2, T3>
	{
		PRIV_SLIB_DEFINE_POINTER_COMMON_FUNCTIONS

	public:
		T1* ptr;
		T2* ptr2;
		T3* ptr3;

	public:
        constexpr Pointer() noexcept: ptr(sl_null), ptr2(sl_null), ptr3(sl_null) {}

        constexpr Pointer(sl_null_t) noexcept: ptr(sl_null), ptr2(sl_null), ptr3(sl_null) {}

		SLIB_INLINE Pointer(T1* v1, T2* v2, T3* v3) noexcept: ptr(v1), ptr2(v2), ptr3(v3) {}

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

	private:
		template <class TYPE>
		SLIB_INLINE void _init(const TYPE& p)
		{
			ptr = p;
			ptr2 = p;
			ptr3 = p;
		}

	};

	template <class T1, class T2, class T3, class T4>
	class Pointer<T1, T2, T3, T4>
	{
		PRIV_SLIB_DEFINE_POINTER_COMMON_FUNCTIONS

	public:
		T1* ptr;
		T2* ptr2;
		T3* ptr3;
		T4* ptr4;

	public:
        constexpr Pointer() noexcept: ptr2(sl_null), ptr3(sl_null), ptr4(sl_null) {}

        constexpr Pointer(sl_null_t) noexcept: ptr2(sl_null), ptr3(sl_null), ptr4(sl_null) {}

		SLIB_INLINE constexpr Pointer(T1* v1, T2* v2, T3* v3, T4* v4) noexcept: ptr(v1), ptr2(v2), ptr3(v3), ptr4(v4) {}

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

	private:
		template <class TYPE>
		SLIB_INLINE void _init(const TYPE& p)
		{
			ptr = p;
			ptr2 = p;
			ptr3 = p;
			ptr4 = p;
		}

	};

}

#endif