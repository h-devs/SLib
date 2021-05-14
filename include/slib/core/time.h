/*
 *   Copyright (c) 2008-2020 SLIBIO <https://github.com/SLIBIO>
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

#include "definition.h"

#include "default_members.h"

/*******************************************************
 Time contains an integral value holding the number of
 microseconds since 00:00, Jan 1 1970 UTC, corresponding
 to POSIX time (seconds).
*******************************************************/

namespace slib
{
	
	class String;
	class String16;
	class StringParam;
	class Locale;
	class Time;
	class TimeZone;

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
		
	};
	
	class SLIB_EXPORT Time
	{
	protected:
		sl_int64 m_time; // microseconds

	public:
		static const TimeZone& LocalZone;

	public:
		constexpr Time() noexcept: m_time(0) {}

		constexpr Time(const Time& other) noexcept: m_time(other.m_time) {}

		constexpr Time(sl_int32 time) noexcept: m_time(time) {}

		constexpr Time(sl_uint32 time) noexcept: m_time(time) {}

		constexpr Time(sl_int64 time) noexcept: m_time(time) {}

		constexpr Time(sl_uint64 time) noexcept: m_time(time) {}

		Time(sl_int32 year, sl_int32 month, sl_int32 date, const TimeZone& zone = Time::LocalZone) noexcept;

		Time(sl_int32 year, sl_int32 month, sl_int32 date, sl_int32 hour, sl_int32 minute, sl_int32 second, sl_int32 milliseconds = 0, sl_int32 microseconds = 0, const TimeZone& zone = Time::LocalZone) noexcept;

		Time(const TimeComponents& comps, const TimeZone& zone = Time::LocalZone) noexcept;
		
	public:
		static Time now() noexcept;
		
		static Time withMicroseconds(sl_int64 s) noexcept;
		
		static Time withMicrosecondsf(double s) noexcept;

		static Time withMilliseconds(sl_int64 s) noexcept;
		
		static Time withMillisecondsf(double s) noexcept;

		static Time withSeconds(sl_int64 s) noexcept;
		
		static Time withSecondsf(double s) noexcept;

		static Time withMinutes(sl_int64 minutes) noexcept;
		
		static Time withMinutesf(double minutes) noexcept;
		
		static Time withHours(sl_int64 hours) noexcept;
		
		static Time withHoursf(double hours) noexcept;
		
		static Time withDays(sl_int64 days) noexcept;
		
		static Time withDaysf(double days) noexcept;
		
		static Time withTime(sl_int64 hours, sl_int64 minutes, sl_int64 seconds) noexcept;
		
		static Time withTimef(double hours, double minutes, double seconds) noexcept;

		static Time withTime(sl_int64 hours, sl_int64 minutes, sl_int64 seconds, sl_int64 milliseconds) noexcept;
		
		static Time withTimef(double hours, double minutes, double seconds, double milliseconds) noexcept;

		static Time withTime(sl_int64 hours, sl_int64 minutes, sl_int64 seconds, sl_int64 milliseconds, sl_int64 microseconds) noexcept;
		
		static Time withTimef(double hours, double minutes, double seconds, double milliseconds, double microseconds) noexcept;
		
		constexpr static Time zero()
		{
			return 0;
		}

		Time& setZero() noexcept
		{
			m_time = 0;
			return *this;
		}

		constexpr sl_bool isZero() const
		{
			return m_time == 0;
		}

		constexpr sl_bool isNotZero() const
		{
			return m_time != 0;
		}

		sl_int64 toInt() const noexcept	
		{
			return m_time;
		}
	
		Time& setInt(sl_int64 time) noexcept;
		
		static Time fromInt(sl_int64 time) noexcept;
		
		sl_int64 toUnixTime() const noexcept
		{
			sl_int64 n = m_time / 1000000;
			if ((m_time % 1000000) < 0) {
				n += 1;
			}
			return n;
		}

		Time& setUnixTime(sl_int64 time) noexcept;
		
		static Time fromUnixTime(sl_int64 time) noexcept;
		
		double toUnixTimef() const noexcept
		{
			return (double)(m_time / 1000000);
		}
		
		Time& setUnixTimef(double time) noexcept;
		
		static Time fromUnixTimef(double time) noexcept;

		// Convert 1970 Based (time_t mode) to 1601 Based (FILETIME mode)
		sl_int64 toWindowsFileTime() const noexcept;

		// Convert 1601 Based (FILETIME mode) to 1970 Based (time_t mode)
		Time& setWindowsFileTime(sl_int64 time) noexcept;

		// Convert 1601 Based (FILETIME mode) to 1970 Based (time_t mode)
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


		sl_bool operator==(const Time& other) const noexcept
		{
			return m_time == other.m_time;
		}

		sl_bool operator<=(const Time& other) const noexcept
		{
			return m_time <= other.m_time;
		}

		sl_bool operator>=(const Time& other) const noexcept
		{
			return m_time >= other.m_time;
		}

		sl_bool operator!=(const Time& other) const noexcept
		{
			return m_time != other.m_time;
		}

		sl_bool operator<(const Time& other) const noexcept
		{
			return m_time < other.m_time;
		}

		sl_bool operator>(const Time& other) const noexcept
		{
			return m_time > other.m_time;
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

		Time& set(sl_int32 year, sl_int32 month, sl_int32 date, sl_int32 hour = 0, sl_int32 minute = 0, sl_int32 second = 0, sl_int32 milliseconds = 0, sl_int32 microseconds = 0, const TimeZone& zone = Time::LocalZone) noexcept;

		Time& setUTC(sl_int32 year, sl_int32 month, sl_int32 date, sl_int32 hour = 0, sl_int32 minute = 0, sl_int32 second = 0, sl_int32 milliseconds = 0, sl_int32 microseconds = 0) noexcept;

		Time& setDate(sl_int32 year, sl_int32 month, sl_int32 day, const TimeZone& zone = Time::LocalZone) noexcept;

		sl_int32 getYear(const TimeZone& zone = Time::LocalZone) const noexcept;

		Time& setYear(sl_int32 year, const TimeZone& zone = Time::LocalZone) noexcept;

		Time& addYears(sl_int32 years, const TimeZone& zone = Time::LocalZone) noexcept;

		sl_int32 getMonth(const TimeZone& zone = Time::LocalZone) const noexcept;

		Time& setMonth(sl_int32 month, const TimeZone& zone = Time::LocalZone) noexcept;

		Time& addMonths(sl_int32 months, const TimeZone& zone = Time::LocalZone) noexcept;

		sl_int32 getDay(const TimeZone& zone = Time::LocalZone) const noexcept;

		Time& setDay(sl_int32 day, const TimeZone& zone = Time::LocalZone) noexcept;

		Time& addDays(sl_int64 days) noexcept;

		double getDayf(const TimeZone& zone = Time::LocalZone) const noexcept;

		Time& setDayf(double day, const TimeZone& zone = Time::LocalZone) noexcept;

		Time& addDaysf(double days) noexcept;

		sl_int32 getHour(const TimeZone& zone = Time::LocalZone) const noexcept;

		Time& setHour(sl_int32 hour, const TimeZone& zone = Time::LocalZone) noexcept;

		Time& addHours(sl_int64 hours) noexcept;

		double getHourf(const TimeZone& zone = Time::LocalZone) const noexcept;

		Time& setHourf(double hour, const TimeZone& zone = Time::LocalZone) noexcept;

		Time& addHoursf(double hours) noexcept;

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

		double getMinutef(const TimeZone& zone = Time::LocalZone) const noexcept;
	
		Time& setMinutef(double minute, const TimeZone& zone = Time::LocalZone) noexcept;

		Time& addMinutesf(double minutes) noexcept;

		sl_int32 getSecond(const TimeZone& zone = Time::LocalZone) const noexcept;

		Time& setSecond(sl_int32 second, const TimeZone& zone = Time::LocalZone) noexcept;

		Time& addSeconds(sl_int64 seconds) noexcept;

		double getSecondf(const TimeZone& zone = Time::LocalZone) const noexcept;

		Time& setSecondf(double second, const TimeZone& zone = Time::LocalZone) noexcept;

		Time& addSecondsf(double seconds) noexcept;

		sl_int32 getMillisecond() const noexcept;

		Time& setMillisecond(sl_int32 millis) noexcept;

		Time& addMilliseconds(sl_int64 millis) noexcept;

		double getMillisecondf() const noexcept;

		Time& setMillisecondf(double millis) noexcept;

		Time& addMillisecondsf(double millis) noexcept;

		sl_int32 getMicrosecond() const noexcept;

		Time& setMicrosecond(sl_int32 micros) noexcept;

		Time& addMicroseconds(sl_int64 micros) noexcept;

		double getMicrosecondf() const noexcept;
	
		Time& setMicrosecondf(double micros) noexcept;

		Time& addMicrosecondsf(double micros) noexcept;

		sl_int32 getDayOfWeek(const TimeZone& zone = Time::LocalZone) const noexcept;

		Time& setDayOfWeek(sl_int32 day, const TimeZone& zone = Time::LocalZone) noexcept;

		sl_int32 getDayOfYear(const TimeZone& zone = Time::LocalZone) const noexcept;

		Time& setDayOfYear(sl_int32 day, const TimeZone& zone = Time::LocalZone) noexcept;

		sl_int64 getDaysCount() const noexcept;

		Time& setDaysCount(sl_int64 days) noexcept;

		double getDaysCountf() const noexcept;

		Time& setDaysCountf(double days) noexcept;

		sl_int64 getHoursCount() const noexcept;

		Time& setHoursCount(sl_int64 hours) noexcept;

		double getHoursCountf() const noexcept;

		Time& setHoursCountf(double hours) noexcept;

		sl_int64 getMinutesCount() const noexcept;

		Time& setMinutesCount(sl_int64 minutes) noexcept;

		double getMinutesCountf() const noexcept;

		Time& setMinutesCountf(double minutes) noexcept;

		sl_int64 getSecondsCount() const noexcept;

		Time& setSecondsCount(sl_int64 seconds) noexcept;

		double getSecondsCountf() const noexcept;

		Time& setSecondsCountf(double seconds) noexcept;

		sl_int64 getMillisecondsCount() const noexcept;

		Time& setMillisecondsCount(sl_int64 milis) noexcept;

		double getMillisecondsCountf() const noexcept;

		Time& setMillisecondsCountf(double milis) noexcept;

		sl_int64 getMicrosecondsCount() const noexcept;

		Time& setMicrosecondsCount(sl_int64 micros) noexcept;

		double getMicrosecondsCountf() const noexcept;
	
		Time& setMicrosecondsCountf(double micros) noexcept;
	
		// In Seconds
		sl_int64 getLocalTimeOffset() const noexcept;

		sl_int32 getDaysCountInMonth(const TimeZone& zone = Time::LocalZone) const noexcept;

		sl_int32 getDaysCountInYear(const TimeZone& zone = Time::LocalZone) const noexcept;

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

		String getDateString(const TimeZone& zone = Time::LocalZone) const noexcept;

		String getDateString(sl_char8 delimiter, const TimeZone& zone = Time::LocalZone) const noexcept;
		
		String getTimeString(const TimeZone& zone = Time::LocalZone) const noexcept;

		String getPeriodString(const Time& minUnit, const Time& maxUnit, const Locale& locale) const noexcept;

		String getPeriodString(const Time& minUnit = Time::withSeconds(1), const Time& maxUnit = Time::zero()) const noexcept;

		String getDiffString(const Time& timeFrom, const Time& minUnit, const Time& maxUnit, const Locale& locale) const noexcept;

		String getDiffString(const Time& timeFrom, const Time& minUnit = Time::withSeconds(1), const Time& maxUnit = Time::zero()) const noexcept;

		
		static String format(const TimeComponents& comps, TimeFormat fmt, const Locale& locale) noexcept;
	
		static String format(const TimeComponents& comps, TimeFormat fmt) noexcept;

		String format(TimeFormat fmt, const TimeZone& zone, const Locale& locale) const noexcept;

		String format(TimeFormat fmt, const TimeZone& zone) const noexcept;

		String format(TimeFormat fmt, const Locale& locale) const noexcept;

		String format(TimeFormat fmt) const noexcept;

		String format(const StringParam& fmt, const Locale& locale) const noexcept;

		String format(const StringParam& fmt) const noexcept;

		
		static Time fromString(const StringParam& str, const TimeZone& zone = Time::LocalZone) noexcept;

		sl_bool setString(const StringParam& str, const TimeZone& zone = Time::LocalZone) noexcept;
		
		
		template <class ST>
		static sl_bool parse(const ST& str, TimeComponents* _out) noexcept;
		
		template <class ST>
		static sl_bool parse(const ST& str, Time* _out) noexcept;

		template <class ST>
		sl_bool parse(const ST& str) noexcept;
		
		template <class ST>
		static sl_bool parse(const ST& str, const TimeZone& zone, Time* _out) noexcept;
		
		template <class ST>
		sl_bool parse(const ST& str, const TimeZone& zone) noexcept;
		
		/* platform functions */
	protected:
		sl_bool _get(TimeComponents& output, sl_bool flagUTC) const noexcept;

		static sl_int64 _set(sl_int32 year, sl_int32 month, sl_int32 date, sl_int32 hour, sl_int32 minute, sl_int32 second, sl_bool flagUTC) noexcept;

		void _setNow() noexcept;

		sl_bool _setToSystem() const noexcept;

	};
	
}

#endif
