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

#if defined(SLIB_PLATFORM_IS_WIN32)

#include "slib/core/win32/windows.h"

#else

#include "slib/core/file.h"

#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>

#endif

namespace slib
{
	
	namespace priv
	{
		namespace named_instance
		{

#if defined(SLIB_PLATFORM_IS_WIN32)
			static String16 MakeInstanceName(const StringParam& _name)
			{
				return String16::join("Global\\", _name);
			}
#else
			static String MakeInstancePath(const StringParam& _name)
			{
				String pathRoot = String::join(System::getHomeDirectory(), "/.named_inst";
				if (!(File::exists(pathRoot))) {
					File::createDirectory(pathRoot);
				}
				return String::join(pathRoot, "/", name);
			}
#endif

			static HandleType* CreateInstanceHandle(const StringParam& _name)
			{
				if (_name.isEmpty()) {
					return sl_null;
				}
#if defined(SLIB_PLATFORM_IS_WIN32)
				String16 name = MakeInstanceName(_name);
				HANDLE hMutex = OpenMutexW(MUTEX_ALL_ACCESS, FALSE, (LPCWSTR)(name.getData()));
				if (hMutex) {
					CloseHandle(hMutex);
					return sl_null;
				}
				hMutex = CreateMutexW(NULL, FALSE, (LPCWSTR)(name.getData()));
				if (hMutex) {
					return (HandleType*)hMutex;
				}
#else
				String path = MakeInstancePath(_name);
				int handle = open(path.getData(), O_RDWR | O_CREAT | O_EXCL, 0644);
				if (handle == -1) {
					handle = open(_path.getData(), O_RDWR);
					if (handle == -1) {
						return sl_null;
					}
				}
				if (handle != -1) {
					struct flock fl;
					Base::zeroMemory(&fl, sizeof(fl));
					fl.l_start = 0;
					fl.l_len = 0;
					fl.l_type = F_WRLCK;
					fl.l_whence = SEEK_SET;
					int ret = fcntl(handle, F_SETLK, &fl);
					if (ret >= 0) {
						return (HandleType*)((void*)((sl_size)handle));
					}
					close(handle);
				}
#endif
				return sl_null;
			}

			void CloseInstanceHandle(HandleType* _handle)
			{
#if defined(SLIB_PLATFORM_IS_WIN32)
				CloseHandle((HANDLE)_handle);
#else
				int handle = int((sl_size)((void*)_handle));
				struct flock fl;
				Base::zeroMemory(&fl, sizeof(fl));
				fl.l_start = 0;
				fl.l_len = 0;
				fl.l_type = F_UNLCK;
				fl.l_whence = SEEK_SET;
				fcntl(handle, F_SETLK, &fl);
				close(handle);
#endif
			}

		}
	}

	using namespace priv::named_instance;

	NamedInstance::NamedInstance(const StringParam& name)
	{
		m_handle = CreateInstanceHandle(name);
	}
	
	sl_bool NamedInstance::exists(const StringParam& _name)
	{
		if (_name.isEmpty()) {
			return sl_false;
		}
#if defined(SLIB_PLATFORM_IS_WIN32)
		String16 name = MakeInstanceName(_name);
		HANDLE hMutex = OpenMutexW(MUTEX_ALL_ACCESS, FALSE, (LPCWSTR)(name.getData()));
		if (hMutex != NULL) {
			CloseHandle(hMutex);
			return sl_true;
		}
#else
		return NamedInstance(_name).isNull();
#endif
		return sl_false;
	}

}
