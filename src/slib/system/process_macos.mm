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

#include "slib/core/app.h"
#include "slib/core/string_buffer.h"
#include "slib/core/command_line.h"
#include "slib/core/scoped_buffer.h"
#include "slib/platform.h"

#include <sys/sysctl.h>
#include <libproc.h>

namespace slib
{
	namespace {

		static id g_activityDisableAppNap;
	
		static String FixArgument(const StringParam& arg)
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

		static String BuildCommand(const StringParam& pathExecutable, const StringParam* arguments, sl_size nArguments)
		{
			StringBuffer commandLine;
			commandLine.add(FixArgument(pathExecutable.toString()));
			if (nArguments > 0) {
				commandLine.addStatic(" ");
			}
			for (sl_size i = 0; i < nArguments; i++) {
				if (i > 0) {
					commandLine.addStatic(" ");
				}
				commandLine.add(FixArgument(arguments[i]));
			}
			return commandLine.merge();
		}

		class TaskProcessImpl : public Process
		{
		public:
			NSTask* m_task;

		public:
			void terminate() override
			{
				[m_task terminate];
				m_status = ProcessStatus::Terminated;
			}

			void kill() override
			{
				terminate();
			}

			void wait() override
			{
				[m_task waitUntilExit];
				int status = [m_task terminationStatus];
				if (!status) {
					m_status = ProcessStatus::Exited;
				} else {
					m_status = ProcessStatus::Unknown;
				}

			}

			sl_bool isAlive() override
			{
				return [m_task isRunning];
			}

			IStream* getStream() override
			{
				return sl_null;
			}

		};

	}

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

	Ref<Process> Process::runBy(const StringParam& pathExecutable, const StringParam& commandLine, const ProcessFlags& flags)
	{
		ListElements<String> args(CommandLine::parse(commandLine));
		if (!(args.count)) {
			return runBy(pathExecutable, sl_null, 0);
		}
		SLIB_SCOPED_BUFFER(StringParam, 64, params, args.count)
		for (sl_size i = 0; i < args.count; i++) {
			params[i] = args[i];
		}
		return runBy(pathExecutable, params, args.count, flags);
	}

	Ref<Process> Process::runBy(const StringParam& pathExecutable, const StringParam* strArguments, sl_size nArguments, const ProcessFlags& flags)
	{
		@try {
			NSMutableArray* arguments = [NSMutableArray array];
			for (sl_uint32 i = 0; i < nArguments; i++) {
				[arguments addObject:(Apple::getNSStringFromString(strArguments[i]))];
			}
			NSTask* task = [NSTask launchedTaskWithLaunchPath:(Apple::getNSStringFromString(pathExecutable)) arguments:arguments];
			if (task != nil) {
				Ref<TaskProcessImpl> ret = new TaskProcessImpl;
				if (ret.isNotNull()) {
					ret->m_task = task;
					return ret;
				}
			}
		} @catch (NSException* e) {
#ifdef SLIB_DEBUG
			NSLog(@"Error at run process: %@\n%@", Apple::getNSStringFromString(pathExecutable), e.debugDescription);
#endif
		}
		return sl_null;
	}

	void Process::runAsAdminBy(const StringParam& pathExecutable, const StringParam& commandLine)
	{
		Apple::runAppleScript(String::concat("do shell script \"", FixArgument(pathExecutable.toString()), " ", commandLine, "\" with administrator privileges"));
	}

	void Process::runAsAdminBy(const StringParam& pathExecutable, const StringParam* arguments, sl_size nArguments)
	{
		String command = BuildCommand(pathExecutable, arguments, nArguments);
		Apple::runAppleScript(String::concat("do shell script \"", command, "\" with administrator privileges"));
	}

	void Process::setAppNapEnabled(sl_bool flag)
	{
		if (flag) {
			NSProcessInfo* info = [NSProcessInfo processInfo];
			if ([info performSelector:@selector(beginActivityWithOptions:reason:)]) {
				g_activityDisableAppNap = [info beginActivityWithOptions:NSActivityUserInitiated reason:@"User Request"];
			}
		} else {
			g_activityDisableAppNap = nil;
		}
	}

}

#endif
