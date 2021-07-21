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

#ifndef CHECKHEADER_SLIB_CORE_RW_LOCKABLE
#define CHECKHEADER_SLIB_CORE_RW_LOCKABLE

#include "lockable.h"

namespace slib
{

	class SLIB_EXPORT RWLockable : public Lockable
	{
	public:
		RWLockable() noexcept;

		RWLockable(const RWLockable& other) = delete;

		RWLockable(RWLockable&& other) = delete;

		~RWLockable() noexcept;

	public:
		sl_bool tryLockRead() const noexcept;

		void lockRead() const noexcept;

		void unlockRead() const noexcept;

	public:
		RWLockable & operator=(const RWLockable& other) = delete;

		RWLockable& operator=(RWLockable&& other) = delete;

	protected:
		SpinLock m_lockReading;
		mutable sl_reg m_nReading;

	};

	class SLIB_EXPORT ReadObjectLocker
	{
	public:
		ReadObjectLocker() noexcept;

		ReadObjectLocker(const RWLockable* object) noexcept;

		~ReadObjectLocker() noexcept;

	public:
		void lock(const RWLockable* object) noexcept;

		void unlock() noexcept;

	private:
		const RWLockable* m_object;

	};

}

#endif

