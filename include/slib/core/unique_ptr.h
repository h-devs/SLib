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

#include "handle_container.h"

namespace slib
{

	template <class T>
	class SLIB_EXPORT UniquePtr
	{
	public:
		SLIB_DEFINE_NULLABLE_HANDLE_CONTAINER_TEMPLATE_MEMBERS(UniquePtr, T*, ptr, delete)

	public:
		T& operator*() const noexcept
		{
			return *ptr;
		}

		SLIB_CONSTEXPR T* operator->() const
		{
			return ptr;
		}

	};

	template <class T>
	class SLIB_EXPORT UniquePtr<T[]>
	{
	public:
		SLIB_DEFINE_NULLABLE_HANDLE_CONTAINER_TEMPLATE_MEMBERS(UniquePtr, T*, ptr, delete[])

	public:
		template <typename INT>
		SLIB_CONSTEXPR T& operator[](INT index) const
		{
			return ptr[index];
		}

	};

}

#endif
