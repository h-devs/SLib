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

#include "slib/core/app.h"
#include "slib/core/command_line.h"

#include "slib/core/hash_map.h"
#include "slib/core/system.h"
#include "slib/core/process.h"
#include "slib/core/file.h"
#include "slib/core/string_buffer.h"
#include "slib/core/safe_static.h"
#include "slib/core/log.h"

#include "slib/core/win32/windows.h"

namespace slib
{

	namespace priv
	{
		namespace app
		{
			
			SLIB_GLOBAL_ZERO_INITIALIZED(AtomicWeakRef<Application>, g_weakref_app)

			SLIB_SAFE_STATIC_GETTER(String, GetAppPath, System::getApplicationPath())
		
			SLIB_SAFE_STATIC_GETTER(String, GetAppDir, System::getApplicationDirectory())
			
#if !defined(SLIB_PLATFORM_IS_MOBILE)
			static void CrashHandler(int)
			{
				Ref<Application> app = Application::getApp();
				if (app.isNotNull()) {
					List<StringParam> args;
					args.addAll_NoLock(app->getArguments());
					if (args.isNotEmpty()) {
						Process::exec(app->getExecutablePath(), args.getData() + 1, args.getCount() - 1);
					} else {
						Process::exec(app->getExecutablePath());
					}
				}
			}
#endif

/**************************************************************************************
For parsing command line in Win32 platform,
	refered to https://msdn.microsoft.com/en-us/library/windows/desktop/17w5ykft(v=vs.85).aspx

Microsoft Specific
	Microsoft C/C++ startup code uses the following rules when interpreting arguments given on the operating system command line:
	* Arguments are delimited by white space, which is either a space or a tab.
	* The caret character (^) is not recognized as an escape character or delimiter. The character is handled completely by the command-line parser in the operating system before being passed to the argv array in the program.
	* A string surrounded by double quotation marks ("string") is interpreted as a single argument, regardless of white space contained within. A quoted string can be embedded in an argument.
	* A double quotation mark preceded by a backslash (\") is interpreted as a literal double quotation mark character (").
	* Backslashes are interpreted literally, unless they immediately precede a double quotation mark.
	* If an even number of backslashes is followed by a double quotation mark, one backslash is placed in the argv array for every pair of backslashes, and the double quotation mark is interpreted as a string delimiter.
	* If an odd number of backslashes is followed by a double quotation mark, one backslash is placed in the argv array for every pair of backslashes, and the double quotation mark is "escaped" by the remaining backslash, causing a literal double quotation mark (") to be placed in argv.
***************************************************************************************/

			List<String> ParseCommandLine(const StringParam& _commandLine, sl_bool flagWin32)
			{
				StringCstr commandLine(_commandLine);
				List<String> ret;
				sl_char8* sz = commandLine.getData();
				sl_size len = commandLine.getLength();
				StringBuffer sb;
				sl_size start = 0;
				sl_size pos = 0;
				sl_bool flagQuote = sl_false;
				while (pos < len) {
					sl_char8 ch = sz[pos];
					if (flagWin32) {
						if (ch == '\"') {
							sl_size k = pos - 1;
							while (k > 0) {
								if (sz[k] != '\\') {
									break;
								}
								k--;
							}
							sl_size n = pos - 1 - k;
							sl_size m = n / 2;
							sb.addStatic(sz + start, k + 1 + m - start);
							if (n % 2) {
								start = pos;
								pos++;
								continue;
							} else {
								start = pos + 1;
							}
						}
					} else {
						if (ch == '\\') {
							if (pos > start) {
								sb.addStatic(sz + start, pos - start);
							}
							start = pos + 1;
							pos++;
							if (pos < len) {
								pos++;
								continue;
							} else {
								break;
							}
						}
					}
					if (flagQuote) {
						if (ch == '\"') {
							flagQuote = sl_false;
							if (pos > start) {
								sb.addStatic(sz + start, pos - start);
							}
							start = pos + 1;
						}
					} else {
						if (SLIB_CHAR_IS_WHITE_SPACE(ch)) {
							if (pos > start) {
								sb.addStatic(sz + start, pos - start);
							}
							start = pos + 1;
							String s = sb.merge();
							if (s.isNotEmpty()) {
								ret.add(s);
							}
							sb.clear();
						} else if (ch == '\"') {
							flagQuote = sl_true;
							if (pos > start) {
								sb.addStatic(sz + start, pos - start);
							}
							start = pos + 1;
						}
					}
					pos++;
				}
				if (!flagQuote) {
					if (pos > start) {
						sb.addStatic(sz + start, pos - start);
					}
					String s = sb.merge();
					if (s.isNotEmpty()) {
						ret.add(s);
					}
				}
				return ret;
			}

		}
	}

