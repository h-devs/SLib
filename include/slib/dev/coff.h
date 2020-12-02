#ifndef CHECKHEADER_SLIB_DEV_COFF
#define CHECKHEADER_SLIB_DEV_COFF

#include "definition.h"

#include "../core/string.h"
#include "../core/list.h"
#include "../core/map.h"
#include "../core/io.h"
#include "../core/ptrx.h"

#define	SLIB_COFF_MACHINE_I386		0x014c
#define	SLIB_COFF_MACHINE_AMD64		0x8664
#define	SLIB_COFF_MACHINE_IA64		0x0200
#define SLIB_COFF_MACHINE_ARM		0x01c0 // ARM Little-Endian

#define SLIB_COFF_CHARACTERISTICS_RELOCS_STRIPPED           0x0001  // Relocation info stripped from file.
#define SLIB_COFF_CHARACTERISTICS_EXECUTABLE_IMAGE          0x0002  // File is executable  (i.e. no unresolved externel references).
#define SLIB_COFF_CHARACTERISTICS_LINE_NUMS_STRIPPED        0x0004  // Line nunbers stripped from file.
#define SLIB_COFF_CHARACTERISTICS_LOCAL_SYMS_STRIPPED       0x0008  // Local symbols stripped from file.
#define SLIB_COFF_CHARACTERISTICS_AGGRESIVE_WS_TRIM         0x0010  // Agressively trim working set
#define SLIB_COFF_CHARACTERISTICS_LARGE_ADDRESS_AWARE       0x0020  // App can handle >2gb addresses
#define SLIB_COFF_CHARACTERISTICS_BYTES_REVERSED_LO         0x0080  // Bytes of machine word are reversed.
#define SLIB_COFF_CHARACTERISTICS_32BIT_MACHINE             0x0100  // 32 bit word machine.
#define SLIB_COFF_CHARACTERISTICS_DEBUG_STRIPPED            0x0200  // Debugging info stripped from file in .DBG file
#define SLIB_COFF_CHARACTERISTICS_REMOVABLE_RUN_FROM_SWAP   0x0400  // If Image is on removable media, copy and run from the swap file.
#define SLIB_COFF_CHARACTERISTICS_NET_RUN_FROM_SWAP         0x0800  // If Image is on Net, copy and run from the swap file.
#define SLIB_COFF_CHARACTERISTICS_SYSTEM                    0x1000  // System File.
#define SLIB_COFF_CHARACTERISTICS_DLL                       0x2000  // File is a DLL.
#define SLIB_COFF_CHARACTERISTICS_UP_SYSTEM_ONLY            0x4000  // File should only be run on a UP machine
#define SLIB_COFF_CHARACTERISTICS_BYTES_REVERSED_HI         0x8000  // Bytes of machine word are reversed.

#define SLIB_COFF_SECTION_CHARACTERISTICS_TYPE_NO_PAD                0x00000008  // Reserved.
#define SLIB_COFF_SECTION_CHARACTERISTICS_CNT_CODE                   0x00000020  // Section contains code.
#define SLIB_COFF_SECTION_CHARACTERISTICS_CNT_INITIALIZED_DATA       0x00000040  // Section contains initialized data.
#define SLIB_COFF_SECTION_CHARACTERISTICS_CNT_UNINITIALIZED_DATA     0x00000080  // Section contains uninitialized data.
#define SLIB_COFF_SECTION_CHARACTERISTICS_LNK_INFO                   0x00000200  // Section contains comments or some other type of information.
#define SLIB_COFF_SECTION_CHARACTERISTICS_LNK_REMOVE                 0x00000800  // Section contents will not become part of image.
#define SLIB_COFF_SECTION_CHARACTERISTICS_LNK_COMDAT                 0x00001000  // Section contents comdat.
#define SLIB_COFF_SECTION_CHARACTERISTICS_NO_DEFER_SPEC_EXC          0x00004000  // Reset speculative exceptions handling bits in the TLB entries for this section.						  RISTICS
#define SLIB_COFF_SECTION_CHARACTERISTICS_GPREL                      0x00008000  // Section content can be accessed relative to GP
#define SLIB_COFF_SECTION_CHARACTERISTICS_MEM_FARDATA                0x00008000
#define SLIB_COFF_SECTION_CHARACTERISTICS_MEM_PURGEABLE              0x00020000
#define SLIB_COFF_SECTION_CHARACTERISTICS_MEM_16BIT                  0x00020000
#define SLIB_COFF_SECTION_CHARACTERISTICS_MEM_LOCKED                 0x00040000
#define SLIB_COFF_SECTION_CHARACTERISTICS_MEM_PRELOAD                0x00080000

