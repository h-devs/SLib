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

#include "slib/core/service.h"

#include "slib/core/service_manager.h"
#include "slib/core/named_instance.h"
#include "slib/core/system.h"
#include "slib/core/process.h"
#include "slib/core/log.h"

#ifndef SLIB_PLATFORM_IS_WINDOWS
#include <signal.h>
#endif

#define TAG "Service"
#define WAIT_SECONDS 300

namespace slib
{

	namespace priv
	{
		namespace service
		{

			static void TermHandler(int signum)
			{
				Service::quitApp();
			}

		}
	}

	using namespace priv::service;

	SLIB_DEFINE_OBJECT(Service, Object)

	Service::Service()
	{
		m_flagPlatformService = sl_false;
		m_eventQuit = Event::create();
		m_flagRequestQuit = sl_false;
	}

	Service::~Service()
	{
	}

	AppType Service::getAppType()
	{
		return AppType::Service;
	}

	Ref<Service> Service::getApp()
	{
		Ref<Application> app = Application::getApp();
		if (app.isNotNull() && app->getAppType() == AppType::Service) {
			return Ref<Service>::from(app);
		}
		return sl_null;
	}

	void Service::quitApp()
	{
		Ref<Service> service = getApp();
		if (service.isNotNull()) {
			service->quit();
		}
	}

	void Service::quit()
	{
		m_flagRequestQuit = sl_true;
		if (m_eventQuit.isNotNull()) {
			m_eventQuit->set();
		}
	}

	sl_bool Service::dispatchStartService()
	{
		return onStartService();
	}
	
	void Service::dispatchStopService()
	{
		onStopService();
	}

#define START_ID "_STARTED"
#define STOP_ID "_STOPPING"

	sl_bool Service::startService()
	{
		String appName = getServiceId();
		if (appName.isEmpty()) {
			LogError(TAG, "SERVICE NAME IS EMPTY");
			return sl_false;
		}
		
		if (NamedInstance::exists(appName + STOP_ID)) {
			LogError(TAG, "OTHER PROCESS IS STOPPING %s", appName);
			return sl_false;
		}
	
		if (!(isUniqueInstanceRunning())) {

			Log(TAG, "STARING %s", appName);

			String appPath = System::getApplicationPath();
			Ref<Process> process = Process::run(appPath);

			for (int i = 0; i < WAIT_SECONDS*10; i++) {
				if (NamedInstance::exists(appName + START_ID)) {
					Log(TAG, "%s IS STARTED", appName);
					return sl_true;
				}
				System::sleep(100);
				if (!(process->isAlive())) {
					break;
				}
			}
			LogError(TAG, "%s IS NOT STARTED", appName);
		} else {
			LogError(TAG, "%s IS ALREADY RUNNING", appName);
		}
		return sl_false;
	}

	sl_bool Service::stopService()
	{
		String appName = getServiceId();

		if (!(isUniqueInstanceRunning())) {
			LogError(TAG, "%s IS NOT RUNNING", appName);
		} else {
			NamedInstance stopInstance = NamedInstance(appName + STOP_ID);
			if (stopInstance.isNotNone()) {
				Log(TAG, "STOPPING %s", appName);
				for (int i = 0; i < WAIT_SECONDS * 10; i++) {
					if (!(isUniqueInstanceRunning())) {
						Log(TAG, "%s IS STOPPED", appName);
						return sl_true;
					}
					System::sleep(100);
				}
				LogError(TAG, "%s IS NOT STOPPED", appName);
			} else {
				LogError(TAG, "OTHER PROCESS IS STOPPING %s", appName);
			}
		}
		return sl_false;
	}

	void Service::statusService()
	{
		String appName = getServiceId();
		if (isUniqueInstanceRunning()) {
			Log(TAG, "%s IS RUNNING", appName);
		} else {
			Log(TAG, "%s IS NOT RUNNING", appName);
		}
	}

	sl_int32 Service::runService()
	{
		return Application::doRun();
	}

	sl_bool Service::onStartService()
	{
		return sl_true;
	}
	
	void Service::onStopService()
	{
	}
	
