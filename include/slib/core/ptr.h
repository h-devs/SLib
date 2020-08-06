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

#ifndef CHECKHEADER_SLIB_CORE_PTR
#define CHECKHEADER_SLIB_CORE_PTR

#include "definition.h"

#include "ref.h"

namespace slib
{
	
	template <class... TYPES>
	class Ptr;
	
	template <class T>
	using AtomicPtr = Atomic< Ptr<T> >;
		
	template <class T>
	class SLIB_EXPORT Ptr<T>
	{
	public:
		T* ptr;
		Ref<Referable> ref;
	
	public:
		Ptr() noexcept;

		Ptr(Ptr<T>&& other) noexcept;
	
		Ptr(const Ptr<T>& other) noexcept;

		template <class O>
		Ptr(Ptr<O>&& other) noexcept;
	
		template <class O>
		Ptr(const Ptr<O>& other) noexcept;

		template <class O>
		Ptr(const AtomicPtr<O>& other) noexcept;

		Ptr(sl_null_t) noexcept;

		Ptr(T* pointer) noexcept;

		template <class O>
		Ptr(const Ref<O>& reference) noexcept;

		template <class O>
		Ptr(const AtomicRef<O>& reference) noexcept;

		template <class O>
		Ptr(const WeakRef<O>& weak) noexcept;

		template <class O>
		Ptr(const AtomicWeakRef<O>& weak) noexcept;

		template <class T1, class T2, class... TYPES>
		Ptr(const Ptr<T1, T2, TYPES...>& other) noexcept;

		template <class T1, class T2, class... TYPES>
		Ptr(Ptr<T1, T2, TYPES...>&& other) noexcept;

		template <class T1, class T2, class... TYPES>
		Ptr(const Ref<T1, T2, TYPES...>& other) noexcept;

		template <class... TYPES>
		Ptr(const Pointer<TYPES...>& other) noexcept;

		template <class REF>
		Ptr(T* pointer, REF&& r) noexcept;

		~Ptr();

	public:
		static const Ptr<T>& null() noexcept;
	
		sl_bool isNull() const noexcept;

		sl_bool isNotNull() const noexcept;

		void setNull() noexcept;

		template <class... TYPES>
		static const Ptr<T>& from(const Ptr<TYPES...>& other) noexcept;

		template <class... TYPES>
		static Ptr<T>& from(Ptr<TYPES...>& other) noexcept;

		template <class... TYPES>
		static Ptr<T>&& from(Ptr<TYPES...>&& other) noexcept;
	
		sl_bool isWeak() const noexcept;

		sl_bool lockRef(Ref<Referable>& ref) const noexcept;
		
		Ptr<T> lock() const noexcept;
		
		Ptr<T> toWeak() const noexcept;

	public:
		T* get() const noexcept;
		
		void set(T* pointer) noexcept;

		void set(sl_null_t) noexcept;

		template <class O>
		void set(Ptr<O>&& other) noexcept;
	
		template <class O>
		void set(const Ptr<O>& other) noexcept;

		template <class O>
		void set(const AtomicPtr<O>& other) noexcept;

		template <class O>
		void set(const Ref<O>& other) noexcept;

		template <class O>
		void set(const AtomicRef<O>& other) noexcept;

		template <class O>
		void set(const WeakRef<O>& other) noexcept;

		template <class O>
		void set(const AtomicWeakRef<O>& other) noexcept;

		template <class T1, class T2, class... TYPES>
		void set(const Ptr<T1, T2, TYPES...>& other) noexcept;

		template <class T1, class T2, class... TYPES>
		void set(Ptr<T1, T2, TYPES...>&& other) noexcept;

		template <class T1, class T2, class... TYPES>
		void set(const Ref<T1, T2, TYPES...>& other) noexcept;

		template <class... TYPES>
		void set(const Pointer<TYPES...>& other) noexcept;

		template <class REF>
		void set(T* pointer, REF&& r) noexcept;

	public:
		Ptr<T>& operator=(Ptr<T>&& other) noexcept;

		Ptr<T>& operator=(const Ptr<T>& other) noexcept;

		template <class OTHER>
		Ptr<T>& operator=(OTHER&& other) noexcept;


		sl_bool operator==(sl_null_t) const noexcept;

		sl_bool operator==(T* other) const noexcept;

		template <class O>
		sl_bool operator==(const Ptr<O>& other) const noexcept;
	
		template <class O>
		sl_bool operator==(const AtomicPtr<O>& other) const noexcept;

		sl_bool operator!=(sl_null_t) const noexcept;
	
		sl_bool operator!=(T* other) const noexcept;

		template <class O>
		sl_bool operator!=(const Ptr<O>& other) const noexcept;

		template <class O>
		sl_bool operator!=(const AtomicPtr<O>& other) const noexcept;


		T& operator*() const noexcept;

		T* operator->() const noexcept;

		operator T*() const noexcept;

		explicit operator sl_bool() const noexcept;

	public:
		void _move_init(void* _other) noexcept;

		void _move_assign(void* _other) noexcept;

		template <class... TYPES>
		static const Ref<Referable>& _getRef(const Ref<TYPES...>& ref) noexcept;

		template <class... TYPES>
		static Ref<Referable>&& _getRef(Ref<TYPES...>&& ref) noexcept;

		template <class O>
		static const AtomicRef<Referable>& _getRef(const AtomicRef<O>& ref) noexcept;

		template <class O>
		static const Ref<Referable>& _getRef(const WeakRef<O>& weak) noexcept;

