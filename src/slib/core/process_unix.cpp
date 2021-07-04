/*
 *   Copyright (c) 2008-2019 SLIBIO <https://github.com/SLIBIO>
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

#include "slib/core/process.h"

#include "slib/core/file.h"
#include "slib/core/list.h"
#include "slib/core/system.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>

#include <sys/wait.h>

namespace slib
{
	
	namespace priv
	{
		namespace process
		{

#if !defined(SLIB_PLATFORM_IS_MOBILE)
			
			static void Exec(const StringParam& _pathExecutable, const StringParam* arguments, sl_size nArguments)
			{
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
					args[i+1] = _args[i].getData();
				}
				args[nArguments+1] = 0;
				
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
				
				sl_int32 read32(void* buf, sl_uint32 size) override
				{
					int handle = m_hRead;
					if (handle >= 0) {
						ssize_t n = ::read(handle, buf, size);
						if (n >= 0) {
							if (n > 0) {
								return (sl_int32)n;
							}
						} else {
							int err = errno;
							if (err == EAGAIN || err == EWOULDBLOCK || err == EINTR) {
								return 0;
							}
						}
					}
					close();
					return -1;
				}
				
				sl_int32 write32(const void* buf, sl_uint32 size) override
				{
					int handle = m_hWrite;
					if (handle) {
						ssize_t n = ::write(handle, buf, size);
						if (n >= 0) {
							if (n > 0) {
								return (sl_int32)n;
							}
						} else {
							int err = errno;
							if (err == EAGAIN || err == EWOULDBLOCK || err == EINTR) {
								return 0;
							}
						}
					}
					close();
					return -1;
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
				pid_t m_pid;
				ProcessStream m_stream;
				
			public:
				ProcessImpl()
				{
					m_pid = -1;
				}
				
				~ProcessImpl()
				{
					if (m_pid != -1) {
						int status = 0;
						waitpid(m_pid, &status, WNOHANG | WUNTRACED | WCONTINUED);
					}
				}
				
			public:
				static Ref<ProcessImpl> create(const StringParam& pathExecutable, const StringParam* arguments, sl_size nArguments)
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
								Exec(pathExecutable, arguments, nArguments);
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
			
#endif
			
		}
	}
	
	using namespace priv::process;

	sl_uint32 Process::getCurrentProcessId()
	{
		return getpid();
	}
	
#if !defined(SLIB_PLATFORM_IS_MOBILE)
	Ref<Process> Process::open(const StringParam& pathExecutable, const StringParam* arguments, sl_size nArguments)
	{
		return Ref<Process>::from(ProcessImpl::create(pathExecutable, arguments, nArguments));
	}
#endif
	
#if !defined(SLIB_PLATFORM_IS_MOBILE) && !defined(SLIB_PLATFORM_IS_MACOS)
	Ref<Process> Process::run(const StringParam& pathExecutable, const StringParam* arguments, sl_size nArguments)
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

			Exec(pathExecutable, arguments, nArguments);
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

	void Process::runAsAdmin(const StringParam& pathExecutable, const StringParam* arguments, sl_size nArguments)
	{
		String command;
		if (File::isFile("/usr/bin/pkexec")) {
			command = "/usr/bin/pkexec";
		} else if (File::isFile("/usr/bin/kdesu")) {
			command = "/usr/bin/kdesu";
		} else if (File::isFile("/usr/bin/gksu")) {
			command = "/usr/bin/gksu";
		} else {
			return;
		}
		List<StringParam> list;
		list.add_NoLock(pathExecutable.toString());
		list.addElements_NoLock(arguments, nArguments);
		Ref<Process> process = open(command, list.getData(), list.getCount());
		if (process.isNotNull()) {
			process->wait();
		}
	}
#endif

#if !defined(SLIB_PLATFORM_IS_MOBILE)
	sl_bool Process::isAdmin()
	{
		return geteuid() == 0;
	}
#endif
	
#if !defined(SLIB_PLATFORM_IS_MOBILE)
	void Process::exec(const StringParam& pathExecutable, const StringParam* arguments, sl_size nArguments)
	{
		Exec(pathExecutable, arguments, nArguments);
	}
	
	void Process::exit(int code)
	{
		::exit(code);
	}
#endif
	
}

#endif
