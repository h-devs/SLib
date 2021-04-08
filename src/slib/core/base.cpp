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

#include "slib/core/base.h"

#include "slib/core/system.h"
#include "slib/core/string.h"
#include "slib/core/memory_traits.h"
#include "slib/core/assert.h"

#if !defined(SLIB_PLATFORM_IS_APPLE)
#	include <malloc.h>
#endif

#include <stdio.h>
#include <memory.h>
#include <string.h>
#include <wchar.h>
#include <string>

#ifdef SLIB_PLATFORM_IS_WINDOWS
#	include "slib/core/platform_windows.h"
#endif

#if defined(SLIB_ARCH_IS_32BIT)
#	define NOT_SUPPORT_ATOMIC_64BIT
#endif

namespace slib
{
	
#if SLIB_WCHAR_SIZE == 2
	typedef wchar_t sl_base_char16;
#else
	typedef char16_t sl_base_char16;
#endif
#if SLIB_WCHAR_SIZE == 4
	typedef wchar_t sl_base_char32;
#else
	typedef char32_t sl_base_char32;
#endif

	void* Base::createMemory(sl_size size) noexcept
	{
		return malloc(size);
	}

	void Base::freeMemory(void* ptr) noexcept
	{
		free(ptr);
	}

	void* Base::reallocMemory(void* ptr, sl_size sizeNew) noexcept
	{
		if (!sizeNew) {
			free(ptr);
			return malloc(1);
		} else {
			return realloc(ptr, sizeNew);
		}
	}

	void* Base::createZeroMemory(sl_size size) noexcept
	{
		void* ptr = malloc(size);
		if (ptr) {
			memset(ptr, 0, size);
		}
		return ptr;
	}

	void Base::copyMemory(void* dst, const void* src, sl_size size) noexcept
	{
		memcpy(dst, src, size);
	}

	void Base::moveMemory(void* dst, const void* src, sl_size size) noexcept
	{
		memmove(dst, src, size);
	}

	void Base::zeroMemory(void* dst, sl_size size) noexcept
	{
		memset(dst, 0, size);
	}

	void Base::resetMemory(void* dst, sl_size size, sl_uint8 value) noexcept
	{
		memset(dst, value, size);
	}

	void Base::resetMemory2(void* dst, sl_size count, sl_uint16 value) noexcept
	{
		std::char_traits<sl_base_char16>::assign((sl_base_char16*)dst, count, (sl_base_char16)value);
	}

	void Base::resetMemory4(void* dst, sl_size count, sl_uint32 value) noexcept
	{
		std::char_traits<sl_base_char32>::assign((sl_base_char32*)dst, count, (sl_base_char32)value);
	}

	void Base::resetMemory8(void* dst, sl_size count, sl_uint64 value) noexcept
	{
		std::char_traits<sl_uint64>::assign((sl_uint64*)dst, count, value);
	}

	sl_bool Base::equalsMemory(const void* m1, const void* m2, sl_size count) noexcept
	{
		return !(memcmp(m1, m2, count));
	}

	sl_compare_result Base::compareMemory(const void* m1, const void* m2, sl_size count) noexcept
	{
		return (sl_compare_result)(memcmp(m1, m2, count));
	}

	sl_compare_result Base::compareMemory2(const void* m1, const void* m2, sl_size count) noexcept
	{
		return (sl_compare_result)(std::char_traits<sl_base_char16>::compare((sl_base_char16*)m1, (sl_base_char16*)m2, count));
	}

	sl_compare_result Base::compareMemory4(const void* m1, const void* m2, sl_size count) noexcept
	{
		return (sl_compare_result)(std::char_traits<sl_base_char32>::compare((sl_base_char32*)m1, (sl_base_char32*)m2, count));
	}

	sl_compare_result Base::compareMemory8(const void* m1, const void* m2, sl_size count) noexcept
	{
		return (sl_compare_result)(std::char_traits<sl_uint64>::compare((sl_uint64*)m1, (sl_uint64*)m2, count));
	}

