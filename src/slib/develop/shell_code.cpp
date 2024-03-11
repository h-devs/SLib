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

#include "slib/develop/shell_code.h"

#include "slib/develop/pe.h"
#include "slib/io/file.h"
#include "slib/io/memory_output.h"
#include "slib/core/mio.h"

namespace slib
{

	namespace {
		static Memory GetLinkedCodeSectionContent(Coff& coff, Coff::CodeSectionSet& set, const Coff::CodeSection& section, List<sl_uint32>& listReloc)
		{
			Memory mem = coff.getSectionData(section);
			if (mem.isNull()) {
				return sl_null;
			}
			sl_uint8* data = (sl_uint8*)(mem.getData());
			for (sl_uint32 i = 0; i < section.numberOfRelocations; i++) {
				Coff::SectionRelocation relocation;
				if (!(coff.getSectionRelocation(section, i, relocation))) {
					return sl_null;
				}

				Coff::Symbol* pSymbol = coff.getSymbol(relocation.symbolTableIndex);
				if (!pSymbol) {
					return sl_null;
				}
				sl_uint32 offsetRelocation = relocation.virtualAddress;
				Coff::CodeSection* pSectionRef = set.getSectionByNumber(pSymbol->sectionNumber);
				if (!pSectionRef) {
					return sl_null;
				}
#ifdef SLIB_PLATFORM_IS_WIN64
				Coff::SymbolDesc *desc = pSymbol;
				
				if (relocation.type >= SLIB_PE_REL_AMD64_REL32 && relocation.type <= SLIB_PE_REL_AMD64_REL32_5 ) {
					MIO::writeUint32(data + offsetRelocation, pSectionRef->codeOffset - section.codeOffset - offsetRelocation - relocation.type + pSymbol->value);
				}
#else
				if (relocation.type == SLIB_PE_RELOC_I386_REL32) {
					MIO::writeUint32(data + offsetRelocation, pSectionRef->codeOffset - section.codeOffset - offsetRelocation - 4);
				}
				else if (relocation.type == SLIB_PE_REL_I386_DIR32 || relocation.type == SLIB_PE_REL_I386_DIR32NB )
				{
					// to do 
					if (pSymbol->storageClass == SLIB_IMAGE_SYM_CLASS_EXTERNAL || pSymbol->storageClass == SLIB_IMAGE_SYM_CLASS_STATIC)	{
						MIO::writeUint32(data + offsetRelocation, pSectionRef->codeOffset + pSymbol->value);
					}

					listReloc.add_NoLock(offsetRelocation);
				}
#endif
			}
			return mem;
		}
	}

