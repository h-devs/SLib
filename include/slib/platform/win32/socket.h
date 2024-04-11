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

#ifndef CHECKHEADER_SLIB_PLATFORM_WIN32_SOCKET
#define CHECKHEADER_SLIB_PLATFORM_WIN32_SOCKET

#include "../definition.h"

#if defined(SLIB_PLATFORM_IS_WINDOWS)

#	include <winsock2.h>
#	include <ws2tcpip.h>
#	include <mswsock.h>

struct SOCKADDR_UN
{
	ADDRESS_FAMILY sun_family; // AF_UNIX
	char sun_path[108];
};
#ifdef sockaddr_un
#undef sockaddr_un
#endif
#define sockaddr_un SOCKADDR_UN

#ifndef AF_UNIX
#	define AF_UNIX 1
#endif

#ifdef CMSG_DATA
#	undef CMSG_DATA
#	define CMSG_DATA WSA_CMSG_DATA
#endif

#	pragma comment(lib, "ws2_32.lib")

#endif

#endif
