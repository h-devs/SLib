/*
*   Copyright (c) 2008-2024 SLIBIO <https://github.com/SLIBIO>
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

#if defined(SLIB_PLATFORM_IS_WINDOWS)

#include "slib/system/dynamic_library.h"
#include "slib/system/module.h"

#include "slib/core/list.h"
#include "slib/platform/win32/windows.h"
#include "slib/dl/win32/psapi.h"

namespace slib
{

	void* DynamicLibrary::loadLibrary(const StringParam& _path)
	{
		StringCstr16 path(_path);
		return (void*)(LoadLibraryW((LPCWSTR)(path.getData())));
	}

	void DynamicLibrary::freeLibrary(void* library)
	{
		FreeLibrary((HMODULE)library);
	}

	void* DynamicLibrary::getFunctionAddress(void* library, const char* name)
	{
		return (void*)(GetProcAddress((HMODULE)library, (LPCSTR)name));
	}


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(ModuleDescription)

	ModuleDescription::ModuleDescription(): baseAddress(sl_null), imageSize(0)
	{
	}


	const void* Module::getBaseAddress()
	{
		return getBaseAddress(sl_null);
	}

	const void* Module::getBaseAddress(const char* moduleName)
	{
		auto funcGetModuleInformation = psapi::getApi_GetModuleInformation();
		if (!funcGetModuleInformation) {
			return sl_null;
		}
		MODULEINFO mi;
		Base::zeroMemory(&mi, sizeof(mi));
		if (funcGetModuleInformation(GetCurrentProcess(), GetModuleHandleA(moduleName), &mi, sizeof(mi))) {
			return mi.lpBaseOfDll;
		}
		return sl_null;
	}

	namespace {
		static List<HMODULE> GetAllModules(HANDLE hProcess)
		{
			auto funcEnumProcessModules = psapi::getApi_EnumProcessModules();
			if (!funcEnumProcessModules) {
				return sl_null;
			}
			DWORD dwSize = 0;
			if (!(funcEnumProcessModules(hProcess, sl_null, 0, &dwSize))) {
				return sl_null;
			}
			sl_size n = (sl_size)(dwSize / sizeof(HMODULE));
			List<HMODULE> ret(n);
			if (ret.isNull()) {
				return sl_null;
			}
			dwSize = 0;
			if (!(funcEnumProcessModules(hProcess, ret.getData(), (DWORD)(n * sizeof(HMODULE)), &dwSize))) {
				return sl_null;
			}
			return ret;
		}
	}

	List<ModuleDescription> Module::getAllModules(sl_uint32 processId, sl_bool flagQueryImagePath, sl_bool flagQueryBaseAddressAndSize)
	{
		HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, (DWORD)processId);
		if (!hProcess) {
			return sl_null;
		}
		psapi::DL_FUNC_TYPE_GetModuleFileNameExW funcGetModuleFileNameExW = sl_null;
		if (flagQueryImagePath) {
			funcGetModuleFileNameExW = psapi::getApi_GetModuleFileNameExW();
		}
		psapi::DL_FUNC_TYPE_GetModuleInformation funcGetModuleInformation = sl_null;
		if (flagQueryBaseAddressAndSize) {
			funcGetModuleInformation = psapi::getApi_GetModuleInformation();
		}
		List<ModuleDescription> ret;
		ListElements<HMODULE> handles(GetAllModules(hProcess));
		for (sl_size i = 0; i < handles.count; i++) {
			HMODULE hModule = handles[i];
			ModuleDescription desc;
			if (funcGetModuleFileNameExW) {
				WCHAR filePath[MAX_PATH + 1];
				DWORD dwLen = funcGetModuleFileNameExW(hProcess, hModule, filePath, MAX_PATH);
				if (dwLen) {
					desc.imagePath = String::from(filePath, dwLen);
				}
			}
			if (funcGetModuleInformation) {
				MODULEINFO mi;
				Base::zeroMemory(&mi, sizeof(mi));
				if (funcGetModuleInformation(hProcess, hModule, &mi, sizeof(mi))) {
					desc.baseAddress = mi.lpBaseOfDll;
					desc.imageSize = (sl_size)(mi.SizeOfImage);
				}
			}
			ret.add_NoLock(Move(desc));
		}
		CloseHandle(hProcess);
		return ret;
	}

}

#endif
