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

		struct MACH_MESSAGE_BASE
		{
			mach_msg_header_t header;
			mach_msg_body_t body;
			mach_msg_ool_descriptor_t data;
			mach_msg_type_number_t count;
		};

		typedef MACH_MESSAGE_BASE MACH_SEND_MESSAGE;
	
		struct MACH_RECEIVE_MESSAGE : MACH_MESSAGE_BASE
		{
			sl_uint8 trailer[sizeof(void*) * 8];
		};

		static sl_bool SendMachMessage(mach_port_t from, mach_port_t to, const void* data, sl_size size, sl_int32 _timeout)
		{
			MACH_SEND_MESSAGE msg = {};
			msg.header.msgh_remote_port = to;
			msg.header.msgh_local_port = from;
			msg.header.msgh_bits = MACH_MSGH_BITS(MACH_MSG_TYPE_COPY_SEND, MACH_MSG_TYPE_COPY_SEND) | MACH_MSGH_BITS_COMPLEX;
			msg.header.msgh_size = sizeof(msg);
			msg.body.msgh_descriptor_count = 1;
			msg.data.address = (void*)data;
			msg.data.size = (mach_msg_size_t)size;
			msg.data.copy = MACH_MSG_VIRTUAL_COPY;
			msg.data.type = MACH_MSG_OOL_DESCRIPTOR;
			msg.count = msg.data.size;
			mach_msg_option_t options = MACH_SEND_MSG;
			mach_msg_timeout_t timeout;
			if (_timeout >= 0) {
				options |= MACH_SEND_TIMEOUT;
				timeout = (mach_msg_timeout_t)_timeout;
				if (timeout == MACH_MSG_TIMEOUT_NONE) {
					timeout = 1;
				}
			} else {
				timeout = MACH_MSG_TIMEOUT_NONE;
			}
			kern_return_t kRet = mach_msg(&msg.header, options, sizeof(msg), 0, MACH_PORT_NULL, timeout, MACH_PORT_NULL);
			return kRet == KERN_SUCCESS;
		}

		class KernelMemory : public CMemory
		{
		public:
			KernelMemory(const void* data, sl_size size): CMemory(data, size) {}

			~KernelMemory()
			{
				vm_deallocate(mach_task_self(), (vm_address_t)data, (vm_size_t)size);
			}
		};

		static sl_bool ReceiveMachMessage(mach_port_t from, sl_int32 _timeout, Memory& outData, mach_port_t& outRemotePort, sl_uint32& outPid)
		{
			MACH_RECEIVE_MESSAGE msg = {};
			msg.header.msgh_size = sizeof(msg);
			mach_msg_option_t options = MACH_RCV_MSG | MACH_RCV_LARGE | MACH_RCV_TRAILER_TYPE(MACH_MSG_TRAILER_FORMAT_0) | MACH_RCV_TRAILER_ELEMENTS(MACH_RCV_TRAILER_AUDIT);
			mach_msg_timeout_t timeout;
			if (_timeout >= 0) {
				options |= MACH_RCV_TIMEOUT;
				timeout = (mach_msg_timeout_t)_timeout;
				if (timeout == MACH_MSG_TIMEOUT_NONE) {
					timeout = 1;
				}
			} else {
				timeout = MACH_MSG_TIMEOUT_NONE;
			}
			kern_return_t kRet = mach_msg(&msg.header, options, 0, sizeof(msg), from, timeout, MACH_PORT_NULL);
			if (kRet != KERN_SUCCESS) {
				return sl_false;
			}
			outRemotePort = msg.header.msgh_remote_port;
			mach_msg_audit_trailer_t* trailer = (mach_msg_audit_trailer_t*)(msg.trailer);
			if(trailer->msgh_trailer_size == sizeof(mach_msg_audit_trailer_t)) {
				outPid = (sl_uint32)(trailer->msgh_audit.val[5]);
			} else {
				outPid = 0;
			}
			if (msg.data.address) {
				if (msg.data.size) {
					outData = new KernelMemory(msg.data.address, msg.data.size);
					return outData.isNotNull();
				}
				vm_deallocate(mach_task_self(), (vm_address_t)(msg.data.address), (vm_size_t)(msg.data.size));
			}
			return sl_true;
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
				MachPort portRemote = MachPort::lookUp(param.targetName);
				if (portRemote.isNotNone()) {
					MachPort portLocal = MachPort::create(sl_true);
					if (portLocal.isNotNone()) {
						sl_int64 tickEnd = GetTickFromTimeout(param.timeout);
						if (SendMachMessage(portLocal.get(), portRemote.get(), param.message.data, param.message.size, param.timeout)) {
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
				sl_int32 timeout = GetTimeoutFromTick(m_tickEnd);
				Memory data;
				mach_port_t remotePort;
				sl_uint32 pid;
				if (ReceiveMachMessage(m_portLocal.get(), timeout, data, remotePort, pid)) {
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
				MachPort port = MachPort::checkIn(param.name, sl_true);
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
					if (ReceiveMachMessage(m_port.get(), 10, data, remotePort, pid)) {
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
				SendMachMessage(m_port.get(), remotePort, response.data, response.size, m_responseTimeout);
			}
		};
	}

	Ref<IPCRequest> IPC::sendMessage(const RequestParam& param)
	{
		return Ref<IPCRequest>::cast(MachPortRequest::create(param));
	}

	sl_bool IPC::sendMessageSynchronous(const RequestParam& param, ResponseMessage& response)
{
		MachPort portRemote = MachPort::lookUp(param.targetName);
		if (portRemote.isNone()) {
			return sl_false;
		}
		MachPort portLocal = MachPort::create(sl_true);
		if (portLocal.isNone()) {
			return sl_false;
		}
		sl_int64 tickEnd = GetTickFromTimeout(param.timeout);
		if (!(SendMachMessage(portLocal.get(), portRemote.get(), param.message.data, param.message.size, param.timeout))) {
			return sl_false;
		}
		sl_int32 timeout = GetTimeoutFromTick(tickEnd);
		Memory data;
		mach_port_t remotePort;
		sl_uint32 pid;
		if (ReceiveMachMessage(portLocal.get(), timeout, data, remotePort, pid)) {
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
