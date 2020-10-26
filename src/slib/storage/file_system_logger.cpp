#include "slib/storage/file_system_logger.h"

#include "slib/core/log.h"

#define TAG "FileSystemLogger"
#define LOG(...) SLIB_LOG(TAG, ##__VA_ARGS__)

#define contextDesc		String::fromUint64(context->handle) + \
					(m_flags & FsLogFileName ? ":" + context->path : \
					(m_flags & FsLogContextAddress ? ":0x" + String::fromUint64((sl_uint64)&context, 16, 8, sl_true) : ""))

namespace slib
{

	FsLogger::FsLogger(Ref<FileSystemProvider> base, sl_uint32 logFlags, String regexFilter)
		: FileSystemWrapper(base), m_flags(logFlags), m_regex(regexFilter)
	{
		if (!(m_flags & FsLogVolumeInfo))
			return;

		LOG("LogFlags: 0x%08X", m_flags);

		LOG("FileSystemInfo:");
		LOG("  volumeName: %s", m_volumeInfo.volumeName);
		LOG("  fileSystemName: %s", m_volumeInfo.fileSystemName);
		LOG("  creationTime: %s", m_flags & FsLogDateAsString
			? m_volumeInfo.creationTime.toString() 
			: String::fromInt64(m_volumeInfo.creationTime.toInt()));
		LOG("  serialNumber: %d", m_volumeInfo.serialNumber);
		LOG("  sectorSize: %d", m_volumeInfo.sectorSize);
		LOG("  sectorsPerAllocationUnit: %d", m_volumeInfo.sectorsPerAllocationUnit);
		LOG("  maxPathLength: %d", m_volumeInfo.maxPathLength);
		LOG("  flags: 0x%X", m_volumeInfo.flags.value);
	}

	FsLogger::~FsLogger()
	{
	}

	const FileSystemInfo& FsLogger::getInformation(VolumeInfoFlags flags)& 
	{
		if (!(m_flags & FsLogGetVolumeInfo))
			return m_base->getInformation(flags);
		if (flags & VolumeInfoFlags::BasicInfo &&
			!(m_flags & FsLogGetVolumeBasicInfo))
			return m_base->getInformation(flags);
		if (flags & VolumeInfoFlags::SizeInfo &&
			!(m_flags & FsLogGetVolumeSizeInfo))
			return m_base->getInformation(flags);

		String desc = "";
		if (flags & VolumeInfoFlags::BasicInfo)
			desc = ":BasicInfo";
		if (flags & VolumeInfoFlags::SizeInfo)
			desc = ":SizeInfo";
		desc = String::format("GetVolumeInfo(%s)", flags, desc);
		if (!(m_flags & FsLogRetAndErrors))
			LOG(desc);

		try {
			const FileSystemInfo &ret = m_base->getInformation(flags);
			if (m_flags & FsLogRet) {
				LOG(desc);
			}
			return ret;
		}
		catch (FileSystemError error) {
			if (m_flags & FsLogErrors)
				LOG("%s\n  Error: %d", desc, error);
			throw error;
		}
	}

	void FsLogger::openFile(FileContext* context, FileCreationParams& params)
	{
		if (!(m_flags & FsLogCreate) || !m_regex.match(context->path))
			return m_base->openFile(context, params);

		String desc = String::format("Create(%s,%s,%s%s,0x%X,0x%X,0x%X)", context->path,
			params.attributes.isDirectory ? "DIR" : "FILE",
			(params.createAlways ? "ALWAYS" : "NEW"), (params.openTruncate ? "|TRUNCATE" : ""),
			params.accessMode, params.shareMode, params.flagsAndAttributes);
		if (!(m_flags & FsLogRetAndErrors))
			LOG(desc);
		try {
			m_base->openFile(context, params);
		}
		catch (FileSystemError error) {
			if (m_flags & FsLogErrors)
				LOG("%s\n  Error: %d", desc, error);
			throw error;
		}
		if (m_flags & FsLogRet) {
			LOG("%s\n"
				"  Handle: %d\n"
				"  Status: %d\n"
				"  DispositionFlags: %s%s", desc, context->handle, context->status,
				(params.createAlways ? "ALWAYS" : "NEW"), (params.openTruncate ? "|TRUNCATE" : ""));
		}
	}

