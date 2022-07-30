#ifndef CHECKHEADER_SLIB_DEV_PE
#define CHECKHEADER_SLIB_DEV_PE

#include "coff.h"

#define SLIB_PE_OPTIONAL_MAGIC_EXE32		0x10b // 32-bit executable image
#define SLIB_PE_OPTIONAL_MAGIC_EXE64		0x20b // 64-bit executable image
#define SLIB_PE_OPTIONAL_MAGIC_ROM			0x107 // ROM image

#define SLIB_PE_SUBSYSTEM_UNKNOWN					0   // Unknown subsystem.
#define SLIB_PE_SUBSYSTEM_NATIVE					1   // Image doesn't require a subsystem.
#define SLIB_PE_SUBSYSTEM_WINDOWS_GUI				2   // Image runs in the Windows GUI subsystem.
#define SLIB_PE_SUBSYSTEM_WINDOWS_CUI				3   // Image runs in the Windows character subsystem.
#define SLIB_PE_SUBSYSTEM_OS2_CUI					5   // image runs in the OS/2 character subsystem.
#define SLIB_PE_SUBSYSTEM_POSIX_CUI					7   // image runs in the Posix character subsystem.
#define SLIB_PE_SUBSYSTEM_NATIVE_WINDOWS			8   // image is a native Win9x driver.
#define SLIB_PE_SUBSYSTEM_WINDOWS_CE_GUI			9   // Image runs in the Windows CE subsystem.
#define SLIB_PE_SUBSYSTEM_EFI_APPLICATION			10
#define SLIB_PE_SUBSYSTEM_EFI_BOOT_SERVICE_DRIVER	11
#define SLIB_PE_SUBSYSTEM_EFI_RUNTIME_DRIVER		12
#define SLIB_PE_SUBSYSTEM_EFI_ROM					13
#define SLIB_PE_SUBSYSTEM_XBOX						14
#define SLIB_PE_SUBSYSTEM_WINDOWS_BOOT_APPLICATION	16

#define SLIB_PE_DLL_CHARACTERISTICS_DYNAMIC_BASE			0x0040  // DLL can move.
#define SLIB_PE_DLL_CHARACTERISTICS_FORCE_INTEGRITY			0x0080  // Code Integrity Image
#define SLIB_PE_DLL_CHARACTERISTICS_NX_COMPAT				0x0100  // Image is NX compatible
#define SLIB_PE_DLL_CHARACTERISTICS_NO_ISOLATION			0x0200  // Image understands isolation and doesn't want it
#define SLIB_PE_DLL_CHARACTERISTICS_NO_SEH					0x0400  // Image does not use SEH.  No SE handler may reside in this image
#define SLIB_PE_DLL_CHARACTERISTICS_NO_BIND					0x0800  // Do not bind this image.
#define SLIB_PE_DLL_CHARACTERISTICS_WDM_DRIVER				0x2000  // Driver uses WDM model
#define SLIB_PE_DLL_CHARACTERISTICS_TERMINAL_SERVER_AWARE	0x8000

#define SLIB_PE_NUMBER_OF_DIRECTORY_ENTRIES			16
#define SLIB_PE_DIRECTORY_EXPORT_TABLE				0
#define SLIB_PE_DIRECTORY_IMPORT_TABLE				1
#define SLIB_PE_DIRECTORY_RESOURCE_TABLE			2
#define SLIB_PE_DIRECTORY_EXCEPTION_TABLE			3
#define SLIB_PE_DIRECTORY_CERTIFICATE_TABLE			4
#define SLIB_PE_DIRECTORY_RELOCATION_TABLE			5
#define SLIB_PE_DIRECTORY_DEBUGGING_INFORMATION		6
#define SLIB_PE_DIRECTORY_ARCHITECTURE_SPECIFIC		7
#define SLIB_PE_DIRECTORY_GLOBAL_POINTER_REGISTER	8
#define SLIB_PE_DIRECTORY_THREAD_LOCAL_STORAGE		9
#define SLIB_PE_DIRECTORY_LOAD_CONFIGURATION		10
#define SLIB_PE_DIRECTORY_BOUND_IMPORT_TABLE		11
#define SLIB_PE_DIRECTORY_IMPORT_ADDRESS_TABLE		12
#define SLIB_PE_DIRECTORY_DELAY_IMPORT_DESCRIPTOR	13
#define SLIB_PE_DIRECTORY_CLR						14
// Reserved											15

#define SLIB_PE_RELOC_I386_REL32					0x0014
#define SLIB_PE_REL_AMD64_REL32								0x0004

namespace slib
{

	/*
			
		Structure of PE
		
		PE_DosHeader
		MS-DOS Stub Program
		PE_Signature
		PE_Header
		PE_OptionalHeader32 | PE_OptionalHeader64
		CoffSectionHeader...
		Sections

	*/
	
