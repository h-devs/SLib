/**
* @file slib/storage/dummyfs.h
* Dummy FileSystem Definition.
*
* @copyright 2020 Steve Han
*/

#pragma once

#include "filesystembase.h"

namespace slib
{

	class DummyFs : public FileSystemBase
	{
	public:
		DummyFs() {
			m_volumeInfo.volumeName = "Dummy";
			m_volumeInfo.fileSystemName = "DummyFs";
			m_volumeInfo.totalSize = 1024 * 1024 * 1024;
			m_volumeInfo.freeSize = 512 * 1024 * 1024;
		}

	protected:
		void fsOpen(FileContext* context, FileCreationParams& params = FileCreationParams()) override
		{
			if (context->path.endsWith("\\") || context->path.endsWith("\\dummy")) {
				context->isDirectory = sl_true;
			}
			else if (context->path.endsWith("\\dummy.txt")) {
				context->isDirectory = sl_false;
			}
			else
				throw FileSystemError::NotFound;
		}

		sl_size fsRead(FileContext* context, const Memory& buffer, sl_uint64 offset) override 
		{
			return buffer.copy(String("dummy").toMemory(), (sl_size)offset, buffer.getSize());
		}

		FileInfo fsGetFileInfo(FileContext* context) override 
		{
			FileInfo info;
			info.createdAt = info.modifiedAt = info.lastAccessedAt = m_volumeInfo.creationTime;
			if (context->path.endsWith("\\") || context->path.endsWith("\\dummy"))
				info.attr.isDirectory = true;
			else if (context->path.endsWith("\\dummy.txt"))
				info.size = info.allocationSize = 5;
			else
				throw FileSystemError::NotFound;
			return info;
		}

		HashMap<String, FileInfo> fsFindFiles(FileContext* context, String pattern) override 
		{
			HashMap<String, FileInfo> files;
			FileInfo info;
			info.createdAt = info.modifiedAt = info.lastAccessedAt = m_volumeInfo.creationTime;

			// directories
			info.attr.isDirectory = true;
			files.add(".", info);
			files.add("..", info);
			files.add("dummy", info);

			// files
			info.attr.isDirectory = false;
			info.size = info.allocationSize = 5;
			files.add("dummy.txt", info);

			return files;
		}
	};

}