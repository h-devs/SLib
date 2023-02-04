/*
 *   Copyright (c) 2008-2019 SLIBIO <https://github.com/SLIBIO>
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

#if defined(SLIB_PLATFORM_IS_ANDROID)

#include "slib/core/app.h"

#include "slib/core/function.h"
#include "slib/core/safe_static.h"
#include "slib/platform.h"

namespace slib
{

	namespace {

		SLIB_JNI_BEGIN_CLASS(JApplication, "slib/android/app/Application")
			SLIB_JNI_STATIC_METHOD(checkPermissions, "checkPermissions", "(Landroid/app/Activity;I)Z");
			SLIB_JNI_STATIC_METHOD(grantPermissions, "grantPermissions", "(Landroid/app/Activity;I)V");

			SLIB_JNI_STATIC_METHOD(isRoleHeld, "isRoleHeld", "(Landroid/app/Activity;I)Z");
			SLIB_JNI_STATIC_METHOD(requestRole, "requestRole", "(Landroid/app/Activity;I)V");

			SLIB_JNI_STATIC_METHOD(openDefaultAppsSetting, "openDefaultAppsSetting", "(Landroid/app/Activity;)V");
			SLIB_JNI_STATIC_METHOD(isSupportedDefaultCallingApp, "isSupportedDefaultCallingApp", "()Z");
			SLIB_JNI_STATIC_METHOD(isDefaultCallingApp, "isDefaultCallingApp", "(Landroid/app/Activity;)Z");
			SLIB_JNI_STATIC_METHOD(setDefaultCallingApp, "setDefaultCallingApp", "(Landroid/app/Activity;)V");
			SLIB_JNI_STATIC_METHOD(isSystemOverlayEnabled, "isSystemOverlayEnabled", "(Landroid/app/Activity;)Z");
			SLIB_JNI_STATIC_METHOD(openSystemOverlaySetting, "openSystemOverlaySetting", "(Landroid/app/Activity;)V");
		SLIB_JNI_END_CLASS

		SLIB_GLOBAL_ZERO_INITIALIZED(AtomicFunction<void()>, g_callbackOnGrantPermission);

		SLIB_JNI_BEGIN_CLASS_SECTION(JApplication)
			SLIB_JNI_NATIVE_IMPL(nativeOnCallbackGrantPermissions, "nativeOnCallbackGrantPermissions", "()V", void)
			{
				g_callbackOnGrantPermission();
				g_callbackOnGrantPermission.setNull();
			}
		SLIB_JNI_END_CLASS_SECTION

		SLIB_GLOBAL_ZERO_INITIALIZED(AtomicFunction<void()>, g_callbackOnRequestRole);

		SLIB_JNI_BEGIN_CLASS_SECTION(JApplication)
			SLIB_JNI_NATIVE_IMPL(nativeOnCallbackRequestRole, "nativeOnCallbackRequestRole", "()V", void)
			{
				g_callbackOnRequestRole();
				g_callbackOnRequestRole.setNull();
			}
		SLIB_JNI_END_CLASS_SECTION

		SLIB_GLOBAL_ZERO_INITIALIZED(AtomicFunction<void()>, g_callbackOnSetDefaultCallingApp);

		SLIB_JNI_BEGIN_CLASS_SECTION(JApplication)
			SLIB_JNI_NATIVE_IMPL(nativeOnCallbackSetDefaultCallingApp, "nativeOnCallbackSetDefaultCallingApp", "()V", void)
			{
				g_callbackOnSetDefaultCallingApp();
				g_callbackOnSetDefaultCallingApp.setNull();
			}
		SLIB_JNI_END_CLASS_SECTION

	}

	sl_bool Application::checkPermissions(const AppPermissions& permissions)
	{
		jobject context = Android::getCurrentContext();
		if (context) {
			return JApplication::checkPermissions.callBoolean(sl_null, context, (int)permissions);
		}
		return sl_null;
	}

	void Application::grantPermissions(const AppPermissions& permissions, const Function<void()>& callback)
	{
		jobject context = Android::getCurrentContext();
		if (context) {
			g_callbackOnGrantPermission();
			g_callbackOnGrantPermission = callback;
			JApplication::grantPermissions.call(sl_null, context, (int)permissions);
		}
	}

	sl_bool Application::isRoleHeld(AppRole role)
	{
		jobject context = Android::getCurrentContext();
		if (context) {
			return JApplication::isRoleHeld.callBoolean(sl_null, context, (int)role);
		}
		return sl_null;
	}

	void Application::requestRole(AppRole role, const Function<void()>& callback)
	{
		jobject context = Android::getCurrentContext();
		if (context) {
			g_callbackOnRequestRole();
			g_callbackOnRequestRole = callback;
			JApplication::requestRole.call(sl_null, context, (int)role);
		}
	}

	void Application::openDefaultAppsSetting()
	{
		jobject context = Android::getCurrentContext();
		if (context) {
			JApplication::openDefaultAppsSetting.call(sl_null, context);
		}
	}

	sl_bool Application::isSupportedDefaultCallingApp()
	{
		return JApplication::isSupportedDefaultCallingApp.callBoolean(sl_null);
	}

	sl_bool Application::isDefaultCallingApp()
	{
		if (Android::getSdkVersion() >= AndroidSdkVersion::Q) {
			return isRoleHeld(AppRole::Dialer);
		}
		jobject context = Android::getCurrentContext();
		if (context) {
			return JApplication::isDefaultCallingApp.callBoolean(sl_null, context);
		}
		return sl_false;
	}

	void Application::setDefaultCallingApp(const Function<void()>& callback)
	{
		if (Android::getSdkVersion() >= AndroidSdkVersion::Q) {
			requestRole(AppRole::Dialer, callback);
			return;
		}
		jobject context = Android::getCurrentContext();
		if (context) {
			g_callbackOnSetDefaultCallingApp();
			g_callbackOnSetDefaultCallingApp = callback;
			JApplication::setDefaultCallingApp.call(sl_null, context);
		}
	}

	sl_bool Application::isSystemOverlayEnabled()
	{
		jobject context = Android::getCurrentContext();
		if (context) {
			return JApplication::isSystemOverlayEnabled.callBoolean(sl_null, context);
		}
		return sl_false;
	}

	void Application::openSystemOverlaySetting()
	{
		jobject context = Android::getCurrentContext();
		if (context) {
			JApplication::openSystemOverlaySetting.call(sl_null, context);
		}
	}

}

#endif
