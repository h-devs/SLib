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

#include "slib/storage/fuse.h"

#define TAG "FuseHost"
#include "slib/storage/file_system_internal.h"

#include "slib/core/dynamic_library.h"

#define FUSE_USE_VERSION 27
#include "fuse/fuse.h"

#include <errno.h>

#ifdef SLIB_PLATFORM_IS_WINDOWS
# define FUSE_ERROR_CODE(err) (-(MapError((int)(err))))
#else
# define FUSE_ERROR_CODE(err) (-(int)(err))
#endif

#define FUSE_TRY SLIB_TRY
#define FUSE_CATCH \
	SLIB_CATCH(FileSystemError err, { \
		return FUSE_ERROR_CODE(err); \
	}) \
	SLIB_CATCH(..., { \
		return -EINVAL; \
	})

#define BLOCK_SIZE 1024

namespace slib
{

	namespace priv
	{
		namespace fuse
		{

#ifdef SLIB_PLATFORM_IS_WINDOWS
			static int MapError(int win_errno)
			{
				switch (win_errno)
				{
				case ERROR_INVALID_FUNCTION:
					return EINVAL;
				case ERROR_FILE_NOT_FOUND:
					return ENOENT;
				case ERROR_PATH_NOT_FOUND:
					return ENOENT;
				case ERROR_TOO_MANY_OPEN_FILES:
					return EMFILE;
				case ERROR_ACCESS_DENIED:
					return EACCES;
				case ERROR_INVALID_HANDLE:
					return EBADF;
				case ERROR_ARENA_TRASHED:
					return ENOMEM;
				case ERROR_NOT_ENOUGH_MEMORY:
					return ENOMEM;
				case ERROR_INVALID_BLOCK:
					return ENOMEM;
				case ERROR_BAD_ENVIRONMENT:
					return E2BIG;
				case ERROR_BAD_FORMAT:
					return ENOEXEC;
				case ERROR_INVALID_ACCESS:
					return EINVAL;
				case ERROR_INVALID_DATA:
					return EINVAL;
				case ERROR_INVALID_DRIVE:
					return ENOENT;
				case ERROR_CURRENT_DIRECTORY:
					return EACCES;
				case ERROR_NOT_SAME_DEVICE:
					return EXDEV;
				case ERROR_NO_MORE_FILES:
					return ENOENT;
				case ERROR_LOCK_VIOLATION:
					return EACCES;
				case ERROR_BAD_NETPATH:
					return ENOENT;
				case ERROR_NETWORK_ACCESS_DENIED:
					return EACCES;
				case ERROR_BAD_NET_NAME:
					return ENOENT;
				case ERROR_FILE_EXISTS:
					return EEXIST;
				case ERROR_CANNOT_MAKE:
					return EACCES;
				case ERROR_FAIL_I24:
					return EACCES;
				case ERROR_INVALID_PARAMETER:
					return EINVAL;
				case ERROR_NO_PROC_SLOTS:
					return EAGAIN;
				case ERROR_DRIVE_LOCKED:
					return EACCES;
				case ERROR_BROKEN_PIPE:
					return EPIPE;
				case ERROR_DISK_FULL:
					return ENOSPC;
				case ERROR_INVALID_TARGET_HANDLE:
					return EBADF;
				case ERROR_WAIT_NO_CHILDREN:
					return ECHILD;
				case ERROR_CHILD_NOT_COMPLETE:
					return ECHILD;
				case ERROR_DIRECT_ACCESS_HANDLE:
					return EBADF;
				case ERROR_NEGATIVE_SEEK:
					return EINVAL;
				case ERROR_SEEK_ON_DEVICE:
					return EACCES;
				case ERROR_DIR_NOT_EMPTY:
					return ENOTEMPTY;
				case ERROR_NOT_LOCKED:
					return EACCES;
				case ERROR_BAD_PATHNAME:
					return ENOENT;
				case ERROR_MAX_THRDS_REACHED:
					return EAGAIN;
				case ERROR_LOCK_FAILED:
					return EACCES;
				case ERROR_ALREADY_EXISTS:
					return EEXIST;
				case ERROR_FILENAME_EXCED_RANGE:
					return ENOENT;
				case ERROR_NESTING_NOT_ALLOWED:
					return EAGAIN;
				case ERROR_NOT_ENOUGH_QUOTA:
					return ENOMEM;
				default:
					if (ERROR_WRITE_PROTECT <= win_errno && win_errno <= ERROR_SHARING_BUFFER_EXCEEDED) {
						return EACCES;
					} else if (ERROR_INVALID_STARTING_CODESEG <= win_errno && win_errno <= ERROR_INFLOOP_IN_RELOC_CHAIN) {
						return ENOEXEC;
					}
				}
				return win_errno;
			}
#endif

