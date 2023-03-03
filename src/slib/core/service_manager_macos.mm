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

#ifdef SLIB_PLATFORM_IS_MACOS

#include "slib/core/service_manager.h"

#include "slib/core/process.h"

namespace slib
{

	namespace {
		static String GetName(const StringParam& _name)
		{
			StringData name(_name);
			if (name.contains('/')) {
				return name.toString(_name);
			} else {
				return String::concat("system/", name);
			}
		}
	}

	sl_bool ServiceManager::isExisting(const StringParam& name)
	{
		String output = Process::getOutput("launchctl", "print", GetName(name));
		return output.contains("state = ");
	}

	sl_bool ServiceManager::create(const CreateServiceParam& param)
	{
		// Not supported
		return sl_false;
	}

	sl_bool ServiceManager::remove(const StringParam& name)
	{
		// Not supported
		return sl_false;
	}

	ServiceState ServiceManager::getState(const StringParam& name)
	{
		String output = Process::getOutput("launchctl", "print", GetName(name));
		sl_reg index = output.indexOf("state = ");
		if (index >= 0) {
			StringView state(output.getData() + index + 8, output.getLength() - index - 8);
			if (state.startsWith("running")) {
				return ServiceState::Running;
			}
			if (state.startsWith("waiting")) {
				return ServiceState::Stopped;
			}
			if (state.startsWith("pending")) {
				return ServiceState::StartPending;
			}
		}
		return ServiceState::None;
	}

	sl_bool ServiceManager::start(const StringParam& name, const String16* argv, sl_uint32 argc, sl_int32 timeoutMilliseconds)
	{
		String output = Process::getOutput("launchctl", "kickstart", GetName(name));
		return output.trim().isEmpty();
	}

	sl_bool ServiceManager::stop(const StringParam& name, sl_int32 timeoutMilliseconds)
	{
		String output = Process::getOutput("launchctl", "kill", "SIGTERM", GetName(name));
		return output.trim().isEmpty();
	}

	sl_bool ServiceManager::pause(const StringParam& name, sl_int32 timeoutMilliseconds)
	{
		// Not supported
		return sl_false;
	}

	sl_bool ServiceManager::setStartType(const StringParam& serviceName, ServiceStartType type)
	{
		// Not supported
		return sl_false;
	}

	ServiceStartType ServiceManager::getStartType(const StringParam& serviceName)
	{
		return ServiceStartType::Unknown;
	}

}

#endif
