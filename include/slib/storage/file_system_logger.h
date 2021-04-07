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

#ifndef CHECKHEADER_SLIB_STORAGE_FILE_SYSTEM_LOGGER
#define CHECKHEADER_SLIB_STORAGE_FILE_SYSTEM_LOGGER

#include "file_system.h"

#include "../core/regex.h"

namespace slib
{

	SLIB_DEFINE_FLAGS(FileSystemLogFlags, {
		
		Create = 0x01,
		Open = 0x02,
		Flush = 0x04,
		Close = 0x08,
		Read = 0x10,
		Write = 0x20,
		Delete = 0x40,
		Move = 0x80,
		OpenOp = Create | Open | Close,
		CreateOpen = Create | Open,
		ReadWrite = Read | Write,

		BasicOp = 0xFF,

		GetInfo = 0x0100,
		SetInfo = 0x0200,
		List = 0x0400,
		Info = 0x0800,
		Size = 0x1000,
		InfoOp = GetInfo | SetInfo,

		AllOp = 0xFFFF,

		TimeInfo = 0x01000000,
		TimeInfoAsInt = 0x02000000,
		FileName = 0x04000000,
		ContextAddress = 0x08000000,

		RetSuccess = 0x10000000,
		RetFail = 0x20000000,
		ErrorString = 0x80000000,
		Ret = RetSuccess | RetFail,

		Success = FileName | RetSuccess,
		Errors = FileName | RetFail | ErrorString,
		Default = FileName | Ret | ErrorString,

		All = 0xFFFFFFFF

	})

	class FileSystemLogger : public FileSystemWrapper
	{
		SLIB_DECLARE_OBJECT
		
	public:
		FileSystemLogger(const Ref<FileSystemProvider>& base);

		FileSystemLogger(const Ref<FileSystemProvider>& base, const FileSystemLogFlags& flags);

		FileSystemLogger(const Ref<FileSystemProvider>& base, const FileSystemLogFlags& flags, const String& filterRegex);

		~FileSystemLogger();

	public:
		sl_bool getInformation(FileSystemInfo& outInfo) override;

		sl_bool getSize(sl_uint64* pTotalSize, sl_uint64* pFreeSize = sl_null) override;

		sl_bool createDirectory(const StringParam& path) override;

		Ref<FileContext> createContext(const StringParam& path) override;

		Ref<FileContext> openFile(const StringParam& path, const FileOpenParam& param) override;

		sl_uint32 readFile(FileContext* context, sl_uint64 offset, void* buf, sl_uint32 size) override;

		sl_uint32 writeFile(FileContext* context, sl_int64 offset, const void* buf, sl_uint32 size) override;

		sl_bool flushFile(FileContext* context) override;

		sl_bool	closeFile(FileContext* context) override;

		sl_bool deleteDirectory(const StringParam& path) override;

		sl_bool deleteFile(const StringParam& path) override;

		sl_bool moveFile(const StringParam& pathOld, const StringParam& pathNew, sl_bool flagReplaceIfExists) override;

		sl_bool getFileInfo(FileContext* context, FileInfo& outInfo, const FileInfoMask& mask) override;

		sl_bool setFileInfo(FileContext* context, const FileInfo& info, const FileInfoMask& mask) override;

		HashMap<String, FileInfo> getFiles(const StringParam& pathDir) override;

	private:
		FileSystemLogFlags m_flags;

		RegEx m_regex;

	};

}

#endif