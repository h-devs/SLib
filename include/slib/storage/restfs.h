/**
* @file slib/storage/restfs.h
* RestFs FileSystem Definition.
*
* @copyright 2020 Steve Han
*/

#pragma once

#include "atomicfilesystem.h"
#include <slib/network.h>

namespace slib
{

	class RestFs : public AtomicFileSystem
	{
	public:
		RestFs(String Url);

	public:
		sl_bool ping();

	protected:
		FileInfo afsGetFileInfo(String uri) override;

		HashMap<String, FileInfo> afsFindFiles(String uri) override;

		sl_size afsRead(String uri, const Memory& buffer, sl_uint64 offset) override;

		sl_size afsWrite(String uri, const Memory& buffer, sl_uint64 offset) override;

		FileInfo afsCreateNew(String uri, sl_bool isDirectory) override;

		void afsSetFileSize(String uri, sl_uint64 size) override;

		void afsRename(String uri, String newUri, sl_bool replaceIfExists) override;

		void afsDelete(String uri, sl_bool checkOnly) override;

	private:
		FileSystemError getErrorFromResponse(Ref<UrlRequest> req);

	private:
		String _BaseUrl;
	};

}
