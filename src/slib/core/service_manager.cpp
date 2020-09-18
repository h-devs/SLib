/*
 *   Copyright (c) 2008-2020 SLIBIO <https://github.com/SLIBIO>
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

#include "slib/core/service_manager.h"

namespace slib
{

	ServiceCreateParam::ServiceCreateParam()
	{
		type = ServiceType::Generic;
		startType = ServiceStartType::Manual;
		errorControl = ServiceErrorControl::Normal;
	}

	ServiceCreateParam::~ServiceCreateParam()
	{
	}

	sl_bool ServiceManager::isStarted(const StringParam& name)
	{
		ServiceState state = getState(name);
		return state == ServiceState::Running || state == ServiceState::Paused || state == ServiceState::PausePending || state == ServiceState::ContinuePending;
	}

	sl_bool ServiceManager::isRunning(const StringParam& name)
	{
		return getState(name) == ServiceState::Running;
	}

	sl_bool ServiceManager::isStopped(const StringParam& name)
	{
		return getState(name) == ServiceState::Stopped;
	}

	sl_bool ServiceManager::isPaused(const StringParam& name)
	{
		return getState(name) == ServiceState::Paused;
	}

	sl_bool ServiceManager::start(const StringParam& name, sl_int32 timeoutMilliseconds)
	{
		return start(name, sl_null, 0, timeoutMilliseconds);
	}

	sl_bool ServiceManager::createAndStart(const ServiceCreateParam& param)
	{
		ServiceState state = getState(param.name);
		if (state == ServiceState::Running) {
			return sl_true;
		}
		if (state != ServiceState::None) {
			return start(param.name);
		}
		if (create(param)) {
			return start(param.name);
		}
		return sl_false;
	}

}
