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

#ifndef CHECKHEADER_SLIB_UI_DL_LINUX_X11
#define CHECKHEADER_SLIB_UI_DL_LINUX_X11

#include "../../../core/definition.h"

#if defined(SLIB_PLATFORM_IS_LINUX_DESKTOP)

#include "../../../core/dl.h"

#include "X11/Xlib.h"
#include "X11/Xutil.h"

namespace slib
{

	SLIB_IMPORT_LIBRARY_BEGIN(x11, "libX11.so.6")

		SLIB_IMPORT_LIBRARY_FUNCTION(
			XSetErrorHandler,
			XErrorHandler, ,
			XErrorHandler	/* handler */
		)
		#define XSetErrorHandler slib::x11::getApi_XSetErrorHandler()

		SLIB_IMPORT_LIBRARY_FUNCTION(
			XGetErrorText,
			int, ,
			Display*	/* display */,
			int			/* code */,
			char*		/* buffer_return */,
			int			/* length */
		)
		#define XGetErrorText slib::x11::getApi_XGetErrorText()

		SLIB_IMPORT_LIBRARY_FUNCTION(
			XOpenDisplay,
			Display *, ,
			_Xconst char*	/* display_name */
		)
		#define XOpenDisplay slib::x11::getApi_XOpenDisplay()

		SLIB_IMPORT_LIBRARY_FUNCTION(
			XCloseDisplay,
			int, ,
			Display*
		)
		#define XCloseDisplay slib::x11::getApi_XCloseDisplay()

		SLIB_IMPORT_LIBRARY_FUNCTION(
			XGetWindowAttributes,
			Status, ,
			Display*, Window, XWindowAttributes*
		)
		#define XGetWindowAttributes slib::x11::getApi_XGetWindowAttributes()

		SLIB_IMPORT_LIBRARY_FUNCTION(
			XGetImage,
			XImage *, ,
			Display*		/* display */,
			XID				/* drawable */,
			int				/* x */,
			int				/* y */,
			unsigned int	/* width */,
			unsigned int	/* height */,
			unsigned long	/* plane_mask */,
			int				/* format */
		)
		#define XGetImage slib::x11::getApi_XGetImage()

		SLIB_IMPORT_LIBRARY_FUNCTION(
			XMapRaised,
			int, ,
			Display*	/* display */,
			Window		/* w */
		)
		#define XMapRaised slib::x11::getApi_XMapRaised()

	SLIB_IMPORT_LIBRARY_END

}

#endif

#endif
