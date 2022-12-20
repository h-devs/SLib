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

#ifdef SLIB_PLATFORM_IS_WIN32

#include "slib/core/service.h"

#include "slib/core/event.h"
#include "slib/core/safe_static.h"
#include "slib/platform/win32/windows.h"

#pragma comment(lib, "advapi32.lib")

namespace slib
{

	namespace priv
	{
		namespace service
		{

			class ServiceHelper : public Service
			{
			public:
				sl_bool _tryService();

				void _doRun();

				void _run();

				void _control(DWORD fdwControl);

			};

			SLIB_GLOBAL_ZERO_INITIALIZED(Ref<ServiceHelper>, g_servicePlatform)
			SLIB_GLOBAL_ZERO_INITIALIZED(Ref<Event>, g_eventStop)

			sl_bool g_flagStopService = sl_false;

			SERVICE_STATUS_HANDLE g_hServiceStatus = NULL;
			SERVICE_STATUS g_statusService;

			static void ReportServiceStatus(DWORD dwCurrentState, DWORD dwExitCode, DWORD dwWaitHint)
			{
				static DWORD dwCheckPoint = 1;

				g_statusService.dwCurrentState = dwCurrentState;
				g_statusService.dwWin32ExitCode = dwExitCode;
				g_statusService.dwWaitHint = dwWaitHint;

				if (dwCurrentState == SERVICE_START_PENDING) {
					g_statusService.dwControlsAccepted = 0;
				} else {
					g_statusService.dwControlsAccepted = SERVICE_ACCEPT_STOP;
				}
				if (dwCurrentState == SERVICE_RUNNING || dwCurrentState == SERVICE_STOPPED) {
					g_statusService.dwCheckPoint = 0;
				} else {
					g_statusService.dwCheckPoint = dwCheckPoint++;
				}
				SetServiceStatus(g_hServiceStatus, &g_statusService);
			}

			static void WINAPI ServiceHandler(DWORD fdwControl)
			{
				if (SLIB_SAFE_STATIC_CHECK_FREED(g_servicePlatform)) {
					return;
				}
				if (SLIB_SAFE_STATIC_CHECK_FREED(g_eventStop)) {
					return;
				}
				if (g_servicePlatform.isNull() || g_eventStop.isNull()) {
					return;
				}
				g_servicePlatform->_control(fdwControl);
			}

			static void WINAPI ServiceMain(DWORD dwArgc, LPTSTR* lpszArgv)
			{
				if (SLIB_SAFE_STATIC_CHECK_FREED(g_servicePlatform)) {
					return;
				}
				if (g_servicePlatform.isNull()) {
					return;
				}

				g_eventStop = Event::create();

				g_hServiceStatus = RegisterServiceCtrlHandlerW(L"", &ServiceHandler);
				if (!g_hServiceStatus) {
					return;
				}

				if (g_eventStop.isNull()) {
					ReportServiceStatus(SERVICE_STOPPED, NO_ERROR, 0);
					return;
				}

				g_statusService.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
				g_statusService.dwServiceSpecificExitCode = 0;
				ReportServiceStatus(SERVICE_START_PENDING, NOERROR, 30000);

				g_servicePlatform->_doRun();
			}

			sl_bool ServiceHelper::_tryService()
			{
				{
					SLIB_STATIC_SPINLOCKER(lock)
					if (g_servicePlatform.isNotNull()) {
						return sl_false;
					}
					g_servicePlatform = (ServiceHelper*)this;
				}

				SERVICE_TABLE_ENTRYW table[] = {
					{ L"", &ServiceMain },
					{ NULL, NULL }
				};
				if (StartServiceCtrlDispatcherW(table)) {
					return sl_true;
				} else {
					DWORD dwError = GetLastError();
					if (dwError != ERROR_FAILED_SERVICE_CONTROLLER_CONNECT) {
						return sl_true;
					} else {
						// This app is generic console application. It is not started from service control manager.
						return sl_false;
					}
				}
			}

			void ServiceHelper::_doRun()
			{
				setCrashRecoverySupport(sl_false);
				m_flagPlatformService = sl_true;
				_initApp();
				doRun();
			}

			void ServiceHelper::_run()
			{
				if (!(dispatchStartService())) {
					ReportServiceStatus(SERVICE_STOP_PENDING, NO_ERROR, 0);
					dispatchStopService();
					ReportServiceStatus(SERVICE_STOPPED, NO_ERROR, 0);
					return;
				}
				ReportServiceStatus(SERVICE_RUNNING, NO_ERROR, 0);

				while (!g_flagStopService) {
					g_eventStop->wait(1000);
				}

				dispatchStopService();
				ReportServiceStatus(SERVICE_STOPPED, NO_ERROR, 0);
			}

			void ServiceHelper::_control(DWORD fdwControl)
			{
				switch (fdwControl) {
				case SERVICE_CONTROL_STOP:
					ReportServiceStatus(SERVICE_STOP_PENDING, NO_ERROR, 0);
					g_flagStopService = sl_true;
					g_eventStop->set();
					break;
				}
			}

		}
	}

	using namespace priv::service;

	sl_bool Service::_tryPlatformService()
	{
		return ((ServiceHelper*)this)->_tryService();
	}

	void Service::_runPlatformService()
	{
		((ServiceHelper*)this)->_run();
	}

}

#endif