	sl_compare_result Base::compareMemorySigned(const void* m1, const void* m2, sl_size count) noexcept
	{
		return (sl_compare_result)(std::char_traits<sl_int8>::compare((sl_int8*)m1, (sl_int8*)m2, count));
	}

	sl_compare_result Base::compareMemorySigned2(const void* m1, const void* m2, sl_size count) noexcept
	{
		return (sl_compare_result)(std::char_traits<sl_int16>::compare((sl_int16*)m1, (sl_int16*)m2, count));
	}

	sl_compare_result Base::compareMemorySigned4(const void* m1, const void* m2, sl_size count) noexcept
	{
		return (sl_compare_result)(std::char_traits<sl_int32>::compare((sl_int32*)m1, (sl_int32*)m2, count));
	}

	sl_compare_result Base::compareMemorySigned8(const void* m1, const void* m2, sl_size count) noexcept
	{
		return (sl_compare_result)(std::char_traits<sl_int64>::compare((sl_int64*)m1, (sl_int64*)m2, count));
	}

	sl_bool Base::equalsMemoryZero(const void* m, sl_size size) noexcept
	{
		sl_size t = (sl_size)m;
		sl_uint8* b;
#ifdef SLIB_ARCH_IS_64BIT
		if (!(t & 7)) {
			sl_uint64* q = (sl_uint64*)m;
			sl_uint64* qe = q + (size >> 3);
			while (q < qe) {
				if (*q) {
					return sl_false;
				}
				q++;
			}
			b = (sl_uint8*)qe;
		} else
#endif
		if (!(t & 3)) {
			sl_uint32* d = (sl_uint32*)m;
			sl_uint32* de = d + (size >> 2);
			while (d < de) {
				if (*d) {
					return sl_false;
				}
				d++;
			}
			b = (sl_uint8*)de;
		} else if (!(t & 1)) {
			sl_uint16* w = (sl_uint16*)m;
			sl_uint16* we = w + (size >> 1);
			while (w < we) {
				if (*w) {
					return sl_false;
				}
				w++;
			}
			b = (sl_uint8*)we;
		} else {
			b = (sl_uint8*)m;
		}
		sl_uint8* be = ((sl_uint8*)m) + size;
		while (b < be) {
			if (*b) {
				return sl_false;
			}
			b++;
		}
		return sl_true;
	}

	sl_compare_result Base::compareMemoryZero(const void* m, sl_size count) noexcept
	{
		return equalsMemoryZero(m, count) ? 0 : 1;
	}

	sl_compare_result Base::compareMemoryZeroSigned(const void* _m, sl_size count) noexcept
	{
		sl_int8* m = (sl_int8*)_m;
		sl_int8* end = m + count;
		for (; m < end; m++) {
			sl_int8 v = *m;
			if (v) {
				if (v < 0) {
					return -1;
				} else {
					return 1;
				}
			}
		}
		return 0;
	}

	sl_compare_result Base::compareMemoryZeroSigned2(const void* _m, sl_size count) noexcept
	{
		sl_int32* m = (sl_int32*)_m;
		sl_int32* end = m + count;
		for (; m < end; m++) {
			sl_int16 v = *m;
			if (v) {
				if (v < 0) {
					return -1;
				} else {
					return 1;
				}
			}
		}
		return 0;
	}

	sl_compare_result Base::compareMemoryZeroSigned4(const void* _m, sl_size count) noexcept
	{
		sl_int32* m = (sl_int32*)_m;
		sl_int32* end = m + count;
		for (; m < end; m++) {
			sl_int32 v = *m;
			if (v) {
				if (v < 0) {
					return -1;
				} else {
					return 1;
				}
			}
		}
		return 0;
	}

