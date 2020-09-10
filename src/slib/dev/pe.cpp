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

#include "slib/dev/pe.h"

#include "slib/core/base.h"

namespace slib
{
	
	sl_bool PE_DosHeader::checkSignature() const
	{
		return signature[0] == 'M' && signature[1] == 'Z';
	}

	sl_bool PE_Signature::check() const
	{
		return signature[0] == 'P' && signature[1] == 'E' && signature[2] == 0 && signature[3] == 0;
	}

	PE::PE()
	{
		flagLoadedHeaders = sl_false;
		flag64Bit = sl_false;
		imageBase = 0;
		nSections = 0;
	}

	PE::~PE()
	{
	}

	sl_bool PE::loadHeaders(const void* dataExe, sl_size sizeExe)
	{
		sl_uint8* data = (sl_uint8*)dataExe;

		if (sizeExe < sizeof(PE_DosHeader)) {
			return sl_false;
		}
		Base::copyMemory(&dos, data, sizeof(dos));
		if (!(dos.checkSignature())) {
			return sl_false;
		}
		data += dos.newHeader;

		sl_size offsetOptional = dos.newHeader + sizeof(PE_Signature) + sizeof(PE_Header);
		if (offsetOptional > sizeExe) {
			return sl_false;
		}

		PE_Signature sig;
		Base::copyMemory(&sig, data, sizeof(sig));
		if (!(sig.check())) {
			return sl_false;
		}
		data += sizeof(sig);
		
		Base::copyMemory(&header, data, sizeof(header));
		data += sizeof(header);

		sl_size offsetSectionHeader = offsetOptional + header.sizeOfOptionalHeader;
		if (sizeExe < offsetSectionHeader) {
			return sl_false;
		}
		if (header.machine == SLIB_COFF_MACHINE_I386) {
			if (sizeExe < offsetOptional + sizeof(PE_OptionalHeader32)) {
				return sl_false;
			}
			Base::copyMemory(&optional32, data, sizeof(optional32));
			flag64Bit = sl_false;
			imageBase = optional32.imageBase;
		} else if (header.machine == SLIB_COFF_MACHINE_AMD64) {
			if (sizeExe < offsetOptional + sizeof(PE_OptionalHeader64)) {
				return sl_false;
			}
			Base::copyMemory(&optional64, data, sizeof(optional64));
			flag64Bit = sl_true;
			imageBase = optional64.imageBase;
		} else {
			return sl_false;
		}
		data += header.sizeOfOptionalHeader;

		{
			nSections = header.numberOfSections;
			sl_uint32 nSectionsMax = (sl_uint32)(CountOfArray(sections));
			if (nSections > nSectionsMax) {
				nSections = nSectionsMax;
			}
			if (offsetSectionHeader + nSections * sizeof(PE_SectionHeader) > sizeExe) {
				return sl_false;
			}
			for (sl_uint32 i = 0; i < nSections; i++) {
				Base::copyMemory(sections + i, data, sizeof(PE_SectionHeader));
				data += sizeof(PE_SectionHeader);
			}
		}

		flagLoadedHeaders = sl_true;
		return sl_true;
	}
	
	PE_DirectoryEntry* PE::getImportTableDirectory()
	{
		if (!flagLoadedHeaders) {
			return sl_null;
		}
		if (flag64Bit) {
			return optional64.directoryEntry + SLIB_PE_DIRECTORY_IMPORT_TABLE;
		} else {
			return optional32.directoryEntry + SLIB_PE_DIRECTORY_IMPORT_TABLE;
		}
	}

	PE_ImportDescriptor* PE::findImportTable(const void* baseAddress, const char* dllName)
	{		
		PE_DirectoryEntry* pImportEntry = getImportTableDirectory();
		if (!pImportEntry) {
			return sl_null;
		}
		sl_uint8* base = (sl_uint8*)baseAddress;
		sl_uint32 n = pImportEntry->size / sizeof(PE_ImportDescriptor);
		PE_ImportDescriptor* import = (PE_ImportDescriptor*)(base + pImportEntry->address);
		for (sl_uint32 i = 0; i < n; i++) {
			if (Base::equalsStringIgnoreCase((char*)(base + import->name), dllName)) {
				return import;
			}
			import++;
		}
		return sl_null;
	}

}