/*
 *   Copyright (c) 2008-2022 SLIBIO <https://github.com/SLIBIO>
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

#if defined(SLIB_PLATFORM_IS_WIN32)

#include "slib/platform/win32/wmi.h"

#include "slib/platform.h"
#include "slib/core/time_zone.h"

#include <objbase.h>
#include <wbemcli.h>
#include <wbemidl.h>
#include <winioctl.h>
#include <comdef.h>

#pragma comment(lib, "shlwapi.lib")

namespace slib
{
	namespace win32
	{

		namespace
		{
			class WmiQueryContext
			{
			public:
				IWbemLocator* locator = NULL;
				IWbemServices* services = NULL;

			public:
				WmiQueryContext()
				{
				}

				~WmiQueryContext()
				{
					if (locator) {
						locator->Release();
					}
					if (services) {
						services->Release();
					}
				}

			public:
				IEnumWbemClassObject* query(const StringParam& _query)
				{
					CoInitializeEx(0, COINIT_MULTITHREADED);
					CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, NULL);
					HRESULT hr = hr = CoCreateInstance(__uuidof(WbemLocator), 0, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&locator));
					if (FAILED(hr)) {
						return sl_null;
					}
					_bstr_t strNetwork(L"ROOT\\CIMV2");
					hr = locator->ConnectServer(strNetwork.GetBSTR(), NULL, NULL, 0, NULL, 0, 0, &services);
					if (FAILED(hr)) {
						return sl_null;
					}
					hr = CoSetProxyBlanket(services, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL, RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE);
					if (FAILED(hr)) {
						return sl_null;
					}
					StringCstr16 query(_query);
					IEnumWbemClassObject* enumerator = NULL;
					hr = services->ExecQuery(L"WQL", (BSTR)(query.getData()), WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, NULL, &enumerator);
					if (FAILED(hr)) {
						return sl_null;
					}
					return enumerator;
				}

			};

			static Variant GetWmiField(IWbemClassObject* obj, const StringParam& _fieldName)
			{
				StringCstr16 fieldName(_fieldName);
				VARIANT var = {0};
				HRESULT hr = obj->Get((LPCWSTR)(fieldName.getData()), 0, &var, 0, 0);
				if (FAILED(hr)) {
					return sl_null;
				}
				Variant ret = Win32::getVariantFromVARIANT(&var);
				VariantClear(&var);
				return ret;
			}
		}

		Variant Wmi::getQueryResponseValue(const StringParam& query, const StringParam& fieldName)
		{
			WmiQueryContext context;
			IEnumWbemClassObject* enumerator = context.query(query);
			if (!enumerator) {
				return sl_null;
			}
			Variant ret;
			IWbemClassObject* obj = NULL;
			ULONG uReturn = 0;
			HRESULT hr = enumerator->Next(WBEM_INFINITE, 1, &obj, &uReturn);
			if (SUCCEEDED(hr)) {
				ret = GetWmiField(obj, fieldName);
				obj->Release();
			}
			enumerator->Release();
			return ret;
		}

		VariantMap Wmi::getQueryResponseRecord(const StringParam& query, const StringParam* fieldNames, sl_size nFields)
		{
			WmiQueryContext context;
			IEnumWbemClassObject* enumerator = context.query(query);
			if (!enumerator) {
				return sl_null;
			}
			VariantMap ret;
			IWbemClassObject* obj = NULL;
			ULONG uReturn = 0;
			HRESULT hr = enumerator->Next(WBEM_INFINITE, 1, &obj, &uReturn);
			if (SUCCEEDED(hr)) {
				for (sl_size i = 0; i < nFields; i++) {
					ret.put_NoLock(String::from(fieldNames[i]), GetWmiField(obj, fieldNames[i]));
				}
				obj->Release();
			}
			enumerator->Release();
			return ret;
		}

		List<VariantMap> Wmi::getQueryResponseRecords(const StringParam& query, const StringParam* fieldNames, sl_size nFields)
		{
			WmiQueryContext context;
			IEnumWbemClassObject* enumerator = context.query(query);
			if (!enumerator) {
				return sl_null;
			}
			List<VariantMap> ret;
			for (;;) {
				IWbemClassObject* obj = NULL;
				ULONG uReturn = 0;
				enumerator->Next(WBEM_INFINITE, 1, &obj, &uReturn);
				if (obj) {
					VariantMap record;
					for (sl_size i = 0; i < nFields; i++) {
						record.put_NoLock(String::from(fieldNames[i]), GetWmiField(obj, fieldNames[i]));
					}
					obj->Release();
					ret.add_NoLock(Move(record));
				} else {
					break;
				}
			}
			enumerator->Release();
			return ret;
		}

		Time Wmi::getDateTime(const Variant& value)
		{
			if (value.is16BitsStringType()) {
				StringView16 str = value.getStringView16();
				if (str.getLength() == 25 && str[14] == '.') {
					sl_char16 s = str[21];
					if (s == '+' || s == '-') {
						sl_uint32 year = str.substring(0, 4).parseUint32();
						sl_uint32 month = str.substring(4, 6).parseUint32();
						sl_uint32 day = str.substring(6, 8).parseUint32();
						sl_uint32 hour = str.substring(8, 10).parseUint32();
						sl_uint32 minute = str.substring(10, 12).parseUint32();
						sl_uint32 second = str.substring(12, 14).parseUint32();
						sl_uint32 ms = str.substring(15, 21).parseUint32();
						sl_uint32 offset = str.substring(22, 25).parseUint32();
						return Time(year, month, day, hour, minute, second, 0, ms, TimeZone::create(offset * 60));
					}
				}
			}
			return Time::zero();
		}

	}
}

#endif