	sl_compare_result Base::compareMemoryZeroSigned8(const void* _m, sl_size count) noexcept
	{
		sl_int64* m = (sl_int64*)_m;
		sl_int64* end = m + count;
		for (; m < end; m++) {
			sl_int64 v = *m;
			if (v) {
				if (v < 0) {
					return -1;
				} else {
					return 1;
				}
			}
		}
		return 0;
	}

	sl_uint8* Base::findMemory(const void* m, sl_size size, sl_uint8 pattern) noexcept
	{
		return (sl_uint8*)(memchr(m, pattern, size));
	}

	sl_uint16* Base::findMemory2(const void* m, sl_size count, sl_uint16 pattern) noexcept
	{
		return (sl_uint16*)(std::char_traits<sl_base_char16>::find((sl_base_char16*)m, count, (sl_base_char16)pattern));
	}

	sl_uint32* Base::findMemory4(const void* m, sl_size count, sl_uint32 pattern) noexcept
	{
		return (sl_uint32*)(std::char_traits<sl_base_char32>::find((sl_base_char32*)m, count, (sl_base_char32)pattern));
	}

	sl_uint64* Base::findMemory8(const void* m, sl_size count, sl_uint64 pattern) noexcept
	{
		return (sl_uint64*)(std::char_traits<sl_uint64>::find((sl_uint64*)m, count, pattern));
	}

	sl_uint8* Base::findMemory(const void* m, sl_size size, const void* pattern, sl_size nPattern) noexcept
	{
		return MemoryTraitsFind<sl_uint8>::find((sl_uint8*)m, size, (sl_uint8*)pattern, nPattern);
	}

	sl_uint16* Base::findMemory2(const void* m, sl_size size, const void* pattern, sl_size nPattern) noexcept
	{
		return MemoryTraitsFind<sl_uint16>::find((sl_uint16*)m, size, (sl_uint16*)pattern, nPattern);
	}

	sl_uint32* Base::findMemory4(const void* m, sl_size size, const void* pattern, sl_size nPattern) noexcept
	{
		return MemoryTraitsFind<sl_uint32>::find((sl_uint32*)m, size, (sl_uint32*)pattern, nPattern);
	}

	sl_uint64* Base::findMemory8(const void* m, sl_size size, const void* pattern, sl_size nPattern) noexcept
	{
		return MemoryTraitsFind<sl_uint64>::find((sl_uint64*)m, size, (sl_uint64*)pattern, nPattern);
	}

	sl_uint8* Base::findMemoryBackward(const void* _m, sl_size size, sl_uint8 pattern) noexcept
	{
		sl_uint8* m = (sl_uint8*)_m;
		sl_uint8* end = m + size;
		while (end > m) {
			end--;
			if (*end == pattern) {
				return end;
			}
		}
		return sl_null;
	}

	sl_uint16* Base::findMemoryBackward2(const void* _m, sl_size count, sl_uint16 pattern) noexcept
	{
		sl_uint16* m = (sl_uint16*)_m;
		sl_uint16* end = m + count;
		while (end > m) {
			end--;
			if (*end == pattern) {
				return end;
			}
		}
		return sl_null;
	}

	sl_uint32* Base::findMemoryBackward4(const void* _m, sl_size count, sl_uint32 pattern) noexcept
	{
		sl_uint32* m = (sl_uint32*)_m;
		sl_uint32* end = m + count;
		while (end > m) {
			end--;
			if (*end == pattern) {
				return end;
			}
		}
		return sl_null;
	}

	sl_uint64* Base::findMemoryBackward8(const void* _m, sl_size count, sl_uint64 pattern) noexcept
	{
		sl_uint64* m = (sl_uint64*)_m;
		sl_uint64* end = m + count;
		while (end > m) {
			end--;
			if (*end == pattern) {
				return end;
			}
		}
		return sl_null;
	}

