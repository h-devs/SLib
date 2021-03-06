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

#if defined(SLIB_PLATFORM_IS_WINDOWS)

#include "slib/core/process.h"

#include "slib/core/file.h"
#include "slib/core/command_line.h"
#include "slib/core/string_buffer.h"
#include "slib/core/handle_ptr.h"
#include "slib/core/win32/platform.h"

#include <process.h>

namespace slib
{

	namespace priv
	{
		namespace process
		{

#if !defined(SLIB_PLATFORM_IS_MOBILE)

			static sl_bool CreatePipe(HANDLE* pRead, HANDLE* pWrite)
			{
				SECURITY_ATTRIBUTES saAttr;
				saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
				saAttr.bInheritHandle = TRUE;
				saAttr.lpSecurityDescriptor = NULL;
				return ::CreatePipe(pRead, pWrite, &saAttr, 0) != 0;
			}
			
			static sl_bool Execute(const StringParam& _pathExecutable, const StringParam* cmds, sl_size nCmds, PROCESS_INFORMATION* pi, STARTUPINFOW* si, DWORD flags, sl_bool flagInheritHandles)
			{
				StringCstr16 pathExecutable(_pathExecutable);
				StringCstr16 cmd;
				{
					StringBuffer16 sb;
					sb.addStatic(SLIB_UNICODE("\""));
					sb.addStatic(pathExecutable.getData(), pathExecutable.getLength());
					sb.addStatic(SLIB_UNICODE("\""));
					String args = CommandLine::build(cmds, nCmds);
					if (args.isNotEmpty()) {
						sb.addStatic(SLIB_UNICODE(" "));
						sb.add(String16::from(args));
					}
					cmd = sb.merge();
				}
				return CreateProcessW(
					(LPCWSTR)(pathExecutable.getData()),
					(LPWSTR)(cmd.getData()),
					NULL, // process security attributes
					NULL, // thread security attributes
					flagInheritHandles, // handles are inherited,
					flags,
					NULL, // Environment (uses parent's environment)
					NULL, // Current Directory (uses parent's current directory)
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

				sl_int32 read32(void* buf, sl_uint32 size) override
				{
					HANDLE handle = m_hRead;
					if (handle != INVALID_HANDLE_VALUE) {
						if (!size) {
							return SLIB_IO_EMPTY_CONTENT;
						}
						sl_int32 n = (HandlePtr<File>(handle))->read32(buf, size);
						if (n <= 0) {
							close();
						}
						return n;
					}
					return SLIB_IO_ERROR;
				}

				sl_int32 write32(const void* buf, sl_uint32 size) override
				{
					HANDLE handle = m_hWrite;
					if (handle != INVALID_HANDLE_VALUE) {
						sl_int32 n = (HandlePtr<File>(handle))->write32(buf, size);
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
				static Ref<ProcessImpl> create(const StringParam& pathExecutable, const StringParam* arguments, sl_size nArguments)
				{
					HANDLE hStdinRead, hStdinWrite, hStdoutRead, hStdoutWrite;
					if (CreatePipe(&hStdinRead, &hStdinWrite)) {
						SetHandleInformation(hStdinWrite, HANDLE_FLAG_INHERIT, 0);
						if (CreatePipe(&hStdoutRead, &hStdoutWrite)) {
							SetHandleInformation(hStdoutRead, HANDLE_FLAG_INHERIT, 0);
							PROCESS_INFORMATION pi;
							Base::zeroMemory(&pi, sizeof(pi));
							STARTUPINFOW si;
							Base::zeroMemory(&si, sizeof(si));
							si.cb = sizeof(si);
							si.hStdInput = hStdinRead;
							si.hStdOutput = hStdoutWrite;
							si.hStdError = hStdoutWrite;
							si.dwFlags = STARTF_USESTDHANDLES;
							if (Execute(pathExecutable, arguments, nArguments, &pi, &si, NORMAL_PRIORITY_CLASS, sl_true)) {
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

#endif

		}
	}

	using namespace priv::process;

	sl_uint32 Process::getCurrentProcessId()
	{
		return (sl_uint32)(GetCurrentProcessId());
	}

#if !defined(SLIB_PLATFORM_IS_MOBILE)
	Ref<Process> Process::open(const StringParam& pathExecutable, const StringParam* arguments, sl_size nArguments)
	{
		return Ref<Process>::from(ProcessImpl::create(pathExecutable, arguments, nArguments));
	}

	Ref<Process> Process::run(const StringParam& pathExecutable, const StringParam* arguments, sl_size nArguments)
	{
		PROCESS_INFORMATION pi;
		ZeroMemory(&pi, sizeof(pi));

		STARTUPINFOW si;
		ZeroMemory(&si, sizeof(si));
		si.cb = sizeof(si);

		if (Execute(pathExecutable, arguments, nArguments, &pi, &si, NORMAL_PRIORITY_CLASS | DETACHED_PROCESS, sl_false)) {
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

	void Process::runAsAdmin(const StringParam& pathExecutable, const StringParam* arguments, sl_size nArguments)
	{
		ShellExecuteParam param;
		param.runAsAdmin = sl_true;
		param.path = pathExecutable;
		param.params = CommandLine::build(arguments, nArguments);
		Win32::shell(param);
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
			WindowsVersion version = Win32::getVersion();
			sl_bool flagError = sl_false;
			HANDLE hTokenToCheck = NULL;
			if (SLIB_WINDOWS_MAJOR_VERSION(version) >= 6) { // Windows Vista or later
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

	void Process::exec(const StringParam& _pathExecutable, const StringParam* arguments, sl_size nArguments)
	{
#if defined(SLIB_PLATFORM_IS_WIN32)
		StringCstr pathExecutable(_pathExecutable);
		char* exe = pathExecutable.getData();
		char* args[64];
		StringCstr _args[60];
		args[0] = exe;
		if (nArguments > 60) {
			nArguments = 60;
		}
		for (sl_size i = 0; i < nArguments; i++) {
			_args[i] = arguments[i];
			args[i + 1] = _args[i].getData();
		}
		args[nArguments + 1] = 0;
		_execvp(exe, args);
		::abort();
#endif
	}

	void Process::abort()
	{
		::abort();
	}

	void Process::exit(int code)
	{
		::exit(code);
	}

#endif

}

#endif
