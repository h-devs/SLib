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

#ifndef CHECKHEADER_SLIB_NETWORK_TAP
#define CHECKHEADER_SLIB_NETWORK_TAP

#include "socket_address.h"

#include "../core/object.h"
#include "../system/service_manager.h"

namespace slib
{

	class SLIB_EXPORT Tap : public Object
	{
		SLIB_DECLARE_OBJECT

	protected:
		Tap();

		~Tap();

	public:
		static Ref<Tap> open(const StringParam& name);

		static Ref<Tap> open();

	public:
		sl_bool isOpened();

		void close();

		String getDeviceName();

		String getInterfaceName();

	public:
		virtual sl_int32 read(void* buf, sl_uint32 size) = 0;

		virtual sl_int32 write(const void* buf, sl_uint32 size) = 0;

		virtual sl_bool setIpAddress(const StringParam& ip, const StringParam& mask) = 0;

	public:
		static ServiceState getDriverState();

		static sl_bool install();

		static sl_bool uninstall();

	protected:
		virtual void _close() = 0;

	protected:
		sl_bool m_flagOpened;
		String m_deviceName;
		String m_interfaceName;

	};

}

#endif