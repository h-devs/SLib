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

#include "slib/core/thread.h"
#include "slib/core/thread_pool.h"

#include "slib/core/system.h"
#include "slib/core/event.h"
#include "slib/core/safe_static.h"

#if defined(SLIB_PLATFORM_IS_ANDROID)
#	include "slib/core/platform.h"
#endif

namespace slib
{

	SLIB_DEFINE_OBJECT(Thread, Object)

	namespace priv
	{
		namespace thread
		{
			typedef HashMap< Thread*, WeakRef<Thread> > ThreadMap;
			SLIB_GLOBAL_ZERO_INITIALIZED(Atomic<ThreadMap>, g_mapThreads)
		}
	}

	using namespace priv::thread;

	Thread::Thread()
	{
		m_flagRunning = sl_false;
		m_flagRequestStop = sl_false;

		m_handle = sl_null;
		m_priority = ThreadPriority::Normal;
	}

	Thread::~Thread()
	{
		if (!SLIB_SAFE_STATIC_CHECK_FREED(g_mapThreads)) {
			ThreadMap(g_mapThreads).remove(this);
		}
	}

	Ref<Thread> Thread::create(const Function<void()>& callback)
	{
		if (SLIB_SAFE_STATIC_CHECK_FREED(g_mapThreads)) {
			return sl_null;
		}
		if (callback.isNull()) {
			return sl_null;
		}
		Ref<Event> eventWake = Event::create(sl_true);
		if (eventWake.isNotNull()) {
			Ref<Event> eventExit = Event::create(sl_false);
			if (eventExit.isNotNull()) {
				Ref<Thread> ret = new Thread();
				if (ret.isNotNull()) {
					ret->m_eventWake = Move(eventWake);
					ret->m_eventExit = Move(eventExit);
					g_mapThreads.put(ret.get(), ret);
					ret->m_callback = callback;
					return ret;
				}
			}
		}
		return sl_null;
	}

	Ref<Thread> Thread::start(const Function<void()>& callback, sl_uint32 stackSize)
	{
		Ref<Thread> ret = create(callback);
		if (ret.isNotNull()) {
			if (ret->start(stackSize)) {
				return ret;
			}
		}
		return sl_null;
	}

	List< Ref<Thread> > Thread::getAllThreads()
	{
		if (SLIB_SAFE_STATIC_CHECK_FREED(g_mapThreads)) {
			return sl_null;
		}
		HashMap< Thread*, WeakRef<Thread> > map = g_mapThreads;
		if (map.isNull()) {
			return sl_null;
		}
		List< Ref<Thread> > list;
		MutexLocker lock(map.getLocker());
		for (auto& item : map) {
			Ref<Thread> thread = item.value;
			if (thread.isNotNull()) {
				list.add_NoLock(thread);
			}
		}
		return list;
	}

	void Thread::finishAllThreads()
	{
		ListElements< Ref<Thread> > threads(getAllThreads());
		sl_size i;
		for (i = 0; i < threads.count; i++) {
			threads[i]->finish();
		}
		for (i = 0; i < threads.count; i++) {
			threads[i]->finishAndWait(100);
		}
	}

	sl_bool Thread::start(sl_uint32 stackSize)
	{
		ObjectLocker lock(this);
		if (!m_flagRunning) {
			m_flagRunning = sl_true;
			m_flagRequestStop = sl_false;
			m_eventExit->reset();
			m_eventWake->reset();
			_nativeStart(stackSize);
			if (m_handle) {
				if (m_priority != ThreadPriority::Normal) {
					_nativeSetPriority();
				}
				return sl_true;
			} else {
				m_flagRunning = sl_false;
			}
		}
		return sl_false;
	}

	void Thread::finish()
	{
		if (isRunning()) {
			m_flagRequestStop = sl_true;
			wake();
		}
	}

	sl_bool Thread::join(sl_int32 timeout)
	{
		if (isRunning()) {
			if (!(m_eventExit->wait(timeout))) {
				return sl_false;
			}
		}
		return sl_true;
	}

	sl_bool Thread::finishAndWait(sl_int32 timeout)
	{
		Ref<Thread> thiz = this;
		if (thiz.isNull()) {
			return sl_true;
		}
		if (isCurrentThread()) {
			if (isRunning()) {
				m_flagRequestStop = sl_true;
			}
			return sl_false;
		}
		if (timeout >= 0) {
			if (isRunning()) {
				m_flagRequestStop = sl_true;
				while (1) {
					wake();
					sl_int32 t = timeout;
					if (t > 100) {
						t = 100;
					}
					if (m_eventExit->wait(t)) {
						return sl_true;
					}
					if (timeout <= 100) {
						break;
					}
					timeout -= 100;
				}
			}
		} else {
			while (isRunning()) {
				m_flagRequestStop = sl_true;
				wake();
				System::sleep(1);
			}
		}
		return sl_false;
	}

