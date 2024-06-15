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

#ifndef CHECKHEADER_SLIB_CORE_TIME
#define CHECKHEADER_SLIB_CORE_TIME

#include "default_members.h"
#include "common_members.h"

/*******************************************************
 Time contains an integral value holding the number of
 microseconds since 00:00, Jan 1 1970 UTC, corresponding
 to POSIX time (seconds).
*******************************************************/

namespace slib
{

	class Locale;
	class Time;
	class TimeZone;
	class StringView;
	class StringParam;

	template <class T> class Atomic;

	enum class TimeTextType
	{
		Long = 0,
		Short = 1,
		SingleChar = 2
	};

	enum class TimeFormat
	{
		DateTime,
		MediumDateTime,
		ShortDateTime,
		DateTime_12Hour,
		MediumDateTime_12Hour,
		ShortDateTime_12Hour,
		Date,
		MediumDate,
		ShortDate,
		Time,
		ShortTime,
		Time_12Hour,
		ShortTime_12Hour,
		Year,
		Month,
		ShortMonth,
		Day,
		Hour,
		Hour_12,
		Hour_AM_PM,
		AM_PM,
		Minute,
		Second,
		YearMonth,
		ShortYearMonth,
		MonthDay,
		ShortMonthDay,
		HourMinute,
		HourMinute_12Hour,
		ShortHourMinute_12Hour,
		MinuteSecond,
		Weekday,
		ShortWeakday,
		WeekdayDateTime,
		MediumWeekdayDateTime,
		ShortWeekdayDateTime,
		WeekdayDateTime_12Hour,
		MediumWeekdayDateTime_12Hour,
		ShortWeekdayDateTime_12Hour,
		WeekdayDate,
		MediumWeekdayDate,
		ShortWeekdayDate
	};

	class SLIB_EXPORT TimeComponents
	{
	public:
		sl_int32 year;
		sl_uint8 month;
		sl_uint8 day;
		sl_uint8 dayOfWeek;
		sl_uint8 hour;
		sl_uint8 minute;
		sl_uint8 second;
		sl_uint16 milliseconds;
		sl_uint16 microseconds;

	public:
		TimeComponents() noexcept;

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(TimeComponents)

	public:
		void clearTime() noexcept;

	public:
		SLIB_DECLARE_CLASS_PARSE_MEMBERS(TimeComponents)

	};

	class SLIB_EXPORT Time
	{
	protected:
		sl_int64 m_time; // microseconds

	public:
		static const TimeZone& LocalZone;

	public:
		SLIB_CONSTEXPR Time() noexcept: m_time(0) {}

		SLIB_CONSTEXPR Time(const Time& other) noexcept: m_time(other.m_time) {}

		SLIB_CONSTEXPR Time(sl_int32 time) noexcept: m_time(time) {}

		SLIB_CONSTEXPR Time(sl_uint32 time) noexcept: m_time(time) {}

		SLIB_CONSTEXPR Time(sl_int64 time) noexcept: m_time(time) {}

		SLIB_CONSTEXPR Time(sl_uint64 time) noexcept: m_time(time) {}

		Time(sl_int32 year, sl_int32 month, sl_int32 day, const TimeZone& zone = Time::LocalZone) noexcept;

		Time(sl_int32 year, sl_int32 month, sl_int32 day, sl_int32 hour, sl_int32 minute, sl_int32 second, sl_int32 milliseconds = 0, sl_int32 microseconds = 0, const TimeZone& zone = Time::LocalZone) noexcept;

		Time(const TimeComponents& comps, const TimeZone& zone = Time::LocalZone) noexcept;

	public:
		static Time now() noexcept;

		static Time withMicroseconds(sl_int64 s) noexcept;

		static Time withMicrosecondsF(double s) noexcept;

		static Time withMilliseconds(sl_int64 s) noexcept;

		static Time withMillisecondsF(double s) noexcept;

		static Time withSeconds(sl_int64 s) noexcept;

		static Time withSecondsF(double s) noexcept;

		static Time withMinutes(sl_int64 minutes) noexcept;

