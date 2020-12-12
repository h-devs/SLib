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
		using Ptr<T>::ptr; \
		using Ptr<T>::ref; \
		using Ptr<T>::lockRef; \
		template <class... OTHERS> \
		Ptr(const Ptr<OTHERS...>& v) noexcept : Ptr<T>(v) { _init(v); } \
		template <class... OTHER> \
		Ptr(Ptr<OTHER...>&& v) noexcept : Ptr<T>(Move(v)) { _init(v); } \
		template <class OTHER> \
		Ptr(const AtomicPtr<OTHER>& v) noexcept : Ptr(Ptr<OTHER>(v)) {} \
		template <class... OTHERS> \
		Ptr(const Ref<OTHERS...>& v) noexcept : Ptr<T>(v) { _init(v); } \
		template <class OTHER> \
		Ptr(const AtomicRef<OTHER>& v) noexcept : Ptr(Ref<OTHER>(v)) {} \
		template <class OTHER> \
		Ptr(const WeakRef<OTHER>& v) noexcept : Ptr(Ptr<OTHER>(v)) {} \
		template <class OTHER> \
		Ptr(const AtomicWeakRef<OTHER>& v) noexcept : Ptr(Ptr<OTHER>(v)) {} \
		template <class OTHER> \
		Ptr(OTHER* v) noexcept : Ptr<T>(v) { _init(v); } \
		template <class... OTHERS> \
		Ptr(const Pointer<OTHERS...>& v) noexcept : Ptr<T>(v) { _init(v); } \
		static const Ptr null() noexcept { return sl_null; } \
		void setNull() noexcept { Ptr<T>::setNull(); _init(sl_null); } \
		template <class... OTHERS> \
		static const Ptr& from(const Ptr<OTHERS...>& other) noexcept { return *(reinterpret_cast<Ptr const*>(&other)); } \
		template <class... OTHERS> \
		static Ptr& from(Ptr<OTHERS...>& other) noexcept { return *(reinterpret_cast<Ptr*>(&other)); } \
		template <class... OTHERS> \
		static Ptr&& from(Ptr<OTHERS...>&& other) noexcept { return static_cast<Ptr&&>(*(reinterpret_cast<Ptr*>(&other))); } \
		void set(sl_null_t) noexcept { setNull(); } \
		template <class OTHER> \
		void set(OTHER* v) noexcept { Ptr<T>::set(v); _init(v); } \
		template <class... OTHERS> \
		void set(const Ptr<OTHERS...>& v) noexcept { Ptr<T>::set(v); _init(v); } \
		template <class... OTHERS> \
		void set(Ptr<OTHERS...>&& v) noexcept { Ptr<T>::set(Move(v)); _init(v); } \
		template <class OTHER> \
		void set(const AtomicPtr<OTHER>& v) noexcept { set(Ptr<OTHER>(v)); } \
		template <class... OTHERS> \
		void set(const Ref<OTHERS...>& v) noexcept { Ptr<T>::set(v); _init(v); } \
		template <class OTHER> \
		void set(const AtomicRef<OTHER>& v) noexcept { set(Ref<OTHER>(v)); } \
		template <class OTHER> \
		void set(const WeakRef<OTHER>& v) noexcept { set(Ptr<OTHER>(v)); } \
		template <class OTHER> \
		void set(const AtomicWeakRef<OTHER>& v) noexcept { set(Ptr<OTHER>(v)); } \
		template <class... OTHERS> \
		void set(const Pointer<OTHERS...>& v) noexcept { Ptr<T>::set(v); _init(v); } \
		template <class OTHER> \
		Ptr& operator=(OTHER&& other) noexcept { set(Forward<OTHER>(other)); return *this; }

#define PRIV_SLIB_DEFINE_PTRX_LOCKER_COMMON_FUNCTIONS(...) \
	private: \
		Ptr<__VA_ARGS__> m_ptr; \
	public: \
		template <class... OTHERS> \
		PtrLocker(const Ptr<OTHERS...>& ptr) noexcept: m_ptr(ptr.lock()) {} \
	public: \
		void unlock() noexcept { m_ptr.setNull(); } \
		sl_bool isNull() noexcept { return m_ptr.isNull(); } \
		sl_bool isNotNull() noexcept { return m_ptr.isNotNull(); }

