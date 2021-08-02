/*
  Dokan : user-mode file system library for Windows

  Copyright (C) 2020 Google, Inc.
  Copyright (C) 2015 - 2019 Adrien J. <liryna.stark@gmail.com> and Maxime C. <maxime@islog.com>
  Copyright (C) 2007 - 2011 Hiroki Asakawa <info@dokan-dev.net>

  http://dokan-dev.github.io

This program is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License as published by the Free
Software Foundation; either version 3 of the License, or (at your option) any
later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along
with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include "dokani.h"
#include "fileinfo.h"
#include "list.h"

/**
* \struct DOKAN_FIND_DATA
* \brief Dokan find file list
*
* Used by FindFiles
*/
typedef struct _DOKAN_FIND_DATA {
  /**
  * File data information link
  */
  WIN32_FIND_DATAW FindData;
  /**
  * Current list entry informations
  */
  LIST_ENTRY ListEntry;
} DOKAN_FIND_DATA, *PDOKAN_FIND_DATA;

VOID DokanFillDirInfo(PFILE_DIRECTORY_INFORMATION Buffer,
                      PWIN32_FIND_DATAW FindData, ULONG Index,
                      PDOKAN_INSTANCE DokanInstance) {
  ULONG nameBytes = (ULONG)wcslen(FindData->cFileName) * sizeof(WCHAR);

  Buffer->FileIndex = Index;
  Buffer->FileAttributes = FindData->dwFileAttributes;
  Buffer->FileNameLength = nameBytes;

  Buffer->EndOfFile.HighPart = FindData->nFileSizeHigh;
  Buffer->EndOfFile.LowPart = FindData->nFileSizeLow;
  Buffer->AllocationSize.HighPart = FindData->nFileSizeHigh;
  Buffer->AllocationSize.LowPart = FindData->nFileSizeLow;
  ALIGN_ALLOCATION_SIZE(&Buffer->AllocationSize, DokanInstance->DokanOptions);

  Buffer->CreationTime.HighPart = FindData->ftCreationTime.dwHighDateTime;
  Buffer->CreationTime.LowPart = FindData->ftCreationTime.dwLowDateTime;

  Buffer->LastAccessTime.HighPart = FindData->ftLastAccessTime.dwHighDateTime;
  Buffer->LastAccessTime.LowPart = FindData->ftLastAccessTime.dwLowDateTime;

  Buffer->LastWriteTime.HighPart = FindData->ftLastWriteTime.dwHighDateTime;
  Buffer->LastWriteTime.LowPart = FindData->ftLastWriteTime.dwLowDateTime;

  Buffer->ChangeTime.HighPart = FindData->ftLastWriteTime.dwHighDateTime;
  Buffer->ChangeTime.LowPart = FindData->ftLastWriteTime.dwLowDateTime;

  RtlCopyMemory(Buffer->FileName, FindData->cFileName, nameBytes);
}

VOID DokanFillFullDirInfo(PFILE_FULL_DIR_INFORMATION Buffer,
                          PWIN32_FIND_DATAW FindData, ULONG Index,
                          PDOKAN_INSTANCE DokanInstance) {
  ULONG nameBytes = (ULONG)wcslen(FindData->cFileName) * sizeof(WCHAR);

  Buffer->FileIndex = Index;
  Buffer->FileAttributes = FindData->dwFileAttributes;
  Buffer->FileNameLength = nameBytes;

  Buffer->EndOfFile.HighPart = FindData->nFileSizeHigh;
  Buffer->EndOfFile.LowPart = FindData->nFileSizeLow;
  Buffer->AllocationSize.HighPart = FindData->nFileSizeHigh;
  Buffer->AllocationSize.LowPart = FindData->nFileSizeLow;
  ALIGN_ALLOCATION_SIZE(&Buffer->AllocationSize, DokanInstance->DokanOptions);

  Buffer->CreationTime.HighPart = FindData->ftCreationTime.dwHighDateTime;
  Buffer->CreationTime.LowPart = FindData->ftCreationTime.dwLowDateTime;

  Buffer->LastAccessTime.HighPart = FindData->ftLastAccessTime.dwHighDateTime;
  Buffer->LastAccessTime.LowPart = FindData->ftLastAccessTime.dwLowDateTime;

  Buffer->LastWriteTime.HighPart = FindData->ftLastWriteTime.dwHighDateTime;
  Buffer->LastWriteTime.LowPart = FindData->ftLastWriteTime.dwLowDateTime;

  Buffer->ChangeTime.HighPart = FindData->ftLastWriteTime.dwHighDateTime;
  Buffer->ChangeTime.LowPart = FindData->ftLastWriteTime.dwLowDateTime;

  Buffer->EaSize = 0;

  RtlCopyMemory(Buffer->FileName, FindData->cFileName, nameBytes);
}

