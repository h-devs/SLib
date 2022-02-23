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

#include "slib/core/log.h"

#include "slib/core/file.h"
#include "slib/core/console.h"
#include "slib/core/safe_static.h"

#if defined(SLIB_PLATFORM_IS_WIN32)
#include "slib/core/win32/windows.h"
#include <stdio.h>
#elif defined(SLIB_PLATFORM_IS_ANDROID)
#include "slib/core/android/log.h"
#elif defined(SLIB_PLATFORM_IS_TIZEN)
#include <dlog.h>
#else
#include <stdio.h>
#endif

namespace slib
{

	namespace priv
	{
		namespace log
		{

			SLIB_GLOBAL_ZERO_INITIALIZED(AtomicRef<Logger>, g_globalLogger)
			
			static String GetLineString(const StringParam& tag, const StringParam& content)
			{
				return String::format("%s [%s] %s\n", Time::now(), tag, content);
			}
			
			static String GetLineStringCRLF(const StringParam& tag, const StringParam& content)
			{
				return String::format("%s [%s] %s\r\n", Time::now(), tag, content);
			}
			
#ifdef SLIB_PLATFORM_IS_WIN32
			static String16 GetLineString16(const StringParam& tag, const StringParam& content)
			{
				return String16::format(SLIB_UNICODE("%s [%s] %s\n"), Time::now(), tag, content);
			}
#endif
			
		}
	}
	
	using namespace priv::log;

	
	SLIB_DEFINE_OBJECT(Logger, Object)

	Logger::Logger()
	{
#if defined(SLIB_PLATFORM_IS_ANDROID)
		m_priorityMinimum = LogPriority::Unknown;
#elif defined(SLIB_DEBUG)
		m_priorityMinimum = LogPriority::Debug;
#else
		m_priorityMinimum = LogPriority::Info;
#endif
	}

	Logger::~Logger()
	{
	}

	void Logger::log(const StringParam& tag, const StringParam& content)
	{
		log(LogPriority::Info, tag, content);
	}

	void Logger::logError(const StringParam& tag, const StringParam& content)
	{
		log(LogPriority::Error, tag, content);
	}

	void Logger::logDebug(const StringParam& tag, const StringParam& content)
	{
		log(LogPriority::Debug, tag, content);
	}

	LogPriority Logger::getMinimumPriority()
	{
		return m_priorityMinimum;
	}

	void Logger::setMinimumPriority(LogPriority priority)
	{
		m_priorityMinimum = priority;
	}

	Ref<Logger> Logger::global()
	{
		if (g_globalLogger.isNotNull()) {
			return g_globalLogger;
		}
		Ref<Logger> logger = getConsoleLogger();
		g_globalLogger = logger;
		return logger;
	}

	void Logger::setGlobal(const Ref<Logger>& logger)
	{
		g_globalLogger = logger;
	}

	Ref<Logger> Logger::getConsoleLogger()
	{
		SLIB_SAFE_LOCAL_STATIC(Ref<Logger>, logger, new ConsoleLogger)
		if (SLIB_SAFE_STATIC_CHECK_FREED(logger)) {
			return sl_null;
		}
		return logger;
	}

	Ref<Logger> Logger::createFileLogger(const String& fileNameFormat)
	{
		return new FileLogger(fileNameFormat);
	}


	FileLogger::FileLogger()
	{
	}

	FileLogger::FileLogger(const String& fileNameFormat)
	{
		m_fileNameFormat = fileNameFormat;
	}

	FileLogger::~FileLogger()
	{
	}

	void FileLogger::log(LogPriority priority, const StringParam& tag, const StringParam& content)
	{
		String fmt = getFileNameFormat();
		if (fmt.isEmpty()) {
			return;
		}
		ObjectLocker lock(this);
		File::appendAllTextUTF8(String::format(fmt, Time::now()), GetLineStringCRLF(tag, content));
	}
	
	String FileLogger::getFileNameFormat()
	{
		return String::format(m_fileNameFormat, Time::now());
	}


	ConsoleLogger::ConsoleLogger()
	{
	}

	ConsoleLogger::~ConsoleLogger()
	{
	}

	void ConsoleLogger::log(LogPriority priority, const StringParam& _tag, const StringParam& _content)
	{
#if defined(SLIB_PLATFORM_IS_ANDROID)
		ObjectLocker lock(this);
		android::Log(priority, _tag, _content);
#elif defined(SLIB_PLATFORM_IS_TIZEN)
		StringCstr tag(_tag);
		StringCstr content(_content);
		ObjectLocker lock(this);
		int _priority;
		if (priority >= LogPriority::Error) {
			_priority = DLOG_ERROR;
		} else {
			_priority = DLOG_INFO;
		}
		if (content.isNotEmpty()) {
			dlog_print(_priority, tag.getData(), "%s", content.getData());
		} else {
			dlog_print(_priority, tag.getData(), " ");
		}
#elif defined(SLIB_PLATFORM_IS_WIN32)
		StringCstr16 s(GetLineString16(_tag, _content));
		OutputDebugStringW((LPCWSTR)(s.getData()));
		if (priority >= LogPriority::Error) {
			HWND hWnd = GetConsoleWindow();
			if (hWnd) {
				Memory mem = Charsets::encode16(s.getData(), s.getLength() + 1, Charset::ANSI);
				fprintf(stderr, "%s", (char*)(mem.getData()));
			}
		} else {
			Console::print(s);
		}
#else
		StringCstr s(GetLineString(_tag, _content));
		if (priority >= LogPriority::Error) {
			fprintf(stderr, "%s", s.getData());
		} else {
			printf("%s", s.getData());
		}
#endif
	}

}
