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

#include "slib/core/app.h"

#include "slib/core/hash_map.h"
#include "slib/core/system.h"
#include "slib/core/process.h"
#include "slib/core/file.h"
#include "slib/core/string_buffer.h"
#include "slib/core/global_unique_instance.h"
#include "slib/core/safe_static.h"
#include "slib/core/log.h"

#include "slib/core/windows.h"

namespace slib
{

	namespace priv
	{
		namespace app
		{
			
			SLIB_GLOBAL_ZERO_INITIALIZED(AtomicWeakRef<Application>, g_weakref_app)

			typedef HashMap<String, String> EnvironmentList;
			
			SLIB_SAFE_STATIC_GETTER(EnvironmentList, getEnvironmentList, EnvironmentList::create())

			SLIB_SAFE_STATIC_GETTER(String, getAppPath, System::getApplicationPath())
		
			SLIB_SAFE_STATIC_GETTER(String, getAppDir, System::getApplicationDirectory())
			
#if !defined(SLIB_PLATFORM_IS_MOBILE)
			static void CrashHandler(int)
			{
				Ref<Application> app = Application::getApp();
				if (app.isNotNull()) {
					List<String> args = app->getArguments();
					String* s = args.getData();
					sl_uint32 n = (sl_uint32)(args.getCount());
					if (n) {
						Process::exec(app->getExecutablePath(), s + 1, n - 1);
					} else {
						Process::exec(app->getExecutablePath(), s, n);
					}
				}
			}
#endif
			
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

	void Application::initialize(const String& commandLine)
	{
		m_commandLine = commandLine;
		m_arguments = breakCommandLine(commandLine);
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
		m_commandLine = buildCommandLine(list.getData(), argc);
		m_arguments = Move(list);
		_initApp();
	}

	void Application::initialize()
	{
#ifdef SLIB_PLATFORM_IS_WIN32
		String commandLine = String::create(GetCommandLineW());
		m_commandLine = commandLine;
		m_arguments = breakCommandLine(commandLine);
#endif
		_initApp();
	}

	void Application::_initApp()
	{
		Application::setApp(this);
		m_executablePath = Application::getApplicationPath();
		m_flagInitialized = sl_true;
	}

