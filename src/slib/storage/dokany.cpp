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

#include "slib/core/definition.h"

#ifdef SLIB_PLATFORM_IS_WIN32

#include "slib/storage/dokany.h"

#include "slib/core/service_manager.h"
#include "slib/core/dynamic_library.h"
#include "slib/core/platform_windows.h"
#include "slib/core/safe_static.h"

#define _EXPORTING
#include "dokany/dokan.h"

#include "file_system.h"

namespace slib
{

	namespace priv
	{
		namespace dokany
		{

			sl_bool g_flagDokany = sl_false;

			void* g_libDll;
			SLIB_GLOBAL_ZERO_INITIALIZED(AtomicString16, g_strDriverName)

			SLIB_IMPORT_FUNCTION_FROM_LIBRARY(g_libDll, DokanMain, int, DOKANAPI,
				PDOKAN_OPTIONS DokanOptions,
				PDOKAN_OPERATIONS DokanOperations)

			SLIB_IMPORT_FUNCTION_FROM_LIBRARY(g_libDll, DokanUnmount, BOOL, DOKANAPI,
				WCHAR DriveLetter)

			SLIB_IMPORT_FUNCTION_FROM_LIBRARY(g_libDll, DokanServiceInstall, BOOL, DOKANAPI,
				LPCWSTR ServiceName,
				DWORD ServiceType,
				LPCWSTR ServiceFullPath)

			SLIB_IMPORT_FUNCTION_FROM_LIBRARY(g_libDll, DokanServiceDelete, BOOL, DOKANAPI,
				LPCWSTR ServiceName)

			SLIB_IMPORT_FUNCTION_FROM_LIBRARY(g_libDll, DokanVersion, ULONG, DOKANAPI)

			SLIB_IMPORT_FUNCTION_FROM_LIBRARY(g_libDll, DokanDriverVersion, ULONG, DOKANAPI)

			SLIB_IMPORT_FUNCTION_FROM_LIBRARY(g_libDll, DokanResetTimeout, BOOL, DOKANAPI,
				ULONG Timeout,
				PDOKAN_FILE_INFO FileInfo)

			SLIB_IMPORT_FUNCTION_FROM_LIBRARY(g_libDll, DokanOpenRequestorToken, HANDLE, DOKANAPI,
				PDOKAN_FILE_INFO DokanFileInfo)

			SLIB_IMPORT_FUNCTION_FROM_LIBRARY(g_libDll, DokanRemoveMountPoint, BOOL, DOKANAPI,
				LPCWSTR MountPoint)

			SLIB_IMPORT_FUNCTION_FROM_LIBRARY(g_libDll, DokanMapKernelToUserCreateFileFlags, void, DOKANAPI,
				ACCESS_MASK DesiredAccess,
				ULONG FileAttributes,
				ULONG CreateOptions,
				ULONG CreateDisposition,
				ACCESS_MASK* outDesiredAccess,
				DWORD *outFileAttributesAndFlags,
				DWORD *outCreationDisposition)

			SLIB_IMPORT_FUNCTION_FROM_LIBRARY(g_libDll, DokanGetMountPointList, BOOL, DOKANAPI,
				PDOKAN_CONTROL list,
				ULONG length,
				BOOL uncOnly,
				PULONG nbRead)

			SLIB_IMPORT_FUNCTION_FROM_LIBRARY(g_libDll, DokanNtStatusFromWin32, NTSTATUS, DOKANAPI,
				DWORD Error)


			static sl_bool CheckDokanyOptions(const DOKAN_OPTIONS& options)
			{
				if (!Base::getStringLength2((sl_char16*)(options.MountPoint))) {
					LOG_ERROR(L"Mount Point required.");
					return sl_false;
				}
				if (g_flagDokany) {
					if (Base::getStringLength2((sl_char16*)(options.UNCName)) && !(options.Options & DOKAN_OPTION_NETWORK)) {
						LOG_ERROR("UNC provider name should be set on network drive only.");
						return sl_false;
					}
					if ((options.Options & DOKAN_OPTION_NETWORK) && (options.Options & DOKAN_OPTION_MOUNT_MANAGER)) {
						LOG_ERROR("Mount manager cannot be used on network drive.");
						return sl_false;
					}
					if ((options.Options & DOKAN_OPTION_MOUNT_MANAGER) && (options.Options & DOKAN_OPTION_CURRENT_SESSION)) {
						LOG_ERROR("Mount Manager always mount the drive for all user sessions.");
						return sl_false;
					}
				}
				return sl_true;
			};

			static sl_bool CheckPovider(FileSystemProvider* provider, DOKAN_OPTIONS& options)
			{
				SLIB_TRY {
					FileSystemInfo info;
					if (provider->getInformation(info, FileSystemInfoMask::Basic)) {
						if (info.sectorSize) {
							options.SectorSize = info.sectorSize;
							if (info.sectorsPerAllocationUnit) {
								options.AllocationUnitSize = info.sectorSize * info.sectorsPerAllocationUnit;
							}
						}
						if (info.flags & FileSystemFlags::ReadOnlyVolume) {
							options.Options |= DOKAN_OPTION_WRITE_PROTECT;
						}
						return sl_true;
					}
				} SLIB_CATCH(...)
				return sl_false;
			}

