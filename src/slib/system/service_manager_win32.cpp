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

#ifdef SLIB_PLATFORM_IS_WIN32

#include "slib/system/service_manager.h"

#include "slib/core/thread.h"
#include "slib/core/time_counter.h"
#include "slib/core/memory.h"
#include "slib/platform/win32/windows.h"

#pragma comment(lib, "advapi32.lib")

namespace slib
{

	namespace {

		static DWORD FromServiceType(ServiceType type, sl_bool flagInteractive)
		{
			DWORD ret = 0;
			switch (type) {
				case ServiceType::Driver:
					return SERVICE_KERNEL_DRIVER;
				case ServiceType::FileSystem:
					return SERVICE_FILE_SYSTEM_DRIVER;
				case ServiceType::Shared:
					ret = SERVICE_WIN32_SHARE_PROCESS;
					break;
				default:
					ret = SERVICE_WIN32_OWN_PROCESS;
					break;
			}
			if (flagInteractive) {
				return ret | SERVICE_INTERACTIVE_PROCESS;
			} else {
				return ret;
			}
		}

		static DWORD FromServiceStartType(ServiceStartType type)
		{
			switch (type) {
				case ServiceStartType::Auto:
					return SERVICE_AUTO_START;
				case ServiceStartType::Boot:
					return SERVICE_BOOT_START;
				case ServiceStartType::Disabled:
					return SERVICE_DISABLED;
				case ServiceStartType::System:
					return SERVICE_SYSTEM_START;
				default:
					break;
			}
			return SERVICE_DEMAND_START;
		}

		static ServiceStartType ToServiceStartType(DWORD type)
		{
			switch (type) {
				case SERVICE_DEMAND_START:
					return ServiceStartType::Manual;
				case SERVICE_AUTO_START:
					return ServiceStartType::Auto;
				case SERVICE_BOOT_START:
					return ServiceStartType::Boot;
				case SERVICE_DISABLED:
					return ServiceStartType::Disabled;
				case SERVICE_SYSTEM_START:
					return ServiceStartType::System;
				default:
					break;
			}
			return ServiceStartType::Unknown;
		}

		static DWORD FromServiceErrorControl(ServiceErrorControl control)
		{
			switch (control) {
				case ServiceErrorControl::Ignore:
					return SERVICE_ERROR_IGNORE;
				case ServiceErrorControl::Critical:
					return SERVICE_ERROR_CRITICAL;
				case ServiceErrorControl::Severe:
					return SERVICE_ERROR_SEVERE;
				default:
					break;
			}
			return SERVICE_ERROR_NORMAL;
		}

		static ServiceState ToServiceState(DWORD state)
		{
			switch (state) {
				case SERVICE_RUNNING:
					return ServiceState::Running;
				case SERVICE_STOPPED:
					return ServiceState::Stopped;
				case SERVICE_PAUSED:
					return ServiceState::Paused;
				case SERVICE_START_PENDING:
					return ServiceState::StartPending;
				case SERVICE_STOP_PENDING:
					return ServiceState::StopPending;
				case SERVICE_PAUSE_PENDING:
					return ServiceState::PausePending;
				case SERVICE_CONTINUE_PENDING:
					return ServiceState::ContinuePending;
			}
			return ServiceState::None;
		}

		class WSManager
		{
		public:
			SC_HANDLE handle;

		public:
			WSManager(DWORD dwAccess)
			{
				handle = OpenSCManagerW(NULL, NULL, dwAccess);
			}

			~WSManager()
			{
				if (handle) {
					CloseServiceHandle(handle);
				}
			}

		public:
			operator SC_HANDLE() const
			{
				return handle;
			}

		};

		class WSService
		{
		public:
			SC_HANDLE handle;

		public:
			WSService(const WSManager& manager, const StringParam& _name, DWORD dwAccess)
			{
				StringCstr16 name(_name);
				handle = OpenServiceW(manager, (LPCWSTR)(name.getData()), dwAccess);
			}

			~WSService()
			{
				if (handle) {
					CloseServiceHandle(handle);
				}
			}

		public:
			operator SC_HANDLE() const
			{
				return handle;
			}

		public:
			DWORD getCurrentState()
			{
				SERVICE_STATUS_PROCESS status;
				DWORD dwBytes = 0;
				if (QueryServiceStatusEx(handle, SC_STATUS_PROCESS_INFO, (LPBYTE)&status, sizeof(status), &dwBytes)) {
					return status.dwCurrentState;
				}
				return 0;
			}

		};

