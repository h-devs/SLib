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
#include "slib/core/memory.h"
#include "slib/core/command_line.h"

namespace slib
{

	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(ProcessParam)

	ProcessParam::ProcessParam()
	{
		timeout = -1;
	}

	void ProcessParam::prepareArgumentString() const
	{
		if (argumentString.isNotNull()) {
			return;
		}
		if (arguments.isNotNull()) {
			ListElements<StringParam> list(arguments);
			((ProcessParam*)this)->argumentString = CommandLine::build(list.data, list.count);
		}
	}

	void ProcessParam::prepareArgumentList() const
	{
		if (arguments.isNotNull()) {
			return;
		}
		if (argumentString.isNotNull()) {
			List<StringParam> argList;
			ListElements<String> list(CommandLine::parse(argumentString));
			for (sl_size i = 0; i < list.count; i++) {
				argList.add_NoLock(Move(list[i]));
			}
			((ProcessParam*)this)->arguments = Move(argList);
		}
	}

	void ProcessParam::setCommand(StringParam&& command)
	{
#ifdef SLIB_PLATFORM_IS_WIN32
		executable = System::getSystemDirectory() + "\\cmd.exe";
		argumentString = String::concat("/C ", command);
#else
		executable = StringView::literal("/bin/sh");
		List<StringParam> argList;
		argList.add_NoLock(StringView::literal("-c"));
		argList.add_NoLock(Move(command));
		arguments = Move(argList);
#endif
	}


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

	Ref<Process> Process::open(const StringParam& executable)
	{
		ProcessParam param;
		param.executable = executable;
		return open(param);
	}

	Ref<Process> Process::run(const StringParam& executable)
	{
		ProcessParam param;
		param.executable = executable;
		return run(param);
	}

	void Process::runAsAdmin(const StringParam& executable)
	{
		ProcessParam param;
		param.executable = executable;
		runAsAdmin(param);
	}

	String Process::getOutput(const ProcessParam& param)
	{
		Ref<Process> process = open(param);
		if (process.isNotNull()) {
			IStream* stream = process->getStream();
			if (stream) {
				Memory mem = stream->readFully(SLIB_SIZE_MAX, 0, param.timeout);
				return String::fromMemory(mem);
			}
		}
		return sl_null;
	}

	String Process::getOutput(const StringParam& executable)
	{
		ProcessParam param;
		param.executable = executable;
		return getOutput(param);
	}

	Ref<Process> Process::runCommand(const StringParam& command, const ProcessFlags& flags)
	{
		ProcessParam param;
		param.flags = flags;
		param.setCommand(command);
		return run(param);
	}

	String Process::getCommandOutput(const StringParam& command, const ProcessFlags& flags, sl_int32 timeout)
	{
		ProcessParam param;
		param.flags = flags;
		param.timeout = timeout;
		param.setCommand(command);
		return getOutput(param);
	}

	void Process::exec(const StringParam& executable)
	{
		ProcessParam param;
		param.executable = executable;
		return exec(param);
	}

#if !defined(SLIB_PLATFORM_IS_MACOS)
	void Process::setAppNapEnabled(sl_bool flag)
	{
	}
#endif

}
