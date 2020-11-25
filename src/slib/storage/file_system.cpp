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

#include "slib/storage/file_system_internal.h"

#include "slib/core/system.h"
#include "slib/core/safe_static.h"

//#define WIN32_USE_FUSE

#ifdef SLIB_PLATFORM_IS_WIN32
# ifdef WIN32_USE_FUSE
#  include "slib/storage/fuse.h"
# else
#  include "slib/storage/dokany.h"
# endif
#else
# include "slib/storage/fuse.h"
#endif


namespace slib
{

	namespace priv
	{
		namespace file_system
		{

			typedef HashMap< String, FileSystemHost* > FileSystemHostMap;
			SLIB_SAFE_STATIC_GETTER(FileSystemHostMap, GetFileSystemHostMap)

		}
	}

	using namespace priv::file_system;

	Ref<FileSystemHost> FileSystem::createHost()
	{
#if defined(SLIB_PLATFORM_IS_WIN32)
# ifdef WIN32_USE_FUSE
		return Fuse::createHost();
# else
		return Dokany::createHost();
# endif
#elif defined(SLIB_PLATFORM_IS_UNIX) && defined(SLIB_PLATFORM_IS_DESKTOP)
        return Fuse::createHost();
#else
		return sl_null;
#endif
	}

	Ref<FileSystemHost> FileSystem::getHost(const String& mountPoint)
	{
		FileSystemHostMap* map = GetFileSystemHostMap();
		if (map) {
			return map->getValue(mountPoint);
		}
		return sl_null;
	}

	sl_bool FileSystem::unmount(const String& mountPoint)
	{
#if defined(SLIB_PLATFORM_IS_WIN32)
# ifdef WIN32_USE_FUSE
		return Fuse::unmount(mountPoint);
# else
		return Dokany::unmount(mountPoint);
# endif
#elif defined(SLIB_PLATFORM_IS_UNIX) && defined(SLIB_PLATFORM_IS_DESKTOP)
		return Fuse::unmount(mountPoint);
#else
		return sl_false;
#endif
	}

	FileSystemError FileSystem::getLastError()
	{
		return (FileSystemError)(System::getLastError());
	}

	void FileSystem::setLastError(FileSystemError error)
	{
		System::setLastError((sl_uint32)error);
	}


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(FileSystemInfo)

	FileSystemInfo::FileSystemInfo() :
		serialNumber(0),
		sectorSize(512),
		sectorsPerAllocationUnit(1),
		maxPathLength(8192)
	{
	}


	SLIB_DEFINE_OBJECT(FileContext, Object)

	FileContext::FileContext(String path, sl_uint64 handle)
		: path(path), handle(handle), flagUseRef(sl_false)
	{
	}

	FileContext::FileContext(String path, Ref<Referable> ref)
		: path(path), ptr(ref.ptr), flagUseRef(sl_true)
	{
		if (ptr) {
			ptr->increaseReference();
		}
	}

	FileContext::~FileContext()
	{
		if (ptr && flagUseRef) {
			ptr->decreaseReference();
		}
	}


	SLIB_DEFINE_OBJECT(FileSystemProvider, Object)

	FileSystemProvider::FileSystemProvider()
	{
	}

	FileSystemProvider::~FileSystemProvider()
	{
	}

	sl_bool FileSystemProvider::getInformation(FileSystemInfo& outInfo)
	{
		outInfo = m_fsInfo;
		return sl_true;
	}

	sl_bool FileSystemProvider::getSize(sl_uint64* pTotalSize, sl_uint64* pFreeSize)
	{
		SET_ERROR_AND_RETURN(FileSystemError::NotImplemented, sl_false)
	}

	sl_uint32 FileSystemProvider::writeFile(FileContext* context, sl_int64 offset, const void* data, sl_uint32 size)
	{
		SET_ERROR_AND_RETURN(FileSystemError::NotImplemented, 0)
	}

	sl_bool FileSystemProvider::flushFile(FileContext* context)
	{
		SET_ERROR_AND_RETURN(FileSystemError::NotImplemented, sl_false)
	}

	sl_bool FileSystemProvider::closeFile(FileContext* context)
	{
		SET_ERROR_AND_RETURN(FileSystemError::NotImplemented, sl_false)
	}

	sl_bool FileSystemProvider::deleteFile(const StringParam& path)
	{
		SET_ERROR_AND_RETURN(FileSystemError::NotImplemented, sl_false)
	}

