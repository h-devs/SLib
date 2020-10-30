/*
*   Copyright (c) 2008-2020 SLIBIO <https://github.com/SLIBIO>
*
*   Permission is hereby granted, free of charge, to any person obtaining a copy
*   of this software and associated documentation files (the "Software"), to deal
*   in the Software without restriction, including without limitation the rights
*   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
*   copies of the Software, and to permit persons to whom the Software is
*   furnished to do so, subject to the following conditions:
*
*   The above copyright notice and this permission notice shall be included in
*   all copies or substantial portions of the Software.
*
*   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
*   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
*   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
*   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
*   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
*   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
*   THE SOFTWARE.
*/

#ifndef CHECKHEADER_SLIB_STORAGE_FILE_SYSTEM_MIRROR
#define CHECKHEADER_SLIB_STORAGE_FILE_SYSTEM_MIRROR

#include "file_system.h"

namespace slib
{

	class MirrorFileSystem : public FileSystemProvider
	{
		SLIB_DECLARE_OBJECT

	public:
		MirrorFileSystem(const String& path);

		~MirrorFileSystem();

	public:
		sl_bool getInformation(FileSystemInfo& outInfo, const FileSystemInfoMask& mask) override;

		sl_bool createDirectory(const StringParam& path) override;

		Ref<FileContext> openFile(const StringParam& path, const FileOpenParam& param) override;

		sl_size	readFile(FileContext* context, sl_uint64 offset, void* buf, sl_size size) override;

		sl_size writeFile(FileContext* context, sl_int64 offset, const void* buf, sl_size size) override;

		sl_bool flushFile(FileContext* context) override;

		sl_bool	closeFile(FileContext* context) override;

		sl_bool deleteDirectory(const StringParam& path) override;

		sl_bool deleteFile(const StringParam& path) override;

		sl_bool moveFile(const StringParam& pathOld, const StringParam& pathNew, sl_bool flagReplaceIfExists) override;

		sl_bool getFileInfo(const StringParam& path, FileContext* context, FileInfo& outInfo, const FileInfoMask& mask) override;

		sl_bool setFileInfo(const StringParam& path, FileContext* context, const FileInfo& info, const FileInfoMask& mask) override;

		HashMap<String, FileInfo> getFiles(const StringParam& pathDir) override;

	private:
		FileSystemError getError(sl_uint32 error = 0);

	private:
		String m_root;

	};

}

#endif