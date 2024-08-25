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
#include "../core/hash_map.h"
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
		HideWindow = 1,
		InheritHandles = 2,
		NoWait = 4 // Used in `runAsAdmin()`
	})

	class SLIB_EXPORT ProcessParam
	{
	public:
		StringParam executable;
		ListParam<StringParam> arguments;
		StringParam argumentString;
		StringParam currentDirectory;
		HashMap<String, String> environment; // Used in `open()`, `run()`, `getOutput()`
		ProcessFlags flags;
		sl_int32 timeout; // Used in `getOutput()`

	public:
		ProcessParam();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(ProcessParam)

	public:
		void prepareArgumentString() const;

		void prepareArgumentList() const;

		void setCommand(StringParam&& command);

		template <class T>
		void setCommand(T&& command)
		{
			setCommand(StringParam(command));
		}

	};

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


		static Ref<Process> open(const ProcessParam& param);

		static Ref<Process> open(const StringParam& executable);

		template <class ARG, class... ARGS>
		static Ref<Process> open(const StringParam& executable, ARGS&&... _args)
		{
			StringParam args[] = { Forward<ARGS>(_args)... };
			ProcessParam param;
			param.executable = executable;
			param.arguments = args;
			return open(param);
		}

		static Ref<Process> run(const ProcessParam& param);

		static Ref<Process> run(const StringParam& executable);

		template <class... ARGS>
		static Ref<Process> run(const StringParam& executable, ARGS&&... _args)
		{
			StringParam args[] = { Forward<ARGS>(_args)... };
			ProcessParam param;
			param.executable = executable;
			param.arguments = args;
			return run(param);
		}

		static void runAsAdmin(const ProcessParam& param);

		static void runAsAdmin(const StringParam& executable);

		template <class... ARGS>
		static void runAsAdmin(const StringParam& executable, ARGS&&... _args)
		{
			StringParam args[] = { Forward<ARGS>(_args)... };
			ProcessParam param;
			param.executable = executable;
			param.arguments = args;
			runAsAdmin(param);
		}

		// check administrative privilege (effective root user on Unix)
		static sl_bool isCurrentProcessAdmin();

		static sl_bool isCurrentProcessInAdminGroup();

		static String getOutput(const ProcessParam& param);

		static String getOutput(const StringParam& executable);

		template <class... ARGS>
		static String getOutput(const StringParam& executable, ARGS&&... _args)
		{
			StringParam args[] = { Forward<ARGS>(_args)... };
			ProcessParam param;
			param.executable = executable;
			param.arguments = args;
			return getOutput(param);
		}

		static Ref<Process> runCommand(const StringParam& command, const ProcessFlags& flags = 0);

		static String getCommandOutput(const StringParam& command, const ProcessFlags& flags = 0, sl_int32 timeout = -1);

		// replace current process context
		static void exec(const ProcessParam& param);

		// replace current process context
		static void exec(const StringParam& executable);

		// replace current process context
		template <class... ARGS>
		static void exec(const StringParam& executable, ARGS&&... _args)
		{
			StringParam args[] = { Forward<ARGS>(_args)... };
			ProcessParam param;
			param.executable = executable;
			param.arguments = args;
			exec(param);
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