		static Time withMinutesF(double minutes) noexcept;

		static Time withHours(sl_int64 hours) noexcept;

		static Time withHoursF(double hours) noexcept;

		static Time withDays(sl_int64 days) noexcept;

		static Time withDaysF(double days) noexcept;

		static Time withTime(sl_int64 hours, sl_int64 minutes, sl_int64 seconds) noexcept;

		static Time withTimeF(double hours, double minutes, double seconds) noexcept;

		static Time withTime(sl_int64 hours, sl_int64 minutes, sl_int64 seconds, sl_int64 milliseconds) noexcept;

		static Time withTimeF(double hours, double minutes, double seconds, double milliseconds) noexcept;

		static Time withTime(sl_int64 hours, sl_int64 minutes, sl_int64 seconds, sl_int64 milliseconds, sl_int64 microseconds) noexcept;

		static Time withTimeF(double hours, double minutes, double seconds, double milliseconds, double microseconds) noexcept;

		SLIB_CONSTEXPR static Time zero()
		{
			return 0;
		}

		Time& setZero() noexcept
		{
			m_time = 0;
			return *this;
		}

		SLIB_CONSTEXPR sl_bool isZero() const
		{
			return m_time == 0;
		}

		SLIB_CONSTEXPR sl_bool isNotZero() const
		{
			return m_time != 0;
		}

		sl_int64 toInt() const noexcept
		{
			return m_time;
		}

		Time& setInt(sl_int64 time) noexcept;

		static Time fromInt(sl_int64 time) noexcept;

		// Convert to time_t mode (1970 Based)
		sl_int64 toUnixTime() const noexcept;

		// Convert from time_t mode (1970 Based)
		Time& setUnixTime(sl_int64 time) noexcept;

		// Convert from time_t mode (1970 Based)
		static Time fromUnixTime(sl_int64 time) noexcept;

		// Convert to time_t mode (1970 Based)
		double toUnixTimeF() const noexcept;

		// Convert from time_t mode (1970 Based)
		Time& setUnixTimeF(double time) noexcept;

		// Convert from time_t mode (1970 Based)
		static Time fromUnixTimeF(double time) noexcept;

		// Convert to FILETIME mode (1601 Based)
		sl_int64 toWindowsFileTime() const noexcept;

		// Convert from FILETIME mode (1601 Based)
		Time& setWindowsFileTime(sl_int64 time) noexcept;

		// Convert from FILETIME mode (1601 Based)
		static Time fromWindowsFileTime(sl_int64 time) noexcept;

		Time& add(sl_int64 time) noexcept;

		Time& add(const Time& other) noexcept;

	public:
		Time& operator=(const Time& other) noexcept
		{
			m_time = other.m_time;
			return *this;
		}

		Time& operator=(sl_int32 time) noexcept
		{
			m_time = time;
			return *this;
		}

		Time& operator=(sl_uint32 time) noexcept
		{
			m_time = time;
			return *this;
		}

		Time& operator=(sl_int64 time) noexcept
		{
			m_time = time;
			return *this;
		}

		Time& operator=(sl_uint64 time) noexcept
		{
			m_time = time;
			return *this;
		}


		Time operator+(sl_int64 time) const noexcept
		{
			return m_time + time;
		}

		Time operator+(const Time& time) const noexcept
		{
			return m_time + time.m_time;
		}

		Time& operator+=(sl_int64 time) noexcept
		{
			m_time += time;
			return *this;
		}

		Time& operator+=(const Time& time) noexcept
		{
			m_time += time.m_time;
			return *this;
		}

		Time operator-(sl_int64 time) const noexcept
		{
			return m_time - time;
		}

		Time operator-(const Time& time) const noexcept
		{
			return m_time - time.m_time;
		}

		Time& operator-=(sl_int64 time) noexcept
		{
			m_time -= time;
			return *this;
		}

		Time& operator-=(const Time& time) noexcept
		{
			m_time -= time.m_time;
			return *this;
		}

	public:
		Time& setNow() noexcept;

