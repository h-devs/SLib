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

#ifndef CHECKHEADER_SLIB_CORE_DEFINITION
#define CHECKHEADER_SLIB_CORE_DEFINITION

#include "option.h"

#if defined(SLIB_COMPILER_IS_VC)

#	if defined (_DEBUG)
#		define SLIB_DEBUG
#	endif

#	define SLIB_STDCALL		__stdcall
#	define SLIB_INLINE		__forceinline
#	define SLIB_CONSTEXPR	constexpr
#	define SLIB_THREAD_OLD	__declspec(thread)
#	define SLIB_THREAD		thread_local
#	define SLIB_INT64(v)	v##i64
#	define SLIB_UINT64(v)	v##ui64
typedef __int64				sl_int64;
typedef unsigned __int64	sl_uint64;
typedef size_t				sl_size_t;

#	define SLIB_ALIGN(n)	__declspec(align(n))

#	define SLIB_EXPORT
#	define SLIB_VISIBLE_LOCAL

#	pragma warning(disable: 4521)
#	pragma warning(disable: 4522)

#elif defined(SLIB_COMPILER_IS_GCC)

#	if ! defined (__OPTIMIZE__)
#		define SLIB_DEBUG
#	endif

#	define SLIB_STDCALL		__attribute__((stdcall))
#	define SLIB_INLINE		inline __attribute__((always_inline))
#	if __GNUC__ >= 6
#	    define SLIB_CONSTEXPR	constexpr
#   else
#	    define SLIB_CONSTEXPR
#   endif
#	define SLIB_INT64(v)	v##LL
#	define SLIB_UINT64(v)	v##ULL
#	define SLIB_THREAD_OLD	__thread
#	define SLIB_THREAD		thread_local
typedef long long			sl_int64;
typedef unsigned long long	sl_uint64;
typedef __SIZE_TYPE__		sl_size_t;

#	define SLIB_ALIGN(n)	__attribute__((aligned(n)))

#	if __GNUC__ >= 4 && defined(SLIB_COMPILE_AS_SHARED_LIBRARY)
#		define SLIB_EXPORT			__attribute__((visibility("default")))
#		define SLIB_VISIBLE_LOCAL	__attribute__((visibility("hidden")))
#	else
#		define SLIB_EXPORT
#		define SLIB_VISIBLE_LOCAL
#	endif

#endif

// Basic Type Definition
typedef int					sl_int;
typedef unsigned int		sl_uint;
typedef signed char			sl_int8;
typedef unsigned char		sl_uint8;
typedef short				sl_int16;
typedef unsigned short		sl_uint16;
#if defined(__SIZEOF_INT__)
#	if __SIZEOF_INT__ == 4
typedef int					sl_int32;
typedef unsigned int		sl_uint32;
#	else
typedef long				sl_int32;
typedef unsigned long		sl_uint32;
#	endif
#else
typedef int					sl_int32;
typedef unsigned int		sl_uint32;
#endif
typedef float				sl_real;

typedef void*				sl_ptr;
typedef const void*			sl_cptr;

#define sl_null				nullptr
typedef decltype(nullptr)	sl_null_t;

typedef bool				sl_bool;
#define sl_true				true
#define sl_false			false

#if defined(SLIB_ARCH_IS_64BIT)
typedef sl_uint64			sl_size;
typedef sl_int64			sl_reg;
#else
typedef sl_uint32			sl_size;
typedef sl_int32			sl_reg;
#endif

typedef char				sl_char8;
typedef char16_t			sl_char16;
typedef char32_t			sl_char32;

#define SLIB_UNICODE(quote)	u##quote
#define SLIB_UNICODE32(quote)	U##quote

#if defined(SLIB_COMPILER_IS_VC)
#	define SLIB_WCHAR_SIZE				2
#else
#	if defined(__SIZEOF_WCHAR_T__)
#		define SLIB_WCHAR_SIZE			__SIZEOF_WCHAR_T__
#	else
#		define SLIB_WCHAR_SIZE			0
#	endif
#endif

