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

#ifndef CHECKHEADER_SLIB_PLATFORM_APPLE_MACH_PORT
#define CHECKHEADER_SLIB_PLATFORM_APPLE_MACH_PORT

#include "../definition.h"

#ifdef SLIB_PLATFORM_IS_MACOS

#include <mach/mach.h>

#include "../../core/handle_container.h"
#include "../../core/string.h"
#include "../../core/memory.h"

namespace slib
{

	namespace apple
	{

		class SLIB_EXPORT MachPort
		{
		public:
			SLIB_DECLARE_HANDLE_CONTAINER_MEMBERS(MachPort, mach_port_t, handle, MACH_PORT_NULL)

		public:
			static MachPort create(sl_bool flagSend = sl_false);

			static MachPort lookUp(const StringParam& name);

			static MachPort checkIn(const StringParam& name, sl_bool flagSend = sl_false);

			static sl_bool sendMessage(mach_port_t localPort, mach_port_t remotePort, const void* data, sl_size size, sl_int32 timeout = -1);

			static sl_bool receiveMessage(Memory& _out, mach_port_t localPort, mach_port_t* pOutRemotePort = sl_null, sl_uint32* pOutProcessId = sl_null, sl_int32 timeout = -1);

		};

	}

}

#endif

#endif
