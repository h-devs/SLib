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

#include "slib/develop/pe.h"

#include "slib/develop/protect.h"
#include "slib/io/memory_reader.h"
#include "slib/io/priv/seekable_reader_helper.h"
#include "slib/io/file.h"

namespace slib
{

	sl_bool PE::DosHeader::checkSignature() const
	{
		return signature[0] == 'M' && signature[1] == 'Z';
	}

	sl_bool PE::Signature::check() const
	{
		return signature[0] == 'P' && signature[1] == 'E' && signature[2] == 0 && signature[3] == 0;
	}


	PE::PE()
	{
		flag64Bit = sl_false;
		imageBase = 0;
	}

	PE::~PE()
	{
	}

	sl_bool PE::load(const void* _baseAddress, const Ptr<IReader, ISeekable>& _reader)
	{
		if (isLoaded()) {
			return sl_false;
		}

		IReader* reader = _reader;
		if (!reader) {
			return sl_false;
		}
		ISeekable* seeker = _reader;
		if (!seeker) {
			return sl_false;
		}

		if (!(reader->readFully(&dos, sizeof(dos)))) {
			return sl_false;
		}
		if (!(dos.checkSignature())) {
			return sl_false;
		}

		if (!(seeker->seek(dos.newHeader, SeekPosition::Begin))) {
			return sl_false;
		}

		Signature sig;
		if (!(reader->readFully(&sig, sizeof(sig)))) {
			return sl_false;
		}
		if (!(sig.check())) {
			return sl_false;
		}

		if (!(reader->readFully(&header, sizeof(header)))) {
			return sl_false;
		}

		if (header.machine == SLIB_COFF_MACHINE_I386) {
			if (!(reader->readFully(&optional32, sizeof(optional32)))) {
				return sl_false;
			}
			flag64Bit = sl_false;
			imageBase = optional32.imageBase;
		} else if (header.machine == SLIB_COFF_MACHINE_AMD64) {
			if (!(reader->readFully(&optional64, sizeof(optional64)))) {
				return sl_false;
			}
			flag64Bit = sl_true;
			imageBase = optional64.imageBase;
		} else {
			return sl_false;
		}

		offsetToSections = dos.newHeader + sizeof(Signature) + sizeof(Header) + header.sizeOfOptionalHeader;
		offsetToSymbolNames = header.offsetToSymbolTable + header.numberOfSymbols * sizeof(SymbolDesc);

		_init(_baseAddress, _reader);

		return sl_true;
	}

	PE::DirectoryEntry* PE::getImportTableDirectory()
	{
		if (!(isLoaded())) {
			return sl_null;
		}
		if (flag64Bit) {
			return optional64.directoryEntry + SLIB_PE_DIRECTORY_IMPORT_TABLE;
		} else {
			return optional32.directoryEntry + SLIB_PE_DIRECTORY_IMPORT_TABLE;
		}
	}

	PE::DirectoryEntry* PE::getDelayImportDescriptors()
	{
		if (!(isLoaded())) {
			return sl_null;
		}
		if (flag64Bit) {
			return optional64.directoryEntry + SLIB_PE_DIRECTORY_DELAY_IMPORT_DESCRIPTOR;
		} else {
			return optional32.directoryEntry + SLIB_PE_DIRECTORY_DELAY_IMPORT_DESCRIPTOR;
		}
	}

	PE::DirectoryEntry* PE::getExportTableDirectory()
	{
		if (!(isLoaded())) {
			return sl_null;
		}
		if (flag64Bit) {
			return optional64.directoryEntry + SLIB_PE_DIRECTORY_EXPORT_TABLE;
		} else {
			return optional32.directoryEntry + SLIB_PE_DIRECTORY_EXPORT_TABLE;
		}
	}

	PE::ImportDescriptor* PE::findImportTable(const StringView& dllName)
	{
		DirectoryEntry* entry = getImportTableDirectory();
		if (!entry) {
			return sl_null;
		}
		sl_uint8* base = (sl_uint8*)baseAddress;
		sl_uint32 n = entry->size / sizeof(ImportDescriptor);
		ImportDescriptor* import = (ImportDescriptor*)(base + entry->address);
		for (sl_uint32 i = 0; i < n; i++) {
			if (import->dllName) {
				if (dllName.equals_IgnoreCase((char*)(base + import->dllName))) {
					return import;
				}
			}
			import++;
		}
		return sl_null;
	}