			void* g_libFuse;
			SLIB_IMPORT_FUNCTION_FROM_LIBRARY(g_libFuse, fuse_main_real, int, ,
				int argc, char *argv[], const struct fuse_operations *op,
				size_t op_size, void *user_data)
			SLIB_IMPORT_FUNCTION_FROM_LIBRARY(g_libFuse, fuse_get_context, struct fuse_context *, ,
				void)
			SLIB_IMPORT_FUNCTION_FROM_LIBRARY(g_libFuse, fuse_unmount, void, ,
				const char *mountpoint, struct fuse_chan *ch)
			SLIB_IMPORT_FUNCTION_FROM_LIBRARY(g_libFuse, fuse_version, int, ,
				void)

			static int fuse_statfs(const char *path, struct statvfs *stbuf)
			{
				FileSystemHost* host = (FileSystemHost*)(getApi_fuse_get_context()()->private_data);
				FileSystemProvider* provider = host->getProvider();

				FUSE_TRY {
					Base::zeroMemory(stbuf, sizeof *stbuf);

					FileSystemInfo info;
					FileSystem::setLastError(FileSystemError::GeneralError);
					if (provider->getInformation(info)) {
						stbuf->f_fsid = info.serialNumber;
						stbuf->f_namemax = info.maxPathLength;
					} else {
						return FUSE_ERROR_CODE(FileSystem::getLastError());
					}

					sl_uint64 totalSize, freeSize;
					if (provider->getSize(&totalSize, &freeSize)) {
						stbuf->f_bsize = BLOCK_SIZE;
						stbuf->f_frsize = BLOCK_SIZE;
						stbuf->f_blocks = (fsblkcnt_t)(totalSize / BLOCK_SIZE);
						stbuf->f_bfree = (fsblkcnt_t)(freeSize / BLOCK_SIZE);
						stbuf->f_bavail = (fsblkcnt_t)(freeSize / BLOCK_SIZE);
					} else {
						return FUSE_ERROR_CODE(FileSystem::getLastError());
					}

					return 0;
				} FUSE_CATCH
			}
			
#if defined(SLIB_PLATFORM_IS_APPLE)
#	define TO_UNIX_TIME1(NAME, T) { sl_uint64 t = T.getMicrosecondsCount(); NAME##timespec.tv_sec = (time_t)(t / 1000000); NAME##timespec.tv_nsec = (long)((t % 1000000) * 1000); }
#else
#	define TO_UNIX_TIME1(NAME, T) { sl_uint64 t = T.getMicrosecondsCount(); NAME##tim.tv_sec = (time_t)(t / 1000000); NAME##tim.tv_nsec = (long)((t % 1000000) * 1000); }
#endif
#define TO_UNIX_TIME(st, info) \
			TO_UNIX_TIME1((st).st_c, info.createdAt) \
			TO_UNIX_TIME1((st).st_m, info.modifiedAt) \
			TO_UNIX_TIME1((st).st_a, info.accessedAt)


			static int fuse_getattr(const char *path, struct stat *stbuf)
			{
				FileSystemHost* host = (FileSystemHost*)(getApi_fuse_get_context()()->private_data);
				FileSystemProvider* provider = host->getProvider();

				FUSE_TRY{
					Base::zeroMemory(stbuf, sizeof *stbuf);
					stbuf->st_mode = 0777;
					stbuf->st_nlink = 1;

					FileInfo info;
					FileSystem::setLastError(FileSystemError::GeneralError);
					if (provider->getFileInfo(path, sl_null, info, FileInfoMask::All)) {
						stbuf->st_mode |= (info.attributes & FileAttributes::Directory ? S_IFDIR : S_IFREG);
						stbuf->st_size = info.size;
						TO_UNIX_TIME(*stbuf, info)
					} else {
						return FUSE_ERROR_CODE(FileSystem::getLastError());
					}

					return 0;
				} FUSE_CATCH
			}

