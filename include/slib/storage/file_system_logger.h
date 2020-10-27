/**
* @file slib/storage/file_system_logger.h
* FileSystem Logger Wrapper Definition.
*
* @copyright 2020 Steve Han
*/

#pragma once

#include "file_system_wrapper.h"

#include "../core/regex.h"

namespace slib
{

	class FsLogger : public FileSystemWrapper
	{
	public:
		enum FsLogFlags : sl_uint32 {
			FsLogCreate = 0x01,
			FsLogOpen = 0x02,
			FsLogFlush = 0x04,
			FsLogClose = 0x08,
			FsLogRead = 0x10,
			FsLogWrite = 0x20,
			FsLogLock = 0x40,
			FsLogUnlock = 0x80,
			FsLogOpenOp = FsLogCreate | FsLogOpen | FsLogClose,
			FsLogCreateOpen = FsLogCreate | FsLogOpen,
			FsLogReadWrite = FsLogRead | FsLogWrite,
			FsLogLockOp = FsLogLock | FsLogUnlock,
			FsLogFileBasicOp = 0xFF,

			FsLogCanDelete = 0x0100,
			FsLogDelete = 0x0200,
			FsLogRename = 0x0400,
			FsLogList = 0x0800,
			FsLogDeleteOp = FsLogCanDelete | FsLogDelete,

			FsLogGetInfo = 0x1000,
			FsLogSetInfo = 0x2000,
			FsLogGetSec = 0x4000,
			FsLogSetSec = 0x8000,
			FsLogInfoOp = FsLogGetInfo | FsLogSetInfo,
			FsLogSecOp = FsLogGetSec | FsLogSetSec,
			FsLogFileGetOp = FsLogGetInfo | FsLogGetSec,
			FsLogFileSetOp = FsLogSetInfo | FsLogSetSec,
			FsLogFileInfoOp = FsLogInfoOp | FsLogSecOp,
			FsLogFileOp = 0xFFFF,

			FsLogSetAttrInfo = 0x010000,
			FsLogSetTimeInfo = 0x020000,
			FsLogSetFileSizeInfo = 0x040000,
			FsLogSetAllocSizeInfo = 0x080000,
			FsLogSetInfoDetail = 0x0F0000,

			FsLogListStream = 0x100000,
			FsLogHandleCountOnClose = 0x200000,
			FsLogAttrNoDate = 0x400000,
			FsLogDateAsString = 0x800000,

			FsLogVolumeInfo = 0x01000000,
			FsLogGetVolumeInfo = 0x02000000,
			FsLogGetVolumeSize = 0x04000000,
			FsLogSetVolumeName = 0x08000000,
			FsLogVolumeOp = 0x0F000000,

			FsLogFileName = 0x10000000,			// when this flag is set, ignores FsLogContextAddress
			FsLogContextAddress = 0x20000000,
			FsLogRet = 0x40000000,
			FsLogErrors = 0x80000000,
			FsLogRetAndErrors = FsLogRet | FsLogErrors,
			FsLogDefault = FsLogFileName | FsLogRet | FsLogErrors,
			FsLogDefaultErrors = FsLogFileName | FsLogErrors,

			FsLogAllOp = FsLogFileOp | FsLogVolumeOp | FsLogSetInfoDetail | FsLogListStream,
			FsLogAll = 0xFFFFFFFF,
		};

	public:
		FsLogger(Ref<FileSystemProvider> base, sl_uint32 logFlags = FsLogAll, String regexFilter = ".*");

		~FsLogger();

	public:
		sl_bool getInformation(FileSystemInfo& info, const FileSystemInfoMask& mask) override;

		Ref<FileContext> openFile(const String& path, const FileOpenParam& param) override;

		sl_size	readFile(FileContext* context, sl_uint64 offset, void* buf, sl_size size) override;

		sl_size writeFile(FileContext* context, sl_int64 offset, const void* buf, sl_size size) override;

		sl_bool flushFile(FileContext* context) override;

		sl_bool	closeFile(FileContext* context) override;

		sl_bool deleteFile(const String& filePath) override;

		sl_bool moveFile(const String& oldFilePath, const String& newFilePath, sl_bool flagReplaceIfExists) override;

		sl_bool lockFile(FileContext* context, sl_uint64 offset, sl_uint64 length) override;

		sl_bool unlockFile(FileContext* context, sl_uint64 offset, sl_uint64 length) override;

		sl_bool	getFileInfo(const String& filePath, FileInfo& outInfo, const FileInfoMask& mask) override;

		sl_bool setFileInfo(const String& filePath, const FileInfo& info, const FileInfoMask& mask) override;

		HashMap<String, FileInfo> getFiles(const String& pathDir) override;

	private:
		sl_uint32 m_flags;
		RegEx m_regex;
	};

}
