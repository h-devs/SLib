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

#ifndef CHECKHEADER_SLIB_CORE_MEMORY_TRAITS
#define CHECKHEADER_SLIB_CORE_MEMORY_TRAITS

#include "base.h"

namespace slib
{

	template <typename T, sl_size ElementSize>
	class MemoryTraitsBase;

	template <typename T>
	class MemoryTraitsFind
	{
	public:
		static T* find(const T* _m, sl_size count, const T* pattern, sl_size nPattern) noexcept
		{
			T* m = (T*)_m;
			if (!nPattern) {
				return m;
			}
			if (nPattern == 1) {
				return MemoryTraitsBase<T, sizeof(T)>::find(m, count, *pattern);
			}
			if (nPattern > count) {
				return sl_null;
			}
			nPattern--;
			const T* pattern1 = pattern + 1;
			sl_size n = count - nPattern;
			do {
				T* pt = MemoryTraitsBase<T, sizeof(T)>::find(m, n, *pattern);
				if (!pt) {
					return sl_null;
				}
				T* pt1 = pt + 1;
				if (MemoryTraitsBase<T, sizeof(T)>::equals(pt1, pattern1, nPattern)) {
					return pt;
				} else {
					n -= (sl_size)(pt1 - m);
					m = pt1;
				}
			} while (n);
			return sl_null;
		}

		static T* findBackward(const T* _m, sl_size count, const T* pattern, sl_size nPattern) noexcept
		{
			T* m = (T*)_m;
			if (!nPattern) {
				return m + count;
			}
			if (nPattern == 1) {
				return MemoryTraitsBase<T, sizeof(T)>::findBackward(m, count, *pattern);
			}
			if (nPattern > count) {
				return sl_null;
			}
			nPattern--;
			const T* pattern1 = pattern + 1;
			sl_size n = count - nPattern;
			do {
				T* pt = MemoryTraitsBase<T, sizeof(T)>::findBackward(m, n, *pattern);
				if (!pt) {
					return sl_null;
				}
				if (MemoryTraitsBase<T, sizeof(T)>::equals(pt + 1, pattern1, nPattern)) {
					return pt;
				} else {
					n = (sl_size)(pt - m);
				}
			} while (n);
			return sl_null;
		}

	};

	template <typename T, sl_size ElementSize>
	class MemoryTraitsBase
	{
	public:
		static void copy(T* dst, const T* src, sl_size count) noexcept
		{
			Base::copyMemory(dst, src, count * ElementSize);
		}

		static void move(T* dst, const T* src, sl_size count) noexcept
		{
			Base::moveMemory(dst, src, count * ElementSize);
		}

		static void reset(T* dst, sl_size count) noexcept
		{
			Base::zeroMemory(dst, count * ElementSize);
		}

		static void reset(T* _dst, sl_size count, const T& value) noexcept
		{
			sl_uint8* dst = (sl_uint8*)_dst;
			sl_uint8* src = reinterpret_cast<sl_uint8*>(&value);
			if (ElementSize < 16) {
				sl_size i = 0;
				while (i < count) {
					sl_size n = count - i;
					if (n > ElementSize) {
						n = ElementSize;
					}
					for (sl_size k = 0; k < n; k++) {
						dst[i] = src[k];
						i++;
					}
				}
			} else {
				for (sl_size i = 0; i < count; i += ElementSize) {
					sl_size n = count - i;
					if (n > ElementSize) {
						n = ElementSize;
					}
					Base::copyMemory(dst + i, src, n);
				}
			}
		}

		static sl_bool equals(const T* m1, const T* m2, sl_size count) noexcept
		{
			return Base::equalsMemory(m1, m2, count * ElementSize);
		}

		static sl_compare_result compare(const T* m1, const T* m2, sl_size count) noexcept
		{
			return Base::compareMemory(m1, m2, count * ElementSize);
		}

		static sl_bool equalsZero(const T* m, sl_size count) noexcept
		{
			return Base::equalsMemoryZero(m, count * ElementSize);
		}

		static sl_compare_result compareZero(const T* m, sl_size count) noexcept
		{
			return Base::compareMemoryZero(m, count * ElementSize);
		}

		static T* find(const T* m, sl_size count, const T& pattern) noexcept
		{
			T* end = m + count;
			while (m < end) {
				if (*m == pattern) {
					return m;
				}
				m++;
			}
			return sl_null;
		}

