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

#include <process.h>
#include "dokani.h"

BOOL DOKANAPI DokanResetTimeout(ULONG Timeout, PDOKAN_FILE_INFO FileInfo) {
  BOOL status;
  ULONG returnedLength;
  PDOKAN_INSTANCE instance;
  PDOKAN_OPEN_INFO openInfo;
  PEVENT_CONTEXT eventContext;
  PEVENT_INFORMATION eventInfo;
  ULONG eventInfoSize = sizeof(EVENT_INFORMATION);
  WCHAR rawDeviceName[MAX_PATH];

  openInfo = (PDOKAN_OPEN_INFO)(UINT_PTR)FileInfo->DokanContext;

  if (openInfo == NULL) {
    return FALSE;
  }

  eventContext = openInfo->EventContext;
  if (eventContext == NULL) {
    return FALSE;
  }

  instance = openInfo->DokanInstance;
  if (instance == NULL) {
    return FALSE;
  }

  eventInfo = (PEVENT_INFORMATION)malloc(eventInfoSize);
  if (eventInfo == NULL) {
    return FALSE;
  }
  RtlZeroMemory(eventInfo, eventInfoSize);

  eventInfo->SerialNumber = eventContext->SerialNumber;
  eventInfo->Operation.ResetTimeout.Timeout = Timeout;
  GetRawDeviceName(instance->DeviceName, rawDeviceName, MAX_PATH);
  status = SendToDevice(rawDeviceName,
      FSCTL_RESET_TIMEOUT, eventInfo, eventInfoSize, NULL, 0, &returnedLength);
  free(eventInfo);
  return status;
}

/* Legacy KeepAlive - Remove for 2.0.0 */
UINT WINAPI DokanKeepAlive(PVOID instance) {
  PDOKAN_INSTANCE DokanInstance = (PDOKAN_INSTANCE)instance;
  HANDLE device;
  ULONG ReturnedLength;
  WCHAR rawDeviceName[MAX_PATH];

  GetRawDeviceName(DokanInstance->DeviceName, rawDeviceName, MAX_PATH);

  while (TRUE) {

    device = CreateFile(rawDeviceName,
        GENERIC_READ | GENERIC_WRITE,       // dwDesiredAccess
        FILE_SHARE_READ | FILE_SHARE_WRITE, // dwShareMode
        NULL,                               // lpSecurityAttributes
        OPEN_EXISTING,                      // dwCreationDistribution
        0,                                  // dwFlagsAndAttributes
        NULL                                // hTemplateFile
    );

    if (device == INVALID_HANDLE_VALUE) {
      DbgPrint(
          "Dokan Error: DokanKeepAlive CreateFile failed %ws: %d\n",
          rawDeviceName,
          GetLastError());
      break;
    }

    BOOL status = DeviceIoControl(device,          // Handle to device
                                  IOCTL_KEEPALIVE, // IO Control code
                                  NULL,            // Input Buffer to driver.
                                  0,    // Length of input buffer in bytes.
                                  NULL, // Output Buffer from driver.
                                  0,    // Length of output buffer in bytes.
                                  &ReturnedLength, // Bytes placed in buffer.
                                  NULL             // synchronous call
    );

    CloseHandle(device);

    if (!status) {
      break;
    }

    Sleep(DOKAN_KEEPALIVE_TIME);
  }

  _endthreadex(0);
  return STATUS_SUCCESS;
}
