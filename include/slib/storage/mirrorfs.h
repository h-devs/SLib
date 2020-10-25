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

	protected:
		void fsCreate(FileContext* context, FileCreationParams& params = FileCreationParams()) override;
		void fsOpen(FileContext* context, FileCreationParams& params = FileCreationParams()) override;
		sl_size fsRead(FileContext* context, const Memory& buffer, sl_uint64 offset) override;
		sl_size fsWrite(FileContext* context, const Memory& buffer, sl_uint64 offset, sl_bool writeToEof) override;
		void fsFlush(FileContext* context) override;
		void fsClose(FileContext* context) override;
		void fsDelete(FileContext* context, sl_bool checkOnly) override;
		void fsRename(FileContext* context, String newFileName, sl_bool replaceIfExists) override;
		void fsLock(FileContext* context, sl_uint64 byteOffset, sl_uint64 length) override;
		void fsUnlock(FileContext* context, sl_uint64 byteOffset, sl_uint64 length) override;
		FileInfo fsGetFileInfo(FileContext* context) override;
		void fsSetFileInfo(FileContext* context, FileInfo fileInfo, FileInfoFlags flags) override;
		sl_size fsGetSecurity(FileContext* context, sl_uint32 securityInformation, const Memory& securityDescriptor) override;
		void fsSetSecurity(FileContext* context, sl_uint32 securityInformation, const Memory& securityDescriptor) override;
		HashMap<String, FileInfo> fsFindFiles(FileContext* context, String pattern) override;
		HashMap<String, StreamInfo> fsFindStreams(FileContext* context) override;

		sl_bool getSize(sl_uint64* pOutTotalSize, sl_uint64* pOutFreeSize) override;

	private:
		FileSystemError getError(sl_uint32 error = 0);

	private:
		String m_root;
	};

}