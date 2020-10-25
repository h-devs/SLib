#include <windows.h>
#include "slib/core/file.h"
#include "slib/core/system.h"
#include "slib/core/variant.h"
#include "slib/storage/mirrorfs.h"

#define FileFromContext(FD)             ((File*)(FD->handle))

namespace slib
{

	MirrorFs::MirrorFs(String path) : m_root(path)
	{
		m_volumeInfo.fileSystemName = "MirrorFs";
		m_volumeInfo.creationTime = File::getCreatedTime(path);
		m_volumeInfo.flags = FileSystemFlags::IsCaseSensitiveSearch;
	}

	sl_bool MirrorFs::getSize(sl_uint64* pOutTotalSize, sl_uint64* pOutFreeSize)
	{
		StringCstr16 root(m_root);
		ULARGE_INTEGER totalSize, freeSize;

		if (GetDiskFreeSpaceExW((LPCWSTR)(root.getData()), 0, &totalSize, &freeSize)) {
			if (pOutTotalSize) {
				*pOutTotalSize = totalSize.QuadPart;
			}
			if (pOutFreeSize) {
				*pOutFreeSize = freeSize.QuadPart;
			}
			return sl_true;
		}
		return sl_false;
	}

	void MirrorFs::fsCreate(FileContext* context, FileCreationParams& params)
	{
		if (context->isDirectory || params.attr.isDirectory) {
			if (!File::createDirectory(m_root + context->path, params.createAlways == sl_false)) {
				throw FileSystemError::AccessDenied;
			}
			params.createAlways = sl_false;
			context->isDirectory = sl_true;
			return;
		}

		Ref<File> file = File::open(m_root + context->path,
			FileMode::Write, // | (params.createAlways ? 0 : FileMode::NotCreate) | (params.openTruncate ? 0 : FileMode::NotTruncate),
			FilePermissions::ShareAll);

		if (file.isNull()) {
			throw getError();
		}

		file->increaseReference();
		context->handle = (sl_uint64)file.ptr;
	}

	void MirrorFs::fsOpen(FileContext* context, FileCreationParams& params)
	{
		if (context->isDirectory || params.attr.isDirectory) {
			if (!File::exists(m_root + context->path)) {
				if (params.createAlways) {
					if (!File::createDirectory(m_root + context->path)) {
						throw FileSystemError::AccessDenied;
					}
				}
				else {
					throw FileSystemError::NotFound;
				}
			}
			params.createAlways = sl_false;
			context->isDirectory = sl_true;
			return;
		}

		Ref<File> file = File::open(m_root + context->path,
			FileMode::ReadWrite | (params.createAlways ? 0 : FileMode::NotCreate) | (params.openTruncate ? 0 : FileMode::NotTruncate),
			FilePermissions::ShareAll);

		if (file.isNull()) {
			throw getError();
		}

		params.createAlways = sl_false;
		params.openTruncate = sl_false;

		file->increaseReference();
		context->handle = (sl_uint64)file.ptr;
	}

	void MirrorFs::fsClose(FileContext* context)
	{
		Ref<File> file = FileFromContext(context);
		if (file.isNotNull()) {
			file->close();
			file->decreaseReference();
		}

		context->handle = 0;
	}

	sl_size MirrorFs::fsRead(FileContext* context, const Memory& buffer, sl_uint64 offset)
	{
		if (context->isDirectory) {
			throw FileSystemError::AccessDenied;
		}

		Ref<File> file = FileFromContext(context);
		if (file.isNull()) {
			file = File::open(m_root + context->path, FileMode::Read, FilePermissions::ShareRead);
		}
		if (file.isNull()) {
			throw getError();
		}

		if (!file->seek(offset, SeekPosition::Begin))
			throw getError();

		//auto ret = file->read(buffer.getData(), buffer.getSize());	// IReader::read() only supports 1GB ?
		auto ret = file->read32(buffer.getData(), (sl_uint32)buffer.getSize());	// File::read32() returns -1 if zero byte read ?
		if (ret < 0) {
			throw getError();
		}

		return ret;
	}

