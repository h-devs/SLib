/**
* @file slib/storage/file_system_logger.h
* FileSystem Logger Wrapper Definition.
*
* @copyright 2020 Steve Han
*/

#pragma once

#include "filesystemwrapper.h"

#define TAG						"FsLogger"
#define printLog(...)			Log(TAG, ##__VA_ARGS__)
#define errorLog(...)			LogError(TAG, ##__VA_ARGS__)
#define debugLog(...)			LogDebug(TAG, ##__VA_ARGS__)

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

#define contextDesc		String::fromUint64(context->handle) + \
					(m_flags & FsLogFileName ? ":" + context->path : \
					(m_flags & FsLogContextAddress ? ":0x" + String::fromUint64((sl_uint64)&context, 16, 8, sl_true) : ""))

	public:
		FsLogger(Ref<FileSystemBase> base, sl_uint32 logFlags = FsLogAll, String regexFilter = ".*")
			: FileSystemWrapper(base), m_flags(logFlags), m_regex(regexFilter)
		{
			if (!(m_flags & FsLogVolumeInfo))
				return;

			printLog("VolumeInfo:");
			printLog("  volumeName: %s", m_volumeInfo.volumeName);
			printLog("  fileSystemName: %s", m_volumeInfo.fileSystemName);
			printLog("  creationTime: %d", m_volumeInfo.creationTime);
			printLog("  serialNumber: %d", m_volumeInfo.serialNumber);
			printLog("  sectorSize: %d", m_volumeInfo.sectorSize);
			printLog("  sectorsPerAllocationUnit: %d", m_volumeInfo.sectorsPerAllocationUnit);
			printLog("  maxComponentLength: %d", m_volumeInfo.maxComponentLength);
			printLog("  fileSystemFlags: 0x%X", m_volumeInfo.fileSystemFlags);
		}
		~FsLogger() {}

	protected:
		const VolumeInfo& fsGetVolumeInfo(VolumeInfoFlags flags = VolumeInfoFlags::BasicInfo)& override 
		{
			if (!(m_flags & FsLogGetVolumeInfo))
				return m_base->fsGetVolumeInfo(flags);
			if (flags & VolumeInfoFlags::BasicInfo &&
				!(m_flags & FsLogGetVolumeBasicInfo))
				return m_base->fsGetVolumeInfo(flags);
			if (flags & VolumeInfoFlags::SizeInfo &&
				!(m_flags & FsLogGetVolumeSizeInfo))
				return m_base->fsGetVolumeInfo(flags);

			String desc = "";
			if (flags & VolumeInfoFlags::BasicInfo)
				desc = ":BasicInfo";
			if (flags & VolumeInfoFlags::SizeInfo)
				desc = ":SizeInfo";
			desc = String::format("GetVolumeInfo(%s)", flags, desc);
			if (!(m_flags & FsLogRetAndErrors))
				printLog(desc);

			try {
				const VolumeInfo &ret = m_base->fsGetVolumeInfo(flags);
				if (m_flags & FsLogRet) {
					printLog(desc);
					if (flags & VolumeInfoFlags::SizeInfo) {
						printLog("  totalSize: %d", ret.totalSize);
						printLog("  freeSize: %d", ret.freeSize);
					}
				}
				return ret;
			}
			catch (FileSystemError error) {
				if (m_flags & FsLogErrors)
					printLog("%s\n  Error: %d", desc, error);
				throw error;
			}
		}

		void fsSetVolumeName(String volumeName) override
		{
			if (!(m_flags & FsLogSetVolumeName))
				return m_base->fsSetVolumeName(volumeName);

			String desc = String::format("SetVolumeName(%s)", volumeName);
			if (!(m_flags & FsLogRetAndErrors))
				printLog(desc);
			try {
				m_base->fsSetVolumeName(volumeName);
			}
			catch (FileSystemError error) {
				if (m_flags & FsLogErrors)
					printLog("%s\n  Error: %d", desc, error);
				throw error;
			}
			if (m_flags & FsLogRet)
				printLog(desc);
		}

		void fsCreate(FileContext* context, FileCreationParams& params = FileCreationParams()) override
		{
			if (!(m_flags & FsLogCreate) || !m_regex.match(context->path))
				return m_base->fsCreate(context, params);

			String desc = String::format("Create(%s,%s,%s%s,0x%X,0x%X,0x%X)", context->path,
				params.attr.isDirectory ? "DIR" : "FILE",
				(params.createAlways ? "ALWAYS" : "NEW"), (params.openTruncate ? "|TRUNCATE" : ""),
				params.accessMode, params.shareMode, params.flagsAndAttributes);
			if (!(m_flags & FsLogRetAndErrors))
				printLog(desc);
			try {
				m_base->fsCreate(context, params);
			}
			catch (FileSystemError error) {
				if (m_flags & FsLogErrors)
					printLog("%s\n  Error: %d", desc, error);
				throw error;
			}
			if (m_flags & FsLogRet) {
				printLog("%s\n"
					"  Handle: %d\n"
					"  Status: %d\n"
					"  DispositionFlags: %s%s", desc, context->handle, context->status,
					(params.createAlways ? "ALWAYS" : "NEW"), (params.openTruncate ? "|TRUNCATE" : ""));
			}
		}

		void fsOpen(FileContext* context, FileCreationParams& params = FileCreationParams()) override
		{
			if (!(m_flags & FsLogOpen) || !m_regex.match(context->path))
				return m_base->fsOpen(context, params);

			String desc = String::format("Open(%s,%s,%s%s,0x%X,0x%X,0x%X)", context->path,
				params.attr.isDirectory ? "DIR" : "FILE",
				(params.createAlways ? "ALWAYS" : "EXISTING"), (params.openTruncate ? "|TRUNCATE" : ""),
				params.accessMode, params.shareMode, params.flagsAndAttributes);
			if (!(m_flags & FsLogRetAndErrors))
				printLog(desc);
			try {
				m_base->fsOpen(context, params);
			}
			catch (FileSystemError error) {
				if (m_flags & FsLogErrors)
					printLog("%s\n  Error: %d", desc, error);
				throw error;
			}
			if (m_flags & FsLogRet) {
				printLog("%s\n"
					"  Handle: %d\n"
					"  Status: %d\n"
					"  DispositionFlags: %s%s", desc, context->handle, context->status,
					(params.createAlways ? "ALWAYS" : "EXISTING"), (params.openTruncate ? "|TRUNCATE" : ""));
			}
		}

		sl_size fsRead(FileContext* context, const Memory& buffer, sl_uint64 offset) override 
		{
			if (!(m_flags & FsLogRead) || !m_regex.match(context->path))
				return m_base->fsRead(context, buffer, offset);

			String desc = String::format("Read(%s,0x%X,0x%X)", contextDesc, offset, buffer.getSize());
			if (!(m_flags & FsLogRetAndErrors))
				printLog(desc);
			sl_size ret;
			try {
				ret = m_base->fsRead(context, buffer, offset);
			}
			catch (FileSystemError error) {
				if (m_flags & FsLogErrors)
					printLog("%s\n  Error: %d", desc, error);
				throw error;
			}
			if (m_flags & FsLogRet)
				printLog("%s\n  Ret: %d", desc, ret);
			return ret;
		}

		sl_size fsWrite(FileContext* context, const Memory& buffer, sl_uint64 offset, sl_bool writeToEof) override
		{
			if (!(m_flags & FsLogWrite) || !m_regex.match(context->path))
				return m_base->fsWrite(context, buffer, offset, writeToEof);

			String desc = String::format("Write(%s,0x%X,0x%X,%d)", contextDesc, offset, buffer.getSize(), writeToEof);
			if (!(m_flags & FsLogRetAndErrors))
				printLog(desc);
			sl_size ret;
			try {
				ret = m_base->fsWrite(context, buffer, offset, writeToEof);
			}
			catch (FileSystemError error) {
				if (m_flags & FsLogErrors)
					printLog("%s\n  Error: %d", desc, error);
				throw error;
			}
			if (m_flags & FsLogRet)
				printLog("%s\n  Ret: %d", desc, ret);
			return ret;
		}

		void fsFlush(FileContext* context) override
		{
			if (!(m_flags & FsLogFlush) || !m_regex.match(context->path))
				return m_base->fsFlush(context);

			String desc = String::format("Flush(%s)", contextDesc);
			if (!(m_flags & FsLogRetAndErrors))
				printLog(desc);
			try {
				m_base->fsFlush(context);
			}
			catch (FileSystemError error) {
				if (m_flags & FsLogErrors)
					printLog("%s\n  Error: %d", desc, error);
				throw error;
			}
			if (m_flags & FsLogRet)
				printLog(desc);
		}

		void fsClose(FileContext* context) override
		{
			if (!(m_flags & FsLogClose) || !m_regex.match(context->path)) {
				m_base->fsClose(context);
				//if (m_flags & FsLogHandleCountOnClose)
				//	printLog("Current open handles count: %d", _FileNames.getCount());
				return;
			}

			String desc = String::format("Close(%s)", contextDesc);
			if (!(m_flags & FsLogRetAndErrors))
				printLog(desc);
			try {
				m_base->fsClose(context);
				//if (m_flags & FsLogHandleCountOnClose)
				//	printLog("Current open handles count: %d", _FileNames.getCount());
			}
			catch (FileSystemError error) {
				if (m_flags & FsLogErrors)
					printLog("%s\n  Error: %d", desc, error);
				throw error;
			}
			if (m_flags & FsLogRet)
				printLog(desc);
		}

		void fsDelete(FileContext* context, sl_bool checkOnly) override
		{
			if (checkOnly) {
				if (!(m_flags & FsLogCanDelete) || !m_regex.match(context->path))
					return m_base->fsDelete(context, checkOnly);
			}
			else {
				if (!(m_flags & FsLogDelete) || !m_regex.match(context->path))
					return m_base->fsDelete(context, checkOnly);
			}

			String desc = String::format("%sDelete(%s)", checkOnly ? "Can" : "", contextDesc);
			if (!(m_flags & FsLogRetAndErrors))
				printLog(desc);
			try {
				m_base->fsDelete(context, checkOnly);
			}
			catch (FileSystemError error) {
				if (m_flags & FsLogErrors)
					printLog("%s\n  Error: %d", desc, error);
				throw error;
			}
			if (m_flags & FsLogRet)
				printLog(desc);
		}

		void fsRename(FileContext* context, String newFileName, sl_bool replaceIfExists) override
		{
			if (!(m_flags & FsLogRename) || !m_regex.match(context->path))
				return m_base->fsRename(context, newFileName, replaceIfExists);

			String desc = String::format("Rename(%s,%s,%d)", contextDesc, newFileName, replaceIfExists);
			if (!(m_flags & FsLogRetAndErrors))
				printLog(desc);
			try {
				m_base->fsRename(context, newFileName, replaceIfExists);
			}
			catch (FileSystemError error) {
				if (m_flags & FsLogErrors)
					printLog("%s\n  Error: %d", desc, error);
				throw error;
			}
			if (m_flags & FsLogRet)
				printLog(desc);
		}

		void fsLock(FileContext* context, sl_uint64 offset, sl_uint64 length) override
		{
			if (!(m_flags & FsLogLock) || !m_regex.match(context->path))
				return m_base->fsLock(context, offset, length);

			String desc = String::format("Lock(%s,0x%X,0x%X)", contextDesc, offset, length);
			if (!(m_flags & FsLogRetAndErrors))
				printLog(desc);
			try {
				m_base->fsLock(context, offset, length);
			}
			catch (FileSystemError error) {
				if (m_flags & FsLogErrors)
					printLog("%s\n  Error: %d", desc, error);
				throw error;
			}
			if (m_flags & FsLogRet)
				printLog(desc);
		}

		void fsUnlock(FileContext* context, sl_uint64 offset, sl_uint64 length) override
		{
			if (!(m_flags & FsLogUnlock) || !m_regex.match(context->path))
				return m_base->fsUnlock(context, offset, length);

			String desc = String::format("Unlock(%s,0x%X,0x%X)", contextDesc, offset, length);
			if (!(m_flags & FsLogRetAndErrors))
				printLog(desc);
			try {
				m_base->fsUnlock(context, offset, length);
			}
			catch (FileSystemError error) {
				if (m_flags & FsLogErrors)
					printLog("%s\n  Error: %d", desc, error);
				throw error;
			}
			if (m_flags & FsLogRet)
				printLog(desc);
		}

		FileInfo fsGetFileInfo(FileContext* context) override
		{
			if (!(m_flags & FsLogGetInfo) || !m_regex.match(context->path))
				return m_base->fsGetFileInfo(context);

			String desc = String::format("GetFileInfo(%s)", contextDesc);
			if (!(m_flags & FsLogRetAndErrors))
				printLog(desc);
			FileInfo ret;
			try {
				ret = m_base->fsGetFileInfo(context);
			}
			catch (FileSystemError error) {
				if (m_flags & FsLogErrors)
					printLog("%s\n  Error: %d", desc, error);
				throw error;
			}
			if (m_flags & FsLogRet)
				printLog("%s\n  Ret: (0x%X,%s,%d,%d,%d,%d,%d)", desc,
					ret.fileAttributes, ret.attr.isDirectory ? "DIR" : "FILE",
					ret.size, ret.allocationSize, ret.createdAt.toInt(), ret.modifiedAt.toInt(), ret.lastAccessedAt.toInt());
			return ret;
		}

		void fsSetFileInfo(FileContext* context, FileInfo fileInfo, FileInfoFlags flags) override
		{
			if (!(m_flags & FsLogSetInfo) || !m_regex.match(context->path))
				return m_base->fsSetFileInfo(context, fileInfo, flags);

			String desc = String::format("SetFileInfo(%s,0x%X)", contextDesc, flags);
			if (m_flags & FsLogSetInfoDetail) {
				if (m_flags & FsLogSetAttrInfo && flags & FileInfoFlags::AttrInfo)
					printLog("  Attr: 0x%X, %s", fileInfo.fileAttributes, fileInfo.attr.isDirectory ? "DIR" : "FILE");
				if (m_flags & FsLogSetTimeInfo && flags & FileInfoFlags::TimeInfo)
					printLog("  Time: %d,%d,%d", fileInfo.createdAt.toInt(), fileInfo.modifiedAt.toInt(), fileInfo.lastAccessedAt.toInt());
				if (m_flags & FsLogSetFileSizeInfo && flags & FileInfoFlags::SizeInfo)
					printLog("  FileSize: %d", fileInfo.size);
				if (m_flags & FsLogSetAllocSizeInfo && flags & FileInfoFlags::AllocSizeInfo)
					printLog("  AllocSize: %d", fileInfo.allocationSize);
			}
			if (!(m_flags & FsLogRetAndErrors))
				printLog(desc);
			try {
				m_base->fsSetFileInfo(context, fileInfo, flags);
			}
			catch (FileSystemError error) {
				if (m_flags & FsLogErrors)
					printLog("%s\n  Error: %d", desc, error);
				throw error;
			}
			if (m_flags & FsLogRet)
				printLog(desc);
		}

		Memory fsGetSecurity(FileContext* context, sl_uint32 securityInformation) override
		{
			if (!(m_flags & FsLogGetSec) || !m_regex.match(context->path))
				return m_base->fsGetSecurity(context, securityInformation);

			String desc = String::format("GetSecurity(%s,0x%X)", contextDesc, securityInformation);
			if (!(m_flags & FsLogRetAndErrors))
				printLog(desc);
			Memory ret;
			try {
				ret = m_base->fsGetSecurity(context, securityInformation);
			}
			catch (FileSystemError error) {
				if (m_flags & FsLogErrors)
					printLog("%s\n  Error: %d", desc, error);
				throw error;
			}
			if (m_flags & FsLogRet)
				printLog("%s\n  Ret: %d", desc, ret.getSize());
			return ret;
		}

		void fsSetSecurity(FileContext* context, sl_uint32 securityInformation, const Memory& securityDescriptor) override
		{
			if (!(m_flags & FsLogSetSec) || !m_regex.match(context->path))
				return m_base->fsSetSecurity(context, securityInformation, securityDescriptor);

			String desc = String::format("SetSecurity(%s,0x%X,%d)", contextDesc, securityInformation, securityDescriptor.getSize());
			if (!(m_flags & FsLogRetAndErrors))
				printLog(desc);
			try {
				m_base->fsSetSecurity(context, securityInformation, securityDescriptor);
			}
			catch (FileSystemError error) {
				if (m_flags & FsLogErrors)
					printLog("%s\n  Error: %d", desc, error);
				throw error;
			}
			if (m_flags & FsLogRet)
				printLog(desc);
		}

		HashMap<String, FileInfo> fsFindFiles(FileContext* context, String pattern) override
		{
			if (!(m_flags & FsLogList) || !m_regex.match(context->path))
				return m_base->fsFindFiles(context, pattern);

			String desc = String::format("FindFiles(%s,%s)", contextDesc, pattern);
			if (!(m_flags & FsLogRetAndErrors))
				printLog(desc);
			HashMap<String, FileInfo> ret;
			try {
				ret = m_base->fsFindFiles(context, pattern);
			}
			catch (FileSystemError error) {
				if (m_flags & FsLogErrors)
					printLog("%s\n  Error: %d", desc, error);
				throw error;
			}
			if (m_flags & FsLogRet) {
				printLog(desc);
				for (auto& file : ret) {
					FileInfo info = file.value;
					printLog("  %s: (0x%X,%s,%d,%d,%d,%d,%d)", file.key,
						info.fileAttributes, info.attr.isDirectory ? "DIR" : "FILE",
						info.size, info.allocationSize, info.createdAt.toInt(), info.modifiedAt.toInt(), info.lastAccessedAt.toInt());
				}
			}
			return ret;
		}

		HashMap<String, StreamInfo> fsFindStreams(FileContext* context) override
		{
			if (!(m_flags & FsLogListStream) || !m_regex.match(context->path))
				return m_base->fsFindStreams(context);

			String desc = String::format("FindStreams(%s)", contextDesc);
			if (!(m_flags & FsLogRetAndErrors))
				printLog(desc);
			HashMap<String, StreamInfo> ret;
			try {
				ret = m_base->fsFindStreams(context);
			}
			catch (FileSystemError error) {
				if (m_flags & FsLogErrors)
					printLog("%s\n  Error: %d", desc, error);
				throw error;
			}
			if (m_flags & FsLogRet) {
				printLog(desc);
				for (auto& stream : ret) {
					auto streamInfo = stream.value;
					printLog("  %s: %d", stream.key, streamInfo.size);
				}
			}
			return ret;
		}

#undef contextDesc

	private:
		sl_uint32 m_flags;
		RegEx m_regex;
	};

}
