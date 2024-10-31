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

#include "slib/platform/definition.h"

#if defined(SLIB_PLATFORM_IS_APPLE)

#include "slib/platform/apple/mach_port.h"

#include <servers/bootstrap.h>

namespace slib
{
	namespace apple
	{

		namespace
		{
			static void CloseMachPort(mach_port_t port)
			{
				mach_port_deallocate(mach_task_self(), port);
			}

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

			class KernelMemory : public CMemory
			{
			public:
				KernelMemory(const void* data, sl_size size): CMemory(data, size) {}

				~KernelMemory()
				{
					vm_deallocate(mach_task_self(), (vm_address_t)data, (vm_size_t)size);
				}
			};
		}

		SLIB_DEFINE_HANDLE_CONTAINER_MEMBERS(MachPort, mach_port_t, handle, MACH_PORT_NULL, CloseMachPort)

		MachPort MachPort::create(sl_bool flagSend)
		{
			mach_port_t port;
			kern_return_t kr = mach_port_allocate(mach_task_self(), MACH_PORT_RIGHT_RECEIVE, &port);
			if (kr == KERN_SUCCESS) {
				if (flagSend) {
					kr = mach_port_insert_right(mach_task_self(), port, port, MACH_MSG_TYPE_MAKE_SEND);
					if (kr == KERN_SUCCESS) {
						return port;
					}
				} else {
					return port;
				}
				CloseMachPort(port);
			}
			return MACH_PORT_NULL;
		}

		MachPort MachPort::lookUp(const StringParam& _name)
		{
			mach_port_t port;
			StringCstr name(_name);
			kern_return_t kr = bootstrap_look_up(bootstrap_port, name.getData(), &port);
			if (kr == KERN_SUCCESS) {
				return port;
			}
			return MACH_PORT_NULL;
		}

		MachPort MachPort::checkIn(const StringParam& _name, sl_bool flagSend)
		{
			mach_port_t port;
			StringCstr name(_name);
			kern_return_t kr = bootstrap_check_in(bootstrap_port, name.getData(), &port);
			if (kr == KERN_SUCCESS) {
				if (flagSend) {
					kr = mach_port_insert_right(mach_task_self(), port, port, MACH_MSG_TYPE_MAKE_SEND);
					if (kr == KERN_SUCCESS) {
						return port;
					}
				} else {
					return port;
				}
			}
			return MACH_PORT_NULL;
		}

		sl_bool MachPort::sendMessage(mach_port_t localPort, mach_port_t remotePort, const void* data, sl_size size, sl_int32 _timeout)
		{
			MACH_SEND_MESSAGE msg = {};
			msg.header.msgh_remote_port = remotePort;
			msg.header.msgh_local_port = localPort;
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

		sl_bool MachPort::receiveMessage(Memory& _out, mach_port_t localPort, mach_port_t* pOutRemotePort, sl_uint32* pOutPid, sl_int32 _timeout)
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
			kern_return_t kRet = mach_msg(&msg.header, options, 0, sizeof(msg), localPort, timeout, MACH_PORT_NULL);
			if (kRet != KERN_SUCCESS) {
				return sl_false;
			}
			if (pOutRemotePort) {
				*pOutRemotePort = msg.header.msgh_remote_port;
			}
			if (pOutPid) {
				mach_msg_audit_trailer_t* trailer = (mach_msg_audit_trailer_t*)(msg.trailer);
				if(trailer->msgh_trailer_size == sizeof(mach_msg_audit_trailer_t)) {
					*pOutPid = (sl_uint32)(trailer->msgh_audit.val[5]);
				} else {
					*pOutPid = 0;
				}
			}
			if (msg.data.address) {
				if (msg.data.size) {
					_out = new KernelMemory(msg.data.address, msg.data.size);
					if (_out.isNotNull()) {
						return sl_true;
					}
				}
				vm_deallocate(mach_task_self(), (vm_address_t)(msg.data.address), (vm_size_t)(msg.data.size));
			}
			return sl_true;
		}
	}
}

#endif
