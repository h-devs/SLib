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

#ifndef CHECKHEADER_SLIB_CORE_APP
#define CHECKHEADER_SLIB_CORE_APP

#include "object.h"
#include "string.h"
#include "list.h"

#include "../system/named_instance.h"

namespace slib
{

	enum class AppType
	{
		UI = 0,
		Service = 1
	};

	class SLIB_EXPORT Application : public Object
	{
		SLIB_DECLARE_OBJECT

	public:
		Application();

		~Application();

	public:
		virtual AppType getAppType() = 0;

		virtual String getApplicationId();

		void setApplicationId(const StringParam& _id, sl_bool flagGlobal = sl_true);

		virtual sl_bool isGlobalUniqueInstance();

		String getExecutablePath();

		String getCommandLine();

		List<String> getArguments();

		sl_bool isInitialized();

		void setInitialized(sl_bool flag);

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

		virtual sl_bool isCrashRecoverySupport();

		void setCrashRecoverySupport(sl_bool flagSupport);

	protected:
		// returns exit code
		virtual sl_int32 doRun();

		virtual void onInitApp();

		// returns exit code
		virtual sl_int32 onRunApp() = 0;

		virtual void onQuitApp();

		// returns exit code
		virtual sl_int32 onExistingInstance();

	public:
		static Ref<Application> getApp();

		static void setApp(Application* app);

		static String getApplicationPath();

		static String getApplicationDirectory();

		static void setApplicationDirectory(const StringParam& path);

		static String findFileAndSetApplicationDirectory(const StringParam& filePath, sl_uint32 nDeep = SLIB_UINT32_MAX);

	protected:
		void _initApp();

	protected:
		sl_bool m_flagInitialized;

		String m_executablePath;
		String m_commandLine;
		List<String> m_arguments;

		AtomicString m_applicationId;
		sl_bool m_flagGlobalUnique;
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
