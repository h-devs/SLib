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

#include "slib/io/pipe.h"
#include "slib/io/pipe_stream.h"
#include "slib/io/pipe_event.h"

#include "slib/core/handle_ptr.h"
#include "slib/io/file.h"
#include "slib/io/priv/impl.h"

#if defined(SLIB_PLATFORM_IS_WINDOWS)
#include "slib/platform/win32/windows.h"
#else
#include <unistd.h>
#include <poll.h>
#endif

namespace slib
{

	namespace {

		static void CreatePipeHandle(HPipe& handle) noexcept
		{
#if defined(SLIB_PLATFORM_IS_WINDOWS)
			if (CreatePipe(&(handle.hRead), &(handle.hWrite), NULL, 4096)) {
				return;
			}
#else
			int fd[2];
			if (!(::pipe(fd))) {
				handle.hRead = fd[0];
				handle.hWrite = fd[1];
				return;
			}
#endif
			handle.hRead = SLIB_PIPE_INVALID_HANDLE;
			handle.hWrite = SLIB_PIPE_INVALID_HANDLE;
		}

		static void ClosePipeHandle(const HPipe& handle) noexcept
		{
#if defined(SLIB_PLATFORM_IS_WINDOWS)
			CloseHandle(handle.hRead);
			CloseHandle(handle.hWrite);
#else
			close(handle.hRead);
			close(handle.hWrite);
#endif
		}

	}

	SLIB_DEFINE_HANDLE_CONTAINER_MEMBERS(Pipe, HPipe, m_handle, sl_null, ClosePipeHandle)
	SLIB_DEFINE_ISTREAM_MEMBERS(Pipe, const noexcept)

	Pipe Pipe::create() noexcept
	{
		HPipe ret;
		CreatePipeHandle(ret);
		return ret;
	}

	sl_bool Pipe::isOpened() const noexcept
	{
		return isNotNone();
	}

	sl_pipe Pipe::getReadHandle() const noexcept
	{
		return m_handle.hRead;
	}

	sl_pipe Pipe::getWriteHandle() const noexcept
	{
		return m_handle.hWrite;
	}

	sl_reg Pipe::read(void* buf, sl_size size, sl_int32 timeout) const noexcept
	{
		if (isOpened()) {
			return (HandlePtr<File>(m_handle.hRead))->read(buf, size, timeout);
		}
		return SLIB_IO_ERROR;
	}

	sl_int32 Pipe::read32(void* buf, sl_uint32 size, sl_int32 timeout) const noexcept
	{
		if (isOpened()) {
			return (HandlePtr<File>(m_handle.hRead))->read32(buf, size, timeout);
		}
		return SLIB_IO_ERROR;
	}

	sl_reg Pipe::write(const void* buf, sl_size size, sl_int32 timeout) const noexcept
	{
		if (isOpened()) {
			return (HandlePtr<File>(m_handle.hWrite))->write(buf, size, timeout);
		}
		return SLIB_IO_ERROR;
	}

	sl_int32 Pipe::write32(const void* buf, sl_uint32 size, sl_int32 timeout) const noexcept
	{
		if (isOpened()) {
			return (HandlePtr<File>(m_handle.hWrite))->write32(buf, size, timeout);
		}
		return SLIB_IO_ERROR;
	}

	void Pipe::close() noexcept
	{
		setNone();
	}


	SLIB_DEFINE_ROOT_OBJECT(PipeStream)

	PipeStream::PipeStream(Pipe&& handle): Stream(Move(handle))
	{
	}

	PipeStream::~PipeStream()
	{
	}

	Ref<PipeStream> PipeStream::create() noexcept
	{
		return create(Pipe::create());
	}

	Ref<PipeStream> PipeStream::create(Pipe&& handle) noexcept
	{
		if (handle.isOpened()) {
			return new PipeStream(Move(handle));
		}
		return sl_null;
	}


