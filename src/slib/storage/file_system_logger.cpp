#include "slib/storage/file_system_logger.h"

#include "slib/core/system.h"

#include "slib/core/log.h"

#define TAG "FileSystemLogger"
#define LOG(...) SLIB_LOG(TAG, ##__VA_ARGS__)

//#define contextDesc		String::fromUint64(context->handle) + \
//						(m_flags & FsLogFileName ? ":" + context->path : \
//						(m_flags & FsLogContextAddress ? ":0x" + String::fromUint64((sl_uint64)&context, 16, 8, sl_true) : ""))
#define contextDesc		"0x" + String::fromUint64((sl_uint64)&context, 16, 8, sl_true)

namespace slib
{

	FsLogger::FsLogger(Ref<FileSystemProvider> base, sl_uint32 logFlags, String regexFilter)
		: FileSystemWrapper(base), m_flags(logFlags), m_regex(regexFilter)
	{
		LOG("LogFlags: 0x%08X", m_flags);
	}

	sl_bool FsLogger::getInformation(FileSystemInfo& info, const FileSystemInfoMask& mask)
	{
		if (!(m_flags & FsLogGetVolumeInfo))
			return m_base->getInformation(info, mask);

		if (mask & FileSystemInfoMask::Basic &&
			!(m_flags & FsLogGetVolumeInfo))
			return m_base->getInformation(info, mask);

		if (mask & FileSystemInfoMask::Size &&
			!(m_flags & FsLogGetVolumeSize))
			return m_base->getInformation(info, mask);

		String desc = "";
		if (mask & FileSystemInfoMask::Basic)
			desc = ":BasicInfo";
		if (mask & FileSystemInfoMask::Size)
			desc = ":SizeInfo";
		desc = String::format("GetVolumeInfo(%s)", mask, desc);
		if (!(m_flags & FsLogRetAndErrors))
			LOG(desc);

		try {
			sl_bool ret = m_base->getInformation(info, mask);
			if (m_flags & FsLogRet) {
				LOG("%s\n  Ret: %d", desc, ret);
				if (mask & FileSystemInfoMask::Basic) {
					LOG("  volumeName: %s", info.volumeName);
					LOG("  fileSystemName: %s", info.fileSystemName);
					LOG("  creationTime: %s", m_flags & FsLogDateAsString
						? info.creationTime.toString()
						: String::fromInt64(info.creationTime.toInt()));
					LOG("  serialNumber: %d", info.serialNumber);
					LOG("  sectorSize: %d", info.sectorSize);
					LOG("  sectorsPerAllocationUnit: %d", info.sectorsPerAllocationUnit);
					LOG("  maxPathLength: %d", info.maxPathLength);
					LOG("  flags: 0x%X", info.flags.value);
				}
				if (mask & FileSystemInfoMask::Size) {
					LOG("  totalSize: %d", info.totalSize);
					LOG("  freeSize: %d", info.freeSize);
				}
			}
			return ret;
		}
		catch (FileSystemError error) {
			if (m_flags & FsLogErrors)
				LOG("%s\n  Error: %d, %s", desc, error, System::formatErrorCode((sl_uint32)error));
			throw error;
		}
	}

	Ref<FileContext> FsLogger::openFile(const StringParam& path, const FileOpenParam& param)
	{
		if (!(m_flags & FsLogOpen) || !m_regex.match(path.toString()))
			return m_base->openFile(path, param);

		String desc = String::format("Open(%s,%s,%s%s,0x%X,0x%X)", path,
			param.attributes & FileAttributes::Directory ? "DIR" : "FILE",
			(param.mode & FileMode::NotCreate ? "NOT_CREATE" : "CREATE"),
			(param.mode & FileMode::NotTruncate ? "" : "|TRUNCATE"),
			param.mode.value, param.attributes.value);
		if (!(m_flags & FsLogRetAndErrors))
			LOG(desc);

		try {
			auto ret = m_base->openFile(path, param);
			if (m_flags & FsLogRet)
				LOG("%s\n  Ret: %d", desc, ret);
			return ret;
		}
		catch (FileSystemError error) {
			if (m_flags & FsLogErrors)
				LOG("%s\n  Error: %d, %s", desc, error, System::formatErrorCode((sl_uint32)error));
			throw error;
		}
	}

