/*
 *   Copyright (c) 2008-2021 SLIBIO <https://github.com/SLIBIO>
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
#include "slib/core/scoped_buffer.h"
#include "slib/core/io/impl.h"

namespace slib
{

	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(FileInfo)

	FileInfo::FileInfo() noexcept: size(0), allocSize(0)
	{
	}


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(FileOpenParam)

	FileOpenParam::FileOpenParam() noexcept
	{
	}


	SLIB_DEFINE_HANDLE_CONTAINER_MEMBERS(File, sl_file, m_file, SLIB_FILE_INVALID_HANDLE, _close)
	SLIB_DEFINE_IO_MEMBERS(File, const noexcept)

	File File::open(const StringParam& filePath, const FileOpenParam& param) noexcept
	{
		return open(filePath, param.mode, param.attributes);
	}

	File File::open(const StringParam& filePath, const FileMode& mode, const FileAttributes& _attrs) noexcept
	{
		if (_attrs & FileAttributes::NotExist) {
			return SLIB_FILE_INVALID_HANDLE;
		}
		FileAttributes attrs = _fixAttributes(_attrs);
		sl_file file = _open(filePath, mode, attrs);
		if (file != SLIB_FILE_INVALID_HANDLE) {
			File ret = file;
			if (mode & FileMode::SeekToEnd) {
				ret.seekToEnd();
			}
			return ret;
		}
		return SLIB_FILE_INVALID_HANDLE;
	}

	File File::open(const StringParam& filePath, const FileMode& mode) noexcept
	{
		return open(filePath, mode, 0);
	}

	File File::openForRead(const StringParam& filePath) noexcept
	{
		return open(filePath, FileMode::Read | FileMode::ShareRead | FileMode::ShareWrite);
	}

	File File::openForWrite(const StringParam& filePath) noexcept
	{
		return open(filePath, FileMode::Write);
	}

	File File::openForReadWrite(const StringParam& filePath) noexcept
	{
		return open(filePath, FileMode::ReadWrite | FileMode::NotTruncate);
	}

	File File::openForAppend(const StringParam& filePath) noexcept
	{
		return open(filePath, FileMode::Append);
	}

	File File::openForRandomAccess(const StringParam& filePath) noexcept
	{
		return open(filePath, FileMode::RandomAccess);
	}

	File File::openForRandomRead(const StringParam& filePath) noexcept
	{
		return open(filePath, FileMode::RandomRead | FileMode::ShareRead | FileMode::ShareWrite);
	}

	File File::openDevice(const StringParam& path, const FileMode& mode) noexcept
	{
		return open(path, mode | FileMode::Device | FileMode::NotCreate | FileMode::NotTruncate | FileMode::HintRandomAccess);
	}
	
	File File::openDeviceForRead(const StringParam& path) noexcept
	{
		return openDevice(path, FileMode::Read | FileMode::ShareRead | FileMode::ShareWrite);
	}

	void File::close() noexcept
	{
		setNone();
	}

	void File::close(sl_file handle) noexcept
	{
		_close(handle);
	}

	sl_reg File::read(void* buf, sl_size size) const noexcept
	{
		return ReaderHelper::readWithRead32(this, buf, size);
	}

	sl_reg File::write(const void* buf, sl_size size) const noexcept
	{
		return WriterHelper::writeWithWrite32(this, buf, size);
	}

	sl_uint64 File::getSize(const StringParam& path) noexcept
	{
		sl_uint64 size;
		if (getSize(path, size)) {
			return size;
		}
		return 0;
	}

	sl_uint64 File::getDiskSize() const noexcept
	{
		sl_uint64 size;
		if (getDiskSize(size)) {
			return size;
		}
		return 0;
	}

	sl_bool File::getDiskSize(const StringParam& devicePath, sl_uint64& outSize) noexcept
	{
		File file = openDevice(devicePath, 0);
		if (file.isNotNone()) {
			return file.getDiskSize(outSize);
		}
		return sl_false;
	}

	sl_uint64 File::getDiskSize(const StringParam& devicePath) noexcept
	{
		sl_uint64 size;
		if (getDiskSize(devicePath, size)) {
			return size;
		}
		return 0;
	}

	FileAttributes File::getAttributes() const noexcept
	{
		FileAttributes attrs = _getAttributes();
		if (attrs & FileAttributes::NotExist) {
			return attrs;
		}
		if (!(attrs & FileAttributes::AllAccess)) {
			attrs |= FileAttributes::NoAccess;
		} else {
			if (!(attrs & FileAttributes::WriteByAnyone)) {
				attrs |= FileAttributes::ReadOnly;
			}
		}
		return attrs;
	}

	FileAttributes File::getAttributes(const StringParam& filePath) noexcept
	{
		if (filePath.isEmpty()) {
			return FileAttributes::NotExist;
		}
		FileAttributes attrs = _getAttributes(filePath);
		if (attrs & FileAttributes::NotExist) {
			return attrs;
		}
		if (!(attrs & FileAttributes::AllAccess)) {
			attrs |= FileAttributes::NoAccess;
		} else {
			if (!(attrs & FileAttributes::WriteByAnyone)) {
				attrs |= FileAttributes::ReadOnly;
			}
		}
		return attrs;
	}

	FileAttributes File::_fixAttributes(const FileAttributes& _attrs) noexcept
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

	sl_bool File::setAttributes(const StringParam& filePath, const FileAttributes& attrs) noexcept
	{
		if (attrs & FileAttributes::NotExist) {
			return sl_false;
		}
		return _setAttributes(filePath, _fixAttributes(attrs));
	}

	sl_bool File::exists(const StringParam& filePath) noexcept
	{
		return (getAttributes(filePath) & FileAttributes::NotExist) == 0;
	}

	sl_bool File::isFile(const StringParam& filePath) noexcept
	{
		FileAttributes attrs = getAttributes(filePath);
		return (attrs & (FileAttributes::NotExist | FileAttributes::Directory)) == 0;
	}

	sl_bool File::isDirectory(const StringParam& filePath) noexcept
	{
		return (getAttributes(filePath) & FileAttributes::Directory) != 0;
	}

	sl_bool File::isHidden(const StringParam& filePath) noexcept
	{
		return (getAttributes(filePath) & FileAttributes::Hidden) != 0;
	}

	sl_bool File::setHidden(const StringParam& filePath, sl_bool flag) noexcept
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

	sl_bool File::isReadOnly(const StringParam& filePath) noexcept
	{
		return (getAttributes(filePath) & FileAttributes::ReadOnly) != 0;
	}

	sl_bool File::setReadOnly(const StringParam& filePath, sl_bool flag) noexcept
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

#if !defined(SLIB_PLATFORM_IS_LINUX_DESKTOP)
	String File::getCap(const StringParam& filePath) noexcept
	{
		return sl_null;
	}

	sl_bool File::setCap(const StringParam& filePath, const StringParam& cap) noexcept
	{
		return sl_false;
	}
#endif

	String File::getParentDirectoryPath(const StringParam& _pathName) noexcept
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

	String File::getFileName(const StringParam& _pathName) noexcept
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

	String File::getFileExtension(const StringParam& _pathName) noexcept
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

	String File::getFileNameOnly(const StringParam& _pathName) noexcept
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

	String File::normalizeDirectoryPath(const StringParam& _str) noexcept
	{
		StringData str(_str);
		if (str.endsWith('\\') || str.endsWith('/')) {
			return str.substring(0, str.getLength() - 1);
		} else {
			return str.toString(_str);
		}
	}

	String File::joinPath(const StringParam& path1, const StringParam& path2) noexcept
	{
		StringData str(path1);
		if (str.endsWith('\\') || str.endsWith('/')) {
			return *((StringView*)&str) + path2;
		} else {
			return String::join(str, "/", path2);
		}
	}

	Memory File::readAllBytes(const StringParam& path, sl_size maxSize) noexcept
	{
		File file = File::openForRead(path);
		if (file.isNotNone()) {
			return file.readAllBytes(maxSize);
		}
		return sl_null;
	}
	
	String File::readAllTextUTF8(const StringParam& path, sl_size maxSize) noexcept
	{
		File file = File::openForRead(path);
		if (file.isNotNone()) {
			return file.readAllTextUTF8();
		}
		return sl_null;
	}
	
	String16 File::readAllTextUTF16(const StringParam& path, EndianType endian, sl_size maxSize) noexcept
	{
		File file = File::openForRead(path);
		if (file.isNotNone()) {
			return file.readAllTextUTF16(endian, maxSize);
		}
		return sl_null;
	}
	
	String File::readAllText(const StringParam& path, Charset* outCharset, sl_size maxSize) noexcept
	{
		File file = File::openForRead(path);
		if (file.isNotNone()) {
			return file.readAllText(outCharset, maxSize);
		}
		return sl_null;
	}
	
	String16 File::readAllText16(const StringParam& path, Charset* outCharset, sl_size maxSize) noexcept
	{
		File file = File::openForRead(path);
		if (file.isNotNone()) {
			return file.readAllText16(outCharset, maxSize);
		}
		return sl_null;
	}

	sl_size File::writeAllBytes(const StringParam& path, const void* buf, sl_size size) noexcept
	{
		File file = File::openForWrite(path);
		if (file.isNotNone()) {
			sl_reg ret = file.write(buf, size);
			if (ret > 0) {
				return ret;
			}
		}
		return 0;
	}

	sl_size File::writeAllBytes(const StringParam& path, const Memory& mem) noexcept
	{
		return File::writeAllBytes(path, mem.getData(), mem.getSize());
	}

	sl_bool File::writeAllTextUTF8(const StringParam& path, const StringParam& text, sl_bool flagWriteByteOrderMark) noexcept
	{
		File file = File::openForWrite(path);
		if (file.isNotNone()) {
			return file.writeTextUTF8(text, flagWriteByteOrderMark);
		}
		return sl_false;
	}

	sl_bool File::writeAllTextUTF16LE(const StringParam& path, const StringParam& text, sl_bool flagWriteByteOrderMark) noexcept
	{
		File file = File::openForWrite(path);
		if (file.isNotNone()) {
			return file.writeTextUTF16LE(text, flagWriteByteOrderMark);
		}
		return sl_false;
	}

	sl_bool File::writeAllTextUTF16BE(const StringParam& path, const StringParam& text, sl_bool flagWriteByteOrderMark) noexcept
	{
		File file = File::openForWrite(path);
		if (file.isNotNone()) {
			return file.writeTextUTF16BE(text, flagWriteByteOrderMark);
		}
		return sl_false;
	}

	sl_size File::appendAllBytes(const StringParam& path, const void* buf, sl_size size) noexcept
	{
		File file = File::openForAppend(path);
		if (file.isNotNone()) {
			sl_reg ret = file.write(buf, size);
			if (ret > 0) {
				return ret;
			}
		}
		return 0;
	}

	sl_size File::appendAllBytes(const StringParam& path, const Memory& mem) noexcept
	{
		return File::appendAllBytes(path, mem.getData(), mem.getSize());
	}

	sl_bool File::appendAllTextUTF8(const StringParam& path, const StringParam& text) noexcept
	{
		File file = File::openForAppend(path);
		if (file.isNotNone()) {
			return file.writeTextUTF8(text, sl_false);
		}
		return sl_false;
	}

	sl_bool File::appendAllTextUTF16LE(const StringParam& path, const StringParam& text) noexcept
	{
		File file = File::openForAppend(path);
		if (file.isNotNone()) {
			return file.writeTextUTF16LE(text, sl_false);
		}
		return sl_false;
	}

	sl_bool File::appendAllTextUTF16BE(const StringParam& path, const StringParam& text) noexcept
	{
		File file = File::openForAppend(path);
		if (file.isNotNone()) {
			return file.writeTextUTF16BE(text, sl_false);
		}
		return sl_false;
	}

	List<String> File::getAllDescendantFiles(const StringParam& dirPath) noexcept
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
						ret.add_NoLock(item + "/" + sub[j]);
					}
				}
			}
		}
		return ret;
	}

	sl_bool File::createDirectory(const StringParam& dirPath, const FileOperationFlags& flags) noexcept
	{
		FileAttributes attr = File::getAttributes(dirPath);
		if (!(attr & FileAttributes::NotExist)) {
			if (attr & FileAttributes::Directory) {
				if (flags & FileOperationFlags::ErrorOnExisting) {
					return sl_false;
				} else {
					return sl_true;
				}
			} else {
				return sl_false;
			}
		}
		return _createDirectory(dirPath);
	}

	sl_bool File::createDirectories(const StringParam& dirPath) noexcept
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

	sl_bool File::remove(const StringParam& path, const FileOperationFlags& flags) noexcept
	{
		FileAttributes attr = File::getAttributes(path);
		if (attr & FileAttributes::NotExist) {
			if (flags & FileOperationFlags::ErrorOnNotExisting) {
				return sl_false;
			} else {
				return sl_true;
			}
		}
		if (attr & FileAttributes::Directory) {
			if (flags & FileOperationFlags::Recursive) {
				sl_bool ret = sl_true;
				ListElements<String> list(File::getFiles(path));
				for (sl_size i = 0; i < list.count; i++) {
					ret = ret && File::remove(File::joinPath(path, list[i]), flags);
					if (!ret) {
						if (flags & FileOperationFlags::AbortOnError) {
							return sl_false;
						}
					}
				}
				ret = ret && File::deleteDirectory(path);
				return ret;
			} else {
				return File::deleteDirectory(path);
			}
		} else {
			return File::deleteFile(path);
		}
	}

	sl_bool File::copyFile(const StringParam& pathSource, const StringParam& pathTarget, const FileOperationFlags& flags) noexcept
	{
		if (flags & FileOperationFlags::NotReplace) {
			FileAttributes attr = File::getAttributes(pathTarget);
			if (attr & FileAttributes::NotExist) {
				return _copyFile(pathSource, pathTarget);
			} else {
				if (flags & FileOperationFlags::ErrorOnExisting) {
					return sl_false;
				} else {
					return sl_true;
				}
			}
		} else {
			return _copyFile(pathSource, pathTarget);
		}
	}

	sl_bool File::copy(const StringParam& pathSource, const StringParam& pathTarget, const FileOperationFlags& flags) noexcept
	{
		FileAttributes attr = File::getAttributes(pathSource);
		if (attr & FileAttributes::NotExist) {
			return sl_false;
		}
		if (attr & FileAttributes::Directory) {
			if (!(File::createDirectory(pathTarget))) {
				return sl_false;
			}
			sl_bool ret = sl_true;
			ListElements<String> list(File::getFiles(pathSource));
			for (sl_size i = 0; i < list.count; i++) {
				if (flags & FileOperationFlags::Recursive) {
					ret = ret && File::copy(joinPath(pathSource, list[i]), joinPath(pathTarget, list[i]), flags);
				} else {
					ret = ret && File::copyFile(joinPath(pathSource, list[i]), joinPath(pathTarget, list[i]), flags);
				}
				if (!ret) {
					if (flags & FileOperationFlags::AbortOnError) {
						return sl_false;
					}
				}
			}
			return ret;
		} else {
			if (File::isDirectory(pathTarget)) {
				return File::copyFile(pathSource, joinPath(pathTarget, getFileName(pathSource)), flags);
			} else {
				return File::copyFile(pathSource, pathTarget, flags);
			}
		}
	}

	sl_bool File::move(const StringParam& pathOriginal, const StringParam& filePathNew, const FileOperationFlags& flags) noexcept
	{
		if (flags & FileOperationFlags::NotReplace) {
			FileAttributes attr = File::getAttributes(filePathNew);
			if (attr & FileAttributes::NotExist) {
				return _move(pathOriginal, filePathNew);
			} else {
				if (flags & FileOperationFlags::ErrorOnExisting) {
					return sl_false;
				} else {
					return sl_true;
				}
			}
		} else {
#ifdef SLIB_PLATFORM_IS_UNIX
			if (File::exists(filePathNew)) {
				File::remove(filePathNew);
			}
#endif
			return _move(pathOriginal, filePathNew);
		}
	}

	String File::makeSafeFileName(const StringParam& fileName) noexcept
	{
		String ret = fileName.newString();
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

	String File::makeSafeFilePath(const StringParam& filePath) noexcept
	{
		String ret = filePath.newString();
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

	String File::findParentPathContainingFile(const StringParam& basePath, const StringParam& filePath, sl_uint32 nDeep) noexcept
	{
		FilePathSegments segments;
		segments.parsePath(basePath.toString());
		segments.flagEndsWithSlash = sl_false;
		if (nDeep > segments.segments.getCount()) {
			nDeep = (sl_uint32)(segments.segments.getCount());
		}
		for (sl_uint32 i = 0; i <= nDeep; i++) {
			String path = segments.buildPath();
			if (File::exists(String::join(path, "/", filePath))) {
				return path;
			}
			segments.segments.popBack();
		}
		return sl_null;
	}


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(FilePathSegments)
	
	FilePathSegments::FilePathSegments() noexcept
	{
		flagStartsWithSlash = sl_false;
		flagEndsWithSlash = sl_false;
		parentLevel = 0;
	}

	void FilePathSegments::parsePath(const String& path) noexcept
	{
		m_path = path;

		parentLevel = 0;
		segments.setNull();

		sl_char8* buf = path.getData();
		sl_size len = path.getLength();
		if (len && (buf[0] == '/' || buf[0] == '\\')) {
			flagStartsWithSlash = sl_true;
			buf++;
			len--;
		} else {
			flagStartsWithSlash = sl_false;
		}
		if (len && (buf[len - 1] == '/' || buf[len - 1] == '\\')) {
			flagEndsWithSlash = sl_true;
			buf++;
			len--;
		} else {
			flagEndsWithSlash = sl_false;
		}

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
				sl_size n = pos - start;
				if (n > 0) {
					StringView segment(buf + start, n);
					segment = segment.trim();
					n = segment.getLength();
					if (n == 1 && segment[0] == '.') {
					} else if (n == 2 && segment[0] == '.' && segment[1] == '.') {
						if (segments.isNotEmpty()) {
							segments.popBack_NoLock();
						} else {
							parentLevel++;
						}
					} else {
						segments.add_NoLock(segment);
					}
				}
				start = pos + 1;
			}
		}
	}

	String FilePathSegments::buildPath() const noexcept
	{
		StringBuffer ret;
		if (flagStartsWithSlash) {
			ret.addStatic("/");
		}
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
			ListElements<StringView> list(segments);
			for (sl_size i = 0; i < list.count; i++) {
				if (!flagFirst) {
					ret.addStatic("/");
				}
				ret.addStatic(list[i].getData(), list[i].getLength());
				flagFirst = sl_false;
			}
		}
		if (flagEndsWithSlash) {
			ret.addStatic("/");
		}
		return ret.merge();
	}

#ifndef SLIB_PLATFORM_IS_WIN32
	DisableWow64FsRedirectionScope::DisableWow64FsRedirectionScope() noexcept
	{
	}

	DisableWow64FsRedirectionScope::~DisableWow64FsRedirectionScope() noexcept
	{
	}
#endif

}
