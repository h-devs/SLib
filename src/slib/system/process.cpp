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

#include "slib/system/process.h"

#include "slib/io/io.h"
#include "slib/system/system.h"
#include "slib/core/list.h"
#include "slib/core/memory.h"
#include "slib/core/command_line.h"
#include "slib/core/scoped_buffer.h"

namespace slib
{

	SLIB_DEFINE_OBJECT(Process, Object)

	Process::Process(): m_status(ProcessStatus::Running), m_exitStatus(-1)
	{
	}

	Process::~Process()
	{
	}

	ProcessStatus Process::getStatus()
	{
		return m_status;
	}

	int Process::getExitStatus()
	{
		return m_exitStatus;
	}

	Ref<Process> Process::open(const StringParam& pathExecutable, const ProcessFlags& flags)
	{
		return openBy(pathExecutable, sl_null, 0, flags);
	}

	Ref<Process> Process::open(const StringParam& pathExecutable, const StringParam& arg)
	{
		return openBy(pathExecutable, &arg, 1);
	}

	Ref<Process> Process::run(const StringParam& pathExecutable, const ProcessFlags& flags)
	{
		return runBy(pathExecutable, sl_null, 0, flags);
	}

	Ref<Process> Process::run(const StringParam& pathExecutable, const StringParam& arg)
	{
		return runBy(pathExecutable, &arg, 1);
	}

	void Process::runAsAdmin(const StringParam& pathExecutable)
	{
		runAsAdminBy(pathExecutable, sl_null);
	}

	String Process::getOutputBy(const StringParam& pathExecutable, const StringParam& commandLine, const ProcessFlags& flags)
	{
		Ref<Process> process = openBy(pathExecutable, commandLine, flags);
		if (process.isNotNull()) {
			IStream* stream = process->getStream();
			if (stream) {
				Memory mem = stream->readFully();
				return String::fromMemory(mem);
			}
		}
		return sl_null;
	}

	String Process::getOutputBy(const StringParam& pathExecutable, const StringParam* args, sl_size nArgs, const ProcessFlags& flags)
	{
		Ref<Process> process = openBy(pathExecutable, args, nArgs, flags);
		if (process.isNotNull()) {
			IStream* stream = process->getStream();
			if (stream) {
				Memory mem = stream->readFully();
				return String::fromMemory(mem);
			}
		}
		return sl_null;
	}

	String Process::getOutput(const StringParam& pathExecutable, const ProcessFlags& flags)
	{
		return getOutputBy(pathExecutable, sl_null, 0, flags);
	}

	String Process::getOutput(const StringParam& pathExecutable, const StringParam& arg)
	{
		return getOutputBy(pathExecutable, &arg, 1);
	}

	void Process::runCommand(const StringParam& command, const ProcessFlags& flags)
	{
#ifdef SLIB_PLATFORM_IS_WIN32
		runBy(System::getSystemDirectory() + "\\cmd.exe", String::concat("/C ", command), flags);
#else
		run("/bin/sh", flags, "-c", command);
#endif
	}

	void Process::execBy(const StringParam& pathExecutable, const StringParam& commandLine)
	{
		ListElements<String> args(CommandLine::parse(commandLine));
		if (!(args.count)) {
			execBy(pathExecutable, sl_null, 0);
			return;
		}
		SLIB_SCOPED_BUFFER(StringParam, 64, params, args.count)
		for (sl_size i = 0; i < args.count; i++) {
			params[i] = args[i];
		}
		execBy(pathExecutable, params, args.count);
	}

	void Process::exec(const StringParam& pathExecutable)
	{
		execBy(pathExecutable, sl_null);
	}

#if !defined(SLIB_PLATFORM_IS_MACOS)
	void Process::setAppNapEnabled(sl_bool flag)
	{
	}
#endif

}
