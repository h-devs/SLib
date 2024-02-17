#include "input_sender.h"

#include <slib/core/service.h>
#include <slib/core/service_manager.h>
#include <slib/core/thread.h>
#include <slib/core/process.h>
#include <slib/core/system.h>
#include <slib/core/mio.h>
#include <slib/core/command_line.h>
#include <slib/core/safe_static.h>
#include <slib/network/ipc.h>
#include <slib/ui/core.h>
#include <slib/platform.h>

#define SERVICE_NAME "InputSender"
#define SERVICE_COMMAND "daemon"
#define AGENT_COMMAND "agent"
#define INSTALL_COMMAND "install"
#define AGENT_NAME "input_agent"

#define IPC_CMD_QUIT 0x01
#define IPC_CMD_SEND_KEY 0x02
#define IPC_CMD_SEND_MOUSE_RELATIVE 0x03
#define IPC_CMD_SEND_MOUSE_ABSOLUTE 0x04

namespace slib
{

	namespace
	{

		static void SendAgentMessage(sl_uint8 uMsg, sl_uint32 param1, sl_uint32 param2, sl_uint32 param3)
		{
			SLIB_SAFE_LOCAL_STATIC(Ref<IPC>, ipc, IPC::create())
			if (ipc.isNotNull()) {
				sl_uint8 buf[13];
				*buf = uMsg;
				MIO::writeUint32(buf + 1, param1);
				MIO::writeUint32(buf + 5, param2);
				MIO::writeUint32(buf + 9, param3);
				ipc->sendMessageSynchronous(AGENT_NAME, Memory::create(buf, 13));
			}
		}

		class InputService : public Service
		{
			SLIB_APPLICATION(InputService)

		public:
			Ref<Thread> m_thread;

		public:
			String getServiceId() override
			{
				return SERVICE_NAME;
			}

			sl_bool onStartService() override
			{
				m_thread = Thread::start(SLIB_FUNCTION_MEMBER(this, onRunAgent));
				return sl_true;
			}

			void onStopService() override
			{
				m_thread->finishAndWait();
			}

			void onRunAgent()
			{
				HANDLE hProcessAgent = NULL;
				while (Thread::isNotStoppingCurrent()) {
					if (hProcessAgent) {
						do {
							DWORD dwExitCode = 0;
							if (GetExitCodeProcess(hProcessAgent, &dwExitCode)) {
								if (dwExitCode == STILL_ACTIVE) {
									break;
								}
							}
							CloseHandle(hProcessAgent);
							hProcessAgent = NULL;
						} while (0);
					}
					if (!hProcessAgent) {
						quitAgent();
						String16 cmd = String16::concat(StringView16::literal(u"\""), System::getApplicationPath(), StringView16::literal(u"\" " AGENT_COMMAND));
						hProcessAgent = Win32::createSystemProcess(cmd);
					}
					Thread::sleep(500);
				}
				quitAgent();
			}

			void quitAgent()
			{
				SendAgentMessage(IPC_CMD_QUIT, 0, 0, 0);
			}

		};

		static HDESK GetInputDesktop()
		{
			return OpenInputDesktop(0, FALSE, DESKTOP_CREATEMENU | DESKTOP_CREATEWINDOW | DESKTOP_ENUMERATE | DESKTOP_HOOKCONTROL | DESKTOP_WRITEOBJECTS | DESKTOP_READOBJECTS | DESKTOP_SWITCHDESKTOP | GENERIC_WRITE);
		}