	sl_int32 Application::doRun()
	{
#if !defined(SLIB_PLATFORM_IS_MOBILE)
		String instanceId = getUniqueInstanceId();
		if (instanceId.isNotEmpty()) {
			m_uniqueInstance = GlobalUniqueInstance::create(instanceId);
			if (m_uniqueInstance.isNull()) {
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
		m_uniqueInstance.setNull();
#endif
	}

	void Application::onQuitApp()
	{
	}

	sl_int32 Application::onExistingInstance()
	{
		LogError("APP", "%s is ALREADY RUNNING", getUniqueInstanceId());
		return -1;
	}

	sl_bool Application::isUniqueInstanceRunning()
	{
#if !defined(SLIB_PLATFORM_IS_MOBILE)
		String instanceId = getUniqueInstanceId();
		if (instanceId.isNotEmpty()) {
			return GlobalUniqueInstance::exists(instanceId);
		}
#endif
		return sl_false;
	}

	String Application::getUniqueInstanceId()
	{
		return m_uniqueInstanceId;
	}

	void Application::setUniqueInstanceId(const String& _id)
	{
		m_uniqueInstanceId = _id;
	}

	sl_bool Application::isCrashRecoverySupport()
	{
		return m_flagCrashRecoverySupport;
	}

	void Application::setCrashRecoverySupport(sl_bool flagSupport)
	{
		m_flagCrashRecoverySupport = flagSupport;
	}
	
	void Application::setEnvironmentPath(const String& key, const String& path)
	{
		EnvironmentList* envMap = getEnvironmentList();
		if (envMap) {
			envMap->put(key, path);
		}
	}

	String Application::getEnvironmentPath(const String& key)
	{
		EnvironmentList* envMap = getEnvironmentList();
		if (envMap) {
			return envMap->getValue(key, String::null());
		}
		return sl_null;
	}

	String Application::parseEnvironmentPath(const String& _path)
	{
		String path = _path;
		if (path.startsWith('<')) {
			sl_reg index = path.indexOf('>', 1);
			if (index > 2) {
				String key = path.substring(1, index);
				String v;
				if (key == "APP") {
					v = getApplicationDirectory();
				} else {
					v = getEnvironmentPath(key);
				}
				path = v + path.substring(index + 1);
			}
		}
		return path;
	}

	String Application::getApplicationPath()
	{
		String* s = getAppPath();
		if (!s) {
			return sl_null;
		}
		return *s;
	}

	String Application::getApplicationDirectory()
	{
		String* s = getAppDir();
		if (!s) {
			return sl_null;
		}
		return *s;
	}

	void Application::setApplicationDirectory(const String& path)
	{
		String* s = getAppDir();
		if (s) {
			*s = path;
		}
	}

	String Application::findFileAndSetAppPath(const String& filePath, sl_uint32 nDeep)
	{
		String appPath = getApplicationDirectory();
		String path = File::findParentPathContainingFile(appPath, filePath);
		if (path.isNotNull()) {
			appPath = path;
			setApplicationDirectory(path);
		}
		return appPath;
	}


/**************************************************************************************
For breaking command line in Win32 platform,
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

	namespace priv
	{
		namespace app
		{

			List<String> BreakCommandLine(const String& commandLine, sl_bool flagWin32)
			{
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

	List<String> Application::breakCommandLine(const String& commandLine)
	{
#if defined(SLIB_PLATFORM_IS_WINDOWS)
		return priv::app::BreakCommandLine(commandLine, sl_true);
#else
		return priv::app::BreakCommandLine(commandLine, sl_false);
#endif
	}

	List<String> Application::breakCommandLine_Win32(const String& commandLine)
	{
		return priv::app::BreakCommandLine(commandLine, sl_true);
	}

	List<String> Application::breakCommandLine_Unix(const String& commandLine)
	{
		return priv::app::BreakCommandLine(commandLine, sl_false);
	}

	String Application::makeSafeArgument(const String& s)
	{
#if defined(SLIB_PLATFORM_IS_WINDOWS)
		return makeSafeArgument_Win32(s);
#else
		return makeSafeArgument_Unix(s);
#endif
	}

	String Application::makeSafeArgument_Win32(const String& s)
	{
		if (s.contains(" ") || s.contains("\t") || s.contains("\r") || s.contains("\n") || s.contains("\"")) {
			StringBuffer buf;
			buf.addStatic("\"");
			ListElements<String> items(s.split("\""));
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
		}
		if (s.isNotEmpty()) {
			return s;
		} else {
			SLIB_RETURN_STRING("\"\"");
		}
	}

	String Application::makeSafeArgument_Unix(const String& s)
	{
		if (s.contains(" ") || s.contains("\t") || s.contains("\r") || s.contains("\n") || s.contains("\"") || s.contains("\\")) {
			String t = s
				.replaceAll("\\", "\\\\")
				.replaceAll("\"", "\\\"");
			return String::join("\"", Move(t), "\"");
		}
		if (s.isNotEmpty()) {
			return s;
		} else {
			SLIB_RETURN_STRING("\"\"");
		}
	}

	String Application::buildCommandLine(const String* argv, sl_size argc)
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

	String Application::buildCommandLine_Win32(const String* argv, sl_size argc)
	{
		StringBuffer buf;
		for (sl_size i = 0; i < argc; i++) {
			if (i > 0) {
				buf.addStatic(" ");
			}
			buf.add(makeSafeArgument_Win32(argv[i]));
		}
		return buf.merge();
	}

	String Application::buildCommandLine_Unix(const String* argv, sl_size argc)
	{
		StringBuffer buf;
		for (sl_size i = 0; i < argc; i++) {
			if (i > 0) {
				buf.addStatic(" ");
			}
			buf.add(makeSafeArgument_Unix(argv[i]));
		}
		return buf.merge();
	}

	String Application::buildCommandLine(const String& pathExecutable, const String* argv, sl_size argc)
	{
		StringBuffer buf;
		buf.add(makeSafeArgument(pathExecutable));
		for (sl_size i = 0; i < argc; i++) {
			buf.addStatic(" ");
			buf.add(makeSafeArgument(argv[i]));
		}
		return buf.merge();
	}

	String Application::buildCommandLine_Win32(const String& pathExecutable, const String* argv, sl_size argc)
	{
		StringBuffer buf;
		buf.add(makeSafeArgument_Win32(pathExecutable));
		for (sl_size i = 0; i < argc; i++) {
			buf.addStatic(" ");
			buf.add(makeSafeArgument_Win32(argv[i]));
		}
		return buf.merge();
	}

	String Application::buildCommandLine_Unix(const String& pathExecutable, const String* argv, sl_size argc)
	{
		StringBuffer buf;
		buf.add(makeSafeArgument_Unix(pathExecutable));
		for (sl_size i = 0; i < argc; i++) {
			buf.addStatic(" ");
			buf.add(makeSafeArgument_Unix(argv[i]));
		}
		return buf.merge();
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

}
