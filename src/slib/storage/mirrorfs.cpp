#include "slib/storage/mirrorfs.h"
#include "slib/core/file.h"
#include "slib/core/system.h"
#include "slib/core/variant.h"

#ifdef SLIB_PLATFORM_IS_WIN32
#include <windows.h>
#endif

#ifdef SLIB_FILE_SYSTEM_CAN_THROW
#define SLIB_USE_THROW
#endif
#include "slib/core/throw.h"

#define FileFromContext(context)	(((MirrorFileContext*)(context))->file)

namespace slib
{

	class MirrorFileContext : public FileContext
	{
	public:
		Ref<File> file;

	public:
		MirrorFileContext()
		{
		}

		MirrorFileContext(Ref<File> file) : file(file)
		{
		}
	};

	MirrorFs::MirrorFs(String path) : m_root(path)
	{
	}

	sl_bool MirrorFs::getInformation(FileSystemInfo& outInfo, const FileSystemInfoMask& mask)
	{
		if (mask & FileSystemInfoMask::Basic) {
			outInfo.fileSystemName = "MirrorFs";
			outInfo.creationTime = File::getCreatedTime(m_root);
			outInfo.flags = FileSystemFlags::CaseSensitive;
		}

		if (mask & FileSystemInfoMask::Size) {
#ifdef SLIB_PLATFORM_IS_WIN32
			StringCstr16 root(m_root);
			ULARGE_INTEGER totalSize, freeSize;

			if (!GetDiskFreeSpaceExW((LPCWSTR)(root.getData()), 0, &totalSize, &freeSize)) {
				return sl_false;
			}

			outInfo.totalSize = totalSize.QuadPart;
			outInfo.freeSize = freeSize.QuadPart;
#else
			SLIB_THROW(FileSystemError::NotImplemented, sl_false); // TODO
#endif
		}

		return sl_true;
	}

	sl_bool MirrorFs::createDirectory(const StringParam& path)
	{
		if (!File::createDirectory(m_root + path)) {
			SLIB_THROW(getError(), sl_false);
		}
		return sl_true;
	}

	Ref<FileContext> MirrorFs::openFile(const StringParam& path, const FileOpenParam& param)
	{
		Ref<File> file = File::open(m_root + path, param);
		if (file.isNull()) {
			SLIB_THROW(getError(), sl_null);
		}
		return new MirrorFileContext(file);
	}

	sl_bool MirrorFs::closeFile(FileContext* context)
	{
		Ref<File> file = FileFromContext(context);
		if (file.isNotNull()) {
			file->close();
			if (file->isOpened()) {
				SLIB_THROW(getError(), sl_false);
			}
		}
		return sl_true;
	}

	sl_size MirrorFs::readFile(FileContext* context, sl_uint64 offset, void* buf, sl_size size)
	{
		Ref<File> file = FileFromContext(context);

		if (file.isNull() || !file->isOpened()) {
			SLIB_THROW(FileSystemError::InvalidContext, sl_false);
		}

		if (!file->seek(offset, SeekPosition::Begin)) {
			SLIB_THROW(getError(), 0);
		}

		//auto ret = file->read(buf, size);	// IReader::read() only supports 1GB ?
		auto ret = file->read32(buf, (sl_uint32)size);	// File::read32() returns -1 if zero byte read ?
		if (ret < 0) {
			SLIB_THROW(getError(), 0);
		}

		return ret;
	}

	sl_size MirrorFs::writeFile(FileContext* context, sl_int64 offset, const void* buf, sl_size size)
	{
		Ref<File> file = FileFromContext(context);

		if (file.isNull() || !file->isOpened()) {
			SLIB_THROW(FileSystemError::InvalidContext, sl_false);
		}

		if (offset < 0) {
			if (!file->seekToEnd()) {
				SLIB_THROW(getError(), 0);
			}
		}
		else {
			if (!file->seek(offset, SeekPosition::Begin)) {
				SLIB_THROW(getError(), 0);
			}
		}

		//auto ret = file->write(buf, size);	// IReader::write() only supports 1GB ?
		auto ret = file->write32(buf, (sl_uint32)size);	// File::write32() returns -1 if zero byte written ?
		if (ret < 0) {
			SLIB_THROW(getError(), 0);
		}

		return ret;
	}

	sl_bool MirrorFs::flushFile(FileContext* context)
	{
		Ref<File> file = FileFromContext(context);

		if (file.isNotNull()) {
#ifdef SLIB_PLATFORM_IS_WIN32
			HANDLE handle = (HANDLE)(file->getHandle());
			if (!handle || handle == INVALID_HANDLE_VALUE) {
				return sl_true;
			}
			if (!FlushFileBuffers(handle)) {
				SLIB_THROW(getError(), sl_false);
			}
#endif
		}

		return sl_true;
	}

