#ifndef CHECKHEADER_SLIB_DEV_PROTECT
#define CHECKHEADER_SLIB_DEV_PROTECT

#include "definition.h"

namespace slib
{

	class MemoryProtection
	{
	public:
		static sl_bool setProtection(const void* address, sl_size size, sl_bool flagExecute, sl_bool flagWrite, sl_bool flagRead = sl_true, sl_bool flagWriteCopy = sl_false);
		
		static sl_bool setNoAccess(const void* address, sl_size size);

		static sl_bool setReadOnly(const void* address, sl_size size);

		static sl_bool setReadWrite(const void* address, sl_size size);
		
		static sl_bool setWriteCopy(const void* address, sl_size size);
		
		static sl_bool setExecute(const void* address, sl_size size);

		static sl_bool setExecuteRead(const void* address, sl_size size);

		static sl_bool setExecuteReadWrite(const void* address, sl_size size);
		
		static sl_bool setExecuteWriteCopy(const void* address, sl_size size);

	};

}

#endif