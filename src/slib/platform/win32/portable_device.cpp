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

#include "slib/platform/definition.h"

#if defined(SLIB_PLATFORM_IS_WINDOWS)

#include "slib/platform/win32/portable_device.h"

#include "slib/core/log.h"

#include <PortableDevice.h>

#pragma comment(lib, "PortableDeviceGUIDs.lib")

namespace slib
{

	namespace priv
	{
		namespace wpd
		{

			static void LogWpdError(const char* szErr, HRESULT hr)
			{
				LogError("WPD", "%s, hr=0x%lx", szErr, hr);
			}

			static void LogWpdError(const char* szErr)
			{
				LogError("WPD", "%s", szErr);
			}

			static void LogWpdCreateInstanceError(const char* szClsid, HRESULT hr)
			{
				LogError("WPD", "Failed to CoCreateInstance: %s, hr=0x%lx", szClsid, hr);
			}

		}
	}

	using namespace priv::wpd;

	namespace win32
	{

		SLIB_DEFINE_WIN32_COM_CONTAINER_MEMBERS(PortableDeviceManager, IPortableDeviceManager, m_object)

		PortableDeviceManager PortableDeviceManager::create()
		{
			IPortableDeviceManager* pPortableDeviceManager = NULL;
			HRESULT hr = CoCreateInstance(CLSID_PortableDeviceManager,
				NULL,
				CLSCTX_INPROC_SERVER,
				IID_PPV_ARGS(&pPortableDeviceManager));
			if (SUCCEEDED(hr)) {
				return pPortableDeviceManager;
			} else {
				LogWpdCreateInstanceError("CLSID_PortableDeviceManager", hr);
			}
			return sl_null;
		}

		List<String> PortableDeviceManager::getDeviceIdentifiers()
		{
			List<String> ret;
			DWORD cPnPDeviceIDs;
			HRESULT hr = m_object->GetDevices(NULL, &cPnPDeviceIDs);
			if (SUCCEEDED(hr)) {
				if (cPnPDeviceIDs > 0) {
					PWSTR* pPnpDeviceIDs = new PWSTR[cPnPDeviceIDs];
					if (pPnpDeviceIDs) {
						hr = m_object->GetDevices(pPnpDeviceIDs, &cPnPDeviceIDs);
						if (SUCCEEDED(hr)) {
							for (DWORD i = 0; i < cPnPDeviceIDs; i++) {
								ret.add_NoLock(String::from(pPnpDeviceIDs[i]));
								CoTaskMemFree(pPnpDeviceIDs[i]);
							}
						} else {
							LogWpdError("Failed to get the device list from the system", hr);
						}
						delete[] pPnpDeviceIDs;
					} else {
						LogWpdError("Insufficient memory");
					}
				}
			} else {
				LogWpdError("Failed to get number of devices on the system", hr);
			}
			return ret;
		}

		List<PortableDeviceInfo> PortableDeviceManager::getDeviceInfos()
		{
			List<PortableDeviceInfo> ret;
			ListElements<String> listId(getDeviceIdentifiers());
			for (sl_size i = 0; i < listId.count; i++) {
				StringCstr16 _id = listId[i];
				WCHAR* id = (WCHAR*)(_id.getData());
				PortableDeviceInfo info;
				info.id = String::from(id);
				WCHAR buf[1024];
				DWORD nBuf = (DWORD)(CountOfArray(buf));
				DWORD dwBuf = nBuf;
				HRESULT hr = m_object->GetDeviceFriendlyName(id, buf, &dwBuf);
				if (SUCCEEDED(hr)) {
					info.name = String::from(buf);
				}
				dwBuf = nBuf;
				hr = m_object->GetDeviceDescription(id, buf, &dwBuf);
				if (SUCCEEDED(hr)) {
					info.description = String::from(buf);
				}
				dwBuf = nBuf;
				hr = m_object->GetDeviceManufacturer(id, buf, &dwBuf);
				if (SUCCEEDED(hr)) {
					info.manufacturer = String::from(buf);
				}
				ret.add_NoLock(Move(info));
			}
			return ret;
		}


		SLIB_DEFINE_WIN32_COM_CONTAINER_MEMBERS(PortableDeviceProperties, IPortableDeviceProperties, m_object)

