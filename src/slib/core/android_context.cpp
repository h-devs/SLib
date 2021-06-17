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

#include "slib/core/android/context.h"
#include "slib/core/android/activity.h"
#include "slib/core/android/preference.h"

namespace slib
{
	
	namespace priv
	{
		namespace android_context
		{

			SLIB_JNI_BEGIN_CLASS(JContext, "android/content/Context")
				SLIB_JNI_METHOD(getSystemService, "getSystemService", "(Ljava/lang/String;)Ljava/lang/Object;")
				SLIB_JNI_METHOD(getExternalFilesDir, "getExternalFilesDir", "(Ljava/lang/String;)Ljava/io/File;")
				SLIB_JNI_METHOD(getAssets, "getAssets", "()Landroid/content/res/AssetManager;")
				SLIB_JNI_METHOD(getSharedPreferences, "getSharedPreferences", "(Ljava/lang/String;I)Landroid/content/SharedPreferences;")

				SLIB_JNI_FINAL_STRING_OBJECT_FIELD(AUDIO_SERVICE)
				SLIB_JNI_FINAL_STRING_OBJECT_FIELD(VIBRATOR_SERVICE)
				SLIB_JNI_FINAL_STRING_OBJECT_FIELD(TELEPHONY_SERVICE)
				SLIB_JNI_FINAL_STRING_OBJECT_FIELD(TELEPHONY_SUBSCRIPTION_SERVICE)
			SLIB_JNI_END_CLASS

			SLIB_JNI_BEGIN_CLASS(JActivity, "android/app/Activity")
				SLIB_JNI_METHOD(finish, "finish", "()V")
				SLIB_JNI_METHOD(getWindowManager, "getWindowManager", "()Landroid/view/WindowManager;")
				SLIB_JNI_METHOD(getWindow, "getWindow", "()Landroid/view/Window;")
			SLIB_JNI_END_CLASS

			SLIB_JNI_BEGIN_CLASS(JEnvironment, "android/os/Environment")
				SLIB_JNI_FINAL_STRING_OBJECT_FIELD(DIRECTORY_PICTURES)
			SLIB_JNI_END_CLASS
			
			SLIB_JNI_BEGIN_CLASS(JSharedPreferences, "android/content/SharedPreferences")
				SLIB_JNI_METHOD(edit, "edit", "()Landroid/content/SharedPreferences$Editor;")				
				SLIB_JNI_METHOD(getString, "getString", "(Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;")
			SLIB_JNI_END_CLASS

			SLIB_JNI_BEGIN_CLASS(JSharedPreferencesEditor, "android/content/SharedPreferences$Editor")
				SLIB_JNI_METHOD(apply, "apply", "()V")
				SLIB_JNI_METHOD(putString, "putString", "(Ljava/lang/String;Ljava/lang/String;)Landroid/content/SharedPreferences$Editor;")
			SLIB_JNI_END_CLASS

		}
	}

	using namespace priv::android_context;

	namespace android
	{

		JniLocal<jobject> Context::getSystemService(jobject thiz, jstring name) noexcept
		{
			if (name) {
				return JContext::getSystemService.callObject(thiz, name);
			}
			return sl_null;
		}

		JniLocal<jobject> Context::getAudioManager(jobject thiz)
		{
			return getSystemService(thiz, JContext::AUDIO_SERVICE.get());
		}

		JniLocal<jobject> Context::getVibrator(jobject thiz)
		{
			return getSystemService(thiz, JContext::VIBRATOR_SERVICE.get());
		}

		JniLocal<jobject> Context::getTelephonyManager(jobject thiz)
		{
			return getSystemService(thiz, JContext::TELEPHONY_SERVICE.get());
		}

		JniLocal<jobject> Context::getTelephonySubscriptionManager(jobject thiz)
		{
			return getSystemService(thiz, JContext::TELEPHONY_SUBSCRIPTION_SERVICE.get());
		}

		JniLocal<jobject> Context::getExternalFilesDir(jobject thiz, jstring type) noexcept
		{
			if (type) {
				return JContext::getExternalFilesDir.callObject(thiz, type);
			}
			return sl_null;
		}

		JniLocal<jobject> Context::getPicturesDir(jobject thiz) noexcept
		{
			return getExternalFilesDir(thiz, JEnvironment::DIRECTORY_PICTURES.get());
		}

		JniLocal<jobject> Context::getAssets(jobject thiz) noexcept
		{
			return JContext::getAssets.callObject(thiz);
		}
		
		JniLocal<jobject> Context::getSharedPreferences(jobject thiz, const StringParam& _name, sl_uint32 mode) noexcept
		{
			if (thiz) {
				JniLocal<jstring> name = Jni::getJniString(_name);
				if (name.isNotNull()) {
					return JContext::getSharedPreferences.callObject(thiz, name.get(), (jint)mode);
				}
			}
			return sl_null;
		}


		sl_bool Activity::isActivity(jobject object) noexcept
		{
			return Jni::isInstanceOf(object, JActivity::get());			
		}

		void Activity::finish(jobject thiz) noexcept
		{
			JActivity::finish.call(thiz);
		}

		JniLocal<jobject> Activity::getWindowManager(jobject thiz) noexcept
		{
			return JActivity::getWindowManager.callObject(thiz);
		}

		JniLocal<jobject> Activity::getWindow(jobject thiz) noexcept
		{
			return JActivity::getWindow.callObject(thiz);
		}


		JniLocal<jobject> SharedPreferences::getEditor(jobject thiz) noexcept
		{
			return JSharedPreferences::edit.callObject(thiz);
		}

		String SharedPreferences::getString(jobject thiz, const StringParam& _key, const StringParam& _def) noexcept
		{
			if (thiz) {
				JniLocal<jstring> key = Jni::getJniString(_key);
				if (key.isNotNull()) {
					JniLocal<jstring> def = Jni::getJniString(_def);
					if (def.isNotNull()) {
						return JSharedPreferences::getString.callString(thiz, key.get(), def.get());						
					}
				}
			}
			return sl_null;
		}

		void SharedPreferencesEditor::apply(jobject thiz) noexcept
		{
			return JSharedPreferencesEditor::apply.call(thiz);
		}

		void SharedPreferencesEditor::putString(jobject thiz, const StringParam& _key, const StringParam& _value) noexcept
		{
			if (thiz) {
				JniLocal<jstring> key = Jni::getJniString(_key);
				if (key.isNotNull()) {
					JniLocal<jstring> value = Jni::getJniString(_value);
					if (value.isNotNull()) {
						JSharedPreferencesEditor::putString.callObject(thiz, key.get(), value.get());						
					}
				}
			}
		}

	}

}

#endif
