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

#ifndef CHECKHEADER_SLIB_CORE_ANDROID_CONTEXT
#define CHECKHEADER_SLIB_CORE_ANDROID_CONTEXT

#include "../definition.h"

#if defined(SLIB_PLATFORM_IS_ANDROID)

#include "../java.h"

namespace slib
{
	namespace android
	{

		class SLIB_EXPORT Context
		{
		public:
			static JniLocal<jobject> getSystemService(jobject thiz, jstring name) noexcept;
			static JniLocal<jobject> getAudioManager(jobject thiz); // AUDIO_SERVICE
			static JniLocal<jobject> getVibrator(jobject thiz); // VIBRATOR_SERVICE
			static JniLocal<jobject> getTelephonyManager(jobject thiz); // TELEPHONY_SERVICE
			static JniLocal<jobject> getTelephonySubscriptionManager(jobject thiz); // TELEPHONY_SUBSCRIPTION_SERVICE

			static JniLocal<jobject> getExternalFilesDir(jobject thiz, jstring type) noexcept;
			static JniLocal<jobject> getPicturesDir(jobject thiz) noexcept; // android.os.Environment.DIRECTORY_PICTURES

			static JniLocal<jobject> getAssets(jobject thiz) noexcept;

			static JniLocal<jobject> getSharedPreferences(jobject thiz, const StringParam& name, sl_uint32 mode) noexcept;

		};

	}

}

#endif

#endif
