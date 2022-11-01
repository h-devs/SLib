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

#ifndef CHECKHEADER_SLIB_CORE_RW_SPIN_LOCK
#define CHECKHEADER_SLIB_CORE_RW_SPIN_LOCK

#include "spin_lock.h"

namespace slib
{

	class SLIB_EXPORT ReadWriteSpinLock
	{
	public:
		SLIB_CONSTEXPR ReadWriteSpinLock(): m_nReading(0) {}

		SLIB_CONSTEXPR ReadWriteSpinLock(const ReadWriteSpinLock& other): m_nReading(0) {}

		SLIB_CONSTEXPR ReadWriteSpinLock(ReadWriteSpinLock&& other): m_nReading(0) {}

	public:
		sl_bool tryLockRead() const noexcept;

		void lockRead() const noexcept;

		void unlockRead() const noexcept;

		sl_bool tryLockWrite() const noexcept;

		void lockWrite() const noexcept;

		void unlockWrite() const noexcept;

	public:
		ReadWriteSpinLock& operator=(const ReadWriteSpinLock& other) noexcept;

		ReadWriteSpinLock& operator=(ReadWriteSpinLock&& other) noexcept;

	private:
		SpinLock m_lockReading;
		SpinLock m_lockWriting;
		mutable sl_reg m_nReading;

	};

	class SLIB_EXPORT ReadSpinLocker
	{
	public:
		ReadSpinLocker() noexcept;

		ReadSpinLocker(const ReadWriteSpinLock* lock) noexcept;

		ReadSpinLocker(const ReadSpinLocker& other) = delete;

		ReadSpinLocker(ReadSpinLocker&& other) = delete;

		~ReadSpinLocker() noexcept;

	public:
		void lock(const ReadWriteSpinLock* rwLock) noexcept;

		void unlock() noexcept;

	public:
		ReadSpinLocker& operator=(const ReadSpinLocker& other) = delete;

		ReadSpinLocker& operator=(ReadSpinLocker&& other) = delete;

	private:
		const ReadWriteSpinLock* m_lock;

	};

	class SLIB_EXPORT WriteSpinLocker
	{
	public:
		WriteSpinLocker() noexcept;

		WriteSpinLocker(const ReadWriteSpinLock* lock) noexcept;

		WriteSpinLocker(const WriteSpinLocker& other) = delete;

		WriteSpinLocker(WriteSpinLocker&& other) = delete;

		~WriteSpinLocker() noexcept;

	public:
		void lock(const ReadWriteSpinLock* rwLock) noexcept;

		void unlock() noexcept;

	public:
		WriteSpinLocker& operator=(const WriteSpinLocker& other) = delete;

		WriteSpinLocker& operator=(WriteSpinLocker&& other) = delete;

	private:
		const ReadWriteSpinLock* m_lock;

	};

}

#endif
