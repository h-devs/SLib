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

	public:
		sl_bool getInformation(FileSystemInfo& outInfo, const FileSystemInfoMask& mask) override;

		Ref<FileContext> openFile(const String& path, const FileOpenParam& param) override;

		sl_size	readFile(FileContext* context, sl_uint64 offset, void* buf, sl_size size) override;

		sl_size writeFile(FileContext* context, sl_int64 offset, const void* buf, sl_size size) override;

		sl_bool flushFile(FileContext* context) override;

		sl_bool	closeFile(FileContext* context) override;

		sl_bool deleteFile(const String& path) override;

		sl_bool moveFile(const String& pathOld, const String& pathNew, sl_bool flagReplaceIfExists) override;

		sl_bool lockFile(FileContext* context, sl_uint64 offset, sl_uint64 length) override;

		sl_bool unlockFile(FileContext* context, sl_uint64 offset, sl_uint64 length) override;

		sl_bool getFileInfo(FileContext* context, FileInfo& outInfo, const FileInfoMask& mask) override;

		sl_bool setFileInfo(FileContext* context, const FileInfo& info, const FileInfoMask& mask) override;

		sl_bool getFileInfo(const String& path, FileInfo& outInfo, const FileInfoMask& mask) override;

		sl_bool setFileInfo(const String& path, const FileInfo& info, const FileInfoMask& mask) override;

		HashMap<String, FileInfo> getFiles(const String& pathDir) override;

	private:
		FileSystemError getError(sl_uint32 error = 0);

	private:
		String m_root;
	};

}