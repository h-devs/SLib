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

	class Memory;
	
	struct SLIB_EXPORT AsyncStreamResult
	{
		AsyncStream* stream;
		void* data;
		sl_uint32 size;
		sl_uint32 requestSize;
		Referable* userObject;
		sl_bool flagError;

	};
	
	class SLIB_EXPORT AsyncStreamRequest : public Referable
	{
		SLIB_DECLARE_OBJECT

	public:
		void* data;
		sl_uint32 size;
		Ref<Referable> userObject;
		Function<void(AsyncStreamResult&)> callback;
		sl_bool flagRead;

	protected:
		AsyncStreamRequest(const void* data, sl_uint32 size, Referable* userObject, const Function<void(AsyncStreamResult&)>& callback, sl_bool flagRead);
		
		~AsyncStreamRequest();
	
	public:
		static Ref<AsyncStreamRequest> createRead(void* data, sl_uint32 size, Referable* userObject, const Function<void(AsyncStreamResult&)>& callback);

		static Ref<AsyncStreamRequest> createWrite(const void* data, sl_uint32 size, Referable* userObject, const Function<void(AsyncStreamResult&)>& callback);

	public:
		void runCallback(AsyncStream* stream, sl_uint32 resultSize, sl_bool flagError);

	};
	
	class SLIB_EXPORT AsyncStreamInstance : public AsyncIoInstance
	{
		SLIB_DECLARE_OBJECT

	public:
		AsyncStreamInstance();

		~AsyncStreamInstance();

	public:
		virtual sl_bool read(void* data, sl_uint32 size, const Function<void(AsyncStreamResult&)>& callback, Referable* userObject);

		virtual sl_bool write(const void* data, sl_uint32 size, const Function<void(AsyncStreamResult&)>& callback, Referable* userObject);

		virtual sl_bool isSeekable();

		virtual sl_bool seek(sl_uint64 pos);

		virtual sl_uint64 getSize();

		sl_size getWaitingSizeForWrite();

	protected:
		sl_bool addReadRequest(const Ref<AsyncStreamRequest>& request);

		sl_bool popReadRequest(Ref<AsyncStreamRequest>& request);

		sl_size getReadRequestsCount();
	
		sl_bool addWriteRequest(const Ref<AsyncStreamRequest>& request);

		sl_bool popWriteRequest(Ref<AsyncStreamRequest>& request);

		sl_size getWriteRequestsCount();
	
	private:
		LinkedQueue< Ref<AsyncStreamRequest> > m_requestsRead;
		LinkedQueue< Ref<AsyncStreamRequest> > m_requestsWrite;
		sl_reg m_sizeWriteWaiting;

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

		virtual sl_bool read(void* data, sl_uint32 size, const Function<void(AsyncStreamResult&)>& callback, Referable* userObject = sl_null) = 0;

		virtual sl_bool write(const void* data, sl_uint32 size, const Function<void(AsyncStreamResult&)>& callback, Referable* userObject = sl_null) = 0;

		virtual sl_bool isSeekable();

		virtual sl_bool seek(sl_uint64 pos);

		virtual sl_uint64 getSize();

		sl_bool readToMemory(const Memory& mem, const Function<void(AsyncStreamResult&)>& callback);
	
		sl_bool writeFromMemory(const Memory& mem, const Function<void(AsyncStreamResult&)>& callback);

		virtual sl_bool addTask(const Function<void()>& callback) = 0;

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
	
		sl_bool read(void* data, sl_uint32 size, const Function<void(AsyncStreamResult&)>& callback, Referable* userObject = sl_null) override;

		sl_bool write(const void* data, sl_uint32 size, const Function<void(AsyncStreamResult&)>& callback, Referable* userObject = sl_null) override;

		sl_bool isSeekable() override;

		sl_bool seek(sl_uint64 pos) override;

		sl_uint64 getSize() override;

		sl_bool addTask(const Function<void()>& callback) override;

		sl_size getWaitingSizeForWrite();
	
	protected:
		Ref<AsyncStreamInstance> getIoInstance();

		sl_bool _initialize(AsyncStreamInstance* instance, AsyncIoMode mode, const Ref<AsyncIoLoop>& loop);
	
		friend class AsyncStream;

	};
	
}

#endif
