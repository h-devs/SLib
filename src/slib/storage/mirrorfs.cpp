#include <windows.h>
#include "slib/storage/mirrorfs.h"

#define ALLOCATION_UNIT                 4096

#define StringToWChar(STR, OUT)			OUT[STR.getUtf16((sl_char16 *)(OUT), sizeof(OUT))] = L'\0'
#define WCharToString(WCSTR)			String::fromUtf16((const sl_char16 *)(WCSTR))
#define FileTimeToTime(ft, t)			{ \
	SYSTEMTIME st = { 0 }; \
	FileTimeToSystemTime(&(ft), &st); \
	t.setUTC(st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds); \
}
#define TimeToFileTime(t, ft)			{ \
	TimeComponents time; \
	t.getUTC(time); \
	SYSTEMTIME st = { (WORD)time.year, (WORD)time.month, (WORD)time.dayOfWeek, (WORD)time.day, \
		(WORD)time.hour, (WORD)time.minute, (WORD)time.second, (WORD)time.milliseconds }; \
	SystemTimeToFileTime(&st, &(ft)); \
}
#define TimeToFileTimeLI(t, li)			TimeToFileTime(t, *((FILETIME*)&(li)))

#define ConcatPath(FN, FP)              StringToWChar(String::format("%s%s", m_path, FN), FP)
#define HandleFromContext(FD)           ((HANDLE)(FD->handle))

#define SLIB_IMPLEMENT_DYNAMIC_LIBRARY
#include "slib/core/dl.h"

namespace slib
{

	/* Windows XP Compatible */
	SLIB_IMPORT_LIBRARY_BEGIN(api, "kernel32.dll")
		SLIB_IMPORT_LIBRARY_FUNCTION(
			SetFileInformationByHandle,
			BOOL, WINAPI,
			HANDLE hFile,
			FILE_INFO_BY_HANDLE_CLASS FileInformationClass,
			LPVOID lpFileInformation,
			DWORD dwBufferSize
		)
		SLIB_IMPORT_LIBRARY_FUNCTION(
			FindFirstStreamW,
			HANDLE, WINAPI,
			LPCWSTR lpFileName,
			STREAM_INFO_LEVELS InfoLevel,
			LPVOID lpFindStreamData,
			DWORD dwFlags
		)
		SLIB_IMPORT_LIBRARY_FUNCTION(
			FindNextStreamW,
			BOOL, APIENTRY,
			HANDLE hFindStream,
			LPVOID lpFindStreamData
		)
	SLIB_IMPORT_LIBRARY_END

	MirrorFs::MirrorFs(String path) : m_path(path)
	{
		WCHAR fullPath[MAX_PATH];
		WCHAR root[MAX_PATH];
		HANDLE handle;
		WCHAR volumeName[MAX_PATH];
		FILETIME creationTime;

		StringToWChar(path, fullPath);

		if (!GetVolumePathName(fullPath, root, MAX_PATH))
			throw FileSystemError::InitFailure;
			
		handle = CreateFileW(
			fullPath, FILE_READ_ATTRIBUTES, 0, 0,
			OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0);
		if (INVALID_HANDLE_VALUE == handle) {
			throw FileSystemError::InitFailure;
		}

		if (!GetFileTime(handle, &creationTime, 0, 0))
		{
			CloseHandle(handle);
			throw FileSystemError::InitFailure;
		}

		CloseHandle(handle);

		FileTimeToTime(creationTime, m_volumeInfo.creationTime);
		m_volumeInfo.serialNumber = 0;
		m_volumeInfo.sectorSize = 4096;
		m_volumeInfo.sectorsPerAllocationUnit = 1;
		m_volumeInfo.maxComponentLength = 256;
		m_volumeInfo.fileSystemFlags = FILE_CASE_SENSITIVE_SEARCH |
			FILE_CASE_PRESERVED_NAMES |
			FILE_SUPPORTS_REMOTE_STORAGE |
			FILE_UNICODE_ON_DISK |
			FILE_PERSISTENT_ACLS;

		if (!GetVolumeInformation(root,
			volumeName, MAX_PATH,
			(LPDWORD)&m_volumeInfo.serialNumber, (LPDWORD)&m_volumeInfo.maxComponentLength,
			0, 0, 0)) {
			//throw FileSystemError::InitFailure;
		}

		m_volumeInfo.volumeName = WCharToString(volumeName);
		m_volumeInfo.fileSystemName = "MirrorFs";

		m_root = WCharToString(root);
	}

