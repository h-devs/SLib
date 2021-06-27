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

#ifdef SLIB_PLATFORM_IS_UNIX

#include "slib/core/file.h"

#include "slib/core/hash_map.h"

#define _FILE_OFFSET_BITS 64
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/time.h>
#if defined(SLIB_PLATFORM_IS_DESKTOP)
#	include <sys/ioctl.h>
#	if defined(SLIB_PLATFORM_IS_MACOS)
#		include <sys/disk.h>
#	elif defined(SLIB_PLATFORM_IS_LINUX)
#		include <linux/fs.h>
#	endif
#endif
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <pwd.h>
#include <errno.h>
#if defined(__APPLE__) || defined(__FreeBSD__)
#	include <copyfile.h>
#elif !defined(SLIB_PLATFORM_IS_ANDROID)
#	include <sys/sendfile.h>
#endif

#if defined(SLIB_PLATFORM_IS_LINUX)
	typedef off64_t _off_t;
	#define _lseek lseek64
#else
	typedef off_t _off_t;
	#define _lseek lseek
#endif

namespace slib
{

	namespace priv
	{
		namespace file
		{
			
			static sl_int64 GetModifiedTime(struct stat& st) noexcept
			{
#if defined(SLIB_PLATFORM_IS_APPLE)
				sl_int64 t = st.st_mtimespec.tv_sec;
				t *= 1000000;
				t += st.st_mtimespec.tv_nsec / 1000;
#elif defined(SLIB_PLATFORM_IS_ANDROID)
				sl_int64 t = st.st_mtime;
				t *= 1000000;
				t += st.st_mtime_nsec / 1000;
#else
				sl_int64 t = st.st_mtim.tv_sec;
				t *= 1000000;
				t += st.st_mtim.tv_nsec / 1000;
#endif
				return t;
			}
			
			static sl_int64 GetAccessedTime(struct stat& st) noexcept
			{
#if defined(SLIB_PLATFORM_IS_APPLE)
				sl_int64 t = st.st_atimespec.tv_sec;
				t *= 1000000;
				t += st.st_atimespec.tv_nsec / 1000;
#elif defined(SLIB_PLATFORM_IS_ANDROID)
				sl_int64 t = st.st_atime;
				t *= 1000000;
				t += st.st_atime_nsec / 1000;
#else
				sl_int64 t = st.st_atim.tv_sec;
				t *= 1000000;
				t += st.st_atim.tv_nsec / 1000;
#endif
				return t;
			}
			
			static sl_int64 GetCreatedTime(struct stat& st) noexcept
			{
#if defined(SLIB_PLATFORM_IS_APPLE)
				sl_int64 t = st.st_ctimespec.tv_sec;
				t *= 1000000;
				t += st.st_ctimespec.tv_nsec / 1000;
#elif defined(SLIB_PLATFORM_IS_ANDROID)
				sl_int64 t = st.st_ctime;
				t *= 1000000;
				t += st.st_ctime_nsec / 1000;
#else
				sl_int64 t = st.st_ctim.tv_sec;
				t *= 1000000;
				t += st.st_ctim.tv_nsec / 1000;
#endif
				return t;
			}

			static sl_bool SetAccessedAndModifiedTime(const StringParam& _filePath, const Time& timeAccess, const Time& timeModify) noexcept
			{
				StringCstr filePath(_filePath);
				if (filePath.isEmpty()) {
					return sl_false;
				}
				timeval t[2];
				t[0].tv_sec = (int)(timeAccess.toInt() / 1000000);
				t[0].tv_usec = (int)(timeAccess.toInt() % 1000000);
				t[1].tv_sec = (int)(timeModify.toInt() / 1000000);
				t[1].tv_usec = (int)(timeModify.toInt() % 1000000);
				return utimes(filePath.getData(), t) == 0;
			}

		}
	}

	using namespace priv::file;
	
