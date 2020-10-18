/**
 * @file slib/storage/atomicfilesystem.h
 * Atomic FileSystem Interface.
 *
 * @copyright 2020 Steve Han
 */

#pragma once

#include <slib/core.h>
#include "filesystembase.h"

namespace slib
{

	class AtomicFileSystem : public FileSystemBase
	{
	public:
		AtomicFileSystem() : FileSystemBase(), _handleCounter(0) {}

	public:
		/* AtomicFileSystem Interfaces */

		virtual const VolumeInfo&
			afsGetVolumeInfo()&
		{
			return _VolumeInfo;
		}

		virtual FileInfo
			afsCreateNew(String fileName, sl_bool isDirectory)
		{
			throw FileSystemError::NotImplemented;
		}

		virtual sl_size
			afsRead(String fileName, const Memory &buffer, sl_uint64 offset)
		{
			throw FileSystemError::NotImplemented;
		}

		virtual sl_size
			afsWrite(String fileName, const Memory &buffer, sl_uint64 offset)
		{
			throw FileSystemError::NotImplemented;
		}

		virtual FileInfo
			afsGetFileInfo(String fileName)
		{
			throw FileSystemError::NotImplemented;
		}
			
		virtual void
			afsSetFileSize(String fileName, sl_uint64 fileSize)
		{
			throw FileSystemError::NotImplemented;
		}

		virtual HashMap<String, FileInfo>
			afsFindFiles(String dirPath)
		{
			throw FileSystemError::NotImplemented;
		}

		virtual void 
			afsDelete(String fileName, sl_bool checkOnly)
		{
			throw FileSystemError::NotImplemented;
		}

		virtual void 
			afsRename(String fileName, String newFileName, sl_bool replaceIfExists)
		{
			throw FileSystemError::NotImplemented;
		}

	protected:
		/* FileSystem Interfaces implementation for AtomicFileSystem */

		virtual const VolumeInfo&
			fsGetVolumeInfo(VolumeInfoFlags flags = VolumeInfoFlags::BasicInfo)& override
		{
			return afsGetVolumeInfo();
		}

		virtual void
			fsCreate(FileContext &context, FileCreationParams &params = FileCreationParams()) override
		{
			if (exists(context.path))
				throw FileSystemError::FileExist;

			FileInfo info = afsCreateNew(context.path, context.isDirectory || params.attr.isDirectory);
			context.isDirectory = info.attr.isDirectory;

			ObjectLocker locker(this);
			sl_uint64 handle = 0;
			if (!_closedHandles.popFront(&handle) || !handle)
				handle = ++_handleCounter;
			context.handle = handle;
		}

		virtual void
			fsOpen(FileContext &context, FileCreationParams &params = FileCreationParams()) override
		{
			if (!exists(context.path))
				throw FileSystemError::NotFound;

			FileInfo info = afsGetFileInfo(context.path);
			context.isDirectory = info.attr.isDirectory;

			ObjectLocker locker(this);
			sl_uint64 handle = 0;
			if (!_closedHandles.popFront(&handle) || !handle)
				handle = ++_handleCounter;
			context.handle = handle;
		}

		virtual sl_size
			fsRead(FileContext &context, const Memory &buffer, sl_uint64 offset) override
		{
			return afsRead(context.path, buffer, offset);
		}

		virtual sl_size
			fsWrite(FileContext &context, const Memory &buffer, sl_uint64 offset, sl_bool writeToEof) override
		{
			if (writeToEof)
				offset = afsGetFileInfo(context.path).size;
			return afsWrite(context.path, buffer, offset);
		}

		virtual void
			fsClose(FileContext &context) override
		{
			if (context.handle) {
				ObjectLocker locker(this);
				_closedHandles.add(context.handle);
				//if (_FileNames.getCount() == 0) {
				//	_handleCounter = 0;
				//	_closedHandles.removeAll();
				//}
			}
			context.handle = 0;
		}

		virtual void
			fsDelete(FileContext &context, sl_bool checkOnly) override
		{
			afsDelete(context.path, checkOnly);
		}

		virtual void
			fsRename(FileContext &context, String newFileName, sl_bool replaceIfExists) override
		{
			afsRename(context.path, newFileName, replaceIfExists);
		}

		virtual FileInfo
			fsGetFileInfo(FileContext &context) override
		{
			return afsGetFileInfo(context.path);
		}

		virtual void
			fsSetFileInfo(FileContext &context, FileInfo fileInfo, FileInfoFlags flags) override
		{
			if (flags & FileInfoFlags::SizeInfo)
				afsSetFileSize(context.path, fileInfo.size);
			else
				throw FileSystemError::NotImplemented;
		}

		virtual HashMap<String, FileInfo>
			fsFindFiles(FileContext &context, String pattern) override
		{
			return afsFindFiles(context.path);
		}

	private:
		sl_uint64 _handleCounter;
		List<sl_uint64> _closedHandles;
	};

}