	MirrorFs::~MirrorFs()
	{
	}

	const VolumeInfo& MirrorFs::fsGetVolumeInfo(VolumeInfoFlags flags)&
	{
		if (flags == VolumeInfoFlags::SizeInfo) {
			WCHAR path[MAX_PATH];
			ULARGE_INTEGER totalSize, freeSize;

			StringToWChar(m_path, path);

			if (!GetDiskFreeSpaceEx(path, 0, &totalSize, &freeSize))
				throw getError();

			m_volumeInfo.totalSize = totalSize.QuadPart;
			m_volumeInfo.freeSize = freeSize.QuadPart;
		}
		return m_volumeInfo;
	}

	void MirrorFs::fsSetVolumeName(String volumeName)
	{
		throw FileSystemError::NotImplemented;
	}
	
	void MirrorFs::fsCreate(FileContext* context, FileCreationParams& params)
	{
		WCHAR fullPath[MAX_PATH];
		DWORD creationDisposition = CREATE_NEW;
		LPSECURITY_ATTRIBUTES pSecurityAttributes = NULL;
		SECURITY_ATTRIBUTES securityAttributes;
		HANDLE handle;

		ConcatPath(context->path, fullPath);

		if (0 == params.accessMode)
			params.accessMode = GENERIC_READ | GENERIC_WRITE;
		if (0 == params.shareMode)
			params.shareMode = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
		if (params.createAlways) {
			creationDisposition = CREATE_ALWAYS;
			params.createAlways = FALSE;
			params.openTruncate = FALSE;
		}

		params.flagsAndAttributes |= FILE_FLAG_BACKUP_SEMANTICS;

		if (params.attr.isDirectory)
		{
			/*
			* It is not widely known but CreateFileW can be used to create directories!
			* It requires the specification of both FILE_FLAG_BACKUP_SEMANTICS and
			* FILE_FLAG_POSIX_SEMANTICS. It also requires that FileAttributes has
			* FILE_ATTRIBUTE_DIRECTORY set.
			*/
			params.flagsAndAttributes |= FILE_FLAG_POSIX_SEMANTICS;
			params.flagsAndAttributes |= FILE_ATTRIBUTE_DIRECTORY;
		}
		else {
			params.flagsAndAttributes &= ~FILE_ATTRIBUTE_DIRECTORY;
		}

		if (0 == (params.flagsAndAttributes & 0x0007FFFF))
			params.flagsAndAttributes |= FILE_ATTRIBUTE_NORMAL;

		if (params.securityDescriptor)
		{
			securityAttributes.nLength = sizeof securityAttributes;
			securityAttributes.lpSecurityDescriptor = params.securityDescriptor;
			securityAttributes.bInheritHandle = FALSE;
			pSecurityAttributes = &securityAttributes;
		}

		handle = CreateFile(fullPath,
			params.accessMode, params.shareMode, pSecurityAttributes,
			creationDisposition, params.flagsAndAttributes, 0);

		if (INVALID_HANDLE_VALUE == handle)
			throw getError();

		context->status = getError();
		context->handle = (sl_uint64)handle;
		context->isDirectory = params.attr.isDirectory;
	}

	void MirrorFs::fsOpen(FileContext* context, FileCreationParams& params)
	{
		WCHAR fullPath[MAX_PATH];
		DWORD creationDisposition = OPEN_EXISTING;
		LPSECURITY_ATTRIBUTES pSecurityAttributes = NULL;
		SECURITY_ATTRIBUTES securityAttributes;
		HANDLE handle;

		ConcatPath(context->path, fullPath);

		if (0 == params.accessMode)
			params.accessMode = GENERIC_READ | GENERIC_WRITE;
		if (0 == params.shareMode)
			params.shareMode = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
		if (params.createAlways) {
			creationDisposition = OPEN_ALWAYS;
			if (params.openTruncate)
				creationDisposition = CREATE_ALWAYS;	// truncate
			params.createAlways = FALSE;
			params.openTruncate = FALSE;
		}
		else if (params.openTruncate) {
			creationDisposition = TRUNCATE_EXISTING;
			params.openTruncate = FALSE;
		}

		params.flagsAndAttributes |= FILE_FLAG_BACKUP_SEMANTICS;

		if (0 == (params.flagsAndAttributes & 0x0007FFFF))
			params.flagsAndAttributes |= FILE_ATTRIBUTE_NORMAL;

		if (params.securityDescriptor)
		{
			securityAttributes.nLength = sizeof securityAttributes;
			securityAttributes.lpSecurityDescriptor = params.securityDescriptor;
			securityAttributes.bInheritHandle = FALSE;
			pSecurityAttributes = &securityAttributes;
		}

		handle = CreateFile(fullPath,
			params.accessMode, params.shareMode, pSecurityAttributes,
			creationDisposition, params.flagsAndAttributes, 0);

		if (INVALID_HANDLE_VALUE == handle)
			throw getError();

		context->status = getError();
		context->handle = (sl_uint64)handle;
		context->isDirectory = params.attr.isDirectory;
	}