	sl_file File::_open(const StringParam& _filePath, const FileMode& mode, const FileAttributes& attrs) noexcept
	{
		StringCstr filePath(_filePath);
		if (filePath.isEmpty()) {
			return -1;
		}
		
		int flags = 0;
		if (mode & FileMode::Write) {
			if (mode & FileMode::Read) {
				flags = O_RDWR;
			} else {
				flags = O_WRONLY;
			}
			if (!(mode & FileMode::NotTruncate)) {
				flags |= O_TRUNC;
			}
			if (!(mode & FileMode::NotCreate)) {
				flags |= O_CREAT;
			}
		} else {
			flags = O_RDONLY;
		}
		
		int perm = 0;
		if (flags & O_CREAT) {
			if (attrs & FileAttributes::ReadByOthers) {
				perm |= S_IROTH;
			}
			if (attrs & FileAttributes::WriteByOthers) {
				perm |= S_IWOTH;
			}
			if (attrs & FileAttributes::ExecuteByOthers) {
				perm |= S_IXOTH;
			}
			if (attrs & FileAttributes::ReadByGroup) {
				perm |= S_IRGRP;
			}
			if (attrs & FileAttributes::WriteByGroup) {
				perm |= S_IWGRP;
			}
			if (attrs & FileAttributes::ExecuteByGroup) {
				perm |= S_IXGRP;
			}
			if (attrs & FileAttributes::ReadByUser) {
				perm |= S_IRUSR;
			}
			if (attrs & FileAttributes::WriteByUser) {
				perm |= S_IWUSR;
			}
			if (attrs & FileAttributes::ExecuteByUser) {
				perm |= S_IXUSR;
			}
		}
		
		int fd = ::open(filePath.getData(), flags, perm);
		return fd;
	}

	sl_bool File::_close(sl_file fd) noexcept
	{
		if (fd != -1) {
			::close(fd);
			return sl_true;
		}
		return sl_false;
	}

	sl_bool File::getPosition(sl_uint64& outPos) const noexcept
	{
		int fd = m_file;
		if (fd != SLIB_FILE_INVALID_HANDLE) {
			_off_t pos = _lseek(fd, 0, SEEK_CUR);
			if (pos != -1) {
				outPos = pos;
				return sl_true;
			}
		}
		return sl_false;
	}

	sl_bool File::seek(sl_int64 pos, SeekPosition from) const noexcept
	{
		int fd = m_file;
		if (fd != SLIB_FILE_INVALID_HANDLE) {
			int origin = SEEK_SET;
			if (from == SeekPosition::Begin) {
				origin = SEEK_SET;
			} else if (from == SeekPosition::Current) {
				origin = SEEK_CUR;
			} else if (from == SeekPosition::End) {
				origin = SEEK_END;
			} else {
				return sl_false;
			}
			_off_t ret = _lseek(fd, pos, origin);
			if (ret != -1) {
				return sl_true;
			}
		}
		return sl_false;
	}

	sl_bool File::isEnd(sl_bool& outFlag) const noexcept
	{
		int fd = m_file;
		if (fd != SLIB_FILE_INVALID_HANDLE) {
			_off_t pos = _lseek(fd, 0, SEEK_CUR);
			if (pos != -1) {
				_off_t end = _lseek(fd, 0, SEEK_END);
				if (end != -1) {
					if (pos == end) {
						outFlag = sl_true;
					} else {
						outFlag = sl_false;
						_lseek(fd, pos, SEEK_SET);
					}
					return sl_true;
				}
			}
		}
		return sl_false;
	}

	sl_int32 File::read32(void* buf, sl_uint32 size) const noexcept
	{
		int fd = m_file;
		if (fd != SLIB_FILE_INVALID_HANDLE) {
			if (!size) {
				return 0;
			}
			ssize_t n = ::read(fd, buf, size);
			if (n >= 0) {
				if (n > 0) {
					return (sl_int32)n;
				}
			} else {
				int err = errno;
				if (err == EAGAIN || err == EWOULDBLOCK) {
					return 0;
				}
			}
		}
		return -1;
	}

	sl_int32 File::write32(const void* buf, sl_uint32 size) const noexcept
	{
		int fd = m_file;
		if (fd != SLIB_FILE_INVALID_HANDLE) {
			if (!size) {
				return 0;
			}
			ssize_t n = ::write(fd, buf, size);
			if (n >= 0) {
				if (n > 0) {
					return (sl_int32)n;
				}
			} else {
				int err = errno;
				if (err == EAGAIN || err == EWOULDBLOCK) {
					return 0;
				}
			}
		}
		return -1;
	}

	sl_bool File::setSize(sl_uint64 newSize) const noexcept
	{
		int fd = m_file;
		if (fd != SLIB_FILE_INVALID_HANDLE) {
			return 0 == ftruncate(fd, newSize);
		}
		return sl_false;
	}

	sl_bool File::getSizeByHandle(sl_file fd, sl_uint64& outSize) noexcept
	{
		if (fd != -1) {
			struct stat st;
			if (!(fstat(fd, &st))) {
				outSize = st.st_size;
				return sl_true;
			}
		}
		return sl_false;
	}
	