			// Only for Dokany
			NTSTATUS DOKAN_CALLBACK Dokany_ZwCreateFile(
					LPCWSTR					FileName,
					PDOKAN_IO_SECURITY_CONTEXT SecurityContext,
					ACCESS_MASK				DesiredAccess,
					ULONG					FileAttributes,
					ULONG					ShareAccess,
					ULONG					CreateDisposition,
					ULONG					CreateOptions,
					PDOKAN_FILE_INFO		DokanFileInfo)
			{
				FileSystemHost* host = (FileSystemHost*)(DokanFileInfo->DokanOptions->GlobalContext);
				FileSystemProvider* provider = host->getProvider();

				DWORD creationDisposition;
				DWORD fileAttributesAndFlags;
				ACCESS_MASK genericDesiredAccess;

				getApi_DokanMapKernelToUserCreateFileFlags()(
					DesiredAccess, FileAttributes, CreateOptions, CreateDisposition,
					&genericDesiredAccess, &fileAttributesAndFlags, &creationDisposition);

				// When filePath is a directory, needs to change the flag so that the file can be opened.
				DWORD attrs = 0;
				SLIB_TRY {
					FileInfo info;
					provider->getFileInfo(FileName, info, FileInfoMask::Attributes);
					attrs = info.attributes & 0x7ffff;
				} SLIB_CATCH(...)
				if (!attrs) {
					attrs = INVALID_FILE_ATTRIBUTES;
				}
				if (fileAttr != INVALID_FILE_ATTRIBUTES
					&& fileAttr & FILE_ATTRIBUTE_DIRECTORY) {
					if (!(CreateOptions & FILE_NON_DIRECTORY_FILE)) {
						DokanFileInfo->IsDirectory = TRUE;
						// Needed by FindFirstFile to list files in it
						// TODO: use ReOpenFile in MirrorFindFiles to set share read temporary
						ShareAccess |= FILE_SHARE_READ;
					} else { // FILE_NON_DIRECTORY_FILE - Cannot open a dir as a file
						return STATUS_FILE_IS_A_DIRECTORY;
					}
				}

				if (DokanFileInfo->IsDirectory) {
					// Check first if we're trying to open a file as a directory.
					if (fileAttr != INVALID_FILE_ATTRIBUTES &&
						!(fileAttr & FILE_ATTRIBUTE_DIRECTORY) &&
						(CreateOptions & FILE_DIRECTORY_FILE)) {
						return STATUS_NOT_A_DIRECTORY;
					}
					fileAttributesAndFlags |= FILE_ATTRIBUTE_DIRECTORY;
				} else {
					// Cannot overwrite a hidden or system file if flag not set
					if (fileAttr != INVALID_FILE_ATTRIBUTES &&
						((!(fileAttributesAndFlags & FILE_ATTRIBUTE_HIDDEN) &&
						(fileAttr & FILE_ATTRIBUTE_HIDDEN)) ||
							(!(fileAttributesAndFlags & FILE_ATTRIBUTE_SYSTEM) &&
							(fileAttr & FILE_ATTRIBUTE_SYSTEM))) &&
								(creationDisposition == TRUNCATE_EXISTING ||
									creationDisposition == CREATE_ALWAYS))
						return STATUS_ACCESS_DENIED;

					// Cannot delete a read only file
					if ((fileAttr != INVALID_FILE_ATTRIBUTES &&
						(fileAttr & FILE_ATTRIBUTE_READONLY) ||
						(fileAttributesAndFlags & FILE_ATTRIBUTE_READONLY)) &&
						(fileAttributesAndFlags & FILE_FLAG_DELETE_ON_CLOSE))
						return STATUS_CANNOT_DELETE;

					// Truncate should always be used with write access
					if (creationDisposition == TRUNCATE_EXISTING)
						genericDesiredAccess |= GENERIC_WRITE;
				}

				// Pass security context to CreateFile()
				DokanFileInfo->Context = (ULONG64)(PVOID)SecurityContext;
				DWORD ret = DokanHost::CreateFile(FileName,
					genericDesiredAccess,
					ShareAccess,
					creationDisposition,
					fileAttributesAndFlags,
					DokanFileInfo);

				if (ret == STATUS_SUCCESS &&
					!DokanFileInfo->IsDirectory) {
					// Need to update FileAttributes with previous when Overwrite file
					if (fileAttr != INVALID_FILE_ATTRIBUTES &&
						creationDisposition == TRUNCATE_EXISTING) {
						DokanHost::SetFileAttributes(FileName,
							fileAttributesAndFlags | fileAttr,
							DokanFileInfo);
					}
				}

				return ret;
			}