	void FsLogger::openFile(FileContext* context, FileCreationParams& params)
	{
		if (!(m_flags & FsLogOpen) || !m_regex.match(context->path))
			return m_base->openFile(context, params);

		String desc = String::format("Open(%s,%s,%s%s,0x%X,0x%X,0x%X)", context->path,
			params.attributes.isDirectory ? "DIR" : "FILE",
			(params.createAlways ? "ALWAYS" : "EXISTING"), (params.openTruncate ? "|TRUNCATE" : ""),
			params.accessMode, params.shareMode, params.flagsAndAttributes);
		if (!(m_flags & FsLogRetAndErrors))
			LOG(desc);
		try {
			m_base->openFile(context, params);
		}
		catch (FileSystemError error) {
			if (m_flags & FsLogErrors)
				LOG("%s\n  Error: %d", desc, error);
			throw error;
		}
		if (m_flags & FsLogRet) {
			LOG("%s\n"
				"  Handle: %d\n"
				"  Status: %d\n"
				"  DispositionFlags: %s%s", desc, context->handle, context->status,
				(params.createAlways ? "ALWAYS" : "EXISTING"), (params.openTruncate ? "|TRUNCATE" : ""));
		}
	}

	sl_size FsLogger::readFile(FileContext* context, const Memory& buffer, sl_uint64 offset) 
	{
		if (!(m_flags & FsLogRead) || !m_regex.match(context->path))
			return m_base->readFile(context, buffer, offset);

		String desc = String::format("Read(%s,0x%X,0x%X)", contextDesc, offset, buffer.getSize());
		if (!(m_flags & FsLogRetAndErrors))
			LOG(desc);
		sl_size ret;
		try {
			ret = m_base->readFile(context, buffer, offset);
		}
		catch (FileSystemError error) {
			if (m_flags & FsLogErrors)
				LOG("%s\n  Error: %d", desc, error);
			throw error;
		}
		if (m_flags & FsLogRet)
			LOG("%s\n  Ret: %d", desc, ret);
		return ret;
	}

	sl_size FsLogger::writeFile(FileContext* context, const Memory& buffer, sl_uint64 offset, sl_bool writeToEof)
	{
		if (!(m_flags & FsLogWrite) || !m_regex.match(context->path))
			return m_base->writeFile(context, buffer, offset, writeToEof);

		String desc = String::format("Write(%s,0x%X,0x%X,%d)", contextDesc, offset, buffer.getSize(), writeToEof);
		if (!(m_flags & FsLogRetAndErrors))
			LOG(desc);
		sl_size ret;
		try {
			ret = m_base->writeFile(context, buffer, offset, writeToEof);
		}
		catch (FileSystemError error) {
			if (m_flags & FsLogErrors)
				LOG("%s\n  Error: %d", desc, error);
			throw error;
		}
		if (m_flags & FsLogRet)
			LOG("%s\n  Ret: %d", desc, ret);
		return ret;
	}

	void FsLogger::flush(FileContext* context)
	{
		if (!(m_flags & FsLogFlush) || !m_regex.match(context->path))
			return m_base->flush(context);

		String desc = String::format("Flush(%s)", contextDesc);
		if (!(m_flags & FsLogRetAndErrors))
			LOG(desc);
		try {
			m_base->flush(context);
		}
		catch (FileSystemError error) {
			if (m_flags & FsLogErrors)
				LOG("%s\n  Error: %d", desc, error);
			throw error;
		}
		if (m_flags & FsLogRet)
			LOG(desc);
	}

	void FsLogger::closeFile(FileContext* context)
	{
		if (!(m_flags & FsLogClose) || !m_regex.match(context->path)) {
			m_base->closeFile(context);
			if (m_flags & FsLogHandleCountOnClose)
				LOG("Current open handles count: %d", m_openHandles[context->path]);
			return;
		}

		String desc = String::format("Close(%s)", contextDesc);
		if (!(m_flags & FsLogRetAndErrors))
			LOG(desc);
		try {
			m_base->closeFile(context);
			if (m_flags & FsLogHandleCountOnClose)
				LOG("Current open handles count: %d", m_openHandles[context->path]);
		}
		catch (FileSystemError error) {
			if (m_flags & FsLogErrors)
				LOG("%s\n  Error: %d", desc, error);
			throw error;
		}
		if (m_flags & FsLogRet)
			LOG(desc);
	}

	void FsLogger::deleteFile(FileContext* context, sl_bool checkOnly)
	{
		if (checkOnly) {
			if (!(m_flags & FsLogCanDelete) || !m_regex.match(context->path))
				return m_base->deleteFile(context, checkOnly);
		}
		else {
			if (!(m_flags & FsLogDelete) || !m_regex.match(context->path))
				return m_base->deleteFile(context, checkOnly);
		}

		String desc = String::format("%sDelete(%s)", checkOnly ? "Can" : "", contextDesc);
		if (!(m_flags & FsLogRetAndErrors))
			LOG(desc);
		try {
			m_base->deleteFile(context, checkOnly);
		}
		catch (FileSystemError error) {
			if (m_flags & FsLogErrors)
				LOG("%s\n  Error: %d", desc, error);
			throw error;
		}
		if (m_flags & FsLogRet)
			LOG(desc);
	}

