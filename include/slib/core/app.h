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

#ifndef CHECKHEADER_SLIB_CORE_APP
#define CHECKHEADER_SLIB_CORE_APP

#include "object.h"
#include "string.h"
#include "function.h"
#include "named_instance.h"
#include "flags.h"

namespace slib
{

	enum class AppType
	{
		UI = 0,
		Service = 1
	};
	
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

	class SLIB_EXPORT Application : public Object
	{
		SLIB_DECLARE_OBJECT

	public:
		Application();

		~Application();

	public:
		virtual AppType getAppType() = 0;

		String getExecutablePath();
	
		String getCommandLine();

		List<String> getArguments();

		sl_bool isInitialized();

		void setInitialized(sl_bool flag);

		void initialize(const StringParam& commandLine);

		void initialize(int argc, const char* argv[]);

		void initialize(int argc, char** argv);

		void initialize();

		// returns exit code
		template <class... ARGS>
		sl_int32 run(ARGS&&... args)
		{
			if (!m_flagInitialized) {
				initialize(Forward<ARGS>(args)...);
			}
			return doRun();
		}

		void dispatchQuitApp();

		sl_bool isUniqueInstanceRunning();

		virtual String getApplicationId();

		void setApplicationId(const StringParam& _id);

		virtual sl_bool isCrashRecoverySupport();

		void setCrashRecoverySupport(sl_bool flagSupport);
	
	protected:
		// returns exit code
		virtual sl_int32 doRun();
	
		// returns exit code
		virtual sl_int32 onRunApp() = 0;

		virtual void onQuitApp();
		
		// returns exit code
		virtual sl_int32 onExistingInstance();
	
	public:
		static Ref<Application> getApp();

		static void setApp(Application* app);


		static void setEnvironmentPath(const StringParam& key, const StringParam& path);

		static String getEnvironmentPath(const StringParam& key);

		static String parseEnvironmentPath(const StringParam& path);
	

		static String getApplicationPath();

		static String getApplicationDirectory();

		static void setApplicationDirectory(const StringParam& path);

		static String findFileAndSetAppPath(const StringParam& filePath, sl_uint32 nDeep = SLIB_UINT32_MAX);

	public:
		static sl_bool checkPermissions(const AppPermissions& permissions);

		static void grantPermissions(const AppPermissions& permissions, const Function<void()>& callback = sl_null);

		static sl_bool isRoleHeld(AppRole role);

		static void requestRole(AppRole role, const Function<void()>& callback = sl_null);

		static void openDefaultAppsSetting();

		static sl_bool isSupportedDefaultCallingApp();

		static sl_bool isDefaultCallingApp();

		static void setDefaultCallingApp(const Function<void()>& callback = sl_null);

		// Android only
		static sl_bool isSystemOverlayEnabled();

		// Android only
		static void openSystemOverlaySetting();
		
		
		// macOS only
		static sl_bool isAccessibilityEnabled();
		
		// macOS only
		static void authenticateAccessibility();
		
		// macOS only
		static void openSystemPreferencesForAccessibility();
		
	public:	
		static void registerRunAtStartup(const StringParam& appName, const StringParam& path);

		static void registerRunAtStartup(const StringParam& path);

		static void registerRunAtStartup();

		static void unregisterRunAtStartup(const StringParam& path);

		static void unregisterRunAtStartup();
	
	protected:
		void _initApp();

	protected:
		sl_bool m_flagInitialized;

		String m_executablePath;
		String m_commandLine;
		List<String> m_arguments;

		AtomicString m_applicationId;
		NamedInstance m_uniqueInstance;

		sl_bool m_flagCrashRecoverySupport;

	};
	
}

#define SLIB_APPLICATION(CLASS) \
public: \
	template <class... ARGS> \
	static int main(ARGS&&... args) { \
		slib::Ref<CLASS> app = new CLASS; \
		if (app.isNotNull()) { \
			return (int)(app->run(Forward<ARGS>(args)...)); \
		} \
		return -1; \
	} \
	static slib::Ref<CLASS> getApp() { \
		return slib::CastRef<CLASS>(slib::Application::getApp()); \
	}

#endif