	sl_uint8* Base::findMemoryBackward(const void* m, sl_size size, const void* pattern, sl_size nPattern) noexcept
	{
		return MemoryTraitsFind<sl_uint8>::findBackward((sl_uint8*)m, size, (sl_uint8*)pattern, nPattern);
	}

	sl_uint16* Base::findMemoryBackward2(const void* m, sl_size size, const void* pattern, sl_size nPattern) noexcept
	{
		return MemoryTraitsFind<sl_uint16>::findBackward((sl_uint16*)m, size, (sl_uint16*)pattern, nPattern);
	}

	sl_uint32* Base::findMemoryBackward4(const void* m, sl_size size, const void* pattern, sl_size nPattern) noexcept
	{
		return MemoryTraitsFind<sl_uint32>::findBackward((sl_uint32*)m, size, (sl_uint32*)pattern, nPattern);
	}

	sl_uint64* Base::findMemoryBackward8(const void* m, sl_size size, const void* pattern, sl_size nPattern) noexcept
	{
		return MemoryTraitsFind<sl_uint64>::findBackward((sl_uint64*)m, size, (sl_uint64*)pattern, nPattern);
	}

#define STRING_LENGTH_LIMIT 0x1000000

	sl_size Base::copyString(sl_char8* dst, const sl_char8* src, sl_reg count) noexcept
	{
		if (count < 0) {
			count = STRING_LENGTH_LIMIT;
		}
		const sl_char8* begin = src;
		const sl_char8* end = src + count;
		while (src < end) {
			sl_char8 c = *(src++);
			*(dst++) = c;
			if (!c) {
				break;
			}
		}
		return src - begin;
	}

	sl_size Base::copyString2(sl_char16* dst, const sl_char16* src, sl_reg count) noexcept
	{
		if (count < 0) {
			count = STRING_LENGTH_LIMIT;
		}
		const sl_char16* begin = src;
		const sl_char16* end = src + count;
		while (src < end) {
			sl_char16 c = *(src++);
			*(dst++) = c;
			if (!c) {
				break;
			}
		}
		return src - begin;
	}

	sl_size Base::copyString4(sl_char32* dst, const sl_char32* src, sl_reg count) noexcept
	{
		if (count < 0) {
			count = STRING_LENGTH_LIMIT;
		}
		const sl_char32* begin = src;
		const sl_char32* end = src + count;
		while (src < end) {
			sl_char32 c = *(src++);
			*(dst++) = c;
			if (!c) {
				break;
			}
		}
		return src - begin;
	}

	sl_size Base::getStringLength(const sl_char8* sz, sl_reg count) noexcept
	{
		if (!sz) {
			return 0;
		}
		if (!count) {
			return 0;
		}
		if (count < 0) {
			count = STRING_LENGTH_LIMIT;
		}
		return strnlen(sz, count);
	}

	sl_size Base::getStringLength2(const sl_char16* sz, sl_reg count) noexcept
	{
		if (!sz) {
			return 0;
		}
		if (!count) {
			return 0;
		}
		if (count < 0) {
			count = STRING_LENGTH_LIMIT;
		}
#if SLIB_WCHAR_SIZE == 2
		return wcsnlen((wchar_t*)sz, count);
#else
		for (sl_reg i = 0; i < count; i++) {
			if (!(sz[i])) {
				return i;
			}
		}
		return count;
#endif
	}

	sl_size Base::getStringLength4(const sl_char32* sz, sl_reg count) noexcept
	{
		if (!sz) {
			return 0;
		}
		if (!count) {
			return 0;
		}
		if (count < 0) {
			count = STRING_LENGTH_LIMIT;
		}
#if SLIB_WCHAR_SIZE == 4
		return wcsnlen((wchar_t*)sz, count);
#else
		for (sl_reg i = 0; i < count; i++) {
			if (!(sz[i])) {
				return i;
			}
		}
		return count;
#endif
	}

