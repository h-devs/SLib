/*
 *   Copyright (c) 2008-2024 SLIBIO <https://github.com/SLIBIO>
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

#include "slib/io/file.h"
#include "slib/io/file_io.h"
#include "slib/io/file_util.h"

#include "slib/io/text.h"
#include "slib/io/priv/impl.h"

namespace slib
{

	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(FileInfo)

	FileInfo::FileInfo() noexcept: size(0), allocSize(0)
	{
	}


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(ComPortParam)

	ComPortParam::ComPortParam() noexcept: baudRate(9600), dataBits(8), stopBits(1)
	{
	}


	SLIB_DEFINE_HANDLE_CONTAINER_MEMBERS(File, sl_file, m_file, SLIB_FILE_INVALID_HANDLE, _close)
	SLIB_DEFINE_IO_MEMBERS(File, const noexcept)
		
	SLIB_DEFINE_NESTED_CLASS_DEFAULT_MEMBERS(File, OpenParam)

	File::OpenParam::OpenParam() noexcept
	{
	}

	File File::open(const StringParam& filePath, const OpenParam& param) noexcept
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

	File File::openCOM(sl_uint32 no, const FileMode& mode)
	{
#ifdef SLIB_PLATFORM_IS_WIN32
		return openDevice(String::concat("\\\\.\\COM", String::fromUint32(no)), mode);
#endif
		return SLIB_FILE_INVALID_HANDLE;
	}

	void File::close() noexcept
	{
		setNone();
	}

	void File::close(sl_file handle) noexcept
	{
		_close(handle);
	}

	sl_reg File::read(void* buf, sl_size size, sl_int32 timeout) const noexcept
	{
		return ReaderHelper::readWithRead32(this, buf, size, timeout);
	}

	sl_reg File::write(const void* buf, sl_size size, sl_int32 timeout) const noexcept
	{
		return WriterHelper::writeWithWrite32(this, buf, size, timeout);
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

	String File::joinPath(const StringParam* params, sl_size count) noexcept
	{
		if (!count) {
			return sl_null;
		}
		if (count == 1) {
			return params->toString();
		}
		sl_size len = 0;
		sl_size i;
		for (i = 0; i < count; i++) {
			const StringParam& s = params[i];
			if (s.isNotEmpty()) {
				if (i) {
					len++;
				}
				StringRawData data;
				s.getData(data);
				sl_size n = 0;
				if (data.charSize == 1) {
					if (data.length < 0) {
						n = Base::getStringLength(data.data8);
					} else {
						n = data.length;
					}
				} else if (data.charSize == 2) {
					n = Charsets::utf16ToUtf8(data.data16, data.length, sl_null, -1);
				} else {
					n = Charsets::utf32ToUtf8(data.data32, data.length, sl_null, -1);
				}
				len += n;
			}
		}
		if (!len) {
			return String::getEmpty();
		}
		String ret = String::allocate(len);
		if (ret.isNotNull()) {
			sl_char8* dst = ret.getData();
			sl_size nWritten = 0;
			sl_bool flagFirst = sl_true;
			sl_bool flagNeedsDelimiter = sl_false;
			for (i = 0; i < count; i++) {
				const StringParam& s = params[i];
				if (s.isNotEmpty()) {
					if (flagFirst) {
						flagFirst = sl_false;
					} else {
						if (flagNeedsDelimiter) {
							dst[nWritten++] = '/';
						}
					}
					StringRawData data;
					s.getData(data);
					sl_size n = 0;
					if (data.charSize == 1) {
						if (data.length < 0) {
							n = Base::copyString(dst + nWritten, data.data8);
						} else {
							n = data.length;
							Base::copyMemory(dst + nWritten, data.data8, n);
						}
					} else if (data.charSize == 2) {
						n = Charsets::utf16ToUtf8(data.data16, data.length, dst + nWritten, -1);
					} else {
						n = Charsets::utf32ToUtf8(data.data32, data.length, dst + nWritten, -1);
					}
					if (n) {
						nWritten += n;
						sl_char8 last = dst[nWritten - 1];
						flagNeedsDelimiter = !(last == '\\' || last == '/');
					} else {
						flagNeedsDelimiter = sl_false;
					}
				}
			}
			if (nWritten == len) {
				return ret;
			} else {
				return ret.substring(0, nWritten);
			}
		}
		return sl_null;
	}

	namespace
	{
		template <class CHAR>
		static sl_compare_result ComparePathChar(CHAR c1, CHAR c2, sl_bool flagIgnoreCase)
		{
			if (c1 == '\\') {
				c1 = '/';
			}
			if (c2 == '\\') {
				c2 = '/';
			}
			if (flagIgnoreCase) {
				c1 = SLIB_CHAR_UPPER_TO_LOWER(c1);
				c2 = SLIB_CHAR_UPPER_TO_LOWER(c2);
			}
			return ComparePrimitiveValues(c1, c2);
		}

		template <class VIEW>
		static sl_compare_result ComparePath(const VIEW& s1, const VIEW& s2, sl_bool flagIgnoreCase)
		{
			auto d1 = s1.getData();
			sl_size n1 = s1.getLength();
			auto d2 = s2.getData();
			sl_size n2 = s2.getLength();
			sl_size n = SLIB_MIN(n1, n2);
			for (sl_size i = 0; i < n; i++) {
				sl_compare_result r = ComparePathChar(*d1, *d2, flagIgnoreCase);
				if (r) {
					return r;
				}
				d1++;
				d2++;
			}
			if (n1 == n2) {
				return 0;
			}
			if (n1 > n2) {
				return 1;
			} else {
				return -1;
			}
		}

		template <class CHAR>
		static sl_bool EqualsPathChar(CHAR c1, CHAR c2, sl_bool flagIngoreCase)
		{
			if (c1 == '\\') {
				c1 = '/';
			}
			if (c2 == '\\') {
				c2 = '/';
			}
			if (flagIngoreCase) {
				return SLIB_CHAR_UPPER_TO_LOWER(c1) == SLIB_CHAR_UPPER_TO_LOWER(c2);
			} else {
				return c1 == c2;
			}
		}

		template <class VIEW>
		static sl_bool EqualsPath(const VIEW& s1, const VIEW& s2, sl_bool flagIgnoreCase)
		{
			sl_size n = s1.getLength();
			if (n != s2.getLength()) {
				return sl_false;
			}
			auto d1 = s1.getData();
			auto d2 = s2.getData();
			for (sl_size i = 0; i < n; i++) {
				if (!(EqualsPathChar(*d1, *d2, flagIgnoreCase))) {
					return sl_false;
				}
				d1++;
				d2++;
			}
			return sl_true;
		}
	}

	sl_compare_result File::comparePath(const StringView& s1, const StringView& s2, sl_bool flagIgnoreCase) noexcept
	{
		return ComparePath(s1, s2, flagIgnoreCase);
	}

	sl_compare_result File::comparePath(const StringView16& s1, const StringView16& s2, sl_bool flagIgnoreCase) noexcept
	{
		return ComparePath(s1, s2, flagIgnoreCase);
	}

	sl_compare_result File::comparePath(const StringView32& s1, const StringView32& s2, sl_bool flagIgnoreCase) noexcept
	{
		return ComparePath(s1, s2, flagIgnoreCase);
	}

	sl_bool File::equalsPath(const StringView& s1, const StringView& s2, sl_bool flagIgnoreCase) noexcept
	{
		return EqualsPath(s1, s2, flagIgnoreCase);
	}

	sl_bool File::equalsPath(const StringView16& s1, const StringView16& s2, sl_bool flagIgnoreCase) noexcept
	{
		return EqualsPath(s1, s2, flagIgnoreCase);
	}

	sl_bool File::equalsPath(const StringView32& s1, const StringView32& s2, sl_bool flagIgnoreCase) noexcept
	{
		return EqualsPath(s1, s2, flagIgnoreCase);
	}

	Memory File::readAllBytes(const StringParam& path, sl_size maxSize) noexcept
	{
		File file = openForRead(path);
		if (file.isNotNone()) {
			return file.readAllBytes(maxSize);
		}
		return sl_null;
	}

	String File::readAllTextUTF8(const StringParam& path, sl_size maxSize) noexcept
	{
		File file = openForRead(path);
		if (file.isNotNone()) {
			return TextIO::readAllUTF8(&file);
		}
		return sl_null;
	}

	String16 File::readAllTextUTF16(const StringParam& path, EndianType endian, sl_size maxSize) noexcept
	{
		File file = openForRead(path);
		if (file.isNotNone()) {
			return TextIO::readAllUTF16(&file, endian, maxSize);
		}
		return sl_null;
	}

	StringParam File::readAllText(const StringParam& path, sl_size maxSize) noexcept
	{
		File file = openForRead(path);
		if (file.isNotNone()) {
			return TextIO::readAll(&file, maxSize);
		}
		return sl_null;
	}

	sl_bool File::writeAllBytes(const StringParam& path, const void* buf, sl_size size) noexcept
	{
		File file = openForWrite(path);
		if (file.isNotNone()) {
			if (size) {
				return file.writeAll(buf, size);
			} else {
				return sl_true;
			}
		} else {
			return sl_false;
		}
	}

	sl_bool File::writeAllBytes(const StringParam& path, const MemoryView& mem) noexcept
	{
		return writeAllBytes(path, mem.data, mem.size);
	}

	sl_bool File::writeAllBytes(const StringParam& path, const StringView& str) noexcept
	{
		return writeAllBytes(path, str.getData(), str.getLength());
	}

	sl_bool File::writeAllTextUTF8(const StringParam& path, const StringView& text, sl_bool flagWriteByteOrderMark) noexcept
	{
		File file = openForWrite(path);
		if (file.isNotNone()) {
			return TextIO::writeUTF8(&file, text, flagWriteByteOrderMark);
		}
		return sl_false;
	}

	sl_bool File::writeAllTextUTF16LE(const StringParam& path, const StringView16& text, sl_bool flagWriteByteOrderMark) noexcept
	{
		File file = openForWrite(path);
		if (file.isNotNone()) {
			return TextIO::writeUTF16LE(&file, text, flagWriteByteOrderMark);
		}
		return sl_false;
	}

	sl_bool File::writeAllTextUTF16BE(const StringParam& path, const StringView16& text, sl_bool flagWriteByteOrderMark) noexcept
	{
		File file = openForWrite(path);
		if (file.isNotNone()) {
			return TextIO::writeUTF16BE(&file, text, flagWriteByteOrderMark);
		}
		return sl_false;
	}

	sl_bool File::appendAllBytes(const StringParam& path, const void* buf, sl_size size) noexcept
	{
		File file = openForAppend(path);
		if (file.isNotNone()) {
			if (size) {
				return file.writeAll(buf, size);
			} else {
				return sl_true;
			}
		} else {
			return sl_false;
		}
	}

	sl_bool File::appendAllBytes(const StringParam& path, const MemoryView& mem) noexcept
	{
		return appendAllBytes(path, mem.data, mem.size);
	}

	sl_bool File::appendAllBytes(const StringParam& path, const StringView& str) noexcept
	{
		return appendAllBytes(path, str.getData(), str.getLength());
	}

	sl_bool File::appendAllTextUTF8(const StringParam& path, const StringView& text) noexcept
	{
		File file = openForAppend(path);
		if (file.isNotNone()) {
			return TextIO::writeUTF8(&file, text, sl_false);
		}
		return sl_false;
	}

	sl_bool File::appendAllTextUTF16LE(const StringParam& path, const StringView16& text) noexcept
	{
		File file = openForAppend(path);
		if (file.isNotNone()) {
			return TextIO::writeUTF16LE(&file, text, sl_false);
		}
		return sl_false;
	}

	sl_bool File::appendAllTextUTF16BE(const StringParam& path, const StringView16& text) noexcept
	{
		File file = openForAppend(path);
		if (file.isNotNone()) {
			return TextIO::writeUTF16BE(&file, text, sl_false);
		}
		return sl_false;
	}

	List<String> File::getAllDescendantFiles(const StringParam& _dirPath) noexcept
	{
		String dirPath = _dirPath.toString();
		if (!isDirectory(dirPath)) {
			return sl_null;
		}
		List<String> listCurrent = getFiles(dirPath);
		listCurrent.sort_NoLock();
		ListElements<String> current(listCurrent);
		if (!(current.count)) {
			return sl_null;
		}
		String dirPathPrefix = dirPath + "/";
		List<String> ret;
		for (sl_size i = 0; i < current.count; i++) {
			String& item = current[i];
			ret.add_NoLock(item);
			String dir = dirPathPrefix + item;
			if (isDirectory(dir)) {
				ListElements<String> sub(getAllDescendantFiles(dir));
				if (sub.count) {
					String itemPrefix = item + "/";
					for (sl_size j = 0; j < sub.count; j++) {
						ret.add_NoLock(itemPrefix + sub[j]);
					}
				}
			}
		}
		return ret;
	}

	sl_bool File::createDirectory(const StringParam& dirPath, const FileOperationFlags& flags) noexcept
	{
		FileAttributes attr = getAttributes(dirPath);
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
		if (isDirectory(dirPath)) {
			return sl_true;
		}
		if (isFile(dirPath)) {
			return sl_false;
		}
		String parent = getParentDirectoryPath(dirPath.toString());
		if (parent.isEmpty()) {
			return createDirectory(dirPath);
		} else {
			if (createDirectories(parent)) {
				return createDirectory(dirPath);
			}
			return sl_false;
		}
	}

	sl_bool File::remove(const StringParam& path, const FileOperationFlags& flags) noexcept
	{
		FileAttributes attr = getAttributes(path);
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
				ListElements<String> list(getFiles(path));
				for (sl_size i = 0; i < list.count; i++) {
					ret = ret && File::remove(concatPath(path, list[i]), flags);
					if (!ret) {
						if (flags & FileOperationFlags::AbortOnError) {
							return sl_false;
						}
					}
				}
				ret = ret && deleteDirectory(path);
				return ret;
			} else {
				return deleteDirectory(path);
			}
		} else {
			return deleteFile(path);
		}
	}

	sl_bool File::copyFile(const StringParam& pathSource, const StringParam& pathTarget, const FileOperationFlags& flags) noexcept
	{
		if (flags & FileOperationFlags::NotReplace) {
			FileAttributes attr = getAttributes(pathTarget);
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
		FileAttributes attr = getAttributes(pathSource);
		if (attr & FileAttributes::NotExist) {
			return sl_false;
		}
		if (attr & FileAttributes::Directory) {
			if (!(createDirectory(pathTarget))) {
				return sl_false;
			}
			sl_bool ret = sl_true;
			ListElements<String> list(getFiles(pathSource));
			for (sl_size i = 0; i < list.count; i++) {
				if (flags & FileOperationFlags::Recursive) {
					ret = ret && copy(concatPath(pathSource, list[i]), concatPath(pathTarget, list[i]), flags);
				} else {
					ret = ret && copyFile(concatPath(pathSource, list[i]), concatPath(pathTarget, list[i]), flags);
				}
				if (!ret) {
					if (flags & FileOperationFlags::AbortOnError) {
						return sl_false;
					}
				}
			}
			return ret;
		} else {
			if (isDirectory(pathTarget)) {
				return copyFile(pathSource, concatPath(pathTarget, getFileName(pathSource)), flags);
			} else {
				return copyFile(pathSource, pathTarget, flags);
			}
		}
	}

	sl_bool File::move(const StringParam& pathOriginal, const StringParam& filePathNew, const FileOperationFlags& flags) noexcept
	{
		if (flags & FileOperationFlags::NotReplace) {
			FileAttributes attr = getAttributes(filePathNew);
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
			if (exists(filePathNew)) {
				remove(filePathNew);
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
			if (exists(String::concat(path, "/", filePath))) {
				return path;
			}
			segments.segments.popBack();
		}
		return sl_null;
	}

	sl_bool File::isDotOrDotDot(const char* name) noexcept
	{
		if (*name == '.') {
			char ch = name[1];
			if (!ch) {
				return sl_true;
			}
			if (ch == '.') {
				if (!(name[2])) {
					return sl_true;
				}
			}
		}
		return sl_false;
	}

	sl_bool File::isDotOrDotDot(const sl_char16* name) noexcept
	{
		if (*name == '.') {
			sl_char16 ch = name[1];
			if (!ch) {
				return sl_true;
			}
			if (ch == '.') {
				if (!(name[2])) {
					return sl_true;
				}
			}
		}
		return sl_false;
	}

	namespace
	{
		template <class CHAR>
		static sl_bool IsWindowsAbsolutePath(const CHAR* s, sl_reg len)
		{
			if (len >= 0 && len < 2) {
				return sl_false;
			}
			CHAR drive = *s;
			if (!SLIB_CHAR_IS_ALPHA(drive)) {
				return sl_false;
			}
			if (s[1] != ':') {
				return sl_false;
			}
			if (len == 2) {
				return sl_true;
			}
			CHAR sep = s[2];
			return sep == '\\' || sep == '/' || (len < 0 && !sep);
		}

		template <class CHAR>
		static sl_bool IsUnixAbsolutePath(const CHAR* s, sl_reg len)
		{
			if (!len) {
				return sl_false;
			}
			return *s == '/';
		}

#ifdef SLIB_PLATFORM_IS_WINDOWS
		template <class CHAR>
		static sl_bool IsAbsolutePath(const CHAR* s, sl_reg len)
		{
			return IsWindowsAbsolutePath(s, len);
		}
#else
		template <class CHAR>
		static sl_bool IsAbsolutePath(const CHAR* s, sl_reg len)
		{
			return IsUnixAbsolutePath(s, len);
		}
#endif
	}

	sl_bool File::isAbsolutePath(const StringView& path) noexcept
	{
		return IsAbsolutePath(path.getData(), path.getUnsafeLength());
	}

	sl_bool File::isAbsolutePath(const StringView16& path) noexcept
	{
		return IsAbsolutePath(path.getData(), path.getUnsafeLength());
	}

	sl_bool File::isAbsolutePath(const StringView32& path) noexcept
	{
		return IsAbsolutePath(path.getData(), path.getUnsafeLength());
	}

	sl_bool File::isWindowsAbsolutePath(const StringView& path) noexcept
	{
		return IsWindowsAbsolutePath(path.getData(), path.getUnsafeLength());
	}

	sl_bool File::isWindowsAbsolutePath(const StringView16& path) noexcept
	{
		return IsWindowsAbsolutePath(path.getData(), path.getUnsafeLength());
	}

	sl_bool File::isWindowsAbsolutePath(const StringView32& path) noexcept
	{
		return IsWindowsAbsolutePath(path.getData(), path.getUnsafeLength());
	}

	sl_bool File::isUnixAbsolutePath(const StringView& path) noexcept
	{
		return IsUnixAbsolutePath(path.getData(), path.getUnsafeLength());
	}

	sl_bool File::isUnixAbsolutePath(const StringView16& path) noexcept
	{
		return IsUnixAbsolutePath(path.getData(), path.getUnsafeLength());
	}

	sl_bool File::isUnixAbsolutePath(const StringView32& path) noexcept
	{
		return IsUnixAbsolutePath(path.getData(), path.getUnsafeLength());
	}


	SLIB_DEFINE_ROOT_OBJECT(FileIO)

	FileIO::FileIO(File&& handle): IO(Move(handle))
	{
	}

	FileIO::~FileIO()
	{
	}

	Ref<FileIO> FileIO::create(File&& handle) noexcept
	{
		if (handle.isOpened()) {
			return new FileIO(Move(handle));
		}
		return sl_null;
	}

#define DEFINE_FILEIO_OPEN_FUNC(FUNC, ...) \
		return create(File::FUNC(__VA_ARGS__));

	Ref<FileIO> FileIO::open(const StringParam& filePath, const File::OpenParam& param) noexcept
	{
		DEFINE_FILEIO_OPEN_FUNC(open, filePath, param)
	}

	Ref<FileIO> FileIO::open(const StringParam& filePath, const FileMode& mode, const FileAttributes& attrs) noexcept
	{
		DEFINE_FILEIO_OPEN_FUNC(open, filePath, mode, attrs)
	}

	Ref<FileIO> FileIO::open(const StringParam& filePath, const FileMode& mode) noexcept
	{
		DEFINE_FILEIO_OPEN_FUNC(open, filePath, mode)
	}

	Ref<FileIO> FileIO::openForRead(const StringParam& filePath) noexcept
	{
		DEFINE_FILEIO_OPEN_FUNC(openForRead, filePath)
	}

	Ref<FileIO> FileIO::openForWrite(const StringParam& filePath) noexcept
	{
		DEFINE_FILEIO_OPEN_FUNC(openForWrite, filePath)
	}

	Ref<FileIO> FileIO::openForReadWrite(const StringParam& filePath) noexcept
	{
		DEFINE_FILEIO_OPEN_FUNC(openForReadWrite, filePath)
	}

	Ref<FileIO> FileIO::openForAppend(const StringParam& filePath) noexcept
	{
		DEFINE_FILEIO_OPEN_FUNC(openForAppend, filePath)
	}

	Ref<FileIO> FileIO::openForRandomAccess(const StringParam& filePath) noexcept
	{
		DEFINE_FILEIO_OPEN_FUNC(openForRandomAccess, filePath)
	}

	Ref<FileIO> FileIO::openForRandomRead(const StringParam& filePath) noexcept
	{
		DEFINE_FILEIO_OPEN_FUNC(openForRandomRead, filePath)
	}

	Ref<FileIO> FileIO::openDevice(const StringParam& path, const FileMode& mode) noexcept
	{
		DEFINE_FILEIO_OPEN_FUNC(openDevice, path, mode)
	}

	Ref<FileIO> FileIO::openDeviceForRead(const StringParam& path) noexcept
	{
		DEFINE_FILEIO_OPEN_FUNC(openDeviceForRead, path)
	}

	sl_bool FileIO::lock(sl_uint64 offset, sl_uint64 length, sl_bool flagShared, sl_bool flagWait) const noexcept
	{
		return base.lock(offset, length, flagShared, flagWait);
	}

	sl_bool FileIO::unlock(sl_uint64 offset, sl_uint64 length) const noexcept
	{
		return base.unlock(offset, length);
	}

	sl_bool FileIO::flush() const noexcept
	{
		return base.flush();
	}

	sl_bool FileIO::setNonBlocking(sl_bool flag) const noexcept
	{
		return base.setNonBlocking(flag);
	}

	sl_bool FileIO::getDiskSize(sl_uint64& outSize) const noexcept
	{
		return base.getDiskSize(outSize);
	}

	sl_uint64 FileIO::getDiskSize() const noexcept
	{
		return base.getDiskSize();
	}

	Time FileIO::getModifiedTime() const noexcept
	{
		return base.getModifiedTime();
	}

	Time FileIO::getAccessedTime() const noexcept
	{
		return base.getAccessedTime();
	}

	Time FileIO::getCreatedTime() const noexcept
	{
		return base.getCreatedTime();
	}

	sl_bool FileIO::setModifiedTime(const Time& time) const noexcept
	{
		return base.setModifiedTime(time);
	}

	sl_bool FileIO::setAccessedTime(const Time& time) const noexcept
	{
		return base.setAccessedTime(time);
	}

	sl_bool FileIO::setCreatedTime(const Time& time) const noexcept
	{
		return base.setCreatedTime(time);
	}

	FileAttributes FileIO::getAttributes() const noexcept
	{
		return base.getAttributes();
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

}