	sl_bool FileSystemProvider::moveFile(const StringParam& pathOld, const StringParam& pathNew, sl_bool flagReplaceIfExists)
	{
		SET_ERROR_AND_RETURN(FileSystemError::NotImplemented, sl_false)
	}

	sl_bool FileSystemProvider::setFileInfo(FileContext* context, const FileInfo& info, const FileInfoMask& mask)
	{
		SET_ERROR_AND_RETURN(FileSystemError::NotImplemented, sl_false)
	}

	sl_bool FileSystemProvider::createDirectory(const StringParam& path)
	{
		SET_ERROR_AND_RETURN(FileSystemError::NotImplemented, sl_false)
	}

	sl_bool FileSystemProvider::deleteDirectory(const StringParam& path)
	{
		SET_ERROR_AND_RETURN(FileSystemError::NotImplemented, sl_false)
	}

	Ref<FileContext> FileSystemProvider::createContext(const StringParam& path, sl_uint64 handle) noexcept
	{
		return new FileContext(path.toString(), handle);
	}

	Ref<FileContext> FileSystemProvider::createContext(const StringParam& path, Ref<Referable> ref) noexcept
	{
		return new FileContext(path.toString(), ref);
	}

	sl_bool FileSystemProvider::getFileInfo(const StringParam& path, FileInfo& outInfo, const FileInfoMask& mask) noexcept
	{
		Ref<FileContext> context = createContext(path);
		return getFileInfo(context, outInfo, mask);
	}

	sl_bool FileSystemProvider::setFileInfo(const StringParam& path, const FileInfo& info, const FileInfoMask& mask) noexcept
	{
		Ref<FileContext> context = createContext(path);
		return setFileInfo(context, info, mask);
	}

	sl_bool FileSystemProvider::getFileSize(FileContext* context, sl_uint64& outSize) noexcept
	{
		FileInfo info;
		if (getFileInfo(context, info, FileInfoMask::Size)) {
			outSize = info.size;
			return sl_true;
		}
		return sl_false;
	}

	sl_bool FileSystemProvider::getFileSize(const StringParam& path, sl_uint64& outSize) noexcept
	{
		FileInfo info;
		if (getFileInfo(path, info, FileInfoMask::Size)) {
			outSize = info.size;
			return sl_true;
		}
		return sl_false;
	}

	sl_bool FileSystemProvider::existsFile(const StringParam& path) noexcept
	{
		FileInfo info;
		if (getFileInfo(path, info, FileInfoMask::Attributes)) {
			return sl_true;
		}
		return sl_false;
	}

	Memory FileSystemProvider::readFile(const StringParam& path, sl_uint64 offset, sl_uint32 size) noexcept
	{
		if (!size) {
			return sl_null;
		}

		FileOpenParam param;
		param.mode = FileMode::Read | FileMode::ShareRead;

		Ref<FileContext> context = openFile(path, param);
		if (context.isNull()) {
			return sl_null;
		}

		FileInfo info;
		if (!getFileInfo(context, info, FileInfoMask::Size)) {
			closeFile(context);
			return sl_null;
		}

		sl_uint64 limit = (sl_uint64)(info.size - offset);
		if (limit > 0x40000000) {
			limit = 0x40000000;
		}
		if (size > limit) {
			size = (sl_uint32)limit;
		}

		Memory mem = Memory::create(size);
		if (mem.isNull()) {
			return sl_null;
		}

		sl_uint32 sizeRead = readFile(context, offset, mem.getData(), size);
		closeFile(context);
		if (sizeRead) {
			return mem.sub(0, sizeRead);
		}

		return sl_null;
	}

	sl_uint32 FileSystemProvider::writeFile(const StringParam& path, const void* buf, sl_uint32 size) noexcept
	{
		FileOpenParam param;
		param.mode = FileMode::Write;

		Ref<FileContext> context = openFile(path, param);
		if (context.isNull()) {
			return 0;
		}
		sl_uint32 sizeWritten = writeFile(context.get(), 0, buf, size);
		closeFile(context);
		return sizeWritten;
	}

	sl_uint32 FileSystemProvider::writeFile(const StringParam& path, const Memory& mem) noexcept
	{
		sl_size size = mem.getSize();
		if (size > 0x40000000) {
			size = 0x40000000;
		}
		return writeFile(path, mem.getData(), (sl_uint32)size);
	}


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(FileSystemHostParam)

