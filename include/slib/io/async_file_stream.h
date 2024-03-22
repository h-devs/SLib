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

#ifndef CHECKHEADER_SLIB_IO_ASYNC_FILE_STREAM
#define CHECKHEADER_SLIB_IO_ASYNC_FILE_STREAM

#include "async_stream.h"
#include "file.h"

namespace slib
{

	class SLIB_EXPORT AsyncFileStreamParam
	{
	public:
		sl_file handle;
		sl_bool flagCloseOnRelease;
		AsyncIoMode mode;
		Ref<AsyncIoLoop> ioLoop;

		sl_uint64 initialPosition;
		sl_bool flagSupportSeeking;

	public:
		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(AsyncFileStreamParam)

		AsyncFileStreamParam();

	public:
#ifdef SLIB_PLATFORM_IS_WIN32
		sl_bool openFile(const StringParam& filePath, FileMode mode);
#endif

	};

	class SLIB_EXPORT AsyncFileStreamInstance : public AsyncStreamInstance
	{
		SLIB_DECLARE_OBJECT

	public:
		AsyncFileStreamInstance();

		~AsyncFileStreamInstance();

	protected:
		void onClose() override;

	private:
		void _free();

	protected:
		Ref<AsyncStreamRequest> m_requestReading;
		Ref<AsyncStreamRequest> m_requestWriting;
		sl_bool m_flagCloseOnRelease;
		sl_bool m_flagFreed;

	};

	// [Linux] `AsyncFileStream` does not work on regular files
	class SLIB_EXPORT AsyncFileStream : public AsyncStreamBase
	{
		SLIB_DECLARE_OBJECT

	protected:
		AsyncFileStream();

		~AsyncFileStream();

	public:
		static Ref<AsyncFileStream> create(AsyncFileStreamInstance* instance, AsyncIoMode mode, const Ref<AsyncIoLoop>& loop);

		static Ref<AsyncFileStream> create(AsyncFileStreamInstance* instance, AsyncIoMode mode);

		static Ref<AsyncFileStream> create(const AsyncFileStreamParam& param);

	public:
		Ref<AsyncFileStreamInstance> getIoInstance();

		sl_file getHandle();

	};

}

#endif
