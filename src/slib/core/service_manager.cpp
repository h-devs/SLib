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

#include "slib/core/service_manager.h"

#include "slib/core/command_line.h"

#if defined(SLIB_PLATFORM_IS_WIN32) || defined(SLIB_PLATFORM_IS_MACOS) || defined(SLIB_PLATFORM_IS_LINUX_DESKTOP)
#define SUPPORT_SERVICE_MANAGER
#endif

namespace slib
{

	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(CreateServiceParam)

	CreateServiceParam::CreateServiceParam() noexcept
	{
		type = ServiceType::Generic;
		flagInteractive = sl_false;
		startType = ServiceStartType::Manual;
		errorControl = ServiceErrorControl::Normal;
	}

	String CreateServiceParam::getCommandLine() const noexcept
	{
		if (commandLine.isNotNull()) {
			return commandLine.toString();
		}
		ListLocker<StringParam> args(arguments);
		return CommandLine::build(path, args.data, args.count);
	}


	sl_bool ServiceManager::checkPath(const CreateServiceParam& param)
	{
		String path = getCommandPath(param.name);
		if (path.isNotNull()) {
			if (path == param.getCommandLine()) {
				return sl_true;
			}
		}
		return sl_false;
	}

	sl_bool ServiceManager::checkPathAndIsRunning(const CreateServiceParam& param)
	{
		if (checkPath(param)) {
			return isRunning(param.name);
		}
		return sl_false;
	}

	sl_bool ServiceManager::checkPathAndCreateAndStart(const CreateServiceParam& param, sl_int32 timeout)
	{
		String path = getCommandPath(param.name);
		if (path.isNotNull()) {
			if (path == param.getCommandLine()) {
				if (isRunning(param.name)) {
					return sl_true;
				} else {
					return start(param.name, timeout);
				}
			}
			stopAndRemove(param.name, timeout);
		}
		if (create(param)) {
			return start(param.name, timeout);
		} else {
			return sl_false;
		}
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

	sl_bool ServiceManager::createAndStart(const CreateServiceParam& param, sl_int32 timeout)
	{
		ServiceState state = getState(param.name);
		if (state == ServiceState::Running) {
			return sl_true;
		}
		if (state != ServiceState::None) {
			return start(param.name);
		}
		if (create(param)) {
			return start(param.name, timeout);
		}
		return sl_false;
	}

	sl_bool ServiceManager::stopAndRemove(const StringParam& serviceName, sl_int32 timeout)
	{
		ServiceState state = getState(serviceName);
		if (state == ServiceState::None) {
			return sl_true;
		}
		if (state != ServiceState::Stopped) {
			if (!(stop(serviceName, timeout))) {
				remove(serviceName);
				return sl_false;
			}
		}
		return remove(serviceName);
	}

#ifndef SUPPORT_SERVICE_MANAGER
	sl_bool ServiceManager::isExisting(const StringParam& name)
	{
		return sl_false;
	}

	sl_bool ServiceManager::create(const CreateServiceParam& param)
	{
		return sl_false;
	}

	sl_bool ServiceManager::remove(const StringParam& name)
	{
		return sl_false;
	}

	ServiceState ServiceManager::getState(const StringParam& name)
	{
		return ServiceState::None;
	}

	sl_bool ServiceManager::start(const StringParam& name, const String16* argv, sl_uint32 argc, sl_int32 timeoutMilliseconds)
	{
		return sl_false;
	}

	sl_bool ServiceManager::stop(const StringParam& name, sl_int32 timeoutMilliseconds)
	{
		return sl_false;
	}

	sl_bool ServiceManager::pause(const StringParam& name, sl_int32 timeoutMilliseconds)
	{
		return sl_false;
	}

	sl_bool ServiceManager::setStartType(const StringParam& serviceName, ServiceStartType type)
	{
		return sl_false;
	}

	ServiceStartType ServiceManager::getStartType(const StringParam& serviceName)
	{
		return ServiceStartType::Unknown;
	}

	String ServiceManager::getCommandPath(const StringParam& serviceName)
	{
		return sl_null;
	}
#endif

}

