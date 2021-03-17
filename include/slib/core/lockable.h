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

#ifndef CHECKHEADER_SLIB_CORE_LOCKABLE
#define CHECKHEADER_SLIB_CORE_LOCKABLE

#include "mutex.h"

namespace slib
{

	class SLIB_EXPORT Lockable
	{
	public:
		Lockable() noexcept;

		Lockable(const Lockable& other) = delete;

		Lockable(Lockable&& other) = delete;

		~Lockable() noexcept;

	public:
		Mutex* getLocker() const noexcept;

		void lock() const noexcept;

		void unlock() const noexcept;

		sl_bool tryLock() const noexcept;

	public:
		Lockable& operator=(const Lockable& other) = delete;

		Lockable& operator=(Lockable&& other) = delete;

	protected:
		Mutex m_locker;

	};

	class SLIB_EXPORT ObjectLocker : public MutexLocker
	{
	public:
		ObjectLocker() noexcept;

		ObjectLocker(const Lockable* object) noexcept;

		~ObjectLocker() noexcept;

	public:
		void lock(const Lockable* object) noexcept;

	};

	class SLIB_EXPORT MultipleObjectsLocker : public MultipleMutexLocker
	{
	public:
		MultipleObjectsLocker() noexcept;

		MultipleObjectsLocker(const Lockable* object) noexcept;

		MultipleObjectsLocker(const Lockable* object1, const Lockable* object2) noexcept;

		~MultipleObjectsLocker() noexcept;

	public:
		void lock(const Lockable* object) noexcept;

		void lock(const Lockable* object1, const Lockable* object2) noexcept;

	};

}

#endif