VOID DokanFillIdFullDirInfo(PFILE_ID_FULL_DIR_INFORMATION Buffer,
                            PWIN32_FIND_DATAW FindData, ULONG Index,
                            PDOKAN_INSTANCE DokanInstance) {
  ULONG nameBytes = (ULONG)wcslen(FindData->cFileName) * sizeof(WCHAR);

  Buffer->FileIndex = Index;
  Buffer->FileAttributes = FindData->dwFileAttributes;
  Buffer->FileNameLength = nameBytes;

  Buffer->EndOfFile.HighPart = FindData->nFileSizeHigh;
  Buffer->EndOfFile.LowPart = FindData->nFileSizeLow;
  Buffer->AllocationSize.HighPart = FindData->nFileSizeHigh;
  Buffer->AllocationSize.LowPart = FindData->nFileSizeLow;
  ALIGN_ALLOCATION_SIZE(&Buffer->AllocationSize, DokanInstance->DokanOptions);

  Buffer->CreationTime.HighPart = FindData->ftCreationTime.dwHighDateTime;
  Buffer->CreationTime.LowPart = FindData->ftCreationTime.dwLowDateTime;

  Buffer->LastAccessTime.HighPart = FindData->ftLastAccessTime.dwHighDateTime;
  Buffer->LastAccessTime.LowPart = FindData->ftLastAccessTime.dwLowDateTime;

  Buffer->LastWriteTime.HighPart = FindData->ftLastWriteTime.dwHighDateTime;
  Buffer->LastWriteTime.LowPart = FindData->ftLastWriteTime.dwLowDateTime;

  Buffer->ChangeTime.HighPart = FindData->ftLastWriteTime.dwHighDateTime;
  Buffer->ChangeTime.LowPart = FindData->ftLastWriteTime.dwLowDateTime;

  Buffer->EaSize = 0;
  Buffer->FileId.QuadPart = 0;

  RtlCopyMemory(Buffer->FileName, FindData->cFileName, nameBytes);
}

VOID DokanFillIdBothDirInfo(PFILE_ID_BOTH_DIR_INFORMATION Buffer,
                            PWIN32_FIND_DATAW FindData, ULONG Index,
                            PDOKAN_INSTANCE DokanInstance) {
  ULONG nameBytes = (ULONG)wcslen(FindData->cFileName) * sizeof(WCHAR);

  Buffer->FileIndex = Index;
  Buffer->FileAttributes = FindData->dwFileAttributes;
  Buffer->FileNameLength = nameBytes;
  Buffer->ShortNameLength = 0;

  Buffer->EndOfFile.HighPart = FindData->nFileSizeHigh;
  Buffer->EndOfFile.LowPart = FindData->nFileSizeLow;
  Buffer->AllocationSize.HighPart = FindData->nFileSizeHigh;
  Buffer->AllocationSize.LowPart = FindData->nFileSizeLow;
  ALIGN_ALLOCATION_SIZE(&Buffer->AllocationSize, DokanInstance->DokanOptions);

  Buffer->CreationTime.HighPart = FindData->ftCreationTime.dwHighDateTime;
  Buffer->CreationTime.LowPart = FindData->ftCreationTime.dwLowDateTime;

  Buffer->LastAccessTime.HighPart = FindData->ftLastAccessTime.dwHighDateTime;
  Buffer->LastAccessTime.LowPart = FindData->ftLastAccessTime.dwLowDateTime;

  Buffer->LastWriteTime.HighPart = FindData->ftLastWriteTime.dwHighDateTime;
  Buffer->LastWriteTime.LowPart = FindData->ftLastWriteTime.dwLowDateTime;

  Buffer->ChangeTime.HighPart = FindData->ftLastWriteTime.dwHighDateTime;
  Buffer->ChangeTime.LowPart = FindData->ftLastWriteTime.dwLowDateTime;

  Buffer->EaSize = 0;
  Buffer->FileId.QuadPart = 0;

  RtlCopyMemory(Buffer->FileName, FindData->cFileName, nameBytes);
}