	Memory ShellCode::generate(const void* obj, sl_size size, const StringParam& entryFuntionName)
	{
		Coff coff;
		if (!(coff.load(obj, size))) {
			return sl_null;
		}

		Coff::Symbol* pSymbolEntry = coff.findSymbol(entryFuntionName);
		if (pSymbolEntry == sl_null) {
			return sl_null;
		}
		Coff::CodeSectionSet sections(coff.getCodeSectionsReferencedFrom(entryFuntionName));
		if (!(sections.count)) {
			return sl_null;
		}

		Coff::CodeSection* pSectionEntry = sections.getSectionByNumber(pSymbolEntry->sectionNumber);
		if (!pSectionEntry) {
			return sl_null;
		}

		sl_uint32 entryCodeOffset = 0;
		{
			sl_uint32 codeOffset = 0;
			for (sl_uint32 i = 0; i < sections.count; i++) {
				Coff::CodeSection& section = sections[i];
				if (pSectionEntry->sectionIndex == section.sectionIndex) {
					continue;
				}
				section.codeOffset = codeOffset;
				codeOffset += section.sizeOfRawData;
			}
			
			// move the SectionEntry to the end 
			pSectionEntry->codeOffset = entryCodeOffset = codeOffset;
			
		}

		
		List<sl_uint32> totalListReloc;
		MemoryOutput sectionWriter;
		{
			for (sl_uint32 i = 0; i < sections.count; i++) {
				Coff::CodeSection& section = sections[i];
				if (pSectionEntry->sectionIndex == section.sectionIndex) {
					continue;
				}
				List<sl_uint32> listReloc;
				//read the raw "SECTION" data using {section} variable
				//all the relocation item's offsets are returned in listReloc list
				Memory mem = GetLinkedCodeSectionContent(coff, sections, section, listReloc);
				if (mem.isNull()) {
					return sl_null;
				}

				//recalculate the relocation item's offset from the shellcode block
				for (auto&& reloc : listReloc) {
					reloc += (sl_uint32)section.codeOffset;
				}
				totalListReloc.addAll_NoLock(listReloc);
				sectionWriter.write(mem);

			}

			// write EntrySection at last position
			List<sl_uint32> listReloc;
			Memory mem = GetLinkedCodeSectionContent(coff, sections, *pSectionEntry, listReloc);
			if (mem.isNull()) {
				return sl_null;
			}
			for (auto&& reloc : listReloc) {
				reloc += (sl_uint32)pSectionEntry->codeOffset;
			}
			totalListReloc.addAll_NoLock(listReloc);
			sectionWriter.write(mem);
		}
		
		sl_size relocSectionSize = 0;
		if (totalListReloc.getCount() > 0) {
			relocSectionSize = totalListReloc.getCount() * sizeof(sl_uint32);
		}

		/************************************************************************/
		/* Build shellcode block from now on                                    */
		/************************************************************************/
		
		// shellcode block structure
		
		/************************************************************************/
		/*  JMP SECTION  (20byte)												*/
		/************************************************************************/
		/*  shellcode block size (4byte)										*/
		/************************************************************************/
		/*  Relocation items count  (4byte)                                     */
		/************************************************************************/
		/*  Relocation items    ( count * sizeof(item) byte)                    */
		/************************************************************************/
		/*  Section 1								                            */
		/*  Section 2															*/
		/*	Section 3															*/		
		/*	.........															*/
		/*	Section n															*/
		/*	Entry Section														*/	
		/************************************************************************/

		MemoryOutput writer;
		{
			
#if !defined SLIB_PLATFORM_IS_WIN64	
			// push eax
			writer.writeUint8(0x50);
#else
			//push ebp
			writer.writeUint8(0x55);
			//sub, rsp, 28h
			writer.writeUint8(0x48);
			writer.writeUint8(0x83);
			writer.writeUint8(0xEC);
			writer.writeUint8(0x40);

			//mov, rcx, rax
			writer.writeUint8(0x48);
			writer.writeUint8(0x8B);
			writer.writeUint8(0xC8);
#endif
			// Call MainEntry
			writer.writeUint8(0xE8);

			sl_size offset = entryCodeOffset + (JMP_SECTION_SIZE - writer.getSize() - (SIZE_OF_CALL - 1)) + sizeof(ShellCodeRelocTableHeader) + relocSectionSize;
			
			writer.writeUint32((sl_uint32)offset);
#if !defined SLIB_PLATFORM_IS_WIN64			
			// pop eax which makes us to original code position
			writer.writeUint8(0x58);
#else
			//add rsp, 28h
			writer.writeUint8(0x48);
			writer.writeUint8(0x83);
			writer.writeUint8(0xC4);
			writer.writeUint8(0x40);
			//pop rbp
			writer.writeUint8(0x5D);
#endif
			//ret
			writer.writeUint8(0xC3);
			
			// alignment
			sl_uint8 zero[16] = { 0 };
			writer.write(zero, JMP_SECTION_SIZE - writer.getSize());
		}
		
		{
			writer.writeUint32((sl_uint32)totalListReloc.getCount());
			sl_size base = writer.getSize() + sizeof(sl_uint32) + totalListReloc.getCount() * sizeof(sl_uint32);
			writer.write(totalListReloc.getData(), totalListReloc.getCount() * sizeof(sl_uint32));
		}
		Memory sectionOut = sectionWriter.merge();
		writer.write(sectionOut.getData(), sectionOut.getSize());

		Memory out = writer.merge();
		sl_size out_size = out.getSize();

		// write shell code's size on alignment space
		out.write(JMP_SECTION_SIZE - 4, 4, &out_size);

		return out;
	}

	Memory ShellCode::generateFromFile(const StringParam& filePath, const StringParam& entryFuntionName)
	{
		Memory fileContent = File::readAllBytes(filePath);
		if (fileContent.isNull()) {
			return sl_null;
		}
		return generate(fileContent.getData(), fileContent.getSize(), entryFuntionName);
	}

}
