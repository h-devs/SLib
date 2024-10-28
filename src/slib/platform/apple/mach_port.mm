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

	}
}

#endif
