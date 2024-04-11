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

#include "slib/core/thread.h"
#include "slib/system/system.h"
#include "slib/io/priv/def.h"
#include "slib/platform/win32/windows.h"

#include "tap/tap-windows.h"

#include <winioctl.h>

namespace slib
{

	namespace {

		static sl_bool SetMediaStatus(HANDLE handle, sl_bool flagOn)
		{
			ULONG status = flagOn ? TRUE : FALSE;
			DWORD len = 0;
			BOOL bRet = DeviceIoControl(
				handle,
				TAP_WIN_IOCTL_SET_MEDIA_STATUS,
				&status, sizeof(status),
				&status, sizeof(status),
				&len,
				NULL
			);
			return bRet ? sl_true : sl_false;
		}

		class TapImpl : public Tap
		{
		public:
			HANDLE m_handle;
			OVERLAPPED m_overlappedRead;
			OVERLAPPED m_overlappedWrite;

		public:
			TapImpl()
			{
				Base::zeroMemory(&m_overlappedRead, sizeof(OVERLAPPED));
				Base::zeroMemory(&m_overlappedWrite, sizeof(OVERLAPPED));
			}

			~TapImpl()
			{
				close();
			}

		public:
			static Ref<TapImpl> open(const StringParam& _deviceName)
			{
				StringData16 deviceName(_deviceName);
				WCHAR adapterId[1024];
				WCHAR adapterName[1024];
				HANDLE handle = INVALID_HANDLE_VALUE;
				{
					HKEY key;
					if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, NETWORK_CONNECTIONS_KEY, 0, KEY_READ, &key)) {
						return sl_null;
					}
					for (sl_uint32 i = 0; ; i++) {
						// Get Adapter Id
						{
							Base::zeroMemory(adapterId, sizeof(adapterId));
							DWORD len = (DWORD)(CountOfArray(adapterId) - 1);
							if (RegEnumKeyExW(key, i, adapterId, &len, NULL, NULL, NULL, NULL)) {
								break;
							}
						}
						// Get Adapter Name
						{
							String16 strSubkey = String16::concat(NETWORK_CONNECTIONS_KEY, SLIB_UNICODE("\\"), adapterId, SLIB_UNICODE("\\Connection"));
							HKEY subkey;
							if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, (LPCWSTR)(strSubkey.getData()), 0, KEY_READ, &subkey)) {
								continue;
							}
							Base::zeroMemory(adapterName, sizeof(adapterName));
							DWORD size = sizeof(adapterName) - 2;
							RegQueryValueExW(subkey, L"Name", NULL, NULL, (LPBYTE)adapterName, &size);
							RegCloseKey(subkey);
						}
						sl_bool flagFound = sl_false;
						if (deviceName.isNotEmpty()) {
							if (deviceName.equals((sl_char16*)adapterId)) {
								flagFound = sl_true;
							}
							if (!flagFound) {
								if (deviceName.equals((sl_char16*)adapterName)) {
									flagFound = sl_true;
								}
							}
							if (!flagFound) {
								continue;
							}
						}
						String16 path = String16::concat(USERMODEDEVICEDIR, adapterId, TAP_WIN_SUFFIX);
						handle = CreateFileW((LPCWSTR)(path.getData()), GENERIC_WRITE | GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_SYSTEM | FILE_FLAG_OVERLAPPED, NULL);
						if (handle != INVALID_HANDLE_VALUE) {
							break;
						}
						if (flagFound) {
							// Invalid device name or cannot open the device
							return sl_null;
						}
					}
					RegCloseKey(key);
				}
				if (handle != INVALID_HANDLE_VALUE) {

					HANDLE hEventRead = CreateEventW(NULL, TRUE, FALSE, NULL);
					if (hEventRead) {
						HANDLE hEventWrite = CreateEventW(NULL, TRUE, FALSE, NULL);
						if (hEventWrite) {
							Ref<TapImpl> tap = new TapImpl;
							if (tap.isNotNull()) {
								tap->m_handle = handle;
								tap->m_deviceName = String::from(adapterId);
								tap->m_interfaceName = String::from(adapterName);
								tap->m_overlappedRead.hEvent = hEventRead;
								tap->m_overlappedWrite.hEvent = hEventWrite;
								SetMediaStatus(handle, sl_true);
								return tap;
							}
							CloseHandle(hEventWrite);
						}
						CloseHandle(hEventRead);
					}
					CloseHandle(handle);
				}
				// No tap device found or cannot open any tap device
				return sl_null;
			}

			void _close() override
			{
				CloseHandle(m_handle);
				CloseHandle(m_overlappedRead.hEvent);
				CloseHandle(m_overlappedWrite.hEvent);
			}

			sl_int32 read(void* buf, sl_uint32 size) override
			{
				if (m_flagOpened) {
					DWORD nRead = 0;
					if (ReadFile(m_handle, buf, size, &nRead, &m_overlappedRead)) {
						return nRead;
					} else {
						DWORD err = GetLastError();
						if (err == ERROR_IO_PENDING) {
							for (;;) {
								DWORD dwRet = WaitForSingleObject(m_overlappedRead.hEvent, 10);
								if (dwRet == WAIT_TIMEOUT) {
									if (Thread::isStoppingCurrent()) {
										return SLIB_IO_WOULD_BLOCK;
									}
								} else {
									break;
								}
							}
							BOOL bRet = GetOverlappedResult(m_handle, &m_overlappedRead, &nRead, FALSE);
							if (bRet) {
								return nRead;
							}
						}
					}
				}
				return SLIB_IO_ERROR;
			}

			sl_int32 write(const void* buf, sl_uint32 size) override
			{
				if (m_flagOpened) {
					DWORD nWrite = 0;
					if (WriteFile(m_handle, buf, size, &nWrite, &m_overlappedWrite)) {
						return nWrite;
					} else {
						DWORD err = GetLastError();
						if (err == ERROR_IO_PENDING) {
							for (;;) {
								DWORD dwRet = WaitForSingleObject(m_overlappedWrite.hEvent, 10);
								if (dwRet == WAIT_TIMEOUT) {
									if (Thread::isStoppingCurrent()) {
										return SLIB_IO_WOULD_BLOCK;
									}
								} else {
									break;
								}
							}
							BOOL bRet = GetOverlappedResult(m_handle, &m_overlappedWrite, &nWrite, FALSE);
							if (bRet) {
								return nWrite;
							}
						}
					}
				}
				return SLIB_IO_ERROR;
			}

			sl_bool setIpAddress(const StringParam& ip, const StringParam& mask) override
			{
				sl_int32 iRet = System::execute(String::concat("netsh interface ip set address \"", m_interfaceName, "\" static ", ip, " ", mask));
				if (!iRet) {
					return sl_true;
				}
				return sl_false;
			}

		};

	}

	Ref<Tap> Tap::open(const StringParam& deviceName)
	{
		return Ref<Tap>::from(TapImpl::open(deviceName));
	}

	ServiceState Tap::getDriverState()
	{
		return ServiceManager::getState("tap0901");
	}

}

#endif