	sl_bool File::getSize(const StringParam& _filePath, sl_uint64& outSize) noexcept
	{
		StringCstr filePath(_filePath);
		if (filePath.isEmpty()) {
			return sl_false;
		}
		struct stat st;
		if (!(stat(filePath.getData(), &st))) {
			outSize = st.st_size;
			return sl_true;
		}
		return sl_false;
	}
	
	sl_bool File::getDiskSizeByHandle(sl_file fd, sl_uint64& outSize) noexcept
	{
#if defined(SLIB_PLATFORM_IS_DESKTOP)
#	if defined(SLIB_PLATFORM_IS_MACOS)
		if (fd != SLIB_FILE_INVALID_HANDLE) {
			sl_uint64 nSectors = 0;
			ioctl(fd, DKIOCGETBLOCKCOUNT, &nSectors);
			sl_uint32 nSectorSize = 0;
			ioctl(fd, DKIOCGETBLOCKSIZE, &nSectorSize);
			outSize = nSectorSize * nSectors;
			return sl_true;
		}
#	elif defined(SLIB_PLATFORM_IS_LINUX)
		if (fd != SLIB_FILE_INVALID_HANDLE) {
			ioctl(fd, BLKGETSIZE64, &outSize);
			return sl_true;
		}
#	endif
#endif
		return sl_false;
	}
	
	sl_bool File::lock() const noexcept
	{
		int fd = m_file;
		if (fd != SLIB_FILE_INVALID_HANDLE) {
			struct flock fl;
			Base::zeroMemory(&fl, sizeof(fl));
			fl.l_start = 0;
			fl.l_len = 0;
			fl.l_type = F_WRLCK;
			fl.l_whence = SEEK_SET;
			if (fcntl(fd, F_SETLK, &fl) >= 0) {
				return sl_true;
			}
		}
		return sl_false;
	}
	
	sl_bool File::unlock() const noexcept
	{
		int fd = m_file;
		if (fd != SLIB_FILE_INVALID_HANDLE) {
			struct flock fl;
			Base::zeroMemory(&fl, sizeof(fl));
			fl.l_start = 0;
			fl.l_len = 0;
			fl.l_type = F_UNLCK;
			fl.l_whence = SEEK_SET;
			if (fcntl(fd, F_SETLK, &fl) >= 0) {
				return sl_true;
			}
		}
		return sl_false;
	}
	
	sl_bool File::flush() const noexcept
	{
		int fd = m_file;
		if (fd != SLIB_FILE_INVALID_HANDLE) {
			return 0 == fsync(fd);
		}
		return sl_false;
	}

	Time File::getModifiedTime() const noexcept
	{
		int fd = m_file;
		if (fd != SLIB_FILE_INVALID_HANDLE) {
			struct stat st;
			if (0 == fstat(fd, &st)) {
				return GetModifiedTime(st);
			}
		}
		return Time::zero();
	}

	Time File::getModifiedTime(const StringParam& _filePath) noexcept
	{
		StringCstr filePath(_filePath);
		if (filePath.isEmpty()) {
			return Time::zero();
		}
		struct stat st;
		if (0 == stat(filePath.getData(), &st)) {
			return GetModifiedTime(st);
		} else {
			return Time::zero();
		}
	}

	Time File::getAccessedTime() const noexcept
	{
		int fd = m_file;
		if (fd != SLIB_FILE_INVALID_HANDLE) {
			struct stat st;
			if (0 == fstat(fd, &st)) {
				return GetAccessedTime(st);
			}
		}
		return Time::zero();
	}

	Time File::getAccessedTime(const StringParam& _filePath) noexcept
	{
		StringCstr filePath(_filePath);
		if (filePath.isEmpty()) {
			return Time::zero();
		}
		struct stat st;
		if (0 == stat(filePath.getData(), &st)) {
			return GetAccessedTime(st);
		} else {
			return Time::zero();
		}
	}

	Time File::getCreatedTime() const noexcept
	{
		int fd = m_file;
		if (fd != SLIB_FILE_INVALID_HANDLE) {
			struct stat st;
			if (0 == fstat(fd, &st)) {
				return GetCreatedTime(st);
			}
		}
		return Time::zero();
	}

	Time File::getCreatedTime(const StringParam& _filePath) noexcept
	{
		StringCstr filePath(_filePath);
		if (filePath.isEmpty()) {
			return Time::zero();
		}
		struct stat st;
		if (0 == stat(filePath.getData(), &st)) {
			return GetCreatedTime(st);
		} else {
			return Time::zero();
		}
	}
	
