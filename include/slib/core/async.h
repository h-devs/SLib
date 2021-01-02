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

#ifndef CHECKHEADER_SLIB_CORE_ASYNC
#define CHECKHEADER_SLIB_CORE_ASYNC

#include "definition.h"

#include "dispatch.h"
#include "function.h"
#include "queue.h"

namespace slib
{
	
	enum class AsyncIoMode
	{
		None = 0,
		In = 1,
		Out = 2,
		InOut = 3
	};

	class AsyncIoInstance;
	class AsyncIoObject;

	class Thread;

	typedef sl_reg sl_async_handle;
	#define SLIB_ASYNC_INVALID_HANDLE ((sl_async_handle)(-1))
	
	class SLIB_EXPORT AsyncIoLoop : public Dispatcher
	{
		SLIB_DECLARE_OBJECT

	private:
		AsyncIoLoop();

		~AsyncIoLoop();

	public:
		static Ref<AsyncIoLoop> getDefault();
	
		static void releaseDefault();

		static Ref<AsyncIoLoop> create(sl_bool flagAutoStart = sl_true);
	
	public:
		void release();
	
		void start();

		sl_bool isRunning();


		sl_bool addTask(const Function<void()>& task);
	
		void wake();


		sl_bool attachInstance(AsyncIoInstance* instance, AsyncIoMode mode);

		void closeInstance(AsyncIoInstance* instance);

		void requestOrder(AsyncIoInstance* instance);

		sl_bool dispatch(const Function<void()>& callback, sl_uint64 delay_ms) override;

	protected:
		sl_bool m_flagInit;
		sl_bool m_flagRunning;
		void* m_handle;

		Ref<Thread> m_thread;

		LinkedQueue< Function<void()> > m_queueTasks;
	
		LinkedQueue< Ref<AsyncIoInstance> > m_queueInstancesOrder;
		LinkedQueue< Ref<AsyncIoInstance> > m_queueInstancesClosing;
		LinkedQueue< Ref<AsyncIoInstance> > m_queueInstancesClosed;

	protected:
		static void* _native_createHandle();
		static void _native_closeHandle(void* handle);
		void _native_runLoop();
		sl_bool _native_attachInstance(AsyncIoInstance* instance, AsyncIoMode mode);
		void _native_detachInstance(AsyncIoInstance* instance);
		void _native_wake();

	protected:
		void _stepBegin();
		void _stepEnd();
	
	};

	class SLIB_EXPORT AsyncIoInstance : public Object
	{
		SLIB_DECLARE_OBJECT

	public:
		AsyncIoInstance();

		~AsyncIoInstance();
	
	public:
		Ref<AsyncIoObject> getObject();

		void setObject(AsyncIoObject* object);
	
		Ref<AsyncIoLoop> getLoop();

		sl_async_handle getHandle();

		sl_bool isOpened();
	
		AsyncIoMode getMode();

		sl_bool isClosing();

		void setClosing();

		void addToQueue(LinkedQueue< Ref<AsyncIoInstance> >& queue);

		void requestOrder();
	
		void processOrder();

	protected:
		void setMode(AsyncIoMode mode);

		void setHandle(sl_async_handle handle);
	
	public:
		virtual void close() = 0;
	
	protected:
		virtual void onOrder() = 0;

		struct EventDesc
		{
#if defined(SLIB_PLATFORM_IS_WIN32)
			void* pOverlapped; // OVERLAPPED
#endif
#if defined(SLIB_PLATFORM_IS_UNIX)
			sl_bool flagIn;
			sl_bool flagOut;
			sl_bool flagError;
#endif
		};
		virtual void onEvent(EventDesc* pev) = 0;
	
	private:
		AtomicWeakRef<AsyncIoObject> m_object;
		sl_async_handle m_handle;
		AsyncIoMode m_mode;
	
		sl_bool m_flagClosing;

		sl_bool m_flagOrdering;
		Mutex m_lockOrdering;

		friend class AsyncIoLoop;
	};
	
	class SLIB_EXPORT AsyncIoObject : public Object
	{
		SLIB_DECLARE_OBJECT

	protected:
		AsyncIoObject();

		~AsyncIoObject();
	
	public:
		Ref<AsyncIoLoop> getIoLoop();

		Ref<AsyncIoInstance> getIoInstance();

		void closeIoInstance();
	
	protected:
		void setIoLoop(const Ref<AsyncIoLoop>& loop);

		void setIoInstance(AsyncIoInstance* instance);
	
	private:
		AtomicWeakRef<AsyncIoLoop> m_ioLoop;
		AtomicRef<AsyncIoInstance> m_ioInstance;
	
	};

}

#endif
