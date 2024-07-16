/*
 *   Copyright (c) 2008-2024 SLIBIO <https://github.com/SLIBIO>
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
#include "common_members.h"

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

#ifdef SLIB_COMPILE_LIB
#	define SLIB_DEFINE_ROOT_OBJECT(CLASS) SLIB_DEFINE_ROOT_OBJECT_BY_CONSTANT_TYPE(CLASS)
#	define SLIB_DEFINE_OBJECT(CLASS, BASE) SLIB_DEFINE_OBJECT_BY_CONSTANT_TYPE(CLASS, BASE)
#else
#	define SLIB_DEFINE_ROOT_OBJECT(CLASS) SLIB_DEFINE_ROOT_OBJECT_BY_VARIABLE_TYPE(CLASS)
#	define SLIB_DEFINE_OBJECT(CLASS, BASE) SLIB_DEFINE_OBJECT_BY_VARIABLE_TYPE(CLASS, BASE)
#endif

#define SLIB_REF_WRAPPER_NO_ATOMIC_NO_OP(WRAPPER, ...) \
public: \
	static sl_object_type ObjectType() noexcept { return __VA_ARGS__::ObjectType(); } \
	SLIB_CONSTEXPR WRAPPER() = default; \
	SLIB_CONSTEXPR WRAPPER(sl_null_t) {} \
	SLIB_CONSTEXPR WRAPPER(priv::ref::DummyContainer* container): ref(container) {} \
	WRAPPER(__VA_ARGS__* obj) noexcept: ref(obj) {} \
	WRAPPER(WRAPPER&& other) noexcept: ref(Move(other.ref)) {} \
	WRAPPER(const WRAPPER& other) noexcept: ref(other.ref) {} \
	static const WRAPPER& null() noexcept { return *(reinterpret_cast<WRAPPER const*>(&(priv::ref::g_null))); } \
	SLIB_CONSTEXPR sl_bool isNull() const { return ref.isNull(); } \
	SLIB_CONSTEXPR sl_bool isNotNull() const { return ref.isNotNull(); } \
	SLIB_CONSTEXPR __VA_ARGS__* getObject() const { return ref.ptr; } \
	void setNull() noexcept { ref.setNull(); } \
	WRAPPER& operator=(sl_null_t) noexcept { ref.setNull(); return *this; } \
	WRAPPER& operator=(__VA_ARGS__* obj) noexcept { ref = obj; return *this; } \
	WRAPPER& operator=(WRAPPER&& other) noexcept { ref = Move(other.ref); return *this; } \
	WRAPPER& operator=(const WRAPPER& other) noexcept { ref = other.ref; return *this; }

#define SLIB_REF_WRAPPER_NO_OP(WRAPPER, ...) \
public: \
	SLIB_REF_WRAPPER_NO_ATOMIC_NO_OP(WRAPPER, __VA_ARGS__) \
	WRAPPER(Atomic<WRAPPER>&& other) noexcept: ref(Move(*(reinterpret_cast<AtomicRef<__VA_ARGS__>*>(&other)))) {} \
	WRAPPER(const Atomic<WRAPPER>& other) noexcept: ref(*(reinterpret_cast<const AtomicRef<__VA_ARGS__>*>(&other))) {} \
	WRAPPER& operator=(Atomic<WRAPPER>&& other) noexcept { ref = Move(*(reinterpret_cast<AtomicRef<__VA_ARGS__>*>(&other))); return *this; } \
	WRAPPER& operator=(const Atomic<WRAPPER>& other) noexcept { ref = *(reinterpret_cast<const AtomicRef<__VA_ARGS__>*>(&other)); return *this; }

#define SLIB_REF_WRAPPER_NO_ATOMIC_OP(WRAPPER, ...) \
	SLIB_CONSTEXPR sl_bool equals(const WRAPPER& other) const { return ref.equals(other.ref.ptr); } \
	template <class OTHER> SLIB_CONSTEXPR sl_bool equals(const OTHER& other) const { return ref.equals(other); } \
	SLIB_CONSTEXPR sl_compare_result compare(const WRAPPER& other) const { return ref.compare(other.ref.ptr); } \
	template <class OTHER> SLIB_CONSTEXPR sl_compare_result compare(const OTHER& other) const { return ref.compare(other); } \
	SLIB_DEFINE_CLASS_DEFAULT_COMPARE_OPERATORS_CONSTEXPR \
	SLIB_CONSTEXPR explicit operator sl_bool() const { return ref.ptr != sl_null; }

#define SLIB_REF_WRAPPER_OP(WRAPPER, ...) \
	SLIB_REF_WRAPPER_NO_ATOMIC_OP(WRAPPER, __VA_ARGS__) \
	SLIB_CONSTEXPR sl_bool equals(const Atomic<WRAPPER>& other) const { return ref.equals(*(reinterpret_cast<const AtomicRef<__VA_ARGS__>*>(&other))); } \
	SLIB_CONSTEXPR sl_compare_result compare(const Atomic<WRAPPER>& other) const { return ref.compare(*(reinterpret_cast<const AtomicRef<__VA_ARGS__>*>(&other))); }

#define SLIB_REF_WRAPPER_NO_ATOMIC(WRAPPER, ...) \
	SLIB_REF_WRAPPER_NO_ATOMIC_NO_OP(WRAPPER, __VA_ARGS__) \
	SLIB_REF_WRAPPER_NO_ATOMIC_OP(WRAPPER, __VA_ARGS__)

#define SLIB_REF_WRAPPER(WRAPPER, ...) \
	SLIB_REF_WRAPPER_NO_OP(WRAPPER, __VA_ARGS__) \
	SLIB_REF_WRAPPER_OP(WRAPPER, __VA_ARGS__)

#define SLIB_ATOMIC_REF_WRAPPER_NO_OP(...) \
public: \
	SLIB_CONSTEXPR Atomic() = default; \
	SLIB_CONSTEXPR Atomic(sl_null_t) {} \
	Atomic(__VA_ARGS__* obj) noexcept: ref(obj) {} \
	Atomic(Atomic&& other) noexcept: ref(Move(other.ref)) {} \
	Atomic(const Atomic& other) noexcept: ref(other.ref) {} \
	Atomic(typename RemoveAtomic<Atomic>::Type&& other) noexcept: ref(Move(*(reinterpret_cast<Ref<__VA_ARGS__>*>(&other)))) {} \
	Atomic(typename RemoveAtomic<Atomic>::Type const& other) noexcept: ref(*(reinterpret_cast<const Ref<__VA_ARGS__>*>(&other))) {} \
	SLIB_CONSTEXPR sl_bool isNull() const { return ref.isNull(); } \
	SLIB_CONSTEXPR sl_bool isNotNull() const { return ref.isNotNull(); } \
	void setNull() noexcept { ref.setNull(); } \
	Atomic& operator=(sl_null_t) noexcept { ref.setNull(); return *this; } \
	Atomic& operator=(__VA_ARGS__* obj) noexcept { ref = obj; return *this; } \
	Atomic& operator=(Atomic&& other) noexcept { ref = Move(other.ref); return *this; } \
	Atomic& operator=(const Atomic& other) noexcept { ref = other.ref; return *this; } \
	Atomic& operator=(typename RemoveAtomic<Atomic>::Type&& other) noexcept { ref = Move(*(reinterpret_cast<Ref<__VA_ARGS__>*>(&other))); return *this; } \
	Atomic& operator=(typename RemoveAtomic<Atomic>::Type const& other) noexcept { ref = *(reinterpret_cast<const Ref<__VA_ARGS__>*>(&other)); return *this; } \
	typename RemoveAtomic<Atomic>::Type release() noexcept { return (priv::ref::DummyContainer*)((void*)(ref._release())); }

#define SLIB_ATOMIC_REF_WRAPPER(...) \
	SLIB_ATOMIC_REF_WRAPPER_NO_OP(__VA_ARGS__) \
	SLIB_CONSTEXPR sl_bool equals(const typename RemoveAtomic<Atomic>::Type& other) const { return ref.equals(reinterpret_cast<const Ref<__VA_ARGS__>*>(&other)->ptr); } \
	SLIB_CONSTEXPR sl_bool equals(const Atomic& other) const { return ref.equals(other.ref._ptr); } \
	SLIB_CONSTEXPR sl_compare_result compare(const typename RemoveAtomic<Atomic>::Type& other) const { return ref.compare(reinterpret_cast<const Ref<__VA_ARGS__>*>(&other)->ptr); } \
	SLIB_CONSTEXPR sl_compare_result compare(const Atomic& other) const { return ref.compare(other.ref._ptr); } \
	template <class OTHER> SLIB_CONSTEXPR sl_bool equals(const OTHER& other) const { return ref.equals(other); } \
	template <class OTHER> SLIB_CONSTEXPR sl_compare_result compare(const OTHER& other) const { return ref.compare(other); } \
	SLIB_DEFINE_CLASS_DEFAULT_COMPARE_OPERATORS_CONSTEXPR \
	SLIB_CONSTEXPR explicit operator sl_bool() const { return ref._ptr != sl_null; }

#define SLIB_REF_WRAPPER_ALL_LRVALUES_NO_ATOMIC(WRAPPER, ...) \
public: \
	WRAPPER(const WRAPPER&& other) noexcept: ref(other.ref) {} \
	WRAPPER(WRAPPER& other) noexcept: ref(other.ref) {} \
	WRAPPER& operator=(const WRAPPER&& other) noexcept { ref = other.ref; return *this; } \
	WRAPPER& operator=(WRAPPER& other) noexcept { ref = other.ref; return *this; }

#define SLIB_REF_WRAPPER_ALL_LRVALUES(WRAPPER, ...) \
public: \
	SLIB_REF_WRAPPER_ALL_LRVALUES_NO_ATOMIC(WRAPPER, __VA_ARGS__) \
	WRAPPER(const Atomic<WRAPPER>&& other) noexcept: ref(*(reinterpret_cast<const AtomicRef<__VA_ARGS__>*>(&other))) {} \
	WRAPPER(Atomic<WRAPPER>& other) noexcept: ref(*(reinterpret_cast<AtomicRef<__VA_ARGS__>*>(&other))) {} \
	WRAPPER& operator=(const Atomic<WRAPPER>&& other) noexcept { ref = *(reinterpret_cast<const AtomicRef<__VA_ARGS__>*>(&other)); return *this; } \
	WRAPPER& operator=(Atomic<WRAPPER>& other) noexcept { ref = *(reinterpret_cast<AtomicRef<__VA_ARGS__>*>(&other)); return *this; }

#define SLIB_ATOMIC_REF_WRAPPER_ALL_LRVALUES(...) \
public: \
	Atomic(const Atomic&& other) noexcept: ref(other.ref) {} \
	Atomic(Atomic& other) noexcept: ref(other.ref) {} \
	Atomic(typename RemoveAtomic<Atomic>::Type const&& other) noexcept: ref(*(reinterpret_cast<const Ref<__VA_ARGS__>*>(&other))) {} \
	Atomic(typename RemoveAtomic<Atomic>::Type& other) noexcept: ref(*(reinterpret_cast<Ref<__VA_ARGS__>*>(&other))) {} \
	Atomic& operator=(const Atomic&& other) noexcept { ref = other.ref; return *this; } \
	Atomic& operator=(Atomic& other) noexcept { ref = other.ref; return *this; } \
	Atomic& operator=(typename RemoveAtomic<Atomic>::Type const&& other) noexcept { ref = *(reinterpret_cast<const Ref<__VA_ARGS__>*>(&other)); return *this; } \
	Atomic& operator=(typename RemoveAtomic<Atomic>::Type& other) noexcept { ref = *(reinterpret_cast<Ref<__VA_ARGS__>*>(&other)); return *this; }

typedef void* sl_object_type;

namespace slib
{

	namespace priv
	{
		namespace ref
		{
			extern void* const g_null;
			class DummyContainer;
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
	class MemoryBuffer;

	class SLIB_EXPORT CRef
	{
	public:
		CRef() noexcept;

		CRef(const CRef& other) noexcept;

		CRef(CRef&& other) noexcept;

		virtual ~CRef();

	public:
		static sl_reg increaseReference(volatile sl_reg& refCount) noexcept;

		static sl_reg decreaseReference(volatile sl_reg& refCount) noexcept;

		sl_reg increaseReference() noexcept;

		sl_reg decreaseReference() noexcept;

		sl_reg getReferenceCount() noexcept;

	protected:
		virtual void init();

		virtual void free();

	public:
		static sl_object_type ObjectType() noexcept;

		static sl_bool isDerivedFrom(sl_object_type type) noexcept;

		virtual sl_object_type getObjectType() const noexcept;

		virtual sl_bool isInstanceOf(sl_object_type type) const noexcept;

	public:
		virtual String toString();

		virtual sl_bool toJsonString(StringBuffer& buf);

		virtual sl_bool toJsonBinary(MemoryBuffer& buf);

		virtual sl_bool runOperator(sl_uint32 op, Variant& result, const Variant& secondOperand, sl_bool flagThisOnLeft);

	public:
		sl_bool _isWeakRef() const noexcept;

		CWeakRef* _getWeakObject() noexcept;

		void _clearWeak() noexcept;

		sl_reg _increaseReference() noexcept;

		sl_reg _decreaseReference() noexcept;

	public:
		CRef& operator=(const CRef& other) noexcept;

		CRef& operator=(CRef&& other) noexcept;

	private:
		volatile sl_reg m_nRefCount;
		CWeakRef* m_weak;

		friend class CWeakRef;

	};


	template <class T>
	class SLIB_EXPORT Ref<T>
	{
	public:
		SLIB_CONSTEXPR Ref(): ptr(sl_null) {}

		SLIB_CONSTEXPR Ref(sl_null_t): ptr(sl_null) {}

		SLIB_CONSTEXPR Ref(priv::ref::DummyContainer* _ptr): ptr((T*)((void*)_ptr)) {}

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
		Ref(AtomicRef<OTHER>&& other) noexcept
		{
			SLIB_TRY_CONVERT_TYPE(OTHER*, T*)
			ptr = other._release();
		}

		template <class OTHER>
		Ref(const AtomicRef<OTHER>& other) noexcept
		{
			SLIB_TRY_CONVERT_TYPE(OTHER*, T*)
			ptr = other._retain();
		}

		template <class OTHER>
		Ref(const WeakRef<OTHER>& _other) noexcept
		{
			SLIB_TRY_CONVERT_TYPE(OTHER*, T*)
			Ref<OTHER> other(_other.lock());
			ptr = other.ptr;
			other.ptr = sl_null;
		}

		template <class OTHER>
		Ref(const AtomicWeakRef<OTHER>& _other) noexcept
		{
			SLIB_TRY_CONVERT_TYPE(OTHER*, T*)
			Ref<OTHER> other(_other.lock());
			ptr = other.ptr;
			other.ptr = sl_null;
		}

		template <class T1, class T2, class... TYPES>
		Ref(const Ref<T1, T2, TYPES...>& other) noexcept;

		template <class... TYPES>
		Ref(const Pointer<TYPES...>& other) noexcept;

		~Ref()
		{
			SLIB_TRY_CONVERT_TYPE(T*, CRef*)
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

		SLIB_CONSTEXPR sl_bool isNull() const
		{
			return !ptr;
		}

		SLIB_CONSTEXPR sl_bool isNotNull() const
		{
			return ptr != sl_null;
		}

		void setNull() noexcept
		{
			_replace(sl_null);
		}

		SLIB_CONSTEXPR T* get() const&
		{
			return ptr;
		}

		SLIB_DEFINE_CAST_REF_FUNCTIONS(class... TYPES, Ref, Ref<TYPES...>)

	public:
		Ref& operator=(sl_null_t) noexcept
		{
			_replace(sl_null);
			return *this;
		}

		Ref& operator=(T* other) noexcept
		{
			_copy(other);
			return *this;
		}

		Ref& operator=(Ref&& other) noexcept
		{
			_move(&other);
			return *this;
		}

		Ref& operator=(const Ref& other) noexcept
		{
			_copy(other.ptr);
			return *this;
		}

		template <class OTHER>
		Ref& operator=(Ref<OTHER>&& other) noexcept
		{
			SLIB_TRY_CONVERT_TYPE(OTHER*, T*)
			_move(&other);
			return *this;
		}

		template <class OTHER>
		Ref& operator=(const Ref<OTHER>& other) noexcept
		{
			SLIB_TRY_CONVERT_TYPE(OTHER*, T*)
			_copy(other.ptr);
			return *this;
		}

		template <class OTHER>
		Ref& operator=(AtomicRef<OTHER>&& other) noexcept
		{
			SLIB_TRY_CONVERT_TYPE(OTHER*, T*)
			if ((void*)this != (void*)&other) {
				T* o = other._release();
				_replace(o);
			}
			return *this;
		}

		template <class OTHER>
		Ref& operator=(const AtomicRef<OTHER>& other) noexcept
		{
			SLIB_TRY_CONVERT_TYPE(OTHER*, T*)
			if (ptr != other._ptr) {
				T* o = other._retain();
				_replace(o);
			}
			return *this;
		}

		template <class OTHER>
		Ref& operator=(const WeakRef<OTHER>& _other) noexcept
		{
			SLIB_TRY_CONVERT_TYPE(OTHER*, T*)
			Ref<OTHER> other(_other.lock());
			_replace(other.ptr);
			other.ptr = sl_null;
			return *this;
		}

		template <class OTHER>
		Ref& operator=(const AtomicWeakRef<OTHER>& _other) noexcept
		{
			SLIB_TRY_CONVERT_TYPE(OTHER*, T*)
			Ref<OTHER> other(_other.lock());
			_replace(other.ptr);
			other.ptr = sl_null;
			return *this;
		}

		template <class T1, class T2, class... TYPES>
		Ref& operator=(const Ref<T1, T2, TYPES...>& other) noexcept;

		template <class... TYPES>
		Ref& operator=(const Pointer<TYPES...>& other) noexcept;

	public:
		SLIB_CONSTEXPR sl_bool equals(sl_null_t) const
		{
			return !ptr;
		}

		template <class OTHER>
		SLIB_CONSTEXPR sl_bool equals(const OTHER* other) const
		{
			return ptr == other;
		}

		template <class OTHER>
		SLIB_CONSTEXPR sl_bool equals(const Ref<OTHER>& other) const
		{
			return ptr == other.ptr;
		}

		template <class OTHER>
		SLIB_CONSTEXPR sl_bool equals(const AtomicRef<OTHER>& other) const
		{
			return ptr == other._ptr;
		}

		SLIB_CONSTEXPR sl_compare_result compare(sl_null_t) const
		{
			return ptr != sl_null;
		}

		template <class OTHER>
		SLIB_CONSTEXPR sl_compare_result compare(const OTHER* other) const
		{
			return ComparePrimitiveValues(ptr, other);
		}

		template <class OTHER>
		SLIB_CONSTEXPR sl_compare_result compare(const Ref<OTHER>& other) const noexcept
		{
			return ComparePrimitiveValues(ptr, other.ptr);
		}

		template <class OTHER>
		SLIB_CONSTEXPR sl_compare_result compare(const AtomicRef<OTHER>& other) const noexcept
		{
			return ComparePrimitiveValues(ptr, other._ptr);
		}

		SLIB_DEFINE_CLASS_DEFAULT_COMPARE_OPERATORS_CONSTEXPR

	public:
		SLIB_CONSTEXPR T& operator*() const&
		{
			return *ptr;
		}

		SLIB_CONSTEXPR T* operator->() const&
		{
			return ptr;
		}

		SLIB_CONSTEXPR operator T*() const&
		{
			return ptr;
		}

		SLIB_CONSTEXPR explicit operator sl_bool() const
		{
			return ptr != sl_null;
		}

	public:
		void _replace(T* other) noexcept
		{
			if (ptr) {
				ptr->decreaseReference();
			}
			ptr = other;
		}

		void _copy(T* other) noexcept
		{
			if (ptr != other) {
				if (other) {
					other->increaseReference();
				}
				_replace(other);
			}
		}

		void _move(void* _other) noexcept
		{
			if ((void*)this != _other) {
				Ref& other = *(reinterpret_cast<Ref*>(_other));
				_replace(other.ptr);
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
		SLIB_CONSTEXPR Atomic(): _ptr(sl_null) {}

		SLIB_CONSTEXPR Atomic(sl_null_t): _ptr(sl_null) {}

		Atomic(T* other) noexcept
		{
			if (other) {
				other->increaseReference();
			}
			_ptr = other;
		}

		Atomic(Atomic&& other) noexcept
		{
			_ptr = other._release();
		}

		Atomic(const Atomic& other) noexcept
		{
			_ptr = other._retain();
		}

		template <class OTHER>
		Atomic(AtomicRef<OTHER>&& other) noexcept
		{
			SLIB_TRY_CONVERT_TYPE(OTHER*, T*)
			_ptr = other._release();
		}

		template <class OTHER>
		Atomic(const AtomicRef<OTHER>& other) noexcept
		{
			SLIB_TRY_CONVERT_TYPE(OTHER*, T*)
			_ptr = other._retain();
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
			_ptr = other.ptr;
			other.ptr = sl_null;
		}

		template <class OTHER>
		Atomic(const AtomicWeakRef<OTHER>& _other) noexcept
		{
			SLIB_TRY_CONVERT_TYPE(OTHER*, T*)
			Ref<OTHER> other(_other.lock());
			_ptr = other.ptr;
			other.ptr = sl_null;
		}

		template <class T1, class T2, class... TYPES>
		Atomic(const Ref<T1, T2, TYPES...>& other) noexcept;

		template <class... TYPES>
		Atomic(const Pointer<TYPES...>& other) noexcept;

		~Atomic()
		{
			SLIB_TRY_CONVERT_TYPE(T*, CRef*)
			T* o = _ptr;
			if (o) {
				o->decreaseReference();
				_ptr = sl_null;
			}
		}

	public:
		SLIB_CONSTEXPR sl_bool isNull() const
		{
			return _ptr == sl_null;
		}

		SLIB_CONSTEXPR sl_bool isNotNull() const
		{
			return _ptr != sl_null;
		}

		void setNull() noexcept
		{
			_replace(sl_null);
		}
		
		Ref<T> release() noexcept
		{
			return (priv::ref::DummyContainer*)((void*)_release());
		}

		SLIB_DEFINE_CAST_REF_FUNCTIONS(class OTHER, Atomic, AtomicRef<OTHER>)

	public:
		Atomic& operator=(sl_null_t) noexcept
		{
			_replace(sl_null);
			return *this;
		}

		Atomic& operator=(T* other) noexcept
		{
			_copy(other);
			return *this;
		}

		Atomic& operator=(Atomic&& other) noexcept
		{
			if (this != &other) {
				_replace(other._release());
			}
			return *this;
		}

		Atomic& operator=(const Atomic& other) noexcept
		{
			if (_ptr != other._ptr) {
				_replace(other._retain());
			}
			return *this;
		}

		template <class OTHER>
		Atomic& operator=(AtomicRef<OTHER>&& other) noexcept
		{
			SLIB_TRY_CONVERT_TYPE(OTHER*, T*)
			if ((void*)this != (void*)&other) {
				_replace(other._release());
			}
			return *this;
		}

		template <class OTHER>
		Atomic& operator=(const AtomicRef<OTHER>& other) noexcept
		{
			SLIB_TRY_CONVERT_TYPE(OTHER*, T*)
			if (_ptr != other._ptr) {
				_replace(other._retain());
			}
			return *this;
		}

		template <class OTHER>
		Atomic& operator=(Ref<OTHER>&& other) noexcept
		{
			SLIB_TRY_CONVERT_TYPE(OTHER*, T*)
			if ((void*)this != (void*)&other) {
				_replace(other.ptr);
				other.ptr = sl_null;
			}
			return *this;
		}

		template <class OTHER>
		Atomic& operator=(const Ref<OTHER>& other) noexcept
		{
			SLIB_TRY_CONVERT_TYPE(OTHER*, T*)
			_copy(other.ptr);
			return *this;
		}

		template <class OTHER>
		Atomic& operator=(const WeakRef<OTHER>& _other) noexcept
		{
			SLIB_TRY_CONVERT_TYPE(OTHER*, T*)
			Ref<OTHER> other(_other.lock());
			_replace(other.ptr);
			other.ptr = sl_null;
			return *this;
		}

		template <class OTHER>
		Atomic& operator=(const AtomicWeakRef<OTHER>& _other) noexcept
		{
			SLIB_TRY_CONVERT_TYPE(OTHER*, T*)
			Ref<OTHER> other(_other.lock());
			_replace(other.ptr);
			other.ptr = sl_null;
			return *this;
		}

		template <class T1, class T2, class... TYPES>
		Atomic& operator=(const Ref<T1, T2, TYPES...>& other) noexcept;

		template <class... TYPES>
		Atomic& operator=(const Pointer<TYPES...>& other) noexcept;

	public:
		SLIB_CONSTEXPR sl_bool equals(sl_null_t) const
		{
			return !_ptr;
		}

		template <class OTHER>
		SLIB_CONSTEXPR sl_bool equals(const OTHER* other) const
		{
			return _ptr == other;
		}

		template <class OTHER>
		SLIB_CONSTEXPR sl_bool equals(const Ref<OTHER>& other) const
		{
			return _ptr == other.ptr;
		}

		template <class OTHER>
		SLIB_CONSTEXPR sl_bool equals(const AtomicRef<OTHER>& other) const
		{
			return _ptr == other._ptr;
		}

		SLIB_CONSTEXPR sl_compare_result compare(sl_null_t) const
		{
			return _ptr != sl_null;
		}

		template <class OTHER>
		SLIB_CONSTEXPR sl_compare_result compare(const OTHER* other) const
		{
			return ComparePrimitiveValues(_ptr, other);
		}

		template <class OTHER>
		SLIB_CONSTEXPR sl_compare_result compare(const Ref<OTHER>& other) const
		{
			return ComparePrimitiveValues(_ptr, other.ptr);
		}

		template <class OTHER>
		SLIB_CONSTEXPR sl_compare_result compare(const AtomicRef<OTHER>& other) const
		{
			return ComparePrimitiveValues(_ptr, other._ptr);
		}

		SLIB_DEFINE_CLASS_DEFAULT_COMPARE_OPERATORS_CONSTEXPR

	public:
		Ref<T> operator*() const noexcept
		{
			return *this;
		}

		SLIB_CONSTEXPR explicit operator sl_bool() const
		{
			return _ptr != sl_null;
		}

	public:
		T* _retain() const noexcept
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

		void _replace(T* other) noexcept
		{
			m_lock.lock();
			T* before = _ptr;
			_ptr = other;
			m_lock.unlock();
			if (before) {
				before->decreaseReference();
			}
		}

		void _copy(T* other) noexcept
		{
			if (_ptr != other) {
				if (other) {
					other->increaseReference();
				}
				_replace(other);
			}
		}

		T* _release() noexcept
		{
			if (!_ptr) {
				return sl_null;
			}
			m_lock.lock();
			T* before = _ptr;
			_ptr = sl_null;
			m_lock.unlock();
			return before;
		}

	public:
		T* _ptr;

	private:
		SpinLock m_lock;

	};


	class SLIB_EXPORT CWeakRef : public CRef
	{
		SLIB_DECLARE_OBJECT

	public:
		CWeakRef() noexcept;

		~CWeakRef();

	public:
		CRef* m_object;
		SpinLock m_lock;

	public:
		static CWeakRef* create(CRef* object) noexcept;

	public:
		Ref<CRef> lock() noexcept;

		void release() noexcept;

	};

	template <class T>
	class SLIB_EXPORT WeakRef
	{
	public:
		SLIB_CONSTEXPR WeakRef() = default;

		SLIB_CONSTEXPR WeakRef(sl_null_t) {}

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
		WeakRef(AtomicWeakRef<OTHER>&& other) noexcept : _weak(Move(other._weak))
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

		~WeakRef()
		{
			SLIB_TRY_CONVERT_TYPE(T*, CRef*)
		}

	public:
		static const WeakRef& null() noexcept
		{
			return *(reinterpret_cast<WeakRef const*>(&(priv::ref::g_null)));
		}

		SLIB_CONSTEXPR sl_bool isNull() const
		{
			return _weak.isNull();
		}

		SLIB_CONSTEXPR sl_bool isNotNull() const
		{
			return _weak.isNotNull();
		}

		void setNull() noexcept
		{
			_weak.setNull();
		}

		Ref<T> lock() const noexcept
		{
			if (_weak.isNotNull()) {
				return Ref<T>::cast(_weak->lock());
			}
			return sl_null;
		}

		SLIB_DEFINE_CAST_REF_FUNCTIONS(class OTHER, WeakRef, WeakRef<OTHER>)

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
			_weak = Move(other._weak);
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
			_weak = Move(other._weak);
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
		WeakRef& operator=(AtomicWeakRef<OTHER>&& other) noexcept
		{
			SLIB_TRY_CONVERT_TYPE(OTHER*, T*)
			_weak = Move(other._weak);
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
		SLIB_CONSTEXPR sl_bool equals(const WeakRef<OTHER>& other) const
		{
			return _weak.equals(other._weak);
		}

		template <class OTHER>
		SLIB_CONSTEXPR sl_bool equals(const AtomicWeakRef<OTHER>& other) const
		{
			return _weak.equals(other._weak);
		}

		template <class OTHER>
		SLIB_CONSTEXPR sl_compare_result compare(const WeakRef<OTHER>& other) const
		{
			return _weak.compare(other._weak);
		}

		template <class OTHER>
		SLIB_CONSTEXPR sl_compare_result compare(const AtomicWeakRef<OTHER>& other) const
		{
			return _weak.compare(other._weak);
		}

		SLIB_DEFINE_CLASS_DEFAULT_COMPARE_OPERATORS_CONSTEXPR

	public:
		SLIB_CONSTEXPR explicit operator sl_bool() const
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
		SLIB_CONSTEXPR Atomic() = default;

		Atomic(sl_null_t) noexcept {}

		Atomic(T* _other) noexcept
		{
			_set(_other);
		}

		Atomic(Atomic&& other) noexcept : _weak(Move(other._weak)) {}

		Atomic(const Atomic& other) noexcept: _weak(other._weak) {}

		template <class OTHER>
		Atomic(AtomicWeakRef<OTHER>&& other) noexcept : _weak(Move(other._weak))
		{
			SLIB_TRY_CONVERT_TYPE(OTHER*, T*)
		}

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

		~Atomic()
		{
			SLIB_TRY_CONVERT_TYPE(T*, CRef*)
		}

	public:
		SLIB_CONSTEXPR sl_bool isNull() const
		{
			return _weak.isNull();
		}

		SLIB_CONSTEXPR sl_bool isNotNull() const
		{
			return _weak.isNotNull();
		}

		void setNull() noexcept
		{
			_weak.setNull();
		}

		Ref<T> lock() const noexcept
		{
			Ref<CWeakRef> weak(_weak);
			if (weak.isNotNull()) {
				return Ref<T>::cast(weak->lock());
			}
			return sl_null;
		}

		WeakRef<T> release() noexcept
		{
			return reinterpret_cast<WeakRef<T>&&>(_weak.release());
		}

		SLIB_DEFINE_CAST_REF_FUNCTIONS(class OTHER, Atomic, AtomicWeakRef<OTHER>)

	public:
		Atomic& operator=(sl_null_t) noexcept
		{
			_weak.setNull();
			return *this;
		}

		Atomic& operator=(T* _other) noexcept
		{
			_set(_other);
			return *this;
		}

		Atomic& operator=(Atomic&& other) noexcept
		{
			_weak = Move(other._weak);
			return *this;
		}

		Atomic& operator=(const Atomic& other) noexcept
		{
			_weak = other._weak;
			return *this;
		}

		template <class OTHER>
		Atomic& operator=(AtomicWeakRef<OTHER>&& other) noexcept
		{
			SLIB_TRY_CONVERT_TYPE(OTHER*, T*)
			_weak = Move(other._weak);
			return *this;
		}

		template <class OTHER>
		Atomic& operator=(const AtomicWeakRef<OTHER>& other) noexcept
		{
			SLIB_TRY_CONVERT_TYPE(OTHER*, T*)
			_weak = other._weak;
			return *this;
		}

		template <class OTHER>
		Atomic& operator=(WeakRef<OTHER>&& other) noexcept
		{
			SLIB_TRY_CONVERT_TYPE(OTHER*, T*)
			_weak = Move(other._weak);
			return *this;
		}

		template <class OTHER>
		Atomic& operator=(const WeakRef<OTHER>& other) noexcept
		{
			SLIB_TRY_CONVERT_TYPE(OTHER*, T*)
			_weak = other._weak;
			return *this;
		}

		template <class OTHER>
		Atomic& operator=(const Ref<OTHER>& other) noexcept
		{
			SLIB_TRY_CONVERT_TYPE(OTHER*, T*)
			_set(other.ptr);
			return *this;
		}

		template <class OTHER>
		Atomic& operator=(const AtomicRef<OTHER>& _other) noexcept
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
		SLIB_CONSTEXPR sl_bool equals(const WeakRef<OTHER>& other) const
		{
			return _weak.equals(other._weak);
		}

		template <class OTHER>
		SLIB_CONSTEXPR sl_bool equals(const AtomicWeakRef<OTHER>& other) const
		{
			return _weak.equals(other._weak);
		}

		template <class OTHER>
		SLIB_CONSTEXPR sl_compare_result compare(const WeakRef<OTHER>& other) const
		{
			return _weak.compare(other._weak);
		}

		template <class OTHER>
		SLIB_CONSTEXPR sl_compare_result compare(const AtomicWeakRef<OTHER>& other) const
		{
			return _weak.compare(other._weak);
		}

		SLIB_DEFINE_CLASS_DEFAULT_COMPARE_OPERATORS_CONSTEXPR

	public:
		WeakRef<T> operator*() const noexcept
		{
			return *this;
		}

		SLIB_CONSTEXPR explicit operator sl_bool() const
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
	constexpr sl_bool operator==(sl_null_t, const Ref<T>& b)
	{
		return !(b.ptr);
	}

	template <class T>
	constexpr sl_bool operator==(T* a, const Ref<T>& b)
	{
		return a == b.ptr;
	}

	template <class T>
	constexpr sl_bool operator!=(sl_null_t, const Ref<T>& b)
	{
		return b.ptr != sl_null;
	}

	template <class T>
	constexpr sl_bool operator!=(T* a, const Ref<T>& b)
	{
		return a != b.ptr;
	}

	template <class T>
	constexpr sl_bool operator==(sl_null_t, const AtomicRef<T>& b)
	{
		return !(b._ptr);
	}

	template <class T>
	constexpr sl_bool operator==(T* a, const AtomicRef<T>& b)
	{
		return a == b._ptr;
	}

	template <class T>
	constexpr sl_bool operator!=(sl_null_t, const AtomicRef<T>& b)
	{
		return b._ptr != sl_null;
	}

	template <class T>
	constexpr sl_bool operator!=(T* a, const AtomicRef<T>& b)
	{
		return a != b._ptr;
	}


	template <class T>
	SLIB_INLINE static Ref<T> New()
	{
		return new T;
	}

	template <class T, class ARG, class... ARGS>
	SLIB_INLINE static Ref<T> New(ARG&& arg, ARGS&&... args)
	{
		Ref<T> o = new T;
		if (o.isNotNull()) {
			o->init(Forward<ARG>(arg), Forward<ARGS>(args)...);
			return o;
		}
		return sl_null;
	}

	template <class T, class OTHER>
	SLIB_INLINE static sl_bool IsInstanceOf(const OTHER* object) noexcept
	{
		SLIB_TRY_CONVERT_TYPE(OTHER*, CRef*)
		if (object) {
			return object->isInstanceOf(T::ObjectType());
		}
		return sl_false;
	}

	template <class T, class OTHER>
	SLIB_INLINE static sl_bool IsInstanceOf(const Ref<OTHER>& object) noexcept
	{
		SLIB_TRY_CONVERT_TYPE(OTHER*, CRef*)
		if (object.isNotNull()) {
			return object->isInstanceOf(T::ObjectType());
		}
		return sl_false;
	}

	template <class T, class OTHER>
	SLIB_INLINE static T* CastInstance(OTHER* object) noexcept
	{
		SLIB_TRY_CONVERT_TYPE(OTHER*, CRef*)
		if (object) {
			if (object->isInstanceOf(T::ObjectType())) {
				return static_cast<T*>(object);
			}
		}
		return sl_null;
	}

	template <class T, class OTHER>
	SLIB_INLINE static const Ref<T>& CastRef(const Ref<OTHER>& object) noexcept
	{
		SLIB_TRY_CONVERT_TYPE(OTHER*, CRef*)
		if (object.isNotNull()) {
			if (object->isInstanceOf(T::ObjectType())) {
				return *(reinterpret_cast<Ref<T> const*>(&object));
			}
		}
		return Ref<T>::null();
	}

	template <class T, class OTHER>
	SLIB_INLINE static Ref<T> CastRef(Ref<OTHER>&& object) noexcept
	{
		SLIB_TRY_CONVERT_TYPE(OTHER*, CRef*)
		if (object.isNotNull()) {
			if (object->isInstanceOf(T::ObjectType())) {
				return Move(*(reinterpret_cast<Ref<T>*>(&object)));
			}
		}
		return sl_null;
	}

	template <class T, class OTHER>
	SLIB_INLINE static const Ref<T>& CastRef(const Ref<OTHER>& object, const Ref<T>& def) noexcept
	{
		SLIB_TRY_CONVERT_TYPE(OTHER*, CRef*)
		if (object.isNotNull()) {
			if (object->isInstanceOf(T::ObjectType())) {
				return *(reinterpret_cast<Ref<T> const*>(&object));
			}
		}
		return def;
	}
	
	template <class T, class OTHER, class DEF>
	SLIB_INLINE static const Ref<T> CastRef(Ref<OTHER>&& object, DEF&& def) noexcept
	{
		SLIB_TRY_CONVERT_TYPE(OTHER*, CRef*)
		if (object.isNotNull()) {
			if (object->isInstanceOf(T::ObjectType())) {
				return Move(*(reinterpret_cast<Ref<T>*>(&object)));
			}
		}
		return Forward<DEF>(def);
	}

	template <class T>
	SLIB_INLINE static Ref<T> ToRef(T* other) noexcept
	{
		return Ref<T>(other);
	}

	template <class T>
	SLIB_INLINE static const Ref<T>& ToRef(T** other) noexcept
	{
		return *(reinterpret_cast<Ref<T>*>(other));
	}

	template <class T>
	SLIB_INLINE static const Ref<T>& ToRef(const Ref<T>& other) noexcept
	{
		return other;
	}

	template <class T>
	SLIB_INLINE static Ref<T>&& ToRef(Ref<T>&& other) noexcept
	{
		return static_cast<Ref<T>&&>(other);
	}

	template <class T>
	SLIB_INLINE static Ref<T> ToRef(const AtomicRef<T>& other) noexcept
	{
		return Ref<T>(other);
	}

	template <class T>
	SLIB_INLINE static Ref<T> ToRef(const WeakRef<T>& other) noexcept
	{
		return Ref<T>(other);
	}

	template <class T>
	SLIB_INLINE static Ref<T> ToRef(const AtomicWeakRef<T>& other) noexcept
	{
		return Ref<T>(other);
	}

	template <class T>
	SLIB_INLINE static WeakRef<T> ToWeakRef(T* other) noexcept
	{
		return WeakRef<T>(other);
	}

	template <class T>
	SLIB_INLINE static WeakRef<T> ToWeakRef(const Ref<T>& other) noexcept
	{
		return WeakRef<T>(other);
	}

	template <class T>
	SLIB_INLINE static WeakRef<T> ToWeakRef(const AtomicRef<T>& other) noexcept
	{
		return WeakRef<T>(other);
	}

	template <class T>
	SLIB_INLINE static const WeakRef<T>& ToWeakRef(const WeakRef<T>& other) noexcept
	{
		return other;
	}

	template <class T>
	SLIB_INLINE static WeakRef<T>&& ToWeakRef(WeakRef<T>&& other) noexcept
	{
		return static_cast<WeakRef<T>&&>(other);
	}

	template <class T>
	SLIB_INLINE static WeakRef<T> ToWeakRef(const AtomicWeakRef<T>& other) noexcept
	{
		return WeakRef<T>(other);
	}


	template <class T>
	class CRefT : public CRef, public T
	{
	public:
		template <class... ARGS>
		CRefT(ARGS&&... args): T(Forward<ARGS>(args)...) {}

	};

	template <class T>
	using RefT = Ref< CRefT<T> >;

	template <class T>
	SLIB_INLINE static RefT<typename RemoveConstReference<T>::Type> ToRefT(T&& t)
	{
		return new CRefT<typename RemoveConstReference<T>::Type>(Forward<T>(t));
	}

	template <class T, class... ARGS>
	SLIB_INLINE static RefT<T> NewRefT(ARGS&&... args)
	{
		return new CRefT<T>(Forward<ARGS>(args)...);
	}

}

#endif
