/*
 *   Copyright (c) 2008-2024 SLIBIO <https://github.com/SLIBIO>
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

#ifndef CHECKHEADER_SLIB_DL_COMMON_PCAP
#define CHECKHEADER_SLIB_DL_COMMON_PCAP

#include "../dl.h"

#if defined(SLIB_PLATFORM_IS_LINUX_DESKTOP) || defined(SLIB_PLATFORM_IS_WIN32)

#include "pcap/pcap.h"

namespace slib
{

#ifdef SLIB_PLATFORM_IS_WIN32
	SLIB_IMPORT_LIBRARY_BEGIN(pcap, "wpcap.dll")
#else
	SLIB_IMPORT_LIBRARY_BEGIN(pcap, "libpcap.so", "libpcap.so.1", "libpcap.so.0.8")
#endif

		SLIB_IMPORT_LIBRARY_FUNCTION(
			pcap_create,
			pcap_t*, ,
			const char *, char *
		)
		#define pcap_create slib::pcap::getApi_pcap_create()

		SLIB_IMPORT_LIBRARY_FUNCTION(
			pcap_open_live,
			pcap_t*, ,
			const char*, int, int, int, char*
		)
		#define pcap_open_live slib::pcap::getApi_pcap_open_live()

		SLIB_IMPORT_LIBRARY_FUNCTION(
			pcap_close,
			void, ,
			pcap_t*
		)
		#define pcap_close slib::pcap::getApi_pcap_close()

		SLIB_IMPORT_LIBRARY_FUNCTION(
			pcap_setbuff,
			int, ,
			pcap_t *p, int dim
		)
		#define pcap_setbuff slib::pcap::getApi_pcap_setbuff()

		SLIB_IMPORT_LIBRARY_FUNCTION(
			pcap_set_snaplen,
			int, ,
			pcap_t *, int
		)
		#define pcap_set_snaplen slib::pcap::getApi_pcap_set_snaplen()

		SLIB_IMPORT_LIBRARY_FUNCTION(
			pcap_set_buffer_size,
			int, ,
			pcap_t *, int
		)
		#define pcap_set_buffer_size slib::pcap::getApi_pcap_set_buffer_size()

#ifdef SLIB_PLATFORM_IS_WIN32
		SLIB_IMPORT_LIBRARY_FUNCTION(
			pcap_setuserbuffer,
			int, ,
			pcap_t *, int
		)
		#define pcap_setuserbuffer slib::pcap::getApi_pcap_setuserbuffer()
#endif

		SLIB_IMPORT_LIBRARY_FUNCTION(
			pcap_set_promisc,
			int, ,
			pcap_t *, int
		)
		#define pcap_set_promisc slib::pcap::getApi_pcap_set_promisc()

		SLIB_IMPORT_LIBRARY_FUNCTION(
			pcap_set_timeout,
			int, ,
			pcap_t *, int
		)
		#define pcap_set_timeout slib::pcap::getApi_pcap_set_timeout()

		SLIB_IMPORT_LIBRARY_FUNCTION(
			pcap_set_immediate_mode,
			int, ,
			pcap_t *, int
		)
		#define pcap_set_immediate_mode slib::pcap::getApi_pcap_set_immediate_mode()

		SLIB_IMPORT_LIBRARY_FUNCTION(
			pcap_setnonblock,
			int, ,
			pcap_t *, int, char *
		)
		#define pcap_setnonblock slib::pcap::getApi_pcap_setnonblock()

		SLIB_IMPORT_LIBRARY_FUNCTION(
			pcap_activate,
			int, ,
			pcap_t *
		)
		#define pcap_activate slib::pcap::getApi_pcap_activate()

		SLIB_IMPORT_LIBRARY_FUNCTION(
			pcap_breakloop,
			void, ,
			pcap_t *
		)
		#define pcap_breakloop slib::pcap::getApi_pcap_breakloop()

		SLIB_IMPORT_LIBRARY_FUNCTION(
			pcap_loop,
			int, ,
			pcap_t *, int, pcap_handler, u_char *
		)
		#define pcap_loop slib::pcap::getApi_pcap_loop()

		SLIB_IMPORT_LIBRARY_FUNCTION(
			pcap_dispatch,
			int, ,
			pcap_t *, int, pcap_handler, u_char *
		)
		#define pcap_dispatch slib::pcap::getApi_pcap_dispatch()

#ifdef SLIB_PLATFORM_IS_WIN32
		SLIB_IMPORT_LIBRARY_FUNCTION(
			pcap_getevent,
			HANDLE, ,
			pcap_t *p
		)
		#define pcap_getevent slib::pcap::getApi_pcap_getevent()
#endif

		SLIB_IMPORT_LIBRARY_FUNCTION(
			pcap_get_selectable_fd,
			int, ,
			pcap_t *
		)
		#define pcap_get_selectable_fd slib::pcap::getApi_pcap_get_selectable_fd()

		SLIB_IMPORT_LIBRARY_FUNCTION(
			pcap_datalink,
			int, ,
			pcap_t *
		)
		#define pcap_datalink slib::pcap::getApi_pcap_datalink()

		SLIB_IMPORT_LIBRARY_FUNCTION(
			pcap_set_datalink,
			int, ,
			pcap_t *, int
		)
		#define pcap_set_datalink slib::pcap::getApi_pcap_set_datalink()

		SLIB_IMPORT_LIBRARY_FUNCTION(
			pcap_sendpacket,
			int, ,
			pcap_t *, const u_char *, int
		)
		#define pcap_sendpacket slib::pcap::getApi_pcap_sendpacket()

#ifdef SLIB_PLATFORM_IS_WIN32
		SLIB_IMPORT_LIBRARY_FUNCTION(
			pcap_sendqueue_alloc,
			pcap_send_queue*, ,
			u_int memsize
		)
		#define pcap_sendqueue_alloc slib::pcap::getApi_pcap_sendqueue_alloc()

		SLIB_IMPORT_LIBRARY_FUNCTION(
			pcap_sendqueue_destroy,
			void, ,
			pcap_send_queue*
		)
		#define pcap_sendqueue_destroy slib::pcap::getApi_pcap_sendqueue_destroy()

		SLIB_IMPORT_LIBRARY_FUNCTION(
			pcap_sendqueue_queue,
			int, ,
			pcap_send_queue*, const struct pcap_pkthdr *, const u_char *pkt_data
		)
		#define pcap_sendqueue_queue slib::pcap::getApi_pcap_sendqueue_queue()

		SLIB_IMPORT_LIBRARY_FUNCTION(
			pcap_sendqueue_transmit,
			int, ,
			pcap_t *p, pcap_send_queue*, int sync
		)
		#define pcap_sendqueue_transmit slib::pcap::getApi_pcap_sendqueue_transmit()
#endif

		SLIB_IMPORT_LIBRARY_FUNCTION(
			pcap_geterr,
			char*, ,
			pcap_t *
		)
		#define pcap_geterr slib::pcap::getApi_pcap_geterr()

		SLIB_IMPORT_LIBRARY_FUNCTION(
			pcap_findalldevs,
			int, ,
			pcap_if_t **, char *
		)
		#define pcap_findalldevs slib::pcap::getApi_pcap_findalldevs()

		SLIB_IMPORT_LIBRARY_FUNCTION(
			pcap_freealldevs,
			void, ,
			pcap_if_t *
		)
		#define pcap_freealldevs slib::pcap::getApi_pcap_freealldevs()

	SLIB_IMPORT_LIBRARY_END

}

#endif

#endif