	class SLIB_EXPORT PE_DosHeader
	{
	public:
		char signature[2]; // "MZ"
		sl_uint16 sizeLastPage; // Bytes on last page of file
		sl_uint16 pageCount; // Pages in file
		sl_uint16 relocations; // Relocations
		sl_uint16 sizeHeaderInParagraphs; // Size of header in paragraphs
		sl_uint16 minAlloc; // Minimum extra paragraphs needed
		sl_uint16 maxAlloc; // Maximum extra paragraphs needed
		sl_uint16 ss; // Initial (relative) SS value
		sl_uint16 sp; // Initial SP value
		sl_uint16 checksum; // Checksum
		sl_uint16 ip; // Initial IP value
		sl_uint16 cs; // Initial (relative) CS value
		sl_uint16 relocationTable; // File address of relocation table
		sl_uint16 overlayNumber; // Overlay number
		sl_uint16 reserved[4]; // Reserved words
		sl_uint16 oemId; // OEM identifier
		sl_uint16 oemInfo; // OEM information
		sl_uint16 reserved2[10]; // Reserved words
		sl_uint32 newHeader; // File address of new exe header

	public:
		sl_bool checkSignature() const;

	};
	
	class SLIB_EXPORT PE_Signature
	{
	public:
		char signature[4]; // "PE\0\0"

	public:
		sl_bool check() const;

	};

	class SLIB_EXPORT PE_DirectoryEntry
	{
	public:
		sl_uint32 address; // relative virtual address of the table.
		sl_uint32 size;
	};
	
	class SLIB_EXPORT PE_OptionalHeader32
	{
	public:
		sl_uint16 magic; // SLIB_PE_OPTIONAL_MAGIC_*
		sl_uint8 majorLinkerVersion;
		sl_uint8 minorLinkerVersion;
		sl_uint32 sizeOfCode; // sum of all code sections
		sl_uint32 sizeOfInitializedData; // sum of all initialized data sections
		sl_uint32 sizeOfUninitializedData; // sum of all uninitialized data sections
		sl_uint32 addressOfEntryPoint; // entry point function, relative to the image base address. 0 when no entry point is present
		sl_uint32 baseOfCode; // A pointer to the beginning of the code section, relative to the image base
		sl_uint32 baseOfData; // A pointer to the beginning of the data section, relative to the image base.
		sl_uint32 imageBase; // The preferred address of the first byte of the image when it is loaded in memory. This value is a multiple of 64K bytes. The default value for DLLs is 0x10000000. The default value for applications is 0x00400000, except on Windows CE where it is 0x00010000.
		sl_uint32 sectionAlignment; // The alignment of sections loaded in memory. must be greater than or equal to the `fileAlignment` member. The default value is the page size for the system.
		sl_uint32 fileAlignment; // The alignment of the raw data of sections in the image file. The value should be a power of 2 between 512 and 64K (inclusive). The default is 512. If the `sectionAlignment` member is less than the system page size, this member must be the same as `sectionAlignment`.
		sl_uint16 majorOperatingSystemVersion; // The major version number of the required operating system.
		sl_uint16 minorOperatingSystemVersion; // The minor version number of the required operating system.
		sl_uint16 majorImageVersion; // The major version number of the image.
		sl_uint16 minorImageVersion; // The minor version number of the image.
		sl_uint16 majorSubsystemVersion; // The major version number of the subsystem.
		sl_uint16 minorSubsystemVersion; // The minor version number of the subsystem.
		sl_uint32 win32VersionValue; // Reserved, 0
		sl_uint32 sizeOfImage; // The size of the image, including all headers. Must be a multiple of `sectionAlignment`.
		sl_uint32 sizeOfHeaders; // The combined size of the MS-DOS stub, the PE header, and the section headers, rounded to a multiple of the value specified in the FileAlignment member.
		sl_uint32 checkSum; // Image file checksum
		sl_uint16 subsystem; // SLIB_PE_SUBSYSTEM_*
		sl_uint16 dllCharacteristics; // SLIB_PE_DLL_CHARACTERISTICS_*
		sl_uint32 sizeOfStackReserve; // The number of bytes to reserve for the stack. Only the memory specified by the `sizeOfStackCommit` member is committed at load time; the rest is made available one page at a time until this reserve size is reached.
		sl_uint32 sizeOfStackCommit; // The number of bytes to commit for the stack.
		sl_uint32 sizeOfHeapReserve; // The number of bytes to reserve for the local heap. Only the memory specified by the `sizeOfHeapCommit` member is committed at load time; the rest is made available one page at a time until this reserve size is reached.
		sl_uint32 sizeOfHeapCommit; // The number of bytes to commit for the local heap.
		sl_uint32 loaderFlags; // This member is obsolete.
		sl_uint32 numberOfRvaAndSizes; // The number of directory entries in the remainder of the optional header. Each entry describes a location and size.
		PE_DirectoryEntry directoryEntry[SLIB_PE_NUMBER_OF_DIRECTORY_ENTRIES];
	};

