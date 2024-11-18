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

#include "slib/core/dispatch.h"
#include "slib/core/dispatch_loop.h"

#include "slib/core/timer.h"
#include "slib/core/safe_static.h"

namespace slib
{

	SLIB_DEFINE_OBJECT(Dispatcher, Object)

	Dispatcher::Dispatcher()
	{
	}

	Dispatcher::~Dispatcher()
	{
	}

	sl_bool Dispatcher::setTimeoutByDefaultDispatchLoop(const Function<void()>& task, sl_uint64 delayMillis)
	{
		WeakRef<Dispatcher> thiz = this;
		return Dispatch::setTimeout([this, thiz, task]() {
			Ref<Dispatcher> ref = thiz;
			if (ref.isNull()) {
				return;
			}
			dispatch(task);
		}, delayMillis);
	}

	sl_bool Dispatch::dispatch(const Ref<Dispatcher>& dispatcher, const Function<void()>& task)
	{
		if (dispatcher.isNotNull()) {
			return dispatcher->dispatch(task);
		} else {
			return dispatch(task);
		}
	}

	sl_bool Dispatch::dispatch(const Function<void()>& task)
	{
		Ref<Dispatcher> dispatcher = DispatchLoop::getDefault();
		if (dispatcher.isNotNull()) {
			return dispatcher->dispatch(task);
		}
		return sl_false;
	}

	sl_bool Dispatch::setTimeout(const Ref<Dispatcher>& dispatcher, const Function<void()>& task, sl_uint64 delayMillis)
	{
		if (dispatcher.isNotNull()) {
			return dispatcher->dispatch(task, delayMillis);
		} else {
			return setTimeout(task, delayMillis);
		}
	}

	sl_bool Dispatch::setTimeout(const Function<void()>& task, sl_uint64 delayMillis)
	{
		Ref<Dispatcher> dispatcher = DispatchLoop::getDefault();
		if (dispatcher.isNotNull()) {
			return dispatcher->dispatch(task, delayMillis);
		}
		return sl_false;
	}

	Ref<Timer> Dispatch::setInterval(const Ref<DispatchLoop>& loop, const Function<void(Timer*)>& task, sl_uint64 intervalMillis)
	{
		return Timer::startWithLoop(loop, task, intervalMillis);
	}

	Ref<Timer> Dispatch::setInterval(const Function<void(Timer*)>& task, sl_uint64 intervalMillis)
	{
		return Dispatch::setInterval(sl_null, task, intervalMillis);
	}


	SLIB_DEFINE_NESTED_CLASS_DEFAULT_MEMBERS(DispatchLoop, TimerTask)

	DispatchLoop::TimerTask::TimerTask()
	{
	}


	SLIB_DEFINE_OBJECT(DispatchLoop, Dispatcher)

	DispatchLoop::DispatchLoop()
	{
		m_serviceLock = getLocker();
		m_onRunService = SLIB_FUNCTION_MEMBER(this, _runLoop);
	}

	DispatchLoop::~DispatchLoop()
	{
		stop();
	}

	namespace
	{
		static Ref<DispatchLoop> CreateDefaultDispatchLoop(sl_bool flagRelease = sl_false)
		{
			if (flagRelease) {
				return sl_null;
			}
			return DispatchLoop::create();
		}

		static Ref<DispatchLoop> GetDefaultDispatchLoop(sl_bool flagRelease = sl_false)
		{
			SLIB_SAFE_LOCAL_STATIC(Ref<DispatchLoop>, ret, CreateDefaultDispatchLoop(flagRelease))
			if (SLIB_SAFE_STATIC_CHECK_FREED(ret)) {
				return sl_null;
			}
			if (ret.isNotNull()) {
				if (flagRelease) {
					ret->release();
				} else {
					return ret;
				}
			}
			return sl_null;
		}
	}

	Ref<DispatchLoop> DispatchLoop::getDefault()
	{
		return GetDefaultDispatchLoop();
	}

	void DispatchLoop::releaseDefault()
	{
		GetDefaultDispatchLoop(sl_true);
	}

	Ref<DispatchLoop> DispatchLoop::create(sl_bool flagAutoStart)
	{
		Ref<DispatchLoop> ret = new DispatchLoop;
		if (ret.isNotNull()) {
			if (flagAutoStart) {
				if (ret->start()) {
					return ret;
				}
			} else {
				return ret;
			}
		}
		return sl_null;
	}

	void DispatchLoop::release()
	{
		ThreadService::release();
		m_queueTasks.removeAll();
		m_timeTasks.removeAll();
		m_queueTimers.removeAll();
	}

	void DispatchLoop::_wake()
	{
		ObjectLocker lock(this);
		if (m_serviceThread.isNotNull()) {
			m_serviceThread->wakeSelfEvent();
		}
	}

