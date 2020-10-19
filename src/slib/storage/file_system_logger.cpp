#include "slib/storage/file_system_logger.h"

#define TAG						"FsLogger"
#define printLog(...)			Log(TAG, ##__VA_ARGS__)
#define errorLog(...)			LogError(TAG, ##__VA_ARGS__)
#define debugLog(...)			LogDebug(TAG, ##__VA_ARGS__)

#define contextDesc		String::fromUint64(context->handle) + \
					(m_flags & FsLogFileName ? ":" + context->path : \
					(m_flags & FsLogContextAddress ? ":0x" + String::fromUint64((sl_uint64)&context, 16, 8, sl_true) : ""))

namespace slib
{

	FsLogger::FsLogger(Ref<FileSystemBase> base, sl_uint32 logFlags, String regexFilter)
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

	FsLogger::~FsLogger()
	{
	}

	const VolumeInfo& FsLogger::fsGetVolumeInfo(VolumeInfoFlags flags)& 
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

	void FsLogger::fsSetVolumeName(String volumeName)
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

	void FsLogger::fsCreate(FileContext* context, FileCreationParams& params)
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

	void FsLogger::fsOpen(FileContext* context, FileCreationParams& params)
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

	sl_size FsLogger::fsRead(FileContext* context, const Memory& buffer, sl_uint64 offset) 
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

	sl_size FsLogger::fsWrite(FileContext* context, const Memory& buffer, sl_uint64 offset, sl_bool writeToEof)
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

	void FsLogger::fsFlush(FileContext* context)
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

	void FsLogger::fsClose(FileContext* context)
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

	void FsLogger::fsDelete(FileContext* context, sl_bool checkOnly)
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

	void FsLogger::fsRename(FileContext* context, String newFileName, sl_bool replaceIfExists)
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

	void FsLogger::fsLock(FileContext* context, sl_uint64 offset, sl_uint64 length)
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

	void FsLogger::fsUnlock(FileContext* context, sl_uint64 offset, sl_uint64 length)
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

	FileInfo FsLogger::fsGetFileInfo(FileContext* context)
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

	void FsLogger::fsSetFileInfo(FileContext* context, FileInfo fileInfo, FileInfoFlags flags)
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

	Memory FsLogger::fsGetSecurity(FileContext* context, sl_uint32 securityInformation)
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

	void FsLogger::fsSetSecurity(FileContext* context, sl_uint32 securityInformation, const Memory& securityDescriptor)
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

	HashMap<String, FileInfo> FsLogger::fsFindFiles(FileContext* context, String pattern)
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

	HashMap<String, StreamInfo> FsLogger::fsFindStreams(FileContext* context)
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

}
