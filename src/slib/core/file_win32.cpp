/*
 *	Copyright (c) 2008-2021 SLIBIO <https://github.com/SLIBIO>
 *
 *	Permission is hereby granted, free of charge, to any person obtaining a copy
 *	of this software and associated documentation files (the "Software"), to deal
 *	in the Software without restriction, including without limitation the rights
 *	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *	copies of the Software, and to permit persons to whom the Software is
 *	furnished to do so, subject to the following conditions:
 *
 *	The above copyright notice and this permission notice shall be included in
 *	all copies or substantial portions of the Software.
 *
 *	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 *	THE SOFTWARE.
 */

#include "slib/core/definition.h"

#ifdef SLIB_PLATFORM_IS_WIN32

#include "slib/core/file.h"
#include "slib/core/file_util.h"

#include "slib/core/hash_map.h"
#include "slib/core/thread.h"
#include "slib/core/win32/platform.h"
#include "slib/core/dl/win32/kernel32.h"

#include <winioctl.h>
#include <objbase.h>
#include <shobjidl.h>
#include <shlguid.h>

#define FILE_SHARE_ALL (FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE)

namespace slib
{

	namespace priv
	{
		namespace file
		{

			SLIB_INLINE static Time FileTimeToTime(const FILETIME& ft) noexcept
			{
				return Time::fromWindowsFileTime(*((sl_int64*)&ft));
			}

			SLIB_INLINE static void TimeToFileTime(const Time& time, FILETIME& ft) noexcept
			{
				*((sl_int64*)&ft) = time.toWindowsFileTime();
			}

			SLIB_INLINE static Time GetModifiedTime(HANDLE handle) noexcept
			{
				FILETIME ft;
				BOOL bRet = GetFileTime(handle, NULL, NULL, &ft);
				if (bRet) {
					return FileTimeToTime(ft);
				} else {
					return Time::zero();
				}
			}

			SLIB_INLINE static Time GetAccessedTime(HANDLE handle) noexcept
			{
				FILETIME ft;
				BOOL bRet = GetFileTime(handle, NULL, &ft, NULL);
				if (bRet) {
					return FileTimeToTime(ft);
				} else {
					return Time::zero();
				}
			}

			SLIB_INLINE static Time GetCreatedTime(HANDLE handle) noexcept
			{
				FILETIME ft;
				BOOL bRet = GetFileTime(handle, &ft, NULL, NULL);
				if (bRet) {
					return FileTimeToTime(ft);
				} else {
					return Time::zero();
				}
			}

			SLIB_INLINE static sl_bool SetModifiedTime(HANDLE handle, const Time& time) noexcept
			{
				FILETIME ft;
				TimeToFileTime(time, ft);
				BOOL bRet = SetFileTime(handle, NULL, NULL, &ft);
				return bRet != 0;
			}

			SLIB_INLINE static sl_bool SetAccessedTime(HANDLE handle, const Time& time) noexcept
			{
				FILETIME ft;
				TimeToFileTime(time, ft);
				BOOL bRet = SetFileTime(handle, NULL, &ft, NULL);
				return bRet != 0;
			}

			SLIB_INLINE static sl_bool SetCreatedTime(HANDLE handle, const Time& time) noexcept
			{
				FILETIME ft;
				TimeToFileTime(time, ft);
				BOOL bRet = SetFileTime(handle, &ft, NULL, NULL);
				return bRet != 0;
			}

		}
	}

	using namespace priv::file;

