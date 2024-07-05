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

#include "input_sender.h"

#include <slib/system/service.h>
#include <slib/system/service_manager.h>
#include <slib/system/process.h>
#include <slib/system/system.h>
#include <slib/core/thread.h>
#include <slib/core/mio.h>
#include <slib/core/command_line.h>
#include <slib/core/safe_static.h>
#include <slib/network/ipc.h>
#include <slib/ui/core.h>
#include <slib/platform.h>

#define SERVICE_COMMAND "daemon"
#define AGENT_COMMAND "agent"
#define INSTALL_COMMAND "install"

#define IPC_CMD_QUIT 0x01
#define IPC_CMD_SEND_KEY 0x02
#define IPC_CMD_SEND_MOUSE_RELATIVE 0x03
#define IPC_CMD_SEND_MOUSE_ABSOLUTE 0x04

namespace slib
{

	namespace
	{

		SLIB_GLOBAL_ZERO_INITIALIZED(AtomicString, g_serviceName)
		SLIB_GLOBAL_ZERO_INITIALIZED(AtomicFunction<void()>, g_onStartService)
		SLIB_GLOBAL_ZERO_INITIALIZED(AtomicFunction<void()>, g_onStopService)
		SLIB_GLOBAL_ZERO_INITIALIZED(AtomicFunction<void()>, g_onInstallService)
		SLIB_GLOBAL_ZERO_INITIALIZED(AtomicFunction<sl_bool()>, g_onCheckInstall)
		SLIB_GLOBAL_ZERO_INITIALIZED(AtomicFunction<void()>, g_onStartAgent)
		SLIB_GLOBAL_ZERO_INITIALIZED(AtomicFunction<void()>, g_onStopAgent)

		static void SendAgentMessage(sl_uint8 uMsg, sl_uint32 param1, sl_uint32 param2, sl_uint32 param3)
		{
			sl_uint8 buf[13];
			*buf = uMsg;
			MIO::writeUint32(buf + 1, param1);
			MIO::writeUint32(buf + 5, param2);
			MIO::writeUint32(buf + 9, param3);
			IPCResponseMessage response;
			IPC::sendMessageSynchronous(g_serviceName, IPCRequestMessage(buf, 13), response);
		}

		class InputService : public Service
		{
			SLIB_APPLICATION(InputService)

		public:
			String getServiceId() override
			{
				return g_serviceName;
			}

			sl_bool onStartService() override
			{
				InputSender::onStartService();
				return sl_true;
			}

			void onStopService() override
			{
				InputSender::onStopService();
			}

		};

		static sl_bool SelectInputDesktop()
		{
			static sl_uint64 tickLastCheck = 0;
			sl_uint64 tickCurrent = System::getTickCount64();
			if (tickLastCheck && tickLastCheck < tickCurrent && tickLastCheck + 500 > tickCurrent) {
				return sl_true;
			}
			tickLastCheck = tickCurrent;
			if (Win32::getInputDesktopName() == Win32::getCurrentDesktopName()) {
				return sl_true;
			}
			return Win32::switchToInputDesktop();
		}

		static sl_bool InstallService()
		{
			CreateServiceParam param;
			param.name = g_serviceName;
			param.path = System::getApplicationPath();
			param.arguments.add(SERVICE_COMMAND);
			param.startType = ServiceStartType::Auto;
			if (ServiceManager::checkParamAndCreateAndStart(param)) {
				g_onInstallService();
				return sl_true;
			}
			return sl_false;
		}

	}

	sl_bool InputSender::prepare(const StringParam& serviceName)
	{
		setServiceName(serviceName);
		String commandLine = String::create(GetCommandLineW());
		List<String> arguments = CommandLine::parse(commandLine);
		if (arguments.contains(SERVICE_COMMAND)) {
			InputService::main();
			return sl_false;
		} else if (arguments.contains(AGENT_COMMAND)) {
			runAgent();
			return sl_false;
		} else if (arguments.contains(INSTALL_COMMAND)) {
			InstallService();
			return sl_false;
		} else {
			if (Process::isCurrentProcessAdmin()) {
				return InstallService();
			} else {
				CreateServiceParam param;
				param.name = serviceName;
				param.path = System::getApplicationPath();
				param.arguments.add(SERVICE_COMMAND);
				if (ServiceManager::checkParamAndIsRunning(param)) {
					Function<sl_bool()> onCheckInstall = g_onCheckInstall;
					if (onCheckInstall.isNull()) {
						return sl_true;
					}
					if (onCheckInstall()) {
						return sl_true;
					}
				}
				Process::runAsAdmin(System::getApplicationPath(), INSTALL_COMMAND);
				return sl_true;
			}
		}
	}