		static T* find(const T* m, sl_size size, const T* pattern, sl_size nPattern) noexcept
		{
			return MemoryTraitsFind<T>::find(m, size, pattern, nPattern);
		}

		static T* findBackward(const T* m, sl_size count, const T& pattern) noexcept
		{
			T* end = m + count;
			while (end > m) {
				end--;
				if (*end == pattern) {
					return end;
				}
			}
			return sl_null;
		}

		static T* findBackward(const T* m, sl_size size, const T* pattern, sl_size nPattern) noexcept
		{
			return MemoryTraitsFind<T>::findBackward(m, size, pattern, nPattern);
		}

	};

	template <typename T>
	class MemoryTraitsBase<T, 1>
	{
	public:
		static void copy(T* dst, const T* src, sl_size count) noexcept
		{
			Base::copyMemory(dst, src, count);
		}

		static void move(T* dst, const T* src, sl_size count) noexcept
		{
			Base::moveMemory(dst, src, count);
		}

		static void reset(T* dst, sl_size count) noexcept
		{
			Base::zeroMemory(dst, count);
		}

		static void reset(T* dst, sl_size count, const T& value) noexcept
		{
			Base::resetMemory(dst, count, *((sl_uint8*)((void*)&value)));
		}

		static sl_bool equals(const T* m1, const T* m2, sl_size count) noexcept
		{
			return Base::equalsMemory(m1, m2, count);
		}

		static sl_compare_result compare(const T* m1, const T* m2, sl_size count) noexcept
		{
			return Base::compareMemory(m1, m2, count);
		}

		static sl_bool equalsZero(const T* m, sl_size count) noexcept
		{
			return Base::equalsMemoryZero(m, count);
		}

		static sl_compare_result compareZero(const T* m, sl_size count) noexcept
		{
			return Base::compareMemoryZero(m, count);
		}

		static T* find(const T* m, sl_size count, const T& pattern) noexcept
		{
			return (T*)(Base::findMemory(m, count, pattern));
		}

		static T* findBackward(const T* m, sl_size count, const T& pattern) noexcept
		{
			return (T*)(Base::findMemoryBackward(m, count, pattern));
		}

		static T* find(const T* m, sl_size size, const T* pattern, sl_size nPattern) noexcept
		{
			return (T*)(Base::findMemory(m, size, pattern, nPattern));
		}

		static T* findBackward(const T* m, sl_size size, const T* pattern, sl_size nPattern) noexcept
		{
			return (T*)(Base::findMemoryBackward(m, size, pattern, nPattern));
		}

	};

	template <typename T>
	class MemoryTraitsBase<T, 2>
	{
	public:
		static void copy(T* dst, const T* src, sl_size count) noexcept
		{
			Base::copyMemory(dst, src, count << 1);
		}

		static void move(T* dst, const T* src, sl_size count) noexcept
		{
			Base::moveMemory(dst, src, count << 1);
		}

		static void reset(T* dst, sl_size count) noexcept
		{
			Base::zeroMemory(dst, count << 1);
		}

		static void reset(T* dst, sl_size count, const T& value) noexcept
		{
			Base::resetMemory2(dst, count, *((sl_uint16*)((void*)&value)));
		}

		static sl_bool equals(const T* m1, const T* m2, sl_size count) noexcept
		{
			return Base::equalsMemory(m1, m2, count << 1);
		}

		static sl_compare_result compare(const T* m1, const T* m2, sl_size count) noexcept
		{
			return Base::compareMemory2(m1, m2, count);
		}

		static sl_bool equalsZero(const T* m, sl_size count) noexcept
		{
			return Base::equalsMemoryZero(m, count << 1);
		}

		static sl_compare_result compareZero(const T* m, sl_size count) noexcept
		{
			return Base::compareMemoryZero(m, count << 1);
		}

		static T* find(const T* m, sl_size count, const T& pattern) noexcept
		{
			return (T*)(Base::findMemory2(m, count, pattern));
		}

		static T* findBackward(const T* m, sl_size count, const T& pattern) noexcept
		{
			return (T*)(Base::findMemoryBackward2(m, count, pattern));
		}

		static T* find(const T* m, sl_size size, const T* pattern, sl_size nPattern) noexcept
		{
			return (T*)(Base::findMemory2(m, size, pattern, nPattern));
		}