	void MirrorFs::fsClose(FileContext* context)
	{
		if (context->handle && !CloseHandle(HandleFromContext(context)))
			throw getError();
		context->handle = 0;
	}

	sl_size MirrorFs::fsRead(FileContext* context, const Memory& buffer, sl_uint64 offset)
	{
		DWORD bytesTransferred = 0;
		HANDLE handle = HandleFromContext(context);
		WCHAR fullPath[MAX_PATH];
		ConcatPath(context->path, fullPath);

		BOOL opened = FALSE;
		if (!handle || handle == INVALID_HANDLE_VALUE) {
			handle = CreateFile(fullPath, GENERIC_READ, FILE_SHARE_READ, NULL,
				OPEN_EXISTING, 0, NULL);
			if (handle == INVALID_HANDLE_VALUE) {
				throw getError();
			}
			else {
				opened = TRUE;
			}
		}

		LARGE_INTEGER li;
		li.QuadPart = offset;
		if (!SetFilePointerEx(handle, li, NULL, FILE_BEGIN)) {
			if (opened)
				CloseHandle(handle);
			throw getError();
		}

		if (!ReadFile(handle, buffer.getData(), (DWORD)buffer.getSize(), &bytesTransferred, NULL)) {
			if (opened)
				CloseHandle(handle);
			throw getError();
		}

		if (opened)
			CloseHandle(handle);
		return bytesTransferred;
	}

	sl_size MirrorFs::fsWrite(FileContext* context, const Memory& buffer, sl_uint64 offset, sl_bool writeToEof)
	{
		DWORD bytesTransferred = 0;
		HANDLE handle = HandleFromContext(context);
		WCHAR fullPath[MAX_PATH];
		ConcatPath(context->path, fullPath);

		BOOL opened = FALSE;
		if (!handle || handle == INVALID_HANDLE_VALUE) {
			handle = CreateFile(fullPath, GENERIC_WRITE, FILE_SHARE_WRITE, NULL,
				OPEN_EXISTING, 0, NULL);
			if (handle == INVALID_HANDLE_VALUE) {
				throw getError();
			}
			else {
				opened = TRUE;
			}
		}

		LARGE_INTEGER li;
		if (writeToEof) {
			li.QuadPart = 0;
			if (!SetFilePointerEx(handle, li, NULL, FILE_END)) {
				if (opened)
					CloseHandle(handle);
				throw getError();
			}
		}
		else {
			li.QuadPart = offset;
			if (!SetFilePointerEx(handle, li, NULL, FILE_BEGIN)) {
				if (opened)
					CloseHandle(handle);
				throw getError();
			}
		}

		if (!WriteFile(handle, buffer.getData(), (DWORD)buffer.getSize(), &bytesTransferred, NULL)) {
			if (opened)
				CloseHandle(handle);
			throw getError();
		}

		if (opened)
			CloseHandle(handle);
		return bytesTransferred;
	}

	void MirrorFs::fsFlush(FileContext* context)
	{
		HANDLE handle = HandleFromContext(context);

		if (!handle || handle == INVALID_HANDLE_VALUE) {
			return;
		}

		if (!FlushFileBuffers(handle))
			throw getError();
	}

