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

#ifndef CHECKHEADER_SLIB_UI_EFL_PLATFORM
#define CHECKHEADER_SLIB_UI_EFL_PLATFORM

#include "../definition.h"

#if defined(SLIB_UI_IS_EFL)

#include "../platform_common.h"

#include <Evas.h>

enum class EFL_ViewType
{
	Generic = 0,
	Grid = 1,
	OpenGL = 2,
	Window = 3
};

namespace slib
{
	
	class SLIB_EXPORT UIPlatform
	{
		PRIV_SLIB_DECLARE_UI_PLATFORM_COMMON_MEMBERS

	public:
		static Ref<ViewInstance> createViewInstance(EFL_ViewType type, Evas_Object* handle, sl_bool flagFreeOnRelease = sl_true);
		static void registerViewInstance(Evas_Object* handle, ViewInstance* instance);
		static Ref<ViewInstance> getViewInstance(Evas_Object* handle);
		static Ref<View> getView(Evas_Object* handle);
		static void removeViewInstance(Evas_Object* handle);
		static Evas_Object* getViewHandle(ViewInstance* instance);
		static Evas_Object* getViewHandle(View* view);

		static Ref<WindowInstance> createWindowInstance(Evas_Object* handle);
		static void registerWindowInstance(Evas_Object* handle, WindowInstance* instance);
		static Ref<WindowInstance> getWindowInstance(Evas_Object* handle);
		static void removeWindowInstance(Evas_Object* handle);
		static Evas_Object* getWindowHandle(WindowInstance* instance);
		static Evas_Object* getWindowHandle(Window* window);

		static Evas_Object* getMainWindow();
		
	};

}

#endif

#endif