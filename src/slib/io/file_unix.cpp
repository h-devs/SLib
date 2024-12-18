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

#define _FILE_OFFSET_BITS 64

#include "slib/io/definition.h"

#ifdef SLIB_PLATFORM_IS_UNIX

#include "slib/io/file.h"

#include "slib/core/string.h"
#include "slib/core/memory.h"
#include "slib/core/hash_map.h"
#include "slib/io/pipe_event.h"

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
#		include "slib/dl/linux/cap.h"
#	endif
#endif
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <pwd.h>
#include <grp.h>
#include <errno.h>
#if defined(__APPLE__) || defined(__FreeBSD__)
#	include <copyfile.h>
#elif !defined(SLIB_PLATFORM_IS_ANDROID)
#	include <sys/sendfile.h>
#endif
#if defined(SLIB_PLATFORM_IS_LINUX_DESKTOP)
#	include "slib/dl/linux/libc.h"
#endif
#ifndef F_SETLK64
#	define flock64 flock
#	define F_SETLK64 F_SETLK
#	define F_SETLKW64 F_SETLKW
#endif

namespace slib
{

	namespace
	{
		static int GetFilePermissions(const FileAttributes& attrs)
		{
			int perm = 0;
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
			return perm;
		}
	}

	sl_file File::_open(const StringParam& _filePath, const FileMode& mode, const FileAttributes& attrs) noexcept
	{
		StringCstr filePath(_filePath);
		if (filePath.isEmpty()) {
			return SLIB_FILE_INVALID_HANDLE;
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
				if (mode & FileMode::NotOverwrite) {
					flags |= O_EXCL;
				}
			}
		} else {
			flags = O_RDONLY;
		}

		int perm = 0;
		if (flags & O_CREAT) {
			perm = GetFilePermissions(attrs);
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
			off_t pos = lseek(fd, 0, SEEK_CUR);
			if (pos != (off_t)-1) {
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
			off_t ret = lseek(fd, pos, origin);
			if (ret != (off_t)-1) {
				return sl_true;
			}
		}
		return sl_false;
	}

	sl_bool File::isEnd(sl_bool& outFlag) const noexcept
	{
		int fd = m_file;
		if (fd != SLIB_FILE_INVALID_HANDLE) {
			off_t pos = lseek(fd, 0, SEEK_CUR);
			if (pos != (off_t)-1) {
				off_t end = lseek(fd, 0, SEEK_END);
				if (end != (off_t)-1) {
					if (pos == end) {
						outFlag = sl_true;
					} else {
						outFlag = sl_false;
						lseek(fd, pos, SEEK_SET);
					}
					return sl_true;
				}
			}
		}
		return sl_false;
	}

	sl_int32 File::read32(void* buf, sl_uint32 size, sl_int32 timeout) const noexcept
	{
		int fd = m_file;
		if (fd != SLIB_FILE_INVALID_HANDLE) {
			if (!size) {
				return SLIB_IO_EMPTY_CONTENT;
			}
			for (;;) {
				ssize_t n = ::read(fd, buf, size);
				if (n > 0) {
					return (sl_int32)n;
				}
				if (!n) {
					return SLIB_IO_ENDED;
				}
				int err = errno;
				if (err == EAGAIN || err == EWOULDBLOCK || err == EINTR) {
					if (!timeout) {
						return SLIB_IO_WOULD_BLOCK;
					}
					Ref<PipeEvent> ev = PipeEvent::create();
					if (ev.isNull()) {
						return SLIB_IO_ERROR;
					}
					if (ev->waitReadFd(fd, timeout)) {
						timeout = 0;
					} else {
						return SLIB_IO_TIMEOUT;
					}
				} else {
					return SLIB_IO_ERROR;
				}
			}
		}
		return SLIB_IO_ERROR;
	}

	sl_int32 File::write32(const void* buf, sl_uint32 size, sl_int32 timeout) const noexcept
	{
		int fd = m_file;
		if (fd != SLIB_FILE_INVALID_HANDLE) {
			for (;;) {
				ssize_t n = ::write(fd, buf, size);
				if (n > 0) {
					return (sl_int32)n;
				}
				if (!n) {
					return SLIB_IO_EMPTY_CONTENT;
				}
				int err = errno;
				if (err == EAGAIN || err == EWOULDBLOCK || err == EINTR) {
					if (!timeout) {
						return SLIB_IO_WOULD_BLOCK;
					}
					Ref<PipeEvent> ev = PipeEvent::create();
					if (ev.isNull()) {
						return SLIB_IO_ERROR;
					}
					if (ev->waitWriteFd(fd, timeout)) {
						if (!size) {
							return SLIB_IO_EMPTY_CONTENT;
						}
						timeout = 0;
					} else {
						return SLIB_IO_TIMEOUT;
					}
				} else {
					return SLIB_IO_ERROR;
				}
			}
		}
		return SLIB_IO_ERROR;
	}

