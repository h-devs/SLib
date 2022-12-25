#ifndef CHECKHEADER_SLIB_DEV_PE_UTILS
#define CHECKHEADER_SLIB_DEV_PE_UTILS

#include "definition.h"

#include "../core/memory.h"
#include "../core/string.h"

namespace slib
{

	class SLIB_EXPORT ShellCode
	{
	public:
		static Memory generate(const void* obj, sl_size size, const StringParam& entryFuntionName);

		static Memory generateFromFile(const StringParam& filePath, const StringParam& entryFuntionName);

	};

}

#endif