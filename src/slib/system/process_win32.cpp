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

#if defined(SLIB_PLATFORM_IS_WIN32)

#include "slib/system/process.h"

#include "slib/io/file.h"
#include "slib/system/system.h"
#include "slib/core/command_line.h"
#include "slib/core/string_buffer.h"
#include "slib/core/handle_ptr.h"
#include "slib/platform.h"
#include "slib/dl/win32/psapi.h"
#include "slib/dl/win32/kernel32.h"

#include <process.h>
#include <tlhelp32.h>

#define MAX_ARGUMENT_COUNT 128

namespace slib
{

	namespace
	{
		static sl_bool CreatePipe(HANDLE* pRead, HANDLE* pWrite)
		{
			SECURITY_ATTRIBUTES saAttr;
			saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
			saAttr.bInheritHandle = TRUE;
			saAttr.lpSecurityDescriptor = NULL;
			return ::CreatePipe(pRead, pWrite, &saAttr, 0) != 0;
		}

		static sl_bool Execute(const ProcessParam& param, PROCESS_INFORMATION* pi, STARTUPINFOW* si, DWORD flags, sl_bool flagInheritHandles)
		{
			param.prepareArgumentString();
			StringCstr16 executable(param.executable);
			String16 cmd;
			if (param.argumentString.isNotNull()) {
				StringBuffer16 sb;
				sb.addStatic(SLIB_UNICODE("\""));
				sb.addStatic(executable.getData(), executable.getLength());
				sb.addStatic(SLIB_UNICODE("\" "));
				StringData16 args(param.argumentString);
				sb.addStatic(args.getData(), args.getLength());
				cmd = sb.merge();
			}
			String16 env;
			if (param.environment.isNotNull()) {
				StringBuffer16 sb;
				HashMapNode<String, String>* node = param.environment.getFirstNode();
				while (node) {
					if (node->key.isNotEmpty()) {
						sb.add(String16::from(node->key));
						sb.addStatic(u"=");
						sb.add(String16::from(node->value));
						sb.addStatic(u"\0");
					}
					node = node->getNext();
				}
				if (sb.isEmpty()) {
					SLIB_STATIC_STRING16(empty, "\0\0")
					env = empty;
				} else {
					sb.addStatic(u"\0");
					env = sb.merge();
				}
				flags |= CREATE_UNICODE_ENVIRONMENT;
			}
			if (param.flags & ProcessFlags::HideWindow) {
				si->wShowWindow = SW_HIDE;
				si->dwFlags |= STARTF_USESHOWWINDOW;
			}
			StringCstr16 currentDirectory(param.currentDirectory);
			return CreateProcessW(
				(LPCWSTR)(executable.getData()),
				cmd ? (LPWSTR)(cmd.getData()) : NULL,
				NULL, // process security attributes
				NULL, // thread security attributes
				flagInheritHandles || (param.flags & ProcessFlags::InheritHandles),
				flags,
				env ? env.getData() : NULL,
				param.currentDirectory.isNotNull() ? (LPCWSTR)(currentDirectory.getData()) : NULL,
				si,
				pi) != 0;
		}

		class ProcessStream : public IStream
		{
		public:
			HANDLE m_hRead;
			HANDLE m_hWrite;

		public:
			ProcessStream()
			{
				m_hRead = INVALID_HANDLE_VALUE;
				m_hWrite = INVALID_HANDLE_VALUE;
			}

			~ProcessStream()
			{
				_close();
			}

		public:
			void close() override
			{
				_close();
			}

			sl_int32 read32(void* buf, sl_uint32 size, sl_int32 timeout) override
			{
				HANDLE handle = m_hRead;
				if (handle != INVALID_HANDLE_VALUE) {
					if (!size) {
						return SLIB_IO_EMPTY_CONTENT;
					}
					sl_int32 n = (HandlePtr<File>(handle))->read32(buf, size, timeout);
					if (n <= 0) {
						close();
						n = SLIB_IO_ENDED;
					}
					return n;
				}
				return SLIB_IO_ERROR;
			}

