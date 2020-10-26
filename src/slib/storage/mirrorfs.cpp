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
		m_volumeInfo.flags = FileSystemFlags::IsCaseSensitive;
			//| FileSystemFlags::SupportsUnicode | FileSystemFlags::SupportsSecurity;
	}

	sl_bool MirrorFs::fsGetVolumeSize(sl_uint64* pOutTotalSize, sl_uint64* pOutFreeSize)
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
				throw getError();
			}
			params.createAlways = sl_false;
			context->isDirectory = sl_true;
			return;
		}

#ifdef SLIB_PLATFORM_IS_WIN32
		StringCstr16 fullPath(m_root + context->path);

		DWORD creationDisposition = CREATE_NEW;
		LPSECURITY_ATTRIBUTES pSecurityAttributes = NULL;
		SECURITY_ATTRIBUTES securityAttributes;
		HANDLE handle;
		
		if (0 == params.accessMode)
			params.accessMode = GENERIC_READ | GENERIC_WRITE;
		if (0 == params.shareMode)
			params.shareMode = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
		if (params.createAlways) {
			creationDisposition = CREATE_ALWAYS;
			params.createAlways = FALSE;
			params.openTruncate = FALSE;
		}

		if (0 == (params.flagsAndAttributes & 0x0007FFFF))
			params.flagsAndAttributes |= FILE_ATTRIBUTE_NORMAL;
		params.flagsAndAttributes |= FILE_FLAG_BACKUP_SEMANTICS;

		if (params.securityDescriptor)
		{
			securityAttributes.nLength = sizeof securityAttributes;
			securityAttributes.lpSecurityDescriptor = params.securityDescriptor;
			securityAttributes.bInheritHandle = FALSE;
			pSecurityAttributes = &securityAttributes;
		}

		handle = CreateFile((LPCWSTR)(fullPath.getData()),
			params.accessMode, params.shareMode, pSecurityAttributes,
			creationDisposition, params.flagsAndAttributes, 0);

		if (INVALID_HANDLE_VALUE == handle)
			throw getError();

		Ref<File> file = new File((sl_file)handle);
#else
		Ref<File> file = File::open(m_root + context->path,
			FileMode::Write, // | (params.createAlways ? 0 : FileMode::NotCreate) | (params.openTruncate ? 0 : FileMode::NotTruncate),
			params.shareMode << 12 | FilePermissions::ShareAll | FilePermissions::All);
		if (file.isNull()) {
			throw getError();
		}
