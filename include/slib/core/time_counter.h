/*
 *   Copyright (c) 2008-2020 SLIBIO <https://github.com/SLIBIO>
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

#ifndef CHECKHEADER_SLIB_CORE_TIME_COUNTER
#define CHECKHEADER_SLIB_CORE_TIME_COUNTER

#include "definition.h"
#include "default_members.h"

namespace slib
{

	class SLIB_EXPORT TimeCounter
	{
	public:
		TimeCounter() noexcept;

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(TimeCounter)

	public:
		static sl_uint64 now() noexcept;

		sl_uint64 getElapsedMilliseconds() const noexcept;

		sl_uint64 getElapsedMilliseconds(sl_uint64 now) const noexcept;

		void reset() noexcept;

		void reset(sl_uint64 now) noexcept;

	protected:
		sl_uint64 m_timeStart;

	};

}

#endif