	sl_int32 Service::doRun()
	{
		if (_tryPlatformService()) {
			return 0;
		}
#if defined(SLIB_PLATFORM_IS_MOBILE)
		Log(TAG, "Can not run on mobile platforms");
#else
		List<String> arguments = getArguments();
		if (arguments.contains("service")) {
			String name = getServiceId();
			if (name.isEmpty()) {
				LogError(TAG, "SERVICE NAME IS EMPTY");
				return -1;
			}
			ServiceState state = ServiceManager::getState(name);
			if (arguments.contains("status")) {
				switch (state) {
				case ServiceState::None:
					Log(TAG, "Not Installed");
					break;
				case ServiceState::Running:
					Log(TAG, "Running");
					break;
				case ServiceState::Paused:
					Log(TAG, "Paused");
					break;
				case ServiceState::Stopped:
					Log(TAG, "Stopped");
					break;
				case ServiceState::StartPending:
					Log(TAG, "StartPending");
					break;
				case ServiceState::PausePending:
					Log(TAG, "PausePending");
					break;
				case ServiceState::StopPending:
					Log(TAG, "StopPending");
					break;
				case ServiceState::ContinuePending:
					Log(TAG, "ContinuePending");
					break;
				default:
					Log(TAG, "Unknown");
					break;
				}
				return 0;
			}
			if (!(Process::isCurrentProcessAdmin())) {
				if (arguments.contains("admin")) {
					List<StringParam> args;
					args.addAll_NoLock(getArguments());
					Process::runAsAdmin(getApplicationPath(), args.getData(), args.getCount());
					return 0;
				} else {
					Log(TAG, "RUN AS ADMIN!");
					return -1;
				}
			}
			if (arguments.contains("install") || arguments.contains("reinstall")) {
				if (state != ServiceState::None) {
					if (arguments.contains("reinstall")) {
						Log(TAG, "UNINSTALLING SERVICE: %s", name);
						if (ServiceManager::stopAndRemove(name)) {
							Log(TAG, "UNINSTALLED SERVICE: %s", name);
						} else {
							Log(TAG, "FAILED TO UNINSTALL SERVICE: %s", name);
							return -1;
						}
					} else {
						Log(TAG, "SERVICE IS ALREADY INSTALLED: %s", name);
						return 0;
					}
				}
				Log(TAG, "INSTALLING SERVICE: %s", name);
				ServiceCreateParam param;
				param.name = name;
				param.path = getApplicationPath();
				if (ServiceManager::create(param)) {
					Log(TAG, "INSTALLED SERVICE: %s", name);
					return 0;
				} else {
					Log(TAG, "FAILED TO INSTALL SERVICE: %s", name);
					return -1;
				}
			}
			if (state == ServiceState::None) {
				Log(TAG, "SERVICE IS NOT INSTALLED: %s", name);
				return -1;
			}
			if (arguments.contains("uninstall")) {
				Log(TAG, "UNINSTALLING SERVICE: %s", name);
				if (ServiceManager::stopAndRemove(name)) {
					Log(TAG, "UNINSTALLED SERVICE: %s", name);
					return 0;
				} else {
					Log(TAG, "FAILED TO UNINSTALL SERVICE: %s", name);
					return -1;
				}
			} else if (arguments.contains("start")) {
				if (state == ServiceState::Running) {
					Log(TAG, "ALREADY RUNNING SERVICE: %s", name);
					return 0;
				}
				Log(TAG, "STARTING SERVICE: %s", name);
				if (ServiceManager::start(name)) {
					Log(TAG, "STARTED SERVICE: %s", name);
					return 0;
				} else {
					Log(TAG, "FAILED TO START SERVICE: %s", name);
					return -1;
				}
			} else if (arguments.contains("stop")) {
				if (state == ServiceState::Stopped) {
					Log(TAG, "ALREADY STOPPED SERVICE: %s", name);
					return 0;
				}
				Log(TAG, "STOPPING SERVICE: %s", name);
				if (ServiceManager::stop(name)) {
					Log(TAG, "STOPPED SERVICE: %s", name);
					return 0;
				} else {
					Log(TAG, "FAILED TO STOP SERVICE: %s", name);
					return -1;
				}
			} else if (arguments.contains("restart")) {
				if (state != ServiceState::Stopped) {
					Log(TAG, "STOPPING SERVICE: %s", name);
					if (ServiceManager::stop(name)) {
						Log(TAG, "STOPPED SERVICE: %s", name);
					} else {
						Log(TAG, "FAILED TO STOP SERVICE: %s", name);
						return -1;
					}
				}
				Log(TAG, "STARTING SERVICE: %s", name);
				if (ServiceManager::start(name)) {
					Log(TAG, "STARTED SERVICE: %s", name);
					return 0;
				}
			}
		} else {
			if (arguments.contains("start")) {
				if (startService()) {
					return 0;
				}
			} else if (arguments.contains("stop")) {
				if (stopService()) {
					return 0;
				}
			} else if (arguments.contains("restart")) {
				stopService();
				if (startService()) {
					return 0;
				}
			} else if (arguments.contains("status")) {
				statusService();
				return 0;
			} else {
				return runService();
			}
		}
#endif
		return -1;
	}
	
	sl_int32 Service::onRunApp()
	{
		if (m_flagPlatformService) {
			_runPlatformService();
			return 0;
		}
		
		if (m_eventQuit.isNull()) {
			return -1;
		}
		
#ifndef SLIB_PLATFORM_IS_WINDOWS
		struct sigaction sa;
		Base::zeroMemory(&sa, sizeof(sa));
		sa.sa_handler = &TermHandler;
		sigaction(SIGTERM, &sa, sl_null);
#endif
		
		String appName = getServiceId();

		if (!(dispatchStartService())) {
			dispatchStopService();
			return -1;
		}

		NamedInstance startInstance(appName + START_ID);

		String stopId = appName + STOP_ID;
		
		while (!m_flagRequestQuit) {
			if (NamedInstance::exists(stopId)) {
				break;
			}
			m_eventQuit->wait(500);
		}

		dispatchStopService();

		return 0;

	}
	
	String Service::getApplicationId()
	{
		return getServiceId();
	}

#if !defined(SLIB_PLATFORM_IS_WIN32)
	sl_bool Service::_tryPlatformService()
	{
		return sl_false;
	}

	void Service::_runPlatformService()
	{
	}
#endif

}
