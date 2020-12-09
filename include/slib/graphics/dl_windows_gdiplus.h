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

#ifndef CHECKHEADER_SLIB_CORE_DL_WINDOWS_GDIPLUS
#define CHECKHEADER_SLIB_CORE_DL_WINDOWS_GDIPLUS

#include "definition.h"

#if defined(SLIB_PLATFORM_IS_WINDOWS)

#include "../core/dl.h"

#include "../core/windows.h"

#include <objidl.h>

#define GDIPVER 0x0110
#define GdipCreateEffect SLIB_GdipCreateEffect
#define GdipDeleteEffect SLIB_GdipDeleteEffect
#define GdipSetEffectParameters SLIB_GdipSetEffectParameters
#define GdipDrawImageFX SLIB_GdipDrawImageFX
#include <gdiplus.h>
#undef GdipCreateEffect
#undef GdipDeleteEffect
#undef GdipSetEffectParameters
#undef GdipDrawImageFX

namespace slib
{

	SLIB_IMPORT_LIBRARY_BEGIN(gdiplus, "gdiplus.dll")
		SLIB_IMPORT_LIBRARY_FUNCTION(
			GdipCreateEffect,
			Gdiplus::Status, __stdcall,
			const GUID guid,
			Gdiplus::CGpEffect **effect
		)
		SLIB_IMPORT_LIBRARY_FUNCTION(
			GdipDeleteEffect,
			Gdiplus::Status, __stdcall,
			Gdiplus::CGpEffect *effect
		)
		SLIB_IMPORT_LIBRARY_FUNCTION(
			GdipSetEffectParameters,
			Gdiplus::Status, __stdcall,
			Gdiplus::CGpEffect *effect,
			const VOID *params,
			const UINT size
		)
		SLIB_IMPORT_LIBRARY_FUNCTION(
			GdipDrawImageFX,
			Gdiplus::GpStatus, WINGDIPAPI,
			Gdiplus::GpGraphics *graphics,
			Gdiplus::GpImage *image,
			Gdiplus::GpRectF *source,
			Gdiplus::GpMatrix *xForm,
			Gdiplus::CGpEffect *effect,
			Gdiplus::GpImageAttributes *imageAttributes,
			Gdiplus::GpUnit srcUnit
		)
	SLIB_IMPORT_LIBRARY_END

}

#endif

#endif