	sl_size MirrorFs::fsWrite(FileContext* context, const Memory& buffer, sl_uint64 offset, sl_bool writeToEof)
	{
		if (context->isDirectory) {
			throw FileSystemError::AccessDenied;
		}

		Ref<File> file = FileFromContext(context);

		if (file.isNull()) {
			file = File::open(m_root + context->path, FileMode::Write, FilePermissions::ShareWrite);
		}
		if (file.isNull()) {
			throw getError();
		}

		if (writeToEof) {
			if (!file->seekToEnd())
				throw getError();
		}
		else {
			if (!file->seek(offset, SeekPosition::Begin))
				throw getError();
		}

		//auto ret = file->write(buffer.getData(), buffer.getSize());	// IReader::write() only supports 1GB ?
		auto ret = file->write32(buffer.getData(), (sl_uint32)buffer.getSize());	// File::write32() returns -1 if zero byte written ?
		if (ret < 0) {
			throw getError();
		}

		return ret;
	}

	void MirrorFs::fsFlush(FileContext* context)
	{
		Ref<File> file = FileFromContext(context);
		if (file.isNotNull()) {
#ifdef SLIB_PLATFORM_IS_WIN32
			HANDLE handle = (HANDLE)(file->getHandle());
			if (!handle || handle == INVALID_HANDLE_VALUE) {
				return;
			}
			if (!FlushFileBuffers(handle))
				throw getError();
#endif
		}
	}

	void MirrorFs::fsDelete(FileContext* context, sl_bool checkOnly)
	{
		if (checkOnly) {
			if (!File::exists(m_root + context->path))
				throw FileSystemError::NotFound;
			if (context->isDirectory) {
				if (!File::getFiles(m_root + context->path).isEmpty()) {
					throw FileSystemError::DirNotEmpty;
				}
			}
			return;
		}

		if (!File::deleteFile(m_root + context->path, sl_true)) {
			throw getError();
		}
	}

	void MirrorFs::fsRename(FileContext* context, String newFileName, sl_bool replaceIfExists)
	{
		// TODO replaceIfExists
		if (!File::rename(m_root + context->path, m_root + newFileName)) {
			throw getError();
		}
	}

	void MirrorFs::fsLock(FileContext* context, sl_uint64 offset, sl_uint64 length)
	{
		// TODO offset, length
		Ref<File> file = FileFromContext(context);
		if (file.isNotNull()) {
			if (!file->lock())
				throw getError();
		}
	}

	void MirrorFs::fsUnlock(FileContext* context, sl_uint64 offset, sl_uint64 length)
	{
		// TODO offset, length
		Ref<File> file = FileFromContext(context);
		if (file.isNotNull()) {
			if (!file->unlock())
				throw getError();
		}
	}

	FileInfo MirrorFs::fsGetFileInfo(FileContext* context)
	{
		String filePath = m_root + context->path;
		FileInfo fileInfo;

		fileInfo.fileAttributes = File::getAttributes(filePath);
		fileInfo.size = fileInfo.allocationSize = File::getSize(filePath);
		fileInfo.createdAt = File::getCreatedTime(filePath);
		fileInfo.modifiedAt = File::getModifiedTime(filePath);
		fileInfo.lastAccessedAt = File::getAccessedTime(filePath);

		return fileInfo;
	}

	void MirrorFs::fsSetFileInfo(FileContext* context, FileInfo fileInfo, FileInfoFlags flags)
	{
		String filePath = m_root + context->path;

		if (flags & FileInfoFlags::AttrInfo) {
			if (!File::setAttributes(filePath, fileInfo.fileAttributes))
				throw getError();
		}

		if (flags & FileInfoFlags::TimeInfo) {
			if (fileInfo.createdAt.isNotZero()) {
				if (!File::setCreatedTime(filePath, fileInfo.createdAt))
					throw getError();
			}
			if (fileInfo.modifiedAt.isNotZero()) {
				if (!File::setModifiedTime(filePath, fileInfo.modifiedAt))
					throw getError();
			}
			if (fileInfo.lastAccessedAt.isNotZero()) {
				if (!File::setAccessedTime(filePath, fileInfo.lastAccessedAt))
					throw getError();
			}
		}

		if (flags & FileInfoFlags::SizeInfo) {
			Ref<File> file = FileFromContext(context);
			if (file.isNotNull()) {
				if (!file->setSize(fileInfo.size))
					throw getError();
			}
			else {
				throw FileSystemError::InvalidContext;
			}
		}

		if (flags & FileInfoFlags::AllocSizeInfo) {
			Ref<File> file = FileFromContext(context);
			if (file.isNotNull()) {
				//if (!file->setAllocationSize(fileInfo.allocationSize))
				//	throw getError();
			}
			else {
				throw FileSystemError::InvalidContext;
			}
		}
	}

