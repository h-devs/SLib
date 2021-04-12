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

#ifndef CHECKHEADER_SLIB_CORE_REF
#define CHECKHEADER_SLIB_CORE_REF

#include "atomic.h"
#include "convert.h"

#define SLIB_DECLARE_OBJECT \
public: \
	static sl_object_type ObjectType() noexcept; \
	static sl_bool isDerivedFrom(sl_object_type type) noexcept; \
	sl_object_type getObjectType() const noexcept override; \
	sl_bool isInstanceOf(sl_object_type type) const noexcept override;

#define SLIB_DEFINE_VARIABLE_OBJECT_TYPE(CLASS) \
	namespace slib_def { namespace obj_##CLASS { char g_objectId[1]; } } \
	sl_object_type CLASS::ObjectType() noexcept { return slib_def::obj_##CLASS::g_objectId; } \

#define SLIB_DEFINE_ROOT_OBJECT_BY_VARIABLE_TYPE(CLASS) \
	SLIB_DEFINE_VARIABLE_OBJECT_TYPE(CLASS) \
	sl_bool CLASS::isDerivedFrom(sl_object_type type) noexcept { return type == slib_def::obj_##CLASS::g_objectId; } \
	sl_object_type CLASS::getObjectType() const noexcept { return slib_def::obj_##CLASS::g_objectId; } \
	sl_bool CLASS::isInstanceOf(sl_object_type type) const noexcept { return type == slib_def::obj_##CLASS::g_objectId; }

#define SLIB_DEFINE_OBJECT_BY_VARIABLE_TYPE(CLASS, BASE) \
	SLIB_DEFINE_VARIABLE_OBJECT_TYPE(CLASS) \
	sl_bool CLASS::isDerivedFrom(sl_object_type type) noexcept { if (type == slib_def::obj_##CLASS::g_objectId) return sl_true; return BASE::isDerivedFrom(type); } \
	sl_object_type CLASS::getObjectType() const noexcept { return slib_def::obj_##CLASS::g_objectId; } \
	sl_bool CLASS::isInstanceOf(sl_object_type type) const noexcept { if (type == slib_def::obj_##CLASS::g_objectId) return sl_true; return BASE::isDerivedFrom(type); }

#define SLIB_DEFINE_ROOT_OBJECT_BY_CONSTANT_TYPE(CLASS) \
	sl_object_type CLASS::ObjectType() noexcept { return (sl_object_type)(sl_size)(object_types::CLASS); } \
	sl_bool CLASS::isDerivedFrom(sl_object_type type) noexcept { return type == (sl_object_type)(sl_size)(object_types::CLASS); } \
	sl_object_type CLASS::getObjectType() const noexcept { return (sl_object_type)(sl_size)(object_types::CLASS); } \
	sl_bool CLASS::isInstanceOf(sl_object_type type) const noexcept { return type == (sl_object_type)(sl_size)(object_types::CLASS); }

#define SLIB_DEFINE_OBJECT_BY_CONSTANT_TYPE(CLASS, BASE) \
	sl_object_type CLASS::ObjectType() noexcept { return (sl_object_type)(sl_size)(object_types::CLASS); } \
	sl_bool CLASS::isDerivedFrom(sl_object_type type) noexcept { if (type == (sl_object_type)(sl_size)(object_types::CLASS)) return sl_true; return BASE::isDerivedFrom(type); } \
	sl_object_type CLASS::getObjectType() const noexcept { return (sl_object_type)(sl_size)(object_types::CLASS); } \
	sl_bool CLASS::isInstanceOf(sl_object_type type) const noexcept { if (type == (sl_object_type)(sl_size)(object_types::CLASS)) return sl_true; return BASE::isDerivedFrom(type); }

#ifdef SLIB_USE_OBJECT_TYPE_CONSTANTS
#	define SLIB_DEFINE_ROOT_OBJECT(CLASS) SLIB_DEFINE_ROOT_OBJECT_BY_CONSTANT_TYPE(CLASS)
#	define SLIB_DEFINE_OBJECT(CLASS, BASE) SLIB_DEFINE_OBJECT_BY_CONSTANT_TYPE(CLASS, BASE)
#else
#	define SLIB_DEFINE_ROOT_OBJECT(CLASS) SLIB_DEFINE_ROOT_OBJECT_BY_VARIABLE_TYPE(CLASS)
#	define SLIB_DEFINE_OBJECT(CLASS, BASE) SLIB_DEFINE_OBJECT_BY_VARIABLE_TYPE(CLASS, BASE)
#endif

#define SLIB_REF_WRAPPER_NO_OP(WRAPPER, ...) \
public: \
	static sl_object_type ObjectType() noexcept { return __VA_ARGS__::ObjectType(); } \
	WRAPPER() noexcept {} \
	WRAPPER(sl_null_t) noexcept {} \
	WRAPPER(__VA_ARGS__* obj) noexcept : ref(obj) {} \
	WRAPPER(const WRAPPER& other) noexcept : ref(other.ref) {} \
	WRAPPER(WRAPPER& other) noexcept : ref(other.ref) {} \
	WRAPPER(const WRAPPER&& other) noexcept : ref(Move(other.ref)) {} \
	WRAPPER(WRAPPER&& other) noexcept : ref(Move(other.ref)) {} \
	WRAPPER(const Atomic<WRAPPER>& other) noexcept : ref(*(reinterpret_cast<const AtomicRef<__VA_ARGS__>*>(&other))) {} \
	WRAPPER(Atomic<WRAPPER>& other) noexcept : ref(*(reinterpret_cast<const AtomicRef<__VA_ARGS__>*>(&other))) {} \
	WRAPPER(const Atomic<WRAPPER>&& other) noexcept : ref(Move(*(reinterpret_cast<const AtomicRef<__VA_ARGS__>*>(&other)))) {} \
	WRAPPER(Atomic<WRAPPER>&& other) noexcept : ref(Move(*(reinterpret_cast<AtomicRef<__VA_ARGS__>*>(&other)))) {} \
	static const WRAPPER& null() noexcept { return *(reinterpret_cast<WRAPPER const*>(&(priv::ref::g_null))); } \
	sl_bool isNull() const noexcept { return ref.isNull(); } \
	sl_bool isNotNull() const noexcept { return ref.isNotNull(); } \
	void setNull() noexcept { ref.setNull(); } \
	WRAPPER& operator=(sl_null_t) noexcept { ref.setNull(); return *this; } \
	WRAPPER& operator=(__VA_ARGS__* obj) noexcept { ref = obj; return *this; } \
	WRAPPER& operator=(const WRAPPER& other) noexcept { ref = other.ref; return *this; } \
	WRAPPER& operator=(WRAPPER& other) noexcept { ref = other.ref; return *this; } \
	WRAPPER& operator=(const WRAPPER&& other) noexcept { ref = Move(other.ref); return *this; } \
	WRAPPER& operator=(WRAPPER&& other) noexcept { ref = Move(other.ref); return *this; } \
	WRAPPER& operator=(const Atomic<WRAPPER>& other) noexcept { ref = *(reinterpret_cast<const AtomicRef<__VA_ARGS__>*>(&other)); return *this; } \
	WRAPPER& operator=(Atomic<WRAPPER>& other) noexcept { ref = *(reinterpret_cast<const AtomicRef<__VA_ARGS__>*>(&other)); return *this; } \
	WRAPPER& operator=(const Atomic<WRAPPER>&& other) noexcept { ref = Move(*(reinterpret_cast<const AtomicRef<__VA_ARGS__>*>(&other))); return *this; } \
	WRAPPER& operator=(Atomic<WRAPPER>&& other) noexcept { ref = Move(*(reinterpret_cast<AtomicRef<__VA_ARGS__>*>(&other))); return *this; }

#define SLIB_REF_WRAPPER(WRAPPER, ...) \
	SLIB_REF_WRAPPER_NO_OP(WRAPPER, __VA_ARGS__) \
	sl_bool operator==(const WRAPPER& other) const noexcept { return ref.ptr == other.ref.ptr; } \
	sl_bool operator==(const Atomic<WRAPPER>& other) const noexcept { return ref.ptr == (reinterpret_cast<const AtomicRef<__VA_ARGS__>*>(&other))->_ptr; } \
	sl_bool operator!=(const WRAPPER& other) const noexcept { return ref.ptr != other.ref.ptr; } \
	sl_bool operator!=(const Atomic<WRAPPER>& other) const noexcept { return ref.ptr != (reinterpret_cast<const AtomicRef<__VA_ARGS__>*>(&other))->_ptr; } \
	explicit operator sl_bool() const noexcept { return ref.ptr != sl_null; }

#define SLIB_ATOMIC_REF_WRAPPER_NO_OP(...) \
public: \
	Atomic() noexcept {} \
	Atomic(sl_null_t) noexcept {} \
	Atomic(__VA_ARGS__* obj) noexcept : ref(obj) {} \
	Atomic(typename RemoveAtomic<Atomic>::Type const& other) noexcept : ref(*(reinterpret_cast<const Ref<__VA_ARGS__>*>(&other))) {} \
	Atomic(typename RemoveAtomic<Atomic>::Type& other) noexcept : ref(*(reinterpret_cast<const Ref<__VA_ARGS__>*>(&other))) {} \
	Atomic(typename RemoveAtomic<Atomic>::Type const&& other) noexcept : ref(Move(*(reinterpret_cast<const Ref<__VA_ARGS__>*>(&other)))) {} \
	Atomic(typename RemoveAtomic<Atomic>::Type&& other) noexcept : ref(Move(*(reinterpret_cast<Ref<__VA_ARGS__>*>(&other)))) {} \
	Atomic(const Atomic& other) noexcept : ref(other.ref) {} \
	Atomic(Atomic& other) noexcept : ref(other.ref) {} \
	Atomic(const Atomic&& other) noexcept : ref(Move(other.ref)) {} \
	Atomic(Atomic&& other) noexcept : ref(Move(other.ref)) {} \
	sl_bool isNull() const noexcept { return ref.isNull(); } \
	sl_bool isNotNull() const noexcept { return ref.isNotNull(); } \
	void setNull() noexcept { ref.setNull(); } \
	Atomic& operator=(sl_null_t) noexcept { ref.setNull(); return *this; } \
	Atomic& operator=(__VA_ARGS__* obj) noexcept { ref = obj; return *this; } \
	Atomic& operator=(typename RemoveAtomic<Atomic>::Type const& other) noexcept { ref = *(reinterpret_cast<const Ref<__VA_ARGS__>*>(&other)); return *this; } \
	Atomic& operator=(typename RemoveAtomic<Atomic>::Type& other) noexcept { ref = *(reinterpret_cast<const Ref<__VA_ARGS__>*>(&other)); return *this; } \
	Atomic& operator=(typename RemoveAtomic<Atomic>::Type const&& other) noexcept { ref = Move(*(reinterpret_cast<const Ref<__VA_ARGS__>*>(&other))); return *this; } \
	Atomic& operator=(typename RemoveAtomic<Atomic>::Type&& other) noexcept { ref = Move(*(reinterpret_cast<Ref<__VA_ARGS__>*>(&other))); return *this; } \
	Atomic& operator=(const Atomic& other) noexcept { ref = other.ref; return *this; } \
	Atomic& operator=(Atomic& other) noexcept { ref = other.ref; return *this; } \
	Atomic& operator=(Atomic const&& other) noexcept { ref = Move(other.ref); return *this; } \
	Atomic& operator=(Atomic&& other) noexcept { ref = Move(other.ref); return *this; }

#define SLIB_ATOMIC_REF_WRAPPER(...) \
	SLIB_ATOMIC_REF_WRAPPER_NO_OP(__VA_ARGS__) \
	sl_bool operator==(typename RemoveAtomic<Atomic>::Type const& other) const noexcept { return ref._ptr == (reinterpret_cast<const Ref<__VA_ARGS__>*>(&other))->ptr; } \
	sl_bool operator==(const Atomic& other) const noexcept { return ref._ptr == other.ref._ptr; } \
	sl_bool operator!=(typename RemoveAtomic<Atomic>::Type const& other) const noexcept { return ref._ptr != (reinterpret_cast<const Ref<__VA_ARGS__>*>(&other))->ptr; } \
	sl_bool operator!=(const Atomic& other) const noexcept { return ref._ptr != other.ref._ptr; } \
	explicit operator sl_bool() const noexcept { return ref._ptr != sl_null; }

typedef void* sl_object_type;

namespace slib
{
	namespace priv
	{
		namespace ref
		{

			extern void* const g_null;

		}
	}

	class CWeakRef;

	template <class... TYPES>
	class Ref;

	template <class T>
	class WeakRef;

	template <class T>
	using AtomicRef = Atomic< Ref<T> >;

	template <class T>
	using AtomicWeakRef = Atomic< WeakRef<T> >;

	template <class... TYPES>
	class Pointer;

	class String;
	class StringBuffer;

	class SLIB_EXPORT Referable
	{
	public:
		Referable() noexcept;

		Referable(const Referable& other) noexcept;
		
		Referable(Referable&& other) noexcept;

		virtual ~Referable();

	public:
		sl_reg increaseReference() noexcept;

		sl_reg decreaseReference() noexcept;

		sl_reg decreaseReferenceNoFree() noexcept;
		
		sl_reg getReferenceCount() noexcept;
		
	protected:
		virtual void init();

	public:
		static sl_object_type ObjectType() noexcept;
		
		static sl_bool isDerivedFrom(sl_object_type type) noexcept;
		
		virtual sl_object_type getObjectType() const noexcept;

		virtual sl_bool isInstanceOf(sl_object_type type) const noexcept;

	public:
		virtual String toString();

		virtual sl_bool toJsonString(StringBuffer& buf);

	private:
		void _clearWeak() noexcept;

	public:
		sl_bool _isWeakRef() const noexcept;

		CWeakRef* _getWeakObject() noexcept;

		void _free() noexcept;
		
	public:
		Referable& operator=(const Referable& other);
		
		Referable& operator=(Referable&& other);

	private:
		sl_reg m_nRefCount;
		CWeakRef* m_weak;

		friend class CWeakRef;
		
	};


	template <class T>
	class SLIB_EXPORT Ref<T>
	{
	public:
		constexpr Ref() noexcept: ptr(sl_null) {}

		constexpr Ref(sl_null_t) noexcept: ptr(sl_null) {}

		Ref(T* other) noexcept
		{
			if (other) {
				other->increaseReference();
			}
			ptr = other;
		}

		Ref(Ref&& other) noexcept
		{
			ptr = other.ptr;
			other.ptr = sl_null;
		}

		Ref(const Ref& other) noexcept
		{
			T* o = other.ptr;
			if (o) {
				o->increaseReference();
			}
			ptr = o;
		}

		template <class OTHER>
		Ref(Ref<OTHER>&& other) noexcept
		{
			ptr = other.ptr;
			other.ptr = sl_null;
		}

		template <class OTHER>
		Ref(const Ref<OTHER>& other) noexcept
		{
			SLIB_TRY_CONVERT_TYPE(OTHER*, T*)
			T* o = other.ptr;
			if (o) {
				o->increaseReference();
			}
			ptr = o;
		}

		template <class OTHER>
		Ref(const AtomicRef<OTHER>& other) noexcept
		{
			SLIB_TRY_CONVERT_TYPE(OTHER*, T*)
			T* o = other._retainObject();
			ptr = o;
		}

		template <class OTHER>
		Ref(const WeakRef<OTHER>& _other) noexcept
		{
			SLIB_TRY_CONVERT_TYPE(OTHER*, T*)
			Ref<OTHER> other(_other.lock());
			T* o = other.ptr;
			if (o) {
				o->increaseReference();
			}
			ptr = o;
		}

		template <class OTHER>
		Ref(const AtomicWeakRef<OTHER>& _other) noexcept
		{
			SLIB_TRY_CONVERT_TYPE(OTHER*, T*)
			Ref<OTHER> other(_other.lock());
			T* o = other.ptr;
			if (o) {
				o->increaseReference();
			}
			ptr = o;
		}

		template <class T1, class T2, class... TYPES>
		Ref(const Ref<T1, T2, TYPES...>& other) noexcept;

		template <class T1, class T2, class... TYPES>
		Ref(Ref<T1, T2, TYPES...>&& other) noexcept;

		template <class... TYPES>
		Ref(const Pointer<TYPES...>& other) noexcept;

		~Ref() noexcept
		{
			SLIB_TRY_CONVERT_TYPE(T*, Referable*)
			if (ptr) {
				ptr->decreaseReference();
				ptr = sl_null;
			}
		}
	
	public:
		static const Ref& null() noexcept
		{
			return *(reinterpret_cast<Ref const*>(&(priv::ref::g_null)));
		}

		sl_bool isNull() const noexcept
		{
			return ptr == sl_null;
		}
	
		sl_bool isNotNull() const noexcept
		{
			return ptr != sl_null;
		}
	
		void setNull() noexcept
		{
			_replaceObject(sl_null);
		}

		T* get() const& noexcept
		{
			return ptr;
		}

		const Ref<Referable>& getReference() const noexcept
		{
			return *(reinterpret_cast<Ref<Referable> const*>(this));
		}

		template <class... TYPES>
		static const Ref& from(const Ref<TYPES...>& other) noexcept
		{
			return *(reinterpret_cast<Ref const*>(&other));
		}

		template <class... TYPES>
		static Ref& from(Ref<TYPES...>& other) noexcept
		{
			return *(reinterpret_cast<Ref*>(&other));
		}

		template <class... TYPES>
		static Ref&& from(Ref<TYPES...>&& other) noexcept
		{
			return static_cast<Ref&&>(*(reinterpret_cast<Ref*>(&other)));
		}

	public:
		Ref& operator=(sl_null_t) noexcept
		{
			_replaceObject(sl_null);
			return *this;
		}

		Ref& operator=(T* other) noexcept
		{
			if (ptr != other) {
				if (other) {
					other->increaseReference();
				}
				_replaceObject(other);
			}
			return *this;
		}
	
		Ref& operator=(Ref&& other) noexcept
		{
			_move_assign(&other);
			return *this;
		}

		Ref& operator=(const Ref& other) noexcept
		{
			T* o = other.ptr;
			if (ptr != o) {
				if (o) {
					o->increaseReference();
				}
				_replaceObject(o);
			}
			return *this;
		}

		template <class OTHER>
		Ref& operator=(Ref<OTHER>&& other) noexcept
		{
			SLIB_TRY_CONVERT_TYPE(OTHER*, T*)
			_move_assign(&other);
			return *this;
		}

		template <class OTHER>
		Ref& operator=(const Ref<OTHER>& other) noexcept
		{
			SLIB_TRY_CONVERT_TYPE(OTHER*, T*)
			T* o = other.ptr;
			if (ptr != o) {
				if (o) {
					o->increaseReference();
				}
				_replaceObject(o);
			}
			return *this;
		}

		template <class OTHER>
		Ref& operator=(const AtomicRef<OTHER>& other) noexcept
		{
			SLIB_TRY_CONVERT_TYPE(OTHER*, T*)
			if (ptr != other._ptr) {
				T* o = other._retainObject();
				_replaceObject(o);
			}
			return *this;
		}

		template <class OTHER>
		Ref& operator=(const WeakRef<OTHER>& _other) noexcept
		{
			SLIB_TRY_CONVERT_TYPE(OTHER*, T*)
			Ref<OTHER> other(_other.lock());
			T* o = other.ptr;
			if (ptr != o) {
				if (o) {
					o->increaseReference();
				}
				_replaceObject(o);
			}
			return *this;
		}

		template <class OTHER>
		Ref& operator=(const AtomicWeakRef<OTHER>& _other) noexcept
		{
			SLIB_TRY_CONVERT_TYPE(OTHER*, T*)
			Ref<OTHER> other(_other.lock());
			T* o = other.ptr;
			if (ptr != o) {
				if (o) {
					o->increaseReference();
				}
				_replaceObject(o);
			}
			return *this;
		}
	
		template <class T1, class T2, class... TYPES>
		Ref& operator=(const Ref<T1, T2, TYPES...>& other) noexcept;

		template <class T1, class T2, class... TYPES>
		Ref& operator=(Ref<T1, T2, TYPES...>&& other) noexcept;

		template <class... TYPES>
		Ref& operator=(const Pointer<TYPES...>& other) noexcept;

	public:
		sl_bool operator==(sl_null_t) const noexcept
		{
			return ptr == sl_null;
		}

		sl_bool operator==(T* other) const noexcept
		{
			return ptr == other;
		}

		template <class OTHER>
		sl_bool operator==(const Ref<OTHER>& other) const noexcept
		{
			return (void*)ptr == (void*)(other.ptr);
		}

		template <class OTHER>
		sl_bool operator==(const AtomicRef<OTHER>& other) const noexcept
		{
			return (void*)ptr == (void*)(other._ptr);
		}

		sl_bool operator!=(sl_null_t) const noexcept
		{
			return ptr != sl_null;
		}
	
		sl_bool operator!=(T* other) const noexcept
		{
			return ptr != other;
		}

		template <class OTHER>
		sl_bool operator!=(const Ref<OTHER>& other) const noexcept
		{
			return (void*)ptr != (void*)(other.ptr);
		}

		template <class OTHER>
		sl_bool operator!=(const AtomicRef<OTHER>& other) const noexcept
		{
			return (void*)ptr != (void*)(other._ptr);
		}
	
	public:
		T& operator*() const noexcept
		{
			return *ptr;
		}

		T* operator->() const noexcept
		{
			return ptr;
		}

		operator T*() const noexcept
		{
			return ptr;
		}
	
		explicit operator sl_bool() const noexcept
		{
			return ptr != sl_null;
		}
	
	public:
		void _replaceObject(T* other) noexcept
		{
			if (ptr) {
				ptr->decreaseReference();
			}
			ptr = other;
		}

		void _move_assign(void* _other) noexcept
		{
			if ((void*)this != _other) {
				Ref& other = *(reinterpret_cast<Ref*>(_other));
				_replaceObject(other.ptr);
				other.ptr = sl_null;
			}
		}

	public:
		T* ptr;

	};
	
	template <class T>
	class SLIB_EXPORT Atomic< Ref<T> >
	{
	public:
		constexpr Atomic() noexcept: _ptr(sl_null) {}

		constexpr Atomic(sl_null_t) noexcept: _ptr(sl_null) {}

		Atomic(T* other) noexcept
		{
			if (other) {
				other->increaseReference();
			}
			_ptr = other;
		}

		Atomic(const Atomic& other) noexcept
		{
			T* o = other._retainObject();
			_ptr = o;
		}

		template <class OTHER>
		Atomic(const AtomicRef<OTHER>& other) noexcept
		{
			SLIB_TRY_CONVERT_TYPE(OTHER*, T*)
			T* o = other._retainObject();
			_ptr = o;
		}

		template <class OTHER>
		Atomic(Ref<OTHER>&& other) noexcept
		{
			_ptr = other.ptr;
			other.ptr = sl_null;
		}
	
		template <class OTHER>
		Atomic(const Ref<OTHER>& other) noexcept
		{
			SLIB_TRY_CONVERT_TYPE(OTHER*, T*)
			T* o = other.ptr;
			if (o) {
				o->increaseReference();
			}
			_ptr = o;
		}

		template <class OTHER>
		Atomic(const WeakRef<OTHER>& _other) noexcept
		{
			SLIB_TRY_CONVERT_TYPE(OTHER*, T*)
			Ref<OTHER> other(_other.lock());
			T* o = other.ptr;
			if (o) {
				o->increaseReference();
			}
			_ptr = o;
		}

		template <class OTHER>
		Atomic(const AtomicWeakRef<OTHER>& _other) noexcept
		{
			SLIB_TRY_CONVERT_TYPE(OTHER*, T*)
			Ref<OTHER> other(_other.lock());
			T* o = other.ptr;
			if (o) {
				o->increaseReference();
			}
			_ptr = o;
		}
	
		template <class T1, class T2, class... TYPES>
		Atomic(const Ref<T1, T2, TYPES...>& other) noexcept;

		template <class T1, class T2, class... TYPES>
		Atomic(Ref<T1, T2, TYPES...>&& other) noexcept;

		template <class... TYPES>
		Atomic(const Pointer<TYPES...>& other) noexcept;

		~Atomic() noexcept
		{
			SLIB_TRY_CONVERT_TYPE(T*, Referable*)
			T* o = _ptr;
			if (o) {
				o->decreaseReference();
				_ptr = sl_null;
			}
		}
	
	public:
		sl_bool isNull() const noexcept
		{
			return _ptr == sl_null;
		}
	
		sl_bool isNotNull() const noexcept
		{
			return _ptr != sl_null;
		}
	
		void setNull() noexcept
		{
			_replaceObject(sl_null);
		}
	
		template <class OTHER>
		static const Atomic& from(const AtomicRef<OTHER>& other) noexcept
		{
			return *(reinterpret_cast<Atomic const*>(&other));
		}

		template <class OTHER>
		static Atomic& from(AtomicRef<OTHER>& other) noexcept
		{
			return *(reinterpret_cast<Atomic*>(&other));
		}

	public:
		Atomic& operator=(sl_null_t) noexcept
		{
			_replaceObject(sl_null);
			return *this;
		}

		Atomic& operator=(T* other) noexcept
		{
			if (_ptr != other) {
				if (other) {
					other->increaseReference();
				}
				_replaceObject(other);
			}
			return *this;
		}

		Atomic& operator=(const Atomic& other) noexcept
		{
			if (_ptr != other._ptr) {
				T* o = other._retainObject();
				_replaceObject(o);
			}
			return *this;
		}

		template <class OTHER>
		Atomic& operator=(const AtomicRef<OTHER>& other) noexcept
		{
			SLIB_TRY_CONVERT_TYPE(OTHER*, T*)
			if (_ptr != other._ptr) {
				T* o = other._retainObject();
				_replaceObject(o);
			}
			return *this;
		}

		template <class OTHER>
		Atomic& operator=(Ref<OTHER>&& other) noexcept
		{
			SLIB_TRY_CONVERT_TYPE(OTHER*, T*)
			_move_assign(&other);
			return *this;
		}
	
		template <class OTHER>
		Atomic& operator=(const Ref<OTHER>& other) noexcept
		{
			SLIB_TRY_CONVERT_TYPE(OTHER*, T*)
			T* o = other.ptr;
			if (_ptr != o) {
				if (o) {
					o->increaseReference();
				}
				_replaceObject(o);
			}
			return *this;
		}

		template <class OTHER>
		Atomic& operator=(const WeakRef<OTHER>& _other) noexcept
		{
			SLIB_TRY_CONVERT_TYPE(OTHER*, T*)
			Ref<OTHER> other(_other.lock());
			T* o = other.ptr;
			if (_ptr != o) {
				if (o) {
					o->increaseReference();
				}
				_replaceObject(o);
			}
			return *this;
		}

		template <class OTHER>
		Atomic& operator=(const AtomicWeakRef<OTHER>& _other) noexcept
		{
			SLIB_TRY_CONVERT_TYPE(OTHER*, T*)
			Ref<OTHER> other(_other.lock());
			T* o = other.ptr;
			if (_ptr != o) {
				if (o) {
					o->increaseReference();
				}
				_replaceObject(o);
			}
			return *this;
		}
	
		template <class T1, class T2, class... TYPES>
		Atomic& operator=(const Ref<T1, T2, TYPES...>& other) noexcept;

		template <class T1, class T2, class... TYPES>
		Atomic& operator=(Ref<T1, T2, TYPES...>&& other) noexcept;

		template <class... TYPES>
		Atomic& operator=(const Pointer<TYPES...>& other) noexcept;

	public:
		sl_bool operator==(sl_null_t) const noexcept
		{
			return _ptr == sl_null;
		}

		sl_bool operator==(T* other) const noexcept
		{
			return _ptr == other;
		}

		template <class OTHER>
		sl_bool operator==(const AtomicRef<OTHER>& other) const noexcept
		{
			return (void*)_ptr == (void*)(other._ptr);
		}

		template <class OTHER>
		sl_bool operator==(const Ref<OTHER>& other) const noexcept
		{
			return (void*)_ptr == (void*)(other.ptr);
		}

		sl_bool operator!=(sl_null_t) const noexcept
		{
			return _ptr != sl_null;
		}

		sl_bool operator!=(T* other) const noexcept
		{
			return _ptr != other;
		}

		template <class OTHER>
		sl_bool operator!=(const AtomicRef<OTHER>& other) const noexcept
		{
			return (void*)_ptr != (void*)(other._ptr);
		}

		template <class OTHER>
		sl_bool operator!=(const Ref<OTHER>& other) const noexcept
		{
			return (void*)_ptr != (void*)(other.ptr);
		}

	public:
		Ref<T> operator*() const noexcept
		{
			return *this;
		}
		
		explicit operator sl_bool() const noexcept
		{
			return _ptr != sl_null;
		}
	
	public:
		T* _retainObject() const noexcept
		{
			if (!_ptr) {
				return sl_null;
			}
			m_lock.lock();
			T* o = _ptr;
			if (o) {
				o->increaseReference();
			}
			m_lock.unlock();
			return o;
		}

		void _replaceObject(T* other) noexcept
		{
			m_lock.lock();
			T* before = _ptr;
			_ptr = other;
			m_lock.unlock();
			if (before) {
				before->decreaseReference();
			}
		}

		void _move_assign(void* _other) noexcept
		{
			if ((void*)this != _other) {
				Ref<T>& other = *(reinterpret_cast<Ref<T>*>(_other));
				_replaceObject(other.ptr);
				other.ptr = sl_null;
			}
		}

	public:
		T* _ptr;
		
	private:
		SpinLock m_lock;
	
	};

	
	class SLIB_EXPORT CWeakRef : public Referable
	{
		SLIB_DECLARE_OBJECT
		
	public:
		CWeakRef() noexcept;
		
		~CWeakRef() noexcept;
		
	public:
		Referable* m_object;
		SpinLock m_lock;
		
	public:
		static CWeakRef* create(Referable* object) noexcept;
		
	public:
		Ref<Referable> lock() noexcept;
		
		void release() noexcept;
		
	};
	
	template <class T>
	class SLIB_EXPORT WeakRef
	{
	public:
		WeakRef() noexcept = default;

		WeakRef(sl_null_t) noexcept {}

		WeakRef(T* _other) noexcept
		{
			_set(_other);
		}
	
		WeakRef(WeakRef&& other) noexcept: _weak(Move(other._weak)) {}

		WeakRef(const WeakRef& other) noexcept: _weak(other._weak) {}

		template <class OTHER>
		WeakRef(WeakRef<OTHER>&& other) noexcept: _weak(Move(other._weak))
		{
			SLIB_TRY_CONVERT_TYPE(OTHER*, T*)
		}
	
		template <class OTHER>
		WeakRef(const WeakRef<OTHER>& other) noexcept: _weak(other._weak)
		{
			SLIB_TRY_CONVERT_TYPE(OTHER*, T*)
		}

		template <class OTHER>
		WeakRef(const AtomicWeakRef<OTHER>& other) noexcept: _weak(other._weak)
		{
			SLIB_TRY_CONVERT_TYPE(OTHER*, T*)
		}

		template <class OTHER>
		WeakRef(const Ref<OTHER>& other) noexcept
		{
			SLIB_TRY_CONVERT_TYPE(OTHER*, T*)
			_set(other.ptr);
		}
	
		template <class OTHER>
		WeakRef(const AtomicRef<OTHER>& _other) noexcept
		{
			SLIB_TRY_CONVERT_TYPE(OTHER*, T*)
			Ref<OTHER> other(_other);
			_set(other.ptr);
		}

		template <class T1, class T2, class... TYPES>
		WeakRef(const Ref<T1, T2, TYPES...>& other) noexcept;

		template <class... TYPES>
		WeakRef(const Pointer<TYPES...>& other) noexcept;

		~WeakRef() noexcept
		{
			SLIB_TRY_CONVERT_TYPE(T*, Referable*)
		}

	public:
		static const WeakRef& null() noexcept
		{
			return *(reinterpret_cast<WeakRef const*>(&(priv::ref::g_null)));
		}

		sl_bool isNull() const noexcept
		{
			return _weak.isNull();
		}

		sl_bool isNotNull() const noexcept
		{
			return _weak.isNotNull();
		}

		void setNull() noexcept
		{
			_weak.setNull();
		}

		template <class OTHER>
		static const WeakRef& from(const WeakRef<OTHER>& other) noexcept
		{
			return *(reinterpret_cast<WeakRef const*>(&other));
		}

		template <class OTHER>
		static WeakRef& from(WeakRef<OTHER>& other) noexcept
		{
			return *(reinterpret_cast<WeakRef*>(&other));
		}

		template <class OTHER>
		static WeakRef&& from(WeakRef<OTHER>&& other) noexcept
		{
			return static_cast<WeakRef&&>(*(reinterpret_cast<WeakRef*>(&other)));
		}

		Ref<T> lock() const noexcept
		{
			if (_weak.isNotNull()) {
				return Ref<T>::from(_weak->lock());
			}
			return sl_null;
		}

		static WeakRef fromReferable(Referable* referable) noexcept
		{
			if (referable) {
				WeakRef ret;
				if (referable->_isWeakRef()) {
					ret._weak = static_cast<CWeakRef*>(referable);
				} else {
					ret._weak = referable->_getWeakObject();
				}
				return ret;
			} else {
				return sl_null;
			}
		}

	public:
		WeakRef& operator=(sl_null_t) noexcept
		{
			_weak.setNull();
			return *this;
		}

		WeakRef& operator=(T* _other) noexcept
		{
			_set(_other);
			return *this;
		}
	
		WeakRef& operator=(WeakRef&& other) noexcept
		{
			_weak._move_assign(&other);
			return *this;
		}

		WeakRef& operator=(const WeakRef& other) noexcept
		{
			_weak = other._weak;
			return *this;
		}

		template <class OTHER>
		WeakRef& operator=(WeakRef<OTHER>&& other) noexcept
		{
			SLIB_TRY_CONVERT_TYPE(OTHER*, T*)
			_weak._move_assign(&other);
			return *this;
		}
	
		template <class OTHER>
		WeakRef& operator=(const WeakRef<OTHER>& other) noexcept
		{
			SLIB_TRY_CONVERT_TYPE(OTHER*, T*)
			_weak = other._weak;
			return *this;
		}

		template <class OTHER>
		WeakRef& operator=(const AtomicWeakRef<OTHER>& other) noexcept
		{
			SLIB_TRY_CONVERT_TYPE(OTHER*, T*)
			_weak = other._weak;
			return *this;
		}
	
		template <class OTHER>
		WeakRef& operator=(const Ref<OTHER>& other) noexcept
		{
			SLIB_TRY_CONVERT_TYPE(OTHER*, T*)
			_set(other.ptr);
			return *this;
		}
	
		template <class OTHER>
		WeakRef& operator=(const AtomicRef<OTHER>& _other) noexcept
		{
			SLIB_TRY_CONVERT_TYPE(OTHER*, T*)
			Ref<OTHER> other(_other);
			_set(other.ptr);
			return *this;
		}

		template <class T1, class T2, class... TYPES>
		WeakRef& operator=(const Ref<T1, T2, TYPES...>& other) noexcept;

		template <class... TYPES>
		WeakRef& operator=(const Pointer<TYPES...>& other) noexcept;

	public:
		template <class OTHER>
		sl_bool operator==(const WeakRef<OTHER>& other) const noexcept
		{
			return _weak == other._weak;
		}
	
		template <class OTHER>
		sl_bool operator==(const AtomicWeakRef<OTHER>& other) const noexcept
		{
			return _weak == other._weak;
		}

		template <class OTHER>
		sl_bool operator!=(const WeakRef<OTHER>& other) const noexcept
		{
			return _weak != other._weak;
		}

		template <class OTHER>
		sl_bool operator!=(const AtomicWeakRef<OTHER>& other) const noexcept
		{
			return _weak != other._weak;
		}
		
	public:
		explicit operator sl_bool() const noexcept
		{
			return _weak.isNotNull();
		}
	
	private:
		void _set(T* object) noexcept
		{
			if (object) {
				_weak = object->_getWeakObject();
			} else {
				_weak.setNull();
			}
		}

	public:
		Ref<CWeakRef> _weak;

	};
	
	template <class T>
	class SLIB_EXPORT Atomic< WeakRef<T> >
	{
	public:
		Atomic() noexcept = default;

		Atomic(sl_null_t) noexcept {}

		Atomic(T* _other) noexcept
		{
			_set(_other);
		}

		Atomic(const Atomic& other) noexcept: _weak(other._weak) {}

		template <class OTHER>
		Atomic(const AtomicWeakRef<OTHER>& other) noexcept: _weak(other._weak)
		{
			SLIB_TRY_CONVERT_TYPE(OTHER*, T*)
		}

		template <class OTHER>
		Atomic(WeakRef<OTHER>&& other) noexcept: _weak(Move(other._weak))
		{
			SLIB_TRY_CONVERT_TYPE(OTHER*, T*)
		}

		template <class OTHER>
		Atomic(const WeakRef<OTHER>& other) noexcept: _weak(other._weak)
		{
			SLIB_TRY_CONVERT_TYPE(OTHER*, T*)
		}

		template <class OTHER>
		Atomic(const Ref<OTHER>& other) noexcept
		{
			SLIB_TRY_CONVERT_TYPE(OTHER*, T*)
			_set(other.ptr);
		}

		template <class OTHER>
		Atomic(const AtomicRef<OTHER>& _other) noexcept
		{
			SLIB_TRY_CONVERT_TYPE(OTHER*, T*)
			Ref<OTHER> other(_other);
			_set(other.ptr);
		}

		template <class T1, class T2, class... TYPES>
		Atomic(const Ref<T1, T2, TYPES...>& other) noexcept;

		template <class... TYPES>
		Atomic(const Pointer<TYPES...>& other) noexcept;

		~Atomic() noexcept
		{
			SLIB_TRY_CONVERT_TYPE(T*, Referable*)
		}

	public:
		sl_bool isNull() const noexcept
		{
			return _weak.isNull();
		}

		sl_bool isNotNull() const noexcept
		{
			return _weak.isNotNull();
		}

		void setNull() noexcept
		{
			_weak.setNull();
		}

		template <class OTHER>
		static const Atomic& from(const AtomicWeakRef<OTHER>& other) noexcept
		{
			return *(reinterpret_cast<Atomic const*>(&other));
		}

		template <class OTHER>
		static Atomic& from(AtomicWeakRef<OTHER>& other) noexcept
		{
			return *(reinterpret_cast<Atomic*>(&other));
		}

		Ref<T> lock() const noexcept
		{
			Ref<CWeakRef> weak(_weak);
			if (weak.isNotNull()) {
				return Ref<T>::from(weak->lock());
			}
			return sl_null;
		}
	
	public:
		AtomicWeakRef<T>& operator=(sl_null_t) noexcept
		{
			_weak.setNull();
			return *this;
		}

		AtomicWeakRef<T>& operator=(T* _other) noexcept
		{
			_set(_other);
			return *this;
		}
	
		AtomicWeakRef<T>& operator=(const AtomicWeakRef<T>& other) noexcept
		{
			_weak = other._weak;
			return *this;
		}

		template <class OTHER>
		AtomicWeakRef<T>& operator=(const AtomicWeakRef<OTHER>& other) noexcept
		{
			SLIB_TRY_CONVERT_TYPE(OTHER*, T*)
			_weak = other._weak;
			return *this;
		}

		template <class OTHER>
		AtomicWeakRef<T>& operator=(WeakRef<OTHER>&& other) noexcept
		{
			SLIB_TRY_CONVERT_TYPE(OTHER*, T*)
			_weak._move_assign(&other);
			return *this;
		}

		template <class OTHER>
		AtomicWeakRef<T>& operator=(const WeakRef<OTHER>& other) noexcept
		{
			SLIB_TRY_CONVERT_TYPE(OTHER*, T*)
			_weak = other._weak;
			return *this;
		}

		template <class OTHER>
		AtomicWeakRef<T>& operator=(const Ref<OTHER>& other) noexcept
		{
			SLIB_TRY_CONVERT_TYPE(OTHER*, T*)
			_set(other.ptr);
			return *this;
		}

		template <class OTHER>
		AtomicWeakRef<T>& operator=(const AtomicRef<OTHER>& _other) noexcept
		{
			SLIB_TRY_CONVERT_TYPE(OTHER*, T*)
			Ref<OTHER> other(_other);
			_set(other.ptr);
			return *this;
		}

		template <class T1, class T2, class... TYPES>
		AtomicWeakRef<T>& operator=(const Ref<T1, T2, TYPES...>& other) noexcept;

		template <class... TYPES>
		AtomicWeakRef<T>& operator=(const Pointer<TYPES...>& other) noexcept;

	public:
		template <class OTHER>
		sl_bool operator==(const AtomicWeakRef<OTHER>& other) const noexcept
		{
			return _weak == other._weak;
		}
		

		template <class OTHER>
		sl_bool operator==(const WeakRef<OTHER>& other) const noexcept
		{
			return _weak == other._weak;
		}

		template <class OTHER>
		sl_bool operator!=(const AtomicWeakRef<OTHER>& other) const noexcept
		{
			return _weak != other._weak;
		}

		template <class OTHER>
		sl_bool operator!=(const WeakRef<OTHER>& other) const noexcept
		{
			return _weak != other._weak;
		}

	public:
		WeakRef<T> operator*() const noexcept
		{
			return *this;
		}
		
		explicit operator sl_bool() const noexcept
		{
			return _weak.isNotNull();
		}
	
	private:
		void _set(T* object) noexcept
		{
			if (object) {
				_weak = object->_getWeakObject();
			} else {
				_weak.setNull();
			}
		}

	public:
		AtomicRef<CWeakRef> _weak;
	
	};
	
	
	template <class T>
	SLIB_INLINE sl_bool operator==(sl_null_t, const Ref<T>& b) noexcept
	{
		return sl_null == b.ptr;
	}
	
	template <class T>
	SLIB_INLINE sl_bool operator==(T* a, const Ref<T>& b) noexcept
	{
		return a == b.ptr;
	}

	template <class T>
	SLIB_INLINE sl_bool operator!=(sl_null_t, const Ref<T>& b) noexcept
	{
		return sl_null != b.ptr;
	}
	
	template <class T>
	SLIB_INLINE sl_bool operator!=(T* a, const Ref<T>& b) noexcept
	{
		return a != b.ptr;
	}

	template <class T>
	SLIB_INLINE sl_bool operator==(sl_null_t, const AtomicRef<T>& b) noexcept
	{
		return sl_null == b._ptr;
	}
	
	template <class T>
	SLIB_INLINE sl_bool operator==(T* a, const AtomicRef<T>& b) noexcept
	{
		return a == b._ptr;
	}

	template <class T>
	SLIB_INLINE sl_bool operator!=(sl_null_t, const AtomicRef<T>& b) noexcept
	{
		return sl_null != b._ptr;
	}
	
	template <class T>
	SLIB_INLINE sl_bool operator!=(T* a, const AtomicRef<T>& b) noexcept
	{
		return a != b._ptr;
	}

	template <class T>
	SLIB_INLINE sl_bool operator>(const Ref<T>& a, const Ref<T>& b) noexcept
	{
		return a.ptr > b.ptr;
	}

	template <class T>
	SLIB_INLINE sl_bool operator<(const Ref<T>& a, const Ref<T>& b) noexcept
	{
		return a.ptr < b.ptr;
	}


	template <class T>
	Ref<T> New() noexcept
	{
		return new T;
	}

	template <class T, class ARG, class... ARGS>
	Ref<T> New(ARG&& arg, ARGS&&... args) noexcept
	{
		Ref<T> o = new T;
		if (o.isNotNull()) {
			o->init(Forward<ARG>(arg), Forward<ARGS>(args)...);
			return o;
		}
		return sl_null;
	}

	template <class T, class OTHER>
	sl_bool IsInstanceOf(const OTHER* object) noexcept
	{
		SLIB_TRY_CONVERT_TYPE(OTHER*, Referable*)
		if (object) {
			return object->isInstanceOf(T::ObjectType());
		}
		return sl_false;
	}

	template <class T, class OTHER>
	sl_bool IsInstanceOf(const Ref<OTHER>& object) noexcept
	{
		SLIB_TRY_CONVERT_TYPE(OTHER*, Referable*)
		if (object.isNotNull()) {
			return object->isInstanceOf(T::ObjectType());
		}
		return sl_false;
	}
	
	template <class T, class OTHER>
	T* CastInstance(OTHER* object) noexcept
	{
		SLIB_TRY_CONVERT_TYPE(OTHER*, Referable*)
		if (object) {
			if (object->isInstanceOf(T::ObjectType())) {
				return static_cast<T*>(object);
			}
		}
		return sl_null;
	}

	template <class T, class OTHER>
	const Ref<T>& CastRef(const Ref<OTHER>& object) noexcept
	{
		SLIB_TRY_CONVERT_TYPE(OTHER*, Referable*)
		if (object.isNotNull()) {
			if (object->isInstanceOf(T::ObjectType())) {
				return *(reinterpret_cast<Ref<T> const*>(&object));
			}
		}
		return Ref<T>::null();
	}

	template <class T, class OTHER>
	const Ref<T>& CastRef(const Ref<OTHER>& object, const Ref<T>& def) noexcept
	{
		SLIB_TRY_CONVERT_TYPE(OTHER*, Referable*)
		if (object.isNotNull()) {
			if (object->isInstanceOf(T::ObjectType())) {
				return *(reinterpret_cast<Ref<T> const*>(&object));
			}
		}
		return def;
	}

	template <class T>
	Ref<T> ToRef(T* other) noexcept
	{
		return Ref<T>(other);
	}

	template <class T>
	const Ref<T>& ToRef(const Ref<T>& other) noexcept
	{
		return other;
	}

	template <class T>
	Ref<T> ToRef(const AtomicRef<T>& other) noexcept
	{
		return Ref<T>(other);
	}

	template <class T>
	Ref<T> ToRef(const WeakRef<T>& other) noexcept
	{
		return Ref<T>(other);
	}

	template <class T>
	Ref<T> ToRef(const AtomicWeakRef<T>& other) noexcept
	{
		return Ref<T>(other);
	}

	template <class T>
	WeakRef<T> ToWeakRef(T* other) noexcept
	{
		return WeakRef<T>(other);
	}

	template <class T>
	WeakRef<T> ToWeakRef(const Ref<T>& other) noexcept
	{
		return WeakRef<T>(other);
	}

	template <class T>
	WeakRef<T> ToWeakRef(const AtomicRef<T>& other) noexcept
	{
		return WeakRef<T>(other);
	}

	template <class T>
	const WeakRef<T>& ToWeakRef(const WeakRef<T>& other) noexcept
	{
		return other;
	}

	template <class T>
	WeakRef<T> ToWeakRef(const AtomicWeakRef<T>& other) noexcept
	{
		return WeakRef<T>(other);
	}

}

#endif
