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

#ifndef CHECKHEADER_SLIB_CORE_PTRX
#define CHECKHEADER_SLIB_CORE_PTRX

#include "definition.h"

#include "ptr.h"
#include "refx.h"

#define PRIV_SLIB_DEFINE_PTRX_COMMON_FUNCTIONS \
		SLIB_DEFINE_CLASS_DEFAULT_MEMBERS_INLINE(Ptr) \
	public: \
		using Ptr<T1>::ptr; \
		template <class... TYPES> \
		SLIB_INLINE Ptr(const Ptr<TYPES...>& v) noexcept : Ptr<T1>(v) { _init(v); } \
		template <class... TYPES> \
		SLIB_INLINE Ptr(Ptr<TYPES...>&& v) noexcept : Ptr<T1>(Move(v)) { _init(v); } \
		template <class T> \
		SLIB_INLINE Ptr(const AtomicPtr<T>& v) noexcept : Ptr(Ptr<T>(v)) {} \
		template <class... TYPES> \
		SLIB_INLINE Ptr(const Ref<TYPES...>& v) noexcept : Ptr<T1>(v) { _init(v); } \
		template <class T> \
		SLIB_INLINE Ptr(const AtomicRef<T>& v) noexcept : Ptr(Ref<T>(v)) {} \
		template <class T> \
		SLIB_INLINE Ptr(const WeakRef<T>& v) noexcept : Ptr<T1>(v) { _init((T*)ptr); } \
		template <class T> \
		SLIB_INLINE Ptr(const AtomicWeakRef<T>& v) noexcept : Ptr<T1>(v) { _init((T*)ptr); } \
		template <class T> \
		SLIB_INLINE Ptr(T* v) noexcept : Ptr<T1>(v) { _init(v); } \
		template <class... TYPES> \
		SLIB_INLINE Ptr(const Pointer<TYPES...>& v) noexcept : Ptr<T1>(v) { _init(v); } \
		static const Ptr null() noexcept { return sl_null; } \
		void setNull() noexcept { Ptr<T1>::setNull(); _init(sl_null); } \
		template <class... TYPES> \
		static const Ptr& from(const Ptr<TYPES...>& other) noexcept { return *(reinterpret_cast<Ptr const*>(&other)); } \
		template <class... TYPES> \
		static Ptr& from(Ptr<TYPES...>& other) noexcept { return *(reinterpret_cast<Ptr*>(&other)); } \
		template <class... TYPES> \
		static Ptr&& from(Ptr<TYPES...>&& other) noexcept { return static_cast<Ptr&&>(*(reinterpret_cast<Ptr*>(&other))); } \
		SLIB_INLINE void set(sl_null_t) noexcept { setNull(); } \
		template <class T> \
		SLIB_INLINE void set(T* v) noexcept { Ptr<T1>::set(v); _init(v); } \
		template <class... TYPES> \
		SLIB_INLINE void set(const Ptr<TYPES...>& v) noexcept { Ptr<T1>::set(v); _init(v); } \
		template <class... TYPES> \
		SLIB_INLINE void set(Ptr<TYPES...>&& v) noexcept { Ptr<T1>::set(Move(v)); _init(v); } \
		template <class T> \
		SLIB_INLINE void set(const AtomicPtr<T>& v) noexcept { set(Ptr<T>(v)); } \
		template <class... TYPES> \
		SLIB_INLINE void set(const Ref<TYPES...>& v) noexcept { Ptr<T1>::set(v); _init(v); } \
		template <class T> \
		SLIB_INLINE void set(const AtomicRef<T>& v) noexcept { set(Ref<T>(v)); } \
		template <class T> \
		SLIB_INLINE void set(const WeakRef<T>& v) noexcept { Ptr<T1>::set(v); _init((T*)ptr); } \
		template <class T> \
		SLIB_INLINE void set(const AtomicWeakRef<T>& v) noexcept { Ptr<T1>::set(v); _init((T*)ptr); } \
		template <class... TYPES> \
		SLIB_INLINE void set(const Pointer<TYPES...>& v) noexcept { Ptr<T1>::set(v); _init(v); } \
		template <class OTHER> \
		SLIB_INLINE Ptr& operator=(OTHER&& other) noexcept { set(Forward<OTHER>(other)); return *this; }

#define PRIV_SLIB_DEFINE_PTRX_LOCKER_COMMON_FUNCTIONS(...) \
	private: \
		Ptr<__VA_ARGS__> m_ptr; \
	public: \
		SLIB_INLINE PtrLocker(const Ptr<__VA_ARGS__>& ptr) noexcept: m_ptr(ptr.lock()) {} \
	public: \
		SLIB_INLINE void unlock() noexcept { m_ptr.setNull(); } \
		SLIB_INLINE sl_bool isNull() noexcept { return m_ptr.isNull(); } \
		SLIB_INLINE sl_bool isNotNull() noexcept { return m_ptr.isNotNull(); }

namespace slib
{

	template <class T1, class T2>
	class SLIB_EXPORT Ptr<T1, T2> : public Ptr<T1>
	{
		PRIV_SLIB_DEFINE_PTRX_COMMON_FUNCTIONS

	public:
		T2 * ptr2;

	public:
		constexpr Ptr() noexcept : ptr2(sl_null) {}

		constexpr Ptr(sl_null_t) noexcept : ptr2(sl_null) {}

