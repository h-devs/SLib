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

#ifdef SLIB_PLATFORM_IS_WIN32
#include "slib/storage/dokany.h"
#endif

namespace slib
{

	Ref<FileSystemHost> FileSystem::createHost()
	{
#if defined(SLIB_PLATFORM_IS_WIN32)
		return Dokany::createHost();
#elif defined(SLIB_PLATFORM_IS_UNIX) && defined(SLIB_PLATFORM_IS_DESKTOP)
#else
		return sl_null;
#endif
	}

	Ref<FileSystemHost> FileSystem::getHost(const String& mountPoint)
	{
		return sl_null;
	}

	sl_bool FileSystem::unmount(const String& mountPoint)
	{
#if defined(SLIB_PLATFORM_IS_WIN32)
		return Dokany::unmount(mountPoint);
#elif defined(SLIB_PLATFORM_IS_UNIX) && defined(SLIB_PLATFORM_IS_DESKTOP)
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


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(FileInfo)

	FileInfo::FileInfo() :
		size(0),
		allocSize(0)
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

	sl_bool FileSystemProvider::lockFile(FileContext* context, sl_uint64 offset, sl_uint64 length)
	{
		SLIB_THROW(FileSystemError::NotImplemented, sl_false)
	}

	sl_bool FileSystemProvider::unlockFile(FileContext* context, sl_uint64 offset, sl_uint64 length)
	{
		SLIB_THROW(FileSystemError::NotImplemented, sl_false)
	}

	sl_bool FileSystemProvider::setFileInfo(FileContext* context, const FileInfo& info, const FileInfoMask& mask)
	{
		SLIB_THROW(FileSystemError::NotImplemented, sl_false)
	}

	sl_bool FileSystemProvider::setFileInfo(const StringParam& path, const FileInfo& info, const FileInfoMask& mask)
	{
		SLIB_THROW(FileSystemError::NotImplemented, sl_false)
	}

	sl_bool FileSystemProvider::existsFile(const StringParam& path)
	{
		SLIB_TRY {
			FileInfo info;
			if (getFileInfo(path, info, FileInfoMask::Attributes)) {
				return sl_true;
			}
		} SLIB_CATCH(...)
		return sl_false;
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
		m_nOpendHandles = 0;
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
		m_param = param;
		m_flagRunning = sl_true;
		lock.unlock();
		sl_bool bRet = _run();
		m_flagRunning = sl_false;
		return bRet;
	}

	void FileSystemHost::stop()
	{
		if (!(m_flagRunning)) {
			return;
		}
		ObjectLocker lock(this);
		if (!(m_flagRunning)) {
			return;
		}
		_stop();
	}

	sl_size FileSystemHost::getOpenedHandlesCount()
	{
		return m_nOpendHandles;
	}

	Ref<FileContext> FileSystemHost::openFile(const StringParam& path, const FileOpenParam& param)
	{
		Ref<FileContext> context = m_param.provider->openFile(path, param);
		if (context.isNotNull()) {
			Base::interlockedIncrement(&m_nOpendHandles);
			return context;
		}
		return sl_null;
	}

	sl_bool FileSystemHost::closeFile(FileContext* context)
	{
		if (m_param.provider->closeFile(context)) {
			Base::interlockedDecrement(&m_nOpendHandles);
			return sl_true;
		}
		return sl_false;
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
		Ref<FileContext> context = m_base->openFile(path, param);
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
		return m_base->deleteFile(path);
	}

	sl_bool FileSystemWrapper::moveFile(const StringParam& pathOld, const StringParam& pathNew, sl_bool flagReplaceIfExists)
	{
		return m_base->moveFile(pathOld, pathNew, flagReplaceIfExists);
	}

	sl_bool FileSystemWrapper::lockFile(FileContext* context, sl_uint64 offset, sl_uint64 length)
	{
		Ref<FileContext> baseContext = getBaseContext(context);
		if (baseContext.isNotNull()) {
			return m_base->lockFile(baseContext.get(), offset, length);
		} else {
			SLIB_THROW(FileSystemError::InvalidContext, sl_false)
		}
	}

	sl_bool FileSystemWrapper::unlockFile(FileContext* context, sl_uint64 offset, sl_uint64 length)
	{
		Ref<FileContext> baseContext = getBaseContext(context);
		if (baseContext.isNotNull()) {
			return m_base->unlockFile(baseContext.get(), offset, length);
		} else {
			SLIB_THROW(FileSystemError::InvalidContext, sl_false)
		}
	}

	sl_bool FileSystemWrapper::getFileInfo(FileContext* context, FileInfo& info, const FileInfoMask& mask)
	{
		Ref<FileContext> baseContext = getBaseContext(context);
		if (baseContext.isNotNull()) {
			return m_base->getFileInfo(baseContext.get(), info, mask);
		} else {
			SLIB_THROW(FileSystemError::InvalidContext, sl_false)
		}
	}

	sl_bool FileSystemWrapper::setFileInfo(FileContext* context, const FileInfo& info, const FileInfoMask& mask)
	{
		Ref<FileContext> baseContext = getBaseContext(context);
		if (baseContext.isNotNull()) {
			return m_base->setFileInfo(baseContext.get(), info, mask);
		} else {
			SLIB_THROW(FileSystemError::InvalidContext, sl_false)
		}
	}

	sl_bool FileSystemWrapper::getFileInfo(const StringParam& path, FileInfo& info, const FileInfoMask& mask)
	{
		return m_base->getFileInfo(path, info, mask);
	}

	sl_bool FileSystemWrapper::setFileInfo(const StringParam& path, const FileInfo& info, const FileInfoMask& mask)
	{
		return m_base->setFileInfo(path, info, mask);
	}

	sl_bool FileSystemWrapper::createDirectory(const StringParam& path)
	{
		return m_base->createDirectory(path);
	}

	sl_bool FileSystemWrapper::deleteDirectory(const StringParam& path)
	{
		return m_base->deleteDirectory(path);
	}

	HashMap<String, FileInfo> FileSystemWrapper::getFiles(const StringParam& pathDir)
	{
		return m_base->getFiles(pathDir);
	}

	Ref<FileContext> FileSystemWrapper::createContext(FileContext* baseContext)
	{
		return baseContext;
	}

	Ref<FileContext> FileSystemWrapper::getBaseContext(FileContext* context)
	{
		return context;
	}

}