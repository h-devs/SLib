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

#ifndef CHECKHEADER_SLIB_DOC_RAR
#define CHECKHEADER_SLIB_DOC_RAR

#include "definition.h"

#include "../core/string.h"
#include "../core/skippable_reader.h"

namespace slib
{

	enum class RarBlockType4
	{
		Unknown = 0,
		Mark = 0x72,
		Main = 0x73,
		File = 0x74,
		Comment = 0x75,
		AV = 0x76,
		OldService = 0x77,
		Protect = 0x78,
		Sign = 0x79,
		Service = 0x7a,
		End = 0x7b,
	};

	enum class RarBlockType5
	{
		Unknown = 0,
		Main = 1,
		File = 2,
		Service = 3,
		Encryption = 4,
		End = 5,

		Min = 1,
		Max = 5
	};

	class SLIB_EXPORT RarBlockFlags4
	{
	public:
		int value;
		SLIB_MEMBERS_OF_FLAGS(RarBlockFlags4, value)

		enum {
			Main_Volume = 0x0001,
			Main_Comment = 0x0002,
			Main_Lock = 0x0004,
			Main_Solid = 0x0008,
			Main_PackComment = 0x0010,
			Main_NewNumbering = 0x0010,
			Main_AV = 0x0020,
			Main_Protect = 0x0040,
			Main_Password = 0x0080,
			Main_FirstVolume = 0x0100,

			File_SplitBefore = 0x0001,
			File_SplitAfter = 0x0002,
			File_Password = 0x0004,
			File_Comment = 0x0008,
			File_Solid = 0x0010,

			File_Window_Mask = 0x00e0,
			File_Window_Directory = 0x00e0,
			File_Window_64 = 0x0000,
			File_Window_128 = 0x0020,
			File_Window_256 = 0x0040,
			File_Window_512 = 0x0060,
			File_Window_1024 = 0x0080,
			File_Window_2048 = 0x00a0,
			File_Window_4096 = 0x00c0,

			File_Large = 0x0100,
			File_Unicode = 0x0200,
			File_Salt = 0x0400,
			File_Version = 0x0800,
			File_ExtTime = 0x1000,

			SkipIfUnknown = 0x4000,
			LongBlock = 0x8000
		};
	};

	class SLIB_EXPORT RarBlockFlags5
	{
	public:
		int value;
		SLIB_MEMBERS_OF_FLAGS(RarBlockFlags5, value)

		enum {
			ExtraArea = 0x0001, // Extra area is present in the end of header
			DataArea = 0x0002, // Data area is present in the end of header
			SkipIfUnknown = 0x0004, // Blocks with unknown type and this flag must be skipped when updating an archive
			DataPreviousVolume = 0x0008, // Data area is continuing from previous volume
			DataNextVolume = 0x0010, // Data area is continuing in next volume
			DependsOnPrecedingBlock = 0x0020, // Block depends on preceding file block
			PreserveChildBlock = 0x0040, // Preserve a child block if host block is modified
		};
	};

	class SLIB_EXPORT RarArchiveFlags5
	{
	public:
		int value;
		SLIB_MEMBERS_OF_FLAGS(RarArchiveFlags5, value)

		enum {
			Volume = 0x0001, // Archive is a part of multivolume set.
			VolumeNumber = 0x0002, // Volume number field is present. This flag is present in all volumes except first.
			Solid = 0x0004, // Solid archive.
			RecoveryRecord = 0x0008, // Recovery record is present.
			Lock = 0x0010, // Locked archive.
		};
	};

	class SLIB_EXPORT RarEncryptionFlags5
	{
	public:
		int value;
		SLIB_MEMBERS_OF_FLAGS(RarEncryptionFlags5, value)

		enum {
			PasswordCheck = 0x0001, // Password check data is present
			TweakedChecksums = 0x0002 // Use tweaked checksums instead of plain checksums. Only used in file encryption record. If flag 0x0002 is present, RAR transforms the checksum preserving file or service data integrity, so it becomes dependent on encryption key. It makes guessing file contents based on checksum impossible. It affects both data CRC32 in file header and checksums in file hash record in extra area.
		};
	};

	class SLIB_EXPORT RarFileFlags5
	{
	public:
		int value;
		SLIB_MEMBERS_OF_FLAGS(RarFileFlags5, value)

		enum {
			Directory = 0x0001, // Directory file system object (file header only)
			Time = 0x0002, // Time field in Unix format is present
			CRC32 = 0x0004, // CRC32 field is present
			UnknownUnpackedSize = 0x0008 // Unpacked size is unknown
		};
	};

	enum class RarHostOS5
	{
		Windows = 0,
		Unix = 1
	};

