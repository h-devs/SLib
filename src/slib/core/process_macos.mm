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

#if defined(SLIB_PLATFORM_IS_MACOS)

#include "slib/core/process.h"

#include "slib/core/app.h"
#include "slib/core/string_buffer.h"
#include "slib/core/platform.h"

namespace slib
{
	namespace priv
	{
		namespace process
		{
        
			static String FixArgument(const StringParam& arg)
			{
				String s = arg.toString();
				s = s.removeAll('\\');
				s = s.removeAll('"');
				s = s.removeAll('\'');
				if (s.contains(' ') || s.contains('\t') || s.contains('\r') || s.contains('\n')) {
					s = String::join("'", s, "'");
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
	}

	using namespace priv::process;

	Ref<Process> Process::run(const StringParam& pathExecutable, const StringParam* strArguments, sl_size nArguments)
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

	void Process::runAsAdmin(const StringParam& pathExecutable, const StringParam* arguments, sl_size nArguments)
	{
		String command = BuildCommand(pathExecutable, arguments, nArguments);
		String source = String::join("do shell script \"", command, "\" with administrator privileges");
		NSAppleScript* script = [[NSAppleScript alloc] initWithSource:(Apple::getNSStringFromString(source))];
		[script executeAndReturnError:nil];
	}
	
}

#endif
