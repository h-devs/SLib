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

#define ConcatPath(FN, FP)              StringToWChar(String::format("%s%s", _Path, FN), FP)
#define HandleFromContext(FD)           ((HANDLE)(FD->handle))

namespace slib
{

	MirrorFs::MirrorFs(String Path) : _Path(Path)
	{
		WCHAR FullPath[MAX_PATH];
		WCHAR Root[MAX_PATH];
		HANDLE Handle;
		WCHAR VolumeName[MAX_PATH];
		FILETIME CreationTime;

		StringToWChar(Path, FullPath);

		if (!GetVolumePathName(FullPath, Root, MAX_PATH))
			throw FileSystemError::InitFailure;
			
		Handle = CreateFileW(
			FullPath, FILE_READ_ATTRIBUTES, 0, 0,
			OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0);
		if (INVALID_HANDLE_VALUE == Handle) {
			throw FileSystemError::InitFailure;
		}

		if (!GetFileTime(Handle, &CreationTime, 0, 0))
		{
			CloseHandle(Handle);
			throw FileSystemError::InitFailure;
		}

		CloseHandle(Handle);

		FileTimeToTime(CreationTime, _VolumeInfo.creationTime);
		_VolumeInfo.serialNumber = 0;
		_VolumeInfo.sectorSize = 4096;
		_VolumeInfo.sectorsPerAllocationUnit = 1;
		_VolumeInfo.maxComponentLength = 256;
		_VolumeInfo.fileSystemFlags = FILE_CASE_SENSITIVE_SEARCH |
			FILE_CASE_PRESERVED_NAMES |
			FILE_SUPPORTS_REMOTE_STORAGE |
			FILE_UNICODE_ON_DISK |
			FILE_PERSISTENT_ACLS;

		if (!GetVolumeInformation(Root,
			VolumeName, MAX_PATH,
			(LPDWORD)&_VolumeInfo.serialNumber, (LPDWORD)&_VolumeInfo.maxComponentLength,
			0, 0, 0)) {
			//throw FileSystemError::InitFailure;
		}

		_VolumeInfo.volumeName = WCharToString(VolumeName);
		_VolumeInfo.fileSystemName = "MirrorFs";

		_Root = WCharToString(Root);
	}

	MirrorFs::~MirrorFs()
	{
	}

	const VolumeInfo& MirrorFs::fsGetVolumeInfo(VolumeInfoFlags Flags)&
	{
		if (Flags == VolumeInfoFlags::SizeInfo) {
			WCHAR Path[MAX_PATH];
			ULARGE_INTEGER TotalSize, FreeSize;

			StringToWChar(_Path, Path);

			if (!GetDiskFreeSpaceEx(Path, 0, &TotalSize, &FreeSize))
				throw getError();

			_VolumeInfo.totalSize = TotalSize.QuadPart;
			_VolumeInfo.freeSize = FreeSize.QuadPart;
		}
		return _VolumeInfo;
	}

	void MirrorFs::fsSetVolumeName(String VolumeName)
	{
		throw FileSystemError::NotImplemented;
	}
	
