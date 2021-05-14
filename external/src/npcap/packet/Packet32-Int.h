/***********************IMPORTANT NPCAP LICENSE TERMS***********************
 *                                                                         *
 * Npcap is a Windows packet sniffing driver and library and is copyright  *
 * (c) 2013-2020 by Insecure.Com LLC ("The Nmap Project").  All rights     *
 * reserved.                                                               *
 *                                                                         *
 * Even though Npcap source code is publicly available for review, it is   *
 * not open source software and may not be redistributed or incorporated   *
 * into other software without special permission from the Nmap Project.   *
 * We fund the Npcap project by selling a commercial license which allows  *
 * companies to redistribute Npcap with their products and also provides   *
 * for support, warranty, and indemnification rights.  For details on      *
 * obtaining such a license, please contact:                               *
 *                                                                         *
 * sales@nmap.com                                                          *
 *                                                                         *
 * Free and open source software producers are also welcome to contact us  *
 * for redistribution requests.  However, we normally recommend that such  *
 * authors instead ask your users to download and install Npcap            *
 * themselves.                                                             *
 *                                                                         *
 * Since the Npcap source code is available for download and review,       *
 * users sometimes contribute code patches to fix bugs or add new          *
 * features.  By sending these changes to the Nmap Project (including      *
 * through direct email or our mailing lists or submitting pull requests   *
 * through our source code repository), it is understood unless you        *
 * specify otherwise that you are offering the Nmap Project the            *
 * unlimited, non-exclusive right to reuse, modify, and relicence your     *
 * code contribution so that we may (but are not obligated to)             *
 * incorporate it into Npcap.  If you wish to specify special license      *
 * conditions or restrictions on your contributions, just say so when you  *
 * send them.                                                              *
 *                                                                         *
 * This software is distributed in the hope that it will be useful, but    *
 * WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                    *
 *                                                                         *
 * Other copyright notices and attribution may appear below this license   *
 * header. We have kept those for attribution purposes, but any license    *
 * terms granted by those notices apply only to their original work, and   *
 * not to any changes made by the Nmap Project or to this entire file.     *
 *                                                                         *
 * This header summarizes a few important aspects of the Npcap license,    *
 * but is not a substitute for the full Npcap license agreement, which is  *
 * in the LICENSE file included with Npcap and also available at           *
 * https://github.com/nmap/npcap/blob/master/LICENSE.                      *
 *                                                                         *
 ***************************************************************************/
/*
 * Copyright (c) 2006 - 2010
 * CACE Technologies Inc., Davis (CA)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the company (CACE Technologies Inc.) nor the 
 * names of its contributors may be used to endorse or promote products 
 * derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

//
// Internal constants
//

// The following is used to check the adapter name in PacketOpenAdapterNPF and prevent 
// opening of firewire adapters 
#define FIREWIRE_SUBSTR L"1394"

#ifdef __MINGW32__
#ifdef __MINGW64__
#include <ntddndis.h>
#else /*__MINGW64__*/
#include <ddk/ntddndis.h>
#endif /*__MINGW64__*/
#else /*__MINGW32__*/
#pragma warning( push )
#pragma warning( disable : 4201 )
#include <ntddndis.h>
#pragma warning( pop )
#endif /*__MINGW32__*/

/*!
  \brief Linked list item containing one of the IP addresses associated with an adapter.
*/
typedef struct _NPF_IF_ADDRESS_ITEM
{
	npf_if_addr Addr;			///< IP address
	struct _NPF_IF_ADDRESS_ITEM *Next; ///< Pointer to the next item in the linked list.
}
	NPF_IF_ADDRESS_ITEM, *PNPF_IF_ADDRESS_ITEM;

