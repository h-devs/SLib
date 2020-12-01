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

#include "slib/storage/dokany.h"

#ifdef SLIB_PLATFORM_IS_WIN32

#include "slib/core/service_manager.h"
#include "slib/core/file.h"
#include "slib/core/process.h"
#include "slib/core/dynamic_library.h"
#include "slib/core/platform_windows.h"
#include "slib/core/log.h"

#include "slib/crypto/zlib.h"

#include "dokany/dokany_core_files.h"

#pragma comment(lib, "dokany.lib")

namespace slib
{

	namespace priv
	{
		namespace dokany
		{

			static sl_bool IsDokanySupported()
			{
				static sl_bool flagFirst = sl_true;
				static sl_bool flagDokany = sl_false;
				if (flagFirst) {
					flagDokany = Windows::getVersion() >= WindowsVersion::Windows7_SP1;
					flagFirst = sl_false;
				}
				return flagDokany;
			}

			static String GetDriverName(sl_bool flagDokany)
			{
				if (flagDokany) {
					return "Dokan1";
				} else {
					return "Dokan";
				}
			}

			static String GetDriverPath(sl_bool flagDokany)
			{
				if (flagDokany) {
					return Windows::getSystemDirectory() + "\\drivers\\dokan1.sys";
				} else {
					return Windows::getSystemDirectory() + "\\drivers\\dokan.sys";
				}
			}

			static String GetCatalogPath(/*sl_bool flagDokany = sl_true*/)
			{
				return Windows::getSystemDirectory() + "\\catroot\\{F750E6C3-38EE-11D1-85E5-00C04FC295EE}\\dokan1.cat";
			}

			static String GetLibraryPath(sl_bool flagDokany)
			{
				if (flagDokany) {
					return Windows::getSystemDirectory() + "\\dokan1.dll";
				} else {
					return Windows::getSystemDirectory() + "\\dokan.dll";
				}
			}

			static sl_bool CheckDriver(sl_bool flagDokany)
			{
				String driverName = GetDriverName(flagDokany);
				return ServiceManager::isRunning(driverName);
			}

			static sl_bool StartMounter()
			{
				String serviceName = "DokanMounter";
				ServiceState state = ServiceManager::getState(serviceName);
				if (state == ServiceState::Running) {
					return sl_true;
				}
				if (state == ServiceState::None) {
					return sl_false;
				}
				return ServiceManager::start(serviceName);
			}

			static sl_bool StartDriver(sl_bool flagDokany)
			{
				String driverName = GetDriverName(flagDokany);
				ServiceState state = ServiceManager::getState(driverName);
				if (state == ServiceState::Running) {
					return sl_true;
				}
				if (state == ServiceState::None) {
					SLIB_LOG_DEBUG("DokanInstall", "StartDriver State: None (%s)", driverName);
					return sl_false;
				}
				if (!(ServiceManager::start(driverName))) {
					SLIB_LOG_DEBUG("DokanInstall", "StartDriver start FAILURE (%s)", driverName);
					return sl_false;
				}
				if (!flagDokany) {
					return StartMounter();
				}
				return sl_true;
			}

			static sl_bool RegisterMounter()
			{
				String serviceName = "DokanMounter";
				ServiceState state = ServiceManager::getState(serviceName);
				if (state != ServiceState::None) {
					return sl_true;
				}
				Memory data;
#ifdef SLIB_PLATFORM_IS_WIN64
				data = Zlib::decompress(::dokany::files::dokan_mounter_compressed_data, ::dokany::files::dokan_mounter_compressed_size);
#else
				DisableWow64FsRedirectionScope scopeDisableWow64;
				if (Windows::is64BitSystem()) {
					data = Zlib::decompress(::dokany::files::dokan_mounter_compressed_data64, ::dokany::files::dokan_mounter_compressed_size64);
				} else {
					data = Zlib::decompress(::dokany::files::dokan_mounter_compressed_data, ::dokany::files::dokan_mounter_compressed_size);
				}
#endif
				if (data.isNull()) {
					return sl_false;
				}
				String path = Windows::getSystemDirectory() + "\\mounter.exe";
				if (File::writeAllBytes(path, data) != data.getSize()) {
					return sl_false;
				}
				ServiceCreateParam param;
				//param.type = ServiceType::FileSystem;
				param.startType = ServiceStartType::Auto;
				param.name = serviceName;
				param.path = path;
				if (!(ServiceManager::create(param))) {
					return sl_false;
				}
				return sl_true;
			}

			static sl_bool CheckLibrary(sl_bool flagDokany)
			{
				String pathLib = GetLibraryPath(flagDokany);
				if (!(File::exists(pathLib))) {
					return sl_false;
				}
				void* funcGetVersion = sl_null;
				void* lib = DynamicLibrary::loadLibrary(pathLib);
				if (lib) {
					funcGetVersion = DynamicLibrary::getFunctionAddress(lib, "DokanVersion");
					DynamicLibrary::freeLibrary(lib);
				}
				if (!funcGetVersion) {
					return sl_false;
				}
				return sl_true;
			}

			static sl_bool InstallLibrary(sl_bool flagDokany)
			{
				if (CheckLibrary(flagDokany)) {
					return sl_true;
				}
				if (!(Process::isAdmin())) {
					return sl_false;
				}
				Memory data;
#ifdef SLIB_PLATFORM_IS_WIN64
				if (flagDokany) {
					data = Zlib::decompress(::dokany::files::dokan1_dll_compressed_data, ::dokany::files::dokan1_dll_compressed_size);
				} else {
					data = Zlib::decompress(::dokany::files::dokan_dll_compressed_data, ::dokany::files::dokan_dll_compressed_size);
				}
#else
				if (flagDokany) {
					data = Zlib::decompress(::dokany::files::dokan1_dll_compressed_data, ::dokany::files::dokan1_dll_compressed_size);
				} else {
					data = Zlib::decompress(::dokany::files::dokan_dll_compressed_data, ::dokany::files::dokan_dll_compressed_size);
				}
#endif
				if (data.isNull()) {
					return sl_false;
				}
				String path = GetLibraryPath(flagDokany);
				if (File::writeAllBytes(path, data) != data.getSize()) {
					return sl_false;
				}
				return CheckLibrary(flagDokany);
			}