	void MirrorFs::fsCreate(FileContext* Context, FileCreationParams& Parameters)
	{
		WCHAR FullPath[MAX_PATH];
		DWORD CreationDisposition = CREATE_NEW;
		LPSECURITY_ATTRIBUTES PSecurityAttributes = NULL;
		SECURITY_ATTRIBUTES SecurityAttributes;
		HANDLE Handle;

		ConcatPath(Context->path, FullPath);

		if (0 == Parameters.accessMode)
			Parameters.accessMode = GENERIC_READ | GENERIC_WRITE;
		if (0 == Parameters.shareMode)
			Parameters.shareMode = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
		if (Parameters.createAlways) {
			CreationDisposition = CREATE_ALWAYS;
			Parameters.createAlways = FALSE;
			Parameters.openTruncate = FALSE;
		}

		Parameters.flagsAndAttributes |= FILE_FLAG_BACKUP_SEMANTICS;

		if (Parameters.attr.isDirectory)
		{
			/*
			* It is not widely known but CreateFileW can be used to create directories!
			* It requires the specification of both FILE_FLAG_BACKUP_SEMANTICS and
			* FILE_FLAG_POSIX_SEMANTICS. It also requires that FileAttributes has
			* FILE_ATTRIBUTE_DIRECTORY set.
			*/
			Parameters.flagsAndAttributes |= FILE_FLAG_POSIX_SEMANTICS;
			Parameters.flagsAndAttributes |= FILE_ATTRIBUTE_DIRECTORY;
		}
		else {
			Parameters.flagsAndAttributes &= ~FILE_ATTRIBUTE_DIRECTORY;
		}

		if (0 == (Parameters.flagsAndAttributes & 0x0007FFFF))
			Parameters.flagsAndAttributes |= FILE_ATTRIBUTE_NORMAL;

		if (Parameters.securityDescriptor)
		{
			SecurityAttributes.nLength = sizeof SecurityAttributes;
			SecurityAttributes.lpSecurityDescriptor = Parameters.securityDescriptor;
			SecurityAttributes.bInheritHandle = FALSE;
			PSecurityAttributes = &SecurityAttributes;
		}

		Handle = CreateFile(FullPath,
			Parameters.accessMode, Parameters.shareMode, PSecurityAttributes,
			CreationDisposition, Parameters.flagsAndAttributes, 0);

		if (INVALID_HANDLE_VALUE == Handle)
			throw getError();

		Context->status = getError();
		Context->handle = (sl_uint64)Handle;
		Context->isDirectory = Parameters.attr.isDirectory;
	}

	void MirrorFs::fsOpen(FileContext* Context, FileCreationParams& Parameters)
	{
		WCHAR FullPath[MAX_PATH];
		DWORD CreationDisposition = OPEN_EXISTING;
		LPSECURITY_ATTRIBUTES PSecurityAttributes = NULL;
		SECURITY_ATTRIBUTES SecurityAttributes;
		HANDLE Handle;

		ConcatPath(Context->path, FullPath);

		if (0 == Parameters.accessMode)
			Parameters.accessMode = GENERIC_READ | GENERIC_WRITE;
		if (0 == Parameters.shareMode)
			Parameters.shareMode = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
		if (Parameters.createAlways) {
			CreationDisposition = OPEN_ALWAYS;
			if (Parameters.openTruncate)
				CreationDisposition = CREATE_ALWAYS;	// truncate
			Parameters.createAlways = FALSE;
			Parameters.openTruncate = FALSE;
		}
		else if (Parameters.openTruncate) {
			CreationDisposition = TRUNCATE_EXISTING;
			Parameters.openTruncate = FALSE;
		}

		Parameters.flagsAndAttributes |= FILE_FLAG_BACKUP_SEMANTICS;

		if (0 == (Parameters.flagsAndAttributes & 0x0007FFFF))
			Parameters.flagsAndAttributes |= FILE_ATTRIBUTE_NORMAL;

		if (Parameters.securityDescriptor)
		{
			SecurityAttributes.nLength = sizeof SecurityAttributes;
			SecurityAttributes.lpSecurityDescriptor = Parameters.securityDescriptor;
			SecurityAttributes.bInheritHandle = FALSE;
			PSecurityAttributes = &SecurityAttributes;
		}

		Handle = CreateFile(FullPath,
			Parameters.accessMode, Parameters.shareMode, PSecurityAttributes,
			CreationDisposition, Parameters.flagsAndAttributes, 0);

		if (INVALID_HANDLE_VALUE == Handle)
			throw getError();

		Context->status = getError();
		Context->handle = (sl_uint64)Handle;
		Context->isDirectory = Parameters.attr.isDirectory;
	}

	void MirrorFs::fsClose(FileContext* Context)
	{
		if (Context->handle && !CloseHandle(HandleFromContext(Context)))
			throw getError();
		Context->handle = 0;
	}

