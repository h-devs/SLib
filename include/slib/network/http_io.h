/*
 *   Copyright (c) 2008-2018 SLIBIO <https://github.com/SLIBIO>
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

#ifndef CHECKHEADER_SLIB_NETWORK_HTTP_IO
#define CHECKHEADER_SLIB_NETWORK_HTTP_IO

#include "async.h"

#include "../io/async_output.h"
#include "../io/async_stream_filter.h"
#include "../core/string.h"
#include "../core/memory_queue.h"
#include "../data/zlib.h"

namespace slib
{

	class SLIB_EXPORT HttpOutputBuffer
	{
	public:
		HttpOutputBuffer();

		~HttpOutputBuffer();

		SLIB_DELETE_CLASS_DEFAULT_MEMBERS(HttpOutputBuffer)

	public:
		void clearOutput();

		sl_bool write(const void* buf, sl_size size);

		sl_bool write(const Memory& mem);

		sl_bool write(const StringParam& str);

		sl_bool copyFrom(AsyncStream* stream, sl_uint64 size);

		sl_bool copyFromFile(const StringParam& path);

		sl_bool copyFromFile(const StringParam& path, const Ref<AsyncIoLoop>& ioLoop, const Ref<Dispatcher>& dispatcher);

		sl_uint64 getOutputLength() const;

	protected:
		AsyncOutputBuffer m_bufferOutput;

	};

	class SLIB_EXPORT HttpHeaderReader
	{
	public:
		HttpHeaderReader();

		~HttpHeaderReader();

		SLIB_DELETE_CLASS_DEFAULT_MEMBERS(HttpHeaderReader)

	public:
		// return sl_true when body section (\r\n\r\n) is detected
		sl_bool add(const void* buf, sl_size size, sl_size& posBody);

		Memory mergeHeader();

		sl_size getHeaderSize();

		void clear();

	protected:
		sl_char16 m_last[3];
		MemoryQueue m_buffer;

	};

	typedef Function<void(void* dataRemained, sl_size sizeRemained, sl_bool flagError)> HttpContentReaderOnComplete;

	class SLIB_EXPORT HttpContentReader : public AsyncStreamFilter
	{
		SLIB_DECLARE_OBJECT

	protected:
		HttpContentReader();

		~HttpContentReader();

	public:
		static Ref<HttpContentReader> createPersistent(const Ref<AsyncStream>& io,
													   const HttpContentReaderOnComplete& onComplete,
													   sl_uint64 contentLength,
													   sl_uint32 bufferSize,
													   sl_bool flagDecompress);

		static Ref<HttpContentReader> createChunked(const Ref<AsyncStream>& io,
													const HttpContentReaderOnComplete& onComplete,
													sl_uint32 bufferSize,
													sl_bool flagDecompress);

		static Ref<HttpContentReader> createTearDown(const Ref<AsyncStream>& io,
													 const HttpContentReaderOnComplete& onComplete,
													 sl_uint32 bufferSize,
													 sl_bool flagDecompress);
	public:
		sl_bool isDecompressing();

	protected:
		void onReadStream(AsyncStreamResult& result) override;

	protected:
		void setError();

		void setCompleted(void* dataRemain, sl_size size);

		sl_bool setDecompressing();

		sl_bool decompressData(MemoryData& output, void* data, sl_size size, CRef* refData);

	protected:
		sl_bool m_flagDecompressing;
		ZlibDecompressor m_zlib;
		HttpContentReaderOnComplete m_onComplete;

	};

}

#endif