	void FsLogger::moveFile(FileContext* context, String newFileName, sl_bool replaceIfExists)
	{
		if (!(m_flags & FsLogRename) || !m_regex.match(context->path))
			return m_base->moveFile(context, newFileName, replaceIfExists);

		String desc = String::format("Rename(%s,%s,%d)", contextDesc, newFileName, replaceIfExists);
		if (!(m_flags & FsLogRetAndErrors))
			LOG(desc);
		try {
			m_base->moveFile(context, newFileName, replaceIfExists);
		}
		catch (FileSystemError error) {
			if (m_flags & FsLogErrors)
				LOG("%s\n  Error: %d", desc, error);
			throw error;
		}
		if (m_flags & FsLogRet)
			LOG(desc);
	}

	void FsLogger::lockFile(FileContext* context, sl_uint64 offset, sl_uint64 length)
	{
		if (!(m_flags & FsLogLock) || !m_regex.match(context->path))
			return m_base->lockFile(context, offset, length);

		String desc = String::format("Lock(%s,0x%X,0x%X)", contextDesc, offset, length);
		if (!(m_flags & FsLogRetAndErrors))
			LOG(desc);
		try {
			m_base->lockFile(context, offset, length);
		}
		catch (FileSystemError error) {
			if (m_flags & FsLogErrors)
				LOG("%s\n  Error: %d", desc, error);
			throw error;
		}
		if (m_flags & FsLogRet)
			LOG(desc);
	}

	void FsLogger::unlockFile(FileContext* context, sl_uint64 offset, sl_uint64 length)
	{
		if (!(m_flags & FsLogUnlock) || !m_regex.match(context->path))
			return m_base->unlockFile(context, offset, length);

		String desc = String::format("Unlock(%s,0x%X,0x%X)", contextDesc, offset, length);
		if (!(m_flags & FsLogRetAndErrors))
			LOG(desc);
		try {
			m_base->unlockFile(context, offset, length);
		}
		catch (FileSystemError error) {
			if (m_flags & FsLogErrors)
				LOG("%s\n  Error: %d", desc, error);
			throw error;
		}
		if (m_flags & FsLogRet)
			LOG(desc);
	}

	FileInfo FsLogger::getFileInfo(FileContext* context)
	{
		if (!(m_flags & FsLogGetInfo) || !m_regex.match(context->path))
			return m_base->getFileInfo(context);

		String desc = String::format("GetFileInfo(%s)", contextDesc);
		if (!(m_flags & FsLogRetAndErrors))
			LOG(desc);
		FileInfo ret;
		try {
			ret = m_base->getFileInfo(context);
		}
		catch (FileSystemError error) {
			if (m_flags & FsLogErrors)
				LOG("%s\n  Error: %d", desc, error);
			throw error;
		}
		if (m_flags & FsLogRet)
			LOG("%s\n  Ret: (0x%X,%s,%d,%d%s)", desc,
				ret.fileAttributes, ret.attributes.isDirectory ? "DIR" : "FILE",
				ret.size, ret.allocSize,
				(m_flags & FsLogAttrNoDate ? "" : "," 
					+ (m_flags & FsLogDateAsString 
						? String::format("%s,%s,%s", ret.createdAt.toString(), ret.modifiedAt.toString(), ret.lastAccessedAt.toString())
						: String::format("%d,%d,%d", ret.createdAt.toInt(), ret.modifiedAt.toInt(), ret.lastAccessedAt.toInt()))));
		return ret;
	}

	void FsLogger::setFileInfo(FileContext* context, FileInfo fileInfo, FileInfoFlags flags)
	{
		if (!(m_flags & FsLogSetInfo) || !m_regex.match(context->path))
			return m_base->setFileInfo(context, fileInfo, flags);

		String desc = String::format("SetFileInfo(%s,0x%X)", contextDesc, flags);
		if (m_flags & FsLogSetInfoDetail) {
			if (m_flags & FsLogSetAttrInfo && flags & FileInfoFlags::AttrInfo) {
				LOG("  Attr: 0x%X, %s", fileInfo.fileAttributes, fileInfo.attributes.isDirectory ? "DIR" : "FILE");
			}
			if (m_flags & FsLogSetTimeInfo && flags & FileInfoFlags::TimeInfo) {
				if (m_flags & FsLogDateAsString)
					LOG("  Time: %s,%s,%s", fileInfo.createdAt.toString(), fileInfo.modifiedAt.toString(), fileInfo.lastAccessedAt.toString());
				else
					LOG("  Time: %d,%d,%d", fileInfo.createdAt.toInt(), fileInfo.modifiedAt.toInt(), fileInfo.lastAccessedAt.toInt());
			}
			if (m_flags & FsLogSetFileSizeInfo && flags & FileInfoFlags::SizeInfo) {
				LOG("  FileSize: %d", fileInfo.size);
			}
			if (m_flags & FsLogSetAllocSizeInfo && flags & FileInfoFlags::AllocSizeInfo) {
				LOG("  AllocSize: %d", fileInfo.allocSize);
			}
		}
		if (!(m_flags & FsLogRetAndErrors))
			LOG(desc);
		try {
			m_base->setFileInfo(context, fileInfo, flags);
		}
		catch (FileSystemError error) {
			if (m_flags & FsLogErrors)
				LOG("%s\n  Error: %d", desc, error);
			throw error;
		}
		if (m_flags & FsLogRet)
			LOG(desc);
	}