	sl_size MirrorFs::fsRead(FileContext* Context, const Memory& Buffer, sl_uint64 Offset)
	{
		DWORD BytesTransferred = 0;

		LARGE_INTEGER li;
		li.QuadPart = Offset;
		if (!SetFilePointerEx(HandleFromContext(Context), li, NULL, FILE_BEGIN))
			throw getError();

		if (!ReadFile(HandleFromContext(Context), Buffer.getData(), (DWORD)Buffer.getSize(), &BytesTransferred, NULL))
			throw getError();

		return BytesTransferred;
	}

	sl_size MirrorFs::fsWrite(FileContext* Context, const Memory& Buffer, sl_uint64 Offset, sl_bool writeToEof)
	{
		DWORD BytesTransferred = 0;

		LARGE_INTEGER li;
		if (writeToEof) {
			li.QuadPart = 0;
			if (!SetFilePointerEx(HandleFromContext(Context), li, NULL, FILE_END))
				throw getError();
		}
		else {
			li.QuadPart = Offset;
			if (!SetFilePointerEx(HandleFromContext(Context), li, NULL, FILE_BEGIN))
				throw getError();
		}

		if (!WriteFile(HandleFromContext(Context), Buffer.getData(), (DWORD)Buffer.getSize(), &BytesTransferred, NULL))
			throw getError();

		return BytesTransferred;
	}

	void MirrorFs::fsFlush(FileContext* Context)
	{
		/* we do not flush the whole volume, so just return SUCCESS */
		if (0 == HandleFromContext(Context)) {
			return;
		}

		if (!FlushFileBuffers(HandleFromContext(Context)))
			throw getError();
	}

	void MirrorFs::fsDelete(FileContext* Context, sl_bool checkOnly)
	{
		WCHAR FullPath[MAX_PATH];
		ZeroMemory(FullPath, sizeof(FullPath));

		if (!GetFinalPathNameByHandle(HandleFromContext(Context), FullPath, MAX_PATH - 1, 0))
			ConcatPath(Context->path, FullPath);

		DWORD fileAttr = GetFileAttributes(FullPath);
		if (INVALID_FILE_ATTRIBUTES == fileAttr)
			throw getError();

		if (checkOnly) {
			if (fileAttr && fileAttr & FILE_ATTRIBUTE_DIRECTORY) {
				HANDLE hFind;
				WIN32_FIND_DATAW findData;

				ULONG fileLen = (ULONG)wcslen(FullPath);
				if (FullPath[fileLen - 1] != L'\\') {
					FullPath[fileLen++] = L'\\';
				}
				FullPath[fileLen] = L'*';

				hFind = FindFirstFile(FullPath, &findData);
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
				FILE_DISPOSITION_INFO DispositionInfo;
				DispositionInfo.DeleteFile = TRUE;

				if (!SetFileInformationByHandle(HandleFromContext(Context),
					FileDispositionInfo, &DispositionInfo, sizeof DispositionInfo))
					throw getError();

				DispositionInfo.DeleteFile = FALSE;
				SetFileInformationByHandle(HandleFromContext(Context),
					FileDispositionInfo, &DispositionInfo, sizeof DispositionInfo);
			}
		}
		else {
			if (fileAttr && fileAttr & FILE_ATTRIBUTE_DIRECTORY) {
				if (!RemoveDirectory(FullPath))
					throw getError();
			}
			else {
				if (!DeleteFile(FullPath))
					throw getError();
			}
		}
	}

	void MirrorFs::fsRename(FileContext* Context, String NewFileName, sl_bool ReplaceIfExists)
	{
		WCHAR FullPath[MAX_PATH], NewFullPath[MAX_PATH];

		ConcatPath(Context->path, FullPath);
		ConcatPath(NewFileName, NewFullPath);

		if (!MoveFileEx(FullPath, NewFullPath, ReplaceIfExists ? MOVEFILE_REPLACE_EXISTING : 0))
			throw getError();
	}