			static int fuse_fgetattr(const char *path, struct stat *stbuf,
				struct fuse_file_info *fi)
			{
				FileSystemHost* host = (FileSystemHost*)(getApi_fuse_get_context()()->private_data);
				FileSystemProvider* provider = host->getProvider();
				FileContext* context = (FileContext*)((sl_size)(fi->fh));
				if (!context) {
					return -EBADF;
				}

				FUSE_TRY{
					Base::zeroMemory(stbuf, sizeof *stbuf);
					stbuf->st_mode = 0777;
					stbuf->st_nlink = 1;

					FileInfo info;
					FileSystem::setLastError(FileSystemError::GeneralError);
					if (provider->getFileInfo(path, context, info, FileInfoMask::All)) {
						stbuf->st_mode |= (info.attributes & FileAttributes::Directory ? S_IFDIR : S_IFREG);
						stbuf->st_size = info.size;
						TO_UNIX_TIME(*stbuf, info)
					} else {
						return FUSE_ERROR_CODE(FileSystem::getLastError());
					}

					return 0;
				} FUSE_CATCH
			}

			static int fuse_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t off,
				struct fuse_file_info *fi)
			{
				FileSystemHost* host = (FileSystemHost*)(getApi_fuse_get_context()()->private_data);
				FileSystemProvider* provider = host->getProvider();
				SLIB_UNUSED(off)

				FUSE_TRY{
					HashMap<String, FileInfo> files = provider->getFiles(path);
					for (auto& item : files) {
						FileInfo& info = item.value;

						struct stat st = { 0 };
						st.st_nlink = 1;
						st.st_mode = 0777;
						st.st_mode |= (info.attributes & FileAttributes::Directory ? S_IFDIR : S_IFREG);
						st.st_size = info.size;
						TO_UNIX_TIME(st, info)

						filler(buf, item.key.getData(), &st, 0);
					}

					return 0;
				} FUSE_CATCH
			}

			static int fuse_mknod(const char *path, mode_t mode, dev_t dev)
			{
				FileSystemHost* host = (FileSystemHost*)(getApi_fuse_get_context()()->private_data);
				FileSystemProvider* provider = host->getProvider();
				SLIB_UNUSED(mode);
				SLIB_UNUSED(dev);

				FUSE_TRY{
					if (provider->existsFile(path)) {
						return -EEXIST;
					}

					FileOpenParam param;
					param.mode = FileMode::Write;
					FileSystem::setLastError(FileSystemError::GeneralError);
					Ref<FileContext> context = provider->openFile(path, param);
					if (context.isNull()) {
						return FUSE_ERROR_CODE(FileSystem::getLastError());
					}
					provider->closeFile(context);
					return 0;
				} FUSE_CATCH
			}

			static int fuse_mkdir(const char *path, mode_t mode)
			{
				FileSystemHost* host = (FileSystemHost*)(getApi_fuse_get_context()()->private_data);
				FileSystemProvider* provider = host->getProvider();
				SLIB_UNUSED(mode);

				FUSE_TRY {
					FileSystem::setLastError(FileSystemError::GeneralError);
					if (provider->createDirectory(path)) {
						return 0;
					}
					return FUSE_ERROR_CODE(FileSystem::getLastError());
				} FUSE_CATCH
			}

			static int fuse_rmdir(const char *path)
			{
				FileSystemHost* host = (FileSystemHost*)(getApi_fuse_get_context()()->private_data);
				FileSystemProvider* provider = host->getProvider();

				FUSE_TRY{
					FileSystem::setLastError(FileSystemError::GeneralError);
					if (provider->deleteDirectory(path)) {
						return 0;
					}
					return FUSE_ERROR_CODE(FileSystem::getLastError());
				} FUSE_CATCH
			}

