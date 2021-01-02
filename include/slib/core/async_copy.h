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

#ifndef CHECKHEADER_SLIB_CORE_ASYNC_COPY
#define CHECKHEADER_SLIB_CORE_ASYNC_COPY

#include "definition.h"

#include "async_stream.h"
#include "memory.h"

namespace slib
{

	class AsyncCopy;

	class SLIB_EXPORT AsyncCopyParam
	{
	public:
		// required
		Ref<AsyncStream> source;
		Ref<AsyncStream> target;

		// optional
		sl_uint64 size; // default: Maximum
		sl_uint32 bufferSize; // default: 0x10000
		sl_uint32 bufferCount; // default: 8
		sl_bool flagAutoStart; // default: true

		Function<Memory(AsyncCopy*, const Memory& input)> onRead;
		Function<void(AsyncCopy*)> onWrite;
		Function<void(AsyncCopy*, sl_bool flagError)> onEnd;

	public:
		AsyncCopyParam();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(AsyncCopyParam)

	};

	class SLIB_EXPORT AsyncCopy : public Object
	{
		SLIB_DECLARE_OBJECT

	private:
		AsyncCopy();

		~AsyncCopy();

	public:
		static Ref<AsyncCopy> create(const AsyncCopyParam& param);

	public:
		sl_bool start();

		void close();

		sl_bool isRunning();

		Ref<AsyncStream> getSource();

		Ref<AsyncStream> getTarget();

		sl_uint64 getTotalSize();

		sl_uint64 getReadSize();

		sl_uint64 getWrittenSize();

		sl_bool isCompleted();

		sl_bool isErrorOccured();

		sl_bool isReadingErrorOccured();

		sl_bool isWritingErrorOccured();

		sl_bool isReading();

		sl_bool isWriting();

	private:
		void onReadStream(AsyncStreamResult& result);

		void onWriteStream(AsyncStreamResult& result);

		void enqueue();

		Memory dispatchRead(const Memory& input);

		void dispatchWrite();

		void dispatchEnd();

	private:
		Ref<AsyncStream> m_source;
		Ref<AsyncStream> m_target;
		Function<Memory(AsyncCopy*, const Memory& input)> m_onRead;
		Function<void(AsyncCopy*)> m_onWrite;
		Function<void(AsyncCopy*, sl_bool flagError)> m_onEnd;
		sl_uint64 m_sizeRead;
		sl_uint64 m_sizeWritten;
		sl_uint64 m_sizeTotal;
		sl_bool m_flagReadError;
		sl_bool m_flagWriteError;
		sl_bool m_flagStarted;
		sl_bool m_flagRunning;
		sl_bool m_flagEnqueue;

		class Buffer : public Referable
		{
		public:
			Memory mem;
			Memory memRead;
			Memory memWrite;
		};
		LinkedQueue< Ref<Buffer> > m_buffersRead;
		AtomicRef<Buffer> m_bufferReading;
		LinkedQueue< Ref<Buffer> > m_buffersWrite;
		AtomicRef<Buffer> m_bufferWriting;

	};

}

#endif