namespace slib
{

	template <class T, class... TYPES>
	using Ptrx = Ptr< PointerxT<T>, TYPES... >;

	template <class T, class... TYPES>
	using PtrxLocker = PtrLocker< PointerxT<T>, TYPES... >;

	template <class T>
	class SLIB_EXPORT Ptr< PointerxT<T> > : public Ptr<T>
	{
		SLIB_DEFINE_CLASS_DEFAULT_MEMBERS_INLINE(Ptr)

	public:
		using Ptr<T>::ptr;
		using Ptr<T>::ref;
		using Ptr<T>::lockRef;

		constexpr Ptr() noexcept {}

		constexpr Ptr(sl_null_t) noexcept {}

		template <class... OTHERS>
		Ptr(const Ptr<OTHERS...>& v) noexcept : Ptr<T>(_cast(v), v.ref) {}

		template <class... OTHERS>
		Ptr(Ptr<OTHERS...>&& v) noexcept : Ptr<T>(_cast(v), Move(v.ref)) {}

		template <class OTHER>
		Ptr(const AtomicPtr<OTHER>& v) noexcept : Ptr(Ptr<OTHER>(v)) {}

		template <class... OTHERS>
		Ptr(const Ref<OTHERS...>& v) noexcept : Ptr<T>(_cast(v), v) {}

		template <class OTHER>
		Ptr(const AtomicRef<OTHER>& v) noexcept : Ptr(Ref<OTHER>(v)) {}

		template <class OTHER>
		Ptr(const WeakRef<OTHER>& v) noexcept : Ptr(Ptr<OTHER>(v)) {}

		template <class OTHER>
		Ptr(const AtomicWeakRef<OTHER>& v) noexcept : Ptr(Ptr<OTHER>(v)) {}

		template <class OTHER>
		Ptr(OTHER* v) noexcept : Ptr<T>(_cast(v)) {}

		template <class... OTHERS>
		Ptr(const Pointer<OTHERS...>& v) noexcept : Ptr<T>(_cast(v)) {}

		template <class REF>
		Ptr(T* v1, REF&& r) noexcept : Ptr<T>(v1, Forward<REF>(r)) {}

	public:
		static const Ptr& null() noexcept
		{
			return *(reinterpret_cast<Ptr const*>(&(priv::ptr::g_null)));
		}
		
		template <class... OTHERS>
		static const Ptr& from(const Ptr<OTHERS...>& other) noexcept
		{
			return *(reinterpret_cast<Ptr const*>(&other));
		}

		template <class... OTHERS>
		static Ptr& from(Ptr<OTHERS...>& other) noexcept
		{
			return *(reinterpret_cast<Ptr*>(&other));
		}

		template <class... OTHERS>
		static Ptr&& from(Ptr<OTHERS...>&& other) noexcept
		{
			return static_cast<Ptr&&>(*(reinterpret_cast<Ptr*>(&other)));
		}
		
	public:
		template <class OTHER>
		void set(OTHER* v) noexcept
		{
			Ptr<T>::set(_cast(v));
		}

		template <class... OTHERS>
		void set(const Ptr<OTHERS...>& v) noexcept
		{
			Ptr<T>::set(_cast(v), v.ref);
		}

		template <class... OTHERS>
		void set(Ptr<OTHERS...>&& v) noexcept
		{
			Ptr<T>::set(_cast(v), Move(v.ref));
		}

		template <class OTHER>
		void set(const AtomicPtr<OTHER>& v) noexcept
		{
			set(Ptr<OTHER>(v));
		}

		template <class... OTHERS>
		void set(const Ref<OTHERS...>& v) noexcept
		{
			Ptr<T>::set(_cast(v), v);
		}

		template <class OTHER>
		void set(const AtomicRef<OTHER>& v) noexcept
		{
			set(Ref<OTHER>(v));
		}

		template <class OTHER>
		void set(const WeakRef<OTHER>& v) noexcept
		{
			set(Ptr<OTHER>(v));
		}

		template <class OTHER>
		void set(const AtomicWeakRef<OTHER>& v) noexcept
		{
			set(Ptr<OTHER>(v));
		}

		template <class... OTHERS>
		void set(const Pointer<OTHERS...>& v) noexcept
		{
			Ptr<T>::set(_cast(v));
		}

