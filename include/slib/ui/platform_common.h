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

#ifndef CHECKHEADER_SLIB_UI_PLATFORM_COMMON
#define CHECKHEADER_SLIB_UI_PLATFORM_COMMON

#include "../graphics/platform.h"

namespace slib
{

	class ViewInstance;
	class View;
	class WindowInstance;
	class Window;
	
}

#define PRIV_SLIB_DECLARE_UI_PLATFORM_COMMON_MEMBERS \
	public: \
		static void runLoop(sl_uint32 level); \
		static void quitLoop(); \
		static void initApp(); \
		static void runApp(); \
		static void quitApp(); \
	private: \
		static Ref<ViewInstance> _getViewInstance(const void* handle); \
		static void _registerViewInstance(const void* handle, ViewInstance* instance); \
		static void _removeViewInstance(const void* handle); \
		static Ref<WindowInstance> _getWindowInstance(const void* handle); \
		static void _registerWindowInstance(const void* handle, WindowInstance* instance); \
		static void _removeWindowInstance(const void* handle); \
		static List< Ref<WindowInstance> > _getAllWindowInstances();


#endif
