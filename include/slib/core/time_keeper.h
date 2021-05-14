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

#ifndef CHECKHEADER_SLIB_CORE_TIME_KEEPER
#define CHECKHEADER_SLIB_CORE_TIME_KEEPER

#include "spin_lock.h"
#include "default_members.h"

namespace slib
{

	class SLIB_EXPORT TimeKeeper
	{
	public:
		TimeKeeper() noexcept;

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(TimeKeeper)

	public:
		static sl_uint64 now() noexcept;

		void start() noexcept;

		void start(sl_uint64 current) noexcept;

		void startAndSetTime(sl_uint64 initialTimeValue) noexcept;

		void startAndSetTime(sl_uint64 initialTimeValue, sl_uint64 current) noexcept;

		void restart() noexcept;

		void restart(sl_uint64 current) noexcept;

		void restartAndSetTime(sl_uint64 initialTimeValue) noexcept;

		void restartAndSetTime(sl_uint64 initialTimeValue, sl_uint64 current) noexcept;

		void stop() noexcept;

		void resume() noexcept;

		void resume(sl_uint64 current) noexcept;

		void pause() noexcept;

		void pause(sl_uint64 current) noexcept;

		sl_uint64 getTime() const noexcept;

		sl_uint64 getTime(sl_uint64 current) const noexcept;

		void setTime(sl_uint64 time) noexcept;

		void setTime(sl_uint64 time, sl_uint64 current) noexcept;

		void update() noexcept;

		void update(sl_uint64 current) noexcept;

		sl_bool isStarted() const noexcept;

		sl_bool isStopped() const noexcept;

		sl_bool isRunning() const noexcept;

		sl_bool isNotRunning() const noexcept;

		sl_bool isPaused() const noexcept;

	protected:
		sl_bool m_flagStarted;
		sl_bool m_flagRunning;
		sl_uint64 m_timeLast;
		sl_uint64 m_timeElapsed;
		SpinLock m_lock;

	};

}

#endif
