/*
*   Copyright (c) 2008-2020 SLIBIO <https://github.com/SLIBIO>
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

#define TAG "FileSystemLogger"
#include "slib/storage/file_system_internal.h"

#include "slib/storage/file_system_logger.h"

#include "slib/core/system.h"

#define TRY SLIB_TRY
#define CATCH(desc) SLIB_CATCH (FileSystemError error, { \
	if (m_flags & FileSystemLogFlags::Exception) { \
		LOG("%s\n  Error: %d%s", desc, error, \
			(m_flags & FileSystemLogFlags::ExceptionString ? ", " + System::formatErrorCode((sl_uint32)error) : "")); \
	} \
	throw error; \
})

#define pathDesc		(m_flags & FileSystemLogFlags::FileName ? path : "")
#define contextDesc		(m_flags & FileSystemLogFlags::ContextAddress ? ":0x" + String::fromUint64((sl_uint64)&context, 16, 8, sl_true) : "")

namespace slib
{

	namespace priv
	{
		namespace file_system_logger
		{

			SLIB_STATIC_STRING(g_defaultFilter, ".*")

		}
	}

	using namespace priv::file_system_logger;

	SLIB_DEFINE_OBJECT(FileSystemLogger, FileSystemWrapper)

	FileSystemLogger::FileSystemLogger(const Ref<FileSystemProvider>& base)
		: FileSystemLogger(base, FileSystemLogFlags::All, g_defaultFilter)
	{
	}

	FileSystemLogger::FileSystemLogger(const Ref<FileSystemProvider>& base, const FileSystemLogFlags& flags)
		: FileSystemLogger(base, flags, g_defaultFilter)
	{
	}

	FileSystemLogger::FileSystemLogger(const Ref<FileSystemProvider>& base, const FileSystemLogFlags& flags, const String& filter)
		: FileSystemWrapper(base), m_flags(flags), m_regex(filter)
	{
		LOG_DEBUG("LogFlags: 0x%08X", m_flags);

		if (m_flags & FileSystemLogFlags::FileSystemInfo) {
			LOG("FileSystemInfo:");
			LOG("  volumeName: %s", m_fsInfo.volumeName);
			LOG("  fileSystemName: %s", m_fsInfo.fileSystemName);
			LOG("  creationTime: %s", m_flags & FileSystemLogFlags::TimeInfoAsString ? m_fsInfo.creationTime.toString() : String::fromInt64(m_fsInfo.creationTime.toInt()));
			LOG("  serialNumber: %d", m_fsInfo.serialNumber);
			LOG("  sectorSize: %d", m_fsInfo.sectorSize);
			LOG("  sectorsPerAllocationUnit: %d", m_fsInfo.sectorsPerAllocationUnit);
			LOG("  maxPathLength: %d", m_fsInfo.maxPathLength);
			LOG("  flags: 0x%X", m_fsInfo.flags.value);
		}
	}

	FileSystemLogger::~FileSystemLogger()
	{
	}

	sl_bool FileSystemLogger::getInformation(FileSystemInfo& info, const FileSystemInfoMask& mask)
	{
		if (!(m_flags & FileSystemLogFlags::FileSystemInfo))
			return m_base->getInformation(info, mask);

		String desc = String::format("GetFileSystemInfo(%s)", mask);
		if (!(m_flags & FileSystemLogFlags::RetAndErrors))
			LOG(desc);

		TRY {
			sl_bool ret = m_base->getInformation(info, mask);
			if (ret && (m_flags & FileSystemLogFlags::RetSuccess)) {
				LOG(desc);
				if (mask & FileSystemInfoMask::Basic) {
					LOG("  volumeName: %s", info.volumeName);
					LOG("  fileSystemName: %s", info.fileSystemName);
					LOG("  creationTime: %s", m_flags & FileSystemLogFlags::TimeInfoAsString
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
			} else if (!ret && (m_flags & FileSystemLogFlags::RetFail)) {
				LOG("%s\n  Error", desc);
			}
			return ret;
		}
		CATCH(desc)
	}

	sl_bool FileSystemLogger::createDirectory(const StringParam& path)
	{
		if (!(m_flags & FileSystemLogFlags::Create) || !m_regex.match(path.toString()))
			return m_base->createDirectory(path);

		String desc = String::format("CreateDirectory(%s)", pathDesc);
		if (!(m_flags & FileSystemLogFlags::RetAndErrors))
			LOG(desc);

		TRY {
			sl_bool ret = m_base->createDirectory(path);
			if (ret && (m_flags & FileSystemLogFlags::RetSuccess)) {
				LOG(desc);
			} else if (!ret && (m_flags & FileSystemLogFlags::RetFail)) {
				LOG("%s\n  Error", desc);
			}
			return ret;
		}
		CATCH(desc)
	}

	Ref<FileContext> FileSystemLogger::openFile(const StringParam& path, const FileOpenParam& param)
	{
		if (!(m_flags & FileSystemLogFlags::Open) || !m_regex.match(path.toString()))
			return m_base->openFile(path, param);

		String desc = String::format("OpenFile(%s,%s,%s%s,%s%s,%s%s%s,0x%X)", pathDesc,
			param.attributes & FileAttributes::Directory ? "DIR" : "FILE",

			(param.mode & FileMode::NotCreate ? "OPEN" : (param.mode & FileMode::NotTruncate ? "OPEN_OR_CREATE" : "CREATE")),
			(param.mode & FileMode::NotTruncate ? "" : "|TRUNCATE"),

			(param.mode & FileMode::Read ? "READ" : ""),
			(param.mode & FileMode::Write ? "WRITE" : ""),

			(param.mode & FileMode::ShareRead ? "R" : "-"), 
			(param.mode & FileMode::ShareWrite ? "W" : "-"), 
			(param.mode & FileMode::ShareDelete ? "D" : "-"),

			param.attributes.value);

		if (!(m_flags & FileSystemLogFlags::RetAndErrors))
			LOG(desc);

		TRY {
			Ref<FileContext> context = m_base->openFile(path, param);
			if (context.isNotNull() && (m_flags & FileSystemLogFlags::RetSuccess)) {
				LOG(desc);
			} else if (context.isNull() && (m_flags & FileSystemLogFlags::RetFail)) {
				LOG("%s\n  Error", desc);
			}
			return context;
		}
		CATCH(desc)
	}

	sl_size FileSystemLogger::readFile(FileContext* context, sl_uint64 offset, void* buf, sl_size size)
	{
		if (!(m_flags & FileSystemLogFlags::Read)/* || !m_regex.match(context->path)*/)
			return m_base->readFile(context, offset, buf, size);

		String desc = String::format("ReadFile(%s,0x%X,0x%X)", contextDesc, offset, size);
		if (!(m_flags & FileSystemLogFlags::RetAndErrors))
			LOG(desc);

		TRY {
			sl_size ret = m_base->readFile(context, offset, buf, size);
			if (ret && (m_flags & FileSystemLogFlags::RetSuccess)) {
				LOG("%s\n  Ret: %d", desc, ret);
			} else if (!ret && (m_flags & FileSystemLogFlags::RetFail)) {
				LOG("%s\n  Error", desc);
			}
			return ret;
		}
		CATCH(desc)
	}

	sl_size FileSystemLogger::writeFile(FileContext* context, sl_int64 offset, const void* buf, sl_size size)
	{
		if (!(m_flags & FileSystemLogFlags::Write)/* || !m_regex.match(context->path)*/)
			return m_base->writeFile(context, offset, buf, size);

		String desc = String::format("WriteFile(%s,0x%X,0x%X)", contextDesc, offset, size);
		if (!(m_flags & FileSystemLogFlags::RetAndErrors))
			LOG(desc);

		TRY {
			sl_size ret = m_base->writeFile(context, offset, buf, size);
			if (ret && (m_flags & FileSystemLogFlags::RetSuccess)) {
				LOG("%s\n  Ret: %d", desc, ret);
			} else if (!ret && (m_flags & FileSystemLogFlags::RetFail)) {
				LOG("%s\n  Error", desc);
			}
			return ret;
		}
		CATCH(desc)
	}

	sl_bool FileSystemLogger::flushFile(FileContext* context)
	{
		if (!(m_flags & FileSystemLogFlags::Flush)/* || !m_regex.match(context->path)*/)
			return m_base->flushFile(context);

		String desc = String::format("FlushFile(%s)", contextDesc);
		if (!(m_flags & FileSystemLogFlags::RetAndErrors))
			LOG(desc);

		TRY {
			sl_bool ret = m_base->flushFile(context);
			if (ret && (m_flags & FileSystemLogFlags::RetSuccess)) {
				LOG(desc);
			} else if (!ret && (m_flags & FileSystemLogFlags::RetFail)) {
				LOG("%s\n  Error", desc);
			}
			return ret;
		}
		CATCH(desc)
	}

	sl_bool FileSystemLogger::closeFile(FileContext* context)
	{
		if (!(m_flags & FileSystemLogFlags::Close)) {
			return m_base->closeFile(context);
		}

		String desc = String::format("CloseFile(%s)", contextDesc);
		if (!(m_flags & FileSystemLogFlags::RetAndErrors))
			LOG(desc);

		TRY {
			sl_bool ret = m_base->closeFile(context);
			if (ret && (m_flags & FileSystemLogFlags::RetSuccess)) {
				LOG(desc);
			} else if (!ret && (m_flags & FileSystemLogFlags::RetFail)) {
				LOG("%s\n  Error", desc);
			}
			return ret;
		}
		CATCH(desc)
	}

	sl_bool FileSystemLogger::deleteDirectory(const StringParam& path)
	{
		if (!(m_flags & FileSystemLogFlags::Delete) || !m_regex.match(path.toString()))
			return m_base->deleteDirectory(path);

		String desc = String::format("DeleteDirectory(%s)", pathDesc);
		if (!(m_flags & FileSystemLogFlags::RetAndErrors))
			LOG(desc);

		TRY {
			sl_bool ret = m_base->deleteDirectory(path);
			if (ret && (m_flags & FileSystemLogFlags::RetSuccess)) {
				LOG(desc);
			} else if (!ret && (m_flags & FileSystemLogFlags::RetFail)) {
				LOG("%s\n  Error", desc);
			}
			return ret;
		}
		CATCH(desc)
	}

	sl_bool FileSystemLogger::deleteFile(const StringParam& path)
	{
		if (!(m_flags & FileSystemLogFlags::Delete) || !m_regex.match(path.toString()))
			return m_base->deleteFile(path);
	
		String desc = String::format("DeleteFile(%s)", pathDesc);
		if (!(m_flags & FileSystemLogFlags::RetAndErrors))
			LOG(desc);

		TRY {
			sl_bool ret = m_base->deleteFile(path);
			if (ret && (m_flags & FileSystemLogFlags::RetSuccess)) {
				LOG(desc);
			} else if (!ret && (m_flags & FileSystemLogFlags::RetFail)) {
				LOG("%s\n  Error", desc);
			}
			return ret;
		}
		CATCH(desc)
	}

	sl_bool FileSystemLogger::moveFile(const StringParam& pathOld, const StringParam& pathNew, sl_bool flagReplaceIfExists)
	{
		if (!(m_flags & FileSystemLogFlags::Move) || !m_regex.match(pathOld.toString()))
			return m_base->moveFile(pathOld, pathNew, flagReplaceIfExists);

		String desc = String::format("MoveFile(%s,%s,%d)", pathOld, pathNew, flagReplaceIfExists);
		if (!(m_flags & FileSystemLogFlags::RetAndErrors))
			LOG(desc);

		TRY {
			sl_bool ret = m_base->moveFile(pathOld, pathNew, flagReplaceIfExists);
			if (ret && (m_flags & FileSystemLogFlags::RetSuccess)) {
				LOG(desc);
			} else if (!ret && (m_flags & FileSystemLogFlags::RetFail)) {
				LOG("%s\n  Error", desc);
			}
			return ret;
		}
		CATCH(desc)
	}

	sl_bool FileSystemLogger::getFileInfo(const StringParam& path, FileContext* context, FileInfo& info, const FileInfoMask& mask)
	{
		if (!(m_flags & FileSystemLogFlags::GetInfo) || path.isEmpty() || !m_regex.match(path.toString()))
			return m_base->getFileInfo(path, context, info, mask);

		String desc = String::format("GetFileInfo(%s%s,0x%X)", pathDesc, contextDesc, mask);
		if (!(m_flags & FileSystemLogFlags::RetAndErrors))
			LOG(desc);

		TRY {
			sl_bool ret = m_base->getFileInfo(path, context, info, mask);
			if (ret && (m_flags & FileSystemLogFlags::RetSuccess)) {
				LOG(desc);
				if (mask & FileInfoMask::Attributes)
					LOG("  Attributes: 0x%X, %s", info.attributes, info.attributes & FileAttributes::Directory ? "DIR" : "FILE");
				if (mask & FileInfoMask::Size)
					LOG("  Size: %d", info.size);
				if (mask & FileInfoMask::AllocSize)
					LOG("  AllocSize: %d", info.allocSize);
				if ((m_flags & FileSystemLogFlags::TimeInfo) &&
					(mask & FileInfoMask::Time)) {
					if (m_flags & FileSystemLogFlags::TimeInfoAsString) {
						LOG("  CreatedAt: %s", info.createdAt.toString());
						LOG("  ModifiedAt: %s", info.modifiedAt.toString());
						LOG("  AccessedAt: %s", info.accessedAt.toString());
					} else {
						LOG("  CreatedAt: %d", info.createdAt.toInt());
						LOG("  ModifiedAt: %d", info.modifiedAt.toInt());
						LOG("  AccessedAt: %d", info.accessedAt.toInt());
					}
				}
			} else if (!ret && (m_flags & FileSystemLogFlags::RetFail)) {
				LOG("%s\n  Error", desc);
			}
			return ret;
		}
		CATCH(desc)
	}

	sl_bool FileSystemLogger::setFileInfo(const StringParam& path, FileContext* context, const FileInfo& info, const FileInfoMask& mask)
	{
		if (!(m_flags & FileSystemLogFlags::SetInfo) || path.isEmpty() || !m_regex.match(path.toString()))
			return m_base->setFileInfo(path, context, info, mask);

		String desc = String::format("SetFileInfo(%s%s,0x%X)", pathDesc, contextDesc, mask);
		if (!(m_flags & FileSystemLogFlags::RetAndErrors))
			LOG(desc);

		TRY {
			sl_bool ret = m_base->setFileInfo(path, context, info, mask);
			if (ret && (m_flags & FileSystemLogFlags::RetSuccess)) {
				LOG(desc);
				if (mask & FileInfoMask::Attributes)
					LOG("  Attributes: 0x%X, %s", info.attributes, info.attributes & FileAttributes::Directory ? "DIR" : "FILE");
				if (mask & FileInfoMask::Size)
					LOG("  Size: %d", info.size);
				if (mask & FileInfoMask::AllocSize)
					LOG("  AllocSize: %d", info.allocSize);
				if ((m_flags & FileSystemLogFlags::TimeInfo) &&
					(mask & FileInfoMask::Time)) {
					if (m_flags & FileSystemLogFlags::TimeInfoAsString) {
						LOG("  CreatedAt: %s", info.createdAt.toString());
						LOG("  ModifiedAt: %s", info.modifiedAt.toString());
						LOG("  AccessedAt: %s", info.accessedAt.toString());
					} else {
						LOG("  CreatedAt: %d", info.createdAt.toInt());
						LOG("  ModifiedAt: %d", info.modifiedAt.toInt());
						LOG("  AccessedAt: %d", info.accessedAt.toInt());
					}
				}
			} else if (!ret && (m_flags & FileSystemLogFlags::RetFail)) {
				LOG("%s\n  Error", desc);
			}
			return ret;
		}
		CATCH(desc)
	}

	HashMap<String, FileInfo> FileSystemLogger::getFiles(const StringParam& path)
	{
		if (!(m_flags & FileSystemLogFlags::List) || !m_regex.match(path.toString()))
			return m_base->getFiles(path);

		String desc = String::format("GetFiles(%s)", pathDesc);
		if (!(m_flags & FileSystemLogFlags::RetAndErrors))
			LOG(desc);

		TRY {
			HashMap<String, FileInfo> files = m_base->getFiles(path);
			if (files.isNotEmpty() && (m_flags & FileSystemLogFlags::RetSuccess)) {
				LOG(desc);
				for (auto& file : files) {
					FileInfo info = file.value;
					LOG("  %s: (0x%X,%s,%d,%d%s)", file.key,
						info.attributes, info.attributes & FileAttributes::Directory ? "DIR" : "FILE",
						info.size, info.allocSize,
						(!(m_flags & FileSystemLogFlags::TimeInfo) ? "" : ","
							+ (m_flags & FileSystemLogFlags::TimeInfoAsString
								? String::format("%s,%s,%s", info.createdAt.toString(), info.modifiedAt.toString(), info.accessedAt.toString())
								: String::format("%d,%d,%d", info.createdAt.toInt(), info.modifiedAt.toInt(), info.accessedAt.toInt()))));
				}
			} else if (files.isEmpty() && (m_flags & FileSystemLogFlags::RetFail)) {
				LOG("%s\n  Error", desc);
			}
			return files;
		}
		CATCH(desc)
	}

}