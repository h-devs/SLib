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

#ifndef CHECKHEADER_SLIB_DL_MACOS_SECURITY_FRAMEWORK
#define CHECKHEADER_SLIB_DL_MACOS_SECURITY_FRAMEWORK

#include "../dl.h"

#if defined(SLIB_PLATFORM_IS_MACOS)

#include <Security/Security.h>

namespace slib
{

	SLIB_IMPORT_LIBRARY_BEGIN(security_framework, "/System/Library/Frameworks/Security.framework/Security")

		SLIB_IMPORT_LIBRARY_FUNCTION(
			AuthorizationCreate,
			OSStatus, ,
			const AuthorizationRights*,
			const AuthorizationEnvironment*,
			AuthorizationFlags,
			AuthorizationRef*
		)
		#define AuthorizationCreate slib::security_framework::getApi_AuthorizationCreate()

		SLIB_IMPORT_LIBRARY_FUNCTION(
			AuthorizationFree,
			OSStatus, ,
			AuthorizationRef,
			AuthorizationFlags
		)
		#define AuthorizationFree slib::security_framework::getApi_AuthorizationFree()

		SLIB_IMPORT_LIBRARY_FUNCTION(
			AuthorizationExecuteWithPrivileges,
			OSStatus, ,
			AuthorizationRef,
			const char* pathToTool,
			AuthorizationFlags,
			char* const* arguments,
			FILE** communicationsPipe
		)
		#define AuthorizationExecuteWithPrivileges slib::security_framework::getApi_AuthorizationExecuteWithPrivileges()

	SLIB_IMPORT_LIBRARY_END

}

#endif

#endif
