/*
 *   Copyright (c) 2008-2022 SLIBIO <https://github.com/SLIBIO>
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

#ifndef CHECKHEADER_SLIB_CORE_MEMORY_VIEW
#define CHECKHEADER_SLIB_CORE_MEMORY_VIEW

#include "default_members.h"

namespace slib
{

	class Memory;
	
	class SLIB_EXPORT MemoryView
	{
	public:
		void* data;
		sl_size size;

	public:
		SLIB_CONSTEXPR MemoryView(): data(sl_null), size(0) {}

		SLIB_CONSTEXPR MemoryView(const void* _data, sl_size _size): data((void*)_data), size(_size) {}

		MemoryView(const Memory& mem) noexcept;

		SLIB_DEFINE_CLASS_DEFAULT_MEMBERS_INLINE(MemoryView)

	public:
		template <sl_size N>
		static MemoryView literal(const char(&s)[N]) noexcept
		{
			return MemoryView(s, N - 1);
		}

	public:
		MemoryView& operator=(const Memory& mem) noexcept;

		SLIB_CONSTEXPR explicit operator sl_bool() const
		{
			return size != 0;
		}

	};
	
}

#endif
