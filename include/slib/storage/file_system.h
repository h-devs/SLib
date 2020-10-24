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

#ifndef CHECKHEADER_SLIB_STORAGE_FILE_SYSTEM
#define CHECKHEADER_SLIB_STORAGE_FILE_SYSTEM

#include "definition.h"

#include "../core/object.h"
#include "../core/string.h"
#include "../core/time.h"
#include "../core/hash_map.h"

namespace slib
{

	class FileSystemInformation
	{
	public:
		String volumeName;
		String fileSystemName;
		Time creationTime;
		sl_uint32 serialNumber;
		sl_uint16 sectorSize;
		sl_uint16 sectorsPerAllocationUnit;
		sl_uint32 maxComponentLength;
		union {
			sl_uint32 fileSystemFlags;
			struct FileSystemFlags {
				sl_bool isCaseSensitiveSearch : 1;            // 0x00000001
				sl_bool isCasePreservedNames : 1;             // 0x00000002  
				sl_bool supportsUnicodeOnDisk : 1;            // 0x00000004  
				sl_bool preservePersistentACLs : 1;           // 0x00000008  
				sl_bool supportsFileCompression : 1;          // 0x00000010  
				sl_bool supportsVolumeQuotas : 1;             // 0x00000020  
				sl_bool supportsSparseFiles : 1;              // 0x00000040  
				sl_bool supportsReparsePoints : 1;            // 0x00000080  
				sl_bool supportsRemoteStorage : 1;            // 0x00000100  
				sl_bool returnsCleanupResultInfo : 1;         // 0x00000200  
				sl_bool supportsPosixUnlinkRename : 1;        // 0x00000400  
				sl_bool reserved0 : 1;                        // 0x00000800
				sl_bool reserved1 : 1;                        // 0x00001000
				sl_bool reserved2 : 1;                        // 0x00002000
				sl_bool reserved3 : 1;                        // 0x00004000
				sl_bool isVolumeCompressed : 1;               // 0x00008000  
				sl_bool supportsObjectIds : 1;                // 0x00010000  
				sl_bool supportsEncryption : 1;               // 0x00020000  
				sl_bool supportsNamedStreams : 1;             // 0x00040000  
				sl_bool isReadOnlyVolume : 1;                 // 0x00080000  
				sl_bool supportsSequentialWriteOnce : 1;      // 0x00100000  
				sl_bool supportsTransactions : 1;             // 0x00200000  
				sl_bool supportsHardLinks : 1;                // 0x00400000  
				sl_bool supportsExtendedAttributes : 1;       // 0x00800000  
				sl_bool supportsOpenByFileId : 1;             // 0x01000000  
				sl_bool supportsUsnJournal : 1;               // 0x02000000  
				sl_bool supportsIntegrityStreams : 1;         // 0x04000000  
				sl_bool supportsBlockRefCounting : 1;         // 0x08000000  
				sl_bool supportsSparseVDL : 1;                // 0x10000000  
				sl_bool isDAXVolume : 1;                      // 0x20000000  
				sl_bool supportsGhosting : 1;                 // 0x40000000  
			} flags;
		};

		sl_uint64 totalSize;	// must be re-calculated in fsGetVolumeInfo(SizeInfo)
		sl_uint64 freeSize;		// must be re-calculated in fsGetVolumeInfo(SizeInfo)

	public:
		FileSystemInformation() {
			this->creationTime = 0;
			this->serialNumber = 0;
			this->sectorSize = 4096;
			this->sectorsPerAllocationUnit = 1;
			this->maxComponentLength = 256;
		}
		SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(FileSystemInformation)
	};