/*!
  \brief Contains comprehensive information about a network adapter.

  This structure is filled with all the accessory information that the user can need about an adapter installed
  on his system.
*/
typedef struct _ADAPTER_INFO  
{
	struct _ADAPTER_INFO *Next;				///< Pointer to the next adapter in the list.
	CHAR Name[ADAPTER_NAME_LENGTH + 1];		///< Name of the device representing the adapter.
	CHAR Description[ADAPTER_DESC_LENGTH + 1];	///< Human understandable description of the adapter
	UINT MacAddressLen;						///< Length of the link layer address.
	UCHAR MacAddress[MAX_MAC_ADDR_LENGTH];	///< Link layer address.
	NetType LinkLayer;						///< Physical characteristics of this adapter. This NetType structure contains the link type and the speed of the adapter.
	PNPF_IF_ADDRESS_ITEM pNetworkAddresses;///< Pointer to a linked list of IP addresses, each of which specifies a network address of this adapter.
	UINT Flags;								///< Adapter's flags. Tell if this adapter must be treated in a different way.
}
ADAPTER_INFO, *PADAPTER_INFO;


//
// Internal functions
//
void PacketPopulateAdaptersInfoList();
BOOL PacketGetFileVersion(LPCTSTR FileName, PCHAR VersionBuff, UINT VersionBuffLen);
PADAPTER_INFO PacketFindAdInfo(PCHAR AdapterName);
BOOLEAN PacketUpdateAdInfo(PCHAR AdapterName);
BOOLEAN IsFireWire(TCHAR *AdapterDesc);
LPADAPTER PacketOpenAdapterNPF(LPCSTR AdapterName);

#ifdef __cplusplus
extern "C" {
#endif
HMODULE LoadLibrarySafe(LPCTSTR lpFileName);
#ifdef __cplusplus
}
#endif

// 
// Definitions and functions specific to the CACETech airpcap API
//
#ifdef HAVE_AIRPCAP_API
typedef PCHAR (*AirpcapGetLastErrorHandler)(PAirpcapHandle Handle);
typedef BOOL (*AirpcapGetDeviceListHandler)(AirpcapDeviceDescription **AllDevsP, PCHAR Ebuf);		///< prototype used to dynamically load the dll
typedef VOID (*AirpcapFreeDeviceListHandler)(AirpcapDeviceDescription *AllDevsP);					///< prototype used to dynamically load the dll
typedef PAirpcapHandle (*AirpcapOpenHandler)(LPCSTR DeviceName, PCHAR Ebuf);							///< prototype used to dynamically load the dll	
typedef VOID (*AirpcapCloseHandler)(PAirpcapHandle Handle);											///< prototype used to dynamically load the dll
typedef BOOL (*AirpcapGetLinkTypeHandler)(PAirpcapHandle Handle, AirpcapLinkType* LinkLayer);		///< prototype used to dynamically load the dll
typedef BOOL (*AirpcapSetKernelBufferHandler)(PAirpcapHandle Handle, ULONG Size);					///< prototype used to dynamically load the dll
typedef BOOL (*AirpcapSetFilterHandler)(PAirpcapHandle Handle, void *Instructions, UINT Len);		///< prototype used to dynamically load the dll
typedef BOOL (*AirpcapGetMacAddressHandler)(PAirpcapHandle Handle, CHAR **MacAddress);				///< prototype used to dynamically load the dll
typedef BOOL (*AirpcapSetMinToCopyHandler)(PAirpcapHandle Handle, ULONG Bytes);						///< prototype used to dynamically load the dll
typedef BOOL (*AirpcapGetReadEventHandler)(PAirpcapHandle Handle, HANDLE* PReadEvent);				///< prototype used to dynamically load the dll
typedef BOOL (*AirpcapReadHandler)(PAirpcapHandle Handle, PUCHAR BufferToFill, ULONG BufSize, PULONG ReceievedBytes);	///< prototype used to dynamically load the dll
typedef BOOL (*AirpcapGetStatsHandler)(PAirpcapHandle Handle, AirpcapStats *Stats);					///< prototype used to dynamically load the dll
typedef BOOL (*AirpcapWriteHandler)(PAirpcapHandle Handle, PCHAR TxPacket, ULONG PacketLen);		///< prototype used to dynamically load the dll

#endif // HAVE_AIRPCAP_API