			SLIB_DOKAN_RET DOKAN_CALLBACK	// !SLIB_DOKAN_IS_DOKANY
				DokanHost::CreateFile(
					LPCWSTR					FileName,
					DWORD					AccessMode,
					DWORD					ShareMode,
					DWORD					CreationDisposition,
					DWORD					FlagsAndAttributes,
					PDOKAN_FILE_INFO		DokanFileInfo)
			{
				PSECURITY_DESCRIPTOR securityDescriptor = 0;
				if (g_flagDokany && DokanFileInfo->Context) {
					PDOKAN_IO_SECURITY_CONTEXT SecurityContext = (PDOKAN_IO_SECURITY_CONTEXT)DokanFileInfo->Context;
					securityDescriptor = SecurityContext->AccessState.SecurityDescriptor;
					DokanFileInfo->Context = 0;
				}

				FileSystemProvider *base = (FileSystemProvider *)DokanFileInfo->DokanOptions->GlobalContext;
				Ref<FileContext> context = GetFileContext(DokanFileInfo, FileName);
				int ret = 0;
				FILESYSTEM_EXCEPTION_GUARD(
					FileCreationParams params;
				params.accessMode = AccessMode;
				params.shareMode = ShareMode;
				params.flagsAndAttributes = FlagsAndAttributes;
				params.securityDescriptor = securityDescriptor;

				switch (CreationDisposition) {
				case CREATE_NEW:		// create only
					base->openFile(context, params);
					break;
				case CREATE_ALWAYS:		// create or overwrite existing			// modeWrite
					params.createAlways = sl_true;
					params.openTruncate = sl_true;
					try {
						base->openFile(context, params);
						if (params.createAlways == sl_false &&
							context->status == FileSystemError::FileExist) 	// openFile() supports createAlways, and file exists
							ret = ERROR_ALREADY_EXISTS;
						if (params.openTruncate)	// openFile() *may* not supports openTruncate, so truncate manually
							base->setFileInfo(context, FileInfo(), FileInfoFlags::SizeInfo);
					} catch (FileSystemError error) {
						if (error == FileSystemError::FileExist) {	// openFile() not supports createAlways, and file exists
							params.createAlways = sl_false;
							params.openTruncate = sl_true;
							base->openFile(context, params);
							if (params.openTruncate)	// openFile() *may* not support openTruncate, so truncate manually
								base->setFileInfo(context, FileInfo(), FileInfoFlags::SizeInfo);
							ret = ERROR_ALREADY_EXISTS;
						} else throw error;
					}
					break;
				case OPEN_ALWAYS:		// open or create
					params.createAlways = sl_true;
					try {
						base->openFile(context, params);
						if (params.createAlways) 	// openFile() not supports createAlways, and file exists
							ret = ERROR_ALREADY_EXISTS;
						else if (context->status == FileSystemError::FileExist) 	// openFile() supports createAlways, and file exists
							ret = ERROR_ALREADY_EXISTS;
					} catch (FileSystemError error) {
						if (error == FileSystemError::NotFound) {	// openFile() not supports createAlways, and file NOT exists, so create manually
							params.createAlways = sl_false;
							base->openFile(context, params);
						} else throw error;
					}
					break;
				case OPEN_EXISTING:		// open only							// modeRead
					base->openFile(context, params);
					break;
				case TRUNCATE_EXISTING:	// open only, and truncate
					params.openTruncate = sl_true;
					base->openFile(context, params);
					if (params.openTruncate)	// openFile() *may* not support openTruncate, so truncate manually
						base->setFileInfo(context, FileInfo(), FileInfoFlags::SizeInfo);
					break;
				}
				)

					DokanFileInfo->IsDirectory = context->isDirectory;
				DokanFileInfo->Context = (ULONG64)(context.ptr);
				context->increaseReference();
				base->increaseHandleCount(context->path);

				return (!g_flagDokany ? ret : DOKAN_RETERROR(ret));
			}

			SLIB_DOKAN_RET DOKAN_CALLBACK	// !SLIB_DOKAN_IS_DOKANY
				DokanHost::CreateDirectory(
					LPCWSTR					FileName,
					PDOKAN_FILE_INFO		DokanFileInfo)
			{
				return DokanHost::CreateFile(FileName,
					0,
					0,
					CREATE_NEW,
					FILE_ATTRIBUTE_DIRECTORY,
					DokanFileInfo);
			}

			SLIB_DOKAN_RET DOKAN_CALLBACK	// !SLIB_DOKAN_IS_DOKANY
				DokanHost::OpenDirectory(
					LPCWSTR					FileName,
					PDOKAN_FILE_INFO		DokanFileInfo)
			{
				return DokanHost::CreateFile(FileName,
					0,
					0,
					OPEN_EXISTING,
					FILE_ATTRIBUTE_DIRECTORY,
					DokanFileInfo);
			}

