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

#ifndef CHECKHEADER_SLIB_SYSTEM_PROCESS
#define CHECKHEADER_SLIB_SYSTEM_PROCESS

#include "definition.h"

#include "../core/object.h"
#include "../core/string.h"
#include "../core/flags.h"

namespace slib
{

	enum class ProcessStatus
	{
		Running = 0,
		Exited = 1,
		Terminated = 2,
		Killed = 3,
		Unknown = 4
	};

	class IStream;

	SLIB_DEFINE_FLAGS(ProcessFlags, {
		HideWindow = 1
	})

	class SLIB_EXPORT Process : public Object
	{
		SLIB_DECLARE_OBJECT

	protected:
		Process();

		~Process();

	public:
		static sl_uint32 getCurrentProcessId();

		static List<sl_uint32> getAllProcessIds();

		static List<sl_uint32> getAllThreadIds(sl_uint32 processId = 0);

		static String getImagePath(sl_uint32 processId);

		static sl_bool is32BitProcess(sl_uint32 processId);

		static sl_bool kill(sl_uint32 processId);

		static sl_bool quit(sl_uint32 processId);


		static Ref<Process> openBy(const StringParam& pathExecutable, const StringParam& commandLine, const ProcessFlags& flags = 0);

		static Ref<Process> openBy(const StringParam& pathExecutable, const StringParam* args, sl_size nArgs, const ProcessFlags& flags = 0);

		static Ref<Process> open(const StringParam& pathExecutable, const ProcessFlags& flags = 0);

		static Ref<Process> open(const StringParam& pathExecutable, const StringParam& arg);

		template <class ARG, class... ARGS>
		static Ref<Process> open(const StringParam& pathExecutable, const ProcessFlags& flags, ARGS&&... args)
		{
			StringParam params[] = { Forward<ARGS>(args)... };
			return openBy(pathExecutable, params, sizeof...(args), flags);
		}

		template <class ARG, class... ARGS>
		static Ref<Process> open(const StringParam& pathExecutable, const StringParam& arg, ARGS&&... args)
		{
			StringParam params[] = { arg, Forward<ARGS>(args)... };
			return openBy(pathExecutable, params, sizeof...(args) + 1);
		}

		static Ref<Process> runBy(const StringParam& pathExecutable, const StringParam& commandLine, const ProcessFlags& flags = 0);

		static Ref<Process> runBy(const StringParam& pathExecutable, const StringParam* args, sl_size nArgs, const ProcessFlags& flags = 0);

		static Ref<Process> run(const StringParam& pathExecutable, const ProcessFlags& flags = 0);

		static Ref<Process> run(const StringParam& pathExecutable, const StringParam& arg);

		template <class... ARGS>
		static Ref<Process> run(const StringParam& pathExecutable, const ProcessFlags& flags, ARGS&&... args)
		{
			StringParam params[] = { Forward<ARGS>(args)... };
			return runBy(pathExecutable, params, sizeof...(args), flags);
		}

		template <class... ARGS>
		static Ref<Process> run(const StringParam& pathExecutable, const StringParam& arg, ARGS&&... args)
		{
			StringParam params[] = { arg, Forward<ARGS>(args)... };
			return runBy(pathExecutable, params, sizeof...(args) + 1);
		}

		static void runAsAdminBy(const StringParam& pathExecutable, const StringParam& commandLine);

		static void runAsAdminBy(const StringParam& pathExecutable, const StringParam* args, sl_size nArgs);

		static void runAsAdmin(const StringParam& pathExecutable);

		template <class... ARGS>
		static void runAsAdmin(const StringParam& pathExecutable, ARGS&&... args)
		{
			StringParam params[] = { Forward<ARGS>(args)... };
			return runAsAdminBy(pathExecutable, params, sizeof...(args));
		}

		// check administrative privilege (effective root user on Unix)
		static sl_bool isCurrentProcessAdmin();

		static sl_bool isCurrentProcessInAdminGroup();

		static String getOutputBy(const StringParam& pathExecutable, const StringParam& commandLine, const ProcessFlags& flags = 0);

		static String getOutputBy(const StringParam& pathExecutable, const StringParam* args, sl_size nArgs, const ProcessFlags& flags = 0);

		static String getOutput(const StringParam& pathExecutable, const ProcessFlags& flags = 0);

		static String getOutput(const StringParam& pathExecutable, const StringParam& arg);

		template <class... ARGS>
		static String getOutput(const StringParam& pathExecutable, const ProcessFlags& flags, ARGS&&... args)
		{
			StringParam params[] = { Forward<ARGS>(args)... };
			return getOutputBy(pathExecutable, params, sizeof...(args), flags);
		}

		template <class... ARGS>
		static String getOutput(const StringParam& pathExecutable, const StringParam& arg, ARGS&&... args)
		{
			StringParam params[] = { arg, Forward<ARGS>(args)... };
			return getOutputBy(pathExecutable, params, sizeof...(args) + 1);
		}

		static void runCommand(const StringParam& command, const ProcessFlags& flags = 0);

		// replace current process context
		static void execBy(const StringParam& pathExecutable, const StringParam& commandLine);

		// replace current process context
		static void execBy(const StringParam& pathExecutable, const StringParam* args, sl_size nArgs);

		// replace current process context
		static void exec(const StringParam& pathExecutable);

		// replace current process context
		template <class... ARGS>
		static void exec(const StringParam& pathExecutable, ARGS&&... args)
		{
			StringParam params[] = { Forward<ARGS>(args)... };
			return execBy(pathExecutable, params, sizeof...(args));
		}

		static void exit(int code);

		static void abort();

		static void setAppNapEnabled(sl_bool flag);

	public:
		virtual void terminate() = 0;

		virtual void kill() = 0;

		virtual void wait() = 0;

		virtual sl_bool isAlive() = 0;

		virtual IStream* getStream() = 0;

	public:
		ProcessStatus getStatus();

		int getExitStatus();

	protected:
		ProcessStatus m_status;
		int m_exitStatus;

	};

}

#endif

