/*
 *   Copyright (c) 2008-2018 SLIBIO <https://github.com/SLIBIO>
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

#include "slib/core/file.h"

#include "slib/core/dl_windows_kernel32.h"

#include <winioctl.h>

namespace slib
{

	namespace priv
	{
		namespace file
		{

			SLIB_INLINE static Time FileTimeToTime(const FILETIME& ft)
			{
				return Time::fromWindowsFileTime(*((sl_int64*)&ft));
			}

			SLIB_INLINE static void TimeToFileTime(Time time, FILETIME& ft)
			{
				*((sl_int64*)&ft) = time.toWindowsFileTime();
			}

			SLIB_INLINE static Time GetModifiedTime(HANDLE handle)
			{
				FILETIME ft;
				BOOL bRet = GetFileTime(handle, NULL, NULL, &ft);
				if (bRet) {
					return FileTimeToTime(ft);
				} else {
					return Time::zero();
				}
			}

			SLIB_INLINE static Time GetAccessedTime(HANDLE handle)
			{
				FILETIME ft;
				BOOL bRet = GetFileTime(handle, NULL, &ft, NULL);
				if (bRet) {
					return FileTimeToTime(ft);
				} else {
					return Time::zero();
				}
			}

			SLIB_INLINE static Time GetCreatedTime(HANDLE handle)
			{
				FILETIME ft;
				BOOL bRet = GetFileTime(handle, &ft, NULL, NULL);
				if (bRet) {
					return FileTimeToTime(ft);
				} else {
					return Time::zero();
				}
			}

			SLIB_INLINE static sl_bool SetModifiedTime(HANDLE handle, Time time)
			{
				FILETIME ft;
				TimeToFileTime(time, ft);
				BOOL bRet = SetFileTime(handle, NULL, NULL, &ft);
				return bRet != 0;
			}

			SLIB_INLINE static sl_bool SetAccessedTime(HANDLE handle, Time time)
			{
				FILETIME ft;
				TimeToFileTime(time, ft);
				BOOL bRet = SetFileTime(handle, NULL, &ft, NULL);
				return bRet != 0;
			}

			SLIB_INLINE static sl_bool SetCreatedTime(HANDLE handle, Time time)
			{
				FILETIME ft;
				TimeToFileTime(time, ft);
				BOOL bRet = SetFileTime(handle, &ft, NULL, NULL);
				return bRet != 0;
			}

		}
	}

	using namespace priv::file;

	sl_file File::_open(const StringParam& _filePath, const FileMode& mode, const FileAttributes& attrs)
	{
		StringCstr16 filePath(_filePath);
		if (filePath.isEmpty()) {
			return (sl_file)(INVALID_HANDLE_VALUE);
		}

		DWORD dwShareMode = 0;
		DWORD dwDesiredAccess = 0;
		DWORD dwCreateDisposition = 0;
		DWORD dwFlags = 0;
		if (mode & FileMode::Write) {
			if (!(mode & FileMode::NotCreate)) {
				dwFlags = attrs & 0x7ffff;
			}
		}
		if (mode & FileMode::ShareRead) {
			dwShareMode |= FILE_SHARE_READ;
		}
		if (mode & FileMode::ShareWrite) {
			dwShareMode |= FILE_SHARE_WRITE;
		}
		if (mode & FileMode::ShareDelete) {
			dwShareMode |= FILE_SHARE_DELETE;
		}
		if (mode & FileMode::Read) {
			if (mode & FileMode::ReadData) {
				dwDesiredAccess |= FILE_READ_DATA;
			}
			if (mode & FileMode::ReadAttrs) {
				dwDesiredAccess |= FILE_READ_ATTRIBUTES | FILE_READ_EA;
			}
			if (!(mode & (FileMode::ReadData | FileMode::ReadAttrs))) {
				dwDesiredAccess |= GENERIC_READ;
			}
		}
		if (mode & FileMode::Write) {
			if (mode & FileMode::WriteData) {
				dwDesiredAccess |= FILE_WRITE_DATA;
			}
			if (mode & FileMode::WriteAttrs) {
				dwDesiredAccess |= FILE_WRITE_ATTRIBUTES | FILE_WRITE_EA;
			}
			if (!(mode & (FileMode::WriteData | FileMode::WriteAttrs))) {
				dwDesiredAccess |= GENERIC_WRITE;
			}
			if (mode & FileMode::SeekToEnd) {
				dwDesiredAccess |= FILE_APPEND_DATA;
			}
		}
		if (mode & FileMode::Sync) {
			dwDesiredAccess |= SYNCHRONIZE;
		}
		if (mode & FileMode::Write) {
			if (mode & FileMode::NotCreate) {
				if (mode & FileMode::NotTruncate) {
					dwCreateDisposition = OPEN_EXISTING;
				} else {
					dwCreateDisposition = TRUNCATE_EXISTING;
				}
			} else {
				if (mode & FileMode::NotTruncate) {
					dwCreateDisposition = OPEN_ALWAYS;
				} else {
					dwCreateDisposition = CREATE_ALWAYS;
				}
			}
		} else {
			dwCreateDisposition = OPEN_EXISTING;
		}
		if (mode & FileMode::HintRandomAccess) {
			dwFlags |= FILE_FLAG_RANDOM_ACCESS;
		}
		if (mode & FileMode::Directory) {
			dwFlags |= FILE_FLAG_BACKUP_SEMANTICS;
		}

		HANDLE handle = CreateFileW(
			(LPCWSTR)(filePath.getData())
			, dwDesiredAccess
			, dwShareMode
			, NULL
			, dwCreateDisposition
			, dwFlags
			, NULL
		);
		return (sl_file)handle;
	}

	sl_bool File::_close(sl_file file)
	{
		HANDLE handle = (HANDLE)file;
		if (handle != INVALID_HANDLE_VALUE) {
			CloseHandle(handle);
			return sl_true;
		}
		return sl_false;
	}

	sl_bool File::getPosition(sl_uint64& outPos)
	{
		HANDLE handle = (HANDLE)m_file;
		if (handle != INVALID_HANDLE_VALUE) {
			static LARGE_INTEGER zero = { 0 };
			if (SetFilePointerEx(handle, zero, ((LARGE_INTEGER*)&outPos), FILE_CURRENT)) {
				return sl_true;
			}
		}
		return sl_false;
	}

	sl_bool File::seek(sl_int64 location, SeekPosition from)
	{
		HANDLE handle = (HANDLE)m_file;
		if (handle != INVALID_HANDLE_VALUE) {
			DWORD dwFrom;
			if (from == SeekPosition::Current) {
				dwFrom = FILE_CURRENT;
			} else if (from == SeekPosition::Begin) {
				dwFrom = FILE_BEGIN;
			} else if (from == SeekPosition::End) {
				dwFrom = FILE_END;
			} else {
				return sl_false;
			}
			if (SetFilePointerEx(handle, *((LARGE_INTEGER*)&location), NULL, dwFrom)) {
				return sl_true;
			}
		}
		return sl_false;
	}

	sl_bool File::isEnd(sl_bool& outFlag)
	{
		HANDLE handle = (HANDLE)m_file;
		if (handle != INVALID_HANDLE_VALUE) {
			static LARGE_INTEGER zero = { 0 };
			sl_uint64 cur;
			if (SetFilePointerEx(handle, zero, ((LARGE_INTEGER*)&cur), FILE_CURRENT)) {
				sl_uint64 end;
				if (SetFilePointerEx(handle, zero, ((LARGE_INTEGER*)&end), FILE_END)) {
					if (cur == end) {
						outFlag = sl_true;
					} else {
						outFlag = sl_false;
						SetFilePointerEx(handle, *((LARGE_INTEGER*)&cur), NULL, FILE_BEGIN);
					}
					return sl_true;
				}
			}
		}
		return sl_false;
	}

	sl_int32 File::read32(void* buf, sl_uint32 size)
	{
		HANDLE handle = (HANDLE)m_file;
		if (handle != INVALID_HANDLE_VALUE) {
			if (size == 0) {
				return 0;
			}
			sl_uint32 ret = 0;
			if (ReadFile(handle, buf, size, (DWORD*)&ret, NULL)) {
				if (ret > 0) {
					return ret;
				}
			}
		}
		return -1;
	}

	sl_int32 File::write32(const void* buf, sl_uint32 size)
	{
		HANDLE handle = (HANDLE)m_file;
		if (handle != INVALID_HANDLE_VALUE) {
			if (size == 0) {
				return 0;
			}
			sl_uint32 ret = 0;
			if (WriteFile(handle, (LPVOID)buf, size, (DWORD*)&ret, NULL)) {
				if (ret > 0) {
					return ret;
				}
			}
		}
		return -1;
	}

	sl_bool File::setSize(sl_uint64 size)
	{
		HANDLE handle = (HANDLE)m_file;
		if (handle != INVALID_HANDLE_VALUE) {
			sl_int64 posOld = getPosition();
			if (seek(size, SeekPosition::Begin)) {
				BOOL bRet = SetEndOfFile(handle);
				seek(posOld, SeekPosition::Begin);
				return bRet != 0;
			}
		}
		return sl_false;
	}

	sl_bool File::getSizeByHandle(sl_file fd, sl_uint64& outSize)
	{
		HANDLE handle = (HANDLE)fd;
		if (handle != INVALID_HANDLE_VALUE) {
			if (GetFileSizeEx(handle, (PLARGE_INTEGER)(&outSize))) {
				return sl_true;
			}
		}
		return sl_false;
	}

	sl_bool File::getSize(const StringParam& _filePath, sl_uint64& outSize)
	{
		StringCstr16 filePath(_filePath);
		if (filePath.isEmpty()) {
			return sl_false;
		}
		HANDLE handle = CreateFileW((LPCWSTR)(filePath.getData()), 0, 0, NULL, OPEN_EXISTING, 0, NULL);
		if (handle != INVALID_HANDLE_VALUE) {
			sl_bool bRet = getSizeByHandle((sl_file)handle, outSize);
			CloseHandle(handle);
			return bRet;
		}
		return sl_false;
	}

	sl_bool File::getDiskSizeByHandle(sl_file fd, sl_uint64& outSize)
	{
		HANDLE handle = (HANDLE)fd;
		if (handle != INVALID_HANDLE_VALUE) {
			sl_uint64 size = 0;
			DWORD nOutput;
			if (DeviceIoControl(handle, IOCTL_DISK_GET_LENGTH_INFO, NULL, 0, &size, sizeof(size), &nOutput, NULL)) {
				outSize = size;
				return sl_true;
			}
		}
		return sl_false;
	}

	sl_bool File::lock()
	{
		HANDLE handle = (HANDLE)m_file;
		if (handle != INVALID_HANDLE_VALUE) {
			OVERLAPPED o;
			ZeroMemory(&o, sizeof(o));
			if (LockFileEx(handle, LOCKFILE_EXCLUSIVE_LOCK | LOCKFILE_FAIL_IMMEDIATELY, 0, 1, 0, &o)) {
				return sl_true;
			}
		}
		return sl_false;
	}

	sl_bool File::unlock()
	{
		HANDLE handle = (HANDLE)m_file;
		if (handle != INVALID_HANDLE_VALUE) {
			OVERLAPPED o;
			ZeroMemory(&o, sizeof(o));
			if (UnlockFileEx(handle, 0, 0, 0, &o)) {
				return sl_true;
			}
		}
		return sl_false;
	}

	sl_bool File::flush()
	{
		HANDLE handle = (HANDLE)m_file;
		if (handle != INVALID_HANDLE_VALUE) {
			if (FlushFileBuffers(handle)) {
				return sl_true;
			}
		}
		return sl_false;
	}

	Time File::getModifiedTime()
	{
		HANDLE handle = (HANDLE)m_file;
		if (handle != INVALID_HANDLE_VALUE) {
			return GetModifiedTime(handle);
		} else {
			return Time::zero();
		}
	}

	Time File::getModifiedTime(const StringParam& _filePath)
	{
		StringCstr16 filePath(_filePath);
		if (filePath.isEmpty()) {
			return Time::zero();
		}
		HANDLE handle = CreateFileW((LPCWSTR)(filePath.getData()), FILE_READ_ATTRIBUTES, 0, NULL, OPEN_EXISTING,
			FILE_FLAG_BACKUP_SEMANTICS, NULL);
		if (handle != INVALID_HANDLE_VALUE) {
			Time ret = GetModifiedTime(handle);
			CloseHandle(handle);
			return ret;
		} else {
			return Time::zero();
		}
	}

	Time File::getAccessedTime()
	{
		HANDLE handle = (HANDLE)m_file;
		if (handle != INVALID_HANDLE_VALUE) {
			return GetAccessedTime(handle);
		} else {
			return Time::zero();
		}
	}

	Time File::getAccessedTime(const StringParam& _filePath)
	{
		StringCstr16 filePath(_filePath);
		if (filePath.isEmpty()) {
			return Time::zero();
		}
		HANDLE handle = CreateFileW((LPCWSTR)(filePath.getData()), FILE_READ_ATTRIBUTES, 0, NULL, OPEN_EXISTING,
			FILE_FLAG_BACKUP_SEMANTICS, NULL);
		if (handle != INVALID_HANDLE_VALUE) {
			Time ret = GetAccessedTime(handle);
			CloseHandle(handle);
			return ret;
		} else {
			return Time::zero();
		}
	}

	Time File::getCreatedTime()
	{
		HANDLE handle = (HANDLE)m_file;
		if (handle != INVALID_HANDLE_VALUE) {
			return GetCreatedTime(handle);
		} else {
			return Time::zero();
		}
	}

	Time File::getCreatedTime(const StringParam& _filePath)
	{
		StringCstr16 filePath(_filePath);
		if (filePath.isEmpty()) {
			return Time::zero();
		}
		HANDLE handle = CreateFileW((LPCWSTR)(filePath.getData()), FILE_READ_ATTRIBUTES, 0, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
		if (handle != INVALID_HANDLE_VALUE) {
			Time ret = GetCreatedTime(handle);
			CloseHandle(handle);
			return ret;
		} else {
			return Time::zero();
		}
	}

	sl_bool File::setModifiedTime(Time time)
	{
		HANDLE handle = (HANDLE)m_file;
		if (handle != INVALID_HANDLE_VALUE) {
			return SetModifiedTime(handle, time);
		} else {
			return sl_false;
		}
	}

	sl_bool File::setModifiedTime(const StringParam& _filePath, Time time)
	{
		StringCstr16 filePath(_filePath);
		if (filePath.isEmpty()) {
			return sl_false;
		}
		HANDLE handle = CreateFileW((LPCWSTR)(filePath.getData()), FILE_WRITE_ATTRIBUTES, 0, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
		if (handle != INVALID_HANDLE_VALUE) {
			sl_bool ret = SetModifiedTime(handle, time);
			CloseHandle(handle);
			return ret;
		} else {
			return sl_false;
		}
	}

	sl_bool File::setAccessedTime(Time time)
	{
		HANDLE handle = (HANDLE)m_file;
		if (handle != INVALID_HANDLE_VALUE) {
			return SetAccessedTime(handle, time);
		} else {
			return sl_false;
		}
	}

	sl_bool File::setAccessedTime(const StringParam& _filePath, Time time)
	{
		StringCstr16 filePath(_filePath);
		if (filePath.isEmpty()) {
			return sl_false;
		}
		HANDLE handle = CreateFileW((LPCWSTR)(filePath.getData()), FILE_WRITE_ATTRIBUTES, 0, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
		if (handle != INVALID_HANDLE_VALUE) {
			sl_bool ret = SetAccessedTime(handle, time);
			CloseHandle(handle);
			return ret;
		} else {
			return sl_false;
		}
	}

	sl_bool File::setCreatedTime(Time time)
	{
		HANDLE handle = (HANDLE)m_file;
		if (handle != INVALID_HANDLE_VALUE) {
			return SetCreatedTime(handle, time);
		} else {
			return sl_false;
		}
	}

	sl_bool File::setCreatedTime(const StringParam& _filePath, Time time)
	{
		StringCstr16 filePath(_filePath);
		if (filePath.isEmpty()) {
			return sl_false;
		}
		HANDLE handle = CreateFileW((LPCWSTR)(filePath.getData()), FILE_WRITE_ATTRIBUTES, 0, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
		if (handle != INVALID_HANDLE_VALUE) {
			sl_bool ret = SetCreatedTime(handle, time);
			CloseHandle(handle);
			return ret;
		} else {
			return sl_false;
		}
	}

	FileAttributes File::getAttributes()
	{
		HANDLE handle = (HANDLE)m_file;
		if (handle != INVALID_HANDLE_VALUE) {
			BY_HANDLE_FILE_INFORMATION info;
			if (GetFileInformationByHandle(handle, &info)) {
				return info.dwFileAttributes & 0x7ffff;
			}
		}
		return FileAttributes::NotExist;
	}

	FileAttributes File::_getAttributes(const StringParam& _filePath)
	{
		StringCstr16 filePath(_filePath);
		DWORD attr = GetFileAttributesW((LPCWSTR)(filePath.getData()));
		if (attr == -1) {
			return FileAttributes::NotExist;
		} else {
			return (int)(attr & 0x7ffff);
		}
	}

	sl_bool File::_setAttributes(const StringParam& _filePath, const FileAttributes& attrs)
	{
		StringCstr16 filePath(_filePath);
		if (filePath.isEmpty()) {
			return sl_false;
		}
		return SetFileAttributesW((LPCWSTR)(filePath.getData()), (DWORD)(attrs.value & 0x7ffff)) != 0;
	}

	List<String> File::getFiles(const StringParam& _filePath)
	{
		String filePath(_filePath.toString());
		if (filePath.isEmpty()) {
			return sl_null;
		}
		if (File::isDirectory(filePath)) {
			filePath = normalizeDirectoryPath(filePath);
		} else {
			return sl_null;
		}

		String16 query = String16::create(filePath + "/*");
		WIN32_FIND_DATAW fd;
		HANDLE handle = FindFirstFileW((LPCWSTR)(query.getData()), &fd);
		if (handle != INVALID_HANDLE_VALUE) {
			List<String> ret;
			BOOL c = TRUE;
			while (c) {
				String str = String::create((sl_char16*)(fd.cFileName));
				if (str != "." && str != "..") {
					ret.add_NoLock(Move(str));
				}
				c = FindNextFileW(handle, &fd);
			}
			FindClose(handle);
			return ret;
		} else {
			return sl_null;
		}
	}

	HashMap<String, FileInfo> File::getFileInfos(const StringParam& _filePath)
	{
		String filePath(_filePath.toString());
		if (filePath.isEmpty()) {
			return sl_null;
		}
		if (File::isDirectory(filePath)) {
			filePath = normalizeDirectoryPath(filePath);
		} else {
			return sl_null;
		}

		String16 query = String16::create(filePath + "/*");
		WIN32_FIND_DATAW fd;
		HANDLE handle = FindFirstFileW((LPCWSTR)(query.getData()), &fd);
		if (handle != INVALID_HANDLE_VALUE) {
			HashMap<String, FileInfo> ret;
			BOOL c = TRUE;
			while (c) {
				String str = String::create((sl_char16*)(fd.cFileName));
				if (str != "." && str != "..") {
					FileInfo info;
					info.attributes = (int)fd.dwFileAttributes;
					info.size = info.allocSize = SLIB_MAKE_QWORD4(fd.nFileSizeHigh, fd.nFileSizeLow);
					info.createdAt = FileTimeToTime(fd.ftCreationTime);
					info.modifiedAt = FileTimeToTime(fd.ftLastWriteTime);
					info.accessedAt = FileTimeToTime(fd.ftLastAccessTime);
					ret.add_NoLock(Move(str), info);
				}
				c = FindNextFileW(handle, &fd);
			}
			FindClose(handle);
			return ret;
		} else {
			return sl_null;
		}
	}

	sl_bool File::_createDirectory(const StringParam& _filePath)
	{
		StringCstr16 filePath(_filePath);
		if (filePath.isEmpty()) {
			return sl_false;
		}
		BOOL ret = CreateDirectoryW((LPCWSTR)(filePath.getData()), NULL);
		return ret != 0;
	}

	sl_bool File::deleteFile(const StringParam& _filePath)
	{
		StringCstr16 filePath(_filePath);
		if (filePath.isEmpty()) {
			return sl_false;
		}
		BOOL ret = DeleteFileW((LPCWSTR)(filePath.getData()));
		return ret != 0;
	}

	sl_bool File::deleteDirectory(const StringParam& _filePath)
	{
		String filePath = _filePath.toString();
		if (filePath.isEmpty()) {
			return sl_false;
		}
		String16 dirPath = String16::from(normalizeDirectoryPath(filePath));
		BOOL ret = RemoveDirectoryW((LPCWSTR)(dirPath.getData()));
		return ret != 0;
	}

	sl_bool File::move(const StringParam& _oldPath, const StringParam& _newPath, sl_bool flagReplaceIfExists)
	{
		StringCstr16 oldPath(_oldPath);
		if (oldPath.isEmpty()) {
			return sl_false;
		}
		StringCstr16 newPath(_newPath);
		if (newPath.isEmpty()) {
			return sl_false;
		}
		DWORD flags = 0;
		if (flagReplaceIfExists) {
			flags |= MOVEFILE_REPLACE_EXISTING;
		}
		BOOL ret = MoveFileExW((LPCWSTR)(oldPath.getData()), (LPCWSTR)(newPath.getData()), flags);
		return ret != 0;
	}

	String File::getRealPath(const StringParam& _filePath)
	{
		StringCstr16 path(_filePath);
		WCHAR buf[4096];
		buf[0] = 0;
		if (GetFullPathNameW((LPCWSTR)(path.getData()), 4096, buf, NULL)) {
			return String::create(buf);
		}
		return sl_null;
	}


	DisableWow64FsRedirectionScope::DisableWow64FsRedirectionScope()
	{
		auto func = kernel32::getApi_Wow64DisableWow64FsRedirection();
		if (func) {
			func(&m_pOldValue);
		}
	}

	DisableWow64FsRedirectionScope::~DisableWow64FsRedirectionScope()
	{
		auto func = kernel32::getApi_Wow64RevertWow64FsRedirection();
		if (func) {
			func(m_pOldValue);
		}
	}

}

#endif
