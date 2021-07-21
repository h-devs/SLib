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

#include "slib/core/rw_lock.h"
#include "slib/core/rw_spin_lock.h"
#include "slib/core/rw_lockable.h"

#include "slib/core/base.h"

#if defined(SLIB_PLATFORM_IS_UNIX)
#include <pthread.h>
#endif

namespace slib
{

	ReadWriteLock::ReadWriteLock() noexcept
	{
		_init();
	}

	ReadWriteLock::ReadWriteLock(const ReadWriteLock& other) noexcept
	{
		_init();
	}
	
	ReadWriteLock::ReadWriteLock(ReadWriteLock&& other) noexcept
	{
		_init();
	}

	ReadWriteLock::~ReadWriteLock() noexcept
	{
#if defined(SLIB_PLATFORM_IS_UNIX)
		_free();
#endif
	}

	void ReadWriteLock::_init() noexcept
	{
#if defined(SLIB_PLATFORM_IS_UNIX)
		m_pObject = Base::createMemory(sizeof(pthread_rwlock_t));
		::pthread_rwlock_init((pthread_rwlock_t*)(m_pObject), NULL);
#else
		m_nReading = 0;
#endif
	}

	void ReadWriteLock::_free() noexcept
	{
#if defined(SLIB_PLATFORM_IS_UNIX)
		::pthread_rwlock_destroy((pthread_rwlock_t*)(m_pObject));
		Base::freeMemory(m_pObject);
		m_pObject = sl_null;
#endif
	}

	sl_bool ReadWriteLock::tryLockRead() const noexcept
	{
#if defined(SLIB_PLATFORM_IS_UNIX)
		if (!m_pObject) {
			return sl_false;
		}
		return ::pthread_rwlock_tryrdlock((pthread_rwlock_t*)(m_pObject)) == 0;
#else
		if (m_lockReading.tryLock()) {
			m_nReading++;
			if (m_nReading == 1) {
				if (m_lockWriting.tryLock()) {
					m_lockReading.unlock();
					return sl_true;
				} else {
					m_nReading = 0;
					m_lockReading.unlock();
					return sl_false;
				}
			} else {
				m_lockReading.unlock();
				return sl_true;
			}
		} else {
			return sl_false;
		}
#endif
	}
	
	void ReadWriteLock::lockRead() const noexcept
	{
#if defined(SLIB_PLATFORM_IS_UNIX)
		if (!m_pObject) {
			return;
		}
		::pthread_rwlock_rdlock((pthread_rwlock_t*)(m_pObject));
#else
		MutexLocker lock(&m_lockReading);
		m_nReading++;
		if (m_nReading == 1) {
			m_lockWriting.lock();
		}
#endif
	}

	void ReadWriteLock::unlockRead() const noexcept
	{
#if defined(SLIB_PLATFORM_IS_UNIX)
		if (!m_pObject) {
			return;
		}
		pthread_rwlock_unlock((pthread_rwlock_t*)(m_pObject));
#else
		MutexLocker lock(&m_lockReading);
		m_nReading--;
		if (m_nReading == 0) {
			m_lockWriting.unlock();
		}
#endif
	}

	sl_bool ReadWriteLock::tryLockWrite() const noexcept
	{
#if defined(SLIB_PLATFORM_IS_UNIX)
		if (!m_pObject) {
			return sl_false;
		}
		return ::pthread_rwlock_trywrlock((pthread_rwlock_t*)(m_pObject)) == 0;
#else
		return m_lockWriting.tryLock();
#endif
	}
	void ReadWriteLock::lockWrite() const noexcept
	{
#if defined(SLIB_PLATFORM_IS_UNIX)
		if (!m_pObject) {
			return;
		}
		::pthread_rwlock_wrlock((pthread_rwlock_t*)(m_pObject));
#else
		m_lockWriting.lock();
#endif
	}

	void ReadWriteLock::unlockWrite() const noexcept
	{
#if defined(SLIB_PLATFORM_IS_UNIX)
		if (!m_pObject) {
			return;
		}
		pthread_rwlock_unlock((pthread_rwlock_t*)(m_pObject));
#else
		m_lockWriting.unlock();
#endif
	}
	
	ReadWriteLock& ReadWriteLock::operator=(const ReadWriteLock& other) noexcept
	{
		return *this;
	}
	
	ReadWriteLock& ReadWriteLock::operator=(ReadWriteLock&& other) noexcept
	{
		return *this;
	}


	ReadLocker::ReadLocker() noexcept
	{
		m_lock = sl_null;
	}

	ReadLocker::ReadLocker(const ReadWriteLock* lock) noexcept
	{
		m_lock = lock;
		if (lock) {
			lock->lockRead();
		}
	}

	ReadLocker::~ReadLocker() noexcept
	{
		unlock();
	}

	void ReadLocker::lock(const ReadWriteLock* lock) noexcept
	{
		if (m_lock) {
			return;
		}
		if (lock) {
			m_lock = lock;
			lock->lockRead();
		}
	}

	void ReadLocker::unlock() noexcept
	{
		const ReadWriteLock* lock = m_lock;
		if (lock) {
			m_lock->unlockRead();
			m_lock = sl_null;
		}
	}

	
	WriteLocker::WriteLocker() noexcept
	{
		m_lock = sl_null;
	}
	
	WriteLocker::WriteLocker(const ReadWriteLock* lock) noexcept
	{
		m_lock = lock;
		if (lock) {
			lock->lockWrite();
		}
	}
	
	WriteLocker::~WriteLocker() noexcept
	{
		unlock();
	}
	