#define SLIB_COFF_SECTION_CHARACTERISTICS_ALIGN_1BYTES               0x00100000
#define SLIB_COFF_SECTION_CHARACTERISTICS_ALIGN_2BYTES               0x00200000
#define SLIB_COFF_SECTION_CHARACTERISTICS_ALIGN_4BYTES               0x00300000
#define SLIB_COFF_SECTION_CHARACTERISTICS_ALIGN_8BYTES               0x00400000
#define SLIB_COFF_SECTION_CHARACTERISTICS_ALIGN_16BYTES              0x00500000  // Default alignment if no others are specified.
#define SLIB_COFF_SECTION_CHARACTERISTICS_ALIGN_32BYTES              0x00600000
#define SLIB_COFF_SECTION_CHARACTERISTICS_ALIGN_64BYTES              0x00700000
#define SLIB_COFF_SECTION_CHARACTERISTICS_ALIGN_128BYTES             0x00800000
#define SLIB_COFF_SECTION_CHARACTERISTICS_ALIGN_256BYTES             0x00900000
#define SLIB_COFF_SECTION_CHARACTERISTICS_ALIGN_512BYTES             0x00A00000
#define SLIB_COFF_SECTION_CHARACTERISTICS_ALIGN_1024BYTES            0x00B00000
#define SLIB_COFF_SECTION_CHARACTERISTICS_ALIGN_2048BYTES            0x00C00000
#define SLIB_COFF_SECTION_CHARACTERISTICS_ALIGN_4096BYTES            0x00D00000
#define SLIB_COFF_SECTION_CHARACTERISTICS_ALIGN_8192BYTES            0x00E00000
#define SLIB_COFF_SECTION_CHARACTERISTICS_ALIGN_MASK                 0x00F00000

#define SLIB_COFF_SECTION_CHARACTERISTICS_LNK_NRELOC_OVFL            0x01000000  // Section contains extended relocations.
#define SLIB_COFF_SECTION_CHARACTERISTICS_MEM_DISCARDABLE            0x02000000  // Section can be discarded.
#define SLIB_COFF_SECTION_CHARACTERISTICS_MEM_NOT_CACHED             0x04000000  // Section is not cachable.
#define SLIB_COFF_SECTION_CHARACTERISTICS_MEM_NOT_PAGED              0x08000000  // Section is not pageable.
#define SLIB_COFF_SECTION_CHARACTERISTICS_MEM_SHARED                 0x10000000  // Section is shareable.
#define SLIB_COFF_SECTION_CHARACTERISTICS_MEM_EXECUTE                0x20000000  // Section is executable.
#define SLIB_COFF_SECTION_CHARACTERISTICS_MEM_READ                   0x40000000  // Section is readable.
#define SLIB_COFF_SECTION_CHARACTERISTICS_MEM_WRITE                  0x80000000  // Section is writeable.

namespace slib
{
	
	class SLIB_EXPORT CoffHeader
	{
	public:
		sl_uint16 machine; // SLIB_COFF_MACHINE_*
		sl_uint16 numberOfSections;
		sl_uint32 timeDateStamp; // represents the time the image was created by the linker - the number of seconds elapsed since 1970-01-01 00:00:00, UTC, according to the system clock.
		sl_uint32 offsetToSymbolTable; // The offset of the symbol table. 0 if no COFF symbol table exists.
		sl_uint32 numberOfSymbols;
		sl_uint16 sizeOfOptionalHeader; // The size of the optional header. This value should be 0 for object files.
		sl_uint16 characteristics; // SLIB_COFF_CHARACTERISTICS_*
	};
	
