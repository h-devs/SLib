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

#ifndef CHECKHEADER_SLIB_CORE_EXPIRING_MAP
#define CHECKHEADER_SLIB_CORE_EXPIRING_MAP

#include "hash_map.h"
#include "dispatch_loop.h"
#include "timer.h"

namespace slib
{
	
	template <class KT, class VT>
	class SLIB_EXPORT ExpiringMap : public Lockable
	{
	public:
		ExpiringMap()
		{
			m_duration = 0;
		}

		~ExpiringMap()
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

		sl_size getCount() const
		{
			return m_mapBackup.getCount() + m_mapCurrent.getCount();
		}

		sl_bool isEmpty() const
		{
			return !(getCount());
		}

		sl_bool isNotEmpty() const
		{
			return getCount() != 0;
		}

		sl_bool get(const KT& key, VT* _out = sl_null, sl_bool flagUpdateLifetime = sl_true)
		{
			ObjectLocker lock(this);
			VT* p = m_mapCurrent.getItemPointer(key);
			if (p) {
				if (_out) {
					*_out = *p;
				}
				return sl_true;
			}
			auto node = m_mapBackup.find_NoLock(key);
			if (node) {
				if (_out) {
					*_out = node->value;
				}
				if (flagUpdateLifetime) {
					m_mapCurrent.add_NoLock(key, Move(node->value));
					m_mapBackup.removeAt(node);
				}
				return sl_true;
			}
			return sl_false;
		}

		VT getValue(const KT& key, const VT& def, sl_bool flagUpdateLifetime = sl_true)
		{
			ObjectLocker lock(this);
			VT* p = m_mapCurrent.getItemPointer(key);
			if (p) {
				return *p;
			}
			auto node = m_mapBackup.find_NoLock(key);
			if (node) {
				if (flagUpdateLifetime) {
					m_mapCurrent.add_NoLock(key, Move(node->value));
					m_mapBackup.removeAt(node);
				}
				return node->value;
			}
			return def;
		}

		template <class KEY, class VALUE>
		sl_bool put(KEY&& key, VALUE&& value)
		{
			ObjectLocker lock(this);
			m_mapBackup.remove_NoLock(key);
			if (m_mapCurrent.put_NoLock(Forward<KEY>(key), Forward<VALUE>(value))) {
				if (m_timer.isNull()) {
					_setupTimer();
				}
				return sl_true;
			}
			return sl_false;
		}

		template <class KEY, class... VALUE_ARGS>
		sl_bool add(KEY&& key, VALUE_ARGS&&... value_args) noexcept
		{
			ObjectLocker lock(this);
			if (m_mapCurrent.add_NoLock(Forward<KEY>(key), Forward<VALUE_ARGS>(value_args)...)) {
				if (m_timer.isNull()) {
					_setupTimer();
				}
				return sl_true;
			}
			return sl_false;
		}

		sl_bool remove(const KT& key, VT* outValue = sl_null)
		{
			ObjectLocker lock(this);
			if (m_mapCurrent.remove_NoLock(key, outValue)) {
				return sl_true;
			}
			return m_mapBackup.remove_NoLock(key, outValue);
		}

		void removeAll()
		{
			ObjectLocker lock(this);
			m_mapCurrent.removeAll_NoLock();
			m_mapBackup.removeAll_NoLock();
		}

		sl_bool contains(const KT& key) const
		{
			ObjectLocker lock(this);
			if (m_mapCurrent.find_NoLock(key)) {
				return sl_true;
			}
			if (m_mapBackup.find_NoLock(key)) {
				return sl_true;
			}
			return sl_false;
		}

		List<KT> getAllKeys() const
		{
			ObjectLocker lock(this);
			List<KT> ret = m_mapCurrent.getAllKeys_NoLock();
			ret.addAll_NoLock(m_mapBackup.getAllKeys_NoLock());
			return ret;
		}

		List<VT> getAllValues() const
		{
			ObjectLocker lock(this);
			List<VT> ret = m_mapCurrent.getAllValues_NoLock();
			ret.addAll_NoLock(m_mapBackup.getAllValues_NoLock());
			return ret;
		}

		List< Pair<KT, VT> > toList() const
		{
			ObjectLocker lock(this);
			List< Pair<KT, VT> > ret = m_mapCurrent.toList_NoLock();
			ret.addAll_NoLock(m_mapBackup.toList_NoLock());
			return ret;
		}

	protected:
		void _update(Timer* timer)
		{
			ObjectLocker lock(this);
			m_mapBackup = Move(m_mapCurrent);
			if (m_mapBackup.isEmpty()) {
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
				m_timer = Timer::startWithLoop(m_dispatchLoop, SLIB_FUNCTION_MEMBER(this, _update), duration);
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

		HashMap<KT, VT> m_mapCurrent;
		HashMap<KT, VT> m_mapBackup;

	};

}

#endif
