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

#ifndef CHECKHEADER_SLIB_CORE_WIN32_PORTABLE_DEVICE
#define CHECKHEADER_SLIB_CORE_WIN32_PORTABLE_DEVICE

#include "../definition.h"

#ifdef SLIB_PLATFORM_IS_WIN32

#include "windows.h"

#include "../string.h"
#include "../list.h"
#include "../log.h"

#include <PortableDeviceApi.h>

#pragma comment(lib, "PortableDeviceGUIDs.lib")

namespace slib
{

	namespace win32
	{

		struct SLIB_EXPORT PortableDeviceInfo
		{
			String id;
			String name;
			String description;
			String manufacturer;
		};

		class SLIB_EXPORT PortableDevice
		{
		public:
			static void initializeCOM()
			{
				CoInitialize(NULL);
			}

			static List<PortableDeviceInfo> getDevices()
			{
				List<PortableDeviceInfo> ret;
				IPortableDeviceManager* pPortableDeviceManager;
				HRESULT hr = CoCreateInstance(CLSID_PortableDeviceManager,
					NULL,
					CLSCTX_INPROC_SERVER,
					IID_PPV_ARGS(&pPortableDeviceManager));
				if (SUCCEEDED(hr)) {
					DWORD cPnPDeviceIDs;
					hr = pPortableDeviceManager->GetDevices(NULL, &cPnPDeviceIDs);
					if (SUCCEEDED(hr)) {
						if (cPnPDeviceIDs > 0) {
							PWSTR* pPnpDeviceIDs = new PWSTR[cPnPDeviceIDs];
							if (pPnpDeviceIDs) {
								hr = pPortableDeviceManager->GetDevices(pPnpDeviceIDs, &cPnPDeviceIDs);
								if (SUCCEEDED(hr)) {
									for (DWORD dwIndex = 0; dwIndex < cPnPDeviceIDs; dwIndex++) {
										PWSTR id = pPnpDeviceIDs[dwIndex];
										PortableDeviceInfo info;
										info.id = String::from(id);
										WCHAR buf[1024];
										DWORD dwBuf = sizeof(buf) / sizeof(WCHAR);
										hr = pPortableDeviceManager->GetDeviceFriendlyName(id, buf, &dwBuf);
										if (SUCCEEDED(hr)) {
											info.name = String::from(buf);
										}
										dwBuf = sizeof(buf) / sizeof(WCHAR);
										hr = pPortableDeviceManager->GetDeviceDescription(id, buf, &dwBuf);
										if (SUCCEEDED(hr)) {
											info.description = String::from(buf);
										}
										dwBuf = sizeof(buf) / sizeof(WCHAR);
										hr = pPortableDeviceManager->GetDeviceManufacturer(id, buf, &dwBuf);
										if (SUCCEEDED(hr)) {
											info.manufacturer = String::from(buf);
										}
										ret.add_NoLock(Move(info));
										CoTaskMemFree(id);
									}
								} else {
									_logError("Failed to get the device list from the system", hr);
								}
								delete[] pPnpDeviceIDs;
							} else {
								_logError("Insufficient memory");
							}
						}
					} else {
						_logError("Failed to get number of devices on the system", hr);
					}
					pPortableDeviceManager->Release();
				} else {
					_logError("Failed to CoCreateInstance CLSID_PortableDeviceManager", hr);
				}
				return ret;
			}

		private:
			static void _logError(const char* szErr, HRESULT hr)
			{
				LogError("WPD", "%s, hr=0x%lx", szErr, hr);
			}

			static void _logError(const char* szErr)
			{
				LogError("WPD", "%s", szErr);
			}

		};

	}
	
}

#endif

#endif