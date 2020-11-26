#ifndef CHECKHEADER_SLIB_DEV_PEB_KERNEL32
#define CHECKHEADER_SLIB_DEV_PEB_KERNEL32

#include "definition.h"

#include "../dev/pe.h"

#include <intrin.h>

namespace slib
{

	void* GetKernel32AddressFromPEB()
	{
#if not defined SLIB_PLATFORM_IS_WIN64
		sl_uint32* peb = (sl_uint32*)__readfsdword(0x30) + 3;
		sl_uint32* pebLoaderDlls = (sl_uint32*)(*peb + 0x14);
		while (1) {
			pebLoaderDlls = (sl_uint32*)*pebLoaderDlls;
			sl_uint32* moduleInformation = (sl_uint32*)*(pebLoaderDlls + 10);
			// 32bit PEB DLL name has kernel32.dll
			if (moduleInformation[0] == 0x0045004b && moduleInformation[1] == 0x004e0052) {
				return (void*)*(pebLoaderDlls + 4);
			}
		}
#elif defined SLIB_PLATFORM_IS_WIN64
		sl_uint64* peb = (sl_uint64*)__readgsqword(0x60) + 3;
		sl_uint64* pebLoaderDlls = (sl_uint64*)(*peb + 0x10);
		while (1) {
			pebLoaderDlls = (sl_uint64*)*pebLoaderDlls;
			sl_uint64* moduleInformation = (sl_uint64*)*(pebLoaderDlls + 10);
			// 64bit PEB DLL name is c:\\windows\\system32\\kernel32.dll
			if (moduleInformation[5] == 0x004e00520045004b) {
				return (void*)*(pebLoaderDlls + 6);
			}
		}
#endif
		return sl_null;
	}

	void* GetKernel32Function(sl_uint8* _functionName)
	{
		sl_uint8* kernel32Base = (sl_uint8*)getKernel32AddressFromPEB();
		if (kernel32Base != sl_null) {
			PE_DosHeader* dosHeader = (PE_DosHeader*)kernel32Base;
			sl_uint32 offsetOptional = dosHeader->newHeader + 4 + sizeof(PE_Header);
			PE_Header* header = (PE_Header*)(kernel32Base + dosHeader->newHeader + 4);
			PE_DirectoryEntry* exportEntry = 0;

			if (header->machine == SLIB_COFF_MACHINE_I386) {
				PE_OptionalHeader32* optional32 = (PE_OptionalHeader32*)(kernel32Base + offsetOptional);
				exportEntry = optional32->directoryEntry;
			} else if (header->machine == SLIB_COFF_MACHINE_AMD64) {
				PE_OptionalHeader64* optional64 = (PE_OptionalHeader64*)(kernel32Base + offsetOptional);
				exportEntry = optional64->directoryEntry;
			}

			if (exportEntry != sl_null) {
				PE_Export_Directory* exportDirectory = (PE_Export_Directory*)(kernel32Base + exportEntry->address);
				sl_uint32 nameRVA = exportDirectory->addressOfNames;
				sl_uint32 functionRVA = exportDirectory->addressOfFunctions;
				sl_uint32 nameIndexRVA = exportDirectory->addressOfNameOrdinals;

				for (sl_uint32 i = 0; i < exportDirectory->numberOfNames; i++) {
					sl_uint32 nameBase = *(sl_uint32*)(kernel32Base + nameRVA + i * 4);
					sl_uint16 functionIndex = *(sl_uint16*)(kernel32Base + nameIndexRVA + i * 2);
					sl_uint8* exportFunctionName = (sl_uint8*)(kernel32Base + nameBase);
					sl_uint8* functionName = _functionName;

					while (1) {
						if (*exportFunctionName != *functionName) {
							break;
						}
						if (*exportFunctionName == 0 && *functionName == 0) {
							return (void*)(kernel32Base + *(sl_uint32*)(kernel32Base + functionRVA + functionIndex * 4));
						}
						exportFunctionName++;
						functionName++;
					}
				}
			}
		}
		return sl_null;
	}

	void* PEB_GetModuleFileNameA()
	{
		sl_uint32 funcName[0x30] = { 'MteG', 'ludo', 'liFe', 'maNe', 'Ae'};
		return getKernel32Function((sl_uint8*)funcName);
	}

}
#endif