/**
 * @file slib/storage/dokan_host.h
 * Dokan FileSystem Host Layer.
 *
 * @copyright 2020 Steve Han
 */

#pragma once

#define _EXPORTING
#include "../../../external/include/dokany/dokan.h"

#define	SLIB_DOKAN_RET	int

namespace slib
{

	class DokanHost : public FileSystemHost
	{
	public:
		DokanHost(Ref<FileSystemProvider> base, sl_uint32 options = 0);
		virtual ~DokanHost();

		void setVersion(sl_uint16 version);
		void setThreadCount(sl_uint16 threadCount);
		void setMountPoint(const StringParam& mountPoint);
		void setUNCName(const StringParam& uncName);
		void setTimeout(sl_uint32 timeout);
		void setDebugMode(sl_bool flagUseStdErr);

	public:
		int fsRun() override;
		int fsStop() override;
		int isRunning() override;

	public:
		static BOOL g_hasSeSecurityPrivilege;
		static BOOL addSeSecurityNamePrivilege();

	private:
		static SLIB_DOKAN_RET DOKAN_CALLBACK	// SLIB_DOKAN_IS_DOKANY
			ZwCreateFile(
				LPCWSTR					FileName,
				PDOKAN_IO_SECURITY_CONTEXT SecurityContext,
				ACCESS_MASK				DesiredAccess,
				ULONG					FileAttributes,
				ULONG					ShareAccess,
				ULONG					CreateDisposition,
				ULONG					CreateOptions,
				PDOKAN_FILE_INFO		DokanFileInfo);

		static SLIB_DOKAN_RET DOKAN_CALLBACK	// !SLIB_DOKAN_IS_DOKANY
			CreateFile(
				LPCWSTR					FileName,
				DWORD					AccessMode,
				DWORD					ShareMode,
				DWORD					CreationDisposition,
				DWORD					FlagsAndAttributes,
				PDOKAN_FILE_INFO		DokanFileInfo);

		static SLIB_DOKAN_RET DOKAN_CALLBACK	// !SLIB_DOKAN_IS_DOKANY
			CreateDirectory(
				LPCWSTR					FileName,
				PDOKAN_FILE_INFO		DokanFileInfo);

		static SLIB_DOKAN_RET DOKAN_CALLBACK	// !SLIB_DOKAN_IS_DOKANY
			OpenDirectory(
				LPCWSTR					FileName,
				PDOKAN_FILE_INFO		DokanFileInfo);

		static SLIB_DOKAN_RET DOKAN_CALLBACK
			Cleanup(
				LPCWSTR					FileName,
				PDOKAN_FILE_INFO		DokanFileInfo);

		static SLIB_DOKAN_RET DOKAN_CALLBACK
			CloseFile(
				LPCWSTR					FileName,
				PDOKAN_FILE_INFO		DokanFileInfo);
	
		static SLIB_DOKAN_RET DOKAN_CALLBACK
			ReadFile(
				LPCWSTR				FileName,
				LPVOID				Buffer,
				DWORD				BufferLength,
				LPDWORD				ReadLength,
				LONGLONG			Offset,
				PDOKAN_FILE_INFO	DokanFileInfo);

		static SLIB_DOKAN_RET DOKAN_CALLBACK
			WriteFile(
				LPCWSTR				FileName,
				LPCVOID				Buffer,
				DWORD				NumberOfBytesToWrite,
				LPDWORD				NumberOfBytesWritten,
				LONGLONG			Offset,
				PDOKAN_FILE_INFO	DokanFileInfo);

		static SLIB_DOKAN_RET DOKAN_CALLBACK
			FlushFileBuffers(
				LPCWSTR		FileName,
				PDOKAN_FILE_INFO	DokanFileInfo);

		static SLIB_DOKAN_RET DOKAN_CALLBACK
			GetFileInformation(
				LPCWSTR							FileName,
				LPBY_HANDLE_FILE_INFORMATION	HandleFileInformation,
				PDOKAN_FILE_INFO				DokanFileInfo);

		static SLIB_DOKAN_RET DOKAN_CALLBACK
			FindFiles(
				LPCWSTR				PathName,
				PFillFindData		FillFindData, // function pointer
				PDOKAN_FILE_INFO	DokanFileInfo);

		static SLIB_DOKAN_RET DOKAN_CALLBACK
			FindFilesWithPattern(
				LPCWSTR				PathName,
				LPCWSTR				SearchPattern,
				PFillFindData		FillFindData, // function pointer
				PDOKAN_FILE_INFO	DokanFileInfo);

