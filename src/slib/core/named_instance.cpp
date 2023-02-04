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

#include "slib/core/named_instance.h"

#if defined(SLIB_PLATFORM_IS_WINDOWS)

#include "slib/platform/win32/windows.h"

#else

#include "slib/io/file.h"
#include "slib/core/system.h"
#include "slib/core/hash_map.h"
#include "slib/core/safe_static.h"

#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>

#endif

namespace slib
{

	namespace {

#if defined(SLIB_PLATFORM_IS_WINDOWS)
		static String16 MakeInstanceName(const StringParam& _name)
		{
			return String16::concat("Global\\", _name);
		}
#else
		struct Container
		{
			int handle;
			String name;
		};

		typedef HashMap<String, sl_bool> NameMap;

		SLIB_SAFE_STATIC_GETTER(NameMap, GetNameMap)

		static String MakeInstancePath(const StringParam& name)
		{
			String pathRoot = String::concat(System::getHomeDirectory(), "/.local/.named_inst");
			if (!(File::exists(pathRoot))) {
				File::createDirectories(pathRoot);
			}
			return String::concat(pathRoot, "/", name);
		}
#endif

		static HNamedInstance CreateInstanceHandle(const StringParam& _name)
		{
			if (_name.isEmpty()) {
				return SLIB_NAMED_INSTANCE_INVALID_HANDLE;
			}
#if defined(SLIB_PLATFORM_IS_WINDOWS)
			String16 name = MakeInstanceName(_name);
			HANDLE hMutex = OpenMutexW(MUTEX_ALL_ACCESS, FALSE, (LPCWSTR)(name.getData()));
			if (hMutex) {
				CloseHandle(hMutex);
				return SLIB_NAMED_INSTANCE_INVALID_HANDLE;
			}
			hMutex = CreateMutexW(NULL, FALSE, (LPCWSTR)(name.getData()));
			if (hMutex) {
				return (HNamedInstance)hMutex;
			}
			return SLIB_NAMED_INSTANCE_INVALID_HANDLE;
#else
			NameMap* names = GetNameMap();
			if (!names) {
				return SLIB_NAMED_INSTANCE_INVALID_HANDLE;
			}

			String name = _name.toString();
			MutexLocker lock(names->getLocker());
			if (names->getValue_NoLock(name)) {
				return SLIB_NAMED_INSTANCE_INVALID_HANDLE;
			}

			String path = MakeInstancePath(name);
			int handle = open(path.getData(), O_RDWR | O_CREAT | O_EXCL, 0644);
			if (handle == -1) {
				handle = open(path.getData(), O_RDWR);
				if (handle == -1) {
					return SLIB_NAMED_INSTANCE_INVALID_HANDLE;
				}
			}

			struct flock fl;
			Base::zeroMemory(&fl, sizeof(fl));
			fl.l_start = 0;
			fl.l_len = 0;
			fl.l_type = F_WRLCK;
			fl.l_whence = SEEK_SET;
			int iRet = fcntl(handle, F_SETLK, &fl);
			if (iRet >= 0) {
				names->put_NoLock(name, sl_true);
				Container* ret = new Container;
				if (ret) {
					ret->handle = handle;
					ret->name = Move(name);
					return (HNamedInstance)((void*)ret);
				}
			}
			close(handle);
			return SLIB_NAMED_INSTANCE_INVALID_HANDLE;
#endif
		}

		static void CloseInstanceHandle(HNamedInstance handle)
		{
#if defined(SLIB_PLATFORM_IS_WINDOWS)
			CloseHandle((HANDLE)handle);
#else
			Container* container = (Container*)((void*)handle);
			struct flock fl;
			Base::zeroMemory(&fl, sizeof(fl));
			fl.l_start = 0;
			fl.l_len = 0;
			fl.l_type = F_UNLCK;
			fl.l_whence = SEEK_SET;
			fcntl(container->handle, F_SETLK, &fl);
			close(container->handle);
			NameMap* names = GetNameMap();
			if (names) {
				names->remove(container->name);
			}
			delete container;
#endif
		}

	}

	SLIB_DEFINE_HANDLE_CONTAINER_MEMBERS(NamedInstance, HNamedInstance, m_handle, SLIB_NAMED_INSTANCE_INVALID_HANDLE, CloseInstanceHandle)

	NamedInstance::NamedInstance(const StringParam& name)
	{
		m_handle = CreateInstanceHandle(name);
	}

	sl_bool NamedInstance::exists(const StringParam& _name)
	{
		if (_name.isEmpty()) {
			return sl_false;
		}
#if defined(SLIB_PLATFORM_IS_WINDOWS)
		String16 name = MakeInstanceName(_name);
		HANDLE hMutex = OpenMutexW(MUTEX_ALL_ACCESS, FALSE, (LPCWSTR)(name.getData()));
		if (hMutex != NULL) {
			CloseHandle(hMutex);
			return sl_true;
		}
		return sl_false;
#else
		return NamedInstance(_name).isNone();
#endif
	}

}