VOID DokanFillIdExtdDirInfo(PFILE_ID_EXTD_DIR_INFO Buffer,
                            PWIN32_FIND_DATAW FindData, ULONG Index,
                            PDOKAN_INSTANCE DokanInstance) {
  ULONG nameBytes = (ULONG)wcslen(FindData->cFileName) * sizeof(WCHAR);

  Buffer->FileIndex = Index;
  Buffer->FileAttributes = FindData->dwFileAttributes;
  Buffer->FileNameLength = nameBytes;

  Buffer->EndOfFile.HighPart = FindData->nFileSizeHigh;
  Buffer->EndOfFile.LowPart = FindData->nFileSizeLow;
  Buffer->AllocationSize.HighPart = FindData->nFileSizeHigh;
  Buffer->AllocationSize.LowPart = FindData->nFileSizeLow;
  ALIGN_ALLOCATION_SIZE(&Buffer->AllocationSize, DokanInstance->DokanOptions);

  Buffer->CreationTime.HighPart = FindData->ftCreationTime.dwHighDateTime;
  Buffer->CreationTime.LowPart = FindData->ftCreationTime.dwLowDateTime;

  Buffer->LastAccessTime.HighPart = FindData->ftLastAccessTime.dwHighDateTime;
  Buffer->LastAccessTime.LowPart = FindData->ftLastAccessTime.dwLowDateTime;

  Buffer->LastWriteTime.HighPart = FindData->ftLastWriteTime.dwHighDateTime;
  Buffer->LastWriteTime.LowPart = FindData->ftLastWriteTime.dwLowDateTime;

  Buffer->ChangeTime.HighPart = FindData->ftLastWriteTime.dwHighDateTime;
  Buffer->ChangeTime.LowPart = FindData->ftLastWriteTime.dwLowDateTime;

  Buffer->EaSize = 0;
  Buffer->ReparsePointTag = 0;
  RtlFillMemory(&Buffer->FileId.Identifier, sizeof Buffer->FileId.Identifier, 0);

  RtlCopyMemory(Buffer->FileName, FindData->cFileName, nameBytes);
}

VOID DokanFillIdExtdBothDirInfo(PFILE_ID_EXTD_BOTH_DIR_INFORMATION Buffer,
                            PWIN32_FIND_DATAW FindData, ULONG Index,
                            PDOKAN_INSTANCE DokanInstance) {
  ULONG nameBytes = (ULONG)wcslen(FindData->cFileName) * sizeof(WCHAR);

  Buffer->FileIndex = Index;
  Buffer->FileAttributes = FindData->dwFileAttributes;
  Buffer->FileNameLength = nameBytes;
  Buffer->ShortNameLength = 0;

  Buffer->EndOfFile.HighPart = FindData->nFileSizeHigh;
  Buffer->EndOfFile.LowPart = FindData->nFileSizeLow;
  Buffer->AllocationSize.HighPart = FindData->nFileSizeHigh;
  Buffer->AllocationSize.LowPart = FindData->nFileSizeLow;
  ALIGN_ALLOCATION_SIZE(&Buffer->AllocationSize, DokanInstance->DokanOptions);

  Buffer->CreationTime.HighPart = FindData->ftCreationTime.dwHighDateTime;
  Buffer->CreationTime.LowPart = FindData->ftCreationTime.dwLowDateTime;

  Buffer->LastAccessTime.HighPart = FindData->ftLastAccessTime.dwHighDateTime;
  Buffer->LastAccessTime.LowPart = FindData->ftLastAccessTime.dwLowDateTime;

  Buffer->LastWriteTime.HighPart = FindData->ftLastWriteTime.dwHighDateTime;
  Buffer->LastWriteTime.LowPart = FindData->ftLastWriteTime.dwLowDateTime;

  Buffer->ChangeTime.HighPart = FindData->ftLastWriteTime.dwHighDateTime;
  Buffer->ChangeTime.LowPart = FindData->ftLastWriteTime.dwLowDateTime;

  Buffer->EaSize = 0;
  Buffer->ReparsePointTag = 0;
  RtlFillMemory(&Buffer->FileId.Identifier, sizeof Buffer->FileId.Identifier, 0);

  RtlCopyMemory(Buffer->FileName, FindData->cFileName, nameBytes);
}

