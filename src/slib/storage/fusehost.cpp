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
				LOG("statfs: %s", path);

				Base::zeroMemory(stbuf, sizeof *stbuf);
				stbuf->f_bsize = 1024;
				stbuf->f_frsize = 1024;
				stbuf->f_blocks = 1024 * 1024;
				stbuf->f_bfree = 512 * 1024;
				stbuf->f_bavail = 512 * 1024;
				stbuf->f_fsid = 0;
				stbuf->f_namemax = 256;
				return 0;
			}

			static int fusehost_getattr(const char *_path, struct stat *stbuf)
			{
				String path(_path);
				if (path.getLength() != 1 && !path.endsWith("dummy.txt") && !path.endsWith("dummy"))
					return -ENOENT;

				sl_bool flagDirectory = !path.endsWith("dummy.txt");

				Base::zeroMemory(stbuf, sizeof *stbuf);
				stbuf->st_mode = 0777 | (flagDirectory ? 0040000/* S_IFDIR */ : 0);
				stbuf->st_nlink = 1;
				stbuf->st_size = (flagDirectory ? 0 : 5);

				sl_int64 now = Time::now().getSecondsCount();
				stbuf->st_ctim.tv_sec = now;
				stbuf->st_mtim.tv_sec = now;
				stbuf->st_atim.tv_sec = now;

				//LOG("getattr: %s : %s", path, flagDirectory ? "DIR" : "FILE");
				return 0;
			}

			static int fusehost_getdir(const char *_path, fuse_dirh_t dh, fuse_dirfil_t filler)
			{
				String path(_path);
				if (path.getLength() != 1 && !path.endsWith("dummy"))
					return -ENOENT;

				LOG("getdir: %s", path);

				filler(0, "dummy", 0777 | 0040000, 0);
				filler(0, "dummy.txt", 0777, 0);
				return 0;
			}

			static int fusehost_readdir(const char *_path, void *buf, fuse_fill_dir_t filler, off_t off,
				struct fuse_file_info *fi)
			{
				String path(_path);
				if (path.getLength() != 1 && !path.endsWith("dummy"))
					return -ENOENT;

				LOG("readdir: %s, %d", path, off);

				struct stat st = { 0 };
				st.st_nlink = 1;
				st.st_mode = 0777 | 0040000;
				filler(buf, "dummy", &st, 0);
				st.st_mode = 0777;
				st.st_size = 5;
				filler(buf, "dummy.txt", &st, 0);
				return 0;
			}

			static int fusehost_open(const char *_path, struct fuse_file_info *fi)
			{
				String path(_path);
				if (!path.endsWith("dummy.txt"))
					return -ENOENT;

				LOG("open: %s", path);
				return 0;
			}

			static int fusehost_read(const char *_path, char *buf, size_t size, off_t offset,
				struct fuse_file_info *fi)
			{
				String path(_path);
				if (!path.endsWith("dummy.txt"))
					return -ENOENT;

				sl_size ret = Memory::createStatic(buf, size).copy(String("dummy").toMemory(), (sl_size)offset, size);
				LOG("read: %s, %d, %d - %d", path, offset, size, ret);
				return (int)ret;
			}

			static int fusehost_release(const char *path, struct fuse_file_info *fi)
			{
				(void)path;
				(void)fi;
				return 0;
			}

			static int fusehost_fsync(const char *path, int isdatasync,
				struct fuse_file_info *fi)
			{
				(void)path;
				(void)isdatasync;
				(void)fi;
				return 0;
			}

			static struct fuse_operations* GetFuseOperations()
			{
				static struct fuse_operations fuse_op = { 0 };
				fuse_op.getattr = fusehost_getattr;
				//fuse_op.getdir = fusehost_getdir;
				fuse_op.readdir = fusehost_readdir;
				//fuse_op.mknod = fusehost_mknod;
				//fuse_op.mkdir = fusehost_mkdir;
				//fuse_op.rmdir = fusehost_rmdir;
				//fuse_op.rename = fusehost_rename;
				//fuse_op.link = fusehost_link;
				//fuse_op.readlink = fusehost_readlink;
				//fuse_op.symlink = fusehost_symlink;
				//fuse_op.unlink = fusehost_unlink;
				//fuse_op.chmod = fusehost_chmod;
				//fuse_op.chown = fusehost_chown;
				//fuse_op.truncate = fusehost_truncate;
				//fuse_op.utimens = fusehost_utimens;
				fuse_op.open = fusehost_open;
				fuse_op.read = fusehost_read;
				//fuse_op.write = fusehost_write;
				fuse_op.statfs = fusehost_statfs;
				fuse_op.release = fusehost_release;
				//fuse_op.fsync = fusehost_fsync;

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
		m_iStatus = 0;
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
			LOG_ERROR("Cannot load fuse library.");
			return sl_false;
		}

		auto funcVersion = getApi_fuse_version();
		if (!funcVersion) {
			LOG_ERROR("Cannot get fuse_version function address.");
			return sl_false;
		}

		int fuse_version = funcVersion();
		LOG("Fuse library version is %d", fuse_version);
		if (fuse_version < FUSE_USE_VERSION) {
			LOG_ERROR("Fuse library version is lower than %d.", FUSE_USE_VERSION);
			return sl_false;
		}

		auto funcMain = getApi_fuse_main_real();
		if (!funcMain) {
			LOG_ERROR("Cannot get fuse_main_real function address.");
			return sl_false;
		}

		fuse_operations* fuse_op = GetFuseOperations();

		List<char*> args;
		args.add(StringCstr("FuseFs").getData());
		args.add(StringCstr("-f").getData());
		args.add(m_param.mountPoint.getData());

		m_iStatus = funcMain(args.getCount(), args.getData(), fuse_op, sizeof(*fuse_op), sl_null);
		return m_iStatus == 0;
	}

	String FuseHost::getErrorMessage()
	{
		return String::format("Ret code %d.", m_iStatus);
	}

}