	sl_bool Base::equalsString(const sl_char8* s1, const sl_char8* s2, sl_reg count) noexcept
	{
		if (count < 0) {
			count = STRING_LENGTH_LIMIT;
		}
		return !(strncmp(s1, s2, count));
	}

	sl_bool Base::equalsString2(const sl_char16* s1, const sl_char16* s2, sl_reg count) noexcept
	{
		if (count < 0) {
			count = STRING_LENGTH_LIMIT;
		}
#if SLIB_WCHAR_SIZE == 2
		return !(wcsncmp((wchar_t*)s1, (wchar_t*)s2, count));
#else
		const sl_char16* s1e = s1 + count;
		while (s1 < s1e) {
			sl_char16 c1 = *(s1++);
			sl_char16 c2 = *(s2++);
			if (c1 != c2) {
				return sl_false;
			}
			if (!c1) {
				break;
			}
		}
		return sl_true;
#endif
	}

	sl_bool Base::equalsString4(const sl_char32* s1, const sl_char32* s2, sl_reg count) noexcept
	{
		if (count < 0) {
			count = STRING_LENGTH_LIMIT;
		}
#if SLIB_WCHAR_SIZE == 4
		return !(wcsncmp((wchar_t*)s1, (wchar_t*)s2, count));
#else
		const sl_char32* s1e = s1 + count;
		while (s1 < s1e) {
			sl_char32 c1 = *(s1++);
			sl_char32 c2 = *(s2++);
			if (c1 != c2) {
				return sl_false;
			}
			if (!c1) {
				break;
			}
		}
		return sl_true;
#endif
	}

	sl_bool Base::equalsStringIgnoreCase(const sl_char8* s1, const sl_char8* s2, sl_reg count) noexcept
	{
		if (count < 0) {
			count = STRING_LENGTH_LIMIT;
		}
#ifdef SLIB_COMPILER_IS_VC
		return !(_strnicmp(s1, s2, count));
#else
		return !(strncasecmp(s1, s2, count));
#endif
	}

	sl_bool Base::equalsStringIgnoreCase2(const sl_char16* s1, const sl_char16* s2, sl_reg count) noexcept
	{
		if (count < 0) {
			count = STRING_LENGTH_LIMIT;
		}
#if SLIB_WCHAR_SIZE == 2
#ifdef SLIB_COMPILER_IS_VC
		return !(_wcsnicmp((wchar_t*)s1, (wchar_t*)s2, count));
#else
		return !(wcsncasecmp((wchar_t*)s1, (wchar_t*)s2, count));
#endif
#else
		const sl_char16* s1e = s1 + count;
		while (s1 < s1e) {
			sl_char16 c1 = *(s1++);
			sl_char16 c2 = *(s2++);
			c1 = SLIB_CHAR_LOWER_TO_UPPER(c1);
			c2 = SLIB_CHAR_LOWER_TO_UPPER(c2);
			if (c1 != c2) {
				return sl_false;
			}
			if (!c1) {
				break;
			}
		}
		return sl_true;
#endif
	}

	sl_bool Base::equalsStringIgnoreCase4(const sl_char32* s1, const sl_char32* s2, sl_reg count) noexcept
	{
		if (count < 0) {
			count = STRING_LENGTH_LIMIT;
		}
#if SLIB_WCHAR_SIZE == 4
#ifdef SLIB_COMPILER_IS_VC
		return !(_wcsnicmp((wchar_t*)s1, (wchar_t*)s2, count));
#else
		return !(wcsncasecmp((wchar_t*)s1, (wchar_t*)s2, count));
#endif
#else
		const sl_char32* s1e = s1 + count;
		while (s1 < s1e) {
			sl_char32 c1 = *(s1++);
			sl_char32 c2 = *(s2++);
			c1 = SLIB_CHAR_LOWER_TO_UPPER(c1);
			c2 = SLIB_CHAR_LOWER_TO_UPPER(c2);
			if (c1 != c2) {
				return sl_false;
			}
			if (!c1) {
				break;
			}
		}
		return sl_true;
#endif
	}

