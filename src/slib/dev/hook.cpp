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

#include "slib/dev/hook.h"

#ifdef SLIB_PLATFORM_IS_WIN32

#include "slib/dev/pe.h"
#include "slib/dev/protect.h"

#include "slib/core/base.h"

#define PSAPI_VERSION 1
#include <windows.h>
#include <psapi.h>

#pragma comment(lib, "psapi.lib")

namespace slib
{
	
	const void* Hook::getBaseAddress()
	{
		return getBaseAddress(NULL);
	}

	const void* Hook::getBaseAddress(const char* moduleName)
	{
		MODULEINFO mi;
		ZeroMemory(&mi, sizeof(mi));
		if (GetModuleInformation(GetCurrentProcess(), GetModuleHandleA(moduleName), &mi, sizeof(mi))) {
			return mi.lpBaseOfDll;
		}
		return sl_null;
	}

	sl_bool Hook::replaceImportEntry(const void* moduleBaseAddress, const char* dllName, const char* procName, const void* newFunctionAddress)
	{
		sl_uint8* base = (sl_uint8*)moduleBaseAddress;
		PE pe;
		if (pe.loadHeaders(base)) {
			PE_ImportDescriptor* import = pe.findImportTable(base, dllName);
			if (import) {
				const void** pNameThunk = (const void**)(base + import->originalFirstThunk);
				const void** pThunk = (const void**)(base + import->firstThunk);
				for (;;) {
					sl_uint32 offsetName = *((sl_uint32*)pNameThunk);
					if (offsetName) {
						// first two bytes are ordinal
						char* name = (char*)(base + offsetName + 2);
						if (Base::equalsString(name, procName)) {
							if (MemoryProtection::setReadWrite(pThunk, sizeof(void*))) {
								*pThunk = newFunctionAddress;
								return sl_true;
							} else {
								return sl_false;
							}
						}
					} else {
						break;
					}
					pNameThunk++;
					pThunk++;
				}
			}
		}
		return sl_false;
	}
	
	sl_bool Hook::replaceImportEntry(const char* dllName, const char* procName, const void* newFunctionAddress)
	{
		const void* base = getBaseAddress();
		if (base) {
			return replaceImportEntry(base, dllName, procName, newFunctionAddress);
		}
		return sl_false;
	}

	const void* Hook::replaceVtableEntry(void* object, sl_uint32 index, const void* newFunctionAddress)
	{
		const void*& fn = (*((const void***)object))[index];
		const void* fnOld = fn;
		if (MemoryProtection::setReadWrite(&fn, sizeof(fn))) {
			fn = newFunctionAddress;
			return fnOld;
		}
		return sl_null;
	}
	
	void Hook::replaceVtable(void* object, sl_uint32 nTotalEntries, VtableEntry* replacingEntries, sl_uint32 nReplacingEntries)
	{
		const void*** pVtableSrc = (const void***)object;
		const void** vtable = (const void**)(Base::createMemory(sizeof(void*) * nTotalEntries));
		if (vtable) {
			Base::copyMemory(vtable, *pVtableSrc, nTotalEntries * sizeof(void*));
			for (sl_uint32 i = 0; i < nReplacingEntries; i++) {
				VtableEntry& entry = replacingEntries[i];
				const void*& fn = vtable[entry.index];
				if (entry.pOldFunctionAddress) {
					*((const void**)(entry.pOldFunctionAddress)) = fn;
				}
				fn = entry.newFunctionAddress;
			}
			*pVtableSrc = vtable;
		}
	}
	
	const void* Hook::hookFunction(void* targetFunctionAddress, const void* newFunctionAddress, sl_uint32 nCodeBytesToBackup)
	{
		sl_uint8* fn = (sl_uint8*)targetFunctionAddress;
		if (slib::MemoryProtection::setExecuteReadWrite(fn, 5)) {
			sl_uint8* fnOld = (sl_uint8*)(Base::createMemory(nCodeBytesToBackup + 5));
			if (fnOld) {
				if (slib::MemoryProtection::setExecuteReadWrite(fnOld, nCodeBytesToBackup + 5)) {
					Base::copyMemory(fnOld, fn, nCodeBytesToBackup);
					fnOld[nCodeBytesToBackup] = 0xE9;
					sl_int32 offset = fn - (fnOld + 5); // equals to: (fn + nCodeBytesToBackup) - (fnOld + nCodeBytesToBackup + 5);
					Base::copyMemory(fnOld + nCodeBytesToBackup + 1, &offset, 4);
					fn[0] = 0xE9;
					offset = (sl_uint8*)(newFunctionAddress) - (fn + 5);
					memcpy(fn + 1, &offset, 4);
					return fnOld;
				}
				Base::freeMemory(fnOld);
			}		
		}
		return sl_null;
	}

	const void* Hook::hookFunction(sl_uint32 targetFunctionRVA, const void* newFunctionAddress, sl_uint32 nCodeBytesToBackup)
	{
		const void* base = slib::Hook::getBaseAddress();
		if (base) {
			return hookFunction((sl_uint8*)base + targetFunctionRVA, newFunctionAddress, nCodeBytesToBackup);
		}
		return sl_null;
	}
	
	sl_bool Hook::replaceCode(void* targetAddress, const void* pNewCode, sl_uint32 nCodeBytes)
	{
		if (slib::MemoryProtection::setExecuteReadWrite(targetAddress, nCodeBytes)) {
			Base::copyMemory(targetAddress, pNewCode, nCodeBytes);
			return sl_true;
		}
		return sl_false;
	}

	sl_bool Hook::replaceCode(sl_uint32 targetRVA, const void* pNewCode, sl_uint32 nCodeBytes)
	{
		const void* base = slib::Hook::getBaseAddress();
		if (base) {
			return replaceCode((sl_uint8*)base + targetRVA, pNewCode, nCodeBytes);
		}
		return sl_false;
	}

}

#endif
