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

#include "slib/dev/pe_utils.h"

#include "slib/core/file.h"
#include "slib/core/mio.h"

namespace slib
{

	namespace priv
	{
		namespace pe_utils
		{

			static Memory GetLinkedCodeSectionContent(Coff& coff, CoffCodeSectionSet& set, const CoffCodeSection& section)
			{
				Memory mem = coff.getSectionData(section);
				if (mem.isNull()) {
					return sl_null;
				}
				sl_uint8* data = (sl_uint8*)(mem.getData());
				for (sl_uint32 i = 0; i < section.numberOfRelocations; i++) {
					CoffSectionRelocation relocation;
					if (!(coff.getSectionRelocation(section, i, relocation))) {
						return sl_null;
					}
					if (relocation.type == SLIB_PE_RELOC_I386_REL32 || relocation.type == SLIB_PE_REL_AMD64_REL32) {
						CoffSymbol* pSymbol = coff.getSymbol(relocation.symbolTableIndex);
						if (!pSymbol) {
							return sl_null;
						}
						sl_uint32 offsetRelocation = relocation.virtualAddress;
						CoffCodeSection* pSectionRef = set.getSectionByNumber(pSymbol->sectionNumber);
						if (!pSectionRef) {
							return sl_null;
						}
						MIO::writeUint32(data + offsetRelocation, pSectionRef->codeOffset - section.codeOffset - 4);
					}
				}
				return mem;
			}

		}
	}

	using namespace priv::pe_utils;

	Memory PE_Utils::generateShellCode(const void* obj, sl_size size, const StringParam& entryFuntionName)
	{
		Coff coff;
		if (!(coff.load(obj, size))) {
			return sl_null;
		}

		CoffSymbol* pSymbolEntry = coff.findSymbol(entryFuntionName);
		if (!pSymbolEntry) {
			return sl_null;
		}
		CoffCodeSectionSet sections(coff.getCodeSectionsReferencedFrom(entryFuntionName));
		if (!(sections.count)) {
			return sl_null;
		}
		CoffCodeSection* pSectionEntry = sections.getSectionByNumber(pSymbolEntry->sectionNumber);
		if (!pSectionEntry) {
			return sl_null;
		}

		MemoryWriter writer;
		{
			writer.writeUint8(0xE9);
			writer.writeUint16(pSectionEntry->codeOffset + 3);
			sl_uint8 zero[11] = { 0 };
			writer.write(zero, 11);
		}

		for (sl_uint32 i = 0; i < sections.count; i++) {
			CoffCodeSection& section = sections[i];
			Memory mem = GetLinkedCodeSectionContent(coff, sections, section);
			if (mem.isNull()) {
				return sl_null;
			}
			writer.write(mem);
			sl_uint8 zero[7] = { 0 };
			if (coff.header.machine == SLIB_COFF_MACHINE_I386) {
				writer.write(zero, ((section.sizeOfRawData - 1) | 3) + 1 - section.sizeOfRawData);
			} else if (coff.header.machine == SLIB_COFF_MACHINE_AMD64 || coff.header.machine == SLIB_COFF_MACHINE_IA64) {
				writer.write(zero, ((section.sizeOfRawData - 1) | 7) + 1 - section.sizeOfRawData);
			}
		}

		return writer.getData();
	}

	Memory PE_Utils::generateShellCodeFromFile(const StringParam& filePath, const StringParam& entryFuntionName)
	{
		Memory fileContent = File::readAllBytes(filePath);
		if (fileContent.isNull()) {
			return sl_null;
		}
		return generateShellCode(fileContent.getData(), fileContent.getSize(), entryFuntionName);
	}

}