	void MirrorFs::fsDelete(FileContext* context, sl_bool checkOnly)
	{
		HANDLE handle = HandleFromContext(context);
		WCHAR fullPath[MAX_PATH];
		ZeroMemory(fullPath, sizeof(fullPath));

		ConcatPath(context->path, fullPath);

		DWORD fileAttr = GetFileAttributes(fullPath);
		if (INVALID_FILE_ATTRIBUTES == fileAttr)
			throw getError();

		if (checkOnly) {
			if (fileAttr && fileAttr & FILE_ATTRIBUTE_DIRECTORY) {
				HANDLE hFind;
				WIN32_FIND_DATAW findData;

				ULONG fileLen = (ULONG)wcslen(fullPath);
				if (fullPath[fileLen - 1] != L'\\') {
					fullPath[fileLen++] = L'\\';
				}
				fullPath[fileLen] = L'*';

				hFind = FindFirstFile(fullPath, &findData);
				while (hFind != INVALID_HANDLE_VALUE) {
					if (wcscmp(findData.cFileName, L"..") != 0 &&
						wcscmp(findData.cFileName, L".") != 0) {
						FindClose(hFind);
						throw getError(ERROR_DIR_NOT_EMPTY);
					}
					if (!FindNextFile(hFind, &findData)) {
						break;
					}
				}
				FindClose(hFind);

				if (GetLastError() != ERROR_NO_MORE_FILES)
					throw getError();

				// TODO check if directory can be deletable
			}
			else {
				auto funcSetFileInformationByHandle = api::getApi_SetFileInformationByHandle();
				if (funcSetFileInformationByHandle) {
					FILE_DISPOSITION_INFO dispositionInfo;
					dispositionInfo.DeleteFile = TRUE;

					if (!funcSetFileInformationByHandle(handle,
						FileDispositionInfo, &dispositionInfo, sizeof dispositionInfo))
						throw getError();

					dispositionInfo.DeleteFile = FALSE;
					funcSetFileInformationByHandle(handle,
						FileDispositionInfo, &dispositionInfo, sizeof dispositionInfo);
				}
				// TODO else
			}
		}
		else {
			if (fileAttr && fileAttr & FILE_ATTRIBUTE_DIRECTORY) {
				if (!RemoveDirectory(fullPath))
					throw getError();
			}
			else {
				if (!DeleteFile(fullPath))
					throw getError();
			}
		}
	}

	void MirrorFs::fsRename(FileContext* context, String newFileName, sl_bool replaceIfExists)
	{
		WCHAR fullPath[MAX_PATH], NewfullPath[MAX_PATH];

		ConcatPath(context->path, fullPath);
		ConcatPath(newFileName, NewfullPath);

		if (!MoveFileEx(fullPath, NewfullPath, replaceIfExists ? MOVEFILE_REPLACE_EXISTING : 0))
			throw getError();
	}

	void MirrorFs::fsLock(FileContext* context, sl_uint64 offset, sl_uint64 length)
	{
		HANDLE handle = HandleFromContext(context);
		if (!handle || handle == INVALID_HANDLE_VALUE) {
			throw FileSystemError::InvalidContext;
		}

		LARGE_INTEGER liOffset;
		LARGE_INTEGER liLength;
		liOffset.QuadPart = offset;
		liLength.QuadPart = length;

		if (!LockFile(handle, 
			liOffset.HighPart, liOffset.LowPart,
			liLength.HighPart, liLength.LowPart)) {
			throw getError();
		}
	}

	void MirrorFs::fsUnlock(FileContext* context, sl_uint64 offset, sl_uint64 length)
	{
		HANDLE handle = HandleFromContext(context);
		if (!handle || handle == INVALID_HANDLE_VALUE) {
			throw FileSystemError::InvalidContext;
		}

		LARGE_INTEGER liOffset;
		LARGE_INTEGER liLength;
		liOffset.QuadPart = offset;
		liLength.QuadPart = length;

		if (!UnlockFile(handle,
			liOffset.HighPart, liOffset.LowPart,
			liLength.HighPart, liLength.LowPart)) {
			throw getError();
		}
	}