	sl_bool File::setSize(sl_uint64 newSize) const noexcept
	{
		int fd = m_file;
		if (fd != SLIB_FILE_INVALID_HANDLE) {
			return !(ftruncate(fd, newSize));
		}
		return sl_false;
	}

	sl_bool File::getSize(sl_uint64& outSize) const noexcept
	{
		int fd = m_file;
		if (fd != SLIB_FILE_INVALID_HANDLE) {
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

	sl_bool File::lock(sl_uint64 offset, sl_uint64 length, sl_bool flagShared, sl_bool flagWait) const noexcept
	{
		int fd = m_file;
		if (fd != SLIB_FILE_INVALID_HANDLE) {
			struct flock64 fl = {0};
			fl.l_start = offset;
			fl.l_len = length;
			fl.l_type = flagShared ? F_RDLCK : F_WRLCK;
			fl.l_whence = SEEK_SET;
			if (flagWait) {
				if (fcntl(fd, F_SETLK64, &fl) >= 0) {
					return sl_true;
				}
			} else {
				if (fcntl(fd, F_SETLK64, &fl) >= 0) {
					return sl_true;
				}
			}
		}
		return sl_false;
	}

	sl_bool File::unlock(sl_uint64 offset, sl_uint64 length) const noexcept
	{
		int fd = m_file;
		if (fd != SLIB_FILE_INVALID_HANDLE) {
			struct flock64 fl = {0};
			fl.l_start = offset;
			fl.l_len = length;
			fl.l_type = F_UNLCK;
			fl.l_whence = SEEK_SET;
			if (fcntl(fd, F_SETLK64, &fl) >= 0) {
				return sl_true;
			}
		}
		return sl_false;
	}

	sl_bool File::flush() const noexcept
	{
		int fd = m_file;
		if (fd != SLIB_FILE_INVALID_HANDLE) {
			return !(fsync(fd));
		}
		return sl_false;
	}

	sl_bool File::setNonBlocking(sl_bool flagEnable) const noexcept
	{
		int fd = m_file;
		if (fd != SLIB_FILE_INVALID_HANDLE) {
			int flag = fcntl(fd, F_GETFL, 0);
			if (flag != -1) {
				if (flagEnable) {
					flag |= O_NONBLOCK;
				} else {
					flag = flag & ~O_NONBLOCK;
				}
				int ret = fcntl(fd, F_SETFL, flag);
				return !ret;
			}
		}
		return sl_false;
	}

	sl_bool File::getDiskSize(sl_uint64& outSize) const noexcept
	{
#if defined(SLIB_PLATFORM_IS_DESKTOP)
#	if defined(SLIB_PLATFORM_IS_MACOS)
		int fd = m_file;
		if (fd != SLIB_FILE_INVALID_HANDLE) {
			sl_uint64 nSectors = 0;
			ioctl(fd, DKIOCGETBLOCKCOUNT, &nSectors);
			sl_uint32 nSectorSize = 0;
			ioctl(fd, DKIOCGETBLOCKSIZE, &nSectorSize);
			outSize = nSectorSize * nSectors;
			return sl_true;
		}
#	elif defined(SLIB_PLATFORM_IS_LINUX)
		int fd = m_file;
		if (fd != SLIB_FILE_INVALID_HANDLE) {
			ioctl(fd, BLKGETSIZE64, &outSize);
			return sl_true;
		}
#	endif
#endif
		return sl_false;
	}

	namespace
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
	}

	Time File::getModifiedTime() const noexcept
	{
		int fd = m_file;
		if (fd != SLIB_FILE_INVALID_HANDLE) {
			struct stat st;
			if (!(fstat(fd, &st))) {
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
		if (!(stat(filePath.getData(), &st))) {
			return GetModifiedTime(st);
		} else {
			return Time::zero();
		}
	}

	namespace
	{
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
	}

	Time File::getAccessedTime() const noexcept
	{
		int fd = m_file;
		if (fd != SLIB_FILE_INVALID_HANDLE) {
			struct stat st;
			if (!(fstat(fd, &st))) {
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
		if (!(stat(filePath.getData(), &st))) {
			return GetAccessedTime(st);
		} else {
			return Time::zero();
		}
	}

	namespace
	{
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
	}

	Time File::getCreatedTime() const noexcept
	{
		int fd = m_file;
		if (fd != SLIB_FILE_INVALID_HANDLE) {
			struct stat st;
			if (!(fstat(fd, &st))) {
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
		if (!(stat(filePath.getData(), &st))) {
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

	namespace
	{
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
			return !(utimes(filePath.getData(), t));
		}
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

	namespace
	{
		static FileAttributes GetAttributes(struct stat& st)
		{
			int ret = 0;
			if (S_ISDIR(st.st_mode)) {
				ret |= FileAttributes::Directory;
			}
			if (S_ISSOCK(st.st_mode)) {
				ret |= FileAttributes::Socket;
			}
			if (S_ISLNK(st.st_mode)) {
				ret |= FileAttributes::Link;
			}
			if (S_ISBLK(st.st_mode)) {
				ret |= FileAttributes::Device;
			}
			if (S_ISCHR(st.st_mode)) {
				ret |= FileAttributes::CharDevice;
			}
			if (S_ISFIFO(st.st_mode)) {
				ret |= FileAttributes::FIFO;
			}
			if (!ret) {
				ret = FileAttributes::Normal;
			}
			if (st.st_mode & S_IRUSR) {
				ret |= FileAttributes::ReadByUser;
			}
			if (st.st_mode & S_IWUSR) {
				ret |= FileAttributes::WriteByUser;
			}
			if (st.st_mode & S_IXUSR) {
				ret |= FileAttributes::ExecuteByUser;
			}
			if (st.st_mode & S_IRGRP) {
				ret |= FileAttributes::ReadByGroup;
			}
			if (st.st_mode & S_IWGRP) {
				ret |= FileAttributes::WriteByGroup;
			}
			if (st.st_mode & S_IXGRP) {
				ret |= FileAttributes::ExecuteByGroup;
			}
			if (st.st_mode & S_IROTH) {
				ret |= FileAttributes::ReadByOthers;
			}
			if (st.st_mode & S_IWOTH) {
				ret |= FileAttributes::WriteByOthers;
			}
			if (st.st_mode & S_IXOTH) {
				ret |= FileAttributes::ExecuteByOthers;
			}
			return ret;
		}		
	}

	FileAttributes File::_getAttributes() const noexcept
	{
		int fd = m_file;
		if (fd != SLIB_FILE_INVALID_HANDLE) {
			struct stat st;
			if (!(fstat(fd, &st))) {
				return GetAttributes(st);
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
		if (!(stat(filePath.getData(), &st))) {
			FileAttributes ret = GetAttributes(st);
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
		StringCstr filePath(_filePath);
		return !(chmod(filePath.getData(), GetFilePermissions(attrs)));
	}

#ifdef SLIB_PLATFORM_IS_LINUX_DESKTOP
	String File::getCap(const StringParam& _filePath) noexcept
	{
		StringCstr filePath(_filePath);
		cap_t cap = cap_get_file(filePath.getData());
		if (cap) {
			String ret = cap_to_text(cap, sl_null);
			cap_free(cap);
			return ret;
		}
		return sl_null;
	}

	sl_bool File::setCap(const StringParam& _filePath, const StringParam& _cap) noexcept
	{
		StringCstr strCap(_cap);
		cap_t cap = cap_from_text(strCap.getData());
		if (cap) {
			StringCstr filePath(_filePath);
			sl_bool ret = !(cap_set_file(filePath.getData(), cap));
			cap_free(cap);
			return ret;
		}
		return sl_false;
	}

	sl_bool File::equalsCap(const StringParam& _filePath, const StringParam& _cap) noexcept
	{
		StringCstr strCap(_cap);
		cap_t cap1 = cap_from_text(strCap.getData());
		if (cap1) {
			sl_bool ret = sl_false;
			StringCstr filePath(_filePath);
			cap_t cap2 = cap_get_file(filePath.getData());
			if (cap2) {
				ret = cap_compare(cap1, cap2) == 0;
				cap_free(cap2);
			}
			cap_free(cap1);
			return ret;
		}
		return sl_false;
	}
#endif

	sl_bool File::_createDirectory(const StringParam& _filePath) noexcept
	{
		StringCstr filePath(_filePath);
		if (filePath.isEmpty()) {
			return sl_false;
		}
		return !(mkdir(filePath.getData(), 0777));
	}

	sl_bool File::createLink(const StringParam& _pathTarget, const StringParam& _pathLink) noexcept
	{
		StringCstr pathTarget(_pathTarget);
		if (pathTarget.isEmpty()) {
			return sl_false;
		}
		StringCstr pathLink(_pathLink);
		if (pathLink.isEmpty()) {
			return sl_false;
		}
		return !(symlink(pathTarget.getData(), pathLink.getData()));
	}

	sl_bool File::createLink(const StringParam& pathTarget, const StringParam& pathLink, sl_bool flagDirectory) noexcept
	{
		return createLink(pathTarget, pathLink);
	}

	sl_bool File::deleteFile(const StringParam& _filePath) noexcept
	{
		StringCstr filePath(_filePath);
		if (filePath.isEmpty()) {
			return sl_false;
		}
		return !(::remove(filePath.getData()));
	}

	sl_bool File::deleteDirectory(const StringParam& _filePath) noexcept
	{
		String filePath = _filePath.toString();
		if (filePath.isEmpty()) {
			return sl_false;
		}
		StringCstr dirPath(normalizeDirectoryPath(filePath));
		return !(rmdir(dirPath.getData()));
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
		return !(copyfile(pathSrc.getData(), pathDst.getData(), sl_null, COPYFILE_ALL));
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
				struct stat stDst = {0};
				fstat(handleDst, &stDst);
				::close(handleDst);
				struct stat stSrc;
				if (!(fstat(handleSrc, &stSrc))) {
					int attrs = stSrc.st_mode & (S_IRWXU | S_IRWXG | S_IRWXO);
					if ((stDst.st_mode & (S_IRWXU | S_IRWXG | S_IRWXO)) != attrs) {
						chmod(pathDst.getData(), attrs);
					}
				}
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
		return !(::rename(oldPath.getData(), newPath.getData()));
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
				char* name = ent->d_name;
				if (!(isDotOrDotDot(name))) {
					ret.add_NoLock(String::fromUtf8(name));
				}
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
				char* name = ent->d_name;
				if (!(isDotOrDotDot(name))) {
					FileInfo info;
					String strName = String::fromUtf8(name);
					filePath = String::concat(dirPath, "/", strName);
					info.attributes = File::getAttributes(filePath);
					info.size = info.allocSize = File::getSize(filePath);
					info.createdAt = File::getCreatedTime(filePath);
					info.modifiedAt = File::getModifiedTime(filePath);
					info.accessedAt = File::getAccessedTime(filePath);
					ret.add_NoLock(strName, info);
				}
			}
			closedir(dir);
		}
		return ret;
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
			if (!(stat(filePath.getData(), &st))) {
				passwd* pw = getpwuid(st.st_uid);
				if (pw) {
					return pw->pw_name;
				}
			}
		}
		return sl_null;
	}

	sl_bool File::setOwnerName(const StringParam& _filePath, const StringParam& _owner) noexcept
	{
		StringCstr owner(_owner);
		StringCstr filePath(_filePath);
		if (owner.isNotEmpty() && filePath.isNotEmpty()) {
			passwd* pw = getpwnam(owner.getData());
			if (pw) {
				return !(chown(filePath.getData(), pw->pw_uid, -1));
			}
		}
		return sl_false;
	}

	String File::getGroupName(const StringParam& _filePath) noexcept
	{
		StringCstr filePath(_filePath);
		if (filePath.isNotEmpty()) {
			struct stat st;
			if (!(stat(filePath.getData(), &st))) {
				group* grp = getgrgid(st.st_gid);
				if (grp) {
					return grp->gr_name;
				}
			}
		}
		return sl_null;
	}

	sl_bool File::setGroupName(const StringParam& _filePath, const StringParam& _groupName) noexcept
	{
		StringCstr groupName(_groupName);
		StringCstr filePath(_filePath);
		if (groupName.isNotEmpty() && filePath.isNotEmpty()) {
			group* grp = getgrnam(groupName.getData());
			if (grp) {
				return !(chown(filePath.getData(), -1, grp->gr_gid));
			}
		}
		return sl_false;
	}

}

#endif