			sl_int32 write32(const void* buf, sl_uint32 size, sl_int32 timeout) override
			{
				HANDLE handle = m_hWrite;
				if (handle != INVALID_HANDLE_VALUE) {
					sl_int32 n = (HandlePtr<File>(handle))->write32(buf, size, timeout);
					if (n < 0) {
						close();
					}
					return n;
				}
				close();
				return SLIB_IO_ERROR;
			}

			void _close()
			{
				if (m_hRead >= 0) {
					CloseHandle(m_hRead);
					m_hRead = INVALID_HANDLE_VALUE;
				}
				if (m_hWrite >= 0) {
					CloseHandle(m_hWrite);
					m_hWrite = INVALID_HANDLE_VALUE;
				}
			}
		};

		class ProcessImpl : public Process
		{
		public:
			HANDLE m_hProcess;
			ProcessStream m_stream;

		public:
			ProcessImpl()
			{
				m_hProcess = INVALID_HANDLE_VALUE;
			}

			~ProcessImpl()
			{
				close();
			}

		public:
			static Ref<ProcessImpl> create(const ProcessParam& param)
			{
				HANDLE hStdinRead, hStdinWrite, hStdoutRead, hStdoutWrite;
				if (CreatePipe(&hStdinRead, &hStdinWrite)) {
					SetHandleInformation(hStdinWrite, HANDLE_FLAG_INHERIT, 0);
					if (CreatePipe(&hStdoutRead, &hStdoutWrite)) {
						SetHandleInformation(hStdoutRead, HANDLE_FLAG_INHERIT, 0);
						PROCESS_INFORMATION pi = {0};
						STARTUPINFOW si = {0};
						si.cb = sizeof(si);
						si.hStdInput = hStdinRead;
						si.hStdOutput = hStdoutWrite;
						si.hStdError = hStdoutWrite;
						si.dwFlags = STARTF_USESTDHANDLES;
						if (Execute(param, &pi, &si, NORMAL_PRIORITY_CLASS, sl_true)) {
							CloseHandle(pi.hThread);
							CloseHandle(hStdinRead);
							CloseHandle(hStdoutWrite);
							Ref<ProcessImpl> ret = new ProcessImpl;
							if (ret.isNotNull()) {
								ret->m_hProcess = pi.hProcess;
								ret->m_stream.m_hRead = hStdoutRead;
								ret->m_stream.m_hWrite = hStdinWrite;
								return ret;
							}
						}
						CloseHandle(hStdoutRead);
						CloseHandle(hStdoutWrite);
					}
					CloseHandle(hStdinRead);
					CloseHandle(hStdinWrite);
				}
				return sl_null;
			}

		public:
			void terminate() override
			{
				ObjectLocker lock(this);
				m_stream.close();
				if (m_hProcess != INVALID_HANDLE_VALUE) {
					HANDLE handle = m_hProcess;
					m_hProcess = INVALID_HANDLE_VALUE;
					lock.unlock();
					TerminateProcess(handle, 0);
					CloseHandle(handle);
					m_status = ProcessStatus::Terminated;
				}
			}

			void kill() override
			{
				terminate();
			}

			void wait() override
			{
				ObjectLocker lock(this);
				if (m_hProcess != INVALID_HANDLE_VALUE) {
					HANDLE handle = m_hProcess;
					m_hProcess = INVALID_HANDLE_VALUE;
					lock.unlock();
					DWORD ret = WaitForSingleObject(handle, INFINITE);
					if (ret == WAIT_OBJECT_0) {
						m_status = ProcessStatus::Exited;
						DWORD code = 0;
						if (GetExitCodeProcess(handle, &code)) {
							m_exitStatus = (int)code;
						}
					} else {
						m_status = ProcessStatus::Unknown;
					}
					CloseHandle(handle);
					m_stream.close();
				}
			}