	sl_bool Thread::wait(sl_int32 timeout)
	{
		if (m_flagRequestStop) {
			return sl_false;
		} else {
			return m_eventWake->wait(timeout);
		}
	}

	void Thread::wakeSelfEvent()
	{
		m_eventWake->set();
	}
	
	Event* Thread::getSelfEvent()
	{
		return m_eventWake.get();
	}

	void Thread::wake()
	{
		Ref<Event> ev = m_eventWaiting;
		if (ev.isNotNull()) {
			ev->set();
		}
	}
	
	Ref<Event> Thread::getWaitingEvent()
	{
		return m_eventWaiting;
	}

	void Thread::setWaitingEvent(Event* ev)
	{
		m_eventWaiting = ev;
	}

	void Thread::clearWaitingEvent()
	{
		m_eventWaiting.setNull();
	}

	ThreadPriority Thread::getPriority()
	{
		return m_priority;
	}

	void Thread::setPriority(ThreadPriority priority)
	{
		m_priority = priority;
		_nativeSetPriority();
	}

	sl_bool Thread::isRunning()
	{
		return m_flagRunning && _nativeCheckRunning();
	}

	sl_bool Thread::isNotRunning()
	{
		return !(isRunning());
	}

	sl_bool Thread::isStopping()
	{
		return m_flagRequestStop;
	}

	sl_bool Thread::isNotStopping()
	{
		return !m_flagRequestStop;
	}

	sl_bool Thread::isWaiting()
	{
		return m_eventWaiting != sl_null;
	}

	sl_bool Thread::isNotWaiting()
	{
		return !m_eventWaiting;
	}

	const Function<void()>& Thread::getCallback()
	{
		return m_callback;
	}

	void Thread::sleep(sl_uint32 ms)
	{
		Thread* thread = getCurrent();
		if (thread) {
			thread->wait(ms);
		} else {
			System::sleep(ms);
		}
	}

	sl_bool Thread::isCurrentThread()
	{
		return _nativeGetCurrentThread() == this;
	}

	Thread* Thread::getCurrent()
	{
		return _nativeGetCurrentThread();
	}

	sl_bool Thread::isStoppingCurrent()
	{
		Thread* thread = getCurrent();
		if (thread) {
			return thread->isStopping();
		}
		return sl_false;
	}

	sl_bool Thread::isNotStoppingCurrent()
	{
		Thread* thread = getCurrent();
		if (thread) {
			return thread->isNotStopping();
		}
		return sl_true;
	}

	sl_uint64 Thread::getCurrentThreadUniqueId()
	{
		static volatile sl_int64 uid = 10000;
		sl_uint64 n = _nativeGetCurrentThreadUniqueId();
		if (n > 0) {
			return n;
		}
		n = Base::interlockedIncrement64(&uid);
		_nativeSetCurrentThreadUniqueId(n);
		return n;
	}

	Ref<Referable> Thread::getAttachedObject(const String& name)
	{
		return m_attachedObjects.getValue(name, Ref<Referable>::null());
	}

	void Thread::attachObject(const String& name, Referable* object)
	{
		m_attachedObjects.put(name, object);
	}

	void Thread::removeAttachedObject(const String& name)
	{
		m_attachedObjects.remove(name);
	}

	void Thread::_run()
	{
#if defined(SLIB_PLATFORM_USE_JNI)
		Jni::attachThread();
#endif

		Thread::_nativeSetCurrentThread(this);
		m_callback();
		m_callback.setNull();

		m_attachedObjects.removeAll();

#if defined(SLIB_PLATFORM_USE_JNI)
		Jni::detachThread();
#endif

		_nativeClose();
		m_handle = sl_null;
		m_flagRunning = sl_false;
		m_eventExit->set();
	}


	Thread* CurrentThread::get() noexcept
	{
		_init();
		return m_thread;
	}

	sl_bool CurrentThread::isNotNull() noexcept
	{
		_init();
		return m_thread != sl_null;
	}

	sl_bool CurrentThread::isNull() noexcept
	{
		_init();
		return !m_thread;
	}

	void CurrentThread::sleep(sl_uint32 ms) noexcept
	{
		_init();
		if (m_thread) {
			m_thread->wait(ms);
		} else {
			System::sleep(ms);
		}
	}

