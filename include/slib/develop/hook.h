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

#if defined(SLIB_ARCH_IS_32BIT)
		// returns old function address
		static void* replaceExportEntry(const void* dllBaseAddress, const char* procName, const void* newFunctionAddress);
#endif

		// returns generated code that can jump to old function
		static void* hookFunction(const void* targetFunctionAddress, const void* newFunctionAddress, sl_uint32 nCodeBytesToBackup);

		// returns generated code that can jump to old function
		static void* hookFunction(sl_uint32 targetFunctionRVA, const void* newFunctionAddress, sl_uint32 nCodeBytesToBackup);

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