			static int fuse_unlink(const char *path)
			{
				FileSystemHost* host = (FileSystemHost*)(getApi_fuse_get_context()()->private_data);
				FileSystemProvider* provider = host->getProvider();

				FUSE_TRY{
					FileSystem::setLastError(FileSystemError::GeneralError);
					if (provider->deleteFile(path)) {
						return 0;
					}
					return FUSE_ERROR_CODE(FileSystem::getLastError());
				} FUSE_CATCH
			}

			static int fuse_rename(const char *oldpath, const char *newpath)
			{
				FileSystemHost* host = (FileSystemHost*)(getApi_fuse_get_context()()->private_data);
				FileSystemProvider* provider = host->getProvider();

				FUSE_TRY{
					FileSystem::setLastError(FileSystemError::GeneralError);
					if (provider->moveFile(oldpath, newpath, sl_true)) {
						return 0;
					}
					return FUSE_ERROR_CODE(FileSystem::getLastError());
				} FUSE_CATCH
			}

			static int fuse_truncate(const char *path, off_t size)
			{
				FileSystemHost* host = (FileSystemHost*)(getApi_fuse_get_context()()->private_data);
				FileSystemProvider* provider = host->getProvider();

				FUSE_TRY{
					FileInfo info;
					info.size = size;
					FileSystem::setLastError(FileSystemError::GeneralError);
					if (provider->setFileInfo(path, sl_null, info, FileInfoMask::Size)) {
						return 0;
					}
					return FUSE_ERROR_CODE(FileSystem::getLastError());
				} FUSE_CATCH
			}

			static int fuse_ftruncate(const char *path, off_t size,
				struct fuse_file_info *fi)
			{
				FileSystemHost* host = (FileSystemHost*)(getApi_fuse_get_context()()->private_data);
				FileSystemProvider* provider = host->getProvider();
				FileContext* context = (FileContext*)((sl_size)(fi->fh));
				if (!context) {
					return -EBADF;
				}

				FUSE_TRY{
					FileInfo info;
					info.size = size;
					FileSystem::setLastError(FileSystemError::GeneralError);
					if (provider->setFileInfo(path, context, info, FileInfoMask::Size)) {
						return 0;
					}
					return FUSE_ERROR_CODE(FileSystem::getLastError());
				} FUSE_CATCH
			}

			static int fuse_utimens(const char *path, const struct timespec tv[2])
			{
				FileSystemHost* host = (FileSystemHost*)(getApi_fuse_get_context()()->private_data);
				FileSystemProvider* provider = host->getProvider();

				FUSE_TRY{
					FileInfo info;
					info.accessedAt.setUnixTime(tv[0].tv_sec);
					info.modifiedAt.setUnixTime(tv[1].tv_sec);
					FileSystem::setLastError(FileSystemError::GeneralError);
					if (provider->setFileInfo(path, sl_null, info, FileInfoMask::Time)) {
						return 0;
					}
					return FUSE_ERROR_CODE(FileSystem::getLastError());
				} FUSE_CATCH
			}

			static int fuse_open(const char *path, struct fuse_file_info *fi)
			{
				FileSystemHost* host = (FileSystemHost*)(getApi_fuse_get_context()()->private_data);
				FileSystemProvider* provider = host->getProvider();
				int oflag = fi->flags;

				FUSE_TRY {
					FileOpenParam param;
					param.mode = FileMode::ShareAll;

					if (oflag == O_RDONLY) {
						param.mode |= FileMode::Read;
					}
					if (oflag & O_WRONLY) {
						param.mode |= FileMode::Write;
					}
					if (oflag & O_RDWR) {
						param.mode |= FileMode::ReadWrite;
					}
					if (oflag & O_APPEND) {
						param.mode |= FileMode::Append;
					}
					if (!(oflag & O_CREAT)) {
						param.mode |= FileMode::NotCreate;
					}
					if (!(oflag & O_TRUNC)) {
						param.mode |= FileMode::NotTruncate;
					}
					if (oflag & O_EXCL) {
						// TODO
					}
#ifdef O_RANDOM
					if (oflag & O_RANDOM) {
						param.mode |= FileMode::HintRandomAccess;
					}
#endif

					FileSystem::setLastError(FileSystemError::GeneralError);
					Ref<FileContext> context = provider->openFile(path, param);
					if (context.isNull()) {
						return FUSE_ERROR_CODE(FileSystem::getLastError());
					}

					host->increaseOpenHandlesCount();
					context->increaseReference();
					fi->fh = (sl_uint64)(sl_size)(context.get());

					return 0;
				} FUSE_CATCH
			}

