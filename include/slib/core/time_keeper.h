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

#include "time.h"
#include "spin_lock.h"

namespace slib
{

	class SLIB_EXPORT TimeKeeper
	{
	public:
		TimeKeeper() noexcept;

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(TimeKeeper)

	public:
		void start() noexcept;

		void start(const Time& current) noexcept;

		void startAndSetTime(const Time& initialTimeValue) noexcept;

		void startAndSetTime(const Time& initialTimeValue, const Time& current) noexcept;

		void restart() noexcept;

		void restart(const Time& current) noexcept;

		void restartAndSetTime(const Time& initialTimeValue) noexcept;

		void restartAndSetTime(const Time& initialTimeValue, const Time& current) noexcept;

		void stop() noexcept;

		void resume() noexcept;

		void resume(const Time& current) noexcept;

		void pause() noexcept;

		void pause(const Time& current) noexcept;

		Time getTime() const noexcept;

		Time getTime(const Time& current) const noexcept;

		void setTime(const Time& time) noexcept;

		void setTime(const Time& time, const Time& current) noexcept;

		void update() noexcept;

		void update(const Time& current) noexcept;

		sl_bool isStarted() const noexcept;

		sl_bool isStopped() const noexcept;

		sl_bool isRunning() const noexcept;

		sl_bool isNotRunning() const noexcept;

		sl_bool isPaused() const noexcept;

	protected:
		sl_bool m_flagStarted;
		sl_bool m_flagRunning;
		Time m_timeLast;
		Time m_timeElapsed;
		SpinLock m_lock;

	};

}

#endif
