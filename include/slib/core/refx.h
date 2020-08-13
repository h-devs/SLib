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
		template <class... OTHERS> \
		SLIB_INLINE Ref(const Ref<OTHERS...>& v) noexcept : Ref<T1>(v) { _init(v); } \
		template <class... OTHERS> \
		SLIB_INLINE Ref(Ref<OTHERS...>&& v) noexcept { ptr = v; _init(v); v.ptr = sl_null; } \
		template <class OTHER> \
		SLIB_INLINE Ref(const AtomicRef<OTHER>& v) noexcept : Ref(Ref<OTHER>(v)) {} \
		template <class OTHER> \
		SLIB_INLINE Ref(const WeakRef<OTHER>& v) noexcept : Ref(Ref<OTHER>(v)) {} \
		template <class OTHER> \
		SLIB_INLINE Ref(const AtomicWeakRef<OTHER>& v) noexcept : Ref(Ref<OTHER>(v)) {} \
		template <class OTHER> \
		SLIB_INLINE Ref(OTHER* v) noexcept : Ref<T1>(v) { _init(v); } \
		template <class... OTHERS> \
		SLIB_INLINE Ref(const Pointer<OTHERS...>& v) noexcept : Ref<T1>(v) { _init(v); } \
		SLIB_INLINE static Ref null() noexcept { return sl_null; } \
		SLIB_INLINE void setNull() noexcept { Ref<T1>::setNull(); _init(sl_null); } \
		template <class... OTHERS> \
		SLIB_INLINE static const Ref& from(const Ref<OTHERS...>& other) noexcept { return *(reinterpret_cast<Ref const*>(&other)); } \
		template <class... OTHERS> \
		SLIB_INLINE static Ref& from(Ref<OTHERS...>& other) noexcept { return *(reinterpret_cast<Ref*>(&other)); } \
		template <class... OTHERS> \
		SLIB_INLINE static Ref&& from(Ref<OTHERS...>&& other) noexcept { return static_cast<Ref&&>(*(reinterpret_cast<Ref*>(&other))); } \
		SLIB_INLINE Ref& operator=(sl_null_t) noexcept { Ref<T1>::setNull(); _init(sl_null); return *this; } \
		template <class... OTHERS> \
		SLIB_INLINE Ref& operator=(const Ref<OTHERS...>& v) noexcept { *((Ref<T1>*)this) = v; _init(v); return *this; } \
		template <class... OTHERS> \
		SLIB_INLINE Ref& operator=(Ref<OTHERS...>&& v) noexcept { _init(v); *((Ref<T1>*)this) = Move(v); return *this; } \
		template <class OTHER> \
		SLIB_INLINE Ref& operator=(const AtomicRef<OTHER>& v) noexcept { return *this = Ref<OTHER>(v); } \
		template <class OTHER> \
		SLIB_INLINE Ref& operator=(const WeakRef<OTHER>& v) noexcept { return *this = Ref<OTHER>(v); } \
		template <class OTHER> \
		SLIB_INLINE Ref& operator=(const AtomicWeakRef<OTHER>& v) noexcept { return *this = Ref<OTHER>(v); } \
		template <class OTHER> \
		SLIB_INLINE Ref& operator=(OTHER* v) noexcept { *((Ref<T1>*)this) = v; _init(v); return *this; } \
		template <class... OTHERS> \
		SLIB_INLINE Ref& operator=(const Pointer<OTHERS...>& v) noexcept { *((Ref<T1>*)this) = v; _init(v); return *this; }


namespace slib
{

	template <class T1, class T2, class... TYPES>
	using Refx = Ref< PointerxT<T1>, T2, TYPES... >;

	template <class T, class T2>
	class SLIB_EXPORT Ref<T, T2> : public Ref<typename PointerxHelper<T>::FirstType>
	{
	public:
		typedef PointerxHelper<T> Helper;
		typedef typename Helper::FirstType T1;
		PRIV_SLIB_DEFINE_REFX_COMMON_FUNCTIONS

	public:
		T2* ptr2;

	public:
		constexpr Ref() noexcept : ptr2(sl_null) {}

		constexpr Ref(sl_null_t) noexcept : ptr2(sl_null) {}

		template <class OTHER>
		SLIB_INLINE Ref(OTHER&& v1, T2* v2) noexcept : Ref<T1>(Forward<OTHER>(v1)), ptr2(v2) {}

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
		template <class OTHER>
		SLIB_INLINE void _init(const OTHER& p)
		{
			Helper::init(ptr2, p);
		}

	};

	template <class T, class T2, class T3>
	class SLIB_EXPORT Ref<T, T2, T3> : public Ref<typename PointerxHelper<T>::FirstType>
	{
	public:
		typedef PointerxHelper<T> Helper;
		typedef typename Helper::FirstType T1;
		PRIV_SLIB_DEFINE_REFX_COMMON_FUNCTIONS

	public:
		T2* ptr2;
		T3* ptr3;

	public:
		constexpr Ref() noexcept : ptr2(sl_null), ptr3(sl_null) {}

		constexpr Ref(sl_null_t) noexcept : ptr2(sl_null), ptr3(sl_null) {}

		template <class OTHER>
		SLIB_INLINE Ref(OTHER&& v1, T2* v2, T3* v3) noexcept : Ref<T1>(Forward<OTHER>(v1)), ptr2(v2), ptr3(v3) {}

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
		template <class OTHER>
		SLIB_INLINE void _init(const OTHER& p)
		{
			Helper::init(ptr2, p);
			Helper::init(ptr3, p);
		}

	};

	template <class T, class T2, class T3, class T4>
	class SLIB_EXPORT Ref<T, T2, T3, T4> : public Ref<typename PointerxHelper<T>::FirstType>
	{
	public:
		typedef PointerxHelper<T> Helper;
		typedef typename Helper::FirstType T1;
		PRIV_SLIB_DEFINE_REFX_COMMON_FUNCTIONS

	public:
		T2* ptr2;
		T3* ptr3;
		T4* ptr4;

	public:
		constexpr Ref() noexcept : ptr2(sl_null), ptr3(sl_null), ptr4(sl_null) {}

		constexpr Ref(sl_null_t) noexcept : ptr2(sl_null), ptr3(sl_null), ptr4(sl_null) {}

		template <class OTHER>
		SLIB_INLINE Ref(OTHER&& v1, T2* v2, T3* v3, T4* v4) noexcept : Ref<T1>(Forward<OTHER>(v1)), ptr2(v2), ptr3(v3), ptr4(v4) {}

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
		template <class OTHER>
		SLIB_INLINE void _init(const OTHER& p)
		{
			Helper::init(ptr2, p);
			Helper::init(ptr3, p);
			Helper::init(ptr4, p);
		}

	};

}

#include "detail/refx.inc"

#endif