		static sl_bool setSystemTime(const Time& time) noexcept;

		void get(TimeComponents& output, const TimeZone& zone = Time::LocalZone) const noexcept;

		void getUTC(TimeComponents& output) const noexcept;

		Time& set(const TimeComponents& comps, const TimeZone& zone = Time::LocalZone) noexcept;

		Time& setUTC(const TimeComponents& comps) noexcept;

		Time& set(sl_int32 year, sl_int32 month, sl_int32 day, sl_int32 hour = 0, sl_int32 minute = 0, sl_int32 second = 0, sl_int32 milliseconds = 0, sl_int32 microseconds = 0, const TimeZone& zone = Time::LocalZone) noexcept;

		Time& setUTC(sl_int32 year, sl_int32 month, sl_int32 day, sl_int32 hour = 0, sl_int32 minute = 0, sl_int32 second = 0, sl_int32 milliseconds = 0, sl_int32 microseconds = 0) noexcept;

		Time& setDate(sl_int32 year, sl_int32 month, sl_int32 day, const TimeZone& zone = Time::LocalZone) noexcept;

		sl_int32 getYear(const TimeZone& zone = Time::LocalZone) const noexcept;

		Time& setYear(sl_int32 year, const TimeZone& zone = Time::LocalZone) noexcept;

		Time& addYears(sl_int32 years, const TimeZone& zone = Time::LocalZone) noexcept;

		sl_bool isLeapYear(const TimeZone& zone = Time::LocalZone) const noexcept;

		static sl_bool isLeapYear(sl_int32 year) noexcept;

		sl_int32 getMonth(const TimeZone& zone = Time::LocalZone) const noexcept;

		Time& setMonth(sl_int32 month, const TimeZone& zone = Time::LocalZone) noexcept;

		Time& addMonths(sl_int32 months, const TimeZone& zone = Time::LocalZone) noexcept;

		sl_int32 getDay(const TimeZone& zone = Time::LocalZone) const noexcept;

		Time& setDay(sl_int32 day, const TimeZone& zone = Time::LocalZone) noexcept;

		Time& addDays(sl_int64 days) noexcept;

		double getDayF(const TimeZone& zone = Time::LocalZone) const noexcept;

		Time& setDayF(double day, const TimeZone& zone = Time::LocalZone) noexcept;

		Time& addDaysF(double days) noexcept;

		sl_int32 getHour(const TimeZone& zone = Time::LocalZone) const noexcept;

		Time& setHour(sl_int32 hour, const TimeZone& zone = Time::LocalZone) noexcept;

		Time& addHours(sl_int64 hours) noexcept;

		double getHourF(const TimeZone& zone = Time::LocalZone) const noexcept;

		Time& setHourF(double hour, const TimeZone& zone = Time::LocalZone) noexcept;

		Time& addHoursF(double hours) noexcept;

		sl_int32 getHour12(const TimeZone& zone = Time::LocalZone) const noexcept;

		sl_bool isAM(const TimeZone& zone = Time::LocalZone) const noexcept;

		sl_bool isPM(const TimeZone& zone = Time::LocalZone) const noexcept;

		String getAM_PM(const TimeZone& zone, const Locale& locale) const noexcept;

		String getAM_PM(const TimeZone& zone) const noexcept;

		String getAM_PM(const Locale& locale) const noexcept;

		String getAM_PM() const noexcept;

		sl_int32 getMinute(const TimeZone& zone = Time::LocalZone) const noexcept;

		Time& setMinute(sl_int32 minute, const TimeZone& zone = Time::LocalZone) noexcept;

		Time& addMinutes(sl_int64 minutes) noexcept;

		double getMinuteF(const TimeZone& zone = Time::LocalZone) const noexcept;

		Time& setMinuteF(double minute, const TimeZone& zone = Time::LocalZone) noexcept;

		Time& addMinutesF(double minutes) noexcept;

		sl_int32 getSecond(const TimeZone& zone = Time::LocalZone) const noexcept;

		Time& setSecond(sl_int32 second, const TimeZone& zone = Time::LocalZone) noexcept;