	PE::DelayImportDescriptor* PE::findDelayImportDescriptor(const StringView& dllName)
	{
		DirectoryEntry* entry = getDelayImportDescriptors();
		if (!entry) {
			return sl_null;
		}
		sl_uint8* base = (sl_uint8*)baseAddress;
		sl_uint32 n = entry->size / sizeof(DelayImportDescriptor);
		DelayImportDescriptor* import = (DelayImportDescriptor*)(base + entry->address);
		for (sl_uint32 i = 0; i < n; i++) {
			if (import->dllName) {
				if (dllName.equals_IgnoreCase((char*)(base + import->dllName))) {
					return import;
				}
			}
			import++;
		}
		return sl_null;
	}

	sl_uint32* PE::_findExportFunctionOffsetEntry(const StringView& functionName)
	{
		DirectoryEntry* pExportEntry = getExportTableDirectory();
		if (!pExportEntry) {
			return sl_null;
		}
		sl_uint8* base = (sl_uint8*)baseAddress;
		ExportDirectory* pExportDirectory = (ExportDirectory*)(base + pExportEntry->address);
		sl_uint32 nameRVA = pExportDirectory->addressOfNames;
		sl_uint32 funcRVA = pExportDirectory->addressOfFunctions;
		sl_uint32 nameOrdinalRVA = pExportDirectory->addressOfNameOrdinals;
		for (sl_uint32 i = 0; i < pExportDirectory->numberOfNames; i++) {
			sl_uint32 nameBase = *(sl_uint32*)(base + nameRVA + i * 4);
			sl_uint16 funcIndex = *(sl_uint16*)(base + nameOrdinalRVA + i * 2);
			if (functionName.equals_IgnoreCase((char*)(base + nameBase))) {
				return (sl_uint32*)(base + funcRVA + funcIndex * 4);
			}
		}
		return sl_null;
	}

	void* PE::findExportFunction(const StringView& functionName)
	{
		sl_uint32* entry = _findExportFunctionOffsetEntry(functionName);
		if (entry) {
			return (sl_uint8*)baseAddress + *entry;
		}
		return sl_null;
	}

	sl_uint32 PE::updateExportFunctionOffset(const StringView& functionName, sl_uint32 offset)
	{
		sl_uint32* entry = _findExportFunctionOffsetEntry(functionName);
		if (entry) {
			if (MemoryProtection::setReadWrite(entry, 4)) {
				sl_uint32 ret = *entry;
				*entry = offset;
				return ret;
			}
		}
		return 0;
	}


	SLIB_DEFINE_NESTED_CLASS_DEFAULT_MEMBERS(Coff, Section)

	Coff::Section::Section()
	{
	}


	SLIB_DEFINE_NESTED_CLASS_DEFAULT_MEMBERS(Coff, CodeSection)

	Coff::CodeSection::CodeSection()
	{
		sectionIndex = 0;
		codeOffset = 0;
	}


	SLIB_DEFINE_NESTED_CLASS_DEFAULT_MEMBERS(Coff, Symbol)

	Coff::Symbol::Symbol()
	{
	}


	Coff::Coff()
	{
		baseAddress = sl_null;
		offsetToSections = 0;
		offsetToSymbolNames = 0;

		m_reader = sl_null;
		m_seekable = sl_null;

		Base::zeroMemory(&header, sizeof(header));
	}

	Coff::~Coff()
	{
	}

	void Coff::_init(const void* _baseAddress, const Ptr<IReader, ISeekable>& reader)
	{
		baseAddress = (sl_uint8*)_baseAddress;
		m_reader = reader;
		m_seekable = reader;
		m_ref = reader.ref;
	}

	sl_bool Coff::load(const void* _baseAddress, const Ptr<IReader, ISeekable>& _reader)
	{
		if (isLoaded()) {
			return sl_false;
		}

		IReader* reader = _reader;
		if (!reader) {
			return sl_false;
		}
		ISeekable* seeker = _reader;
		if (!seeker) {
			return sl_false;
		}

		if (!(reader->readFully(&header, sizeof(header)))) {
			return sl_false;
		}

		offsetToSections = sizeof(header);
		offsetToSymbolNames = header.offsetToSymbolTable + header.numberOfSymbols * sizeof(SymbolDesc);

		_init(_baseAddress, _reader);

		return sl_true;
	}

	sl_bool Coff::load(const void* _baseAddress, sl_size size)
	{
		return load(_baseAddress, NewRefT<MemoryReader>(_baseAddress, size));
	}

