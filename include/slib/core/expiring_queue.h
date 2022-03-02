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

#ifndef CHECKHEADER_SLIB_CORE_EXPIRING_QUEUE
#define CHECKHEADER_SLIB_CORE_EXPIRING_QUEUE

#include "linked_list.h"
#include "dispatch_loop.h"
#include "timer.h"

namespace slib
{
	
	template <class T, sl_uint32 LAYER_COUNT = 2>
	class SLIB_EXPORT ExpiringQueue : public Lockable
	{
	public:
		ExpiringQueue()
		{
			m_duration = 0;
		}

		~ExpiringQueue()
		{
			_release();
		}

	public:
		sl_uint32 getExpiringMilliseconds() const
		{
			return m_duration;
		}

		void setExpiringMilliseconds(sl_uint32 expiring_duration_ms)
		{
			if (m_duration != expiring_duration_ms) {
				m_duration = expiring_duration_ms;
				if (m_timer.isNotNull()) {
					ObjectLocker lock(this);
					_setupTimer();
				}
			}
		}

		Ref<DispatchLoop> getDispatchLoop() const
		{
			ObjectLocker lock(this);
			return m_dispatchLoop;
		}

		void setDispatchLoop(const Ref<DispatchLoop>& loop)
		{
			if (m_dispatchLoop != loop) {
				ObjectLocker lock(this);
				m_dispatchLoop = loop;
				if (m_timer.isNotNull()) {
					_setupTimer();
				}
			}
		}

		void setupTimer(sl_uint32 expiring_duration_ms, const Ref<DispatchLoop>& loop)
		{
			if (m_duration != expiring_duration_ms || m_dispatchLoop != loop) {
				ObjectLocker lock(this);
				m_duration = expiring_duration_ms;
				m_dispatchLoop = loop;
				if (m_timer.isNotNull()) {
					_setupTimer();
				}
			}
		}

		template <class... ARGS>
		sl_bool push(ARGS&&... args) noexcept
		{
			ObjectLocker lock(this);
			if (m_queue->pushBack_NoLock(Forward<ARGS>(args)...)) {
				if (m_timer.isNull()) {
					_setupTimer();
				}
				return sl_true;
			}
			return sl_false;
		}

		sl_bool pop(T* _out = sl_null)
		{
			ObjectLocker lock(this);
			sl_uint32 n = LAYER_COUNT - 1;
			for (;;) {
				if (m_queue[n].popFront_NoLock(_out)) {
					return sl_true;
				}
				if (n) {
					n--;
				} else {
					break;
				}
			}
			return sl_false;
		}

		void removeAll()
		{
			ObjectLocker lock(this);
			for (sl_uint32 i = 0; i < LAYER_COUNT; i++) {
				m_queue[i].removeAll_NoLock();
			}
		}

	protected:
		void _update(Timer* timer)
		{
			ObjectLocker lock(this);
			sl_uint32 n = LAYER_COUNT - 1;
			sl_bool flagEmpty = sl_true;
			for (;;) {
				m_queue[n] = Move(m_queue[n - 1]);
				if (m_queue[n].isNotNull()) {
					flagEmpty = sl_false;
				}
				if (n >= 2) {
					n--;
				} else {
					break;
				}
			}
			if (flagEmpty) {
				if (m_timer.isNotNull()) {
					m_timer->stop();
					m_timer.setNull();
				}
			}
		}

		void _setupTimer()
		{
			if (m_timer.isNotNull()) {
				m_timer->stopAndWait();
				m_timer.setNull();
			}
			sl_uint32 duration = m_duration;
			if (duration > 0) {
				m_timer = Timer::startWithLoop(m_dispatchLoop, SLIB_FUNCTION_MEMBER(this, _update), duration / LAYER_COUNT);
			}
		}

		void _release()
		{
			if (m_timer.isNotNull()) {
				m_timer->stopAndWait();
			}
		}

	protected:
		sl_uint32 m_duration;
		Ref<DispatchLoop> m_dispatchLoop;

		Ref<Timer> m_timer;

		LinkedList<T> m_queue[LAYER_COUNT];

	};

}

#endif
