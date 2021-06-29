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

#include "slib/render/opengl.h"

namespace slib
{

	SLIB_DEFINE_OBJECT(GLRenderEngine, RenderEngine)
	
	GLRenderEngine::GLRenderEngine()
	{
	}
	
	GLRenderEngine::~GLRenderEngine()
	{
	}
	
}

#if defined(SLIB_RENDER_SUPPORT_OPENGL_GL)

#include "slib/render/program.h"
#include "slib/core/queue.h"
#include "slib/core/event.h"

#include "opengl_gl.h"

#define PRIV_OPENGL_IMPL
#define GL_BASE GL
#define GL_ENGINE GL_Engine
#define GL_ENTRY(x) PRIV_GL_ENTRY(x)
#include "opengl_impl.h"

#if defined(SLIB_PLATFORM_IS_WIN32) || (defined(SLIB_PLATFORM_IS_LINUX) && defined(SLIB_PLATFORM_IS_DESKTOP))

#include "slib/core/dynamic_library.h"

namespace slib
{

	namespace priv
	{
		namespace gl
		{

			EntryPoints g_entries;

			static sl_bool g_flagLoadedEntryPoints = sl_false;

#if defined(SLIB_PLATFORM_IS_WIN32)
			static void* GetFunctionAddress(const char* name)
			{
				return wglGetProcAddress(name);
			}
#else
			static void* GetFunctionAddress(const char* name)
			{
				auto func = glx::getApi_glXGetProcAddress();
				if (func) {
					return func((GLubyte*)name);
				}
				return sl_null;
			}
#endif

		}
	}

	using namespace priv::gl;
	
#undef PRIV_SLIB_RENDER_GL_ENTRY
#define PRIV_SLIB_RENDER_GL_ENTRY(TYPE, name, ...) \
	if (dl) { \
		proc = DynamicLibrary::getFunctionAddress(dl, #name); \
	} else { \
		proc = GetFunctionAddress(#name); \
		if (!proc) { \
			if (dlCommon) { \
				proc = DynamicLibrary::getFunctionAddress(dlCommon, #name); \
			} \
		} \
	} \
	if (!proc) { \
		flagSupport = sl_false; \
	} \
	*((void**)(&(g_entries.name))) = proc;
	
#undef PRIV_SLIB_RENDER_GL_SUPPORT
#define PRIV_SLIB_RENDER_GL_SUPPORT(name) \
	g_entries.flagSupports##name = flagSupport; \
	flagSupport = sl_true;
	
	void GL::loadEntries(const StringParam& pathDll, sl_bool flagReload)
	{
		if (!flagReload) {
			if (g_flagLoadedEntryPoints) {
				return;
			}
		}
		void* dl;
		if (pathDll.isEmpty()) {
			dl = sl_null;
		} else {
			dl = DynamicLibrary::loadLibrary(pathDll);
			if (!dl) {
				return;
			}
		}
#if defined(SLIB_PLATFORM_IS_WIN32)
		void* dlCommon = DynamicLibrary::loadLibrary(L"opengl32.dll");
#else
		void* dlCommon = DynamicLibrary::loadLibrary(L"libGL.so.1");
#endif
		void* proc;
		sl_bool flagSupport = sl_true;
		PRIV_SLIB_RENDER_GL_ENTRIES
		g_flagLoadedEntryPoints = sl_true;
	}
	
	void GL::loadEntries(sl_bool flagReload)
	{
		loadEntries(String::null(), flagReload);
	}
	
	sl_bool GL::isAvailable()
	{
		return g_flagLoadedEntryPoints;
	}

	sl_bool GL::isShaderAvailable()
	{
		return g_flagLoadedEntryPoints && g_entries.flagSupportsVersion_2_0;
	}
}

#else

namespace slib
{
	void GL::loadEntries(const StringParam& pathDll, sl_bool flagReload)
	{
	}
	
	void GL::loadEntries(sl_bool flagReload)
	{
	}
	
	sl_bool GL::isAvailable()
	{
		return sl_true;
	}

	sl_bool GL::isShaderAvailable()
	{
		return sl_true;
	}
}

#endif

#else

namespace slib
{
	Ref<GLRenderEngine> GL::createEngine()
	{
		return sl_null;
	}
	
	void GL::loadEntries(const StringParam& pathDll, sl_bool flagReload)
	{
	}
	
	void GL::loadEntries(sl_bool flagReload)
	{
	}
	
	sl_bool GL::isAvailable()
	{
		return sl_false;
	}

	sl_bool GL::isShaderAvailable()
	{
		return sl_false;
	}
}

#endif