	sl_bool Coff::isLoaded()
	{
		return m_reader != sl_null;
	}

	sl_bool Coff::getSection(sl_uint32 index, Section& outSection)
	{
		if (!(isLoaded())) {
			return sl_false;
		}
		IReader* reader = m_reader;
		ISeekable* seeker = m_seekable;
		if (!(seeker->seek(offsetToSections + index * sizeof(SectionDesc), SeekPosition::Begin))) {
			return sl_false;
		}
		SectionDesc& desc = outSection;
		if (!(reader->readFully(&desc, sizeof(SectionDesc)))) {
			return sl_false;
		}
		outSection.name = String(desc.name, Base::getStringLength(desc.name, 8));
		return sl_true;
	}

	Memory Coff::getSectionData(const SectionDesc& section)
	{
		if (!(isLoaded())) {
			return sl_null;
		}
		IReader* reader = m_reader;
		ISeekable* seeker = m_seekable;
		if (!(seeker->seek(section.offsetToRawData, SeekPosition::Begin))) {
			return sl_null;
		}
		sl_uint32 sizeOfRawData = section.sizeOfRawData;
		Memory mem = reader->readToMemory(sizeOfRawData);
		if (mem.getSize() == sizeOfRawData) {
			return mem;
		}
		return sl_null;
	}

	sl_bool Coff::getSectionRelocation(const SectionDesc& section, sl_uint32 index, SectionRelocation& outRelocation)
	{
		if (!(isLoaded())) {
			return sl_false;
		}
		IReader* reader = m_reader;
		ISeekable* seeker = m_seekable;
		if (!(seeker->seek(section.offsetToRelocations + sizeof(SectionRelocation) * index, SeekPosition::Begin))) {
			return sl_null;
		}
		return reader->readFully(&outRelocation, sizeof(SectionRelocation));
	}

	Coff::Symbol* Coff::getSymbol(sl_uint32 index)
	{
		if (!(isLoaded())) {
			return sl_null;
		}
		if (_loadSymbols()) {
			return m_symbols.getPointerAt(index);
		}
		return sl_null;
	}

	Coff::Symbol* Coff::findSymbol(const StringParam& _name)
	{
		if (!(isLoaded())) {
			return sl_null;
		}
		if (_loadSymbols()) {
			ListElements<Symbol> symbols(m_symbols);
			StringData name(_name);
			for (sl_uint32 i = 0; i < symbols.count; i++) {
				Symbol& symbol = symbols[i];
				if (symbol.name == name) {
					return &symbol;
				}
			}
		}
		return sl_null;
	}


	List<Coff::CodeSection> Coff::getCodeSections()
	{
		if (!(isLoaded())) {
			return sl_null;
		}
		List<Coff::CodeSection> ret;
		sl_uint32 codeOffset = 0;
		for (sl_uint32 i = 0; i < header.numberOfSections; i++) {
			Coff::CodeSection section;
			if (getSection(i, section)) {
				if (section.name == "text" || section.name == ".rdata" || section.name == ".bss" 
					|| section.name == ".data" || section.name == ".pdata" || section.name ==".xdata") {
					section.codeOffset = codeOffset;
					section.sectionIndex = i;

					codeOffset += getCodeSectionSize(section);
					if (!(ret.add_NoLock(Move(section)))) {
						return sl_null;
					}
				}
			}
		}
		return ret;
	}

	sl_bool Coff::getDataSection(Coff::Section &section)
	{
		if (!(isLoaded())) {
			return sl_false;
		}
		sl_uint32 codeOffset = 0;
		for (sl_uint32 i = 0; i < header.numberOfSections; i++) {
			if (getSection(i, section)) {
				if (section.name == ".data") {
					return sl_true;
				}
			}
		}
		return sl_false;
	}