	void WriteLocker::lock(const ReadWriteLock* lock) noexcept
	{
		if (m_lock) {
			return;
		}
		if (lock) {
			m_lock = lock;
			lock->lockWrite();
		}
	}
	
	void WriteLocker::unlock() noexcept
	{
		const ReadWriteLock* lock = m_lock;
		if (lock) {
			m_lock->unlockWrite();
			m_lock = sl_null;
		}
	}


	sl_bool ReadWriteSpinLock::tryLockRead() const noexcept
	{
		if (m_lockReading.tryLock()) {
			m_nReading++;
			if (m_nReading == 1) {
				if (m_lockWriting.tryLock()) {
					m_lockReading.unlock();
					return sl_true;
				} else {
					m_nReading = 0;
					m_lockReading.unlock();
					return sl_false;
				}
			} else {
				m_lockReading.unlock();
				return sl_true;
			}
		} else {
			return sl_false;
		}
	}

	void ReadWriteSpinLock::lockRead() const noexcept
	{
		SpinLocker lock(&m_lockReading);
		m_nReading++;
		if (m_nReading == 1) {
			m_lockWriting.lock();
		}
	}

	void ReadWriteSpinLock::unlockRead() const noexcept
	{
		SpinLocker lock(&m_lockReading);
		m_nReading--;
		if (!m_nReading) {
			m_lockWriting.unlock();
		}
	}

	sl_bool ReadWriteSpinLock::tryLockWrite() const noexcept
	{
		return m_lockWriting.tryLock();
	}
	void ReadWriteSpinLock::lockWrite() const noexcept
	{
		m_lockWriting.lock();
	}

	void ReadWriteSpinLock::unlockWrite() const noexcept
	{
		m_lockWriting.unlock();
	}

	ReadWriteSpinLock& ReadWriteSpinLock::operator=(const ReadWriteSpinLock& other) noexcept
	{
		return *this;
	}

	ReadWriteSpinLock& ReadWriteSpinLock::operator=(ReadWriteSpinLock&& other) noexcept
	{
		return *this;
	}


	ReadSpinLocker::ReadSpinLocker() noexcept
	{
		m_lock = sl_null;
	}

	ReadSpinLocker::ReadSpinLocker(const ReadWriteSpinLock* lock) noexcept
	{
		m_lock = lock;
		if (lock) {
			lock->lockRead();
		}
	}

	ReadSpinLocker::~ReadSpinLocker() noexcept
	{
		unlock();
	}

	void ReadSpinLocker::lock(const ReadWriteSpinLock* lock) noexcept
	{
		if (m_lock) {
			return;
		}
		if (lock) {
			m_lock = lock;
			lock->lockRead();
		}
	}

	void ReadSpinLocker::unlock() noexcept
	{
		const ReadWriteSpinLock* lock = m_lock;
		if (lock) {
			m_lock->unlockRead();
			m_lock = sl_null;
		}
	}


	WriteSpinLocker::WriteSpinLocker() noexcept
	{
		m_lock = sl_null;
	}

	WriteSpinLocker::WriteSpinLocker(const ReadWriteSpinLock* lock) noexcept
	{
		m_lock = lock;
		if (lock) {
			lock->lockWrite();
		}
	}

	WriteSpinLocker::~WriteSpinLocker() noexcept
	{
		unlock();
	}

	void WriteSpinLocker::lock(const ReadWriteSpinLock* lock) noexcept
	{
		if (m_lock) {
			return;
		}
		if (lock) {
			m_lock = lock;
			lock->lockWrite();
		}
	}

	void WriteSpinLocker::unlock() noexcept
	{
		const ReadWriteSpinLock* lock = m_lock;
		if (lock) {
			m_lock->unlockWrite();
			m_lock = sl_null;
		}
	}


	RWLockable::RWLockable() noexcept : m_nReading(0)
	{
	}

	RWLockable::~RWLockable() noexcept
	{
	}

	sl_bool RWLockable::tryLockRead() const noexcept
	{
		if (m_lockReading.tryLock()) {
			m_nReading++;
			if (m_nReading == 1) {
				if (m_locker.tryLock()) {
					m_lockReading.unlock();
					return sl_true;
				} else {
					m_nReading = 0;
					m_lockReading.unlock();
					return sl_false;
				}
			} else {
				m_lockReading.unlock();
				return sl_true;
			}
		} else {
			return sl_false;
		}
	}

	void RWLockable::lockRead() const noexcept
	{
		SpinLocker lock(&m_lockReading);
		m_nReading++;
		if (m_nReading == 1) {
			m_locker.lock();
		}
	}

	void RWLockable::unlockRead() const noexcept
	{
		SpinLocker lock(&m_lockReading);
		m_nReading--;
		if (!m_nReading) {
			m_locker.unlock();
		}
	}


	ReadObjectLocker::ReadObjectLocker() noexcept : m_object(sl_null)
	{
	}

	ReadObjectLocker::ReadObjectLocker(const RWLockable* object) noexcept : m_object(object)
	{
		if (object) {
			object->lockRead();
		}
	}

	ReadObjectLocker::~ReadObjectLocker() noexcept
	{
		unlock();
	}

	void ReadObjectLocker::lock(const RWLockable* object) noexcept
	{
		if (m_object) {
			return;
		}
		if (object) {
			m_object = object;
			object->lockRead();
		}
	}

	void ReadObjectLocker::unlock() noexcept
	{
		const RWLockable* object = m_object;
		if (object) {
			object->unlockRead();
			m_object = sl_null;
		}
	}

}
