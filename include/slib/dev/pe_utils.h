#ifndef CHECKHEADER_SLIB_DEV_PE_UTILS
#define CHECKHEADER_SLIB_DEV_PE_UTILS

#include "definition.h"

#include "../core/memory.h"
#include "../core/string.h"
#include "../dev/pe.h"

namespace slib
{
	
	class SLIB_EXPORT PE_Utils
	{
	public:
		static Memory generateShellCode(const void* obj, sl_size size);

		static Memory generateShellCodeFromFile(const StringParam& filePath);

		static sl_uint32 getObjSectionVirtualOffset(void* section, sl_int32 sectionIndex);

		static String getObjSymbolName(const void* baseAddress, sl_uint32 symbolIndex);

		static PE_Symbol* getObjSymbol(const void* baseAddress, sl_uint32 symbolIndex);
	};

}

#endif