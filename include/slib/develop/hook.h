#ifndef CHECKHEADER_SLIB_DEV_HOOK
#define CHECKHEADER_SLIB_DEV_HOOK

#include "definition.h"

namespace slib
{

	class StringParam;

	class SLIB_EXPORT Hook
	{
	public:
		// returns old function address
		static void* replaceImportEntry(const void* moduleBaseAddress, const char* dllName, const char* procName, const void* newFunctionAddress);

		// returns old function address
		static void* replaceImportEntry(const char* dllName, const char* procName, const void* newFunctionAddress);

		// replaces import entries of all modules
		static void replaceAllImportEntries(const char* dllName, const char* procName, const void* newFunctionAddress);

		// returns old function address
		static void* replaceExportEntry(const void* dllBaseAddress, const char* procName, sl_uint32 newFunctionOffset);
#if defined(SLIB_ARCH_IS_32BIT)
		static void* replaceExportEntry(const void* dllBaseAddress, const char* procName, const void* newFunctionAddress);
#endif

		static sl_bool hookFunction(const void* targetFunctionAddress, const void* newFunctionAddress, void* outCallHookedFunction);

		static sl_bool hookFunction(sl_uint32 targetFunctionRVA, const void* newFunctionAddress, void* outCallHookedFunction);

		// returns old jump address
		static void* hookJmpNear(const void* targetAddress, const void* newAddress);

		// returns old jump address
		static void* hookJmpNear(sl_uint32 targetRVA, const void* newAddress);

		static sl_bool replaceCode(const void* targetAddress, const void* pNewCode, sl_uint32 nCodeBytes);

		static sl_bool replaceCode(sl_uint32 targetRVA, const void* pNewCode, sl_uint32 nCodeBytes);


		static sl_bool injectDllIntoRemoteProcess(sl_uint32 processId, const StringParam& pathDll);

	};

}

#endif