	FileSystemHostParam::FileSystemHostParam()
	{
		threadCount = 0;
		timeout = 0;
		flags = 0;
	}

	SLIB_DEFINE_OBJECT(FileSystemHost, Object)

	FileSystemHost::FileSystemHost()
	{
		m_flagRunning = sl_false;
		m_nOpenedHandles = 0;
	}

	FileSystemHost::~FileSystemHost()
	{
	}

	String FileSystemHost::getMountPoint()
	{
		return m_param.mountPoint;
	}

	FileSystemProvider* FileSystemHost::getProvider()
	{
		return m_param.provider.get();
	}

	sl_bool FileSystemHost::isRunning()
	{
		return m_flagRunning;
	}

	sl_bool FileSystemHost::run(const FileSystemHostParam& param)
	{
		FileSystemHostMap* map = GetFileSystemHostMap();
		if (!map) {
			return sl_false;
		}
		if (param.mountPoint.isEmpty() || param.provider.isNull()) {
			return sl_false;
		}
		ObjectLocker lock(this);
		if (m_flagRunning) {
			return sl_false;
		}
		if (map->find(param.mountPoint)) {
			return sl_false;
		}
		m_param = param;
		m_flagRunning = sl_true;
		map->put(param.mountPoint, this);
		lock.unlock();
		sl_bool bRet = _run();
		map->remove(param.mountPoint);
		m_flagRunning = sl_false;
		return bRet;
	}

	sl_size FileSystemHost::getOpenedHandlesCount()
	{
		return m_nOpenedHandles;
	}

	sl_size FileSystemHost::increaseOpenHandlesCount()
	{
		return Base::interlockedIncrement(&m_nOpenedHandles);
	}

	sl_size FileSystemHost::decreaseOpenHandlesCount()
	{
		return Base::interlockedDecrement(&m_nOpenedHandles);
	}


	FileSystemWrapper::FileSystemWrapper(const Ref<FileSystemProvider>& base, 
		const String& fileSystemName, const String& volumeName,
		sl_uint32 serialNumber) : m_base(base)
	{
		base->getInformation(m_fsInfo);
		
		if (fileSystemName.isNotNull()) {
			m_fsInfo.fileSystemName = fileSystemName;
		}
		if (volumeName.isNotNull()) {
			m_fsInfo.volumeName = volumeName;
		}
		if (serialNumber != SLIB_UINT32_MAX) {
			m_fsInfo.serialNumber = serialNumber;
		}
	}

	FileSystemWrapper::~FileSystemWrapper()
	{
	}

	sl_bool FileSystemWrapper::getInformation(FileSystemInfo& outInfo)
	{
		outInfo = m_fsInfo;
		return sl_true;
	}

	sl_bool FileSystemWrapper::getSize(sl_uint64* pTotalSize, sl_uint64* pFreeSize)
	{
		return m_base->getSize(pTotalSize, pFreeSize);
	}

	Ref<FileContext> FileSystemWrapper::openFile(const StringParam& path, const FileOpenParam& param)
	{
		Ref<FileContext> baseContext = m_base->openFile(toBasePath(path), param);
		if (baseContext.isNotNull()) {
			return getWrapperContext(baseContext, path);
		}
		return sl_null;
	}

	sl_uint32 FileSystemWrapper::readFile(FileContext* context, sl_uint64 offset, void* buf, sl_uint32 size)
	{
		Ref<FileContext> baseContext = getBaseContext(context);
		if (baseContext.isNotNull()) {
			return m_base->readFile(baseContext.get(), offset, buf, size);
		} else {
			SET_ERROR_AND_RETURN(FileSystemError::InvalidContext, 0)
		}
	}

	sl_uint32 FileSystemWrapper::writeFile(FileContext* context, sl_int64 offset, const void* buf, sl_uint32 size)
	{
		Ref<FileContext> baseContext = getBaseContext(context);
		if (baseContext.isNotNull()) {
			return m_base->writeFile(baseContext.get(), offset, buf, size);
		} else {
			SET_ERROR_AND_RETURN(FileSystemError::InvalidContext, 0)
		}
	}

	sl_bool FileSystemWrapper::flushFile(FileContext* context)
	{
		Ref<FileContext> baseContext = getBaseContext(context);
		if (baseContext.isNotNull()) {
			return m_base->flushFile(baseContext.get());
		} else {
			SET_ERROR_AND_RETURN(FileSystemError::InvalidContext, sl_false)
		}
	}

