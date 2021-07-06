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

#ifndef CHECKHEADER_SLIB_RENDER_DL_LINUX_GL
#define CHECKHEADER_SLIB_RENDER_DL_LINUX_GL

#include "../../../core/definition.h"

#if defined(SLIB_PLATFORM_IS_LINUX_DESKTOP)

#include "../../../core/dl.h"

#include "gl/GLX/glx.h"
#include "X11/slib_fix.h"

namespace slib
{

	SLIB_IMPORT_LIBRARY_BEGIN(glx, "libGL.so.1")

		SLIB_IMPORT_LIBRARY_FUNCTION(
			glXGetProcAddress,
			void*, ,
			const GLubyte *procname
		)

		SLIB_IMPORT_LIBRARY_FUNCTION(
			glXChooseVisual,
			XVisualInfo*, ,
			Display *dpy, int screen, int *attribList
		) 
		#define glXChooseVisual slib::glx::getApi_glXChooseVisual()

		SLIB_IMPORT_LIBRARY_FUNCTION(
			glXCreateContext,
			GLXContext, ,
			Display *dpy, XVisualInfo *vis, GLXContext shareList, Bool direct
		) 
		#define glXCreateContext slib::glx::getApi_glXCreateContext()

		SLIB_IMPORT_LIBRARY_FUNCTION(
			glXDestroyContext,
			void, ,
			Display *dpy, GLXContext ctx
		) 
		#define glXDestroyContext slib::glx::getApi_glXDestroyContext()

		SLIB_IMPORT_LIBRARY_FUNCTION(
			glXMakeCurrent,
			Bool, ,
			Display *dpy, GLXDrawable drawable, GLXContext ctx
		) 
		#define glXMakeCurrent slib::glx::getApi_glXMakeCurrent()

		SLIB_IMPORT_LIBRARY_FUNCTION(
			glXSwapBuffers,
			void, ,
			Display *dpy, GLXDrawable drawable
		) 
		#define glXSwapBuffers slib::glx::getApi_glXSwapBuffers()

	SLIB_IMPORT_LIBRARY_END

}

#endif

#endif