		String PortableDeviceProperties::getObjectName(const StringParam& _id)
		{
			String ret;
			StringCstr16 id(_id);
			ComPtr<IPortableDeviceValues> pValues;
			HRESULT hr = m_object->GetValues((WCHAR*)(id.getData()), NULL, &(pValues.ptr));
			if (SUCCEEDED(hr)) {
				LPWSTR pValue = NULL;
				hr = pValues->GetStringValue(WPD_OBJECT_NAME, &pValue);
				if (SUCCEEDED(hr)) {
					ret = String::from(pValue);
					CoTaskMemFree(pValue);
				}
			}
			return ret;
		}


		SLIB_DEFINE_WIN32_COM_CONTAINER_MEMBERS(PortableDeviceContent, IPortableDeviceContent, m_object)

		List<String> PortableDeviceContent::getObjectIdentifiers(const StringParam& _parentId)
		{
			List<String> ret;
			ComPtr<IEnumPortableDeviceObjectIDs> pEnum;
			HRESULT hr;
			StringCstr16 parentId(_parentId);
			if (parentId.isNotNull()) {
				hr = m_object->EnumObjects(0, (LPCWSTR)(parentId.getData()), NULL, &(pEnum.ptr));
			} else {
				hr = m_object->EnumObjects(0, WPD_DEVICE_OBJECT_ID, NULL, &(pEnum.ptr));
			}
			if (SUCCEEDED(hr)) {
				for (;;) {
					DWORD cFetched = 0;
					PWSTR szObjectIDArray[256] = { 0 };
					hr = pEnum->Next((ULONG)(CountOfArray(szObjectIDArray)), szObjectIDArray, &cFetched);
					for (DWORD i = 0; i < cFetched; i++) {
						ret.add_NoLock(String::from(szObjectIDArray[i]));
						CoTaskMemFree(szObjectIDArray[i]);
					}
					if (hr != S_OK) {
						break;
					}
				}
			}
			return ret;
		}

		List<String> PortableDeviceContent::getObjectIdentifiers()
		{
			return getObjectIdentifiers(sl_null);
		}

		PortableDeviceProperties PortableDeviceContent::getProperties()
		{
			IPortableDeviceProperties* pProps = NULL;
			HRESULT hr = m_object->Properties(&pProps);
			if (SUCCEEDED(hr)) {
				return pProps;
			}
			return sl_null;
		}

		List<PortableDeviceObjectInfo> PortableDeviceContent::getObjectInfos(const StringParam& parentId)
		{
			List<PortableDeviceObjectInfo> ret;
			PortableDeviceProperties props = getProperties();
			if (props.isNotNull()) {
				ListElements<String> listId(getObjectIdentifiers(parentId));
				for (sl_size i = 0; i < listId.count; i++) {
					PortableDeviceObjectInfo info;
					info.id = listId[i];
					info.name = props.getObjectName(info.id);
					ret.add_NoLock(Move(info));
				}
			}
			return ret;
		}

		List<PortableDeviceObjectInfo> PortableDeviceContent::getObjectInfos()
		{
			return getObjectInfos(sl_null);
		}


		SLIB_DEFINE_WIN32_COM_CONTAINER_MEMBERS(PortableDevice, IPortableDevice, m_object)

		List<String> PortableDevice::getDeviceIdentifiers()
		{
			PortableDeviceManager manager = PortableDeviceManager::create();
			if (manager.isNotNull()) {
				return manager.getDeviceIdentifiers();
			}
			return sl_null;
		}

		List<PortableDeviceInfo> PortableDevice::getDeviceInfos()
		{
			PortableDeviceManager manager = PortableDeviceManager::create();
			if (manager.isNotNull()) {
				return manager.getDeviceInfos();
			}
			return sl_null;
		}

