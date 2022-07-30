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

#include "slib/core/system.h"
#include "slib/core/dynamic_library.h"

#define FUSE_USE_VERSION 27
#include "fuse/fuse.h"

#include <errno.h>
#define FUSE_ERROR_CODE(err) ((FileSystemError)(err) == FileSystemError::NotImplemented \
								? -EINVAL \
								: (-(int)(System::mapError((sl_uint32)(err), PlatformType::Unix))))

#define BLOCK_SIZE 1024

namespace slib
{

	namespace priv
	{
		namespace fuse
		{
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
			}
			
#if defined(SLIB_PLATFORM_IS_APPLE)
#	define TO_UNIX_TIME1(NAME, T) { sl_uint64 t = T.getMicrosecondCount(); NAME##timespec.tv_sec = (time_t)(t / 1000000); NAME##timespec.tv_nsec = (long)((t % 1000000) * 1000); }
#else
#	define TO_UNIX_TIME1(NAME, T) { sl_uint64 t = T.getMicrosecondCount(); NAME##tim.tv_sec = (time_t)(t / 1000000); NAME##tim.tv_nsec = (long)((t % 1000000) * 1000); }
#endif
#define TO_UNIX_TIME(st, info) \
			TO_UNIX_TIME1((st).st_c, info.createdAt) \
			TO_UNIX_TIME1((st).st_m, info.modifiedAt) \
			TO_UNIX_TIME1((st).st_a, info.accessedAt)


			static int fuse_getattr(const char *path, struct stat *stbuf)
			{
				FileSystemHost* host = (FileSystemHost*)(getApi_fuse_get_context()()->private_data);
				FileSystemProvider* provider = host->getProvider();

				Base::zeroMemory(stbuf, sizeof *stbuf);
				stbuf->st_mode = 0777;
				stbuf->st_nlink = 1;

				FileInfo info;
				FileSystem::setLastError(FileSystemError::GeneralError);
				if (provider->getFileInfo(path, info, FileInfoMask::All)) {
					stbuf->st_mode |= (info.attributes & FileAttributes::Directory ? S_IFDIR : S_IFREG);
					stbuf->st_size = info.size;
					TO_UNIX_TIME(*stbuf, info)
				} else {
					return FUSE_ERROR_CODE(FileSystem::getLastError());
				}

				return 0;
			}

			static int fuse_fgetattr(const char *path, struct stat *stbuf,
				struct fuse_file_info *fi)
			{
				FileSystemHost* host = (FileSystemHost*)(getApi_fuse_get_context()()->private_data);
				FileSystemProvider* provider = host->getProvider();
				Ref<FileContext> context = (FileContext*)((sl_size)(fi->fh));
				if (context.isNull()) {
					return -EBADF;
				}

				Base::zeroMemory(stbuf, sizeof *stbuf);
				stbuf->st_mode = 0777;
				stbuf->st_nlink = 1;

				FileInfo info;
				FileSystem::setLastError(FileSystemError::GeneralError);
				if (provider->getFileInfo(context, info, FileInfoMask::All)) {
					stbuf->st_mode |= (info.attributes & FileAttributes::Directory ? S_IFDIR : S_IFREG);
					stbuf->st_size = info.size;
					TO_UNIX_TIME(*stbuf, info)
				} else {
					return FUSE_ERROR_CODE(FileSystem::getLastError());
				}

				return 0;
			}

			static int fuse_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t off,
				struct fuse_file_info *fi)
			{
				FileSystemHost* host = (FileSystemHost*)(getApi_fuse_get_context()()->private_data);
				FileSystemProvider* provider = host->getProvider();
				SLIB_UNUSED(off)

				HashMap<String, FileInfo> files = provider->getFiles(path);
				for (auto& item : files) {
					FileInfo& info = item.value;

					struct stat st = { 0 };
					st.st_nlink = 1;
					st.st_mode = 0777;
					st.st_mode |= (info.attributes & FileAttributes::Directory ? S_IFDIR : S_IFREG);
					st.st_size = info.size;
					TO_UNIX_TIME(st, info)

					StringCstr key(item.key);
					filler(buf, key.getData(), &st, 0);
				}