	enum class RarHostOS4
	{
		MSDOS = 0,
		OS2 = 1,
		Win32 = 2,
		Unix = 3,
		macOS = 4,
		BEOS = 5
	};

	enum class RarExtraType5
	{
		Unknown = 0,
		FileEncryption = 1,
		FileHash = 2,
		FileTime = 3,
		FileVersion = 4,
		Redirection = 5,
		UnixOwner = 6,
		ServiceData = 7
	};

	class SLIB_EXPORT RarBlockBaseHeader
	{
	public:
		Memory rawHeader;

	public:
		RarBlockBaseHeader();

		~RarBlockBaseHeader();

	public:
		sl_bool readHeaderContent(IReader* reader, sl_size size, Memory* pMemory = sl_null);

	};

	class SLIB_EXPORT RarBlockHeader4 : public RarBlockBaseHeader
	{
	public:
		sl_uint16 headerCRC;
		RarBlockType4 type;
		RarBlockFlags4 flags;
		sl_uint16 headerSize;

	public:
		RarBlockHeader4();

		~RarBlockHeader4();

	public:
		sl_bool read(IReader* reader, Memory* pMemory = sl_null);

	};

	class SLIB_EXPORT RarBlockHeader5 : public RarBlockBaseHeader
	{
	public:
		sl_uint32 headerCRC; // CRC32 of header data starting from Header size field and up to and including the optional extra area.
		sl_uint32 headerSize; // Size of header data starting from Header type field and up to and including the optional extra area. This field must not be longer than 3 bytes in current implementation, resulting in 2 MB maximum header size.

		RarBlockType5 type;
		RarBlockFlags5 flags;
		sl_uint64 extraAreaSize; // Size of extra area. Optional field, present only if 0x0001 header flag is set.
		sl_uint64 dataSize; // Size of data area. Optional field, present only if 0x0002 header flag is set.

		sl_size customHeaderPosition;

	public:
		RarBlockHeader5();

		~RarBlockHeader5();

	public:
		sl_bool read(IReader* reader, Memory* pMemory = sl_null);

	};

	class SLIB_EXPORT RarExtraArea5
	{
	public:
		sl_uint32 size; // Size of record data starting from type.
		RarExtraType5 type;
		sl_size dataPosition;
		sl_size dataSize;

	public:
		RarExtraArea5();

		~RarExtraArea5();

	public:
		sl_bool read(MemoryReader& reader);

	};

	class SLIB_EXPORT RarMainBlock4
	{
	public:
		sl_uint64 posAV;

	public:
		RarMainBlock4();

		~RarMainBlock4();

	public:
		sl_bool readHeader(const RarBlockHeader4& header);

	};

	class SLIB_EXPORT RarMainBlock5
	{
	public:
		RarArchiveFlags5 flags;
		sl_uint64 volumeNumber;
		
	public:
		RarMainBlock5();

		~RarMainBlock5();

	public:
		sl_bool readHeader(const RarBlockHeader5& header);

	};

	class SLIB_EXPORT RarEncryptionBlock5
	{
	public:
		sl_uint32 version; // Version of encryption algorithm. Now only 0 version (AES-256) is supported.
		RarEncryptionFlags5 flags;
		sl_uint8 countKDF; // Binary logarithm of iteration number for PBKDF2 function. RAR can refuse to process KDF count exceeding some threshold. Concrete value of threshold is a version dependent.
		sl_uint8 salt[16]; // Salt value used globally for all encrypted archive headers.
		sl_uint8 checkValue[12]; // Value used to verify the password validity. Present only if 0x0001 encryption flag is set. First 8 bytes are calculated using additional PBKDF2 rounds, 4 last bytes is the additional checksum. Together with the standard header CRC32 we have 64 bit checksum to reliably verify this field integrity and distinguish invalid password and damaged data.

	public:
		RarEncryptionBlock5();

		~RarEncryptionBlock5();

	public:
		sl_bool readHeader(const RarBlockHeader5& header);

	};

	class SLIB_EXPORT RarCompressionInformation5
	{
	public:
		sl_uint8 version;
		sl_bool flagSolid; // If it is set, RAR continues to use the compression dictionary left after processing preceding files. It can be set only for file headers and is never set for service headers.
		sl_uint8 method; // Currently only values 0 - 5 are used. 0 means no compression.
		sl_uint8 dictionarySize; // minimum size of dictionary required to extract data. Value 0 means 128 KB, 1 - 256 KB, ..., 14 - 2048 MB, 15 - 4096 MB.

	public:
		RarCompressionInformation5();

		~RarCompressionInformation5();

	public:
		void setValue(sl_uint32 value);

	};

