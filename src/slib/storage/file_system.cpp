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

#include "slib/storage/file_system.h"

#include "slib/core/log.h"

#define TAG						"FileSystemBase"
#define errorLog(...)			LogError(TAG, ##__VA_ARGS__)
#define debugLog(...)			LogDebug(TAG, ##__VA_ARGS__)

namespace slib
{

	/* Helpers */

	sl_bool FileSystemBase::exists(String fileName) noexcept
	{
		try {
			fsGetFileInfo(new FileContext(fileName));
			return sl_true;
		}
		catch (...) {
			return sl_false;
		}
	}

	Memory FileSystemBase::readFile(String fileName, sl_int64 offset, sl_uint32 length) noexcept
	{
		Ref<FileContext> context;
		fileName = fileName.replaceAll("/", "\\");
		if (!fileName.startsWith("\\")) fileName = "\\" + fileName;
		try {
			context = new FileContext(fileName);
			fsOpen(context);	// FIXME sharing violation error (32)
			FileInfo info = fsGetFileInfo(context);
			if (offset < 0) offset = info.size + offset;
			if (offset < 0) offset = 0;
			if (length == 0) {
				length = (info.size - (sl_uint64)offset) & 0xFFFFFFFF;
			}
			//if (info.attr.isDirectory) throw;
			Memory buffer = Memory::create(length);
			sl_size ret = fsRead(context, buffer, offset);
			fsClose(context);
			return buffer.sub(0, ret);
		}
		catch (FileSystemError error) {
			debugLog("readFile(%s,%d,%d)\n  Error: %d", fileName, offset, length, error);
			if (context.isNotNull()) {
				try {
					fsClose(context);
				}
				catch (...) {}
			}
			return sl_null;
		}
	}

	sl_bool FileSystemBase::writeFile(String fileName, const Memory& buffer, FileCreationParams& params) noexcept
	{
		Ref<FileContext> context;
		fileName = fileName.replaceAll("/", "\\");
		if (!fileName.startsWith("\\")) fileName = "\\" + fileName;
		try {
			context = new FileContext(fileName);
			fsCreate(context, params);
			fsWrite(context, buffer, 0, sl_false);
			fsClose(context);
			return sl_true;
		}
		catch (FileSystemError error) {
			debugLog("writeFile(%s,%d)\n  Error: %d", fileName, buffer.getSize(), error);
			if (context.isNotNull()) {
				try {
					fsClose(context);
				}
				catch (...) {}
			}
			return sl_false;
		}
	}

	sl_bool FileSystemBase::deleteFile(String fileName) noexcept
	{
		Ref<FileContext> context;
		fileName = fileName.replaceAll("/", "\\");
		if (!fileName.startsWith("\\")) fileName = "\\" + fileName;
		try {
			context = new FileContext(fileName);
			fsOpen(context);
			fsDelete(context, sl_false);
			fsClose(context);
			return sl_true;
		}
		catch (FileSystemError) {
			if (context.isNotNull()) {
				try {
					fsClose(context);
				}
				catch (...) {}
			}
			return sl_false;
		}
	}

	sl_size FileSystemBase::increaseHandleCount(String fileName)
	{
		ObjectLocker locker(this);
		m_openHandles.put(fileName, m_openHandles[fileName] + 1);
		return m_openHandles[fileName];
	}

	sl_size FileSystemBase::decreaseHandleCount(String fileName)
	{
		ObjectLocker locker(this);
		sl_size count = m_openHandles[fileName];
		if (count > 1)
			m_openHandles.put(fileName, count - 1);
		else
			m_openHandles.remove(fileName);
		return m_openHandles[fileName];
	}

	sl_size FileSystemBase::getOpenHandlesCount()
	{
		return m_openHandles.getCount();
	}

}