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

#include "default_members.h"

#define PRIV_SLIB_DEFINE_POINTER_COMMON_FUNCTIONS \
		SLIB_DEFINE_CLASS_DEFAULT_MEMBERS_INLINE(Pointer) \
	public: \
		template <class OTHER> \
		Pointer(const OTHER& v) noexcept { _init(v); } \
		Pointer& operator=(sl_null_t) noexcept { _init(sl_null); return *this; } \
		template <class OTHER> \
		Pointer& operator=(const OTHER& v) noexcept { _init(v); return *this; }

namespace slib
{

	template <class... TYPES>
	class Pointer;
	
	template <class T>
	class PointerxT {};

	template <class T1, class T2, class... TYPES>
	using Pointerx = Pointer< PointerxT<T1>, T2, TYPES... >;

	template <class T>
	class Pointer<T>
	{
		PRIV_SLIB_DEFINE_POINTER_COMMON_FUNCTIONS

	public:
		T* ptr;

	public:
		SLIB_CONSTEXPR Pointer(): ptr(sl_null) {}

		SLIB_CONSTEXPR Pointer(sl_null_t): ptr(sl_null) {}

		SLIB_CONSTEXPR Pointer(T* other): ptr(other) {}

	public:
		SLIB_CONSTEXPR operator T*() const
		{
			return ptr;
		}
		
		SLIB_CONSTEXPR T& operator*() const
		{
			return *ptr;
		}

		SLIB_CONSTEXPR T* operator->() const
		{
			return ptr;
		}

		SLIB_CONSTEXPR explicit operator sl_bool() const
		{
			return ptr != sl_null;
		}

	private:
		template <class OTHER>
		void _init(const OTHER& p) noexcept
		{
			ptr = p;
		}

	};

	template <class T, bool>
	class PointerxCastHelper;

	template <class T>
	class PointerxCastHelper<T, true>
	{
	public:
		template <class FROM>
		static T* cast(const FROM& from) noexcept
		{
			return from;
		}
	};

	template <class T>
	class PointerxCastHelper<T, false>
	{
	public:
		template <class FROM>
		static T* cast(const FROM& from) noexcept
		{
			return sl_null;
		}
	};

	template <class T>
	class PointerxHelper
	{
	public:
		typedef T FirstType;

		template <class TO, class FROM>
		static void init(TO*& to, const FROM& from) noexcept
		{
			to = from;
		}
	};

	template <class T>
	class PointerxHelper< PointerxT<T> >
	{
	public:
		typedef T FirstType;

		template <class TO, class FROM>
		static void init(TO*& to, const FROM& from) noexcept
		{
			to = PointerxCastHelper<TO, IsConvertible<FROM, TO*>::value>::cast(from);
		}
	};
	
	template <class T, class T2>
	class Pointer<T, T2>
	{
	public:
		typedef PointerxHelper<T> Helper;
		typedef typename Helper::FirstType T1;
		PRIV_SLIB_DEFINE_POINTER_COMMON_FUNCTIONS

	public:
		T1* ptr;
		T2* ptr2;

	public:
		SLIB_CONSTEXPR Pointer(): ptr(sl_null), ptr2(sl_null) {}

		SLIB_CONSTEXPR Pointer(sl_null_t): ptr(sl_null), ptr2(sl_null) {}

		SLIB_CONSTEXPR Pointer(T1* v1, T2* v2): ptr(v1), ptr2(v2) {}

	public:
		SLIB_CONSTEXPR operator T1*() const
		{
			return ptr;
		}

		SLIB_CONSTEXPR operator T2*() const
		{
			return ptr2;
		}

	private:
		template <class OTHER>
		void _init(const OTHER& p) noexcept
		{
			Helper::init(ptr, p);
			Helper::init(ptr2, p);
		}

	};

	template <class T, class T2, class T3>
	class Pointer<T, T2, T3>
	{
	public:
		typedef PointerxHelper<T> Helper;
		typedef typename Helper::FirstType T1;
		PRIV_SLIB_DEFINE_POINTER_COMMON_FUNCTIONS

	public:
		T1* ptr;
		T2* ptr2;
		T3* ptr3;

	public:
		SLIB_CONSTEXPR Pointer(): ptr(sl_null), ptr2(sl_null), ptr3(sl_null) {}

		SLIB_CONSTEXPR Pointer(sl_null_t): ptr(sl_null), ptr2(sl_null), ptr3(sl_null) {}

		SLIB_CONSTEXPR Pointer(T1* v1, T2* v2, T3* v3): ptr(v1), ptr2(v2), ptr3(v3) {}

	public:
		SLIB_CONSTEXPR operator T1*() const
		{
			return ptr;
		}

		SLIB_CONSTEXPR operator T2*() const
		{
			return ptr2;
		}

		SLIB_CONSTEXPR operator T3*() const
		{
			return ptr3;
		}

	private:
		template <class OTHER>
		void _init(const OTHER& p) noexcept
		{
			Helper::init(ptr, p);
			Helper::init(ptr2, p);
			Helper::init(ptr3, p);
		}
		
	};

	template <class T, class T2, class T3, class T4>
	class Pointer<T, T2, T3, T4>
	{
	public:
		typedef PointerxHelper<T> Helper;
		typedef typename Helper::FirstType T1;
		PRIV_SLIB_DEFINE_POINTER_COMMON_FUNCTIONS

	public:
		T1* ptr;
		T2* ptr2;
		T3* ptr3;
		T4* ptr4;

	public:
		SLIB_CONSTEXPR Pointer(): ptr2(sl_null), ptr3(sl_null), ptr4(sl_null) {}

		SLIB_CONSTEXPR Pointer(sl_null_t): ptr2(sl_null), ptr3(sl_null), ptr4(sl_null) {}

		SLIB_CONSTEXPR Pointer(T1* v1, T2* v2, T3* v3, T4* v4): ptr(v1), ptr2(v2), ptr3(v3), ptr4(v4) {}

	public:
		SLIB_CONSTEXPR operator T1*() const
		{
			return ptr;
		}

		SLIB_CONSTEXPR operator T2*() const
		{
			return ptr2;
		}

		SLIB_CONSTEXPR operator T3*() const
		{
			return ptr3;
		}

		SLIB_CONSTEXPR operator T4*() const
		{
			return ptr4;
		}

	private:
		template <class OTHER>
		void _init(const OTHER& p) noexcept
		{
			Helper::init(ptr, p);
			Helper::init(ptr2, p);
			Helper::init(ptr3, p);
			Helper::init(ptr4, p);
		}

	};

}

#endif