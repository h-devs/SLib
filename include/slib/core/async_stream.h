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

#ifndef CHECKHEADER_SLIB_CORE_ASYNC_STREAM
#define CHECKHEADER_SLIB_CORE_ASYNC_STREAM

#include "async.h"

namespace slib
{

	class AsyncStream;
	class AsyncStreamRequest;
	class Memory;

	enum class AsyncStreamResultCode
	{
		Success = 0,
		Ended = 1,
		Closed = 2,
		Unknown = 100
	};
	
	class SLIB_EXPORT AsyncStreamResult
	{
	public:
		AsyncStream* stream;
		AsyncStreamRequest* request;
		void* data;
		sl_size size;
		sl_size requestSize;
		Referable* userObject;
		Callable<void(AsyncStreamResult&)>* callback;
		AsyncStreamResultCode resultCode;

	public:
		sl_bool isSuccess() noexcept
		{
			return resultCode == AsyncStreamResultCode::Success;
		}

		sl_bool isEnded() noexcept
		{
			return resultCode == AsyncStreamResultCode::Ended;
		}

		sl_bool isError() noexcept
		{
			return resultCode > AsyncStreamResultCode::Ended;
		}

	};
	
	class SLIB_EXPORT AsyncStreamRequest : public Referable
	{
		SLIB_DECLARE_OBJECT

	public:
		sl_bool flagRead;
		void* data;
		sl_size size;
		Ref<Referable> userObject;
		Function<void(AsyncStreamResult&)> callback;

		sl_size sizeWritten;

	public:
		AsyncStreamRequest(sl_bool flagRead, const void* data, sl_size size, Referable* userObject, const Function<void(AsyncStreamResult&)>& callback);
		
		~AsyncStreamRequest();
	
	public:
		static Ref<AsyncStreamRequest> createRead(void* data, sl_size size, Referable* userObject, const Function<void(AsyncStreamResult&)>& callback);

		static Ref<AsyncStreamRequest> createWrite(const void* data, sl_size size, Referable* userObject, const Function<void(AsyncStreamResult&)>& callback);

	public:
		void runCallback(AsyncStream* stream, sl_size resultSize, AsyncStreamResultCode resultCode);

	};
	
	class SLIB_EXPORT AsyncStreamInstance : public AsyncIoInstance
	{
		SLIB_DECLARE_OBJECT

	public:
		AsyncStreamInstance();

		~AsyncStreamInstance();

	public:
		virtual sl_bool addRequest(const Ref<AsyncStreamRequest>& request);

	protected:
		sl_bool popReadRequest(Ref<AsyncStreamRequest>& request);

		sl_size getReadRequestCount();
	
		sl_bool popWriteRequest(Ref<AsyncStreamRequest>& request);

		sl_size getWriteRequestCount();

		void processStreamResult(AsyncStreamRequest* request, sl_size size, AsyncStreamResultCode resultCode);

		void onClose() override;

	public:
		virtual sl_bool isSeekable();

		virtual sl_bool seek(sl_uint64 pos);

		virtual sl_uint64 getPosition();

		virtual sl_uint64 getSize();

	private:
		void _freeRequests();

	private:
		LinkedQueue< Ref<AsyncStreamRequest> > m_requestsRead;
		LinkedQueue< Ref<AsyncStreamRequest> > m_requestsWrite;

	};
	
	class SLIB_EXPORT AsyncStream : public AsyncIoObject
	{
		SLIB_DECLARE_OBJECT

	public:
		AsyncStream();

		~AsyncStream();

	public:
		static Ref<AsyncStream> create(AsyncStreamInstance* instance, AsyncIoMode mode, const Ref<AsyncIoLoop>& loop);

		static Ref<AsyncStream> create(AsyncStreamInstance* instance, AsyncIoMode mode);
	
	public:
		virtual void close() = 0;

		virtual sl_bool isOpened() = 0;

		virtual sl_bool requestIo(const Ref<AsyncStreamRequest>& request) = 0;

		sl_bool read(void* data, sl_size size, const Function<void(AsyncStreamResult&)>& callback, Referable* userObject = sl_null);

		sl_bool read(const Memory& mem, const Function<void(AsyncStreamResult&)>& callback);

		sl_bool write(const void* data, sl_size size, const Function<void(AsyncStreamResult&)>& callback, Referable* userObject = sl_null);

		sl_bool write(const Memory& mem, const Function<void(AsyncStreamResult&)>& callback);

		virtual sl_bool addTask(const Function<void()>& callback) = 0;

		virtual sl_bool isSeekable();

		virtual sl_bool seek(sl_uint64 pos);

		virtual sl_uint64 getPosition();

		virtual sl_uint64 getSize();

	};
	
	class SLIB_EXPORT AsyncStreamBase : public AsyncStream
	{
		SLIB_DECLARE_OBJECT
		
	public:
		AsyncStreamBase();

		~AsyncStreamBase();

	public:
		void close() override;

		sl_bool isOpened() override;
	
		sl_bool requestIo(const Ref<AsyncStreamRequest>& req) override;

		sl_bool addTask(const Function<void()>& callback) override;

	protected:
		Ref<AsyncStreamInstance> getIoInstance();

		sl_bool _initialize(AsyncStreamInstance* instance, AsyncIoMode mode, const Ref<AsyncIoLoop>& loop);
	
		friend class AsyncStream;

	};
	
}

#endif