	sl_compare_result Base::compareString(const sl_char8* s1, const sl_char8* s2, sl_reg count) noexcept
	{
		if (count < 0) {
			count = STRING_LENGTH_LIMIT;
		}
		return (sl_compare_result)(strncmp(s1, s2, count));
	}

	sl_compare_result Base::compareString2(const sl_char16* s1, const sl_char16* s2, sl_reg count) noexcept
	{
		if (count < 0) {
			count = STRING_LENGTH_LIMIT;
		}
#if SLIB_WCHAR_SIZE == 2
		return (sl_compare_result)(wcsncmp((wchar_t*)s1, (wchar_t*)s2, count));
#else
		const sl_char16* s1e = s1 + count;
		while (s1 < s1e) {
			sl_char16 c1 = *(s1++);
			sl_char16 c2 = *(s2++);
			if (c1 < c2) {
				return -1;
			} else if (c1 > c2) {
				return 1;
			}
			if (!c1) {
				break;
			}
		}
		return 0;
#endif
	}

	sl_compare_result Base::compareString4(const sl_char32* s1, const sl_char32* s2, sl_reg count) noexcept
	{
		if (count < 0) {
			count = STRING_LENGTH_LIMIT;
		}
#if SLIB_WCHAR_SIZE == 4
		return (sl_compare_result)(wcsncmp((wchar_t*)s1, (wchar_t*)s2, count));
#else
		const sl_char32* s1e = s1 + count;
		while (s1 < s1e) {
			sl_char32 c1 = *(s1++);
			sl_char32 c2 = *(s2++);
			if (c1 < c2) {
				return -1;
			} else if (c1 > c2) {
				return 1;
			}
			if (!c1) {
				break;
			}
		}
		return 0;
#endif
	}

	sl_compare_result Base::compareStringIgnoreCase(const sl_char8* s1, const sl_char8* s2, sl_reg count) noexcept
	{
		if (count < 0) {
			count = STRING_LENGTH_LIMIT;
		}
#ifdef SLIB_COMPILER_IS_VC
		return (sl_compare_result)(_strnicmp(s1, s2, count));
#else
		return (sl_compare_result)(strncasecmp(s1, s2, count));
#endif
	}

	sl_compare_result Base::compareStringIgnoreCase2(const sl_char16* s1, const sl_char16* s2, sl_reg count) noexcept
	{
		if (count < 0) {
			count = STRING_LENGTH_LIMIT;
		}
#if SLIB_WCHAR_SIZE == 2
#ifdef SLIB_COMPILER_IS_VC
		return (sl_compare_result)(_wcsnicmp((wchar_t*)s1, (wchar_t*)s2, count));
#else
		return (sl_compare_result)(wcsncasecmp((wchar_t*)s1, (wchar_t*)s2, count));
#endif
#else
		const sl_char16* s1e = s1 + count;
		while (s1 < s1e) {
			sl_char16 c1 = *(s1++);
			sl_char16 c2 = *(s2++);
			c1 = SLIB_CHAR_LOWER_TO_UPPER(c1);
			c2 = SLIB_CHAR_LOWER_TO_UPPER(c2);
			if (c1 < c2) {
				return -1;
			} else if (c1 > c2) {
				return 1;
			}
			if (!c1) {
				break;
			}
		}
		return 0;
#endif
	}

