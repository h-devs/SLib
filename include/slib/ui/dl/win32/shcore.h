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

#ifndef CHECKHEADER_SLIB_UI_DL_WIN32_SHCORE
#define CHECKHEADER_SLIB_UI_DL_WIN32_SHCORE

#include "../../definition.h"

#if defined(SLIB_PLATFORM_IS_WINDOWS)

#include "../../../core/dl.h"

#include "../../../core/win32/windows.h"

namespace slib
{

	SLIB_IMPORT_LIBRARY_BEGIN(shcore, "shcore.dll")

		enum PROCESS_DPI_AWARENESS
		{
			PROCESS_DPI_UNAWARE = 0,
			PROCESS_SYSTEM_DPI_AWARE = 1,
			PROCESS_PER_MONITOR_DPI_AWARE = 2
		};
		SLIB_IMPORT_LIBRARY_FUNCTION(
			GetProcessDpiAwareness,
			HRESULT, WINAPI,
			HANDLE hProcess,
			PROCESS_DPI_AWARENESS* value
		)

		SLIB_IMPORT_LIBRARY_FUNCTION(
			GetDpiForMonitor,
			HRESULT, WINAPI,
			HMONITOR hMonitor,
			int dpiType, /* MONITOR_DPI_TYPE */
			UINT *dpiX,
			UINT *dpiY
		)

		SLIB_IMPORT_LIBRARY_FUNCTION(
			GetScaleFactorForMonitor,
			HRESULT, WINAPI,
			HMONITOR hmonitor,
			UINT *pScale
		)

	SLIB_IMPORT_LIBRARY_END

}

#endif

#endif
