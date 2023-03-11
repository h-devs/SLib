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

#ifndef CHECKHEADER_SLIB_IO_ASYNC_STREAM_FILTER
#define CHECKHEADER_SLIB_IO_ASYNC_STREAM_FILTER

#include "async_stream.h"

#include "../core/memory_queue.h"

namespace slib
{

#define SLIB_ASYNC_STREAM_FILTER_DEFAULT_BUFFER_SIZE 16384

	class SLIB_EXPORT AsyncStreamFilter : public AsyncStream
	{
		SLIB_DECLARE_OBJECT

	protected:
		AsyncStreamFilter();

		~AsyncStreamFilter();

	public:
		Ref<AsyncStream> getSourceStream() const;

		void setSourceStream(const Ref<AsyncStream>& stream);


		void close() override;

		sl_bool isOpened() override;

		sl_bool requestIo(const Ref<AsyncStreamRequest>& request) override;

		sl_bool addTask(const Function<void()>& callback) override;


		sl_bool addReadData(void* data, sl_size size, CRef* userObject);

		sl_bool addReadData(const Memory& data);

		sl_bool addReadData(void* data, sl_size size);

		void setReadingBufferSize(sl_uint32 sizeBuffer);


		sl_bool isReadingError();

		sl_bool isReadingEnded();


		sl_bool isWritingError();

		sl_bool isWritingEnded();

	protected:
		virtual sl_bool filterRead(MemoryData& output, void* data, sl_size size, CRef* userObject);

		virtual sl_bool filterWrite(MemoryData& output, void* data, sl_size size, CRef* userObject);

	protected:
		void setReadingError();

		void setReadingEnded();

		void setWritingError();

		void setWritingEnded();

	protected:
		sl_bool m_flagOpened;

		AtomicRef<AsyncStream> m_stream;

		MemoryQueue m_bufReadConverted;
		LinkedQueue< Ref<AsyncStreamRequest> > m_requestsRead;
		Mutex m_lockReading;
		sl_bool m_flagReading;
		sl_bool m_flagReadingError;
		sl_bool m_flagReadingEnded;
		AtomicMemory m_memReading;

		Mutex m_lockWriting;
		sl_bool m_flagWritingError;
		sl_bool m_flagWritingEnded;

	protected:
		sl_bool _read();

		void _closeAllReadRequests();

	protected:
		virtual void onReadStream(AsyncStreamResult& result);

	};

}

#endif