		Time& addSeconds(sl_int64 seconds) noexcept;

		double getSecondF(const TimeZone& zone = Time::LocalZone) const noexcept;

		Time& setSecondF(double second, const TimeZone& zone = Time::LocalZone) noexcept;

		Time& addSecondsF(double seconds) noexcept;

		sl_int32 getMillisecond() const noexcept;

		Time& setMillisecond(sl_int32 millis) noexcept;

		Time& addMilliseconds(sl_int64 millis) noexcept;

		double getMillisecondF() const noexcept;

		Time& setMillisecondF(double millis) noexcept;

		Time& addMillisecondsF(double millis) noexcept;

		sl_int32 getMicrosecond() const noexcept;

		Time& setMicrosecond(sl_int64 micros) noexcept;

		Time& addMicroseconds(sl_int64 micros) noexcept;

		double getMicrosecondF() const noexcept;

		Time& setMicrosecondF(double micros) noexcept;

		Time& addMicrosecondsF(double micros) noexcept;

		sl_int32 getDayOfWeek(const TimeZone& zone = Time::LocalZone) const noexcept;

		Time& setDayOfWeek(sl_int32 day, const TimeZone& zone = Time::LocalZone) noexcept;

		sl_int32 getDayOfYear(const TimeZone& zone = Time::LocalZone) const noexcept;

		Time& setDayOfYear(sl_int32 day, const TimeZone& zone = Time::LocalZone) noexcept;

		sl_int64 getDayCount() const noexcept;

		Time& setDayCount(sl_int64 days) noexcept;

		double getDayCountF() const noexcept;

		Time& setDayCountF(double days) noexcept;

		sl_int64 getHourCount() const noexcept;

		Time& setHourCount(sl_int64 hours) noexcept;

		double getHourCountF() const noexcept;

		Time& setHourCountF(double hours) noexcept;

		sl_int64 getMinuteCount() const noexcept;

		Time& setMinuteCount(sl_int64 minutes) noexcept;

		double getMinuteCountF() const noexcept;

		Time& setMinuteCountF(double minutes) noexcept;

		sl_int64 getSecondCount() const noexcept;

		Time& setSecondCount(sl_int64 seconds) noexcept;

		double getSecondCountF() const noexcept;

		Time& setSecondCountF(double seconds) noexcept;

		sl_int64 getMillisecondCount() const noexcept;

		Time& setMillisecondCount(sl_int64 milis) noexcept;

		double getMillisecondCountF() const noexcept;

		Time& setMillisecondCountF(double milis) noexcept;

		sl_int64 getMicrosecondCount() const noexcept;

		Time& setMicrosecondCount(sl_int64 micros) noexcept;

		double getMicrosecondCountF() const noexcept;

		Time& setMicrosecondCountF(double micros) noexcept;

		// In Seconds
		sl_int64 getLocalTimeOffset() const noexcept;

		static sl_int64 getLocalTimeOffset(sl_int32 year, sl_int32 month, sl_int32 day) noexcept;

		sl_int32 getDayCountInMonth(const TimeZone& zone = Time::LocalZone) const noexcept;

		sl_int32 getDayCountInYear(const TimeZone& zone = Time::LocalZone) const noexcept;

		sl_int32 getQuarter(const TimeZone& zone = Time::LocalZone) const noexcept;

		Time getDateOnly(const TimeZone& zone = Time::LocalZone) const noexcept;

		Time getTimeOnly(const TimeZone& zone = Time::LocalZone) const noexcept;


		static String getWeekdayText(sl_int32 weekday, TimeTextType type, const Locale& locale) noexcept;

		String getWeekdayShort(const TimeZone& zone, const Locale& locale) const noexcept;

		String getWeekdayShort(const TimeZone& zone) const noexcept;

		String getWeekdayShort(const Locale& locale) const noexcept;

		String getWeekdayShort() const noexcept;

		String getWeekdayLong(const TimeZone& zone, const Locale& locale) const noexcept;

		String getWeekdayLong(const TimeZone& zone) const noexcept;

