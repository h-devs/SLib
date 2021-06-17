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

#include "slib/core/definition.h"

#ifdef SLIB_PLATFORM_IS_ANDROID

#include "slib/core/android/window.h"
#include "slib/core/android/window_manager.h"
#include "slib/core/android/display.h"

namespace slib
{
	
	namespace priv
	{
		namespace android_context
		{

			SLIB_JNI_BEGIN_CLASS(JWindow, "android/view/Window")
				SLIB_JNI_METHOD(setSoftInputMode, "setSoftInputMode", "(I)V")
			SLIB_JNI_END_CLASS

			SLIB_JNI_BEGIN_CLASS(JWindowManager, "android/view/WindowManager")
				SLIB_JNI_METHOD(getDefaultDisplay, "getDefaultDisplay", "()Landroid/view/Display;")
			SLIB_JNI_END_CLASS

			SLIB_JNI_BEGIN_CLASS(JDisplay, "android/view/Display")
				SLIB_JNI_METHOD(getMetrics, "getMetrics", "(Landroid/util/DisplayMetrics;)V")
			SLIB_JNI_END_CLASS

			SLIB_JNI_BEGIN_CLASS(JDisplayMetrics, "android/util/DisplayMetrics")
				SLIB_JNI_NEW(init, "()V")
				SLIB_JNI_INT_FIELD(widthPixels)
				SLIB_JNI_INT_FIELD(heightPixels)
				SLIB_JNI_INT_FIELD(densityDpi)
			SLIB_JNI_END_CLASS

		}
	}

	using namespace priv::android_context;

	namespace android
	{

		void Window::setSoftInputMode(jobject thiz, sl_uint32 mode) noexcept
		{
			JWindow::setSoftInputMode.callObject(thiz, mode);
		}


		JniLocal<jobject> WindowManager::getDefaultDisplay(jobject thiz) noexcept
		{
			return JWindowManager::getDefaultDisplay.callObject(thiz);
		}


		JniLocal<jobject> Display::getMetrics(jobject thiz) noexcept
		{
			JniLocal<jobject> metrics = JDisplayMetrics::init.newObject(sl_null);
			if (metrics.isNotNull()) {
				JDisplay::getMetrics.call(thiz, metrics.get());
				return metrics;
			}
			return sl_null;
		}


		sl_int32 DisplayMetrics::getWidthPixels(jobject thiz) noexcept
		{
			return JDisplayMetrics::widthPixels.get(thiz);
		}

		sl_int32 DisplayMetrics::getHeightPixels(jobject thiz) noexcept
		{
			return JDisplayMetrics::heightPixels.get(thiz);
		}

		sl_int32 DisplayMetrics::getDensityDpi(jobject thiz) noexcept
		{
			return JDisplayMetrics::densityDpi.get(thiz);
		}

	}

}

#endif
