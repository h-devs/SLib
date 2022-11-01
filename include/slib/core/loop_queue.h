/*
 *   Copyright (c) 2008-2018 SLIBIO <https://github.com/SLIBIO>
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

#ifndef CHECKHEADER_SLIB_CORE_LOOP_QUEUE
#define CHECKHEADER_SLIB_CORE_LOOP_QUEUE

#include "lockable.h"
#include "new_helper.h"

namespace slib
{

	template <class T>
	class SLIB_EXPORT LoopQueue : public Lockable
	{
	protected:
		T* m_data;
		sl_size m_size;
		sl_size m_first;
		sl_size m_count;
		sl_size m_latency;

	public:
		LoopQueue(sl_size size = 0, sl_size latency = 0) noexcept
		{
			m_first = 0;
			m_count = 0;
			m_latency = latency;
			do {
				if (size) {
					m_data = NewHelper<T>::create(size);
					if (m_data) {
						m_size = size;
						break;
					}
				} else {
					m_data = sl_null;
				}
				m_size = 0;
			} while (0);
		}

		~LoopQueue() noexcept
		{
			if (m_data) {
				NewHelper<T>::free(m_data, m_size);
			}
		}

	public:
		sl_size getQueueSize() const noexcept
		{
			return m_size;
		}

		sl_bool setQueueSize(sl_size size) noexcept
		{
			ObjectLocker lock(this);
			if (m_data) {
				NewHelper<T>::free(m_data, m_size);
				m_data = sl_null;
				m_first = 0;
				m_count = 0;
				m_size = 0;
			}
			if (size) {
				m_data = NewHelper<T>::create(size);
				if (m_data) {
					m_size = size;
					return sl_true;
				}
			} else {
				return sl_true;
			}
			return sl_false;
		}

		sl_size removeAll() noexcept
		{
			ObjectLocker lock(this);
			sl_size count = m_count;
			m_first = 0;
			m_count = 0;
			return count;
		}

		T* getBuffer() const noexcept
		{
			return m_data;
		}

		sl_size getCount() const noexcept
		{
			return m_count;
		}

		void setLatency(sl_size latency) noexcept
		{
			m_latency = latency;
		}

		sl_size getLatency() const noexcept
		{
			return m_latency;
		}

		template <class VALUE>
		sl_bool push_NoLock(VALUE&& value, sl_bool flagShift = sl_true) noexcept
		{
			if (!m_size) {
				return sl_false;
			}
			if (!flagShift && m_count == m_size) {
				return sl_false;
			}
			sl_size last = m_first + m_count;
			m_data[last % m_size] = Forward<VALUE>(value);
			m_count++;
			if (m_count > m_size) {
				m_count = m_size;
				m_first = (last + 1) % m_size;
			}
			return sl_true;
		}

		template <class VALUE>
		sl_bool push(VALUE&& value, sl_bool flagShift = sl_true) noexcept
		{
			ObjectLocker lock(this);
			return push_NoLock(Forward<VALUE>(value), flagShift);
		}

		template <class VALUE>
		sl_bool push_NoLock(VALUE&& value, VALUE* _outShifted) noexcept
		{
			if (!m_size) {
				return sl_false;
			}
			sl_size last = m_first + m_count;
			T* p = m_data + (last % m_size);
			*_outShifted = Move(*p);
			new (p) T(Forward<VALUE>(value));
			m_count++;
			if (m_count > m_size) {
				m_count = m_size;
				m_first = (last + 1) % m_size;
			}
			return sl_true;
		}

		template <class VALUE>
		sl_bool push(VALUE&& value, VALUE* _outShifted) noexcept
		{
			ObjectLocker lock(this);
			return push_NoLock(Forward<VALUE>(value), _outShifted);
		}

		sl_bool pushAll_NoLock(const T* buffer, sl_size count, sl_bool flagShift = sl_true) noexcept
		{
			if (!m_size) {
				return sl_false;
			}
			if (!flagShift && m_count + count > m_size) {
				return sl_false;
			}
			sl_size last = m_first + m_count;
			if (count > m_size) {
				sl_size m = count - m_size;
				buffer += m;
				last += m;
				m_count += m;
				count = m_size;
			}
			sl_size i = last % m_size;
			sl_size n = 0;
			while (i < m_size && n < count) {
				m_data[i] = buffer[n];
				i++;
				n++;
			}
			i = 0;
			while (n < count) {
				m_data[i] = buffer[n];
				i++;
				n++;
			}
			m_count += count;
			if (m_count > m_size) {
				m_first = (m_first + m_count) % m_size;
				m_count = m_size;
			}
			return sl_true;
		}

		sl_bool pushAll(const T* buffer, sl_size count, sl_bool flagShift = sl_true) noexcept
		{
			ObjectLocker lock(this);
			return pushAll_NoLock(buffer, count, flagShift);
		}

		sl_bool pop_NoLock(T& output) noexcept
		{
			if (m_count > m_latency) {
				output = Move(m_data[m_first % m_size]);
				m_first = (m_first + 1) % m_size;
				m_count --;
				return sl_true;
			}
			return sl_false;
		}

		sl_bool pop(T& output) noexcept
		{
			ObjectLocker lock(this);
			return pop_NoLock(output);
		}

		T pop_NoLock() noexcept
		{
			if (m_count > m_latency) {
				T ret = Move(m_data[m_first % m_size]);
				m_first = (m_first + 1) % m_size;
				m_count--;
				return ret;
			} else {
				return T();
			}
		}

		T pop() noexcept
		{
			ObjectLocker lock(this);
			return pop_NoLock();
		}

		sl_bool pop_NoLock(T* buffer, sl_size count) noexcept
		{
			if (count <= m_count && m_count > m_latency) {
				sl_size n = 0;
				sl_size i = m_first;
				while (i < m_size && n < count) {
					buffer[n] = Move(m_data[i]);
					i++;
					n++;
				}
				i = 0;
				while (n < count) {
					buffer[n] = Move(m_data[i]);
					i++;
					n++;
				}
				m_first = (m_first + count) % m_size;
				m_count -= count;
				return sl_true;
			}
			return sl_false;
		}

		sl_bool pop(T* buffer, sl_size count) noexcept
		{
			ObjectLocker lock(this);
			return pop_NoLock(buffer, count);
		}

		sl_size read_NoLock(sl_size offset, T* _out, sl_size count) const noexcept
		{
			if (offset > m_count) {
				return 0;
			}
			sl_size n = m_count - offset;
			if (count > n) {
				count = n;
			}
			{
				n = 0;
				sl_size i = (m_first + offset) % m_size;
				while (n < count) {
					_out[n] = m_data[i];
					i++;
					if (i >= m_size) {
						i = 0;
					}
					n++;
				}
			}
			return count;
		}

		sl_size read(sl_size offset, T* _out, sl_size count) const noexcept
		{
			ObjectLocker lock(this);
			return read_NoLock(offset, _out, count);
		}

		sl_size read_NoLock(T* _out, sl_size count) const noexcept
		{
			return read_NoLock(0, _out, count);
		}

		sl_size read(T* _out, sl_size count) const noexcept
		{
			return read(0, _out, count);
		}

	};


	template <class T, sl_size SIZE>
	class SLIB_EXPORT StaticLoopQueue
	{
	protected:
		T m_data[SIZE];
		sl_size m_first;
		sl_size m_count;
		SpinLock m_lock;

	public:
		StaticLoopQueue() noexcept
		{
			m_first = 0;
			m_count = 0;
		}

	public:
		sl_size removeAll() noexcept
		{
			SpinLocker locker(&m_lock);
			sl_size count = m_count;
			m_first = 0;
			m_count = 0;
			return count;
		}

		T* getBuffer() const noexcept
		{
			return m_data;
		}

		sl_size getCount() const noexcept
		{
			return m_count;
		}

		template <class VALUE>
		sl_bool push_NoLock(VALUE&& value, sl_bool flagShift = sl_true) noexcept
		{
			if (!flagShift && m_count == SIZE) {
				return sl_false;
			}
			sl_size last = m_first + m_count;
			m_data[last % SIZE] = Forward<VALUE>(value);
			m_count++;
			if (m_count > SIZE) {
				m_count = SIZE;
				m_first = (last + 1) % SIZE;
			}
			return sl_true;
		}

		template <class VALUE>
		sl_bool push(VALUE&& value, sl_bool flagShift = sl_true) noexcept
		{
			SpinLocker locker(&m_lock);
			return push_NoLock(Forward<VALUE>(value), flagShift);
		}

		template <class VALUE>
		void push_NoLock(VALUE&& value, VALUE* _outShifted) noexcept
		{
			sl_size last = m_first + m_count;
			T* p = m_data + (last % SIZE);
			*_outShifted = Move(*p);
			new (p) T(Forward<VALUE>(value));
			m_count++;
			if (m_count > SIZE) {
				m_count = SIZE;
				m_first = (last + 1) % SIZE;
			}
		}

		template <class VALUE>
		void push(VALUE&& value, VALUE* _outShifted) noexcept
		{
			SpinLocker locker(&m_lock);
			push_NoLock(Forward<VALUE>(value), _outShifted);
		}

		sl_bool pop_NoLock(T& output) noexcept
		{
			if (m_count) {
				output = Move(m_data[m_first % SIZE]);
				m_first = (m_first + 1) % SIZE;
				m_count --;
				return sl_true;
			}
			return sl_false;
		}

		sl_bool pop(T& output) noexcept
		{
			SpinLocker locker(&m_lock);
			return pop_NoLock(output);
		}

		T pop_NoLock() noexcept
		{
			if (m_count) {
				T ret = Move(m_data[m_first % SIZE]);
				m_first = (m_first + 1) % SIZE;
				m_count--;
				return ret;
			} else {
				return T();
			}
		}

		T pop() noexcept
		{
			SpinLocker locker(&m_lock);
			return pop_NoLock();
		}

	};

}

#endif
