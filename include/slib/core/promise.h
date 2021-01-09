/*
 *   Copyright (c) 2008-2019 SLIBIO <https://github.com/SLIBIO>
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

#ifndef CHECKHEADER_SLIB_CORE_PROMISE
#define CHECKHEADER_SLIB_CORE_PROMISE

#include "function.h"
#include "event.h"
#include "dispatch.h"

// Javascript-style Promise

namespace slib
{
	
	template <class T>
	class CPromise;
	
	template <class T>
	class Promise;
	
	template <class T>
	using AtomicPromise = Atomic< Promise<T> >;
	
	enum class PromiseState
	{
		Pending = 0,
		Resolved = 1,
		Completed = 2
	};
	
	namespace priv
	{
		namespace promise
		{
			extern const char g_classID[];

			template <class T>
			struct PromiseAllContext : public Referable
			{
				SpinLock lock;
				List<T> results;
				sl_reg nCompleted;
			};
		}
	}
	
	template <class T>
	class SLIB_EXPORT CPromise : public Referable
	{
		SLIB_TEMPLATE_ROOT_OBJECT(priv::promise::g_classID)

	private:
		PromiseState m_state;
		Function<void(T& result)> m_callback;
		SpinLock m_lock;
		sl_uint8 m_result[sizeof(T)];

	public:
		CPromise(): m_state(PromiseState::Pending)
		{
		}
		
		template <class... ARGS>
		CPromise(PromiseState state, ARGS&&... args): m_state(state)
		{
			new ((T*)((void*)m_result)) T(Forward<ARGS>(args)...);
		}
		
		~CPromise()
		{
			if (m_state != PromiseState::Pending) {
				(*((T*)((void*)m_result))).T::~T();
			}
		}
		
	public:
		PromiseState getState()
		{
			return m_state;
		}
		
		template <class... ARGS>
		void resolve(ARGS&&... args)
		{
			SpinLocker lock(&m_lock);
			if (m_state != PromiseState::Pending) {
				return;
			}
			new ((T*)((void*)m_result)) T(Forward<ARGS>(args)...);
			auto callback = Move(m_callback);
			if (callback.isNotNull()) {
				m_state = PromiseState::Completed;
				lock.unlock();
				callback(*((T*)((void*)m_result)));
			} else {
				m_state = PromiseState::Resolved;
			}
		}
		
		template <class CALLBACK_TYPE>
		void then(CALLBACK_TYPE&& callback)
		{
			SpinLocker lock(&m_lock);
			if (m_state == PromiseState::Resolved) {
				m_state = PromiseState::Completed;
				lock.unlock();
				callback(*((T*)((void*)m_result)));
			} else if (m_state == PromiseState::Pending) {
				m_callback = Forward<CALLBACK_TYPE>(callback);
			}
		}
		
	};
	
	template <class T>
	class SLIB_EXPORT Promise
	{
	public:
		Ref< CPromise<T> > ref;
		SLIB_REF_WRAPPER(Promise, CPromise<T>)

	public:
		static Promise<T> create()
		{
			return new CPromise<T>();
		}
		
		template <class... ARGS>
		static Promise<T> fromValue(ARGS&&... args)
		{
			return new CPromise<T>(PromiseState::Resolved, Forward<ARGS>(args)...);
		}
		
		static const Promise<T>& from(const Promise<T>& other)
		{
			return other;
		}
		
		template <class OTHER>
		static Promise<T> from(const Promise<OTHER>& other)
		{
			Promise<T> ret;
			ret.initialize();
			other.then([ret](OTHER& result) {
				ret.resolve(result);
			});
			return ret;
		}
		
		void initialize()
		{
			ref = new CPromise<T>();
		}
		
		PromiseState getState() const
		{
			CPromise<T>* object = ref.ptr;
			if (object) {
				return object->getState();
			} else {
				return PromiseState::Resolved;
			}
		}
		
		template <class... ARGS>
		void resolve(ARGS&&... args) const
		{
			CPromise<T>* object = ref.ptr;
			if (object) {
				object->resolve(Forward<ARGS>(args)...);
			}
		}
		
		template <class CALLBACK_TYPE>
		void then(CALLBACK_TYPE&& callback) const
		{
			CPromise<T>* object = ref.ptr;
			if (object) {
				object->then(Forward<CALLBACK_TYPE>(callback));
			} else {
				T result{};
				callback(result);
			}
		}
		
		template <class RET>
		Promise<RET> thenReturn(const Function<RET(T& result)>& callback) const
		{
			Promise<RET> promise;
			promise.initialize();
			then([promise, callback](T& result) {
				promise.resolve(callback(result));
			});
			return promise;
		}
		
		template <class RET>
		Promise<RET> thenPromise(const Function<Promise<RET>(T& result)>& callback) const
		{
			Promise<RET> promise;
			promise.initialize();
			then([promise, callback](T& result) {
				callback(result).then([promise](RET& ret) {
					promise.resolve(Move(ret));
				});
			});
			return promise;
		}
		
		// don't use mixed with `then()`, timeoutMilliseconds: negative means infinite timeout
		sl_bool wait(sl_int32 timeoutMilliseconds = -1) const
		{
			Ref<Event> event = Event::create();
			then([&event](T& result) {
				event->set();
			});
			return event->wait(timeoutMilliseconds);
		}
		
		// don't use mixed with `then()`, timeoutMilliseconds: negative means infinite timeout
		sl_bool wait(T* _output, sl_int32 timeoutMilliseconds = -1) const
		{
			Ref<Event> event = Event::create();
			then([&event, _output](T& result) {
				if (_output) {
					*_output = Move(result);
				}
				event->set();
			});
			return event->wait(timeoutMilliseconds);
		}
		
		template <class... PROMISES>
		static Promise< List<T> > all(PROMISES&&... _promises)
		{
			Promise<T> promises[] = {Forward<PROMISES>(_promises)...};
			sl_size n = sizeof...(_promises);
			if (!n) {
				return sl_null;
			}
			Ref< priv::promise::PromiseAllContext<T> > refContext = new priv::promise::PromiseAllContext<T>;
			priv::promise::PromiseAllContext<T>* context = refContext.get();
			context->results = List<T>::create(n);
			if (context->results.isNull()) {
				return sl_null;
			}
			context->nCompleted = 0;
			Promise< List<T> > ret;
			ret.initialize();
			for (sl_size i = 0; i < n; i++) {
				Promise<T>& promise = promises[i];
				sl_size index = i;
				promise.then([ret, index, n, context, refContext](T& result) {
					context->results.setAt_NoLock(index, Move(result));
					if (Base::interlockedIncrement(&(context->nCompleted)) == n) {
						ret.resolve(context->results);
					}
				});
			}
			return ret;
		}
		
		static Promise< List<T> > allList(const List< Promise<T> >& _promises)
		{
			sl_size n = _promises.getCount();
			if (!n) {
				return sl_null;
			}
			ListElements< Promise<T> > promises(_promises.duplicate());
			if (!(promises.count)) {
				return sl_null;
			}
			Ref< priv::promise::PromiseAllContext<T> > refContext = new priv::promise::PromiseAllContext<T>;
			priv::promise::PromiseAllContext<T>* context = refContext.get();
			context->results = List<T>::create(n);
			if (context->results.isNull()) {
				return sl_null;
			}
			context->nCompleted = 0;
			Promise< List<T> > ret;
			ret.initialize();
			for (sl_size i = 0; i < promises.count; i++) {
				Promise<T>& promise = promises[i];
				sl_size index = i;
				promise.then([ret, index, n, context, refContext](T& result) {
					context->results.setAt_NoLock(index, Move(result));
					if (Base::interlockedIncrement(&(context->nCompleted)) == n) {
						ret.resolve(context->results);
					}
				});
			}
			return ret;
		}
		
		template <class... PROMISES>
		static Promise<T> race(PROMISES&&... _promises)
		{
			Promise<T> promises[] = {Forward<PROMISES>(_promises)...};
			sl_size n = sizeof...(_promises);
			if (!n) {
				return sl_null;
			}
			Promise<T> ret;
			ret.initialize();
			for (sl_size i = 0; i < n; i++) {
				Promise<T>& promise = promises[i];
				promise.then([ret](T& result) {
					ret.resolve(Move(result));
				});
			}
			return ret;
		}
		
		static Promise<T> raceList(const List< Promise<T> >& _promises)
		{
			sl_size n = _promises.getCount();
			if (!n) {
				return sl_null;
			}
			ListElements< Promise<T> > promises(_promises.duplicate());
			if (!(promises.count)) {
				return sl_null;
			}
			Promise<T> ret;
			ret.initialize();
			for (sl_size i = 0; i < promises.count; i++) {
				Promise<T>& promise = promises[i];
				promise.then([ret](T& result) {
					ret.resolve(Move(result));
				});
			}
			return ret;
		}

		static Promise<T> dispatch(const Ref<Dispatcher>& dispatcher, const Function<T()>& task)
		{
			if (dispatcher.isNotNull()) {
				Promise<T> ret;
				ret.initialize();
				if (dispatcher->dispatch([ret, task]() {
					ret.resolve(task());
				})) {
					return ret;
				}
			}
			return sl_null;
		}
		
		static Promise<T> dispatch(const Function<T()>& task)
		{
			Promise<T> ret;
			ret.initialize();
			if (Dispatch::dispatch([ret, task]() {
				ret.resolve(task());
			})) {
				return ret;
			}
			return sl_null;
		}
		
		static Promise<T> setTimeout(const Ref<Dispatcher>& dispatcher, const Function<T()>& task, sl_uint64 delay_ms)
		{
			if (dispatcher.isNotNull()) {
				Promise<T> ret;
				ret.initialize();
				if (dispatcher->dispatch([ret, task]() {
					ret.resolve(task());
				}, delay_ms)) {
					return ret;
				}
			}
			return sl_null;
		}
		
		static Promise<T> setTimeout(const Function<T()>& task, sl_uint64 delay_ms)
		{
			Promise<T> ret;
			ret.initialize();
			if (Dispatch::setTimeout([ret, task]() {
				ret.resolve(task());
			}, delay_ms)) {
				return ret;
			}
			return sl_null;
		}

		static Promise<T> dispatchPromise(const Ref<Dispatcher>& dispatcher, const Function<Promise<T>()>& task)
		{
			if (dispatcher.isNotNull()) {
				Promise<T> ret;
				ret.initialize();
				if (dispatcher->dispatch([ret, task]() {
					task().then([ret](T& result) {
						ret.resolve(result);
					});
				})) {
					return ret;
				}
			}
			return sl_null;
		}
		
		static Promise<T> dispatchPromise(const Function<Promise<T>()>& task)
		{
			Promise<T> ret;
			ret.initialize();
			if (Dispatch::dispatch([ret, task]() {
				task().then([ret](T& result) {
					ret.resolve(result);
				});
			})) {
				return ret;
			}
			return sl_null;
		}
		
		static Promise<T> setTimeoutPromise(const Ref<Dispatcher>& dispatcher, const Function<Promise<T>()>& task, sl_uint64 delay_ms)
		{
			if (dispatcher.isNotNull()) {
				Promise<T> ret;
				ret.initialize();
				if (dispatcher->dispatch([ret, task]() {
					task().then([ret](T& result) {
						ret.resolve(result);
					});
				}, delay_ms)) {
					return ret;
				}
			}
			return sl_null;
		}
		
		static Promise<T> setTimeoutPromise(const Function<Promise<T>()>& task, sl_uint64 delay_ms)
		{
			Promise<T> ret;
			ret.initialize();
			if (Dispatch::setTimeout([ret, task]() {
				task().then([ret](T& result) {
					ret.resolve(result);
				});
			}, delay_ms)) {
				return ret;
			}
			return sl_null;
		}

	};
	
	template <class T>
	class SLIB_EXPORT Atomic< Promise<T> >
	{
	public:
		AtomicRef< CPromise<T> > ref;
		SLIB_ATOMIC_REF_WRAPPER(CPromise<T>)
	};
	
}

#endif