	sl_size MirrorFs::fsGetSecurity(FileContext* context, sl_uint32 securityInformation, const Memory& securityDescriptor)
	{
#ifdef SLIB_PLATFORM_IS_WIN32
		StringCstr16 fullPath(m_root + context->path);
		DWORD lengthNeeded;

		BOOL requestingSaclInfo = (
			(securityInformation & 0x00000008L/*SACL_SECURITY_INFORMATION*/) ||
			(securityInformation & 0x00010000L/*BACKUP_SECURITY_INFORMATION*/));

		HANDLE handle = CreateFile(
			(LPCWSTR)(fullPath.getData()),
			READ_CONTROL | (requestingSaclInfo ? ACCESS_SYSTEM_SECURITY : 0),
			FILE_SHARE_WRITE | FILE_SHARE_READ | FILE_SHARE_DELETE,
			NULL, // security attribute
			OPEN_EXISTING,
			FILE_FLAG_BACKUP_SEMANTICS, // |FILE_FLAG_NO_BUFFERING,
			NULL);

		if (!handle || handle == INVALID_HANDLE_VALUE)
			throw getError();

		if (!GetUserObjectSecurity(handle,
			(PSECURITY_INFORMATION)&securityInformation,
			(PSECURITY_DESCRIPTOR)securityDescriptor.getData(),
			(DWORD)securityDescriptor.getSize(),
			&lengthNeeded)) {
			if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
				CloseHandle(handle);
				return lengthNeeded;
			}
			else {
				CloseHandle(handle);
				throw getError();
			}
		}

		CloseHandle(handle);
		return GetSecurityDescriptorLength((PSECURITY_DESCRIPTOR)securityDescriptor.getData());
#else
		throw FileSystemError::NotImplemented;
#endif
	}

	void MirrorFs::fsSetSecurity(FileContext* context, sl_uint32 securityInformation, const Memory& securityDescriptor)
	{
#ifdef SLIB_PLATFORM_IS_WIN32
		Ref<File> file = FileFromContext(context);
		if (file.isNull()) {
			throw FileSystemError::InvalidContext;
		}

		if (!SetUserObjectSecurity((HANDLE)(file->getHandle()),
			(PSECURITY_INFORMATION)&securityInformation,
			(PSECURITY_DESCRIPTOR)securityDescriptor.getData()))
			throw getError();
#else
		throw FileSystemError::NotImplemented;
#endif
	}

	HashMap<String, FileInfo> MirrorFs::fsFindFiles(FileContext* context, String patternString)
	{
		String filePath = m_root + context->path;	// CHECK if filePath can be file
		HashMap<String, FileInfo> files;

		if (patternString.isNull() || patternString.isEmpty())
			patternString = "*";

		List<String> names = File::getFiles(filePath);	// TODO patternString, return FileInfos, return . and ..

		if (context->path.getLength() > 1) {
			// add . and ..
			files.add(".", fsGetFileInfo(new FileContext(context->path)));
			files.add("..", fsGetFileInfo(new FileContext(File::getParentDirectoryPath(context->path).replaceAll("/", "\\"))));
		}

		for (auto& name : names) {
			FileInfo info = fsGetFileInfo(new FileContext(context->path + "\\" + name));
			files.add(name, info);
		}

		return files;
	}

	HashMap<String, StreamInfo> MirrorFs::fsFindStreams(FileContext* context)
	{
#if 0
		auto funcFindFirstStream = api::getApi_FindFirstStreamW();
		auto funcFindNextStream = api::getApi_FindNextStreamW();
		if (!funcFindFirstStream || !funcFindNextStream)
			throw FileSystemError::NotImplemented;

		WCHAR fullPath[MAX_PATH];
		ULONG length;
		HANDLE findHandle;
		WIN32_FIND_STREAM_DATA findData;
		HashMap<String, StreamInfo> streams;

		ConcatPath(context->path, fullPath);
		length = (ULONG)wcslen(fullPath);

		findHandle = funcFindFirstStream(fullPath, FindStreamInfoStandard, &findData, 0);
		if (INVALID_HANDLE_VALUE == findHandle)
			throw getError();

		do {
			String streamName = WCharToString(findData.cStreamName);
			StreamInfo streamInfo;
			streamInfo.size = findData.StreamSize.QuadPart;
			streams.add(streamName, streamInfo);
		} while (funcFindNextStream(findHandle, &findData) != 0);

		FindClose(findHandle);

		if (GetLastError() != ERROR_HANDLE_EOF)
			throw getError();

		return streams;
#else
		throw FileSystemError::NotImplemented;
#endif
	}

	FileSystemError MirrorFs::getError(sl_uint32 error)
	{
		return (FileSystemError)(error == 0 ? System::getLastError() : error);
	}

}