	sl_bool File::setModifiedTime(const Time& time) const noexcept
	{
		// not supported
		return sl_false;
	}

	sl_bool File::setAccessedTime(const Time& time) const noexcept
	{
		// not supported
		return sl_false;
	}

	sl_bool File::setCreatedTime(const Time& time) const noexcept
	{
		// not supported
		return sl_false;
	}

	sl_bool File::setModifiedTime(const StringParam& _filePath, const Time& time) noexcept
	{
		StringCstr filePath(_filePath);
		Time timeAccess = getAccessedTime(filePath);
		return SetAccessedAndModifiedTime(filePath, timeAccess, time);
	}

	sl_bool File::setAccessedTime(const StringParam& _filePath, const Time& time) noexcept
	{
		StringCstr filePath(_filePath);
		Time timeModify = getModifiedTime(filePath);
		return SetAccessedAndModifiedTime(filePath, time, timeModify);
	}

	sl_bool File::setCreatedTime(const StringParam& filePath, const Time& time) noexcept
	{
		// not supported
		return sl_false;
	}

	FileAttributes File::getAttributes() const noexcept
	{
		int fd = m_file;
		if (fd != SLIB_FILE_INVALID_HANDLE) {
			struct stat st;
			if (0 == fstat(fd, &st)) {
				int ret = 0;
				if (S_ISDIR(st.st_mode)) {
					ret |= FileAttributes::Directory;
				} else {
					ret |= FileAttributes::Normal;
				}
				return ret;
			}
		}
		return FileAttributes::NotExist;
	}

	FileAttributes File::_getAttributes(const StringParam& _filePath) noexcept
	{
		StringCstr filePath(_filePath);
		if (filePath.isEmpty()) {
			return FileAttributes::NotExist;
		}
		struct stat st;
		if (0 == stat(filePath.getData(), &st)) {
			int ret = 0;
			if (S_ISDIR(st.st_mode)) {
				ret |= FileAttributes::Directory;
			}
			if (filePath.startsWith('.')) {
				ret |= FileAttributes::Hidden;
			}
			return ret;
		} else {
			return FileAttributes::NotExist;
		}
	}

	sl_bool File::_setAttributes(const StringParam& _filePath, const FileAttributes& attrs) noexcept
	{
		// not supported
		return sl_false;
	}

	List<String> File::getFiles(const StringParam& _filePath) noexcept
	{
		String filePath = _filePath.toString();
		if (filePath.isEmpty()) {
			return sl_null;
		}
		if (File::isDirectory(filePath)) {
			filePath = normalizeDirectoryPath(filePath);
		} else {
			return sl_null;
		}
		List<String> ret;
		StringCstr dirPath(filePath);
		DIR* dir = opendir(dirPath.getData());
		if (dir) {
			dirent* ent;
			while ((ent = readdir(dir))) {
				ret.add_NoLock(String::fromUtf8(ent->d_name));
			}
			closedir(dir);
		}
		return ret;
	}

	HashMap<String, FileInfo> File::getFileInfos(const StringParam& _filePath) noexcept
	{
		String filePath = _filePath.toString();
		if (filePath.isEmpty()) {
			return sl_null;
		}
		if (File::isDirectory(filePath)) {
			filePath = normalizeDirectoryPath(filePath);
		} else {
			return sl_null;
		}
		HashMap<String, FileInfo> ret;
		StringCstr dirPath(filePath);
		DIR* dir = opendir(dirPath.getData());
		if (dir) {
			dirent* ent;
			while ((ent = readdir(dir))) {
				FileInfo info;
				filePath = String::join(dirPath, "/", String::fromUtf8(ent->d_name));
				info.attributes = File::getAttributes(filePath);
				info.size = info.allocSize = File::getSize(filePath);
				info.createdAt = File::getCreatedTime(filePath);
				info.modifiedAt = File::getModifiedTime(filePath);
				info.accessedAt = File::getAccessedTime(filePath);
				ret.add_NoLock(String::fromUtf8(ent->d_name), info);
			}
			closedir(dir);
		}
		return ret;
	}

	sl_bool File::_createDirectory(const StringParam& _filePath) noexcept
	{
		StringCstr filePath(_filePath);
		if (filePath.isEmpty()) {
			return sl_false;
		}
		return 0 == mkdir(filePath.getData(), 0777);
	}

	sl_bool File::deleteFile(const StringParam& _filePath) noexcept
	{
		StringCstr filePath(_filePath);
		if (filePath.isEmpty()) {
			return sl_false;
		}
		return 0 == ::remove(filePath.getData());
	}

