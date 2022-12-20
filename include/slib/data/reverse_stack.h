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

#ifndef CHECKHEADER_SLIB_DATA_REVERSE_STACK
#define CHECKHEADER_SLIB_DATA_REVERSE_STACK

#include "definition.h"

#include "../core/base.h"

namespace slib
{

	template <sl_size SIZE>
	class SLIB_EXPORT ReverseStack
	{
	public:
		ReverseStack(): m_end(m_buf + SIZE), m_current(m_buf + SIZE) {}

	public:
		sl_uint8* getData() const
		{
			return m_current;
		}

		sl_size getSize() const
		{
			return m_end - m_current;
		}

		sl_uint8* allocate(sl_size size)
		{
			if (size <= (sl_size)(m_current - m_buf)) {
				sl_uint8* p = m_current - size;
				m_current = p;
				return p;
			}
			return sl_null;
		}

		sl_bool push(const void* data, sl_size size)
		{
			sl_uint8* p = allocate(size);
			if (p) {
				Base::copyMemory(p, data, size);
				return sl_true;
			}
			return sl_false;
		}

	protected:
		sl_uint8 m_buf[SIZE];
		sl_uint8* m_end;
		sl_uint8* m_current;
	};

}

#endif
