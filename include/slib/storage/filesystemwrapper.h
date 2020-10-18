/**
 * @file slib/storage/filesystemwrapper.h
 * FileSystem Wrapper Layer Interface.
 *
 * @copyright 2020 Steve Han
 */

#pragma once

#include "filesystembase.h"

namespace slib
{

	class FileSystemWrapper : public FileSystemBase
	{
	public:
		/* ctor/dtor */
		FileSystemWrapper(Ref<FileSystemBase> base) : _BaseFs(base)
		{
			_VolumeInfo = _BaseFs->fsGetVolumeInfo();
		}
			
		~FileSystemWrapper()
		{
		}

	protected:
		/* FileSystem Interfaces */

		virtual const VolumeInfo&
			fsGetVolumeInfo(VolumeInfoFlags flags = VolumeInfoFlags::BasicInfo)& override
		{
			return _BaseFs->fsGetVolumeInfo(flags);
		}

		virtual void
			fsSetVolumeName(String volumeName) override
		{
			return _BaseFs->fsSetVolumeName(volumeName);
		}

		virtual void
			fsCreate(FileContext* context, FileCreationParams& params = FileCreationParams()) override
		{
			return _BaseFs->fsCreate(getBaseContext(context), params);
		}

		virtual void
			fsOpen(FileContext* context, FileCreationParams& params = FileCreationParams()) override
		{
			return _BaseFs->fsOpen(getBaseContext(context), params);
		}

		virtual sl_size
			fsRead(FileContext* context, const Memory& buffer, sl_uint64 offset) override
		{
			return _BaseFs->fsRead(getBaseContext(context), buffer, offset);
		}

		virtual sl_size
			fsWrite(FileContext* context, const Memory& buffer, sl_uint64 offset, sl_bool writeToEof) override
		{
			return _BaseFs->fsWrite(getBaseContext(context), buffer, offset, writeToEof);
		}

		virtual void
			fsFlush(FileContext* context) override
		{
			return _BaseFs->fsFlush(getBaseContext(context));
		}

		virtual void
			fsClose(FileContext* context) override
		{
			return _BaseFs->fsClose(getBaseContext(context));
		}

		virtual void
			fsDelete(FileContext* context, sl_bool checkOnly) override
		{
			return _BaseFs->fsDelete(getBaseContext(context), checkOnly);
		}

		virtual void
			fsRename(FileContext* context, String newFileName, sl_bool replaceIfExists) override
		{
			return _BaseFs->fsRename(getBaseContext(context), getBaseFileName(newFileName), replaceIfExists);
		}

		virtual void
			fsLock(FileContext* context, sl_uint64 offset, sl_uint64 length) override
		{
			return _BaseFs->fsLock(getBaseContext(context), offset, length);
		}

		virtual void
			fsUnlock(FileContext* context, sl_uint64 offset, sl_uint64 length) override
		{
			return _BaseFs->fsUnlock(getBaseContext(context), offset, length);
		}

		virtual FileInfo
			fsGetFileInfo(FileContext* context) override
		{
			return _BaseFs->fsGetFileInfo(getBaseContext(context));
		}

		virtual void
			fsSetFileInfo(FileContext* context, FileInfo fileInfo, FileInfoFlags flags) override
		{
			return _BaseFs->fsSetFileInfo(getBaseContext(context), fileInfo, flags);
		}

		virtual Memory
			fsGetSecurity(FileContext* context, sl_uint32 securityInformation) override
		{
			return _BaseFs->fsGetSecurity(getBaseContext(context), securityInformation);
		}

		virtual void
			fsSetSecurity(FileContext* context, sl_uint32 securityInformation, const Memory& securityDescriptor) override
		{
			return _BaseFs->fsSetSecurity(getBaseContext(context), securityInformation, securityDescriptor);
		}

		virtual HashMap<String, FileInfo>
			fsFindFiles(FileContext* context, String pattern) override
		{
			return _BaseFs->fsFindFiles(getBaseContext(context), pattern);
		}

		virtual HashMap<String, StreamInfo>
			fsFindStreams(FileContext* context) override
		{
			return _BaseFs->fsFindStreams(getBaseContext(context));
		}

	protected:
		/**
		 * If you want to use different FileContext in wrapper, you will need to override this function.
		 * You can use context.handle to save base context pointer.
		 * You should call base context's increaseReference() function if you want to preserve newly 
		 * created context (esp. in fsCreate/fsOpen) and call decreaseReference() to free up (esp. in fsClose).
		 * @see CryptoFsWrapper::getBaseContext, CryptoFsWrapper::fsCreate, CryptoFsWrapper::fsClose
		 */
		virtual Ref<FileContext> getBaseContext(FileContext* context)
		{
			return context;
		}

		/**
		* If you want to use different file name in wrapper, you will need to override this function.
		* You must override getBaseContext() and call this function when you create base context.
		* @see CryptoFsWrapper::getBaseContext
		*/
		virtual String getBaseFileName(String fileName)
		{
			return fileName;
		}

	protected:
		Ref<FileSystemBase> _BaseFs;
	};

}