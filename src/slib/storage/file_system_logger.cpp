#include "slib/storage/file_system_logger.h"
#include "slib/core/system.h"

#define TAG "FileSystemLogger"
#include "file_system.h"

#define FSLOG_TRY try
#define FSLOG_CATCH(desc) catch (FileSystemError error) { \
	if (m_flags & FsLogException) { \
		LOG("%s\n  Error: %d%s", desc, error, \
			(m_flags & FsLogExceptionString ? ", " + System::formatErrorCode((sl_uint32)error) : "")); \
	} \
	throw error; \
}

//#define contextDesc		String::fromUint64(context->handle) + \
//						(m_flags & FsLogFileName ? ":" + context->path : \
//						(m_flags & FsLogContextAddress ? ":0x" + String::fromUint64((sl_uint64)&context, 16, 8, sl_true) : ""))
#define contextDesc		"0x" + String::fromUint64((sl_uint64)&context, 16, 8, sl_true)

namespace slib
{

	FileSystemLogger::FileSystemLogger(Ref<FileSystemProvider> base, sl_uint32 logFlags, String regexFilter)
		: FileSystemWrapper(base), m_flags(logFlags), m_regex(regexFilter)
	{
		LOG_DEBUG("LogFlags: 0x%08X", m_flags);
	}

