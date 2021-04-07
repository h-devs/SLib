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

#include "async_stream_simulator.h"
#include "file.h"

namespace slib
{

	class SLIB_EXPORT AsyncFile : public AsyncStreamSimulator
	{
		SLIB_DECLARE_OBJECT

	protected:
		AsyncFile();

		~AsyncFile();

	public:
		static Ref<AsyncFile> create(const Ref<File>& file);

		static Ref<AsyncFile> create(const Ref<File>& file, const Ref<Dispatcher>& dispatcher);


		static Ref<AsyncFile> open(const StringParam& path, FileMode mode);

		static Ref<AsyncFile> open(const StringParam& path, FileMode mode, const Ref<Dispatcher>& dispatcher);


		static Ref<AsyncFile> openForRead(const StringParam& path);

		static Ref<AsyncFile> openForRead(const StringParam& path, const Ref<Dispatcher>& dispatcher);


		static Ref<AsyncFile> openForWrite(const StringParam& path);

		static Ref<AsyncFile> openForWrite(const StringParam& path, const Ref<Dispatcher>& dispatcher);


		static Ref<AsyncFile> openForAppend(const StringParam& path);

		static Ref<AsyncFile> openForAppend(const StringParam& path, const Ref<Dispatcher>& dispatcher);


#if defined(SLIB_PLATFORM_IS_WIN32)
		static Ref<AsyncStream> openIOCP(const StringParam& path, FileMode mode, const Ref<AsyncIoLoop>& loop);

		static Ref<AsyncStream> openIOCP(const StringParam& path, FileMode mode);
#endif

	public:
		void close() override;

		sl_bool isOpened() override;

		sl_bool isSeekable() override;

		sl_bool seek(sl_uint64 pos) override;

		sl_uint64 getSize() override;

	public:
		Ref<File> getFile();

	protected:
		void processRequest(AsyncStreamRequest* request) override;

	private:
		AtomicRef<File> m_file;

	};

}

#endif
