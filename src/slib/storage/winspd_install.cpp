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

#include "slib/core/definition.h"

#ifdef SLIB_PLATFORM_IS_WIN32

#include "slib/storage/winspd.h"

#include "slib/core/file_util.h"
#include "slib/core/process.h"
#include "slib/core/system.h"
#include "slib/core/memory.h"
#include "slib/crypto/zstd.h"

#include "winspd/winspd_files.h"

#pragma comment(lib, "winspd.lib")

#define WINSPD_DRIVER_NAME "WinSpd"

namespace slib
{

	namespace priv
	{
		namespace winspd
		{

			static sl_bool InstallDriver()
			{
				ServiceState state = ServiceManager::getState(WINSPD_DRIVER_NAME);
				if (state == ServiceState::Running) {
					return sl_true;
				}
				if (!(Process::isCurrentProcessAdmin())) {
					return sl_false;
				}
				if (state != ServiceState::None) {
					if (ServiceManager::start(WINSPD_DRIVER_NAME)) {
						return sl_true;
					}
				}
				Memory dataCatalog;
				Memory dataDriver;
#ifdef SLIB_PLATFORM_IS_WIN64
				dataCatalog = Zstd::decompress(::winspd::files::winspd_cat_compressed_data64, ::winspd::files::winspd_cat_compressed_size64);
				dataDriver = Zstd::decompress(::winspd::files::winspd_sys_compressed_data64, ::winspd::files::winspd_sys_compressed_size64);
#else
				DisableWow64FsRedirectionScope scopeDisableWow64;
				sl_bool flag64Bit = System::is64BitSystem();
				dataCatalog = Zstd::decompress(flag64Bit ? ::winspd::files::winspd_cat_compressed_data64 : ::winspd::files::winspd_cat_compressed_data86, flag64Bit ? ::winspd::files::winspd_cat_compressed_size64 : ::winspd::files::winspd_cat_compressed_size86);
				dataDriver = Zstd::decompress(flag64Bit ? ::winspd::files::winspd_sys_compressed_data64 : ::winspd::files::winspd_sys_compressed_data86, flag64Bit ? ::winspd::files::winspd_sys_compressed_size64 : ::winspd::files::winspd_sys_compressed_size86);
#endif
				if (dataCatalog.isNull() || dataDriver.isNull()) {
					return sl_false;
				}
				String pathCatalog = System::getSystemDirectory() + "\\catroot\\{F750E6C3-38EE-11D1-85E5-00C04FC295EE}\\winspd.cat";
				if (File::readAllBytes(pathCatalog) != dataCatalog) {
					if (File::writeAllBytes(pathCatalog, dataCatalog) != dataCatalog.getSize()) {
						return sl_false;
					}
				}
				String pathDriver = System::getSystemDirectory() + "\\drivers\\winspd.sys";
				if (File::readAllBytes(pathDriver) != dataDriver) {
					if (File::writeAllBytes(pathDriver, dataDriver) != dataDriver.getSize()) {
						return sl_false;
					}
				}
				ServiceCreateParam param;
				param.type = ServiceType::FileSystem;
				param.startType = ServiceStartType::Auto;
				param.name = WINSPD_DRIVER_NAME;
				param.path = pathDriver;
				if (!(ServiceManager::create(param))) {
					return sl_false;
				}
				return ServiceManager::start(WINSPD_DRIVER_NAME);
			}

		}
	}

	using namespace priv::winspd;
	
	sl_bool Winspd::install()
	{
		return InstallDriver();
	}

}

#endif