	sl_bool FileSystemLogger::getInformation(FileSystemInfo& info, const FileSystemInfoMask& mask)
	{
		if (!(m_flags & FsLogFileSystemInfo))
			return m_base->getInformation(info, mask);

		String desc = String::format("GetFileSystemInfo(%s)", mask);
		if (!(m_flags & FsLogRetAndErrors))
			LOG(desc);

		FSLOG_TRY {
			sl_bool ret = m_base->getInformation(info, mask);
			if (ret && (m_flags & FsLogRetSuccess)) {
				LOG(desc);
				if (mask & FileSystemInfoMask::Basic) {
					LOG("  volumeName: %s", info.volumeName);
					LOG("  fileSystemName: %s", info.fileSystemName);
					LOG("  creationTime: %s", m_flags & FsLogTimeInfoAsString
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
			else if (!ret && (m_flags & FsLogRetFail)) {
				LOG("%s\n  Error", desc);
			}
			return ret;
		}
		FSLOG_CATCH(desc)
	}

	sl_bool FileSystemLogger::createDirectory(const StringParam& path)
	{
		if (!(m_flags & FsLogCreate) || !m_regex.match(path.toString()))
			return m_base->createDirectory(path);

		String desc = String::format("CreateDirectory(%s)", path);
		if (!(m_flags & FsLogRetAndErrors))
			LOG(desc);

		FSLOG_TRY {
			sl_bool ret = m_base->createDirectory(path);
			if (ret && (m_flags & FsLogRet)) {
				LOG(desc);
			}
			else if (!ret && (m_flags & FsLogRetFail)) {
				LOG("%s\n  Error", desc);
			}
			return ret;
		}
		FSLOG_CATCH(desc)
	}

	Ref<FileContext> FileSystemLogger::openFile(const StringParam& path, const FileOpenParam& param)
	{
		if (!(m_flags & FsLogOpen) || !m_regex.match(path.toString()))
			return m_base->openFile(path, param);

		String desc = String::format("OpenFile(%s,%s,%s%s,0x%X,0x%X)", path,
			param.attributes & FileAttributes::Directory ? "DIR" : "FILE",
			(param.mode & FileMode::NotCreate ? "NOT_CREATE" : "CREATE"),
			(param.mode & FileMode::NotTruncate ? "" : "|TRUNCATE"),
			param.mode.value, param.attributes.value);
		if (!(m_flags & FsLogRetAndErrors))
			LOG(desc);

		FSLOG_TRY {
			Ref<FileContext> context = m_base->openFile(path, param);
			if (context.isNotNull() && (m_flags & FsLogRet)) {
				LOG(desc);
			}
			else if (context.isNull() && (m_flags & FsLogRetFail)) {
				LOG("%s\n  Error", desc);
			}
			return context;
		}
		FSLOG_CATCH(desc)
	}

	sl_size FileSystemLogger::readFile(FileContext* context, sl_uint64 offset, void* buf, sl_size size)
	{
		if (!(m_flags & FsLogRead)/* || !m_regex.match(context->path)*/)
			return m_base->readFile(context, offset, buf, size);

		String desc = String::format("ReadFile(%s,0x%X,0x%X)", contextDesc, offset, size);
		if (!(m_flags & FsLogRetAndErrors))
			LOG(desc);

		FSLOG_TRY {
			sl_size ret = m_base->readFile(context, offset, buf, size);
			if (ret && (m_flags & FsLogRet)) {
				LOG("%s\n  Ret: %d", desc, ret);
			}
			else if (!ret && (m_flags & FsLogRetFail)) {
				LOG("%s\n  Error", desc);
			}
			return ret;
		}
		FSLOG_CATCH(desc)
	}

	sl_size FileSystemLogger::writeFile(FileContext* context, sl_int64 offset, const void* buf, sl_size size)
	{
		if (!(m_flags & FsLogWrite)/* || !m_regex.match(context->path)*/)
			return m_base->writeFile(context, offset, buf, size);

		String desc = String::format("WriteFile(%s,0x%X,0x%X)", contextDesc, offset, size);
		if (!(m_flags & FsLogRetAndErrors))
			LOG(desc);

		FSLOG_TRY {
			sl_size ret = m_base->writeFile(context, offset, buf, size);
			if (ret && (m_flags & FsLogRet)) {
				LOG("%s\n  Ret: %d", desc, ret);
			}
			else if (!ret && (m_flags & FsLogRetFail)) {
				LOG("%s\n  Error", desc);
			}
			return ret;
		}
		FSLOG_CATCH(desc)
	}

	sl_bool FileSystemLogger::flushFile(FileContext* context)
	{
		if (!(m_flags & FsLogFlush)/* || !m_regex.match(context->path)*/)
			return m_base->flushFile(context);

		String desc = String::format("FlushFile(%s)", contextDesc);
		if (!(m_flags & FsLogRetAndErrors))
			LOG(desc);

		FSLOG_TRY {
			sl_bool ret = m_base->flushFile(context);
			if (ret && (m_flags & FsLogRet)) {
				LOG(desc);
			}
			else if (!ret && (m_flags & FsLogRetFail)) {
				LOG("%s\n  Error", desc);
			}
			return ret;
		}
		FSLOG_CATCH(desc)
	}

	sl_bool FileSystemLogger::closeFile(FileContext* context)
	{
		if (!(m_flags & FsLogClose)) {
			return m_base->closeFile(context);
		}

		String desc = String::format("CloseFile(%s)", contextDesc);
		if (!(m_flags & FsLogRetAndErrors))
			LOG(desc);

		FSLOG_TRY {
			sl_bool ret = m_base->closeFile(context);
			if (ret && (m_flags & FsLogRet)) {
				LOG(desc);
			}
			else if (!ret && (m_flags & FsLogRetFail)) {
				LOG("%s\n  Error", desc);
			}
			return ret;
		}
		FSLOG_CATCH(desc)
	}

	sl_bool FileSystemLogger::deleteDirectory(const StringParam& path)
	{
		if (!(m_flags & FsLogDelete) || !m_regex.match(path.toString()))
			return m_base->deleteDirectory(path);

		String desc = String::format("DeleteDirectory(%s)", path);
		if (!(m_flags & FsLogRetAndErrors))
			LOG(desc);

		FSLOG_TRY {
			sl_bool ret = m_base->deleteDirectory(path);
			if (ret && (m_flags & FsLogRet)) {
				LOG(desc);
			}
			else if (!ret && (m_flags & FsLogRetFail)) {
				LOG("%s\n  Error", desc);
			}
			return ret;
		}
		FSLOG_CATCH(desc)
	}

	sl_bool FileSystemLogger::deleteFile(const StringParam& path)
	{
		if (!(m_flags & FsLogDelete) || !m_regex.match(path.toString()))
			return m_base->deleteFile(path);
	
		String desc = String::format("DeleteFile(%s)", path);
		if (!(m_flags & FsLogRetAndErrors))
			LOG(desc);

		FSLOG_TRY {
			sl_bool ret = m_base->deleteFile(path);
			if (ret && (m_flags & FsLogRet)) {
				LOG(desc);
			}
			else if (!ret && (m_flags & FsLogRetFail)) {
				LOG("%s\n  Error", desc);
			}
			return ret;
		}
		FSLOG_CATCH(desc)
	}

	sl_bool FileSystemLogger::moveFile(const StringParam& pathOld, const StringParam& pathNew, sl_bool flagReplaceIfExists)
	{
		if (!(m_flags & FsLogMove) || !m_regex.match(pathOld.toString()))
			return m_base->moveFile(pathOld, pathNew, flagReplaceIfExists);

		String desc = String::format("MoveFile(%s,%s,%d)", pathOld, pathNew, flagReplaceIfExists);
		if (!(m_flags & FsLogRetAndErrors))
			LOG(desc);

		FSLOG_TRY {
			sl_bool ret = m_base->moveFile(pathOld, pathNew, flagReplaceIfExists);
			if (ret && (m_flags & FsLogRet)) {
				LOG(desc);
			}
			else if (!ret && (m_flags & FsLogRetFail)) {
				LOG("%s\n  Error", desc);
			}
			return ret;
		}
		FSLOG_CATCH(desc)
	}

	sl_bool FileSystemLogger::getFileInfo(const StringParam& path, FileContext* context, FileInfo& info, const FileInfoMask& mask)
	{
		if (!(m_flags & FsLogGetInfo) || path.isEmpty() || !m_regex.match(path.toString()))
			return m_base->getFileInfo(path, context, info, mask);

		String desc = String::format("GetFileInfo(%s,0x%X)", path, mask);
		if (!(m_flags & FsLogRetAndErrors))
			LOG(desc);

		FSLOG_TRY {
			sl_bool ret = m_base->getFileInfo(path, context, info, mask);
			if (ret && (m_flags & FsLogRet)) {
				LOG(desc);
				if (mask & FileInfoMask::Attributes)
					LOG("  Attributes: 0x%X, %s", info.attributes, info.attributes & FileAttributes::Directory ? "DIR" : "FILE");
				if (mask & FileInfoMask::Size)
					LOG("  Size: %d", info.size);
				if (mask & FileInfoMask::AllocSize)
					LOG("  AllocSize: %d", info.allocSize);
				if ((m_flags & FsLogTimeInfo) &&
					(mask & FileInfoMask::Time)) {
					if (m_flags & FsLogTimeInfoAsString) {
						LOG("  CreatedAt: %s", info.createdAt.toString());
						LOG("  ModifiedAt: %s", info.modifiedAt.toString());
						LOG("  AccessedAt: %s", info.accessedAt.toString());
					}
					else {
						LOG("  CreatedAt: %d", info.createdAt.toInt());
						LOG("  ModifiedAt: %d", info.modifiedAt.toInt());
						LOG("  AccessedAt: %d", info.accessedAt.toInt());
					}
				}
			}
			else if (!ret && (m_flags & FsLogRetFail)) {
				LOG("%s\n  Error", desc);
			}
			return ret;
		}
		FSLOG_CATCH(desc)
	}

	sl_bool FileSystemLogger::setFileInfo(const StringParam& path, FileContext* context, const FileInfo& info, const FileInfoMask& mask)
	{
		if (!(m_flags & FsLogSetInfo) || path.isEmpty() || !m_regex.match(path.toString()))
			return m_base->setFileInfo(path, context, info, mask);

		String desc = String::format("SetFileInfo(%s,0x%X)", path, mask);
		if (!(m_flags & FsLogRetAndErrors))
			LOG(desc);

		FSLOG_TRY {
			sl_bool ret = m_base->setFileInfo(path, context, info, mask);
			if (ret && (m_flags & FsLogRet)) {
				LOG(desc);
				if (mask & FileInfoMask::Attributes)
					LOG("  Attributes: 0x%X, %s", info.attributes, info.attributes & FileAttributes::Directory ? "DIR" : "FILE");
				if (mask & FileInfoMask::Size)
					LOG("  Size: %d", info.size);
				if (mask & FileInfoMask::AllocSize)
					LOG("  AllocSize: %d", info.allocSize);
				if ((m_flags & FsLogTimeInfo) &&
					(mask & FileInfoMask::Time)) {
					if (m_flags & FsLogTimeInfoAsString) {
						LOG("  CreatedAt: %s", info.createdAt.toString());
						LOG("  ModifiedAt: %s", info.modifiedAt.toString());
						LOG("  AccessedAt: %s", info.accessedAt.toString());
					}
					else {
						LOG("  CreatedAt: %d", info.createdAt.toInt());
						LOG("  ModifiedAt: %d", info.modifiedAt.toInt());
						LOG("  AccessedAt: %d", info.accessedAt.toInt());
					}
				}
			}
			else if (!ret && (m_flags & FsLogRetFail)) {
				LOG("%s\n  Error", desc);
			}
			return ret;
		}
		FSLOG_CATCH(desc)
	}

	HashMap<String, FileInfo> FileSystemLogger::getFiles(const StringParam& path)
	{
		if (!(m_flags & FsLogList) || !m_regex.match(path.toString()))
			return m_base->getFiles(path);

		String desc = String::format("GetFiles(%s)", path);
		if (!(m_flags & FsLogRetAndErrors))
			LOG(desc);

		FSLOG_TRY {
			HashMap<String, FileInfo> files = m_base->getFiles(path);
			if (files.isNotEmpty() && (m_flags & FsLogRet)) {
				LOG(desc);
				for (auto& file : files) {
					FileInfo info = file.value;
					LOG("  %s: (0x%X,%s,%d,%d%s)", file.key,
						info.attributes, info.attributes & FileAttributes::Directory ? "DIR" : "FILE",
						info.size, info.allocSize,
						(!(m_flags & FsLogTimeInfo) ? "" : ","
							+ (m_flags & FsLogTimeInfoAsString
								? String::format("%s,%s,%s", info.createdAt.toString(), info.modifiedAt.toString(), info.accessedAt.toString())
								: String::format("%d,%d,%d", info.createdAt.toInt(), info.modifiedAt.toInt(), info.accessedAt.toInt()))));
				}
			}
			else if (files.isEmpty() && (m_flags & FsLogRetFail)) {
				LOG("%s\n  Error", desc);
			}
			return files;
		}
		FSLOG_CATCH(desc)
	}

}