/*
*   Copyright (c) 2008-2023 SLIBIO <https://github.com/SLIBIO>
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

#include "slib/develop/hook.h"

#ifdef SLIB_PLATFORM_IS_WIN32

#include "slib/develop/pe.h"
#include "slib/develop/protect.h"
#include "slib/develop/module.h"
#include "slib/core/process.h"
#include "slib/platform/win32/windows.h"
#include "slib/dl/win32/kernel32.h"

#ifdef SLIB_ARCH_IS_64BIT
#define JMP_FAR_CODE_MAX_LENGTH 17
#else
#define JMP_FAR_CODE_MAX_LENGTH 9
#endif

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

	void Hook::replaceAllImportEntries(const char* dllName, const char* procName, const void* newFunctionAddress)
	{
		ListElements<ModuleDescription> modules(Module::getAllModules(Process::getCurrentProcessId(), sl_false));
		for (sl_size i = 0; i < modules.count; i++) {
			ModuleDescription& module = modules[i];
			if (module.baseAddress) {
				replaceImportEntry(module.baseAddress, dllName, procName, newFunctionAddress);
			}
		}
	}

	void* Hook::replaceExportEntry(const void* dllBaseAddress, const char* procName, sl_uint32 newFunctionOffset)
	{
		PE pe;
		if (pe.load(dllBaseAddress)) {
			sl_uint32 offset = pe.updateExportFunctionOffset(procName, newFunctionOffset);
			if (offset) {
				return (sl_uint8*)dllBaseAddress + offset;
			}
		}
		return sl_null;
	}

#if defined(SLIB_ARCH_IS_32BIT)
	void* Hook::replaceExportEntry(const void* dllBaseAddress, const char* procName, const void* newFunctionAddress)
	{
		return replaceExportEntry(dllBaseAddress, procName, (sl_uint32)newFunctionAddress - (sl_uint32)dllBaseAddress);
	}
#endif

	namespace {

		static sl_uint8 Generate_Jmp(void* _dst, const void* address, const void* callerAddress = sl_null, sl_bool flagUseAX = sl_false)
		{
			sl_reg offset = 0;
			if (callerAddress) {
				offset = (sl_reg)((sl_uint8*)address - ((sl_uint8*)callerAddress + 5));
#if defined(SLIB_ARCH_IS_64BIT)
				if (offset != (sl_int32)offset) {
					offset = 0;
				}
#endif
			}
			sl_uint8* dst = (sl_uint8*)_dst;
			if (offset) {
				if (dst) {
					*dst = 0xE9; // jmp offset
					Base::copyMemory(dst + 1, &offset, 4);
				}
				return 5;
			} else {
				if (flagUseAX) {
#if defined(SLIB_ARCH_IS_64BIT)
					if (dst) {
						dst[0] = 0x48; // mov rax, &address
						dst[1] = 0xB8;
						Base::copyMemory(dst + 2, &address, 8);
						dst[10] = 0xFF; // jmp rax
						dst[11] = 0xE0;
					}
					return 12;
#else
					if (dst) {
						dst[0] = 0xB8; // mov eax, &address
						Base::copyMemory(dst + 1, &address, 4);
						dst[5] = 0xFF; // jmp eax
						dst[6] = 0xE0;
					}
					return 7;
#endif
				} else {
#if defined(SLIB_ARCH_IS_64BIT)
					if (dst) {
						dst[0] = 0x50; // push rax
						dst[1] = 0xC7; // mov dword ptr [rsp], &address
						dst[2] = 0x04;
						dst[3] = 0x24;
						Base::copyMemory(dst + 4, &address, 4);
						dst[8] = 0xC7; // mov dword ptr [rsp+4], &address+4
						dst[9] = 0x44;
						dst[10] = 0x24;
						dst[11] = 0x04;
						Base::copyMemory(dst + 12, ((char*)&address) + 4, 4);
						dst[16] = 0xC3; // ret
					}
					return 17;
#else
					if (dst) {
						dst[0] = 0x50; // push eax
						dst[1] = 0xC7; // mov dword ptr [esp], &address
						dst[2] = 0x04;
						dst[3] = 0x24;
						Base::copyMemory(dst + 4, &address, 4);
						dst[8] = 0xC3; // ret
					}
					return 9; 
#endif
				}
			}
		}

		SLIB_INLINE static void WriteByte(sl_uint8* dst, sl_uint32& pos, sl_uint8 value)
		{
			if (dst) {
				dst[pos++] = value;
			} else {
				pos++;
			}
		}

		SLIB_INLINE static void WriteBytes(sl_uint8* dst, sl_uint32& pos, sl_uint8 value1, sl_uint8 value2)
		{
			if (dst) {
				dst[pos++] = value1;
				dst[pos++] = value2;
			} else {
				pos += 2;
			}
		}

		SLIB_INLINE static void WriteBytes(sl_uint8* dst, sl_uint32& pos, sl_uint8 value1, sl_uint8 value2, sl_uint8 value3)
		{
			if (dst) {
				dst[pos++] = value1;
				dst[pos++] = value2;
				dst[pos++] = value3;
			} else {
				pos += 3;
			}
		}

		SLIB_INLINE static void WriteBytes(sl_uint8* dst, sl_uint32& pos, sl_uint8 value1, sl_uint8 value2, sl_uint8 value3, sl_uint8 value4)
		{
			if (dst) {
				dst[pos++] = value1;
				dst[pos++] = value2;
				dst[pos++] = value3;
				dst[pos++] = value4;
			} else {
				pos += 4;
			}
		}

		SLIB_INLINE static void WriteBytes(sl_uint8* dst, sl_uint32& pos, sl_uint8 value1, sl_uint8 value2, sl_uint8 value3, sl_uint8 value4, sl_uint8 value5)
		{
			if (dst) {
				dst[pos++] = value1;
				dst[pos++] = value2;
				dst[pos++] = value3;
				dst[pos++] = value4;
				dst[pos++] = value5;
			} else {
				pos += 5;
			}
		}

		static sl_uint32 Generate_CopyBytes(void* _dst, const void* targetAddress, const void* _src, sl_uint8 size)
		{
			if (!size) {
				return 0;
			}
			sl_uint8* dst = (sl_uint8*)_dst;
			sl_uint8* src = (sl_uint8*)_src;
#if defined(SLIB_ARCH_IS_64BIT)
			if (dst) {
				dst[0] = 0x48; // mov rax, targetAddress
				dst[1] = 0xB8;
				Base::copyMemory(dst + 2, &targetAddress, 8);
			}
			sl_uint32 pos = 10;
#else
			if (dst) {
				dst[0] = 0xB8; // mov eax, targetAddress
				Base::copyMemory(dst + 1, &targetAddress, 4);
			}
			sl_uint32 pos = 5;
#endif
			WriteBytes(dst, pos, 0xC6, 0x00, *(src++)); // mov byte ptr [eax], *src;
			for (sl_uint8 i = 1; i < size; i++) {
				WriteBytes(dst, pos, 0xC6, 0x40, i, *(src++)); // mov byte ptr [eax + i], src[i];
			}
			return pos;
		}

		static sl_uint32 Generate_CallHookedFunction(void* _dst, const void* targetFunctionAddress, const void* codesReplacing, sl_uint8 sizeReplacing)
		{
			sl_uint8* dst = (sl_uint8*)_dst;
			sl_uint32 pos = 0;
			pos += Generate_CopyBytes(dst, targetFunctionAddress, targetFunctionAddress, sizeReplacing);
#if defined(SLIB_ARCH_IS_64BIT)
			WriteBytes(dst, pos, 0x48, 0xB8); // mov rax, pBackupCaller
#else
			WriteByte(dst, pos, 0xB8); // mov eax, pBackupCaller
#endif
			sl_uint8* pBackupCaller = dst ? dst + pos : sl_null;
			pos += sizeof(void*);
			WriteByte(dst, pos, 0x51); // push rcx(ecx)
#if defined(SLIB_ARCH_IS_64BIT)
			WriteBytes(dst, pos, 0x48, 0x8B, 0x4C, 0x24, 0x08); // mov rcx, qword ptr [rsp+8]
			WriteBytes(dst, pos, 0x48, 0x89, 0x08); // mov qword ptr [rax], rcx
#else
			WriteBytes(dst, pos, 0x8B, 0x4C, 0x24, 0x04); // mov ecx, dword ptr [esp+4]
			WriteBytes(dst, pos, 0x89, 0x08); // mov dword ptr [eax], ecx
#endif
			WriteByte(dst, pos, 0x59); // pop rcx(ecx)
#if defined(SLIB_ARCH_IS_64BIT)
			WriteBytes(dst, pos, 0x48, 0xB8); // mov rax, pReturn
#else
			WriteByte(dst, pos, 0xB8); // mov eax, pReturn
#endif
			sl_uint8* pReturn = dst ? dst + pos : sl_null;
			pos += sizeof(void*);
#if defined(SLIB_ARCH_IS_64BIT)
			WriteBytes(dst, pos, 0x48, 0x89, 0x04, 0x24); // mov qword ptr [rsp], rax
#else
			WriteBytes(dst, pos, 0x89, 0x04, 0x24); // mov dword ptr [esp], eax
#endif
			pos += Generate_Jmp(dst ? dst + pos : sl_null, targetFunctionAddress, dst ? dst + pos : sl_null, sl_true);
			if (dst) {
				sl_uint8* p = dst + pos;
				Base::copyMemory(pReturn, &p, sizeof(void*));
			}
			WriteByte(dst, pos, 0x50); // push rax(eax)
			WriteByte(dst, pos, 0x50); // push rax(eax)
			pos += Generate_CopyBytes(dst ? dst + pos : sl_null, targetFunctionAddress, codesReplacing, sizeReplacing);
#if defined(SLIB_ARCH_IS_64BIT)
			WriteBytes(dst, pos, 0x48, 0xB8); // mov rax, pBackupCaller
#else
			WriteByte(dst, pos, 0xB8); // mov eax, pBackupCaller
#endif
			sl_uint8* pBackupCaller2 = dst ? dst + pos : sl_null;
			pos += sizeof(void*);
#if defined(SLIB_ARCH_IS_64BIT)
			WriteBytes(dst, pos, 0x48, 0x8B, 0x00); // mov rax, [rax]
			WriteBytes(dst, pos, 0x48, 0x89, 0x44, 0x24, 0x08); // mov qword ptr [rsp+8], rax
#else
			WriteBytes(dst, pos, 0x8B, 0x00); // mov eax, [eax]
			WriteBytes(dst, pos, 0x89, 0x44, 0x24, 0x04); // mov dword ptr [esp+4], eax
#endif
			WriteByte(dst, pos, 0x58); // pop rax(eax)
			WriteByte(dst, pos, 0xC3); // ret
			pos = ((pos + 15) >> 4) << 4;
			if (dst) {
				sl_uint8* p = dst + pos;
				Base::copyMemory(pBackupCaller, &p, sizeof(void*));
				Base::copyMemory(pBackupCaller2, &p, sizeof(void*));
			}
			pos += sizeof(void*);
			return pos;
		}

	}

	sl_bool Hook::hookFunction(const void* targetFunctionAddress, const void* newFunctionAddress, void* outCallHookedFunction)
	{
		sl_uint8 codesReplacing[JMP_FAR_CODE_MAX_LENGTH];
		sl_uint8 sizeReplacing = Generate_Jmp(codesReplacing, newFunctionAddress, targetFunctionAddress, sl_true);

		sl_uint32 sizeCallHooked = Generate_CallHookedFunction(sl_null, targetFunctionAddress, codesReplacing, sizeReplacing);
		sl_uint8* fnCallHooked = (sl_uint8*)(Base::createMemory(sizeCallHooked));
		if (!fnCallHooked) {
			return sl_false;
		}
		Generate_CallHookedFunction(fnCallHooked, targetFunctionAddress, codesReplacing, sizeReplacing);

		if (MemoryProtection::setExecuteReadWrite(fnCallHooked, sizeCallHooked)) {
			if (MemoryProtection::setExecuteReadWrite(targetFunctionAddress, sizeReplacing)) {
				if (outCallHookedFunction) {
					*((void* volatile*)outCallHookedFunction) = fnCallHooked;
				}
				Base::copyMemory((void*)targetFunctionAddress, codesReplacing, sizeReplacing);
				return sl_true;
			}
		}

		Base::freeMemory(fnCallHooked);
		return sl_false;
	}

	sl_bool Hook::hookFunction(sl_uint32 targetFunctionRVA, const void* newFunctionAddress, void* outCallHookedFunction)
	{
		const void* base = Module::getBaseAddress();
		if (base) {
			return hookFunction((sl_uint8*)base + targetFunctionRVA, newFunctionAddress, outCallHookedFunction);
		}
		return sl_false;
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

	sl_bool Hook::injectDllIntoRemoteProcess(sl_uint32 processId, const StringParam& _pathDll)
	{
		HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_CREATE_THREAD | PROCESS_VM_OPERATION | PROCESS_VM_WRITE, FALSE, (DWORD)processId);
		if (!hProcess) {
			return sl_false;
		}
		sl_bool bRet = sl_false;
		StringCstr16 pathDll(_pathDll);
		DWORD sizePathDll = (DWORD)((pathDll.getLength() + 1) << 1);
		LPVOID pMemProcessDllPath = VirtualAllocEx(hProcess, NULL, sizePathDll, MEM_COMMIT, PAGE_READWRITE);
		if (pMemProcessDllPath) {
			if (WriteProcessMemory(hProcess, pMemProcessDllPath, pathDll.getData(), sizePathDll, NULL)) {
				void* lib = kernel32::getLibrary();
				if (lib) {
					void* fnLoadDll = GetProcAddress((HMODULE)lib, "LoadLibraryW");
					if (fnLoadDll) {
						HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0, (PTHREAD_START_ROUTINE)fnLoadDll, pMemProcessDllPath, 0, NULL);
						if (hThread) {
							WaitForSingleObject(hThread, INFINITE);
							CloseHandle(hThread);
							bRet = sl_true;
						}
					}
				}
			}
			VirtualFreeEx(hProcess, pMemProcessDllPath, 0, MEM_RELEASE);
		}
		CloseHandle(hProcess);
		return bRet;
	}

}

#endif