	using namespace priv::app;

	SLIB_DEFINE_OBJECT(Application, Object)

	Application::Application()
	{
		m_flagInitialized = sl_false;
		m_flagCrashRecoverySupport = sl_false;
	}

	Application::~Application()
	{
	}

	Ref<Application> Application::getApp()
	{
		if (SLIB_SAFE_STATIC_CHECK_FREED(g_weakref_app)) {
			return sl_null;
		}
		return g_weakref_app;
	}

	void Application::setApp(Application* app)
	{
		if (SLIB_SAFE_STATIC_CHECK_FREED(g_weakref_app)) {
			return;
		}
		g_weakref_app = app;
	}

	String Application::getApplicationId()
	{
		return m_applicationId;
	}

	void Application::setApplicationId(const StringParam& _id)
	{
		m_applicationId = _id.toString();
	}

	String Application::getExecutablePath()
	{
		return m_executablePath;
	}

	String Application::getCommandLine()
	{
		return m_commandLine;
	}

	List<String> Application::getArguments()
	{
		return m_arguments;
	}

	sl_bool Application::isInitialized()
	{
		return m_flagInitialized;
	}

	void Application::setInitialized(sl_bool flag)
	{
		m_flagInitialized = flag;
	}

	void Application::initialize(const StringParam& commandLine)
	{
		m_commandLine = commandLine.toString();
		m_arguments = CommandLine::parse(commandLine);
		_initApp();
	}

	void Application::initialize(int argc, const char* argv[])
	{
		List<String> list;
		for (int i = 0; i < argc; i++) {
#ifdef SLIB_PLATFORM_IS_WIN32
			list.add(String::decode(Charset::ANSI, argv[i], Base::getStringLength(argv[i])));
#else
			list.add(argv[i]);
#endif
		}
		List<StringParam> params;
		params.addAll_NoLock(list);
		m_commandLine = CommandLine::build(params.getData(), argc);
		m_arguments = Move(list);
		_initApp();
	}

	void Application::initialize(int argc, char** argv)
	{
		initialize(argc, (const char**)argv);
	}

	void Application::initialize()
	{
#ifdef SLIB_PLATFORM_IS_WIN32
		String commandLine = String::create(GetCommandLineW());
		m_commandLine = commandLine;
		m_arguments = CommandLine::parse(commandLine);
#endif
		_initApp();
	}

	void Application::_initApp()
	{
		Application::setApp(this);
		m_executablePath = Application::getApplicationPath();
		onInitApp();
		m_flagInitialized = sl_true;
	}

	void Application::onInitApp()
	{
	}

	sl_int32 Application::doRun()
	{
#if !defined(SLIB_PLATFORM_IS_MOBILE)
		String instanceId = getApplicationId();
		if (instanceId.isNotEmpty()) {
			m_uniqueInstance = NamedInstance(instanceId);
			if (m_uniqueInstance.isNone()) {
				return onExistingInstance();
			}
		}
		
		if (isCrashRecoverySupport()) {
			System::setCrashHandler(CrashHandler);
		}
#endif
		
		sl_int32 iRet = onRunApp();
		
		dispatchQuitApp();

		return iRet;
		
	}

	void Application::dispatchQuitApp()
	{
		onQuitApp();
#if !defined(SLIB_PLATFORM_IS_MOBILE)
		m_uniqueInstance.setNone();
#endif
	}

	void Application::onQuitApp()
	{
	}

	sl_int32 Application::onExistingInstance()
	{
		LogError("APP", "%s is ALREADY RUNNING", getApplicationId());
		return -1;
	}

