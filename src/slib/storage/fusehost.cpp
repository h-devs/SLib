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

#include "slib/storage/fusehost.h"

#define TAG "FuseHost"
#include "slib/storage/file_system_internal.h"

#include "slib/core/dynamic_library.h"

#define FUSE_USE_VERSION 27
#include "fuse/fuse.h"

#include <errno.h>
#define FUSE_ERROR_CODE(err) (-(int)(err))

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
		namespace fusehost
		{
			void* g_libFuse;
			SLIB_IMPORT_FUNCTION_FROM_LIBRARY(g_libFuse, fuse_main_real, int, ,
				int argc, char *argv[], const struct fuse_operations *op,
				size_t op_size, void *user_data)
			SLIB_IMPORT_FUNCTION_FROM_LIBRARY(g_libFuse, fuse_get_context, struct fuse_context *, ,
				void)
			SLIB_IMPORT_FUNCTION_FROM_LIBRARY(g_libFuse, fuse_version, int, ,
				void)

			static int fusehost_statfs(const char *path, struct statvfs *stbuf)
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
						stbuf->f_blocks = totalSize / BLOCK_SIZE;
						stbuf->f_bfree = freeSize / BLOCK_SIZE;
						stbuf->f_bavail = freeSize / BLOCK_SIZE;
					} else {
						return FUSE_ERROR_CODE(FileSystem::getLastError());
					}

					return 0;
				} FUSE_CATCH
			}

			static int fusehost_getattr(const char *path, struct stat *stbuf)
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
						stbuf->st_ctim.tv_sec = info.createdAt.toUnixTime();
						stbuf->st_atim.tv_sec = info.accessedAt.toUnixTime();
						stbuf->st_mtim.tv_sec = info.modifiedAt.toUnixTime();
					} else {
						return FUSE_ERROR_CODE(FileSystem::getLastError());
					}

					return 0;
				} FUSE_CATCH
			}

			static int fusehost_fgetattr(const char *path, struct stat *stbuf,
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
						stbuf->st_ctim.tv_sec = info.createdAt.toUnixTime();
						stbuf->st_atim.tv_sec = info.accessedAt.toUnixTime();
						stbuf->st_mtim.tv_sec = info.modifiedAt.toUnixTime();
					} else {
						return FUSE_ERROR_CODE(FileSystem::getLastError());
					}

					return 0;
				} FUSE_CATCH
			}

			static int fusehost_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t off,
				struct fuse_file_info *fi)
			{
				FileSystemHost* host = (FileSystemHost*)(getApi_fuse_get_context()()->private_data);
				FileSystemProvider* provider = host->getProvider();
				(void)off;

				FUSE_TRY{
					HashMap<String, FileInfo> files = provider->getFiles(path);
					for (auto& item : files) {
						FileInfo& info = item.value;

						struct stat st = { 0 };
						st.st_nlink = 1;
						st.st_mode = 0777;
						st.st_mode |= (info.attributes & FileAttributes::Directory ? S_IFDIR : S_IFREG);
						st.st_size = info.size;
						st.st_ctim.tv_sec = info.createdAt.toUnixTime();
						st.st_atim.tv_sec = info.accessedAt.toUnixTime();
						st.st_mtim.tv_sec = info.modifiedAt.toUnixTime();

						filler(buf, item.key.getData(), &st, 0);
					}

					return 0;
				} FUSE_CATCH
			}

			static int fusehost_mknod(const char *path, mode_t mode, dev_t dev)
			{
				FileSystemHost* host = (FileSystemHost*)(getApi_fuse_get_context()()->private_data);
				FileSystemProvider* provider = host->getProvider();
				(void)mode;
				(void)dev;

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

			static int fusehost_mkdir(const char *path, mode_t mode)
			{
				FileSystemHost* host = (FileSystemHost*)(getApi_fuse_get_context()()->private_data);
				FileSystemProvider* provider = host->getProvider();
				(void)mode;

				FUSE_TRY {
					FileSystem::setLastError(FileSystemError::GeneralError);
					if (provider->createDirectory(path)) {
						return 0;
					}
					return FUSE_ERROR_CODE(FileSystem::getLastError());
				} FUSE_CATCH
			}

			static int fusehost_rmdir(const char *path)
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

			static int fusehost_unlink(const char *path)
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

			static int fusehost_rename(const char *oldpath, const char *newpath)
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

			static int fusehost_truncate(const char *path, off_t size)
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

			static int fusehost_ftruncate(const char *path, off_t size,
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

			static int fusehost_utimens(const char *path, const struct timespec tv[2])
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

			static int fusehost_open(const char *path, struct fuse_file_info *fi)
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

			static int fusehost_read(const char *path, char *buf, size_t size, off_t offset,
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
					sl_uint64 ret = provider->readFile(context, offset, buf, size);
					if (ret == 0) { // success or error ?
						return FUSE_ERROR_CODE(FileSystem::getLastError());
					}
					return (int)(ret & SLIB_INT32_MAX);
				} FUSE_CATCH
			}

			static int fusehost_write(const char *path, const char *buf, size_t size, off_t offset,
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
					sl_uint64 ret = provider->writeFile(context, offset, buf, size);
					if (ret == 0) { // success or error ?
						return FUSE_ERROR_CODE(FileSystem::getLastError());
					}
					return (int)(ret & SLIB_INT32_MAX);
				} FUSE_CATCH
			}

			static int fusehost_flush(const char *path, struct fuse_file_info *fi)
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

			static int fusehost_release(const char *path, struct fuse_file_info *fi)
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
				fuse_op.getattr = fusehost_getattr;
				//fuse_op.readlink = fusehost_readlink;
				//fuse_op.getdir = fusehost_getdir;
				fuse_op.mknod = fusehost_mknod;
				fuse_op.mkdir = fusehost_mkdir;
				fuse_op.unlink = fusehost_unlink;
				fuse_op.rmdir = fusehost_rmdir;
				//fuse_op.symlink = fusehost_symlink;
				fuse_op.rename = fusehost_rename;
				//fuse_op.link = fusehost_link;
				//fuse_op.chmod = fusehost_chmod;
				//fuse_op.chown = fusehost_chown;
				fuse_op.truncate = fusehost_truncate;
				//fuse_op.utime = fusehost_utime;
				fuse_op.utimens = fusehost_utimens;

				fuse_op.open = fusehost_open;
				fuse_op.read = fusehost_read;
				fuse_op.write = fusehost_write;
				fuse_op.statfs = fusehost_statfs;
				fuse_op.flush = fusehost_flush;
				fuse_op.release = fusehost_release;
				//fuse_op.fsync = fusehost_fsync;

				//fuse_op.opendir = fusehost_opendir;
				fuse_op.readdir = fusehost_readdir;
				//fuse_op.releasedir = fusehost_releasedir;
				//fuse_op.fsyncdir = fusehost_fsyncdir;
				//fuse_op.init = fusehost_init;
				//fuse_op.destroy = fusehost_destroy;
				//fuse_op.access = fusehost_access;
				//fuse_op.create = fusehost_create;
				fuse_op.ftruncate = fusehost_ftruncate;
				fuse_op.fgetattr = fusehost_fgetattr;

				return &fuse_op;
			}

		}
	}

	using namespace priv::fusehost;


	FuseHost::FuseHost()
	{
#ifdef SLIB_PLATFORM_IS_WIN32
		g_libFuse = DynamicLibrary::loadLibrary("dokanfuse1.dll");
#else
		g_libFuse = DynamicLibrary::loadLibrary("libfuse.so.2");
#endif
		m_iRet = 0;
	}

	FuseHost::~FuseHost()
	{
		if (g_libFuse) {
			DynamicLibrary::freeLibrary(g_libFuse);
		}
	}

	sl_bool FuseHost::_run()
	{
		if (!g_libFuse) {
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

		fuse_operations* fuse_op = GetFuseOperations();

		List<char*> args;

		FileSystemProvider* provider = getProvider();
		StringCstr fsName = "FuseFs";

		SLIB_TRY {
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

		m_iRet = funcMain(args.getCount(), args.getData(), fuse_op, sizeof(*fuse_op), this);
		return m_iRet == 0;
	}

	String FuseHost::getErrorMessage()
	{
		if (m_strError.isEmpty()) {
			return String::format("Fuse ret code: %d.", m_iRet);
		}
		return m_strError;
	}

}

