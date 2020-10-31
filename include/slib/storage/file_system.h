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

	class FileSystemProvider;
	class FileSystemHost;

	class SLIB_EXPORT FileSystem
	{
	public:
		static Ref<FileSystemHost> createHost();

		static Ref<FileSystemHost> getHost(const String& mountPoint);

		static sl_bool unmount(const String& mountPoint);

	};

	// Equals to WinNT File System Flags
	class FileSystemFlags
	{
	public:
		int value;
		SLIB_MEMBERS_OF_FLAGS(FileSystemFlags, value)

		enum {
			CaseSensitive = 0x1,
			SupportsFileCompression = 0x10,
			SupportsEncryption = 0x20000,
			ReadOnlyVolume = 0x80000,
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

	// Equals to WinNT error codes
	enum class FileSystemError
	{
		Success = 0, // ERROR_SUCCESS
		GeneralError = 1, // ERROR_INVALID_FUNCTION
		NotFound = 2, // ERROR_FILE_NOT_FOUND
		PathNotFound = 3, // ERROR_PATH_NOT_FOUND
		AccessDenied = 5, // ERROR_ACCESS_DENIED
		InvalidContext = 6, // ERROR_INVALID_HANDLE
		InvalidData = 13, // ERROR_INVALID_DATA
		OutOfMemory = 14, // ERROR_OUTOFMEMORY
		FileExist = 80, // ERROR_FILE_EXISTS
		InvalidPassword = 86, // ERROR_INVALID_PASSWORD
		BufferOverflow = 122, // ERROR_INSUFFICIENT_BUFFER
		DirNotEmpty = 145, // ERROR_DIR_NOT_EMPTY
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
		virtual sl_bool getInformation(FileSystemInfo& outInfo);
		
		virtual sl_bool getSize(sl_uint64* pTotalSize, sl_uint64* pFreeSize = sl_null);

		virtual Ref<FileContext> openFile(const StringParam& path, const FileOpenParam& param) = 0;

		virtual sl_uint32 readFile(FileContext* context, sl_uint64 offset, void* buf, sl_uint32 size) = 0;

		// offset: negative value means end of file
		virtual sl_uint32 writeFile(FileContext* context, sl_int64 offset, const void* data, sl_uint32 size);

		virtual sl_bool flushFile(FileContext* context);

		virtual sl_bool closeFile(FileContext* context) = 0;

		virtual sl_bool deleteFile(const StringParam& path);

		virtual sl_bool moveFile(const StringParam& pathOld, const StringParam& pathNew, sl_bool flagReplaceIfExists);

		// path or context can be null
		virtual sl_bool getFileInfo(const StringParam& path, FileContext* context, FileInfo& outInfo, const FileInfoMask& mask) = 0;

		// path or context can be null
		virtual sl_bool setFileInfo(const StringParam& path, FileContext* context, const FileInfo& info, const FileInfoMask& mask);

		virtual sl_bool createDirectory(const StringParam& path) = 0;

		virtual sl_bool deleteDirectory(const StringParam& path);

		virtual HashMap<String, FileInfo> getFiles(const StringParam& pathDir) = 0;

	public: // Helpers
		virtual sl_bool existsFile(const StringParam& path) noexcept;

		virtual sl_uint64 getFileSize(FileContext* context);

		virtual sl_uint64 getFileSize(const StringParam& path);

		virtual Memory readFile(const StringParam& path, sl_uint64 offset = 0, sl_uint32 size = SLIB_UINT32_MAX) noexcept;

		virtual sl_uint32 writeFile(const StringParam& path, const void* buf, sl_uint32 size) noexcept;

		virtual sl_uint32 writeFile(const StringParam& path, const Memory& mem) noexcept;

	public:
		virtual FileSystemError getLastError() noexcept;

	protected:
		FileSystemInfo m_fsInfo;

	};

	class FileSystemHostFlags
	{
	public:
		int value;
		SLIB_MEMBERS_OF_FLAGS(FileSystemHostFlags, value)

		enum {
			DebugMode = 0x01,
			UseStdErr = 0x02,
			WriteProtect = 0x04,
			MountAsRemovable = 0x08,
			MountAsNetworkDrive = 0x10,
		};
	};

	class FileSystemHostParam
	{
	public:
		String mountPoint;
		Ref<FileSystemProvider> provider;

		sl_uint32 threadCount;
		sl_uint32 timeout;
		FileSystemHostFlags flags;

	public:
		FileSystemHostParam();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(FileSystemHostParam)

	};

	class FileSystemHost : public Object
	{
		SLIB_DECLARE_OBJECT

	public:
		FileSystemHost();

		~FileSystemHost();

	public:
		String getMountPoint();

		FileSystemProvider* getProvider();

		sl_bool isRunning();

		sl_bool run(const FileSystemHostParam& param);

		sl_size getOpenedHandlesCount();

		sl_size increaseOpenHandlesCount();

		sl_size decreaseOpenHandlesCount();

		virtual String getErrorMessage() = 0;

	protected:
		virtual sl_bool _run() = 0;

	protected:
		FileSystemHostParam m_param;

		sl_bool m_flagRunning;
		sl_reg m_nOpenedHandles;

	};

	class FileSystemWrapper : public FileSystemProvider
	{
	public:
		FileSystemWrapper(const Ref<FileSystemProvider>& base, const String& fileSystemName = sl_null, const String& volumeName = sl_null);

		~FileSystemWrapper();

	public:
		sl_bool getInformation(FileSystemInfo& outInfo) override;

		sl_bool getSize(sl_uint64* pTotalSize, sl_uint64* pFreeSize = sl_null) override;

		Ref<FileContext> openFile(const StringParam& path, const FileOpenParam& param) override;

		sl_uint32	readFile(FileContext* context, sl_uint64 offset, void* buf, sl_uint32 size) override;

		sl_uint32 writeFile(FileContext* context, sl_int64 offset, const void* buf, sl_uint32 size) override;

		sl_bool flushFile(FileContext* context) override;

		sl_bool	closeFile(FileContext* context) override;

		sl_bool deleteFile(const StringParam& path) override;

		sl_bool moveFile(const StringParam& pathOld, const StringParam& pathNew, sl_bool flagReplaceIfExists) override;

		sl_bool getFileInfo(const StringParam& path, FileContext* context, FileInfo& outInfo, const FileInfoMask& mask) override;

		sl_bool setFileInfo(const StringParam& path, FileContext* context, const FileInfo& info, const FileInfoMask& mask) override;

		sl_bool createDirectory(const StringParam& path) override;

		sl_bool deleteDirectory(const StringParam& path) override;

		HashMap<String, FileInfo> getFiles(const StringParam& pathDir) override;

	public:
		FileSystemError getLastError() noexcept override;

	protected:
		// If you want to use different FileContext in wrapper, you will need to override these functions.
		virtual Ref<FileContext> createContext(FileContext* baseContext, const StringParam& path);
		virtual Ref<FileContext> getBaseContext(FileContext* context);

		// If you want to use different path in wrapper, you will need to override these functions.
		virtual String toBasePath(const StringParam& path);
		virtual String toWrapperPath(const String& basePath, sl_bool flagNameOnly);

		// If you want to use different file info in wrapper, you will need to override these functions.
		virtual void convertToBaseFileInfo(FileInfo& info, const FileInfoMask& mask) noexcept;
		virtual void convertToWrapperFileInfo(FileInfo& baseInfo, const FileInfoMask& mask) noexcept;

	protected:
		Ref<FileSystemProvider> m_base;

	};
	
}

#endif