	void MirrorFs::fsLock(FileContext* Context, sl_uint64 Offset, sl_uint64 Length)
	{
		LARGE_INTEGER offset;
		LARGE_INTEGER length;
		offset.QuadPart = Offset;
		length.QuadPart = Length;

		if (!LockFile(HandleFromContext(Context), 
			offset.HighPart, offset.LowPart, 
			length.HighPart, length.LowPart)) {
			throw getError();
		}
	}

	void MirrorFs::fsUnlock(FileContext* Context, sl_uint64 Offset, sl_uint64 Length)
	{
		LARGE_INTEGER offset;
		LARGE_INTEGER length;
		offset.QuadPart = Offset;
		length.QuadPart = Length;

		if (!UnlockFile(HandleFromContext(Context),
			offset.HighPart, offset.LowPart,
			length.HighPart, length.LowPart)) {
			throw getError();
		}
	}

	FileInfo MirrorFs::fsGetFileInfo(FileContext* Context)
	{
		FileInfo FileInfo;

		if (0 == HandleFromContext(Context)) {
			WCHAR FullPath[MAX_PATH];
			ConcatPath(Context->path, FullPath);

			if (Context->path.getLength() == 1) {
				FileInfo.fileAttributes = GetFileAttributes(FullPath);
			}
			else {
				WIN32_FIND_DATAW find;
				ZeroMemory(&find, sizeof(WIN32_FIND_DATAW));
				HANDLE findHandle = FindFirstFile(FullPath, &find);
				if (findHandle == INVALID_HANDLE_VALUE)
					throw getError();

				FileInfo.fileAttributes = find.dwFileAttributes;
				FileInfo.size =
					((UINT64)find.nFileSizeHigh << 32) | (UINT64)find.nFileSizeLow;
				FileInfo.allocationSize = (FileInfo.size + ALLOCATION_UNIT - 1)
					/ ALLOCATION_UNIT * ALLOCATION_UNIT;
				FileTimeToTime(find.ftCreationTime, FileInfo.createdAt);
				FileTimeToTime(find.ftLastAccessTime, FileInfo.lastAccessedAt);
				FileTimeToTime(find.ftLastWriteTime, FileInfo.modifiedAt);

				FindClose(findHandle);
			}
		}
		else {
			BY_HANDLE_FILE_INFORMATION ByHandleFileInfo;

			if (!GetFileInformationByHandle(HandleFromContext(Context), &ByHandleFileInfo))
				throw getError();

			if (Context->path.getLength() == 1) {
				WCHAR Root[MAX_PATH];
				StringToWChar(_Root, Root);
				FileInfo.fileAttributes = GetFileAttributes(Root);
			}
			else {
				FileInfo.fileAttributes = ByHandleFileInfo.dwFileAttributes;
			}
			if (INVALID_FILE_ATTRIBUTES == FileInfo.fileAttributes)
				throw getError();

			FileInfo.size =
				((UINT64)ByHandleFileInfo.nFileSizeHigh << 32) | (UINT64)ByHandleFileInfo.nFileSizeLow;
			FileInfo.allocationSize = (FileInfo.size + ALLOCATION_UNIT - 1)
				/ ALLOCATION_UNIT * ALLOCATION_UNIT;
			FileTimeToTime(ByHandleFileInfo.ftCreationTime, FileInfo.createdAt);
			FileTimeToTime(ByHandleFileInfo.ftLastAccessTime, FileInfo.lastAccessedAt);
			FileTimeToTime(ByHandleFileInfo.ftLastWriteTime, FileInfo.modifiedAt);
		}

		return FileInfo;
	}

