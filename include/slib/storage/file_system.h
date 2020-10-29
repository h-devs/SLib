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
		virtual sl_bool getInformation(FileSystemInfo& outInfo, const FileSystemInfoMask& mask) = 0;
		
		virtual Ref<FileContext> openFile(const StringParam& path, const FileOpenParam& param) = 0;

		virtual sl_size readFile(FileContext* context, sl_uint64 offset, void* buf, sl_size size) = 0;

		// offset: negative value means end of file
		virtual sl_size writeFile(FileContext* context, sl_int64 offset, const void* data, sl_size size);

		virtual sl_bool flushFile(FileContext* context);

		virtual sl_bool closeFile(FileContext* context) = 0;

		virtual sl_bool deleteFile(const StringParam& path);

		virtual sl_bool moveFile(const StringParam& pathOld, const StringParam& pathNew, sl_bool flagReplaceIfExists);

		virtual sl_bool getFileInfo(const StringParam& path, FileContext* context, FileInfo& outInfo, const FileInfoMask& mask) = 0;

		virtual sl_bool setFileInfo(const StringParam& path, FileContext* context, const FileInfo& info, const FileInfoMask& mask);

		virtual sl_bool createDirectory(const StringParam& path) = 0;

		virtual sl_bool deleteDirectory(const StringParam& path);

		virtual HashMap<String, FileInfo> getFiles(const StringParam& pathDir) = 0;

	public: // Helpers
		virtual sl_bool existsFile(const StringParam& path);

		virtual sl_uint64 getFileSize(FileContext* context);

		virtual sl_uint64 getFileSize(const StringParam& path);

	};

	class FileSystemHostParam
	{
	public:
		String mountPoint;
		Ref<FileSystemProvider> provider;

		sl_uint32 threadCount;
		sl_uint32 timeout;
		sl_bool flagDebugMode;
		sl_bool flagUseStderr;

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
		
		void stop();

		sl_size getOpenedHandlesCount();

	public:
		Ref<FileContext> openFile(const StringParam& path, const FileOpenParam& param);

		sl_bool closeFile(FileContext* context);

	protected:
		virtual sl_bool _run() = 0;

		virtual void _stop() = 0;

	protected:
		FileSystemHostParam m_param;

		sl_bool m_flagRunning;
		sl_reg m_nOpendHandles;

	};

	class FileSystemWrapper : public FileSystemProvider
	{
	public:
		FileSystemWrapper(const Ref<FileSystemProvider>& base);

		~FileSystemWrapper();

	public:
		sl_bool getInformation(FileSystemInfo& outInfo, const FileSystemInfoMask& mask) override;

		Ref<FileContext> openFile(const StringParam& path, const FileOpenParam& param) override;

		sl_size	readFile(FileContext* context, sl_uint64 offset, void* buf, sl_size size) override;

		sl_size writeFile(FileContext* context, sl_int64 offset, const void* buf, sl_size size) override;

		sl_bool flushFile(FileContext* context) override;

		sl_bool	closeFile(FileContext* context) override;

		sl_bool deleteFile(const StringParam& path) override;

		sl_bool moveFile(const StringParam& pathOld, const StringParam& pathNew, sl_bool flagReplaceIfExists) override;

		sl_bool getFileInfo(const StringParam& path, FileContext* context, FileInfo& outInfo, const FileInfoMask& mask) override;

		sl_bool setFileInfo(const StringParam& path, FileContext* context, const FileInfo& info, const FileInfoMask& mask) override;

		sl_bool createDirectory(const StringParam& path) override;

		sl_bool deleteDirectory(const StringParam& path) override;

		HashMap<String, FileInfo> getFiles(const StringParam& pathDir) override;

	public: // Helpers
		sl_bool existsFile(const StringParam& path) override;

		sl_uint64 getFileSize(FileContext* context) override;

		sl_uint64 getFileSize(const StringParam& path) override;

	protected:
		// If you want to use different FileContext in wrapper, you will need to override this function.
		virtual Ref<FileContext> createContext(FileContext* baseContext);

		// If you want to use different FileContext in wrapper, you will need to override this function.
		virtual Ref<FileContext> getBaseContext(FileContext* context);

		// If you want to use different path in wrapper, you will need to override this function.
		virtual String getBaseFileName(const StringParam& path);

	protected:
		Ref<FileSystemProvider> m_base;

	};
	
}

#endif