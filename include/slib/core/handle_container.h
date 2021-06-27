/*
 *   Copyright (c) 2008-2021 SLIBIO <https://github.com/SLIBIO>
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

#ifndef CHECKHEADER_SLIB_CORE_HANDLE_CONTAINER
#define CHECKHEADER_SLIB_CORE_HANDLE_CONTAINER

#include "atomic.h"

#define SLIB_DEFINE_HANDLE_CONTAINER_TEMPLATE_MEMBERS(CLASS, HANDLE_TYPE, MEMBER_NAME, HANDLE_NONE, HANDLE_DELETER) \
	HANDLE_TYPE MEMBER_NAME; \
public: \
	constexpr CLASS(): MEMBER_NAME(HANDLE_NONE) {} \
	constexpr CLASS(HANDLE_TYPE other): MEMBER_NAME(other) {} \
	CLASS(const CLASS&) = delete; \
	CLASS(CLASS&& other) noexcept \
	{ \
		MEMBER_NAME = other.MEMBER_NAME; \
		other.MEMBER_NAME = HANDLE_NONE; \
	} \
	~CLASS() \
	{ \
		if (MEMBER_NAME != HANDLE_NONE) { \
			HANDLE_DELETER (MEMBER_NAME); \
		} \
	} \
	CLASS& operator=(const CLASS&) = delete; \
	CLASS& operator=(CLASS&& other) \
	{ \
		if (MEMBER_NAME != HANDLE_NONE) { \
			HANDLE_DELETER (MEMBER_NAME); \
		} \
		MEMBER_NAME = other.MEMBER_NAME; \
		other.MEMBER_NAME = HANDLE_NONE; \
		return *this; \
	} \
	constexpr explicit operator sl_bool() const \
	{ \
		return MEMBER_NAME != HANDLE_NONE; \
	} \
	constexpr operator HANDLE_TYPE() const& \
	{ \
		return MEMBER_NAME; \
	} \
	operator HANDLE_TYPE() && = delete; \
	constexpr HANDLE_TYPE operator->() const& \
	{ \
		return MEMBER_NAME; \
	} \
	HANDLE_TYPE operator->() && = delete; \
	constexpr HANDLE_TYPE get() const& \
	{ \
		return MEMBER_NAME; \
	} \
	HANDLE_TYPE get() && = delete; \
	void set(HANDLE_TYPE other) \
	{ \
		if (MEMBER_NAME != HANDLE_NONE) { \
			HANDLE_DELETER (MEMBER_NAME); \
		} \
		MEMBER_NAME = other; \
	} \
	constexpr sl_bool isNone() const \
	{ \
		return MEMBER_NAME == HANDLE_NONE; \
	} \
	constexpr sl_bool isNotNone() const \
	{ \
		return MEMBER_NAME != HANDLE_NONE; \
	} \
	void setNone() \
	{ \
		set(HANDLE_NONE); \
	} \
	HANDLE_TYPE release() noexcept \
	{ \
		HANDLE_TYPE ret = MEMBER_NAME; \
		MEMBER_NAME = HANDLE_NONE; \
		return ret; \
	} \
	CLASS& operator=(HANDLE_TYPE other) \
	{ \
		set(other); \
		return *this; \
	}

#define SLIB_DEFINE_NULLABLE_HANDLE_CONTAINER_TEMPLATE_MEMBERS(CLASS, HANDLE_TYPE, MEMBER_NAME, HANDLE_DELETER) \
	SLIB_DEFINE_HANDLE_CONTAINER_TEMPLATE_MEMBERS(CLASS, HANDLE_TYPE, MEMBER_NAME, sl_null, HANDLE_DELETER) \
	constexpr CLASS(sl_null_t): MEMBER_NAME(sl_null) {} \
	CLASS& operator=(sl_null_t) \
	{ \
		set(sl_null); \
		return *this; \
	} \
	constexpr sl_bool isNull() const \
	{ \
		return !MEMBER_NAME; \
	} \
	constexpr sl_bool isNotNull() const \
	{ \
		return MEMBER_NAME != sl_null; \
	} \
	void setNull() \
	{ \
		set(sl_null); \
	}


#define SLIB_DEFINE_ATOMIC_HANDLE_CONTAINER_TEMPLATE_MEMBERS(CLASS, HANDLE_TYPE, MEMBER_NAME, HANDLE_NONE, HANDLE_DELETER) \
	HANDLE_TYPE MEMBER_NAME; \
private: \
	SpinLock _lock; \
public: \
	constexpr Atomic(): MEMBER_NAME(HANDLE_NONE) {} \
	constexpr Atomic(HANDLE_TYPE other): MEMBER_NAME(other) {} \
	Atomic(const Atomic&) = delete; \
	Atomic(CLASS&& _other) noexcept \
	{ \
		Atomic& other = *(reinterpret_cast<Atomic*>(&_other)); \
		MEMBER_NAME = other.MEMBER_NAME; \
		other.MEMBER_NAME = HANDLE_NONE; \
	} \
	~Atomic() \
	{ \
		if (MEMBER_NAME != HANDLE_NONE) { \
			HANDLE_DELETER (MEMBER_NAME); \
		} \
	} \
	Atomic& operator=(const Atomic&) = delete; \
	Atomic& operator=(CLASS&& _other) \
	{ \
		Atomic& other = *(reinterpret_cast<Atomic*>(&_other)); \
		HANDLE_TYPE old = MEMBER_NAME; \
		{ \
			SpinLocker locker(&_lock); \
			MEMBER_NAME = other.MEMBER_NAME; \
			other.MEMBER_NAME = HANDLE_NONE; \
		} \
		if (old != HANDLE_NONE) { \
			HANDLE_DELETER (old); \
		} \
		return *this; \
	} \
	constexpr explicit operator sl_bool() const \
	{ \
		return MEMBER_NAME != sl_null; \
	} \
	constexpr HANDLE_TYPE get() const& \
	{ \
		return MEMBER_NAME; \
	} \
	HANDLE_TYPE get() && = delete; \
	void set(HANDLE_TYPE other) \
	{ \
		HANDLE_TYPE old; \
		{ \
			SpinLocker locker(&_lock); \
			old = MEMBER_NAME; \
			MEMBER_NAME = other; \
		} \
		if (old != HANDLE_NONE) { \
			HANDLE_DELETER (old); \
		} \
	} \
	constexpr sl_bool isNone() const \
	{ \
		return MEMBER_NAME == HANDLE_NONE; \
	} \
	constexpr sl_bool isNotNone() const \
	{ \
		return MEMBER_NAME != HANDLE_NONE; \
	} \
	void setNone() \
	{ \
		set(HANDLE_NONE); \
	} \
	HANDLE_TYPE release() noexcept \
	{ \
		HANDLE_TYPE ret; \
		{ \
			SpinLocker locker(&_lock); \
			ret = MEMBER_NAME; \
			MEMBER_NAME = HANDLE_NONE; \
		} \
		return ret; \
	} \
	Atomic& operator=(HANDLE_TYPE other) \
	{ \
		set(other); \
		return *this; \
	}

#define SLIB_DEFINE_ATOMIC_NULLABLE_HANDLE_CONTAINER_TEMPLATE_MEMBERS(CLASS, HANDLE_TYPE, MEMBER_NAME, HANDLE_DELETER) \
	SLIB_DEFINE_ATOMIC_HANDLE_CONTAINER_TEMPLATE_MEMBERS(CLASS, HANDLE_TYPE, MEMBER_NAME, sl_null, HANDLE_DELETER) \
	constexpr Atomic(sl_null_t): MEMBER_NAME(sl_null) {} \
	Atomic& operator=(sl_null_t) \
	{ \
		set(sl_null); \
		return *this; \
	} \
	constexpr sl_bool isNull() const \
	{ \
		return !MEMBER_NAME; \
	} \
	constexpr sl_bool isNotNull() const \
	{ \
		return MEMBER_NAME != sl_null; \
	} \
	void setNull() \
	{ \
		set(sl_null); \
	}


#define SLIB_DECLARE_HANDLE_CONTAINER_MEMBERS(CLASS, HANDLE_TYPE, MEMBER_NAME, HANDLE_NONE) \
	HANDLE_TYPE MEMBER_NAME; \
public: \
	constexpr CLASS(): MEMBER_NAME(HANDLE_NONE) {} \
	constexpr CLASS(HANDLE_TYPE other): MEMBER_NAME(other) {} \
	CLASS(const CLASS&) = delete; \
	CLASS(CLASS&& other) noexcept; \
	~CLASS(); \
	CLASS& operator=(const CLASS&) = delete; \
	CLASS& operator=(CLASS&& other); \
	constexpr explicit operator sl_bool() const \
	{ \
		return MEMBER_NAME != HANDLE_NONE; \
	} \
	constexpr operator HANDLE_TYPE() const& \
	{ \
		return MEMBER_NAME; \
	} \
	operator HANDLE_TYPE() && = delete; \
	constexpr HANDLE_TYPE operator->() const& \
	{ \
		return MEMBER_NAME; \
	} \
	HANDLE_TYPE operator->() && = delete; \
	constexpr HANDLE_TYPE get() const& \
	{ \
		return MEMBER_NAME; \
	} \
	HANDLE_TYPE get() && = delete; \
	void set(HANDLE_TYPE other); \
	constexpr sl_bool isNone() const \
	{ \
		return MEMBER_NAME == HANDLE_NONE; \
	} \
	constexpr sl_bool isNotNone() const \
	{ \
		return MEMBER_NAME != HANDLE_NONE; \
	} \
	void setNone(); \
	HANDLE_TYPE release() noexcept; \
	CLASS& operator=(HANDLE_TYPE other);

#define SLIB_DECLARE_NULLABLE_HANDLE_CONTAINER_MEMBERS(CLASS, HANDLE_TYPE, MEMBER_NAME) \
	SLIB_DECLARE_HANDLE_CONTAINER_MEMBERS(CLASS, HANDLE_TYPE, MEMBER_NAME, sl_null) \
	constexpr CLASS(sl_null_t): MEMBER_NAME(sl_null) {} \
	CLASS& operator=(sl_null_t); \
	constexpr sl_bool isNull() const \
	{ \
		return MEMBER_NAME == sl_null; \
	} \
	constexpr sl_bool isNotNull() const \
	{ \
		return MEMBER_NAME != sl_null; \
	} \
	void setNull();

#define SLIB_DEFINE_HANDLE_CONTAINER_MEMBERS(CLASS, HANDLE_TYPE, MEMBER_NAME, HANDLE_NONE, HANDLE_DELETER) \
	CLASS::CLASS(CLASS&& other) noexcept \
	{ \
		MEMBER_NAME = other.MEMBER_NAME; \
		other.MEMBER_NAME = HANDLE_NONE; \
	} \
	CLASS::~CLASS() \
	{ \
		if (MEMBER_NAME != HANDLE_NONE) { \
			HANDLE_DELETER (MEMBER_NAME); \
		} \
	} \
	CLASS& CLASS::operator=(CLASS&& other) \
	{ \
		if (MEMBER_NAME != HANDLE_NONE) { \
			HANDLE_DELETER (MEMBER_NAME); \
		} \
		MEMBER_NAME = other.MEMBER_NAME; \
		other.MEMBER_NAME = HANDLE_NONE; \
		return *this; \
	} \
	void CLASS::set(HANDLE_TYPE other) \
	{ \
		if (MEMBER_NAME != HANDLE_NONE) { \
			HANDLE_DELETER (MEMBER_NAME); \
		} \
		MEMBER_NAME = other; \
	} \
	void CLASS::setNone() \
	{ \
		set(HANDLE_NONE); \
	} \
	HANDLE_TYPE CLASS::release() noexcept \
	{ \
		HANDLE_TYPE ret = MEMBER_NAME; \
		MEMBER_NAME = HANDLE_NONE; \
		return ret; \
	} \
	CLASS& CLASS::operator=(HANDLE_TYPE other) \
	{ \
		set(other); \
		return *this; \
	}

#define SLIB_DEFINE_NULLABLE_HANDLE_CONTAINER_MEMBERS(CLASS, HANDLE_TYPE, MEMBER_NAME, HANDLE_DELETER) \
	SLIB_DEFINE_HANDLE_CONTAINER_MEMBERS(CLASS, HANDLE_TYPE, MEMBER_NAME, sl_null, HANDLE_DELETER) \
	CLASS& CLASS::operator=(sl_null_t) \
	{ \
		set(sl_null); \
		return *this; \
	} \
	void CLASS::setNull() \
	{ \
		set(sl_null); \
	}


#define SLIB_DECLARE_ATOMIC_HANDLE_CONTAINER_MEMBERS(CLASS, HANDLE_TYPE, MEMBER_NAME, HANDLE_NONE) \
	HANDLE_TYPE MEMBER_NAME; \
private: \
	SpinLock _lock; \
public: \
	constexpr Atomic(): MEMBER_NAME(HANDLE_NONE) {} \
	constexpr Atomic(HANDLE_TYPE other): MEMBER_NAME(other) {} \
	Atomic(const Atomic&) = delete; \
	Atomic(CLASS&& _other) noexcept; \
	~Atomic(); \
	Atomic& operator=(const Atomic&) = delete; \
	Atomic& operator=(CLASS&& _other); \
	constexpr explicit operator sl_bool() const \
	{ \
		return MEMBER_NAME != sl_null; \
	} \
	constexpr HANDLE_TYPE get() const& \
	{ \
		return MEMBER_NAME; \
	} \
	HANDLE_TYPE get() && = delete; \
	void set(HANDLE_TYPE other); \
	constexpr sl_bool isNone() const \
	{ \
		return MEMBER_NAME == HANDLE_NONE; \
	} \
	constexpr sl_bool isNotNone() const \
	{ \
		return MEMBER_NAME != HANDLE_NONE; \
	} \
	void setNone(); \
	HANDLE_TYPE release() noexcept; \
	Atomic& operator=(HANDLE_TYPE other);

#define SLIB_DECLARE_ATOMIC_NULLABLE_HANDLE_CONTAINER_MEMBERS(CLASS, HANDLE_TYPE, MEMBER_NAME) \
	SLIB_DECLARE_ATOMIC_HANDLE_CONTAINER_MEMBERS(CLASS, HANDLE_TYPE, MEMBER_NAME, sl_null) \
	constexpr Atomic(sl_null_t): MEMBER_NAME(sl_null) {} \
	Atomic& operator=(sl_null_t); \
	constexpr sl_bool isNull() const \
	{ \
		return MEMBER_NAME == sl_null; \
	} \
	constexpr sl_bool isNotNull() const \
	{ \
		return MEMBER_NAME != sl_null; \
	} \
	void setNull();

#define SLIB_DEFINE_ATOMIC_HANDLE_CONTAINER_MEMBERS(CLASS, HANDLE_TYPE, MEMBER_NAME, HANDLE_NONE, HANDLE_DELETER) \
	Atomic<CLASS>::Atomic(CLASS&& _other) noexcept \
	{ \
		Atomic& other = *(reinterpret_cast<Atomic*>(&_other)); \
		MEMBER_NAME = other.MEMBER_NAME; \
		other.MEMBER_NAME = HANDLE_NONE; \
	} \
	Atomic<CLASS>::~Atomic() \
	{ \
		if (MEMBER_NAME != HANDLE_NONE) { \
			HANDLE_DELETER (MEMBER_NAME); \
		} \
	} \
	Atomic<CLASS>& Atomic<CLASS>::operator=(CLASS&& _other) \
	{ \
		Atomic& other = *(reinterpret_cast<Atomic*>(&_other)); \
		HANDLE_TYPE old = MEMBER_NAME; \
		{ \
			SpinLocker locker(&_lock); \
			MEMBER_NAME = other.MEMBER_NAME; \
			other.MEMBER_NAME = HANDLE_NONE; \
		} \
		if (old != HANDLE_NONE) { \
			HANDLE_DELETER (old); \
		} \
		return *this; \
	} \
	void Atomic<CLASS>::set(HANDLE_TYPE other) \
	{ \
		HANDLE_TYPE old; \
		{ \
			SpinLocker locker(&_lock); \
			old = MEMBER_NAME; \
			MEMBER_NAME = other; \
		} \
		if (old != HANDLE_NONE) { \
			HANDLE_DELETER (old); \
		} \
	} \
	void Atomic<CLASS>::setNone() \
	{ \
		set(HANDLE_NONE); \
	} \
	HANDLE_TYPE Atomic<CLASS>::release() noexcept \
	{ \
		HANDLE_TYPE ret; \
		{ \
			SpinLocker locker(&_lock); \
			ret = MEMBER_NAME; \
			MEMBER_NAME = HANDLE_NONE; \
		} \
		return ret; \
	} \
	Atomic<CLASS>& Atomic<CLASS>::operator=(HANDLE_TYPE other) \
	{ \
		set(other); \
		return *this; \
	}

#define SLIB_DEFINE_ATOMIC_NULLABLE_HANDLE_CONTAINER_MEMBERS(CLASS, HANDLE_TYPE, MEMBER_NAME, HANDLE_DELETER) \
	SLIB_DEFINE_ATOMIC_HANDLE_CONTAINER_MEMBERS(CLASS, HANDLE_TYPE, MEMBER_NAME, sl_null, HANDLE_DELETER) \
	Atomic<CLASS>& Atomic<CLASS>::operator=(sl_null_t) \
	{ \
		set(sl_null); \
		return *this; \
	} \
	void Atomic<CLASS>::setNull() \
	{ \
		set(sl_null); \
	}

#endif