		String getWeekdayLong(const Locale& locale) const noexcept;

		String getWeekdayLong() const noexcept;

		String getWeekday(const TimeZone& zone, const Locale& locale) const noexcept;

		String getWeekday(const TimeZone& zone) const noexcept;

		String getWeekday(const Locale& locale) const noexcept;

		String getWeekday() const noexcept;


		static String getAM_Text(const Locale& locale) noexcept;

		static String getPM_Text(const Locale& locale) noexcept;


		// English
		static String getMonthText(sl_int32 month, TimeTextType type) noexcept;

		String getMonthShort(const TimeZone& zone = Time::LocalZone) const noexcept;

		String getMonthLong(const TimeZone& zone = Time::LocalZone) const noexcept;


		// IMF-fixdate
		String toHttpDate() const noexcept;

		static sl_reg parseHttpDate(Time* _output, const sl_char8* buf, sl_size posBegin = 0, sl_size posEnd = SLIB_SIZE_MAX) noexcept;

		sl_bool parseHttpDate(const StringParam& date) noexcept;


		String toString(const TimeZone& zone = Time::LocalZone) const noexcept;

		// ISO 8601 (UTC): YYYY-MM-DDTHH:mm:ss.sssZ
		String toISOString() const noexcept;

		String getDateString(const TimeZone& zone = Time::LocalZone) const noexcept;

		String getDateString(sl_char8 delimiter, const TimeZone& zone = Time::LocalZone) const noexcept;

		String getTimeString(const TimeZone& zone = Time::LocalZone) const noexcept;

		String getPeriodString(const Time& minUnit, const Time& maxUnit, sl_bool flagLong, const Locale& locale) const noexcept;

		String getPeriodString(const Time& minUnit = Time::withSeconds(1), const Time& maxUnit = Time::zero(), sl_bool flagLong = sl_true) const noexcept;

		String getDiffString(const Time& timeFrom, const Time& minUnit, const Time& maxUnit, sl_bool flagLong, const Locale& locale) const noexcept;

		String getDiffString(const Time& timeFrom, const Time& minUnit = Time::withSeconds(1), const Time& maxUnit = Time::zero(), sl_bool flagLong = sl_true) const noexcept;


		static String format(const TimeComponents& comps, TimeFormat fmt, const Locale& locale) noexcept;

		static String format(const TimeComponents& comps, TimeFormat fmt) noexcept;

		String format(TimeFormat fmt, const TimeZone& zone, const Locale& locale) const noexcept;

		String format(TimeFormat fmt, const TimeZone& zone) const noexcept;

		String format(TimeFormat fmt, const Locale& locale) const noexcept;

		String format(TimeFormat fmt) const noexcept;

		String format(const StringView& fmt, const Locale& locale) const noexcept;

		String format(const StringView& fmt) const noexcept;


		static Time fromString(const StringParam& str, const TimeZone& zone = Time::LocalZone) noexcept;

	public:
		SLIB_DECLARE_CLASS_COMPARE_HASH_MEMBERS(Time)
		SLIB_DECLARE_CLASS_PARSE_MEMBERS(Time)
		SLIB_DECLARE_CLASS_PARSE2_MEMBERS(Time, const TimeZone&, zone)

		sl_bool equals(sl_int64 other) const noexcept;
		sl_compare_result compare(sl_int64 other) const noexcept;

	protected:
		// platform functions
		static sl_bool _toPlatformComponents(TimeComponents& output, sl_int64 seconds, sl_bool flagUTC) noexcept;

		static sl_bool _toPlatformSeconds(sl_int64& seconds, sl_int32 year, sl_int32 month, sl_int32 day, sl_bool flagUTC) noexcept;

		void _setNow() noexcept;

		sl_bool _setToSystem() const noexcept;

		// helper functions
		static void _toComponents(TimeComponents& output, sl_int64 seconds, sl_bool flagUTC) noexcept;

		static sl_int64 _toSeconds(sl_int32 year, sl_int32 month, sl_int32 day, sl_bool flagUTC) noexcept;

	};

}

#endif