	sl_int32 DispatchLoop::_getTimeout()
	{
		sl_int32 t1 = _getTimeout_TimeTasks();
		sl_int32 t2 = _getTimeout_Timer();
		if (m_queueTasks.isNotEmpty()) {
			return 0;
		}
		if (t1 < 0) {
			return t2;
		} else {
			if (t2 < 0) {
				return t1;
			} else {
				return SLIB_MIN(t1, t2);
			}
		}
	}

	sl_bool DispatchLoop::dispatch(const Function<void()>& task, sl_uint64 delayMillis)
	{
		if (task.isNull()) {
			return sl_false;
		}
		if (!delayMillis) {
			if (m_queueTasks.push(task)) {
				_wake();
				return sl_true;
			}
		} else {
			TimeTask tt;
			tt.time = getElapsedMilliseconds() + delayMillis;
			tt.task = task;
			if (m_timeTasks.add(tt.time, tt)) {
				_wake();
				return sl_true;
			}
		}
		return sl_false;
	}

	sl_int32 DispatchLoop::_getTimeout_TimeTasks()
	{
		ObjectLocker lock(&m_timeTasks);
		sl_uint64 rel = getElapsedMilliseconds();
		if (m_timeTasks.isEmpty()) {
			return -1;
		}
		sl_int32 timeout = -1;
		LinkedQueue< Function<void()> > tasks;
		while (MapNode<sl_uint64, TimeTask>* node = m_timeTasks.getFirstNode()) {
			if (rel >= node->key) {
				tasks.push_NoLock(node->value.task);
				m_timeTasks.removeAt(node);
			} else {
				timeout = (sl_int32)(node->key - rel);
				break;
			}
		}
		lock.unlock();

		Function<void()> task;
		while (tasks.pop_NoLock(&task)) {
			task();
		}
		return timeout;
	}

	sl_int32 DispatchLoop::_getTimeout_Timer()
	{
		ObjectLocker lock(&m_queueTimers);

		sl_int32 timeout = -1;
		LinkedQueue< Ref<Timer> > tasks;

		sl_uint64 rel = getElapsedMilliseconds();
		Link<TimerTask>* link = m_queueTimers.getFront();
		while (link) {
			Ref<Timer> timer(link->value.timer);
			if (timer.isNotNull()) {
				if (timer->isStarted()) {
					sl_uint64 t = rel - timer->getLastRunTime();
					if (t >= timer->getInterval()) {
						tasks.push_NoLock(timer);
						timer->setLastRunTime(rel);
						t = timer->getInterval();
					} else {
						t = timer->getInterval() - t;
					}
					if (timeout < 0 || t < (sl_uint64)timeout) {
						timeout = (sl_uint32)t;
					}
				}
				link = link->next;
			} else {
				Link<TimerTask>* linkRemove = link;
				link = link->next;
				m_queueTimers.removeAt(linkRemove);
			}
		}

		lock.unlock();

		Ref<Timer> task;
		while (tasks.pop_NoLock(&task)) {
			if (task.isNotNull()) {
				task->run();
			}
		}
		return timeout;
	}

	sl_bool DispatchLoop::TimerTask::operator==(const DispatchLoop::TimerTask& other) const noexcept
	{
		return timer == other.timer;
	}

	sl_bool DispatchLoop::addTimer(const Ref<Timer>& timer)
	{
		if (timer.isNull()) {
			return sl_false;
		}
		TimerTask t;
		t.timer = timer;
		if (m_queueTimers.push(t)) {
			_wake();
			return sl_true;
		} else {
			return sl_false;
		}
	}

	void DispatchLoop::removeTimer(const Ref<Timer>& timer)
	{
		TimerTask t;
		t.timer = timer;
		m_queueTimers.remove(t);
	}

	sl_uint64 DispatchLoop::getElapsedMilliseconds()
	{
		return m_timeCounter.getElapsedMilliseconds();
	}

	void DispatchLoop::_runLoop()
	{
		Thread* thread = Thread::getCurrent();
		if (!thread) {
			return;
		}

		while (thread->isNotStopping()) {

			increaseReference_NoSync();

			// Async Tasks
			{
				LinkedQueue< Function<void()> > tasks;
				{
					ObjectLocker lock(&m_queueTasks);
					tasks.merge_NoLock(&m_queueTasks);
				}
				Function<void()> task;
				while (tasks.pop_NoLock(&task)) {
					task();
				}
			}

			sl_int32 t = _getTimeout();
			if (t != 0) {
				if (t < 0 || t > 10000) {
					t = 10000;
				}
				thread->wait(t);
			}

			if (!(decreaseReference())) {
				break;
			}
		}
	}

}
