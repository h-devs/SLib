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

#if defined(SLIB_PLATFORM_IS_UNIX)

#include "slib/system/process.h"

#include "slib/system/system.h"
#include "slib/io/file.h"
#include "slib/core/command_line.h"
#include "slib/core/handle_ptr.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>

#include <sys/wait.h>

#define MAX_ARGUMENT_COUNT 128

namespace slib
{

	namespace
	{
		static void Exec(const ProcessParam& param)
		{
			param.prepareArgumentList();
			if (param.currentDirectory.isNotNull()) {
				System::setCurrentDirectory(param.currentDirectory);
			}
			if (param.flags & ProcessFlags::ResetEnvironment) {
				clearenv();
			}
			if (param.environment.isNotNull()) {
				HashMapNode<String, String>* node = param.environment.getFirstNode();
				while (node) {
					StringCstr name(node->key);
					StringCstr value(node->value);
					setenv(name.getData(), value.getData(), 1);
					node = node->next;
				}
			}
			StringCstr executable(param.executable);
			char* exe = executable.getData();
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
					args[i+1] = _args[i].getData();
				}
				args[list.count+1] = 0;
			}
			execvp(exe, args);
			::abort();
		}

		class ProcessStream : public IStream
		{
		public:
			int m_hRead;
			int m_hWrite;

		public:
			ProcessStream()
			{
				m_hRead = -1;
				m_hWrite = -1;
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
				int handle = m_hRead;
				if (handle >= 0) {
					if (!size) {
						return SLIB_IO_EMPTY_CONTENT;
					}
					sl_int32 n = (HandlePtr<File>(handle))->read32(buf, size, timeout);
					if (n <= 0 && n != SLIB_IO_WOULD_BLOCK) {
						close();
					}
					return n;
				}
				return SLIB_IO_ERROR;
			}

			sl_int32 write32(const void* buf, sl_uint32 size, sl_int32 timeout) override
			{
				int handle = m_hWrite;
				if (handle) {
					sl_int32 n = (HandlePtr<File>(handle))->write32(buf, size, timeout);
					if (n < 0 && n != SLIB_IO_WOULD_BLOCK) {
						close();
					}
					return n;
				}
				return SLIB_IO_ERROR;
			}

			void _close()
			{
				if (m_hRead >= 0) {
					::close(m_hRead);
					m_hRead = -1;
				}
				if (m_hWrite >= 0) {
					::close(m_hWrite);
					m_hWrite = -1;
				}
			}

		};

		class ProcessImpl : public Process
		{
		public:
			pid_t m_pid = -1;
			ProcessStream m_stream;

		public:
			static Ref<ProcessImpl> create(const ProcessParam& param)
			{
				int hStdin[2], hStdout[2];
				if (pipe(hStdin) == 0) {
					if (pipe(hStdout) == 0) {
						pid_t pid = fork();
						if (!pid) {
							// child process
							::close(hStdin[1]); // WRITE
							::close(hStdout[0]); // READ
							dup2(hStdin[0], 0); // STDIN
							dup2(hStdout[1], 1); // STDOUT
							::close(hStdin[0]);
							::close(hStdout[1]);
							Exec(param);
							return sl_null;
						} else if (pid > 0) {
							// parent process
							Ref<ProcessImpl> ret = new ProcessImpl;
							if (ret.isNotNull()) {
								ret->m_pid = pid;
								::close(hStdin[0]); // READ
								::close(hStdout[1]); // WRITE
								ret->m_stream.m_hRead = hStdout[0];
								ret->m_stream.m_hWrite = hStdin[1];
								return ret;
							}
						}
						::close(hStdout[0]);
						::close(hStdout[1]);
					}
					::close(hStdin[0]);
					::close(hStdin[1]);
				}
				return sl_null;
			}

		public:
			void terminate() override
			{
				ObjectLocker lock(this);
				m_stream.close();
				if (m_pid > 0) {
					pid_t pid = m_pid;
					m_pid = -1;
					lock.unlock();
					::kill(pid, SIGTERM);
					m_status = ProcessStatus::Terminated;
				}
			}

			void kill() override
			{
				ObjectLocker lock(this);
				m_stream.close();
				if (m_pid > 0) {
					pid_t pid = m_pid;
					m_pid = -1;
					lock.unlock();
					::kill(pid, SIGKILL);
					m_status = ProcessStatus::Killed;
				}
			}

			void wait() override
			{
				ObjectLocker lock(this);
				if (m_pid > 0) {
					pid_t pid = m_pid;
					m_pid = -1;
					lock.unlock();
					for (;;) {
						int status = 0;
						int ret = waitpid(pid, &status, WUNTRACED | WCONTINUED);
						if (ret == -1) {
							m_stream.close();
							::kill(pid, SIGKILL);
							m_status = ProcessStatus::Killed;
							return;
						} else if (ret != 0) {
							if (WIFEXITED(status)) {
								m_status = ProcessStatus::Exited;
								m_exitStatus = WEXITSTATUS(status);
								break;
							} else if (WIFSIGNALED(status)) {
								int sig = WTERMSIG(status);
								if (sig == SIGTERM) {
									m_status = ProcessStatus::Terminated;
								} else if (sig == SIGKILL) {
									m_status = ProcessStatus::Killed;
								} else {
									m_status = ProcessStatus::Unknown;
								}
								break;
							}
							System::sleep(1);
						}
					}
					m_stream.close();
				}
			}

			sl_bool isAlive() override
			{
				pid_t pid = m_pid;
				int status;
				return waitpid(pid, &status, WNOHANG) != -1;
			}

			IStream* getStream() override
			{
				return &m_stream;
			}
		};
	}

	sl_bool Process::kill(sl_uint32 processId)
	{
		return !(::kill((pid_t)processId, SIGKILL));
	}

	sl_bool Process::quit(sl_uint32 processId)
	{
		return !(::kill((pid_t)processId, SIGTERM));
	}

	sl_uint32 Process::getCurrentProcessId()
	{
		return getpid();
	}

	Ref<Process> Process::open(const ProcessParam& param)
	{
		return Ref<Process>::cast(ProcessImpl::create(param));
	}

