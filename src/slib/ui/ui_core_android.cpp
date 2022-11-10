/*
 *   Copyright (c) 2008-2018 SLIBIO <https://github.com/SLIBIO>
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

#include "slib/ui/definition.h"

#if defined(SLIB_UI_IS_ANDROID)

#include "slib/ui/core.h"

#include "slib/ui/screen.h"
#include "slib/ui/platform.h"
#include "slib/ui/mobile_app.h"
#include "slib/ui/resource.h"
#include "slib/device/device.h"
#include "slib/core/locale.h"
#include "slib/core/log.h"
#include "slib/core/safe_static.h"

#include "slib/core/android/activity.h"
#include "slib/core/android/window.h"

#include "ui_core_common.h"

namespace slib
{

	namespace priv
	{

		namespace ui_core
		{

			Ref<UIApp> g_app;

			SLIB_JNI_BEGIN_CLASS(JRect, "android/graphics/Rect")
				SLIB_JNI_INT_FIELD(left);
				SLIB_JNI_INT_FIELD(top);
				SLIB_JNI_INT_FIELD(right);
				SLIB_JNI_INT_FIELD(bottom);
			SLIB_JNI_END_CLASS

			SLIB_JNI_BEGIN_CLASS(JUtil, "slib/android/ui/Util")
				SLIB_JNI_STATIC_METHOD(getScreenOrientation, "getScreenOrientation", "(Landroid/app/Activity;)I")
				SLIB_JNI_STATIC_METHOD(setScreenOrientations, "setScreenOrientations", "(Landroid/app/Activity;ZZZZ)V")
				SLIB_JNI_STATIC_METHOD(showKeyboard, "showKeyboard", "(Landroid/app/Activity;)V")
				SLIB_JNI_STATIC_METHOD(dismissKeyboard, "dismissKeyboard", "(Landroid/app/Activity;)V")
				SLIB_JNI_STATIC_METHOD(getSafeAreaInsets, "getSafeAreaInsets", "(Landroid/app/Activity;)Landroid/graphics/Rect;")
				SLIB_JNI_STATIC_METHOD(getStatusBarHeight, "getStatusBarHeight", "(Landroid/content/Context;)I")
				SLIB_JNI_STATIC_METHOD(setStatusBarStyle, "setStatusBarStyle", "(Landroid/app/Activity;ZZ)V")
				SLIB_JNI_STATIC_METHOD(setBadgeNumber, "setBadgeNumber", "(Landroid/content/Context;I)V")
				SLIB_JNI_STATIC_METHOD(openUrl, "openUrl", "(Landroid/content/Context;Ljava/lang/String;)V")
				SLIB_JNI_STATIC_METHOD(sendFile, "sendFile", "(Landroid/content/Context;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V")
			SLIB_JNI_END_CLASS

			void DispatchCallback(JNIEnv* env, jobject _this);
			void DispatchDelayedCallback(JNIEnv* env, jobject _this, jlong ptr);

			SLIB_JNI_BEGIN_CLASS(JUiThread, "slib/android/ui/UiThread")
				SLIB_JNI_STATIC_METHOD(isUiThread, "isUiThread", "()Z");
				SLIB_JNI_STATIC_METHOD(dispatch, "dispatch", "()V");
				SLIB_JNI_STATIC_METHOD(dispatchDelayed, "dispatchDelayed", "(JI)V");
				SLIB_JNI_STATIC_METHOD(runLoop, "runLoop", "()V");
				SLIB_JNI_STATIC_METHOD(quitLoop, "quitLoop", "()V");

				SLIB_JNI_NATIVE(nativeDispatchCallback, "nativeDispatchCallback", "()V", DispatchCallback);
				SLIB_JNI_NATIVE(nativeDispatchDelayedCallback, "nativeDispatchDelayedCallback", "(J)V", DispatchDelayedCallback);
			SLIB_JNI_END_CLASS

			void OnCreateActivity(JNIEnv* env, jobject _this, jobject activity);
			void OnDestroyActivity(JNIEnv* env, jobject _this, jobject activity);
			void OnResumeActivity(JNIEnv* env, jobject _this, jobject activity);
			void OnPauseActivity(JNIEnv* env, jobject _this, jobject activity);
			jboolean OnBack(JNIEnv* env, jobject _this, jobject activity);
			void OnConfigurationChanged(JNIEnv* env, jobject _this, jobject activity);
			void OnChangeWindowInsets(JNIEnv* env, jobject _this, jobject activity);
			void OnOpenUrl(JNIEnv* env, jobject _this, jobject activity, jstring url);

			SLIB_JNI_BEGIN_CLASS(JAndroid, "slib/android/Android")
				SLIB_JNI_NATIVE(onCreateActivity, "nativeOnCreateActivity", "(Landroid/app/Activity;)V", OnCreateActivity);
				SLIB_JNI_NATIVE(onDestroyActivity, "nativeOnDestroyActivity", "(Landroid/app/Activity;)V", OnDestroyActivity);
				SLIB_JNI_NATIVE(onResumeActivity, "nativeOnResumeActivity", "(Landroid/app/Activity;)V", OnResumeActivity);
				SLIB_JNI_NATIVE(onPauseActivity, "nativeOnPauseActivity", "(Landroid/app/Activity;)V", OnPauseActivity);
				SLIB_JNI_NATIVE(onBack, "nativeOnBack", "(Landroid/app/Activity;)Z", OnBack);
				SLIB_JNI_NATIVE(onConfigurationChanged, "nativeOnConfigurationChanged", "(Landroid/app/Activity;)V", OnConfigurationChanged);
				SLIB_JNI_NATIVE(onChangeWindowInsets, "nativeOnChangeWindowInsets", "(Landroid/app/Activity;)V", OnChangeWindowInsets);
				SLIB_JNI_NATIVE(onOpenUrl, "nativeOnOpenUrl", "(Landroid/app/Activity;Ljava/lang/String;)V", OnOpenUrl);
			SLIB_JNI_END_CLASS

			class ScreenImpl : public Screen
			{
			public:
				sl_ui_len m_width;
				sl_ui_len m_height;

			public:
				static Ref<ScreenImpl> create()
				{
					Ref<ScreenImpl> ret = new ScreenImpl();
					if (ret.isNotNull()) {
						UISize size = UI::getScreenSize();
						ret->m_width = size.x;
						ret->m_height = size.y;
						return ret;
					}
					return sl_null;
				}

			public:
				UIRect getRegion() override
				{
					UIRect ret;
					ret.left = 0;
					ret.top = 0;
					ret.right = (sl_ui_pos)m_width;
					ret.bottom = (sl_ui_pos)m_height;
					return ret;
				}
			};

			void OnCreateActivity(JNIEnv* env, jobject _this, jobject activity)
			{
				Log("Activity", "onCreateActivity");
				Android::initializeContext(activity);
				Ref<UIApp> app = UIApp::getApp();
				if (app.isNotNull()) {
					static sl_bool flagStartApp = sl_false;
					if (! flagStartApp) {
						flagStartApp = sl_true;
						UIApp::dispatchStartToApp();
					}
					MobileApp::dispatchCreateActivityToApp();
				}
				Locale::dispatchChangeCurrentLocale();
			}

			void OnDestroyActivity(JNIEnv* env, jobject _this, jobject activity)
			{
				Log("Activity", "onDestroyActivity");
				MobileApp::dispatchDestroyActivityToApp();
			}

			void OnResumeActivity(JNIEnv* env, jobject _this, jobject activity)
			{
				Log("Activity", "onResumeActivity");
				Android::initializeContext(activity);
				MobileApp::dispatchResumeToApp();
			}

			void OnPauseActivity(JNIEnv* env, jobject _this, jobject activity)
			{
				Log("Activity", "onPauseActivity");
				MobileApp::dispatchPauseToApp();
			}

			jboolean OnBack(JNIEnv* env, jobject _this, jobject activity)
			{
				Log("Activity", "onBackPressed");
				return (jboolean)(MobileApp::dispatchBackPressedToApp());
			}

			void OnConfigurationChanged(JNIEnv* env, jobject _this, jobject activity)
			{
				Log("Activity", "onConfigurationChanged");
				Locale::dispatchChangeCurrentLocale();
			}

			void OnChangeWindowInsets(JNIEnv* env, jobject _this, jobject activity)
			{
				Log("Activity", "onChangeWindowInsets");
				UIResource::updateDefaultScreenSize();
			}

			void OnOpenUrl(JNIEnv* env, jobject _this, jobject activity, jstring jurl)
			{
				String url = Jni::getString(jurl);
				Log("Activity", "onOpenUrl: %s", url);
				MobileApp::dispatchOpenUrlToApp(url);
			}

			void DispatchCallback(JNIEnv* env, jobject _this)
			{
				UIDispatcher::processCallbacks();
			}

			void DispatchDelayedCallback(JNIEnv* env, jobject _this, jlong ptr)
			{
				UIDispatcher::processDelayedCallback((sl_reg)ptr);
			}

		}

		namespace mobile_app
		{

			void UpdateKeyboardAdjustMode(UIKeyboardAdjustMode mode)
			{
				jobject context = Android::getCurrentContext();
				if (android::Activity::isActivity(context)) {
					JniLocal<jobject> window = android::Activity::getWindow(context);
					if (window.isNotNull()) {
						sl_uint32 jmode = 0;
						switch (mode) {
							case UIKeyboardAdjustMode::Pan:
								jmode = 0x20; // WindowManager.LayoutParams.SOFT_INPUT_ADJUST_PAN
								break;
							case UIKeyboardAdjustMode::Resize:
								jmode = 0x10; // WindowManager.LayoutParams.SOFT_INPUT_ADJUST_RESIZE
								break;
							default:
								break;
						}
						android::Window::setSoftInputMode(window, jmode);
					}
				}
			}

		}

	}

	using namespace priv::ui_core;

	Ref<Screen> UI::getPrimaryScreen()
	{
		SLIB_GLOBAL_ZERO_INITIALIZED(AtomicRef<Screen>, ret)
		if (SLIB_SAFE_STATIC_CHECK_FREED(ret)) {
			return sl_null;
		}
		if (ret.isNull()) {
			ret = ScreenImpl::create();
		}
		return ret;
	}

	List< Ref<Screen> > UI::getScreens()
	{
		List< Ref<Screen> > ret;
		Ref<Screen> screen = UI::getPrimaryScreen();
		if (screen.isNotNull()) {
			ret.add_NoLock(screen);
		}
		return ret;
	}

	sl_bool UI::isUiThread()
	{
		return JUiThread::isUiThread.callBoolean(sl_null) != 0;
	}

	void UI::dispatchToUiThread(const Function<void()>& callback, sl_uint32 delayMillis)
	{
		if (delayMillis == 0) {
			if (UIDispatcher::addCallback(callback)) {
				JUiThread::dispatch.call(sl_null);
			}
		} else {
			if (delayMillis >> 31) {
				delayMillis = 0x7fffffff;
			}
			sl_reg ptr;
			if (UIDispatcher::addDelayedCallback(callback, ptr)) {
				JUiThread::dispatchDelayed.call(sl_null, (jlong)ptr, delayMillis);
			}
		}
	}

	void UI::openUrl(const StringParam& _url) {
		jobject context = Android::getCurrentContext();
		if (context) {
			JniLocal<jstring> url = Jni::getJniString(_url);
			JUtil::openUrl.call(sl_null, context, url.get());
		}
	}

	void UI::showKeyboard()
	{
		jobject context = Android::getCurrentContext();
		if (android::Activity::isActivity(context)) {
			JUtil::showKeyboard.call(sl_null, context);
		}
	}

	void UI::dismissKeyboard()
	{
		jobject context = Android::getCurrentContext();
		if (android::Activity::isActivity(context)) {
			JUtil::dismissKeyboard.call(sl_null, context);
		}
	}

	void UIPlatform::runLoop(sl_uint32 level)
	{
		JUiThread::runLoop.call(sl_null);
	}

	void UIPlatform::quitLoop()
	{
		JUiThread::quitLoop.call(sl_null);
	}

	void UIPlatform::initApp()
	{
		g_app = UIApp::getApp();
	}

	void UIPlatform::runApp()
	{
	}

	void UIPlatform::quitApp()
	{
	}

	void UIPlatform::sendFile(const StringParam& filePath, const StringParam& mimeType, const StringParam& chooserTitle)
	{
		jobject context = Android::getCurrentContext();
		if (context) {
			JniLocal<jstring> jfilePath = Jni::getJniString(filePath);
			JniLocal<jstring> jmimeType = Jni::getJniString(mimeType);
			JniLocal<jstring> jchooserTitle = Jni::getJniString(chooserTitle);
			return JUtil::sendFile.call(sl_null, context, jfilePath.get(), jmimeType.get(), jchooserTitle.get());
		}
	}

	void UIApp::setBadgeNumber(sl_uint32 number)
	{
		jobject context = Android::getCurrentContext();
		if (context) {
			JUtil::setBadgeNumber.call(sl_null, context, number);
		}
	}

	ScreenOrientation MobileApp::getScreenOrientation()
	{
		jobject context = Android::getCurrentContext();
		if (android::Activity::isActivity(context)) {
			return (ScreenOrientation)(JUtil::getScreenOrientation.callInt(sl_null, context));
		}
		return ScreenOrientation::Portrait;
	}

	void MobileApp::attemptRotateScreenOrientation()
	{
		jobject context = Android::getCurrentContext();
		if (android::Activity::isActivity(context)) {
			List<ScreenOrientation> orientations(MobileApp::getAvailableScreenOrientations());
			if (orientations.isEmpty()) {
				JUtil::setScreenOrientations.call(sl_null, context, sl_true, sl_true, sl_true, sl_true);
			} else {
				JUtil::setScreenOrientations.call(sl_null, context,
					orientations.contains(ScreenOrientation::Portrait),
					orientations.contains(ScreenOrientation::LandscapeRight),
					orientations.contains(ScreenOrientation::PortraitUpsideDown),
					orientations.contains(ScreenOrientation::LandscapeLeft)
					);
			}
		}
	}

	sl_ui_len MobileApp::getStatusBarHeight()
	{
		jobject context = Android::getCurrentContext();
		if (context) {
			return JUtil::getStatusBarHeight.callInt(sl_null, context);
		}
		return 0;
	}

	void MobileApp::setStatusBarStyle(StatusBarStyle style)
	{
		jobject context = Android::getCurrentContext();
		if (android::Activity::isActivity(context)) {
			JUtil::setStatusBarStyle.call(sl_null, context, style == StatusBarStyle::Hidden, style == StatusBarStyle::Dark);
			UIResource::updateDefaultScreenSize();
		}
	}

	UIEdgeInsets MobileApp::getSafeAreaInsets()
	{
		jobject context = Android::getCurrentContext();
		if (android::Activity::isActivity(context)) {
			JniLocal<jobject> jrect = JUtil::getSafeAreaInsets.callObject(sl_null, context);
			if (jrect.isNotNull()) {
				UIEdgeInsets ret;
				ret.left = (sl_ui_len)(JRect::left.get(jrect));
				ret.top = (sl_ui_len)(JRect::top.get(jrect));
				ret.right = (sl_ui_len)(JRect::right.get(jrect));
				ret.bottom = (sl_ui_len)(JRect::bottom.get(jrect));
				return ret;
			}
		}
		UIEdgeInsets ret = {0, 0, 0, 0};
		return ret;
	}

}

#endif