	sl_size FsLogger::readFile(FileContext* context, sl_uint64 offset, void* buf, sl_size size)
	{
		if (!(m_flags & FsLogRead)/* || !m_regex.match(context->path)*/)
			return m_base->readFile(context, offset, buf, size);

		String desc = String::format("Read(%s,0x%X,0x%X)", contextDesc, offset, size);
		if (!(m_flags & FsLogRetAndErrors))
			LOG(desc);

		try {
			sl_size ret = m_base->readFile(context, offset, buf, size);
			if (m_flags & FsLogRet)
				LOG("%s\n  Ret: %d", desc, ret);
			return ret;
		}
		catch (FileSystemError error) {
			if (m_flags & FsLogErrors)
				LOG("%s\n  Error: %d, %s", desc, error, System::formatErrorCode((sl_uint32)error));
			throw error;
		}
	}

	sl_size FsLogger::writeFile(FileContext* context, sl_int64 offset, const void* buf, sl_size size)
	{
		if (!(m_flags & FsLogWrite)/* || !m_regex.match(context->path)*/)
			return m_base->writeFile(context, offset, buf, size);

		String desc = String::format("Write(%s,0x%X,0x%X)", contextDesc, offset, size);
		if (!(m_flags & FsLogRetAndErrors))
			LOG(desc);

		try {
			sl_size ret = m_base->writeFile(context, offset, buf, size);
			if (m_flags & FsLogRet)
				LOG("%s\n  Ret: %d", desc, ret);
			return ret;
		}
		catch (FileSystemError error) {
			if (m_flags & FsLogErrors)
				LOG("%s\n  Error: %d, %s", desc, error, System::formatErrorCode((sl_uint32)error));
			throw error;
		}
	}

	sl_bool FsLogger::flushFile(FileContext* context)
	{
		if (!(m_flags & FsLogFlush)/* || !m_regex.match(context->path)*/)
			return m_base->flushFile(context);

		String desc = String::format("Flush(%s)", contextDesc);
		if (!(m_flags & FsLogRetAndErrors))
			LOG(desc);

		try {
			sl_bool ret = m_base->flushFile(context);
			if (m_flags & FsLogRet)
				LOG("%s\n  Ret: %d", desc, ret);
			return ret;
		}
		catch (FileSystemError error) {
			if (m_flags & FsLogErrors)
				LOG("%s\n  Error: %d, %s", desc, error, System::formatErrorCode((sl_uint32)error));
			throw error;
		}
	}

	sl_bool FsLogger::closeFile(FileContext* context)
	{
		if (!(m_flags & FsLogClose)/* || !m_regex.match(context->path)*/) {
			m_base->closeFile(context);
			//if (m_flags & FsLogHandleCountOnClose)
			//	LOG("Current open handles count: %d", m_openHandles[context->path]);
			return;
		}

		String desc = String::format("Close(%s)", contextDesc);
		if (!(m_flags & FsLogRetAndErrors))
			LOG(desc);

		try {
			sl_bool ret = m_base->closeFile(context);
			if (m_flags & FsLogRet)
				LOG("%s\n  Ret: %d", desc, ret);
			//if (m_flags & FsLogHandleCountOnClose)
			//	LOG("Current open handles count: %d", m_openHandles[context->path]);
			return ret;
		}
		catch (FileSystemError error) {
			if (m_flags & FsLogErrors)
				LOG("%s\n  Error: %d, %s", desc, error, System::formatErrorCode((sl_uint32)error));
			throw error;
		}
	}