		static sl_bool IsInputDesktopSelected()
		{
			HDESK hCurrent = GetThreadDesktop(GetCurrentThreadId());
			if (!hCurrent) {
				return sl_false;
			}
			sl_bool bRet = sl_false;
			HDESK hInput = OpenInputDesktop(0, FALSE,
				DESKTOP_CREATEMENU | DESKTOP_CREATEWINDOW |
				DESKTOP_ENUMERATE | DESKTOP_HOOKCONTROL |
				DESKTOP_WRITEOBJECTS | DESKTOP_READOBJECTS |
				DESKTOP_SWITCHDESKTOP | GENERIC_WRITE);
			if (hInput) {
				DWORD size;
				WCHAR currentName[256] = { 0 };
				if (GetUserObjectInformationW(hCurrent, UOI_NAME, currentName, sizeof(currentName) - 2, &size)) {
					WCHAR inputName[256] = { 0 };
					if (GetUserObjectInformationW(hInput, UOI_NAME, inputName, sizeof(inputName) - 2, &size)) {
						if (Base::equalsString2((sl_char16*)currentName, (sl_char16*)inputName)) {
							bRet = sl_true;
						}
					}
				}
				CloseDesktop(hInput);
			}
			return bRet;
		}

		static sl_bool SwitchToDesktop(HDESK hDesktop)
		{
			HDESK hDesktopOld = GetThreadDesktop(GetCurrentThreadId());
			if (SetThreadDesktop(hDesktop)) {
				if (hDesktopOld) {
					CloseDesktop(hDesktopOld);
				}
				return sl_true;
			} else {
				return sl_false;
			}
		}

		static sl_bool SelectInputDesktop()
		{
			static sl_uint64 tickLastCheck = 0;
			sl_uint64 tickCurrent = System::getTickCount64();
			if (tickLastCheck && tickLastCheck < tickCurrent && tickLastCheck + 500 > tickCurrent) {
				return sl_true;
			}
			tickLastCheck = tickCurrent;
			if (IsInputDesktopSelected()) {
				return sl_true;
			}
			HDESK hDesktop = GetInputDesktop();
			if (hDesktop) {
				if (SwitchToDesktop(hDesktop)) {
					return sl_true;
				}
				CloseDesktop(hDesktop);
			}
			return sl_false;
		}

		static void RunAgent()
		{
			static sl_bool flagQuit = sl_false;

			NamedInstance instance(AGENT_NAME);
			if (instance.isNone()) {
				return;
			}

			IPCParam param;
			param.name = AGENT_NAME;
			param.onReceiveMessage = [](sl_uint8* data, sl_uint32 size, MemoryOutput* output) {
				if (size != 13) {
					return;
				}
				SelectInputDesktop();
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
			Ref<IPC> ipc = IPC::create(param);
			if (ipc.isNull()) {
				return;
			}

			while (!flagQuit) {
				System::sleep(500);
			}
		}

		static sl_bool InstallService()
		{
			CreateServiceParam param;
			param.name = SERVICE_NAME;
			param.path = System::getApplicationPath();
			param.arguments.add(SERVICE_COMMAND);
			String path = ServiceManager::getCommandPath(SERVICE_NAME);
			if (path.isNotNull()) {
				if (path == param.getCommandLine()) {
					if (ServiceManager::isRunning(SERVICE_NAME)) {
						return sl_true;
					} else {
						return ServiceManager::start(SERVICE_NAME);
					}
				}
				ServiceManager::remove(SERVICE_NAME);
			}
			ServiceManager::create(param);
			return ServiceManager::start(SERVICE_NAME);
		}

	}

	sl_bool InputSender::prepare()
	{
		String commandLine = String::create(GetCommandLineW());
		List<String> arguments = CommandLine::parse(commandLine);
		if (arguments.contains(SERVICE_COMMAND)) {
			InputService::main();
			return sl_false;
		} else if (arguments.contains(AGENT_COMMAND)) {
			RunAgent();
			return sl_false;
		} else if (arguments.contains(INSTALL_COMMAND)) {
			InstallService();
			return sl_false;
		} else {
			if (Process::isCurrentProcessAdmin()) {
				return InstallService();
			} else {
				String path = ServiceManager::getCommandPath(SERVICE_NAME);
				if (path.isNotNull()) {
					CreateServiceParam param;
					param.path = System::getApplicationPath();
					param.arguments.add(SERVICE_COMMAND);
					if (path == param.getCommandLine()) {
						if (ServiceManager::isRunning(SERVICE_NAME)) {
							return sl_true;
						}
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

}
