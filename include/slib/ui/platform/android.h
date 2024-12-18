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

#ifndef CHECKHEADER_SLIB_UI_ANDROID_PLATFORM
#define CHECKHEADER_SLIB_UI_ANDROID_PLATFORM

#include "../definition.h"

#if defined(SLIB_UI_IS_ANDROID)

#include "../platform_common.h"

namespace slib
{

	class SLIB_EXPORT UIPlatform
	{
		PRIV_SLIB_DECLARE_UI_PLATFORM_COMMON_MEMBERS

	public: 
		static Ref<ViewInstance> createViewInstance(jobject jhandle);
		static void registerViewInstance(jobject jhandle, ViewInstance* instance);
		static Ref<ViewInstance> getViewInstance(jobject jhandle);
		static void removeViewInstance(jobject jhandle);
		static jobject getViewHandle(ViewInstance* instance);
		static jobject getViewHandle(View* view);

		static Ref<WindowInstance> createWindowInstance(jobject window);
		static void registerWindowInstance(jobject window, WindowInstance* instance);
		static Ref<WindowInstance> getWindowInstance(jobject window);
		static void removeWindowInstance(jobject window);
		static jobject getWindowHandle(WindowInstance* instance);

		static void sendFile(const StringParam& filePath, const StringParam& mimeType, const StringParam& chooserTitle = sl_null);

	};

}

#endif

#endif