#ifndef CHECKHEADER_SLIB_DEV_PE_UTILS
#define CHECKHEADER_SLIB_DEV_PE_UTILS

#include "definition.h"
#include "../core/map.h"
#include "../dev/pe.h"
#include "../core/memory.h"
#include "../core/string.h"

namespace slib
{
	
	class SLIB_EXPORT PE_Utils
	{
	public:
		static Memory generateShellCode(const void* obj, sl_size size, const StringParam& entryFuntionName);

		static Memory generateShellCodeFromFile(const StringParam& filePath, const StringParam& entryFuntionName);

		static sl_uint32 getObjSectionVirtualOffset(void* section, sl_int32 sectionIndex);

		static String getObjSymbolName(const void* baseAddress, sl_uint32 symbolIndex);

		static PE_Symbol* getObjSymbol(const void* baseAddress, sl_uint32 symbolIndex);

		static PE_Symbol* findSymbol(const void* baseAddress, const StringParam& symbolName);

		static void optimizeSections(const void* baseAddress, const StringParam& entryFuntionName);

		static void enumerateValidSections(const void* baseAddress, Map<sl_uint32, sl_uint32> validSections);
		
		
	};

}

#endif