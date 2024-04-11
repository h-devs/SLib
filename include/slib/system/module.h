#ifndef CHECKHEADER_SLIB_SYSTEM_MODULE
#define CHECKHEADER_SLIB_SYSTEM_MODULE

#include "definition.h"

#include "../core/string.h"

namespace slib
{

	class SLIB_EXPORT ModuleDescription
	{
	public:
		String imagePath;
		void* baseAddress;
		sl_size imageSize;

	public:
		ModuleDescription();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(ModuleDescription)

	};

	class SLIB_EXPORT Module
	{
	public:
		static const void* getBaseAddress();

		static const void* getBaseAddress(const char* moduleName);

		static List<ModuleDescription> getAllModules(sl_uint32 processId, sl_bool flagQueryImagePath = sl_true, sl_bool flagQueryBaseAddressAndSize = sl_true);

	};

}

#endif