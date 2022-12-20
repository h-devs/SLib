/*
 *   Copyright (c) 2008-2021 SLIBIO <https://github.com/SLIBIO>
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

#ifndef CHECKHEADER_SLIB_NETWORK_IPC
#define CHECKHEADER_SLIB_NETWORK_IPC

#include "definition.h"

#include "../core/object.h"
#include "../core/function.h"
#include "../core/string.h"
#include "../core/default_members.h"
#include "../io/memory_output.h"

namespace slib
{

	class SLIB_EXPORT IPCParam
	{
	public:
		StringParam name;

		sl_uint32 maxThreadCount;
		sl_uint32 maxReceivingMessageSize;
		sl_uint32 timeout; // milliseconds

		sl_bool flagAcceptOtherUsers;

		Function<void(sl_uint8* data, sl_uint32 size, MemoryOutput* output)> onReceiveMessage;

	public:
		IPCParam();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(IPCParam)

	};

	class SLIB_EXPORT IPC : public Object
	{
		SLIB_DECLARE_OBJECT

	public:
		IPC();

		~IPC();

	public:
		static Ref<IPC> create();

		static Ref<IPC> create(const IPCParam& param);

		static Ref<IPC> createDomainSocket(const IPCParam& param);

	public:
		virtual void sendMessage(const StringParam& targetName, const Memory& data, const Function<void(sl_uint8* data, sl_uint32 size)>& callbackResponse) = 0;

	protected:
		void _init(const IPCParam& param) noexcept;

	protected:
		sl_uint32 m_maxThreadCount;
		sl_uint32 m_maxReceivingMessageSize;
		sl_uint32 m_timeout;
		sl_bool m_flagAcceptOtherUsers;
		Function<void(sl_uint8* data, sl_uint32 size, MemoryOutput* output)> m_onReceiveMessage;

	};

}

#endif