	void InputSender::sendKeyEvent(UIAction action, Keycode key)
	{
		SendAgentMessage(IPC_CMD_SEND_KEY, (sl_uint32)action, (sl_uint32)key, 0);
	}

	void InputSender::sendMouseEvent(UIAction action, sl_ui_pos x, sl_ui_pos y, sl_bool flagAbsolutePos)
	{
		SendAgentMessage(flagAbsolutePos ? IPC_CMD_SEND_MOUSE_ABSOLUTE : IPC_CMD_SEND_MOUSE_RELATIVE, (sl_uint32)action, (sl_uint32)x, (sl_uint32)y);
	}

	void InputSender::setServiceName(const StringParam& _serviceName)
	{
		g_serviceName = String::from(_serviceName);
	}

	void InputSender::setOnStartService(const Function<void()>& callback)
	{
		g_onStartService = callback;
	}

	void InputSender::setOnStopService(const Function<void()>& callback)
	{
		g_onStopService = callback;
	}

	void InputSender::setOnInstallService(const Function<void()>& callback)
	{
		g_onInstallService = callback;
	}

	void InputSender::setOnCheckInstall(const Function<sl_bool()>& callback)
	{
		g_onCheckInstall = callback;
	}

	void InputSender::setOnStartAgent(const Function<void()>& callback)
	{
		g_onStartAgent = callback;
	}

	void InputSender::setOnStopAgent(const Function<void()>& callback)
	{
		g_onStopAgent = callback;
	}

	void InputSender::onStartService()
	{
		String16 cmd = String16::concat(StringView16::literal(u"\""), System::getApplicationPath(), StringView16::literal(u"\" " AGENT_COMMAND));
		Win32::createSystemProcess(cmd);
		g_onStartService();
	}

	void InputSender::onStopService()
	{
		g_onStopService();
		SendAgentMessage(IPC_CMD_QUIT, 0, 0, 0);
	}

	void InputSender::runAgent()
	{
		static sl_bool flagQuit = sl_false;

		String serviceName = g_serviceName;
		NamedInstance instance(String::concat(serviceName, "_agent"));
		if (instance.isNone()) {
			return;
		}

		IPCServerParam param;
		param.name = serviceName;
		param.onReceiveMessage = [](IPCRequestMessage& request, IPCResponseMessage& response) {
			if (request.size != 13) {
				return;
			}
			SelectInputDesktop();
			sl_uint8* data = (sl_uint8*)(request.data);
			sl_uint8 cmd = *data;
			sl_uint32 param1 = MIO::readUint32(data + 1);
			sl_uint32 param2 = MIO::readUint32(data + 5);
			sl_uint32 param3 = MIO::readUint32(data + 9);
			switch (cmd) {
				case IPC_CMD_QUIT:
					flagQuit = sl_true;
					break;
				case IPC_CMD_SEND_KEY:
					UI::sendKeyEvent((UIAction)param1, (Keycode)param2);
					break;
				case IPC_CMD_SEND_MOUSE_RELATIVE:
					UI::sendMouseEvent((UIAction)param1, (sl_ui_pos)param2, (sl_ui_pos)param3, sl_false);
					break;
				case IPC_CMD_SEND_MOUSE_ABSOLUTE:
					UI::sendMouseEvent((UIAction)param1, (sl_ui_pos)param2, (sl_ui_pos)param3, sl_true);
					break;
			}
		};
		auto ipc = IPC::createServer(param);
		if (ipc.isNull()) {
			return;
		}

		g_onStartAgent();
		while (!flagQuit) {
			System::sleep(500);
		}
		g_onStopAgent();
	}

}
