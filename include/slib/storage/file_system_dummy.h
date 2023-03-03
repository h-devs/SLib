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

#ifndef CHECKHEADER_SLIB_STORAGE_FILE_SYSTEM_DUMMY
#define CHECKHEADER_SLIB_STORAGE_FILE_SYSTEM_DUMMY

#include "file_system.h"

namespace slib
{

	class DummyFileSystem : public FileSystemProvider
	{
	public:
		DummyFileSystem()
		{
			m_fsInfo.volumeName = "Dummy";
			m_fsInfo.fileSystemName = "DummyFs";
			m_fsInfo.creationTime = Time::now();
			m_fsInfo.flags = FileSystemFlags::CaseSensitive;
		}

		~DummyFileSystem()
		{
		}

	public:

		sl_bool getSize(sl_uint64* pTotalSize, sl_uint64* pFreeSize = sl_null) override
		{
			if (pTotalSize) {
				*pTotalSize = 1024 * 1024 * 1024;
			}
			if (pFreeSize) {
				*pFreeSize = 512 * 1024 * 1024;
			}

			return sl_true;
		}

		Ref<FileContext> openFile(const StringParam& _path, const File::OpenParam& param) override
		{
			String path = _path.toString();
			if (!path.endsWith("/dummy.txt")) {
				FileSystem::setLastError(FileSystemError::NotFound);
				return sl_null;
			}

			return new FileContext();
		}

		sl_uint32 readFile(FileContext* context, sl_uint64 offset, void* buf, sl_uint32 size) override
		{
			return Memory::createStatic(buf, size).copy(String("dummy").toMemory(), (sl_size)offset, size);
		}

		sl_bool getFileInfo(const StringParam& _path, FileContext* context, FileInfo& outInfo, const FileInfoMask& mask) override
		{
			String path = _path.toString();
			outInfo.createdAt = outInfo.modifiedAt = outInfo.accessedAt = m_fsInfo.creationTime;
			if (path.endsWith("/") || path.endsWith("/dummy")) {
				outInfo.attributes = FileAttributes::Directory;
			}
			else if (path.endsWith("/dummy.txt")) {
				outInfo.attributes = FileAttributes::Normal;
				outInfo.size = outInfo.allocSize = 5;
			}
			else {
				FileSystem::setLastError(FileSystemError::NotFound);
				return sl_false;
			}

			return sl_true;
		}

		HashMap<String, FileInfo> getFiles(const StringParam& _path) override
		{
			String path = _path.toString();
			if (!path.endsWith("/") && !path.endsWith("/dummy")) {
				FileSystem::setLastError(FileSystemError::NotFound);
				return sl_null;
			}

			HashMap<String, FileInfo> files;
			FileInfo info;
			info.createdAt = info.modifiedAt = info.accessedAt = m_fsInfo.creationTime;

			// directories
			info.attributes = FileAttributes::Directory;
			files.add("dummy", info);

			// files
			info.attributes = FileAttributes::Normal;
			info.size = info.allocSize = 5;
			files.add("dummy.txt", info);

			return files;
		}

	};

}

#endif