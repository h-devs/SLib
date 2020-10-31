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

#define TAG "MirrorFileSystem"
#include "slib/storage/file_system_internal.h"

#include "slib/storage/file_system_mirror.h"

#include "slib/core/file.h"
#include "slib/core/system.h"
#include "slib/core/variant.h"
#include "slib/storage/disk.h"

#define FILE_FROM_CONTEXT(context)	(context ? ((MirrorFileContext*)(context))->file : sl_null)
#define CONCAT_PATH(path)			(path.isNotEmpty() ? m_root + path : sl_null)

namespace slib
{

	namespace priv
	{
		namespace file_system_mirror
		{

			class MirrorFileContext : public FileContext
			{
			public:
				Ref<File> file;

			public:
				MirrorFileContext(Ref<File> file) : file(file)
				{
				}

			};

		}
	}

	using namespace priv::file_system_mirror;


	SLIB_DEFINE_OBJECT(MirrorFileSystem, FileSystemProvider)

	MirrorFileSystem::MirrorFileSystem(const String& path) : m_root(path)
	{
		m_fsInfo.fileSystemName = "MirrorFs";
		m_fsInfo.creationTime = File::getCreatedTime(m_root);
		m_fsInfo.flags = FileSystemFlags::CaseSensitive;
	}

	MirrorFileSystem::~MirrorFileSystem()
	{
	}

	sl_bool MirrorFileSystem::getSize(sl_uint64* pTotalSize, sl_uint64* pFreeSize)
	{
		return Disk::getSize(m_root, pTotalSize, pFreeSize);
	}

	sl_bool MirrorFileSystem::createDirectory(const StringParam& path)
	{
		if (!File::createDirectory(CONCAT_PATH(path))) {
			SLIB_THROW(getError(), sl_false);
		}
		return sl_true;
	}

	Ref<FileContext> MirrorFileSystem::openFile(const StringParam& path, const FileOpenParam& param)
	{
		Ref<File> file = File::open(CONCAT_PATH(path), param);
		if (file.isNull()) {
			SLIB_THROW(getError(), sl_null);
		}
		return new MirrorFileContext(file);
	}

	sl_bool MirrorFileSystem::closeFile(FileContext* context)
	{
		Ref<File> file = FILE_FROM_CONTEXT(context);
		if (file.isNotNull()) {
			file->close();
			if (file->isOpened()) {
				SLIB_THROW(getError(), sl_false);
			}
		}
		return sl_true;
	}

	sl_uint32 MirrorFileSystem::readFile(FileContext* context, sl_uint64 offset, void* buf, sl_uint32 size)
	{
		Ref<File> file = FILE_FROM_CONTEXT(context);

		if (file.isNull() || !file->isOpened()) {
			SLIB_THROW(FileSystemError::InvalidContext, 0);
		}

		if (!file->seek(offset, SeekPosition::Begin)) {
			SLIB_THROW(getError(), 0);
		}

		sl_int32 ret = file->read32(buf, size);
		if (ret < 0) {
			SLIB_THROW(getError(), 0);
		}

		return ret;
	}

	sl_uint32 MirrorFileSystem::writeFile(FileContext* context, sl_int64 offset, const void* buf, sl_uint32 size)
	{
		Ref<File> file = FILE_FROM_CONTEXT(context);

		if (file.isNull() || !file->isOpened()) {
			SLIB_THROW(FileSystemError::InvalidContext, 0);
		}

		if (offset < 0) {
			if (!file->seekToEnd()) {
				SLIB_THROW(getError(), 0);
			}
		} else {
			if (!file->seek(offset, SeekPosition::Begin)) {
				SLIB_THROW(getError(), 0);
			}
		}

		sl_int32 ret = file->write32(buf, size);
		if (ret < 0) {
			SLIB_THROW(getError(), 0);
		}

		return ret;
	}

	sl_bool MirrorFileSystem::flushFile(FileContext* context)
	{
		Ref<File> file = FILE_FROM_CONTEXT(context);
		if (file.isNotNull()) {
			if (file->flush()) {
				return sl_true;
			}
		}
		return sl_false;
	}

	sl_bool MirrorFileSystem::deleteDirectory(const StringParam& path)
	{
		if (!File::deleteDirectory(CONCAT_PATH(path))) {
			SLIB_THROW(getError(), sl_false);
		}
		return sl_true;
	}

	sl_bool MirrorFileSystem::deleteFile(const StringParam& path)
	{
		if (!File::deleteFile(CONCAT_PATH(path))) {
			SLIB_THROW(getError(), sl_false);
		}
		return sl_true;
	}

