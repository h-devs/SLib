#include <windows.h>
#include "slib/core/file.h"
#include "slib/core/system.h"
#include "slib/core/throw.h"
#include "slib/core/variant.h"
#include "slib/storage/mirrorfs.h"

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

		MirrorFileContext(HANDLE handle)
		{
			file = new File((sl_file)handle);
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
			outInfo.flags = FileSystemFlags::IsCaseSensitive;
		}

		if (mask & FileSystemInfoMask::Size) {
			StringCstr16 root(m_root);
			ULARGE_INTEGER totalSize, freeSize;

			if (!GetDiskFreeSpaceExW((LPCWSTR)(root.getData()), 0, &totalSize, &freeSize)) {
				return sl_false;
			}

			outInfo.totalSize = totalSize.QuadPart;
			outInfo.freeSize = freeSize.QuadPart;
		}

		return sl_true;
	}

	Ref<FileContext> MirrorFs::openFile(const String& path, const FileOpenParam& param)
	{
		String filePath = m_root + path;
		if (param.attributes & FileAttributes::Directory) {
			if (param.mode.NotCreate == sl_false) {
				if (!File::createDirectory(filePath)) {
					throw getError();
				}
			}
			if (!File::exists(filePath))
				throw FileSystemError::NotFound;
			return new MirrorFileContext();
		}

#if 0
		StringCstr16 fullPath(m_root + path);

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

		DWORD flags = (params.attributes & 0x7ffff) | FILE_FLAG_BACKUP_SEMANTICS;
		if (params.attributes & FileAttributes::Directory) {
			flags |= FILE_FLAG_POSIX_SEMANTICS;
		}
		if (!(flags & 0x7ffff)) {
			flags |= FILE_ATTRIBUTE_NORMAL;
		}
		if (params.securityDescriptor) {
			securityAttributes.nLength = sizeof securityAttributes;
			securityAttributes.lpSecurityDescriptor = params.securityDescriptor;
			securityAttributes.bInheritHandle = FALSE;
			pSecurityAttributes = &securityAttributes;
		}

		handle = CreateFileW((LPCWSTR)(fullPath.getData()),
			params.accessMode, params.shareMode, pSecurityAttributes,
			creationDisposition, flags, 0);

		if (INVALID_HANDLE_VALUE == handle)
			throw getError();

		Ref<File> file = new File((sl_file)handle);
#endif

		Ref<File> file = File::open(m_root + path, param);
		if (file.isNull()) {
			SLIB_THROW(getError(), sl_null);
		}

		Ref<MirrorFileContext> context = new MirrorFileContext(file);
		context->increaseReference();
		return context;
	}
	/*
	void MirrorFs::openFile(FileContext* context, FileCreationParams& params)
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

		DWORD flags = (params.attributes & 0x7ffff) | FILE_FLAG_BACKUP_SEMANTICS;
		if (!(flags & 0x7ffff)) {
			flags |= FILE_ATTRIBUTE_NORMAL;
		}

		if (params.securityDescriptor)
		{
			securityAttributes.nLength = sizeof securityAttributes;
			securityAttributes.lpSecurityDescriptor = params.securityDescriptor;
			securityAttributes.bInheritHandle = FALSE;
			pSecurityAttributes = &securityAttributes;
		}

		handle = CreateFile((LPCWSTR)(fullPath.getData()),
			params.accessMode, params.shareMode, pSecurityAttributes,
			creationDisposition, flags, 0);

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
	*/
	sl_bool MirrorFs::closeFile(FileContext* context)
	{
		Ref<File> file = FileFromContext(context);
		if (file.isNotNull()) {
			file->close();
			if (file->isOpened())
				SLIB_THROW(getError(), sl_false);
			context->decreaseReference();
		}

		return sl_true;
	}

	sl_size MirrorFs::readFile(FileContext* context, sl_uint64 offset, void* buf, sl_size size)
	{
		// if (context->isDirectory) {
		// 	throw FileSystemError::AccessDenied;
		// }

		Ref<File> file = FileFromContext(context);
		if (file.isNull() || !file->isOpened()) {
			//file = File::openForRead(m_root + context->path);
		}
		if (file.isNull()) {
			SLIB_THROW(getError(), 0);
		}

		if (!file->seek(offset, SeekPosition::Begin))
			SLIB_THROW(getError(), 0);

		//auto ret = file->read(buf, size);	// IReader::read() only supports 1GB ?
		auto ret = file->read32(buf, (sl_uint32)size);	// File::read32() returns -1 if zero byte read ?
		if (ret < 0) {
			SLIB_THROW(getError(), 0);
		}

		return ret;
	}

	sl_size MirrorFs::writeFile(FileContext* context, sl_int64 offset, const void* buf, sl_size size)
	{
		// if (context->isDirectory) {
		// 	throw FileSystemError::AccessDenied;
		// }

		Ref<File> file = FileFromContext(context);
		if (file.isNull() || !file->isOpened()) {
			//file = File::openForWrite(m_root + context->path);
		}
		if (file.isNull()) {
			SLIB_THROW(getError(), 0);
		}

		if (offset < 0) {
			if (!file->seekToEnd())
				SLIB_THROW(getError(), 0);
		}
		else {
			if (!file->seek(offset, SeekPosition::Begin))
				SLIB_THROW(getError(), 0);
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

	sl_bool MirrorFs::deleteFile(const String& path)
	{
		if (!File::deleteFile(m_root + path, sl_true)) {
			SLIB_THROW(getError(), sl_false);
		}
		return sl_true;
	}

	sl_bool MirrorFs::moveFile(const String& oldPath, const String& newPath, sl_bool flagReplaceIfExists)
	{
		// TODO replaceIfExists
		if (!File::rename(m_root + oldPath, m_root + newPath)) {
			SLIB_THROW(getError(), sl_false);
		}
		return sl_true;
	}

	sl_bool MirrorFs::lockFile(FileContext* context, sl_uint64 offset, sl_uint64 length)
	{
		// TODO offset, length
		Ref<File> file = FileFromContext(context);
		if (file.isNotNull() && file->isOpened()) {
			if (!file->lock())
				SLIB_THROW(getError(), sl_false);
		}
		return sl_true;
	}

	sl_bool MirrorFs::unlockFile(FileContext* context, sl_uint64 offset, sl_uint64 length)
	{
		// TODO offset, length
		Ref<File> file = FileFromContext(context);
		if (file.isNotNull() && file->isOpened()) {
			if (!file->unlock())
				SLIB_THROW(getError(), sl_false);
		}
		return sl_true;
	}

	sl_bool MirrorFs::getFileInfo(const String& path, FileInfo& outInfo, const FileInfoMask& mask)
	{
		String filePath = m_root + path;
		FileAttributes attr = File::getAttributes(filePath);	// FIXME returns NotExist on error
		if (attr == FileAttributes::NotExist) {
#if 0
			if (file.isNotNull() && file->isOpened()) {
				HANDLE handle = (HANDLE)(file->getHandle());
				BY_HANDLE_FILE_INFORMATION byHandleFileInfo;

				if (GetFileInformationByHandle(handle, &byHandleFileInfo)) {
					outInfo.fileAttributes = byHandleFileInfo.dwFileAttributes;
				}
				else
					throw getError();
			}
			else
#endif
				SLIB_THROW(FileSystemError::NotFound, sl_false);
		}
		else {
			outInfo.attributes = (sl_uint32)attr;
		}

		// if (file.isNotNull() && file->isOpened()) {
		// 	outInfo.size = outInfo.allocSize = file->getSize();
		// 	outInfo.createdAt = file->getCreatedTime();
		// 	outInfo.modifiedAt = file->getModifiedTime();
		// 	outInfo.lastAccessedAt = file->getAccessedTime();
		// }
		// else {
			if ((mask & FileInfoMask::Size) || (mask & FileInfoMask::AllocSize)) {
				outInfo.size = outInfo.allocSize = File::getSize(filePath);
			}
			if (mask & FileInfoMask::Time) {
				outInfo.createdAt = File::getCreatedTime(filePath);
				outInfo.modifiedAt = File::getModifiedTime(filePath);
				outInfo.lastAccessedAt = File::getAccessedTime(filePath);
			}
		// }

		return sl_true;
	}

	sl_bool MirrorFs::setFileInfo(const String& path, const FileInfo& info, const FileInfoMask& mask)
	{
		String filePath = m_root + path;

		if (mask & FileInfoMask::Attributes) {
			if (!File::setAttributes(filePath, info.attributes))
				SLIB_THROW(getError(), sl_false);
		}

		if (mask & FileInfoMask::Time) {
			// sl_bool isOpened = (file.isNotNull() && file->isOpened());
			if (info.createdAt.isNotZero()) {
			// 	if (!isOpened || !file->setCreatedTime(info.createdAt)) {
					if (!File::setCreatedTime(filePath, info.createdAt))
						SLIB_THROW(getError(), sl_false);
				// }
			}
			if (info.modifiedAt.isNotZero()) {
			// 	if (!isOpened || !file->setModifiedTime(info.modifiedAt)) {
					if (!File::setModifiedTime(filePath, info.modifiedAt))
						SLIB_THROW(getError(), sl_false);
			// 	}
			}
			if (info.lastAccessedAt.isNotZero()) {
			// 	if (!isOpened || !file->setAccessedTime(info.lastAccessedAt)) {
					if (!File::setAccessedTime(filePath, info.lastAccessedAt))
						SLIB_THROW(getError(), sl_false);
			// 	}
			}
		}

		if (mask & FileInfoMask::Size) {
			if (file.isNotNull() && file->isOpened()) {
				if (!file->setSize(info.size)) {
					SLIB_THROW(getError(), sl_false);
				}
			}
			else {
				SLIB_THROW(FileSystemError::InvalidContext, sl_false);
			}
		}

		if (mask & FileInfoMask::AllocSize) {
			// if (file.isNotNull() && file->isOpened()) {
			// 	//if (!file->setAllocationSize(info.allocationSize))
			// 	//	SLIB_THROW(getError(), sl_false);
			// }
			// else {
			// 	SLIB_THROW(FileSystemError::InvalidContext, sl_false);
			// }
		}

		return sl_true;
	}

#if 0
	sl_size MirrorFs::fsGetSecurity(FileContext* context, sl_uint32 securityInformation, const Memory& securityDescriptor)
	{
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
	}

	void MirrorFs::fsSetSecurity(FileContext* context, sl_uint32 securityInformation, const Memory& securityDescriptor)
	{
		Ref<File> file = FileFromContext(context);
		if (file.isNull()) {
			throw FileSystemError::InvalidContext;
		}

		if (!SetUserObjectSecurity((HANDLE)(file->getHandle()),
			(PSECURITY_INFORMATION)&securityInformation,
			(PSECURITY_DESCRIPTOR)securityDescriptor.getData()))
			throw getError();
	}
#endif

	HashMap<String, FileInfo> MirrorFs::getFiles(const String& pathDir)
	{
		String filePath = m_root + pathDir;
		HashMap<String, FileInfo> files;

		List<String> names = File::getFiles(m_root + pathDir);	// TODO return FileInfos, return . and ..

		if (pathDir.getLength() > 1) {
			// FIXME HARDCODE add . and ..
			FileInfo info;
			if (!getFileInfo(pathDir, info, FileInfoMask::All)) {
				SLIB_THROW(getError(), sl_null);
			}
			files.add(".", info);
			files.add("..", info);
		}

		for (auto& name : names) {
			try {
				FileInfo info;
				if (getFileInfo(pathDir + "\\" + name, info, FileInfoMask::All)) {
					files.add(name, info);
				}
			}
			catch (...) {}
		}

		return files;
	}

#if 0
	HashMap<String, StreamInfo> MirrorFs::fsFindStreams(FileContext* context)
	{
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
	}
#endif

	FileSystemError MirrorFs::getError(sl_uint32 error)
	{
		return (FileSystemError)(error == 0 ? System::getLastError() : error);
	}

}