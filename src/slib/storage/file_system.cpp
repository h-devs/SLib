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

namespace slib
{

	Ref<FileSystemHost> FileSystem::mount(const String& mountPoint, const Ref<FileSystemProvider>& provider)
	{
		return sl_null;
	}

	Ref<FileSystemHost> getHost(const String& mountPoint)
	{
		return sl_null;
	}

	void FileSystem::unmount(const String& mountPoint)
	{

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

	sl_bool FileSystemProvider::deleteFile(const String& path)
	{
		SLIB_THROW(FileSystemError::NotImplemented, sl_false)
	}

	sl_bool FileSystemProvider::moveFile(const String& pathOld, const String& pathNew, sl_bool flagReplaceIfExists)
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

	sl_bool FileSystemProvider::setFileInfo(const String& filePath, const FileInfo& info, const FileInfoMask& mask)
	{
		SLIB_THROW(FileSystemError::NotImplemented, sl_false)
	}


	SLIB_DEFINE_OBJECT(FileSystemHost, Object)

	FileSystemHost::FileSystemHost(const String& mountPoint, const Ref<FileSystemProvider>& provider): m_mountPoint(mountPoint), m_provider(provider)
	{
	}

	FileSystemHost::~FileSystemHost()
	{
	}

	String FileSystemHost::getMountPoint()
	{
		return m_mountPoint;
	}

	Ref<FileSystemProvider> FileSystemHost::getProvider()
	{
		return m_provider;
	}


	FileSystemWrapper::FileSystemWrapper(const Ref<FileSystemProvider>& base) : m_base(base)
	{
	}

	FileSystemWrapper::~FileSystemWrapper()
	{
	}

	sl_bool FileSystemWrapper::getInformation(FileSystemInfo& info, const FileSystemInfoMask& mask)
	{
		return m_base->getInformation(info, mask);
	}

	Ref<FileContext> FileSystemWrapper::openFile(const String& path, const FileOpenParam& param)
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

	sl_bool FileSystemWrapper::deleteFile(const String& filePath)
	{
		return m_base->deleteFile(filePath);
	}

	sl_bool FileSystemWrapper::moveFile(const String& oldFilePath, const String& newFilePath, sl_bool flagReplaceIfExists)
	{
		return m_base->moveFile(oldFilePath, newFilePath, flagReplaceIfExists);
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

	sl_bool FileSystemWrapper::getFileInfo(const String& filePath, FileInfo& info, const FileInfoMask& mask)
	{
		return m_base->getFileInfo(filePath, info, mask);
	}

	sl_bool FileSystemWrapper::setFileInfo(const String& filePath, const FileInfo& info, const FileInfoMask& mask)
	{
		return m_base->setFileInfo(filePath, info, mask);
	}

	HashMap<String, FileInfo> FileSystemWrapper::getFiles(const String& pathDir)
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