	sl_bool FsLogger::deleteFile(const StringParam& path)
	{
		if (!(m_flags & FsLogDelete) || !m_regex.match(path.toString()))
			return m_base->deleteFile(path);
	
		String desc = String::format("Delete(%s)", path);
		if (!(m_flags & FsLogRetAndErrors))
			LOG(desc);

		try {
			sl_bool ret = m_base->deleteFile(path);
			if (m_flags & FsLogRet)
				LOG("%s\n  Ret: %d", desc, ret);
			return ret;
		}
		catch (FileSystemError error) {
			if (m_flags & FsLogErrors)
				LOG("%s\n  Error: %d, %s", desc, error, System::formatErrorCode((sl_uint32)error));
			throw error;
		}
	}

	sl_bool FsLogger::moveFile(const StringParam& pathOld, const StringParam& pathNew, sl_bool flagReplaceIfExists)
	{
		if (!(m_flags & FsLogRename) || !m_regex.match(pathOld.toString()))
			return m_base->moveFile(pathOld, pathNew, flagReplaceIfExists);

		String desc = String::format("Move(%s,%s,%d)", pathOld, pathNew, flagReplaceIfExists);
		if (!(m_flags & FsLogRetAndErrors))
			LOG(desc);

		try {
			sl_bool ret = m_base->moveFile(pathOld, pathNew, flagReplaceIfExists);
			if (m_flags & FsLogRet)
				LOG("%s\n  Ret: %d", desc, ret);
			return ret;
		}
		catch (FileSystemError error) {
			if (m_flags & FsLogErrors)
				LOG("%s\n  Error: %d, %s", desc, error, System::formatErrorCode((sl_uint32)error));
			throw error;
		}
	}

	sl_bool FsLogger::lockFile(FileContext* context, sl_uint64 offset, sl_uint64 length)
	{
		if (!(m_flags & FsLogLock)/* || !m_regex.match(context->path)*/)
			return m_base->lockFile(context, offset, length);

		String desc = String::format("Lock(%s,0x%X,0x%X)", contextDesc, offset, length);
		if (!(m_flags & FsLogRetAndErrors))
			LOG(desc);

		try {
			sl_bool ret = m_base->lockFile(context, offset, length);
			if (m_flags & FsLogRet)
				LOG("%s\n  Ret: %d", desc, ret);
			return ret;
		}
		catch (FileSystemError error) {
			if (m_flags & FsLogErrors)
				LOG("%s\n  Error: %d, %s", desc, error, System::formatErrorCode((sl_uint32)error));
			throw error;
		}
	}

	sl_bool FsLogger::unlockFile(FileContext* context, sl_uint64 offset, sl_uint64 length)
	{
		if (!(m_flags & FsLogUnlock)/* || !m_regex.match(context->path)*/)
			return m_base->unlockFile(context, offset, length);

		String desc = String::format("Unlock(%s,0x%X,0x%X)", contextDesc, offset, length);
		if (!(m_flags & FsLogRetAndErrors))
			LOG(desc);

		try {
			sl_bool ret = m_base->unlockFile(context, offset, length);
			if (m_flags & FsLogRet)
				LOG("%s\n  Ret: %d", desc, ret);
			return ret;
		}
		catch (FileSystemError error) {
			if (m_flags & FsLogErrors)
				LOG("%s\n  Error: %d, %s", desc, error, System::formatErrorCode((sl_uint32)error));
			throw error;
		}
	}

	sl_bool FsLogger::getFileInfo(const StringParam& path, FileInfo& info, const FileInfoMask& mask)
	{
		if (!(m_flags & FsLogGetInfo) || !m_regex.match(path.toString()))
			return m_base->getFileInfo(path, info, mask);

		String desc = String::format("GetFileInfo(%s,0x%X)", path, mask);
		if (!(m_flags & FsLogRetAndErrors))
			LOG(desc);

		try {
			sl_bool ret = m_base->getFileInfo(path, info, mask);
			if (m_flags & FsLogRet) {
				LOG("%s\n  Ret: %d", desc, ret);
				if (mask & FileInfoMask::Attributes)
					LOG("  Attributes: 0x%X, %d", info.attributes, info.attributes & FileAttributes::Directory ? "DIR" : "FILE");
				if (mask & FileInfoMask::Size)
					LOG("  Size: %d", info.size);
				if (mask & FileInfoMask::AllocSize)
					LOG("  AllocSize: %d", info.allocSize);
				if (!(m_flags & FsLogAttrNoDate) &&
					(mask & FileInfoMask::Time)) {
					if (m_flags & FsLogDateAsString) {
						LOG("  CreatedAt: %s", info.createdAt.toString());
						LOG("  ModifiedAt: %s", info.modifiedAt.toString());
						LOG("  AccessedAt: %s", info.lastAccessedAt.toString());
					}
					else {
						LOG("  CreatedAt: %d", info.createdAt.toInt());
						LOG("  ModifiedAt: %d", info.modifiedAt.toInt());
						LOG("  AccessedAt: %d", info.lastAccessedAt.toInt());
					}
				}
			}
			return ret;
		}
		catch (FileSystemError error) {
			if (m_flags & FsLogErrors)
				LOG("%s\n  Error: %d, %s", desc, error, System::formatErrorCode((sl_uint32)error));
			throw error;
		}
	}