VOID DokanFillBothDirInfo(PFILE_BOTH_DIR_INFORMATION Buffer,
                          PWIN32_FIND_DATAW FindData, ULONG Index,
                          PDOKAN_INSTANCE DokanInstance) {
  ULONG nameBytes = (ULONG)wcslen(FindData->cFileName) * sizeof(WCHAR);

  Buffer->FileIndex = Index;
  Buffer->FileAttributes = FindData->dwFileAttributes;
  Buffer->FileNameLength = nameBytes;
  Buffer->ShortNameLength = 0;

  Buffer->EndOfFile.HighPart = FindData->nFileSizeHigh;
  Buffer->EndOfFile.LowPart = FindData->nFileSizeLow;
  Buffer->AllocationSize.HighPart = FindData->nFileSizeHigh;
  Buffer->AllocationSize.LowPart = FindData->nFileSizeLow;
  ALIGN_ALLOCATION_SIZE(&Buffer->AllocationSize, DokanInstance->DokanOptions);

  Buffer->CreationTime.HighPart = FindData->ftCreationTime.dwHighDateTime;
  Buffer->CreationTime.LowPart = FindData->ftCreationTime.dwLowDateTime;

  Buffer->LastAccessTime.HighPart = FindData->ftLastAccessTime.dwHighDateTime;
  Buffer->LastAccessTime.LowPart = FindData->ftLastAccessTime.dwLowDateTime;

  Buffer->LastWriteTime.HighPart = FindData->ftLastWriteTime.dwHighDateTime;
  Buffer->LastWriteTime.LowPart = FindData->ftLastWriteTime.dwLowDateTime;

  Buffer->ChangeTime.HighPart = FindData->ftLastWriteTime.dwHighDateTime;
  Buffer->ChangeTime.LowPart = FindData->ftLastWriteTime.dwLowDateTime;

  Buffer->EaSize = 0;

  RtlCopyMemory(Buffer->FileName, FindData->cFileName, nameBytes);
}

VOID DokanFillNamesInfo(PFILE_NAMES_INFORMATION Buffer,
                        PWIN32_FIND_DATAW FindData, ULONG Index) {
  ULONG nameBytes = (ULONG)wcslen(FindData->cFileName) * sizeof(WCHAR);

  Buffer->FileIndex = Index;
  Buffer->FileNameLength = nameBytes;

  RtlCopyMemory(Buffer->FileName, FindData->cFileName, nameBytes);
}

ULONG
DokanFillDirectoryInformation(FILE_INFORMATION_CLASS DirectoryInfo,
                              PVOID Buffer, PULONG LengthRemaining,
                              PWIN32_FIND_DATAW FindData, ULONG Index,
                              PDOKAN_INSTANCE DokanInstance) {
  ULONG nameBytes;
  ULONG thisEntrySize;

  nameBytes = (ULONG)wcslen(FindData->cFileName) * sizeof(WCHAR);

  thisEntrySize = nameBytes;

  switch (DirectoryInfo) {
  case FileDirectoryInformation:
    thisEntrySize += sizeof(FILE_DIRECTORY_INFORMATION);
    break;
  case FileFullDirectoryInformation:
    thisEntrySize += sizeof(FILE_FULL_DIR_INFORMATION);
    break;
  case FileIdFullDirectoryInformation:
    thisEntrySize += sizeof(FILE_ID_FULL_DIR_INFORMATION);
    break;
  case FileNamesInformation:
    thisEntrySize += sizeof(FILE_NAMES_INFORMATION);
    break;
  case FileBothDirectoryInformation:
    thisEntrySize += sizeof(FILE_BOTH_DIR_INFORMATION);
    break;
  case FileIdBothDirectoryInformation:
    thisEntrySize += sizeof(FILE_ID_BOTH_DIR_INFORMATION);
    break;
  case FileIdExtdDirectoryInformation:
    thisEntrySize += sizeof(FILE_ID_EXTD_DIR_INFO);
    break;
  case FileIdExtdBothDirectoryInformation:
    thisEntrySize += sizeof(FILE_ID_EXTD_BOTH_DIR_INFORMATION);
    break;
  default:
    break;
  }

  // Must be align on a 8-byte boundary.
  thisEntrySize = QuadAlign(thisEntrySize);

  // no more memory, don't fill any more
  if (*LengthRemaining < thisEntrySize) {
    DbgPrint("  no memory\n");
    return 0;
  }

  RtlZeroMemory(Buffer, thisEntrySize);

  switch (DirectoryInfo) {
  case FileDirectoryInformation:
    DokanFillDirInfo(Buffer, FindData, Index, DokanInstance);
    break;
  case FileFullDirectoryInformation:
    DokanFillFullDirInfo(Buffer, FindData, Index, DokanInstance);
    break;
  case FileIdFullDirectoryInformation:
    DokanFillIdFullDirInfo(Buffer, FindData, Index, DokanInstance);
    break;
  case FileNamesInformation:
    DokanFillNamesInfo(Buffer, FindData, Index);
    break;
  case FileBothDirectoryInformation:
    DokanFillBothDirInfo(Buffer, FindData, Index, DokanInstance);
    break;
  case FileIdBothDirectoryInformation:
    DokanFillIdBothDirInfo(Buffer, FindData, Index, DokanInstance);
    break;
  case FileIdExtdDirectoryInformation:
    DokanFillIdExtdDirInfo(Buffer, FindData, Index, DokanInstance);
    break;
  case FileIdExtdBothDirectoryInformation:
    DokanFillIdExtdBothDirInfo(Buffer, FindData, Index, DokanInstance);
    break;    
  default:
    break;
  }

  *LengthRemaining -= thisEntrySize;

  return thisEntrySize;
}

