/*
 *   Copyright (c) 2008-2018 SLIBIO <https://github.com/SLIBIO>
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

#include "slib/core/file.h"
#include "slib/core/file_util.h"

#include "slib/core/string_buffer.h"
#include "slib/core/scoped.h"

namespace slib
{

	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(FileInfo)

	FileInfo::FileInfo(): size(0), allocSize(0)
	{
	}


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(FileOpenParam)

	FileOpenParam::FileOpenParam()
	{
	}


	SLIB_DEFINE_OBJECT(File, IO)

	File::File(sl_file file) : m_file(file)
	{
	}

	File::~File()
	{
		close();
	}

	Ref<File> File::open(const StringParam& filePath, const FileOpenParam& param)
	{
		return open(filePath, param.mode, param.attributes);
	}


	Ref<File> File::open(const StringParam& filePath, const FileMode& mode, const FileAttributes& _attrs)
	{
		if (_attrs & FileAttributes::NotExist) {
			return sl_null;
		}
		FileAttributes attrs = _fixAttributes(_attrs);
		sl_file file = _open(filePath, mode, attrs);
		if (file != SLIB_FILE_INVALID_HANDLE) {
			Ref<File> ret = new File(file);
			if (mode & FileMode::SeekToEnd) {
				ret->seekToEnd();
			}
			return ret;
		}
		return sl_null;
	}

	Ref<File> File::open(const StringParam& filePath, const FileMode& mode)
	{
		return open(filePath, mode, 0);
	}

	Ref<File> File::openForRead(const StringParam& filePath)
	{
		return open(filePath, FileMode::Read | FileMode::ShareRead | FileMode::ShareWrite);
	}

	Ref<File> File::openForWrite(const StringParam& filePath)
	{
		return open(filePath, FileMode::Write);
	}

	Ref<File> File::openForReadWrite(const StringParam& filePath)
	{
		return open(filePath, FileMode::ReadWrite | FileMode::NotTruncate);
	}

	Ref<File> File::openForAppend(const StringParam& filePath)
	{
		return open(filePath, FileMode::Append);
	}

	Ref<File> File::openForRandomAccess(const StringParam& filePath)
	{
		return open(filePath, FileMode::RandomAccess);
	}

	Ref<File> File::openForRandomRead(const StringParam& filePath)
	{
		return open(filePath, FileMode::RandomRead | FileMode::ShareRead | FileMode::ShareWrite);
	}

	Ref<File> File::openDevice(const StringParam& path, const FileMode& mode)
	{
		return open(path, mode | FileMode::NotCreate | FileMode::NotTruncate | FileMode::HintRandomAccess);
	}
	
	Ref<File> File::openDeviceForRead(const StringParam& path)
	{
		return openDevice(path, FileMode::Read | FileMode::ShareRead | FileMode::ShareWrite);
	}

	void File::close()
	{
		if (isOpened()) {
			if (_close(m_file)) {
				m_file = SLIB_FILE_INVALID_HANDLE;
			}
		}
	}

	sl_bool File::isOpened() const
	{
		return m_file != SLIB_FILE_INVALID_HANDLE;
	}

	sl_file File::getHandle() const
	{
		return m_file;
	}

	void File::setHandle(sl_file handle)
	{
		m_file = handle;
	}

	void File::clearHandle()
	{
		m_file = SLIB_FILE_INVALID_HANDLE;
	}

	sl_bool File::getSize(sl_uint64& outSize)
	{
		return getSizeByHandle(m_file, outSize);
	}

	sl_uint64 File::getSizeByHandle(sl_file handle)
	{
		sl_uint64 size;
		if (getSizeByHandle(handle, size)) {
			return size;
		}
		return 0;
	}

	sl_uint64 File::getSize(const StringParam& path)
	{
		sl_uint64 size;
		if (getSize(path, size)) {
			return size;
		}
		return 0;
	}

	sl_bool File::getDiskSize(sl_uint64& outSize)
	{
		return getDiskSizeByHandle(m_file, outSize);
	}

	sl_uint64 File::getDiskSize()
	{
		return getDiskSizeByHandle(m_file);
	}

	sl_uint64 File::getDiskSizeByHandle(sl_file handle)
	{
		sl_uint64 size;
		if (getDiskSizeByHandle(handle, size)) {
			return size;
		}
		return 0;
	}

	sl_bool File::getDiskSize(const StringParam& devicePath, sl_uint64& outSize)
	{
		Ref<File> file = openDevice(devicePath, 0);
		if (file.isNotNull()) {
			return getDiskSizeByHandle(file->m_file, outSize);
		}
		return sl_false;
	}

	sl_uint64 File::getDiskSize(const StringParam& devicePath)
	{
		sl_uint64 size;
		if (getDiskSize(devicePath, size)) {
			return size;
		}
		return 0;
	}

	FileAttributes File::getAttributes(const StringParam& filePath)
	{
		if (filePath.isEmpty()) {
			return FileAttributes::NotExist;
		}
		FileAttributes attrs = _getAttributes(filePath);
		if (!(attrs & FileAttributes::AllAccess)) {
			attrs |= FileAttributes::NoAccess;
		} else {
			if (!(attrs & FileAttributes::WriteByAnyone)) {
				attrs |= FileAttributes::ReadOnly;
			}
		}
		return attrs;
	}

	FileAttributes File::_fixAttributes(const FileAttributes& _attrs)
	{
		FileAttributes attrs = _attrs;
		if (attrs & FileAttributes::NoAccess) {
			attrs &= ~(FileAttributes::AllAccess);
		} else {
			if (!(attrs & FileAttributes::AllAccess)) {
				attrs |= FileAttributes::AllAccess;
			}
		}
		if (attrs & FileAttributes::ReadOnly) {
			attrs &= ~(FileAttributes::WriteByAnyone);
		} else {
			if (!(attrs & FileAttributes::ReadByAnyone)) {
				attrs |= FileAttributes::ReadByAnyone;
			}
		}
		if (!(attrs & 0x7ffff)) {
			// For Win32
			attrs |= FileAttributes::Normal;
		}
		return attrs;
	}

	sl_bool File::setAttributes(const StringParam& filePath, const FileAttributes& attrs)
	{
		if (attrs & FileAttributes::NotExist) {
			return sl_false;
		}
		return _setAttributes(filePath, _fixAttributes(attrs));
	}

	sl_bool File::exists(const StringParam& filePath)
	{
		return (getAttributes(filePath) & FileAttributes::NotExist) == 0;
	}

	sl_bool File::isFile(const StringParam& filePath)
	{
		FileAttributes attrs = getAttributes(filePath);
		return (attrs & (FileAttributes::NotExist | FileAttributes::Directory)) == 0;
	}

	sl_bool File::isDirectory(const StringParam& filePath)
	{
		return (getAttributes(filePath) & FileAttributes::Directory) != 0;
	}

	sl_bool File::isHidden(const StringParam& filePath)
	{
		return (getAttributes(filePath) & FileAttributes::Hidden) != 0;
	}

	sl_bool File::setHidden(const StringParam& filePath, sl_bool flag)
	{
		FileAttributes attrs = getAttributes(filePath);
		if (!(attrs & FileAttributes::NotExist)) {
			if (flag) {
				SLIB_SET_FLAG(attrs.value, FileAttributes::Hidden);
			} else {
				SLIB_RESET_FLAG(attrs.value, FileAttributes::Hidden);
			}
			return setAttributes(filePath, attrs);
		}
		return sl_false;
	}

	sl_bool File::isReadOnly(const StringParam& filePath)
	{
		return (getAttributes(filePath) & FileAttributes::ReadOnly) != 0;
	}

	sl_bool File::setReadOnly(const StringParam& filePath, sl_bool flag)
	{
		FileAttributes attrs = getAttributes(filePath);
		if (!(attrs & FileAttributes::NotExist)) {
			if (flag) {
				SLIB_SET_FLAG(attrs.value, FileAttributes::ReadOnly);
			} else {
				SLIB_RESET_FLAG(attrs.value, FileAttributes::ReadOnly);
			}
			return setAttributes(filePath, attrs);
		}
		return sl_false;
	}

	String File::getParentDirectoryPath(const StringParam& _pathName)
	{
		StringData pathName(_pathName);
		if (pathName.isEmpty()) {
			return sl_null;
		}
		sl_reg indexSlash = pathName.lastIndexOf('/');
		sl_reg indexBackSlash = pathName.lastIndexOf('\\');
		sl_reg index = -1;
		if (indexSlash != -1) {
			if (indexBackSlash != -1) {
				if (indexSlash < indexBackSlash) {
					index = indexBackSlash;
				} else {
					index = indexSlash;
				}
			} else {
				index = indexSlash;
			}
		} else {
			if (indexBackSlash != -1) {
				index = indexBackSlash;
			}
		}
		if (index == -1) {
			return sl_null;
		} else {
			if (index == 0 && indexSlash == 0 && pathName.getLength() != 1) {
				return "/";
			} else {
				return pathName.substring(0, index);
			}
		}
	}

	String File::getFileName(const StringParam& _pathName)
	{
		StringData pathName(_pathName);
		if (pathName.isEmpty()) {
			return sl_null;
		}
		sl_reg indexSlash = pathName.lastIndexOf('/');
		sl_reg indexBackSlash = pathName.lastIndexOf('\\');
		sl_reg index = -1;
		if (indexSlash != -1) {
			if (indexBackSlash != -1) {
				if (indexSlash < indexBackSlash) {
					index = indexBackSlash;
				} else {
					index = indexSlash;
				}
			} else {
				index = indexSlash;
			}
		} else {
			if (indexBackSlash != -1) {
				index = indexBackSlash;
			}
		}
		return pathName.substring(index + 1);
	}


	String File::getFileExtension(const StringParam& _pathName)
	{
		StringData pathName(_pathName);
		String fileName = getFileName(pathName);
		if (fileName.isEmpty()) {
			return sl_null;
		}
		sl_reg index = fileName.lastIndexOf('.');
		if (index > 0) {
			return fileName.substring(index + 1);
		} else {
			return sl_null;
		}
	}

	String File::getFileNameOnly(const StringParam& _pathName)
	{
		StringData pathName(_pathName);
		String fileName = getFileName(pathName);
		if (fileName.isEmpty()) {
			return sl_null;
		}
		sl_reg index = fileName.lastIndexOf('.');
		if (index > 0) {
			return fileName.substring(0, index);
		} else {
			return fileName;
		}
	}

	String File::normalizeDirectoryPath(const StringParam& _str)
	{
		StringData str(_str);
		if (str.endsWith('\\') || str.endsWith('/')) {
			return str.substring(0, str.getLength() - 1);
		} else {
			return str.toString(_str);
		}
	}
	
	Memory File::readAllBytes(sl_size maxSize)
	{
		return IO::readAllBytes(maxSize);
	}

	Memory File::readAllBytes(const StringParam& path, sl_size maxSize)
	{
		Ref<File> file = File::openForRead(path);
		if (file.isNotNull()) {
			return file->readAllBytes(maxSize);
		}
		return sl_null;
	}
	
	String File::readAllTextUTF8(sl_size maxSize)
	{
		return IO::readAllTextUTF8(maxSize);
	}
	
	String File::readAllTextUTF8(const StringParam& path, sl_size maxSize)
	{
		Ref<File> file = File::openForRead(path);
		if (file.isNotNull()) {
			return file->readAllTextUTF8();
		}
		return sl_null;
	}
	
	String16 File::readAllTextUTF16(EndianType endian, sl_size maxSize)
	{
		return IO::readAllTextUTF16(endian, maxSize);
	}

	String16 File::readAllTextUTF16(const StringParam& path, EndianType endian, sl_size maxSize)
	{
		Ref<File> file = File::openForRead(path);
		if (file.isNotNull()) {
			return file->readAllTextUTF16(endian, maxSize);
		}
		return sl_null;
	}
	
	String File::readAllText(Charset* outCharset, sl_size maxSize)
	{
		return IO::readAllText(outCharset, maxSize);
	}

	String File::readAllText(const StringParam& path, Charset* outCharset, sl_size maxSize)
	{
		Ref<File> file = File::openForRead(path);
		if (file.isNotNull()) {
			return file->readAllText(outCharset, maxSize);
		}
		return sl_null;
	}
	
	String16 File::readAllText16(Charset* outCharset, sl_size maxSize)
	{
		return IO::readAllText16(outCharset, maxSize);
	}
	
	String16 File::readAllText16(const StringParam& path, Charset* outCharset, sl_size maxSize)
	{
		Ref<File> file = File::openForRead(path);
		if (file.isNotNull()) {
			return file->readAllText16(outCharset, maxSize);
		}
		return sl_null;
	}

	sl_size File::writeAllBytes(const StringParam& path, const void* buf, sl_size size)
	{
		Ref<File> file = File::openForWrite(path);
		if (file.isNotNull()) {
			sl_reg ret = file->write(buf, size);
			if (ret > 0) {
				return ret;
			}
		}
		return 0;
	}

	sl_size File::writeAllBytes(const StringParam& path, const Memory& mem)
	{
		return File::writeAllBytes(path, mem.getData(), mem.getSize());
	}

	sl_bool File::writeAllTextUTF8(const StringParam& path, const StringParam& text, sl_bool flagWriteByteOrderMark)
	{
		Ref<File> file = File::openForWrite(path);
		if (file.isNotNull()) {
			return file->writeTextUTF8(text, flagWriteByteOrderMark);
		}
		return sl_false;
	}

	sl_bool File::writeAllTextUTF16LE(const StringParam& path, const StringParam& text, sl_bool flagWriteByteOrderMark)
	{
		Ref<File> file = File::openForWrite(path);
		if (file.isNotNull()) {
			return file->writeTextUTF16LE(text, flagWriteByteOrderMark);
		}
		return sl_false;
	}

	sl_bool File::writeAllTextUTF16BE(const StringParam& path, const StringParam& text, sl_bool flagWriteByteOrderMark)
	{
		Ref<File> file = File::openForWrite(path);
		if (file.isNotNull()) {
			return file->writeTextUTF16BE(text, flagWriteByteOrderMark);
		}
		return sl_false;
	}

	sl_size File::appendAllBytes(const StringParam& path, const void* buf, sl_size size)
	{
		Ref<File> file = File::openForAppend(path);
		if (file.isNotNull()) {
			sl_reg ret = file->write(buf, size);
			if (ret > 0) {
				return ret;
			}
		}
		return 0;
	}

	sl_size File::appendAllBytes(const StringParam& path, const Memory& mem)
	{
		return File::appendAllBytes(path, mem.getData(), mem.getSize());
	}

	sl_bool File::appendAllTextUTF8(const StringParam& path, const StringParam& text)
	{
		Ref<File> file = File::openForAppend(path);
		if (file.isNotNull()) {
			return file->writeTextUTF8(text, sl_false);
		}
		return sl_false;
	}

	sl_bool File::appendAllTextUTF16LE(const StringParam& path, const StringParam& text)
	{
		Ref<File> file = File::openForAppend(path);
		if (file.isNotNull()) {
			return file->writeTextUTF16LE(text, sl_false);
		}
		return sl_false;
	}

	sl_bool File::appendAllTextUTF16BE(const StringParam& path, const StringParam& text)
	{
		Ref<File> file = File::openForAppend(path);
		if (file.isNotNull()) {
			return file->writeTextUTF16BE(text, sl_false);
		}
		return sl_false;
	}

	List<String> File::getAllDescendantFiles(const StringParam& dirPath)
	{
		if (!isDirectory(dirPath)) {
			return sl_null;
		}
		List<String> ret;
		List<String> listCurrent = getFiles(dirPath);
		listCurrent.sort_NoLock();
		String* p = listCurrent.getData();
		sl_size n = listCurrent.getCount();
		for (sl_size i = 0; i < n; i++) {
			String& item = p[i];
			if (item != "." && item != "..") {
				ret.add_NoLock(item);
				String dir = dirPath.toString() + "/" + item;
				if (File::isDirectory(dir)) {
					ListElements<String> sub(File::getAllDescendantFiles(dir));
					for (sl_size j = 0; j < sub.count; j++) {
						ret.add(item + "/" + sub[j]);
					}
				}
			}
		}
		return ret;
	}

	sl_bool File::createDirectory(const StringParam& dirPath, sl_bool flagErrorOnCreateExistingDirectory)
	{
		FileAttributes attr = File::getAttributes(dirPath);
		if (!(attr & FileAttributes::NotExist)) {
			if (attr & FileAttributes::Directory) {
				if (flagErrorOnCreateExistingDirectory) {
					return sl_false;
				} else {
					return sl_true;
				}
			} else {
				return sl_false;
			}
		}
		_createDirectory(dirPath);
		return File::isDirectory(dirPath);
	}

	sl_bool File::createDirectories(const StringParam& dirPath)
	{
		if (dirPath.isEmpty()) {
			return sl_false;
		}
		if (File::isDirectory(dirPath)) {
			return sl_true;
		}
		if (File::isFile(dirPath)) {
			return sl_false;
		}
		String parent = File::getParentDirectoryPath(dirPath.toString());
		if (parent.isEmpty()) {
			return File::createDirectory(dirPath);
		} else {
			if (File::createDirectories(parent)) {
				return File::createDirectory(dirPath);
			}
			return sl_false;
		}
	}

	sl_bool File::remove(const StringParam& filePath, sl_bool flagErrorOnDeleteNotExistingFile)
	{
		FileAttributes attr = File::getAttributes(filePath);
		if (attr & FileAttributes::NotExist) {
			if (flagErrorOnDeleteNotExistingFile) {
				return sl_false;
			} else {
				return sl_true;
			}
		}
		if (attr & FileAttributes::Directory) {
			deleteDirectory(filePath);
		} else {
			deleteFile(filePath);
		}
		return !(File::exists(filePath));
	}

	sl_bool File::deleteDirectoryRecursively(const StringParam& _dirPath)
	{
		String dirPath = _dirPath.toString();
		if (File::isDirectory(dirPath)) {
			String path = dirPath + "/";
			ListElements<String> list(File::getFiles(dirPath));
			sl_bool ret = sl_true;
			for (sl_size i = 0; i < list.count; i++) {
				String sub = path + list[i];
				if (File::exists(sub)) {
					if (File::isDirectory(sub)) {
						ret = ret && File::deleteDirectoryRecursively(sub);
					} else {
						ret = ret && File::deleteFile(sub);
					}
				}
			}
			ret = ret && File::deleteDirectory(path);
			return ret;
		} else {
			return sl_false;
		}
	}

	String File::makeSafeFileName(const String& fileName)
	{
		String ret = fileName.duplicate();
		if (ret.isEmpty()) {
			return ret;
		}
		sl_char8* buf = ret.getData();
		sl_size len = ret.getLength();
		for (sl_size i = 0; i < len; i++) {
			sl_uint32 ch = (sl_uint8)(buf[i]);
			if (ch < 0x20) {
				buf[i] = '_';
			} else if (ch >= 0x7f && ch < 0xA0) {
				buf[i] = '_';
			} else {
				switch (ch) {
				case '\\':
				case '/':
				case ':':
				case '*':
				case '?':
				case '"':
				case '<':
				case '>':
				case '|':
					buf[i] = '_';
					break;
				}
			}
		}
		return ret;
	}

	String File::makeSafeFilePath(const String& filePath)
	{
		String ret = filePath.duplicate();
		if (ret.isEmpty()) {
			return ret;
		}
		sl_char8* buf = ret.getData();
		sl_size len = ret.getLength();
		for (sl_size i = 0; i < len; i++) {
			sl_uint32 ch = (sl_uint8)(buf[i]);
			if (ch < 0x20) {
				buf[i] = '_';
			} else if (ch >= 0x7f && ch < 0xA0) {
				buf[i] = '_';
			} else {
				switch (ch) {
				case ':':
				case '*':
				case '?':
				case '"':
				case '<':
				case '>':
				case '|':
					buf[i] = '_';
					break;
				}
			}
		}
		return ret;
	}

	String File::findParentPathContainingFile(const String& basePath, const String& filePath, sl_uint32 nDeep)
	{
		FilePathSegments segments;
		segments.parsePath(basePath);
		if (nDeep > segments.segments.getCount()) {
			nDeep = (sl_uint32)(segments.segments.getCount());
		}
		for (sl_uint32 i = 0; i <= nDeep; i++) {
			String path = segments.buildPath();
			if (File::exists(path + "/" + filePath)) {
				return path;
			}
			segments.segments.popBack();
		}
		return sl_null;
	}


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(FilePathSegments)
	
	FilePathSegments::FilePathSegments()
	{
		parentLevel = 0;
	}

	void FilePathSegments::parsePath(const String& path)
	{
		parentLevel = 0;
		segments.setNull();

		sl_char8* buf = path.getData();
		sl_size len = path.getLength();
		sl_size pos = 0;
		sl_size start = 0;
		for (; pos <= len; pos++) {
			sl_char8 ch;
			if (pos == len) {
				ch = '/';
			} else {
				ch = buf[pos];
			}
			if (ch == '/' || ch == '\\') {
				if (pos == 0) {
					segments.add_NoLock(String::null());
				} else {
					sl_size n = pos - start;
					if (n > 0) {
						if (n == 1 && buf[start] == '.') {
						} else if (n == 2 && buf[start] == '.' && buf[start + 1] == '.') {
							if (segments.getCount() > 0) {
								segments.popBack_NoLock();
							} else {
								parentLevel++;
							}
						} else {
							String s(buf + start, n);
							segments.add_NoLock(s);
						}
					}
				}
				start = pos + 1;
			}
		}
	}

	String FilePathSegments::buildPath()
	{
		StringBuffer ret;
		sl_bool flagFirst = sl_true;
		{
			for (sl_uint32 i = 0; i < parentLevel; i++) {
				if (!flagFirst) {
					ret.addStatic("/");
				}
				ret.addStatic("..");
				flagFirst = sl_false;
			}
		}
		{
			ListElements<String> list(segments);
			for (sl_size i = 0; i < list.count; i++) {
				if (!flagFirst) {
					ret.addStatic("/");
				}
				ret.add(list[i]);
				flagFirst = sl_false;
			}
		}
		return ret.merge();
	}

#ifndef SLIB_PLATFORM_IS_WIN32
	DisableWow64FsRedirectionScope::DisableWow64FsRedirectionScope()
	{
	}

	DisableWow64FsRedirectionScope::~DisableWow64FsRedirectionScope()
	{
	}
#endif

}
