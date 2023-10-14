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

		String16 Wmi::getQueryResponseValue(const StringParam& _query, const StringParam& _fieldName)
		{
			CoInitializeEx(0, COINIT_MULTITHREADED);
			HRESULT hr = CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, NULL);
			if (FAILED(hr)) {
				return sl_null;
			}
			String16 ret;
			IWbemLocator* pLoc = NULL;
			hr = CoCreateInstance(__uuidof(WbemLocator), 0, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pLoc));
			if (SUCCEEDED(hr)) {
				IWbemServices *pSvc = NULL;
				_bstr_t strNetwork(L"ROOT\\CIMV2");
				hr = pLoc->ConnectServer(strNetwork.GetBSTR(), NULL, NULL, 0, NULL, 0, 0, &pSvc);
				if (SUCCEEDED(hr)) {
					hr = CoSetProxyBlanket(pSvc, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL, RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE);
					if (SUCCEEDED(hr)) {
						StringCstr16 query(_query);
						IEnumWbemClassObject* pEnumerator = NULL;
						hr = pSvc->ExecQuery(L"WQL", (BSTR)(query.getData()), WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, NULL, &pEnumerator);
						if (SUCCEEDED(hr)) {
							IWbemClassObject* obj;
							while (pEnumerator) {
								ULONG uReturn;
								hr = pEnumerator->Next(WBEM_INFINITE, 1, &obj, &uReturn);
								if (FAILED(hr)) {
									break;
								}
								StringCstr16 fieldName(_fieldName);
								VARIANT vtProp;
								hr = obj->Get((LPCWSTR)(fieldName.getData()), 0, &vtProp, 0, 0);
								if (hr == S_OK) {
									ret = String16::from(vtProp.bstrVal);
								}
								VariantClear(&vtProp);
								obj->Release();
								if (ret.isNotNull()) {
									break;
								}
							}
						}
						pEnumerator->Release();
					}
				}
				pSvc->Release();
			}
			pLoc->Release();
			return ret;
		}

	}
}

#endif