		static QUERY_SERVICE_CONFIGW* GetServiceConfig(const StringParam& serviceName, Memory& out)
		{
			WSManager manager(GENERIC_READ);
			if (manager) {
				StringCstr16 name(serviceName);
				WSService service(manager, name, GENERIC_READ);
				if (service) {
					DWORD dwBytes = 0;
					QueryServiceConfigW(service, NULL, 0, &dwBytes);
					if (dwBytes) {
						Memory mem = Memory::create(dwBytes);
						if (mem.isNotNull()) {
							QUERY_SERVICE_CONFIGW& config = *((QUERY_SERVICE_CONFIGW*)(mem.getData()));
							if (QueryServiceConfigW(service, &config, dwBytes, &dwBytes)) {
								out = Move(mem);
								return &config;
							}
						}
					}
				}
			}
			return sl_null;
		}

	}

	sl_bool ServiceManager::isExisting(const StringParam& name)
	{
		WSManager manager(GENERIC_READ);
		if (manager) {
			WSService service(manager, name, GENERIC_READ);
			if (service) {
				return sl_true;
			}
		}
		return sl_false;
	}

	sl_bool ServiceManager::create(const CreateServiceParam& param)
	{
		WSManager manager(SC_MANAGER_CREATE_SERVICE);
		if (manager) {
			StringCstr16 name(param.name);
			StringParam _displayName = param.displayName;
			if (_displayName.isNull()) {
				_displayName = name;
			}
			StringCstr16 displayName(_displayName);
			StringCstr16 path(param.getCommandLine());
			SC_HANDLE handle = CreateServiceW(
				manager,
				(LPCWSTR)(name.getData()),
				(LPCWSTR)(displayName.getData()),
				param.description.isNotNull() ? SERVICE_CHANGE_CONFIG : 0,
				FromServiceType(param.type, param.flagInteractive),
				FromServiceStartType(param.startType),
				FromServiceErrorControl(param.errorControl),
				(LPCWSTR)(path.getData()),
				NULL, NULL, NULL, NULL, NULL
				);
			if (handle) {
				if (param.description.isNotNull()) {
					StringCstr16 description(param.description);
					SERVICE_DESCRIPTIONW sd;
					sd.lpDescription = (LPWSTR)(description.getData());
					ChangeServiceConfig2W(handle, SERVICE_CONFIG_DESCRIPTION, &sd);
				}
				CloseServiceHandle(handle);
				return sl_true;
			}
		}
		return sl_false;
	}

	sl_bool ServiceManager::remove(const StringParam& name)
	{
		WSManager manager(SC_MANAGER_ALL_ACCESS);
		if (manager) {
			WSService service(manager, name, DELETE);
			if (service) {
				if (DeleteService(service)) {
					return sl_true;
				}
			}
		}
		return sl_false;
	}

	ServiceState ServiceManager::getState(const StringParam& name)
	{
		WSManager manager(GENERIC_READ);
		if (manager) {
			WSService service(manager, name, GENERIC_READ);
			if (service) {
				return ToServiceState(service.getCurrentState());
			}
		}
		return ServiceState::None;
	}

	sl_bool ServiceManager::start(const StringParam& name, const String16* argv, sl_uint32 argc, sl_int32 timeoutMilliseconds)
	{
		WSManager manager(GENERIC_READ | SC_MANAGER_CONNECT);
		if (manager) {
			WSService service(manager, name, GENERIC_READ | SERVICE_START | SERVICE_PAUSE_CONTINUE);
			if (service) {
				TimeCounter timer;
				for (;;) {
					DWORD state = service.getCurrentState();
					if (!state) {
						return sl_false;
					}
					if (state == SERVICE_RUNNING) {
						return sl_true;
					}
					BOOL bRet = TRUE;
					if (state == SERVICE_PAUSED) {
						SERVICE_STATUS statusReturned;
						bRet = ControlService(service, SERVICE_CONTROL_CONTINUE, &statusReturned);
					} else if (state == SERVICE_STOPPED) {
						if (argc) {
							StringCstr16 argName(name);
							LPCWSTR args[64];
							StringCstr16 _args[60];
							if (argc > 60) {
								argc = 60;
							}
							args[0] = (LPCWSTR)(argName.getData());
							for (sl_size i = 0; i < argc; i++) {
								_args[i] = argv[i];
								args[i + 1] = (LPCWSTR)(_args[i].getData());
							}
							args[argc + 1] = 0;
							bRet = StartServiceW(service, (DWORD)(argc + 1), args);
						} else {
							bRet = StartServiceW(service, 0, NULL);
						}
					}
					if (!bRet) {
						DWORD dwError = GetLastError();
						if (dwError != ERROR_SERVICE_REQUEST_TIMEOUT) {
							return sl_false;
						}
					}
					Thread::sleep(10);
					if (timeoutMilliseconds >= 0) {
						if (timer.getElapsedMilliseconds() > (sl_uint32)timeoutMilliseconds) {
							return sl_false;
						}
					}
				}
			}
		}
		return sl_false;
	}