	void MirrorFs::fsSetFileInfo(FileContext* Context, FileInfo Info, FileInfoFlags Flags)
	{
		if (Flags & FileInfoFlags::AttrAndTimeInfo) {
			FILE_BASIC_INFO BasicInfo = { 0 };
			FileInfo OriginFileInfo = fsGetFileInfo(Context);

			TimeToFileTimeLI(OriginFileInfo.createdAt, BasicInfo.CreationTime);
			TimeToFileTimeLI(OriginFileInfo.lastAccessedAt, BasicInfo.LastAccessTime);
			TimeToFileTimeLI(OriginFileInfo.modifiedAt, BasicInfo.LastWriteTime);
			TimeToFileTimeLI(OriginFileInfo.modifiedAt, BasicInfo.ChangeTime);
			BasicInfo.FileAttributes = 0;	// This means not to change original attribute

			if (Flags & FileInfoFlags::AttrInfo) {
				if (INVALID_FILE_ATTRIBUTES == Info.fileAttributes)
					Info.fileAttributes = 0;
				//else if (0 == FileInfo.fileAttributes)
				//	Info.fileAttributes = FILE_ATTRIBUTE_NORMAL;

				BasicInfo.FileAttributes = Info.fileAttributes;
			}

			if (Flags & FileInfoFlags::TimeInfo) {
				if (!Info.createdAt.isZero())
					TimeToFileTimeLI(Info.createdAt, BasicInfo.CreationTime);
				if (!Info.lastAccessedAt.isZero())
					TimeToFileTimeLI(Info.lastAccessedAt, BasicInfo.LastAccessTime);
				if (!Info.modifiedAt.isZero()) {
					TimeToFileTimeLI(Info.modifiedAt, BasicInfo.LastWriteTime);
					TimeToFileTimeLI(Info.modifiedAt, BasicInfo.ChangeTime);
				}
			}

			if (!SetFileInformationByHandle(HandleFromContext(Context),
				FileBasicInfo, &BasicInfo, sizeof BasicInfo))
				throw getError();
		}

		if (Flags & FileInfoFlags::SizeInfo) {
			FILE_END_OF_FILE_INFO EndOfFileInfo;
			EndOfFileInfo.EndOfFile.QuadPart = Info.size;

			if (!SetFileInformationByHandle(HandleFromContext(Context),
				FileEndOfFileInfo, &EndOfFileInfo, sizeof EndOfFileInfo))
				throw getError();
		}

		if (Flags & FileInfoFlags::AllocSizeInfo) {
			FILE_ALLOCATION_INFO AllocationInfo;
			AllocationInfo.AllocationSize.QuadPart = Info.allocationSize;

			if (!SetFileInformationByHandle(HandleFromContext(Context),
				FileAllocationInfo, &AllocationInfo, sizeof AllocationInfo))
				throw getError();
		}
	}