#define SLIB_UINT8_MAX		(sl_uint8)(0xFF)
#define SLIB_INT8_MAX		(sl_int8)(0x7F)
#define SLIB_INT8_MIN		(sl_int8)(-0x80)
#define SLIB_UINT16_MAX		(sl_uint16)(0xFFFF)
#define SLIB_INT16_MAX		(sl_int16)(0x7FFF)
#define SLIB_INT16_MIN		(sl_int16)(-0x8000)
#define SLIB_UINT32_MAX		0xFFFFFFFF
#define SLIB_INT32_MAX		0x7FFFFFFF
#define SLIB_INT32_MIN		-0x80000000
#define SLIB_UINT64_MAX		SLIB_UINT64(0xFFFFFFFFFFFFFFFF)
#define SLIB_INT64_MAX		SLIB_INT64(0x7FFFFFFFFFFFFFFF)
#define SLIB_INT64_MIN		SLIB_INT64(-0x8000000000000000)

#ifdef SLIB_ARCH_IS_64BIT
#   define SLIB_SIZE_MAX	SLIB_UINT64_MAX
#   define SLIB_SIZE_TEST_SIGN_BIT SLIB_UINT64(0x8000000000000000)
#   define SLIB_SIZE_MASK_NO_SIGN_BITS SLIB_UINT64(0x7FFFFFFFFFFFFFFF)
#   define SLIB_REG_MAX		SLIB_INT64_MAX
#   define SLIB_REG_MIN		SLIB_INT64_MIN
#	define SLIB_SIZE_FROM_UINT64(x)		(sl_size)(x)
#	define SLIB_UINT32_FROM_SIZE(x)		((x) >> 32 ? 0xFFFFFFFF : (sl_uint32)(x))
#else
#   define SLIB_SIZE_MAX	SLIB_UINT32_MAX
#   define SLIB_SIZE_TEST_SIGN_BIT 0x80000000
#   define SLIB_SIZE_MASK_NO_SIGN_BITS 0x7FFFFFFF
#   define SLIB_REG_MAX		SLIB_INT32_MAX
#   define SLIB_REG_MIN		SLIB_INT32_MIN
#	define SLIB_SIZE_FROM_UINT64(x)		((x) >> 32 ? 0xFFFFFFFF : (sl_size)(x))
#	define SLIB_UINT32_FROM_SIZE(x)		(sl_uint32)(x)
#endif

#if defined(__SIZEOF_INT__)
#	define SLIB_INT_SIZE	__SIZEOF_INT__
#else
#	define SLIB_INT_SIZE	4
#endif
#if defined(__SIZEOF_LONG__)
#	define SLIB_LONG_SIZE	__SIZEOF_LONG__
#else
#	define SLIB_LONG_SIZE	4
#endif
#if defined(__SIZEOF_POINTER__)
#	define SLIB_POINTER_SIZE		__SIZEOF_POINTER__
#else
#	if defined(SLIB_ARCH_IS_64BIT)
#		define SLIB_POINTER_SIZE	8
#	else
#		define SLIB_POINTER_SIZE	4
#	endif
#endif

#define SLIB_UNUSED(x) (void)x;

#define SLIB_MAX(a, b)				((a)>(b)?(a):(b))
#define SLIB_MIN(a, b)				((a)<(b)?(a):(b))

#define SLIB_CHECK_FLAG(v, flag)	(((v) & (flag)) != 0)
#define SLIB_SET_FLAG(v, flag)		v |= (flag);
#define SLIB_RESET_FLAG(v, flag)	v &= (~(flag));

#define SLIB_IS_ALIGNED(p, a)		(!((unsigned long)(p) & ((a) - 1)))
#define SLIB_IS_ALIGNED_4(x)		(!(((sl_reg)((void*)(x))) & 3))
#define SLIB_IS_ALIGNED_8(x)		(!(((sl_reg)((void*)(x))) & 7))