	sl_bool FsLogger::getFileInfo(FileContext* context, FileInfo& info, const FileInfoMask& mask)
	{
		if (!(m_flags & FsLogGetInfo)/* || !m_regex.match(context->path)*/)
			return m_base->getFileInfo(context, info, mask);

		String desc = String::format("GetFileInfo(%s,0x%X)", contextDesc, mask);
		if (!(m_flags & FsLogRetAndErrors))
			LOG(desc);

		try {
			sl_bool ret = m_base->getFileInfo(context, info, mask);
			if (m_flags & FsLogRet) {
				LOG("%s\n  Ret: %d", desc, ret);
				if (mask & FileInfoMask::Attributes)
					LOG("  Attributes: 0x%X, %d", info.attributes, info.attributes & FileAttributes::Directory ? "DIR" : "FILE");
				if (mask & FileInfoMask::Size)
					LOG("  Size: %d", info.size);
				if (mask & FileInfoMask::AllocSize)
					LOG("  AllocSize: %d", info.allocSize);
				if (!(m_flags & FsLogAttrNoDate) &&
					(mask & FileInfoMask::Time)) {
					if (m_flags & FsLogDateAsString) {
						LOG("  CreatedAt: %s", info.createdAt.toString());
						LOG("  ModifiedAt: %s", info.modifiedAt.toString());
						LOG("  AccessedAt: %s", info.lastAccessedAt.toString());
					}
					else {
						LOG("  CreatedAt: %d", info.createdAt.toInt());
						LOG("  ModifiedAt: %d", info.modifiedAt.toInt());
						LOG("  AccessedAt: %d", info.lastAccessedAt.toInt());
					}
				}
			}
			return ret;
		}
		catch (FileSystemError error) {
			if (m_flags & FsLogErrors)
				LOG("%s\n  Error: %d, %s", desc, error, System::formatErrorCode((sl_uint32)error));
			throw error;
		}
	}

	sl_bool FsLogger::setFileInfo(const StringParam& path, const FileInfo& info, const FileInfoMask& mask)
	{
		if (!(m_flags & FsLogSetInfo) || !m_regex.match(path.toString()))
			return m_base->setFileInfo(path, info, mask);

		String desc = String::format("SetFileInfo(%s,0x%X)", path, mask);
		if (!(m_flags & FsLogRetAndErrors))
			LOG(desc);

		try {
			sl_bool ret = m_base->setFileInfo(path, info, mask);
			if (m_flags & FsLogRet) {
				LOG("%s\n  Ret: %d", desc, ret);
				if (mask & FileInfoMask::Attributes)
					LOG("  Attributes: 0x%X, %d", info.attributes, info.attributes & FileAttributes::Directory ? "DIR" : "FILE");
				if (mask & FileInfoMask::Size)
					LOG("  Size: %d", info.size);
				if (mask & FileInfoMask::AllocSize)
					LOG("  AllocSize: %d", info.allocSize);
				if (!(m_flags & FsLogAttrNoDate) &&
					(mask & FileInfoMask::Time)) {
					if (m_flags & FsLogDateAsString) {
						LOG("  CreatedAt: %s", info.createdAt.toString());
						LOG("  ModifiedAt: %s", info.modifiedAt.toString());
						LOG("  AccessedAt: %s", info.lastAccessedAt.toString());
					}
					else {
						LOG("  CreatedAt: %d", info.createdAt.toInt());
						LOG("  ModifiedAt: %d", info.modifiedAt.toInt());
						LOG("  AccessedAt: %d", info.lastAccessedAt.toInt());
					}
				}
			}
			return ret;
		}
		catch (FileSystemError error) {
			if (m_flags & FsLogErrors)
				LOG("%s\n  Error: %d, %s", desc, error, System::formatErrorCode((sl_uint32)error));
			throw error;
		}
	}

