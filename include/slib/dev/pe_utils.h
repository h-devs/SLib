#ifndef CHECKHEADER_SLIB_DEV_PE_UTILS
#define CHECKHEADER_SLIB_DEV_PE_UTILS

#include "definition.h"

#include "../dev/pe.h"

namespace slib
{

	class SLIB_EXPORT PE_Utils
	{
	public:
		static Memory generateShellCode(const void* obj, sl_size size, const StringParam& entryFuntionName);

		static Memory generateShellCodeFromFile(const StringParam& filePath, const StringParam& entryFuntionName);

	};

}

#endif