			sl_bool isAlive() override
			{
				ObjectLocker lock(this);
				if (m_hProcess != INVALID_HANDLE_VALUE) {
					DWORD code = 0;
					if (GetExitCodeProcess(m_hProcess, &code)) {
						if (code == STILL_ACTIVE) {
							return sl_true;
						}
					}
				}
				return sl_false;
			}

			IStream* getStream() override
			{
				return &m_stream;
			}

			void close()
			{
				if (m_hProcess != INVALID_HANDLE_VALUE) {
					CloseHandle(m_hProcess);
					m_hProcess = INVALID_HANDLE_VALUE;
				}
			}
		};
	}

	List<sl_uint32> Process::getAllProcessIds()
	{
		auto funcEnumProcesses = psapi::getApi_EnumProcesses();
		if (!funcEnumProcesses) {
			return sl_null;
		}
		List<sl_uint32> ret;
		for (sl_size i = 0; i < 8; i++) {
			sl_uint32 n = 256 << i;
			List<sl_uint32> listNew = List<sl_uint32>::create(n);
			if (listNew.isNull()) {
				break;
			}
			DWORD m = 0;
			if (funcEnumProcesses((DWORD*)(listNew.getData()), n << 2, &m)) {
				ret = Move(listNew);
				if (m != (n << 2)) {
					ret.setCount_NoLock(m >> 2);
					break;
				}
			} else {
				break;
			}
		}
		return ret;
	}

	List<sl_uint32> Process::getAllThreadIds(sl_uint32 processId)
	{
		List<sl_uint32> ret;
		HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
		if (hSnapshot != INVALID_HANDLE_VALUE) {
			THREADENTRY32 entry = { 0 };
			entry.dwSize = sizeof(entry);
			if (Thread32First(hSnapshot, &entry)) {
				do {
					if (!processId || entry.th32OwnerProcessID == processId) {
						ret.add_NoLock((sl_uint32)(entry.th32ThreadID));
					}
				} while (Thread32Next(hSnapshot, &entry));
			}
			CloseHandle(hSnapshot);
		}
		return ret;
	}