		template <class REF>
		void set(T* v1, REF&& r) noexcept
		{
			Ptr<T>::set(v1, Forward<REF>(r));
		}

	public:
		template <class OTHER>
		Ptr& operator=(OTHER&& other) noexcept
		{
			set(Forward<OTHER>(other));
			return *this;
		}

		operator T*() const noexcept
		{
			return ptr;
		}

		Ptr lock() const noexcept
		{
			Ref<Referable> r;
			if (lockRef(r)) {
				return Ptr(ptr, Move(r));
			}
			return sl_null;
		}

	private:
		template <class OTHER>
		T* _cast(const OTHER& other)
		{
			return PointerxCastHelper<T, IsConvertible<const OTHER&, T*>::value>::cast(other);
		}

	};

	template <class T, class T2>
	class SLIB_EXPORT Ptr<T, T2> : public Ptr<T>
	{
	public:
		typedef PointerxHelper<T> Helper;
		typedef typename Helper::FirstType T1;
		PRIV_SLIB_DEFINE_PTRX_COMMON_FUNCTIONS

	public:
		T2 * ptr2;

	public:
		constexpr Ptr() noexcept : ptr2(sl_null) {}

		constexpr Ptr(sl_null_t) noexcept : ptr2(sl_null) {}

		Ptr(T1* v1, T2* v2) noexcept : Ptr<T>(v1), ptr2(v2) {}

		template <class REF>
		Ptr(T1* v1, T2* v2, REF&& r) noexcept : Ptr<T>(v1, Forward<REF>(r)), ptr2(v2) {}

	public:
		operator T1*() const noexcept
		{
			return ptr;
		}

		operator T2*() const noexcept
		{
			return ptr2;
		}

	public:
		void set(T1* v1, T2* v2) noexcept
		{
			Ptr<T>::set(v1);
			ptr2 = v2;
		}

		template <class REF>
		void set(T1* v1, T2* v2, REF&& r) noexcept
		{
			Ptr<T>::set(v1, Forward<REF>(r));
			ptr2 = v2;
		}

		Ptr lock() const noexcept
		{
			Ref<Referable> r;
			if (lockRef(r)) {
				return Ptr(ptr, ptr2, Move(r));
			}
			return sl_null;
		}

	private:
		template <class OTHER>
		void _init(const OTHER& p)
		{
			Helper::init(ptr2, p);
		}

	};

	template <class T, class T2, class T3>
	class SLIB_EXPORT Ptr<T, T2, T3> : public Ptr<T>
	{
	public:
		typedef PointerxHelper<T> Helper;
		typedef typename Helper::FirstType T1;
		PRIV_SLIB_DEFINE_PTRX_COMMON_FUNCTIONS

	public:
		T2* ptr2;
		T3* ptr3;

	public:
		constexpr Ptr() noexcept : ptr2(sl_null), ptr3(sl_null) {}

		constexpr Ptr(sl_null_t) noexcept : ptr2(sl_null), ptr3(sl_null) {}

		Ptr(T1* v1, T2* v2, T3* v3) noexcept : Ptr<T>(v1), ptr2(v2), ptr3(v3) {}

		template <class REF>
		Ptr(T1* v1, T2* v2, T3* v3, REF&& r) noexcept : Ptr<T>(v1, Forward<REF>(r)), ptr2(v2), ptr3(v3) {}

	public:
		operator T1*() const noexcept
		{
			return ptr;
		}

		operator T2*() const noexcept
		{
			return ptr2;
		}

		operator T3*() const noexcept
		{
			return ptr3;
		}

	public:
		void set(T1* v1, T2* v2, T3* v3) noexcept
		{
			Ptr<T>::set(v1);
			ptr2 = v2;
			ptr3 = v3;
		}

		template <class REF>
		void set(T1* v1, T2* v2, T3* v3, REF&& r) noexcept
		{
			Ptr<T>::set(v1, Forward<REF>(r));
			ptr2 = v2;
			ptr3 = v3;
		}

		Ptr lock() const noexcept
		{
			Ref<Referable> r;
			if (lockRef(r)) {
				return Ptr(ptr, ptr2, ptr3, Move(r));
			}
			return sl_null;
		}

	private:
		template <class OTHER>
		void _init(const OTHER& p)
		{
			Helper::init(ptr2, p);
			Helper::init(ptr3, p);
		}

	};