#endif

		file->increaseReference();
		context->handle = (sl_uint64)(file.ptr);
		context->status = getError();
	}

	void MirrorFs::fsOpen(FileContext* context, FileCreationParams& params)
	{
		if (context->isDirectory || params.attr.isDirectory) {
			if (!File::exists(m_root + context->path)) {
				if (params.createAlways) {
					if (!File::createDirectory(m_root + context->path)) {
						throw getError();
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

#ifdef SLIB_PLATFORM_IS_WIN32
		StringCstr16 fullPath(m_root + context->path);

		DWORD creationDisposition = OPEN_EXISTING;
		LPSECURITY_ATTRIBUTES pSecurityAttributes = NULL;
		SECURITY_ATTRIBUTES securityAttributes;
		HANDLE handle;

		if (0 == params.accessMode)
			params.accessMode = GENERIC_READ | GENERIC_WRITE;
		if (0 == params.shareMode)
			params.shareMode = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
		if (params.createAlways) {
			creationDisposition = OPEN_ALWAYS;
			if (params.openTruncate)
				creationDisposition = CREATE_ALWAYS;	// truncate
			params.createAlways = FALSE;
			params.openTruncate = FALSE;
		}
		else if (params.openTruncate) {
			creationDisposition = TRUNCATE_EXISTING;
			params.openTruncate = FALSE;
		}

		params.flagsAndAttributes |= FILE_FLAG_BACKUP_SEMANTICS;

		if (params.securityDescriptor)
		{
			securityAttributes.nLength = sizeof securityAttributes;
			securityAttributes.lpSecurityDescriptor = params.securityDescriptor;
			securityAttributes.bInheritHandle = FALSE;
			pSecurityAttributes = &securityAttributes;
		}

		handle = CreateFile((LPCWSTR)(fullPath.getData()),
			params.accessMode, params.shareMode, pSecurityAttributes,
			creationDisposition, params.flagsAndAttributes, 0);

		if (INVALID_HANDLE_VALUE == handle)
			throw getError();

		Ref<File> file = new File((sl_file)handle);
#else
		Ref<File> file = File::open(m_root + context->path,
			FileMode::ReadWrite | (params.createAlways ? 0 : FileMode::NotCreate) | (params.openTruncate ? 0 : FileMode::NotTruncate),
			params.shareMode << 12 | FilePermissions::ShareAll | FilePermissions::All);
		if (file.isNull()) {
			throw getError();
		}

		params.createAlways = sl_false;
		params.openTruncate = sl_false;
#endif

		file->increaseReference();
		context->handle = (sl_uint64)(file.ptr);
		context->status = getError();
	}

	void MirrorFs::fsClose(FileContext* context)
	{
		Ref<File> file = FileFromContext(context);
		if (file.isNotNull()) {
			file->close();
			if (file->isOpened())
				throw getError();
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
		if (file.isNull() || !file->isOpened()) {
			file = File::openForRead(m_root + context->path);
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
		if (file.isNull() || !file->isOpened()) {
			file = File::openForWrite(m_root + context->path);
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
			else {
#ifdef SLIB_PLATFORM_IS_WIN32
				Ref<File> file = FileFromContext(context);
				if (file.isNotNull()) {
					HANDLE handle = (HANDLE)(file->getHandle());
					FILE_DISPOSITION_INFO dispositionInfo;
					dispositionInfo.DeleteFile = TRUE;

					if (!SetFileInformationByHandle(handle,
						FileDispositionInfo, &dispositionInfo, sizeof dispositionInfo))
						throw getError();

					//dispositionInfo.DeleteFile = FALSE;
					//SetFileInformationByHandle(handle,
					//	FileDispositionInfo, &dispositionInfo, sizeof dispositionInfo);
				}
#endif
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
		if (file.isNotNull() && file->isOpened()) {
			if (!file->lock())
				throw getError();
		}
	}

	void MirrorFs::fsUnlock(FileContext* context, sl_uint64 offset, sl_uint64 length)
	{
		// TODO offset, length
		Ref<File> file = FileFromContext(context);
		if (file.isNotNull() && file->isOpened()) {
			if (!file->unlock())
				throw getError();
		}
	}

	FileInfo MirrorFs::fsGetFileInfo(FileContext* context)
	{
		FileInfo fileInfo;
		String filePath = m_root + context->path;
		Ref<File> file = FileFromContext(context);
		if (file.isNull() || !file->isOpened()) {
			file = File::openForRead(filePath);
		}

		FileAttributes attr = File::getAttributes(filePath);	// FIXME returns NotExist on error
		if (attr == FileAttributes::NotExist) {
#ifdef SLIB_PLATFORM_IS_WIN32
			if (file.isNotNull() && file->isOpened()) {
				HANDLE handle = (HANDLE)(file->getHandle());
				BY_HANDLE_FILE_INFORMATION byHandleFileInfo;

				if (GetFileInformationByHandle(handle, &byHandleFileInfo)) {
					fileInfo.fileAttributes = byHandleFileInfo.dwFileAttributes;
				}
				else
					throw getError();
			}
			else
#endif
				throw FileSystemError::NotFound;
		}
		else {
			fileInfo.fileAttributes = (sl_uint32)attr;
		}

		if (file.isNotNull() && file->isOpened()) {
			fileInfo.size = fileInfo.allocationSize = file->getSize();
			fileInfo.createdAt = file->getCreatedTime();
			fileInfo.modifiedAt = file->getModifiedTime();
			fileInfo.lastAccessedAt = file->getAccessedTime();
		}
		else {
			fileInfo.size = fileInfo.allocationSize = File::getSize(filePath);
			fileInfo.createdAt = File::getCreatedTime(filePath);
			fileInfo.modifiedAt = File::getModifiedTime(filePath);
			fileInfo.lastAccessedAt = File::getAccessedTime(filePath);
		}

		return fileInfo;
	}

	void MirrorFs::fsSetFileInfo(FileContext* context, FileInfo fileInfo, FileInfoFlags flags)
	{
		Ref<File> file = FileFromContext(context);
		String filePath = m_root + context->path;

		if (flags & FileInfoFlags::AttrInfo) {
			if (!File::setAttributes(filePath, fileInfo.fileAttributes))
				throw getError();
		}

		if (flags & FileInfoFlags::TimeInfo) {
			sl_bool isOpened = (file.isNotNull() && file->isOpened());
			if (fileInfo.createdAt.isNotZero()) {
				if (!isOpened || !file->setCreatedTime(fileInfo.createdAt)) {
					if (!File::setCreatedTime(filePath, fileInfo.createdAt))
						throw getError();
				}
			}
			if (fileInfo.modifiedAt.isNotZero()) {
				if (!isOpened || !file->setModifiedTime(fileInfo.modifiedAt)) {
					if (!File::setModifiedTime(filePath, fileInfo.modifiedAt))
						throw getError();
				}
			}
			if (fileInfo.lastAccessedAt.isNotZero()) {
				if (!isOpened || !file->setAccessedTime(fileInfo.lastAccessedAt)) {
					if (!File::setAccessedTime(filePath, fileInfo.lastAccessedAt))
						throw getError();
				}
			}
		}

		if (flags & FileInfoFlags::SizeInfo) {
			if (file.isNotNull() && file->isOpened()) {
				if (!file->setSize(fileInfo.size))
					throw getError();
			}
			else {
				throw FileSystemError::InvalidContext;
			}
		}

		if (flags & FileInfoFlags::AllocSizeInfo) {
			if (file.isNotNull() && file->isOpened()) {
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
				context->status = FileSystemError::BufferOverflow;
				return lengthNeeded;
			}
			else {
				CloseHandle(handle);
				throw getError();
			}
		}

		CloseHandle(handle);
		context->status = FileSystemError::Success;
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
			// FIXME HARDCODE add . and ..
			files.add(".", fsGetFileInfo(context));
			files.add("..", fsGetFileInfo(context));
		}

		for (auto& name : names) {
			try {
				FileInfo info = fsGetFileInfo(new FileContext(context->path + "\\" + name));
				files.add(name, info);
			}
			catch (...) {}
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