	sl_bool FsLogger::setFileInfo(FileContext* context, const FileInfo& info, const FileInfoMask& mask)
	{
		if (!(m_flags & FsLogSetInfo)/* || !m_regex.match(context->path)*/)
			return m_base->setFileInfo(context, info, mask);

		String desc = String::format("SetFileInfo(%s,0x%X)", contextDesc, mask);
		if (!(m_flags & FsLogRetAndErrors))
			LOG(desc);

		try {
			sl_bool ret = m_base->setFileInfo(context, info, mask);
			if (m_flags & FsLogRet) {
				LOG("%s\n  Ret: %d", desc, ret);
				if (mask & FileInfoMask::Attributes)
					LOG("  Attributes: 0x%X, %d", info.attributes, info.attributes & FileAttributes::Directory ? "DIR" : "FILE");
				if (mask & FileInfoMask::Size)
					LOG("  Size: %d", info.size);
				if (mask & FileInfoMask::AllocSize)
					LOG("  AllocSize: %d", info.allocSize);
				if (!(m_flags & FsLogAttrNoDate) &&
					(mask & FileInfoMask::Time)) {
					if (m_flags & FsLogDateAsString) {
						LOG("  CreatedAt: %s", info.createdAt.toString());
						LOG("  ModifiedAt: %s", info.modifiedAt.toString());
						LOG("  AccessedAt: %s", info.lastAccessedAt.toString());
					}
					else {
						LOG("  CreatedAt: %d", info.createdAt.toInt());
						LOG("  ModifiedAt: %d", info.modifiedAt.toInt());
						LOG("  AccessedAt: %d", info.lastAccessedAt.toInt());
					}
				}
			}
			return ret;
		}
		catch (FileSystemError error) {
			if (m_flags & FsLogErrors)
				LOG("%s\n  Error: %d, %s", desc, error, System::formatErrorCode((sl_uint32)error));
			throw error;
		}
	}

	HashMap<String, FileInfo> FsLogger::getFiles(const StringParam& path)
	{
		if (!(m_flags & FsLogList) || !m_regex.match(path.toString()))
			return m_base->getFiles(path);

		String desc = String::format("GetFiles(%s)", path);
		if (!(m_flags & FsLogRetAndErrors))
			LOG(desc);

		try {
			HashMap<String, FileInfo> ret = m_base->getFiles(path);
			if (m_flags & FsLogRet) {
				LOG(desc);
				for (auto& file : ret) {
					FileInfo info = file.value;
					LOG("  %s: (0x%X,%s,%d,%d%s)", file.key,
						info.attributes, info.attributes & FileAttributes::Directory ? "DIR" : "FILE",
						info.size, info.allocSize,
						(m_flags & FsLogAttrNoDate ? "" : ","
							+ (m_flags & FsLogDateAsString
								? String::format("%s,%s,%s", info.createdAt.toString(), info.modifiedAt.toString(), info.lastAccessedAt.toString())
								: String::format("%d,%d,%d", info.createdAt.toInt(), info.modifiedAt.toInt(), info.lastAccessedAt.toInt()))));
				}
			}
			return ret;
		}
		catch (FileSystemError error) {
			if (m_flags & FsLogErrors)
				LOG("%s\n  Error: %d, %s", desc, error, System::formatErrorCode((sl_uint32)error));
			throw error;
		}
	}

}