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

#include "definition.h"

#define SLIB_DEFINE_HANDLE_CONTAINER_TEMPLATE_BASE_MEMBERS(CLASS, HANDLE_TYPE, MEMBER_NAME, HANDLE_NONE, HANDLE_DELETER) \
	HANDLE_TYPE MEMBER_NAME; \
public: \
	typedef HANDLE_TYPE HandleType; \
	SLIB_CONSTEXPR CLASS(): MEMBER_NAME(HANDLE_NONE) {} \
	SLIB_CONSTEXPR CLASS(HANDLE_TYPE other): MEMBER_NAME(other) {} \
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
	SLIB_CONSTEXPR explicit operator sl_bool() const \
	{ \
		return MEMBER_NAME != HANDLE_NONE; \
	} \
	SLIB_CONSTEXPR operator HANDLE_TYPE() const& \
	{ \
		return MEMBER_NAME; \
	} \
	operator HANDLE_TYPE() && = delete; \
	SLIB_CONSTEXPR HANDLE_TYPE get() const& \
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

#define SLIB_DEFINE_HANDLE_CONTAINER_TEMPLATE_MEMBERS(CLASS, HANDLE_TYPE, MEMBER_NAME, HANDLE_NONE, HANDLE_DELETER) \
	SLIB_DEFINE_HANDLE_CONTAINER_TEMPLATE_BASE_MEMBERS(CLASS, HANDLE_TYPE, MEMBER_NAME, HANDLE_NONE, HANDLE_DELETER) \
	SLIB_CONSTEXPR sl_bool isNone() const \
	{ \
		return MEMBER_NAME == HANDLE_NONE; \
	} \
	SLIB_CONSTEXPR sl_bool isNotNone() const \
	{ \
		return MEMBER_NAME != HANDLE_NONE; \
	} \
	void setNone() \
	{ \
		set(HANDLE_NONE); \
	}

#define SLIB_DEFINE_NULLABLE_HANDLE_CONTAINER_TEMPLATE_MEMBERS(CLASS, HANDLE_TYPE, MEMBER_NAME, HANDLE_DELETER) \
	SLIB_DEFINE_HANDLE_CONTAINER_TEMPLATE_BASE_MEMBERS(CLASS, HANDLE_TYPE, MEMBER_NAME, sl_null, HANDLE_DELETER) \
	SLIB_CONSTEXPR CLASS(sl_null_t): MEMBER_NAME(sl_null) {} \
	CLASS& operator=(sl_null_t) \
	{ \
		set(sl_null); \
		return *this; \
	} \
	SLIB_CONSTEXPR sl_bool isNull() const \
	{ \
		return !MEMBER_NAME; \
	} \
	SLIB_CONSTEXPR sl_bool isNotNull() const \
	{ \
		return MEMBER_NAME != sl_null; \
	} \
	void setNull() \
	{ \
		set(sl_null); \
	}

#define SLIB_DECLARE_HANDLE_CONTAINER_BASE_MEMBERS(CLASS, HANDLE_TYPE, MEMBER_NAME, HANDLE_NONE) \
	HANDLE_TYPE MEMBER_NAME; \
public: \
	typedef HANDLE_TYPE HandleType; \
	SLIB_CONSTEXPR CLASS(): MEMBER_NAME(HANDLE_NONE) {} \
	SLIB_CONSTEXPR CLASS(HANDLE_TYPE other): MEMBER_NAME(other) {} \
	CLASS(const CLASS&) = delete; \
	CLASS(CLASS&& other) noexcept; \
	~CLASS(); \
	CLASS& operator=(const CLASS&) = delete; \
	CLASS& operator=(CLASS&& other); \
	SLIB_CONSTEXPR explicit operator sl_bool() const \
	{ \
		return MEMBER_NAME != HANDLE_NONE; \
	} \
	SLIB_CONSTEXPR operator HANDLE_TYPE() const& \
	{ \
		return MEMBER_NAME; \
	} \
	operator HANDLE_TYPE() && = delete; \
	SLIB_CONSTEXPR HANDLE_TYPE get() const& \
	{ \
		return MEMBER_NAME; \
	} \
	HANDLE_TYPE get() && = delete; \
	void set(HANDLE_TYPE other); \
	HANDLE_TYPE release() noexcept; \
	CLASS& operator=(HANDLE_TYPE other);

#define SLIB_DECLARE_HANDLE_CONTAINER_MEMBERS(CLASS, HANDLE_TYPE, MEMBER_NAME, HANDLE_NONE) \
	SLIB_DECLARE_HANDLE_CONTAINER_BASE_MEMBERS(CLASS, HANDLE_TYPE, MEMBER_NAME, HANDLE_NONE) \
	SLIB_CONSTEXPR sl_bool isNone() const \
	{ \
		return MEMBER_NAME == HANDLE_NONE; \
	} \
	SLIB_CONSTEXPR sl_bool isNotNone() const \
	{ \
		return MEMBER_NAME != HANDLE_NONE; \
	} \
	void setNone(); \

#define SLIB_DECLARE_NULLABLE_HANDLE_CONTAINER_MEMBERS(CLASS, HANDLE_TYPE, MEMBER_NAME) \
	SLIB_DECLARE_HANDLE_CONTAINER_BASE_MEMBERS(CLASS, HANDLE_TYPE, MEMBER_NAME, sl_null) \
	SLIB_CONSTEXPR CLASS(sl_null_t): MEMBER_NAME(sl_null) {} \
	CLASS& operator=(sl_null_t); \
	SLIB_CONSTEXPR sl_bool isNull() const \
	{ \
		return !MEMBER_NAME; \
	} \
	SLIB_CONSTEXPR sl_bool isNotNull() const \
	{ \
		return MEMBER_NAME != sl_null; \
	} \
	void setNull(); \

#define SLIB_DEFINE_HANDLE_CONTAINER_BASE_MEMBERS(CLASS, HANDLE_TYPE, MEMBER_NAME, HANDLE_NONE, HANDLE_DELETER) \
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

#define SLIB_DEFINE_HANDLE_CONTAINER_MEMBERS(CLASS, HANDLE_TYPE, MEMBER_NAME, HANDLE_NONE, HANDLE_DELETER) \
	SLIB_DEFINE_HANDLE_CONTAINER_BASE_MEMBERS(CLASS, HANDLE_TYPE, MEMBER_NAME, HANDLE_NONE, HANDLE_DELETER) \
	void CLASS::setNone() \
	{ \
		set(HANDLE_NONE); \
	}

#define SLIB_DEFINE_NULLABLE_HANDLE_CONTAINER_MEMBERS(CLASS, HANDLE_TYPE, MEMBER_NAME, HANDLE_DELETER) \
	SLIB_DEFINE_HANDLE_CONTAINER_BASE_MEMBERS(CLASS, HANDLE_TYPE, MEMBER_NAME, sl_null, HANDLE_DELETER) \
	CLASS& CLASS::operator=(sl_null_t) \
	{ \
		set(sl_null); \
		return *this; \
	} \
	void CLASS::setNull() \
	{ \
		set(sl_null); \
	}


#endif