		static T* findBackward(const T* m, sl_size size, const T* pattern, sl_size nPattern) noexcept
		{
			return (T*)(Base::findMemoryBackward2(m, size, pattern, nPattern));
		}

	};

	template <typename T>
	class MemoryTraitsBase<T, 4>
	{
	public:
		static void copy(T* dst, const T* src, sl_size count) noexcept
		{
			Base::copyMemory(dst, src, count << 2);
		}

		static void move(T* dst, const T* src, sl_size count) noexcept
		{
			Base::moveMemory(dst, src, count << 2);
		}

		static void reset(T* dst, sl_size count) noexcept
		{
			Base::zeroMemory(dst, count << 2);
		}

		static void reset(T* dst, sl_size count, const T& value) noexcept
		{
			Base::resetMemory4(dst, count, *((sl_uint32*)((void*)&value)));
		}

		static sl_bool equals(const T* m1, const T* m2, sl_size count) noexcept
		{
			return Base::equalsMemory(m1, m2, count << 2);
		}

		static sl_compare_result compare(const T* m1, const T* m2, sl_size count) noexcept
		{
			return Base::compareMemory4(m1, m2, count);
		}

		static sl_bool equalsZero(const T* m, sl_size count) noexcept
		{
			return Base::equalsMemoryZero(m, count << 2);
		}

		static sl_compare_result compareZero(const T* m, sl_size count) noexcept
		{
			return Base::compareMemoryZero(m, count << 2);
		}

		static T* find(const T* m, sl_size count, const T& pattern) noexcept
		{
			return (T*)(Base::findMemory4(m, count, pattern));
		}

		static T* findBackward(const T* m, sl_size count, const T& pattern) noexcept
		{
			return (T*)(Base::findMemoryBackward4(m, count, pattern));
		}

		static T* find(const T* m, sl_size size, const T* pattern, sl_size nPattern) noexcept
		{
			return (T*)(Base::findMemory4(m, size, pattern, nPattern));
		}

		static T* findBackward(const T* m, sl_size size, const T* pattern, sl_size nPattern) noexcept
		{
			return (T*)(Base::findMemoryBackward4(m, size, pattern, nPattern));
		}

	};

	template <typename T>
	class MemoryTraitsBase<T, 8>
	{
	public:
		static void copy(T* dst, const T* src, sl_size count) noexcept
		{
			Base::copyMemory(dst, src, count << 3);
		}

		static void move(T* dst, const T* src, sl_size count) noexcept
		{
			Base::moveMemory(dst, src, count << 3);
		}

		static void reset(T* dst, sl_size count) noexcept
		{
			Base::zeroMemory(dst, count << 3);
		}

		static void reset(T* dst, sl_size count, const T& value) noexcept
		{
			Base::resetMemory4(dst, count, *((sl_uint64*)((void*)&value)));
		}

		static sl_bool equals(const T* m1, const T* m2, sl_size count) noexcept
		{
			return Base::equalsMemory(m1, m2, count << 3);
		}

		static sl_compare_result compare(const T* m1, const T* m2, sl_size count) noexcept
		{
			return Base::compareMemory8(m1, m2, count);
		}

		static sl_bool equalsZero(const T* m, sl_size count) noexcept
		{
			return Base::equalsMemoryZero(m, count << 3);
		}

		static sl_compare_result compareZero(const T* m, sl_size count) noexcept
		{
			return Base::compareMemoryZero(m, count << 3);
		}

		static T* find(const T* m, sl_size count, const T& pattern) noexcept
		{
			return (T*)(Base::findMemory8(m, count, pattern));
		}

		static T* findBackward(const T* m, sl_size count, const T& pattern) noexcept
		{
			return (T*)(Base::findMemoryBackward8(m, count, pattern));
		}

		static T* find(const T* m, sl_size size, const T* pattern, sl_size nPattern) noexcept
		{
			return (T*)(Base::findMemory8(m, size, pattern, nPattern));
		}

		static T* findBackward(const T* m, sl_size size, const T* pattern, sl_size nPattern) noexcept
		{
			return (T*)(Base::findMemoryBackward8(m, size, pattern, nPattern));
		}

	};

	template <typename T>
	class MemoryTraits : public MemoryTraitsBase<T, sizeof(T)> {};

}

#endif