	// File header and service header
	class SLIB_EXPORT RarFileBlock4
	{
	public:
		sl_uint64 dataSize; // Packed size
		sl_uint64 fileSize; // Unpacked file or service data size
		sl_bool flagUnknownFileSize;
		sl_bool flagDirectory;
		RarHostOS4 hostOS;
		sl_uint32 fileCRC;
		sl_uint32 modifiedTime;
		sl_uint8 compressionVersion;
		sl_uint8 compressionMethod;
		sl_uint32 attributes;
		String name;
		sl_uint8 salt[8];

	public:
		RarFileBlock4();

		~RarFileBlock4();

	public:
		sl_bool readHeader(MemoryReader& reader, const RarBlockHeader4& header, sl_bool flagReadName = sl_true);

		sl_bool readHeader(const RarBlockHeader4& header, sl_bool flagReadName = sl_true);

		sl_bool isDirectory();

	};

	// File header and service header
	class SLIB_EXPORT RarFileBlock5
	{
	public:
		RarFileFlags5 flags;
		sl_uint64 fileSize; // Unpacked file or service data size.
		sl_uint64 attributes; // Operating system specific file attributes in case of file header. Might be either used for data specific needs or just reserved and set to 0 for service header.
		sl_uint32 modifiedTime; // File modification time in Unix time format. Optional, present if 0x0002 file flag is set.
		sl_uint32 fileCRC; // CRC32 of unpacked file or service data. For files split between volumes it contains CRC32 of file packed data contained in current volume for all file parts except the last. Optional, present if 0x0004 file flag is set.
		RarCompressionInformation5 compression;
		RarHostOS5 hostOS;
		String name;

	public:
		RarFileBlock5();

		~RarFileBlock5();

	public:
		sl_bool readHeader(MemoryReader& reader, sl_bool flagReadName = sl_true);

		sl_bool readHeader(const RarBlockHeader5& header, sl_bool flagReadName = sl_true);

		sl_bool isDirectory();

	};

	class SLIB_EXPORT RarFileEncryptionRecord5
	{
	public:
		sl_uint32 version; // Version of encryption algorithm. Now only 0 version (AES-256) is supported.
		RarEncryptionFlags5 flags;
		sl_uint8 countKDF; // Binary logarithm of iteration number for PBKDF2 function. RAR can refuse to process KDF count exceeding some threshold. Concrete value of threshold is version dependent.
		sl_uint8 salt[16]; // Salt value to set the decryption key for encrypted file
		sl_uint8 IV[16]; // AES-256 initialization vector
		sl_uint8 checkValue[12]; // Value used to verify the password validity. Present only if 0x0001 encryption flag is set. First 8 bytes are calculated using additional PBKDF2 rounds, 4 last bytes is the additional checksum. Together with the standard header CRC32 we have 64 bit checksum to reliably verify this field integrity and distinguish invalid password and damaged data.

	public:
		RarFileEncryptionRecord5();

		~RarFileEncryptionRecord5();

	public:
		sl_bool read(const void* data, sl_size size);

	};

	class SLIB_EXPORT RarFile
	{
	public:
		sl_bool flagRAR5;
		sl_bool flagEncryptedHeaders;

		RarMainBlock4 mainBlock4;

		RarEncryptionBlock5 encryption5;
		RarMainBlock5 mainBlock5;

	public:
		RarFile();

		~RarFile();
		
	public:
		void setReader(const Ptrx<IReader, ISeekable>& reader);

		sl_bool readSignature();

		// call after `readSignature()`
		sl_bool readMainHeader();

		sl_bool readFromSignatureToMainHeader();

		// call after `readMainHeader()`
		List<String> readFileNames();

		// call after `readMainHeader()`
		sl_bool isEncrypted(sl_int32 maxCheckFileCount);


		sl_bool readBlockHeader(RarBlockHeader4& header);

		sl_bool readBlockHeader(RarBlockHeader5& header);

		sl_bool skipData(const RarBlockHeader4& header);

		sl_bool skipData(const RarBlockHeader5& header);

		sl_bool readBlockHeaderAndSkipData(RarBlockHeader5& header);

	public:
		/*
			Return value:
				0 - Invalid file
				4 - RAR 4.x
				5 - RAR 5.0
		*/
		static sl_uint32 getFileVersion(const StringParam& path);

		static List<String> getFileNamesInFile(const StringParam& path);

		// maxCheckFileCount: negative value means no-limit
		static sl_bool isEncryptedFile(const StringParam& path, sl_int32 maxCheckFileCount = 1);
	
	private:
		SkippableReader m_reader;

		Memory m_bufferHeader;

	};

}

#endif