			SLIB_DOKAN_RET DOKAN_CALLBACK
				DokanHost::Cleanup(
					LPCWSTR					FileName,
					PDOKAN_FILE_INFO		DokanFileInfo)
			{
				FileSystemProvider *base = (FileSystemProvider *)DokanFileInfo->DokanOptions->GlobalContext;
				Ref<FileContext> context = GetFileContext(DokanFileInfo, FileName);
				FILESYSTEM_EXCEPTION_GUARD(
					if (DokanFileInfo->DeleteOnClose) {
						base->closeFile(context);
						// Should already be deleted by CloseHandle
						// if open with FILE_FLAG_DELETE_ON_CLOSE
						base->deleteFile(context, FALSE);
					}
				)
					return 0;
			}

			SLIB_DOKAN_RET DOKAN_CALLBACK
				DokanHost::CloseFile(
					LPCWSTR					FileName,
					PDOKAN_FILE_INFO		DokanFileInfo)
			{
				FileSystemProvider *base = (FileSystemProvider *)DokanFileInfo->DokanOptions->GlobalContext;
				Ref<FileContext> context = GetFileContext(DokanFileInfo, FileName);
				FILESYSTEM_EXCEPTION_GUARD(
					base->closeFile(context);
				base->decreaseHandleCount(context->path);
				context->decreaseReference();
				DokanFileInfo->Context = 0;
				)
					return 0;
			}

			SLIB_DOKAN_RET DOKAN_CALLBACK
				DokanHost::ReadFile(
					LPCWSTR				FileName,
					LPVOID				Buffer,
					DWORD				BufferLength,
					LPDWORD				ReadLength,
					LONGLONG			Offset,
					PDOKAN_FILE_INFO	DokanFileInfo)
			{
				FileSystemProvider *base = (FileSystemProvider *)DokanFileInfo->DokanOptions->GlobalContext;
				Ref<FileContext> context = GetFileContext(DokanFileInfo, FileName);
				FILESYSTEM_EXCEPTION_GUARD(
					MemoryData buf;
				*ReadLength = (DWORD)base->readFile(context,
					Memory::createStatic(Buffer, BufferLength), Offset);
				)
					return 0;
			}

			SLIB_DOKAN_RET DOKAN_CALLBACK
				DokanHost::WriteFile(
					LPCWSTR		FileName,
					LPCVOID		Buffer,
					DWORD		NumberOfBytesToWrite,
					LPDWORD		NumberOfBytesWritten,
					LONGLONG			Offset,
					PDOKAN_FILE_INFO	DokanFileInfo)
			{
				FileSystemProvider *base = (FileSystemProvider *)DokanFileInfo->DokanOptions->GlobalContext;
				Ref<FileContext> context = GetFileContext(DokanFileInfo, FileName);
				FILESYSTEM_EXCEPTION_GUARD(
					// Paging IO cannot write after allocate file size.
					if (DokanFileInfo->PagingIo) {
						UINT64 fileSize = base->getFileInfo(context).size;
						if ((UINT64)Offset >= fileSize) {
							*NumberOfBytesWritten = 0;
							return 0;
						}
						if (((UINT64)Offset + NumberOfBytesToWrite) > fileSize) {
							NumberOfBytesToWrite = (DWORD)((fileSize - Offset) & 0xFFFFFFFFUL);
						}
					}

				*NumberOfBytesWritten = (DWORD)base->writeFile(context,
					Memory::createStatic(Buffer, NumberOfBytesToWrite), Offset,
					DokanFileInfo->WriteToEndOfFile);
				)
					return 0;
			}

			SLIB_DOKAN_RET DOKAN_CALLBACK
				DokanHost::FlushFileBuffers(
					LPCWSTR		FileName,
					PDOKAN_FILE_INFO	DokanFileInfo)
			{
				FileSystemProvider *base = (FileSystemProvider *)DokanFileInfo->DokanOptions->GlobalContext;
				Ref<FileContext> context = GetFileContext(DokanFileInfo, FileName);
				FILESYSTEM_EXCEPTION_GUARD(
					base->flush(context);
				)
					return 0;
			}

			SLIB_DOKAN_RET DOKAN_CALLBACK
				DokanHost::GetFileInformation(
					LPCWSTR							FileName,
					LPBY_HANDLE_FILE_INFORMATION	HandleFileInformation,
					PDOKAN_FILE_INFO				DokanFileInfo)
			{
				FileSystemProvider *base = (FileSystemProvider *)DokanFileInfo->DokanOptions->GlobalContext;
				Ref<FileContext> context = GetFileContext(DokanFileInfo, FileName);
				FILESYSTEM_EXCEPTION_GUARD(
					FileInfo fileInfo = base->getFileInfo(context);
				HandleFileInformation->dwFileAttributes = fileInfo.fileAttributes;
				HandleFileInformation->nFileSizeHigh = (UINT32)(fileInfo.size >> 32);
				HandleFileInformation->nFileSizeLow = (UINT32)(fileInfo.size & 0xFFFFFFFF);
				TimeToFileTime(fileInfo.createdAt, HandleFileInformation->ftCreationTime);
				TimeToFileTime(fileInfo.lastAccessedAt, HandleFileInformation->ftLastAccessTime);
				TimeToFileTime(fileInfo.modifiedAt, HandleFileInformation->ftLastWriteTime);
				)
					return 0;
			}