int DokanFillFileDataEx(PWIN32_FIND_DATAW FindData, PDOKAN_FILE_INFO FileInfo,
                        BOOLEAN InsertTail) {
  PLIST_ENTRY listHead =
      ((PDOKAN_OPEN_INFO)(UINT_PTR)FileInfo->DokanContext)->DirListHead;
  PDOKAN_FIND_DATA findData;

  findData = (PDOKAN_FIND_DATA)malloc(sizeof(DOKAN_FIND_DATA));
  if (findData == NULL) {
    return 0;
  }
  ZeroMemory(findData, sizeof(DOKAN_FIND_DATA));
  InitializeListHead(&findData->ListEntry);

  findData->FindData = *FindData;

  if (InsertTail)
    InsertTailList(listHead, &findData->ListEntry);
  else
    InsertHeadList(listHead, &findData->ListEntry);
  return 0;
}

int WINAPI DokanFillFileData(PWIN32_FIND_DATAW FindData,
                             PDOKAN_FILE_INFO FileInfo) {
  return DokanFillFileDataEx(FindData, FileInfo, TRUE);
}

VOID ClearFindData(PLIST_ENTRY ListHead) {
  // free all list entries
  while (!IsListEmpty(ListHead)) {
    PLIST_ENTRY entry = RemoveHeadList(ListHead);
    PDOKAN_FIND_DATA find =
        CONTAINING_RECORD(entry, DOKAN_FIND_DATA, ListEntry);
    free(find);
  }
}

