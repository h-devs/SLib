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

#ifndef CHECKHEADER_SLIB_CORE_ASYNC_WRITER
#define CHECKHEADER_SLIB_CORE_ASYNC_WRITER

#include "definition.h"

#include "async_stream_simulator.h"
#include "ptr.h"

namespace slib
{

	class IWriter;
	
	class SLIB_EXPORT AsyncWriter : public AsyncStreamSimulator
	{
		SLIB_DECLARE_OBJECT

	protected:
		AsyncWriter();

		~AsyncWriter();

	public:
		static Ref<AsyncWriter> create(const Ptr<IWriter>& writer);

		static Ref<AsyncWriter> create(const Ptr<IWriter>& writer, const Ref<Dispatcher>& dispatcher);


	public:
		void close() override;

		sl_bool isOpened() override;

		sl_bool read(void* data, sl_uint32 size, const Function<void(AsyncStreamResult&)>& callback, Referable* userObject = sl_null) override;

	public:
		Ptr<IWriter> getWriter();

	protected:
		void processRequest(AsyncStreamRequest* request) override;

	protected:
		AtomicPtr<IWriter> m_writer;

	};

}

#endif
