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

#ifndef CHECKHEADER_SLIB_SERVICE_DOKANY
#define CHECKHEADER_SLIB_SERVICE_DOKANY

#include "definition.h"

#ifdef SLIB_PLATFORM_IS_WIN32

#include "../core/service_manager.h"

namespace slib
{
	
	class SLIB_EXPORT Dokany
	{
	public:
		static sl_bool initialize(const StringParam& pathDll);

		static sl_bool initialize();

		static ServiceState getDriverState();

		static sl_bool startDriver();

		static sl_bool stopDriver();

		static sl_bool registerDriver(const StringParam& pathSys);

		static sl_bool registerDriver();

		static sl_bool registerAndStartDriver(const StringParam& pathSys);

		static sl_bool registerAndStartDriver();

		static sl_bool unregisterDriver();

		static sl_bool unmount(const StringParam& mountPoint);
		
	};

	class SLIB_EXPORT DokanyControl
	{
	public:
		static sl_bool install();

		static void uninstall();

		static void unmount(const StringParam& mountPoint);

	};
	
}

#endif

#endif
