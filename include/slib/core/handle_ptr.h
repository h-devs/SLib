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

#ifndef CHECKHEADER_SLIB_CORE_HANDLE_PTR
#define CHECKHEADER_SLIB_CORE_HANDLE_PTR

#include "definition.h"

namespace slib
{

	template <class T>
	class SLIB_EXPORT HandlePtr
	{
	private:
		typedef typename T::HandleType HandleType;
		HandleType m_handle;

	public:
		HandlePtr(HandleType handle): m_handle(handle) {}

		HandlePtr(const HandlePtr&) = default;

	public:
		const T* get() const noexcept
		{
			return reinterpret_cast<const T*>(this);
		}

		T* get() noexcept
		{
			return reinterpret_cast<T*>(this);
		}

		const T& operator*() const noexcept
		{
			return *(get());
		}

		T& operator*() noexcept
		{
			return *(get());
		}

		const T* operator->() const noexcept
		{
			return get();
		}

		T* operator->() noexcept
		{
			return get();
		}

	};

}


#endif