	sl_bool Application::isUniqueInstanceRunning()
	{
#if !defined(SLIB_PLATFORM_IS_MOBILE)
		String instanceId = getApplicationId();
		if (instanceId.isNotEmpty()) {
			return NamedInstance::exists(instanceId);
		}
#endif
		return sl_false;
	}

	sl_bool Application::isCrashRecoverySupport()
	{
		return m_flagCrashRecoverySupport;
	}

	void Application::setCrashRecoverySupport(sl_bool flagSupport)
	{
		m_flagCrashRecoverySupport = flagSupport;
	}
	
	String Application::getApplicationPath()
	{
		String* s = GetAppPath();
		if (!s) {
			return sl_null;
		}
		return *s;
	}

	String Application::getApplicationDirectory()
	{
		String* s = GetAppDir();
		if (!s) {
			return sl_null;
		}
		return *s;
	}

	void Application::setApplicationDirectory(const StringParam& path)
	{
		String* s = GetAppDir();
		if (s) {
			*s = path.toString();
		}
	}

	String Application::findFileAndSetApplicationDirectory(const StringParam& filePath, sl_uint32 nDeep)
	{
		String appPath = getApplicationDirectory();
		String path = File::findParentPathContainingFile(appPath, filePath);
		if (path.isNotNull()) {
			appPath = path;
			setApplicationDirectory(path);
		}
		return appPath;
	}

#if !defined(SLIB_UI_IS_ANDROID)
	sl_bool Application::checkPermissions(const AppPermissions& permissions)
	{
		return sl_true;
	}
	
	void Application::grantPermissions(const AppPermissions& permissions, const Function<void()>& callback)
	{
		callback();
	}

	sl_bool Application::isRoleHeld(AppRole role)
	{
		return sl_true;
	}

	void Application::requestRole(AppRole role, const Function<void()>& callback)
	{
		callback();
	}

	void Application::openDefaultAppsSetting()
	{
	}

	sl_bool Application::isSupportedDefaultCallingApp()
	{
		return sl_false;
	}

	sl_bool Application::isDefaultCallingApp()
	{
		return sl_false;
	}

	void Application::setDefaultCallingApp(const Function<void()>& callback)
	{
	}

	sl_bool Application::isSystemOverlayEnabled()
	{
		return sl_false;
	}

	void Application::openSystemOverlaySetting()
	{
	}
#endif

#if !defined(SLIB_PLATFORM_IS_MACOS)
	sl_bool Application::isAccessibilityEnabled()
	{
		return sl_true;
	}

	void Application::authenticateAccessibility()
	{
	}

	void Application::openSystemPreferencesForAccessibility()
	{
	}
#endif

#if !defined(SLIB_PLATFORM_IS_WIN32) && !defined(SLIB_PLATFORM_IS_LINUX_DESKTOP)
	void Application::registerRunAtStartup(const StringParam& appName, const StringParam& path)
	{
		registerRunAtStartup(path);
	}
#endif
	
#if !defined(SLIB_PLATFORM_IS_WIN32) && !defined(SLIB_PLATFORM_IS_MACOS) && !defined(SLIB_PLATFORM_IS_LINUX_DESKTOP)
	void Application::registerRunAtStartup(const StringParam& path)
	{
	}
	
	void Application::registerRunAtStartup()
	{
	}
	
	void Application::unregisterRunAtStartup(const StringParam& path)
	{
	}
	
	void Application::unregisterRunAtStartup()
	{
	}
#endif

	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(StartMenuParam)

	StartMenuParam::StartMenuParam()
	{
	}

#if !defined(SLIB_PLATFORM_IS_WIN32) && !defined(SLIB_PLATFORM_IS_LINUX_DESKTOP)
	void Application::registerAtStartMenu(const StartMenuParam& param)
	{
	}
#endif


	List<String> CommandLine::parse(const StringParam& commandLine)
	{
#if defined(SLIB_PLATFORM_IS_WINDOWS)
		return ParseCommandLine(commandLine, sl_true);
#else
		return ParseCommandLine(commandLine, sl_false);
#endif
	}

	List<String> CommandLine::parseForWin32(const StringParam& commandLine)
	{
		return ParseCommandLine(commandLine, sl_true);
	}