	sl_bool FileSystemWrapper::closeFile(FileContext* context)
	{
		Ref<FileContext> baseContext = getBaseContext(context);
		if (baseContext.isNotNull()) {
			return m_base->closeFile(baseContext.get());
		} else {
			SET_ERROR_AND_RETURN(FileSystemError::InvalidContext, sl_false)
		}
	}

	sl_bool FileSystemWrapper::deleteFile(const StringParam& path)
	{
		return m_base->deleteFile(toBasePath(path));
	}

	sl_bool FileSystemWrapper::moveFile(const StringParam& pathOld, const StringParam& pathNew, sl_bool flagReplaceIfExists)
	{
		return m_base->moveFile(toBasePath(pathOld), toBasePath(pathNew), flagReplaceIfExists);
	}

	sl_bool FileSystemWrapper::getFileInfo(FileContext* context, FileInfo& info, const FileInfoMask& mask)
	{
		sl_bool ret = m_base->getFileInfo(getBaseContext(context), info, mask);
		if (ret) {
			if (!convertToWrapperFileInfo(info, mask)) {
				SET_ERROR_AND_RETURN(FileSystemError::AccessDenied, sl_false);
			}
		}
		return ret;
	}

	sl_bool FileSystemWrapper::setFileInfo(FileContext* context, const FileInfo& _info, const FileInfoMask& mask)
	{
		FileInfo info = _info;
		if (!convertToBaseFileInfo(info, mask)) {
			SET_ERROR_AND_RETURN(FileSystemError::AccessDenied, sl_false);
		}
		return m_base->setFileInfo(getBaseContext(context), info, mask);
	}

	sl_bool FileSystemWrapper::createDirectory(const StringParam& path)
	{
		return m_base->createDirectory(toBasePath(path));
	}

	sl_bool FileSystemWrapper::deleteDirectory(const StringParam& path)
	{
		return m_base->deleteDirectory(toBasePath(path));
	}

	HashMap<String, FileInfo> FileSystemWrapper::getFiles(const StringParam& pathDir)
	{
		HashMap<String, FileInfo> files;

		String pathDirBase = toBasePath(pathDir);
		HashMap<String, FileInfo> filesBase = m_base->getFiles(pathDirBase);

		pathDirBase = File::normalizeDirectoryPath(pathDirBase);

		for (auto& file : filesBase) {
			String name = toWrapperPath(file.key, sl_true);
			if (name.isEmpty()) {
				String path = toWrapperPath(String::join(pathDirBase, "/", file.key), sl_false);
				if (path.isEmpty()) {
					// ignore this entry
					continue;
				}
				name = File::getFileName(path);
				if (name.isEmpty()) {
					// ignore this entry
					continue;
				}
			}

			FileInfo& info = file.value;
			convertToWrapperFileInfo(info, FileInfoMask::All);
			files.add_NoLock(name, info);
		}
		return files;
	}

	Ref<FileContext> FileSystemWrapper::createContext(const StringParam& path, sl_uint64 handle) noexcept
	{
		return getWrapperContext(m_base->createContext(toBasePath(path), handle), path);
	}

	Ref<FileContext> FileSystemWrapper::createContext(const StringParam& path, Ref<Referable> ref) noexcept
	{
		return getWrapperContext(m_base->createContext(toBasePath(path), ref), path);
	}

	Ref<FileContext> FileSystemWrapper::getBaseContext(FileContext* context) noexcept
	{
		return context;
	}

	Ref<FileContext> FileSystemWrapper::getWrapperContext(FileContext* baseContext, const StringParam& path) noexcept
	{
		return baseContext;
	}

	String FileSystemWrapper::toBasePath(const StringParam& path) noexcept
	{
		return path.toString();
	}

	String FileSystemWrapper::toWrapperPath(const String& basePath, sl_bool flagNameOnly) noexcept
	{
		return basePath;
	}

	sl_bool FileSystemWrapper::convertToBaseFileInfo(FileInfo& info, const FileInfoMask& mask) noexcept
	{
		return sl_true;
	}

	sl_bool FileSystemWrapper::convertToWrapperFileInfo(FileInfo& baseInfo, const FileInfoMask& mask) noexcept
	{
		return sl_true;
	}

}
