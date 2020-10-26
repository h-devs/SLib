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

	sl_bool FileSystemProvider::setFileInfo(FileContext* context, const FileInfo& info, const FileInfoMask& mask)
	{
		SLIB_THROW(FileSystemError::NotImplemented, sl_false)
	}

}