	FileInfo MirrorFs::fsGetFileInfo(FileContext* context)
	{
		HANDLE handle = HandleFromContext(context);
		WCHAR fullPath[MAX_PATH];
		ConcatPath(context->path, fullPath);

		BOOL opened = FALSE;
		if (!handle || handle == INVALID_HANDLE_VALUE) {
			handle = CreateFile(fullPath, GENERIC_READ, FILE_SHARE_READ, NULL,
				OPEN_EXISTING, 0, NULL);
			if (handle == INVALID_HANDLE_VALUE) {
				throw getError();
			}
			else {
				opened = TRUE;
			}
		}

		FileInfo fileInfo;
		BY_HANDLE_FILE_INFORMATION byHandleFileInfo;

		if (!GetFileInformationByHandle(handle, &byHandleFileInfo)) {
			if (context->path.getLength() == 1) {
				fileInfo.fileAttributes = GetFileAttributes(fullPath);
			}
			else {
				WIN32_FIND_DATAW find;
				ZeroMemory(&find, sizeof(WIN32_FIND_DATAW));
				HANDLE findHandle = FindFirstFile(fullPath, &find);
				if (findHandle == INVALID_HANDLE_VALUE) {
					if (opened)
						CloseHandle(handle);
					throw getError();
				}

				fileInfo.fileAttributes = find.dwFileAttributes;
				fileInfo.size =
					((UINT64)find.nFileSizeHigh << 32) | (UINT64)find.nFileSizeLow;
				fileInfo.allocationSize = (fileInfo.size + ALLOCATION_UNIT - 1)
					/ ALLOCATION_UNIT * ALLOCATION_UNIT;
				FileTimeToTime(find.ftCreationTime, fileInfo.createdAt);
				FileTimeToTime(find.ftLastAccessTime, fileInfo.lastAccessedAt);
				FileTimeToTime(find.ftLastWriteTime, fileInfo.modifiedAt);

				FindClose(findHandle);
			}
		}
		else {
			fileInfo.fileAttributes = byHandleFileInfo.dwFileAttributes;
			fileInfo.size =
				((UINT64)byHandleFileInfo.nFileSizeHigh << 32) | (UINT64)byHandleFileInfo.nFileSizeLow;
			fileInfo.allocationSize = (fileInfo.size + ALLOCATION_UNIT - 1)
				/ ALLOCATION_UNIT * ALLOCATION_UNIT;
			FileTimeToTime(byHandleFileInfo.ftCreationTime, fileInfo.createdAt);
			FileTimeToTime(byHandleFileInfo.ftLastAccessTime, fileInfo.lastAccessedAt);
			FileTimeToTime(byHandleFileInfo.ftLastWriteTime, fileInfo.modifiedAt);
		}

		if (opened)
			CloseHandle(handle);
		return fileInfo;
	}

