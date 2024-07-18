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

#ifndef CHECKHEADER_SLIB_DL_WIN32_IPHLPAPI
#define CHECKHEADER_SLIB_DL_WIN32_IPHLPAPI

#include "../dl.h"

#if defined(SLIB_PLATFORM_IS_WINDOWS)

#include <winsock2.h>
#include <iphlpapi.h>

namespace slib
{

	SLIB_IMPORT_LIBRARY_BEGIN(iphlpapi, "iphlpapi.dll")
		SLIB_IMPORT_LIBRARY_FUNCTION(
			if_nametoindex,
			NET_IFINDEX, NTAPI,
			PCSTR InterfaceName
		)
		SLIB_IMPORT_LIBRARY_FUNCTION(
			if_indextoname,
			PCHAR, NTAPI,
			NET_IFINDEX InterfaceIndex,
			PCHAR InterfaceName
		)
		SLIB_IMPORT_LIBRARY_FUNCTION(
			GetIpAddrTable,
			DWORD, WINAPI,
			PMIB_IPADDRTABLE pIpAddrTable,
			PULONG pdwSize,
			BOOL bOrder
		)
		SLIB_IMPORT_LIBRARY_FUNCTION(
			GetAdaptersAddresses,
			ULONG, WINAPI,
			ULONG Family,
			ULONG Flags,
			PVOID Reserved,
			PIP_ADAPTER_ADDRESSES AdapterAddresses,
			PULONG SizePointer
		)
		SLIB_IMPORT_LIBRARY_FUNCTION(
			GetAdaptersInfo,
			ULONG, WINAPI,
			PIP_ADAPTER_INFO AdapterInfo,
			PULONG SizePointer
		)
		SLIB_IMPORT_LIBRARY_FUNCTION(
			GetIpNetTable,
			DWORD, WINAPI,
			PMIB_IPNETTABLE pIpNetTable,
			PULONG pdwSize,
			BOOL bOrder
		)
		SLIB_IMPORT_LIBRARY_FUNCTION(
			DeleteIpNetEntry,
			DWORD, WINAPI,
			PMIB_IPNETROW pArpEntry
		)
	SLIB_IMPORT_LIBRARY_END

}

#endif

#endif
