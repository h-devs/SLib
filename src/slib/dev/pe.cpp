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

#include "slib/core/memory_reader.h"

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

		PE_Signature sig;
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

		offsetToSections = dos.newHeader + sizeof(PE_Signature) + sizeof(CoffHeader) + header.sizeOfOptionalHeader;
		offsetToSymbolNames = header.offsetToSymbolTable + header.numberOfSymbols * sizeof(CoffSymbolDesc);

		_init(_baseAddress, _reader);

		return sl_true;
	}

	PE_DirectoryEntry* PE::getImportTableDirectory()
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

	PE_DirectoryEntry* PE::getExportTableDirectory()
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

	PE_ImportDescriptor* PE::findImportTable(const StringParam& _dllName)
	{
		PE_DirectoryEntry* pImportEntry = getImportTableDirectory();
		if (!pImportEntry) {
			return sl_null;
		}
		sl_uint8* base = (sl_uint8*)baseAddress;
		sl_uint32 n = pImportEntry->size / sizeof(PE_ImportDescriptor);
		PE_ImportDescriptor* import = (PE_ImportDescriptor*)(base + pImportEntry->address);
		StringData dllName(_dllName);
		for (sl_uint32 i = 0; i < n; i++) {
			if (dllName.equalsIgnoreCase((char*)(base + import->name))) {
				return import;
			}
			import++;
		}
		return sl_null;
	}

	void* PE::findExportFunction(const StringParam& _functionName)
	{
		PE_DirectoryEntry* pExportEntry = getExportTableDirectory();
		if (!pExportEntry) {
			return sl_null;
		}
		sl_uint8* base = (sl_uint8*)baseAddress;
		PE_ExportDirectory* pExportDirectory = (PE_ExportDirectory*)(base + pExportEntry->address);
		sl_uint32 nameRVA = pExportDirectory->addressOfNames;
		sl_uint32 funcRVA = pExportDirectory->addressOfFunctions;
		sl_uint32 nameOrdinalRVA = pExportDirectory->addressOfNameOrdinals;
		StringData functionName(_functionName);
		for (sl_uint32 i = 0; i < pExportDirectory->numberOfNames; i++) {
			sl_uint32 nameBase = *(sl_uint32*)(base + nameRVA + i * 4);
			sl_uint16 funcIndex = *(sl_uint16*)(base + nameOrdinalRVA + i * 2);
			if (functionName.equalsIgnoreCase((char*)(base + nameBase))) {
				return (base + *(sl_uint32*)(base + funcRVA + funcIndex * 4));
			}
		}
		return sl_null;
	}


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(CoffSection)

	CoffSection::CoffSection()
	{
	}


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(CoffCodeSection)

	CoffCodeSection::CoffCodeSection()
	{
		sectionIndex = 0;
		codeOffset = 0;
	}


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(CoffSymbol)

	CoffSymbol::CoffSymbol()
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
		offsetToSymbolNames = header.offsetToSymbolTable + header.numberOfSymbols * sizeof(CoffSymbolDesc);

		_init(_baseAddress, _reader);

		return sl_true;
	}

	sl_bool Coff::load(const void* _baseAddress, sl_size size)
	{
		return load(_baseAddress, ToRef(new MemoryReader(_baseAddress, size)));
	}

	sl_bool Coff::isLoaded()
	{
		return m_reader != sl_null;
	}

	sl_bool Coff::getSection(sl_uint32 index, CoffSection& outSection)
	{
		if (!(isLoaded())) {
			return sl_false;
		}
		IReader* reader = m_reader;
		ISeekable* seeker = m_seekable;
		if (!(seeker->seek(offsetToSections + index * sizeof(CoffSectionDesc), SeekPosition::Begin))) {
			return sl_false;
		}
		CoffSectionDesc& desc = outSection;
		if (!(reader->readFully(&desc, sizeof(CoffSectionDesc)))) {
			return sl_false;
		}
		outSection.name = String(desc.name, Base::getStringLength(desc.name, 8));
		return sl_true;
	}

	Memory Coff::getSectionData(const CoffSectionDesc& section)
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

	sl_bool Coff::getSectionRelocation(const CoffSectionDesc& section, sl_uint32 index, CoffSectionRelocation& outRelocation)
	{
		if (!(isLoaded())) {
			return sl_false;
		}
		IReader* reader = m_reader;
		ISeekable* seeker = m_seekable;
		if (!(seeker->seek(section.offsetToRelocations + sizeof(CoffSectionRelocation) * index, SeekPosition::Begin))) {
			return sl_null;
		}
		return reader->readFully(&outRelocation, sizeof(CoffSectionRelocation));
	}

	CoffSymbol* Coff::getSymbol(sl_uint32 index)
	{
		if (!(isLoaded())) {
			return sl_null;
		}
		if (_loadSymbols()) {
			return m_symbols.getPointerAt(index);
		}
		return sl_null;
	}

	CoffSymbol* Coff::findSymbol(const StringParam& _name)
	{
		if (!(isLoaded())) {
			return sl_null;
		}
		if (_loadSymbols()) {
			ListElements<CoffSymbol> symbols(m_symbols);
			StringData name(_name);
			for (sl_uint32 i = 0; i < symbols.count; i++) {
				CoffSymbol& symbol = symbols[i];
				if (symbol.name == name) {
					return &symbol;
				}
			}
		}
		return sl_null;
	}

	List<CoffCodeSection> Coff::getCodeSections()
	{
		if (!(isLoaded())) {
			return sl_null;
		}
		List<CoffCodeSection> ret;
		sl_uint32 codeOffset = 0;
		for (sl_uint32 i = 0; i < header.numberOfSections; i++) {
			CoffCodeSection section;
			if (getSection(i, section)) {
				if (section.name == "text") {
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

	List<CoffCodeSection> Coff::getCodeSectionsReferencedFrom(const StringParam& entrySymbolName)
	{
		if (!(isLoaded())) {
			return sl_null;
		}

		IReader* reader = m_reader;
		ISeekable* seeker = m_seekable;

		CoffCodeSectionSet sections(getCodeSections());
		if (!(sections.count)) {
			return sl_null;
		}

		CoffSymbol* pSymbolEntry = findSymbol(entrySymbolName);
		if (!pSymbolEntry) {
			return sl_null;
		}

		CMap<sl_uint32, sl_uint32> refSections;
		refSections.add_NoLock(pSymbolEntry->sectionNumber, 0);

		for (;;) {
			sl_bool flagFoundNewSection = sl_false;
			for (auto& item : refSections) {
				CoffCodeSection* pSection = sections.getSectionByNumber(item.key);
				if (pSection) {
					if (!(item.value)) {
						item.value = 1;
						flagFoundNewSection = sl_true;
						if (!(seeker->seek(pSection->offsetToRelocations, SeekPosition::Begin))) {
							return sl_null;
						}
						for (sl_uint32 i = 0; i < pSection->numberOfRelocations; i++) {
							CoffSectionRelocation relocation;
							if (!(reader->readFully(&relocation, sizeof(relocation)))) {
								return sl_null;
							}
							if (relocation.type == SLIB_PE_RELOC_I386_REL32 || relocation.type == SLIB_PE_REL_AMD64_REL32) {
								CoffSymbol* pSymbol = getSymbol(relocation.symbolTableIndex);
								if (pSymbol) {
									if (!(refSections.find_NoLock(pSymbol->sectionNumber))) {
										refSections.put_NoLock(pSymbol->sectionNumber, 0);
									}
								} else {
									return sl_null;
								}
							}
						}
					}
				} else {
					return sl_null;
				}
			}
			if (!flagFoundNewSection) {
				break;
			}
		}

		List<CoffCodeSection> ret;
		{
			for (auto& item : refSections) {
				CoffCodeSection* pSection = sections.getSectionByNumber(item.key);
				if (pSection) {
					ret.add_NoLock(*pSection);
				} else {
					return sl_null;
				}
			}
		}
		return ret;
	}

	sl_uint32 Coff::getCodeSectionSize(const CoffSectionDesc& section)
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

		List<CoffSymbol> list;
		for (sl_uint32 i = 0; i < header.numberOfSymbols; i++) {
			CoffSymbol symbol;
			CoffSymbolDesc& desc = symbol;
			if (!(reader->readFully(&desc, sizeof(CoffSymbolDesc)))) {
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
				if (!(seeker->seek(header.offsetToSymbolTable + (i + 1) * sizeof(CoffSymbolDesc), SeekPosition::Begin))) {
					return sl_false;
				}
			}
			list.add_NoLock(Move(symbol));
		}

		m_symbols = list;
		return sl_true;

	}


	CoffCodeSectionSet::CoffCodeSectionSet(const List<CoffCodeSection>& sections): ListElements(sections)
	{
		for (sl_uint32 i = 0; i < count; i++) {
			m_mapSectionIndex.put_NoLock(data[i].sectionIndex + 1, (sl_uint32)i);
		}
	}

	CoffCodeSectionSet::~CoffCodeSectionSet()
	{
	}

	CoffCodeSection* CoffCodeSectionSet::getSectionByNumber(sl_uint32 sectionNumber)
	{
		sl_uint32 index;
		if (m_mapSectionIndex.get_NoLock(sectionNumber, &index)) {
			return &(data[index]);
		}
		return sl_null;
	}

}