	void MirrorFs::fsSetFileInfo(FileContext* context, FileInfo fileInfo, FileInfoFlags flags)
	{
		HANDLE handle = HandleFromContext(context);
		auto funcSetFileInformationByHandle = api::getApi_SetFileInformationByHandle();

		if (flags & FileInfoFlags::AttrAndTimeInfo) {
			if (funcSetFileInformationByHandle) {
				FILE_BASIC_INFO basicInfo = { 0 };
				FileInfo orgFileInfo = fsGetFileInfo(context);

				TimeToFileTimeLI(orgFileInfo.createdAt, basicInfo.CreationTime);
				TimeToFileTimeLI(orgFileInfo.lastAccessedAt, basicInfo.LastAccessTime);
				TimeToFileTimeLI(orgFileInfo.modifiedAt, basicInfo.LastWriteTime);
				TimeToFileTimeLI(orgFileInfo.modifiedAt, basicInfo.ChangeTime);
				basicInfo.FileAttributes = 0;	// This means not to change original attribute

				if (flags & FileInfoFlags::AttrInfo) {
					if (INVALID_FILE_ATTRIBUTES == fileInfo.fileAttributes)
						fileInfo.fileAttributes = 0;
					//else if (0 == fileInfo.fileAttributes)
					//	fileInfo.fileAttributes = FILE_ATTRIBUTE_NORMAL;

					basicInfo.FileAttributes = fileInfo.fileAttributes;
				}

				if (flags & FileInfoFlags::TimeInfo) {
					if (!fileInfo.createdAt.isZero())
						TimeToFileTimeLI(fileInfo.createdAt, basicInfo.CreationTime);
					if (!fileInfo.lastAccessedAt.isZero())
						TimeToFileTimeLI(fileInfo.lastAccessedAt, basicInfo.LastAccessTime);
					if (!fileInfo.modifiedAt.isZero()) {
						TimeToFileTimeLI(fileInfo.modifiedAt, basicInfo.LastWriteTime);
						TimeToFileTimeLI(fileInfo.modifiedAt, basicInfo.ChangeTime);
					}
				}

				if (!funcSetFileInformationByHandle(handle,
					FileBasicInfo, &basicInfo, sizeof basicInfo))
					throw getError();
			}
			else {
				if (flags & FileInfoFlags::AttrInfo) {
					WCHAR fullPath[MAX_PATH];
					ConcatPath(context->path, fullPath);
					if (!SetFileAttributes(fullPath, fileInfo.fileAttributes))
						throw getError();
				}

				if (flags & FileInfoFlags::TimeInfo) {
					FILETIME ctime, atime, mtime;
					TimeToFileTime(fileInfo.createdAt, ctime);
					TimeToFileTime(fileInfo.lastAccessedAt, atime);
					TimeToFileTime(fileInfo.modifiedAt, mtime);
					if (!SetFileTime(handle,
						fileInfo.createdAt.isZero() ? NULL : &ctime,
						fileInfo.lastAccessedAt.isZero() ? NULL : &atime,
						fileInfo.modifiedAt.isZero() ? NULL : &mtime))
						throw getError();
				}
			}
		}

		if (flags & FileInfoFlags::SizeInfo) {
			if (funcSetFileInformationByHandle) {
				FILE_END_OF_FILE_INFO endOfFileInfo;
				endOfFileInfo.EndOfFile.QuadPart = fileInfo.size;

				if (!funcSetFileInformationByHandle(handle,
					FileEndOfFileInfo, &endOfFileInfo, sizeof endOfFileInfo))
					throw getError();
			}
			else {
				LARGE_INTEGER offset;
				offset.QuadPart = fileInfo.size;

				if (!SetFilePointerEx(handle, offset, NULL, FILE_BEGIN))
					throw getError();
				if (!SetEndOfFile(handle))
					throw getError();
			}
		}

		if (flags & FileInfoFlags::AllocSizeInfo) {
			if (funcSetFileInformationByHandle) {
				FILE_ALLOCATION_INFO allocationInfo;
				allocationInfo.AllocationSize.QuadPart = fileInfo.allocationSize;

				if (!funcSetFileInformationByHandle(handle,
					FileAllocationInfo, &allocationInfo, sizeof allocationInfo))
					throw getError();
			}
			else {
				LARGE_INTEGER fileSize;
				LARGE_INTEGER offset;
				if (!GetFileSizeEx(handle, &fileSize))
					throw getError();

				if (fileInfo.allocationSize < (sl_uint64)fileSize.QuadPart) {
					offset.QuadPart = fileInfo.allocationSize;
					if (!SetFilePointerEx(handle, offset, NULL, FILE_BEGIN))
						throw getError();
					if (!SetEndOfFile(handle))
						throw getError();
				}
			}
		}
	}

	sl_size MirrorFs::fsGetSecurity(FileContext* context, sl_uint32 securityInformation, const Memory& securityDescriptor)
	{
		DWORD lengthNeeded;
		WCHAR fullPath[MAX_PATH];
		ConcatPath(context->path, fullPath);

		BOOL requestingSaclInfo = (
			(securityInformation & 0x00000008L/*SACL_SECURITY_INFORMATION*/) ||
			(securityInformation & 0x00010000L/*BACKUP_SECURITY_INFORMATION*/));

		HANDLE handle = CreateFile(
			fullPath,
			READ_CONTROL | (requestingSaclInfo ? ACCESS_SYSTEM_SECURITY : 0),
			FILE_SHARE_WRITE | FILE_SHARE_READ | FILE_SHARE_DELETE,
			NULL, // security attribute
			OPEN_EXISTING,
			FILE_FLAG_BACKUP_SEMANTICS, // |FILE_FLAG_NO_BUFFERING,
			NULL);

		if (!handle || handle == INVALID_HANDLE_VALUE)
			throw getError();

		if (!GetUserObjectSecurity(handle,
			(PSECURITY_INFORMATION)&securityInformation,
			(PSECURITY_DESCRIPTOR)securityDescriptor.getData(),
			(DWORD)securityDescriptor.getSize(),
			&lengthNeeded)) {
			if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
				CloseHandle(handle);
				return lengthNeeded;
			}
			else {
				CloseHandle(handle);
				throw getError();
			}
		}

