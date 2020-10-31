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

#define _EXPORTING
#include "dokany/dokan.h"

#include "slib/core/service_manager.h"
#include "slib/core/dynamic_library.h"
#include "slib/core/platform_windows.h"
#include "slib/core/safe_static.h"

#define TAG "DokanHost"
#include "slib/storage/file_system_internal.h"

#define DOKAN_CHECK_FLAG(val, flag) if (val & flag) { LOG_DEBUG("\t" #flag); }

#define DOKAN_ERROR_CODE(err) (g_flagDokany ? getApi_DokanNtStatusFromWin32()((DWORD)err) : -(int)err)

#define DOKANY_TRY SLIB_TRY
#define DOKANY_CATCH \
	SLIB_CATCH(FileSystemError err, { \
		if (err == FileSystemError::NotImplemented) { \
			return (g_flagDokany ? STATUS_NOT_IMPLEMENTED : 0); \
		} \
		return DOKAN_ERROR_CODE(err); \
	}) \
	SLIB_CATCH(..., { \
		return DOKAN_ERROR_CODE(FileSystemError::GeneralError); \
	})

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
				} SLIB_CATCH(...)
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

#if 0
				LOG_DEBUG("CreateFile : %s", StringCstr16(szFileName));

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
				DOKAN_CHECK_FLAG(dwAccessMode, STANDARD_RIGHTS_READ);
				DOKAN_CHECK_FLAG(dwAccessMode, STANDARD_RIGHTS_WRITE);
				DOKAN_CHECK_FLAG(dwAccessMode, STANDARD_RIGHTS_EXECUTE);

				LOG_DEBUG("\tFlagsAndAttributes = 0x%x", dwFlagsAndAttributes);
				DOKAN_CHECK_FLAG(dwFlagsAndAttributes, FILE_FLAG_WRITE_THROUGH);
				DOKAN_CHECK_FLAG(dwFlagsAndAttributes, FILE_FLAG_OVERLAPPED);
				DOKAN_CHECK_FLAG(dwFlagsAndAttributes, FILE_FLAG_NO_BUFFERING);
				DOKAN_CHECK_FLAG(dwFlagsAndAttributes, FILE_FLAG_RANDOM_ACCESS);
				DOKAN_CHECK_FLAG(dwFlagsAndAttributes, FILE_FLAG_SEQUENTIAL_SCAN);
				DOKAN_CHECK_FLAG(dwFlagsAndAttributes, FILE_FLAG_DELETE_ON_CLOSE);
				DOKAN_CHECK_FLAG(dwFlagsAndAttributes, FILE_FLAG_BACKUP_SEMANTICS);
				DOKAN_CHECK_FLAG(dwFlagsAndAttributes, FILE_FLAG_POSIX_SEMANTICS);
				DOKAN_CHECK_FLAG(dwFlagsAndAttributes, FILE_FLAG_OPEN_REPARSE_POINT);
				DOKAN_CHECK_FLAG(dwFlagsAndAttributes, FILE_FLAG_OPEN_NO_RECALL);
