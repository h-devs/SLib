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

#include "file_system.h"

#include "slib/core/safe_static.h"

#ifdef SLIB_PLATFORM_IS_WIN32
#include "slib/storage/dokany.h"
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
		return Dokany::createHost();
#elif defined(SLIB_PLATFORM_IS_UNIX) && defined(SLIB_PLATFORM_IS_DESKTOP)
		return sl_null;
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
		return Dokany::unmount(mountPoint);
#elif defined(SLIB_PLATFORM_IS_UNIX) && defined(SLIB_PLATFORM_IS_DESKTOP)
		return sl_false;
#else
		return sl_false;
#endif
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

	sl_size FileSystemProvider::writeFile(FileContext* context, sl_int64 offset, const void* data, sl_size size)
	{
		SLIB_THROW(FileSystemError::NotImplemented, 0)
	}

	sl_bool FileSystemProvider::flushFile(FileContext* context)
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

	sl_bool FileSystemProvider::deleteDirectory(const StringParam& path)
	{
		SLIB_THROW(FileSystemError::NotImplemented, sl_false)
	}

	sl_bool FileSystemProvider::existsFile(const StringParam& path)
	{
		SLIB_TRY {
			FileInfo info;
			if (getFileInfo(path, sl_null, info, FileInfoMask::Attributes)) {
				return sl_true;
			}
		} SLIB_CATCH(...)
		return sl_false;
	}

	sl_uint64 FileSystemProvider::getFileSize(FileContext* context)
	{
		FileInfo info;
		if (getFileInfo(sl_null, context, info, FileInfoMask::Size)) {
			return info.size;
		}
		return 0;
	}

	sl_uint64 FileSystemProvider::getFileSize(const StringParam& path)
	{
		FileInfo info;
		if (getFileInfo(path, sl_null, info, FileInfoMask::Size)) {
			return info.size;
		}
		return 0;
	}


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(FileSystemHostParam)

	FileSystemHostParam::FileSystemHostParam()
	{
		threadCount = 0;
		timeout = 0;
		flagDebugMode = sl_false;
		flagUseStderr = sl_false;
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
		if (m_flagRunning) {
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


	FileSystemWrapper::FileSystemWrapper(const Ref<FileSystemProvider>& base) : m_base(base)
	{
	}

	FileSystemWrapper::~FileSystemWrapper()
	{
	}

	sl_bool FileSystemWrapper::getInformation(FileSystemInfo& outInfo, const FileSystemInfoMask& mask)
	{
		return m_base->getInformation(outInfo, mask);
	}

	Ref<FileContext> FileSystemWrapper::openFile(const StringParam& path, const FileOpenParam& param)
	{
		Ref<FileContext> context = m_base->openFile(getBaseFileName(path), param);
		if (context.isNotNull()) {
			return createContext(context.get());
		}
		return sl_null;
	}

	sl_size	FileSystemWrapper::readFile(FileContext* context, sl_uint64 offset, void* buf, sl_size size)
	{
		Ref<FileContext> baseContext = getBaseContext(context);
		if (baseContext.isNotNull()) {
			return m_base->readFile(baseContext.get(), offset, buf, size);
		} else {
			SLIB_THROW(FileSystemError::InvalidContext, 0)
		}
	}

	sl_size FileSystemWrapper::writeFile(FileContext* context, sl_int64 offset, const void* buf, sl_size size)
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
		return m_base->deleteFile(getBaseFileName(path));
	}

	sl_bool FileSystemWrapper::moveFile(const StringParam& pathOld, const StringParam& pathNew, sl_bool flagReplaceIfExists)
	{
		return m_base->moveFile(getBaseFileName(pathOld), getBaseFileName(pathNew), flagReplaceIfExists);
	}

	sl_bool FileSystemWrapper::getFileInfo(const StringParam& path, FileContext* context, FileInfo& info, const FileInfoMask& mask)
	{
		Ref<FileContext> baseContext = getBaseContext(context);
		if (baseContext.isNotNull()) {
			return m_base->getFileInfo(getBaseFileName(path), baseContext.get(), info, mask);
		} else {
			SLIB_THROW(FileSystemError::InvalidContext, sl_false)
		}
	}

	sl_bool FileSystemWrapper::setFileInfo(const StringParam& path, FileContext* context, const FileInfo& info, const FileInfoMask& mask)
	{
		Ref<FileContext> baseContext = getBaseContext(context);
		if (baseContext.isNotNull()) {
			return m_base->setFileInfo(getBaseFileName(path), baseContext.get(), info, mask);
		} else {
			SLIB_THROW(FileSystemError::InvalidContext, sl_false)
		}
	}

	sl_bool FileSystemWrapper::createDirectory(const StringParam& path)
	{
		return m_base->createDirectory(getBaseFileName(path));
	}

	sl_bool FileSystemWrapper::deleteDirectory(const StringParam& path)
	{
		return m_base->deleteDirectory(getBaseFileName(path));
	}

	HashMap<String, FileInfo> FileSystemWrapper::getFiles(const StringParam& pathDir)
	{
		return m_base->getFiles(getBaseFileName(pathDir));
	}

	sl_bool FileSystemWrapper::existsFile(const StringParam& path)
	{
		return m_base->existsFile(getBaseFileName(path));
	}

	sl_uint64 FileSystemWrapper::getFileSize(FileContext* context)
	{
		return m_base->getFileSize(getBaseContext(context));
	}

	sl_uint64 FileSystemWrapper::getFileSize(const StringParam& path)
	{
		return m_base->getFileSize(getBaseFileName(path));
	}

	Ref<FileContext> FileSystemWrapper::createContext(FileContext* baseContext)
	{
		return baseContext;
	}

	Ref<FileContext> FileSystemWrapper::getBaseContext(FileContext* context)
	{
		return context;
	}

	String FileSystemWrapper::getBaseFileName(const StringParam& path)
	{
		return path.toString();
	}

}