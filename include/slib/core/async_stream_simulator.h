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

#ifndef CHECKHEADER_SLIB_CORE_ASYNC_STREAM_SIMULATOR
#define CHECKHEADER_SLIB_CORE_ASYNC_STREAM_SIMULATOR

#include "async_stream.h"

namespace slib
{

	class DispatchLoop;
	
	class SLIB_EXPORT AsyncStreamSimulator : public AsyncStream
	{
		SLIB_DECLARE_OBJECT

	protected:
		AsyncStreamSimulator();

		~AsyncStreamSimulator();
	
	public:
		sl_bool read(void* data, sl_uint32 size, const Function<void(AsyncStreamResult&)>& callback, Referable* userObject = sl_null) override;

		sl_bool write(const void* data, sl_uint32 size, const Function<void(AsyncStreamResult&)>& callback, Referable* userObject = sl_null) override;

		sl_bool addTask(const Function<void()>& callback) override;

	protected:
		virtual void processRequest(AsyncStreamRequest* request) = 0;
	
	protected:
		void initialize();

		void initialize(const Ref<Dispatcher>& dispatcher);

	protected:
		sl_bool _addRequest(AsyncStreamRequest* request);

		void _runProcessor();

	private:
		LinkedQueue< Ref<AsyncStreamRequest> > m_requests;
		sl_bool m_flagProcessRequest;

		Ref<DispatchLoop> m_dispatchLoop;
		WeakRef<Dispatcher> m_dispatcher;
	
	};
	
}

#endif
