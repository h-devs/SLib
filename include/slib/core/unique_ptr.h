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

#ifndef CHECKHEADER_SLIB_CORE_UNIQUE_PTR
#define CHECKHEADER_SLIB_CORE_UNIQUE_PTR

#include "definition.h"

namespace slib
{

#define SLIB_DEFINE_UNIQUE_PTR_MEMBERS_NO_ASSIGN(CLASS, POINTER_TYPE, POINTER_NAME, POINTER_NULL, FREE_POINTER) \
public: \
	POINTER_TYPE POINTER_NAME; \
	constexpr CLASS(): POINTER_NAME(POINTER_NULL) {} \
	constexpr CLASS(sl_null_t): POINTER_NAME(POINTER_NULL) {} \
	CLASS(const CLASS&) = delete; \
	CLASS(CLASS&& other) noexcept \
	{ \
		POINTER_NAME = other.POINTER_NAME; \
		other.POINTER_NAME = POINTER_NULL; \
	} \
	~CLASS() \
	{ \
		if (POINTER_NAME != POINTER_NULL) { \
			FREE_POINTER (POINTER_NAME); \
		} \
	} \
	CLASS& operator=(const CLASS&) = delete; \
	CLASS& operator=(CLASS&& other) noexcept \
	{ \
		if (POINTER_NAME != POINTER_NULL) { \
			FREE_POINTER (POINTER_NAME); \
		} \
		POINTER_NAME = other.POINTER_NAME; \
		other.POINTER_NAME = sl_null; \
		return *this; \
	} \
	CLASS& operator=(sl_null_t) noexcept \
	{ \
		if (POINTER_NAME != POINTER_NULL) { \
			FREE_POINTER (POINTER_NAME); \
			POINTER_NAME = POINTER_NULL; \
		} \
		return *this; \
	} \
	constexpr explicit operator bool() const \
	{ \
		return POINTER_NAME != sl_null; \
	} \
	constexpr explicit operator POINTER_TYPE() const& \
	{ \
		return POINTER_NAME; \
	} \
	operator POINTER_TYPE() && = delete; \
	constexpr POINTER_TYPE operator->() const& \
	{ \
		return POINTER_NAME; \
	} \
	POINTER_TYPE operator->() && = delete; \
	constexpr sl_bool isNull() const \
	{ \
		return !POINTER_NAME; \
	} \
	constexpr sl_bool isNotNull() const \
	{ \
		return POINTER_NAME != sl_null; \
	} \
	void setNull() noexcept \
	{ \
		if (POINTER_NAME != POINTER_NULL) { \
			FREE_POINTER (POINTER_NAME); \
			POINTER_NAME = POINTER_NULL; \
		} \
	} \
	constexpr POINTER_TYPE get() const& \
	{ \
		return POINTER_NAME; \
	} \
	POINTER_TYPE get() && = delete;

#define SLIB_DEFINE_UNIQUE_PTR_MEMBERS(CLASS, POINTER_TYPE, POINTER_NAME, POINTER_NULL, FREE_POINTER) \
	SLIB_DEFINE_UNIQUE_PTR_MEMBERS_NO_ASSIGN(CLASS, POINTER_TYPE, POINTER_NAME, POINTER_NULL, FREE_POINTER) \
	constexpr explicit CLASS(POINTER_TYPE other): POINTER_NAME(other) {} \
	explicit CLASS& operator=(POINTER_TYPE other) noexcept \
	{ \
		if (POINTER_NAME != POINTER_NULL) { \
			FREE_POINTER (POINTER_NAME); \
		} \
		POINTER_NAME = other; \
		return *this; \
	}

	template <class T>
	class SLIB_EXPORT UniquePtr
	{
		SLIB_DEFINE_UNIQUE_PTR_MEMBERS(UniquePtr, T*, ptr, sl_null, delete)

	public:
		T& operator*() const noexcept
		{
			return *((ValueType*)(void*)ptr);
		}

	};

	template <class T>
	class SLIB_EXPORT UniquePtr<T[]>
	{
		SLIB_DEFINE_UNIQUE_PTR_MEMBERS(UniquePtr, T*, ptr, sl_null, delete[])
	};

}

#endif
