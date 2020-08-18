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

#ifndef CHECKHEADER_SLIB_NETWORK_DL_LINUX_CURL
#define CHECKHEADER_SLIB_NETWORK_DL_LINUX_CURL

#include "definition.h"

#if defined(SLIB_PLATFORM_IS_LINUX) && defined(SLIB_PLATFORM_IS_DESKTOP)

#include "dl.h"

#include "curl/curl.h"

namespace slib
{

	SLIB_IMPORT_LIBRARY_BEGIN(curl, "libcurl.so.4", "libcurl.so.3")

		SLIB_IMPORT_LIBRARY_FUNCTION(
			curl_easy_init,
			CURL*
		)
		#define curl_easy_init slib::curl::getApi_curl_easy_init()

		SLIB_IMPORT_LIBRARY_FUNCTION(
			curl_easy_setopt,
			CURLcode, ,
			CURL *curl, CURLoption option, ...
		)
		#define curl_easy_setopt slib::curl::getApi_curl_easy_setopt()

		SLIB_IMPORT_LIBRARY_FUNCTION(
			curl_easy_perform,
			CURLcode, ,
			CURL *curl
		)
		#define curl_easy_perform slib::curl::getApi_curl_easy_perform()

		SLIB_IMPORT_LIBRARY_FUNCTION(
			curl_easy_cleanup,
			void, ,
			CURL *curl
		)
		#define curl_easy_cleanup slib::curl::getApi_curl_easy_cleanup()
		
		SLIB_IMPORT_LIBRARY_FUNCTION(
			curl_easy_getinfo,
			CURLcode, ,
			CURL *curl, CURLINFO info, ...
		)
		#define curl_easy_getinfo slib::curl::getApi_curl_easy_getinfo()
		
		SLIB_IMPORT_LIBRARY_FUNCTION(
			curl_easy_strerror,
			const char *, ,
			CURLcode
		)
		#define curl_easy_strerror slib::curl::getApi_curl_easy_strerror()

		SLIB_IMPORT_LIBRARY_FUNCTION(
			curl_slist_append,
			struct curl_slist*, ,
			struct curl_slist *, const char *
		)
		#define curl_slist_append slib::curl::getApi_curl_slist_append()

		SLIB_IMPORT_LIBRARY_FUNCTION(
			curl_slist_free_all,
			void, ,
			struct curl_slist*
		)
		#define curl_slist_free_all slib::curl::getApi_curl_slist_free_all()

	SLIB_IMPORT_LIBRARY_END

}

#endif

#endif