	sl_bool File::deleteDirectory(const StringParam& _filePath) noexcept
	{
		String filePath = _filePath.toString();
		if (filePath.isEmpty()) {
			return sl_false;
		}
		StringCstr dirPath(normalizeDirectoryPath(filePath));
		return 0 == rmdir(dirPath.getData());
	}

	sl_bool File::_copyFile(const StringParam& _pathSrc, const StringParam& _pathDst) noexcept
	{
		StringCstr pathSrc(_pathSrc);
		if (pathSrc.isEmpty()) {
			return sl_false;
		}
		StringCstr pathDst(_pathDst);
		if (pathDst.isEmpty()) {
			return sl_false;
		}
#if defined(__APPLE__) || defined(__FreeBSD__)
		return copyfile(pathSrc.getData(), pathDst.getData(), sl_null, COPYFILE_ALL) == 0;
#else
		sl_bool bRet = sl_false;
		int handleSrc = ::open(pathSrc.getData(), O_RDONLY);
		if (handleSrc != -1) {
			int handleDst = ::open(pathDst.getData(), O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
			if (handleDst != -1) {
				int iRet;
				struct stat statSrc;
				while ((iRet = fstat(handleSrc, &statSrc)) < 0 && errno == EINTR);
				if (!iRet) {
					sl_int64 size = statSrc.st_size;
#ifdef SLIB_PLATFORM_IS_ANDROID
					if (size) {
#else
					bRet = sl_true;
					while (size > 0) {
						ssize_t sent = sendfile(handleDst, handleSrc, sl_null, size > 0x7ffff000 ? 0x7ffff000 : (size_t)size);
						if (sent < 0) {
							if (errno != EINVAL && errno != ENOSYS) {
								bRet = sl_false;
							}
							break;
						} else {
							size -= sent;
						}
					}
					if (bRet && size) {
#endif
#define BUF_SIZE 0x40000
						Memory memBuf = Memory::create(BUF_SIZE);
						if (memBuf.isNull()) {
							bRet = sl_false;
						} else {
							char* buf = (char*)(memBuf.getData());
							while (size > 0) {
								ssize_t nRead = ::read(handleSrc, buf, size > BUF_SIZE ? BUF_SIZE : (size_t)size);
								if (nRead < 0) {
									if (errno != EINTR) {
										bRet = sl_false;
										break;
									}
								} else {
									char* p = buf;
									while (nRead > 0) {
										ssize_t nWrite = ::write(handleDst, p, nRead);
										if (nWrite < 0) {
											if (errno != EINTR) {
												break;
											}
										} else {
											nRead -= nWrite;
											p += nWrite;
										}
									}
									if (nRead) {
										bRet = sl_false;
										break;
									}
								}
							}
						}
#undef BUF_SIZE
					}
				}
				::close(handleDst);
			}
			::close(handleSrc);
		}
		return bRet;
#endif
	}

	sl_bool File::_move(const StringParam& _oldPath, const StringParam& _newPath) noexcept
	{
		StringCstr oldPath(_oldPath);
		if (oldPath.isEmpty()) {
			return sl_false;
		}
		StringCstr newPath(_newPath);
		if (newPath.isEmpty()) {
			return sl_false;
		}
		return 0 == ::rename(oldPath.getData(), newPath.getData());
	}

	sl_bool File::setNonBlocking(int fd, sl_bool flagEnable) noexcept
	{
		int flag = fcntl(fd, F_GETFL, 0);
		if (flag != -1) {
			if (flagEnable) {
				flag |= O_NONBLOCK;
			} else {
				flag = flag & ~O_NONBLOCK;
			}
			int ret = fcntl(fd, F_SETFL, flag);
			return ret == 0;
		} else {
			return sl_false;
		}
	}
	
	String File::getRealPath(const StringParam& _filePath) noexcept
	{
		StringCstr filePath(_filePath);
		if (filePath.isEmpty()) {
			return sl_null;
		}
		char path[4096];
		path[0] = 0;
		return realpath(filePath.getData(), path);
	}

	String File::getOwnerName(const StringParam& _filePath) noexcept
	{
		StringCstr filePath(_filePath);
		if (filePath.isNotEmpty()) {
			struct stat st;
			if (0 == stat(filePath.getData(), &st)) {
				passwd* pw = getpwuid(st.st_uid);
				if (pw) {
					return pw->pw_name;
				}
			}
		}
		return sl_null;
	}

}

#endif
