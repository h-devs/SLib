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

#ifndef CHECKHEADER_SLIB_CORE_ASYNC_OUTPUT
#define CHECKHEADER_SLIB_CORE_ASYNC_OUTPUT

#include "async_stream.h"
#include "memory_queue.h"

namespace slib
{

	class AsyncCopy;

	class SLIB_EXPORT AsyncOutputBufferElement : public Referable
	{
	public:
		AsyncOutputBufferElement();

		AsyncOutputBufferElement(const Memory& header);

		AsyncOutputBufferElement(AsyncStream* stream, sl_uint64 size);

		~AsyncOutputBufferElement();

		SLIB_DELETE_CLASS_DEFAULT_MEMBERS(AsyncOutputBufferElement)

	public:
		sl_bool isEmpty() const;

		sl_bool isEmptyBody() const;


		sl_bool addHeader(const Memory& header);

		void setBody(AsyncStream* stream, sl_uint64 size);

		MemoryQueue& getHeader();

		Ref<AsyncStream> getBody();

		sl_uint64 getBodySize();

	protected:
		MemoryQueue m_header;
		sl_uint64 m_sizeBody;
		AtomicRef<AsyncStream> m_body;

	};

	class SLIB_EXPORT AsyncOutputBuffer : public Object
	{
		SLIB_DECLARE_OBJECT

	public:
		AsyncOutputBuffer();

		~AsyncOutputBuffer();

	public:
		void clearOutput();

		sl_bool write(const void* buf, sl_size size);

		sl_bool write(const Memory& mem);

		sl_bool copyFrom(AsyncStream* stream, sl_uint64 size);

		sl_bool copyFromFile(const StringParam& path);

		sl_bool copyFromFile(const StringParam& path, const Ref<AsyncIoLoop>& ioLoop, const Ref<Dispatcher>& dispatcher);

		sl_uint64 getOutputLength() const;

	protected:
		sl_uint64 m_lengthOutput;
		LinkedQueue< Ref<AsyncOutputBufferElement> > m_queueOutput;

		friend class AsyncOutput;
	};

	class AsyncOutput;

	class AsyncOutputParam
	{
	public:
		Ref<AsyncStream> stream;

		// optional
		sl_uint32 bufferSize;
		sl_uint32 bufferCount;

		Function<void(AsyncOutput*, sl_bool flagError)> onEnd;

	public:
		AsyncOutputParam();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(AsyncOutputParam)

	};

	class SLIB_EXPORT AsyncOutput : public AsyncOutputBuffer
	{
		SLIB_DECLARE_OBJECT

	protected:
		AsyncOutput();

		~AsyncOutput();

	public:
		static Ref<AsyncOutput> create(const AsyncOutputParam& param);

	public:
		void mergeBuffer(AsyncOutputBuffer* buffer);

		void startWriting();

		sl_bool isWriting();

		void close();

	private:
		void onAsyncCopyEnd(AsyncCopy* task, sl_bool flagError);

		void onWriteStream(AsyncStreamResult& result);

	protected:
		void _onError();

		void _onComplete();

		void _write(sl_bool flagCompleted);

	protected:
		Ref<AsyncStream> m_streamOutput;
		sl_uint32 m_bufferSize;
		sl_uint32 m_bufferCount;

		Function<void(AsyncOutput*, sl_bool)> m_onEnd;

		Ref<AsyncOutputBufferElement> m_elementWriting;
		Ref<AsyncCopy> m_copy;
		Memory m_bufWrite;
		sl_bool m_flagWriting;
		sl_bool m_flagClosed;

	};

}

#endif