	struct FileAttrs {
		sl_bool isReadOnly : 1;             // 0x00000001  
		sl_bool isHidden : 1;               // 0x00000002  
		sl_bool isSysten : 1;               // 0x00000004  
		sl_bool reserved0 : 1;              // 0x00000008  
		sl_bool isDirectory : 1;            // 0x00000010  
		sl_bool isArchive : 1;              // 0x00000020  
		sl_bool isDevice : 1;               // 0x00000040  
		sl_bool isNormal : 1;               // 0x00000080  
		sl_bool isTemporary : 1;            // 0x00000100  
		sl_bool isSparseFile : 1;           // 0x00000200  
		sl_bool isReparsePoint : 1;         // 0x00000400  
		sl_bool isCompressed : 1;           // 0x00000800  
		//sl_bool isOffline : 1;              // 0x00001000  
		//sl_bool isNotContentIndexed : 1;    // 0x00002000  
		//sl_bool isEncrypted : 1;            // 0x00004000  
		//sl_bool isIntegrityStream : 1;      // 0x00008000  
		//sl_bool isVirtual : 1;              // 0x00010000  
		//sl_bool isNoScrubData : 1;          // 0x00020000  
		//sl_bool isEA : 1;                   // 0x00040000  
	};

	class FileInfo
	{
	public:
		union {
			sl_uint32 fileAttributes;
			FileAttrs attr;
		};
		sl_uint64 size;
		sl_uint64 allocationSize;
		Time createdAt;
		Time modifiedAt;
		Time lastAccessedAt;

	public:
		FileInfo() { Base::zeroMemory(this, sizeof(*this)); }
		SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(FileInfo)
	};

	class StreamInfo
	{
	public:
		sl_uint64 size;

	public:
		StreamInfo() { Base::zeroMemory(this, sizeof(*this)); }
		SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(StreamInfo)
	};

	class VolumeInfoFlags {
	public:
		int value;
		SLIB_MEMBERS_OF_FLAGS(VolumeInfoFlags, value)

		enum {
			BasicInfo,
			SizeInfo,
		};
	};

	class FileInfoFlags {
	public:
		int value;
		SLIB_MEMBERS_OF_FLAGS(FileInfoFlags, value)

		enum {
			AttrInfo = 0x1,
			TimeInfo = 0x2,
			AttrAndTimeInfo = AttrInfo | TimeInfo,
			SizeInfo = 0x4,
			AllocSizeInfo = 0x8,
		};
	};

	class FileCreationParams
	{
	public:
		sl_uint32 accessMode;
		union {
			sl_uint32 shareMode;
			struct {
				sl_bool r : 1;	// SHARE_READ
				sl_bool w : 1;	// SHARE_WRITE
				sl_bool d : 1;	// SHARE_DELETE
			} share;
		};
		union {
			sl_uint32 flagsAndAttributes;
			FileAttrs attr;
		};
		/* fsCreate/fsOpen must erase this flag, and set context.status to FileExist if file already exists */
		sl_bool createAlways : 1;	// fsCreate: CREATE_ALWAYS, fsOpen: openTruncate ? CREATE_ALWAYS : OPEN_ALWAYS
		/* fsOpen must erase this flag if file truncated */
		sl_bool openTruncate : 1;	// fsOpen: createAlways ? CREATE_ALWAYS : TRUNCATE_EXISTING
		sl_ptr securityDescriptor;

	public:
		FileCreationParams() { Base::zeroMemory(this, sizeof(*this)); }
		SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(FileCreationParams)
	};

	enum class FileSystemError : sl_uint32
	{
		Success = 0U,			/*ERROR_SUCCESS*/
		GeneralError = 1U,		/*ERROR_INVALID_FUNCTION*/
		NotFound = 2U,			/*ERROR_FILE_NOT_FOUND*/
		AccessDenied = 5U,		/*ERROR_ACCESS_DENIED*/
		InvalidContext = 6U,	/*ERROR_INVALID_HANDLE*/
		InvalidData = 13U,		/*ERROR_INVALID_DATA*/
		FileExist = 80U,		/*ERROR_FILE_EXISTS*/
		InvalidPassword = 86U,	/*ERROR_INVALID_PASSWORD*/
		BufferOverflow = 122U,	/*ERROR_INSUFFICIENT_BUFFER*/
		AlreadyExist = 183U,	/*ERROR_ALREADY_EXISTS*/
		InitFailure = 575U,		/*ERROR_APP_INIT_FAILURE*/
		NotImplemented = 0xFFFFFFFFU,
	};