	PipeEvent::PipeEvent(Pipe&& pipe): m_pipe(Move(pipe))
	{
		m_flagSet = sl_false;
#if defined(SLIB_PLATFORM_IS_UNIX)
		HandlePtr<File>(m_pipe.getReadHandle())->setNonBlocking();
		HandlePtr<File>(m_pipe.getWriteHandle())->setNonBlocking();
#endif
	}

	PipeEvent::~PipeEvent()
	{
	}

	Ref<PipeEvent> PipeEvent::create()
	{
		Pipe pipe = Pipe::create();
		if (pipe.isOpened()) {
			return new PipeEvent(Move(pipe));
		}
		return sl_null;
	}

	Pipe& PipeEvent::getPipe()
	{
		return m_pipe;
	}

	sl_pipe PipeEvent::getReadPipeHandle()
	{
		return m_pipe.getReadHandle();
	}

	sl_pipe PipeEvent::getWritePipeHandle()
	{
		return m_pipe.getWriteHandle();
	}

	sl_bool PipeEvent::isOpened()
	{
		return m_pipe.isOpened();
	}

	void PipeEvent::close()
	{
		m_pipe.close();
	}

#ifdef SLIB_PLATFORM_IS_UNIX
	sl_bool PipeEvent::waitFd(int fd, sl_uint32 events, sl_uint32* revents, sl_int32 timeout)
	{
		if (m_pipe.isNone()) {
			return sl_false;
		}
		Thread* thread = Thread::getCurrent();
		if (thread) {
			if (thread->isStopping()) {
				return sl_false;
			}
			thread->setWaitingEvent(this);
		}
		pollfd fds[2];
		Base::zeroMemory(fds, sizeof(pollfd) * 2);
		fds[0].fd = (int)(m_pipe.getReadHandle());
		fds[0].events = POLLIN | POLLPRI | POLLERR | POLLHUP;
		fds[1].fd = fd;
		fds[1].events = (short)events;
		int t = timeout >= 0 ? (int)timeout : -1;
		sl_bool ret = sl_false;
		if (poll(fds, 2, t) > 0) {
			if (fds[1].revents) {
				ret = sl_true;
				if (revents) {
					*revents = (sl_uint32)(fds[1].revents);
				}
			}
		}
		if (thread) {
			thread->clearWaitingEvent();
		}
		return ret;
	}

	sl_bool PipeEvent::waitReadFd(int fd, sl_int32 timeout)
	{
		return waitFd(fd, POLLIN | POLLPRI | POLLERR | POLLHUP, sl_null, timeout);
	}

	sl_bool PipeEvent::waitWriteFd(int fd, sl_int32 timeout)
	{
		return waitFd(fd, POLLOUT | POLLERR | POLLHUP, sl_null, timeout);
	}
#endif

	void PipeEvent::set()
	{
		if (m_pipe.isNone()) {
			return;
		}
		SpinLocker lock(&m_lock);
		if (m_flagSet) {
			return;
		}
		m_flagSet = sl_true;
		char c = 1;
		m_pipe.write(&c, 1);
	}

	void PipeEvent::reset()
	{
		if (m_pipe.isNone()) {
			return;
		}
		SpinLocker lock(&m_lock);
		if (!m_flagSet) {
			return;
		}
		m_flagSet = sl_false;
		for (;;) {
			static char t[200];
			sl_reg n = m_pipe.read(t, 200);
			if (n < 200) {
				break;
			}
		}
	}

	sl_bool PipeEvent::doWait(sl_int32 timeout)
	{
		if (m_pipe.isNone()) {
			return sl_false;
		}
#if defined(SLIB_PLATFORM_IS_WINDOWS)
		static char t[200];
		m_pipe.read(t, 200);
		return sl_true;
#else
		pollfd fd;
		Base::zeroMemory(&fd, sizeof(pollfd));
		fd.fd = (int)(m_pipe.getReadHandle());
		fd.events = POLLIN | POLLPRI | POLLERR | POLLHUP;
		int t = timeout >= 0 ? (int)timeout : -1;
		int ret = poll(&fd, 1, t);
		return ret > 0;
#endif
	}

}