	template <class T, class T2, class T3, class T4>
	class SLIB_EXPORT Ptr<T, T2, T3, T4> : public Ptr<T>
	{
	public:
		typedef PointerxHelper<T> Helper;
		typedef typename Helper::FirstType T1;
		PRIV_SLIB_DEFINE_PTRX_COMMON_FUNCTIONS

	public:
		T2* ptr2;
		T3* ptr3;
		T4* ptr4;

	public:
		constexpr Ptr() noexcept : ptr2(sl_null), ptr3(sl_null), ptr4(sl_null) {}

		constexpr Ptr(sl_null_t) noexcept : ptr2(sl_null), ptr3(sl_null), ptr4(sl_null) {}

		Ptr(T1* v1, T2* v2, T3* v3, T4* v4) noexcept : Ptr<T>(v1), ptr2(v2), ptr3(v3), ptr4(v4) {}

		template <class REF>
		Ptr(T1* v1, T2* v2, T3* v3, T4* v4, REF&& r) noexcept : Ptr<T>(v1, Forward<REF>(r)), ptr2(v2), ptr3(v3), ptr4(v4) {}

	public:
		operator T1*() const noexcept
		{
			return ptr;
		}

		operator T2*() const noexcept
		{
			return ptr2;
		}

		operator T3*() const noexcept
		{
			return ptr3;
		}

		operator T4*() const noexcept
		{
			return ptr4;
		}

	public:
		void set(T1* v1, T2* v2, T3* v3, T4* v4) noexcept
		{
			Ptr<T>::set(v1);
			ptr2 = v2;
			ptr3 = v3;
			ptr4 = v4;
		}

		template <class REF>
		void set(T1* v1, T2* v2, T3* v3, T4* v4, REF&& r) noexcept
		{
			Ptr<T>::set(v1, Forward<REF>(r));
			ptr2 = v2;
			ptr3 = v3;
			ptr4 = v4;
		}

		Ptr lock() const noexcept
		{
			Ref<Referable> r;
			if (lockRef(r)) {
				return Ptr(ptr, ptr2, ptr3, ptr4, Move(r));
			}
			return sl_null;
		}

	private:
		template <class OTHER>
		void _init(const OTHER& p)
		{
			Helper::init(ptr2, p);
			Helper::init(ptr3, p);
			Helper::init(ptr4, p);
		}

	};


	template <class T, class T2>
	class SLIB_EXPORT PtrLocker<T, T2>
	{
	public:
		typedef PointerxHelper<T> Helper;
		typedef typename Helper::FirstType T1;
		PRIV_SLIB_DEFINE_PTRX_LOCKER_COMMON_FUNCTIONS(T, T2)
		
	public:
		operator T1*() const noexcept
		{
			return m_ptr.ptr;
		}

		operator T2*() const noexcept
		{
			return m_ptr.ptr2;
		}

	};
	
	template <class T, class T2, class T3>
	class SLIB_EXPORT PtrLocker<T, T2, T3>
	{
	public:
		typedef PointerxHelper<T> Helper;
		typedef typename Helper::FirstType T1;
		PRIV_SLIB_DEFINE_PTRX_LOCKER_COMMON_FUNCTIONS(T, T2, T3)

	public:
		operator T1*() const noexcept
		{
			return m_ptr.ptr;
		}

		operator T2*() const noexcept
		{
			return m_ptr.ptr2;
		}

		operator T3*() const noexcept
		{
			return m_ptr.ptr3;
		}

	};
	
	template <class T, class T2, class T3, class T4>
	class SLIB_EXPORT PtrLocker<T, T2, T3, T4>
	{
	public:
		typedef PointerxHelper<T> Helper;
		typedef typename Helper::FirstType T1;
		PRIV_SLIB_DEFINE_PTRX_LOCKER_COMMON_FUNCTIONS(T, T2, T3, T4)

	public:
		operator T1*() const noexcept
		{
			return m_ptr.ptr;
		}

		operator T2*() const noexcept
		{
			return m_ptr.ptr2;
		}

		operator T3*() const noexcept
		{
			return m_ptr.ptr3;
		}
		
		operator T4*() const noexcept
		{
			return m_ptr.ptr4;
		}
		
	};

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

#endif