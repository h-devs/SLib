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

#ifndef CHECKHEADER_SLIB_DL_WIN32_T2EMBED
#define CHECKHEADER_SLIB_DL_WIN32_T2EMBED

#include "../dl.h"

#if defined(SLIB_PLATFORM_IS_WINDOWS)

#include "../../platform/win32/windows.h"

namespace slib
{
	typedef unsigned long (CALLBACK *READ_EMBED_FONT_CALLBACK)(
		void *lpvReadStream,
		void *lpvBuffer,
		const unsigned long cbBuffer
	);

	SLIB_IMPORT_LIBRARY_BEGIN(t2embed, "t2embed.dll")
		SLIB_IMPORT_LIBRARY_FUNCTION(
			TTLoadEmbeddedFont,
			LONG, CALLBACK,
			HANDLE *phFontReference,
			ULONG ulFlags,
			ULONG *pulPrivStatus,
			ULONG ulPrivs,
			ULONG *pulStatus,
			READ_EMBED_FONT_CALLBACK lpfnReadFromStream,
			LPVOID lpvReadStream,
			LPWSTR szWinFamilyName,
			LPSTR szMacFamilyName,
			void* pTTLoadInfo /* TTLOADINFO* */
		)

		SLIB_IMPORT_LIBRARY_FUNCTION(
			TTDeleteEmbeddedFont,
			LONG, CALLBACK,
			HANDLE hFontReference,
			ULONG ulFlags,
			ULONG *pulStatus
		)
	SLIB_IMPORT_LIBRARY_END

}

#endif

#endif
