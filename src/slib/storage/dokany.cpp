/*
*   Copyright (c) 2008-2021 SLIBIO <https://github.com/SLIBIO>
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

#define _EXPORTING
#include "dokany/dokan.h"

#include "slib/core/safe_static.h"
#define TAG "DokanHost"
#include "slib/storage/file_system_internal.h"

#define DOKAN_CHECK_FLAG(val, flag) if (val & flag) { LOG_DEBUG("\t" #flag); }

#define DOKAN_ERROR_CODE(err) ((FileSystemError)(err) == FileSystemError::NotImplemented \
								? STATUS_NOT_IMPLEMENTED \
								: DokanNtStatusFromWin32((DWORD)err))

#define DOKANY_DRIVER_NAME "dokan1"

#pragma comment(lib, "dokany.lib")

extern "C"
{
	void InitializeDokany();
	void UninitializeDokany();
}

namespace slib
{

	namespace priv
	{
		namespace dokany
		{

			SLIB_INLINE static String NormalizePath(LPCWSTR szPath)
			{
				return String::from(szPath).replaceAll('\\', '/');
			}

			static sl_bool CheckDokanyOptions(const DOKAN_OPTIONS& options)
			{
				if (!Base::getStringLength2((sl_char16*)(options.MountPoint))) {
					LOG_ERROR("Mount Point required.");
					return sl_false;
				}
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
				return sl_true;
			};

			static sl_bool CheckPovider(FileSystemProvider* provider, DOKAN_OPTIONS& options)
			{
				FileSystemInfo info;
				if (provider->getInformation(info)) {
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
				return sl_false;
			}

			static int DOKAN_CALLBACK Dokany_CreateFile(
					LPCWSTR					szFileName,
					DWORD					dwAccessMode,
					DWORD					dwShareMode,
					DWORD					dwCreationDisposition,
					DWORD					dwFlagsAndAttributes,
					PDOKAN_FILE_INFO		pDokanFileInfo)
			{
				FileSystemHost* host = (FileSystemHost*)(pDokanFileInfo->DokanOptions->GlobalContext);
				FileSystemProvider* provider = host->getProvider();
				String path = NormalizePath(szFileName);

#if 0
				LOG_DEBUG("CreateFile : %s", path);

				//LOG_DEBUG("\tShareMode = 0x%x", dwShareMode);
				//DOKAN_CHECK_FLAG(dwShareMode, FILE_SHARE_READ);
				//DOKAN_CHECK_FLAG(dwShareMode, FILE_SHARE_WRITE);
				//DOKAN_CHECK_FLAG(dwShareMode, FILE_SHARE_DELETE);

				LOG_DEBUG(L"\tDesiredAccess = 0x%x", dwAccessMode);
				DOKAN_CHECK_FLAG(dwAccessMode, GENERIC_READ);
				DOKAN_CHECK_FLAG(dwAccessMode, GENERIC_WRITE);
				DOKAN_CHECK_FLAG(dwAccessMode, GENERIC_EXECUTE);
				DOKAN_CHECK_FLAG(dwAccessMode, DELETE);
				DOKAN_CHECK_FLAG(dwAccessMode, FILE_READ_DATA);
				DOKAN_CHECK_FLAG(dwAccessMode, FILE_READ_ATTRIBUTES);
				DOKAN_CHECK_FLAG(dwAccessMode, FILE_READ_EA);
				DOKAN_CHECK_FLAG(dwAccessMode, READ_CONTROL);
				DOKAN_CHECK_FLAG(dwAccessMode, FILE_WRITE_DATA);
				DOKAN_CHECK_FLAG(dwAccessMode, FILE_WRITE_ATTRIBUTES);
				DOKAN_CHECK_FLAG(dwAccessMode, FILE_WRITE_EA);
				DOKAN_CHECK_FLAG(dwAccessMode, FILE_APPEND_DATA);
				DOKAN_CHECK_FLAG(dwAccessMode, WRITE_DAC);
				DOKAN_CHECK_FLAG(dwAccessMode, WRITE_OWNER);
				DOKAN_CHECK_FLAG(dwAccessMode, SYNCHRONIZE);
				DOKAN_CHECK_FLAG(dwAccessMode, FILE_EXECUTE);
				//DOKAN_CHECK_FLAG(dwAccessMode, STANDARD_RIGHTS_READ);
				//DOKAN_CHECK_FLAG(dwAccessMode, STANDARD_RIGHTS_WRITE);
				//DOKAN_CHECK_FLAG(dwAccessMode, STANDARD_RIGHTS_EXECUTE);

				//LOG_DEBUG("\tFlagsAndAttributes = 0x%x", dwFlagsAndAttributes);
				//DOKAN_CHECK_FLAG(dwFlagsAndAttributes, FILE_FLAG_WRITE_THROUGH);
				//DOKAN_CHECK_FLAG(dwFlagsAndAttributes, FILE_FLAG_OVERLAPPED);
				//DOKAN_CHECK_FLAG(dwFlagsAndAttributes, FILE_FLAG_NO_BUFFERING);
				//DOKAN_CHECK_FLAG(dwFlagsAndAttributes, FILE_FLAG_RANDOM_ACCESS);
				//DOKAN_CHECK_FLAG(dwFlagsAndAttributes, FILE_FLAG_SEQUENTIAL_SCAN);
				//DOKAN_CHECK_FLAG(dwFlagsAndAttributes, FILE_FLAG_DELETE_ON_CLOSE);
				//DOKAN_CHECK_FLAG(dwFlagsAndAttributes, FILE_FLAG_BACKUP_SEMANTICS);
				//DOKAN_CHECK_FLAG(dwFlagsAndAttributes, FILE_FLAG_POSIX_SEMANTICS);
				//DOKAN_CHECK_FLAG(dwFlagsAndAttributes, FILE_FLAG_OPEN_REPARSE_POINT);
				//DOKAN_CHECK_FLAG(dwFlagsAndAttributes, FILE_FLAG_OPEN_NO_RECALL);
#endif

				int iRetSuccess = 0;
				if (dwCreationDisposition == CREATE_ALWAYS || dwCreationDisposition == OPEN_ALWAYS) {
					if (provider->existsFile(path)) {
						iRetSuccess = STATUS_OBJECT_NAME_COLLISION;
					}
				}

				FileOpenParam param;

				if (dwAccessMode & (GENERIC_READ | FILE_READ_DATA | FILE_READ_ATTRIBUTES | FILE_READ_EA)) {
					param.mode |= FileMode::Read;
					if (dwAccessMode & FILE_READ_DATA) {
						param.mode |= FileMode::ReadData;
					}
					if (dwAccessMode & FILE_READ_ATTRIBUTES) {
						param.mode |= FileMode::ReadAttrs;
					}
					if (dwAccessMode & FILE_READ_EA) {
						param.mode |= FileMode::ReadAttrs;
					}
				}
				if (dwAccessMode & (GENERIC_WRITE | FILE_WRITE_DATA | FILE_WRITE_ATTRIBUTES | FILE_WRITE_EA)) {
					param.mode |= FileMode::Write;
					if (dwAccessMode & FILE_WRITE_DATA) {
						param.mode |= FileMode::WriteData;
					}
					if (dwAccessMode & FILE_WRITE_ATTRIBUTES) {
						param.mode |= FileMode::WriteAttrs;
					}
					if (dwAccessMode & FILE_WRITE_EA) {
						param.mode |= FileMode::WriteAttrs;
					}
				}
				if (dwAccessMode & SYNCHRONIZE) {
					param.mode |= FileMode::Sync;
				}
				if (dwShareMode & FILE_SHARE_READ) {
					param.mode |= FileMode::ShareRead;
				}
				if (dwShareMode & FILE_SHARE_WRITE) {
					param.mode |= FileMode::ShareWrite;
				}
				if (dwShareMode & FILE_SHARE_DELETE) {
					param.mode |= FileMode::ShareDelete;
				}

				switch (dwCreationDisposition) {
				case CREATE_NEW:
					if (provider->existsFile(path)) {
						return DOKAN_ERROR_CODE(ERROR_FILE_EXISTS);
					}
					break;
				case CREATE_ALWAYS:
					break;
				case OPEN_ALWAYS:
					param.mode |= FileMode::NotTruncate;
					break;
				case OPEN_EXISTING:
					param.mode |= (FileMode::NotTruncate | FileMode::NotCreate);
					break;
				case TRUNCATE_EXISTING:
					param.mode |= FileMode::NotCreate;
					break;
				}

				param.attributes = (int)(dwFlagsAndAttributes & 0x7ffff);
				if (dwFlagsAndAttributes & FILE_DELETE_ON_CLOSE) {
					pDokanFileInfo->DeleteOnClose = TRUE;
				}
				if (dwFlagsAndAttributes & FILE_RANDOM_ACCESS) {
					param.mode |= FileMode::RandomAccess;
				}

				FileSystem::setLastError(FileSystemError::GeneralError);
				Ref<FileContext> context = provider->openFile(path, param);
				if (context.isNull()) {
					FileSystemError err = FileSystem::getLastError();
					if (err == FileSystemError::PathNotFound) {
						if (!(param.mode & FileMode::NotCreate)) {
							return STATUS_ACCESS_DENIED;
						}
					}
					return DOKAN_ERROR_CODE(err);
				}

				host->increaseOpenHandleCount();
				context->increaseReference();
				pDokanFileInfo->Context = (ULONG64)(sl_size)(context.get());

				return iRetSuccess;
			}

			// Only for Dokan
			static int DOKAN_CALLBACK Dokany_CreateDirectory(
				LPCWSTR					szFileName,
				PDOKAN_FILE_INFO		pDokanFileInfo)
			{
				FileSystemHost* host = (FileSystemHost*)(pDokanFileInfo->DokanOptions->GlobalContext);
				FileSystemProvider* provider = host->getProvider();

				FileSystem::setLastError(FileSystemError::GeneralError);
				if (provider->createDirectory(NormalizePath(szFileName))) {
					return 0;
				}
				return DOKAN_ERROR_CODE(FileSystem::getLastError());
			}

			// Only for Dokan
			static int DOKAN_CALLBACK Dokany_OpenDirectory(
				LPCWSTR					szFileName,
				PDOKAN_FILE_INFO		pDokanFileInfo)
			{
				FileSystemHost* host = (FileSystemHost*)(pDokanFileInfo->DokanOptions->GlobalContext);
				FileSystemProvider* provider = host->getProvider();

				if (provider->existsFile(NormalizePath(szFileName))) {
					return 0;
				}
				return DOKAN_ERROR_CODE(ERROR_PATH_NOT_FOUND);
			}

			static NTSTATUS DOKAN_CALLBACK Dokany_ZwCreateFile(
					LPCWSTR						szFileName,
					PDOKAN_IO_SECURITY_CONTEXT	pSecurityContext,
					ACCESS_MASK					dwDesiredAccess,
					ULONG						uFileAttributes,
					ULONG						uShareAccess,
					ULONG						uCreateDisposition,
					ULONG						uCreateOptions,
					PDOKAN_FILE_INFO			pDokanFileInfo)
			{
				FileSystemHost* host = (FileSystemHost*)(pDokanFileInfo->DokanOptions->GlobalContext);
				FileSystemProvider* provider = host->getProvider();

				DWORD dwCreationDisposition;
				DWORD dwFileAttributesAndFlags;
				ACCESS_MASK dwGenericDesiredAccess;

				DokanMapKernelToUserCreateFileFlags(
					dwDesiredAccess, uFileAttributes, uCreateOptions, uCreateDisposition,
					&dwGenericDesiredAccess, &dwFileAttributesAndFlags, &dwCreationDisposition);

				// When filePath is a directory, needs to change the flag so that the file can be opened.
				DWORD attrs = INVALID_FILE_ATTRIBUTES;
				FileInfo info;
				if (provider->getFileInfo(NormalizePath(szFileName), info, FileInfoMask::Attributes)) {
					if (!(info.attributes & FileAttributes::NotExist)) {
						attrs = info.attributes & 0x7ffff;
					}
				}
				if (attrs != INVALID_FILE_ATTRIBUTES) {
					if (attrs & FILE_ATTRIBUTE_DIRECTORY) {
						if (uCreateOptions & FILE_NON_DIRECTORY_FILE) {
							return STATUS_FILE_IS_A_DIRECTORY;
						}
						pDokanFileInfo->IsDirectory = TRUE;
						uShareAccess |= FILE_SHARE_READ;
					}
					if (pDokanFileInfo->IsDirectory) {
						if (!(attrs & FILE_ATTRIBUTE_DIRECTORY) && (uCreateOptions & FILE_DIRECTORY_FILE)) {
							return STATUS_NOT_A_DIRECTORY;
						}
						dwFileAttributesAndFlags |= FILE_ATTRIBUTE_DIRECTORY;
					} else {
						if (dwCreationDisposition == TRUNCATE_EXISTING) {
							dwGenericDesiredAccess |= GENERIC_WRITE;
						}
					}
				} else {
					if (pDokanFileInfo->IsDirectory) {
						dwFileAttributesAndFlags |= FILE_ATTRIBUTE_DIRECTORY;
					}
				}
				
				if (pDokanFileInfo->IsDirectory) {
					if (dwCreationDisposition == CREATE_NEW) {
						return Dokany_CreateDirectory(szFileName, pDokanFileInfo);
					} else {
						return Dokany_OpenDirectory(szFileName, pDokanFileInfo);
					}
				} else {
					return Dokany_CreateFile(szFileName,
						dwGenericDesiredAccess,
						uShareAccess,
						dwCreationDisposition,
						dwFileAttributesAndFlags,
						pDokanFileInfo);
				}
			}

			static void DOKAN_CALLBACK Dokany_CloseFile(
				LPCWSTR					szFileName,
				PDOKAN_FILE_INFO		pDokanFileInfo)
			{
				FileSystemHost* host = (FileSystemHost*)(pDokanFileInfo->DokanOptions->GlobalContext);
				FileSystemProvider* provider = host->getProvider();
				Ref<FileContext> context = (FileContext*)((sl_size)(pDokanFileInfo->Context));
				if (context.isNull()) {
					return;
				}

				FileSystem::setLastError(FileSystemError::Success);
				provider->closeFile(context);
				host->decreaseOpenHandleCount();
				context->decreaseReference();
				pDokanFileInfo->Context = 0;
			}

			static void DOKAN_CALLBACK Dokany_Cleanup(
					LPCWSTR					szFileName,
					PDOKAN_FILE_INFO		pDokanFileInfo)
			{
				FileSystemHost* host = (FileSystemHost*)(pDokanFileInfo->DokanOptions->GlobalContext);
				FileSystemProvider* provider = host->getProvider();
				Ref<FileContext> context = (FileContext*)((sl_size)(pDokanFileInfo->Context));

				if (pDokanFileInfo->DeleteOnClose) {
					FileSystem::setLastError(FileSystemError::GeneralError);
					if (pDokanFileInfo->IsDirectory) {
						provider->deleteDirectory(NormalizePath(szFileName));
					} else {
						if (context) {
							Dokany_CloseFile(szFileName, pDokanFileInfo);
						}
						provider->deleteFile(NormalizePath(szFileName));
					}
				}
			}

			static NTSTATUS DOKAN_CALLBACK Dokany_ReadFile(
					LPCWSTR				szFileName,
					LPVOID				pBuffer,
					DWORD				dwBufferLength,
					LPDWORD				pReadLength,
					LONGLONG			iOffset,
					PDOKAN_FILE_INFO	pDokanFileInfo)
			{
				FileSystemHost* host = (FileSystemHost*)(pDokanFileInfo->DokanOptions->GlobalContext);
				FileSystemProvider* provider = host->getProvider();
				Ref<FileContext> context = (FileContext*)((sl_size)(pDokanFileInfo->Context));
				if (context.isNull()) {
					return DOKAN_ERROR_CODE(ERROR_INVALID_HANDLE);
				}

				*pReadLength = 0;
				if (!dwBufferLength) {
					return 0;
				}

				FileSystem::setLastError(FileSystemError::GeneralError);
				*pReadLength = (DWORD)(provider->readFile(context, (sl_uint64)iOffset, pBuffer, dwBufferLength));
				if (*pReadLength) {
					return 0;
				}
				sl_uint64 size;
				if (provider->getFileSize(context, size)) {
					if ((sl_uint64)iOffset == size) {
						return 0;
					}
				}
				return DOKAN_ERROR_CODE(FileSystem::getLastError());
			}

			static NTSTATUS DOKAN_CALLBACK Dokany_WriteFile(
					LPCWSTR				szFileName,
					LPCVOID				pBuffer,
					DWORD				dwNumberOfBytesToWrite,
					LPDWORD				pNumberOfBytesWritten,
					LONGLONG			iOffset,
					PDOKAN_FILE_INFO	pDokanFileInfo)
			{
				FileSystemHost* host = (FileSystemHost*)(pDokanFileInfo->DokanOptions->GlobalContext);
				FileSystemProvider* provider = host->getProvider();
				Ref<FileContext> context = (FileContext*)((sl_size)(pDokanFileInfo->Context));
				if (context.isNull()) {
					return DOKAN_ERROR_CODE(ERROR_INVALID_HANDLE);
				}

				*pNumberOfBytesWritten = 0;
				if (pDokanFileInfo->WriteToEndOfFile) {
					iOffset = -1;
				} else {
					if (pDokanFileInfo->PagingIo) {
						sl_uint64 size;
						FileSystem::setLastError(FileSystemError::GeneralError);
						if (provider->getFileSize(context, size)) {
							if ((sl_uint64)iOffset >= size) {
								dwNumberOfBytesToWrite = 0;
							}
							if ((sl_uint64)(iOffset + dwNumberOfBytesToWrite) > size) {
								dwNumberOfBytesToWrite = (DWORD)((size - iOffset) & 0xFFFFFFFFUL);
							}
						} else {
							return DOKAN_ERROR_CODE(FileSystem::getLastError());
						}
					}
				}

				if (!dwNumberOfBytesToWrite) {
					return 0;
				}

				FileSystem::setLastError(FileSystemError::GeneralError);
				*pNumberOfBytesWritten = (DWORD)(provider->writeFile(context, (sl_int64)iOffset, pBuffer, dwNumberOfBytesToWrite));
				if (*pNumberOfBytesWritten) {
					return 0;
				}
				return DOKAN_ERROR_CODE(FileSystem::getLastError());
			}

			static NTSTATUS DOKAN_CALLBACK Dokany_FlushFileBuffers(
					LPCWSTR				szFileName,
					PDOKAN_FILE_INFO	pDokanFileInfo)
			{
				FileSystemHost* host = (FileSystemHost*)(pDokanFileInfo->DokanOptions->GlobalContext);
				FileSystemProvider* provider = host->getProvider();
				Ref<FileContext> context = (FileContext*)((sl_size)(pDokanFileInfo->Context));
				if (context.isNull()) {
					return 0;
				}

				FileSystem::setLastError(FileSystemError::GeneralError);
				if (provider->flushFile(context)) {
					return 0;
				}
				return DOKAN_ERROR_CODE(FileSystem::getLastError());
			}

			static NTSTATUS DOKAN_CALLBACK Dokany_GetFileInformation(
					LPCWSTR							szFileName,
					LPBY_HANDLE_FILE_INFORMATION	pFileInfo,
					PDOKAN_FILE_INFO				pDokanFileInfo)
			{
				FileSystemHost* host = (FileSystemHost*)(pDokanFileInfo->DokanOptions->GlobalContext);
				FileSystemProvider* provider = host->getProvider();
				Ref<FileContext> context = (FileContext*)((sl_size)(pDokanFileInfo->Context));
				if (context.isNull()) {
					context = provider->createContext(NormalizePath(szFileName));
				}

				FileInfo info;
				FileSystem::setLastError(FileSystemError::GeneralError);
				if (provider->getFileInfo(context, info, FileInfoMask::All)) {
					pFileInfo->dwFileAttributes = info.attributes & 0x7ffff;
					pFileInfo->nFileSizeLow = SLIB_GET_DWORD0(info.size);
					pFileInfo->nFileSizeHigh = SLIB_GET_DWORD1(info.size);
					((PLARGE_INTEGER)(&pFileInfo->ftCreationTime))->QuadPart = info.createdAt.toWindowsFileTime();
					((PLARGE_INTEGER)(&pFileInfo->ftLastAccessTime))->QuadPart = info.accessedAt.toWindowsFileTime();
					((PLARGE_INTEGER)(&pFileInfo->ftLastWriteTime))->QuadPart = info.modifiedAt.toWindowsFileTime();
					return 0;
				}
				return DOKAN_ERROR_CODE(FileSystem::getLastError());
			}

			static NTSTATUS DOKAN_CALLBACK Dokany_FindFiles(
					LPCWSTR				szPathName,
					PFillFindData		funcFillFindData,
					PDOKAN_FILE_INFO	pDokanFileInfo)
			{
				FileSystemHost* host = (FileSystemHost*)(pDokanFileInfo->DokanOptions->GlobalContext);
				FileSystemProvider* provider = host->getProvider();

				WIN32_FIND_DATAW fd;
				Base::zeroMemory(&fd, sizeof(fd));
				fd.dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
				FileInfo dirInfo;
				FileSystem::setLastError(FileSystemError::GeneralError);
				if (provider->getFileInfo(NormalizePath(szPathName), dirInfo, FileInfoMask::Time)) {
					((PLARGE_INTEGER)(&fd.ftCreationTime))->QuadPart = dirInfo.createdAt.toWindowsFileTime();
					((PLARGE_INTEGER)(&fd.ftLastAccessTime))->QuadPart = dirInfo.accessedAt.toWindowsFileTime();
					((PLARGE_INTEGER)(&fd.ftLastWriteTime))->QuadPart = dirInfo.modifiedAt.toWindowsFileTime();
				} else {
					return DOKAN_ERROR_CODE(FileSystem::getLastError());
				}
				fd.cFileName[0] = '.';
				funcFillFindData(&fd, pDokanFileInfo);
				fd.cFileName[1] = '.';
				funcFillFindData(&fd, pDokanFileInfo);
				HashMap<String, FileInfo> files = provider->getFiles(NormalizePath(szPathName));
				for (auto& item : files) {
					FileInfo& info = item.value;
					Base::zeroMemory(&fd, sizeof(fd));
					fd.dwFileAttributes = info.attributes & 0x7ffff;
					fd.nFileSizeLow = SLIB_GET_DWORD0(info.size);
					fd.nFileSizeHigh = SLIB_GET_DWORD1(info.size);
					((PLARGE_INTEGER)(&fd.ftCreationTime))->QuadPart = info.createdAt.toWindowsFileTime();
					((PLARGE_INTEGER)(&fd.ftLastAccessTime))->QuadPart = info.accessedAt.toWindowsFileTime();
					((PLARGE_INTEGER)(&fd.ftLastWriteTime))->QuadPart = info.modifiedAt.toWindowsFileTime();
					item.key.getUtf16((sl_char16*)(fd.cFileName), CountOfArray(fd.cFileName));
					funcFillFindData(&fd, pDokanFileInfo);
				}
				return 0;
			}

			static NTSTATUS DOKAN_CALLBACK Dokany_DeleteFile(
					LPCWSTR				szFileName,
					PDOKAN_FILE_INFO	pDokanFileInfo)
			{
				FileSystemHost* host = (FileSystemHost*)(pDokanFileInfo->DokanOptions->GlobalContext);
				FileSystemProvider* provider = host->getProvider();

				if (provider->existsFile(NormalizePath(szFileName))) {
					return 0;
				}
				return DOKAN_ERROR_CODE(ERROR_FILE_NOT_FOUND);
			}

			static NTSTATUS DOKAN_CALLBACK Dokany_DeleteDirectory (
					LPCWSTR				szFileName,
					PDOKAN_FILE_INFO	pDokanFileInfo)
			{
				FileSystemHost* host = (FileSystemHost*)(pDokanFileInfo->DokanOptions->GlobalContext);
				FileSystemProvider* provider = host->getProvider();

				if (provider->getFiles(NormalizePath(szFileName)).isEmpty()) {
					return 0;
				}
				return DOKAN_ERROR_CODE(ERROR_DIR_NOT_EMPTY);
			}

			static NTSTATUS DOKAN_CALLBACK Dokany_MoveFile(
					LPCWSTR				szFileName,
					LPCWSTR				szNewFileName,
					BOOL				bReplaceIfExisting,
					PDOKAN_FILE_INFO	pDokanFileInfo)
			{
				FileSystemHost* host = (FileSystemHost*)(pDokanFileInfo->DokanOptions->GlobalContext);
				FileSystemProvider* provider = host->getProvider();

				FileSystem::setLastError(FileSystemError::GeneralError);
				if (provider->moveFile(NormalizePath(szFileName), NormalizePath(szNewFileName), bReplaceIfExisting)) {
					return 0;
				}
				return DOKAN_ERROR_CODE(FileSystem::getLastError());
			}

			static NTSTATUS DOKAN_CALLBACK Dokany_SetEndOfFile(
					LPCWSTR				szFileName,
					LONGLONG			iOffset,
					PDOKAN_FILE_INFO	pDokanFileInfo)
			{
				FileSystemHost* host = (FileSystemHost*)(pDokanFileInfo->DokanOptions->GlobalContext);
				FileSystemProvider* provider = host->getProvider();
				Ref<FileContext> context = (FileContext*)((sl_size)(pDokanFileInfo->Context));
				if (context.isNull()) {
					context = provider->createContext(NormalizePath(szFileName));
				}

				FileInfo info;
				info.size = (sl_uint64)iOffset;
				FileSystem::setLastError(FileSystemError::GeneralError);
				if (provider->setFileInfo(context, info, FileInfoMask::Size)) {
					return 0;
				}
				return DOKAN_ERROR_CODE(FileSystem::getLastError());
			}

			static NTSTATUS DOKAN_CALLBACK Dokany_SetAllocationSize(
					LPCWSTR				szFileName,
					LONGLONG			iAllocSize,
					PDOKAN_FILE_INFO	pDokanFileInfo)
			{
				FileSystemHost* host = (FileSystemHost*)(pDokanFileInfo->DokanOptions->GlobalContext);
				FileSystemProvider* provider = host->getProvider();
				Ref<FileContext> context = (FileContext*)((sl_size)(pDokanFileInfo->Context));
				if (context.isNull()) {
					context = provider->createContext(NormalizePath(szFileName));
				}

				FileInfo info;
				info.allocSize = (sl_uint64)iAllocSize;
				FileSystem::setLastError(FileSystemError::GeneralError);
				if (provider->setFileInfo(context, info, FileInfoMask::AllocSize)) {
					return 0;
				}
				return DOKAN_ERROR_CODE(FileSystem::getLastError());
			}

			static NTSTATUS DOKAN_CALLBACK Dokany_SetFileAttributes(
					LPCWSTR				szFileName,
					DWORD				dwFileAttributes,
					PDOKAN_FILE_INFO	pDokanFileInfo)
			{
				FileSystemHost* host = (FileSystemHost*)(pDokanFileInfo->DokanOptions->GlobalContext);
				FileSystemProvider* provider = host->getProvider();
				Ref<FileContext> context = (FileContext*)((sl_size)(pDokanFileInfo->Context));
				if (context.isNull()) {
					context = provider->createContext(NormalizePath(szFileName));
				}

				FileInfo info;
				info.attributes = (int)(dwFileAttributes & 0x7ffff);
				FileSystem::setLastError(FileSystemError::GeneralError);
				if (provider->setFileInfo(context, info, FileInfoMask::Attributes)) {
					return 0;
				}
				return DOKAN_ERROR_CODE(FileSystem::getLastError());
			}

			static NTSTATUS DOKAN_CALLBACK Dokany_SetFileTime(
					LPCWSTR				szFileName,
					CONST FILETIME*		ftCreationTime,
					CONST FILETIME*		ftLastAccessTime,
					CONST FILETIME*		ftLastWriteTime,
					PDOKAN_FILE_INFO	pDokanFileInfo)
			{
				FileSystemHost* host = (FileSystemHost*)(pDokanFileInfo->DokanOptions->GlobalContext);
				FileSystemProvider* provider = host->getProvider();
				Ref<FileContext> context = (FileContext*)((sl_size)(pDokanFileInfo->Context));
				if (context.isNull()) {
					context = provider->createContext(NormalizePath(szFileName));
				}

				FileInfo info;
				if (ftCreationTime && ftCreationTime->dwLowDateTime && ftCreationTime->dwHighDateTime) {
					info.createdAt.setWindowsFileTime(*((sl_int64*)ftCreationTime));
				}
				if (ftLastAccessTime && ftLastAccessTime->dwLowDateTime && ftLastAccessTime->dwHighDateTime) {
					info.accessedAt.setWindowsFileTime(*((sl_int64*)ftLastAccessTime));
				}
				if (ftLastWriteTime && ftLastWriteTime->dwLowDateTime && ftLastWriteTime->dwHighDateTime) {
					info.modifiedAt.setWindowsFileTime(*((sl_int64*)ftLastWriteTime));
				}
				FileSystem::setLastError(FileSystemError::GeneralError);
				if (provider->setFileInfo(context, info, FileInfoMask::Time)) {
					return 0;
				}
				return DOKAN_ERROR_CODE(FileSystem::getLastError());
			}

			static NTSTATUS DOKAN_CALLBACK Dokany_GetDiskFreeSpace(
					PULONGLONG			pFreeBytesAvailable,
					PULONGLONG			pTotalNumberOfBytes,
					PULONGLONG			pTotalNumberOfFreeBytes,
					PDOKAN_FILE_INFO	pDokanFileInfo)
			{
				FileSystemHost* host = (FileSystemHost*)(pDokanFileInfo->DokanOptions->GlobalContext);
				FileSystemProvider* provider = host->getProvider();

				sl_uint64 totalSize, freeSize;
				FileSystem::setLastError(FileSystemError::GeneralError);
				if (provider->getSize(&totalSize, &freeSize)) {
					*pFreeBytesAvailable = freeSize;
					*pTotalNumberOfFreeBytes = freeSize;
					*pTotalNumberOfBytes = totalSize;
					return 0;
				}
				return DOKAN_ERROR_CODE(FileSystem::getLastError());
			}

			static NTSTATUS DOKAN_CALLBACK Dokany_GetVolumeInformation(
					LPWSTR				szVolumeNameBuffer,
					DWORD				dwVolumeNameSize,
					LPDWORD				pVolumeSerialNumber,
					LPDWORD				pMaximumComponentLength,
					LPDWORD				pFileSystemFlags,
					LPWSTR				szFileSystemNameBuffer,
					DWORD				dwFileSystemNameSize,
					PDOKAN_FILE_INFO	pDokanFileInfo)
			{
				FileSystemHost* host = (FileSystemHost*)(pDokanFileInfo->DokanOptions->GlobalContext);
				FileSystemProvider* provider = host->getProvider();

				FileSystemInfo info;
				FileSystem::setLastError(FileSystemError::GeneralError);
				if (provider->getInformation(info)) {
					if (dwVolumeNameSize) {
						szVolumeNameBuffer[0] = 0;
						info.volumeName.getUtf16((sl_char16*)szVolumeNameBuffer, (sl_size)dwVolumeNameSize);
					}
					*pVolumeSerialNumber = (DWORD)(info.serialNumber);
					*pMaximumComponentLength = (DWORD)(info.maxPathLength);
					*pFileSystemFlags = (DWORD)(info.flags);
					if (dwFileSystemNameSize) {
						szFileSystemNameBuffer[0] = 0;
						info.fileSystemName.getUtf16((sl_char16*)szFileSystemNameBuffer, (sl_size)dwFileSystemNameSize);
					}
					return 0;
				}
				return DOKAN_ERROR_CODE(FileSystem::getLastError());
			}

			class DokanyHostInitializer
			{
			public:
				DokanyHostInitializer()
				{
					InitializeDokany();
				}

				~DokanyHostInitializer()
				{
					UninitializeDokany();
				}

			};

			SLIB_SAFE_STATIC_GETTER(DokanyHostInitializer, GetDokanyHostInitializer)

			static void InitializeDokanyHost()
			{
				GetDokanyHostInitializer();
			}

			class DokanHost : public FileSystemHost
			{
			public:
				DokanHost() : m_iRet(0)
				{
				}

			public:
				sl_bool _run() override
				{
					InitializeDokanyHost();

					FileSystemHostParam& param = m_param;

					StringCstr16 mountPoint = param.mountPoint;

					DOKAN_OPTIONS options;
					Base::zeroMemory(&options, sizeof(options));

					options.Version = (USHORT)(DokanVersion());
					options.ThreadCount = (USHORT)(param.threadCount);
					options.UNCName = L"";
					options.MountPoint = (LPCWSTR)(mountPoint.getData());

					if (param.flags & FileSystemHostFlags::DebugMode) {
						options.Options |= DOKAN_OPTION_DEBUG;
					}
					if (param.flags & FileSystemHostFlags::UseStdErr) {
						options.Options |= DOKAN_OPTION_STDERR;
					}
					if (param.flags & FileSystemHostFlags::WriteProtect) {
						options.Options |= DOKAN_OPTION_WRITE_PROTECT;	// not supported in legacy dokan
					}
					if (param.flags & FileSystemHostFlags::MountAsRemovable) {
						options.Options |= DOKAN_OPTION_REMOVABLE;
					}
					if (param.flags & FileSystemHostFlags::MountAsNetworkDrive) {
						options.Options |= DOKAN_OPTION_NETWORK;
					}

					options.GlobalContext = (ULONG64)(void*)this;

					if (!(CheckDokanyOptions(options))) {
						m_strError = "Invalid dokan options.";
						return sl_false;
					}
					if (!(CheckPovider(param.provider.get(), options))) {
						m_strError = "Invalid provider.";
						return sl_false;
					}

					DOKAN_OPERATIONS op;
					Base::zeroMemory(&op, sizeof(op));
					op.ZwCreateFile = Dokany_ZwCreateFile;
					op.Cleanup = Dokany_Cleanup;
					op.CloseFile = Dokany_CloseFile;
					op.ReadFile = Dokany_ReadFile;
					op.WriteFile = Dokany_WriteFile;
					op.FlushFileBuffers = Dokany_FlushFileBuffers;
					op.GetFileInformation = Dokany_GetFileInformation;
					op.FindFiles = Dokany_FindFiles;
					op.SetFileAttributes = Dokany_SetFileAttributes;
					op.SetFileTime = Dokany_SetFileTime;
					op.DeleteFile = Dokany_DeleteFile;
					op.DeleteDirectory = Dokany_DeleteDirectory;
					op.MoveFile = Dokany_MoveFile;
					op.SetEndOfFile = Dokany_SetEndOfFile;
					op.SetAllocationSize = Dokany_SetAllocationSize;
					op.GetDiskFreeSpace = Dokany_GetDiskFreeSpace;
					op.GetVolumeInformation = Dokany_GetVolumeInformation;

					m_iRet = DokanMain(&options, &op);

					return (m_iRet == DOKAN_SUCCESS);
				}

				String getErrorMessage() override
				{
					if (m_strError.isNotEmpty()) {
						return m_strError;
					}
					switch (m_iRet) {
					case DOKAN_SUCCESS:
						SLIB_RETURN_STRING("Success");
					case DOKAN_ERROR:
						SLIB_RETURN_STRING("Drive mount error");
					case DOKAN_DRIVE_LETTER_ERROR:
						SLIB_RETURN_STRING("Bad drive letter");
					case DOKAN_DRIVER_INSTALL_ERROR:
						SLIB_RETURN_STRING("Can't install dokan driver");
					case DOKAN_START_ERROR:
						SLIB_RETURN_STRING("Driver tells something wrong");
					case DOKAN_MOUNT_ERROR:
						SLIB_RETURN_STRING("Can't assign a drive letter");
					case DOKAN_MOUNT_POINT_ERROR:
						SLIB_RETURN_STRING("Mount point error");
					case DOKAN_VERSION_ERROR:
						SLIB_RETURN_STRING("Driver version error");
					default:
						return String::format("Unknown error: %d", m_iRet);
					}
				}
				
			private:
				int m_iRet;
				String m_strError;

			};

		}
	}

	using namespace priv::dokany;


	ServiceState Dokany::getDriverState()
	{
		return ServiceManager::getState(DOKANY_DRIVER_NAME);
	}

	sl_bool Dokany::startDriver()
	{
		ServiceState state = getDriverState();
		if (state == ServiceState::None) {
			return sl_false;
		}
		if (state == ServiceState::Running) {
			return sl_true;
		}
		return ServiceManager::start(DOKANY_DRIVER_NAME);
	}

	sl_bool Dokany::stopDriver()
	{
		ServiceState state = getDriverState();
		if (state == ServiceState::None) {
			return sl_false;
		}
		if (state == ServiceState::Stopped) {
			return sl_true;
		}
		return ServiceManager::stop(DOKANY_DRIVER_NAME);
	}

	Ref<FileSystemHost> Dokany::createHost()
	{
		return new DokanHost;
	}

	sl_bool Dokany::unmount(const StringParam& _mountPoint)
	{
		InitializeDokanyHost();
		StringCstr16 mountPoint(_mountPoint);
		return DokanRemoveMountPoint((LPCWSTR)(mountPoint.getData()));
	}

}

#endif
