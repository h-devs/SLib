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

#if defined(SLIB_PLATFORM_IS_MACOS)

#include "slib/system/process.h"

#include "slib/core/thread.h"
#include "slib/core/string_buffer.h"
#include "slib/platform.h"
#include "slib/dl/macos/security_framework.h"

#include <sys/sysctl.h>
#include <libproc.h>

#define MAX_ARGUMENT_COUNT 128

namespace slib
{

	List<sl_uint32> Process::getAllProcessIds()
	{
		List<sl_uint32> ret;
		int mib[4] = {CTL_KERN, KERN_PROC, KERN_PROC_ALL, 0};
		size_t size = 0;
		if (!(sysctl(mib, (int)(CountOfArray(mib)), NULL, &size, NULL, 0))) {
			kinfo_proc* processes = (kinfo_proc*)(Base::createMemory(size));
			if (processes) {
				if (!(sysctl(mib, (int)(CountOfArray(mib)), processes, &size, NULL, 0))) {
					int n = (int)(size / sizeof(struct kinfo_proc));
					for (int i = 0; i < n; i++) {
						ret.add_NoLock((sl_uint32)(processes[i].kp_proc.p_pid));
					}
				}
				Base::freeMemory(processes);
			}
		}
		return ret;
	}

	List<sl_uint32> Process::getAllThreadIds(sl_uint32 processId)
	{
		List<sl_uint32> ret;
		task_t task;
		if (task_for_pid(mach_task_self(), (int)processId, &task) == KERN_SUCCESS) {
			thread_act_array_t threads = sl_null;
			mach_msg_type_number_t nThreads = 0;
			if (task_threads(task, &threads, &nThreads) == KERN_SUCCESS) {
				for (mach_msg_type_number_t i = 0; i < nThreads; i++) {
					ret.add_NoLock(threads[i]);
				}
				vm_deallocate(mach_task_self(), (vm_address_t)threads, nThreads * sizeof(thread_act_t));
			}
		}
		return ret;
	}

	String Process::getImagePath(sl_uint32 processId)
	{
		char buf[PROC_PIDPATHINFO_MAXSIZE];
		if (proc_pidpath(processId, buf, sizeof(buf)) > 0) {
			return String::fromUtf8(buf, Base::getStringLength(buf, sizeof(buf)));
		}
		return sl_null;
	}

	sl_bool Process::is32BitProcess(sl_uint32 processId)
	{
		return sl_false;
	}

	namespace
	{
		static String FixAppleScriptArgument(const StringParam& arg)
		{
			String s = arg.toString();
			s = s.removeAll('\\');
			s = s.removeAll('"');
			s = s.removeAll('\'');
			if (s.contains(' ') || s.contains('\t') || s.contains('\r') || s.contains('\n')) {
				s = String::concat("'", s, "'");
			}
			if (s.isEmpty()) {
				return "''";
			}
			return s;
		}

		static String BuildAppleScriptCommand(const ProcessParam& param)
		{
			StringBuffer commandLine;
			commandLine.add(FixAppleScriptArgument(param.executable.toString()));
			ListElements<StringParam> arguments(param.arguments);
			if (arguments.count > 0) {
				commandLine.addStatic(" ");
			}
			for (sl_size i = 0; i < arguments.count; i++) {
				if (i > 0) {
					commandLine.addStatic(" ");
				}
				commandLine.add(FixAppleScriptArgument(arguments[i].toString()));
			}
			return commandLine.merge();
		}

		static void RunAsAdminByAppleScript(const ProcessParam& param)
		{
			String command = BuildAppleScriptCommand(param);
			Apple::runAppleScript(String::concat("do shell script \"", command, "\" with administrator privileges"));
		}

		sl_bool RunAsAdminByAuthorization(const ProcessParam& param)
		{
			sl_bool bRet = sl_false;
			AuthorizationRef authorizationRef;
			OSStatus status = AuthorizationCreate(NULL, kAuthorizationEmptyEnvironment, kAuthorizationFlagDefaults, &authorizationRef);
			if (status == errAuthorizationSuccess) {
				param.prepareArgumentList();
				StringCstr path(param.executable);
				StringCstr _args[MAX_ARGUMENT_COUNT];
				char* args[MAX_ARGUMENT_COUNT + 1];
				{
					ListElements<StringParam> list(param.arguments);
					if (list.count > MAX_ARGUMENT_COUNT) {
						list.count = MAX_ARGUMENT_COUNT;
					}
					for (sl_size i = 0; i < list.count; i++) {
						_args[i] = list[i];
						args[i] = _args[i].getData();
					}
					args[list.count] = 0;
				}
				if (param.flags & ProcessFlags::InheritConsole) {
					FILE* pipe = sl_null;
					status = AuthorizationExecuteWithPrivileges(authorizationRef, path.getData(), kAuthorizationFlagDefaults, args, &pipe);
					if (status == errAuthorizationSuccess) {
						if (pipe) {
							char buf[256];
							for (;;) {
								size_t len = fread(buf, 1, sizeof(buf), pipe);
								if (!len) {
									break;
								}
								fwrite(buf, 1, len, stdout);
							}
							fclose(pipe);
						}
						bRet = sl_true;
					}
				} else {
					status = AuthorizationExecuteWithPrivileges(authorizationRef, path.getData(), kAuthorizationFlagDefaults, args, sl_null);
					if (status == errAuthorizationSuccess) {
						bRet = sl_true;
					}
				}
				AuthorizationFree(authorizationRef, kAuthorizationFlagDefaults);
			}
			return bRet;
		}

		void RunAsAdmin(const ProcessParam& param)
		{
			if (!(param.flags & ProcessFlags::UseAppleScript)) {
				if (AuthorizationExecuteWithPrivileges) {
					RunAsAdminByAuthorization(param);
					return;
				}
			}
			RunAsAdminByAppleScript(param);
		}
	}

	void Process::runAsAdmin(const ProcessParam& param)
	{
		if (param.flags & ProcessFlags::NoWait) {
			Ref<Event> ev = Event::create();
			Thread::start([ev, param]() {
				ev->set();
				RunAsAdmin(param);
			});
			ev->wait(10000);
			Thread::sleep(300);
		} else {
			RunAsAdmin(param);
		}
	}

	void Process::setAppNapEnabled(sl_bool flag)
	{
		static id activityDisableAppNap = nil;
		if (flag) {
			if (activityDisableAppNap == nil) {
				return;
			}
			[[NSProcessInfo processInfo] endActivity:activityDisableAppNap];
			activityDisableAppNap = nil;
		} else {
			if (activityDisableAppNap != nil) {
				return;
			}
			activityDisableAppNap = [[NSProcessInfo processInfo] beginActivityWithOptions:NSActivityUserInitiated reason:@"Prevent App Nap"];
		}
	}

}

#endif