	sl_file File::_open(const StringParam& _filePath, const FileMode& mode, const FileAttributes& attrs) noexcept
	{
		DWORD dwShareMode = 0;
		DWORD dwDesiredAccess = 0;
		if (mode & FileMode::Write) {
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
			dwDesiredAccess |= GENERIC_READ;
		}
		if (mode & FileMode::Write) {
			if (mode & FileMode::WriteData) {
				dwDesiredAccess |= FILE_WRITE_DATA;
			}
			if (mode & FileMode::WriteAttrs) {
				dwDesiredAccess |= FILE_WRITE_ATTRIBUTES | FILE_WRITE_EA;
			}
			dwDesiredAccess |= GENERIC_WRITE;
			if (mode & FileMode::SeekToEnd) {
				dwDesiredAccess |= FILE_APPEND_DATA;
			}
		}
		if (mode & FileMode::Sync) {
			dwDesiredAccess |= SYNCHRONIZE;
		}

		if (mode & FileMode::Device) {
			return (sl_file)(Win32::createDeviceHandle(_filePath, dwDesiredAccess, dwShareMode));
		}

		DWORD dwCreateDisposition = 0;
		DWORD dwFlags = 0;
		if (mode & FileMode::Write) {
			if (mode & FileMode::NotCreate) {
				if (mode & FileMode::NotTruncate) {
					dwCreateDisposition = OPEN_EXISTING;
				} else {
					dwCreateDisposition = TRUNCATE_EXISTING;
				}
			} else {
				dwFlags = attrs & 0x7ffff;
				if (mode & FileMode::NotOverwrite) {
					dwCreateDisposition = CREATE_NEW;
				} else if (mode & FileMode::NotTruncate) {
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

		StringCstr16 filePath(_filePath);
		HANDLE handle = CreateFileW(
			(LPCWSTR)(filePath.getData()),
			dwDesiredAccess,
			dwShareMode,
			NULL,
			dwCreateDisposition,
			dwFlags,
			NULL
		);
		return handle;
	}

	sl_bool File::_close(sl_file handle) noexcept
	{
		if (handle != INVALID_HANDLE_VALUE) {
			CloseHandle(handle);
			return sl_true;
		}
		return sl_false;
	}

	sl_bool File::getPosition(sl_uint64& outPos) const noexcept
	{
		HANDLE handle = m_file;
		if (handle != INVALID_HANDLE_VALUE) {
			static LARGE_INTEGER zero = { 0 };
			if (SetFilePointerEx(handle, zero, ((LARGE_INTEGER*)&outPos), FILE_CURRENT)) {
				return sl_true;
			}
		}
		return sl_false;
	}

	sl_bool File::seek(sl_int64 location, SeekPosition from) const noexcept
	{
		HANDLE handle = m_file;
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

	sl_bool File::isEnd(sl_bool& outFlag) const noexcept
	{
		HANDLE handle = m_file;
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

	sl_int32 File::read32(void* buf, sl_uint32 size) const noexcept
	{
		HANDLE handle = m_file;
		if (handle != INVALID_HANDLE_VALUE) {
			if (!size) {
				return SLIB_IO_EMPTY_CONTENT;
			}
			sl_uint32 ret = 0;
			if (ReadFile(handle, buf, size, (DWORD*)&ret, NULL)) {
				if (ret) {
					return ret;
				} else {
					return SLIB_IO_ENDED;
				}
			}
		}
		return SLIB_IO_ERROR;
	}

	sl_bool File::waitRead(sl_int32 timeout) const noexcept
	{
		Thread::sleep(1);
		return sl_true;
	}

	sl_int32 File::write32(const void* buf, sl_uint32 size) const noexcept
	{
		HANDLE handle = m_file;
		if (handle != INVALID_HANDLE_VALUE) {
			sl_uint32 ret = 0;
			if (WriteFile(handle, (LPVOID)buf, size, (DWORD*)&ret, NULL)) {
				if (ret) {
					return ret;
				} else {
					return SLIB_IO_EMPTY_CONTENT;
				}
			}
		}
		return SLIB_IO_ERROR;
	}

	sl_bool File::waitWrite(sl_int32 timeout) const noexcept
	{
		Thread::sleep(1);
		return sl_true;
	}

	sl_bool File::setSize(sl_uint64 size) const noexcept
	{
		HANDLE handle = m_file;
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

	sl_bool File::getSize(sl_uint64& outSize) const noexcept
	{
		HANDLE handle = m_file;
		if (handle != SLIB_FILE_INVALID_HANDLE) {
			if (GetFileSizeEx(handle, (PLARGE_INTEGER)(&outSize))) {
				return sl_true;
			}
		}
		return sl_false;
	}

	sl_bool File::getSize(const StringParam& _filePath, sl_uint64& outSize) noexcept
	{
		StringCstr16 filePath(_filePath);
		if (filePath.isEmpty()) {
			return sl_false;
		}
		HANDLE handle = CreateFileW((LPCWSTR)(filePath.getData()), 0, FILE_SHARE_ALL, NULL, OPEN_EXISTING, 0, NULL);
		if (handle != INVALID_HANDLE_VALUE) {
			sl_bool bRet = sl_false;
			if (GetFileSizeEx(handle, (PLARGE_INTEGER)(&outSize))) {
				bRet = sl_true;
			}
			CloseHandle(handle);
			return bRet;
		}
		return sl_false;
	}

	sl_bool File::getDiskSize(sl_uint64& outSize) const noexcept
	{
		HANDLE handle = m_file;
		if (handle != SLIB_FILE_INVALID_HANDLE) {
			sl_uint64 size = 0;
			DWORD nOutput;
			if (DeviceIoControl(handle, IOCTL_DISK_GET_LENGTH_INFO, NULL, 0, &size, sizeof(size), &nOutput, NULL)) {
				outSize = size;
				return sl_true;
			}
		}
		return sl_false;
	}

	sl_bool File::lock() const noexcept
	{
		HANDLE handle = m_file;
		if (handle != INVALID_HANDLE_VALUE) {
			OVERLAPPED o;
			ZeroMemory(&o, sizeof(o));
			if (LockFileEx(handle, LOCKFILE_EXCLUSIVE_LOCK | LOCKFILE_FAIL_IMMEDIATELY, 0, 1, 0, &o)) {
				return sl_true;
			}
		}
		return sl_false;
	}

	sl_bool File::unlock() const noexcept
	{
		HANDLE handle = m_file;
		if (handle != INVALID_HANDLE_VALUE) {
			OVERLAPPED o;
			ZeroMemory(&o, sizeof(o));
			if (UnlockFileEx(handle, 0, 0, 0, &o)) {
				return sl_true;
			}
		}
		return sl_false;
	}

	sl_bool File::flush() const noexcept
	{
		HANDLE handle = m_file;
		if (handle != INVALID_HANDLE_VALUE) {
			if (FlushFileBuffers(handle)) {
				return sl_true;
			}
		}
		return sl_false;
	}

	sl_bool File::setNonBlocking(sl_bool flag) const noexcept
	{
		return sl_false;
	}

	Time File::getModifiedTime() const noexcept
	{
		HANDLE handle = m_file;
		if (handle != INVALID_HANDLE_VALUE) {
			return GetModifiedTime(handle);
		} else {
			return Time::zero();
		}
	}

	Time File::getModifiedTime(const StringParam& _filePath) noexcept
	{
		StringCstr16 filePath(_filePath);
		if (filePath.isEmpty()) {
			return Time::zero();
		}
		HANDLE handle = CreateFileW((LPCWSTR)(filePath.getData()), FILE_READ_ATTRIBUTES, FILE_SHARE_ALL, NULL, OPEN_EXISTING,
			FILE_FLAG_BACKUP_SEMANTICS, NULL);
		if (handle != INVALID_HANDLE_VALUE) {
			Time ret = GetModifiedTime(handle);
			CloseHandle(handle);
			return ret;
		} else {
			return Time::zero();
		}
	}

	Time File::getAccessedTime() const noexcept
	{
		HANDLE handle = m_file;
		if (handle != INVALID_HANDLE_VALUE) {
			return GetAccessedTime(handle);
		} else {
			return Time::zero();
		}
	}

	Time File::getAccessedTime(const StringParam& _filePath) noexcept
	{
		StringCstr16 filePath(_filePath);
		if (filePath.isEmpty()) {
			return Time::zero();
		}
		HANDLE handle = CreateFileW((LPCWSTR)(filePath.getData()), FILE_READ_ATTRIBUTES, FILE_SHARE_ALL, NULL, OPEN_EXISTING,
			FILE_FLAG_BACKUP_SEMANTICS, NULL);
		if (handle != INVALID_HANDLE_VALUE) {
			Time ret = GetAccessedTime(handle);
			CloseHandle(handle);
			return ret;
		} else {
			return Time::zero();
		}
	}

	Time File::getCreatedTime() const noexcept
	{
		HANDLE handle = m_file;
		if (handle != INVALID_HANDLE_VALUE) {
			return GetCreatedTime(handle);
		} else {
			return Time::zero();
		}
	}

	Time File::getCreatedTime(const StringParam& _filePath) noexcept
	{
		StringCstr16 filePath(_filePath);
		if (filePath.isEmpty()) {
			return Time::zero();
		}
		HANDLE handle = CreateFileW((LPCWSTR)(filePath.getData()), FILE_READ_ATTRIBUTES, FILE_SHARE_ALL, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
		if (handle != INVALID_HANDLE_VALUE) {
			Time ret = GetCreatedTime(handle);
			CloseHandle(handle);
			return ret;
		} else {
			return Time::zero();
		}
	}

	sl_bool File::setModifiedTime(const Time& time) const noexcept
	{
		HANDLE handle = m_file;
		if (handle != INVALID_HANDLE_VALUE) {
			return SetModifiedTime(handle, time);
		} else {
			return sl_false;
		}
	}

	sl_bool File::setModifiedTime(const StringParam& _filePath, const Time& time) noexcept
	{
		StringCstr16 filePath(_filePath);
		if (filePath.isEmpty()) {
			return sl_false;
		}
		HANDLE handle = CreateFileW((LPCWSTR)(filePath.getData()), FILE_WRITE_ATTRIBUTES, FILE_SHARE_ALL, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
		if (handle != INVALID_HANDLE_VALUE) {
			sl_bool ret = SetModifiedTime(handle, time);
			CloseHandle(handle);
			return ret;
		} else {
			return sl_false;
		}
	}

	sl_bool File::setAccessedTime(const Time& time) const noexcept
	{
		HANDLE handle = m_file;
		if (handle != INVALID_HANDLE_VALUE) {
			return SetAccessedTime(handle, time);
		} else {
			return sl_false;
		}
	}

	sl_bool File::setAccessedTime(const StringParam& _filePath, const Time& time) noexcept
	{
		StringCstr16 filePath(_filePath);
		if (filePath.isEmpty()) {
			return sl_false;
		}
		HANDLE handle = CreateFileW((LPCWSTR)(filePath.getData()), FILE_WRITE_ATTRIBUTES, FILE_SHARE_ALL, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
		if (handle != INVALID_HANDLE_VALUE) {
			sl_bool ret = SetAccessedTime(handle, time);
			CloseHandle(handle);
			return ret;
		} else {
			return sl_false;
		}
	}

	sl_bool File::setCreatedTime(const Time& time) const noexcept
	{
		HANDLE handle = m_file;
		if (handle != INVALID_HANDLE_VALUE) {
			return SetCreatedTime(handle, time);
		} else {
			return sl_false;
		}
	}

	sl_bool File::setCreatedTime(const StringParam& _filePath, const Time& time) noexcept
	{
		StringCstr16 filePath(_filePath);
		if (filePath.isEmpty()) {
			return sl_false;
		}
		HANDLE handle = CreateFileW((LPCWSTR)(filePath.getData()), FILE_WRITE_ATTRIBUTES, FILE_SHARE_ALL, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
		if (handle != INVALID_HANDLE_VALUE) {
			sl_bool ret = SetCreatedTime(handle, time);
			CloseHandle(handle);
			return ret;
		} else {
			return sl_false;
		}
	}

	FileAttributes File::_getAttributes() const noexcept
	{
		HANDLE handle = m_file;
		if (handle != INVALID_HANDLE_VALUE) {
			BY_HANDLE_FILE_INFORMATION info;
			if (GetFileInformationByHandle(handle, &info)) {
				return info.dwFileAttributes & 0x7ffff;
			}
		}
		return FileAttributes::NotExist;
	}

	FileAttributes File::_getAttributes(const StringParam& _filePath) noexcept
	{
		StringCstr16 filePath(_filePath);
		DWORD attr = GetFileAttributesW((LPCWSTR)(filePath.getData()));
		if (attr == 0xffffffff) {
			return FileAttributes::NotExist;
		} else {
			return (int)(attr & 0x7ffff);
		}
	}

	sl_bool File::_setAttributes(const StringParam& _filePath, const FileAttributes& attrs) noexcept
	{
		StringCstr16 filePath(_filePath);
		if (filePath.isEmpty()) {
			return sl_false;
		}
		return SetFileAttributesW((LPCWSTR)(filePath.getData()), (DWORD)(attrs.value & 0x7ffff)) != 0;
	}

	sl_bool File::_createDirectory(const StringParam& _filePath) noexcept
	{
		StringCstr16 filePath(_filePath);
		if (filePath.isEmpty()) {
			return sl_false;
		}
		BOOL ret = CreateDirectoryW((LPCWSTR)(filePath.getData()), NULL);
		return ret != 0;
	}

	sl_bool File::createLink(const StringParam& _pathTarget, const StringParam& _pathLink) noexcept
	{
		CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
		IShellLinkW* psl = NULL;
		HRESULT hr = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLinkW, (LPVOID*)&psl);
		if (SUCCEEDED(hr)) {
			StringCstr16 pathTarget(_pathTarget);
			psl->SetPath((LPCWSTR)(pathTarget.getData()));
			StringCstr16 workDir(getParentDirectoryPath(pathTarget));
			psl->SetWorkingDirectory((LPCWSTR)(workDir.getData()));
			IPersistFile* ppf = NULL;
			hr = psl->QueryInterface(IID_IPersistFile, (LPVOID*)&ppf);
			if (SUCCEEDED(hr)) {
				StringCstr16 pathLink(_pathLink);
				hr = ppf->Save((LPCWSTR)(pathLink.getData()), TRUE);
				ppf->Release();
			}
			psl->Release();
		}
		return SUCCEEDED(hr);
	}

	sl_bool File::deleteFile(const StringParam& _filePath) noexcept
	{
		StringCstr16 filePath(_filePath);
		if (filePath.isEmpty()) {
			return sl_false;
		}
		BOOL ret = DeleteFileW((LPCWSTR)(filePath.getData()));
		return ret != 0;
	}

	sl_bool File::deleteDirectory(const StringParam& _filePath) noexcept
	{
		String filePath = _filePath.toString();
		if (filePath.isEmpty()) {
			return sl_false;
		}
		String16 dirPath = String16::from(normalizeDirectoryPath(filePath));
		BOOL ret = RemoveDirectoryW((LPCWSTR)(dirPath.getData()));
		return ret != 0;
	}

	sl_bool File::_copyFile(const StringParam& _pathSrc, const StringParam& _pathDst) noexcept
	{
		StringCstr16 pathSrc(_pathSrc);
		if (pathSrc.isEmpty()) {
			return sl_false;
		}
		StringCstr16 pathDst(_pathDst);
		if (pathDst.isEmpty()) {
			return sl_false;
		}
		BOOL ret = CopyFileW((LPCWSTR)(pathSrc.getData()), (LPCWSTR)(pathDst.getData()), FALSE);
		return ret != 0;
	}

	sl_bool File::_move(const StringParam& _oldPath, const StringParam& _newPath) noexcept
	{
		StringCstr16 oldPath(_oldPath);
		if (oldPath.isEmpty()) {
			return sl_false;
		}
		StringCstr16 newPath(_newPath);
		if (newPath.isEmpty()) {
			return sl_false;
		}
		BOOL ret = MoveFileExW((LPCWSTR)(oldPath.getData()), (LPCWSTR)(newPath.getData()), MOVEFILE_REPLACE_EXISTING);
		return ret != 0;
	}

	List<String> File::getFiles(const StringParam& _filePath) noexcept
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

	HashMap<String, FileInfo> File::getFileInfos(const StringParam& _filePath) noexcept
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

	String File::getRealPath(const StringParam& _filePath) noexcept
	{
		StringCstr16 path(_filePath);
		WCHAR buf[4096];
		buf[0] = 0;
		if (GetFullPathNameW((LPCWSTR)(path.getData()), 4096, buf, NULL)) {
			return String::create(buf);
		}
		return sl_null;
	}

	String File::getOwnerName(const StringParam& filePath) noexcept
	{
		return sl_null;
	}

	sl_bool File::setOwnerName(const StringParam& filePath, const StringParam& owner) noexcept
	{
		return sl_false;
	}

	String File::getGroupName(const StringParam& filePath) noexcept
	{
		return sl_null;
	}

	sl_bool File::setGroupName(const StringParam& filePath, const StringParam& group) noexcept
	{
		return sl_false;
	}


	DisableWow64FsRedirectionScope::DisableWow64FsRedirectionScope() noexcept
	{
		auto func = kernel32::getApi_Wow64DisableWow64FsRedirection();
		if (func) {
			func(&m_pOldValue);
		}
	}

	DisableWow64FsRedirectionScope::~DisableWow64FsRedirectionScope() noexcept
	{
		auto func = kernel32::getApi_Wow64RevertWow64FsRedirection();
		if (func) {
			func(m_pOldValue);
		}
	}

}

#endif
