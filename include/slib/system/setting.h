/*
 *   Copyright (c) 2008-2024 SLIBIO <https://github.com/SLIBIO>
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

#ifndef CHECKHEADER_SLIB_SYSTEM_SETTING
#define CHECKHEADER_SLIB_SYSTEM_SETTING

#include "definition.h"

#include "../core/string.h"
#include "../core/function.h"
#include "../core/flags.h"

namespace slib
{

	SLIB_DEFINE_FLAGS(AppPermissions, {

		Camera = 1,

		RecordAudio = (1<<1),

		WriteExternalStorage = (1<<2),
		ReadExternalStorage = (1<<3),

		ReadPhoneState = (1<<4),
		ReadPhoneNumbers = (1<<5),
		CallPhone = (1<<6),
		AnswerPhoneCalls = (1<<7),
		AddVoiceMail = (1<<8),
		UseSip = (1<<9),

		SendSMS = (1<<10),
		ReceiveSMS = (1<<11),
		ReadSMS = (1<<12),
		ReceiveWapPush = (1<<13),
		ReceiveMMS = (1<<14),

		ReadContacts = (1<<15),
		WriteContacts = (1<<16),
		GetAccounts = (1<<17),

		AccessFineLocation = (1<<18),
		AccessCoarseLocation = (1<<19),

		ReadCalendar = (1<<20),
		WriteCalendar = (1<<21),

		ReadCallLog = (1<<22),
		WriteCallLog = (1<<23),
		ProcessOutgoingCalls = (1<<24),

		BodySensors = (1<<25)

	})

	enum class AppRole
	{
		Home = 0,
		Browser = 1,
		Dialer = 2,
		SMS = 3,
		Emergency = 4,
		CallRedirection = 5,
		CallScreening = 6,
		Assistant = 7
	};

	// same as UNAuthorizationOptions
	SLIB_DEFINE_FLAGS(NotificationOptions, {
		Badge = 1,
		Sound = (1<<1),
		Alert = (1<<2),
		CarPlay = (1<<3),
		CriticalAlert = (1<<4),
		ProvidesAppNotificationSettings = (1<<5),
		Provisional = (1<<6),
		Announcement = (1<<7)
	})

	class SLIB_EXPORT StartMenuParam
	{
	public:
		StringParam appId; // [Linux] Uses `Application::getApp()->getApplicationId()` by default
		StringParam appName; // Non-empty. Application's display name
		StringParam executablePath; // Uses `Application::getApplicationPath()` by default
		StringParam iconPath; // [Linux] Path to PNG file
		StringParam categories; // [Linux] eg, "Development;Utility;"

	public:
		StartMenuParam();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(StartMenuParam)

	};

	class SLIB_EXPORT Setting
	{
	public:
		static void registerRunAtStartup(const StringParam& appName, const StringParam& path);

		static void registerRunAtStartup(const StringParam& path);

		static void registerRunAtStartup();

		static void unregisterRunAtStartup(const StringParam& path);

		static void unregisterRunAtStartup();

		static void registerAtStartMenu(const StartMenuParam& param);


		static void openDefaultApps();

#ifdef SLIB_PLATFORM_IS_MACOS
		static void openAccessibility();

		static void openScreenRecording();

		static void openNotifications();
#endif

#ifdef SLIB_PLATFORM_IS_ANDROID
		static void openSystemOverlay();
#endif


#ifdef SLIB_PLATFORM_IS_APPLE
		static void checkNotification(const Function<void(sl_bool flagGranted)>& callback);

		static void requestNotification(const NotificationOptions& options, const Function<void(sl_bool flagGranted)>& callback);
#endif

#ifdef SLIB_PLATFORM_IS_MACOS
		static sl_bool isAccessibilityEnabled();

		static void requestAccessibility();

		static sl_bool isScreenRecordingEnabled();

		static void requestScreenRecording();

		static sl_bool isMicrophoneEnabled();

		static void requestMicrophone();

		static void requestMicrophone(const Function<void(sl_bool flagGranted)>& callback);

		static sl_bool isCameraEnabled();

		static void requestCamera();

		static void requestCamera(const Function<void(sl_bool flagGranted)>& callback);

		static sl_bool isInputMonitoringEnabled();

		static void requestInputMonitoring();
#endif


#ifdef SLIB_PLATFORM_IS_ANDROID
		static sl_bool checkPermissions(const AppPermissions& permissions);

		static void grantPermissions(const AppPermissions& permissions, const Function<void()>& callback = sl_null);

		static sl_bool isRoleHeld(AppRole role);

		static void requestRole(AppRole role, const Function<void()>& callback = sl_null);

		static sl_bool isSupportedDefaultCallingApp();

		static sl_bool isDefaultCallingApp();

		static void setDefaultCallingApp(const Function<void()>& callback = sl_null);

		static sl_bool isSystemOverlayEnabled();
#endif


#ifdef SLIB_PLATFORM_IS_MACOS
		static void resetAccessibility(const StringParam& appBundleId);

		static void resetAutomation(const StringParam& appBundleId);

		static void resetScreenRecording(const StringParam& appBundleId);

		static void resetMicrophone(const StringParam& appBundleId);

		static void resetCamera(const StringParam& appBundleId);

		static void resetInputMonitoring(const StringParam& appBundleId);
#endif

	};

}

#endif