		SLIB_INLINE Ptr(T1* v1, T2* v2) noexcept : Ptr<T1>(v1), ptr2(v2) {}

		template <class REF>
		SLIB_INLINE Ptr(T1* v1, T2* v2, REF&& r) noexcept : Ptr<T1>(v1, Forward<REF>(r)), ptr2(v2) {}

	public:
		SLIB_INLINE operator T1*() const noexcept
		{
			return ptr;
		}

		SLIB_INLINE operator T2*() const noexcept
		{
			return ptr2;
		}

	public:
		SLIB_INLINE Ptr lock() const noexcept
		{
			Ptr<T1> p = Ptr<T1>::lock();
			if (p.ptr) {
				return Ptr(p.ptr, ptr2, Move(p.ref));
			}
			return sl_null;
		}

	private:
		template <class T>
		SLIB_INLINE void _init(const T& p)
		{
			ptr2 = p;
		}

	};

	template <class T1, class T2, class T3>
	class SLIB_EXPORT Ptr<T1, T2, T3> : public Ptr<T1>
	{
		PRIV_SLIB_DEFINE_PTRX_COMMON_FUNCTIONS

	public:
		T2* ptr2;
		T3* ptr3;

	public:
		constexpr Ptr() noexcept : ptr2(sl_null), ptr3(sl_null) {}

		constexpr Ptr(sl_null_t) noexcept : ptr2(sl_null), ptr3(sl_null) {}

		SLIB_INLINE Ptr(T1* v1, T2* v2, T3* v3) noexcept : Ptr<T1>(v1), ptr2(v2), ptr3(v3) {}

		template <class REF>
		SLIB_INLINE Ptr(T1* v1, T2* v2, T3* v3, REF&& r) noexcept : Ptr<T1>(v1, Forward<REF>(r)), ptr2(v2), ptr3(v3) {}

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

	public:
		SLIB_INLINE Ptr lock() const noexcept
		{
			Ptr<T1> p = Ptr<T1>::lock();
			if (p.ptr) {
				return Ptr(p.ptr, ptr2, ptr3, Move(p.ref));
			}
			return sl_null;
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
	class SLIB_EXPORT Ptr<T1, T2, T3, T4> : public Ptr<T1>
	{
		PRIV_SLIB_DEFINE_PTRX_COMMON_FUNCTIONS

	public:
		T2* ptr2;
		T3* ptr3;
		T4* ptr4;

	public:
		constexpr Ptr() noexcept : ptr2(sl_null), ptr3(sl_null), ptr4(sl_null) {}

		constexpr Ptr(sl_null_t) noexcept : ptr2(sl_null), ptr3(sl_null), ptr4(sl_null) {}

		SLIB_INLINE Ptr(T1* v1, T2* v2, T3* v3, T4* v4) noexcept : Ptr<T1>(v1), ptr2(v2), ptr3(v3), ptr4(v4) {}

		template <class REF>
		SLIB_INLINE Ptr(T1* v1, T2* v2, T3* v3, T4* v4, REF&& r) noexcept : Ptr<T1>(v1, Forward<REF>(r)), ptr2(v2), ptr3(v3), ptr4(v4) {}

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

	public:
		SLIB_INLINE Ptr lock() const noexcept
		{
			Ptr<T1> p = Ptr<T1>::lock();
			if (p.ptr) {
				return Ptr(p.ptr, ptr2, ptr3, ptr4, Move(p.ref));
			}
			return sl_null;
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


	template <class T1, class T2>
	class SLIB_EXPORT PtrLocker<T1, T2>
	{
		PRIV_SLIB_DEFINE_PTRX_LOCKER_COMMON_FUNCTIONS(T1, T2)
		
	public:
		SLIB_INLINE operator T1*() const noexcept
		{
			return m_ptr.ptr;
		}

		SLIB_INLINE operator T2*() const noexcept
		{
			return m_ptr.ptr2;
		}

	};
	
	template <class T1, class T2, class T3>
	class SLIB_EXPORT PtrLocker<T1, T2, T3>
	{
		PRIV_SLIB_DEFINE_PTRX_LOCKER_COMMON_FUNCTIONS(T1, T2, T3)

	public:
		SLIB_INLINE operator T1*() const noexcept
		{
			return m_ptr.ptr;
		}

		SLIB_INLINE operator T2*() const noexcept
		{
			return m_ptr.ptr2;
		}

		SLIB_INLINE operator T3*() const noexcept
		{
			return m_ptr.ptr3;
		}

	};
	
	template <class T1, class T2, class T3, class T4>
	class SLIB_EXPORT PtrLocker<T1, T2, T3, T4>
	{
		PRIV_SLIB_DEFINE_PTRX_LOCKER_COMMON_FUNCTIONS(T1, T2, T3, T4)

	public:
		SLIB_INLINE operator T1*() const noexcept
		{
			return m_ptr.ptr;
		}

		SLIB_INLINE operator T2*() const noexcept
		{
			return m_ptr.ptr2;
		}

		SLIB_INLINE operator T3*() const noexcept
		{
			return m_ptr.ptr3;
		}
		
		SLIB_INLINE operator T4*() const noexcept
		{
			return m_ptr.ptr4;
		}
		
	};
	
}

#include "detail/ptrx.inc"

#endif