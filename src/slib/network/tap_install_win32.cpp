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

#include "slib/network/tap.h"

#include "slib/core/process.h"
#include "slib/core/file_util.h"
#include "slib/core/system.h"
#include "slib/core/platform.h"
#include "slib/core/win32/setup.h"
#include "slib/crypto/zlib.h"

#include "tap/tap_files.h"

#define DRIVER_NAME "tap0901"

namespace slib
{

	namespace priv
	{
		namespace tap
		{

			static sl_bool InstallDriver()
			{
				if (ServiceManager::isRunning(DRIVER_NAME)) {
					return sl_true;
				}
				if (!(Process::isAdmin())) {
					return sl_false;
				}

				String path = System::getTempDirectory() + "\\slib_tap";
				File::createDirectory(path);
				if (!(File::isDirectory(path))) {
					return sl_false;
				}

#ifdef SLIB_PLATFORM_IS_WIN64
				unsigned char* tap_inf_compressed_data = ::tap::files::tap_inf_compressed_data64;
				unsigned long tap_inf_compressed_size = ::tap::files::tap_inf_compressed_size64;
				unsigned char* tap_sys_compressed_data = ::tap::files::tap_sys_compressed_data64;
				unsigned long tap_sys_compressed_size = ::tap::files::tap_sys_compressed_size64;
				unsigned char* tap_cat_compressed_data = ::tap::files::tap_cat_compressed_data64;
				unsigned long tap_cat_compressed_size = ::tap::files::tap_cat_compressed_size64;
#else
				sl_bool flag64Bit = Win32::is64BitSystem();
				unsigned char* tap_inf_compressed_data = flag64Bit ? ::tap::files::tap_inf_compressed_data64 : ::tap::files::tap_inf_compressed_data86;
				unsigned long tap_inf_compressed_size = flag64Bit ? ::tap::files::tap_inf_compressed_size64 : ::tap::files::tap_inf_compressed_size86;
				unsigned char* tap_sys_compressed_data = flag64Bit ? ::tap::files::tap_sys_compressed_data64 : ::tap::files::tap_sys_compressed_data86;
				unsigned long tap_sys_compressed_size = flag64Bit ? ::tap::files::tap_sys_compressed_size64 : ::tap::files::tap_sys_compressed_size86;
				unsigned char* tap_cat_compressed_data = flag64Bit ? ::tap::files::tap_cat_compressed_data64 : ::tap::files::tap_cat_compressed_data86;
				unsigned long tap_cat_compressed_size = flag64Bit ? ::tap::files::tap_cat_compressed_size64 : ::tap::files::tap_cat_compressed_size86;
				if (flag64Bit) {
					Memory data = Zlib::decompress(::tap::files::tapinstall_exe_compressed_data, ::tap::files::tapinstall_exe_compressed_size);
					if (File::writeAllBytes(path + "\\tapinstall.exe", data) != data.getSize()) {
						return sl_false;
					}
				}
#endif
				{
					Memory data = Zlib::decompress(tap_inf_compressed_data, tap_inf_compressed_size);
					if (File::writeAllBytes(path + "\\tap0901.inf", data) != data.getSize()) {
						return sl_false;
					}
				}
				{
					Memory data = Zlib::decompress(tap_sys_compressed_data, tap_sys_compressed_size);
					if (File::writeAllBytes(path + "\\tap0901.sys", data) != data.getSize()) {
						return sl_false;
					}
				}
				{
					Memory data = Zlib::decompress(tap_cat_compressed_data, tap_cat_compressed_size);
					if (File::writeAllBytes(path + "\\tap0901.cat", data) != data.getSize()) {
						return sl_false;
					}
				}
#ifndef SLIB_PLATFORM_IS_WIN64
				if (flag64Bit) {
					Ref<Process> process = Process::open(path + "\\tapinstall.exe", "install", path + "\\tap0901.inf", "tap0901");
					if (process.isNotNull()) {
						IStream* stream = process->getStream();
						if (stream) {
							char buf[512];
							sl_reg n = stream->readFully(buf, sizeof(buf) - 1);
							if (n > 0) {
								if (StringView(buf).startsWith("Device node created")) {
									return sl_true;
								}
							}
						}
					}
					return sl_false;
				}
#endif
				return win32::Setup::installDriver(path + "\\tap0901.inf", DRIVER_NAME);
			}

			static sl_bool UninstallDriver()
			{
				if (!(ServiceManager::isExisting(DRIVER_NAME))) {
					return sl_true;
				}
				if (!(Process::isAdmin())) {
					return sl_false;
				}
#ifndef SLIB_PLATFORM_IS_WIN64
				if (Win32::is64BitSystem()) {
					String path = System::getTempDirectory() + "\\slib_tap";
					File::createDirectory(path);
					if (!(File::isDirectory(path))) {
						return sl_false;
					}
					Memory data = Zlib::decompress(::tap::files::tapinstall_exe_compressed_data, ::tap::files::tapinstall_exe_compressed_size);
					if (File::writeAllBytes(path + "\\tapinstall.exe", data) != data.getSize()) {
						return sl_false;
					}
					Ref<Process> process = Process::open(path + "\\tapinstall.exe", "remove", "tap0901");
					if (process.isNotNull()) {
						IStream* stream = process->getStream();
						if (stream) {
							char buf[512] = { 0 };
							sl_reg n = stream->readFully(buf, sizeof(buf) - 1);
							if (n > 0) {
								if (StringView(buf).contains(" device(s) were removed.")) {
									return sl_true;
								}
							}
						}
					}
					return sl_false;
				}
#endif
				return win32::Setup::uninstallDriver(DRIVER_NAME);
			}

		}
	}

	using namespace priv::tap;

	sl_bool Tap::install()
	{
		return InstallDriver();
	}

	sl_bool Tap::uninstall()
	{
		return UninstallDriver();
	}

}

#endif