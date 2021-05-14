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

#ifndef CHECKHEADER_SLIB_CORE_BASE
#define CHECKHEADER_SLIB_CORE_BASE

#include "compare.h"

namespace slib
{

	class SLIB_EXPORT Base
	{
	public:
		// Memory Allocation Functions
		static void* createMemory(sl_size size) noexcept;

		static void* reallocMemory(void* ptr, sl_size sizeNew) noexcept;

		static void freeMemory(void* ptr) noexcept;

		static void* createZeroMemory(sl_size size) noexcept;

		// Memory Utilities
		static void copyMemory(void* dst, const void* src, sl_size size) noexcept;

		static void moveMemory(void* dst, const void* src, sl_size size) noexcept;

		static void zeroMemory(void* dst, sl_size size) noexcept;

		static void resetMemory(void* dst, sl_size size, sl_uint8 value) noexcept;
		static void resetMemory2(void* dst, sl_size count, sl_uint16 value) noexcept;
		static void resetMemory4(void* dst, sl_size count, sl_uint32 value) noexcept;
		static void resetMemory8(void* dst, sl_size count, sl_uint64 value) noexcept;

		static sl_bool equalsMemory(const void* m1, const void* m2, sl_size size) noexcept;

		static sl_compare_result compareMemory(const void* m1, const void* m2, sl_size size) noexcept;
		static sl_compare_result compareMemory2(const void* m1, const void* m2, sl_size count) noexcept;
		static sl_compare_result compareMemory4(const void* m1, const void* m2, sl_size count) noexcept;
		static sl_compare_result compareMemory8(const void* m1, const void* m2, sl_size count) noexcept;

		static sl_compare_result compareMemorySigned(const void* m1, const void* m2, sl_size size) noexcept;
		static sl_compare_result compareMemorySigned2(const void* m1, const void* m2, sl_size count) noexcept;
		static sl_compare_result compareMemorySigned4(const void* m1, const void* m2, sl_size count) noexcept;
		static sl_compare_result compareMemorySigned8(const void* m1, const void* m2, sl_size count) noexcept;

		static sl_bool equalsMemoryZero(const void* m, sl_size size) noexcept;

		static sl_compare_result compareMemoryZero(const void* m, sl_size count) noexcept;

		static sl_compare_result compareMemoryZeroSigned(const void* m, sl_size count) noexcept;
		static sl_compare_result compareMemoryZeroSigned2(const void* m, sl_size count) noexcept;
		static sl_compare_result compareMemoryZeroSigned4(const void* m, sl_size count) noexcept;
		static sl_compare_result compareMemoryZeroSigned8(const void* m, sl_size count) noexcept;

		static sl_uint8* findMemory(const void* m, sl_size size, sl_uint8 pattern) noexcept;
		static sl_uint16* findMemory2(const void* m, sl_size count, sl_uint16 pattern) noexcept;
		static sl_uint32* findMemory4(const void* m, sl_size count, sl_uint32 pattern) noexcept;
		static sl_uint64* findMemory8(const void* m, sl_size count, sl_uint64 pattern) noexcept;

		static sl_uint8* findMemory(const void* m, sl_size size, const void* pattern, sl_size nPattern) noexcept;
		static sl_uint16* findMemory2(const void* m, sl_size count, const void* pattern, sl_size nPattern) noexcept;
		static sl_uint32* findMemory4(const void* m, sl_size count, const void* pattern, sl_size nPattern) noexcept;
		static sl_uint64* findMemory8(const void* m, sl_size count, const void* pattern, sl_size nPattern) noexcept;

		static sl_uint8* findMemoryBackward(const void* m, sl_size size, sl_uint8 pattern) noexcept;
		static sl_uint16* findMemoryBackward2(const void* m, sl_size count, sl_uint16 pattern) noexcept;
		static sl_uint32* findMemoryBackward4(const void* m, sl_size count, sl_uint32 pattern) noexcept;
		static sl_uint64* findMemoryBackward8(const void* m, sl_size count, sl_uint64 pattern) noexcept;

		static sl_uint8* findMemoryBackward(const void* m, sl_size size, const void* pattern, sl_size nPattern) noexcept;
		static sl_uint16* findMemoryBackward2(const void* m, sl_size count, const void* pattern, sl_size nPattern) noexcept;
		static sl_uint32* findMemoryBackward4(const void* m, sl_size count, const void* pattern, sl_size nPattern) noexcept;
		static sl_uint64* findMemoryBackward8(const void* m, sl_size count, const void* pattern, sl_size nPattern) noexcept;

		static sl_size copyString(sl_char8* dst, const sl_char8* src) noexcept;
		static sl_size copyString(sl_char8* dst, const sl_char8* src, sl_size count) noexcept;
		static sl_size copyString2(sl_char16* dst, const sl_char16* src) noexcept;
		static sl_size copyString2(sl_char16* dst, const sl_char16* src, sl_size count) noexcept;
		static sl_size copyString4(sl_char32* dst, const sl_char32* src) noexcept;
		static sl_size copyString4(sl_char32* dst, const sl_char32* src, sl_size count) noexcept;