	sl_bool MirrorFs::deleteDirectory(const StringParam& path)
	{
		if (!File::deleteFile(m_root + path, sl_true)) {
			SLIB_THROW(getError(), sl_false);
		}
		return sl_true;
	}

	sl_bool MirrorFs::deleteFile(const StringParam& path)
	{
		if (!File::deleteFile(m_root + path, sl_true)) {
			SLIB_THROW(getError(), sl_false);
		}
		return sl_true;
	}

	sl_bool MirrorFs::moveFile(const StringParam& oldPath, const StringParam& newPath, sl_bool flagReplaceIfExists)
	{
		// TODO replaceIfExists
		if (!File::rename(m_root + oldPath, m_root + newPath)) {
			SLIB_THROW(getError(), sl_false);
		}
		return sl_true;
	}

	sl_bool MirrorFs::getFileInfo(const StringParam& path, FileContext* context, FileInfo& outInfo, const FileInfoMask& mask)
	{
		String filePath = (path.isNotEmpty() ? m_root + path : sl_null);
		Ref<File> file = (context ? FileFromContext(context) : sl_null);
		sl_bool isOpened = file.isNotNull() && file->isOpened();

		if (mask & FileInfoMask::Attributes) {
			FileAttributes attr = File::getAttributes(filePath);	// FIXME returns NotExist on error
			if (attr == FileAttributes::NotExist) {
				if (!isOpened) {
					SLIB_THROW(FileSystemError::InvalidContext, sl_false);
				}

#ifdef SLIB_PLATFORM_IS_WIN32
				HANDLE handle = (HANDLE)(file->getHandle());
				BY_HANDLE_FILE_INFORMATION byHandleFileInfo;

				if (GetFileInformationByHandle(handle, &byHandleFileInfo)) {
					outInfo.attributes = byHandleFileInfo.dwFileAttributes;
				} else {
					SLIB_THROW(getError(), sl_false);
				}
#else
				SLIB_THROW(FIleSystemError::NotImplemented, sl_false);
#endif
			}
			else {
				outInfo.attributes = (sl_uint32)attr;
			}
		}

		if ((mask & FileInfoMask::Size) || (mask & FileInfoMask::AllocSize)) {
			if (isOpened) {
				outInfo.size = outInfo.allocSize = file->getSize();
			} else {
				outInfo.size = outInfo.allocSize = File::getSize(filePath);
			}
		}

		if (mask & FileInfoMask::Time) {
			if (isOpened) {
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

	sl_bool MirrorFs::setFileInfo(const StringParam& path, FileContext* context, const FileInfo& info, const FileInfoMask& mask)
	{
		String filePath = (path.isNotEmpty() ? m_root + path : sl_null);
		Ref<File> file = (context ? FileFromContext(context) : sl_null);
		sl_bool isOpened = file.isNotNull() && file->isOpened();

		if (mask & FileInfoMask::Attributes) {
			if (!File::setAttributes(filePath, info.attributes)) {
				SLIB_THROW(getError(), sl_false);
			}
		}

		if (mask & FileInfoMask::Time) {
			if (info.createdAt.isNotZero()) {
				sl_bool ret = sl_false;
				if (isOpened) {
					ret = file->setCreatedTime(info.createdAt);
				}
				if (!ret && !File::setCreatedTime(filePath, info.createdAt)) {
					SLIB_THROW(getError(), sl_false);
				}
			}
			if (info.modifiedAt.isNotZero()) {
				sl_bool ret = sl_false;
				if (isOpened) {
					ret = file->setModifiedTime(info.modifiedAt);
				}
				if (!ret && !File::setModifiedTime(filePath, info.modifiedAt)) {
					SLIB_THROW(getError(), sl_false);
				}
			}
			if (info.accessedAt.isNotZero()) {
				sl_bool ret = sl_false;
				if (isOpened) {
					ret = file->setAccessedTime(info.accessedAt);
				}
				if (!ret && !File::setAccessedTime(filePath, info.accessedAt)) {
					SLIB_THROW(getError(), sl_false);
				}
			}
		}

		if (mask & FileInfoMask::Size) {
			if (!isOpened) {
				SLIB_THROW(FileSystemError::InvalidContext, sl_false);
			}
			if (!file->setSize(info.size)) {
				SLIB_THROW(getError(), sl_false);
			}
		}

		if (mask & FileInfoMask::AllocSize) {
			if (!isOpened) {
				SLIB_THROW(FileSystemError::InvalidContext, sl_false);
			}
			//if (!file->setAllocationSize(info.allocationSize))
			//	SLIB_THROW(getError(), sl_false);
		}

		return sl_true;
	}

	HashMap<String, FileInfo> MirrorFs::getFiles(const StringParam& pathDir)
	{
		return File::getFileInfos(m_root + pathDir);
	}

	FileSystemError MirrorFs::getError(sl_uint32 error)
	{
		return (FileSystemError)(error == 0 ? System::getLastError() : error);
	}

}