			static int fuse_read(const char *path, char *buf, size_t size, off_t offset,
				struct fuse_file_info *fi)
			{
				FileSystemHost* host = (FileSystemHost*)(getApi_fuse_get_context()()->private_data);
				FileSystemProvider* provider = host->getProvider();
				FileContext* context = (FileContext*)((sl_size)(fi->fh));
				if (!context) {
					return -EBADF;
				}

				FUSE_TRY{
					FileSystem::setLastError(FileSystemError::Success);
					sl_uint64 ret = provider->readFile(context, offset, buf, (sl_uint32)size);
					if (ret == 0) { // success or error ?
						return FUSE_ERROR_CODE(FileSystem::getLastError());
					}
					return (int)(ret & SLIB_INT32_MAX);
				} FUSE_CATCH
			}

			static int fuse_write(const char *path, const char *buf, size_t size, off_t offset,
				struct fuse_file_info *fi)
			{
				FileSystemHost* host = (FileSystemHost*)(getApi_fuse_get_context()()->private_data);
				FileSystemProvider* provider = host->getProvider();
				FileContext* context = (FileContext*)((sl_size)(fi->fh));
				if (!context) {
					return -EBADF;
				}

				FUSE_TRY{
					FileSystem::setLastError(FileSystemError::Success);
					sl_uint64 ret = provider->writeFile(context, offset, buf, (sl_uint32)size);
					if (ret == 0) { // success or error ?
						return FUSE_ERROR_CODE(FileSystem::getLastError());
					}
					return (int)(ret & SLIB_INT32_MAX);
				} FUSE_CATCH
			}

			static int fuse_flush(const char *path, struct fuse_file_info *fi)
			{
				FileSystemHost* host = (FileSystemHost*)(getApi_fuse_get_context()()->private_data);
				FileSystemProvider* provider = host->getProvider();
				FileContext* context = (FileContext*)((sl_size)(fi->fh));
				if (!context) {
					return 0;
				}

				FUSE_TRY{
					FileSystem::setLastError(FileSystemError::Success);
					provider->flushFile(context);
					return FUSE_ERROR_CODE(FileSystem::getLastError());
				} FUSE_CATCH
			}

			static int fuse_release(const char *path, struct fuse_file_info *fi)
			{
				FileSystemHost* host = (FileSystemHost*)(getApi_fuse_get_context()()->private_data);
				FileSystemProvider* provider = host->getProvider();
				FileContext* context = (FileContext*)((sl_size)(fi->fh));
				if (!context) {
					return 0;
				}

				FUSE_TRY{
					FileSystem::setLastError(FileSystemError::Success);
					provider->closeFile(context);
					host->decreaseOpenHandlesCount();
					context->decreaseReference();
					fi->fh = 0;
					return FUSE_ERROR_CODE(FileSystem::getLastError());
				} FUSE_CATCH
			}

			static struct fuse_operations* GetFuseOperations()
			{
				static struct fuse_operations fuse_op = { 0 };
				fuse_op.getattr = fuse_getattr;
				//fuse_op.readlink = fuse_readlink;
				//fuse_op.getdir = fuse_getdir;
				fuse_op.mknod = fuse_mknod;
				fuse_op.mkdir = fuse_mkdir;
				fuse_op.unlink = fuse_unlink;
				fuse_op.rmdir = fuse_rmdir;
				//fuse_op.symlink = fuse_symlink;
				fuse_op.rename = fuse_rename;
				//fuse_op.link = fuse_link;
				//fuse_op.chmod = fuse_chmod;
				//fuse_op.chown = fuse_chown;
				fuse_op.truncate = fuse_truncate;
				//fuse_op.utime = fuse_utime;
				fuse_op.utimens = fuse_utimens;

