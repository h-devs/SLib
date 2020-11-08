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

	FileContext::FileContext()
	{
	}

	FileContext::~FileContext()
	{
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
		SLIB_THROW(FileSystemError::NotImplemented, sl_false)
	}

	sl_uint32 FileSystemProvider::writeFile(FileContext* context, sl_int64 offset, const void* data, sl_uint32 size)
	{
		SLIB_THROW(FileSystemError::NotImplemented, 0)
	}

	sl_bool FileSystemProvider::flushFile(FileContext* context)
	{
		SLIB_THROW(FileSystemError::NotImplemented, sl_false)
	}

	sl_bool FileSystemProvider::closeFile(FileContext* context)
	{
		SLIB_THROW(FileSystemError::NotImplemented, sl_false)
	}

	sl_bool FileSystemProvider::deleteFile(const StringParam& path)
	{
		SLIB_THROW(FileSystemError::NotImplemented, sl_false)
	}

	sl_bool FileSystemProvider::moveFile(const StringParam& pathOld, const StringParam& pathNew, sl_bool flagReplaceIfExists)
	{
		SLIB_THROW(FileSystemError::NotImplemented, sl_false)
	}

	sl_bool FileSystemProvider::setFileInfo(const StringParam& path, FileContext* context, const FileInfo& info, const FileInfoMask& mask)
	{
		SLIB_THROW(FileSystemError::NotImplemented, sl_false)
	}

	sl_bool FileSystemProvider::createDirectory(const StringParam& path)
	{
		SLIB_THROW(FileSystemError::NotImplemented, sl_false)
	}

	sl_bool FileSystemProvider::deleteDirectory(const StringParam& path)
	{
		SLIB_THROW(FileSystemError::NotImplemented, sl_false)
	}

	sl_bool FileSystemProvider::existsFile(const StringParam& path) noexcept
	{
		SLIB_TRY {
			FileInfo info;
			if (getFileInfo(path, sl_null, info, FileInfoMask::Attributes)) {
				return sl_true;
			}
		} SLIB_CATCH(...)
		return sl_false;
	}

	sl_bool FileSystemProvider::getFileSize(FileContext* context, sl_uint64& outSize) noexcept
	{
		SLIB_TRY {
			FileInfo info;
			if (getFileInfo(sl_null, context, info, FileInfoMask::Size)) {
				outSize = info.size;
				return sl_true;
			}
		} SLIB_CATCH(...)
		return sl_false;
	}

	sl_bool FileSystemProvider::getFileSize(const StringParam& path, sl_uint64& outSize) noexcept
	{
		SLIB_TRY {
			FileInfo info;
			if (getFileInfo(path, sl_null, info, FileInfoMask::Size)) {
				outSize = info.size;
				return sl_true;
			}
		} SLIB_CATCH(...)
		return sl_false;
	}

	Memory FileSystemProvider::readFile(const StringParam& path, sl_uint64 offset, sl_uint32 size) noexcept
	{
		if (!size) {
			return sl_null;
		}

		Ref<FileContext> context;

		SLIB_TRY {
			
			FileOpenParam param;
			param.mode = FileMode::Read | FileMode::ShareRead;
			
			context = openFile(path, param);
			if (context.isNull()) {
				return sl_null;
			}

			FileInfo info;
			if (!getFileInfo(sl_null, context, info, FileInfoMask::Size)) {
				SLIB_TRY {
					closeFile(context);
				} SLIB_CATCH(...)
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

		} SLIB_CATCH(FileSystemError error, {
			LOG_DEBUG("ReadFile(%s,%d,%d)\n  Error: %d", path, offset, size, error);
			SLIB_UNUSED(error);
			if (context.isNotNull()) {
				SLIB_TRY {
					closeFile(context);
				} SLIB_CATCH(...)
			}
		}) SLIB_CATCH(...)

		return sl_null;
	}

	sl_uint32 FileSystemProvider::writeFile(const StringParam& path, const void* buf, sl_uint32 size) noexcept
	{
		Ref<FileContext> context;
		SLIB_TRY {
			FileOpenParam param;
			param.mode = FileMode::Write;
			context = openFile(path, param);
			if (context.isNull()) {
				return 0;
			}
			sl_uint32 sizeWritten = writeFile(context.get(), 0, buf, size);
			closeFile(context);
			return sizeWritten;
		} SLIB_CATCH (FileSystemError error, {
			LOG_DEBUG("WriteFile(%s,%d)\n  Error: %d", path, size, error);
			SLIB_UNUSED(error);
			if (context.isNotNull()) {
				SLIB_TRY {
					closeFile(context);
				} SLIB_CATCH(...)
			}
		}) SLIB_CATCH (...)
		
		return sl_false;
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


	FileSystemWrapper::FileSystemWrapper(const Ref<FileSystemProvider>& base, const String& fileSystemName, const String& volumeName) : m_base(base)
	{
		SLIB_TRY{
			base->getInformation(m_fsInfo);
		} SLIB_CATCH(...)
		if (fileSystemName.isNotNull()) {
			m_fsInfo.fileSystemName = fileSystemName;
		}
		if (volumeName.isNotNull()) {
			m_fsInfo.volumeName = volumeName;
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
			return createContext(baseContext.get(), path);
		}
		return sl_null;
	}

	sl_uint32 FileSystemWrapper::readFile(FileContext* context, sl_uint64 offset, void* buf, sl_uint32 size)
	{
		Ref<FileContext> baseContext = getBaseContext(context);
		if (baseContext.isNotNull()) {
			return m_base->readFile(baseContext.get(), offset, buf, size);
		} else {
			SLIB_THROW(FileSystemError::InvalidContext, 0)
		}
	}

	sl_uint32 FileSystemWrapper::writeFile(FileContext* context, sl_int64 offset, const void* buf, sl_uint32 size)
	{
		Ref<FileContext> baseContext = getBaseContext(context);
		if (baseContext.isNotNull()) {
			return m_base->writeFile(baseContext.get(), offset, buf, size);
		} else {
			SLIB_THROW(FileSystemError::InvalidContext, 0)
		}
	}

	sl_bool FileSystemWrapper::flushFile(FileContext* context)
	{
		Ref<FileContext> baseContext = getBaseContext(context);
		if (baseContext.isNotNull()) {
			return m_base->flushFile(baseContext.get());
		} else {
			SLIB_THROW(FileSystemError::InvalidContext, sl_false)
		}
	}

	sl_bool	FileSystemWrapper::closeFile(FileContext* context)
	{
		Ref<FileContext> baseContext = getBaseContext(context);
		if (baseContext.isNotNull()) {
			return m_base->closeFile(baseContext.get());
		} else {
			SLIB_THROW(FileSystemError::InvalidContext, sl_false)
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

	sl_bool FileSystemWrapper::getFileInfo(const StringParam& path, FileContext* context, FileInfo& info, const FileInfoMask& mask)
	{
		sl_bool ret = m_base->getFileInfo(toBasePath(path), getBaseContext(context), info, mask);
		if (ret) {
			convertToWrapperFileInfo(info, mask);
		}
		return ret;
	}

	sl_bool FileSystemWrapper::setFileInfo(const StringParam& path, FileContext* context, const FileInfo& _info, const FileInfoMask& mask)
	{
		FileInfo info = _info;
		convertToBaseFileInfo(info, mask);
		return m_base->setFileInfo(toBasePath(path), getBaseContext(context), info, mask);
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

	Ref<FileContext> FileSystemWrapper::createContext(FileContext* baseContext, const StringParam& path)
	{
		return baseContext;
	}

	Ref<FileContext> FileSystemWrapper::getBaseContext(FileContext* context)
	{
		return context;
	}

	String FileSystemWrapper::toBasePath(const StringParam& path)
	{
		return path.toString();
	}

	String FileSystemWrapper::toWrapperPath(const String& basePath, sl_bool flagNameOnly)
	{
		return basePath;
	}

	void FileSystemWrapper::convertToBaseFileInfo(FileInfo& info, const FileInfoMask& mask) noexcept
	{
	}

	void FileSystemWrapper::convertToWrapperFileInfo(FileInfo& baseInfo, const FileInfoMask& mask) noexcept
	{
	}

}
