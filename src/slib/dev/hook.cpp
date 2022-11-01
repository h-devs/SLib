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
#include "slib/dev/module.h"
#include "slib/core/base.h"

namespace slib
{

	void* Hook::replaceImportEntry(const void* moduleBaseAddress, const char* dllName, const char* procName, const void* newFunctionAddress)
	{
		sl_uint8* base = (sl_uint8*)moduleBaseAddress;
		PE pe;
		if (pe.load(base)) {
			PE_ImportDescriptor* import = pe.findImportTable(dllName);
			if (import) {
				void** pNameThunk = (void**)(base + import->originalFirstThunk);
				void** pThunk = (void**)(base + import->firstThunk);
				for (;;) {
					sl_uint32 offsetName = *((sl_uint32*)pNameThunk);
					if (offsetName) {
						// first two bytes are ordinal
						char* name = (char*)(base + offsetName + 2);
						if (Base::equalsString(name, procName)) {
							if (MemoryProtection::setReadWrite(pThunk, sizeof(void*))) {
								void* old = *pThunk;
								*pThunk = (void*)newFunctionAddress;
								return old;
							} else {
								return sl_null;
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
		return sl_null;
	}

	void* Hook::replaceImportEntry(const char* dllName, const char* procName, const void* newFunctionAddress)
	{
		const void* base = Module::getBaseAddress();
		if (base) {
			return replaceImportEntry(base, dllName, procName, newFunctionAddress);
		}
		return sl_null;
	}

#ifdef SLIB_ARCH_IS_64BIT
	namespace priv
	{
		namespace hook
		{
			void GenerateJmpFar(void* _dst, const void* address)
			{
				sl_uint8* dst = (sl_uint8*)_dst;
				dst[0] = 0xC7; // mov dword ptr [esp-8], &address
				dst[1] = 0x44;
				dst[2] = 0x24;
				dst[3] = 0xF8;
				Base::copyMemory(dst + 4, &address, 4);
				dst[8] = 0xC7; // mov dword ptr [esp-4], &address+4
				dst[9] = 0x44;
				dst[10] = 0x24;
				dst[11] = 0xFC;
				Base::copyMemory(dst + 12, ((char*)&address) + 4, 4);
				dst[16] = 0x48; // sub rsp, 8
				dst[17] = 0x83;
				dst[18] = 0xEC;
				dst[19] = 0x08;
				dst[20] = 0xC3; // ret
			}
		}
	}
#endif

	void* Hook::hookFunction(const void* targetFunctionAddress, const void* newFunctionAddress, sl_uint32 nCodeBytesToBackup)
	{
		sl_uint8* fnTarget = (sl_uint8*)targetFunctionAddress;
#ifdef SLIB_ARCH_IS_64BIT
		sl_uint32 sizeFnRestore = nCodeBytesToBackup + 21;
#else
		sl_uint32 sizeFnRestore = nCodeBytesToBackup + 6;
#endif
		sl_uint8* fnRestore = (sl_uint8*)(Base::createMemory(sizeFnRestore));
		if (fnRestore) {
			if (MemoryProtection::setExecuteReadWrite(fnRestore, sizeFnRestore)) {
				Base::copyMemory(fnRestore, fnTarget, nCodeBytesToBackup);
				sl_uint8* fnReturn = fnTarget + nCodeBytesToBackup;
#ifdef SLIB_ARCH_IS_64BIT
				priv::hook::GenerateJmpFar(fnRestore + nCodeBytesToBackup, fnReturn);
#else
				fnRestore[nCodeBytesToBackup] = 0x68; // push fnReturn
				Base::copyMemory(fnRestore + nCodeBytesToBackup + 1, &fnReturn, 4);
				fnRestore[sizeFnRestore - 1] = 0xC3; // ret
#endif

				sl_reg offset = (sl_reg)((sl_uint8*)(newFunctionAddress) - (fnTarget + 5));
#ifdef SLIB_ARCH_IS_64BIT
				if (offset != (sl_int32)offset) {
					if (MemoryProtection::setExecuteReadWrite(fnTarget, 21)) {
						priv::hook::GenerateJmpFar(fnTarget, newFunctionAddress);
						return fnRestore;
					}
				} else
#endif
				if (MemoryProtection::setExecuteReadWrite(fnTarget, 5)) {
					fnTarget[0] = 0xE9; // jmp near offset
					Base::copyMemory(fnTarget + 1, &offset, 4);
					return fnRestore;
				}
				Base::freeMemory(fnRestore);
			}
		}
		return sl_null;
	}

	void* Hook::hookFunction(sl_uint32 targetFunctionRVA, const void* newFunctionAddress, sl_uint32 nCodeBytesToBackup)
	{
		const void* base = Module::getBaseAddress();
		if (base) {
			return hookFunction((sl_uint8*)base + targetFunctionRVA, newFunctionAddress, nCodeBytesToBackup);
		}
		return sl_null;
	}

	void* Hook::hookJmpNear(const void* targetAddress, const void* newAddress)
	{
		sl_uint8* target = (sl_uint8*)targetAddress;
		sl_reg offsetNew = (sl_reg)((sl_uint8*)newAddress - (target + 5));
#ifdef SLIB_ARCH_IS_64BIT
		if (offsetNew != (sl_int32)offsetNew) {
			return sl_null;
		}
#endif
		if (MemoryProtection::setExecuteReadWrite(target, 5)) {
			if (target[0] == 0xE9) { // jmp near
				sl_int32 offsetOld = SLIB_MAKE_DWORD(target[4], target[3], target[2], target[1]);
				target[1] = SLIB_GET_BYTE0(offsetNew);
				target[2] = SLIB_GET_BYTE1(offsetNew);
				target[3] = SLIB_GET_BYTE2(offsetNew);
				target[4] = SLIB_GET_BYTE3(offsetNew);
				return target + 5 + offsetOld;
			}
		}
		return sl_null;
	}

	void* Hook::hookJmpNear(sl_uint32 targetRVA, const void* newAddress)
	{
		const void* base = Module::getBaseAddress();
		if (base) {
			return hookJmpNear((sl_uint8*)base + targetRVA, newAddress);
		}
		return sl_null;
	}

	sl_bool Hook::replaceCode(const void* targetAddress, const void* pNewCode, sl_uint32 nCodeBytes)
	{
		if (MemoryProtection::setExecuteReadWrite(targetAddress, nCodeBytes)) {
			Base::copyMemory((void*)targetAddress, pNewCode, nCodeBytes);
			return sl_true;
		}
		return sl_false;
	}

	sl_bool Hook::replaceCode(sl_uint32 targetRVA, const void* pNewCode, sl_uint32 nCodeBytes)
	{
		const void* base = Module::getBaseAddress();
		if (base) {
			return replaceCode((sl_uint8*)base + targetRVA, pNewCode, nCodeBytes);
		}
		return sl_false;
	}

}

#endif
