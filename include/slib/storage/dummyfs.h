/**
* @file slib/storage/dummyfs.h
* Dummy FileSystem Definition.
*
* @copyright 2020 Steve Han
*/

#pragma once

#include "file_system.h"

namespace slib
{

	class DummyFs : public FileSystemProvider
	{
	public:
		DummyFs() {
			m_volumeInfo.volumeName = "Dummy";
			m_volumeInfo.fileSystemName = "DummyFs";
		}

	protected:
		void openFile(FileContext* context, FileCreationParams& params = FileCreationParams()) override
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

		sl_size readFile(FileContext* context, const Memory& buffer, sl_uint64 offset) override 
		{
			return buffer.copy(String("dummy").toMemory(), (sl_size)offset, buffer.getSize());
		}

		FileInfo getFileInfo(FileContext* context) override 
		{
			FileInfo info;
			info.createdAt = info.modifiedAt = info.lastAccessedAt = m_volumeInfo.creationTime;
			if (context->path.endsWith("\\") || context->path.endsWith("\\dummy"))
				info.attributes.isDirectory = true;
			else if (context->path.endsWith("\\dummy.txt"))
				info.size = info.allocSize = 5;
			else
				throw FileSystemError::NotFound;
			return info;
		}

		HashMap<String, FileInfo> getFiles(FileContext* context, String pattern) override 
		{
			HashMap<String, FileInfo> files;
			FileInfo info;
			info.createdAt = info.modifiedAt = info.lastAccessedAt = m_volumeInfo.creationTime;

			// directories
			info.attributes.isDirectory = true;
			files.add(".", info);
			files.add("..", info);
			files.add("dummy", info);

			// files
			info.attributes.isDirectory = false;
			info.size = info.allocSize = 5;
			files.add("dummy.txt", info);

			return files;
		}
	};

}