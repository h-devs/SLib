/**
* @file slib/storage/mirrorfs.h
* Mirror FileSystem Definition.
*
* @copyright 2020 Steve Han
*/

#pragma once

#include "file_system.h"

namespace slib
{

	class MirrorFs : public FileSystemProvider
	{
	public:
		MirrorFs(String path);
		~MirrorFs();

	public:
		sl_bool getInformation(FileSystemInfo& info) override;

		void openFile(FileContext* context, FileCreationParams& params = FileCreationParams()) override;
		void openFile(FileContext* context, FileCreationParams& params = FileCreationParams()) override;
		sl_size readFile(FileContext* context, const Memory& buffer, sl_uint64 offset) override;
		sl_size writeFile(FileContext* context, const Memory& buffer, sl_uint64 offset, sl_bool writeToEof) override;
		void flush(FileContext* context) override;
		void closeFile(FileContext* context) override;
		void deleteFile(FileContext* context, sl_bool checkOnly) override;
		void moveFile(FileContext* context, String newFileName, sl_bool replaceIfExists) override;
		void lockFile(FileContext* context, sl_uint64 byteOffset, sl_uint64 length) override;
		void unlockFile(FileContext* context, sl_uint64 byteOffset, sl_uint64 length) override;
		FileInfo getFileInfo(FileContext* context) override;
		void setFileInfo(FileContext* context, FileInfo fileInfo) override;
		sl_size fsGetSecurity(FileContext* context, sl_uint32 securityInformation, const Memory& securityDescriptor) override;
		void fsSetSecurity(FileContext* context, sl_uint32 securityInformation, const Memory& securityDescriptor) override;
		HashMap<String, FileInfo> getFiles(FileContext* context, String pattern) override;
		HashMap<String, StreamInfo> fsFindStreams(FileContext* context) override;

		sl_bool getSize(sl_uint64* pOutTotalSize, sl_uint64* pOutFreeSize) override;

	private:
		FileSystemError getError(sl_uint32 error = 0);

	private:
		String m_path;
		String m_root;
	};

}