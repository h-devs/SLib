/*
 *   Copyright (c) 2008-2021 SLIBIO <https://github.com/SLIBIO>
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

#include "handle_container.h"
#include "io.h"
#include "time.h"
#include "flags.h"

#ifdef SLIB_PLATFORM_IS_WINDOWS
typedef void* sl_file;
#define SLIB_FILE_INVALID_HANDLE ((void*)((sl_reg)-1))
#else
typedef int sl_file;
#define SLIB_FILE_INVALID_HANDLE ((sl_file)-1)
#endif

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

		Device = 0x8000,

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
		System = 0x4, // [Win32]
		Directory = 0x10,
		Archive = 0x20, // [Win32]
		Device = 0x40,
		Normal = 0x80,
		Temporary = 0x100, // [Win32]
		SparseFile = 0x200, // [Win32]
		ReparsePoint = 0x400, // [Win32]
		Compressed = 0x800, // [Win32]
		Offline = 0x1000, // [Win32]
		NotContentIndexed = 0x2000, // [Win32]
		Encrypted = 0x4000, // [Win32]
		Virtual = 0x10000, // [Win32]
		Socket = 0x20000, // [Unix]
		Link = 0x40000, // [Unix] Symbolic Link
		CharDevice = 0x80000, // [Unix]
		FIFO = 0x100000, // [Unix]

		ReadByOthers = 0x00200000,
		WriteByOthers = 0x00400000,
		ExecuteByOthers = 0x00800000,
		ReadByGroup = 0x01000000,
		WriteByGroup = 0x02000000,
		ExecuteByGroup = 0x04000000,
		ReadByUser = 0x08000000,
		WriteByUser = 0x10000000,
		ExecuteByUser = 0x20000000,
		ReadByAnyone = ReadByUser | ReadByGroup | ReadByOthers,
		WriteByAnyone = WriteByUser | WriteByGroup | WriteByOthers,
		ExecuteByAnyone = ExecuteByUser | ExecuteByGroup | ExecuteByOthers,
		AllAccess = ReadByAnyone | WriteByAnyone | ExecuteByAnyone,
		NoAccess = 0x40000000,

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
		FileInfo() noexcept;

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(FileInfo)

	};

	class SLIB_EXPORT FileOpenParam
	{
	public:
		FileMode mode;
		FileAttributes attributes;

	public:
		FileOpenParam() noexcept;

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(FileOpenParam)

	};

	class SLIB_EXPORT File
	{
		SLIB_DECLARE_HANDLE_CONTAINER_MEMBERS(File, sl_file, m_file, SLIB_FILE_INVALID_HANDLE)
		SLIB_DECLARE_IO_MEMBERS(const noexcept)

	public:
		static File open(const StringParam& filePath, const FileOpenParam& param) noexcept;

		static File open(const StringParam& filePath, const FileMode& mode, const FileAttributes& attrs) noexcept;

		static File open(const StringParam& filePath, const FileMode& mode) noexcept;
	
		static File openForRead(const StringParam& filePath) noexcept;

		static File openForWrite(const StringParam& filePath) noexcept;

		static File openForReadWrite(const StringParam& filePath) noexcept;

		static File openForAppend(const StringParam& filePath) noexcept;

		static File openForRandomAccess(const StringParam& filePath) noexcept;
	
		static File openForRandomRead(const StringParam& filePath) noexcept;

		/*
			Physical Disks and Volumes
		 
				"\\\\.\\PhysicalDrive0"  (Win32)
				"\\\\.\\CdRom0"
				"\\\\.\\A:"
				"C:\\"
				"\\\\?\\Volume{...}\\"
		 
		 		"/dev/disk0"  (macOS)
		 		"/dev/sda1"   (Linux)
		*/
		static File openDevice(const StringParam& path, const FileMode& mode) noexcept;

		static File openDeviceForRead(const StringParam& path) noexcept;

	public:
		SLIB_CONSTEXPR sl_bool isOpened() const
		{
			return m_file != SLIB_FILE_INVALID_HANDLE;
		}
		
		void close() noexcept;


		sl_bool getPosition(sl_uint64& outPos) const noexcept;

		sl_bool getSize(sl_uint64& outSize) const noexcept;
		
		sl_bool seek(sl_int64 offset, SeekPosition from) const noexcept;

		sl_bool isEnd(sl_bool& outFlag) const noexcept;
	

		sl_reg read(void* buf, sl_size size) const noexcept;

		sl_int32 read32(void* buf, sl_uint32 size) const noexcept;

		sl_bool waitRead(sl_int32 timeout = -1) const noexcept;

		sl_reg write(const void* buf, sl_size size) const noexcept;

		sl_int32 write32(const void* buf, sl_uint32 size) const noexcept;
	
		sl_bool waitWrite(sl_int32 timeout = -1) const noexcept;

		// works only if the file is already opened
		sl_bool setSize(sl_uint64 size) const noexcept;
		
		static sl_bool getSize(const StringParam& path, sl_uint64& outSize) noexcept;
		
		static sl_uint64 getSize(const StringParam& path) noexcept;

		static sl_bool getDiskSize(const StringParam& devicePath, sl_uint64& outSize) noexcept;

		static sl_uint64 getDiskSize(const StringParam& devicePath) noexcept;

		
		sl_bool lock() const noexcept;

		sl_bool unlock() const noexcept;

		sl_bool flush() const noexcept;

		// Unix only
		sl_bool setNonBlocking(sl_bool flag) const noexcept;

		sl_bool getDiskSize(sl_uint64& outSize) const noexcept;

		sl_uint64 getDiskSize() const noexcept;

		
		Time getModifiedTime() const noexcept;

		Time getAccessedTime() const noexcept;

		Time getCreatedTime() const noexcept;

		static Time getModifiedTime(const StringParam& filePath) noexcept;

		static Time getAccessedTime(const StringParam& filePath) noexcept;

		static Time getCreatedTime(const StringParam& filePath) noexcept;

		sl_bool setModifiedTime(const Time& time) const noexcept;

		sl_bool setAccessedTime(const Time& time) const noexcept;

		sl_bool setCreatedTime(const Time& time) const noexcept;

		static sl_bool setModifiedTime(const StringParam& filePath, const Time& time) noexcept;

		static sl_bool setAccessedTime(const StringParam& filePath, const Time& time) noexcept;

		static sl_bool setCreatedTime(const StringParam& filePath, const Time& time) noexcept;


		FileAttributes getAttributes() const noexcept;

		static FileAttributes getAttributes(const StringParam& filePath) noexcept;

		static sl_bool setAttributes(const StringParam& filePath, const FileAttributes& attrs) noexcept;

		static sl_bool exists(const StringParam& filePath) noexcept;
	
		static sl_bool isFile(const StringParam& filePath) noexcept;
	
		static sl_bool isDirectory(const StringParam& filePath) noexcept;

		static sl_bool isHidden(const StringParam& filePath) noexcept;
	
		static sl_bool setHidden(const StringParam& filePath, sl_bool flagHidden = sl_true) noexcept;

		static sl_bool isReadOnly(const StringParam& filePath) noexcept;

		static sl_bool setReadOnly(const StringParam& filePath, sl_bool flagReadOnly = sl_true) noexcept;

		// Linux only
		static String getCap(const StringParam& filePath) noexcept;

		// Linux only
		static sl_bool setCap(const StringParam& filePath, const StringParam& cap) noexcept;

		// Linux only
		static sl_bool equalsCap(const StringParam& filePath, const StringParam& cap) noexcept;


		static sl_bool createDirectory(const StringParam& dirPath, const FileOperationFlags& flags = FileOperationFlags::Default) noexcept;

		static sl_bool createDirectories(const StringParam& dirPath) noexcept;

		// [Win32] Shortcut, [Unix] Symbolic Link
		static sl_bool createLink(const StringParam& pathTarget, const StringParam& pathLink) noexcept;

		static sl_bool deleteFile(const StringParam& filePath) noexcept;

		static sl_bool deleteDirectory(const StringParam& filePath) noexcept;

		static sl_bool remove(const StringParam& path, const FileOperationFlags& flags = FileOperationFlags::Default) noexcept;

		static sl_bool copyFile(const StringParam& pathSource, const StringParam& pathTarget, const FileOperationFlags& flags = FileOperationFlags::Default) noexcept;

		static sl_bool copy(const StringParam& pathSource, const StringParam& pathTarget, const FileOperationFlags& flags = FileOperationFlags::Default) noexcept;

		static sl_bool move(const StringParam& pathOriginal, const StringParam& filePathNew, const FileOperationFlags& flags = FileOperationFlags::Default) noexcept;
	

		static List<String> getFiles(const StringParam& dirPath) noexcept;

		static HashMap< String, FileInfo, Hash<String>, Compare<String> > getFileInfos(const StringParam& dirPath) noexcept;
	
		static List<String> getAllDescendantFiles(const StringParam& dirPath) noexcept;
	
		
		static String getRealPath(const StringParam& filePath) noexcept;
		
		static String getOwnerName(const StringParam& filePath) noexcept;
		
		static sl_bool setOwnerName(const StringParam& filePath, const StringParam& owner) noexcept;
		
		static String getGroupName(const StringParam& filePath) noexcept;
		
		static sl_bool setGroupName(const StringParam& filePath, const StringParam& group) noexcept;

		
		static Memory readAllBytes(const StringParam& path, sl_size maxSize = SLIB_SIZE_MAX) noexcept;
		
		static String readAllTextUTF8(const StringParam& path, sl_size maxSize = SLIB_SIZE_MAX) noexcept;
		
		static String16 readAllTextUTF16(const StringParam& path, EndianType endian = Endian::Little, sl_size maxSize = SLIB_SIZE_MAX) noexcept;
		
		static String readAllText(const StringParam& path, Charset* outCharset = sl_null, sl_size maxSize = SLIB_SIZE_MAX) noexcept;
		
		static String16 readAllText16(const StringParam& path, Charset* outCharset = sl_null, sl_size maxSize = SLIB_SIZE_MAX) noexcept;
	
		static sl_size writeAllBytes(const StringParam& path, const void* buf, sl_size size) noexcept;

		static sl_size writeAllBytes(const StringParam& path, const Memory& mem) noexcept;

		static sl_bool writeAllTextUTF8(const StringParam& path, const StringParam& text, sl_bool flagWriteByteOrderMark = sl_false) noexcept;

		static sl_bool writeAllTextUTF16LE(const StringParam& path, const StringParam& text, sl_bool flagWriteByteOrderMark = sl_false) noexcept;

		static sl_bool writeAllTextUTF16BE(const StringParam& path, const StringParam& text, sl_bool flagWriteByteOrderMark = sl_false) noexcept;

		static sl_size appendAllBytes(const StringParam& path, const void* buf, sl_size size) noexcept;

		static sl_size appendAllBytes(const StringParam& path, const Memory& mem) noexcept;

		static sl_bool appendAllTextUTF8(const StringParam& path, const StringParam& text) noexcept;

		static sl_bool appendAllTextUTF16LE(const StringParam& path, const StringParam& text) noexcept;

		static sl_bool appendAllTextUTF16BE(const StringParam& path, const StringParam& text) noexcept;

	
		static String getParentDirectoryPath(const StringParam& path) noexcept;

		static String getFileName(const StringParam& path) noexcept;

		static String getFileExtension(const StringParam& path) noexcept;

		static String getFileNameOnly(const StringParam& path) noexcept;

		static String normalizeDirectoryPath(const StringParam& path) noexcept;

		static String joinPath(const StringParam& path1, const StringParam& path2) noexcept;
	

		// converts any invalid characters (0~0x1f, 0x7f~0x9f, :*?"<>|\/) into "_"
		static String makeSafeFileName(const StringParam& fileName) noexcept;
	
		// converts any invalid characters (0~0x1f, 0x7f~0x9f, :*?"<>|) into "_"
		static String makeSafeFilePath(const StringParam& filePath) noexcept;
	
		static String findParentPathContainingFile(const StringParam& basePath, const StringParam& filePath, sl_uint32 nDeep = SLIB_UINT32_MAX) noexcept;
	
	private:
		static sl_file _open(const StringParam& filePath, const FileMode& mode, const FileAttributes& attrs) noexcept;

		static sl_bool _close(sl_file file) noexcept;

		static FileAttributes _fixAttributes(const FileAttributes& attrs) noexcept;

		FileAttributes _getAttributes() const noexcept;

		static FileAttributes _getAttributes(const StringParam& filePath) noexcept;

		static sl_bool _setAttributes(const StringParam& filePath, const FileAttributes& attrs) noexcept;

		static sl_bool _createDirectory(const StringParam& dirPath) noexcept;

		static sl_bool _copyFile(const StringParam& pathSource, const StringParam& pathTarget) noexcept;

		static sl_bool _move(const StringParam& pathOriginal, const StringParam& filePathNew) noexcept;

	public:
		File& operator*() noexcept
		{
			return *this;
		}

	};

}

#endif
