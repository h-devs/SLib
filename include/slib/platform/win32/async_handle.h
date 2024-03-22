/*
 *   Copyright (c) 2008-2024 SLIBIO <https://github.com/SLIBIO>
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

#ifndef CHECKHEADER_SLIB_PLATFORM_WIN32_ASYNC_HANDLE
#define CHECKHEADER_SLIB_PLATFORM_WIN32_ASYNC_HANDLE

#include "windows.h"

#include "../../io/io.h"
#include "../../core/event.h"

namespace slib
{

	class SLIB_EXPORT AsyncHandleIO
	{
	public:
		HANDLE handle;
		sl_uint64 offset;
		Ref<Event> event;

	public:
		AsyncHandleIO(): handle(INVALID_HANDLE_VALUE), offset(0) {}

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(AsyncHandleIO)

		SLIB_DECLARE_ISTREAM_MEMBERS(noexcept)

	public:
		sl_reg read(void* buf, sl_size size, sl_int32 timeout = -1) noexcept;

		sl_int32 read32(void* buf, sl_uint32 size, sl_int32 timeout = -1) noexcept;

		sl_reg write(const void* buf, sl_size size, sl_int32 timeout = -1) noexcept;

		sl_int32 write32(const void* buf, sl_uint32 size, sl_int32 timeout = -1) noexcept;

	private:
		sl_bool prepareIO(OVERLAPPED& overlapped);

		sl_int32 processResult(OVERLAPPED& overlapped, BOOL bRet, sl_int32 timeout);

	};

}

#endif
