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

#include "slib/network/definition.h"

#if defined(SLIB_PLATFORM_IS_MACOS)

#include "slib/network/ipc.h"

#include "slib/core/thread.h"
#include "slib/core/timeout.h"
#include "slib/core/safe_static.h"
#include "slib/platform.h"
#include "slib/platform/apple/mach_port.h"

namespace slib
{

	namespace
	{
		typedef apple::MachPort MachPort;

		static String GetTargetName(const StringParam& name, sl_bool flagGlobal)
		{
			if (flagGlobal) {
				return name.toString();
			} else {
				return String::concat(String::fromUint32((sl_uint32)(getuid())), StringView::literal("_"), name);
			}
		}
	
		class MachPortRequest : public IPCRequest
		{
		public:
			MachPort m_portLocal;
			MachPort m_portRemote;
			dispatch_source_t m_dispatchSource = nil;

		public:
			MachPortRequest()
			{
			}

			~MachPortRequest()
			{
				if (m_dispatchSource != nil) {
					dispatch_source_cancel(m_dispatchSource);
				}
			}

		public:
			static Ref<MachPortRequest> create(const IPCRequestParam& param)
			{
				MachPort portRemote = MachPort::lookUp(GetTargetName(param.targetName, param.flagGlobal));
				if (portRemote.isNotNone()) {
					MachPort portLocal = MachPort::create(sl_true);
					if (portLocal.isNotNone()) {
						sl_int64 tickEnd = GetTickFromTimeout(param.timeout);
						if (MachPort::sendMessage(portLocal.get(), portRemote.get(), param.message.data, param.message.size, param.timeout)) {
							dispatch_queue_t queue = getDispatchQueue();
							if (queue != nil) {
								dispatch_source_t source = dispatch_source_create(DISPATCH_SOURCE_TYPE_MACH_RECV, portLocal.get(), 0, queue);
								if (source != nil) {
									Ref<MachPortRequest> ret = new MachPortRequest;
									if (ret.isNotNull()) {
										ret->m_portLocal = Move(portLocal);
										ret->m_portRemote = Move(portRemote);
										ret->initialize(param, tickEnd);
										WeakRef<MachPortRequest> weak(ret);
										dispatch_source_set_event_handler(source, ^{
											Ref<MachPortRequest> request(weak);
											if (request.isNotNull()) {
												request->receive();
											}
										});
										dispatch_resume(source);
										sl_int32 timeout = GetTimeoutFromTick(tickEnd);
										if (timeout >= 0) {
											dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(timeout * NSEC_PER_MSEC)), queue, ^{
												Ref<MachPortRequest> request(weak);
												if (request.isNotNull()) {
													request->dispatchError();
												}
											});
										}
										ret->m_dispatchSource = source;
										return ret;
									}
								}
							}
						}
					}
				}
				IPCResponseMessage errorMsg;
				param.onResponse(errorMsg);
				return sl_null;
			}

			static dispatch_queue_t getDispatchQueue()
			{
				static dispatch_queue_t ret = nil;
				if (ret != nil) {
					return ret;
				}
				SLIB_STATIC_SPINLOCKER(lock)
				if (ret == nil) {
					ret = dispatch_queue_create(sl_null, DISPATCH_QUEUE_SERIAL);
				}
				return ret;
			}

		public:
			void receive()
			{
				Memory data;
				mach_port_t remotePort;
				sl_int32 timeout = GetTimeoutFromTick(m_tickEnd);
				if (MachPort::receiveMessage(data, m_portLocal.get(), &remotePort, sl_null, timeout)) {
					if (remotePort != m_portRemote.get()) {
						return;
					}
				}
				dispatchResponse(data);
			}
		};

		class MachPortServer : public IPCServer
		{
		public:
			MachPort m_port;
			Ref<Thread> m_thread;

		public:
			MachPortServer()
			{
			}

			~MachPortServer()
			{
				if (m_thread.isNotNull()) {
					m_thread->finishAndWait();
				}
			}

		public:
			static Ref<MachPortServer> create(const IPCServerParam& param)
			{
				MachPort port = MachPort::checkIn(GetTargetName(param.name, param.flagGlobal), sl_true);
				if (port.isNotNone()) {
					Ref<MachPortServer> ret = new MachPortServer;
					if (ret.isNotNull()) {
						Ref<Thread> thread = Thread::create(SLIB_FUNCTION_MEMBER(ret.get(), run));
						if (thread.isNotNull()) {
							ret->initialize(param);
							ret->m_port = Move(port);
							ret->m_thread = Move(thread);
							if (ret->m_thread->start()) {
								return ret;
							}
						}
					}
				}
				return sl_null;
			}

		public:
			void run()
			{
				Thread* thread = Thread::getCurrent();
				if (!thread) {
					return;
				}
				while (thread->isNotStopping()) {
					Memory data;
					mach_port_t remotePort;
					sl_uint32 pid;
					if (MachPort::receiveMessage(data, m_port.get(), &remotePort, &pid, 10)) {
						if (m_dispatcher.isNotNull()) {
							WeakRef<IPCServer> thiz = this;
							m_dispatcher->dispatch([this, thiz, data, remotePort, pid]() {
								Ref<IPCServer> ref = thiz;
								if (ref.isNull()) {
									return;
								}
								processRequest(data, remotePort, pid);
							});
						} else {
							processRequest(data, remotePort, pid);
						}
					}
				}
			}

			void processRequest(const Memory& data, mach_port_t remotePort, sl_uint32 pid)
			{
				IPCRequestMessage request(data);
				request.remoteProcessId = pid;
				IPCResponseMessage response;
				m_onReceiveMessage(request, response);
				MachPort::sendMessage(m_port.get(), remotePort, response.data, response.size, m_responseTimeout);
			}
		};
	}

	Ref<IPCRequest> IPC::sendMessage(const RequestParam& param)
	{
		return Ref<IPCRequest>::cast(MachPortRequest::create(param));
	}

	sl_bool IPC::sendMessageSynchronous(const RequestParam& param, ResponseMessage& response)
	{
		MachPort portRemote = MachPort::lookUp(GetTargetName(param.targetName, param.flagGlobal));
		if (portRemote.isNone()) {
			return sl_false;
		}
		MachPort portLocal = MachPort::create(sl_true);
		if (portLocal.isNone()) {
			return sl_false;
		}
		sl_int64 tickEnd = GetTickFromTimeout(param.timeout);
		if (!(MachPort::sendMessage(portLocal.get(), portRemote.get(), param.message.data, param.message.size, param.timeout))) {
			return sl_false;
		}
		sl_int32 timeout = GetTimeoutFromTick(tickEnd);
		Memory data;
		mach_port_t remotePort;
		if (MachPort::receiveMessage(data, portLocal.get(), &remotePort, sl_null, timeout)) {
			if (remotePort == portRemote.get()) {
				response.setMemory(data);
				return sl_true;
			}
		}
		return sl_false;
	}

	Ref<IPCServer> IPC::createServer(const ServerParam& param)
	{
		return Ref<IPCServer>::cast(MachPortServer::create(param));
	}

}

#endif
