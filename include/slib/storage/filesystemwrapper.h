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

	class FileSystemWrapper : public FileSystemProvider
	{
	public:
		/* ctor/dtor */
		FileSystemWrapper(Ref<FileSystemProvider> base,
			const StringParam& fileSystemName = sl_null,
			const StringParam& volumeName = sl_null) : m_base(base)
		{
			if (fileSystemName.isNotEmpty())
				m_volumeInfo.fileSystemName = fileSystemName.toString();
			if (volumeName.isNotNull())	// volume name can be empty string
				m_volumeInfo.volumeName = volumeName.toString();
		}
			
		~FileSystemWrapper()
		{
		}

	public:
		sl_bool getInformation(FileSystemInfo& info) override
		{
			sl_bool bRet = m_base->getInformation(info);
			if (bRet) {
				if (info.mask & FileSystemInfo::MASK_BASIC) {
					if (m_volumeInfo.fileSystemName.isNotEmpty()) {
						info.fileSystemName = m_volumeInfo.fileSystemName;
					}
					if (m_volumeInfo.volumeName.isNotEmpty()) {
						info.volumeName = m_volumeInfo.volumeName;
					}
				}
			}
			return sl_false;
		}

		virtual void
			openFile(FileContext* context, FileCreationParams& params = FileCreationParams()) override
		{
			return m_base->openFile(getBaseContext(context), params);
		}

		virtual void
			openFile(FileContext* context, FileCreationParams& params = FileCreationParams()) override
		{
			return m_base->openFile(getBaseContext(context), params);
		}

		virtual sl_size
			readFile(FileContext* context, const Memory& buffer, sl_uint64 offset) override
		{
			return m_base->readFile(getBaseContext(context), buffer, offset);
		}

		virtual sl_size
			writeFile(FileContext* context, const Memory& buffer, sl_uint64 offset, sl_bool writeToEof) override
		{
			return m_base->writeFile(getBaseContext(context), buffer, offset, writeToEof);
		}

		virtual void
			flush(FileContext* context) override
		{
			return m_base->flush(getBaseContext(context));
		}

		virtual void
			closeFile(FileContext* context) override
		{
			return m_base->closeFile(getBaseContext(context));
		}

		virtual void
			deleteFile(FileContext* context, sl_bool checkOnly) override
		{
			return m_base->deleteFile(getBaseContext(context), checkOnly);
		}

		virtual void
			moveFile(FileContext* context, String newFileName, sl_bool replaceIfExists) override
		{
			return m_base->moveFile(getBaseContext(context), getBaseFileName(newFileName), replaceIfExists);
		}

		virtual void
			lockFile(FileContext* context, sl_uint64 offset, sl_uint64 length) override
		{
			return m_base->lockFile(getBaseContext(context), offset, length);
		}

		virtual void
			unlockFile(FileContext* context, sl_uint64 offset, sl_uint64 length) override
		{
			return m_base->unlockFile(getBaseContext(context), offset, length);
		}

		virtual FileInfo
			getFileInfo(FileContext* context) override
		{
			return m_base->getFileInfo(getBaseContext(context));
		}

		virtual void
			setFileInfo(FileContext* context, FileInfo fileInfo) override
		{
			return m_base->setFileInfo(getBaseContext(context), fileInfo);
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
			getFiles(FileContext* context, String pattern) override
		{
			return m_base->getFiles(getBaseContext(context), pattern);
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
		 * created context (esp. in openFile/openFile) and call decreaseReference() to free up (esp. in closeFile).
		 * @see CryptoFsWrapper::getBaseContext, CryptoFsWrapper::openFile, CryptoFsWrapper::closeFile
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
		Ref<FileSystemProvider> m_base;
	};

}