// add entry which matches the pattern specified in EventContext
// to the buffer specified in EventInfo
//
LONG MatchFiles(PEVENT_CONTEXT EventContext, PEVENT_INFORMATION EventInfo,
                PLIST_ENTRY FindDataList, BOOLEAN PatternCheck,
                PDOKAN_INSTANCE DokanInstance) {
  PLIST_ENTRY thisEntry, listHead, nextEntry;

  ULONG lengthRemaining = EventInfo->BufferLength;
  PVOID currentBuffer = EventInfo->Buffer;
  PVOID lastBuffer = currentBuffer;
  ULONG index = 0;
  BOOL caseSensitive = FALSE;
  PWCHAR pattern = NULL;

  // search patten is specified
  if (PatternCheck &&
      EventContext->Operation.Directory.SearchPatternLength != 0) {
    pattern = (PWCHAR)(
        (SIZE_T)&EventContext->Operation.Directory.SearchPatternBase[0] +
        (SIZE_T)EventContext->Operation.Directory.SearchPatternOffset);
  }

  caseSensitive =
      DokanInstance->DokanOptions->Options & DOKAN_OPTION_CASE_SENSITIVE;

  listHead = FindDataList;

  for (thisEntry = listHead->Flink; thisEntry != listHead;
       thisEntry = nextEntry) {

    PDOKAN_FIND_DATA find;
    nextEntry = thisEntry->Flink;

    find = CONTAINING_RECORD(thisEntry, DOKAN_FIND_DATA, ListEntry);

    DbgPrintW(L"FileMatch? : %s (%s,%d,%d)\n", find->FindData.cFileName,
              (pattern ? pattern : L"null"),
              EventContext->Operation.Directory.FileIndex, index);

    // pattern is not specified or pattern match is ignore cases
    if (!pattern || DokanIsNameInExpression(pattern, find->FindData.cFileName,
                                            !caseSensitive)) {

      if (EventContext->Operation.Directory.FileIndex <= index) {
        // index+1 is very important, should use next entry index
        ULONG entrySize = DokanFillDirectoryInformation(
            EventContext->Operation.Directory.FileInformationClass,
            currentBuffer, &lengthRemaining, &find->FindData, index + 1,
            DokanInstance);
        // buffer is full
        if (entrySize == 0)
          break;

        // pointer of the current last entry
        lastBuffer = currentBuffer;

        // end if needs to return single entry
        if (EventContext->Flags & SL_RETURN_SINGLE_ENTRY) {
          DbgPrint("  =>return single entry\n");
          index++;
          break;
        }

        DbgPrint("  =>return\n");

        // the offset of next entry
        ((PFILE_BOTH_DIR_INFORMATION)currentBuffer)->NextEntryOffset =
            entrySize;

        // next buffer position
        currentBuffer = (PCHAR)currentBuffer + entrySize;
      }
      index++;
    }
  }

  // Since next of the last entry doesn't exist, clear next offset
  ((PFILE_BOTH_DIR_INFORMATION)lastBuffer)->NextEntryOffset = 0;

  // acctualy used length of buffer
  EventInfo->BufferLength =
      EventContext->Operation.Directory.BufferLength - lengthRemaining;

  if (index <= EventContext->Operation.Directory.FileIndex) {

    if (thisEntry != listHead)
      return -2; // BUFFER_OVERFLOW

    return -1; // NO_MORE_FILES
  }

  return index;
}

VOID AddMissingCurrentAndParentFolder(PEVENT_CONTEXT EventContext,
                                      PLIST_ENTRY FindDataList,
                                      PDOKAN_FILE_INFO fileInfo) {
  PLIST_ENTRY thisEntry, listHead, nextEntry;
  PWCHAR pattern = NULL;
  BOOLEAN currentFolder = FALSE, parentFolder = FALSE;
  WIN32_FIND_DATAW findData;
  FILETIME systime;

  if (EventContext->Operation.Directory.SearchPatternLength != 0) {
    pattern = (PWCHAR)(
        (SIZE_T)&EventContext->Operation.Directory.SearchPatternBase[0] +
        (SIZE_T)EventContext->Operation.Directory.SearchPatternOffset);
  }

  if (wcscmp(EventContext->Operation.Directory.DirectoryName, L"\\") == 0 ||
      (pattern != NULL && wcscmp(pattern, L"*") != 0))
    return;

  listHead = FindDataList;
  for (thisEntry = listHead->Flink; thisEntry != listHead;
       thisEntry = nextEntry) {

    PDOKAN_FIND_DATA find;
    nextEntry = thisEntry->Flink;

    find = CONTAINING_RECORD(thisEntry, DOKAN_FIND_DATA, ListEntry);

    if (wcscmp(find->FindData.cFileName, L".") == 0)
      currentFolder = TRUE;
    if (wcscmp(find->FindData.cFileName, L"..") == 0)
      parentFolder = TRUE;
    if (currentFolder == TRUE && parentFolder == TRUE)
      return; // folders are already there
  }

  GetSystemTimeAsFileTime(&systime);
  ZeroMemory(&findData, sizeof(WIN32_FIND_DATAW));
  findData.dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
  findData.ftCreationTime = systime;
  findData.ftLastAccessTime = systime;
  findData.ftLastWriteTime = systime;
  // Folders times should be the real current and parent folder times...
  if (!parentFolder) {
    findData.cFileName[0] = '.';
    findData.cFileName[1] = '.';
    DokanFillFileDataEx(&findData, fileInfo, FALSE);
  }

  if (!currentFolder) {
    findData.cFileName[0] = '.';
    findData.cFileName[1] = '\0';
    DokanFillFileDataEx(&findData, fileInfo, FALSE);
  }
}