				fuse_op.open = fuse_open;
				fuse_op.read = fuse_read;
				fuse_op.write = fuse_write;
				fuse_op.statfs = fuse_statfs;
				fuse_op.flush = fuse_flush;
				fuse_op.release = fuse_release;
				//fuse_op.fsync = fuse_fsync;

				//fuse_op.opendir = fuse_opendir;
				fuse_op.readdir = fuse_readdir;
				//fuse_op.releasedir = fuse_releasedir;
				//fuse_op.fsyncdir = fuse_fsyncdir;
				//fuse_op.init = fuse_init;
				//fuse_op.destroy = fuse_destroy;
				//fuse_op.access = fuse_access;
				//fuse_op.create = fuse_create;
				fuse_op.ftruncate = fuse_ftruncate;
				fuse_op.fgetattr = fuse_fgetattr;

				return &fuse_op;
			}

			class FuseHost : public FileSystemHost
			{
			public:
				FuseHost() : m_iRet(0)
				{
				}

			public:
				sl_bool _run() override
				{
					if (!Fuse::initialize()) {
						m_strError = "Cannot load fuse library.";
						return sl_false;
					}

					auto funcVersion = getApi_fuse_version();
					if (!funcVersion) {
						m_strError = "Cannot get fuse_version function address.";
						return sl_false;
					}

					int fuse_version = funcVersion();
					LOG("Fuse library version is %d", fuse_version);
					if (fuse_version < FUSE_USE_VERSION) {
						m_strError = String::format("Fuse library version is lower than %d.", FUSE_USE_VERSION);
						return sl_false;
					}

					auto funcMain = getApi_fuse_main_real();
					if (!funcMain) {
						m_strError = "Cannot get fuse_main_real function address.";
						return sl_false;
					}

					FileSystemProvider* provider = getProvider();
					if (!provider) {
						m_strError = "Invalid provider.";
						return sl_false;
					}


					List<char*> args;
					StringCstr fsName = "FuseFs";

					SLIB_TRY{
						FileSystemInfo info;
						if (provider->getInformation(info)) {
							fsName = info.fileSystemName;
						}
					} SLIB_CATCH(...)

					args.add(fsName.getData());
					args.add(StringCstr("-f").getData());	// always use foreground mode
					if (m_param.flags & FileSystemHostFlags::DebugMode) {
						args.add(StringCstr("-d").getData());
					} else if (m_param.flags & FileSystemHostFlags::UseStdErr) {
						// TODO
					}
					if (m_param.flags & FileSystemHostFlags::WriteProtect) {
						// TODO
					}
					if (m_param.flags & FileSystemHostFlags::MountAsRemovable) {
						// TODO
					}
					if (m_param.flags & FileSystemHostFlags::MountAsNetworkDrive) {
						// TODO
					}
					args.add(m_param.mountPoint.getData());

					fuse_operations* fuse_op = GetFuseOperations();

					m_iRet = funcMain((int)(args.getCount()), args.getData(), fuse_op, sizeof(*fuse_op), this);
					return m_iRet == 0;
				}

				String getErrorMessage() override
				{
					if (m_strError.isNotEmpty()) {
						return m_strError;
					}
					return String::format("Fuse ret code: %d.", m_iRet);
				}

			public:
				int m_iRet;
				String m_strError;

			};

		}
	}

	using namespace priv::fuse;


	sl_bool Fuse::initialize(const StringParam& libPath)
	{
		void* lib = DynamicLibrary::loadLibrary(libPath);
		if (lib) {
			g_libFuse = lib;
			return sl_true;
		}
		return sl_false;
	}

	sl_bool Fuse::initialize()
	{
		if (g_libFuse) {
			return sl_true;
		}
#ifdef SLIB_PLATFORM_IS_WIN32
		return initialize("dokanfuse1.dll");
#else
		return initialize("libfuse.so.2");
#endif
	}

	Ref<FileSystemHost> Fuse::createHost()
	{
		return new FuseHost;
	}

	sl_bool Fuse::unmount(const StringParam& _mountPoint)
	{
		if (!(initialize())) {
			return sl_false;
		}
		auto func = getApi_fuse_unmount();
		if (func) {
			StringCstr mountPoint(_mountPoint);
			func(mountPoint.getData(), sl_null);
			return sl_true;
		}
		return sl_false;
	}

}