	sl_bool MirrorFileSystem::moveFile(const StringParam& pathOld, const StringParam& pathNew, sl_bool flagReplaceIfExists)
	{
		if (!File::move(CONCAT_PATH(pathOld), CONCAT_PATH(pathNew), flagReplaceIfExists)) {
			SLIB_THROW(getError(), sl_false);
		}
		return sl_true;
	}

	sl_bool MirrorFileSystem::getFileInfo(const StringParam& path, FileContext* context, FileInfo& outInfo, const FileInfoMask& mask)
	{
		String filePath = CONCAT_PATH(path);
		Ref<File> file = FILE_FROM_CONTEXT(context);
		sl_bool flagOpened = file.isNotNull() && file->isOpened();

		if (mask & FileInfoMask::Attributes) {
			FileAttributes attr = File::getAttributes(filePath);
			if (attr & FileAttributes::NotExist) {
				if (!flagOpened) {
					SLIB_THROW(FileSystemError::NotFound, sl_false);
				}
				attr = file->getAttributes();
				if (attr & FileAttributes::NotExist) {
					SLIB_THROW(FileSystemError::NotFound, sl_false);
				}
			}
			outInfo.attributes = (sl_uint32)attr;
		}

		if ((mask & FileInfoMask::Size) || (mask & FileInfoMask::AllocSize)) {
			if (flagOpened) {
				outInfo.size = outInfo.allocSize = file->getSize();
			} else {
				outInfo.size = outInfo.allocSize = File::getSize(filePath);
			}
		}

		if (mask & FileInfoMask::Time) {
			if (flagOpened) {
				outInfo.createdAt = file->getCreatedTime();
				outInfo.modifiedAt = file->getModifiedTime();
				outInfo.accessedAt = file->getAccessedTime();
			} else {
				outInfo.createdAt = File::getCreatedTime(filePath);
				outInfo.modifiedAt = File::getModifiedTime(filePath);
				outInfo.accessedAt = File::getAccessedTime(filePath);
			}
		}

		return sl_true;
	}

	sl_bool MirrorFileSystem::setFileInfo(const StringParam& path, FileContext* context, const FileInfo& info, const FileInfoMask& mask)
	{
		String filePath = CONCAT_PATH(path);
		Ref<File> file = FILE_FROM_CONTEXT(context);
		sl_bool flagOpened = file.isNotNull() && file->isOpened();

		if (mask & FileInfoMask::Attributes) {
			if (!File::setAttributes(filePath, info.attributes)) {
				SLIB_THROW(getError(), sl_false);
			}
		}

		if (mask & FileInfoMask::Time) {
			if (info.createdAt.isNotZero()) {
				sl_bool ret = sl_false;
				if (flagOpened) {
					ret = file->setCreatedTime(info.createdAt);
				}
				if (!ret && !File::setCreatedTime(filePath, info.createdAt)) {
					SLIB_THROW(getError(), sl_false);
				}
			}
			if (info.modifiedAt.isNotZero()) {
				sl_bool ret = sl_false;
				if (flagOpened) {
					ret = file->setModifiedTime(info.modifiedAt);
				}
				if (!ret && !File::setModifiedTime(filePath, info.modifiedAt)) {
					SLIB_THROW(getError(), sl_false);
				}
			}
			if (info.accessedAt.isNotZero()) {
				sl_bool ret = sl_false;
				if (flagOpened) {
					ret = file->setAccessedTime(info.accessedAt);
				}
				if (!ret && !File::setAccessedTime(filePath, info.accessedAt)) {
					SLIB_THROW(getError(), sl_false);
				}
			}
		}

		if (mask & FileInfoMask::Size) {
			if (!flagOpened) {
				SLIB_THROW(FileSystemError::InvalidContext, sl_false);
			}
			if (!file->setSize(info.size)) {
				SLIB_THROW(getError(), sl_false);
			}
		}

		if (mask & FileInfoMask::AllocSize) {
			if (!flagOpened) {
				SLIB_THROW(FileSystemError::InvalidContext, sl_false);
			}
			//if (!file->setAllocationSize(info.allocationSize))
			//	SLIB_THROW(getError(), sl_false);
		}

		return sl_true;
	}

	HashMap<String, FileInfo> MirrorFileSystem::getFiles(const StringParam& pathDir)
	{
		return File::getFileInfos(CONCAT_PATH(pathDir));
	}

	FileSystemError MirrorFileSystem::getError(sl_uint32 error)
	{
		return (FileSystemError)(error ? error : System::getLastError());
	}

}