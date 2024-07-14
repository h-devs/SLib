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

#include "slib/core/definition.h"

#ifdef SLIB_PLATFORM_IS_LINUX_DESKTOP

#include "slib/system/service_manager.h"

#include "slib/system/system.h"
#include "slib/system/process.h"
#include "slib/core/string_buffer.h"
#include "slib/io/file.h"

namespace slib
{

	namespace {

		static sl_bool WaitState(const StringParam& name, ServiceState state, sl_int32 timeout)
		{
			sl_uint32 n = 0;
			if (timeout > 0) {
				n = timeout / 100;
				if (n < 5) {
					n = 5;
				}
				if (n > 50) {
					n = 50;
				}
			} else {
				n = 50;
			}
			for (sl_uint32 i = 0; i < n; i++) {
				if (ServiceManager::getState(name) == state) {
					return sl_true;
				}
				System::sleep(100);
			}
			return sl_false;
		}

		static String GetUnitFilePath(const StringParam& serviceName)
		{
			return String::concat(StringView::literal("/etc/systemd/system/"), serviceName, StringView::literal(".service"));
		}

	}

	sl_bool ServiceManager::isExisting(const StringParam& name)
	{
		return getState(name) != ServiceState::None;
	}

	sl_bool ServiceManager::create(const CreateServiceParam& param)
	{
		if (!(Process::isCurrentProcessAdmin())) {
			return sl_false;
		}
		StringBuffer sb;
		sb.addStatic("[Unit]\nDescription=");
		sb.add(param.displayName.toString());
		sb.addStatic("\n\n[Service]\nType=simple\nUser=root\nGroup=root\nWorkingDirectory=");
		String workingDir = File::getParentDirectoryPath(param.path);
		if (workingDir.isNotEmpty()) {
			sb.add(workingDir);
		} else {
			sb.add(System::getHomeDirectory());
		}
		sb.addStatic("\nExecStart=");
		sb.add(param.getCommandLine());
		sb.addStatic("\nRestart=always\n\n[Install]\nWantedBy=multi-user.target");
		File::writeAllTextUTF8(GetUnitFilePath(param.name), sb.merge());
		System::execute("systemctl daemon-reload");
		if (param.startType == ServiceStartType::Auto) {
			System::execute(String::concat(StringView::literal("systemctl enable "), param.name));
		}
		return isExisting(param.name);
	}

	sl_bool ServiceManager::remove(const StringParam& name)
	{
		if (!(Process::isCurrentProcessAdmin())) {
			return sl_false;
		}
		if (getState(name) == ServiceState::Running) {
			stop(name);
		}
		String path = GetUnitFilePath(name);
		File::deleteFile(path);
		System::execute("systemctl daemon-reload");
		return WaitState(name, ServiceState::None, 1000);
	}

	ServiceState ServiceManager::getState(const StringParam& name)
	{
		String output = System::getCommandOutput(String::concat(StringView::literal("systemctl status "), name));
		sl_reg index = output.indexOf("Active: ");
		if (index >= 0) {
			StringView state(output.getData() + index + 8, output.getLength() - index - 8);
			if (state.startsWith("active")) {
				return ServiceState::Running;
			} else if (state.startsWith("inactive")) {
				return ServiceState::Stopped;
			} else if (output.contains("Loaded: loaded")) {
				return ServiceState::Loaded;
			}
		}
		return ServiceState::None;
	}

	sl_bool ServiceManager::start(const StringParam& name, const String16* argv, sl_uint32 argc, sl_int32 timeoutMilliseconds)
	{
		if (!(Process::isCurrentProcessAdmin())) {
			return sl_false;
		}
		System::execute(String::concat(StringView::literal("systemctl start "), name));
		return WaitState(name, ServiceState::Running, timeoutMilliseconds);
	}

	sl_bool ServiceManager::stop(const StringParam& name, sl_int32 timeoutMilliseconds)
	{
		if (!(Process::isCurrentProcessAdmin())) {
			return sl_false;
		}
		System::execute(String::concat(StringView::literal("systemctl stop "), name));
		return WaitState(name, ServiceState::Stopped, timeoutMilliseconds);
	}

	sl_bool ServiceManager::pause(const StringParam& name, sl_int32 timeoutMilliseconds)
	{
		// Not supported
		return sl_false;
	}

	sl_bool ServiceManager::setStartType(const StringParam& serviceName, ServiceStartType type)
	{
		if (!(Process::isCurrentProcessAdmin())) {
			return sl_false;
		}
		String output;
		if (type == ServiceStartType::Auto) {
			output = System::getCommandOutput(String::concat(StringView::literal("systemctl enable "), serviceName));
		} else {
			output = System::getCommandOutput(String::concat(StringView::literal("systemctl disable"), serviceName));
		}
		return output.contains(StringView::literal("Synchronizing state"));
	}

	ServiceStartType ServiceManager::getStartType(const StringParam& serviceName)
	{
		return ServiceStartType::Unknown;
	}

	String ServiceManager::getCommandPath(const StringParam& serviceName)
	{
		// Not supported
		return sl_null;
	}

}

#endif
