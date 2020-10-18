#include "slib/storage/filesystembase.h"

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
		_Handles.put(fileName, _Handles[fileName] + 1);
		return _Handles[fileName];
	}

	sl_size FileSystemBase::decreaseHandleCount(String fileName)
	{
		ObjectLocker locker(this);
		sl_size count = _Handles[fileName];
		if (count > 1)
			_Handles.put(fileName, count - 1);
		else
			_Handles.remove(fileName);
		return _Handles[fileName];
	}

	sl_size FileSystemBase::getOpenHandlesCount()
	{
		return _Handles.getCount();
	}

}