	String Process::getImagePath(sl_uint32 processId)
	{
		auto funcQueryFullProcessImageNameW = kernel32::getApi_QueryFullProcessImageNameW();
		auto funcGetModuleFileNameExW = psapi::getApi_GetModuleFileNameExW();
		if (!(funcQueryFullProcessImageNameW || funcGetModuleFileNameExW)) {
			return sl_null;
		}
		String ret;
		if (funcQueryFullProcessImageNameW) {
			HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, (DWORD)processId);
			if (hProcess) {
				WCHAR filePath[MAX_PATH + 1];
				DWORD lenPath = MAX_PATH;
				if (funcQueryFullProcessImageNameW(hProcess, NULL, filePath, &lenPath)) {
					ret = String::from(filePath, lenPath);
				}
				CloseHandle(hProcess);
			}
		} else {
			HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, (DWORD)processId);
			if (hProcess) {
				WCHAR filePath[MAX_PATH + 1];
				DWORD dwLen = funcGetModuleFileNameExW(hProcess, NULL, filePath, MAX_PATH);
				if (dwLen) {
					ret = String::from(filePath, dwLen);
				}
				CloseHandle(hProcess);
			}
		}
		return ret;
	}

	sl_bool Process::is32BitProcess(sl_uint32 processId)
	{
		if (!(System::is64BitSystem())) {
			return sl_true;
		}
		auto func_IsWow64Process = kernel32::getApi_IsWow64Process();
		if (func_IsWow64Process) {
			HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, processId);
			if (hProcess) {
				BOOL bIsWow64 = FALSE;
				func_IsWow64Process(hProcess, &bIsWow64);
				CloseHandle(hProcess);
				if (bIsWow64) {
					return sl_true;
				}
			}
		}
		return sl_false;
	}

	sl_bool Process::kill(sl_uint32 processId)
	{
		HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, processId);
		if (hProcess) {
			BOOL bRet = TerminateProcess(hProcess, 0);
			CloseHandle(hProcess);
			return bRet != 0;
		}
		return sl_false;
	}

	sl_bool Process::quit(sl_uint32 processId)
	{
		ListElements<sl_uint32> threads(getAllThreadIds(processId));
		for (sl_size i = 0; i < threads.count; i++) {
			PostThreadMessageW(threads[i], WM_QUIT, 0, 0);
		}
		return sl_true;
	}

	sl_uint32 Process::getCurrentProcessId()
	{
		return (sl_uint32)(GetCurrentProcessId());
	}

	Ref<Process> Process::open(const ProcessParam& param)
	{
		return Ref<Process>::cast(ProcessImpl::create(param));
	}

	Ref<Process> Process::run(const ProcessParam& param)
	{
		PROCESS_INFORMATION pi = {0};
		STARTUPINFOW si = {0};
		si.cb = sizeof(si);
		if (Execute(param, &pi, &si, NORMAL_PRIORITY_CLASS | DETACHED_PROCESS, sl_false)) {
			CloseHandle(pi.hThread);
			Ref<ProcessImpl> ret = new ProcessImpl;
			if (ret.isNotNull()) {
				ret->m_hProcess = pi.hProcess;
				return ret;
			}
			CloseHandle(pi.hProcess);
		}
		return sl_null;
	}

	void Process::runAsAdmin(const ProcessParam& pp)
	{
		pp.prepareArgumentString();
		ShellExecuteParam sep;
		sep.flagRunAsAdmin = sl_true;
		sep.path = pp.executable;
		sep.parameters = pp.argumentString;
		sep.currentDirectory = pp.currentDirectory;
		sep.flagWait = !(pp.flags & ProcessFlags::NoWait);
		if (pp.flags & ProcessFlags::HideWindow) {
			sep.nShow = SW_HIDE;
		}
		Win32::shell(sep);
	}

	sl_bool Process::isCurrentProcessAdmin()
	{
		BOOL flagResult = FALSE;
		SID_IDENTIFIER_AUTHORITY siAuthority = SECURITY_NT_AUTHORITY;
		PSID pSidAdmin = NULL;
		if (AllocateAndInitializeSid(&siAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &pSidAdmin)) {
			CheckTokenMembership(NULL, pSidAdmin, &flagResult);
			FreeSid(pSidAdmin);
		}
		return flagResult != FALSE;
	}

	sl_bool Process::isCurrentProcessInAdminGroup()
	{
		BOOL flagResult = FALSE;
		HANDLE hToken;
		if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY | TOKEN_DUPLICATE, &hToken)) {
			const WindowsVersion& version = Win32::getVersion();
			sl_bool flagError = sl_false;
			HANDLE hTokenToCheck = NULL;
			if (version.majorVersion >= WindowsVersion::Vista_MajorVersion) {
				TOKEN_ELEVATION_TYPE elevType;
				DWORD cbSize = 0;
				if (GetTokenInformation(hToken, TokenElevationType, &elevType, sizeof(elevType), &cbSize)) {
					if (elevType == TokenElevationTypeLimited) {
						if (!GetTokenInformation(hToken, TokenLinkedToken, &hTokenToCheck, sizeof(hTokenToCheck), &cbSize)) {
							flagError = sl_true;
						}
					}
				} else {
					flagError = sl_true;
				}
			}
			if (!flagError) {
				if (!hTokenToCheck) {
					DuplicateToken(hToken, SecurityIdentification, &hTokenToCheck);
				}
				if (hTokenToCheck) {
					BYTE adminSID[SECURITY_MAX_SID_SIZE];
					DWORD cbSize = sizeof(adminSID);
					if (CreateWellKnownSid(WinBuiltinAdministratorsSid, NULL, &adminSID, &cbSize)) {
						CheckTokenMembership(hTokenToCheck, &adminSID, &flagResult);
					}
					CloseHandle(hTokenToCheck);
				}
			}
			CloseHandle(hToken);
		}
		return flagResult != FALSE;
	}

	void Process::exec(const ProcessParam& param)
	{
		param.prepareArgumentList();
		if (param.currentDirectory.isNotNull()) {
			System::setCurrentDirectory(param.currentDirectory);
		}
		StringCstr pathExecutable(param.executable);
		char* exe = pathExecutable.getData();
		char* args[MAX_ARGUMENT_COUNT + 2];
		StringCstr _args[MAX_ARGUMENT_COUNT];
		{
			args[0] = exe;
			ListElements<StringParam> list(param.arguments);
			if (list.count > MAX_ARGUMENT_COUNT) {
				list.count = MAX_ARGUMENT_COUNT;
			}
			for (sl_size i = 0; i < list.count; i++) {
				_args[i] = list[i];
				args[i + 1] = _args[i].getData();
			}
			args[list.count + 1] = 0;
		}
		_execvp(exe, args);
		::abort();
	}

	void Process::abort()
	{
		::abort();
	}

	void Process::exit(int code)
	{
		::exit(code);
	}

	namespace
	{
		static sl_bool GetLogonPid(DWORD& pid)
		{
			auto funcProcessIdToSessionId = kernel32::getApi_ProcessIdToSessionId();
			if (!funcProcessIdToSessionId) {
				return sl_false;
			}
			auto funcWTSGetActiveConsoleSessionId = kernel32::getApi_WTSGetActiveConsoleSessionId();
			if (!funcWTSGetActiveConsoleSessionId) {
				return sl_false;
			}
			sl_bool bRet = sl_false;
			HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
			if (hSnapshot != INVALID_HANDLE_VALUE) {
				PROCESSENTRY32W entry;
				entry.dwSize = sizeof(entry);
				if (Process32FirstW(hSnapshot, &entry)) {
					do {
						if (Base::equalsString2((sl_char16*)(entry.szExeFile), u"winlogon.exe")) {
							DWORD sessionId = 0;
							if (funcProcessIdToSessionId(entry.th32ProcessID, &sessionId)) {
								if (sessionId == funcWTSGetActiveConsoleSessionId()) {
									pid = entry.th32ProcessID;
									bRet = sl_true;
								}
							}
						}
					} while (Process32NextW(hSnapshot, &entry));
				}
				CloseHandle(hSnapshot);
			}
			return bRet;
		}

		static sl_bool GetLogonSessionToken(HANDLE& token)
		{
			sl_bool bRet = FALSE;
			DWORD pid;
			if (GetLogonPid(pid)) {
				HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
				if (hProcess) {
					bRet = OpenProcessToken(hProcess, TOKEN_ALL_ACCESS, &token) != 0;
					CloseHandle(hProcess);
				}
			}
			return bRet;
		}
	}

	HANDLE Win32::createSystemProcess(const StringParam& _command)
	{
		HANDLE hProcess = NULL;
		HANDLE hToken;
		if (GetLogonSessionToken(hToken)) {
			STARTUPINFO si = { 0 };
			si.cb = sizeof si;
			StringCstr16 command(_command);
			PROCESS_INFORMATION pi;
			if (CreateProcessAsUserW(hToken, NULL, (LPWSTR)(command.getData()), NULL, NULL, FALSE, DETACHED_PROCESS, NULL, NULL, &si, &pi)) {
				CloseHandle(pi.hThread);
				hProcess = pi.hProcess;
			}
			CloseHandle(hToken);
		}
		return hProcess;
	}

	String Win32::getProcessPath(HWND hWnd)
	{
		DWORD dwProcessId = 0;
		GetWindowThreadProcessId(hWnd, &dwProcessId);
		if (dwProcessId) {
			return Process::getImagePath(dwProcessId);
		}
		return sl_null;
	}

}

#endif
