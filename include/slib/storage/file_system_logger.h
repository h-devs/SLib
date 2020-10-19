/**
* @file slib/storage/file_system_logger.h
* FileSystem Logger Wrapper Definition.
*
* @copyright 2020 Steve Han
*/

#pragma once

#include "filesystemwrapper.h"

namespace slib
{

	class FsLogger : public FileSystemWrapper
	{
	public:
		enum FsLogFlags {
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
			//FsLogAttrNoDate = 0x400000,
			//FsLogDateConversion = 0x800000,

			FsLogVolumeInfo = 0x01000000,
			FsLogGetVolumeBasicInfo = 0x02000000,
			FsLogGetVolumeSizeInfo = 0x04000000,
			FsLogSetVolumeName = 0x08000000,
			FsLogGetVolumeInfo = FsLogGetVolumeBasicInfo | FsLogGetVolumeSizeInfo,
			FsLogVolumeOp = 0x0F000000,

			FsLogFileName = 0x10000000,			// when this flag is set, ignores FsLogContextAddress
			FsLogContextAddress = 0x20000000,
			FsLogRet = 0x40000000,
			FsLogErrors = 0x80000000,
			FsLogRetAndErrors = FsLogRet | FsLogErrors,
			FsLogDefault = FsLogFileName | FsLogRet | FsLogErrors,
			FsLogDefaultErrors = FsLogFileName | FsLogErrors,

			FsLogAll = 0xFFFFFFFF,
		};

	public:
		FsLogger(Ref<FileSystemBase> base, sl_uint32 logFlags = FsLogAll, String regexFilter = ".*");

		~FsLogger();

	protected:
		const VolumeInfo& fsGetVolumeInfo(VolumeInfoFlags flags = VolumeInfoFlags::BasicInfo)& override;

		void fsSetVolumeName(String volumeName) override;

		void fsCreate(FileContext* context, FileCreationParams& params = FileCreationParams()) override;

		void fsOpen(FileContext* context, FileCreationParams& params = FileCreationParams()) override;

		sl_size fsRead(FileContext* context, const Memory& buffer, sl_uint64 offset) override;

		sl_size fsWrite(FileContext* context, const Memory& buffer, sl_uint64 offset, sl_bool writeToEof) override;

		void fsFlush(FileContext* context) override;

		void fsClose(FileContext* context) override;

		void fsDelete(FileContext* context, sl_bool checkOnly) override;

		void fsRename(FileContext* context, String newFileName, sl_bool replaceIfExists) override;

		void fsLock(FileContext* context, sl_uint64 offset, sl_uint64 length) override;

		void fsUnlock(FileContext* context, sl_uint64 offset, sl_uint64 length) override;

		FileInfo fsGetFileInfo(FileContext* context) override;

		void fsSetFileInfo(FileContext* context, FileInfo fileInfo, FileInfoFlags flags) override;

		Memory fsGetSecurity(FileContext* context, sl_uint32 securityInformation) override;

		void fsSetSecurity(FileContext* context, sl_uint32 securityInformation, const Memory& securityDescriptor) override;

		HashMap<String, FileInfo> fsFindFiles(FileContext* context, String pattern) override;

		HashMap<String, StreamInfo> fsFindStreams(FileContext* context) override;

	private:
		sl_uint32 m_flags;
		RegEx m_regex;
	};

}
