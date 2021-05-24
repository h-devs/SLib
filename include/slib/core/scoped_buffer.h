/*
 *   Copyright (c) 2008-2018 SLIBIO <https://github.com/SLIBIO>
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

#ifndef CHECKHEADER_SLIB_CORE_SCOPED_BUFFER
#define CHECKHEADER_SLIB_CORE_SCOPED_BUFFER

#include "new_helper.h"

namespace slib
{
	
	template <class T, sl_size CountStack>
	class SLIB_EXPORT ScopedBuffer
	{
	public:
		T* data;
		sl_size count;

	public:
		ScopedBuffer(sl_size _count) noexcept
		{
			if (_count < CountStack) {
				data = stack;
			} else {
				data = NewHelper<T>::create(_count);
			}
			count = _count;
		}

		ScopedBuffer(const ScopedBuffer& other) = delete;

		ScopedBuffer(ScopedBuffer&& other) = delete;

		~ScopedBuffer() noexcept
		{
			_free();
		}

	public:
		ScopedBuffer& operator=(const ScopedBuffer& other) = delete;

		ScopedBuffer& operator=(ScopedBuffer&& other) = delete;

		T& operator[](sl_size index) const noexcept
		{
			return data[index];
		}

		explicit operator bool() const noexcept
		{
			return data != sl_null;
		}

	public:
		sl_bool isNull() const noexcept
		{
			return !data;
		}

		sl_bool isNotNull() const noexcept
		{
			return data != sl_null;
		}

		T* get() const noexcept
		{
			return data;
		}

		void reset() noexcept
		{
			_free();
			data = sl_null;
			count = 0;
		}

	private:
		void _free() noexcept
		{
			if (data) {
				if (data != stack) {
					NewHelper<T>::free(data, count);
				}
			}
		}

	private:
		T stack[CountStack];

	};

#define SLIB_SCOPED_BUFFER(TYPE, STACK, NAME, COUNT) \
	ScopedBuffer<TYPE, STACK> _scoped_buf__##NAME(COUNT); \
	TYPE* NAME = _scoped_buf__##NAME.data;
	
}

#endif
