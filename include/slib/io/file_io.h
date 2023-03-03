/*
*   Copyright (c) 2008-2022 SLIBIO <https://github.com/SLIBIO>
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

#ifndef CHECKHEADER_SLIB_IO_FILE_IO
#define CHECKHEADER_SLIB_IO_FILE_IO

#include "file.h"
#include "io.h"

namespace slib
{

	class FileIO : public Referable, public IO<File>
	{
	public:
		SLIB_DECLARE_OBJECT

	protected:
		FileIO(File&& handle);

		~FileIO();

	public:
		static Ref<FileIO> create(File&& handle) noexcept;

		static Ref<FileIO> open(const StringParam& filePath, const File::OpenParam& param) noexcept;

		static Ref<FileIO> open(const StringParam& filePath, const FileMode& mode, const FileAttributes& attrs) noexcept;

		static Ref<FileIO> open(const StringParam& filePath, const FileMode& mode) noexcept;

		static Ref<FileIO> openForRead(const StringParam& filePath) noexcept;

		static Ref<FileIO> openForWrite(const StringParam& filePath) noexcept;

		static Ref<FileIO> openForReadWrite(const StringParam& filePath) noexcept;

		static Ref<FileIO> openForAppend(const StringParam& filePath) noexcept;

		static Ref<FileIO> openForRandomAccess(const StringParam& filePath) noexcept;

		static Ref<FileIO> openForRandomRead(const StringParam& filePath) noexcept;

		static Ref<FileIO> openDevice(const StringParam& path, const FileMode& mode) noexcept;

		static Ref<FileIO> openDeviceForRead(const StringParam& path) noexcept;

	public:
		sl_bool lock(sl_uint64 offset = 0, sl_uint64 length = 0, sl_bool flagShared = sl_false, sl_bool flagWait = sl_false) const noexcept;

		sl_bool unlock(sl_uint64 offset = 0, sl_uint64 length = 0) const noexcept;

		sl_bool flush() const noexcept;

		// Unix only
		sl_bool setNonBlocking(sl_bool flag = sl_true) const noexcept;

		sl_bool getDiskSize(sl_uint64& outSize) const noexcept;

		sl_uint64 getDiskSize() const noexcept;


		Time getModifiedTime() const noexcept;

		Time getAccessedTime() const noexcept;

		Time getCreatedTime() const noexcept;

		sl_bool setModifiedTime(const Time& time) const noexcept;

		sl_bool setAccessedTime(const Time& time) const noexcept;

		sl_bool setCreatedTime(const Time& time) const noexcept;

		FileAttributes getAttributes() const noexcept;

	};

}

#endif