			static sl_bool RegisterCatalog(/*sl_bool flagDokany = sl_true*/)
			{
				// Correct way of catalog installation using SignTool
				//   signtool catdb dokan1.cat

				// Correct way of catalog installation using WinTrust API:
				// 
				//#include <SoftPub.h>
				////#include <mscat.h>
				//#include "dl_windows_wintrust.h"
				// 
				// HCATADMIN catAdmin;
				// GUID catRootGuid = DRIVER_ACTION_VERIFY;
				// if (!(CryptCATAdminAcquireContext(&catAdmin, &catRootGuid, NULL))) {
				// 	return sl_false;
				// }
				// HCATINFO catInfo = CryptCATAdminAddCatalog(catAdmin, 
				//		L"C:\\Windows\\System32\\DRVSTORE\\dokan_E30037CFB60886C502594F6D325117CE8637F427\\dokan1.cat", L"dokan1.cat", NULL);
				// if (catInfo == NULL) {
				// 	CryptCATAdminReleaseContext(catAdmin, NULL);
				// 	return sl_false;
				// }
				// CryptCATAdminReleaseCatalogContext(catAdmin, catInfo, NULL);
				// CryptCATAdminReleaseContext(catAdmin, NULL);

				Memory data = Zlib::decompress(::dokany::files::dokan1_cat_compressed_data, ::dokany::files::dokan1_cat_compressed_size);
				String path = GetCatalogPath();
				if (File::writeAllBytes(path, data) != data.getSize()) {
					return sl_false;
				}
				return sl_true;
			}

			static sl_bool RegisterDriver(sl_bool flagDokany)
			{
				String driverName = GetDriverName(flagDokany);
				ServiceState state = ServiceManager::getState(driverName);
				if (state != ServiceState::None) {
					return sl_true;
				}
				Memory data;
#ifdef SLIB_PLATFORM_IS_WIN64
				if (flagDokany) {
					data = Zlib::decompress(::dokany::files::dokan1_sys_compressed_data, ::dokany::files::dokan1_sys_compressed_size);
				} else {
					data = Zlib::decompress(::dokany::files::dokan_sys_compressed_data, ::dokany::files::dokan_sys_compressed_size);
				}
#else
				DisableWow64FsRedirectionScope scopeDisableWow64;
				if (Windows::is64BitSystem()) {
					if (flagDokany) {
						data = Zlib::decompress(::dokany::files::dokan1_sys_compressed_data64, ::dokany::files::dokan1_sys_compressed_size64);
					} else {
						data = Zlib::decompress(::dokany::files::dokan_sys_compressed_data64, ::dokany::files::dokan_sys_compressed_size64);
					}
				} else {
					if (flagDokany) {
						data = Zlib::decompress(::dokany::files::dokan1_sys_compressed_data, ::dokany::files::dokan1_sys_compressed_size);
					} else {
						data = Zlib::decompress(::dokany::files::dokan_sys_compressed_data, ::dokany::files::dokan_sys_compressed_size);
					}
				}
#endif
				if (data.isNull()) {
					return sl_false;
				}
				String path = GetDriverPath(flagDokany);
				if (File::writeAllBytes(path, data) != data.getSize()) {
					return sl_false;
				}

				ServiceCreateParam param;
				param.type = ServiceType::FileSystem;
				param.startType = ServiceStartType::Auto;
				param.name = driverName;
				param.path = path;
				if (!(ServiceManager::create(param))) {
					return sl_false;
				}
				if (!flagDokany) {
					return RegisterMounter();
				}
				return sl_true;
			}

			static sl_bool InstallDriver(sl_bool& flagDokany)
			{
				if (CheckDriver(sl_true)) {
					flagDokany = sl_true;
					return sl_true;
				}
				if (CheckDriver(sl_false)) {
					flagDokany = sl_false;
					return sl_true;
				}
				if (!(Process::isAdmin())) {
					return sl_false;
				}
				if (StartDriver(sl_true)) {
					flagDokany = sl_true;
					return sl_true;
				}
				if (StartDriver(sl_false)) {
					flagDokany = sl_false;
					return sl_true;
				}
				flagDokany = IsDokanySupported();
				if (!(InstallLibrary(flagDokany))) {
					return sl_false;
				}
				SLIB_LOG_DEBUG("DokanInstall", "InstallLibrary OK (flagDokany: %d)", flagDokany);
				if (flagDokany) {
					if (!(RegisterCatalog())) {
						return sl_false;
					}
					SLIB_LOG_DEBUG("DokanInstall", "RegisterCatalog OK (flagDokany: %d)", flagDokany);
				}
				if (!(RegisterDriver(flagDokany))) {
					return sl_false;
				}
				SLIB_LOG_DEBUG("DokanInstall", "RegisterDriver OK (flagDokany: %d)", flagDokany);
				return StartDriver(flagDokany);
			}

		}
	}

	using namespace priv::dokany;
	
	sl_bool Dokany::install()
	{
		sl_bool flagDokany = sl_true;
		if (InstallDriver(flagDokany)) {
			return initialize(flagDokany, GetDriverName(flagDokany), GetLibraryPath(flagDokany));
		}
		return sl_false;
	}

}

#endif