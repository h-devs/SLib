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

#if defined(SLIB_PLATFORM_IS_WIN32)

#include "slib/core/app.h"

#include "slib/io/file.h"
#include "slib/core/system.h"
#include "slib/platform.h"

namespace slib
{

	namespace {
		static void SetRunAtStartup(const StringParam& _appName, const StringParam& _path, sl_bool flagRegister)
		{
			StringCstr16 path(_path);
			List<String16> listDelete;
			HKEY hKey = NULL;
			RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_QUERY_VALUE | KEY_SET_VALUE, &hKey);
			if (hKey) {
				DWORD dwIndex = 0;
				sl_char16 name[513] = { 0 };
				sl_char16 data[1025] = { 0 };
				for (;;) {
					DWORD dwType = 0;
					DWORD dwLenName = 512;
					DWORD nData = 1024;
					LSTATUS lRet = RegEnumValueW(hKey, dwIndex, (LPWSTR)name, &dwLenName, NULL, &dwType, (LPBYTE)data, &nData);
					if (lRet == ERROR_SUCCESS) {
						if (dwType == REG_SZ) {
							if (path.equals(data)) {
								if (flagRegister) {
									// already registered
									return;
								} else {
									listDelete.add_NoLock(name);
								}
							}
						}
					} else {
						break;
					}
					dwIndex++;
				}
				if (flagRegister) {
					StringCstr16 appName(_appName);
					RegSetValueExW(hKey, (LPCWSTR)(appName.getData()), NULL, REG_SZ, (BYTE*)(path.getData()), (DWORD)(path.getLength() + 1) * 2);
				} else {
					ListElements<String16> names(listDelete);
					for (sl_size i = 0; i < names.count; i++) {
						RegDeleteValueW(hKey, (LPCWSTR)(names[i].getData()));
					}
				}
				RegCloseKey(hKey);
			}
		}
	}

	void Application::registerRunAtStartup(const StringParam& appName, const StringParam& path)
	{
		SetRunAtStartup(appName, path, sl_true);
	}

	void Application::registerRunAtStartup(const StringParam& path)
	{
		String name = File::getFileNameOnly(path);
		SetRunAtStartup(name, path, sl_true);
	}

	void Application::registerRunAtStartup()
	{
		registerRunAtStartup(getApplicationPath());
	}

	void Application::unregisterRunAtStartup(const StringParam& path)
	{
		SetRunAtStartup(sl_null, path, sl_false);
	}

	void Application::unregisterRunAtStartup()
	{
		unregisterRunAtStartup(getApplicationPath());
	}

	void Application::registerAtStartMenu(const StartMenuParam& param)
	{
		StringParam executablePath = param.executablePath;
		if (executablePath.isNull()) {
			executablePath = Application::getApplicationPath();
		}
		Win32::createShortcut(executablePath, String16::concat(System::getProgramsDirectory(), "/", param.appName, ".lnk"));
	}

}

#endif
