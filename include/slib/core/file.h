/*
 *   Copyright (c) 2008-2018 SLIBIO <https://github.com/SLIBIO>
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

#ifndef CHECKHEADER_SLIB_CORE_FILE
#define CHECKHEADER_SLIB_CORE_FILE

#include "io.h"
#include "flags.h"

typedef sl_reg sl_file;
#define SLIB_FILE_INVALID_HANDLE ((sl_file)(-1))

namespace slib
{

	template <class T> class List;
	template <class KT, class VT, class HASH, class KEY_COMPARE> class HashMap;
	
	SLIB_DEFINE_FLAGS(FileMode, {

		Read = 1,
		Write = 2,
		Sync = 4,
		Directory = 8,

		ReadData = 0x10,
		WriteData = 0x20,
		ReadAttrs = 0x40,
		WriteAttrs = 0x80,

		NotCreate = 0x100,
		NotTruncate = 0x200,
		SeekToEnd = 0x1000,
		HintRandomAccess = 0x2000,

		ReadWrite = Read | Write,
		Append = Write | NotTruncate | SeekToEnd,
		RandomAccess = Read | Write | NotTruncate | HintRandomAccess,
		RandomRead = Read | HintRandomAccess,

		ShareRead = 0x10000,
		ShareWrite = 0x20000,
		ShareReadWrite = ShareRead | ShareWrite,
		ShareDelete = 0x40000,
		ShareAll = ShareRead | ShareWrite | ShareDelete

	})

	// Equals to WinNT File Attributes
	SLIB_DEFINE_FLAGS(FileAttributes, {
	
		Default = 0,
		ReadOnly = 0x1,
		Hidden = 0x2,
		System = 0x4,
		Directory = 0x10,
		Archive = 0x20,
		Device = 0x40,
		Normal = 0x80,
		Temporary = 0x100,
		SparseFile = 0x200,
		ReparsePoint = 0x400,
		Compressed = 0x800,
		Offline = 0x1000,
		NotContentIndexed = 0x2000,
		Encrypted = 0x4000,
		Virtual = 0x10000,

		ReadByOthers = 0x00100000,
		WriteByOthers = 0x00200000,
		ExecuteByOthers = 0x00400000,
		ReadByGroup = 0x00800000,
		WriteByGroup = 0x01000000,
		ExecuteByGroup = 0x02000000,
		ReadByUser = 0x04000000,
		WriteByUser = 0x08000000,
		ExecuteByUser = 0x10000000,
		ReadByAnyone = ReadByUser | ReadByGroup | ReadByOthers,
		WriteByAnyone = WriteByUser | WriteByGroup | WriteByOthers,
		ExecuteByAnyone = ExecuteByUser | ExecuteByGroup | ExecuteByOthers,
		AllAccess = ReadByAnyone | WriteByAnyone | ExecuteByAnyone,
		NoAccess = 0x20000000,

		NotExist = 0x80000000
		
	})

	SLIB_DEFINE_FLAGS(FileOperationFlags, {
		Default = 0,

		Recursive = 0x1,
		NotReplace = 0x2,

		ErrorOnExisting = 0x10000,
		ErrorOnNotExisting = 0x20000,
		AbortOnError = 0x40000
	})

	class SLIB_EXPORT FileInfo
	{
	public:
		FileAttributes attributes;
		sl_uint64 size;
		sl_uint64 allocSize;
		Time createdAt;
		Time modifiedAt;
		Time accessedAt;

	public:
		FileInfo();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(FileInfo)

	};

	class SLIB_EXPORT FileOpenParam
	{
	public:
		FileMode mode;
		FileAttributes attributes;

	public:
		FileOpenParam();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(FileOpenParam)

	};
	
	class SLIB_EXPORT File : public IO
	{
		SLIB_DECLARE_OBJECT
	
	private:
		sl_file m_file;

	public:
		File(sl_file file);
	
		~File();
	
	public:
		static Ref<File> open(const StringParam& filePath, const FileOpenParam& param);

		static Ref<File> open(const StringParam& filePath, const FileMode& mode, const FileAttributes& attrs);

		static Ref<File> open(const StringParam& filePath, const FileMode& mode);
	
		static Ref<File> openForRead(const StringParam& filePath);

		static Ref<File> openForWrite(const StringParam& filePath);

		static Ref<File> openForReadWrite(const StringParam& filePath);

		static Ref<File> openForAppend(const StringParam& filePath);

		static Ref<File> openForRandomAccess(const StringParam& filePath);
	
		static Ref<File> openForRandomRead(const StringParam& filePath);

		/*
			Physical Disks and Volumes
		 
				"\\\\.\\PhysicalDrive0"  (Win32)
				"\\\\.\\CdRom0"
				"\\\\.\\A:"
		 
		 		"/dev/disk0"  (macOS)
		 		"/dev/sda1"   (Linux)
		*/
		static Ref<File> openDevice(const StringParam& path, const FileMode& mode);

		static Ref<File> openDeviceForRead(const StringParam& path);

	public:
		void close() override;

		sl_bool isOpened() const;
		
		sl_file getHandle() const;
		
		void setHandle(sl_file handle);
		
		void clearHandle();

		using IO::getPosition;
		sl_bool getPosition(sl_uint64& outPos) override;

		using IO::getSize;
		sl_bool getSize(sl_uint64& outSize) override;
		
		sl_bool seek(sl_int64 offset, SeekPosition from) override;

		using IO::isEnd;
		sl_bool isEnd(sl_bool& outFlag) override;
	

		sl_int32 read32(void* buf, sl_uint32 size) override;

		sl_int32 write32(const void* buf, sl_uint32 size) override;
	
	
		// works only if the file is already opened
		sl_bool setSize(sl_uint64 size) override;

		
		static sl_bool getSizeByHandle(sl_file fd, sl_uint64& outSize);

		static sl_uint64 getSizeByHandle(sl_file fd);
		
		static sl_bool getSize(const StringParam& path, sl_uint64& outSize);
		
		static sl_uint64 getSize(const StringParam& path);

		static sl_bool getDiskSizeByHandle(sl_file fd, sl_uint64& outSize);

		static sl_uint64 getDiskSizeByHandle(sl_file fd);

		static sl_bool getDiskSize(const StringParam& devicePath, sl_uint64& outSize);

		static sl_uint64 getDiskSize(const StringParam& devicePath);

		
		sl_bool lock();

		sl_bool unlock();

		sl_bool flush();
	
		sl_bool getDiskSize(sl_uint64& outSize);

		sl_uint64 getDiskSize();

		
		Time getModifiedTime();

		Time getAccessedTime();

		Time getCreatedTime();

		static Time getModifiedTime(const StringParam& filePath);

		static Time getAccessedTime(const StringParam& filePath);

		static Time getCreatedTime(const StringParam& filePath);

		sl_bool setModifiedTime(Time time);

		sl_bool setAccessedTime(Time time);

		sl_bool setCreatedTime(Time time);

		static sl_bool setModifiedTime(const StringParam& filePath, Time time);

		static sl_bool setAccessedTime(const StringParam& filePath, Time time);

		static sl_bool setCreatedTime(const StringParam& filePath, Time time);


		FileAttributes getAttributes();

		static FileAttributes getAttributes(const StringParam& filePath);

		static sl_bool setAttributes(const StringParam& filePath, const FileAttributes& attrs);

		static sl_bool exists(const StringParam& filePath);
	
		static sl_bool isFile(const StringParam& filePath);
	
		static sl_bool isDirectory(const StringParam& filePath);

		static sl_bool isHidden(const StringParam& filePath);
	
		static sl_bool setHidden(const StringParam& filePath, sl_bool flagHidden = sl_true);

		static sl_bool isReadOnly(const StringParam& filePath);

		static sl_bool setReadOnly(const StringParam& filePath, sl_bool flagReadOnly = sl_true);


		static sl_bool createDirectory(const StringParam& dirPath, const FileOperationFlags& flags = FileOperationFlags::Default);

		static sl_bool createDirectories(const StringParam& dirPath);

		static sl_bool deleteFile(const StringParam& filePath);

		static sl_bool deleteDirectory(const StringParam& filePath);

		static sl_bool remove(const StringParam& path, const FileOperationFlags& flags = FileOperationFlags::Default);

		static sl_bool copyFile(const StringParam& pathSource, const StringParam& pathTarget, const FileOperationFlags& flags = FileOperationFlags::Default);

		static sl_bool copy(const StringParam& pathSource, const StringParam& pathTarget, const FileOperationFlags& flags = FileOperationFlags::Default);

		static sl_bool move(const StringParam& pathOriginal, const StringParam& filePathNew, const FileOperationFlags& flags = FileOperationFlags::Default);
	

		static List<String> getFiles(const StringParam& dirPath);

		static HashMap< String, FileInfo, Hash<String>, Compare<String> > getFileInfos(const StringParam& dirPath);
	
		static List<String> getAllDescendantFiles(const StringParam& dirPath);
	

		Memory readAllBytes(sl_size maxSize = SLIB_SIZE_MAX);
		
		static Memory readAllBytes(const StringParam& path, sl_size maxSize = SLIB_SIZE_MAX);
		
		String readAllTextUTF8(sl_size maxSize = SLIB_SIZE_MAX);

		static String readAllTextUTF8(const StringParam& path, sl_size maxSize = SLIB_SIZE_MAX);

		String16 readAllTextUTF16(EndianType endian = Endian::Little, sl_size maxSize = SLIB_SIZE_MAX);
		
		static String16 readAllTextUTF16(const StringParam& path, EndianType endian = Endian::Little, sl_size maxSize = SLIB_SIZE_MAX);
		
		String readAllText(Charset* outCharset = sl_null, sl_size maxSize = SLIB_SIZE_MAX);

		static String readAllText(const StringParam& path, Charset* outCharset = sl_null, sl_size maxSize = SLIB_SIZE_MAX);
		
		String16 readAllText16(Charset* outCharset = sl_null, sl_size maxSize = SLIB_SIZE_MAX);

		static String16 readAllText16(const StringParam& path, Charset* outCharset = sl_null, sl_size maxSize = SLIB_SIZE_MAX);
	
		static sl_size writeAllBytes(const StringParam& path, const void* buf, sl_size size);

		static sl_size writeAllBytes(const StringParam& path, const Memory& mem);

		static sl_bool writeAllTextUTF8(const StringParam& path, const StringParam& text, sl_bool flagWriteByteOrderMark = sl_false);

		static sl_bool writeAllTextUTF16LE(const StringParam& path, const StringParam& text, sl_bool flagWriteByteOrderMark = sl_false);

		static sl_bool writeAllTextUTF16BE(const StringParam& path, const StringParam& text, sl_bool flagWriteByteOrderMark = sl_false);

		static sl_size appendAllBytes(const StringParam& path, const void* buf, sl_size size);

		static sl_size appendAllBytes(const StringParam& path, const Memory& mem);

		static sl_bool appendAllTextUTF8(const StringParam& path, const StringParam& text);

		static sl_bool appendAllTextUTF16LE(const StringParam& path, const StringParam& text);

		static sl_bool appendAllTextUTF16BE(const StringParam& path, const StringParam& text);

	
		static String getParentDirectoryPath(const StringParam& path);

		static String getFileName(const StringParam& path);

		static String getFileExtension(const StringParam& path);

		static String getFileNameOnly(const StringParam& path);

		static String normalizeDirectoryPath(const StringParam& path);

		static String joinPath(const StringParam& path1, const StringParam& path2);
	

		// converts any invalid characters (0~0x1f, 0x7f~0x9f, :*?"<>|\/) into "_"
		static String makeSafeFileName(const String& fileName);
	
		// converts any invalid characters (0~0x1f, 0x7f~0x9f, :*?"<>|) into "_"
		static String makeSafeFilePath(const String& filePath);
	
		static String findParentPathContainingFile(const String& basePath, const String& filePath, sl_uint32 nDeep = SLIB_UINT32_MAX);
	
#if defined(SLIB_PLATFORM_IS_UNIX)
		static sl_bool setNonBlocking(int fd, sl_bool flag);
#endif
		
		static String getRealPath(const StringParam& filePath);

	private:
		static sl_file _open(const StringParam& filePath, const FileMode& mode, const FileAttributes& attrs);

		static sl_bool _close(sl_file file);

		static FileAttributes _fixAttributes(const FileAttributes& attrs);

		static FileAttributes _getAttributes(const StringParam& filePath);

		static sl_bool _setAttributes(const StringParam& filePath, const FileAttributes& attrs);

		static sl_bool _createDirectory(const StringParam& dirPath);

		static sl_bool _copyFile(const StringParam& pathSource, const StringParam& pathTarget);

		static sl_bool _move(const StringParam& pathOriginal, const StringParam& filePathNew);

	};
	
}

#endif
