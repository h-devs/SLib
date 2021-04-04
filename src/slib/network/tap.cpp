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

#include "slib/network/tap.h"

namespace slib
{

	SLIB_DEFINE_OBJECT(Tap, Object)

	Tap::Tap()
	{
		m_flagOpened = sl_true;
	}

	Tap::~Tap()
	{
	}

	Ref<Tap> Tap::open()
	{
		return open(sl_null);
	}

	sl_bool Tap::isOpened()
	{
		return m_flagOpened;
	}

	void Tap::close()
	{
		ObjectLocker lock(this);
		if (m_flagOpened) {
			m_flagOpened = sl_false;
			_close();
		}
	}

	String Tap::getDeviceName()
	{
		return m_deviceName;
	}

	String Tap::getInterfaceName()
	{
		return m_interfaceName;
	}

#if defined(SLIB_PLATFORM_IS_MOBILE) || (!defined(SLIB_PLATFORM_IS_WIN32) && !defined(SLIB_PLATFORM_IS_UNIX))
	Ref<Tap> Tap::open(const StringParam& deviceName)
	{
		return sl_null;
	}

	ServiceState Tap::getDriverState()
	{
		return ServiceState::None;
	}

	sl_bool Tap::install()
	{
		return sl_false;
	}

	sl_bool Tap::uninstall()
	{
		return sl_false;
	}
#endif

}