	List<String> CommandLine::parseForUnix(const StringParam& commandLine)
	{
		return ParseCommandLine(commandLine, sl_false);
	}

	String CommandLine::makeSafeArgument(const StringParam& s)
	{
#if defined(SLIB_PLATFORM_IS_WINDOWS)
		return makeSafeArgumentForWin32(s);
#else
		return makeSafeArgumentForUnix(s);
#endif
	}

	String CommandLine::makeSafeArgumentForWin32(const StringParam& _s)
	{
		StringData s(_s);
		if (s.contains(" ") || s.contains("\t") || s.contains("\r") || s.contains("\n") || s.contains("\"")) {
			StringBuffer buf;
			buf.addStatic("\"");
			ListElements<StringView> items(s.split("\""));
			for (sl_size k = 0; k < items.count; k++) {
				String t = items[k];
				buf.add(t);
				sl_char8* sz = t.getData();
				sl_size len = t.getLength();
				sl_size p = 0;
				for (; p < len; p++) {
					if (sz[len - 1 - p] != '\\') {
						break;
					}
				}
				buf.add(String('\\', p));
				if (k < items.count - 1) {
					buf.addStatic("\\\"");
				}
			}
			buf.addStatic("\"");
			return buf.merge();
		} else {
			if (s.isNotEmpty()) {
				return s.toString(_s);
			} else {
				SLIB_RETURN_STRING("\"\"")
			}
		}
	}

	String CommandLine::makeSafeArgumentForUnix(const StringParam& _s)
	{
		StringData s(_s);
		if (s.contains(" ") || s.contains("\t") || s.contains("\r") || s.contains("\n") || s.contains("\"") || s.contains("\\")) {
			String t = s.replaceAll("\\", "\\\\").replaceAll("\"", "\\\"");
			return String::concat("\"", Move(t), "\"");
		} else {
			if (s.isNotEmpty()) {
				return s.toString(_s);
			} else {
				SLIB_RETURN_STRING("\"\"")
			}
		}
	}

	String CommandLine::build(const StringParam* argv, sl_size argc)
	{
		StringBuffer buf;
		for (sl_size i = 0; i < argc; i++) {
			if (i > 0) {
				buf.addStatic(" ");
			}
			buf.add(makeSafeArgument(argv[i]));
		}
		return buf.merge();
	}

	String CommandLine::buildForWin32(const StringParam* argv, sl_size argc)
	{
		StringBuffer buf;
		for (sl_size i = 0; i < argc; i++) {
			if (i > 0) {
				buf.addStatic(" ");
			}
			buf.add(makeSafeArgumentForWin32(argv[i]));
		}
		return buf.merge();
	}

	String CommandLine::buildForUnix(const StringParam* argv, sl_size argc)
	{
		StringBuffer buf;
		for (sl_size i = 0; i < argc; i++) {
			if (i > 0) {
				buf.addStatic(" ");
			}
			buf.add(makeSafeArgumentForUnix(argv[i]));
		}
		return buf.merge();
	}

	String CommandLine::build(const StringParam& pathExecutable, const StringParam* argv, sl_size argc)
	{
		StringBuffer buf;
		buf.add(makeSafeArgument(pathExecutable));
		for (sl_size i = 0; i < argc; i++) {
			buf.addStatic(" ");
			buf.add(makeSafeArgument(argv[i]));
		}
		return buf.merge();
	}

	String CommandLine::buildForWin32(const StringParam& pathExecutable, const StringParam* argv, sl_size argc)
	{
		StringBuffer buf;
		buf.add(makeSafeArgumentForWin32(pathExecutable));
		for (sl_size i = 0; i < argc; i++) {
			buf.addStatic(" ");
			buf.add(makeSafeArgumentForWin32(argv[i]));
		}
		return buf.merge();
	}

	String CommandLine::buildForUnix(const StringParam& pathExecutable, const StringParam* argv, sl_size argc)
	{
		StringBuffer buf;
		buf.add(makeSafeArgumentForUnix(pathExecutable));
		for (sl_size i = 0; i < argc; i++) {
			buf.addStatic(" ");
			buf.add(makeSafeArgumentForUnix(argv[i]));
		}
		return buf.merge();
	}

}
