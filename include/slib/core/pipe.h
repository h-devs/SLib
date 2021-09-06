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

#ifndef CHECKHEADER_SLIB_CORE_PIPE
#define CHECKHEADER_SLIB_CORE_PIPE

#include "io.h"
#include "handle_container.h"
#include "default_members.h"

#ifdef SLIB_PLATFORM_IS_WINDOWS
typedef void* sl_pipe;
#define SLIB_PIPE_INVALID_HANDLE ((void*)((sl_reg)-1))
#else
typedef int sl_pipe;
#define SLIB_PIPE_INVALID_HANDLE (-1)
#endif

namespace slib
{

	class HPipe
	{
	public:
		sl_pipe hRead;
		sl_pipe hWrite;

	public:
		HPipe() = default;

		SLIB_CONSTEXPR HPipe(sl_null_t): hRead(SLIB_PIPE_INVALID_HANDLE), hWrite(SLIB_PIPE_INVALID_HANDLE) {}

		SLIB_CONSTEXPR HPipe(const HPipe& other): hRead(other.hRead), hWrite(other.hWrite) {}

		HPipe& operator=(sl_null_t) noexcept
		{
			hRead = SLIB_PIPE_INVALID_HANDLE;
			hWrite = SLIB_PIPE_INVALID_HANDLE;
			return *this;
		}

		HPipe& operator=(const HPipe& other) noexcept
		{
			hRead = other.hRead;
			hWrite = other.hWrite;
			return *this;
		}

		SLIB_CONSTEXPR sl_bool operator==(sl_null_t) const
		{
			return hRead == SLIB_PIPE_INVALID_HANDLE;
		}

		SLIB_CONSTEXPR sl_bool operator!=(sl_null_t) const
		{
			return hRead != SLIB_PIPE_INVALID_HANDLE;
		}

	};
	
	class SLIB_EXPORT Pipe
	{
		SLIB_DECLARE_HANDLE_CONTAINER_MEMBERS(Pipe, HPipe, m_handle, sl_null)
		SLIB_DECLARE_ISTREAM_MEMBERS(const noexcept)

	public:
		static Pipe create() noexcept;

	public:
		sl_bool isOpened() const noexcept;

		sl_pipe getReadHandle() const noexcept;

		sl_pipe getWriteHandle() const noexcept;

		sl_reg read(void* buf, sl_size size) const noexcept;

		sl_int32 read32(void* buf, sl_uint32 size) const noexcept;

		sl_bool waitRead(sl_int32 timeout = -1) const noexcept;

		sl_reg write(const void* buf, sl_size size) const noexcept;

		sl_int32 write32(const void* buf, sl_uint32 size) const noexcept;

		sl_bool waitWrite(sl_int32 timeout = -1) const noexcept;

		void close() noexcept;

	public:
		Pipe& operator*() noexcept
		{
			return *this;
		}

	};

}

#endif
