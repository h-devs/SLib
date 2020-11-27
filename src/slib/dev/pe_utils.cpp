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

namespace slib
{

	sl_uint32 PE_Utils::getObjSectionVirtualOffset(void* base, sl_int32 sectionIndex)
	{
		PE_Header* header = (PE_Header*)base;
		sl_uint8* sectionBase = (sl_uint8*)base + sizeof(PE_Header);
		sl_uint32 ret = 0;
		if (sectionIndex > 0) {
			for (sl_int32 i = 0; i < sectionIndex - 1; i++) {
				PE_SectionHeader* sectionHeader = (PE_SectionHeader*)sectionBase;
				if (Base::compareString((char*)(sectionHeader->name), "text") == 0) {
					sl_uint32 sizeOfRawData = sectionHeader->sizeOfRawData;
					if (header->machine == SLIB_COFF_MACHINE_I386) {
						sizeOfRawData = ((sizeOfRawData - 1) | 3) + 1;
					} else if (header->machine == SLIB_COFF_MACHINE_AMD64 || header->machine == SLIB_COFF_MACHINE_IA64) {
						sizeOfRawData = ((sizeOfRawData - 1) | 7) + 1;
					}
					ret += sizeOfRawData;
				}
				sectionBase += sizeof(PE_SectionHeader);
			}
		}
		return ret;
	}

	Memory PE_Utils::generateShellCode(const void* obj, sl_size size, const StringParam& entryFuntionName)
	{
		PE_Header* header = (PE_Header*)obj;
		sl_uint8* base = (sl_uint8*)obj;
		sl_uint8* sectionBase = base + sizeof(PE_Header);
		
		MemoryBuffer shellCodeBuffer;
		
		PE_Symbol* entrySymbol = findSymbol(base, entryFuntionName);
		sl_uint32 shellCodeEntryPoint = getObjSectionVirtualOffset(base, entrySymbol->sectionNumber) + 3;

		Memory entryCode = Memory::create(8);
		sl_uint8 jmpCode = 0xE9;
		entryCode.write(0, 1, &jmpCode);
		entryCode.write(1, 4, &shellCodeEntryPoint);

		shellCodeBuffer.add(entryCode);

		for (sl_uint32 sectionIndex = 0; sectionIndex < header->numberOfSections; sectionIndex++) {

			PE_SectionHeader* sectionHeader = (PE_SectionHeader*)sectionBase;
			sl_uint32 sizeOfRawData = sectionHeader->sizeOfRawData;
			sl_uint32 pointerToRawData = sectionHeader->pointerToRawData;

			if (Base::compareString((char*)sectionHeader->name, "text") == 0) {
				sl_uint8* relocBase = base + sectionHeader->pointerToRelocations;
				for (sl_uint32 relocIndex = 0; relocIndex < sectionHeader->numberOfRelocations; relocIndex++) {
					PE_SectionRelocator* reloc = (PE_SectionRelocator*)relocBase;
					if (reloc->type == SLIB_PE_RELOC_I386_REL32 || reloc->type == SLIB_PE_REL_AMD64_REL32) {
						PE_Symbol* symbol = getObjSymbol(base, reloc->symbolTableIndex);
						sl_uint32 relocVirtualOffset = reloc->virtualAddress;
						sl_uint8* relocBaseOffset = base + pointerToRawData + relocVirtualOffset;

						sl_uint32 targetAddr = getObjSectionVirtualOffset(base, symbol->sectionNumber);
						sl_uint32 currentAddr = getObjSectionVirtualOffset(base, sectionIndex) + relocVirtualOffset;
						sl_uint32 newAddr = targetAddr - currentAddr - 4; // 4: CALL Address size

						Base::copyMemory(relocBaseOffset, &newAddr, 4);
					}
					relocBase += sizeof(PE_SectionRelocator);
				}
				if (header->machine == SLIB_COFF_MACHINE_I386) {
					sizeOfRawData = ((sizeOfRawData - 1) | 3) + 1;
				} else if (header->machine == SLIB_COFF_MACHINE_AMD64 || header->machine == SLIB_COFF_MACHINE_IA64) {
					sizeOfRawData = ((sizeOfRawData - 1) | 7) + 1;
				}
				shellCodeBuffer.addStatic(base + pointerToRawData, sizeOfRawData);
			}
			sectionBase += sizeof(PE_SectionHeader);
		}
		return shellCodeBuffer.merge();
	}

	Memory PE_Utils::generateShellCodeFromFile(const StringParam& filePath, const StringParam& entryFuntionName)
	{
		Memory fileContent = File::readAllBytes(filePath);
		if (fileContent.isNull()) {
			return sl_null;
		}
		return generateShellCode(fileContent.getData(), fileContent.getSize(), entryFuntionName);
	}


	PE_Symbol* PE_Utils::getObjSymbol(const void* baseAddress, sl_uint32 symbolIndex)
	{
		sl_uint8* base = (sl_uint8*)baseAddress;
		PE_Header* header = (PE_Header*)baseAddress;
		sl_uint32 pointerToSymbolTable = header->pointerToSymbolTable;

		sl_uint8* symbolBase = (base + pointerToSymbolTable);
		return (PE_Symbol*)(symbolBase + sizeof(PE_Symbol) * symbolIndex);
	}

	PE_Symbol* PE_Utils::findSymbol(const void* baseAddress, const StringParam& symbolName)
	{
		sl_uint8* base = (sl_uint8*)baseAddress;
		PE_Header* header = (PE_Header*)baseAddress;
		sl_uint32 numberOfSymbols = header->numberOfSymbols;
		
		sl_uint32 symbolIndex = 0;
		while (symbolIndex < numberOfSymbols) {
			String name = getObjSymbolName(baseAddress, symbolIndex);
			if (name.contains(symbolName)) {
				return getObjSymbol(baseAddress, symbolIndex);
			}
			symbolIndex++;
		}
		return sl_null;
	}

	String PE_Utils::getObjSymbolName(const void* baseAddress, sl_uint32 symbolIndex)
	{
		sl_uint8* base = (sl_uint8*)baseAddress;
		PE_Header* header = (PE_Header*)baseAddress;
		sl_uint32 numberOfSymbols = header->numberOfSymbols;
		sl_uint32 pointerToSymbolTable = header->pointerToSymbolTable;

		sl_uint8* symbolBase = (base + pointerToSymbolTable);
		PE_Symbol* symbol = (PE_Symbol*)(symbolBase + sizeof(PE_Symbol) * symbolIndex);
		sl_uint8* symbolStringBase = (symbolBase + sizeof(PE_Symbol) * numberOfSymbols);
		char *strName = sl_null;

		if (symbol->name.longName[0] == 0 && symbol->name.longName[1] != 0) {
			strName = (char*)(symbolStringBase + symbol->name.longName[1]);
			sl_int32 nameLen = Base::getStringLength(strName);
			return String::fromUtf8(strName, nameLen);
		} else {
			strName = (char*)(symbol->name.shortName);
			sl_int32 nameLen = Base::getStringLength(strName, 8);
			return String::fromUtf8(strName, nameLen);
		}
		return sl_null;
	}
}
