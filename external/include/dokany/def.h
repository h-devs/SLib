#ifndef DOKANY_DEF_H
#define DOKANY_DEF_H

#define WIN32_LEAN_AND_MEAN
#define NOCOMM
#include <windows.h>

#ifndef FILE_FLAG_SESSION_AWARE
#define FILE_FLAG_SESSION_AWARE 0x00800000
#endif

typedef struct _FILE_ID_128 {
	BYTE Identifier[16];
} FILE_ID_128, *PFILE_ID_128;

typedef struct _FILE_ID_EXTD_DIR_INFO {
	ULONG         NextEntryOffset;
	ULONG         FileIndex;
	LARGE_INTEGER CreationTime;
	LARGE_INTEGER LastAccessTime;
	LARGE_INTEGER LastWriteTime;
	LARGE_INTEGER ChangeTime;
	LARGE_INTEGER EndOfFile;
	LARGE_INTEGER AllocationSize;
	ULONG         FileAttributes;
	ULONG         FileNameLength;
	ULONG         EaSize;
	ULONG         ReparsePointTag;
	FILE_ID_128   FileId;
	WCHAR         FileName[1];
} FILE_ID_EXTD_DIR_INFO, *PFILE_ID_EXTD_DIR_INFO;

#endif