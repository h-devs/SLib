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

#ifndef CHECKHEADER_SLIB_CORE_WIN32_SCOPED_HANDLE
#define CHECKHEADER_SLIB_CORE_WIN32_SCOPED_HANDLE

#include "../definition.h"

#ifdef SLIB_PLATFORM_IS_WIN32

#include "../windows.h"

namespace slib
{
		
	class SLIB_EXPORT ScopedHandle
	{
	public:
		HANDLE handle;

	public:
		constexpr ScopedHandle(HANDLE _handle): handle(_handle) {}

		ScopedHandle(const ScopedHandle&) = delete;

		ScopedHandle(ScopedHandle&& other) noexcept
		{
			handle = other.handle;
			other.handle = INVALID_HANDLE_VALUE;
		}
		
		~ScopedHandle()
		{
			close();
		}

	public:
		ScopedHandle& operator=(const ScopedHandle&) = delete;

		ScopedHandle& operator=(ScopedHandle&& other) noexcept
		{
			handle = other.handle;
			other.handle = INVALID_HANDLE_VALUE;
			return *this;
		}

		explicit operator HANDLE() const
		{
			return handle;
		}

	public:
		void close()
		{
			HANDLE h = handle;
			if (h != INVALID_HANDLE_VALUE) {
				::CloseHandle(h);
				handle = INVALID_HANDLE_VALUE;
			}
		}

		constexpr HANDLE get() const
		{
			return handle;
		}

	};
}

#endif

#endif