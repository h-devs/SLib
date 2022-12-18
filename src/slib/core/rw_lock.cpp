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

#include "slib/core/base.h"

#if defined(SLIB_PLATFORM_IS_WIN32)
#include "slib/dl/win32/kernel32.h"
#else
#include <pthread.h>
#endif

namespace slib
{

	ReadWriteLock::ReadWriteLock() noexcept
	{
#if defined(SLIB_PLATFORM_IS_WIN32)
		if (kernel32::getApi_TryAcquireSRWLockExclusive()) {
			m_pObject = Base::createMemory(sizeof(SRWLOCK));
			kernel32::getApi_InitializeSRWLock()((SRWLOCK*)m_pObject);
			m_flagSpinLock = sl_false;
		} else {
			m_pObject = Base::createMemory(sizeof(ReadWriteSpinLock));
			Base::zeroMemory(m_pObject, sizeof(ReadWriteSpinLock));
			m_flagSpinLock = sl_true;
		}
#else
		m_pObject = Base::createMemory(sizeof(pthread_rwlock_t));
		pthread_rwlock_init((pthread_rwlock_t*)m_pObject, NULL);
#endif
	}

	ReadWriteLock::ReadWriteLock(const ReadWriteLock& other) noexcept: ReadWriteLock()
	{
	}

	ReadWriteLock::ReadWriteLock(ReadWriteLock&& other) noexcept: ReadWriteLock()
	{
	}

	ReadWriteLock::~ReadWriteLock() noexcept
	{
#if defined(SLIB_PLATFORM_IS_WIN32)
		Base::freeMemory(m_pObject);
#else
		pthread_rwlock_destroy((pthread_rwlock_t*)m_pObject);
		Base::freeMemory(m_pObject);
#endif
		m_pObject = sl_null;
	}

	sl_bool ReadWriteLock::tryLockRead() const noexcept
	{
#if defined(SLIB_PLATFORM_IS_WIN32)
		if (m_flagSpinLock) {
			return ((ReadWriteSpinLock*)m_pObject)->tryLockRead();
		} else {
			return kernel32::getApi_TryAcquireSRWLockShared()((SRWLOCK*)m_pObject);
		}
#else
		return !(pthread_rwlock_tryrdlock((pthread_rwlock_t*)m_pObject));
#endif
	}

	void ReadWriteLock::lockRead() const noexcept
	{
#if defined(SLIB_PLATFORM_IS_WIN32)
		if (m_flagSpinLock) {
			((ReadWriteSpinLock*)m_pObject)->lockRead();
		} else {
			kernel32::getApi_AcquireSRWLockShared()((SRWLOCK*)m_pObject);
		}
#else
		pthread_rwlock_rdlock((pthread_rwlock_t*)m_pObject);
#endif
	}

	void ReadWriteLock::unlockRead() const noexcept
	{
#if defined(SLIB_PLATFORM_IS_WIN32)
		if (m_flagSpinLock) {
			((ReadWriteSpinLock*)m_pObject)->unlockRead();
		} else {
			kernel32::getApi_ReleaseSRWLockShared()((SRWLOCK*)m_pObject);
		}
#else
		pthread_rwlock_unlock((pthread_rwlock_t*)m_pObject);
#endif
	}

	sl_bool ReadWriteLock::tryLockWrite() const noexcept
	{
#if defined(SLIB_PLATFORM_IS_WIN32)
		if (m_flagSpinLock) {
			return ((ReadWriteSpinLock*)m_pObject)->tryLockWrite();
		} else {
			return kernel32::getApi_TryAcquireSRWLockExclusive()((SRWLOCK*)m_pObject);
		}
#else
		return !(pthread_rwlock_trywrlock((pthread_rwlock_t*)m_pObject));
#endif
	}

	void ReadWriteLock::lockWrite() const noexcept
	{
#if defined(SLIB_PLATFORM_IS_WIN32)
		if (m_flagSpinLock) {
			((ReadWriteSpinLock*)m_pObject)->lockWrite();
		} else {
			kernel32::getApi_AcquireSRWLockExclusive()((SRWLOCK*)m_pObject);
		}
#else
		pthread_rwlock_wrlock((pthread_rwlock_t*)m_pObject);
#endif
	}

	void ReadWriteLock::unlockWrite() const noexcept
	{
#if defined(SLIB_PLATFORM_IS_WIN32)
		if (m_flagSpinLock) {
			((ReadWriteSpinLock*)m_pObject)->unlockWrite();
		} else {
			kernel32::getApi_ReleaseSRWLockExclusive()((SRWLOCK*)m_pObject);
		}
#else
		pthread_rwlock_unlock((pthread_rwlock_t*)m_pObject);
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
		if (m_lock) {
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
		if (m_lock) {
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
		if (m_lock) {
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
		if (m_lock) {
			m_lock->unlockWrite();
			m_lock = sl_null;
		}
	}

}