		PortableDevice PortableDevice::open(const StringParam& _id)
		{
			IPortableDevice* pDevice = NULL;
			HRESULT hr = CoCreateInstance(CLSID_PortableDeviceFTM,
				NULL,
				CLSCTX_INPROC_SERVER,
				IID_PPV_ARGS(&pDevice));
			if (SUCCEEDED(hr)) {
				ComPtr<IPortableDeviceValues> pInfo;
				hr = CoCreateInstance(CLSID_PortableDeviceValues,
					NULL,
					CLSCTX_INPROC_SERVER,
					IID_PPV_ARGS(&(pInfo.ptr)));
				if (SUCCEEDED(hr)) {
					StringCstr16 id(_id);
					hr = pDevice->Open((LPCWSTR)(id.getData()), pInfo);
					if (hr == E_ACCESSDENIED) {
						// Failed to Open the device for Read Write access, will open it for Read-only access instead
						pInfo->SetUnsignedIntegerValue(WPD_CLIENT_DESIRED_ACCESS, GENERIC_READ);
						hr = pDevice->Open((LPCWSTR)(id.getData()), pInfo);
					}
					if (SUCCEEDED(hr)) {
						return pDevice;
					} else {
						LogWpdError("Failed to Open the device", hr);
					}
				} else {
					LogWpdCreateInstanceError("CLSID_PortableDeviceValues", hr);
				}
				pDevice->Release();
			} else {
				LogWpdCreateInstanceError("CLSID_PortableDeviceFTM", hr);
			}
			return sl_null;
		}

		PortableDeviceContent PortableDevice::getContent()
		{
			IPortableDeviceContent* content = NULL;
			HRESULT hr = m_object->Content(&content);
			if (SUCCEEDED(hr)) {
				return content;
			}
			return sl_null;
		}

		List<PortableDeviceObjectInfo> PortableDevice::getObjectInfos(const StringParam& parentId)
		{
			PortableDeviceContent content = getContent();
			if (content.isNotNull()) {
				return content.getObjectInfos(parentId);
			}
			return sl_null;
		}

		List<PortableDeviceObjectInfo> PortableDevice::getObjectInfos()
		{
			return getObjectInfos(sl_null);
		}

		PortableDeviceType PortableDevice::getType()
		{
			PortableDeviceType ret = PortableDeviceType::Unknown;
			PortableDeviceContent content = getContent();
			if (content.isNotNull()) {
				PortableDeviceProperties props = content.getProperties();
				if (props.isNotNull()) {
					IPortableDeviceProperties* pProps = props.get();
					ComPtr<IPortableDeviceValues> pValues;
					HRESULT hr = props.get()->GetValues(WPD_DEVICE_OBJECT_ID, NULL, &(pValues.ptr));
					if (SUCCEEDED(hr)) {
						ULONG value;
						hr = pValues->GetUnsignedIntegerValue(WPD_DEVICE_TYPE, &value);
						if (SUCCEEDED(hr)) {
							switch (value) {
							case WPD_DEVICE_TYPE_GENERIC:
								ret = PortableDeviceType::Generic;
								break;
							case WPD_DEVICE_TYPE_CAMERA:
								ret = PortableDeviceType::Camera;
								break;
							case WPD_DEVICE_TYPE_MEDIA_PLAYER:
								ret = PortableDeviceType::MediaPlayer;
								break;
							case WPD_DEVICE_TYPE_PHONE:
								ret = PortableDeviceType::Phone;
								break;
							case WPD_DEVICE_TYPE_VIDEO:
								ret = PortableDeviceType::Video;
								break;
							case WPD_DEVICE_TYPE_PERSONAL_INFORMATION_MANAGER:
								ret = PortableDeviceType::PersonalInformationManager;
								break;
							case WPD_DEVICE_TYPE_AUDIO_RECORDER:
								ret = PortableDeviceType::AudioRecorder;
								break;
							}
						}
					}
				}
			}
			return ret;
		}

		String PortableDevice::getProtocol()
		{
			String ret;
			PortableDeviceContent content = getContent();
			if (content.isNotNull()) {
				PortableDeviceProperties props = content.getProperties();
				if (props.isNotNull()) {
					IPortableDeviceProperties* pProps = props.get();
					ComPtr<IPortableDeviceValues> pValues;
					HRESULT hr = props.get()->GetValues(WPD_DEVICE_OBJECT_ID, NULL, &(pValues.ptr));
					if (SUCCEEDED(hr)) {
						LPWSTR pValue = NULL;
						hr = pValues->GetStringValue(WPD_DEVICE_PROTOCOL, &pValue);
						if (SUCCEEDED(hr)) {
							ret = String::from(pValue);
							CoTaskMemFree(pValue);
						}
					}
				}
			}
			return ret;
		}

	}

}

#endif
