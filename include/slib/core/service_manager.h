/*
 *   Copyright (c) 2008-2020 SLIBIO <https://github.com/SLIBIO>
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

#ifndef CHECKHEADER_SLIB_CORE_SERVICE_MANAGER
#define CHECKHEADER_SLIB_CORE_SERVICE_MANAGER

#include "string.h"
#include "list.h"

namespace slib
{

	enum class ServiceType
	{
		Generic = 0,
		Driver = 1,
		FileSystem = 2, // [Win32] File system driver
		Shared = 3 // [Win32] shares a process with one or more other services
	};

	enum class ServiceStartType
	{
		Manual = 0,
		Auto = 1,
		Boot = 2, // [Win32] started by the system loader, valid only for driver services
		System = 3, // [Win32] started by the `IoInitSystem` function, valid only for driver services
		Disabled = 4
	};

	enum class ServiceErrorControl
	{
		Normal = 0,
		Ignore = 1,
		Critical = 2, // [Win32] The startup program logs the error in the event log, if possible. If the last-known-good configuration is being started, the startup operation fails. Otherwise, the system is restarted with the last-known good configuration.
		Severe = 3 // [Win32] The startup program logs the error in the event log. If the last-known-good configuration is being started, the startup operation continues. Otherwise, the system is restarted with the last-known-good configuration.
	};

	enum class ServiceState
	{
		None = 0,
		Running = 1,
		Paused = 2,
		Stopped = 3,
		StartPending = 0x11,
		PausePending = 0x12,
		StopPending = 0x13,
		ContinuePending = 0x14
	};

	class SLIB_EXPORT ServiceCreateParam
	{
	public:
		StringParam name;
		StringParam displayName;
		ServiceType type;
		ServiceStartType startType;
		ServiceErrorControl errorControl;
		StringParam commandLine;
		StringParam path;
		List<String> arguments;

	public:
		ServiceCreateParam();

		~ServiceCreateParam();

	};

	class SLIB_EXPORT ServiceManager
	{
	public:
		static sl_bool isExisting(const StringParam& serviceName);

		static sl_bool create(const ServiceCreateParam& param);

		static sl_bool createAndStart(const ServiceCreateParam& param, sl_int32 timeout = -1);

		static sl_bool remove(const StringParam& serviceName);

		static sl_bool stopAndRemove(const StringParam& serviceName, sl_int32 timeout = -1);

		static ServiceState getState(const StringParam& serviceName);

		static sl_bool isStarted(const StringParam& serviceName);

		static sl_bool isRunning(const StringParam& serviceName);

		static sl_bool isStopped(const StringParam& serviceName);

		static sl_bool isPaused(const StringParam& serviceName);

		static sl_bool start(const StringParam& serviceName, const String16* argv, sl_uint32 argc, sl_int32 timeout = -1);

		static sl_bool start(const StringParam& serviceName, sl_int32 timeout = -1);

		static sl_bool stop(const StringParam& serviceName, sl_int32 timeout = -1);

		static sl_bool pause(const StringParam& serviceName, sl_int32 timeout = -1);

	};
	
}

#endif
