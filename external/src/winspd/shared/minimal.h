/**
 * @file shared/minimal.h
 *
 * @copyright 2018-2020 Bill Zissimopoulos
 */
/*
 * This file is part of WinSpd.
 *
 * You can redistribute it and/or modify it under the terms of the GNU
 * General Public License version 3 as published by the Free Software
 * Foundation.
 *
 * Licensees holding a valid commercial license may use this software
 * in accordance with the commercial license agreement provided in
 * conjunction with the software.  The terms and conditions of any such
 * commercial license agreement shall govern, supersede, and render
 * ineffective any application of the GPLv3 license to this software,
 * notwithstanding of any reference thereto in the software or
 * associated repository.
 */

#ifndef WINSPD_SHARED_MINIMAL_H_INCLUDED
#define WINSPD_SHARED_MINIMAL_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Eliminate dependency on the MSVCRT libraries.
 *
 * For this to work the following project settings must be set:
 * - "C/C++ > General > SDL checks" must be empty (not "Yes" or "No").
 * - "C/C++ > Code Generation > Basic Runtime Checks" must be set to "Default"
 * - "C/C++ > Code Generation > Runtime Library" must be set to "Multi-threaded (/MT)".
 * - "C/C++ > Code Generation > Security Check" must be disabled (/GS-).
 * - "Linker > Input > Ignore All Default Libraries" must be "Yes".
 *
 *
 * Update 1:
 *
 * It is possible to have the "Linker > Input > Ignore All Default Libraries"
 * setting to "No" and still eliminate most dependencies on the MSVCRT libraries.
 * For example, the WinFsp DLL does this on 32-bit builds (only) to include the
 * __allmul symbol that is used when doing int64_t multiplications.
 *
 * The following project setting must be changed:
 * - "Linker > Input > Ignore All Default Libraries" must be "No".
 *
 * Extreme care must be taken to ensure that the linker does not pull in symbols
 * that are not required (or worse create a half-baked CRT). For example, the WinSpd
 * DLL ensures this by setting the "Linker > Input > Ignore All Default Libraries"
 * to "Yes" on 64-bit builds and "No" on 32-bit builds.
 *
 *
 * Update 2:
 *
 * Using the /Gs[size] compiler option with a large size is a very bad idea.
 * Turns out that the compiler uses the _chkstk call to ensure that enough
 * stack space has been committed even when a function accesses locations in
 * the stack below the guard page.
 *
 * The following links explain the problem very well:
 * - http://stackoverflow.com/questions/8400118/what-is-the-purpose-of-the-chkstk-function#8400171
 * - https://support.microsoft.com/en-us/kb/100775
 *
 * A library/program that does not wish to use the MSVCRT libraries (and hence
 * does not have _chkstk available) must take care to not use more than a page
 * (4096 bytes) of stack within a single function.
 */

#undef RtlFillMemory
#undef RtlMoveMemory
NTSYSAPI VOID NTAPI RtlFillMemory(VOID *Destination, DWORD Length, BYTE Fill);
NTSYSAPI VOID NTAPI RtlMoveMemory(VOID *Destination, CONST VOID *Source, DWORD Length);

#pragma function(memset)
#pragma function(memcpy)
#pragma warning(push)
#pragma warning(disable:4163)           /* not available as an intrinsic function */
#pragma function(memmove)
#pragma warning(pop)
static inline
void *memset(void *dst, int val, size_t siz)
{
    RtlFillMemory(dst, (DWORD)siz, val);
    return dst;
}
static inline
void *memcpy(void *dst, const void *src, size_t siz)
{
    RtlMoveMemory(dst, src, (DWORD)siz);
    return dst;
}
static inline
void *memmove(void *dst, const void *src, size_t siz)
{
    RtlMoveMemory(dst, src, (DWORD)siz);
    return dst;
}

#define WINSPD_SHARED_MINIMAL_STRCMP(NAME, TYPE, CONV)\
    static inline\
    int NAME(const TYPE *s, const TYPE *t)\
    {\
        int v = 0;\
        while (0 == (v = CONV(*s) - CONV(*t)) && *t)\
            ++s, ++t;\
        return v;/*(0 < v) - (0 > v);*/\
    }