	sl_size FsLogger::fsGetSecurity(FileContext* context, sl_uint32 securityInformation, const Memory& securityDescriptor)
	{
		if (!(m_flags & FsLogGetSec) || !m_regex.match(context->path))
			return m_base->fsGetSecurity(context, securityInformation, securityDescriptor);

		String desc = String::format("GetSecurity(%s,0x%X,%d)", contextDesc, securityInformation, securityDescriptor.getSize());
		if (!(m_flags & FsLogRetAndErrors))
			LOG(desc);
		sl_size ret;
		try {
			ret = m_base->fsGetSecurity(context, securityInformation, securityDescriptor);
		}
		catch (FileSystemError error) {
			if (m_flags & FsLogErrors)
				LOG("%s\n  Error: %d", desc, error);
			throw error;
		}
		if (m_flags & FsLogRet)
			LOG("%s\n  Ret: %d\n  Status: %d", desc, ret, context->status);
		return ret;
	}

	void FsLogger::fsSetSecurity(FileContext* context, sl_uint32 securityInformation, const Memory& securityDescriptor)
	{
		if (!(m_flags & FsLogSetSec) || !m_regex.match(context->path))
			return m_base->fsSetSecurity(context, securityInformation, securityDescriptor);

		String desc = String::format("SetSecurity(%s,0x%X,%d)", contextDesc, securityInformation, securityDescriptor.getSize());
		if (!(m_flags & FsLogRetAndErrors))
			LOG(desc);
		try {
			m_base->fsSetSecurity(context, securityInformation, securityDescriptor);
		}
		catch (FileSystemError error) {
			if (m_flags & FsLogErrors)
				LOG("%s\n  Error: %d", desc, error);
			throw error;
		}
		if (m_flags & FsLogRet)
			LOG(desc);
	}

	HashMap<String, FileInfo> FsLogger::getFiles(FileContext* context, String pattern)
	{
		if (!(m_flags & FsLogList) || !m_regex.match(context->path))
			return m_base->getFiles(context, pattern);

		String desc = String::format("FindFiles(%s,%s)", contextDesc, pattern);
		if (!(m_flags & FsLogRetAndErrors))
			LOG(desc);
		HashMap<String, FileInfo> ret;
		try {
			ret = m_base->getFiles(context, pattern);
		}
		catch (FileSystemError error) {
			if (m_flags & FsLogErrors)
				LOG("%s\n  Error: %d", desc, error);
			throw error;
		}
		if (m_flags & FsLogRet) {
			LOG(desc);
			for (auto& file : ret) {
				FileInfo info = file.value;
				LOG("  %s: (0x%X,%s,%d,%d%s)", file.key,
					info.fileAttributes, info.attributes.isDirectory ? "DIR" : "FILE",
					info.size, info.allocSize,
					(m_flags & FsLogAttrNoDate ? "" : ","
						+ (m_flags & FsLogDateAsString
							? String::format("%s,%s,%s", info.createdAt.toString(), info.modifiedAt.toString(), info.lastAccessedAt.toString())
							: String::format("%d,%d,%d", info.createdAt.toInt(), info.modifiedAt.toInt(), info.lastAccessedAt.toInt()))));
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
			LOG(desc);
		HashMap<String, StreamInfo> ret;
		try {
			ret = m_base->fsFindStreams(context);
		}
		catch (FileSystemError error) {
			if (m_flags & FsLogErrors)
				LOG("%s\n  Error: %d", desc, error);
			throw error;
		}
		if (m_flags & FsLogRet) {
			LOG(desc);
			for (auto& stream : ret) {
				auto streamInfo = stream.value;
				LOG("  %s: %d", stream.key, streamInfo.size);
			}
		}
		return ret;
	}

}
