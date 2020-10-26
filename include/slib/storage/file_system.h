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
#include "../core/file.h"
#include "../core/hash_map.h"

#define SLIB_FILE_SYSTEM_CAN_THROW

namespace slib
{

	class FileSystemFlags
	{
	public:
		int value;
		SLIB_MEMBERS_OF_FLAGS(FileSystemFlags, value)

		enum {
			IsCaseSensitiveSearch = 0x1,
			SupportsFileCompression = 0x2,
			SupportsEncryption = 0x4,
			IsReadOnlyVolume = 0x8
		};
	};

	class FileSystemInfoMask
	{
	public:
		int value;
		SLIB_MEMBERS_OF_FLAGS(FileSystemInfoMask, value)

		enum {
			Basic = 0x1,
			Size = 0x2,
			All = 0xffff
		};
	};

	class SLIB_EXPORT FileSystemInfo
	{
	public:
		FileSystemFlags flags;
		String volumeName;
		String fileSystemName;
		Time creationTime;
		sl_uint32 serialNumber;
		sl_uint16 sectorSize;
		sl_uint16 sectorsPerAllocationUnit;
		sl_uint32 maxPathLength;

		sl_uint64 totalSize;
		sl_uint64 freeSize;

	public:
		FileSystemInfo();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(FileSystemInfo)

	};

	class FileInfoMask
	{
	public:
		int value;
		SLIB_MEMBERS_OF_FLAGS(FileInfoMask, value)

		enum {
			Attributes = 0x1,
			Size = 0x2,
			AllocSize = 0x4,
			Time = 0x8,
			All = 0xffff
		};
	};

	class SLIB_EXPORT FileInfo
	{
	public:
		String path;
		FileAttributes attributes;
		sl_uint64 size;
		sl_uint64 allocSize;
		Time createdAt;
		Time modifiedAt;
		Time lastAccessedAt;

	public:
		FileInfo();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(FileInfo)

	};

	// Equals to WinNT error codes
	enum class FileSystemError
	{
		Success = 0, // ERROR_SUCCESS
		GeneralError = 1, // ERROR_INVALID_FUNCTION
		NotFound = 2, // ERROR_FILE_NOT_FOUND
		AccessDenied = 5, // ERROR_ACCESS_DENIED
		InvalidContext = 6, // ERROR_INVALID_HANDLE
		InvalidData = 13, // ERROR_INVALID_DATA
		FileExist = 80, // ERROR_FILE_EXISTS
		InvalidPassword = 86, // ERROR_INVALID_PASSWORD
		BufferOverflow = 122, // ERROR_INSUFFICIENT_BUFFER
		AlreadyExist = 183,	// ERROR_ALREADY_EXISTS
		InitFailure = 575, // ERROR_APP_INIT_FAILURE
		NotImplemented = -1,
	};

	class FileContext : public Object
	{
		SLIB_DECLARE_OBJECT

	public:
		FileContext();

		~FileContext();

	};

	class FileSystemProvider : public Object
	{
		SLIB_DECLARE_OBJECT

	public:
		FileSystemProvider();

		~FileSystemProvider();

	public:
		virtual sl_bool getInformation(FileSystemInfo& outInfo, const FileSystemInfoMask& mask) = 0;
		
		virtual Ref<FileContext> openFile(const String& path, const FileOpenParam& param) = 0;

		virtual sl_size readFile(FileContext* context, sl_uint64 offset, void* buf, sl_size size) = 0;

		// offset: negative value means end of file
		virtual sl_size writeFile(FileContext* context, sl_int64 offset, const void* data, sl_size size);

		virtual sl_bool flushFile(FileContext* context);

		virtual sl_bool closeFile(FileContext* context) = 0;

		virtual sl_bool deleteFile(const String& path);

		virtual sl_bool moveFile(const String& pathOld, const String& pathNew, sl_bool flagReplaceIfExists);

		virtual sl_bool lockFile(FileContext* context, sl_uint64 offset, sl_uint64 length);

		virtual sl_bool unlockFile(FileContext* context, sl_uint64 offset, sl_uint64 length);

		virtual sl_bool	getFileInfo(FileContext* context, FileInfo& outInfo, const FileInfoMask& mask) = 0;

		virtual sl_bool setFileInfo(FileContext* context, const FileInfo& info, const FileInfoMask& mask);

		virtual HashMap<String, FileInfo> getFiles(const String& pathDir) = 0;

	};

}

#endif