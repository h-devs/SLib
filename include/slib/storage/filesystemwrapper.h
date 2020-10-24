/**
 * @file slib/storage/filesystemwrapper.h
 * FileSystem Wrapper Layer Interface.
 *
 * @copyright 2020 Steve Han
 */

#pragma once

#include "file_system.h"

namespace slib
{

	class FileSystemWrapper : public FileSystemBase
	{
	public:
		/* ctor/dtor */
		FileSystemWrapper(Ref<FileSystemBase> base,
			const StringParam& fileSystemName = sl_null,
			const StringParam& volumeName = sl_null) : m_base(base)
		{
			m_volumeInfo = m_base->fsGetVolumeInfo();
			if (fileSystemName.isNotEmpty())
				m_volumeInfo.fileSystemName = fileSystemName.toString();
			if (volumeName.isNotNull())	// volume name can be empty string
				m_volumeInfo.volumeName = volumeName.toString();
		}
			
		~FileSystemWrapper()
		{
		}

	protected:
		/* FileSystem Interfaces */

		virtual const VolumeInfo&
			fsGetVolumeInfo(VolumeInfoFlags flags = VolumeInfoFlags::BasicInfo)& override
		{
			if (flags == VolumeInfoFlags::BasicInfo)
				return m_volumeInfo;
			return m_base->fsGetVolumeInfo(flags);
		}

		virtual void
			fsSetVolumeName(String volumeName) override
		{
			return m_base->fsSetVolumeName(volumeName);
		}

		virtual void
			fsCreate(FileContext* context, FileCreationParams& params = FileCreationParams()) override
		{
			return m_base->fsCreate(getBaseContext(context), params);
		}

		virtual void
			fsOpen(FileContext* context, FileCreationParams& params = FileCreationParams()) override
		{
			return m_base->fsOpen(getBaseContext(context), params);
		}

		virtual sl_size
			fsRead(FileContext* context, const Memory& buffer, sl_uint64 offset) override
		{
			return m_base->fsRead(getBaseContext(context), buffer, offset);
		}

		virtual sl_size
			fsWrite(FileContext* context, const Memory& buffer, sl_uint64 offset, sl_bool writeToEof) override
		{
			return m_base->fsWrite(getBaseContext(context), buffer, offset, writeToEof);
		}

		virtual void
			fsFlush(FileContext* context) override
		{
			return m_base->fsFlush(getBaseContext(context));
		}

		virtual void
			fsClose(FileContext* context) override
		{
			return m_base->fsClose(getBaseContext(context));
		}

		virtual void
			fsDelete(FileContext* context, sl_bool checkOnly) override
		{
			return m_base->fsDelete(getBaseContext(context), checkOnly);
		}

		virtual void
			fsRename(FileContext* context, String newFileName, sl_bool replaceIfExists) override
		{
			return m_base->fsRename(getBaseContext(context), getBaseFileName(newFileName), replaceIfExists);
		}

		virtual void
			fsLock(FileContext* context, sl_uint64 offset, sl_uint64 length) override
		{
			return m_base->fsLock(getBaseContext(context), offset, length);
		}

		virtual void
			fsUnlock(FileContext* context, sl_uint64 offset, sl_uint64 length) override
		{
			return m_base->fsUnlock(getBaseContext(context), offset, length);
		}

		virtual FileInfo
			fsGetFileInfo(FileContext* context) override
		{
			return m_base->fsGetFileInfo(getBaseContext(context));
		}

		virtual void
			fsSetFileInfo(FileContext* context, FileInfo fileInfo, FileInfoFlags flags) override
		{
			return m_base->fsSetFileInfo(getBaseContext(context), fileInfo, flags);
		}

		virtual sl_size
			fsGetSecurity(FileContext* context, sl_uint32 securityInformation, const Memory& securityDescriptor) override
		{
			return m_base->fsGetSecurity(getBaseContext(context), securityInformation, securityDescriptor);
		}

		virtual void
			fsSetSecurity(FileContext* context, sl_uint32 securityInformation, const Memory& securityDescriptor) override
		{
			return m_base->fsSetSecurity(getBaseContext(context), securityInformation, securityDescriptor);
		}

		virtual HashMap<String, FileInfo>
			fsFindFiles(FileContext* context, String pattern) override
		{
			return m_base->fsFindFiles(getBaseContext(context), pattern);
		}

		virtual HashMap<String, StreamInfo>
			fsFindStreams(FileContext* context) override
		{
			return m_base->fsFindStreams(getBaseContext(context));
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
		Ref<FileSystemBase> m_base;
	};

}