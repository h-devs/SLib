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

	class FileSystemLogger : public FileSystemWrapper
	{
	public:
		enum FsLogFlags : sl_uint32 {
			FsLogCreate = 0x01,
			FsLogOpen = 0x02,
			FsLogFlush = 0x04,
			FsLogClose = 0x08,
			FsLogRead = 0x10,
			FsLogWrite = 0x20,
			FsLogDelete = 0x40,
			FsLogMove = 0x80,
			FsLogOpenOp = FsLogCreate | FsLogOpen | FsLogClose,
			FsLogCreateOpen = FsLogCreate | FsLogOpen,
			FsLogReadWrite = FsLogRead | FsLogWrite,

			FsLogBasicOp = 0xFF,

			FsLogGetInfo = 0x0100,
			FsLogSetInfo = 0x0200,
			FsLogList = 0x0400,
			FsLogFileSystemInfo = 0x0800,
			FsLogInfoOp = FsLogGetInfo | FsLogSetInfo,

			FsLogAllOp = 0xFFFF,

			FsLogTimeInfo = 0x01000000,
			FsLogTimeInfoAsString = 0x02000000,
			FsLogFileName = 0x04000000,
			FsLogContextAddress = 0x08000000,

			FsLogRetSuccess = 0x10000000,
			FsLogRetFail = 0x20000000,
			FsLogException = 0x40000000,
			FsLogExceptionString = 0x80000000,
			FsLogRet = FsLogRetSuccess | FsLogRetFail,
			FsLogRetAndErrors = FsLogRet | FsLogException,

			FsLogSuccess = FsLogFileName | FsLogRetSuccess,
			FsLogErrors = FsLogFileName | FsLogRetFail | FsLogException | FsLogExceptionString,
			FsLogDefault = FsLogFileName | FsLogRet | FsLogException | FsLogExceptionString,

			FsLogAll = 0xFFFFFFFFu,
		};

	public:
		FileSystemLogger(Ref<FileSystemProvider> base, sl_uint32 logFlags = FsLogAll, String regexFilter = ".*");

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

		sl_bool getFileInfo(const StringParam& path, FileContext* context, FileInfo& outInfo, const FileInfoMask& mask) override;

		sl_bool setFileInfo(const StringParam& path, FileContext* context, const FileInfo& info, const FileInfoMask& mask) override;

		HashMap<String, FileInfo> getFiles(const StringParam& pathDir) override;

	private:
		sl_uint32 m_flags;

		RegEx m_regex;
	};

}
