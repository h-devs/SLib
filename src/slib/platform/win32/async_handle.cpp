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

#include "slib/platform/win32/async_handle.h"

#include "slib/io/priv/impl.h"
#include "slib/platform/win32/platform.h"

namespace slib
{

	SLIB_DEFINE_ISTREAM_MEMBERS(AsyncHandleIO, noexcept)

	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(AsyncHandleIO)

	sl_bool AsyncHandleIO::prepareIO(OVERLAPPED& overlapped)
	{
		if (event.isNull()) {
			event = Event::create();
			if (event.isNull()) {
				return sl_false;
			}
		}
		Base::zeroMemory(&overlapped, sizeof(overlapped));
		overlapped.hEvent = Win32::getEventHandle(event.get());
		overlapped.Offset = (DWORD)offset;
		overlapped.OffsetHigh = (DWORD)(offset >> 32);
		return sl_true;
	}

	sl_int32 AsyncHandleIO::processResult(OVERLAPPED& overlapped, BOOL bRet, sl_int32 timeout)
	{
		DWORD dwError = ERROR_SUCCESS;
		if (!bRet) {
			dwError = GetLastError();
			if (dwError == ERROR_HANDLE_EOF) {
				return SLIB_IO_ENDED;
			}
		}
		if (dwError == ERROR_SUCCESS || dwError == ERROR_IO_PENDING) {
			for (;;) {
				DWORD dwTransfered = 0;
				if (GetOverlappedResult(handle, &overlapped, &dwTransfered, FALSE)) {
					offset += (sl_uint32)dwTransfered;
					ResetEvent(overlapped.hEvent);
					return (sl_int32)dwTransfered;
				} else {
					dwError = GetLastError();
					if (dwError == ERROR_HANDLE_EOF) {
						return SLIB_IO_ENDED;
					}
					if (dwError == ERROR_IO_INCOMPLETE) {
						if (!timeout) {
							return SLIB_IO_TIMEOUT;
						}
						event->wait(timeout);
						timeout = 0;
					} else {
						return SLIB_IO_ERROR;
					}
				}
			}
		} else {
			return SLIB_IO_ERROR;
		}
		return SLIB_IO_ERROR;
	}

	sl_reg AsyncHandleIO::read(void* buf, sl_size size, sl_int32 timeout) noexcept
	{
		return ReaderHelper::readWithRead32(this, buf, size, timeout);
	}

	sl_int32 AsyncHandleIO::read32(void* buf, sl_uint32 size, sl_int32 timeout) noexcept
	{
		if (!size) {
			return SLIB_IO_EMPTY_CONTENT;
		}
		OVERLAPPED overlapped;
		if (!(prepareIO(overlapped))) {
			return SLIB_IO_ERROR;
		}
		BOOL bRet = ReadFile(handle, buf, size, NULL, &overlapped);
		return processResult(overlapped, bRet, timeout);
	}

	sl_reg AsyncHandleIO::write(const void* buf, sl_size size, sl_int32 timeout) noexcept
	{
		return WriterHelper::writeWithWrite32(this, buf, size, timeout);
	}

	sl_int32 AsyncHandleIO::write32(const void* buf, sl_uint32 size, sl_int32 timeout) noexcept
	{
		OVERLAPPED overlapped;
		if (!(prepareIO(overlapped))) {
			return SLIB_IO_ERROR;
		}
		BOOL bRet = WriteFile(handle, buf, size, NULL, &overlapped);
		return processResult(overlapped, bRet, timeout);
	}

}
