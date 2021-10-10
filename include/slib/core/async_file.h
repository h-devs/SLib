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

#ifndef CHECKHEADER_SLIB_CORE_ASYNC_FILE
#define CHECKHEADER_SLIB_CORE_ASYNC_FILE

#include "async_stream.h"
#include "file.h"

namespace slib
{

	class SLIB_EXPORT AsyncFileParam
	{
	public:
		sl_file handle;
		sl_bool flagCloseOnRelease;
		Ref<AsyncIoLoop> ioLoop;

		sl_int64 initialPosition;

	public:
		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(AsyncFileParam)

		AsyncFileParam();

	public:
		sl_bool open(const StringParam& path, FileMode mode);

	};

	class SLIB_EXPORT AsyncFileInstance : public AsyncStreamInstance
	{
		SLIB_DECLARE_OBJECT

	public:
		AsyncFileInstance();

		~AsyncFileInstance();

	public:
		void close();

	protected:
		sl_bool m_flagCloseOnRelease;

	};

	class SLIB_EXPORT AsyncFile : public AsyncStreamBase
	{
		SLIB_DECLARE_OBJECT

	protected:
		AsyncFile();

		~AsyncFile();

	public:
		static Ref<AsyncFile> create(AsyncFileInstance* instance, AsyncIoMode mode, const Ref<AsyncIoLoop>& loop);

		static Ref<AsyncFile> create(AsyncFileInstance* instance, AsyncIoMode mode);
		
	public:
		static Ref<AsyncFile> create(const AsyncFileParam& param);

		static Ref<AsyncFile> open(const StringParam& path, FileMode mode);

		static Ref<AsyncFile> open(const StringParam& path, FileMode mode, const Ref<AsyncIoLoop>& loop);

		static Ref<AsyncFile> openForRead(const StringParam& path);

		static Ref<AsyncFile> openForRead(const StringParam& path, const Ref<AsyncIoLoop>& loop);

		static Ref<AsyncFile> openForWrite(const StringParam& path);

		static Ref<AsyncFile> openForWrite(const StringParam& path, const Ref<AsyncIoLoop>& loop);

		static Ref<AsyncFile> openForAppend(const StringParam& path);

		static Ref<AsyncFile> openForAppend(const StringParam& path, const Ref<AsyncIoLoop>& loop);

	public:
		Ref<AsyncFileInstance> getIoInstance();

		void close() override;

		sl_file getHandle();

	};

}

#endif