#define SLIB_GET_BYTE(A,n)			((sl_uint8)((A) >> (n << 3)))
#define SLIB_GET_BYTE0(A)			((sl_uint8)(A))
#define SLIB_GET_BYTE1(A)			((sl_uint8)((A) >> 8))
#define SLIB_GET_BYTE2(A)			((sl_uint8)((A) >> 16))
#define SLIB_GET_BYTE3(A)			((sl_uint8)((A) >> 24))
#define SLIB_GET_BYTE4(A)			((sl_uint8)((A) >> 32))
#define SLIB_GET_BYTE5(A)			((sl_uint8)((A) >> 40))
#define SLIB_GET_BYTE6(A)			((sl_uint8)((A) >> 48))
#define SLIB_GET_BYTE7(A)			((sl_uint8)((A) >> 56))
#define SLIB_GET_WORD(A,n)			((sl_uint16)((A) >> (n << 4)))
#define SLIB_GET_WORD0(A)			((sl_uint16)(A))
#define SLIB_GET_WORD1(A)			((sl_uint16)((A) >> 16))
#define SLIB_GET_WORD2(A)			((sl_uint16)((A) >> 32))
#define SLIB_GET_WORD3(A)			((sl_uint16)((A) >> 48))
#define SLIB_GET_DWORD(A,n)			((sl_uint32)((A) >> (n << 5)))
#define SLIB_GET_DWORD0(A)			((sl_uint32)(A))
#define SLIB_GET_DWORD1(A)			((sl_uint32)((A) >> 32))

#define SLIB_MAKE_WORD(A,B)					((sl_uint16)((((sl_uint16)(sl_uint8)(A))<<8) | ((sl_uint16)(sl_uint8)(B))))
#define SLIB_MAKE_DWORD(A,B,C,D)			((((sl_uint32)(sl_uint8)(A))<<24) | (((sl_uint32)(sl_uint8)(B))<<16) | (((sl_uint32)(sl_uint8)(C))<<8) | ((sl_uint32)(sl_uint8)(D)))
#define SLIB_MAKE_DWORD2(A,B)				((((sl_uint32)(sl_uint16)(A))<<16) | ((sl_uint32)(sl_uint16)(B)))
#define SLIB_MAKE_QWORD(A,B,C,D,E,F,G,H)	((((sl_uint64)SLIB_MAKE_DWORD(A,B,C,D)) << 32) | SLIB_MAKE_DWORD(E,F,G,H))
#define SLIB_MAKE_QWORD2(A,B,C,D)			((((sl_uint64)SLIB_MAKE_DWORD2(A,B)) << 32) | SLIB_MAKE_DWORD2(C,D))
#define SLIB_MAKE_QWORD4(A,B)				((((sl_uint64)(sl_uint32)(A)) << 32) | (sl_uint32)(B))

#if defined(__GNUC__)
#   define SLIB_EXPECT(x, c) __builtin_expect(x, c)
#   define SLIB_LIKELY(x) __builtin_expect(!!(x), 1)
#   define SLIB_UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
#   define SLIB_EXPECT(x, c) (x)
#   define SLIB_LIKELY(v) (v)
#   define SLIB_UNLIKELY(v) (v)
#endif

#if defined(SLIB_COMPILE_LIB)
#   ifndef PRIV_SLIB_INCLUDED_OBJECT_TYPES
#       define PRIV_SLIB_INCLUDED_OBJECT_TYPES
#       include "object_types.h"
#   endif
#else
#   if !defined(SLIB_NOT_SUPPORT_STD_TYPES)
#	    ifndef SLIB_SUPPORT_STD_TYPES
#		    define SLIB_SUPPORT_STD_TYPES
#	    endif
#   endif
#endif

/************************************************************************/
/* Operating System Related Difinitions                                 */
/************************************************************************/
#if (SLIB_PLATFORM==SLIB_PLATFORM_WIN32)
#	ifndef _WIN32_WINNT
#		define _WIN32_WINNT 0x0501
#	endif
#endif

#endif