		static SLIB_DOKAN_RET DOKAN_CALLBACK	// SLIB_DOKAN_IS_DOKANY
			FindStreams(
				LPCWSTR				FileName,
				PFillFindStreamData	FillFindStreamData,
				PDOKAN_FILE_INFO	DokanFileInfo);

		static SLIB_DOKAN_RET DOKAN_CALLBACK
			DeleteFile(
				LPCWSTR				FileName,
				PDOKAN_FILE_INFO	DokanFileInfo);

		static SLIB_DOKAN_RET DOKAN_CALLBACK
			DeleteDirectory(
				LPCWSTR				FileName,
				PDOKAN_FILE_INFO	DokanFileInfo);

		static SLIB_DOKAN_RET DOKAN_CALLBACK
			MoveFile(
				LPCWSTR				FileName, // existing file name
				LPCWSTR				NewFileName,
				BOOL				ReplaceIfExisting,
				PDOKAN_FILE_INFO	DokanFileInfo);

		static SLIB_DOKAN_RET DOKAN_CALLBACK
			SetEndOfFile(
				LPCWSTR				FileName,
				LONGLONG			ByteOffset,
				PDOKAN_FILE_INFO	DokanFileInfo);

		static SLIB_DOKAN_RET DOKAN_CALLBACK
			SetAllocationSize(
				LPCWSTR				FileName,
				LONGLONG			AllocSize,
				PDOKAN_FILE_INFO	DokanFileInfo);

		static SLIB_DOKAN_RET DOKAN_CALLBACK
			SetFileAttributes(
				LPCWSTR				FileName,
				DWORD				FileAttributes,
				PDOKAN_FILE_INFO	DokanFileInfo);

		static SLIB_DOKAN_RET DOKAN_CALLBACK
			SetFileTime(
				LPCWSTR				FileName,
				CONST FILETIME*		CreationTime,
				CONST FILETIME*		LastAccessTime,
				CONST FILETIME*		LastWriteTime,
				PDOKAN_FILE_INFO	DokanFileInfo);

		static SLIB_DOKAN_RET DOKAN_CALLBACK
			GetFileSecurity(
				LPCWSTR					FileName,
				PSECURITY_INFORMATION	SecurityInformation,
				PSECURITY_DESCRIPTOR	SecurityDescriptor,
				ULONG				BufferLength,
				PULONG				LengthNeeded,
				PDOKAN_FILE_INFO	DokanFileInfo);

		static SLIB_DOKAN_RET DOKAN_CALLBACK
			SetFileSecurity(
				LPCWSTR					FileName,
				PSECURITY_INFORMATION	SecurityInformation,
				PSECURITY_DESCRIPTOR	SecurityDescriptor,
				ULONG				SecurityDescriptorLength,
				PDOKAN_FILE_INFO	DokanFileInfo);

		static SLIB_DOKAN_RET DOKAN_CALLBACK
			LockFile(
				LPCWSTR				FileName,
				LONGLONG			ByteOffset,
				LONGLONG			Length,
				PDOKAN_FILE_INFO	DokanFileInfo);

		static SLIB_DOKAN_RET DOKAN_CALLBACK
			UnlockFile(
				LPCWSTR				FileName,
				LONGLONG			ByteOffset,
				LONGLONG			Length,
				PDOKAN_FILE_INFO	DokanFileInfo);

		static SLIB_DOKAN_RET DOKAN_CALLBACK
			GetDiskFreeSpace(
				PULONGLONG			FreeBytesAvailable,
				PULONGLONG			TotalNumberOfBytes,
				PULONGLONG			TotalNumberOfFreeBytes,
				PDOKAN_FILE_INFO	DokanFileInfo);

		static SLIB_DOKAN_RET DOKAN_CALLBACK
			GetVolumeInformation(
				LPWSTR		VolumeNameBuffer,
				DWORD		VolumeNameSize,
				LPDWORD		VolumeSerialNumber,
				LPDWORD		MaximumComponentLength,
				LPDWORD		FileSystemFlags,
				LPWSTR		FileSystemNameBuffer,
				DWORD		FileSystemNameSize,
				PDOKAN_FILE_INFO	DokanFileInfo);

		static SLIB_DOKAN_RET DOKAN_CALLBACK	// SLIB_DOKAN_IS_DOKANY
			Mounted(
				PDOKAN_FILE_INFO	DokanFileInfo);

		static SLIB_DOKAN_RET DOKAN_CALLBACK
			Unmounted(
				PDOKAN_FILE_INFO	DokanFileInfo);

	public:
		static void* Interface();

	private:
		DOKAN_OPTIONS m_dokanOptions;
		WCHAR m_mountPoint[MAX_PATH];
		WCHAR m_uncName[MAX_PATH];
		BOOL m_flagStarted;
	};

}