#if !defined(SLIB_PLATFORM_IS_MACOS)
	Ref<Process> Process::run(const ProcessParam& param)
	{
		pid_t pid = fork();
		if (!pid) {
			// Child process
			// Daemonize
			setsid();
			close(0);
			close(1);
			close(2);
			int handle = ::open("/dev/null", O_RDWR);
			if (handle >= 0) {
				if (handle) {
					dup2(handle, 0);
				}
				dup2(handle, 1);
				dup2(handle, 2);
			}
			signal(SIGHUP, SIG_IGN);
			Exec(param);
		} else if (pid > 0) {
			// Parent process
			Ref<ProcessImpl> ret = new ProcessImpl;
			if (ret.isNotNull()) {
				ret->m_pid = pid;
				return ret;
			}
		}
		return sl_null;
	}

	void Process::runAsAdmin(const ProcessParam& input)
	{
#if !defined(SLIB_PLATFORM_IS_MOBILE)
		ProcessParam param;
		if (File::isFile(StringView::literal("/usr/bin/pkexec"))) {
			param.executable = StringView::literal("/usr/bin/pkexec");
		} else if (File::isFile(StringView::literal("/usr/bin/kdesu"))) {
			param.executable = StringView::literal("/usr/bin/kdesu");
		} else if (File::isFile(StringView::literal("/usr/bin/gksu"))) {
			param.executable = StringView::literal("/usr/bin/gksu");
		} else {
			return;
		}
		List<StringParam> arguments;
		arguments.add_NoLock(StringView::literal("env"));
		arguments.add_NoLock(String::concat(StringView::literal("DISPLAY="), System::getEnvironmentVariable(StringView::literal("DISPLAY"))));
		arguments.add_NoLock(String::concat(StringView::literal("XAUTHORITY="), System::getEnvironmentVariable(StringView::literal("XAUTHORITY"))));
		if (input.currentDirectory.isNotNull()) {
			arguments.add_NoLock(StringView::literal("/bin/sh"));
			arguments.add_NoLock(StringView::literal("-c"));
			input.prepareArgumentString();
			String command = String::concat(StringView::literal("cd "), CommandLine::makeSafeArgumentForUnix(input.currentDirectory), StringView::literal(" && "), CommandLine::makeSafeArgumentForUnix(input.executable), StringView::literal(" "), input.argumentString);
			arguments.add_NoLock(Move(command));
		} else {
			arguments.add_NoLock(input.executable);
			input.prepareArgumentList();
			arguments.addElements_NoLock(input.arguments.getData(), input.arguments.getCount());
		}
		param.arguments = Move(arguments);
		param.flags = input.flags;
		if (input.flags & ProcessFlags::NoWait) {
			run(param);
		} else {
			Ref<Process> process = run(param);
			if (process.isNotNull()) {
				process->wait();
			}
		}
#endif
	}
#endif

	sl_bool Process::isCurrentProcessAdmin()
	{
		return !(geteuid());
	}

	void Process::exec(const ProcessParam& param)
	{
		Exec(param);
	}

	void Process::abort()
	{
		::abort();
	}

	void Process::exit(int code)
	{
		::exit(code);
	}

}

#endif
