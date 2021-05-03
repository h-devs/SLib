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
 * Copyright (c) 2005-2007 CACE Technologies
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
 * 3. Neither the name of CACE Technologies nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
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
#ifndef __WPCAPNAMES_H_EED6D131C6DB4dd696757D219977A7E5
#define __WPCAPNAMES_H_EED6D131C6DB4dd696757D219977A7E5


//
// Original names
//
//  NOTE: 
//  - please do not use prefix names longer than 70 chars. 
//  - the following characters are surely accepted in the prefixes:  "[A-Z][a-z][0-9]_-',"   
//
#define NPF_DRIVER_NAME							"NPCAP"												///< (HHH) Packet.dll
#define NPF_DRIVER_NAME_WIDECHAR				L"NPCAP"											///< (HHH) Packet.dll
#define NPF_DRIVER_NAME_SMALL					"npcap"												///< (HHH) Packet.dll
#define NPF_DRIVER_NAME_SMALL_WIDECHAR			L"npcap"											///< (HHH) Packet.dll

#define NPF_DRIVER_NAME_SMALL_WIFI				NPF_DRIVER_NAME_SMALL "_wifi"
#define NPF_DRIVER_NAME_SMALL_WIDECHAR_WIFI		NPF_DRIVER_NAME_SMALL_WIDECHAR L"_wifi"

// Used as a logo string in NPFInstall
#define NPF_DRIVER_NAME_NORMAL					"Npcap"												///< (HHH) Packet.dll
#define NPF_DRIVER_NAME_NORMAL_WIDECHAR			L"Npcap"											///< (HHH) Packet.dll

// Used as the registry software key name
#define NPF_SOFT_REGISTRY_NAME					"NPCAP"												///< (HHH) Packet.dll
#define NPF_SOFT_REGISTRY_NAME_WIDECHAR			L"NPCAP"											///< (HHH) Packet.dll

#define NPF_ORGAN_NAME							"INSECURE"											///< (HHH) Packet.dll
#define NPF_ORGAN_NAME_WIDECHAR					L"INSECURE"											///< (HHH) Packet.dll

//
// Derived strings
//

// Used in packetWin7\Dll and the driver
#define NPF_DEVICE_NAMES_PREFIX					NPF_DRIVER_NAME "\\"     								///< (AAA) packet.dll
#define NPF_DEVICE_NAMES_PREFIX_WIDECHAR		NPF_DRIVER_NAME_WIDECHAR L"\\"     						///< (AAA) used by the NPF driver, that does not accept the TEXT(a) macro correctly.
#define NPF_DEVICE_NAMES_TAG_WIFI "WIFI_"
#define NPF_DEVICE_NAMES_TAG_WIDECHAR_WIFI L"WIFI_"
#define NPF_DEVICE_NAMES_PREFIX_WIFI			NPF_DEVICE_NAMES_PREFIX NPF_DEVICE_NAMES_TAG_WIFI
#define NPF_DEVICE_NAMES_PREFIX_WIDECHAR_WIFI	NPF_DEVICE_NAMES_PREFIX_WIDECHAR NPF_DEVICE_NAMES_TAG_WIDECHAR_WIFI

// Used in packetWin7\Dll
#define FAKE_LOOPBACK_ADAPTER_NAME "\\Device\\" NPF_DRIVER_NAME "\\Loopback"	///< (CCC) Name of a fake loopback adapter
#define FAKE_LOOPBACK_ADAPTER_DESCRIPTION "Adapter for loopback traffic capture" ///< (DDD) Description of a fake loopback adapter that is always available

// Used in packetWin7\Dll, NPFInstall and the driver
#define NPF_SERVICE_DESC						NPF_DRIVER_NAME_NORMAL " Packet Driver (" NPF_DRIVER_NAME ")"						///< (FFF) packet.dll
#define NPF_SERVICE_DESC_WIDECHAR				NPF_DRIVER_NAME_NORMAL_WIDECHAR L" Packet Driver (" NPF_DRIVER_NAME_WIDECHAR L")"	///< (FFF) packet.dll
#define NPF_SERVICE_DESC_TCHAR					_T(NPF_DRIVER_NAME_NORMAL) _T(" Packet Driver (") _T(NPF_DRIVER_NAME) _T(")")		///< (FFF) packet.dll
#define NPF_SERVICE_DESC_WIFI					NPF_SERVICE_DESC " (WiFi version)"
#define NPF_SERVICE_DESC_WIDECHAR_WIFI			NPF_SERVICE_DESC_WIDECHAR L" (WiFi version)"
#define NPF_SERVICE_DESC_TCHAR_WIFI				NPF_SERVICE_DESC_TCHAR _T(" (WiFi version)")

// Used in packetWin7\Dll
#define NPF_DRIVER_COMPLETE_DEVICE_PREFIX		"\\Device\\" NPF_DEVICE_NAMES_PREFIX					///< (III) packet.dll
// Used in packetWin7\Dll
#define NPF_DRIVER_COMPLETE_PATH				"system32\\drivers\\" NPF_DRIVER_NAME ".sys"			///< (LLL) packet.dll


//
// WinPcap Global Registry Key
//
#define WINPCAP_GLOBAL_KEY						"SOFTWARE\\CaceTech\\WinPcapOem"
#define WINPCAP_GLOBAL_KEY_WIDECHAR				L"SOFTWARE\\CaceTech\\WinPcapOem"
#define WINPCAP_INSTANCE_KEY					WINPCAP_GLOBAL_KEY "\\" NPF_DRIVER_NAME
#define WINPCAP_INSTANCE_KEY_WIDECHAR			WINPCAP_GLOBAL_KEY_WIDECHAR	L"\\" NPF_DRIVER_NAME_WIDECHAR
#define MAX_WINPCAP_KEY_CHARS					512

#endif //__WPCAPNAMES_H_EED6D131C6DB4dd696757D219977A7E5