	List<Coff::CodeSection> Coff::getCodeSectionsReferencedFrom(const StringParam& entrySymbolName)
	{
		if (!(isLoaded())) {
			return sl_null;
		}

		IReader* reader = m_reader;
		ISeekable* seeker = m_seekable;

		Coff::CodeSectionSet sections(getCodeSections());
		if (!(sections.count)) {
			return sl_null;
		}

		Coff::Symbol* pSymbolEntry = findSymbol(entrySymbolName);
		if (!pSymbolEntry) {
			return sl_null;
		}

		CMap<sl_uint32, sl_uint32> refSections;
		refSections.add_NoLock(pSymbolEntry->sectionNumber, 0);

		for (;;) {
			sl_bool flagFoundNewSection = sl_false;
			for (auto&& item : refSections) {
				if (item.key) {
					Coff::CodeSection* pSection = sections.getSectionByNumber(item.key);
					if (pSection) {
						if (!(item.value)) {
							item.value = 1;
							flagFoundNewSection = sl_true;
							if (!(seeker->seek(pSection->offsetToRelocations, SeekPosition::Begin))) {
								return sl_null;
							}
							for (sl_uint32 i = 0; i < pSection->numberOfRelocations; i++) {
								Coff::SectionRelocation relocation;
								if (!(reader->readFully(&relocation, sizeof(relocation)))) {
									return sl_null;
								}
#ifdef SLIB_PLATFORM_IS_WIN64
								{
#else
								if (relocation.type == SLIB_PE_RELOC_I386_REL32 || relocation.type == SLIB_PE_REL_I386_DIR32) {
#endif // SLIB_PLATFORM_IS_WIN64
									Coff::Symbol* pSymbol = getSymbol(relocation.symbolTableIndex);
									if (pSymbol) {
										if (!(refSections.find_NoLock(pSymbol->sectionNumber))) {
											refSections.put_NoLock(pSymbol->sectionNumber, 0);
										}
									}
									else {
										return sl_null;
									}
								}
							}
						}
					}
					else {
						return sl_null;
					}
				}

			}
			if (!flagFoundNewSection) {
				break;
			}
		}

		List<Coff::CodeSection> ret;
		{
			for (auto&& item : refSections) {
				if (item.key) {
					Coff::CodeSection* pSection = sections.getSectionByNumber(item.key);
					if (pSection) {
						ret.add_NoLock(*pSection);
					}
					else {
						return sl_null;
					}
				}
			}
		}
		return ret;
	}
	sl_uint32 Coff::getCodeSectionSize(const SectionDesc& section)
	{
		sl_uint32 sizeOfRawData = section.sizeOfRawData;
		if (header.machine == SLIB_COFF_MACHINE_I386) {
			return ((sizeOfRawData - 1) | 3) + 1;
		} else if (header.machine == SLIB_COFF_MACHINE_AMD64 || header.machine == SLIB_COFF_MACHINE_IA64) {
			return ((sizeOfRawData - 1) | 7) + 1;
		}
		return sizeOfRawData;
	}

	sl_bool Coff::_loadSymbols()
	{
		if (m_symbols.isNotNull()) {
			return sl_true;
		}

		IReader* reader = m_reader;
		ISeekable* seeker = m_seekable;

		if (!(seeker->seek(header.offsetToSymbolTable, SeekPosition::Begin))) {
			return sl_false;
		}

		List<Symbol> list;
		for (sl_uint32 i = 0; i < header.numberOfSymbols; i++) {
			Symbol symbol;
			SymbolDesc& desc = symbol;
			if (!(reader->readFully(&desc, sizeof(SymbolDesc)))) {
				return sl_false;
			}
			if (desc.name.longName[0]) {
				// short name
				symbol.name = String(desc.name.shortName, Base::getStringLength(desc.name.shortName, 8));
			} else {
				if (!(seeker->seek(offsetToSymbolNames + desc.name.longName[1], SeekPosition::Begin))) {
					return sl_false;
				}
				symbol.name = SeekableReaderHelper::readNullTerminatedString(reader, seeker);
				if (!(seeker->seek(header.offsetToSymbolTable + (i + 1) * sizeof(SymbolDesc), SeekPosition::Begin))) {
					return sl_false;
				}
			}
			list.add_NoLock(Move(symbol));
		}

		m_symbols = list;
		return sl_true;

	}


	Coff::CodeSectionSet::CodeSectionSet(const List<CodeSection>& sections): ListElements(sections)
	{
		for (sl_uint32 i = 0; i < count; i++) {
			m_mapSectionIndex.put_NoLock(data[i].sectionIndex + 1, (sl_uint32)i);
		}
	}

	Coff::CodeSectionSet::~CodeSectionSet()
	{
	}

	Coff::CodeSection* Coff::CodeSectionSet::getSectionByNumber(sl_uint32 sectionNumber)
	{
		sl_uint32 index;
		if (m_mapSectionIndex.get_NoLock(sectionNumber, &index)) {
			return &(data[index]);
		}
		return sl_null;
	}

}