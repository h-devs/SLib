/**
* @file slib/storage/mirrorfs.h
* Mirror FileSystem Definition.
*
* @copyright 2020 Steve Han
*/

#pragma once

#include "filesystembase.h"

namespace slib
{

	class MirrorFs : public FileSystemBase
	{
	public:
		MirrorFs(String path);
		~MirrorFs();

	protected:
		const VolumeInfo& fsGetVolumeInfo(VolumeInfoFlags flags = VolumeInfoFlags::BasicInfo)& override;
		void fsSetVolumeName(String volumeName) override;

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
		Memory fsGetSecurity(FileContext* context, sl_uint32 securityInformation) override;
		void fsSetSecurity(FileContext* context, sl_uint32 securityInformation, const Memory& securityDescriptor) override;
		HashMap<String, FileInfo> fsFindFiles(FileContext* context, String pattern) override;
		HashMap<String, StreamInfo> fsFindStreams(FileContext* context) override;

	private:
		FileSystemError getError(sl_uint32 error = 0);

	private:
		String _Path;
		String _Root;
	};

}