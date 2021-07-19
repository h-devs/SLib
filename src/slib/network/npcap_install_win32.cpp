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

#include "slib/network/definition.h"

#if defined(SLIB_PLATFORM_IS_WIN32)

#include "slib/network/npcap.h"

#include "slib/core/process.h"
#include "slib/core/system.h"
#include "slib/core/memory.h"
#include "slib/core/file_util.h"
#include "slib/core/platform.h"
#include "slib/crypto/zlib.h"

#include "npcap/npcap_files.h"

#define DRIVER_NAME "NPCAP"

namespace slib
{

	namespace priv
	{
		namespace npcap
		{

			static sl_bool InstallDriver()
			{
				if (ServiceManager::isRunning(DRIVER_NAME)) {
					return sl_true;
				}
				if (!(Process::isCurrentProcessAdmin())) {
					return sl_false;
				}

				ServiceManager::setStartType(DRIVER_NAME, ServiceStartType::Auto);
				if (ServiceManager::start(DRIVER_NAME)) {
					return sl_true;
				}

				String path = System::getTempDirectory() + "\\.npcap";
				File::createDirectory(path);
				if (!(File::isDirectory(path))) {
					return sl_false;
				}

				sl_bool flagWin10 = System::getMajorVersion() >= 10;
				unsigned char* npcap_sys_compressed_data64 = flagWin10 ? ::npcap::files::npcap_win10_sys_compressed_data64 : ::npcap::files::npcap_win7_sys_compressed_data64;
				unsigned long npcap_sys_compressed_size64 = flagWin10 ? ::npcap::files::npcap_win10_sys_compressed_size64 : ::npcap::files::npcap_win7_sys_compressed_size64;
#ifdef SLIB_PLATFORM_IS_WIN64
				unsigned char* npcap_inf_compressed_data = ::npcap::files::npcap_inf_compressed_data64;
				unsigned long npcap_inf_compressed_size = ::npcap::files::npcap_inf_compressed_size64;
				unsigned char* npcap_sys_compressed_data = npcap_sys_compressed_data64;
				unsigned long npcap_sys_compressed_size = npcap_sys_compressed_size64;
				unsigned char* npcap_cat_compressed_data = ::npcap::files::npcap_cat_compressed_data64;
				unsigned long npcap_cat_compressed_size = ::npcap::files::npcap_cat_compressed_size64;
				unsigned char* npfinstall_exe_compressed_data = ::npcap::files::npfinstall_exe_compressed_data64;
				unsigned long npfinstall_exe_compressed_size = ::npcap::files::npfinstall_exe_compressed_size64;
#else
				sl_bool flag64Bit = System::is64BitSystem();
				unsigned char* npcap_inf_compressed_data = flag64Bit ? ::npcap::files::npcap_inf_compressed_data64 : ::npcap::files::npcap_inf_compressed_data86;
				unsigned long npcap_inf_compressed_size = flag64Bit ? ::npcap::files::npcap_inf_compressed_size64 : ::npcap::files::npcap_inf_compressed_size86;
				unsigned char* npcap_sys_compressed_data = flag64Bit ? npcap_sys_compressed_data64 : ::npcap::files::npcap_sys_compressed_data86;
				unsigned long npcap_sys_compressed_size = flag64Bit ? npcap_sys_compressed_size64 : ::npcap::files::npcap_sys_compressed_size86;
				unsigned char* npcap_cat_compressed_data = flag64Bit ? ::npcap::files::npcap_cat_compressed_data64 : ::npcap::files::npcap_cat_compressed_data86;
				unsigned long npcap_cat_compressed_size = flag64Bit ? ::npcap::files::npcap_cat_compressed_size64 : ::npcap::files::npcap_cat_compressed_size86;
				unsigned char* npfinstall_exe_compressed_data = flag64Bit ? ::npcap::files::npfinstall_exe_compressed_data64 : ::npcap::files::npfinstall_exe_compressed_data86;
				unsigned long npfinstall_exe_compressed_size = flag64Bit ? ::npcap::files::npfinstall_exe_compressed_size64 : ::npcap::files::npfinstall_exe_compressed_size86;
#endif
				{
					Memory data = Zlib::decompress(npcap_inf_compressed_data, npcap_inf_compressed_size);
					if (File::writeAllBytes(path + "\\npcap.inf", data) != data.getSize()) {
						return sl_false;
					}
				}
				{
					Memory data = Zlib::decompress(npcap_sys_compressed_data, npcap_sys_compressed_size);
					if (File::writeAllBytes(path + "\\npcap.sys", data) != data.getSize()) {
						return sl_false;
					}
				}
				{
					Memory data = Zlib::decompress(npcap_cat_compressed_data, npcap_cat_compressed_size);
					if (File::writeAllBytes(path + "\\npcap.cat", data) != data.getSize()) {
						return sl_false;
					}
				}
				{
					Memory data = Zlib::decompress(npfinstall_exe_compressed_data, npfinstall_exe_compressed_size);
					if (File::writeAllBytes(path + "\\npfinstall.exe", data) != data.getSize()) {
						return sl_false;
					}
				}
				String output = Process::getOutput(path + "\\npfinstall.exe", "-i");
				if (output.contains("successfully installed")) {
					ServiceManager::setStartType(DRIVER_NAME, ServiceStartType::Auto);
					if (ServiceManager::start(DRIVER_NAME)) {
						return sl_true;
					}
				}
				return sl_false;
			}

			static sl_bool UninstallDriver()
			{
				if (!(ServiceManager::isExisting(DRIVER_NAME))) {
					return sl_true;
				}
				if (!(Process::isCurrentProcessAdmin())) {
					return sl_false;
				}

				String path = System::getTempDirectory() + "\\.npcap";
				File::createDirectory(path);
				if (!(File::isDirectory(path))) {
					return sl_false;
				}

#ifdef SLIB_PLATFORM_IS_WIN64
				unsigned char* npfinstall_exe_compressed_data = ::npcap::files::npfinstall_exe_compressed_data64;
				unsigned long npfinstall_exe_compressed_size = ::npcap::files::npfinstall_exe_compressed_size64;
#else
				sl_bool flag64Bit = System::is64BitSystem();
				unsigned char* npfinstall_exe_compressed_data = flag64Bit ? ::npcap::files::npfinstall_exe_compressed_data64 : ::npcap::files::npfinstall_exe_compressed_data86;
				unsigned long npfinstall_exe_compressed_size = flag64Bit ? ::npcap::files::npfinstall_exe_compressed_size64 : ::npcap::files::npfinstall_exe_compressed_size86;
#endif
				{
					Memory data = Zlib::decompress(npfinstall_exe_compressed_data, npfinstall_exe_compressed_size);
					if (File::writeAllBytes(path + "\\npfinstall.exe", data) != data.getSize()) {
						return sl_false;
					}
				}
				String output = Process::getOutput(path + "\\npfinstall.exe", "-u");
				return output.contains("successfully uninstalled");
			}

		}
	}

	using namespace priv::npcap;

	sl_bool Npcap::install()
	{
		return InstallDriver();
	}

	sl_bool Npcap::uninstall()
	{
		return UninstallDriver();
	}
}

#endif
