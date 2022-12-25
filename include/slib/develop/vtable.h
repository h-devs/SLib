#ifndef CHECKHEADER_SLIB_DEV_VTABLE
#define CHECKHEADER_SLIB_DEV_VTABLE

#include "definition.h"

namespace slib
{

	struct SLIB_EXPORT VtableEntry
	{
		sl_uint32 index;
		const void* newFunctionAddress;
		const void* pOldFunctionAddress;
	};

	class SLIB_EXPORT Vtable
	{
	public:
		static void* getEntry(const void* object, sl_uint32 index);

		// returns old function address
		static void* replaceEntry(const void* object, sl_uint32 index, const void* newFunctionAddress);

		static void replaceTable(const void* object, VtableEntry* replacingEntries, sl_uint32 nReplacingEntries);

		static void replaceTable(const void* object, const void* vtableDst, sl_uint32 nTotalEntries, VtableEntry* replacingEntries, sl_uint32 nReplacingEntries);

		// returns new vtable
		static void** replaceTable(const void* object, sl_uint32 nTotalEntries, VtableEntry* replacingEntries, sl_uint32 nReplacingEntries);

	};

}

#endif