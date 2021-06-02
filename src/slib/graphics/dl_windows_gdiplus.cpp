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

#define SLIB_IMPLEMENT_DYNAMIC_LIBRARY

#include "slib/graphics/dl/win32/gdiplus.h"

extern "C"
{

	Gdiplus::Status __stdcall SLIB_GdipCreateEffect(const GUID guid, Gdiplus::CGpEffect **effect)
	{
		auto func = slib::gdiplus::getApi_GdipCreateEffect();
		if (func) {
			return func(guid, effect);
		}
		return Gdiplus::AccessDenied;
	}

	Gdiplus::Status __stdcall SLIB_GdipDeleteEffect(Gdiplus::CGpEffect *effect)
	{
		auto func = slib::gdiplus::getApi_GdipDeleteEffect();
		if (func) {
			return func(effect);
		}
		return Gdiplus::AccessDenied;
	}

	Gdiplus::Status __stdcall SLIB_GdipSetEffectParameters(Gdiplus::CGpEffect *effect, const VOID *params, const UINT size)
	{
		auto func = slib::gdiplus::getApi_GdipSetEffectParameters();
		if (func) {
			return func(effect, params, size);
		}
		return Gdiplus::AccessDenied;
	}

	Gdiplus::GpStatus WINGDIPAPI SLIB_GdipDrawImageFX(
		Gdiplus::GpGraphics *graphics,
		Gdiplus::GpImage *image,
		Gdiplus::GpRectF *source,
		Gdiplus::GpMatrix *xForm,
		Gdiplus::CGpEffect *effect,
		Gdiplus::GpImageAttributes *imageAttributes,
		Gdiplus::GpUnit srcUnit)
	{
		auto func = slib::gdiplus::getApi_GdipDrawImageFX();
		if (func) {
			return func(graphics, image, source, xForm, effect, imageAttributes, srcUnit);
		}
		return Gdiplus::AccessDenied;
	}

}