VOID DispatchDirectoryInformation(HANDLE Handle, PEVENT_CONTEXT EventContext,
                                  PDOKAN_INSTANCE DokanInstance) {
  PEVENT_INFORMATION eventInfo;
  DOKAN_FILE_INFO fileInfo;
  PDOKAN_OPEN_INFO openInfo;
  NTSTATUS status = STATUS_SUCCESS;
  ULONG fileInfoClass = EventContext->Operation.Directory.FileInformationClass;
  BOOLEAN patternCheck = TRUE;
  ULONG sizeOfEventInfo = DispatchGetEventInformationLength(
      EventContext->Operation.Directory.BufferLength);

  CheckFileName(EventContext->Operation.Directory.DirectoryName);

  eventInfo = DispatchCommon(EventContext, sizeOfEventInfo, DokanInstance,
                             &fileInfo, &openInfo);

  // check whether this is handled FileInfoClass
  if (fileInfoClass != FileDirectoryInformation &&
      fileInfoClass != FileFullDirectoryInformation &&
      fileInfoClass != FileBothDirectoryInformation &&
      fileInfoClass != FileNamesInformation &&
      fileInfoClass != FileIdBothDirectoryInformation &&
      fileInfoClass != FileIdFullDirectoryInformation &&
      fileInfoClass != FileIdExtdDirectoryInformation &&
      fileInfoClass != FileIdExtdBothDirectoryInformation) {

    DbgPrint("not suported type %d\n", fileInfoClass);

    // send directory info to driver
    eventInfo->BufferLength = 0;
    eventInfo->Status = STATUS_INVALID_PARAMETER;
    SendEventInformation(Handle, eventInfo, sizeOfEventInfo);
    ReleaseDokanOpenInfo(eventInfo, &fileInfo, DokanInstance);
    free(eventInfo);
    return;
  }

  // IMPORTANT!!
  // this buffer length is fixed in MatchFiles function
  eventInfo->BufferLength = EventContext->Operation.Directory.BufferLength;

  if (openInfo->DirListHead == NULL) {
    openInfo->DirListHead = malloc(sizeof(LIST_ENTRY));
    if (openInfo->DirListHead != NULL) {
      InitializeListHead(openInfo->DirListHead);
    } else {
      eventInfo->BufferLength = 0;
      eventInfo->Status = STATUS_NO_MEMORY;
      SendEventInformation(Handle, eventInfo, sizeOfEventInfo);
      ReleaseDokanOpenInfo(eventInfo, &fileInfo, DokanInstance);
      free(eventInfo);
      return;
    }
  }

  if (EventContext->Operation.Directory.FileIndex == 0) {
    ClearFindData(openInfo->DirListHead);
  }

  if (IsListEmpty(openInfo->DirListHead)) {

    DbgPrint("###FindFiles %04d\n", openInfo->EventId);

    // if user defined FindFilesWithPattern
    if (DokanInstance->DokanOperations->FindFilesWithPattern) {
      LPCWSTR pattern = L"*";

      // if search pattern is specified
      if (EventContext->Operation.Directory.SearchPatternLength != 0) {
        pattern = (PWCHAR)(
            (SIZE_T)&EventContext->Operation.Directory.SearchPatternBase[0] +
            (SIZE_T)EventContext->Operation.Directory.SearchPatternOffset);
      }

      patternCheck = FALSE; // do not recheck pattern later in MatchFiles

      status = DokanInstance->DokanOperations->FindFilesWithPattern(
          EventContext->Operation.Directory.DirectoryName, pattern,
          DokanFillFileData, &fileInfo);

    } else {
      status = STATUS_NOT_IMPLEMENTED;
    }

    if (status == STATUS_NOT_IMPLEMENTED &&
        DokanInstance->DokanOperations->FindFiles) {

      patternCheck = TRUE; // do pattern check later in MachFiles

      // call FileSystem specifeid callback routine
      status = DokanInstance->DokanOperations->FindFiles(
          EventContext->Operation.Directory.DirectoryName, DokanFillFileData,
          &fileInfo);
    }
  }

  if (status != STATUS_SUCCESS) {

    if (EventContext->Operation.Directory.FileIndex == 0) {
      DbgPrint("  STATUS_NO_SUCH_FILE\n");
      eventInfo->Status = STATUS_NO_SUCH_FILE;
    } else {
      DbgPrint("  STATUS_NO_MORE_FILES\n");
      eventInfo->Status = STATUS_NO_MORE_FILES;
    }

    eventInfo->BufferLength = 0;
    eventInfo->Operation.Directory.Index =
        EventContext->Operation.Directory.FileIndex;
    // free all of list entries
    ClearFindData(openInfo->DirListHead);
  } else {
    LONG index;
    eventInfo->Status = STATUS_SUCCESS;

    AddMissingCurrentAndParentFolder(EventContext, openInfo->DirListHead,
                                     &fileInfo);

    DbgPrint("index from %d\n", EventContext->Operation.Directory.FileIndex);
    // extract entries that match search pattern from FindFiles result
    index = MatchFiles(EventContext, eventInfo, openInfo->DirListHead,
                       patternCheck, DokanInstance);

    // there is no matched file
    if (index < 0) {
      eventInfo->BufferLength = 0;
      eventInfo->Operation.Directory.Index =
          EventContext->Operation.Directory.FileIndex;
      if (index == -1) {
        if (EventContext->Operation.Directory.FileIndex == 0) {
          DbgPrint("  STATUS_NO_SUCH_FILE\n");
          eventInfo->Status = STATUS_NO_SUCH_FILE;
        } else {
          DbgPrint("  STATUS_NO_MORE_FILES\n");
          eventInfo->Status = STATUS_NO_MORE_FILES;
        }
      } else {
        DbgPrint("  STATUS_BUFFER_OVERFLOW\n");
        eventInfo->Status = STATUS_BUFFER_OVERFLOW;
      }
      ClearFindData(openInfo->DirListHead);
    } else {
      DbgPrint("index to %d\n", index);
      eventInfo->Operation.Directory.Index = index;
    }
  }

  // information for FileSystem
  openInfo->UserContext = fileInfo.Context;

  // send directory information to driver
  SendEventInformation(Handle, eventInfo, sizeOfEventInfo);
  ReleaseDokanOpenInfo(eventInfo, &fileInfo, DokanInstance);
  free(eventInfo);
}