	sl_compare_result Base::compareStringIgnoreCase4(const sl_char32* s1, const sl_char32* s2, sl_reg count) noexcept
	{
		if (count < 0) {
			count = STRING_LENGTH_LIMIT;
		}
#if SLIB_WCHAR_SIZE == 4
#ifdef SLIB_COMPILER_IS_VC
		return (sl_compare_result)(_wcsnicmp((wchar_t*)s1, (wchar_t*)s2, count));
#else
		return (sl_compare_result)(wcsncasecmp((wchar_t*)s1, (wchar_t*)s2, count));
#endif
#else
		const sl_char32* s1e = s1 + count;
		while (s1 < s1e) {
			sl_char32 c1 = *(s1++);
			sl_char32 c2 = *(s2++);
			c1 = SLIB_CHAR_LOWER_TO_UPPER(c1);
			c2 = SLIB_CHAR_LOWER_TO_UPPER(c2);
			if (c1 < c2) {
				return -1;
			} else if (c1 > c2) {
				return 1;
			}
			if (!c1) {
				break;
			}
		}
		return 0;
#endif
	}

	sl_int32 Base::interlockedIncrement32(sl_int32* pValue) noexcept
	{
		SLIB_ASSERT(SLIB_IS_ALIGNED_4(pValue));
#ifdef SLIB_PLATFORM_IS_WINDOWS
		return (sl_int32)InterlockedIncrement((LONG*)pValue);
#else
		return __atomic_add_fetch(pValue, 1, __ATOMIC_RELAXED);
#endif
	}

	sl_int32 Base::interlockedDecrement32(sl_int32* pValue) noexcept
	{
		SLIB_ASSERT(SLIB_IS_ALIGNED_4(pValue));
#ifdef SLIB_PLATFORM_IS_WINDOWS
		return (sl_int32)InterlockedDecrement((LONG*)pValue);
#else
		return __atomic_add_fetch(pValue, -1, __ATOMIC_RELAXED);
#endif
	}

	sl_int32 Base::interlockedAdd32(sl_int32* pDst, sl_int32 value) noexcept
	{
		SLIB_ASSERT(SLIB_IS_ALIGNED_4(pDst));
#ifdef SLIB_PLATFORM_IS_WINDOWS
		return ((sl_int32)InterlockedExchangeAdd((LONG*)pDst, (LONG)value)) + value;
#else
		return __atomic_add_fetch(pDst, value, __ATOMIC_RELAXED);
#endif
	}

	sl_bool Base::interlockedCompareExchange32(sl_int32* pDst, sl_int32 value, sl_int32 comparand) noexcept
	{
		SLIB_ASSERT(SLIB_IS_ALIGNED_4(pDst));
#ifdef SLIB_PLATFORM_IS_WINDOWS
		sl_int32 old;
#	if (SLIB_COMPILER >= SLIB_COMPILER_VC7)
		old = ((sl_int32)InterlockedCompareExchange((LONG*)pDst, (LONG)value, (LONG)comparand));
#	else
		old = ((sl_int32)InterlockedCompareExchange((void**)pDst, (void*)value, (void*)comparand));
#	endif
		return old == comparand;
#else
		return __sync_bool_compare_and_swap(pDst, comparand, value) != 0;
#endif
	}

	sl_int64 Base::interlockedIncrement64(sl_int64* pValue) noexcept
	{
		SLIB_ASSERT(SLIB_IS_ALIGNED_8(pValue));
#ifdef NOT_SUPPORT_ATOMIC_64BIT
		SpinLocker lock(SpinLockPoolForBase::get(pValue));
		sl_int64 r = *pValue = *pValue + 1;
		return r;
#else
#	ifdef SLIB_PLATFORM_IS_WINDOWS
		return (sl_int64)InterlockedIncrement64((LONGLONG*)pValue);
#	else
		return __atomic_add_fetch(pValue, 1, __ATOMIC_RELAXED);
#	endif
#endif
	}

	sl_int64 Base::interlockedDecrement64(sl_int64* pValue) noexcept
	{
		SLIB_ASSERT(SLIB_IS_ALIGNED_8(pValue));
#ifdef NOT_SUPPORT_ATOMIC_64BIT
		SpinLocker lock(SpinLockPoolForBase::get(pValue));
		sl_int64 r = *pValue = *pValue - 1;
		return r;
#else
#	ifdef SLIB_PLATFORM_IS_WINDOWS
		return (sl_int64)InterlockedDecrement64((LONGLONG*)pValue);
#	else
		return __atomic_add_fetch(pValue, -1, __ATOMIC_RELAXED);
#	endif
#endif
	}