		template <class O>
		static Ref<Referable>&& _getRef(WeakRef<O>&& weak) noexcept;

		template <class O>
		static const AtomicRef<Referable>& _getRef(const AtomicWeakRef<O>& weak) noexcept;

	};
	
	
	template <class T>
	class SLIB_EXPORT Atomic< Ptr<T> >
	{
	public:
		T* _ptr;
		Ref<Referable> _ref;
	
	public:
		Atomic() noexcept;

		Atomic(const AtomicPtr<T>& other) noexcept;

		template <class O>
		Atomic(const AtomicPtr<O>& other) noexcept;

		template <class O>
		Atomic(Ptr<O>&& other) noexcept;
	
		template <class O>
		Atomic(const Ptr<O>& other) noexcept;

		Atomic(sl_null_t) noexcept;

		Atomic(T* pointer) noexcept;

		template <class O>
		Atomic(const Ref<O>& reference) noexcept;

		template <class O>
		Atomic(const AtomicRef<O>& reference) noexcept;

		template <class O>
		Atomic(const WeakRef<O>& weak) noexcept;

		template <class O>
		Atomic(const AtomicWeakRef<O>& weak) noexcept;

		template <class T1, class T2, class... TYPES>
		Atomic(const Ptr<T1, T2, TYPES...>& other) noexcept;

		template <class T1, class T2, class... TYPES>
		Atomic(Ptr<T1, T2, TYPES...>&& other) noexcept;

		template <class T1, class T2, class... TYPES>
		Atomic(const Ref<T1, T2, TYPES...>& other) noexcept;

		template <class... TYPES>
		Atomic(const Pointer<TYPES...>& other) noexcept;

		template <class REF>
		Atomic(T* pointer, REF&& r) noexcept;

		~Atomic();

	public:
		static const AtomicPtr<T>& null() noexcept;

		sl_bool isNull() const noexcept;

		sl_bool isNotNull() const noexcept;

		void setNull() noexcept;

		template <class OTHER>
		static const AtomicPtr<T>& from(const AtomicPtr<OTHER>& other) noexcept;

		template <class OTHER>
		static AtomicPtr<T>& from(AtomicPtr<OTHER>& other) noexcept;

		Ptr<T> lock() const noexcept;

	public:
		void set(T* pointer) noexcept;

		void set(sl_null_t) noexcept;

		template <class O>
		void set(const AtomicPtr<O>& other) noexcept;

		template <class O>
		void set(Ptr<O>&& other) noexcept;
	
		template <class O>
		void set(const Ptr<O>& other) noexcept;

		template <class O>
		void set(const Ref<O>& other) noexcept;

		template <class O>
		void set(const AtomicRef<O>& other) noexcept;

		template <class O>
		void set(const WeakRef<O>& other) noexcept;

		template <class O>
		void set(const AtomicWeakRef<O>& other) noexcept;

		template <class T1, class T2, class... TYPES>
		void set(const Ptr<T1, T2, TYPES...>& other) noexcept;

		template <class T1, class T2, class... TYPES>
		void set(Ptr<T1, T2, TYPES...>&& other) noexcept;

		template <class T1, class T2, class... TYPES>
		void set(const Ref<T1, T2, TYPES...>& other) noexcept;

		template <class... TYPES>
		void set(const Pointer<TYPES...>& other) noexcept;

		template <class REF>
		void set(T* pointer, REF&& r) noexcept;

	public:
		AtomicPtr<T>& operator=(const AtomicPtr<T>& other) noexcept;

		template <class REF>
		AtomicPtr<T>& operator=(REF&& r) noexcept;


		sl_bool operator==(sl_null_t) const noexcept;

		sl_bool operator==(T* other) const noexcept;

		template <class O>
		sl_bool operator==(const AtomicPtr<O>& other) const noexcept;

		template <class O>
		sl_bool operator==(const Ptr<O>& other) const noexcept;

		sl_bool operator!=(sl_null_t) const noexcept;

		sl_bool operator!=(T* other) const noexcept;

		template <class O>
		sl_bool operator!=(const AtomicPtr<O>& other) const noexcept;

		template <class O>
		sl_bool operator!=(const Ptr<O>& other) const noexcept;

	public:
		T* _retain(Ref<Referable>& reference) const noexcept;

		void _replace(T* pointer, const Ref<Referable>& reference) noexcept;

		void _move_init(void* _other) noexcept;

		void _move_assign(void* _other) noexcept;
	
	private:
		SpinLock m_lock;
	
	};
	

	template <class... TYPES>
	class SLIB_EXPORT PtrLocker;

	template <class T>
	class SLIB_EXPORT PtrLocker<T>
	{
	private:
		Ptr<T> m_ptr;

	public:
		PtrLocker(const Ptr<T>& ptr) noexcept;

		PtrLocker(const AtomicPtr<T>& ptr) noexcept;

	public:
		void unlock() noexcept;
	
		T* get() noexcept;

		sl_bool isNull() noexcept;
	
		sl_bool isNotNull() noexcept;
	
		T& operator*() const noexcept;
	
		T* operator->() const noexcept;

		operator T*() const noexcept;

		explicit operator sl_bool() const noexcept;

	};
	
	template <class T>
	Ptr<T> SharedPtr(T* ptr);
	
	template <class T, class Deleter>
	Ptr<T> SharedPtr(T* ptr, const Deleter& deleter);

	template <class T, class... Args>
	Ptr<T> MakeShared(Args&&... args);

}

#include "detail/ptr.inc"

#endif
