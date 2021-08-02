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
#include <conio.h>
#include <process.h>
#include <stdlib.h>
#include <tchar.h>
#include <strsafe.h>

#define DokanMapKernelBit(dest, src, userBit, kernelBit)                       \
  if (((src) & (kernelBit)) == (kernelBit))                                    \
  (dest) |= (userBit)

// DokanOptions->DebugMode is ON?
BOOL g_DebugMode = TRUE;

// DokanOptions->UseStdErr is ON?
BOOL g_UseStdErr = FALSE;

CRITICAL_SECTION g_InstanceCriticalSection;
LIST_ENTRY g_InstanceList;
HANDLE g_notify_handle = INVALID_HANDLE_VALUE;

VOID DOKANAPI DokanUseStdErr(BOOL Status) { g_UseStdErr = Status; }

VOID DOKANAPI DokanDebugMode(BOOL Status) { g_DebugMode = Status; }

VOID DispatchDriverLogs(HANDLE Handle, PEVENT_CONTEXT EventContext,
                        PDOKAN_INSTANCE DokanInstance) {
  UNREFERENCED_PARAMETER(Handle);
  UNREFERENCED_PARAMETER(DokanInstance);

  PDOKAN_LOG_MESSAGE log_message =
      (PDOKAN_LOG_MESSAGE)((PCHAR)EventContext + sizeof(EVENT_CONTEXT));
  if (log_message->MessageLength) {
    ULONG paquet_size = FIELD_OFFSET(DOKAN_LOG_MESSAGE, Message[0]) +
                        log_message->MessageLength;
    if (((PCHAR)log_message + paquet_size) <=
        ((PCHAR)EventContext + EventContext->Length)) {
      DbgPrint("DriverLog: %.*s\n", log_message->MessageLength,
               log_message->Message);
    } else {
      DbgPrint("Invalid driver log message received.\n");
    }
  }
}

PDOKAN_INSTANCE
NewDokanInstance() {
  PDOKAN_INSTANCE instance = (PDOKAN_INSTANCE)malloc(sizeof(DOKAN_INSTANCE));
  if (instance == NULL)
    return NULL;

  ZeroMemory(instance, sizeof(DOKAN_INSTANCE));

  (void)InitializeCriticalSectionAndSpinCount(&instance->CriticalSection,
                                              0x80000400);

  InitializeListHead(&instance->ListEntry);

  EnterCriticalSection(&g_InstanceCriticalSection);
  InsertTailList(&g_InstanceList, &instance->ListEntry);
  LeaveCriticalSection(&g_InstanceCriticalSection);

  return instance;
}

VOID DeleteDokanInstance(PDOKAN_INSTANCE Instance) {
  DeleteCriticalSection(&Instance->CriticalSection);

  EnterCriticalSection(&g_InstanceCriticalSection);
  RemoveEntryList(&Instance->ListEntry);
  LeaveCriticalSection(&g_InstanceCriticalSection);

  free(Instance);
}

BOOL IsMountPointDriveLetter(LPCWSTR mountPoint) {
  size_t mountPointLength;

  if (!mountPoint || *mountPoint == 0) {
    return FALSE;
  }

  mountPointLength = wcslen(mountPoint);

  if (mountPointLength == 1 ||
      (mountPointLength == 2 && mountPoint[1] == L':') ||
      (mountPointLength == 3 && mountPoint[1] == L':' &&
       mountPoint[2] == L'\\')) {

    return TRUE;
  }

  return FALSE;
}

BOOL IsValidDriveLetter(WCHAR DriveLetter) {
  return (L'a' <= DriveLetter && DriveLetter <= L'z') ||
         (L'A' <= DriveLetter && DriveLetter <= L'Z');
}

BOOL CheckDriveLetterAvailability(WCHAR DriveLetter) {
  DWORD result = 0;
  WCHAR buffer[MAX_PATH];
  WCHAR dosDevice[] = L"\\\\.\\C:";
  WCHAR driveName[] = L"C:";
  WCHAR driveLetter = towupper(DriveLetter);
  HANDLE device = NULL;
  dosDevice[4] = driveLetter;
  driveName[0] = driveLetter;

  DokanMountPointsCleanUp();

  if (!IsValidDriveLetter(driveLetter)) {
    DbgPrintW(L"CheckDriveLetterAvailability failed, bad drive letter %c\n",
              DriveLetter);
    return FALSE;
  }

  device = CreateFile(dosDevice, GENERIC_READ | GENERIC_WRITE,
                      FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
                      FILE_FLAG_NO_BUFFERING, NULL);

  if (device != INVALID_HANDLE_VALUE) {
    DbgPrintW(L"CheckDriveLetterAvailability failed, %c: is already used\n",
              DriveLetter);
    CloseHandle(device);
    return FALSE;
  }

  ZeroMemory(buffer, MAX_PATH * sizeof(WCHAR));
  result = QueryDosDevice(driveName, buffer, MAX_PATH);
  if (result > 0) {
    DbgPrintW(L"CheckDriveLetterAvailability failed, QueryDosDevice - Drive "
              L"letter \"%c\" is already used.\n",
              DriveLetter);
    return FALSE;
  }

  DWORD drives = GetLogicalDrives();
  result = (drives >> (driveLetter - L'A') & 0x00000001);
  if (result > 0) {
    DbgPrintW(L"CheckDriveLetterAvailability failed, GetLogicalDrives - Drive "
              L"letter \"%c\" is already used.\n",
              DriveLetter);
    return FALSE;
  }

  return TRUE;
}