	sl_int64 Base::interlockedAdd64(sl_int64* pDst, sl_int64 value) noexcept
	{
		SLIB_ASSERT(SLIB_IS_ALIGNED_8(pDst));
#ifdef NOT_SUPPORT_ATOMIC_64BIT
		SpinLocker lock(SpinLockPoolForBase::get(pDst));
		sl_int64 r = *pDst = *pDst + value;
		return r;
#else
#	ifdef SLIB_PLATFORM_IS_WINDOWS
		return ((sl_int64)InterlockedExchangeAdd64((LONGLONG*)pDst, (LONGLONG)value)) + value;
#	else
		return __atomic_add_fetch(pDst, value, __ATOMIC_RELAXED);
#	endif
#endif
	}

	sl_bool Base::interlockedCompareExchange64(sl_int64* pDst, sl_int64 value, sl_int64 comparand) noexcept
	{
		SLIB_ASSERT(SLIB_IS_ALIGNED_8(pDst));
#ifdef NOT_SUPPORT_ATOMIC_64BIT
		SpinLocker lock(SpinLockPoolForBase::get(pDst));
		sl_int64 o = *pDst;
		if (o == comparand) {
			o = value;
			return sl_true;
		}
		return sl_false;
#else
#	ifdef SLIB_PLATFORM_IS_WINDOWS
		sl_int64 old = ((sl_int64)InterlockedCompareExchange64((LONGLONG*)pDst, (LONGLONG)value, (LONGLONG)comparand));
		return old == comparand;
#	else
		return __sync_bool_compare_and_swap(pDst, comparand, value) != 0;
#	endif
#endif
	}

	sl_reg Base::interlockedIncrement(sl_reg* pValue) noexcept
	{
#ifdef SLIB_ARCH_IS_64BIT
		return interlockedIncrement64(pValue);
#else
		return interlockedIncrement32(pValue);
#endif
	}

	sl_reg Base::interlockedDecrement(sl_reg* pValue) noexcept
	{
#ifdef SLIB_ARCH_IS_64BIT
		return interlockedDecrement64(pValue);
#else
		return interlockedDecrement32(pValue);
#endif
	}

	sl_reg Base::interlockedAdd(sl_reg* pDst, sl_reg value) noexcept
	{
#ifdef SLIB_ARCH_IS_64BIT
		return interlockedAdd64(pDst, value);
#else
		return interlockedAdd32(pDst, value);
#endif
	}

	sl_bool Base::interlockedCompareExchange(sl_reg* pDst, sl_reg value, sl_reg comparand) noexcept
	{
#ifdef SLIB_ARCH_IS_64BIT
		return interlockedCompareExchange64(pDst, value, comparand);
#else
		return interlockedCompareExchange32(pDst, value, comparand);
#endif
	}

	void* Base::interlockedAddPtr(void** pDst, sl_reg value) noexcept
	{
#ifdef SLIB_ARCH_IS_64BIT
		return (void*)interlockedAdd64((sl_int64*)(pDst), value);
#else
		return (void*)interlockedAdd32((sl_int32*)pDst, value);
#endif
	}

	sl_bool Base::interlockedCompareExchangePtr(void** pDst, const void* value, const void* comparand) noexcept
	{
#ifdef SLIB_ARCH_IS_64BIT
		return interlockedCompareExchange64((sl_int64*)(pDst), (sl_int64)value, (sl_int64)comparand);
#else
		return interlockedCompareExchange32((sl_int32*)pDst, (sl_int32)value, (sl_int32)comparand);
#endif
	}

}