			SLIB_DOKAN_RET DOKAN_CALLBACK
				DokanHost::FindFiles(
					LPCWSTR				PathName,
					PFillFindData		FillFindData,
					PDOKAN_FILE_INFO	DokanFileInfo)
			{
				return FindFilesWithPattern(PathName, L"*", FillFindData, DokanFileInfo);
			}

			SLIB_DOKAN_RET DOKAN_CALLBACK
				DokanHost::FindFilesWithPattern(
					LPCWSTR				PathName,
					LPCWSTR				SearchPattern,
					PFillFindData		FillFindData,
					PDOKAN_FILE_INFO	DokanFileInfo)
			{
				FileSystemProvider *base = (FileSystemProvider *)DokanFileInfo->DokanOptions->GlobalContext;
				Ref<FileContext> context = GetFileContext(DokanFileInfo, PathName);
				FILESYSTEM_EXCEPTION_GUARD(
					WIN32_FIND_DATA findData;
				auto files = base->getFiles(context, WCharToString(SearchPattern));

				for (auto& file : files) {
					FileInfo fileInfo = file.value;
					memset(&findData, 0, sizeof(findData));
					findData.dwFileAttributes = fileInfo.fileAttributes;
					findData.nFileSizeHigh = (UINT32)(fileInfo.size >> 32);
					findData.nFileSizeLow = (UINT32)(fileInfo.size & 0xFFFFFFFF);
					TimeToFileTime(fileInfo.createdAt, findData.ftCreationTime);
					TimeToFileTime(fileInfo.lastAccessedAt, findData.ftLastAccessTime);
					TimeToFileTime(fileInfo.modifiedAt, findData.ftLastWriteTime);
					StringToWChar(file.key, findData.cFileName);
					FillFindData(&findData, DokanFileInfo);
				}
				)
					return 0;
			}

			SLIB_DOKAN_RET DOKAN_CALLBACK	// SLIB_DOKAN_IS_DOKANY
				DokanHost::FindStreams(
					LPCWSTR				FileName,
					PFillFindStreamData	FillFindStreamData,
					PDOKAN_FILE_INFO	DokanFileInfo)
			{
				FileSystemProvider *base = (FileSystemProvider *)DokanFileInfo->DokanOptions->GlobalContext;
				Ref<FileContext> context = GetFileContext(DokanFileInfo, FileName);
				FILESYSTEM_EXCEPTION_GUARD(
					WIN32_FIND_STREAM_DATA findData;
				auto streams = base->fsFindStreams(context);

				for (auto& stream : streams) {
					StreamInfo streamInfo = stream.value;
					memset(&findData, 0, sizeof(findData));
					findData.StreamSize.QuadPart = streamInfo.size;
					StringToWChar(stream.key, findData.cStreamName);
					FillFindStreamData(&findData, DokanFileInfo);
				}
				)
					return 0;
			}

			SLIB_DOKAN_RET DOKAN_CALLBACK
				DokanHost::DeleteFile(
					LPCWSTR				FileName,
					PDOKAN_FILE_INFO	DokanFileInfo)
			{
				FileSystemProvider *base = (FileSystemProvider *)DokanFileInfo->DokanOptions->GlobalContext;
				Ref<FileContext> context = GetFileContext(DokanFileInfo, FileName);
				FILESYSTEM_EXCEPTION_GUARD(
					base->deleteFile(context, TRUE);
				)
					return 0;
			}

			SLIB_DOKAN_RET DOKAN_CALLBACK
				DokanHost::DeleteDirectory(
					LPCWSTR				FileName,
					PDOKAN_FILE_INFO	DokanFileInfo)
			{
				// Dokan notify that the file is requested not to be deleted.
				if (!DokanFileInfo->DeleteOnClose)
					return 0;
				return DeleteFile(FileName, DokanFileInfo);
			}

			SLIB_DOKAN_RET DOKAN_CALLBACK
				DokanHost::MoveFile(
					LPCWSTR				FileName, // existing file name
					LPCWSTR				NewFileName,
					BOOL				ReplaceIfExisting,
					PDOKAN_FILE_INFO	DokanFileInfo)
			{
				FileSystemProvider *base = (FileSystemProvider *)DokanFileInfo->DokanOptions->GlobalContext;
				Ref<FileContext> context = GetFileContext(DokanFileInfo, FileName);
				FILESYSTEM_EXCEPTION_GUARD(
					base->moveFile(context, WCharToString(NewFileName), ReplaceIfExisting);
				context->path = WCharToString(NewFileName);
				)
					return 0;
			}

			SLIB_DOKAN_RET DOKAN_CALLBACK
				DokanHost::SetEndOfFile(
					LPCWSTR				FileName,
					LONGLONG			ByteOffset,
					PDOKAN_FILE_INFO	DokanFileInfo)
			{
				FileSystemProvider *base = (FileSystemProvider *)DokanFileInfo->DokanOptions->GlobalContext;
				Ref<FileContext> context = GetFileContext(DokanFileInfo, FileName);
				FILESYSTEM_EXCEPTION_GUARD(
					FileInfo fileInfo;
				fileInfo.size = ByteOffset;
				base->setFileInfo(context, fileInfo, FileInfoFlags::SizeInfo);
				)
					return 0;
			}

