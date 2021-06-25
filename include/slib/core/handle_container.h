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

#define SLIB_DEFINE_HANDLE_CONTAINER_BASE_MEMBERS(CLASS, HANDLE_TYPE, MEMBER_NAME, HANDLE_INVALID, HANDLE_DELETER) \
	HANDLE_TYPE MEMBER_NAME; \
public: \
	constexpr CLASS(): MEMBER_NAME(HANDLE_INVALID) {} \
	constexpr CLASS(HANDLE_TYPE other): MEMBER_NAME(other) {} \
	CLASS(const CLASS&) = delete; \
	CLASS(CLASS&& other) noexcept \
	{ \
		MEMBER_NAME = other.MEMBER_NAME; \
		other.MEMBER_NAME = HANDLE_INVALID; \
	} \
	~CLASS() \
	{ \
		if (MEMBER_NAME != HANDLE_INVALID) { \
			HANDLE_DELETER (MEMBER_NAME); \
		} \
	} \
	CLASS& operator=(const CLASS&) = delete; \
	CLASS& operator=(CLASS&& other) \
	{ \
		if (MEMBER_NAME != HANDLE_INVALID) { \
			HANDLE_DELETER (MEMBER_NAME); \
		} \
		MEMBER_NAME = other.MEMBER_NAME; \
		other.MEMBER_NAME = HANDLE_INVALID; \
		return *this; \
	}

#define SLIB_DEFINE_HANDLE_CONTAINER_MEMBERS(CLASS, HANDLE_TYPE, MEMBER_NAME, HANDLE_INVALID, HANDLE_DELETER) \
	SLIB_DEFINE_HANDLE_CONTAINER_BASE_MEMBERS(CLASS, HANDLE_TYPE, MEMBER_NAME, HANDLE_INVALID, HANDLE_DELETER) \
	constexpr explicit operator sl_bool() const \
	{ \
		return MEMBER_NAME != HANDLE_INVALID; \
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
		if (MEMBER_NAME != HANDLE_INVALID) { \
			HANDLE_DELETER (MEMBER_NAME); \
		} \
		MEMBER_NAME = other; \
	} \
	HANDLE_TYPE release() noexcept \
	{ \
		HANDLE_TYPE ret = MEMBER_NAME; \
		MEMBER_NAME = HANDLE_INVALID; \
		return ret; \
	} \
	CLASS& operator=(HANDLE_TYPE other) \
	{ \
		set(other); \
		return *this; \
	}

#define SLIB_DEFINE_NULLABLE_HANDLE_CONTAINER_MEMBERS(CLASS, HANDLE_TYPE, MEMBER_NAME, HANDLE_DELETER) \
	SLIB_DEFINE_HANDLE_CONTAINER_MEMBERS(CLASS, HANDLE_TYPE, MEMBER_NAME, sl_null, HANDLE_DELETER) \
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


#define SLIB_DEFINE_ATOMIC_HANDLE_CONTAINER_BASE_MEMBERS(CLASS, HANDLE_TYPE, MEMBER_NAME, HANDLE_INVALID, HANDLE_DELETER) \
	HANDLE_TYPE MEMBER_NAME; \
private: \
	SpinLock _lock; \
public: \
	constexpr Atomic(): MEMBER_NAME(HANDLE_INVALID) {} \
	constexpr Atomic(HANDLE_TYPE other): MEMBER_NAME(other) {} \
	Atomic(const Atomic&) = delete; \
	Atomic(CLASS&& _other) noexcept \
	{ \
		Atomic& other = *(reinterpret_cast<Atomic*>(&_other)); \
		MEMBER_NAME = other.MEMBER_NAME; \
		other.MEMBER_NAME = HANDLE_INVALID; \
	} \
	~Atomic() \
	{ \
		if (MEMBER_NAME != HANDLE_INVALID) { \
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
			other.MEMBER_NAME = HANDLE_INVALID; \
		} \
		if (old != HANDLE_INVALID) { \
			HANDLE_DELETER (old); \
		} \
		return *this; \
	}

#define SLIB_DEFINE_ATOMIC_HANDLE_CONTAINER_MEMBERS(CLASS, HANDLE_TYPE, MEMBER_NAME, HANDLE_INVALID, HANDLE_DELETER) \
	SLIB_DEFINE_ATOMIC_HANDLE_CONTAINER_BASE_MEMBERS(CLASS, HANDLE_TYPE, MEMBER_NAME, HANDLE_INVALID, HANDLE_DELETER) \
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
		if (old != HANDLE_INVALID) { \
			HANDLE_DELETER (old); \
		} \
	} \
	HANDLE_TYPE release() noexcept \
	{ \
		HANDLE_TYPE ret; \
		{ \
			SpinLocker locker(&_lock); \
			ret = MEMBER_NAME; \
			MEMBER_NAME = HANDLE_INVALID; \
		} \
		return ret; \
	} \
	Atomic& operator=(HANDLE_TYPE other) \
	{ \
		set(other); \
		return *this; \
	}

#define SLIB_DEFINE_ATOMIC_NULLABLE_HANDLE_CONTAINER_MEMBERS(CLASS, HANDLE_TYPE, MEMBER_NAME, HANDLE_DELETER) \
	SLIB_DEFINE_ATOMIC_HANDLE_CONTAINER_MEMBERS(CLASS, HANDLE_TYPE, MEMBER_NAME, sl_null, HANDLE_DELETER) \
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

#endif
