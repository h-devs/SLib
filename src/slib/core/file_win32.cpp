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

#include <windows.h>

namespace slib
{

	sl_file File::_open(const StringParam& _filePath, const FileMode& mode, const FilePermissions& permisions)
	{
		StringCstr16 filePath(_filePath);
		if (filePath.isEmpty()) {
			return (sl_file)(INVALID_HANDLE_VALUE);
		}

		DWORD dwShareMode = 0;
		DWORD dwDesiredAccess = 0;
		DWORD dwCreateDisposition = 0;
		DWORD dwFlags = FILE_ATTRIBUTE_NORMAL;

		if (permisions & FilePermissions::ShareRead) {
			dwShareMode |= FILE_SHARE_READ;
		}
		if (permisions & FilePermissions::ShareWrite) {
			dwShareMode |= FILE_SHARE_WRITE;
		}
		if (permisions & FilePermissions::ShareDelete) {
			dwShareMode |= FILE_SHARE_DELETE;
		}

		if (mode & FileMode::Write) {
			dwDesiredAccess = GENERIC_WRITE;
			if (mode & FileMode::Read) {
				dwDesiredAccess |= GENERIC_READ;
			}
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
			dwDesiredAccess = GENERIC_READ;
			dwCreateDisposition = OPEN_EXISTING;
		}
		if (mode & FileMode::HintRandomAccess) {
			dwFlags |= FILE_FLAG_RANDOM_ACCESS;
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

	sl_uint64 File::getPosition()
	{
		if (isOpened()) {
			HANDLE handle = (HANDLE)m_file;
			sl_int64 pos = 0;
			DWORD ret = SetFilePointer(handle, 0, ((LONG*)&pos) + 1, FILE_CURRENT);
			DWORD err = NOERROR;
			if (ret == -1) {
				err = GetLastError();
			}
			if (err == NOERROR) {
				*((DWORD*)&pos) = ret;
				return pos;
			} else {
				return 0;
			}
		} else {
			return 0;
		}
	}

	sl_bool File::seek(sl_int64 location, SeekPosition from)
	{
		if (isOpened()) {
			HANDLE handle = (HANDLE)m_file;
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
			DWORD ret = SetFilePointer(handle, *((LONG*)&location), ((LONG*)&location) + 1, dwFrom);
			DWORD err = NOERROR;
			if (ret == -1) {
				err = GetLastError();
			}
			if (err == NOERROR) {
				return sl_true;
			}
		}
		return sl_false;
	}

	sl_int32 File::read32(void* buf, sl_uint32 size)
	{
		if (isOpened()) {
			if (size == 0) {
				return 0;
			}
			sl_uint32 ret = 0;
			HANDLE handle = (HANDLE)m_file;
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
		if (isOpened()) {
			if (size == 0) {
				return 0;
			}
			sl_uint32 ret = 0;
			HANDLE handle = (HANDLE)m_file;
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
		if (isOpened()) {
			sl_int64 pos_orig = getPosition();
			if (seek(size, SeekPosition::Begin)) {
				HANDLE handle = (HANDLE)m_file;
				BOOL bRet = SetEndOfFile(handle);
				seek(pos_orig, SeekPosition::Begin);
				return bRet != 0;
			}
		}
		return sl_false;
	}

	sl_uint64 File::getSize(sl_file fd)
	{
		HANDLE handle = (HANDLE)fd;
		if (handle != INVALID_HANDLE_VALUE) {
			sl_uint64 ret = 0;
			if (GetFileSizeEx(handle, (PLARGE_INTEGER)(&ret))) {
				return ret;
			} else {
				int n = GetFileType(handle);
			}
		}
		return 0;
	}

	sl_uint64 File::getSize(const StringParam& _filePath)
	{
		StringCstr16 filePath(_filePath);
		if (filePath.isEmpty()) {
			return 0;
		}
		HANDLE handle = CreateFileW((LPCWSTR)(filePath.getData()), 0, 0, NULL, OPEN_EXISTING, 0, NULL);
		if (handle != INVALID_HANDLE_VALUE) {
			sl_uint64 ret = getSize((sl_file)handle);
			CloseHandle(handle);
			return ret;
		} else {
			return 0;
		}
	}

	sl_uint64 File::getDiskSize(sl_file fd)
	{
		HANDLE handle = (HANDLE)fd;
		if (handle != INVALID_HANDLE_VALUE) {
			sl_uint64 size = 0;
			DWORD nOutput;
			DeviceIoControl(handle, IOCTL_DISK_GET_LENGTH_INFO, NULL, 0, &size, sizeof(size), &nOutput, NULL);
			return size;
		}
		return 0;
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

	namespace priv
	{
		namespace file
		{

			static SLIB_INLINE Time filetimeToTime(const FILETIME& ft)
			{
				Time time;
				SYSTEMTIME st = { 0 };
				FileTimeToSystemTime(&(ft), &st);
				time.setUTC(st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
				return time;
			}

			static SLIB_INLINE void timeToFiletime(Time time, FILETIME& ft)
			{
				TimeComponents tc;
				time.getUTC(tc);
				SYSTEMTIME st = { (WORD)tc.year, (WORD)tc.month, (WORD)tc.dayOfWeek, (WORD)tc.day,
					(WORD)tc.hour, (WORD)tc.minute, (WORD)tc.second, (WORD)tc.milliseconds };
				SystemTimeToFileTime(&st, &ft);
			}

			static Time getModifiedTime(HANDLE handle)
			{
				FILETIME ft;
				BOOL bRet = GetFileTime(handle, NULL, NULL, &ft);
				if (bRet) {
					return filetimeToTime(ft);
				} else {
					return Time::zero();
				}
			}

			static Time getAccessedTime(HANDLE handle)
			{
				FILETIME ft;
				BOOL bRet = GetFileTime(handle, NULL, &ft, NULL);
				if (bRet) {
					return filetimeToTime(ft);
				} else {
					return Time::zero();
				}
			}

			static Time getCreatedTime(HANDLE handle)
			{
				FILETIME ft;
				BOOL bRet = GetFileTime(handle, &ft, NULL, NULL);
				if (bRet) {
					return filetimeToTime(ft);
				} else {
					return Time::zero();
				}
			}

			static sl_bool setModifiedTime(HANDLE handle, Time time)
			{
				FILETIME ft;
				timeToFiletime(time, ft);
				BOOL bRet = SetFileTime(handle, NULL, NULL, &ft);
				CloseHandle(handle);
				return bRet != 0;
			}

			static sl_bool setAccessedTime(HANDLE handle, Time time)
			{
				FILETIME ft;
				timeToFiletime(time, ft);
				BOOL bRet = SetFileTime(handle, NULL, &ft, NULL);
				CloseHandle(handle);
				return bRet != 0;
			}

			static sl_bool setCreatedTime(HANDLE handle, Time time)
			{
				FILETIME ft;
				timeToFiletime(time, ft);
				BOOL bRet = SetFileTime(handle, &ft, NULL, NULL);
				CloseHandle(handle);
				return bRet != 0;
			}

		}
	}

	Time File::getModifiedTime()
	{
		if (isOpened()) {
			return priv::file::getModifiedTime((HANDLE)m_file);
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
			Time ret = priv::file::getModifiedTime(handle);
			CloseHandle(handle);
			return ret;
		} else {
			return Time::zero();
		}
	}

	Time File::getAccessedTime()
	{
		if (isOpened()) {
			return priv::file::getAccessedTime((HANDLE)m_file);
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
			Time ret = priv::file::getAccessedTime(handle);
			CloseHandle(handle);
			return ret;
		} else {
			return Time::zero();
		}
	}

	Time File::getCreatedTime()
	{
		if (isOpened()) {
			return priv::file::getCreatedTime((HANDLE)m_file);
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
		HANDLE handle = CreateFileW((LPCWSTR)(filePath.getData()), FILE_READ_ATTRIBUTES, 0, NULL, OPEN_EXISTING,
			FILE_FLAG_BACKUP_SEMANTICS, NULL);
		if (handle != INVALID_HANDLE_VALUE) {
			Time ret = priv::file::getCreatedTime(handle);
			CloseHandle(handle);
			return ret;
		} else {
			return Time::zero();
		}
	}

	sl_bool File::setModifiedTime(const StringParam& _filePath, Time time)
	{
		StringCstr16 filePath(_filePath);
		if (filePath.isEmpty()) {
			return sl_false;
		}
		HANDLE handle = CreateFileW((LPCWSTR)(filePath.getData()), GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
		if (handle != INVALID_HANDLE_VALUE) {
			sl_bool ret = priv::file::setModifiedTime(handle, time);
			CloseHandle(handle);
			return ret;
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
		HANDLE handle = CreateFileW((LPCWSTR)(filePath.getData()), GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
		if (handle != INVALID_HANDLE_VALUE) {
			sl_bool ret = priv::file::setAccessedTime(handle, time);
			CloseHandle(handle);
			return ret;
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
		HANDLE handle = CreateFileW((LPCWSTR)(filePath.getData()), GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
		if (handle != INVALID_HANDLE_VALUE) {
			sl_bool ret = priv::file::setCreatedTime(handle, time);
			CloseHandle(handle);
			return ret;
		} else {
			return sl_false;
		}
	}

	FileAttributes File::getAttributes(const StringParam& _filePath)
	{
		StringCstr16 filePath(_filePath);
		if (filePath.isEmpty()) {
			return FileAttributes::NotExist;
		}
		DWORD attr = GetFileAttributesW((LPCWSTR)(filePath.getData()));
		if (attr == -1) {
			return FileAttributes::NotExist;
		} else {
			return (int)attr;
		}
	}

	sl_bool File::setAttributes(const StringParam& _filePath, const FileAttributes& attrs)
	{
		StringCstr16 filePath(_filePath);
		if (filePath.isEmpty()) {
			return sl_false;
		}
		if (attrs & FileAttributes::NotExist) {
			return sl_false;
		}
		return SetFileAttributesW((LPCWSTR)(filePath.getData()), (DWORD)(attrs.value & 0xffff)) != 0;
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

	sl_bool File::_createDirectory(const StringParam& _filePath)
	{
		StringCstr16 filePath(_filePath);
		if (filePath.isEmpty()) {
			return sl_false;
		}
		BOOL ret = CreateDirectoryW((LPCWSTR)(filePath.getData()), NULL);
		return ret != 0;
	}

	sl_bool File::_deleteFile(const StringParam& _filePath)
	{
		StringCstr16 filePath(_filePath);
		if (filePath.isEmpty()) {
			return sl_false;
		}
		BOOL ret = DeleteFileW((LPCWSTR)(filePath.getData()));
		return ret != 0;
	}

	sl_bool File::_deleteDirectory(const StringParam& _filePath)
	{
		String filePath = _filePath.toString();
		if (filePath.isEmpty()) {
			return sl_false;
		}
		String16 dirPath = String16::from(normalizeDirectoryPath(filePath));
		BOOL ret = RemoveDirectoryW((LPCWSTR)(dirPath.getData()));
		return ret != 0;
	}

	sl_bool File::rename(const StringParam& _oldPath, const StringParam& _newPath)
	{
		StringCstr16 oldPath(_oldPath);
		if (oldPath.isEmpty()) {
			return sl_false;
		}
		StringCstr16 newPath(_newPath);
		if (newPath.isEmpty()) {
			return sl_false;
		}
		BOOL ret = MoveFileExW((LPCWSTR)(oldPath.getData()), (LPCWSTR)(newPath.getData()), 0);
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