void CheckAllocationUnitSectorSize(PDOKAN_OPTIONS DokanOptions) {
  ULONG allocationUnitSize = DokanOptions->AllocationUnitSize;
  ULONG sectorSize = DokanOptions->SectorSize;

  if ((allocationUnitSize < 512 || allocationUnitSize > 65536 ||
       (allocationUnitSize & (allocationUnitSize - 1)) != 0) // Is power of two
      || (sectorSize < 512 || sectorSize > 65536 ||
          (sectorSize & (sectorSize - 1)))) { // Is power of two
    // Reset to default if values does not fit windows FAT/NTFS value
    // https://support.microsoft.com/en-us/kb/140365
    DokanOptions->SectorSize = DOKAN_DEFAULT_SECTOR_SIZE;
    DokanOptions->AllocationUnitSize = DOKAN_DEFAULT_ALLOCATION_UNIT_SIZE;
  }

  DbgPrintW(L"AllocationUnitSize: %d SectorSize: %d\n",
            DokanOptions->AllocationUnitSize, DokanOptions->SectorSize);
}

int DOKANAPI DokanMain(PDOKAN_OPTIONS DokanOptions,
                       PDOKAN_OPERATIONS DokanOperations) {
  HANDLE device;
  HANDLE threadIds[DOKAN_MAX_THREAD];
  HANDLE legacyKeepAliveThreadIds = NULL;
  BOOL keepalive_active = FALSE;
  PDOKAN_INSTANCE instance;

  g_DebugMode = DokanOptions->Options & DOKAN_OPTION_DEBUG;
  g_UseStdErr = DokanOptions->Options & DOKAN_OPTION_STDERR;

  if (g_DebugMode) {
    DbgPrintW(L"Dokan: debug mode on\n");
  }

  if (g_UseStdErr) {
    DbgPrintW(L"Dokan: use stderr\n");
    g_DebugMode = TRUE;
  }

  if (DokanOptions->Options & DOKAN_OPTION_NETWORK &&
      !IsMountPointDriveLetter(DokanOptions->MountPoint)) {
    DokanOptions->Options &= ~DOKAN_OPTION_NETWORK;
    DbgPrintW(L"Dokan: Mount point folder is specified with network device "
              L"option. Disable network device.\n");
  }

  if (DokanOptions->Version < DOKAN_MINIMUM_COMPATIBLE_VERSION) {
    DokanDbgPrintW(
        L"Dokan Error: Incompatible version (%d), minimum is (%d) \n",
        DokanOptions->Version, DOKAN_MINIMUM_COMPATIBLE_VERSION);
    return DOKAN_VERSION_ERROR;
  }

  CheckAllocationUnitSectorSize(DokanOptions);

  if (DokanOptions->ThreadCount == 0) {
    DokanOptions->ThreadCount = 5;

  } else if (DOKAN_MAX_THREAD < DokanOptions->ThreadCount) {
    DokanDbgPrintW(L"Dokan Error: too many thread count %d\n",
                   DokanOptions->ThreadCount);
    DokanOptions->ThreadCount = DOKAN_MAX_THREAD;
  }

  device = CreateFile(DOKAN_GLOBAL_DEVICE_NAME,           // lpFileName
                      GENERIC_READ | GENERIC_WRITE,       // dwDesiredAccess
                      FILE_SHARE_READ | FILE_SHARE_WRITE, // dwShareMode
                      NULL,          // lpSecurityAttributes
                      OPEN_EXISTING, // dwCreationDistribution
                      0,             // dwFlagsAndAttributes
                      NULL           // hTemplateFile
  );

  if (device == INVALID_HANDLE_VALUE) {
    DokanDbgPrintW(L"Dokan Error: CreateFile Failed %s: %d\n",
                   DOKAN_GLOBAL_DEVICE_NAME, GetLastError());
    return DOKAN_DRIVER_INSTALL_ERROR;
  }

  DbgPrint("Global device opened\n");
  instance = NewDokanInstance();
  instance->DokanOptions = DokanOptions;
  instance->DokanOperations = DokanOperations;

  if (DokanOptions->MountPoint != NULL) {
    wcscpy_s(instance->MountPoint, sizeof(instance->MountPoint) / sizeof(WCHAR),
             DokanOptions->MountPoint);
    if (IsMountPointDriveLetter(instance->MountPoint)
      && !CheckDriveLetterAvailability(instance->MountPoint[0])) {
        DokanDbgPrint("Dokan Error: CheckDriveLetterAvailability Failed\n");
        CloseHandle(device);

        EnterCriticalSection(&g_InstanceCriticalSection);
        RemoveTailList(&g_InstanceList);
        LeaveCriticalSection(&g_InstanceCriticalSection);
        return DOKAN_MOUNT_ERROR;
      }
  }

  if (DokanOptions->UNCName != NULL) {
    wcscpy_s(instance->UNCName, sizeof(instance->UNCName) / sizeof(WCHAR),
             DokanOptions->UNCName);
  }

  if (!DokanStart(instance)) {
    CloseHandle(device);
    return DOKAN_START_ERROR;
  }

  for (ULONG i = 0; i < DokanOptions->ThreadCount; ++i) {
    threadIds[i] = (HANDLE)_beginthreadex(NULL, // Security Attributes
                                          0,    // stack size
                                          DokanLoop,
                                          (PVOID)instance, // param
                                          0, // create flag
                                          NULL);
  }

  if (!DokanMount(instance->MountPoint, instance->DeviceName, DokanOptions)) {
    SendReleaseIRP(instance->DeviceName);
    DokanDbgPrint("Dokan Error: DokanMount Failed\n");
    CloseHandle(device);
    return DOKAN_MOUNT_ERROR;
  }

  wchar_t keepalive_path[128];
  StringCbPrintfW(keepalive_path, sizeof(keepalive_path), L"\\\\?%s%s",
                  instance->DeviceName, DOKAN_KEEPALIVE_FILE_NAME);
  HANDLE keepalive_handle =
      CreateFile(keepalive_path, 0, 0, NULL, OPEN_EXISTING, 0, NULL);
  if (keepalive_handle == INVALID_HANDLE_VALUE) {
    // We don't consider this a fatal error because the keepalive handle is only
    // needed for abnormal termination cases anyway.
    DbgPrintW(L"Failed to open keepalive file: %s\n", keepalive_path);
  } else {
    DWORD keepalive_bytes_returned = 0;
    keepalive_active =
        DeviceIoControl(keepalive_handle, FSCTL_ACTIVATE_KEEPALIVE, NULL, 0,
                        NULL, 0, &keepalive_bytes_returned, NULL);
    if (!keepalive_active)
      DbgPrintW(L"Failed to activate keepalive handle.\n");
  }

  if (!keepalive_active) {
    DbgPrintW(L"Enable legacy keepalive.\n");
    // Start Legacy Keep Alive thread - We are probably using older driver
    legacyKeepAliveThreadIds =
        (HANDLE)_beginthreadex(NULL, // Security Attributes
                               0,    // stack size
                               DokanKeepAlive,
                               (PVOID)instance, // param
                               0,               // create flag
                               NULL);
  }

  if (DokanOptions->Options & DOKAN_OPTION_ENABLE_NOTIFICATION_API) {
    wchar_t notify_path[128];
    StringCbPrintfW(notify_path, sizeof(notify_path), L"\\\\?%s%s",
                    instance->DeviceName, DOKAN_NOTIFICATION_FILE_NAME);
    g_notify_handle = CreateFile(
        notify_path, 0, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
    if (g_notify_handle == INVALID_HANDLE_VALUE) {
      DbgPrintW(L"Failed to open notify handle: %s\n", notify_path);
    }
  }

  // Here we should have been mounter by mountmanager thanks to
  // IOCTL_MOUNTDEV_QUERY_SUGGESTED_LINK_NAME
  DbgPrintW(L"mounted: %s -> %s\n", instance->MountPoint, instance->DeviceName);

  if (DokanOperations->Mounted) {
    DOKAN_FILE_INFO fileInfo;
    RtlZeroMemory(&fileInfo, sizeof(DOKAN_FILE_INFO));
    fileInfo.DokanOptions = DokanOptions;
    // ignore return value
    DokanOperations->Mounted(&fileInfo);
  }

  // wait for loop thread terminations
  WaitForMultipleObjects(DokanOptions->ThreadCount, threadIds, TRUE, INFINITE);
  for (ULONG i = 0; i < DokanOptions->ThreadCount; ++i) {
    CloseHandle(threadIds[i]);
  }

  if (legacyKeepAliveThreadIds) {
    WaitForSingleObject(legacyKeepAliveThreadIds, INFINITE);
    CloseHandle(legacyKeepAliveThreadIds);
  }

  if (g_notify_handle != INVALID_HANDLE_VALUE)
    CloseHandle(g_notify_handle);
  // Note that the keepalive close that actually has unmounting effect is the
  // implicit one that happens if the process dies. If this one runs, it will be
  // a no-op.
  if (keepalive_handle != INVALID_HANDLE_VALUE)
    CloseHandle(keepalive_handle);
  CloseHandle(device);

  if (DokanOperations->Unmounted) {
    DOKAN_FILE_INFO fileInfo;
    RtlZeroMemory(&fileInfo, sizeof(DOKAN_FILE_INFO));
    fileInfo.DokanOptions = DokanOptions;
    // ignore return value
    DokanOperations->Unmounted(&fileInfo);
  }

  Sleep(1000);

  DbgPrint("\nunload\n");

  DeleteDokanInstance(instance);

  return DOKAN_SUCCESS;
}

VOID
GetRawDeviceName(LPCWSTR DeviceName, LPWSTR DestinationBuffer,
                 rsize_t DestinationBufferSizeInElements) {
  if (DeviceName && DestinationBuffer && DestinationBufferSizeInElements > 0) {
    wcscpy_s(DestinationBuffer, DestinationBufferSizeInElements, L"\\\\.");
    wcscat_s(DestinationBuffer, DestinationBufferSizeInElements, DeviceName);
  }
}

void ALIGN_ALLOCATION_SIZE(PLARGE_INTEGER size, PDOKAN_OPTIONS DokanOptions) {
  long long r = size->QuadPart % DokanOptions->AllocationUnitSize;
  size->QuadPart =
      (size->QuadPart + (r > 0 ? DokanOptions->AllocationUnitSize - r : 0));
}

UINT WINAPI DokanLoop(PVOID pDokanInstance) {
  HANDLE device = INVALID_HANDLE_VALUE;
  char *buffer = NULL;
  BOOL status;
  ULONG returnedLength;
  DWORD result = 0;
  DWORD lastError = 0;
  WCHAR rawDeviceName[MAX_PATH];
  PDOKAN_INSTANCE DokanInstance = pDokanInstance;

  buffer = malloc(sizeof(char) * EVENT_CONTEXT_MAX_SIZE);
  if (buffer == NULL) {
    result = (DWORD)-1;
    _endthreadex(result);
    return result;
  }
  RtlZeroMemory(buffer, sizeof(char) * EVENT_CONTEXT_MAX_SIZE);

  GetRawDeviceName(DokanInstance->DeviceName, rawDeviceName, MAX_PATH);

  status = TRUE;
  while (status) {

    device = CreateFile(rawDeviceName,                      // lpFileName
                        0,                                  // dwDesiredAccess
                        FILE_SHARE_READ | FILE_SHARE_WRITE, // dwShareMode
                        NULL,          // lpSecurityAttributes
                        OPEN_EXISTING, // dwCreationDistribution
                        0, // dwFlagsAndAttributes
                        NULL           // hTemplateFile
    );

    if (device == INVALID_HANDLE_VALUE) {
      DbgPrintW(
          L"Dokan Error: CreateFile failed %s: %d\n",
          rawDeviceName,
          GetLastError());
      free(buffer);
      result = (DWORD)-1;
      _endthreadex(result);
      return result;
    }

    status = DeviceIoControl(
        device,           // Handle to device
        FSCTL_EVENT_WAIT, // IO Control code
        NULL,             // Input Buffer to driver.
        0,                // Length of input buffer in bytes.
        buffer,           // Output Buffer from driver.
        sizeof(char) *
            EVENT_CONTEXT_MAX_SIZE, // Length of output buffer in bytes.
        &returnedLength,            // Bytes placed in buffer.
        NULL                        // synchronous call
        );

    if (!status) {
      lastError = GetLastError();
      DbgPrint("Ioctl failed for wait with code %d.\n", lastError);
      if (lastError == ERROR_NO_SYSTEM_RESOURCES) {
        DbgPrint("Processing will continue\n");
        status = TRUE;
        CloseHandle(device);
        Sleep(200);
        continue;
      }
      DbgPrint("Thread will be terminated\n");
      break;
    }

    // printf("#%d got notification %d\n", (ULONG)Param, count++);

    if (returnedLength > 0) {
      PEVENT_CONTEXT context = (PEVENT_CONTEXT)buffer;
      if (context->MountId != DokanInstance->MountId) {
        DbgPrint("Dokan Error: Invalid MountId (expected:%d, acctual:%d)\n",
                 DokanInstance->MountId, context->MountId);
        CloseHandle(device);
        continue;
      }

      switch (context->MajorFunction) {
      case IRP_MJ_CREATE:
        DispatchCreate(device, context, DokanInstance);
        break;
      case IRP_MJ_CLEANUP:
        DispatchCleanup(device, context, DokanInstance);
        break;
      case IRP_MJ_CLOSE:
        DispatchClose(device, context, DokanInstance);
        break;
      case IRP_MJ_DIRECTORY_CONTROL:
        DispatchDirectoryInformation(device, context, DokanInstance);
        break;
      case IRP_MJ_READ:
        DispatchRead(device, context, DokanInstance);
        break;
      case IRP_MJ_WRITE:
        DispatchWrite(device, context, DokanInstance);
        break;
      case IRP_MJ_QUERY_INFORMATION:
        DispatchQueryInformation(device, context, DokanInstance);
        break;
      case IRP_MJ_QUERY_VOLUME_INFORMATION:
        DispatchQueryVolumeInformation(device, context, DokanInstance);
        break;
      case IRP_MJ_LOCK_CONTROL:
        DispatchLock(device, context, DokanInstance);
        break;
      case IRP_MJ_SET_INFORMATION:
        DispatchSetInformation(device, context, DokanInstance);
        break;
      case IRP_MJ_FLUSH_BUFFERS:
        DispatchFlush(device, context, DokanInstance);
        break;
      case IRP_MJ_QUERY_SECURITY:
        DispatchQuerySecurity(device, context, DokanInstance);
        break;
      case IRP_MJ_SET_SECURITY:
        DispatchSetSecurity(device, context, DokanInstance);
        break;
      case DOKAN_IRP_LOG_MESSAGE:
        DispatchDriverLogs(device, context, DokanInstance);
      default:
        break;
      }

    } else {
      DbgPrint("ReturnedLength %d\n", returnedLength);
    }

    CloseHandle(device);
  }

  CloseHandle(device);
  free(buffer);
  _endthreadex(result);

  return result;
}

VOID SendEventInformation(HANDLE Handle, PEVENT_INFORMATION EventInfo,
                          ULONG EventLength) {
  BOOL status;
  ULONG returnedLength;

  // DbgPrint("###EventInfo->Context %X\n", EventInfo->Context);

  // send event info to driver
  status = DeviceIoControl(Handle,           // Handle to device
                           FSCTL_EVENT_INFO, // IO Control code
                           EventInfo,        // Input Buffer to driver.
                           EventLength,      // Length of input buffer in bytes.
                           NULL,             // Output Buffer from driver.
                           0,               // Length of output buffer in bytes.
                           &returnedLength, // Bytes placed in buffer.
                           NULL             // synchronous call
                           );

  if (!status) {
    DWORD errorCode = GetLastError();
    DbgPrint("Dokan Error: Ioctl failed with code %d\n", errorCode);
  }
}

VOID CheckFileName(LPWSTR FileName) {
  size_t len = wcslen(FileName);
  // if the beginning of file name is "\\",
  // replace it with "\"
  if (len >= 2 && FileName[0] == L'\\' && FileName[1] == L'\\') {
    int i;
    for (i = 0; FileName[i + 1] != L'\0'; ++i) {
      FileName[i] = FileName[i + 1];
    }
    FileName[i] = L'\0';
  }

  // Remove "\" in front of Directory
  len = wcslen(FileName);
  if (len > 2 && FileName[len - 1] == L'\\')
    FileName[len - 1] = '\0';
}

ULONG DispatchGetEventInformationLength(ULONG bufferSize) {
  // EVENT_INFORMATION has a buffer of size 8 already
  // we remote it to the struct size and add the requested buffer size
  // but we need at least to have enough space to set EVENT_INFORMATION
  return max((ULONG)sizeof(EVENT_INFORMATION),
             sizeof(EVENT_INFORMATION) - 8 + bufferSize);
}

PEVENT_INFORMATION
DispatchCommon(PEVENT_CONTEXT EventContext, ULONG SizeOfEventInfo,
               PDOKAN_INSTANCE DokanInstance, PDOKAN_FILE_INFO DokanFileInfo,
               PDOKAN_OPEN_INFO *DokanOpenInfo) {
  PEVENT_INFORMATION eventInfo = (PEVENT_INFORMATION)malloc(SizeOfEventInfo);

  if (eventInfo == NULL) {
    return NULL;
  }
  RtlZeroMemory(eventInfo, SizeOfEventInfo);
  RtlZeroMemory(DokanFileInfo, sizeof(DOKAN_FILE_INFO));

  eventInfo->BufferLength = 0;
  eventInfo->SerialNumber = EventContext->SerialNumber;

  DokanFileInfo->ProcessId = EventContext->ProcessId;
  DokanFileInfo->DokanOptions = DokanInstance->DokanOptions;
  if (EventContext->FileFlags & DOKAN_DELETE_ON_CLOSE) {
    DokanFileInfo->DeleteOnClose = 1;
  }
  if (EventContext->FileFlags & DOKAN_PAGING_IO) {
    DokanFileInfo->PagingIo = 1;
  }
  if (EventContext->FileFlags & DOKAN_WRITE_TO_END_OF_FILE) {
    DokanFileInfo->WriteToEndOfFile = 1;
  }
  if (EventContext->FileFlags & DOKAN_SYNCHRONOUS_IO) {
    DokanFileInfo->SynchronousIo = 1;
  }
  if (EventContext->FileFlags & DOKAN_NOCACHE) {
    DokanFileInfo->Nocache = 1;
  }

  *DokanOpenInfo = GetDokanOpenInfo(EventContext, DokanInstance);
  if (*DokanOpenInfo == NULL) {
    DbgPrint("error openInfo is NULL\n");
    return eventInfo;
  }

  DokanFileInfo->Context = (ULONG64)(*DokanOpenInfo)->UserContext;
  DokanFileInfo->IsDirectory = (UCHAR)(*DokanOpenInfo)->IsDirectory;
  DokanFileInfo->DokanContext = (ULONG64)(*DokanOpenInfo);

  eventInfo->Context = (ULONG64)(*DokanOpenInfo);

  return eventInfo;
}

PDOKAN_OPEN_INFO
GetDokanOpenInfo(PEVENT_CONTEXT EventContext, PDOKAN_INSTANCE DokanInstance) {
  PDOKAN_OPEN_INFO openInfo;
  EnterCriticalSection(&DokanInstance->CriticalSection);

  openInfo = (PDOKAN_OPEN_INFO)(UINT_PTR)EventContext->Context;
  if (openInfo != NULL) {
    openInfo->OpenCount++;
    openInfo->EventContext = EventContext;
    openInfo->DokanInstance = DokanInstance;
  }
  LeaveCriticalSection(&DokanInstance->CriticalSection);
  return openInfo;
}

VOID ReleaseDokanOpenInfo(PEVENT_INFORMATION EventInformation,
                          PDOKAN_FILE_INFO FileInfo,
                          PDOKAN_INSTANCE DokanInstance) {
  PDOKAN_OPEN_INFO openInfo;
  LPWSTR fileNameForClose = NULL;
  EnterCriticalSection(&DokanInstance->CriticalSection);

  openInfo = (PDOKAN_OPEN_INFO)(UINT_PTR)EventInformation->Context;
  if (openInfo != NULL) {
    openInfo->OpenCount--;
    if (openInfo->OpenCount < 1) {
      if (openInfo->DirListHead != NULL) {
        ClearFindData(openInfo->DirListHead);
        free(openInfo->DirListHead);
        openInfo->DirListHead = NULL;
      }
      if (openInfo->StreamListHead != NULL) {
        ClearFindStreamData(openInfo->StreamListHead);
        free(openInfo->StreamListHead);
        openInfo->StreamListHead = NULL;
      }
      if (openInfo->FileName) {
        fileNameForClose = openInfo->FileName;
      }
      free(openInfo);
      EventInformation->Context = 0;
    }
  }
  LeaveCriticalSection(&DokanInstance->CriticalSection);

  if (fileNameForClose) {
    if (DokanInstance->DokanOperations->CloseFile) {
      DokanInstance->DokanOperations->CloseFile(fileNameForClose, FileInfo);
    }
    free(fileNameForClose);
  }
}

// ask driver to release all pending IRP to prepare for Unmount.
BOOL SendReleaseIRP(LPCWSTR DeviceName) {
  ULONG returnedLength;
  WCHAR rawDeviceName[MAX_PATH];

  DbgPrintW(L"send release to %s\n", DeviceName);

  GetRawDeviceName(DeviceName, rawDeviceName, MAX_PATH);
  if (!SendToDevice(rawDeviceName,
                    FSCTL_EVENT_RELEASE, NULL, 0, NULL, 0, &returnedLength)) {

    DbgPrintW(L"Failed to unmount device:%s\n", DeviceName);
    return FALSE;
  }

  return TRUE;
}

BOOL SendGlobalReleaseIRP(LPCWSTR MountPoint) {
  if (MountPoint != NULL) {
    size_t length = wcslen(MountPoint);
    if (length > 0) {
      ULONG returnedLength;
      ULONG inputLength = sizeof(DOKAN_UNICODE_STRING_INTERMEDIATE) +
                          (MAX_PATH * sizeof(WCHAR));
      PDOKAN_UNICODE_STRING_INTERMEDIATE szMountPoint = malloc(inputLength);

      if (szMountPoint != NULL) {
        ZeroMemory(szMountPoint, inputLength);
        szMountPoint->MaximumLength = MAX_PATH * sizeof(WCHAR);
        szMountPoint->Length = (USHORT)(length * sizeof(WCHAR));
        CopyMemory(szMountPoint->Buffer, MountPoint, szMountPoint->Length);

        DbgPrintW(L"send global release for %s\n", MountPoint);

        if (!SendToDevice(DOKAN_GLOBAL_DEVICE_NAME, FSCTL_EVENT_RELEASE,
                          szMountPoint, inputLength, NULL, 0,
                          &returnedLength)) {

          DbgPrintW(L"Failed to unmount: %s\n", MountPoint);
          free(szMountPoint);
          return FALSE;
        }

        free(szMountPoint);
        return TRUE;
      }
    }
  }

  return FALSE;
}

BOOL DokanStart(PDOKAN_INSTANCE Instance) {
  EVENT_START eventStart;
  EVENT_DRIVER_INFO driverInfo;
  ULONG returnedLength = 0;

  ZeroMemory(&eventStart, sizeof(EVENT_START));
  ZeroMemory(&driverInfo, sizeof(EVENT_DRIVER_INFO));

  eventStart.UserVersion = DOKAN_DRIVER_VERSION;
  if (Instance->DokanOptions->Options & DOKAN_OPTION_ALT_STREAM) {
    eventStart.Flags |= DOKAN_EVENT_ALTERNATIVE_STREAM_ON;
  }
  if (Instance->DokanOptions->Options & DOKAN_OPTION_NETWORK) {
    eventStart.DeviceType = DOKAN_NETWORK_FILE_SYSTEM;
  }
  if (Instance->DokanOptions->Options & DOKAN_OPTION_REMOVABLE) {
    eventStart.Flags |= DOKAN_EVENT_REMOVABLE;
  }
  if (Instance->DokanOptions->Options & DOKAN_OPTION_WRITE_PROTECT) {
    eventStart.Flags |= DOKAN_EVENT_WRITE_PROTECT;
  }
  if (Instance->DokanOptions->Options & DOKAN_OPTION_MOUNT_MANAGER) {
    eventStart.Flags |= DOKAN_EVENT_MOUNT_MANAGER;
  }
  if (Instance->DokanOptions->Options & DOKAN_OPTION_CURRENT_SESSION) {
    eventStart.Flags |= DOKAN_EVENT_CURRENT_SESSION;
  }
  if (Instance->DokanOptions->Options & DOKAN_OPTION_FILELOCK_USER_MODE) {
    eventStart.Flags |= DOKAN_EVENT_FILELOCK_USER_MODE;
  }
  if (Instance->DokanOptions->Options & DOKAN_OPTION_ENABLE_UNMOUNT_NETWORK_DRIVE) {
    eventStart.Flags |= DOKAN_EVENT_ENABLE_NETWORK_UNMOUNT;
  }
  if (Instance->DokanOptions->Options &
      DOKAN_OPTION_ENABLE_FCB_GARBAGE_COLLECTION) {
    eventStart.Flags |= DOKAN_EVENT_ENABLE_FCB_GC;
  }
  if (Instance->DokanOptions->Options & DOKAN_OPTION_CASE_SENSITIVE) {
    eventStart.Flags |= DOKAN_EVENT_CASE_SENSITIVE;
  }
  if (Instance->DokanOptions->Options & DOKAN_OPTION_DISPATCH_DRIVER_LOGS) {
    eventStart.Flags |= DOKAN_EVENT_DISPATCH_DRIVER_LOGS;
  }

  memcpy_s(eventStart.MountPoint, sizeof(eventStart.MountPoint),
           Instance->MountPoint, sizeof(Instance->MountPoint));
  memcpy_s(eventStart.UNCName, sizeof(eventStart.UNCName), Instance->UNCName,
           sizeof(Instance->UNCName));

  eventStart.IrpTimeout = Instance->DokanOptions->Timeout;

  SendToDevice(DOKAN_GLOBAL_DEVICE_NAME, FSCTL_EVENT_START, &eventStart,
               sizeof(EVENT_START), &driverInfo, sizeof(EVENT_DRIVER_INFO),
               &returnedLength);

  if (driverInfo.Status == DOKAN_START_FAILED) {
    if (driverInfo.DriverVersion != eventStart.UserVersion) {
      DokanDbgPrint("Dokan Error: driver version mismatch, driver %X, dll %X\n",
                    driverInfo.DriverVersion, eventStart.UserVersion);
    } else {
      DokanDbgPrint("Dokan Error: driver start error\n");
    }
    return FALSE;
  } else if (driverInfo.Status == DOKAN_MOUNTED) {
    Instance->MountId = driverInfo.MountId;
    Instance->DeviceNumber = driverInfo.DeviceNumber;
    wcscpy_s(Instance->DeviceName, sizeof(Instance->DeviceName) / sizeof(WCHAR),
             driverInfo.DeviceName);
    return TRUE;
  }
  return FALSE;
}

BOOL DOKANAPI DokanSetDebugMode(ULONG Mode) {
  ULONG returnedLength;
  return SendToDevice(DOKAN_GLOBAL_DEVICE_NAME, FSCTL_SET_DEBUG_MODE, &Mode,
                      sizeof(ULONG), NULL, 0, &returnedLength);
}

BOOL DOKANAPI DokanMountPointsCleanUp() {
    ULONG returnedLength;
    return SendToDevice(DOKAN_GLOBAL_DEVICE_NAME, FSCTL_MOUNTPOINT_CLEANUP, NULL,
        0, NULL, 0, &returnedLength);
}

BOOL SendToDevice(LPCWSTR DeviceName, DWORD IoControlCode, PVOID InputBuffer,
                  ULONG InputLength, PVOID OutputBuffer, ULONG OutputLength,
                  PULONG ReturnedLength) {
  HANDLE device;
  BOOL status;

  device = CreateFile(DeviceName,                         // lpFileName
                      0,                                  // dwDesiredAccess
                      FILE_SHARE_READ | FILE_SHARE_WRITE, // dwShareMode
                      NULL,          // lpSecurityAttributes
                      OPEN_EXISTING, // dwCreationDistribution
                      0,             // dwFlagsAndAttributes
                      NULL           // hTemplateFile
  );

  if (device == INVALID_HANDLE_VALUE) {
    DWORD dwErrorCode = GetLastError();
    DbgPrintW(L"Dokan Error: Failed to open %ws with code %d\n", DeviceName,
             dwErrorCode);
    return FALSE;
  }

  status = DeviceIoControl(device,         // Handle to device
                           IoControlCode,  // IO Control code
                           InputBuffer,    // Input Buffer to driver.
                           InputLength,    // Length of input buffer in bytes.
                           OutputBuffer,   // Output Buffer from driver.
                           OutputLength,   // Length of output buffer in bytes.
                           ReturnedLength, // Bytes placed in buffer.
                           NULL            // synchronous call
                           );

  CloseHandle(device);

  if (!status) {
    DbgPrint("DokanError: Ioctl 0x%x failed with code %d on Device %ws\n",
             IoControlCode, GetLastError(), DeviceName);
    return FALSE;
  }

  return TRUE;
}

PDOKAN_CONTROL DOKANAPI DokanGetMountPointList(BOOL uncOnly, PULONG nbRead) {
  ULONG returnedLength = 0;
  PDOKAN_CONTROL dokanControl = NULL;
  PDOKAN_CONTROL results = NULL;
  ULONG bufferLength = 32 * sizeof(*dokanControl);
  BOOL success;

  *nbRead = 0;

  do {
    if (dokanControl != NULL)
      free(dokanControl);
    dokanControl = malloc(bufferLength);
    if (dokanControl == NULL)
      return NULL;
    ZeroMemory(dokanControl, bufferLength);

    success =
        SendToDevice(DOKAN_GLOBAL_DEVICE_NAME, FSCTL_EVENT_MOUNTPOINT_LIST,
                     NULL, 0, dokanControl, bufferLength, &returnedLength);

    if (!success && GetLastError() != ERROR_MORE_DATA) {
      free(dokanControl);
      return NULL;
    }
    bufferLength *= 2;
  } while (!success);

  if (returnedLength == 0) {
    free(dokanControl);
    return NULL;
  }

  *nbRead = returnedLength / sizeof(DOKAN_CONTROL);
  results = malloc(returnedLength);
  if (results != NULL) {
    ZeroMemory(results, returnedLength);
    for (ULONG i = 0; i < *nbRead; ++i) {
      if (!uncOnly || wcscmp(dokanControl[i].UNCName, L"") != 0)
        CopyMemory(&results[i], &dokanControl[i], sizeof(DOKAN_CONTROL));
    }
  }
  free(dokanControl);
  return results;
}

VOID DOKANAPI DokanReleaseMountPointList(PDOKAN_CONTROL list) { free(list); }

void InitializeDokany()
{
	InitializeCriticalSectionAndSpinCount(&g_InstanceCriticalSection,
		0x80000400);
	InitializeListHead(&g_InstanceList);
}

void UninitializeDokany()
{
	EnterCriticalSection(&g_InstanceCriticalSection);

	while (!IsListEmpty(&g_InstanceList)) {
		PLIST_ENTRY entry = RemoveHeadList(&g_InstanceList);
		PDOKAN_INSTANCE instance =
			CONTAINING_RECORD(entry, DOKAN_INSTANCE, ListEntry);
		DokanRemoveMountPointEx(instance->MountPoint, FALSE);
		free(instance);
	}

	LeaveCriticalSection(&g_InstanceCriticalSection);
	DeleteCriticalSection(&g_InstanceCriticalSection);
}

/*
BOOL WINAPI DllMain(HINSTANCE Instance, DWORD Reason, LPVOID Reserved) {
  UNREFERENCED_PARAMETER(Reserved);
  UNREFERENCED_PARAMETER(Instance);

  switch (Reason) {
  case DLL_PROCESS_ATTACH: {
    (void)InitializeCriticalSectionAndSpinCount(&g_InstanceCriticalSection,
                                          0x80000400);

    InitializeListHead(&g_InstanceList);
  } break;
  case DLL_PROCESS_DETACH: {
    EnterCriticalSection(&g_InstanceCriticalSection);

    while (!IsListEmpty(&g_InstanceList)) {
      PLIST_ENTRY entry = RemoveHeadList(&g_InstanceList);
      PDOKAN_INSTANCE instance =
          CONTAINING_RECORD(entry, DOKAN_INSTANCE, ListEntry);
      DokanRemoveMountPointEx(instance->MountPoint, FALSE);
      free(instance);
    }

    LeaveCriticalSection(&g_InstanceCriticalSection);
    DeleteCriticalSection(&g_InstanceCriticalSection);
  } break;
  default:
    break;
  }
  return TRUE;
}
*/

void DOKANAPI DokanMapKernelToUserCreateFileFlags(
	ACCESS_MASK DesiredAccess, ULONG FileAttributes, ULONG CreateOptions, ULONG CreateDisposition,
	ACCESS_MASK* outDesiredAccess, DWORD *outFileAttributesAndFlags, DWORD *outCreationDisposition) {
	BOOL genericRead = FALSE, genericWrite = FALSE, genericExecute = FALSE,
		genericAll = FALSE;

  if (outFileAttributesAndFlags) {

    *outFileAttributesAndFlags = FileAttributes;

    DokanMapKernelBit(*outFileAttributesAndFlags, CreateOptions,
                      FILE_FLAG_WRITE_THROUGH, FILE_WRITE_THROUGH);
    DokanMapKernelBit(*outFileAttributesAndFlags, CreateOptions,
                      FILE_FLAG_SEQUENTIAL_SCAN, FILE_SEQUENTIAL_ONLY);
    DokanMapKernelBit(*outFileAttributesAndFlags, CreateOptions,
                      FILE_FLAG_RANDOM_ACCESS, FILE_RANDOM_ACCESS);
    DokanMapKernelBit(*outFileAttributesAndFlags, CreateOptions,
                      FILE_FLAG_NO_BUFFERING, FILE_NO_INTERMEDIATE_BUFFERING);
    DokanMapKernelBit(*outFileAttributesAndFlags, CreateOptions,
                      FILE_FLAG_OPEN_REPARSE_POINT, FILE_OPEN_REPARSE_POINT);
    DokanMapKernelBit(*outFileAttributesAndFlags, CreateOptions,
                      FILE_FLAG_DELETE_ON_CLOSE, FILE_DELETE_ON_CLOSE);
    DokanMapKernelBit(*outFileAttributesAndFlags, CreateOptions,
                      FILE_FLAG_BACKUP_SEMANTICS, FILE_OPEN_FOR_BACKUP_INTENT);

#if (_WIN32_WINNT >= _WIN32_WINNT_WIN8)
    DokanMapKernelBit(*outFileAttributesAndFlags, CreateOptions,
                      FILE_FLAG_SESSION_AWARE, FILE_SESSION_AWARE);
#endif
  }

  if (outCreationDisposition) {

    switch (CreateDisposition) {
    case FILE_CREATE:
      *outCreationDisposition = CREATE_NEW;
      break;
    case FILE_OPEN:
      *outCreationDisposition = OPEN_EXISTING;
      break;
    case FILE_OPEN_IF:
      *outCreationDisposition = OPEN_ALWAYS;
      break;
    case FILE_OVERWRITE:
      *outCreationDisposition = TRUNCATE_EXISTING;
      break;
    case FILE_SUPERSEDE:
    // The documentation isn't clear on the difference between replacing a file
    // and truncating it.
    // For now we just map it to create/truncate
    case FILE_OVERWRITE_IF:
      *outCreationDisposition = CREATE_ALWAYS;
      break;
    default:
      *outCreationDisposition = 0;
      break;
    }
  }

  if (outDesiredAccess) {

	  *outDesiredAccess = DesiredAccess;

	  if ((*outDesiredAccess & FILE_GENERIC_READ) == FILE_GENERIC_READ) {
		  *outDesiredAccess |= GENERIC_READ;
		  genericRead = TRUE;
	  }
	  if ((*outDesiredAccess & FILE_GENERIC_WRITE) == FILE_GENERIC_WRITE) {
		  *outDesiredAccess |= GENERIC_WRITE;
		  genericWrite = TRUE;
	  }
	  if ((*outDesiredAccess & FILE_GENERIC_EXECUTE) == FILE_GENERIC_EXECUTE) {
		  *outDesiredAccess |= GENERIC_EXECUTE;
		  genericExecute = TRUE;
	  }
	  if ((*outDesiredAccess & FILE_ALL_ACCESS) == FILE_ALL_ACCESS) {
		  *outDesiredAccess |= GENERIC_ALL;
		  genericAll = TRUE;
	  }

	  if (genericRead)
		  *outDesiredAccess &= ~FILE_GENERIC_READ;
	  if (genericWrite)
		  *outDesiredAccess &= ~FILE_GENERIC_WRITE;
	  if (genericExecute)
		  *outDesiredAccess &= ~FILE_GENERIC_EXECUTE;
	  if (genericAll)
		  *outDesiredAccess &= ~FILE_ALL_ACCESS;
  }
}

BOOL DOKANAPI DokanNotifyPath(LPCWSTR FilePath, ULONG CompletionFilter,
                              ULONG Action) {
  if (FilePath == NULL || g_notify_handle == INVALID_HANDLE_VALUE) {
    return FALSE;
  }
  size_t length = wcslen(FilePath);
  const size_t prefixSize = 2; // size of mount letter plus ":"
  if (length <= prefixSize) {
    return FALSE;
  }
  // remove the mount letter and colon from length, for example: "G:"
  length -= prefixSize;
  ULONG returnedLength;
  ULONG inputLength = (ULONG)(
      sizeof(DOKAN_NOTIFY_PATH_INTERMEDIATE) + (length * sizeof(WCHAR)));
  PDOKAN_NOTIFY_PATH_INTERMEDIATE pNotifyPath = malloc(inputLength);
  if (pNotifyPath == NULL) {
    DbgPrint("Failed to allocate NotifyPath\n");
    return FALSE;
  }
  ZeroMemory(pNotifyPath, inputLength);
  pNotifyPath->CompletionFilter = CompletionFilter;
  pNotifyPath->Action = Action;
  pNotifyPath->Length = (USHORT)(length * sizeof(WCHAR));
  CopyMemory(pNotifyPath->Buffer, FilePath + prefixSize, pNotifyPath->Length);
  if (!DeviceIoControl(g_notify_handle, FSCTL_NOTIFY_PATH, pNotifyPath,
                       inputLength, NULL, 0, &returnedLength, NULL)) {
    DbgPrint("Failed to send notify path command:%ws\n", FilePath);
    free(pNotifyPath);
    return FALSE;
  }
  free(pNotifyPath);
  return TRUE;
}

BOOL DOKANAPI DokanNotifyCreate(LPCWSTR FilePath, BOOL IsDirectory) {
  return DokanNotifyPath(FilePath,
                         IsDirectory ? FILE_NOTIFY_CHANGE_DIR_NAME
                                     : FILE_NOTIFY_CHANGE_FILE_NAME,
                         FILE_ACTION_ADDED);
}

BOOL DOKANAPI DokanNotifyDelete(LPCWSTR FilePath, BOOL IsDirectory) {
  return DokanNotifyPath(FilePath,
                         IsDirectory ? FILE_NOTIFY_CHANGE_DIR_NAME
                                     : FILE_NOTIFY_CHANGE_FILE_NAME,
                         FILE_ACTION_REMOVED);
}

BOOL DOKANAPI DokanNotifyUpdate(LPCWSTR FilePath) {
  return DokanNotifyPath(FilePath, FILE_NOTIFY_CHANGE_ATTRIBUTES,
                         FILE_ACTION_MODIFIED);
}

BOOL DOKANAPI DokanNotifyXAttrUpdate(LPCWSTR FilePath) {
  return DokanNotifyPath(FilePath, FILE_NOTIFY_CHANGE_ATTRIBUTES,
                         FILE_ACTION_MODIFIED);
}

BOOL DOKANAPI DokanNotifyRename(LPCWSTR OldPath, LPCWSTR NewPath,
                                BOOL IsDirectory, BOOL IsInSameDirectory) {
  BOOL success = DokanNotifyPath(
      OldPath,
      IsDirectory ? FILE_NOTIFY_CHANGE_DIR_NAME : FILE_NOTIFY_CHANGE_FILE_NAME,
      IsInSameDirectory ? FILE_ACTION_RENAMED_OLD_NAME : FILE_ACTION_REMOVED);
  success &= DokanNotifyPath(
      NewPath,
      IsDirectory ? FILE_NOTIFY_CHANGE_DIR_NAME : FILE_NOTIFY_CHANGE_FILE_NAME,
      IsInSameDirectory ? FILE_ACTION_RENAMED_NEW_NAME : FILE_ACTION_ADDED);
  return success;
}