			SLIB_DOKAN_RET DOKAN_CALLBACK
				DokanHost::SetAllocationSize(
					LPCWSTR				FileName,
					LONGLONG			AllocSize,
					PDOKAN_FILE_INFO	DokanFileInfo)
			{
				FileSystemProvider *base = (FileSystemProvider *)DokanFileInfo->DokanOptions->GlobalContext;
				Ref<FileContext> context = GetFileContext(DokanFileInfo, FileName);
				FILESYSTEM_EXCEPTION_GUARD(
					FileInfo fileInfo;
				fileInfo.allocSize = AllocSize;
				base->setFileInfo(context, fileInfo, FileInfoFlags::AllocSizeInfo);
				)
					return 0;
			}

			SLIB_DOKAN_RET DOKAN_CALLBACK
				DokanHost::SetFileAttributes(
					LPCWSTR				FileName,
					DWORD				FileAttributes,
					PDOKAN_FILE_INFO	DokanFileInfo)
			{
				FileSystemProvider *base = (FileSystemProvider *)DokanFileInfo->DokanOptions->GlobalContext;
				Ref<FileContext> context = GetFileContext(DokanFileInfo, FileName);
				FILESYSTEM_EXCEPTION_GUARD(
					FileInfo fileInfo;
				fileInfo.fileAttributes = FileAttributes;
				base->setFileInfo(context, fileInfo, FileInfoFlags::AttrInfo);
				)
					return 0;
			}

			SLIB_DOKAN_RET DOKAN_CALLBACK
				DokanHost::SetFileTime(
					LPCWSTR				FileName,
					CONST FILETIME*		CreationTime,
					CONST FILETIME*		LastAccessTime,
					CONST FILETIME*		LastWriteTime,
					PDOKAN_FILE_INFO	DokanFileInfo)
			{
				FileSystemProvider *base = (FileSystemProvider *)DokanFileInfo->DokanOptions->GlobalContext;
				Ref<FileContext> context = GetFileContext(DokanFileInfo, FileName);
				FILESYSTEM_EXCEPTION_GUARD(
					FileInfo fileInfo;
				if (CreationTime)
					FileTimeToTime(*CreationTime, fileInfo.createdAt);
				if (LastAccessTime)
					FileTimeToTime(*LastAccessTime, fileInfo.lastAccessedAt);
				if (LastWriteTime)
					FileTimeToTime(*LastWriteTime, fileInfo.modifiedAt);
				base->setFileInfo(context, fileInfo, FileInfoFlags::TimeInfo);
				)
					return 0;
			}

			SLIB_DOKAN_RET DOKAN_CALLBACK
				DokanHost::GetFileSecurity(
					LPCWSTR					FileName,
					PSECURITY_INFORMATION	SecurityInformation,
					PSECURITY_DESCRIPTOR	SecurityDescriptor,
					ULONG				BufferLength,
					PULONG				LengthNeeded,
					PDOKAN_FILE_INFO	DokanFileInfo)
			{
				FileSystemProvider *base = (FileSystemProvider *)DokanFileInfo->DokanOptions->GlobalContext;
				Ref<FileContext> context = GetFileContext(DokanFileInfo, FileName);

				if (!DokanHost::g_hasSeSecurityPrivilege) {
					*SecurityInformation &= ~0x00000008L/*SACL_SECURITY_INFORMATION*/;
					*SecurityInformation &= ~0x00010000L/*BACKUP_SECURITY_INFORMATION*/;
				}

				FILESYSTEM_EXCEPTION_GUARD(
					*LengthNeeded = (ULONG)base->fsGetSecurity(context, *SecurityInformation,
						Memory::createStatic(SecurityDescriptor, BufferLength));
				if (BufferLength < *LengthNeeded)
					return (!g_flagDokany ? DOKAN_RETERROR(ERROR_INSUFFICIENT_BUFFER) : STATUS_BUFFER_OVERFLOW);
				)
					return 0;
			}

			SLIB_DOKAN_RET DOKAN_CALLBACK
				DokanHost::SetFileSecurity(
					LPCWSTR					FileName,
					PSECURITY_INFORMATION	SecurityInformation,
					PSECURITY_DESCRIPTOR	SecurityDescriptor,
					ULONG				SecurityDescriptorLength,
					PDOKAN_FILE_INFO	DokanFileInfo)
			{
				FileSystemProvider *base = (FileSystemProvider *)DokanFileInfo->DokanOptions->GlobalContext;
				Ref<FileContext> context = GetFileContext(DokanFileInfo, FileName);
				FILESYSTEM_EXCEPTION_GUARD(
					base->fsSetSecurity(context, *SecurityInformation, Memory::create(SecurityDescriptor, SecurityDescriptorLength));
				)
					return 0;
			}