		static sl_size getStringLength(const sl_char8* src) noexcept;
		static sl_size getStringLength(const sl_char8* src, sl_size count) noexcept;
		static sl_size getStringLength2(const sl_char16* src) noexcept;
		static sl_size getStringLength2(const sl_char16* src, sl_size count) noexcept;
		static sl_size getStringLength4(const sl_char32* src) noexcept;
		static sl_size getStringLength4(const sl_char32* src, sl_size count) noexcept;

		static sl_bool equalsString(const sl_char8* s1, const sl_char8* s2) noexcept;
		static sl_bool equalsString(const sl_char8* s1, const sl_char8* s2, sl_size count) noexcept;
		static sl_bool equalsString2(const sl_char16* s1, const sl_char16* s2) noexcept;
		static sl_bool equalsString2(const sl_char16* s1, const sl_char16* s2, sl_size count) noexcept;
		static sl_bool equalsString4(const sl_char32* s1, const sl_char32* s2) noexcept;
		static sl_bool equalsString4(const sl_char32* s1, const sl_char32* s2, sl_size count) noexcept;

		static sl_bool equalsStringIgnoreCase(const sl_char8* s1, const sl_char8* s2) noexcept;
		static sl_bool equalsStringIgnoreCase(const sl_char8* s1, const sl_char8* s2, sl_size count) noexcept;
		static sl_bool equalsStringIgnoreCase2(const sl_char16* s1, const sl_char16* s2) noexcept;
		static sl_bool equalsStringIgnoreCase2(const sl_char16* s1, const sl_char16* s2, sl_size count) noexcept;
		static sl_bool equalsStringIgnoreCase4(const sl_char32* s1, const sl_char32* s2) noexcept;
		static sl_bool equalsStringIgnoreCase4(const sl_char32* s1, const sl_char32* s2, sl_size count) noexcept;

		static sl_compare_result compareString(const sl_char8* s1, const sl_char8* s2) noexcept;
		static sl_compare_result compareString(const sl_char8* s1, const sl_char8* s2, sl_size count) noexcept;
		static sl_compare_result compareString2(const sl_char16* s1, const sl_char16* s2) noexcept;
		static sl_compare_result compareString2(const sl_char16* s1, const sl_char16* s2, sl_size count) noexcept;
		static sl_compare_result compareString4(const sl_char32* s1, const sl_char32* s2) noexcept;
		static sl_compare_result compareString4(const sl_char32* s1, const sl_char32* s2, sl_size count) noexcept;

		static sl_compare_result compareStringIgnoreCase(const sl_char8* s1, const sl_char8* s2) noexcept;
		static sl_compare_result compareStringIgnoreCase(const sl_char8* s1, const sl_char8* s2, sl_size count) noexcept;
		static sl_compare_result compareStringIgnoreCase2(const sl_char16* s1, const sl_char16* s2) noexcept;
		static sl_compare_result compareStringIgnoreCase2(const sl_char16* s1, const sl_char16* s2, sl_size count) noexcept;
		static sl_compare_result compareStringIgnoreCase4(const sl_char32* s1, const sl_char32* s2) noexcept;
		static sl_compare_result compareStringIgnoreCase4(const sl_char32* s1, const sl_char32* s2, sl_size count) noexcept;

		// Interlocked Functions (Synchronization of Atom Operations)
		static sl_int32 interlockedIncrement32(sl_int32* pValue) noexcept;
		static sl_int32 interlockedDecrement32(sl_int32* pValue) noexcept;
		static sl_int32 interlockedAdd32(sl_int32* pDst, sl_int32 value) noexcept;
		static sl_bool interlockedCompareExchange32(sl_int32* pDst, sl_int32 value, sl_int32 comparand) noexcept;
		static sl_int64 interlockedIncrement64(sl_int64* pValue) noexcept;
		static sl_int64 interlockedDecrement64(sl_int64* pValue) noexcept;
		static sl_int64 interlockedAdd64(sl_int64* pDst, sl_int64 value) noexcept;
		static sl_bool interlockedCompareExchange64(sl_int64* pDst, sl_int64 value, sl_int64 comparand) noexcept;

		static sl_reg interlockedIncrement(sl_reg* pValue) noexcept;
		static sl_reg interlockedDecrement(sl_reg* pValue) noexcept;
		static sl_reg interlockedAdd(sl_reg* pDst, sl_reg value) noexcept;
		static sl_bool interlockedCompareExchange(sl_reg* pDst, sl_reg value, sl_reg comparand) noexcept;
		static void* interlockedAddPtr(void** pDst, sl_reg value) noexcept;
		static sl_bool interlockedCompareExchangePtr(void** pDst, const void* value, const void* comparand) noexcept;

	};

}

#endif