#define WINSPD_SHARED_MINIMAL_STRNCMP(NAME, TYPE, CONV)\
    static inline\
    int NAME(const TYPE *s, const TYPE *t, size_t n)\
    {\
        int v = 0;\
        const void *e = t + n;\
        while (e > (const void *)t && 0 == (v = CONV(*s) - CONV(*t)) && *t)\
            ++s, ++t;\
        return v;/*(0 < v) - (0 > v);*/\
    }
static inline
unsigned invariant_toupper(unsigned c)
{
    return ('a' <= c && c <= 'z') ? c & ~0x20 : c;
}
WINSPD_SHARED_MINIMAL_STRCMP(invariant_strcmp, char, (unsigned))
WINSPD_SHARED_MINIMAL_STRCMP(invariant_stricmp, char, invariant_toupper)
WINSPD_SHARED_MINIMAL_STRNCMP(invariant_strncmp, char, (unsigned))
WINSPD_SHARED_MINIMAL_STRNCMP(invariant_strnicmp, char, invariant_toupper)
WINSPD_SHARED_MINIMAL_STRCMP(invariant_wcscmp, wchar_t, (unsigned))
WINSPD_SHARED_MINIMAL_STRCMP(invariant_wcsicmp, wchar_t, invariant_toupper)
WINSPD_SHARED_MINIMAL_STRNCMP(invariant_wcsncmp, wchar_t, (unsigned))
WINSPD_SHARED_MINIMAL_STRNCMP(invariant_wcsnicmp, wchar_t, invariant_toupper)
#undef WINSPD_SHARED_MINIMAL_STRCMP
#undef WINSPD_SHARED_MINIMAL_STRNCMP

static inline void *MemAlloc(size_t Size)
{
    return HeapAlloc(GetProcessHeap(), 0, Size);
}
static inline void *MemRealloc(void *Pointer, size_t Size)
{
    if (0 == Pointer)
        return HeapAlloc(GetProcessHeap(), 0, Size);
    else if (0 == Size)
        return HeapFree(GetProcessHeap(), 0, Pointer), 0;
    else
        return HeapReAlloc(GetProcessHeap(), 0, Pointer, Size);
}
static inline void MemFree(void *Pointer)
{
    if (0 != Pointer)
        HeapFree(GetProcessHeap(), 0, Pointer);
}

static FORCEINLINE
VOID InsertTailList(PLIST_ENTRY ListHead, PLIST_ENTRY Entry)
{
    PLIST_ENTRY Blink;

    Blink = ListHead->Blink;
    Entry->Flink = ListHead;
    Entry->Blink = Blink;
    Blink->Flink = Entry;
    ListHead->Blink = Entry;
}
static FORCEINLINE
BOOLEAN RemoveEntryList(PLIST_ENTRY Entry)
{
    PLIST_ENTRY Blink;
    PLIST_ENTRY Flink;

    Flink = Entry->Flink;
    Blink = Entry->Blink;
    Blink->Flink = Flink;
    Flink->Blink = Blink;
    return Flink == Blink;
}

/*
 * Overlapped
 */
static inline DWORD SpdOverlappedInit(OVERLAPPED *Overlapped)
{
    memset(Overlapped, 0, sizeof *Overlapped);
    Overlapped->hEvent = CreateEventW(0, TRUE, TRUE, 0);
    return 0 != Overlapped->hEvent ? ERROR_SUCCESS : GetLastError();
}
static inline VOID SpdOverlappedFini(OVERLAPPED *Overlapped)
{
    if (0 != Overlapped->hEvent)
        CloseHandle(Overlapped->hEvent);
}
static inline DWORD SpdOverlappedWaitResult(BOOL Success,
    HANDLE Handle, OVERLAPPED *Overlapped, PDWORD PBytesTransferred)
{
    if (!Success && ERROR_IO_PENDING != GetLastError())
        return GetLastError();
    if (!GetOverlappedResult(Handle, Overlapped, PBytesTransferred, TRUE))
        return GetLastError();
    return ERROR_SUCCESS;
}

#ifdef __cplusplus
}
#endif

#endif