			SLIB_DOKAN_RET DOKAN_CALLBACK
				DokanHost::LockFile(
					LPCWSTR				FileName,
					LONGLONG			ByteOffset,
					LONGLONG			Length,
					PDOKAN_FILE_INFO	DokanFileInfo)
			{
				FileSystemProvider *base = (FileSystemProvider *)DokanFileInfo->DokanOptions->GlobalContext;
				Ref<FileContext> context = GetFileContext(DokanFileInfo, FileName);
				FILESYSTEM_EXCEPTION_GUARD(
					base->lockFile(context, ByteOffset, Length);
				)
					return 0;
			}

			SLIB_DOKAN_RET DOKAN_CALLBACK
				DokanHost::UnlockFile(
					LPCWSTR				FileName,
					LONGLONG			ByteOffset,
					LONGLONG			Length,
					PDOKAN_FILE_INFO	DokanFileInfo)
			{
				FileSystemProvider *base = (FileSystemProvider *)DokanFileInfo->DokanOptions->GlobalContext;
				Ref<FileContext> context = GetFileContext(DokanFileInfo, FileName);
				FILESYSTEM_EXCEPTION_GUARD(
					base->unlockFile(context, ByteOffset, Length);
				)
					return 0;
			}

			SLIB_DOKAN_RET DOKAN_CALLBACK
				DokanHost::GetDiskFreeSpace(
					PULONGLONG			FreeBytesAvailable,
					PULONGLONG			TotalNumberOfBytes,
					PULONGLONG			TotalNumberOfFreeBytes,
					PDOKAN_FILE_INFO	DokanFileInfo)
			{
				FileSystemProvider *base = (FileSystemProvider *)DokanFileInfo->DokanOptions->GlobalContext;
				FILESYSTEM_EXCEPTION_GUARD(
					sl_uint64 sizeTotal, sizeFree;
				if (base->getSize(&sizeTotal, &sizeFree)) {
					*FreeBytesAvailable = sizeFree;
					*TotalNumberOfBytes = sizeTotal;
					*TotalNumberOfFreeBytes = sizeFree;
				} else {
					return 0;
				}
				)
					return 0;
			}

			SLIB_DOKAN_RET DOKAN_CALLBACK
				DokanHost::GetVolumeInformation(
					LPWSTR		VolumeNameBuffer,
					DWORD		VolumeNameSize,
					LPDWORD		VolumeSerialNumber,
					LPDWORD		MaximumComponentLength,
					LPDWORD		FileSystemFlags,
					LPWSTR		FileSystemNameBuffer,
					DWORD		FileSystemNameSize,
					PDOKAN_FILE_INFO	DokanFileInfo)
			{
				FileSystemProvider *base = (FileSystemProvider *)DokanFileInfo->DokanOptions->GlobalContext;
				FILESYSTEM_EXCEPTION_GUARD(
					FileSystemInfo volumeInfo = base->getInformation();
				WCHAR Buffer[MAX_PATH] = L"";

				*VolumeSerialNumber = volumeInfo.serialNumber;
				*MaximumComponentLength = volumeInfo.maxPathLength;
				*FileSystemFlags = volumeInfo.flags;

				StringToWChar(volumeInfo.volumeName, Buffer);
				wcscpy_s(VolumeNameBuffer, VolumeNameSize / sizeof(WCHAR), Buffer);

				StringToWChar(volumeInfo.fileSystemName, Buffer);
				wcscpy_s(FileSystemNameBuffer, FileSystemNameSize / sizeof(WCHAR), Buffer);
				)
					return 0;
			}

			SLIB_DOKAN_RET DOKAN_CALLBACK	// SLIB_DOKAN_IS_DOKANY
				DokanHost::Mounted(
					PDOKAN_FILE_INFO	DokanFileInfo)
			{
				return 0;
			}

			SLIB_DOKAN_RET DOKAN_CALLBACK
				DokanHost::Unmounted(
					PDOKAN_FILE_INFO	DokanFileInfo)
			{
				return 0;
			}


			static void* GetDokanOperations()
			{
				if (g_flagDokany) {
					static void* op[] = {
						ZwCreateFile,
						Cleanup,
						CloseFile,
						ReadFile,
						WriteFile,
						FlushFileBuffers,
						GetFileInformation,
						FindFiles,
						NULL, //FindFilesWithPattern,
						SetFileAttributes,
						SetFileTime,
						DeleteFile,
						DeleteDirectory,
						MoveFile,
						SetEndOfFile,
						SetAllocationSize,
						LockFile,
						UnlockFile,
						GetDiskFreeSpace,
						GetVolumeInformation,
						Mounted,
						Unmounted,
						GetFileSecurity,
						SetFileSecurity,
						FindStreams,
					};
					return &op;
				} else {
					static void* op[] = {
						CreateFile,
						OpenDirectory,
						CreateDirectory,
						Cleanup,
						CloseFile,
						ReadFile,
						WriteFile,
						FlushFileBuffers,
						GetFileInformation,
						FindFiles,
						NULL, //FindFilesWithPattern,
						SetFileAttributes,
						SetFileTime,
						DeleteFile,
						DeleteDirectory,
						MoveFile,
						SetEndOfFile,
						SetAllocationSize,
						LockFile,
						UnlockFile,
						GetDiskFreeSpace,
						GetVolumeInformation,
						Unmounted,
						GetFileSecurity,
						SetFileSecurity,
					};
					return &op;
				}
			}