	sl_bool CurrentThread::isStopping() noexcept
	{
		_init();
		if (m_thread) {
			return m_thread->isStopping();
		} else {
			return sl_false;
		}
	}

	sl_bool CurrentThread::isNotStopping() noexcept
	{
		_init();
		if (m_thread) {
			return m_thread->isNotStopping();
		} else {
			return sl_true;
		}
	}

	void CurrentThread::_init() noexcept
	{
		if (!m_flagInit) {
			m_flagInit = sl_true;
			m_thread = Thread::getCurrent();
		}
	}


	SLIB_DEFINE_OBJECT(ThreadPool, Dispatcher)

	ThreadPool::ThreadPool()
	{
		m_minimumThreadsCount = 1;
		m_maximumThreadsCount = 30;
		m_threadStackSize = SLIB_THREAD_DEFAULT_STACK_SIZE;
		m_flagRunning = sl_true;
	}

	ThreadPool::~ThreadPool()
	{
		release();
	}

	Ref<ThreadPool> ThreadPool::create(sl_uint32 minThreads, sl_uint32 maxThreads)
	{
		Ref<ThreadPool> ret = new ThreadPool();
		if (ret.isNotNull()) {
			ret->setMinimumThreadsCount(minThreads);
			ret->setMaximumThreadsCount(maxThreads);
		}
		return ret;
	}

	sl_uint32 ThreadPool::getMinimumThreadsCount()
	{
		return m_minimumThreadsCount;
	}

	void ThreadPool::setMinimumThreadsCount(sl_uint32 n)
	{
		m_minimumThreadsCount = n;
	}

	sl_uint32 ThreadPool::getMaximumThreadsCount()
	{
		return m_maximumThreadsCount;
	}

	void ThreadPool::setMaximumThreadsCount(sl_uint32 n)
	{
		m_maximumThreadsCount = n;
	}

	sl_uint32 ThreadPool::getThreadStackSize()
	{
		return m_threadStackSize;
	}

	void ThreadPool::setThreadStackSize(sl_uint32 n)
	{
		m_threadStackSize = n;
	}

	void ThreadPool::release()
	{
		ObjectLocker lock(this);
		if (!m_flagRunning) {
			return;
		}
		m_flagRunning = sl_false;

		ListElements< Ref<Thread> > threads(m_threadWorkers);
		sl_size i;
		for (i = 0; i < threads.count; i++) {
			threads[i]->finish();
		}
		for (i = 0; i < threads.count; i++) {
			threads[i]->finishAndWait();
		}
	}

	sl_bool ThreadPool::isRunning()
	{
		return m_flagRunning;
	}

	sl_uint32 ThreadPool::getThreadsCount()
	{
		return (sl_uint32)(m_threadWorkers.getCount());
	}

	sl_bool ThreadPool::addTask(const Function<void()>& task)
	{
		if (task.isNull()) {
			return sl_false;
		}
		ObjectLocker lock(this);
		if (!m_flagRunning) {
			return sl_false;
		}
		// add task
		if (!(m_tasks.push(task))) {
			return sl_false;
		}

		// wake a sleeping worker
		{
			Ref<Thread> thread;
			if (m_threadSleeping.pop_NoLock(&thread)) {
				thread->wakeSelfEvent();
				return sl_true;
			}
		}

		// increase workers
		{
			sl_size nThreads = m_threadWorkers.getCount();
			if (nThreads == 0 || (nThreads < getMaximumThreadsCount())) {
				Ref<Thread> worker = Thread::start(SLIB_FUNCTION_MEMBER(ThreadPool, onRunWorker, this), getThreadStackSize());
				if (worker.isNotNull()) {
					m_threadWorkers.add_NoLock(worker);
				}
			}
		}
		return sl_true;
	}

	sl_bool ThreadPool::dispatch(const Function<void()>& callback, sl_uint64 delayMillis)
	{
		if (delayMillis) {
			return setTimeoutByDefaultDispatchLoop(callback, delayMillis);
		}
		return addTask(callback);
	}

	void ThreadPool::onRunWorker()
	{
		Thread* thread = Thread::getCurrent();
		if (!thread) {
			return;
		}
		while (m_flagRunning && thread->isNotStopping()) {
			Function<void()> task;
			if (m_tasks.pop(&task)) {
				task();
			} else {
				ObjectLocker lock(this);
				sl_size nThreads = m_threadWorkers.getCount();
				if (nThreads > getMinimumThreadsCount()) {
					m_threadWorkers.remove_NoLock(thread);
					return;
				} else {
					m_threadSleeping.push_NoLock(thread);
					lock.unlock();
					thread->wait();
				}
			}
		}
	}

}
