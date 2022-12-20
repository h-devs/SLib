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

#include "slib/core/process.h"

#include "slib/core/list.h"
#include "slib/core/memory.h"
#include "slib/io/io.h"

namespace slib
{

	SLIB_DEFINE_OBJECT(Process, Object)

	Process::Process():
		m_status(ProcessStatus::Running),
		m_exitStatus(-1)
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

#if defined(SLIB_PLATFORM_IS_MOBILE)

	Ref<Process> Process::open(const StringParam& pathExecutable, const StringParam* arguments, sl_size nArguments)
	{
		return sl_null;
	}

	Ref<Process> Process::run(const StringParam& pathExecutable, const StringParam* arguments, sl_size nArguments)
	{
		return sl_null;
	}

	void Process::runAsAdmin(const StringParam& pathExecutable, const StringParam* arguments, sl_size nArguments)
	{
	}

	sl_bool Process::isCurrentProcessAdmin()
	{
		return sl_false;
	}

	void Process::exec(const StringParam& pathExecutable, const StringParam* arguments, sl_size nArguments)
	{
	}

	void Process::abort()
	{
	}

	void Process::exit(int code)
	{
	}

	String Process::getOutput(const StringParam& pathExecutable, const StringParam* args, sl_size nArgs)
	{
		return sl_null;
	}

#else

	String Process::getOutput(const StringParam& pathExecutable, const StringParam* args, sl_size nArgs)
	{
		Ref<Process> process = open(pathExecutable, args, nArgs);
		if (process.isNotNull()) {
			IStream* stream = process->getStream();
			if (stream) {
				Memory mem = stream->readFully();
				return String::fromMemory(mem);
			}
		}
		return sl_null;
	}

#endif

#if !defined(SLIB_UI_IS_MACOS)
	void Process::setAppNapEnabled(sl_bool flag)
	{
	}
#endif

}