#endif

				DOKANY_TRY {

					int iRetSuccess = 0;
					if (dwCreationDisposition == CREATE_ALWAYS || dwCreationDisposition == OPEN_ALWAYS) {
						if (provider->existsFile(szFileName)) {
							if (g_flagDokany) {
								iRetSuccess = STATUS_OBJECT_NAME_COLLISION;
							} else {
								iRetSuccess = ERROR_ALREADY_EXISTS;
							}
						}
					}

					FileOpenParam param;

					if (dwAccessMode & (GENERIC_READ | FILE_READ_DATA | FILE_READ_ATTRIBUTES | FILE_READ_EA)) {
						param.mode |= FileMode::Read;
					}
					if (dwAccessMode & (GENERIC_WRITE | FILE_WRITE_DATA | FILE_WRITE_ATTRIBUTES | FILE_WRITE_EA)) {
						param.mode |= FileMode::Write;
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
						if (provider->existsFile(szFileName)) {
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

					Ref<FileContext> context = provider->openFile(szFileName, param);
					if (context.isNull()) {
						return DOKAN_ERROR_CODE(FileSystemError::GeneralError);
					}

					host->increaseOpenHandlesCount();
					context->increaseReference();
					pDokanFileInfo->Context = (ULONG64)(sl_size)(context.get());

					return iRetSuccess;

				} DOKANY_CATCH

			}

			// Only for Dokan
			static int DOKAN_CALLBACK Dokany_CreateDirectory(
				LPCWSTR					szFileName,
				PDOKAN_FILE_INFO		pDokanFileInfo)
			{
				FileSystemHost* host = (FileSystemHost*)(pDokanFileInfo->DokanOptions->GlobalContext);
				FileSystemProvider* provider = host->getProvider();

				DOKANY_TRY{
					if (provider->createDirectory(szFileName)) {
						return 0;
					} else {
						return DOKAN_ERROR_CODE(FileSystemError::GeneralError);
					}
				} DOKANY_CATCH
			}

			// Only for Dokan
			static int DOKAN_CALLBACK Dokany_OpenDirectory(
				LPCWSTR					szFileName,
				PDOKAN_FILE_INFO		pDokanFileInfo)
			{
				FileSystemHost* host = (FileSystemHost*)(pDokanFileInfo->DokanOptions->GlobalContext);
				FileSystemProvider* provider = host->getProvider();

				DOKANY_TRY{
					if (provider->existsFile(szFileName)) {
						return 0;
					} else {
						return DOKAN_ERROR_CODE(ERROR_NOT_FOUND);
					}
				} DOKANY_CATCH
			}

			// Only for Dokany
			static int DOKAN_CALLBACK Dokany_ZwCreateFile(
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

				getApi_DokanMapKernelToUserCreateFileFlags()(
					dwDesiredAccess, uFileAttributes, uCreateOptions, uCreateDisposition,
					&dwGenericDesiredAccess, &dwFileAttributesAndFlags, &dwCreationDisposition);

				// When filePath is a directory, needs to change the flag so that the file can be opened.
				DWORD attrs = INVALID_FILE_ATTRIBUTES;
				SLIB_TRY {
					FileInfo info;
					provider->getFileInfo(szFileName, sl_null, info, FileInfoMask::Attributes);
					if (!(info.attributes & FileAttributes::NotExist)) {
						attrs = info.attributes & 0x7ffff;
					}
				} SLIB_CATCH(...)
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

			static int DOKAN_CALLBACK Dokany_Cleanup(
					LPCWSTR					szFileName,
					PDOKAN_FILE_INFO		pDokanFileInfo)
			{
				FileSystemHost* host = (FileSystemHost*)(pDokanFileInfo->DokanOptions->GlobalContext);
				FileSystemProvider* provider = host->getProvider();
				FileContext* context = (FileContext*)((sl_size)(pDokanFileInfo->Context));

				SLIB_TRY {
					if (pDokanFileInfo->DeleteOnClose) {
						if (pDokanFileInfo->IsDirectory) {
							provider->deleteDirectory(szFileName);
						} else {
							if (context) {
								provider->closeFile(context);
								host->decreaseOpenHandlesCount();
								context->decreaseReference();
								pDokanFileInfo->Context = 0;
							}
							provider->deleteFile(szFileName);
						}
					}
				} SLIB_CATCH(...)
				return 0;
			}

			static int DOKAN_CALLBACK Dokany_CloseFile(
					LPCWSTR					szFileName,
					PDOKAN_FILE_INFO		pDokanFileInfo)
			{
				FileSystemHost* host = (FileSystemHost*)(pDokanFileInfo->DokanOptions->GlobalContext);
				FileSystemProvider* provider = host->getProvider();
				FileContext* context = (FileContext*)((sl_size)(pDokanFileInfo->Context));

				if (!context) {
					return 0;
				}

				SLIB_TRY {
					provider->closeFile(context);
					host->decreaseOpenHandlesCount();
					context->decreaseReference();
				} SLIB_CATCH(...)
				return 0;
			}

			static int DOKAN_CALLBACK Dokany_ReadFile(
					LPCWSTR				szFileName,
					LPVOID				pBuffer,
					DWORD				dwBufferLength,
					LPDWORD				pReadLength,
					LONGLONG			iOffset,
					PDOKAN_FILE_INFO	pDokanFileInfo)
			{
				FileSystemHost* host = (FileSystemHost*)(pDokanFileInfo->DokanOptions->GlobalContext);
				FileSystemProvider* provider = host->getProvider();
				FileContext* context = (FileContext*)((sl_size)(pDokanFileInfo->Context));

				if (!context) {
					return DOKAN_ERROR_CODE(ERROR_INVALID_HANDLE);
				}
				
				if (!dwBufferLength) {
					return 0;
				}

				DOKANY_TRY {
					*pReadLength = (DWORD)(provider->readFile(context, (sl_uint64)iOffset, pBuffer, dwBufferLength));
					if (*pReadLength) {
						return 0;
					}
					return DOKAN_ERROR_CODE(FileSystemError::GeneralError);
				} DOKANY_CATCH
			}

			static int DOKAN_CALLBACK Dokany_WriteFile(
					LPCWSTR				szFileName,
					LPCVOID				pBuffer,
					DWORD				dwNumberOfBytesToWrite,
					LPDWORD				pNumberOfBytesWritten,
					LONGLONG			iOffset,
					PDOKAN_FILE_INFO	pDokanFileInfo)
			{
				FileSystemHost* host = (FileSystemHost*)(pDokanFileInfo->DokanOptions->GlobalContext);
				FileSystemProvider* provider = host->getProvider();
				FileContext* context = (FileContext*)((sl_size)(pDokanFileInfo->Context));

				if (!context) {
					return DOKAN_ERROR_CODE(ERROR_INVALID_HANDLE);
				}

				if (pDokanFileInfo->WriteToEndOfFile) {
					iOffset = -1;
				} else {
					if (pDokanFileInfo->PagingIo) {
						sl_uint64 size = provider->getFileSize(context);
						if ((sl_uint64)iOffset >= size) {
							*pNumberOfBytesWritten = 0;
							return 0;
						}
						if ((sl_uint64)(iOffset + dwNumberOfBytesToWrite) > size) {
							dwNumberOfBytesToWrite = (DWORD)((size - iOffset) & 0xFFFFFFFFUL);
						}
					}
				}

				if (!dwNumberOfBytesToWrite) {
					return 0;
				}

				DOKANY_TRY {
					*pNumberOfBytesWritten = (DWORD)(provider->writeFile(context, (sl_int64)iOffset, pBuffer, dwNumberOfBytesToWrite));
					if (*pNumberOfBytesWritten) {
						return 0;
					}
					return DOKAN_ERROR_CODE(FileSystemError::GeneralError);
				} DOKANY_CATCH
			}

			static int DOKAN_CALLBACK Dokany_FlushFileBuffers(
					LPCWSTR				szFileName,
					PDOKAN_FILE_INFO	pDokanFileInfo)
			{
				FileSystemHost* host = (FileSystemHost*)(pDokanFileInfo->DokanOptions->GlobalContext);
				FileSystemProvider* provider = host->getProvider();
				FileContext* context = (FileContext*)((sl_size)(pDokanFileInfo->Context));

				if (!context) {
					return 0;
				}

				DOKANY_TRY {
					if (provider->flushFile(context)) {
						return 0;
					}
					return DOKAN_ERROR_CODE(FileSystemError::GeneralError);
				} DOKANY_CATCH
			}

			static int DOKAN_CALLBACK Dokany_GetFileInformation(
					LPCWSTR							szFileName,
					LPBY_HANDLE_FILE_INFORMATION	pFileInfo,
					PDOKAN_FILE_INFO				pDokanFileInfo)
			{
				FileSystemHost* host = (FileSystemHost*)(pDokanFileInfo->DokanOptions->GlobalContext);
				FileSystemProvider* provider = host->getProvider();
				FileContext* context = (FileContext*)((sl_size)(pDokanFileInfo->Context));

				DOKANY_TRY {
					FileInfo info;
					if (provider->getFileInfo(szFileName, context, info, FileInfoMask::All)) {
						pFileInfo->dwFileAttributes = info.attributes & 0x7ffff;
						pFileInfo->nFileSizeLow = SLIB_GET_DWORD0(info.size);
						pFileInfo->nFileSizeHigh = SLIB_GET_DWORD1(info.size);
						((PLARGE_INTEGER)(&pFileInfo->ftCreationTime))->QuadPart = info.createdAt.toWindowsFileTime();
						((PLARGE_INTEGER)(&pFileInfo->ftLastAccessTime))->QuadPart = info.accessedAt.toWindowsFileTime();
						((PLARGE_INTEGER)(&pFileInfo->ftLastWriteTime))->QuadPart = info.modifiedAt.toWindowsFileTime();
						return 0;
					}
					return DOKAN_ERROR_CODE(FileSystemError::GeneralError);
				} DOKANY_CATCH
			}

			static int DOKAN_CALLBACK Dokany_FindFiles(
					LPCWSTR				szPathName,
					PFillFindData		funcFillFindData,
					PDOKAN_FILE_INFO	pDokanFileInfo)
			{
				FileSystemHost* host = (FileSystemHost*)(pDokanFileInfo->DokanOptions->GlobalContext);
				FileSystemProvider* provider = host->getProvider();

				DOKANY_TRY {
					WIN32_FIND_DATAW fd;
					Base::zeroMemory(&fd, sizeof(fd));
					fd.dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
					fd.cFileName[0] = '.';
					funcFillFindData(&fd, pDokanFileInfo);
					fd.cFileName[1] = '.';
					funcFillFindData(&fd, pDokanFileInfo);
					HashMap<String, FileInfo> files = provider->getFiles(szPathName);
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
				} DOKANY_CATCH
			}

			static int DOKAN_CALLBACK Dokany_DeleteFile(
					LPCWSTR				szFileName,
					PDOKAN_FILE_INFO	pDokanFileInfo)
			{
				FileSystemHost* host = (FileSystemHost*)(pDokanFileInfo->DokanOptions->GlobalContext);
				FileSystemProvider* provider = host->getProvider();

				DOKANY_TRY {
					if (provider->existsFile(szFileName)) {
						return 0;
					}
					return DOKAN_ERROR_CODE(ERROR_NOT_FOUND);
				} DOKANY_CATCH
			}

			static int DOKAN_CALLBACK Dokany_DeleteDirectory (
					LPCWSTR				szFileName,
					PDOKAN_FILE_INFO	pDokanFileInfo)
			{
				FileSystemHost* host = (FileSystemHost*)(pDokanFileInfo->DokanOptions->GlobalContext);
				FileSystemProvider* provider = host->getProvider();

				DOKANY_TRY {
					if (provider->getFiles(szFileName).isEmpty()) {
						return 0;
					}
					return DOKAN_ERROR_CODE(ERROR_DIR_NOT_EMPTY);
				} DOKANY_CATCH
			}

			static int DOKAN_CALLBACK Dokany_MoveFile(
					LPCWSTR				szFileName,
					LPCWSTR				szNewFileName,
					BOOL				bReplaceIfExisting,
					PDOKAN_FILE_INFO	pDokanFileInfo)
			{
				FileSystemHost* host = (FileSystemHost*)(pDokanFileInfo->DokanOptions->GlobalContext);
				FileSystemProvider* provider = host->getProvider();

				DOKANY_TRY {
					if (provider->moveFile(szFileName, szNewFileName, bReplaceIfExisting)) {
						return 0;
					}
					return DOKAN_ERROR_CODE(FileSystemError::GeneralError);
				} DOKANY_CATCH
			}

			static int DOKAN_CALLBACK Dokany_SetEndOfFile(
					LPCWSTR				szFileName,
					LONGLONG			iOffset,
					PDOKAN_FILE_INFO	pDokanFileInfo)
			{
				FileSystemHost* host = (FileSystemHost*)(pDokanFileInfo->DokanOptions->GlobalContext);
				FileSystemProvider* provider = host->getProvider();
				FileContext* context = (FileContext*)((sl_size)(pDokanFileInfo->Context));

				DOKANY_TRY {
					FileInfo info;
				info.size = (sl_uint64)iOffset;
					if (provider->setFileInfo(szFileName, context, info, FileInfoMask::Size)) {
						return 0;
					}
					return DOKAN_ERROR_CODE(FileSystemError::GeneralError);
				} DOKANY_CATCH
			}

			static int DOKAN_CALLBACK Dokany_SetAllocationSize(
					LPCWSTR				szFileName,
					LONGLONG			iAllocSize,
					PDOKAN_FILE_INFO	pDokanFileInfo)
			{
				FileSystemHost* host = (FileSystemHost*)(pDokanFileInfo->DokanOptions->GlobalContext);
				FileSystemProvider* provider = host->getProvider();
				FileContext* context = (FileContext*)((sl_size)(pDokanFileInfo->Context));

				DOKANY_TRY{
					FileInfo info;
					info.allocSize = (sl_uint64)iAllocSize;
					if (provider->setFileInfo(szFileName, context, info, FileInfoMask::AllocSize)) {
						return 0;
					}
					return DOKAN_ERROR_CODE(FileSystemError::GeneralError);
				} DOKANY_CATCH
			}

			static int DOKAN_CALLBACK Dokany_SetFileAttributes(
					LPCWSTR				szFileName,
					DWORD				dwFileAttributes,
					PDOKAN_FILE_INFO	pDokanFileInfo)
			{
				FileSystemHost* host = (FileSystemHost*)(pDokanFileInfo->DokanOptions->GlobalContext);
				FileSystemProvider* provider = host->getProvider();
				FileContext* context = (FileContext*)((sl_size)(pDokanFileInfo->Context));

				DOKANY_TRY{
					FileInfo info;
					info.attributes = (int)(dwFileAttributes & 0x7ffff);
					if (provider->setFileInfo(szFileName, context, info, FileInfoMask::Attributes)) {
						return 0;
					}
					return DOKAN_ERROR_CODE(FileSystemError::GeneralError);
				} DOKANY_CATCH
			}

			static int DOKAN_CALLBACK Dokany_SetFileTime(
					LPCWSTR				szFileName,
					CONST FILETIME*		ftCreationTime,
					CONST FILETIME*		ftLastAccessTime,
					CONST FILETIME*		ftLastWriteTime,
					PDOKAN_FILE_INFO	pDokanFileInfo)
			{
				FileSystemHost* host = (FileSystemHost*)(pDokanFileInfo->DokanOptions->GlobalContext);
				FileSystemProvider* provider = host->getProvider();
				FileContext* context = (FileContext*)((sl_size)(pDokanFileInfo->Context));

				DOKANY_TRY{
					FileInfo info;
					info.createdAt.setWindowsFileTime(*((sl_int64*)ftCreationTime));
					info.accessedAt.setWindowsFileTime(*((sl_int64*)ftLastAccessTime));
					info.modifiedAt.setWindowsFileTime(*((sl_int64*)ftLastWriteTime));
					if (provider->setFileInfo(szFileName, context, info, FileInfoMask::Time)) {
						return 0;
					}
					return DOKAN_ERROR_CODE(FileSystemError::GeneralError);
				} DOKANY_CATCH
			}

			static int DOKAN_CALLBACK Dokany_GetDiskFreeSpace(
					PULONGLONG			pFreeBytesAvailable,
					PULONGLONG			pTotalNumberOfBytes,
					PULONGLONG			pTotalNumberOfFreeBytes,
					PDOKAN_FILE_INFO	pDokanFileInfo)
			{
				FileSystemHost* host = (FileSystemHost*)(pDokanFileInfo->DokanOptions->GlobalContext);
				FileSystemProvider* provider = host->getProvider();

				DOKANY_TRY{
					sl_uint64 freeSize, totalSize;
					if (provider->getSize(&totalSize, &freeSize)) {
						*pFreeBytesAvailable = freeSize;
						*pTotalNumberOfFreeBytes = freeSize;
						*pTotalNumberOfBytes = totalSize;
						return 0;
					}
					return DOKAN_ERROR_CODE(FileSystemError::GeneralError);
				} DOKANY_CATCH
			}

			static int DOKAN_CALLBACK Dokany_GetVolumeInformation(
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

				DOKANY_TRY{
					FileSystemInfo info;
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
					return DOKAN_ERROR_CODE(FileSystemError::GeneralError);
				} DOKANY_CATCH
			}

			static void* GetDokanOperations()
			{
				if (g_flagDokany) {
					static void* op[] = {
						Dokany_ZwCreateFile,
						Dokany_Cleanup,
						Dokany_CloseFile,
						Dokany_ReadFile,
						Dokany_WriteFile,
						Dokany_FlushFileBuffers,
						Dokany_GetFileInformation,
						Dokany_FindFiles,
						NULL, // FindFilesWithPattern
						Dokany_SetFileAttributes,
						Dokany_SetFileTime,
						Dokany_DeleteFile,
						Dokany_DeleteDirectory,
						Dokany_MoveFile,
						Dokany_SetEndOfFile,
						Dokany_SetAllocationSize,
						NULL, // LockFile
						NULL, // UnlockFile
						Dokany_GetDiskFreeSpace,
						Dokany_GetVolumeInformation,
						NULL, // Mounted
						NULL, // Unmounted
						NULL, // GetFileSecurity
						NULL, // SetFileSecurity
						NULL // FindStreams
					};
					return &op;
				} else {
					static void* op[] = {
						Dokany_CreateFile,
						Dokany_OpenDirectory,
						Dokany_CreateDirectory,
						Dokany_Cleanup,
						Dokany_CloseFile,
						Dokany_ReadFile,
						Dokany_WriteFile,
						Dokany_FlushFileBuffers,
						Dokany_GetFileInformation,
						Dokany_FindFiles,
						NULL, // FindFilesWithPattern
						Dokany_SetFileAttributes,
						Dokany_SetFileTime,
						Dokany_DeleteFile,
						Dokany_DeleteDirectory,
						Dokany_MoveFile,
						Dokany_SetEndOfFile,
						Dokany_SetAllocationSize,
						NULL, // LockFile
						NULL, // UnlockFile
						Dokany_GetDiskFreeSpace,
						Dokany_GetVolumeInformation,
						NULL, // Unmounted
						NULL, // GetFileSecurity
						NULL // SetFileSecurity
					};
					return &op;
				}
			}

			class DokanHost : public FileSystemHost
			{
			public:
				DokanHost()
				{
					m_iStatus = 0;
				}

			public:
				sl_bool _run() override
				{
					if (!Dokany::initialize()) {
						return sl_false;
					}

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

					m_iStatus = funcMain(&options, (PDOKAN_OPERATIONS)GetDokanOperations());

					return (m_iStatus == DOKAN_SUCCESS);
				}

				String getErrorMessage() override
				{
					switch (m_iStatus) {
					case DOKAN_SUCCESS:
						return "Success";
					case DOKAN_ERROR:
						return "Drive mount error";
					case DOKAN_DRIVE_LETTER_ERROR:
						return "Bad drive letter";
					case DOKAN_DRIVER_INSTALL_ERROR:
						return "Can't install dokan driver";
					case DOKAN_START_ERROR:
						return "Driver tells something wrong";
					case DOKAN_MOUNT_ERROR:
						return "Can't assign a drive letter";
					case DOKAN_MOUNT_POINT_ERROR:
						return "Mount point error";
					case DOKAN_VERSION_ERROR:
						return "Driver version error";
					default:
						return String::format("Unknown error: %d", m_iStatus);
					}
				}
				
			private:
				int m_iStatus;

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
