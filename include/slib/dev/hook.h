#ifndef CHECKHEADER_SLIB_DEV_HOOK
#define CHECKHEADER_SLIB_DEV_HOOK

#include "definition.h"

namespace slib
{
	
	class SLIB_EXPORT Hook
	{
	public:
		static const void* getBaseAddress();

		static const void* getBaseAddress(const char* moduleName);

		static sl_bool replaceImportEntry(const void* moduleBaseAddress, const char* dllName, const char* procName, const void* newFunctionAddress);

		static sl_bool replaceImportEntry(const char* dllName, const char* procName, const void* newFunctionAddress);

		// returns old function address
		static const void* replaceVtableEntry(void* object, sl_uint32 index, const void* newFunctionAddress);
		
		struct VtableEntry
		{
			sl_uint32 index;
			const void* newFunctionAddress;
			const void* pOldFunctionAddress;
		};
		static void replaceVtable(void* object, sl_uint32 nTotalEntries, VtableEntry* replacingEntries, sl_uint32 nReplacingEntries);
		
		// returns generated code that can jump to old function
		static const void* hookFunction(void* targetFunctionAddress, const void* newFunctionAddress, sl_uint32 nCodeBytesToBackup);

		// returns generated code that can jump to old function
		static const void* hookFunction(sl_uint32 targetFunctionRVA, const void* newFunctionAddress, sl_uint32 nCodeBytesToBackup);
		
		static sl_bool replaceCode(void* targetAddress, const void* pNewCode, sl_uint32 nCodeBytes);

		static sl_bool replaceCode(sl_uint32 targetRVA, const void* pNewCode, sl_uint32 nCodeBytes);

	};

}

#endif