#define DOS_STAR (L'<')
#define DOS_QM (L'>')
#define DOS_DOT (L'"')

BOOL DOKANAPI DokanIsNameInExpression(LPCWSTR Expression, // matching pattern
                                      LPCWSTR Name,       // file name
                                      BOOL IgnoreCase) {
  ULONG ei = 0;
  ULONG ni = 0;

  while (Expression[ei] != '\0') {

    if (Expression[ei] == L'*') {
      ei++;
      if (Expression[ei] == '\0')
        return TRUE;

      while (Name[ni] != '\0') {
        if (DokanIsNameInExpression(&Expression[ei], &Name[ni], IgnoreCase))
          return TRUE;
        ni++;
      }

    } else if (Expression[ei] == DOS_STAR) {

      ULONG p = ni;
      ULONG lastDot = 0;
      ei++;

      while (Name[p] != '\0') {
        if (Name[p] == L'.')
          lastDot = p;
        p++;
      }

      BOOL endReached = FALSE;
      while (!endReached) {

        endReached = (Name[ni] == '\0' || ni == lastDot);

        if (!endReached) {
          if (DokanIsNameInExpression(&Expression[ei], &Name[ni], IgnoreCase))
            return TRUE;

          ni++;
        }
      }

    } else if (Expression[ei] == DOS_QM) {

      ei++;
      if (Name[ni] != L'.') {
        ni++;
      } else {

        ULONG p = ni + 1;
        while (Name[p] != '\0') {
          if (Name[p] == L'.')
            break;
          p++;
        }

        if (Name[p] == L'.')
          ni++;
      }

    } else if (Expression[ei] == DOS_DOT) {
      ei++;

      if (Name[ni] == L'.')
        ni++;

    } else {
      if (Expression[ei] == L'?') {
        ei++;
        ni++;
      } else if (IgnoreCase && towupper(Expression[ei]) == towupper(Name[ni])) {
        ei++;
        ni++;
      } else if (!IgnoreCase && Expression[ei] == Name[ni]) {
        ei++;
        ni++;
      } else {
        return FALSE;
      }
    }
  }

  if (ei == wcslen(Expression) && ni == wcslen(Name))
    return TRUE;

  return FALSE;
}
