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

#ifndef CHECKHEADER_SLIB_IO_PIPE_EVENT
#define CHECKHEADER_SLIB_IO_PIPE_EVENT

#include "pipe.h"

#include "../core/event.h"
#include "../core/spin_lock.h"

namespace slib
{

	class SLIB_EXPORT PipeEvent : public Event
	{
	public:
		PipeEvent(Pipe&& pipe);

		~PipeEvent();

	public:
		static Ref<PipeEvent> create();

		Pipe& getPipe();

		sl_pipe getReadPipeHandle();

		sl_pipe getWritePipeHandle();

		sl_bool isOpened();

		void close();

#ifdef SLIB_PLATFORM_IS_UNIX
		sl_bool waitFd(int fd, sl_uint32 events, sl_uint32* revents, sl_int32 timeout = -1);

		sl_bool waitReadFd(int fd, sl_int32 timeout = -1);

		sl_bool waitWriteFd(int fd, sl_int32 timeout = -1);
#endif

	public:
		void set() override;

		void reset() override;

	protected:
		sl_bool doWait(sl_int32 timeout) override;

	protected:
		Pipe m_pipe;
		sl_bool m_flagSet;
		SpinLock m_lock;

	};

}

#endif
