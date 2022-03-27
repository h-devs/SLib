/*
 *`Copyright (c) 2008-2021 SLIBIO <https://github.com/SLIBIO>
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

#include "slib/core/cpu.h"
#include "slib/core/memory.h"

#include "slib/core/math.h"

#if defined(SLIB_COMPILER_IS_VC)
#	include <intrin.h>
#else
#	ifdef SLIB_ARCH_IS_X64
#		include <cpuid.h>
#	endif
#endif

#if defined(SLIB_PLATFORM_IS_WIN32)
#	include "slib/core/win32/windows.h"
#elif defined(SLIB_PLATFORM_IS_APPLE)
#	include <sys/sysctl.h>
#   include <mach/mach_host.h>
#elif defined(SLIB_PLATFORM_IS_LINUX)
#	include <sched.h>
#	include <sys/sysinfo.h>
#endif

namespace slib
{

	namespace priv
	{
		namespace cpu
		{

			static sl_uint32 GetCoreCount()
			{
#if defined(SLIB_PLATFORM_IS_WIN32)
				DWORD_PTR dwMaskProcess, dwMaskSystem;
				if (GetProcessAffinityMask(GetCurrentProcess(), &dwMaskProcess, &dwMaskSystem)) {
					return Math::popCount((sl_size)dwMaskSystem);
				}
#elif defined(SLIB_PLATFORM_IS_APPLE)
				int mib[2] = {CTL_HW, HW_NCPU};
				sl_uint32 n = 1;
				size_t len = sizeof(n);
				if (!(sysctl(mib, 2, &n, &len, sl_null, 0))) {
					return n;
				}
#elif defined(SLIB_PLATFORM_IS_LINUX)
				cpu_set_t set;
				CPU_ZERO(&set);
				if (!(sched_getaffinity(0, sizeof(set), &set))) {
					return (sl_uint32)(CPU_COUNT(&set));
				}
#endif
				return 1;
			}


#ifdef SLIB_ARCH_IS_X64
			static sl_bool IsSupportedSSE42() noexcept
			{
#if defined(SLIB_COMPILER_IS_VC)
				int cpu_info[4];
				__cpuid(cpu_info, 1);
				return (cpu_info[2] & (1 << 20)) != 0;
#else
				unsigned int eax, ebx, ecx, edx;
				return __get_cpuid(1, &eax, &ebx, &ecx, &edx) && ((ecx & (1 << 20)) != 0);
#endif
			}
#endif

		}
	}

	using namespace priv::cpu;

	sl_uint32 Cpu::getCoreCount()
	{
		static sl_uint32 n = GetCoreCount();
		return n;
	}

#ifdef SLIB_ARCH_IS_X64
	sl_bool Cpu::isSupportedSSE42() noexcept
	{
		static sl_bool f = IsSupportedSSE42();
		return f;
	}
#endif


	sl_bool Memory::getPhysicalMemoryStatus(PhysicalMemoryStatus& _out)
	{
#if defined(SLIB_PLATFORM_IS_WIN32)
		MEMORYSTATUSEX status;
		Base::zeroMemory(&status, sizeof(status));
		status.dwLength = sizeof(status);
		if (GlobalMemoryStatusEx(&status)) {
			_out.total = (sl_uint64)(status.ullTotalPhys);
			_out.available = (sl_uint64)(status.ullAvailPhys);
			return sl_true;
		}
#elif defined(SLIB_PLATFORM_IS_APPLE)
		int mib[2] = {CTL_HW, HW_MEMSIZE};
		sl_uint64 n = 0;
		size_t len = sizeof(n);
		if (!(sysctl(mib, 2, &n, &len, sl_null, 0))) {
			_out.total = n;
			mach_port_t port = mach_host_self();
			vm_size_t pageSize = 0;
			if (host_page_size(port, &pageSize) == KERN_SUCCESS) {
				vm_statistics64_data_t stat;
				mach_msg_type_number_t sizeStat = sizeof(stat) / sizeof(integer_t);
				if (host_statistics64(port, HOST_VM_INFO64, (host_info_t)&stat, &sizeStat) == KERN_SUCCESS) {
					_out.available = stat.free_count * pageSize;
					return sl_true;
				}
			}
		}
#elif defined(SLIB_PLATFORM_IS_LINUX)
		struct sysinfo si;
		Base::zeroMemory(&si, sizeof(si));
		si.mem_unit = 1;
		if (!(sysinfo(&si))) {
			_out.total = (sl_uint64)(si.totalram) * si.mem_unit;
			_out.available = (sl_uint64)(si.freeram) * si.mem_unit;
			return sl_true;
		}
#endif
		return sl_false;
	}

}