		CloseHandle(handle);
		return GetSecurityDescriptorLength((PSECURITY_DESCRIPTOR)securityDescriptor.getData());
	}

	void MirrorFs::fsSetSecurity(FileContext* context, sl_uint32 securityInformation, const Memory& securityDescriptor)
	{
		if (!SetUserObjectSecurity(HandleFromContext(context),
			(PSECURITY_INFORMATION)&securityInformation,
			(PSECURITY_DESCRIPTOR)securityDescriptor.getData()))
			throw getError();
	}

	HashMap<String, FileInfo> MirrorFs::fsFindFiles(FileContext* context, String patternString)
	{
		WCHAR fullPath[MAX_PATH];
		WCHAR pattern[MAX_PATH];
		ULONG length, patternLength;
		HANDLE findHandle;
		WIN32_FIND_DATA findData;
		HashMap<String, FileInfo> files;

		if (patternString.isNull() || patternString.isEmpty())
			patternString = "*";

		ConcatPath(context->path, fullPath);
		length = (ULONG)wcslen(fullPath);

		StringToWChar(patternString, pattern);
		patternLength = (ULONG)wcslen(pattern);

		if (length + 1 + patternLength >= MAX_PATH)
			throw getError(ERROR_INVALID_NAME);

		if (L'\\' != fullPath[length - 1])
			fullPath[length++] = L'\\';
		memcpy(fullPath + length, pattern, patternLength * sizeof(WCHAR));
		fullPath[length + patternLength] = L'\0';

		findHandle = FindFirstFile(fullPath, &findData);
		if (INVALID_HANDLE_VALUE == findHandle)
			throw getError();

		BOOLEAN rootFolder = context->path.getLength() == 1;
		do {
			String fileName = WCharToString(findData.cFileName);
			FileInfo fileInfo;
			if (rootFolder && (fileName == "." || fileName == ".."))
				continue;

			fileInfo.fileAttributes = findData.dwFileAttributes;
			fileInfo.size =
				((UINT64)findData.nFileSizeHigh << 32) | (UINT64)findData.nFileSizeLow;
			fileInfo.allocationSize = (fileInfo.size + ALLOCATION_UNIT - 1)
				/ ALLOCATION_UNIT * ALLOCATION_UNIT;
			FileTimeToTime(findData.ftCreationTime, fileInfo.createdAt);
			FileTimeToTime(findData.ftLastAccessTime, fileInfo.lastAccessedAt);
			FileTimeToTime(findData.ftLastWriteTime, fileInfo.modifiedAt);

			files.add(fileName, fileInfo);
		} while (FindNextFile(findHandle, &findData) != 0);
			
		FindClose(findHandle);

		if (GetLastError() != ERROR_NO_MORE_FILES)
			throw getError();

		return files;
	}

	HashMap<String, StreamInfo> MirrorFs::fsFindStreams(FileContext* context)
	{
		auto funcFindFirstStream = api::getApi_FindFirstStreamW();
		auto funcFindNextStream = api::getApi_FindNextStreamW();
		if (!funcFindFirstStream || !funcFindNextStream)
			throw FileSystemError::NotImplemented;

		WCHAR fullPath[MAX_PATH];
		ULONG length;
		HANDLE findHandle;
		WIN32_FIND_STREAM_DATA findData;
		HashMap<String, StreamInfo> streams;

		ConcatPath(context->path, fullPath);
		length = (ULONG)wcslen(fullPath);

		findHandle = funcFindFirstStream(fullPath, FindStreamInfoStandard, &findData, 0);
		if (INVALID_HANDLE_VALUE == findHandle)
			throw getError();

		do {
			String streamName = WCharToString(findData.cStreamName);
			StreamInfo streamInfo;
			streamInfo.size = findData.StreamSize.QuadPart;
			streams.add(streamName, streamInfo);
		} while (funcFindNextStream(findHandle, &findData) != 0);

		FindClose(findHandle);

		if (GetLastError() != ERROR_HANDLE_EOF)
			throw getError();

		return streams;
	}

	FileSystemError MirrorFs::getError(sl_uint32 error)
	{
		return (FileSystemError)(error == 0 ? GetLastError() : error);
	}

}