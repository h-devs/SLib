/*
*   Copyright (c) 2008-2020 SLIBIO <https://github.com/SLIBIO>
*
*   Permission is hereby granted, free of charge, to any person obtaining a copy
*   of this software and associated documentation files (the "Software"), to deal
*   in the Software without restriction, including without limitation the rights
*   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
*   copies of the Software, and to permit persons to whom the Software is
*   furnished to do so, subject to the following conditions:
*
*   The above copyright notice and this permission notice shall be included in
*   all copies or substantial portions of the Software.
*
*   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
*   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
*   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
*   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
*   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
*   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
*   THE SOFTWARE.
*/

#include "slib/dev/vtable.h"

#include "slib/dev/protect.h"
#include "slib/core/base.h"

namespace slib
{

	void* Vtable::getEntry(const void* object, sl_uint32 index)
	{
		return (*((void***)object))[index];
	}

	void* Vtable::replaceEntry(const void* object, sl_uint32 index, const void* newFunctionAddress)
	{
		void*& fn = (*((void***)object))[index];
		void* fnOld = fn;
		if (MemoryProtection::setReadWrite(&fn, sizeof(fn))) {
			fn = (void*)newFunctionAddress;
			return fnOld;
		}
		return sl_null;
	}

	void Vtable::replaceTable(const void* object, VtableEntry* replacingEntries, sl_uint32 nReplacingEntries)
	{
		const void** vtable = *((const void***)object);
		for (sl_uint32 i = 0; i < nReplacingEntries; i++) {
			VtableEntry& entry = replacingEntries[i];
			const void*& fn = vtable[entry.index];
			if (entry.pOldFunctionAddress) {
				*((const void**)(entry.pOldFunctionAddress)) = fn;
			}
			if (MemoryProtection::setReadWrite(&fn, sizeof(fn))) {
				fn = entry.newFunctionAddress;
			}
		}
	}

	void Vtable::replaceTable(const void* object, const void* _vtable, sl_uint32 nTotalEntries, VtableEntry* replacingEntries, sl_uint32 nReplacingEntries)
	{
		const void*** pVtableSrc = (const void***)object;
		const void** vtable = (const void**)_vtable;
		Base::copyMemory(vtable, *pVtableSrc, nTotalEntries * sizeof(void*));
		for (sl_uint32 i = 0; i < nReplacingEntries; i++) {
			VtableEntry& entry = replacingEntries[i];
			const void*& fn = vtable[entry.index];
			if (entry.pOldFunctionAddress) {
				*((const void**)(entry.pOldFunctionAddress)) = fn;
			}
			fn = entry.newFunctionAddress;
		}
		*pVtableSrc = vtable;
	}

	void** Vtable::replaceTable(const void* object, sl_uint32 nTotalEntries, VtableEntry* replacingEntries, sl_uint32 nReplacingEntries)
	{
		void** vtable = (void**)(Base::createMemory(sizeof(void*) * nTotalEntries));
		if (vtable) {
			replaceTable(object, vtable, nTotalEntries, replacingEntries, nReplacingEntries);
			return vtable;
		}
		return sl_null;
	}

}