				return 0;
			}

			static int fuse_mknod(const char *path, mode_t mode, dev_t dev)
			{
				FileSystemHost* host = (FileSystemHost*)(getApi_fuse_get_context()()->private_data);
				FileSystemProvider* provider = host->getProvider();
				SLIB_UNUSED(mode);
				SLIB_UNUSED(dev);

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
			}

			static int fuse_mkdir(const char *path, mode_t mode)
			{
				FileSystemHost* host = (FileSystemHost*)(getApi_fuse_get_context()()->private_data);
				FileSystemProvider* provider = host->getProvider();
				SLIB_UNUSED(mode);

				FileSystem::setLastError(FileSystemError::GeneralError);
				if (provider->createDirectory(path)) {
					return 0;
				}
				return FUSE_ERROR_CODE(FileSystem::getLastError());
			}

			static int fuse_rmdir(const char *path)
			{
				FileSystemHost* host = (FileSystemHost*)(getApi_fuse_get_context()()->private_data);
				FileSystemProvider* provider = host->getProvider();

				FileSystem::setLastError(FileSystemError::GeneralError);
				if (provider->deleteDirectory(path)) {
					return 0;
				}
				return FUSE_ERROR_CODE(FileSystem::getLastError());
			}

			static int fuse_unlink(const char *path)
			{
				FileSystemHost* host = (FileSystemHost*)(getApi_fuse_get_context()()->private_data);
				FileSystemProvider* provider = host->getProvider();

				FileSystem::setLastError(FileSystemError::GeneralError);
				if (provider->deleteFile(path)) {
					return 0;
				}
				return FUSE_ERROR_CODE(FileSystem::getLastError());
			}

			static int fuse_rename(const char *oldpath, const char *newpath)
			{
				FileSystemHost* host = (FileSystemHost*)(getApi_fuse_get_context()()->private_data);
				FileSystemProvider* provider = host->getProvider();

				FileSystem::setLastError(FileSystemError::GeneralError);
				if (provider->moveFile(oldpath, newpath, sl_true)) {
					return 0;
				}
				return FUSE_ERROR_CODE(FileSystem::getLastError());
			}

			static int fuse_truncate(const char *path, off_t size)
			{
				FileSystemHost* host = (FileSystemHost*)(getApi_fuse_get_context()()->private_data);
				FileSystemProvider* provider = host->getProvider();

				FileInfo info;
				info.size = size;
				FileSystem::setLastError(FileSystemError::GeneralError);
				if (provider->setFileInfo(path, info, FileInfoMask::Size)) {
					return 0;
				}
				return FUSE_ERROR_CODE(FileSystem::getLastError());
			}

			static int fuse_ftruncate(const char *path, off_t size,
				struct fuse_file_info *fi)
			{
				FileSystemHost* host = (FileSystemHost*)(getApi_fuse_get_context()()->private_data);
				FileSystemProvider* provider = host->getProvider();
				Ref<FileContext> context = (FileContext*)((sl_size)(fi->fh));
				if (context.isNull()) {
					return -EBADF;
				}

				FileInfo info;
				info.size = size;
				FileSystem::setLastError(FileSystemError::GeneralError);
				if (provider->setFileInfo(context, info, FileInfoMask::Size)) {
					return 0;
				}
				return FUSE_ERROR_CODE(FileSystem::getLastError());
			}

			static int fuse_utimens(const char *path, const struct timespec tv[2])
			{
				FileSystemHost* host = (FileSystemHost*)(getApi_fuse_get_context()()->private_data);
				FileSystemProvider* provider = host->getProvider();

				FileInfo info;
				info.accessedAt.setUnixTime(tv[0].tv_sec);
				info.modifiedAt.setUnixTime(tv[1].tv_sec);
				FileSystem::setLastError(FileSystemError::GeneralError);
				if (provider->setFileInfo(path, info, FileInfoMask::Time)) {
					return 0;
				}
				return FUSE_ERROR_CODE(FileSystem::getLastError());
			}