	class SLIB_EXPORT CoffSectionDesc
	{
	public:
		char name[8]; // An 8-byte, null-padded UTF-8 string. There is no terminating null character if the string is exactly eight characters long. For longer names, this member contains a forward slash (/) followed by an ASCII representation of a decimal number that is an offset into the string table. Executable images do not use a string table and do not support section names longer than eight characters.
		union {
			sl_uint32 physicalAddress; // The file address
			sl_uint32 virtualSize; // The total size of the section when loaded into memory. If this value is greater than the `sizeOfRawData` member, the section is filled with zeroes. This field is valid only for executable images and should be set to 0 for object files.
		};
		sl_uint32 virtualAddress; // The address of the first byte of the section when loaded into memory, relative to the image base. For object files, this is the address of the first byte before relocation is applied.
		sl_uint32 sizeOfRawData; // The size of the initialized data on disk. This value must be a multiple of the `fileAlignment` member of the `PE_OptionalHeader`. If this value is less than the `virtualSize` member, the remainder of the section is filled with zeroes. If the section contains only uninitialized data, the member is zero.
		sl_uint32 offsetToRawData; // A file pointer to the first page within the COFF file. This value must be a multiple of the `fileAlignment` member of the `PE_OptionalHeader`. If a section contains only uninitialized data, set this member is zero.
		sl_uint32 offsetToRelocations; // A file pointer to the beginning of the relocation entries for the section. If there are no relocations, this value is zero.
		sl_uint32 offsetToLinenumbers; // A file pointer to the beginning of the line-number entries for the section. If there are no COFF line numbers, this value is zero.
		sl_uint16 numberOfRelocations;
		sl_uint16 numberOfLinenumbers;
		sl_uint32 characteristics; // SLIB_COFF_SECTION_CHARACTERISTICS_*
	};

#pragma pack(push, 1)
	class SLIB_EXPORT CoffSectionRelocation
	{
	public:
		union
		{
			sl_uint32 virtualAddress;
			sl_uint32 relocCount; // Set to the real count when IMAGE_SCN_LNK_NRELOC_OVFL is set
		};
		sl_uint32 symbolTableIndex;
		sl_int16 type;
	};

	class SLIB_EXPORT CoffSymbolDesc
	{
	public:
		union
		{
			sl_char8 shortName[8];
			sl_uint32 longName[2];
		} name;
		sl_uint32 value;
		sl_uint16 sectionNumber;
		sl_uint16 type;
		sl_uint8 storageClass;
		sl_uint8 numberOfAuxSymbols;
	};
#pragma pack(pop)

	class SLIB_EXPORT CoffSection : public CoffSectionDesc
	{
	public:
		String name;

	public:
		CoffSection();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(CoffSection)

	};

	class SLIB_EXPORT CoffCodeSection : public CoffSection
	{
	public:
		sl_uint32 sectionIndex;
		sl_uint32 codeOffset;

	public:
		CoffCodeSection();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(CoffCodeSection)

	};

	class SLIB_EXPORT CoffSymbol : public CoffSymbolDesc
	{
	public:
		String name;

	public:
		CoffSymbol();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(CoffSymbol)

	};

	class SLIB_EXPORT Coff
	{
	public:
		sl_uint8* baseAddress;
		CoffHeader header;

		sl_uint32 offsetToSections;
		sl_uint32 offsetToSymbolNames;

	public:
		Coff();

		~Coff();

	public:
		virtual sl_bool load(const void* baseAddress, const Ptr<IReader, ISeekable>& reader);
		
		sl_bool load(const void* baseAddress, sl_size size = 0x7fffffff);

		sl_bool isLoaded();

		sl_bool getSection(sl_uint32 index, CoffSection& outSection);

		Memory getSectionData(const CoffSectionDesc& section);

		sl_bool getSectionRelocation(const CoffSectionDesc& section, sl_uint32 index, CoffSectionRelocation& outRelocation);

		CoffSymbol* getSymbol(sl_uint32 index);

		CoffSymbol* findSymbol(const StringParam& name);

		List<CoffCodeSection> getCodeSections();

		List<CoffCodeSection> getCodeSectionsReferencedFrom(const StringParam& entrySymbolName);

	protected:
		void _init(const void* baseAddress, const Ptr<IReader, ISeekable>& reader);

		sl_bool _loadSymbols();

	protected:
		IReader* m_reader;
		ISeekable* m_seekable;
		Ref<Referable> m_ref;

		List<CoffSymbol> m_symbols;

	};

	class CoffCodeSectionSet : public ListElements<CoffCodeSection>
	{
	public:
		CoffCodeSectionSet(const List<CoffCodeSection>& sections);

		~CoffCodeSectionSet();

	public:
		CoffCodeSection* getSectionByNumber(sl_uint32 sectionNumber);

	private:
		CMap<sl_uint32, sl_uint32> m_mapSectionIndex;

	};

}

#endif