			class DokanHost : public FileSystemHost
			{
			public:
				DokanHost()
				{
				}

				~DokanHost()
				{
					stop();
				}

			public:
				sl_bool _run() override
				{
					FileSystemHostParam& param = m_param;

					String16 mountPoint = String16::from(param.mountPoint);

					DOKAN_OPTIONS options;
					Base::zeroMemory(&options, sizeof(options));

					options.Version = (USHORT)(getApi_DokanVersion()());
					options.ThreadCount = (USHORT)(param.threadCount);
					options.UNCName = L"";
					options.MountPoint = (LPCWSTR)(mountPoint.getData());

					if (!g_flagDokany) {
						options.Options = 8; // DOKAN_OPTION_KEEP_ALIVE (auto unmount)
					}

					if (param.flagDebugMode) {
						options.Options |= DOKAN_OPTION_DEBUG;
					}
					if (param.flagUseStderr) {
						options.Options |= DOKAN_OPTION_STDERR;
					}

					options.GlobalContext = (ULONG64)(void*)this;
					
					if (!(CheckDokanyOptions(options))) {
						return sl_false;
					}
					if (!(CheckPovider(param.provider.get(), options))) {
						return sl_false;
					}

					auto funcMain = getApi_DokanMain();
					if (!funcMain) {
						LOG_ERROR("Cannot get DokanMain function address.");
						return sl_false;
					}

					int status = funcMain(&options, (PDOKAN_OPERATIONS)GetDokanOperations());

					switch (status) {
					case DOKAN_SUCCESS:
						return sl_true;
					case DOKAN_ERROR:
						LOG_ERROR("Error");
						break;
					case DOKAN_DRIVE_LETTER_ERROR:
						LOG_ERROR("Bad Drive letter");
						break;
					case DOKAN_DRIVER_INSTALL_ERROR:
						LOG_ERROR("Can't install driver");
						break;
					case DOKAN_START_ERROR:
						LOG_ERROR("Driver something wrong");
						break;
					case DOKAN_MOUNT_ERROR:
						LOG_ERROR("Can't assign a drive letter");
						break;
					case DOKAN_MOUNT_POINT_ERROR:
						LOG_ERROR("Mount point error");
						break;
					case DOKAN_VERSION_ERROR:
						LOG_ERROR("Version error");
						break;
					default:
						LOG_ERROR("Unknown error: %d", status);
						break;
					}
					return sl_false;
				}

				void _stop() override
				{
					
				}
				
			};


		}
	}

	using namespace priv::dokany;

	sl_bool Dokany::initialize(sl_bool flagDokany, const StringParam& driverName, const StringParam& pathDll)
	{
		void* lib = DynamicLibrary::loadLibrary(pathDll);
		if (lib) {
			g_flagDokany = flagDokany;
			g_libDll = lib;
			g_strDriverName = driverName.toString16();
			return sl_true;
		}
		return sl_false;
	}

	sl_bool Dokany::initialize()
	{
		if (g_libDll) {
			return sl_true;
		}
		if (initialize(sl_true, L"Dokan1", "dokan1.dll")) {
			return sl_true;
		}
		if (initialize(sl_false, L"Dokan", "dokan.dll")) {
			return sl_true;
		}
		return sl_false;
	}

	sl_bool Dokany::isDokany()
	{
		if (initialize()) {
			return g_flagDokany;
		}
		return sl_false;
	}

	ServiceState Dokany::getDriverState()
	{
		if (!(initialize())) {
			return ServiceState::None;
		}
		return ServiceManager::getState(g_strDriverName);
	}

	sl_bool Dokany::startDriver()
	{
		if (!(initialize())) {
			return sl_false;
		}
		ServiceState state = getDriverState();
		if (state == ServiceState::None) {
			return sl_false;
		}
		if (state == ServiceState::Running) {
			return sl_true;
		}
		return ServiceManager::start(g_strDriverName);
	}

	sl_bool Dokany::stopDriver()
	{
		if (!(initialize())) {
			return sl_false;
		}
		ServiceState state = getDriverState();
		if (state == ServiceState::None) {
			return sl_false;
		}
		if (state == ServiceState::Stopped) {
			return sl_true;
		}
		return ServiceManager::stop(g_strDriverName);
	}

	Ref<FileSystemHost> Dokany::createHost()
	{
		return new DokanHost;
	}

	sl_bool Dokany::unmount(const StringParam& _mountPoint)
	{
		if (!(initialize())) {
			return sl_false;
		}
		auto func = getApi_DokanRemoveMountPoint();
		if (func) {
			StringCstr16 mountPoint(_mountPoint);
			return func((LPCWSTR)(mountPoint.getData()));
		}
		return sl_false;
	}

}

#endif