	class FileContext : public Referable
	{
	public:
		String path;
		sl_bool isDirectory : 1;
		sl_uint64 handle;
		FileSystemError status;

	public:
		FileContext(String path, sl_bool isDir = sl_false)
			: path(path), isDirectory(isDir), handle(0), status(FileSystemError::Success) {}
	};

	class FileSystemProvider : public Object
	{
		SLIB_DECLARE_OBJECT

	public:
		FileSystemProvider();

		virtual ~FileSystemProvider()
		{
		}

	public:
		/* FileSystem Interfaces */

		virtual const FileSystemInformation&
			fsGetVolumeInfo(VolumeInfoFlags flags = VolumeInfoFlags::BasicInfo)&
		{
			return m_volumeInfo;
		}
		
		virtual void
			fsSetVolumeName(String volumeName)
		{
			throw FileSystemError::NotImplemented;
		}

		virtual void
			fsCreate(FileContext* context, FileCreationParams& params = FileCreationParams())
		{
			throw FileSystemError::NotImplemented;
		}

		virtual void
			fsOpen(FileContext* context, FileCreationParams& params = FileCreationParams())
		{
			throw FileSystemError::NotImplemented;
		}

		virtual sl_size
			fsRead(FileContext* context, const Memory& buffer, sl_uint64 offset)
		{
			throw FileSystemError::NotImplemented;
		}

		virtual sl_size
			fsWrite(FileContext* context, const Memory& buffer, sl_uint64 offset, sl_bool writeToEof)
		{
			throw FileSystemError::NotImplemented;
		}

		virtual void 
			fsFlush(FileContext* context)
		{
			throw FileSystemError::NotImplemented;
		}

		virtual void
			fsClose(FileContext* context)
		{
			throw FileSystemError::NotImplemented;
		}

		virtual void
			fsDelete(FileContext* context, sl_bool checkOnly)
		{
			throw FileSystemError::NotImplemented;
		}

		virtual void
			fsRename(FileContext* context, String newFileName, sl_bool replaceIfExists)
		{
			throw FileSystemError::NotImplemented;
		}

		virtual void 
			fsLock(FileContext* context, sl_uint64 offset, sl_uint64 length)
		{
			throw FileSystemError::NotImplemented;
		}

		virtual void
			fsUnlock(FileContext* context, sl_uint64 offset, sl_uint64 length)
		{
			throw FileSystemError::NotImplemented;
		}

		virtual FileInfo
			fsGetFileInfo(FileContext* context)
		{
			throw FileSystemError::NotImplemented;
		}

		virtual void
			fsSetFileInfo(FileContext* context, FileInfo fileInfo, FileInfoFlags flags)
		{
			throw FileSystemError::NotImplemented;
		}

		virtual sl_size
			fsGetSecurity(FileContext* context, sl_uint32 securityInformation, const Memory& securityDescriptor)
		{
			throw FileSystemError::NotImplemented;
		}

		virtual void
			fsSetSecurity(FileContext* context, sl_uint32 securityInformation, const Memory& securityDescriptor)
		{
			throw FileSystemError::NotImplemented;
		}

		virtual HashMap<String, FileInfo>
			fsFindFiles(FileContext* context, String pattern)
		{
			throw FileSystemError::NotImplemented;
		}

		virtual HashMap<String, StreamInfo>
			fsFindStreams(FileContext* context)
		{
			throw FileSystemError::NotImplemented;
		}

	public:
		/* Helpers */
		virtual sl_bool exists(String fileName) noexcept;

		virtual Memory readFile(String fileName, sl_int64 offset = 0, sl_uint32 length = 0) noexcept;

		virtual sl_bool writeFile(String fileName, const Memory& buffer, FileCreationParams& params = FileCreationParams()) noexcept;

		virtual sl_bool deleteFile(String fileName) noexcept;

	public:
		/* Handle counter functions for Host */
		sl_size increaseHandleCount(String fileName);

		sl_size decreaseHandleCount(String fileName);

		sl_size getOpenHandlesCount();

	protected:
		FileSystemInformation m_volumeInfo;
		HashMap<String, sl_size> m_openHandles;
	};

}

#endif