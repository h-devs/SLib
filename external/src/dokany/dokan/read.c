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

VOID DispatchRead(HANDLE Handle, PEVENT_CONTEXT EventContext,
                  PDOKAN_INSTANCE DokanInstance) {
  PEVENT_INFORMATION eventInfo;
  PDOKAN_OPEN_INFO openInfo;
  ULONG readLength = 0;
  NTSTATUS status = STATUS_NOT_IMPLEMENTED;
  DOKAN_FILE_INFO fileInfo;
  ULONG sizeOfEventInfo = DispatchGetEventInformationLength(
      EventContext->Operation.Read.BufferLength);

  CheckFileName(EventContext->Operation.Read.FileName);

  eventInfo = DispatchCommon(EventContext, sizeOfEventInfo, DokanInstance,
                             &fileInfo, &openInfo);

  DbgPrint("###Read %04d\n", openInfo != NULL ? openInfo->EventId : -1);

  if (DokanInstance->DokanOperations->ReadFile) {
    status = DokanInstance->DokanOperations->ReadFile(
        EventContext->Operation.Read.FileName, eventInfo->Buffer,
        EventContext->Operation.Read.BufferLength, &readLength,
        EventContext->Operation.Read.ByteOffset.QuadPart, &fileInfo);
  }

  if (openInfo != NULL)
    openInfo->UserContext = fileInfo.Context;
  eventInfo->BufferLength = 0;
  eventInfo->Status = status;

  if (status == STATUS_SUCCESS) {
    if (readLength == 0) {
      eventInfo->Status = STATUS_END_OF_FILE;
    } else {
      eventInfo->BufferLength = readLength;
      eventInfo->Operation.Read.CurrentByteOffset.QuadPart =
          EventContext->Operation.Read.ByteOffset.QuadPart + readLength;
    }
  }

  SendEventInformation(Handle, eventInfo, sizeOfEventInfo);
  ReleaseDokanOpenInfo(eventInfo, &fileInfo, DokanInstance);
  free(eventInfo);
}