	sl_bool ServiceManager::stop(const StringParam& name, sl_int32 timeoutMilliseconds)
	{
		WSManager manager(GENERIC_READ | SC_MANAGER_CONNECT);
		if (manager) {
			WSService service(manager, name, GENERIC_READ | SERVICE_STOP);
			if (service) {
				TimeCounter timer;
				for (;;) {
					DWORD state = service.getCurrentState();
					if (!state) {
						return sl_false;
					}
					if (state == SERVICE_STOPPED) {
						return sl_true;
					}
					BOOL bRet = TRUE;
					if (state == SERVICE_RUNNING || state == SERVICE_PAUSED) {
						SERVICE_STATUS statusReturned;
						bRet = ControlService(service, SERVICE_CONTROL_STOP, &statusReturned);
					}
					if (!bRet) {
						DWORD dwError = GetLastError();
						if (dwError != ERROR_SERVICE_REQUEST_TIMEOUT) {
							return sl_false;
						}
					}
					Thread::sleep(10);
					if (timeoutMilliseconds >= 0) {
						if (timer.getElapsedMilliseconds() > (sl_uint32)timeoutMilliseconds) {
							return sl_false;
						}
					}
				}
			}
		}
		return sl_false;
	}

	sl_bool ServiceManager::pause(const StringParam& name, sl_int32 timeoutMilliseconds)
	{
		WSManager manager(GENERIC_READ | SC_MANAGER_CONNECT);
		if (manager) {
			WSService service(manager, name, GENERIC_READ | SERVICE_STOP);
			if (service) {
				TimeCounter timer;
				for (;;) {
					DWORD state = service.getCurrentState();
					if (!state) {
						return sl_false;
					}
					if (state == SERVICE_STOPPED) {
						return sl_false;
					}
					if (state == SERVICE_PAUSED) {
						return sl_true;
					}
					BOOL bRet = TRUE;
					if (state == SERVICE_RUNNING) {
						SERVICE_STATUS statusReturned;
						bRet = ControlService(service, SERVICE_CONTROL_PAUSE, &statusReturned);
					}
					if (!bRet) {
						DWORD dwError = GetLastError();
						if (dwError != ERROR_SERVICE_REQUEST_TIMEOUT) {
							return sl_false;
						}
					}
					Thread::sleep(10);
					if (timeoutMilliseconds >= 0) {
						if (timer.getElapsedMilliseconds() > (sl_uint32)timeoutMilliseconds) {
							return sl_false;
						}
					}
				}
			}
		}
		return sl_false;
	}

	sl_bool ServiceManager::setStartType(const StringParam& serviceName, ServiceStartType type)
	{
		WSManager manager(GENERIC_READ | GENERIC_WRITE | SC_MANAGER_CONNECT);
		if (manager) {
			WSService service(manager, serviceName, SERVICE_CHANGE_CONFIG);
			if (service) {
				if (ChangeServiceConfigW(service, SERVICE_NO_CHANGE, FromServiceStartType(type), SERVICE_NO_CHANGE, NULL, NULL, NULL, NULL, NULL, NULL, NULL)) {
					return sl_true;
				}
			}
		}
		return sl_false;
	}

	ServiceStartType ServiceManager::getStartType(const StringParam& serviceName)
	{
		Memory mem;
		QUERY_SERVICE_CONFIGW* config = GetServiceConfig(serviceName, mem);
		if (config) {
			return ToServiceStartType(config->dwStartType);
		}
		return ServiceStartType::Unknown;
	}

	String ServiceManager::getCommandPath(const StringParam& serviceName)
	{
		Memory mem;
		QUERY_SERVICE_CONFIGW* config = GetServiceConfig(serviceName, mem);
		if (config) {
			return String::from(config->lpBinaryPathName);
		}
		return sl_null;
	}

}

#endif