	Memory MirrorFs::fsGetSecurity(FileContext* Context, sl_uint32 SecurityInformation)
	{
		Memory SecurityDescriptor;
		DWORD LengthNeeded;
		if (!GetUserObjectSecurity(HandleFromContext(Context),
			(PSECURITY_INFORMATION)&SecurityInformation,
			(PSECURITY_DESCRIPTOR)SecurityDescriptor.getData(),
			(DWORD)SecurityDescriptor.getSize(),
			&LengthNeeded)) {
			if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
				SecurityDescriptor = Memory::create(LengthNeeded);
				if (!GetUserObjectSecurity(HandleFromContext(Context),
					(PSECURITY_INFORMATION)&SecurityInformation,
					(PSECURITY_DESCRIPTOR)SecurityDescriptor.getData(),
					(DWORD)SecurityDescriptor.getSize(),
					&LengthNeeded))
					throw getError();
			}
			else
				throw getError();
		}
		return SecurityDescriptor;
	}

	void MirrorFs::fsSetSecurity(FileContext* Context, sl_uint32 SecurityInformation, const Memory& SecurityDescriptor)
	{
		if (!SetUserObjectSecurity(HandleFromContext(Context),
			(PSECURITY_INFORMATION)&SecurityInformation,
			(PSECURITY_DESCRIPTOR)SecurityDescriptor.getData()))
			throw getError();
	}

	HashMap<String, FileInfo> MirrorFs::fsFindFiles(FileContext* Context, String PatternString)
	{
		HANDLE Handle = HandleFromContext(Context);
		WCHAR FullPath[MAX_PATH];
		WCHAR Pattern[MAX_PATH];
		ULONG Length, PatternLength;
		HANDLE FindHandle;
		WIN32_FIND_DATA FindData;
		HashMap<String, FileInfo> Files;

		if (PatternString.isNull() || PatternString.isEmpty())
			PatternString = "*";

		if (Handle) {
			Length = GetFinalPathNameByHandle(Handle, FullPath, MAX_PATH - 1, 0);
			if (0 == Length)
				throw getError();
		}
		else {
			ConcatPath(Context->path, FullPath);
			Length = (ULONG)wcslen(FullPath);
		}

		StringToWChar(PatternString, Pattern);
		PatternLength = (ULONG)wcslen(Pattern);

		if (Length + 1 + PatternLength >= MAX_PATH)
			throw getError(ERROR_INVALID_NAME);

		if (L'\\' != FullPath[Length - 1])
			FullPath[Length++] = L'\\';
		memcpy(FullPath + Length, Pattern, PatternLength * sizeof(WCHAR));
		FullPath[Length + PatternLength] = L'\0';

		FindHandle = FindFirstFile(FullPath, &FindData);
		if (INVALID_HANDLE_VALUE == FindHandle)
			throw getError();

		BOOLEAN rootFolder = Context->path.getLength() == 1;
		do {
			String FileName = WCharToString(FindData.cFileName);
			FileInfo FileInfo;
			if (rootFolder && (FileName == "." || FileName == ".."))
				continue;

			FileInfo.fileAttributes = FindData.dwFileAttributes;
			FileInfo.size =
				((UINT64)FindData.nFileSizeHigh << 32) | (UINT64)FindData.nFileSizeLow;
			FileInfo.allocationSize = (FileInfo.size + ALLOCATION_UNIT - 1)
				/ ALLOCATION_UNIT * ALLOCATION_UNIT;
			FileTimeToTime(FindData.ftCreationTime, FileInfo.createdAt);
			FileTimeToTime(FindData.ftLastAccessTime, FileInfo.lastAccessedAt);
			FileTimeToTime(FindData.ftLastWriteTime, FileInfo.modifiedAt);

			Files.add(FileName, FileInfo);
		} while (FindNextFile(FindHandle, &FindData) != 0);
			
		FindClose(FindHandle);

		if (GetLastError() != ERROR_NO_MORE_FILES)
			throw getError();

		return Files;
	}

	HashMap<String, StreamInfo> MirrorFs::fsFindStreams(FileContext* Context)
	{
		HANDLE Handle = HandleFromContext(Context);
		WCHAR FullPath[MAX_PATH];
		ULONG Length;
		HANDLE FindHandle;
		WIN32_FIND_STREAM_DATA FindData;
		HashMap<String, StreamInfo> Streams;

		if (Handle) {
			Length = GetFinalPathNameByHandle(Handle, FullPath, MAX_PATH - 1, 0);
			if (0 == Length)
				throw getError();
		}
		else {
			ConcatPath(Context->path, FullPath);
			Length = (ULONG)wcslen(FullPath);
		}

		FindHandle = FindFirstStreamW(FullPath, FindStreamInfoStandard, &FindData, 0);
		if (INVALID_HANDLE_VALUE == FindHandle)
			throw getError();

		do {
			String StreamName = WCharToString(FindData.cStreamName);
			StreamInfo StreamInfo;
			StreamInfo.size = FindData.StreamSize.QuadPart;
			Streams.add(StreamName, StreamInfo);
		} while (FindNextStreamW(FindHandle, &FindData) != 0);

		FindClose(FindHandle);

		if (GetLastError() != ERROR_HANDLE_EOF)
			throw getError();

		return Streams;
	}

	FileSystemError MirrorFs::getError(sl_uint32 error)
	{
		return (FileSystemError)(error == 0 ? GetLastError() : error);
	}

}