			static int fuse_open(const char *path, struct fuse_file_info *fi)
			{
				FileSystemHost* host = (FileSystemHost*)(getApi_fuse_get_context()()->private_data);
				FileSystemProvider* provider = host->getProvider();
				int oflag = fi->flags;

				FileOpenParam param;
				param.mode = FileMode::ShareAll;

				if ((oflag & 0x03) == O_RDONLY) {
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
				if (oflag & (O_CREAT | O_EXCL)) {
					// TODO CREATE_NEW
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

				host->increaseOpenHandleCount();
				context->increaseReference();
				fi->fh = (sl_uint64)(sl_size)(context.get());

				return 0;
			}

			static int fuse_read(const char *path, char *buf, size_t size, off_t offset,
				struct fuse_file_info *fi)
			{
				FileSystemHost* host = (FileSystemHost*)(getApi_fuse_get_context()()->private_data);
				FileSystemProvider* provider = host->getProvider();
				Ref<FileContext> context = (FileContext*)((sl_size)(fi->fh));
				if (context.isNull()) {
					return -EBADF;
				}

				FileSystem::setLastError(FileSystemError::Success);
				sl_uint64 ret = provider->readFile(context, offset, buf, (sl_uint32)size);
				if (ret == 0) { // success or error ?
					return FUSE_ERROR_CODE(FileSystem::getLastError());
				}
				return (int)(ret & SLIB_INT32_MAX);
			}

			static int fuse_write(const char *path, const char *buf, size_t size, off_t offset,
				struct fuse_file_info *fi)
			{
				FileSystemHost* host = (FileSystemHost*)(getApi_fuse_get_context()()->private_data);
				FileSystemProvider* provider = host->getProvider();
				Ref<FileContext> context = (FileContext*)((sl_size)(fi->fh));
				if (context.isNull()) {
					return -EBADF;
				}

				FileSystem::setLastError(FileSystemError::Success);
				sl_uint64 ret = provider->writeFile(context, offset, buf, (sl_uint32)size);
				if (ret == 0) { // success or error ?
					return FUSE_ERROR_CODE(FileSystem::getLastError());
				}
				return (int)(ret & SLIB_INT32_MAX);
			}

			static int fuse_fsync(const char *path, int isdatasync, struct fuse_file_info *fi)
			{
				FileSystemHost* host = (FileSystemHost*)(getApi_fuse_get_context()()->private_data);
				FileSystemProvider* provider = host->getProvider();
				SLIB_UNUSED(isdatasync);

				Ref<FileContext> context = (FileContext*)((sl_size)(fi->fh));
				if (context.isNull()) {
					return 0;
				}

				FileSystem::setLastError(FileSystemError::GeneralError);
				if (provider->flushFile(context)) {
					return 0;
				}
				return FUSE_ERROR_CODE(FileSystem::getLastError());
			}

			static int fuse_release(const char *path, struct fuse_file_info *fi)
			{
				FileSystemHost* host = (FileSystemHost*)(getApi_fuse_get_context()()->private_data);
				FileSystemProvider* provider = host->getProvider();
				Ref<FileContext> context = (FileContext*)((sl_size)(fi->fh));
				if (context.isNull()) {
					return 0;
				}

				FileSystem::setLastError(FileSystemError::Success);
				provider->closeFile(context);
				host->decreaseOpenHandleCount();
				context->decreaseReference();
				fi->fh = 0;
				return FUSE_ERROR_CODE(FileSystem::getLastError());
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
				//fuse_op.flush = fuse_flush;
				fuse_op.release = fuse_release;
				fuse_op.fsync = fuse_fsync;

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
					FileSystemInfo info;
					if (provider->getInformation(info)) {
						fsName = info.fileSystemName;
					}

					args.add_NoLock(fsName.getData());
					args.add_NoLock((char*)"-f");	// always use foreground mode
					if (m_param.flags & FileSystemHostFlags::DebugMode) {
						args.add_NoLock((char*)"-d");
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
					StringCstr mountPoint(m_param.mountPoint);
					args.add(mountPoint.getData());

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

