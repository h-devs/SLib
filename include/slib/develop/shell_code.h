#ifndef CHECKHEADER_SLIB_DEV_PE_UTILS
#define CHECKHEADER_SLIB_DEV_PE_UTILS

#include "definition.h"

#include "../core/memory.h"
#include "../core/string.h"

#define JMP_SECTION_SIZE				28
#define SIZE_OF_CALL					5

namespace slib
{
	struct ShellCodeRelocTableHeader
	{
		sl_uint32 nRelocsCount;
	};

	struct ShellCodeRelocTableItem
	{
		sl_uint32 OffsetToReloc;
	};

	struct ShellCodeHeader
	{
		sl_uint8 jmpSection[JMP_SECTION_SIZE];
		ShellCodeRelocTableHeader relocTable;
	};

	class SLIB_EXPORT ShellCode
	{
	public:
		static Memory generate(const void* obj, sl_size size, const StringParam& entryFuntionName);

		static Memory generateFromFile(const StringParam& filePath, const StringParam& entryFuntionName);

	};

}

#endif