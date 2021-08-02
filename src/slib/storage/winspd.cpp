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

#include "slib/core/definition.h"

#ifdef SLIB_PLATFORM_IS_WIN32

#include "slib/storage/winspd.h"

#include "winspd/winspd.h"

#include "slib/core/safe_static.h"

#define WINSPD_DRIVER_NAME "WinSpd"

#pragma comment(lib, "winspd.lib")

namespace slib
{

	namespace priv
	{
		namespace winspd
		{

			class BlockDeviceImpl : public BlockDevice
			{
			public:
				static Ref<BlockDeviceImpl> create(const WinspdParam& param)
				{
					return sl_null;
				}

			};

		}
	}

	using namespace priv::winspd;


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(WinspdParam)

	WinspdParam::WinspdParam()
	{
	}


	ServiceState Winspd::getDriverState()
	{
		return ServiceManager::getState(WINSPD_DRIVER_NAME);
	}

	sl_bool Winspd::startDriver()
	{
		ServiceState state = getDriverState();
		if (state == ServiceState::None) {
			return sl_false;
		}
		if (state == ServiceState::Running) {
			return sl_true;
		}
		return ServiceManager::start(WINSPD_DRIVER_NAME);
	}

	sl_bool Winspd::stopDriver()
	{
		ServiceState state = getDriverState();
		if (state == ServiceState::None) {
			return sl_false;
		}
		if (state == ServiceState::Stopped) {
			return sl_true;
		}
		return ServiceManager::stop(WINSPD_DRIVER_NAME);
	}

	Ref<BlockDevice> Winspd::create(const WinspdParam& param)
	{
		return Ref<BlockDevice>::from(BlockDeviceImpl::create(param));
	}

}

#endif
