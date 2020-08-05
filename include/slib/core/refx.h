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

#define PRIV_SLIB_DEFINE_REFX_COMMON_FUNCTIONS \
		SLIB_DEFINE_CLASS_DEFAULT_MEMBERS_INLINE(Ref) \
	public: \
		using Ref<T1>::ptr; \
		template <class... TYPES> \
		SLIB_INLINE Ref(const Ref<TYPES...>& v) noexcept : Ref<T1>(v) { _init(v); } \
		template <class... TYPES> \
		SLIB_INLINE Ref(Ref<TYPES...>&& v) noexcept { ptr = v; _init(v); v.ptr = sl_null; } \
		template <class T> \
		SLIB_INLINE Ref(const AtomicRef<T>& v) noexcept : Ref(Ref<T>(v)) {} \
		template <class T> \
		SLIB_INLINE Ref(const WeakRef<T>& v) noexcept : Ref<T1>(v) { _init((T*)ptr); } \
		template <class T> \
		SLIB_INLINE Ref(const AtomicWeakRef<T>& v) noexcept : Ref<T1>(v) { _init((T*)ptr); } \
		template <class T> \
		SLIB_INLINE Ref(T* v) noexcept : Ref<T1>(v) { _init(v); } \
		template <class _T1, class _T2, class... TYPES> \
		SLIB_INLINE Ref(const Ref<_T1, _T2, TYPES...>& v) noexcept : Ref<T1>(v.ptr) { _init(v); } \
		template <class... TYPES> \
		SLIB_INLINE Ref(const Pointer<TYPES...>& v) noexcept : Ref<T1>(static_cast<T1*>(v)) { _init(v); } \
		static Ref null() noexcept { return sl_null; } \
		void setNull() noexcept { Ref<T1>::setNull(); _init(sl_null); } \
		template <class... TYPES> \
		static const Ref& from(const Ref<TYPES...>& other) noexcept { return *(reinterpret_cast<Ref const*>(&other)); } \
		template <class... TYPES> \
		static Ref& from(Ref<TYPES...>& other) noexcept { return *(reinterpret_cast<Ref*>(&other)); } \
		template <class... TYPES> \
		static Ref&& from(Ref<TYPES...>&& other) noexcept { return static_cast<Ref&&>(*(reinterpret_cast<Ref*>(&other))); } \
		SLIB_INLINE Ref& operator=(sl_null_t) noexcept { Ref<T1>::setNull(); _init(sl_null); return *this; } \
		template <class T> \
		SLIB_INLINE Ref& operator=(const Ref<T>& v) noexcept { *((Ref<T1>*)this) = v; _init(v.ptr); return *this; } \
		template <class T> \
		SLIB_INLINE Ref& operator=(Ref<T>&& v) noexcept { *((Ref<T1>*)this) = Move(v); _init((T*)ptr); return *this; } \
		template <class T> \
		SLIB_INLINE Ref& operator=(const AtomicRef<T>& v) noexcept { *((Ref<T1>*)this) = v; _init((T*)ptr); return *this; } \
		template <class T> \
		SLIB_INLINE Ref& operator=(AtomicRef<T>&& v) noexcept { *((Ref<T1>*)this) = Move(v); _init((T*)ptr); return *this; } \
		template <class T> \
		SLIB_INLINE Ref& operator=(const WeakRef<T>& v) noexcept { *((Ref<T1>*)this) = v; _init((T*)ptr); return *this; } \
		template <class T> \
		SLIB_INLINE Ref& operator=(const AtomicWeakRef<T>& v) noexcept { *((Ref<T1>*)this) = v; _init((T*)ptr); return *this; } \
		template <class T> \
		SLIB_INLINE Ref& operator=(T* v) noexcept { *((Ref<T1>*)this) = v; _init(v); return *this; } \
		template <class _T1, class _T2, class... TYPES> \
		SLIB_INLINE Ref& operator=(const Ref<_T1, _T2, TYPES...>& v) noexcept { *((Ref<T1>*)this) = v.ptr; _init(v); return *this; } \
		template <class... TYPES> \
		SLIB_INLINE Ref& operator=(const Pointer<TYPES...>& v) noexcept { *((Ref<T1>*)this) = static_cast<T1*>(v); _init(v); return *this; }


namespace slib
{

	template <class T1, class T2>
	class SLIB_EXPORT Ref<T1, T2> : public Ref<T1>
	{
		PRIV_SLIB_DEFINE_REFX_COMMON_FUNCTIONS

	public:
		T2* ptr2;

	public:
		constexpr Ref() noexcept : ptr2(sl_null) {}

		constexpr Ref(sl_null_t) noexcept : ptr2(sl_null) {}

		template <class T>
		SLIB_INLINE Ref(T&& v1, T2* v2) noexcept : Ref<T1>(Forward<T>(v1)), ptr2(v2) {}

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
		template <class T>
		SLIB_INLINE void _init(const T& p)
		{
			ptr2 = p;
		}

	};

	template <class T1, class T2, class T3>
	class SLIB_EXPORT Ref<T1, T2, T3> : public Ref<T1>
	{
		PRIV_SLIB_DEFINE_REFX_COMMON_FUNCTIONS

	public:
		T2* ptr2;
		T3* ptr3;

	public:
		constexpr Ref() noexcept : ptr2(sl_null), ptr3(sl_null) {}

		constexpr Ref(sl_null_t) noexcept : ptr2(sl_null), ptr3(sl_null) {}

		template <class T>
		SLIB_INLINE Ref(T&& v1, T2* v2, T3* v3) noexcept : Ref<T1>(Forward<T>(v1)), ptr2(v2), ptr3(v3) {}

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
		template <class T>
		SLIB_INLINE void _init(const T& p)
		{
			ptr2 = p;
			ptr3 = p;
		}

	};

	template <class T1, class T2, class T3, class T4>
	class SLIB_EXPORT Ref<T1, T2, T3, T4> : public Ref<T1>
	{
		PRIV_SLIB_DEFINE_REFX_COMMON_FUNCTIONS

	public:
		T2* ptr2;
		T3* ptr3;
		T4* ptr4;

	public:
		constexpr Ref() noexcept : ptr2(sl_null), ptr3(sl_null), ptr4(sl_null) {}

		constexpr Ref(sl_null_t) noexcept : ptr2(sl_null), ptr3(sl_null), ptr4(sl_null) {}

		template <class T>
		SLIB_INLINE Ref(T&& v1, T2* v2, T3* v3, T4* v4) noexcept : Ref<T1>(Forward<T>(v1)), ptr2(v2), ptr3(v3), ptr4(v4) {}

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
		template <class T>
		SLIB_INLINE void _init(const T& p)
		{
			ptr2 = p;
			ptr3 = p;
			ptr4 = p;
		}

	};

}

#include "detail/refx.inc"

#endif