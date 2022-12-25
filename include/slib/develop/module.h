#ifndef CHECKHEADER_SLIB_DEV_MODULE
#define CHECKHEADER_SLIB_DEV_MODULE

#include "definition.h"

namespace slib
{

	class SLIB_EXPORT Module
	{
	public:
		static const void* getBaseAddress();

		static const void* getBaseAddress(const char* moduleName);

	};

}

#endif