	class SLIB_EXPORT PE_OptionalHeader64
	{
	public:
		sl_uint16 magic; // SLIB_PE_OPTIONAL_MAGIC_*
		sl_uint8 majorLinkerVersion;
		sl_uint8 minorLinkerVersion;
		sl_uint32 sizeOfCode; // sum of all code sections
		sl_uint32 sizeOfInitializedData; // sum of all initialized data sections
		sl_uint32 sizeOfUninitializedData; // sum of all uninitialized data sections
		sl_uint32 addressOfEntryPoint; // entry point function, relative to the image base address. 0 when no entry point is present
		sl_uint32 baseOfCode; // A pointer to the beginning of the code section, relative to the image base
		sl_uint64 imageBase;  // The preferred address of the first byte of the image when it is loaded in memory. This value is a multiple of 64K bytes. The default value for DLLs is 0x10000000. The default value for applications is 0x00400000, except on Windows CE where it is 0x00010000.
		sl_uint32 sectionAlignment; // The alignment of sections loaded in memory. must be greater than or equal to the `fileAlignment` member. The default value is the page size for the system.
		sl_uint32 fileAlignment; // The alignment of the raw data of sections in the image file. The value should be a power of 2 between 512 and 64K (inclusive). The default is 512. If the `sectionAlignment` member is less than the system page size, this member must be the same as `sectionAlignment`.
		sl_uint16 majorOperatingSystemVersion; // The major version number of the required operating system.
		sl_uint16 minorOperatingSystemVersion; // The minor version number of the required operating system.
		sl_uint16 majorImageVersion; // The major version number of the image.
		sl_uint16 minorImageVersion; // The minor version number of the image.
		sl_uint16 majorSubsystemVersion; // The major version number of the subsystem.
		sl_uint16 minorSubsystemVersion; // The minor version number of the subsystem.
		sl_uint32 win32VersionValue; // Reserved, 0
		sl_uint32 sizeOfImage; // The size of the image, including all headers. Must be a multiple of `sectionAlignment`.
		sl_uint32 sizeOfHeaders; // The combined size of the MS-DOS stub, the PE header, and the section headers, rounded to a multiple of the value specified in the FileAlignment member.
		sl_uint32 checkSum; // Image file checksum
		sl_uint16 subsystem; // SLIB_PE_SUBSYSTEM_*
		sl_uint16 dllCharacteristics; // SLIB_PE_DLL_CHARACTERISTICS_*
		sl_uint64 sizeOfStackReserve; // The number of bytes to reserve for the stack. Only the memory specified by the `sizeOfStackCommit` member is committed at load time; the rest is made available one page at a time until this reserve size is reached.
		sl_uint64 sizeOfStackCommit; // The number of bytes to commit for the stack.
		sl_uint64 sizeOfHeapReserve; // The number of bytes to reserve for the local heap. Only the memory specified by the `sizeOfHeapCommit` member is committed at load time; the rest is made available one page at a time until this reserve size is reached.
		sl_uint64 sizeOfHeapCommit; // The number of bytes to commit for the local heap.
		sl_uint32 loaderFlags; // This member is obsolete.
		sl_uint32 numberOfRvaAndSizes; // The number of directory entries in the remainder of the optional header. Each entry describes a location and size.
		PE_DirectoryEntry directoryEntry[SLIB_PE_NUMBER_OF_DIRECTORY_ENTRIES];
	};

	class SLIB_EXPORT PE_ImportDescriptor
	{
	public:
		sl_uint32 originalFirstThunk; // relative virtual address to original unbound Import-Address-Table (Import Name Table)
		sl_uint32 timeDateStamp; // 0 if not bound, -1 if bound, and real date\time stamp in PE_BoundImport (new BIND), otherwise date/time stamp of DLL bound to (Old BIND)
		sl_uint32 forwarderChain; // -1 if no forwarders
		sl_uint32 name; // relative virtual address to dll name
		sl_uint32 firstThunk; // relative virtual address to Import-Address-Table (if bound this IAT has actual addresses)
	};

	class SLIB_EXPORT PE_ExportDirectory
	{
	public:
		sl_uint32 characteristics;
		sl_uint32 timeDateStamp;
		sl_uint16 majorVersion;
		sl_uint16 minorVersion;
		sl_uint32 name;
		sl_uint32 base;
		sl_uint32 numberOfFunctions;
		sl_uint32 numberOfNames;
		sl_uint32 addressOfFunctions; // RVA from base of image
		sl_uint32 addressOfNames; // RVA from base of image
		sl_uint32 addressOfNameOrdinals; // RVA from base of image
	};

	class SLIB_EXPORT PE : public Coff
	{
	public:
		PE_DosHeader dos;
		union {
			PE_OptionalHeader32 optional32;
			PE_OptionalHeader64 optional64;
		};

		sl_bool flag64Bit;
		sl_uint64 imageBase;

	public:
		PE();

		~PE();

	public:
		using Coff::load;
		sl_bool load(const void* baseAddress, const Ptr<IReader, ISeekable>& reader) override;

		PE_DirectoryEntry* getImportTableDirectory();

		PE_DirectoryEntry* getExportTableDirectory();

		PE_ImportDescriptor* findImportTable(const StringParam& dllName);
		
		void* findExportFunction(const StringParam& functionName);

	};

}

#endif
