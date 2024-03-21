/*
 *   Copyright (c) 2008-2024 SLIBIO <https://github.com/SLIBIO>
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

#include "slib/io/async.h"
#include "slib/io/async_stream.h"
#include "slib/io/async_stream_simulator.h"
#include "slib/io/async_stream_filter.h"
#include "slib/io/async_file.h"
#include "slib/io/async_file_stream.h"
#include "slib/io/async_copy.h"
#include "slib/io/async_output.h"

#include "slib/core/thread.h"
#include "slib/core/dispatch_loop.h"
#include "slib/core/handle_ptr.h"
#include "slib/core/mio.h"
#include "slib/core/memory_buffer.h"
#include "slib/core/timeout.h"
#include "slib/core/safe_static.h"

namespace slib
{

	SLIB_DEFINE_OBJECT(AsyncIoLoop, Dispatcher)

	AsyncIoLoop::AsyncIoLoop()
	{
		m_flagInit = sl_false;
		m_flagRunning = sl_false;
		m_handle = sl_null;
	}

	AsyncIoLoop::~AsyncIoLoop()
	{
		release();
	}

	namespace {

		static Ref<AsyncIoLoop> CreateDefaultAsyncIoLoop(sl_bool flagRelease = sl_false)
		{
			if (flagRelease) {
				return sl_null;
			}
			return AsyncIoLoop::create();
		}

		static Ref<AsyncIoLoop> GetDefaultAsyncIoLoop(sl_bool flagRelease = sl_false)
		{
			SLIB_SAFE_LOCAL_STATIC(Ref<AsyncIoLoop>, ret, CreateDefaultAsyncIoLoop(flagRelease))
			if (SLIB_SAFE_STATIC_CHECK_FREED(ret)) {
				return sl_null;
			}
			if (ret.isNotNull()) {
				if (flagRelease) {
					ret->release();
				} else {
					return ret;
				}
			}
			return sl_null;
		}

	}

	Ref<AsyncIoLoop> AsyncIoLoop::getDefault()
	{
		return GetDefaultAsyncIoLoop();
	}

	void AsyncIoLoop::releaseDefault()
	{
		GetDefaultAsyncIoLoop(sl_true);
	}

	Ref<AsyncIoLoop> AsyncIoLoop::create(sl_bool flagAutoStart)
	{
		void* handle = _native_createHandle();
		if (handle) {
			Ref<AsyncIoLoop> ret = new AsyncIoLoop;
			if (ret.isNotNull()) {
				ret->m_handle = handle;
				ret->m_thread = Thread::create(SLIB_FUNCTION_MEMBER(ret.get(), _native_runLoop));
				if (ret->m_thread.isNotNull()) {
					ret->m_flagInit = sl_true;
					if (flagAutoStart) {
						ret->start();
					}
					return ret;
				}
			}
			_native_closeHandle(handle);
		}
		return sl_null;
	}

	void AsyncIoLoop::release()
	{
		ObjectLocker lock(this);
		if (!m_flagInit) {
			return;
		}
		m_flagInit = sl_false;

		if (m_flagRunning) {
			m_flagRunning = sl_false;
			m_thread->finish();
			_native_wake();
			lock.unlock();
			m_thread->finishAndWait();
		}

		_native_closeHandle(m_handle);

		m_queueInstancesOrder.removeAll();
		m_queueInstancesClosing.removeAll();
		m_queueInstancesClosed.removeAll();

	}

	void AsyncIoLoop::start()
	{
		ObjectLocker lock(this);
		if (!m_flagInit) {
			return;
		}
		if (m_flagRunning) {
			return;
		}
		m_flagRunning = sl_true;
		if (!(m_thread->start())) {
			m_flagRunning = sl_false;
		}
	}

	sl_bool AsyncIoLoop::isRunning()
	{
		return m_flagRunning;
	}

	sl_bool AsyncIoLoop::addTask(const Function<void()>& task)
	{
		if (task.isNull()) {
			return sl_false;
		}
		if (m_queueTasks.push(task)) {
			wake();
			return sl_true;
		}
		return sl_false;
	}

	sl_bool AsyncIoLoop::dispatch(const Function<void()>& callback, sl_uint64 delayMillis)
	{
		if (delayMillis) {
			return setTimeoutByDefaultDispatchLoop(callback, delayMillis);
		}
		return addTask(callback);
	}

	void AsyncIoLoop::wake()
	{
		ObjectLocker lock(this);
		if (!m_flagRunning) {
			return;
		}
		_native_wake();
	}

	sl_bool AsyncIoLoop::attachInstance(AsyncIoInstance* instance, AsyncIoMode mode)
	{
		if (m_handle) {
			if (instance && instance->isOpened()) {
				ObjectLocker lock(this);
				return _native_attachInstance(instance, mode);
			}
		}
		return sl_false;
	}

	void AsyncIoLoop::closeInstance(AsyncIoInstance* instance)
	{
		if (m_handle) {
			if (instance && instance->isOpened()) {
				if (!(instance->isClosing())) {
					instance->setClosing();
					m_queueInstancesClosing.push(instance);
					wake();
				}
			}
		}
	}

	void AsyncIoLoop::requestOrder(AsyncIoInstance* instance)
	{
		if (m_handle) {
			if (instance && instance->isOpened()) {
				instance->addToQueue(m_queueInstancesOrder);
				wake();
			}
		}
	}

	void AsyncIoLoop::_stepBegin()
	{
		// Async Tasks
		{
			LinkedQueue< Function<void()> > tasks;
			tasks.merge(&m_queueTasks);
			Function<void()> task;
			while (tasks.pop(&task)) {
				task();
			}
		}

		// Request Orders
		{
			LinkedQueue< Ref<AsyncIoInstance> > instances;
			instances.merge(&m_queueInstancesOrder);
			Ref<AsyncIoInstance> instance;
			while (instances.pop(&instance)) {
				if (instance.isNotNull() && instance->isOpened()) {
					instance->processOrder();
				}
			}
		}
	}

	void AsyncIoLoop::_stepEnd()
	{
		Ref<AsyncIoInstance> instance;
		while (m_queueInstancesClosing.pop(&instance)) {
			if (instance.isNotNull() && instance->isOpened()) {
				_native_detachInstance(instance.get());
				instance->onClose();
				m_queueInstancesClosed.push(instance);
			}
		}
	}


	SLIB_DEFINE_OBJECT(AsyncIoInstance, Object)

	AsyncIoInstance::AsyncIoInstance()
	{
		m_handle = 0;
		m_flagClosing = sl_false;
		m_flagOrdering = sl_false;
		m_mode = AsyncIoMode::InOut;
	}

	AsyncIoInstance::~AsyncIoInstance()
	{
	}

	Ref<AsyncIoObject> AsyncIoInstance::getObject()
	{
		return m_object;
	}

	void AsyncIoInstance::setObject(AsyncIoObject* object)
	{
		m_object = object;
	}

	Ref<AsyncIoLoop> AsyncIoInstance::getLoop()
	{
		Ref<AsyncIoObject> object(m_object);
		if (object.isNotNull()) {
			return object->getIoLoop();
		}
		return sl_null;
	}

	sl_async_handle AsyncIoInstance::getHandle()
	{
		return m_handle;
	}

	void AsyncIoInstance::setHandle(sl_async_handle handle)
	{
		m_handle = handle;
	}

	sl_bool AsyncIoInstance::isOpened()
	{
		return m_handle != SLIB_ASYNC_INVALID_HANDLE;
	}

	AsyncIoMode AsyncIoInstance::getMode()
	{
		return m_mode;
	}

	void AsyncIoInstance::setMode(AsyncIoMode mode)
	{
		m_mode = mode;
	}

	sl_bool AsyncIoInstance::isClosing()
	{
		return m_flagClosing;
	}

	void AsyncIoInstance::setClosing()
	{
		m_flagClosing = sl_true;
	}

	void AsyncIoInstance::addToQueue(LinkedQueue< Ref<AsyncIoInstance> >& queue)
	{
		MutexLocker lock(&m_lockOrdering);
		if (!m_flagOrdering) {
			m_flagOrdering = sl_true;
			queue.push(this);
		}
	}

	void AsyncIoInstance::requestOrder()
	{
		Ref<AsyncIoLoop> loop(getLoop());
		if (loop.isNotNull()) {
			loop->requestOrder(this);
		}
	}

	void AsyncIoInstance::processOrder()
	{
		MutexLocker lock(&m_lockOrdering);
		m_flagOrdering = sl_false;
		lock.unlock();
		onOrder();
	}


	SLIB_DEFINE_OBJECT(AsyncIoObject, Object)

	AsyncIoObject::AsyncIoObject()
	{
	}

	AsyncIoObject::~AsyncIoObject()
	{
		closeInstance();
	}

	Ref<AsyncIoLoop> AsyncIoObject::getIoLoop()
	{
		return m_ioLoop;
	}

	sl_bool AsyncIoObject::isOpened()
	{
		return m_ioInstance.isNotNull();
	}

	void AsyncIoObject::close()
	{
		closeInstance();
	}

	sl_bool AsyncIoObject::addTask(const Function<void()>& callback)
	{
		Ref<AsyncIoLoop> loop = getIoLoop();
		if (loop.isNotNull()) {
			return loop->addTask(callback);
		}
		return sl_false;
	}

	sl_bool AsyncIoObject::dispatch(const Function<void()>& callback, sl_uint64 delayMillis)
	{
		if (delayMillis) {
			return setTimeoutByDefaultDispatchLoop(callback, delayMillis);
		}
		return addTask(callback);
	}

	sl_bool AsyncIoObject::initialize(const Ref<AsyncIoLoop>& loop, AsyncIoInstance* instance, AsyncIoMode mode)
	{
		if (m_ioLoop.isNotNull()) {
			return sl_false;
		}
		if (!instance) {
			return sl_false;
		}
		if (loop.isNull()) {
			Ref<AsyncIoLoop> defaultLoop = AsyncIoLoop::getDefault();
			if (defaultLoop.isNotNull()) {
				return initialize(defaultLoop, instance, mode);
			} else {
				return sl_false;
			}
		}
		instance->setObject(this);
		m_ioInstance = instance;
		m_ioLoop = loop;
		return loop->attachInstance(instance, mode);
	}

	void AsyncIoObject::closeInstance()
	{
		ObjectLocker lock(this);
		Ref<AsyncIoInstance> instance(m_ioInstance);
		if (instance.isNotNull()) {
			Ref<AsyncIoLoop> loop = getIoLoop();
			if (loop.isNotNull()) {
				loop->closeInstance(instance.get());
			}
			m_ioInstance.setNull();
		}
	}


	AsyncStreamErrorResult::AsyncStreamErrorResult(AsyncStream* _stream, const void* _data, sl_size _size, const Function<void(AsyncStreamResult&)>& _callback, CRef* _userObject, AsyncStreamResultCode code) noexcept
	{
		stream = _stream;
		request = sl_null;
		size = 0;
		resultCode = code;
		data = (void*)_data;
		requestSize = _size;
		userObject = _userObject;
		callback = _callback.ref.get();
	}


	SLIB_DEFINE_ROOT_OBJECT(AsyncStreamRequest)

	AsyncStreamRequest::AsyncStreamRequest(sl_bool _flagRead, const void* _data, sl_size _size, CRef* _userObject, const Function<void(AsyncStreamResult&)>& _callback): flagRead(_flagRead), data((void*)_data), size(_size), userObject(_userObject), callback(_callback), m_sizePassed(0), m_flagFinished(sl_false), m_flagFully(sl_false)
	{
	}

	AsyncStreamRequest::~AsyncStreamRequest()
	{
	}

	Ref<AsyncStreamRequest> AsyncStreamRequest::createRead(void* data, sl_size size, CRef* userObject, const Function<void(AsyncStreamResult&)>& callback)
	{
		if (!size) {
			return sl_null;
		}
		return new AsyncStreamRequest(sl_true, data, size, userObject, callback);
	}

	Ref<AsyncStreamRequest> AsyncStreamRequest::createWrite(const void* data, sl_size size, CRef* userObject, const Function<void(AsyncStreamResult&)>& callback)
	{
		if (!size) {
			return sl_null;
		}
		return new AsyncStreamRequest(sl_false, data, size, userObject, callback);
	}

	void AsyncStreamRequest::runCallback(AsyncStream* stream, sl_size resultSize, AsyncStreamResultCode code)
	{
		if (m_flagFinished) {
			return;
		}
		if (stream) {
			stream->setLastResultCode(code);
		}
		if (callback.isNotNull()) {
			if (stream && m_flagFully) {
				if (code == AsyncStreamResultCode::Success && resultSize && resultSize < size) {
					data = (char*)data + resultSize;
					size -= resultSize;
					m_sizePassed += resultSize;
					if (stream->requestIo(this)) {
						return;
					}
				}
				if (m_sizePassed) {
					data = (char*)data - m_sizePassed;
					size += m_sizePassed;
					resultSize += m_sizePassed;
					m_sizePassed = 0;
				}
			}
			AsyncStreamResult result;
			result.stream = stream;
			result.request = this;
			result.data = data;
			result.size = resultSize;
			result.requestSize = size;
			result.userObject = userObject.get();
			result.callback = callback.getObject();
			result.resultCode = code;
			callback(result);
		}
		m_flagFinished = sl_true;
	}

	void AsyncStreamRequest::resetPassedSize()
	{
		m_sizePassed = 0;
	}


	SLIB_DEFINE_OBJECT(AsyncStreamInstance, AsyncIoInstance)

	AsyncStreamInstance::AsyncStreamInstance()
	{
	}

	AsyncStreamInstance::~AsyncStreamInstance()
	{
		_freeRequests();
	}

	sl_bool AsyncStreamInstance::request(AsyncStreamRequest* req)
	{
		if (req->flagRead) {
			if (m_requestRead.isNull()) {
				m_requestRead = req;
				return sl_true;
			}
		} else {
			if (m_requestWrite.isNull()) {
				m_requestWrite = req;
				return sl_true;
			}
		}
		return sl_false;
	}

	Ref<AsyncStreamRequest> AsyncStreamInstance::getReadRequest()
	{
		return m_requestRead.release();
	}

	Ref<AsyncStreamRequest> AsyncStreamInstance::getWriteRequest()
	{
		return m_requestWrite.release();
	}

	void AsyncStreamInstance::processStreamResult(AsyncStreamRequest* request, sl_size size, AsyncStreamResultCode code)
	{
		Ref<AsyncIoObject> object = getObject();
		if (object.isNotNull()) {
			request->runCallback(static_cast<AsyncStream*>(object.get()), size, code);
		} else {
			request->runCallback(sl_null, 0, AsyncStreamResultCode::Closed);
		}
	}

	void AsyncStreamInstance::onClose()
	{
		_freeRequests();
	}

	void AsyncStreamInstance::_freeRequests()
	{
		Ref<AsyncIoObject> object = getObject();
		AsyncStream* stream = static_cast<AsyncStream*>(object.get());
		{
			Ref<AsyncStreamRequest> req(m_requestRead.release());
			if (req.isNotNull()) {
				req->runCallback(stream, 0, AsyncStreamResultCode::Closed);
			}
		}
		{
			Ref<AsyncStreamRequest> req(m_requestWrite.release());
			if (req.isNotNull()) {
				req->runCallback(stream, 0, AsyncStreamResultCode::Closed);
			}
		}
	}

	sl_bool AsyncStreamInstance::isSeekable()
	{
		return sl_false;
	}

	sl_bool AsyncStreamInstance::seek(sl_uint64 pos)
	{
		return sl_false;
	}

	sl_uint64 AsyncStreamInstance::getPosition()
	{
		return 0;
	}

	sl_uint64 AsyncStreamInstance::getSize()
	{
		return 0;
	}


	SLIB_DEFINE_OBJECT(AsyncStream, Object)

	AsyncStream::AsyncStream()
	{
		m_lastResultCode = AsyncStreamResultCode::Success;
	}

	AsyncStream::~AsyncStream()
	{
	}

	Ref<AsyncStream> AsyncStream::create(AsyncStreamInstance* instance, AsyncIoMode mode, const Ref<AsyncIoLoop>& loop)
	{
		if (instance) {
			Ref<AsyncStreamBase> ret = new AsyncStreamBase;
			if (ret.isNotNull()) {
				if (ret->initialize(loop, instance, mode)) {
					return ret;
				}
			}
		}
		return sl_null;
	}

	Ref<AsyncStream> AsyncStream::create(AsyncStreamInstance* instance, AsyncIoMode mode)
	{
		return create(instance, mode, sl_null);
	}

	sl_bool AsyncStream::requestIo(AsyncStreamRequest* request, sl_int32 timeout)
	{
		if (timeout >= 0) {
			WeakRef<AsyncStream> thiz = this;
			WeakRef<AsyncStreamRequest> weakRequest = request;
			if (!(dispatch([thiz, weakRequest]() {
				Ref<AsyncStreamRequest> request = weakRequest;
				if (request.isNull()) {
					return;
				}
				Ref<AsyncStream> stream = thiz;
				request->runCallback(stream.get(), 0, AsyncStreamResultCode::Timeout);
			}, timeout))) {
				return sl_false;
			}
		}
		return requestIo(request);
	}

	void AsyncStream::read(void* data, sl_size size, const Function<void(AsyncStreamResult&)>& callback, CRef* userObject, sl_int32 timeout)
	{
		Ref<AsyncStreamRequest> req = AsyncStreamRequest::createRead(data, size, userObject, callback);
		if (req.isNotNull()) {
			if (requestIo(req, timeout)) {
				return;
			}
		}
		AsyncStreamErrorResult error(this, data, size, callback, userObject);
		callback(error);
	}

	void AsyncStream::read(const Memory& mem, const Function<void(AsyncStreamResult&)>& callback, sl_int32 timeout)
	{
		read(mem.getData(), mem.getSize(), callback, mem.ref.get(), timeout);
	}

	void AsyncStream::readFully(void* data, sl_size size, const Function<void(AsyncStreamResult&)>& callback, CRef* userObject, sl_int32 timeout)
	{
		Ref<AsyncStreamRequest> req = AsyncStreamRequest::createRead(data, size, userObject, callback);
		if (req.isNotNull()) {
			req->m_flagFully = sl_true;
			if (requestIo(req, timeout)) {
				return;
			}
		}
		AsyncStreamErrorResult error(this, data, size, callback, userObject);
		callback(error);
	}

	void AsyncStream::readFully(const Memory& mem, const Function<void(AsyncStreamResult&)>& callback, sl_int32 timeout)
	{
		readFully(mem.getData(), mem.getSize(), callback, mem.ref.get(), timeout);
	}

	void AsyncStream::readFully(sl_size size, const Function<void(AsyncStream*, Memory&, sl_bool flagError)>& callback, sl_int32 timeout)
	{
		Memory mem = Memory::create(size);
		if (mem.isNull()) {
			callback(this, mem, sl_true);
			return;
		}
		readFully(mem, [callback](AsyncStreamResult& result) {
			if (result.isError()) {
				Memory null;
				callback(result.stream, null, sl_true);
				return;
			}
			Memory mem = (CMemory*)(result.userObject);
			if (result.size != result.requestSize) {
				mem = mem.sub(0, result.size);
				if (mem.isNull()) {
					callback(result.stream, mem, sl_true);
					return;
				}
			}
			callback(result.stream, mem, sl_false);
		}, timeout);
	}

	namespace
	{
		class ReadStreamResult
		{
		public:
			AsyncStream* stream;
			Memory ret;
			sl_bool flagError;
			Callable<void(ReadStreamResult&)>* callback;

		public:
			ReadStreamResult(AsyncStream* _stream, Memory&& _ret, const Function<void(ReadStreamResult&)>& _callback): stream(_stream), ret(Move(_ret)), callback(_callback.ref.ptr)
			{
				flagError = sl_false;
			}

			ReadStreamResult(AsyncStream* _stream, const Function<void(ReadStreamResult&)>& _callback): stream(_stream), callback(_callback.ref.ptr)
			{
				flagError = sl_true;
			}

		};

		static void ReadStreamFully(AsyncStream* stream, sl_size size, const Function<void(ReadStreamResult&)>& callback, sl_int32 timeout)
		{
			Memory mem = Memory::create(size);
			if (mem.isNull()) {
				ReadStreamResult err(stream, callback);
				callback(err);
				return;
			}
			stream->readFully(mem, [callback](AsyncStreamResult& result) {
				if (result.isError()) {
					ReadStreamResult err(result.stream, callback);
					callback(err);
					return;
				}
				Memory mem = (CMemory*)(result.userObject);
				if (result.size != result.requestSize) {
					mem = mem.sub(0, result.size);
					if (mem.isNull()) {
						ReadStreamResult err(result.stream, callback);
						callback(err);
						return;
					}
				}
				ReadStreamResult success(result.stream, Move(mem), callback);
				callback(success);
			}, timeout);
		}
	}

	void AsyncStream::readFully(sl_size size, sl_size segmentSize, const Function<void(AsyncStream*, MemoryBuffer&, sl_bool flagError)>& callback, sl_int32 timeout)
	{
		if (!segmentSize) {
			MemoryBuffer null;
			callback(this, null, sl_true);
			return;
		}
		RefT<MemoryBuffer> buf = new CRefT<MemoryBuffer>();
		if (buf.isNull()) {
			MemoryBuffer null;
			callback(this, null, sl_true);
			return;
		}
		if (segmentSize > size) {
			segmentSize = size;
		}
		sl_int64 tickEnd = GetTickFromTimeout(timeout);
		ReadStreamFully(this, segmentSize, [callback, size, segmentSize, buf, tickEnd](ReadStreamResult& result) {
			if (!(result.flagError)) {
				if (buf->add(Move(result.ret))) {
					sl_size bufSize = buf->getSize();
					if (bufSize >= size) {
						callback(result.stream, *buf, sl_false);
					} else {
						ReadStreamFully(result.stream, SLIB_MIN(size - bufSize, segmentSize), result.callback, GetTimeoutFromTick(tickEnd));
					}
					return;
				}
			}
			MemoryBuffer null;
			callback(result.stream, null, sl_true);
		}, timeout);
	}

	void AsyncStream::write(const void* data, sl_size size, const Function<void(AsyncStreamResult&)>& callback, CRef* userObject, sl_int32 timeout)
	{
		Ref<AsyncStreamRequest> req = AsyncStreamRequest::createWrite(data, size, userObject, callback);
		if (req.isNotNull()) {
			req->m_flagFully = sl_true;
			if (requestIo(req, timeout)) {
				return;
			}
		}
		AsyncStreamErrorResult error(this, data, size, callback, userObject);
		callback(error);
	}

	void AsyncStream::write(const Memory& mem, const Function<void(AsyncStreamResult&)>& callback, sl_int32 timeout)
	{
		write(mem.getData(), mem.getSize(), callback, mem.ref.get(), timeout);
	}

	void AsyncStream::createMemoryAndWrite(const void* data, sl_size size, const Function<void(AsyncStreamResult&)>& callback, sl_int32 timeout)
	{
		Memory mem = Memory::create(data, size);
		if (mem.isNull()) {
			AsyncStreamErrorResult result(this, data, size, callback, sl_null);
			callback(result);
			return;
		}
		write(mem.getData(), mem.getSize(), callback, mem.ref.get(), timeout);
	}

	sl_bool AsyncStream::isSeekable()
	{
		Ref<AsyncStreamInstance> instance = Ref<AsyncStreamInstance>::from(getIoInstance());
		if (instance.isNotNull()) {
			return instance->isSeekable();
		}
		return sl_false;
	}

	sl_bool AsyncStream::seek(sl_uint64 pos)
	{
		Ref<AsyncStreamInstance> instance = Ref<AsyncStreamInstance>::from(getIoInstance());
		if (instance.isNotNull()) {
			instance->seek(pos);
			return sl_true;
		}
		return sl_false;
	}

	sl_uint64 AsyncStream::getPosition()
	{
		Ref<AsyncStreamInstance> instance = Ref<AsyncStreamInstance>::from(getIoInstance());
		if (instance.isNotNull()) {
			return instance->getPosition();
		}
		return 0;
	}

	sl_uint64 AsyncStream::getSize()
	{
		Ref<AsyncStreamInstance> instance = Ref<AsyncStreamInstance>::from(getIoInstance());
		if (instance.isNotNull()) {
			return instance->getSize();
		}
		return 0;
	}

	AsyncStreamResultCode AsyncStream::getLastResultCode()
	{
		return m_lastResultCode;
	}

	void AsyncStream::setLastResultCode(AsyncStreamResultCode code)
	{
		m_lastResultCode = code;
	}


	SLIB_DEFINE_OBJECT(AsyncStreamBase, AsyncStream)

	AsyncStreamBase::AsyncStreamBase()
	{
	}

	AsyncStreamBase::~AsyncStreamBase()
	{
	}

	Ref<AsyncStreamInstance> AsyncStreamBase::getIoInstance()
	{
		return Ref<AsyncStreamInstance>::from(AsyncIoObject::getIoInstance());
	}

	sl_bool AsyncStreamBase::requestIo(AsyncStreamRequest* req)
	{
		Ref<AsyncIoLoop> loop = getIoLoop();
		if (loop.isNull()) {
			return sl_false;
		}
		Ref<AsyncStreamInstance> instance = getIoInstance();
		if (instance.isNotNull()) {
			if (instance->request(req)) {
				loop->requestOrder(instance.get());
				return sl_true;
			}
		}
		return sl_false;
	}


	SLIB_DEFINE_OBJECT(AsyncStreamSimulator, AsyncStream)

	AsyncStreamSimulator::AsyncStreamSimulator()
	{
		m_flagProcessRequest = sl_false;
	}

	AsyncStreamSimulator::~AsyncStreamSimulator()
	{
	}

	sl_bool AsyncStreamSimulator::requestIo(AsyncStreamRequest* req)
	{
		if (isOpened()) {
			return _addRequest(req);
		}
		return sl_false;
	}

	sl_bool AsyncStreamSimulator::addTask(const Function<void()>& callback)
	{
		Ref<Dispatcher> dispatcher(m_dispatcher);
		if (dispatcher.isNotNull()) {
			return dispatcher->dispatch(callback);
		}
		return sl_false;
	}

	void AsyncStreamSimulator::initialize()
	{
		m_dispatchLoop = DispatchLoop::create();
		m_dispatcher = m_dispatchLoop;
	}

	void AsyncStreamSimulator::initialize(const Ref<Dispatcher>& dispatcher)
	{
		if (dispatcher.isNotNull()) {
			m_dispatcher = dispatcher;
		} else {
			initialize();
		}
	}

	sl_bool AsyncStreamSimulator::_addRequest(AsyncStreamRequest* req)
	{
		Ref<Dispatcher> dispatcher(m_dispatcher);
		if (dispatcher.isNotNull()) {
			ObjectLocker lock(this);
			m_requests.push_NoLock(req);
			if (!m_flagProcessRequest) {
				m_flagProcessRequest = sl_true;
				lock.unlock();
				dispatcher->dispatch(SLIB_FUNCTION_WEAKREF(this, _runProcessor));
			}
			return sl_true;
		}
		return sl_false;
	}

	void AsyncStreamSimulator::_runProcessor()
	{
		if (isOpened()) {
			Thread* thread = Thread::getCurrent();
			while (!thread || thread->isNotStopping()) {
				Ref<AsyncStreamRequest> req;
				{
					ObjectLocker lock(this);
					if (!(m_requests.pop_NoLock(&req))) {
						m_flagProcessRequest = sl_false;
						break;
					}
				}
				if (req.isNotNull()) {
					processRequest(req.get());
				}
			}
		}
	}


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(AsyncFileStreamParam)

	AsyncFileStreamParam::AsyncFileStreamParam()
	{
		handle = SLIB_FILE_INVALID_HANDLE;
		flagCloseOnRelease = sl_true;
		mode = AsyncIoMode::InOut;
		initialPosition = 0;
		flagSupportSeeking = sl_false;
	}


	SLIB_DEFINE_OBJECT(AsyncFileStreamInstance, AsyncStreamInstance)

	AsyncFileStreamInstance::AsyncFileStreamInstance()
	{
		m_flagCloseOnRelease = sl_false;
	}

	AsyncFileStreamInstance::~AsyncFileStreamInstance()
	{
		_free();
	}

	void AsyncFileStreamInstance::onClose()
	{
		_free();
		AsyncStreamInstance::onClose();
	}

	void AsyncFileStreamInstance::_free()
	{
		if (m_requestReading.isNotNull()) {
			processStreamResult(m_requestReading.get(), 0, AsyncStreamResultCode::Closed);
			m_requestReading.setNull();
		}
		if (m_requestWriting.isNotNull()) {
			processStreamResult(m_requestWriting.get(), 0, AsyncStreamResultCode::Closed);
			m_requestWriting.setNull();
		}
		if (m_flagCloseOnRelease) {
			sl_file handle = (sl_file)(getHandle());
			if (handle != SLIB_ASYNC_INVALID_HANDLE) {
				File::close(handle);
			}
		}
	}


	SLIB_DEFINE_OBJECT(AsyncFileStream, AsyncStreamBase)

	AsyncFileStream::AsyncFileStream()
	{
	}

	AsyncFileStream::~AsyncFileStream()
	{
		close();
	}

	Ref<AsyncFileStream> AsyncFileStream::create(AsyncFileStreamInstance* instance, AsyncIoMode mode, const Ref<AsyncIoLoop>& loop)
	{
		if (instance) {
			Ref<AsyncFileStream> ret = new AsyncFileStream;
			if (ret.isNotNull()) {
				if (ret->initialize(loop, instance, mode)) {
					return ret;
				}
			}
		}
		return sl_null;
	}

	Ref<AsyncFileStream> AsyncFileStream::create(AsyncFileStreamInstance* instance, AsyncIoMode mode)
	{
		return create(instance, mode, sl_null);
	}

	Ref<AsyncFileStreamInstance> AsyncFileStream::getIoInstance()
	{
		return Ref<AsyncFileStreamInstance>::from(AsyncIoObject::getIoInstance());
	}

	sl_file AsyncFileStream::getHandle()
	{
		return SLIB_FILE_INVALID_HANDLE;
	}


	SLIB_DEFINE_OBJECT(AsyncFile, AsyncStreamSimulator)

	AsyncFile::AsyncFile()
	{
	}

	AsyncFile::~AsyncFile()
	{
		close();
	}

	Ref<AsyncFile> AsyncFile::create(File&& file)
	{
		if (file.isOpened()) {
			Ref<AsyncFile> ret = new AsyncFile;
			if (ret.isNotNull()) {
				ret->m_file = Move(file);
				ret->initialize();
				return ret;
			}
		}
		return sl_null;
	}

	Ref<AsyncFile> AsyncFile::create(File&& file, const Ref<Dispatcher>& dispatcher)
	{
		if (file.isOpened()) {
			Ref<AsyncFile> ret = new AsyncFile;
			if (ret.isNotNull()) {
				ret->m_file = Move(file);
				ret->initialize(dispatcher);
				return ret;
			}
		}
		return sl_null;
	}

	Ref<AsyncFile> AsyncFile::open(const StringParam& path, FileMode mode)
	{
		File file = File::open(path, mode);
		if (file.isOpened()) {
			return AsyncFile::create(Move(file));
		}
		return sl_null;
	}

	Ref<AsyncFile> AsyncFile::open(const StringParam& path, FileMode mode, const Ref<Dispatcher>& dispatcher)
	{
		File file = File::open(path, mode);
		if (file.isOpened()) {
			return AsyncFile::create(Move(file), dispatcher);
		}
		return sl_null;
	}

	Ref<AsyncFile> AsyncFile::openForRead(const StringParam& path)
	{
		return AsyncFile::open(path, FileMode::Read);
	}

	Ref<AsyncFile> AsyncFile::openForRead(const StringParam& path, const Ref<Dispatcher>& dispatcher)
	{
		return AsyncFile::open(path, FileMode::Read, dispatcher);
	}

	Ref<AsyncFile> AsyncFile::openForWrite(const StringParam& path)
	{
		return AsyncFile::open(path, FileMode::Write);
	}

	Ref<AsyncFile> AsyncFile::openForWrite(const StringParam& path, const Ref<Dispatcher>& dispatcher)
	{
		return AsyncFile::open(path, FileMode::Write, dispatcher);
	}

	Ref<AsyncFile> AsyncFile::openForAppend(const StringParam& path)
	{
		return AsyncFile::open(path, FileMode::Append);
	}

	Ref<AsyncFile> AsyncFile::openForAppend(const StringParam& path, const Ref<Dispatcher>& dispatcher)
	{
		return AsyncFile::open(path, FileMode::Append, dispatcher);
	}

	Ref<AsyncStream> AsyncFile::openStream(const StringParam& path, FileMode mode)
	{
		return openStream(path, mode, sl_null, sl_null);
	}

	Ref<AsyncStream> AsyncFile::openStream(const StringParam& path, FileMode mode, const Ref<AsyncIoLoop>& ioLoop, const Ref<Dispatcher>& dispatcher)
	{
#ifdef SLIB_PLATFORM_IS_WIN32
		AsyncFileStreamParam param;
		if (param.openFile(path, mode)) {
			param.ioLoop = ioLoop;
			return AsyncFileStream::create(param);
		}
		return sl_null;
#else
		return open(path, mode, dispatcher);
#endif
	}

	File& AsyncFile::getFile()
	{
		return m_file;
	}

	void AsyncFile::close()
	{
		m_file.close();
	}

	sl_bool AsyncFile::isOpened()
	{
		return m_file.isOpened();
	}

	void AsyncFile::processRequest(AsyncStreamRequest* request)
	{
		File& file = m_file;
		if (file.isOpened()) {
			if (request->data && request->size) {
				sl_reg size;
				if (request->flagRead) {
					size = file.read(request->data, request->size);
				} else {
					size = file.write(request->data, request->size);
				}
				if (size > 0) {
					request->runCallback(this, size, AsyncStreamResultCode::Success);
				} else if (size == SLIB_IO_ENDED) {
					request->runCallback(this, size, AsyncStreamResultCode::Ended);
				} else {
					request->runCallback(this, size, AsyncStreamResultCode::Unknown);
				}
			} else {
				request->runCallback(this, 0, AsyncStreamResultCode::Success);
			}
		}
	}

	sl_bool AsyncFile::isSeekable()
	{
		return sl_true;
	}

	sl_bool AsyncFile::seek(sl_uint64 pos)
	{
		return m_file.seek(pos, SeekPosition::Begin);
	}

	sl_uint64 AsyncFile::getPosition()
	{
		return m_file.getPosition();
	}

	sl_uint64 AsyncFile::getSize()
	{
		return m_file.getSize();
	}


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(AsyncCopyParam)

	AsyncCopyParam::AsyncCopyParam()
	{
		size = SLIB_UINT64_MAX;
		bufferSize = 0x10000;
		bufferCount = 8;
		flagAutoStart = sl_true;
	}

	SLIB_DEFINE_OBJECT(AsyncCopy, Object)

	AsyncCopy::AsyncCopy()
	{
		m_sizeRead = 0;
		m_sizeWritten = 0;
		m_flagReadError = sl_false;
		m_flagReadEnded = sl_false;
		m_flagWriteError = sl_false;
		m_flagRunning = sl_true;
		m_flagStarted = sl_false;
		m_flagEnqueue = sl_false;

		m_sizeTotal = 0;
	}

	AsyncCopy::~AsyncCopy()
	{
		close();
	}

	Ref<AsyncCopy> AsyncCopy::create(const AsyncCopyParam& param)
	{
		if (param.target.isNull()) {
			return sl_null;
		}
		if (param.source.isNull()) {
			return sl_null;
		}
		if (!(param.size)) {
			return sl_null;
		}
		if (!(param.bufferSize) || !(param.bufferCount)) {
			return sl_null;
		}
		Ref<AsyncCopy> ret = new AsyncCopy();
		if (ret.isNotNull()) {
			ret->m_source = param.source;
			ret->m_target = param.target;
			ret->m_onRead = param.onRead;
			ret->m_onWrite = param.onWrite;
			ret->m_onEnd = param.onEnd;
			ret->m_sizeTotal = param.size;
			for (sl_uint32 i = 0; i < param.bufferCount; i++) {
				Memory mem = Memory::create(param.bufferSize);
				if (mem.isNotNull()) {
					Ref<Buffer> buf = new Buffer;
					if (buf.isNotNull()) {
						buf->mem = mem;
						ret->m_buffersRead.push(buf);
					}
				} else {
					return sl_null;
				}
			}
			if (param.flagAutoStart) {
				if (ret->start()) {
					return ret;
				}
			} else {
				return ret;
			}
		}
		return sl_null;
	}

	sl_bool AsyncCopy::start()
	{
		ObjectLocker lock(this);
		if (!m_flagStarted) {
			m_flagStarted = sl_true;
			enqueue();
			return sl_true;
		}
		return sl_false;
	}

	void AsyncCopy::close()
	{
		ObjectLocker lock(this);
		if (m_flagRunning) {
			m_flagRunning = sl_false;
			dispatchEnd();
			m_source.setNull();
			m_target.setNull();
			m_bufferReading.setNull();
			m_buffersRead.removeAll();
			m_bufferWriting.setNull();
			m_buffersWrite.removeAll();
		}
	}

	sl_bool AsyncCopy::isRunning()
	{
		return m_flagRunning;
	}

	Ref<AsyncStream> AsyncCopy::getSource()
	{
		return m_source;
	}

	Ref<AsyncStream> AsyncCopy::getTarget()
	{
		return m_target;
	}

	sl_uint64 AsyncCopy::getTotalSize()
	{
		return m_sizeTotal;
	}

	sl_uint64 AsyncCopy::getReadSize()
	{
		return m_sizeRead;
	}

	sl_uint64 AsyncCopy::getWrittenSize()
	{
		return m_sizeWritten;
	}

	sl_bool AsyncCopy::isCompleted()
	{
		return m_sizeWritten == m_sizeTotal;
	}

	sl_bool AsyncCopy::isErrorOccured()
	{
		return m_flagReadError || m_flagWriteError;
	}

	sl_bool AsyncCopy::isReadingErrorOccured()
	{
		return m_flagReadError;
	}

	sl_bool AsyncCopy::isEndedReading()
	{
		return m_flagReadEnded;
	}

	sl_bool AsyncCopy::isWritingErrorOccured()
	{
		return m_flagWriteError;
	}

	sl_bool AsyncCopy::isReading()
	{
		return m_bufferReading.isNotNull();
	}

	sl_bool AsyncCopy::isWriting()
	{
		return m_bufferWriting.isNotNull();
	}

	void AsyncCopy::onReadStream(AsyncStreamResult& result)
	{
		ObjectLocker lock(this);
		if (!m_flagRunning) {
			return;
		}

		Ref<Buffer> bufferReading = Move(m_bufferReading);

		if (bufferReading.isNotNull()) {
			do {
				if (result.size) {
					m_sizeRead += result.size;
					Memory memWrite = bufferReading->mem.sub(0, result.size);
					if (memWrite.isNull()) {
						m_flagReadError = sl_true;
					} else {
						memWrite = dispatchRead(memWrite);
						if (memWrite.isNotNull()) {
							bufferReading->memWrite = memWrite;
							m_buffersWrite.pushBack(bufferReading);
							break;
						}
					}
				}
				bufferReading->memWrite.setNull();
				m_buffersRead.pushBack(bufferReading);
			} while (0);
		}

		if (result.isError()) {
			m_flagReadError = sl_true;
		} else if (result.isEnded()) {
			m_flagReadEnded = sl_true;
			if (m_sizeTotal == SLIB_UINT64_MAX) {
				m_sizeTotal = m_sizeRead;
			}
		}

		enqueue();

	}

	void AsyncCopy::onWriteStream(AsyncStreamResult& result)
	{
		ObjectLocker lock(this);
		if (!m_flagRunning) {
			return;
		}
		if (result.isError()) {
			m_flagWriteError = sl_true;
		}

		Ref<Buffer> bufferWriting = Move(m_bufferWriting);

		if (bufferWriting.isNotNull()) {
			m_sizeWritten += result.size;
			bufferWriting->memWrite.setNull();
			m_buffersRead.pushBack(bufferWriting);
			dispatchWrite();
		}

		enqueue();
	}

	void AsyncCopy::enqueue()
	{
		if (!m_flagRunning) {
			return;
		}

		if (m_flagEnqueue) {
			return;
		}
		m_flagEnqueue = sl_true;

		// read
		do {
			if (m_flagReadError || m_flagReadEnded) {
				break;
			}
			if (m_sizeRead >= m_sizeTotal) {
				break;
			}
			if (m_bufferReading.isNotNull()) {
				break;
			}
			Ref<Buffer> buffer;
			if (m_buffersRead.popFront(&buffer)) {
				sl_size size = buffer->mem.getSize();
				sl_uint64 remain = m_sizeTotal - m_sizeRead;
				if (size > remain) {
					size = (sl_size)remain;
				}
				buffer->memRead = buffer->mem.sub(0, size);
				m_bufferReading = buffer;
				if (buffer->memRead.isNotNull()) {
					Ref<AsyncStream> source = m_source;
					if (source.isNotNull()) {
						source->read(buffer->memRead, SLIB_FUNCTION_WEAKREF(this, onReadStream));
					}
				}
			}
		} while (0);

		// write
		do {
			if (m_flagWriteError) {
				break;
			}
			if (m_bufferWriting.isNotNull()) {
				break;
			}
			Ref<Buffer> buffer;
			if (m_buffersWrite.popFront(&buffer)) {
				m_bufferWriting = buffer;
				Ref<AsyncStream> target = m_target;
				if (target.isNotNull()) {
					target->write(buffer->memWrite, SLIB_FUNCTION_WEAKREF(this, onWriteStream));
				}
			}
		} while (0);

		if (m_bufferReading.isNull() && m_bufferWriting.isNull()) {
			close();
		}

		m_flagEnqueue = sl_false;

	}

	Memory AsyncCopy::dispatchRead(const Memory& input)
	{
		if (m_onRead.isNotNull()) {
			return m_onRead(this, input);
		} else {
			return input;
		}
	}

	void AsyncCopy::dispatchWrite()
	{
		m_onWrite(this);
	}

	void AsyncCopy::dispatchEnd()
	{
		m_onEnd(this, isErrorOccured());
	}


	AsyncOutputBufferElement::AsyncOutputBufferElement()
	{
		m_sizeBody = 0;
	}

	AsyncOutputBufferElement::AsyncOutputBufferElement(const Memory& header)
	{
		m_header.add(header);
		m_sizeBody = 0;
	}

	AsyncOutputBufferElement::AsyncOutputBufferElement(AsyncStream* stream, sl_uint64 size)
	{
		m_body = stream;
		m_sizeBody = size;
	}

	AsyncOutputBufferElement::~AsyncOutputBufferElement()
	{
	}

	sl_bool AsyncOutputBufferElement::isEmpty() const
	{
		if (!(m_header.getSize()) && (!m_sizeBody || m_body.isNull())) {
			return sl_true;
		}
		return sl_false;
	}

	sl_bool AsyncOutputBufferElement::isEmptyBody() const
	{
		if (!m_sizeBody || m_body.isNull()) {
			return sl_true;
		}
		return sl_false;
	}

	sl_bool AsyncOutputBufferElement::addHeader(const Memory& header)
	{
		return m_header.add(header);
	}

	void AsyncOutputBufferElement::setBody(AsyncStream* stream, sl_uint64 size)
	{
		m_body = stream;
		m_sizeBody = size;
	}

	MemoryQueue& AsyncOutputBufferElement::getHeader()
	{
		return m_header;
	}

	Ref<AsyncStream> AsyncOutputBufferElement::getBody()
	{
		return m_body;
	}

	sl_uint64 AsyncOutputBufferElement::getBodySize()
	{
		return m_sizeBody;
	}


	SLIB_DEFINE_OBJECT(AsyncOutputBuffer, Object)

	AsyncOutputBuffer::AsyncOutputBuffer()
	{
		m_lengthOutput = 0;
	}

	AsyncOutputBuffer::~AsyncOutputBuffer()
	{
	}

	void AsyncOutputBuffer::clearOutput()
	{
		m_lengthOutput = 0;
		m_queueOutput.removeAll();
	}

	sl_bool AsyncOutputBuffer::write(const void* buf, sl_size size)
	{
		return write(Memory::create(buf, size));
	}

	sl_bool AsyncOutputBuffer::write(const Memory& mem)
	{
		if (mem.isNull()) {
			return sl_false;
		}
		ObjectLocker lock(this);
		Link< Ref<AsyncOutputBufferElement> >* link = m_queueOutput.getBack();
		if (link && link->value->isEmptyBody()) {
			if (link->value->addHeader(mem)) {
				m_lengthOutput += mem.getSize();
			} else {
				return sl_false;
			}
		} else {
			Ref<AsyncOutputBufferElement> data = new AsyncOutputBufferElement(mem);
			if (data.isNotNull()) {
				m_queueOutput.push(data);
				m_lengthOutput += mem.getSize();
			} else {
				return sl_false;
			}
		}
		return sl_true;
	}

	sl_bool AsyncOutputBuffer::copyFrom(AsyncStream* stream, sl_uint64 size)
	{
		if (!size) {
			return sl_true;
		}
		if (!stream) {
			return sl_false;
		}
		ObjectLocker lock(this);
		Link< Ref<AsyncOutputBufferElement> >* link = m_queueOutput.getBack();
		if (link && link->value->isEmptyBody()) {
			link->value->setBody(stream, size);
			m_lengthOutput += size;
		} else {
			Ref<AsyncOutputBufferElement> data = new AsyncOutputBufferElement(stream, size);
			if (data.isNotNull()) {
				if (m_queueOutput.push(data)) {
					m_lengthOutput += size;
				} else {
					return sl_false;
				}
			} else {
				return sl_false;
			}
		}
		return sl_true;
	}

	sl_bool AsyncOutputBuffer::copyFromFile(const StringParam& path)
	{
		return copyFromFile(path, sl_null, sl_null);
	}

	sl_bool AsyncOutputBuffer::copyFromFile(const StringParam& path, const Ref<AsyncIoLoop>& ioLoop, const Ref<Dispatcher>& dispatcher)
	{
		sl_uint64 size;
		if (File::getSize(path, size)) {
			if (size > 0) {
				Ref<AsyncStream> file = AsyncFile::openStream(path, FileMode::Read, ioLoop, dispatcher);
				if (file.isNotNull()) {
					return copyFrom(file.get(), size);
				} else {
					return sl_false;
				}
			} else {
				return sl_true;
			}
		}
		return sl_false;
	}

	sl_uint64 AsyncOutputBuffer::getOutputLength() const
	{
		return m_lengthOutput;
	}


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(AsyncOutputParam)

	AsyncOutputParam::AsyncOutputParam()
	{
		bufferSize = 0x10000;
		bufferCount = 3;
	}

	SLIB_DEFINE_OBJECT(AsyncOutput, AsyncOutputBuffer)

	AsyncOutput::AsyncOutput()
	{
		m_flagClosed = sl_false;
		m_flagWriting = sl_false;

		m_bufferCount = 1;
		m_bufferSize = 0x10000;
	}

	AsyncOutput::~AsyncOutput()
	{
		close();
	}

	void AsyncOutput::close()
	{
		ObjectLocker lock(this);
		if (m_flagClosed) {
			return;
		}
		m_flagClosed = sl_true;
		Ref<AsyncCopy> copy = m_copy;
		if (copy.isNotNull()) {
			copy->close();
		}
		m_copy.setNull();
		m_streamOutput.setNull();
	}

	Ref<AsyncOutput> AsyncOutput::create(const AsyncOutputParam& param)
	{
		if (param.stream.isNull()) {
			return sl_null;
		}
		Memory buffer = Memory::create(param.bufferSize);
		if (buffer.isNull()) {
			return sl_null;
		}
		Ref<AsyncOutput> ret = new AsyncOutput;
		if (ret.isNotNull()) {
			ret->m_streamOutput = param.stream;
			ret->m_bufferSize = param.bufferSize;
			ret->m_bufferCount = param.bufferCount;
			ret->m_onEnd = param.onEnd;
			ret->m_bufWrite = buffer;
			return ret;
		}
		return sl_null;
	}

	void AsyncOutput::mergeBuffer(AsyncOutputBuffer* buffer)
	{
		ObjectLocker lock(this);
		m_queueOutput.merge(&(buffer->m_queueOutput));
		m_lengthOutput += buffer->m_lengthOutput;
	}

	void AsyncOutput::startWriting()
	{
		_write(sl_false);
	}

	sl_bool AsyncOutput::isWriting()
	{
		return m_flagWriting;
	}

	void AsyncOutput::_write(sl_bool flagCompleted)
	{
		ObjectLocker lock(this);
		if (m_flagClosed) {
			return;
		}
		if (m_flagWriting) {
			return;
		}
		for (;;) {
			if (m_elementWriting.isNotNull()) {
				if (m_elementWriting->isEmpty()) {
					m_elementWriting.setNull();
				} else {
					break;
				}
			}
			if (!(m_queueOutput.pop(&m_elementWriting))) {
				if (flagCompleted) {
					_onComplete();
				}
				return;
			}
		}
		MemoryQueue& header = m_elementWriting->getHeader();
		if (header.getSize() > 0) {
			sl_size size = header.pop(m_bufWrite.getData(), m_bufWrite.getSize());
			if (size > 0) {
				m_flagWriting = sl_true;
				m_streamOutput->write(m_bufWrite.getData(), size, SLIB_FUNCTION_WEAKREF(this, onWriteStream), m_bufWrite.ref.get());
			}
		} else {
			sl_uint64 sizeBody = m_elementWriting->getBodySize();
			Ref<AsyncStream> body = m_elementWriting->getBody();
			if (sizeBody != 0 && body.isNotNull()) {
				m_flagWriting = sl_true;
				m_elementWriting.setNull();
				AsyncCopyParam param;
				param.source = body;
				param.target = m_streamOutput;
				param.size = sizeBody;
				param.bufferSize = m_bufferSize;
				param.bufferCount = m_bufferCount;
				param.onEnd = SLIB_FUNCTION_WEAKREF(this, onAsyncCopyEnd);
				Ref<AsyncCopy> copy = AsyncCopy::create(param);
				if (copy.isNotNull()) {
					m_copy = copy;
				} else {
					m_flagWriting = sl_false;
					_onError();
				}
			}
		}
	}

	void AsyncOutput::onAsyncCopyEnd(AsyncCopy* task, sl_bool flagError)
	{
		m_flagWriting = sl_false;
		if (flagError || !(task->isCompleted())) {
			_onError();
		} else {
			_write(sl_true);
		}
	}

	void AsyncOutput::onWriteStream(AsyncStreamResult& result)
	{
		m_flagWriting = sl_false;
		if (!(result.isSuccess())) {
			_onError();
			return;
		}
		_write(sl_true);
	}

	void AsyncOutput::_onError()
	{
		m_onEnd(this, sl_true);
	}

	void AsyncOutput::_onComplete()
	{
		m_onEnd(this, sl_false);
	}


	SLIB_DEFINE_OBJECT(AsyncStreamFilter, AsyncStream)

	AsyncStreamFilter::AsyncStreamFilter()
	{
		m_flagOpened = sl_true;

		m_flagReading = sl_false;
		m_flagReadingError = sl_false;
		m_flagReadingEnded = sl_false;

		m_flagWritingError = sl_false;
		m_flagWritingEnded = sl_false;
	}

	AsyncStreamFilter::~AsyncStreamFilter()
	{
		close();
	}

	void AsyncStreamFilter::close()
	{
		MultipleMutexLocker lock(&m_lockReading, &m_lockWriting);
		if (m_flagOpened) {
			m_flagOpened = sl_false;
		}
		setReadingEnded();
		setWritingEnded();
		m_stream.setNull();
	}

	sl_bool AsyncStreamFilter::isOpened()
	{
		return m_flagOpened;
	}

	sl_bool AsyncStreamFilter::requestIo(AsyncStreamRequest* request)
	{
		if (request->flagRead) {
			MutexLocker lock(&m_lockReading);
			if (!m_flagOpened) {
				return sl_false;
			}
			if (m_requestsRead.push(request)) {
				if (m_flagReadingEnded) {
					return sl_false;
				}
				return _read();
			}
			return sl_false;
		} else {
			MutexLocker lock(&m_lockWriting);
			Ref<AsyncStream> stream = m_stream;
			if (stream.isNull()) {
				return sl_false;
			}
			if (!m_flagOpened) {
				return sl_false;
			}
			if (m_flagWritingError) {
				return sl_false;
			}
			if (m_flagWritingEnded) {
				return sl_false;
			}
			if (request->size) {
				MemoryData memConverted;
				if (filterWrite(memConverted, request->data, request->size, request->userObject.get())) {
					if (request->size) {
						auto thiz = ToWeakRef(this);
						void* data = request->data;
						sl_size size = request->size;
						MoveT< Ref<CRef> > _userObject = Move(request->userObject);
						MoveT< Function<void(AsyncStreamResult&)> > callback = Move(request->callback);
						request->callback = [thiz, this, data, size, _userObject, callback](AsyncStreamResult& result) {
							auto ref = ToRef(thiz);
							if (ref.isNull()) {
								return;
							}
							AsyncStreamRequest* request = result.request;
							request->data = data;
							request->size = size;
							request->userObject = _userObject.release();
							request->callback = callback.release();
							request->resetPassedSize();
							if (!(result.isSuccess())) {
								m_flagWritingError = sl_true;
							}
							request->runCallback(this, size, m_flagWritingError ? AsyncStreamResultCode::Unknown : AsyncStreamResultCode::Success);
						};
						request->data = memConverted.data;
						request->size = memConverted.size;
						request->userObject = Move(memConverted.ref);
						return stream->requestIo(request);
					} else {
						return sl_true;
					}
				}
			} else {
				return stream->requestIo(request);
			}
			return sl_false;
		}
	}

	sl_bool AsyncStreamFilter::addReadData(void* data, sl_size size, CRef* userObject)
	{
		if (data) {
			if (size) {
				MemoryData mem;
				MutexLocker lock(&m_lockReading);
				if (filterRead(mem, data, size, userObject)) {
					if (mem.size) {
						return m_bufReadConverted.add(Move(mem));
					} else {
						return sl_true;
					}
				}
				return sl_false;
			} else {
				return sl_true;
			}
		}
		return sl_false;
	}

	sl_bool AsyncStreamFilter::addReadData(const Memory& mem)
	{
		return addReadData(mem.getData(), mem.getSize(), mem.getRef());
	}

	sl_bool AsyncStreamFilter::addReadData(void* data, sl_size size)
	{
		if (data) {
			if (size) {
				Memory mem = Memory::create(data, size);
				if (mem.isNotNull()) {
					return addReadData(mem);
				}
			} else {
				return sl_true;
			}
		}
		return sl_false;
	}

	void AsyncStreamFilter::setReadingBufferSize(sl_uint32 sizeBuffer)
	{
		if (sizeBuffer > 0) {
			m_memReading = Memory::create(sizeBuffer);
		}
	}

	sl_bool AsyncStreamFilter::_read()
	{
		Ref<AsyncStream> stream = m_stream;
		if (stream.isNull()) {
			return sl_false;
		}
		if (m_flagReading) {
			return sl_false;
		}
		if (m_flagReadingEnded) {
			return sl_false;
		}
		if (m_flagReadingError) {
			return sl_false;
		}
		_processReadRequests();
		if (m_requestsRead.isEmpty()) {
			return sl_true;
		}
		Memory mem = m_memReading;
		if (mem.isNull()) {
			mem = Memory::create(SLIB_ASYNC_STREAM_FILTER_DEFAULT_BUFFER_SIZE);
			if (mem.isNull()) {
				m_flagReadingError = sl_true;
				_closeAllReadRequests();
				return sl_false;
			}
			m_memReading = mem;
		}
		m_flagReading = sl_true;
		stream->read(mem, SLIB_FUNCTION_WEAKREF(this, onReadStream));
		return sl_true;
	}

	// returns true when no request is remained
	void AsyncStreamFilter::_processReadRequests()
	{
		while (m_bufReadConverted.getSize()) {
			Ref<AsyncStreamRequest> req;
			if (!(m_requestsRead.pop(&req))) {
				break;
			}
			if (req.isNotNull()) {
				if (req->data && req->size) {
					sl_size m = m_bufReadConverted.pop(req->data, req->size);
					if (m_bufReadConverted.getSize()) {
						req->runCallback(this, m, AsyncStreamResultCode::Success);
					} else {
						if (m_flagReadingEnded) {
							req->runCallback(this, m, AsyncStreamResultCode::Ended);
						} else if (m_flagReadingError) {
							req->runCallback(this, m, AsyncStreamResultCode::Unknown);
						} else {
							req->runCallback(this, m, AsyncStreamResultCode::Success);
						}
						break;
					}
				} else {
					req->runCallback(this, 0, AsyncStreamResultCode::Success);
				}
			}
		}
	}

	void AsyncStreamFilter::onReadStream(AsyncStreamResult& result)
	{
		MutexLocker lock(&m_lockReading);
		m_flagReading = sl_false;
		if (!m_flagOpened) {
			return;
		}
		if (result.size > 0) {
			addReadData(result.data, result.size, result.userObject);
		}
		if (result.isError()) {
			m_flagReadingError = sl_true;
		} else if (result.isEnded()) {
			m_flagReadingEnded = sl_true;
		}
		_processReadRequests();
		if (m_flagReadingError) {
			_closeAllReadRequests();
			return;
		}
		if (m_requestsRead.isNotEmpty()) {
			_read();
		}
	}

	sl_bool AsyncStreamFilter::addTask(const Function<void()>& callback)
	{
		Ref<AsyncStream> stream = m_stream;
		if (stream.isNotNull()) {
			return stream->addTask(callback);
		}
		return sl_false;
	}

	sl_bool AsyncStreamFilter::isReadingError()
	{
		return m_flagReadingError;
	}

	void AsyncStreamFilter::setReadingError()
	{
		m_flagReadingError = sl_true;
	}

	sl_bool AsyncStreamFilter::isReadingEnded()
	{
		return m_flagReadingEnded;
	}

	void AsyncStreamFilter::setReadingEnded()
	{
		m_flagReadingEnded = sl_true;
	}

	sl_bool AsyncStreamFilter::isWritingError()
	{
		return m_flagWritingError;
	}

	void AsyncStreamFilter::setWritingError()
	{
		m_flagWritingError = sl_true;
	}

	sl_bool AsyncStreamFilter::isWritingEnded()
	{
		return m_flagWritingEnded;
	}

	void AsyncStreamFilter::setWritingEnded()
	{
		m_flagWritingEnded = sl_true;
	}

	sl_bool AsyncStreamFilter::filterRead(MemoryData& output, void* data, sl_size size, CRef* userObject)
	{
		output.data = data;
		output.size = size;
		output.ref = userObject;
		return sl_true;
	}

	sl_bool AsyncStreamFilter::filterWrite(MemoryData& output, void* data, sl_size size, CRef* userObject)
	{
		output.data = data;
		output.size = size;
		output.ref = userObject;
		return sl_true;
	}

	void AsyncStreamFilter::_closeAllReadRequests()
	{
		Ref<AsyncStreamRequest> req;
		while (m_requestsRead.pop(&req)) {
			if (req.isNotNull()) {
				req->runCallback(this, 0, AsyncStreamResultCode::Closed);
			}
		}
	}

}
