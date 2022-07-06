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

#ifndef CHECKHEADER_SLIB_CORE_LOG
#define CHECKHEADER_SLIB_CORE_LOG

#include "variant.h"

namespace slib
{

	enum class LogPriority
	{
		Unknown = 0,
		Default = 1,
		Verbose = 2,
		Debug = 3,
		Info = 4,
		Warning = 5,
		Error = 6,
		Fatal = 7,
		Silent = 8
	};

	class SLIB_EXPORT Logger : public Object
	{
		SLIB_DECLARE_OBJECT

	public:
		Logger();

		~Logger();

	public:
		virtual void log(LogPriority priority, const StringParam& tag, const StringParam& content) = 0;

		void log(const StringParam& tag, const StringParam& content);

		void logError(const StringParam& tag, const StringParam& content);

		void logDebug(const StringParam& tag, const StringParam& content);

	public:
		LogPriority getMinimumPriority();

		void setMinimumPriority(LogPriority priority);

	public:
		static Ref<Logger> global();

		static void setGlobal(const Ref<Logger>& logger);

		static Ref<Logger> getConsoleLogger();

		static Ref<Logger> createFileLogger(const String& fileNameFormat);

		static Ref<Logger> createFileLogger(const String& fileNameFormat, const String& errorFileNameFormat);

		static Ref<Logger> join(const Ref<Logger>& logger1, const Ref<Logger>& logger2);

	protected:
		LogPriority m_priorityMinimum;

	};
	
	class SLIB_EXPORT FileLogger : public Logger
	{
	public:
		FileLogger();

		FileLogger(const String& fileNameFormat);

		FileLogger(const String& fileNameFormat, const String& errorFileNameFormat);

		~FileLogger();

	public:
		void log(LogPriority priority, const StringParam& tag, const StringParam& content) override;

	protected:
		String m_fileNameFormat;
		String m_errorFileNameFormat;

	};

	class ConsoleLogger : public Logger
	{
	public:
		ConsoleLogger();

		~ConsoleLogger();

	public:
		void log(LogPriority priority, const StringParam& tag, const StringParam& content) override;

	};

	class LoggerSet : public Logger
	{
	public:
		LoggerSet();

		~LoggerSet();

	public:
		void add(const Ref<Logger>& logger);

	public:
		void log(LogPriority priority, const StringParam& tag, const StringParam& content) override;

	protected:
		CList< Ref<Logger> > m_loggers;

	};

	template <class... ARGS>
	SLIB_INLINE static void Log(LogPriority priority, const StringParam& tag, const StringView& fmt, ARGS&&... args)
	{
		Ref<Logger> logger = Logger::global();
		if (logger.isNotNull()) {
			if (logger->getMinimumPriority() <= priority) {
				logger->log(priority, tag, String::format(fmt, Forward<ARGS>(args)...));
			}
		}
	}

	template <class... ARGS>
	SLIB_INLINE static void Log(const StringParam& tag, const StringView& fmt, ARGS&&... args)
	{
		Log(LogPriority::Info, tag, fmt, Forward<ARGS>(args)...);
	}
	
	template <class... ARGS>
	SLIB_INLINE static void LogError(const StringParam& tag, const StringView& fmt, ARGS&&... args)
	{
		Log(LogPriority::Error, tag, fmt, Forward<ARGS>(args)...);
	}
	
	template <class... ARGS>
	SLIB_INLINE static void LogDebug(const StringParam& tag, const StringView& fmt, ARGS&&... args)
	{
		Log(LogPriority::Debug, tag, fmt, Forward<ARGS>(args)...);
	}

#define SLIB_LOG(TAG, FORMAT, ...) slib::Log(TAG, FORMAT, ##__VA_ARGS__)
#define SLIB_LOG_ERROR(TAG, FORMAT, ...) slib::Log(LogPriority::Error, TAG, FORMAT, ##__VA_ARGS__)
#ifdef SLIB_DEBUG
#define SLIB_LOG_DEBUG(TAG, FORMAT, ...) slib::Log(LogPriority::Debug, TAG, FORMAT, ##__VA_ARGS__)
#else
#define SLIB_LOG_DEBUG(TAG, FORMAT, ...)
#endif

}

#endif
