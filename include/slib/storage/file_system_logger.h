/**
* @file slib/storage/file_system_logger.h
* FileSystem Logger Wrapper Definition.
*
* @copyright 2020 Steve Han
*/

#pragma once

#include "file_system.h"

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

			FsLogBasicOp = 0xFF,

			FsLogGetInfoByName = 0x0100,
			FsLogSetInfoByName = 0x0200,
			FsLogGetInfoByContext = 0x0400,
			FsLogSetInfoByContext = 0x0800,
			FsLogGetInfoOp = FsLogGetInfoByName | FsLogGetInfoByContext,
			FsLogSetInfoOp = FsLogSetInfoByName | FsLogSetInfoByContext,
			FsLogInfoOp = FsLogGetInfoOp | FsLogSetInfoOp,

			FsLogDelete = 0x1000,
			FsLogMove = 0x2000,
			FsLogList = 0x4000,
			FsLogFileSystemInfo = 0x8000,

			FsLogAllOp = 0xFFFF,

			FsLogTimeInfo = 0x01000000,
			FsLogTimeInfoAsString = 0x02000000 | FsLogTimeInfo,

			//FsLogFileName = 0x10000000,			// when this flag is set, ignores FsLogContextAddress
			//FsLogContextAddress = 0x20000000,

			FsLogRetSuccess = 0x10000000,
			FsLogRetFail = 0x20000000,
			FsLogException = 0x40000000,
			FsLogExceptionString = 0x80000000 | FsLogException,

			FsLogRet = FsLogRetSuccess | FsLogRetFail,
			FsLogSuccess = FsLogRetSuccess,
			FsLogRetAndErrors = FsLogRet | FsLogException,
			FsLogErrors = FsLogRetFail | FsLogExceptionString,
			FsLogDefault = FsLogRet | FsLogException,

			FsLogAll = 0xFFFFFFFF,
		};

	public:
		FsLogger(Ref<FileSystemProvider> base, sl_uint32 logFlags = FsLogAll, String regexFilter = ".*");

	public:
		sl_bool getInformation(FileSystemInfo& outInfo, const FileSystemInfoMask& mask) override;

		sl_bool createDirectory(const StringParam& path) override;

		Ref<FileContext> openFile(const StringParam& path, const FileOpenParam& param) override;

		sl_size	readFile(FileContext* context, sl_uint64 offset, void* buf, sl_size size) override;

		sl_size writeFile(FileContext* context, sl_int64 offset, const void* buf, sl_size size) override;

		sl_bool flushFile(FileContext* context) override;

		sl_bool	closeFile(FileContext* context) override;

		sl_bool deleteDirectory(const StringParam& path) override;

		sl_bool deleteFile(const StringParam& path) override;

		sl_bool moveFile(const StringParam& pathOld, const StringParam& pathNew, sl_bool flagReplaceIfExists) override;

		sl_bool lockFile(FileContext* context, sl_uint64 offset, sl_uint64 length) override;

		sl_bool unlockFile(FileContext* context, sl_uint64 offset, sl_uint64 length) override;

		sl_bool getFileInfo(FileContext* context, FileInfo& outInfo, const FileInfoMask& mask) override;

		sl_bool setFileInfo(FileContext* context, const FileInfo& info, const FileInfoMask& mask) override;

		sl_bool getFileInfo(const StringParam& path, FileInfo& outInfo, const FileInfoMask& mask) override;

		sl_bool setFileInfo(const StringParam& path, const FileInfo& info, const FileInfoMask& mask) override;

		HashMap<String, FileInfo> getFiles(const StringParam& pathDir) override;

	private:
		sl_uint32 m_flags;

		RegEx m_regex;
	};

}
