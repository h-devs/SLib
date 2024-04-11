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

#include "slib/core/time.h"
#include "slib/core/time_counter.h"
#include "slib/core/time_keeper.h"
#include "slib/core/time_zone.h"

#include "slib/system/system.h"
#include "slib/core/locale.h"
#include "slib/core/variant.h"
#include "slib/core/string_buffer.h"
#include "slib/core/safe_static.h"

#define TIME_MILLIS SLIB_INT64(1000)
#define TIME_MILLISF 1000.0
#define TIME_SECOND SLIB_INT64(1000000)
#define TIME_SECONDF 1000000.0
#define TIME_MINUTE SLIB_INT64(60000000)
#define TIME_MINUTEF 60000000.0
#define TIME_HOUR SLIB_INT64(3600000000)
#define TIME_HOURF 3600000000.0
#define TIME_DAY SLIB_INT64(86400000000)
#define TIME_DAYF 86400000000.0

namespace slib
{

	namespace
	{
		SLIB_INLINE static sl_int64 ToMicroseconds(sl_int64 time)
		{
			return (sl_int64)((sl_uint64)time & SLIB_UINT64(0xFFFFFFFFFFFFFFFE));
		}

		SLIB_INLINE static sl_int64 ToTimeValue(sl_int64 time)
		{
			return (sl_int64)((sl_uint64)time | 1);
		}
	}

	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(TimeComponents)

	TimeComponents::TimeComponents() noexcept
	{
		Base::zeroMemory(this, sizeof(TimeComponents));
	}

	void TimeComponents::clearTime() noexcept
	{
		hour = 0;
		minute = 0;
		second = 0;
		milliseconds = 0;
		microseconds = 0;
	}

	Time::Time(sl_int32 year, sl_int32 month, sl_int32 day, const TimeZone& zone) noexcept
	{
		set(year, month, day, 0, 0, 0, 0, 0, zone);
	}

	Time::Time(sl_int32 year, sl_int32 month, sl_int32 day, sl_int32 hour, sl_int32 minute, sl_int32 second, sl_int32 milliseconds, sl_int32 microseconds, const TimeZone& zone) noexcept
	{
		set(year, month, day, hour, minute, second, milliseconds, microseconds, zone);
	}

	Time::Time(const TimeComponents& comps, const TimeZone& zone) noexcept
	{
		set(comps, zone);
	}

	Time Time::now() noexcept
	{
		Time ret;
		ret.setNow();
		return ret;
	}

	Time Time::withMicroseconds(sl_int64 s) noexcept
	{
		return s;
	}

	Time Time::withMicrosecondsF(double s) noexcept
	{
		return ToMicroseconds((sl_int64)s);
	}

	Time Time::withMilliseconds(sl_int64 s) noexcept
	{
		return s * TIME_MILLIS;
	}

	Time Time::withMillisecondsF(double s) noexcept
	{
		return ToMicroseconds((sl_int64)(s * TIME_MILLISF));
	}

	Time Time::withSeconds(sl_int64 s) noexcept
	{
		return s * TIME_SECOND;
	}

	Time Time::withSecondsF(double s) noexcept
	{
		return ToMicroseconds((sl_int64)(s * TIME_SECONDF));
	}

	Time Time::withMinutes(sl_int64 minutes) noexcept
	{
		return minutes * TIME_MINUTE;
	}

	Time Time::withMinutesF(double minutes) noexcept
	{
		return ToMicroseconds((sl_int64)(minutes * TIME_MINUTEF));
	}

	Time Time::withHours(sl_int64 hours) noexcept
	{
		return hours * TIME_HOUR;
	}

	Time Time::withHoursF(double hours) noexcept
	{
		return ToMicroseconds((sl_int64)(hours * TIME_HOURF));
	}

	Time Time::withDays(sl_int64 days) noexcept
	{
		return days * TIME_DAY;
	}

	Time Time::withDaysF(double days) noexcept
	{
		return ToMicroseconds((sl_int64)(days * TIME_DAYF));
	}

	Time Time::withTime(sl_int64 hours, sl_int64 minutes, sl_int64 seconds) noexcept
	{
		return hours * TIME_HOUR + minutes * TIME_MINUTE + seconds * TIME_SECOND;
	}

	Time Time::withTimeF(double hours, double minutes, double seconds) noexcept
	{
		return ToMicroseconds((sl_int64)(hours * TIME_HOURF + minutes * TIME_MINUTEF + seconds * TIME_SECONDF));
	}

	Time Time::withTime(sl_int64 hours, sl_int64 minutes, sl_int64 seconds, sl_int64 milliseconds) noexcept
	{
		return hours * TIME_HOUR + minutes * TIME_MINUTE + seconds * TIME_SECOND + milliseconds * TIME_MILLIS;
	}

	Time Time::withTimeF(double hours, double minutes, double seconds, double milliseconds) noexcept
	{
		return ToMicroseconds((sl_int64)(hours * TIME_HOURF + minutes * TIME_MINUTEF + seconds * TIME_SECONDF + milliseconds * TIME_MILLISF));
	}

	Time Time::withTime(sl_int64 hours, sl_int64 minutes, sl_int64 seconds, sl_int64 milliseconds, sl_int64 microseconds) noexcept
	{
		return hours * TIME_HOUR + minutes * TIME_MINUTE + seconds * TIME_SECOND + milliseconds * TIME_MILLIS + ToMicroseconds(microseconds);
	}

	Time Time::withTimeF(double hours, double minutes, double seconds, double milliseconds, double microseconds) noexcept
	{
		return ToMicroseconds((sl_int64)(hours * TIME_HOURF + minutes * TIME_MINUTEF + seconds * TIME_SECONDF + milliseconds * TIME_MILLISF + microseconds));
	}

	Time& Time::setInt(sl_int64 time) noexcept
	{
		m_time = time;
		return *this;
	}

	Time Time::fromInt(sl_int64 time) noexcept
	{
		return time;
	}

	sl_int64 Time::toUnixTime() const noexcept
	{
		sl_int64 time = ToMicroseconds(m_time);
		sl_int64 n = time / TIME_SECOND;
		if ((time % TIME_SECOND) < 0) {
			n += 1;
		}
		return n;
	}

	Time& Time::setUnixTime(sl_int64 time) noexcept
	{
		m_time = ToTimeValue(time * TIME_SECOND);
		return *this;
	}

	Time Time::fromUnixTime(sl_int64 time) noexcept
	{
		return ToTimeValue(time * TIME_SECOND);
	}

	double Time::toUnixTimeF() const noexcept
	{
		return (double)(ToMicroseconds(m_time) / TIME_SECOND);
	}

	Time& Time::setUnixTimeF(double time) noexcept
	{
		m_time = ToTimeValue((sl_int64)(time * TIME_SECOND));
		return *this;
	}

	Time Time::fromUnixTimeF(double time) noexcept
	{
		return ToTimeValue((sl_int64)(time * TIME_SECOND));
	}

	sl_int64 Time::toWindowsFileTime() const noexcept
	{
		return (ToMicroseconds(m_time) + SLIB_INT64(11644473600000000)) * 10;
	}

	Time& Time::setWindowsFileTime(sl_int64 time) noexcept
	{
		m_time = ToTimeValue(time / 10 - SLIB_INT64(11644473600000000));
		return *this;
	}

	Time Time::fromWindowsFileTime(sl_int64 time) noexcept
	{
		return ToTimeValue(time / 10 - SLIB_INT64(11644473600000000));
	}

	Time& Time::add(sl_int64 time) noexcept
	{
		m_time += time;
		return *this;
	}

	Time& Time::add(const Time& other) noexcept
	{
		m_time += other.m_time;
		return *this;
	}

	Time& Time::setNow() noexcept
	{
		_setNow();
		m_time = ToTimeValue(m_time);
		return *this;
	}

	sl_bool Time::setSystemTime(const Time& time) noexcept
	{
		return time._setToSystem();
	}

	namespace {

		static const sl_int32 g_normalYearDaysPerMonth[] = { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 };
		static const sl_int32 g_leapYearDaysPerMonth[] = { 0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335 };

		template <typename T1, typename T2>
		SLIB_INLINE static T1 DivSigned(T1 n, T2 m)
		{
			if (n % m < 0) {
				return n / m - 1;
			} else {
				return n / m;
			}
		}

		SLIB_INLINE static sl_int32 GetLeapsTo(sl_int32 year)
		{
			return DivSigned(year, 4) - DivSigned(year, 100) + DivSigned(year, 400) + 1;
		}

		static void GetDateFromDays(sl_int32& year, sl_int32& month, sl_int32& day, sl_int32& weekday, sl_int32 days)
		{
			// January 1, 1970 was a Thursday
			weekday = (sl_int32)((4 + days) % 7);
			if (weekday < 0) {
				weekday += 7;
			}
			year = 1970;
			while (days < 0 || days >= (Time::isLeapYear(year) ? 366 : 365)) {
				sl_int32 yg = year + (sl_int32)(DivSigned(days, 365));
				days -= (sl_int64)(yg - year) * 365 + GetLeapsTo(yg - 1) - GetLeapsTo(year - 1);
				year = yg;
			}
			day = (sl_int32)days;
			const sl_int32* md = Time::isLeapYear(year) ? g_leapYearDaysPerMonth : g_normalYearDaysPerMonth;
			for (month = 11; ; month--) {
				if (day >= md[month]) {
					day -= md[month];
					month++;
					break;
				}
			}
			day++;
		}

		static sl_int32 GetDaysFromEpoch(sl_int32 year, sl_int32 month, sl_int32 day)
		{
			month -= 1;
			sl_int32 d = month / 12;
			month %= 12;
			if (month < 0) {
				month += 12;
				d -= 1;
			}
			year += d;
			const sl_int32* md = Time::isLeapYear(year) ? g_leapYearDaysPerMonth : g_normalYearDaysPerMonth;
			return year * 365 + GetLeapsTo(year - 1) - 719528 + md[month] + day - 1;
		}

	}

	void Time::_toComponents(TimeComponents& output, sl_int64 t, sl_bool flagUTC) noexcept
	{
		if (_toPlatformComponents(output, t, flagUTC)) {
			return;
		}

		sl_int32 days = (sl_int32)(t / 86400);
		sl_int32 rem = (sl_int32)(t % 86400);
		if (rem < 0) {
			rem += 86400;
			days--;
		}

		sl_int32 year, month, day, weekday;
		GetDateFromDays(year, month, day, weekday, days);

		if (!flagUTC) {
			t += Time::getLocalTimeOffset(year, month, day);
			days = (sl_int32)(t / 86400);
			rem = (sl_int32)(t % 86400);
			if (rem < 0) {
				rem += 86400;
				days--;
			}
			GetDateFromDays(year, month, day, weekday, days);
		}

		output.year = year;
		output.month = (sl_uint8)month;
		output.day = (sl_uint8)day;
		output.dayOfWeek = (sl_uint8)weekday;
		output.hour = (sl_uint8)(rem / 3600);
		rem %= 3600;
		output.minute = (sl_uint8)(rem / 60);
		output.second = (sl_uint8)(rem % 60);
	}

	void Time::get(TimeComponents& output, const TimeZone& zone) const noexcept
	{
		sl_int64 time = ToMicroseconds(m_time);
		sl_int64 t = time / TIME_SECOND;
		sl_int64 r = time % TIME_SECOND;
		if (r < 0) {
			t -= 1;
			r += TIME_SECOND;
		}
		if (zone.isNull()) {
			_toComponents(output, t, sl_false);
		} else if (zone.isUTC()) {
			_toComponents(output, t, sl_true);
		} else {
			sl_int64 offset = zone.getOffset(*this);
			_toComponents(output, t + offset, sl_true);
		}
		output.milliseconds = (sl_uint16)(r / TIME_MILLIS);
		output.microseconds = (sl_uint16)(r % TIME_MILLIS);
	}

	void Time::getUTC(TimeComponents& output) const noexcept
	{
		get(output, TimeZone::UTC());
	}

	Time& Time::set(const TimeComponents& comps, const TimeZone& zone) noexcept
	{
		set(comps.year, comps.month, comps.day, comps.hour, comps.minute, comps.second, comps.milliseconds, comps.microseconds, zone);
		return *this;
	}

	Time& Time::setUTC(const TimeComponents& comps) noexcept
	{
		set(comps, TimeZone::UTC());
		return *this;
	}

	sl_int64 Time::_toSeconds(sl_int32 year, sl_int32 month, sl_int32 day, sl_bool flagUTC) noexcept
	{
		sl_int64 t = 0;
		if (_toPlatformSeconds(t, year, month, day, flagUTC)) {
			return t;
		}
		if (flagUTC) {
			return (sl_int64)(GetDaysFromEpoch(year, month, day)) * 86400;
		} else {
			sl_int32 days = GetDaysFromEpoch(year, month, day);
			sl_int32 weekday;
			GetDateFromDays(year, month, day, weekday, days);
			return (sl_int64)days * 86400 - Time::getLocalTimeOffset(year, month, day);
		}
	}

	Time& Time::set(sl_int32 year, sl_int32 month, sl_int32 day, sl_int32 hour, sl_int32 minute, sl_int32 second, sl_int32 milliseconds, sl_int32 microseconds, const TimeZone& zone) noexcept
	{
		sl_int64 s = hour * TIME_HOUR + minute * TIME_MINUTE + second * TIME_SECOND + milliseconds * TIME_MILLIS + microseconds;
		sl_int32 d = (sl_int32)(s / TIME_DAY);
		s %= TIME_DAY;
		if (s < 0) {
			s += TIME_DAY;
			d--;
		}
		day += d;
		sl_int64 t;
		if (zone.isNull()) {
			t = _toSeconds(year, month, day, sl_false);
		} else if (zone.isUTC()) {
			t = _toSeconds(year, month, day, sl_true);
		} else {
			t = _toSeconds(year, month, day, sl_true);
			t -= zone.getOffset(t);
		}
		m_time = ToTimeValue(t * TIME_SECOND + s);
		return *this;
	}

	Time& Time::setUTC(sl_int32 year, sl_int32 month, sl_int32 day, sl_int32 hour, sl_int32 minute, sl_int32 second, sl_int32 milliseconds, sl_int32 microseconds) noexcept
	{
		set(year, month, day, hour, minute, second, milliseconds, microseconds, TimeZone::UTC());
		return *this;
	}

	Time& Time::setDate(sl_int32 year, sl_int32 month, sl_int32 day, const TimeZone& zone) noexcept
	{
		TimeComponents comps;
		get(comps, zone);
		comps.year = year;
		comps.month = month;
		comps.day = day;
		set(comps, zone);
		return *this;
	}

	sl_int32 Time::getYear(const TimeZone& zone) const noexcept
	{
		TimeComponents comps;
		get(comps, zone);
		return comps.year;
	}

	Time& Time::setYear(sl_int32 year, const TimeZone& zone) noexcept
	{
		TimeComponents comps;
		get(comps, zone);
		comps.year = year;
		set(comps, zone);
		return *this;
	}

	Time& Time::addYears(sl_int32 years, const TimeZone& zone) noexcept
	{
		TimeComponents comps;
		get(comps, zone);
		comps.year += years;
		set(comps, zone);
		return *this;
	}

	sl_bool Time::isLeapYear(const TimeZone& zone) const noexcept
	{
		return isLeapYear(getYear(zone));
	}

	sl_bool Time::isLeapYear(sl_int32 year) noexcept
	{
		if (year & 3) {
			return sl_false;
		}
		if (year % 100) {
			return sl_true;
		}
		return !(year % 400);
	}

	sl_int32 Time::getMonth(const TimeZone& zone) const noexcept
	{
		TimeComponents comps;
		get(comps, zone);
		return comps.month;
	}

	Time& Time::setMonth(sl_int32 month, const TimeZone& zone) noexcept
	{
		TimeComponents comps;
		get(comps, zone);
		sl_int32 monthNew = (month - 1) % 12;
		sl_int32 yearAdd = (month - 1) / 12;
		if (monthNew < 0) {
			monthNew += 12;
			yearAdd--;
		}
		comps.year += yearAdd;
		comps.month = monthNew + 1;
		set(comps, zone);
		return *this;
	}

	Time& Time::addMonths(sl_int32 months, const TimeZone& zone) noexcept
	{
		TimeComponents comps;
		get(comps, zone);
		months += comps.month;
		sl_int32 monthNew = (months - 1) % 12;
		sl_int32 yearAdd = (months - 1) / 12;
		if (monthNew < 0) {
			monthNew += 12;
			yearAdd--;
		}
		comps.year += yearAdd;
		comps.month = monthNew + 1;
		set(comps, zone);
		return *this;
	}

	sl_int32 Time::getDay(const TimeZone& zone) const noexcept
	{
		TimeComponents comps;
		get(comps, zone);
		return comps.day;
	}

	Time& Time::setDay(sl_int32 day, const TimeZone& zone) noexcept
	{
		m_time += (sl_int64)(day - getDay(zone))*TIME_DAY;
		return *this;
	}

	Time& Time::addDays(sl_int64 days) noexcept
	{
		m_time += days*TIME_DAY;
		return *this;
	}

	double Time::getDayF(const TimeZone& zone) const noexcept
	{
		TimeComponents comps;
		get(comps, zone);
		return comps.day + (comps.hour * TIME_HOUR + comps.minute * TIME_MINUTE + comps.second * TIME_SECOND + comps.milliseconds * TIME_MILLIS + comps.microseconds) / TIME_DAYF;
	}

	Time& Time::setDayF(double day, const TimeZone& zone) noexcept
	{
		m_time += ToMicroseconds((sl_int64)((day - getDayF(zone))*TIME_DAYF));
		return *this;
	}

	Time& Time::addDaysF(double days) noexcept
	{
		m_time += ToMicroseconds((sl_int64)(days*TIME_DAYF));
		return *this;
	}

	sl_int32 Time::getHour(const TimeZone& zone) const noexcept
	{
		TimeComponents comps;
		get(comps, zone);
		return comps.hour;
	}

	Time& Time::setHour(sl_int32 hour, const TimeZone& zone) noexcept
	{
		m_time += (sl_int64)(hour - getHour(zone))*TIME_HOUR;
		return *this;
	}

	Time& Time::addHours(sl_int64 hours) noexcept
	{
		m_time += hours*TIME_HOUR;
		return *this;
	}

	double Time::getHourF(const TimeZone& zone) const noexcept
	{
		TimeComponents comps;
		get(comps, zone);
		return comps.hour + (comps.minute * TIME_MINUTE + comps.second * TIME_SECOND + comps.milliseconds * TIME_MILLIS + comps.microseconds) / TIME_HOURF;
	}

	Time& Time::setHourF(double hour, const TimeZone& zone) noexcept
	{
		m_time += ToMicroseconds((sl_int64)((hour - getHourF(zone))*TIME_HOURF));
		return *this;
	}

	Time& Time::addHoursF(double hours) noexcept
	{
		m_time += ToMicroseconds((sl_int64)(hours*TIME_HOURF));
		return *this;
	}

	namespace {
		constexpr static sl_uint32 GetHour12(sl_uint32 hour)
		{
			return hour % 12 ? hour % 12 : 12;
		}
	}

	sl_int32 Time::getHour12(const TimeZone& zone) const noexcept
	{
		TimeComponents comps;
		get(comps, zone);
		return GetHour12(comps.hour);
	}

	sl_bool Time::isAM(const TimeZone& zone) const noexcept
	{
		TimeComponents comps;
		get(comps, zone);
		return comps.hour < 12;
	}

	sl_bool Time::isPM(const TimeZone& zone) const noexcept
	{
		TimeComponents comps;
		get(comps, zone);
		return comps.hour >= 12;
	}

	String Time::getAM_PM(const TimeZone& zone, const Locale& locale) const noexcept
	{
		return format(TimeFormat::AM_PM, zone, locale);
	}

	String Time::getAM_PM(const TimeZone& zone) const noexcept
	{
		return format(TimeFormat::AM_PM, zone);
	}

	String Time::getAM_PM(const Locale& locale) const noexcept
	{
		return format(TimeFormat::AM_PM, locale);
	}

	String Time::getAM_PM() const noexcept
	{
		return format(TimeFormat::AM_PM);
	}

	sl_int32 Time::getMinute(const TimeZone& zone) const noexcept
	{
		TimeComponents comps;
		get(comps, zone);
		return comps.minute;
	}

	Time& Time::setMinute(sl_int32 minute, const TimeZone& zone) noexcept
	{
		m_time += (sl_int64)(minute - getMinute(zone))*TIME_MINUTE;
		return *this;
	}

	Time& Time::addMinutes(sl_int64 minutes) noexcept
	{
		m_time += minutes*TIME_MINUTE;
		return *this;
	}

	double Time::getMinuteF(const TimeZone& zone) const noexcept
	{
		TimeComponents comps;
		get(comps, zone);
		return comps.minute + (comps.second * TIME_SECOND + comps.milliseconds * TIME_MILLIS + comps.microseconds) / TIME_MINUTEF;
	}

	Time& Time::setMinuteF(double minute, const TimeZone& zone) noexcept
	{
		m_time += ToMicroseconds((sl_int64)((minute - getMinuteF(zone))*TIME_MINUTEF));
		return *this;
	}

	Time& Time::addMinutesF(double minutes) noexcept
	{
		m_time += ToMicroseconds((sl_int64)(minutes*TIME_MINUTEF));
		return *this;
	}

	sl_int32 Time::getSecond(const TimeZone& zone) const noexcept
	{
		TimeComponents comps;
		get(comps, zone);
		return comps.second;
	}

	Time& Time::setSecond(sl_int32 second, const TimeZone& zone) noexcept
	{
		m_time += (sl_int64)(second - getSecond(zone))*TIME_SECOND;
		return *this;
	}

	Time& Time::addSeconds(sl_int64 seconds) noexcept
	{
		m_time += seconds*TIME_SECOND;
		return *this;
	}

	double Time::getSecondF(const TimeZone& zone) const noexcept
	{
		TimeComponents comps;
		get(comps, zone);
		return comps.second + (comps.milliseconds * TIME_MILLIS + comps.microseconds) / TIME_SECONDF;
	}

	Time& Time::setSecondF(double second, const TimeZone& zone) noexcept
	{
		m_time += ToMicroseconds((sl_int64)((second - getSecondF(zone))*TIME_SECONDF));
		return *this;
	}

	Time& Time::addSecondsF(double seconds) noexcept
	{
		m_time += ToMicroseconds((sl_int64)(seconds*TIME_SECONDF));
		return *this;
	}

	sl_int32 Time::getMillisecond() const noexcept
	{
		sl_int32 n = (sl_int32)(ToMicroseconds(m_time) % TIME_SECOND);
		if (n < 0) {
			n += TIME_SECOND;
		}
		return n / TIME_MILLIS;
	}

	Time& Time::setMillisecond(sl_int32 millis) noexcept
	{
		m_time += (sl_int64)(millis - getMillisecond())*TIME_MILLIS;
		return *this;
	}

	Time& Time::addMilliseconds(sl_int64 milis) noexcept
	{
		m_time += milis*TIME_MILLIS;
		return *this;
	}

	double Time::getMillisecondF() const noexcept
	{
		sl_int32 n = (sl_int32)(ToMicroseconds(m_time) % TIME_SECOND);
		if (n < 0) {
			n += TIME_SECOND;
		}
		return n / TIME_MILLISF;
	}

	Time& Time::setMillisecondF(double millis) noexcept
	{
		m_time += ToMicroseconds((sl_int64)((millis - getMillisecondF())*TIME_MILLIS));
		return *this;
	}

	Time& Time::addMillisecondsF(double milis) noexcept
	{
		m_time += ToMicroseconds((sl_int64)(milis*TIME_MILLISF));
		return *this;
	}

	sl_int32 Time::getMicrosecond() const noexcept
	{
		sl_int32 n = (sl_int32)(ToMicroseconds(m_time) % TIME_MILLIS);
		if (n < 0) {
			n += TIME_MILLIS;
		}
		return n;
	}

	Time& Time::setMicrosecond(sl_int64 micros) noexcept
	{
		m_time += ToMicroseconds(micros - getMicrosecond());
		return *this;
	}

	Time& Time::addMicroseconds(sl_int64 micros) noexcept
	{
		m_time += ToMicroseconds(micros);
		return *this;
	}

	double Time::getMicrosecondF() const noexcept
	{
		return (double)(getMicrosecond());
	}

	Time& Time::setMicrosecondF(double micros) noexcept
	{
		setMicrosecond((sl_int64)micros);
		return *this;
	}

	Time& Time::addMicrosecondsF(double micros) noexcept
	{
		m_time += ToMicroseconds((sl_int64)micros);
		return *this;
	}

	sl_int32 Time::getDayOfWeek(const TimeZone& zone) const noexcept
	{
		TimeComponents comps;
		get(comps, zone);
		return comps.dayOfWeek;
	}

	Time& Time::setDayOfWeek(sl_int32 day, const TimeZone& zone) noexcept
	{
		m_time += (sl_int64)(day - getDayOfWeek(zone))*TIME_DAY;
		return *this;
	}

	sl_int32 Time::getDayOfYear(const TimeZone& zone) const noexcept
	{
		return (sl_int32)((m_time - Time(getYear(zone), 1, 1, zone).m_time) / TIME_DAY) + 1;
	}

	Time& Time::setDayOfYear(sl_int32 day, const TimeZone& zone) noexcept
	{
		m_time += (sl_int64)(day - getDayOfYear(zone))*TIME_DAY;
		return *this;
	}

	sl_int64 Time::getDayCount() const noexcept
	{
		return m_time / TIME_DAY;
	}

	Time& Time::setDayCount(sl_int64 days) noexcept
	{
		m_time = days * TIME_DAY;
		return *this;
	}

	double Time::getDayCountF() const noexcept
	{
		return ToMicroseconds(m_time) / TIME_DAYF;
	}

	Time& Time::setDayCountF(double days) noexcept
	{
		m_time = ToMicroseconds((sl_int64)(days * TIME_DAYF));
		return *this;
	}

	sl_int64 Time::getHourCount() const noexcept
	{
		return m_time / TIME_HOUR;
	}

	Time& Time::setHourCount(sl_int64 hours) noexcept
	{
		m_time = hours * TIME_HOUR;
		return *this;
	}

	double Time::getHourCountF() const noexcept
	{
		return ToMicroseconds(m_time) / TIME_HOURF;
	}

	Time& Time::setHourCountF(double hours) noexcept
	{
		m_time = ToMicroseconds((sl_int64)(hours * TIME_HOURF));
		return *this;
	}

	sl_int64 Time::getMinuteCount() const noexcept
	{
		return m_time / TIME_MINUTE;
	}

	Time& Time::setMinuteCount(sl_int64 minutes) noexcept
	{
		m_time = minutes * TIME_MINUTE;
		return *this;
	}

	double Time::getMinuteCountF() const noexcept
	{
		return ToMicroseconds(m_time) / TIME_MINUTEF;
	}

	Time& Time::setMinuteCountF(double minutes) noexcept
	{
		m_time = ToMicroseconds((sl_int64)(minutes * TIME_MINUTEF));
		return *this;
	}

	sl_int64 Time::getSecondCount() const noexcept
	{
		return m_time / TIME_SECOND;
	}

	Time& Time::setSecondCount(sl_int64 seconds) noexcept
	{
		m_time = seconds * TIME_SECOND;
		return *this;
	}

	double Time::getSecondCountF() const noexcept
	{
		return ToMicroseconds(m_time) / TIME_SECONDF;
	}

	Time& Time::setSecondCountF(double seconds) noexcept
	{
		m_time = ToMicroseconds((sl_int64)(seconds * TIME_SECONDF));
		return *this;
	}

	sl_int64 Time::getMillisecondCount() const noexcept
	{
		return m_time / TIME_MILLIS;
	}

	Time& Time::setMillisecondCount(sl_int64 millis) noexcept
	{
		m_time = millis * TIME_MILLIS;
		return *this;
	}

	double Time::getMillisecondCountF() const noexcept
	{
		return ToMicroseconds(m_time) / TIME_MILLISF;
	}

	Time& Time::setMillisecondCountF(double millis) noexcept
	{
		m_time = ToMicroseconds((sl_int64)(millis*TIME_MILLISF));
		return *this;
	}

	sl_int64 Time::getMicrosecondCount() const noexcept
	{
		return ToMicroseconds(m_time);
	}

	Time& Time::setMicrosecondCount(sl_int64 micros) noexcept
	{
		m_time = ToMicroseconds(micros);
		return *this;
	}

	double Time::getMicrosecondCountF() const noexcept
	{
		return (double)(ToMicroseconds(m_time));
	}

	Time& Time::setMicrosecondCountF(double micros) noexcept
	{
		m_time = ToMicroseconds((sl_int64)micros);
		return *this;
	}

	sl_int64 Time::getLocalTimeOffset() const noexcept
	{
		return getLocalTimeOffset(getYear(), 1, 1);
	}

	sl_int32 Time::getDayCountInMonth(const TimeZone& zone) const noexcept
	{
		TimeComponents comps;
		get(comps, zone);
		Time timeStart(comps.year, comps.month, 1, zone);
		Time timeEnd = timeStart;
		timeEnd.addMonths(1, zone);
		return (sl_int32)((timeEnd.m_time - timeStart.m_time) / TIME_DAY);
	}

	sl_int32 Time::getDayCountInYear(const TimeZone& zone) const noexcept
	{
		TimeComponents comps;
		get(comps, zone);
		Time timeStart(comps.year, 1, 1, zone);
		Time timeEnd(comps.year + 1, 1, 1, zone);
		return (sl_int32)((timeEnd.m_time - timeStart.m_time) / TIME_DAY);
	}

	sl_int32 Time::getQuarter(const TimeZone& zone) const noexcept
	{
		return ((getMonth(zone) - 1) / 3) + 1;
	}

	Time Time::getDateOnly(const TimeZone& zone) const noexcept
	{
		return *this - getTimeOnly(zone);
	}

	Time Time::getTimeOnly(const TimeZone& zone) const noexcept
	{
		TimeComponents comps;
		get(comps, zone);
		return comps.hour * TIME_HOUR + comps.minute * TIME_MINUTE + comps.second * TIME_SECOND + comps.milliseconds * TIME_MILLIS + comps.microseconds;
	}

	String Time::getWeekdayText(sl_int32 weekday, TimeTextType type, const Locale& _locale) noexcept
	{
		Locale locale = _locale;
		if (locale == Locale::Unknown) {
			locale = Locale::getCurrent();
		}
		switch (locale.getLanguage()) {
			case Language::Korean:
				switch (type) {
					case TimeTextType::Long:
						switch (weekday) {
							case 0: SLIB_RETURN_STRING("\xec\x9d\xbc\xec\x9a\x94\xec\x9d\xbc");
							case 1: SLIB_RETURN_STRING("\xec\x9b\x94\xec\x9a\x94\xec\x9d\xbc");
							case 2: SLIB_RETURN_STRING("\xed\x99\x94\xec\x9a\x94\xec\x9d\xbc");
							case 3: SLIB_RETURN_STRING("\xec\x88\x98\xec\x9a\x94\xec\x9d\xbc");
							case 4: SLIB_RETURN_STRING("\xeb\xaa\xa9\xec\x9a\x94\xec\x9d\xbc");
							case 5: SLIB_RETURN_STRING("\xea\xb8\x88\xec\x9a\x94\xec\x9d\xbc");
							case 6: SLIB_RETURN_STRING("\xed\x86\xa0\xec\x9a\x94\xec\x9d\xbc");
						}
						return sl_null;
					case TimeTextType::Short:
					case TimeTextType::SingleChar:
						switch (weekday) {
							case 0: SLIB_RETURN_STRING("\xec\x9d\xbc");
							case 1: SLIB_RETURN_STRING("\xec\x9b\x94");
							case 2: SLIB_RETURN_STRING("\xed\x99\x94");
							case 3: SLIB_RETURN_STRING("\xec\x88\x98");
							case 4: SLIB_RETURN_STRING("\xeb\xaa\xa9");
							case 5: SLIB_RETURN_STRING("\xea\xb8\x88");
							case 6: SLIB_RETURN_STRING("\xed\x86\xa0");
						}
						return sl_null;
				}
				return sl_null;
			case Language::Chinese:
				switch (type) {
					case TimeTextType::Long:
						switch (weekday) {
							case 0: SLIB_RETURN_STRING("\xe6\x98\x9f\xe6\x9c\x9f\xe6\x97\xa5");
							case 1: SLIB_RETURN_STRING("\xe6\x98\x9f\xe6\x9c\x9f\xe4\xb8\x80");
							case 2: SLIB_RETURN_STRING("\xe6\x98\x9f\xe6\x9c\x9f\xe4\xba\x8c");
							case 3: SLIB_RETURN_STRING("\xe6\x98\x9f\xe6\x9c\x9f\xe4\xb8\x89");
							case 4: SLIB_RETURN_STRING("\xe6\x98\x9f\xe6\x9c\x9f\xe5\x9b\x9b");
							case 5: SLIB_RETURN_STRING("\xe6\x98\x9f\xe6\x9c\x9f\xe4\xba\x94");
							case 6: SLIB_RETURN_STRING("\xe6\x98\x9f\xe6\x9c\x9f\xe5\x85\xad");
						}
						return sl_null;
					case TimeTextType::Short:
						if (locale.getScript() == LanguageScript::Hant) {
							switch (weekday) {
								case 0: SLIB_RETURN_STRING("\xe9\x80\xb1\xe6\x97\xa5");
								case 1: SLIB_RETURN_STRING("\xe9\x80\xb1\xe4\xb8\x80");
								case 2: SLIB_RETURN_STRING("\xe9\x80\xb1\xe4\xba\x8c");
								case 3: SLIB_RETURN_STRING("\xe9\x80\xb1\xe4\xb8\x89");
								case 4: SLIB_RETURN_STRING("\xe9\x80\xb1\xe5\x9b\x9b");
								case 5: SLIB_RETURN_STRING("\xe9\x80\xb1\xe4\xba\x94");
								case 6: SLIB_RETURN_STRING("\xe9\x80\xb1\xe5\x85\xad");
							}
						} else {
							switch (weekday) {
								case 0: SLIB_RETURN_STRING("\xe5\x91\xa8\xe6\x97\xa5");
								case 1: SLIB_RETURN_STRING("\xe5\x91\xa8\xe4\xb8\x80");
								case 2: SLIB_RETURN_STRING("\xe5\x91\xa8\xe4\xba\x8c");
								case 3: SLIB_RETURN_STRING("\xe5\x91\xa8\xe4\xb8\x89");
								case 4: SLIB_RETURN_STRING("\xe5\x91\xa8\xe5\x9b\x9b");
								case 5: SLIB_RETURN_STRING("\xe5\x91\xa8\xe4\xba\x94");
								case 6: SLIB_RETURN_STRING("\xe5\x91\xa8\xe5\x85\xad");
							}
						}
						return sl_null;
					case TimeTextType::SingleChar:
						switch (weekday) {
							case 0: SLIB_RETURN_STRING("\xe6\x97\xa5");
							case 1: SLIB_RETURN_STRING("\xe4\xb8\x80");
							case 2: SLIB_RETURN_STRING("\xe4\xba\x8c");
							case 3: SLIB_RETURN_STRING("\xe4\xb8\x89");
							case 4: SLIB_RETURN_STRING("\xe5\x9b\x9b");
							case 5: SLIB_RETURN_STRING("\xe4\xba\x94");
							case 6: SLIB_RETURN_STRING("\xe5\x85\xad");
						}
						return sl_null;
				}
				return sl_null;
			case Language::Japanese:
				switch (type) {
					case TimeTextType::Long:
						switch (weekday) {
							case 0: SLIB_RETURN_STRING("\xe6\x97\xa5\xe6\x9b\x9c\xe6\x97\xa5");
							case 1: SLIB_RETURN_STRING("\xe6\x9c\x88\xe6\x9b\x9c\xe6\x97\xa5");
							case 2: SLIB_RETURN_STRING("\xe7\x81\xab\xe6\x9b\x9c\xe6\x97\xa5");
							case 3: SLIB_RETURN_STRING("\xe6\xb0\xb4\xe6\x9b\x9c\xe6\x97\xa5");
							case 4: SLIB_RETURN_STRING("\xe6\x9c\xa8\xe6\x9b\x9c\xe6\x97\xa5");
							case 5: SLIB_RETURN_STRING("\xe9\x87\x91\xe6\x9b\x9c\xe6\x97\xa5");
							case 6: SLIB_RETURN_STRING("\xe5\x9c\x9f\xe6\x9b\x9c\xe6\x97\xa5");
						}
						return sl_null;
					case TimeTextType::Short:
					case TimeTextType::SingleChar:
						switch (weekday) {
							case 0: SLIB_RETURN_STRING("\xe6\x97\xa5");
							case 1: SLIB_RETURN_STRING("\xe6\x9c\x88");
							case 2: SLIB_RETURN_STRING("\xe7\x81\xab");
							case 3: SLIB_RETURN_STRING("\xe6\xb0\xb4");
							case 4: SLIB_RETURN_STRING("\xe6\x9c\xa8");
							case 5: SLIB_RETURN_STRING("\xe9\x87\x91");
							case 6: SLIB_RETURN_STRING("\xe5\x9c\x9f");
						}
						return sl_null;
				}
				return sl_null;
			default:
				switch (type) {
					case TimeTextType::Long:
						switch (weekday) {
							case 0: SLIB_RETURN_STRING("Sunday");
							case 1: SLIB_RETURN_STRING("Monday");
							case 2: SLIB_RETURN_STRING("Tuesday");
							case 3: SLIB_RETURN_STRING("Wednesday");
							case 4: SLIB_RETURN_STRING("Thursday");
							case 5: SLIB_RETURN_STRING("Friday");
							case 6: SLIB_RETURN_STRING("Saturday");
						}
						return sl_null;
					case TimeTextType::Short:
						switch (weekday) {
							case 0: SLIB_RETURN_STRING("Sun");
							case 1: SLIB_RETURN_STRING("Mon");
							case 2: SLIB_RETURN_STRING("Tue");
							case 3: SLIB_RETURN_STRING("Wed");
							case 4: SLIB_RETURN_STRING("Thu");
							case 5: SLIB_RETURN_STRING("Fri");
							case 6: SLIB_RETURN_STRING("Sat");
						}
						return sl_null;
					case TimeTextType::SingleChar:
						switch (weekday) {
							case 0: SLIB_RETURN_STRING("S");
							case 1: SLIB_RETURN_STRING("M");
							case 2: SLIB_RETURN_STRING("T");
							case 3: SLIB_RETURN_STRING("W");
							case 4: SLIB_RETURN_STRING("T");
							case 5: SLIB_RETURN_STRING("F");
							case 6: SLIB_RETURN_STRING("S");
						}
						return sl_null;
				}
				return sl_null;
		}
	}

	String Time::getWeekdayShort(const TimeZone& zone, const Locale& locale) const noexcept
	{
		sl_int32 day = getDayOfWeek(zone);
		return getWeekdayText(day, TimeTextType::Short, locale);
	}

	String Time::getWeekdayShort(const TimeZone& zone) const noexcept
	{
		sl_int32 day = getDayOfWeek(zone);
		return getWeekdayText(day, TimeTextType::Short, Locale::Unknown);
	}

	String Time::getWeekdayShort(const Locale& locale) const noexcept
	{
		sl_int32 day = getDayOfWeek();
		return getWeekdayText(day, TimeTextType::Short, locale);
	}

	String Time::getWeekdayShort() const noexcept
	{
		sl_int32 day = getDayOfWeek();
		return getWeekdayText(day, TimeTextType::Short, Locale::Unknown);
	}

	String Time::getWeekdayLong(const TimeZone& zone, const Locale& locale) const noexcept
	{
		sl_int32 day = getDayOfWeek(zone);
		return getWeekdayText(day, TimeTextType::Long, locale);
	}

	String Time::getWeekdayLong(const TimeZone& zone) const noexcept
	{
		sl_int32 day = getDayOfWeek(zone);
		return getWeekdayText(day, TimeTextType::Long, Locale::Unknown);
	}

	String Time::getWeekdayLong(const Locale& locale) const noexcept
	{
		sl_int32 day = getDayOfWeek();
		return getWeekdayText(day, TimeTextType::Long, locale);
	}

	String Time::getWeekdayLong() const noexcept
	{
		sl_int32 day = getDayOfWeek();
		return getWeekdayText(day, TimeTextType::Long, Locale::Unknown);
	}

	String Time::getWeekday(const TimeZone& zone, const Locale& locale) const noexcept
	{
		return getWeekdayLong(zone, locale);
	}

	String Time::getWeekday(const TimeZone& zone) const noexcept
	{
		return getWeekdayLong(zone);
	}

	String Time::getWeekday(const Locale& locale) const noexcept
	{
		return getWeekdayLong(locale);
	}

	String Time::getWeekday() const noexcept
	{
		return getWeekdayLong();
	}

	String Time::getAM_Text(const Locale& _locale) noexcept
	{
		Locale locale = _locale;
		if (locale == Locale::Unknown) {
			locale = Locale::getCurrent();
		}
		switch (locale.getLanguage()) {
			case Language::Korean:
				SLIB_RETURN_STRING("\xEC\x98\xA4\xEC\xA0\x84");
			case Language::Chinese:
				SLIB_RETURN_STRING("\xE4\xB8\x8A\xE5\x8D\x88");
			case Language::Japanese:
				SLIB_RETURN_STRING("\xE5\x8D\x88\xE5\x89\x8D");
			default:
				SLIB_RETURN_STRING("AM");
		}
	}

	String Time::getPM_Text(const Locale& _locale) noexcept
	{
		Locale locale = _locale;
		if (locale == Locale::Unknown) {
			locale = Locale::getCurrent();
		}
		switch (locale.getLanguage()) {
			case Language::Korean:
				SLIB_RETURN_STRING("\xEC\x98\xA4\xED\x9B\x84");
			case Language::Chinese:
				SLIB_RETURN_STRING("\xE4\xB8\x8B\xE5\x8D\x88");
			case Language::Japanese:
				SLIB_RETURN_STRING("\xE5\x8D\x88\xE5\xBE\x8C");
			default:
				SLIB_RETURN_STRING("PM");
		}
	}

	String Time::getMonthText(sl_int32 month, TimeTextType type) noexcept
	{
		switch (type) {
			case TimeTextType::Long:
				switch (month) {
					case 1: SLIB_RETURN_STRING("January");
					case 2: SLIB_RETURN_STRING("February");
					case 3: SLIB_RETURN_STRING("March");
					case 4: SLIB_RETURN_STRING("April");
					case 5: SLIB_RETURN_STRING("May");
					case 6: SLIB_RETURN_STRING("June");
					case 7: SLIB_RETURN_STRING("July");
					case 8: SLIB_RETURN_STRING("August");
					case 9: SLIB_RETURN_STRING("September");
					case 10: SLIB_RETURN_STRING("October");
					case 11: SLIB_RETURN_STRING("November");
					case 12: SLIB_RETURN_STRING("December");
				}
				return sl_null;
			case TimeTextType::Short:
			case TimeTextType::SingleChar:
				switch (month) {
					case 1: SLIB_RETURN_STRING("Jan");
					case 2: SLIB_RETURN_STRING("Feb");
					case 3: SLIB_RETURN_STRING("Mar");
					case 4: SLIB_RETURN_STRING("Apr");
					case 5: SLIB_RETURN_STRING("May");
					case 6: SLIB_RETURN_STRING("Jun");
					case 7: SLIB_RETURN_STRING("Jul");
					case 8: SLIB_RETURN_STRING("Aug");
					case 9: SLIB_RETURN_STRING("Sep");
					case 10: SLIB_RETURN_STRING("Oct");
					case 11: SLIB_RETURN_STRING("Nov");
					case 12: SLIB_RETURN_STRING("Dec");
				}
				return sl_null;
		}
		return sl_null;
	}

	String Time::getMonthShort(const TimeZone& zone) const noexcept
	{
		sl_int32 month = getMonth(zone);
		return getMonthText(month, TimeTextType::Short);
	}

	String Time::getMonthLong(const TimeZone& zone) const noexcept
	{
		sl_int32 month = getMonth(zone);
		return getMonthText(month, TimeTextType::Long);
	}

	String Time::toHttpDate() const noexcept
	{
		TimeComponents d;
		getUTC(d);
		return String::format("%s, %02d %s %04d %02d:%02d:%02d GMT", getWeekdayText(d.dayOfWeek, TimeTextType::Short, Locale::en), d.day, getMonthText(d.month, TimeTextType::Short), d.year, d.hour, d.minute, d.second);
	}

	sl_reg Time::parseHttpDate(Time* _output, const sl_char8* buf, sl_size posBegin, sl_size posEnd) noexcept
	{
		static char const* weekdays[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
		static char const* months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
		if (posBegin + 29 > posEnd) {
			return SLIB_PARSE_ERROR;
		}
		sl_int32 i;
		const sl_char8* p = buf + posBegin;
		for (i = 0; i < 7; i++) {
			if (Base::equalsMemory(p, weekdays[i], 3)) {
				break;
			}
		}
		if (i == 7) {
			return SLIB_PARSE_ERROR;
		}
		if (p[3] != ',') {
			return SLIB_PARSE_ERROR;
		}
		if (p[4] != ' ') {
			return SLIB_PARSE_ERROR;
		}
		if (!SLIB_CHAR_IS_DIGIT(p[5])) {
			return SLIB_PARSE_ERROR;
		}
		if (!SLIB_CHAR_IS_DIGIT(p[6])) {
			return SLIB_PARSE_ERROR;
		}
		if (p[7] != ' ') {
			return SLIB_PARSE_ERROR;
		}
		for (i = 0; i < 12; i++) {
			if (Base::equalsMemory(p + 8, months[i], 3)) {
				break;
			}
		}
		if (i == 12) {
			return SLIB_PARSE_ERROR;
		}
		sl_int32 month = i + 1;
		if (p[11] != ' ') {
			return SLIB_PARSE_ERROR;
		}
		if (!SLIB_CHAR_IS_DIGIT(p[12])) {
			return SLIB_PARSE_ERROR;
		}
		if (!SLIB_CHAR_IS_DIGIT(p[13])) {
			return SLIB_PARSE_ERROR;
		}
		if (!SLIB_CHAR_IS_DIGIT(p[14])) {
			return SLIB_PARSE_ERROR;
		}
		if (!SLIB_CHAR_IS_DIGIT(p[15])) {
			return SLIB_PARSE_ERROR;
		}
		if (p[16] != ' ') {
			return SLIB_PARSE_ERROR;
		}
		if (!SLIB_CHAR_IS_DIGIT(p[17])) {
			return SLIB_PARSE_ERROR;
		}
		if (!SLIB_CHAR_IS_DIGIT(p[18])) {
			return SLIB_PARSE_ERROR;
		}
		if (p[19] != ':') {
			return SLIB_PARSE_ERROR;
		}
		if (!SLIB_CHAR_IS_DIGIT(p[20])) {
			return SLIB_PARSE_ERROR;
		}
		if (!SLIB_CHAR_IS_DIGIT(p[21])) {
			return SLIB_PARSE_ERROR;
		}
		if (p[22] != ':') {
			return SLIB_PARSE_ERROR;
		}
		if (!SLIB_CHAR_IS_DIGIT(p[23])) {
			return SLIB_PARSE_ERROR;
		}
		if (!SLIB_CHAR_IS_DIGIT(p[24])) {
			return SLIB_PARSE_ERROR;
		}
		if (p[25] != ' ') {
			return SLIB_PARSE_ERROR;
		}
		if (p[26] != 'G') {
			return SLIB_PARSE_ERROR;
		}
		if (p[27] != 'M') {
			return SLIB_PARSE_ERROR;
		}
		if (p[28] != 'T') {
			return SLIB_PARSE_ERROR;
		}
		if (_output) {
			sl_int32 year = ((sl_int32)(p[12]-'0')) * 1000 + ((sl_int32)(p[13] - '0')) * 100 + ((sl_int32)(p[14] - '0')) * 10 + ((sl_int32)(p[15] - '0'));
			sl_int32 day = ((sl_int32)(p[5]-'0')) * 10 + ((sl_int32)(p[6] - '0'));
			sl_int32 hour = ((sl_int32)(p[17]-'0')) * 10 + ((sl_int32)(p[18] - '0'));
			sl_int32 minute = ((sl_int32)(p[20]-'0')) * 10 + ((sl_int32)(p[21] - '0'));
			sl_int32 second = ((sl_int32)(p[23]-'0')) * 10 + ((sl_int32)(p[24] - '0'));
			_output->setUTC(year, month, day, hour, minute, second);
		}
		return posBegin + 29;
	}

	sl_bool Time::parseHttpDate(const StringParam& _date) noexcept
	{
		StringData date(_date);
		sl_size len = date.getLength();
		if (len != 29) {
			return sl_false;
		}
		return parseHttpDate(this, date.getData(), 0, len) == len;
	}

	String Time::toString(const TimeZone& zone) const noexcept
	{
		if (isZero()) {
			return sl_null;
		}
		TimeComponents d;
		get(d, zone);
		char s[19];
		s[0] = (char)('0' + ((d.year / 1000) % 10));
		s[1] = (char)('0' + ((d.year / 100) % 10));
		s[2] = (char)('0' + ((d.year / 10) % 10));
		s[3] = (char)('0' + (d.year % 10));
		s[4] = '-';
		s[5] = (char)('0' + ((d.month / 10) % 10));
		s[6] = (char)('0' + (d.month % 10));
		s[7] = '-';
		s[8] = (char)('0' + ((d.day / 10) % 10));
		s[9] = (char)('0' + (d.day % 10));
		s[10] = ' ';
		s[11] = (char)('0' + ((d.hour / 10) % 10));
		s[12] = (char)('0' + (d.hour % 10));
		s[13] = ':';
		s[14] = (char)('0' + ((d.minute / 10) % 10));
		s[15] = (char)('0' + (d.minute % 10));
		s[16] = ':';
		s[17] = (char)('0' + ((d.second / 10) % 10));
		s[18] = (char)('0' + (d.second % 10));
		return String(s, 19);
	}

	String Time::toISOString() const noexcept
	{
		if (isZero()) {
			return sl_null;
		}
		TimeComponents d;
		get(d, TimeZone::UTC());
		char s[24];
		s[0] = (char)('0' + ((d.year / 1000) % 10));
		s[1] = (char)('0' + ((d.year / 100) % 10));
		s[2] = (char)('0' + ((d.year / 10) % 10));
		s[3] = (char)('0' + (d.year % 10));
		s[4] = '-';
		s[5] = (char)('0' + ((d.month / 10) % 10));
		s[6] = (char)('0' + (d.month % 10));
		s[7] = '-';
		s[8] = (char)('0' + ((d.day / 10) % 10));
		s[9] = (char)('0' + (d.day % 10));
		s[10] = 'T';
		s[11] = (char)('0' + ((d.hour / 10) % 10));
		s[12] = (char)('0' + (d.hour % 10));
		s[13] = ':';
		s[14] = (char)('0' + ((d.minute / 10) % 10));
		s[15] = (char)('0' + (d.minute % 10));
		s[16] = ':';
		s[17] = (char)('0' + ((d.second / 10) % 10));
		s[18] = (char)('0' + (d.second % 10));
		s[19] = '.';
		s[20] = (char)('0' + ((d.milliseconds / 100) % 10));
		s[21] = (char)('0' + ((d.milliseconds / 10) % 10));
		s[22] = (char)('0' + (d.milliseconds % 10));
		s[23] = 'Z';
		return String(s, 24);
	}

	String Time::getDateString(const TimeZone& zone) const noexcept
	{
		return getDateString('-', zone);
	}

	String Time::getDateString(sl_char8 delimiter, const TimeZone& zone) const noexcept
	{
		if (isZero()) {
			return sl_null;
		}
		TimeComponents d;
		get(d, zone);
		char s[10];
		s[0] = (char)('0' + ((d.year / 1000) % 10));
		s[1] = (char)('0' + ((d.year / 100) % 10));
		s[2] = (char)('0' + ((d.year / 10) % 10));
		s[3] = (char)('0' + (d.year % 10));
		s[4] = '-';
		s[5] = (char)('0' + ((d.month / 10) % 10));
		s[6] = (char)('0' + (d.month % 10));
		s[7] = '-';
		s[8] = (char)('0' + ((d.day / 10) % 10));
		s[9] = (char)('0' + (d.day % 10));
		return String(s, 10);
	}

	String Time::getTimeString(const TimeZone& zone) const noexcept
	{
		TimeComponents d;
		get(d, zone);
		char s[8];
		s[0] = (char)('0' + ((d.hour / 10) % 10));
		s[1] = (char)('0' + (d.hour % 10));
		s[2] = ':';
		s[3] = (char)('0' + ((d.minute / 10) % 10));
		s[4] = (char)('0' + (d.minute % 10));
		s[5] = ':';
		s[6] = (char)('0' + ((d.second / 10) % 10));
		s[7] = (char)('0' + (d.second % 10));
		return String(s, 8);
	}

	String Time::getPeriodString(const Time& minUnit, const Time& maxUnit, const Locale& _locale) const noexcept
	{
		Locale locale = _locale;
		if (locale == Locale::Unknown) {
			locale = Locale::getCurrent();
		}
		sl_int64 n = m_time;
		if (n < 0) {
			n = -n;
		}
		if (n < minUnit.m_time) {
			return sl_null;
		}
		sl_int64 max = maxUnit.m_time;
		if (max <= 0) {
			max = TIME_DAY * 1000;
		}
		Language lang = locale.getLanguage();
		if (n < TIME_SECOND || max < TIME_SECOND) {
			sl_int64 t = getMillisecondCount();
			return String::fromInt64(t) + "ms";
		} else if (n < TIME_MINUTE || max <= TIME_SECOND) {
			sl_int64 t = getSecondCount();
			if (lang == Language::Korean) {
				return String::fromInt64(t) + "\xEC\xB4\x88";
			} else {
				if (t == 1) {
					SLIB_RETURN_STRING("1 second");
				} else {
					return String::fromInt64(t) + " seconds";
				}
			}
		} else if (n < TIME_HOUR || max <= TIME_MINUTE) {
			sl_int64 t = getMinuteCount();
			if (lang == Language::Korean) {
				return String::fromInt64(t) + "\xEB\xB6\x84";
			} else {
				if (t == 1) {
					SLIB_RETURN_STRING("1 minute");
				} else {
					return String::fromInt64(t) + " minutes";
				}
			}
		} else if (n < TIME_DAY || max <= TIME_HOUR) {
			sl_int64 t = getHourCount();
			if (lang == Language::Korean) {
				return String::fromInt64(t) + "\xEC\x8B\x9C\xEA\xB0\x84";
			} else {
				if (t == 1) {
					SLIB_RETURN_STRING("1 hour");
				} else {
					return String::fromInt64(t) + " hours";
				}
			}
		} else if (n < TIME_DAY*32 || max <= TIME_DAY) {
			sl_int64 t = getDayCount();
			if (lang == Language::Korean) {
				return String::fromInt64(t) + "\xEC\x9D\xBC";
			} else {
				if (t == 1) {
					SLIB_RETURN_STRING("1 day");
				} else {
					return String::fromInt64(t) + " days";
				}
			}
		} else if (n < TIME_DAY*366 || max < TIME_DAY*32) {
			sl_int64 t = (sl_int64)(getDayCountF() / 30.5);
			if (lang == Language::Korean) {
				return String::fromInt64(t) + "\xEA\xB0\x9C\xEB\x8B\xAC";
			} else {
				if (t == 1) {
					SLIB_RETURN_STRING("1 month");
				} else {
					return String::fromInt64(t) + " months";
				}
			}
		} else {
			sl_int64 t = (sl_int64)(getDayCountF() / 365.25);
			if (lang == Language::Korean) {
				return String::fromInt64(t) + "\xEB\x85\x84";
			} else {
				if (t == 1) {
					SLIB_RETURN_STRING("1 year");
				} else {
					return String::fromInt64(t) + " years";
				}
			}
		}
	}

	String Time::getPeriodString(const Time& minUnit, const Time& maxUnit) const noexcept
	{
		return getPeriodString(minUnit, maxUnit, Locale::Unknown);
	}

	String Time::getDiffString(const Time& timeFrom, const Time& minUnit, const Time& maxUnit, const Locale& _locale) const noexcept
	{
		Locale locale = _locale;
		if (locale == Locale::Unknown) {
			locale = Locale::getCurrent();
		}
		Time diff = *this - timeFrom;
		sl_bool flagAgo;
		if (diff.m_time > 0) {
			flagAgo = sl_false;
		} else {
			flagAgo = sl_true;
			diff.m_time = - (diff.m_time);
		}
		if (diff < minUnit) {
			return sl_null;
		}
		String s = diff.getPeriodString(minUnit, maxUnit, locale);
		Language lang = locale.getLanguage();
		if (lang == Language::Korean) {
			if (locale.getCountry() == Country::DPRK) {
				if (flagAgo) {
					return s + "\xEC\xA0\x84";
				} else {
					return s + "\xED\x9B\x84";
				}
			} else {
				if (flagAgo) {
					return s + " \xEC\xA0\x84";
				} else {
					return s + " \xED\x9B\x84";
				}
			}
		} else {
			if (flagAgo) {
				return s + " ago";
			} else {
				return s + " later";
			}
		}
	}

	String Time::getDiffString(const Time& timeFrom, const Time& minUnit, const Time& maxUnit) const noexcept
	{
		return getDiffString(timeFrom, minUnit, maxUnit, Locale::Unknown);
	}

	namespace {
		static sl_bool IsMDY(const Locale& locale) {
			// country list from https://en.wikipedia.org/wiki/Date_format_by_country
			Country country = locale.getCountry();
			switch (country) {
				case Country::AmericanSamoa:
				case Country::CaymanIslands:
				case Country::Micronesia:
				case Country::Ghana:
				case Country::Greenland:
				case Country::Guam:
				case Country::Kenya:
				case Country::Malaysia:
				case Country::MarshallIslands:
				case Country::NorthernMarianaIslands:
				case Country::Panama:
				case Country::Philippines:
				case Country::Somalia:
				case Country::SouthAfrica:
				case Country::Togo:
				case Country::UnitedStatesMinorOutlyingIslands:
				case Country::UnitedStates:
				case Country::VirginIslands_US:
					return sl_true;
				default:
					return sl_false;
			}
		}
	}

	String Time::format(const TimeComponents& d, TimeFormat fmt, const Locale& _locale) noexcept
	{
		Locale locale = _locale;
		if (locale == Locale::Unknown) {
			locale = Locale::getCurrent();
		}
		Language lang = locale.getLanguage();
		switch (lang) {
			case Language::Korean:
				switch (fmt) {
					case TimeFormat::DateTime:
						return String::format("%d\xEB\x85\x84 %d\xEC\x9B\x94 %d\xEC\x9D\xBC %d\xEC\x8B\x9C %d\xEB\xB6\x84 %d\xEC\xB4\x88", d.year, d.month, d.day, d.hour, d.minute, d.second);
					case TimeFormat::MediumDateTime:
						return String::format("%d\xEB\x85\x84 %d\xEC\x9B\x94 %d\xEC\x9D\xBC %02d:%02d:%02d", d.year, d.month, d.day, d.hour, d.minute, d.second);
					case TimeFormat::ShortDateTime:
						return String::format("%d.%d.%d %02d:%02d:%02d", d.year, d.month, d.day, d.hour, d.minute, d.second);
					case TimeFormat::DateTime_12Hour:
						return String::format("%d\xEB\x85\x84 %d\xEC\x9B\x94 %d\xEC\x9D\xBC %s %d\xEC\x8B\x9C %d\xEB\xB6\x84 %d\xEC\xB4\x88", d.year, d.month, d.day, format(d, TimeFormat::AM_PM, locale), GetHour12(d.hour), d.minute, d.second);
					case TimeFormat::MediumDateTime_12Hour:
						return String::format("%d\xEB\x85\x84 %d\xEC\x9B\x94 %d\xEC\x9D\xBC %s %d:%02d:%02d", d.year, d.month, d.day, format(d, TimeFormat::AM_PM, locale), GetHour12(d.hour), d.minute, d.second);
					case TimeFormat::ShortDateTime_12Hour:
						return String::format("%d.%d.%d %s %d:%02d:%02d", d.year, d.month, d.day, format(d, TimeFormat::AM_PM, locale), GetHour12(d.hour), d.minute, d.second);
					case TimeFormat::Date:
					case TimeFormat::MediumDate:
						return String::format("%d\xEB\x85\x84 %d\xEC\x9B\x94 %d\xEC\x9D\xBC", d.year, d.month, d.day);
					case TimeFormat::ShortDate:
						return String::format("%d.%d.%d", d.year, d.month, d.day);
					case TimeFormat::Time:
						return String::format("%d\xEC\x8B\x9C %d\xEB\xB6\x84 %d\xEC\xB4\x88", d.hour, d.minute, d.second);
					case TimeFormat::ShortTime:
						return String::format("%02d:%02d:%02d", d.hour, d.minute, d.second);
					case TimeFormat::Time_12Hour:
						return String::format("%s %d\xEC\x8B\x9C %d\xEB\xB6\x84 %d\xEC\xB4\x88", format(d, TimeFormat::AM_PM, locale), GetHour12(d.hour), d.minute, d.second);
					case TimeFormat::ShortTime_12Hour:
						return String::format("%s %d:%02d:%02d", format(d, TimeFormat::AM_PM, locale), GetHour12(d.hour), d.minute, d.second);
					case TimeFormat::Year:
						return String::fromInt32(d.year) + "\xEB\x85\x84";
					case TimeFormat::Month:
					case TimeFormat::ShortMonth:
						return String::fromUint32(d.month) + "\xEC\x9B\x94";
					case TimeFormat::Day:
						return String::fromUint32(d.day) + "\xEC\x9D\xBC";
					case TimeFormat::Hour:
						return String::fromUint32(d.hour) + "\xEC\x8B\x9C";
					case TimeFormat::Hour_12:
						return String::fromUint32(GetHour12(d.hour)) + "\xEC\x8B\x9C";
					case TimeFormat::Hour_AM_PM:
						return String::format("%s %d\xEC\x8B\x9C", format(d, TimeFormat::AM_PM, locale), GetHour12(d.hour));
					case TimeFormat::AM_PM:
						if (d.hour >= 12) {
							return getPM_Text(locale);
						} else {
							return getAM_Text(locale);
						}
					case TimeFormat::Minute:
						return String::fromUint32(d.minute) + "\xEB\xB6\x84";
					case TimeFormat::Second:
						return String::fromUint32(d.second) + "\xEC\xB4\x88";
					case TimeFormat::YearMonth:
					case TimeFormat::ShortYearMonth:
						return String::format("%d\xEB\x85\x84 %d\xEC\x9B\x94", d.year, d.month);
					case TimeFormat::MonthDay:
					case TimeFormat::ShortMonthDay:
						return String::format("%d\xEC\x9B\x94 %d\xEC\x9D\xBC", d.month, d.day);
					case TimeFormat::HourMinute:
						return String::format("%d\xEC\x8B\x9C %d\xEB\xB6\x84", d.hour, d.minute);
					case TimeFormat::HourMinute_12Hour:
						return String::format("%s %d\xEC\x8B\x9C %d\xEB\xB6\x84", format(d, TimeFormat::AM_PM, locale), GetHour12(d.hour), d.minute);
					case TimeFormat::ShortHourMinute_12Hour:
						return String::format("%s %d:%02d", format(d, TimeFormat::AM_PM, locale), GetHour12(d.hour), d.minute);
					case TimeFormat::MinuteSecond:
						return String::format("%d\xEB\xB6\x84 %d\xEC\xB4\x88", d.minute, d.second);
					case TimeFormat::Weekday:
						return getWeekdayText(d.dayOfWeek, TimeTextType::Long, locale);
					case TimeFormat::ShortWeakday:
						return getWeekdayText(d.dayOfWeek, TimeTextType::Short, locale);
					case TimeFormat::WeekdayDateTime:
						return String::format("%d\xEB\x85\x84 %d\xEC\x9B\x94 %d\xEC\x9D\xBC %s %d\xEC\x8B\x9C %d\xEB\xB6\x84 %d\xEC\xB4\x88", d.year, d.month, d.day, getWeekdayText(d.dayOfWeek, TimeTextType::Long, locale), d.hour, d.minute, d.second);
					case TimeFormat::MediumWeekdayDateTime:
						return String::format("%d\xEB\x85\x84 %d\xEC\x9B\x94 %d\xEC\x9D\xBC %s %02d:%02d:%02d", d.year, d.month, d.day, getWeekdayText(d.dayOfWeek, TimeTextType::Long, locale), d.hour, d.minute, d.second);
					case TimeFormat::ShortWeekdayDateTime:
						return String::format("%d.%d.%d(%s) %02d:%02d:%02d", d.year, d.month, d.day, getWeekdayText(d.dayOfWeek, TimeTextType::Short, locale), d.hour, d.minute, d.second);
					case TimeFormat::WeekdayDateTime_12Hour:
						return String::format("%d\xEB\x85\x84 %d\xEC\x9B\x94 %d\xEC\x9D\xBC %s %s %d\xEC\x8B\x9C %d\xEB\xB6\x84 %d\xEC\xB4\x88", d.year, d.month, d.day, getWeekdayText(d.dayOfWeek, TimeTextType::Long, locale), format(d, TimeFormat::AM_PM, locale), GetHour12(d.hour), d.minute, d.second);
					case TimeFormat::MediumWeekdayDateTime_12Hour:
						return String::format("%d\xEB\x85\x84 %d\xEC\x9B\x94 %d\xEC\x9D\xBC %s %s %d:%02d:%02d", d.year, d.month, d.day, getWeekdayText(d.dayOfWeek, TimeTextType::Long, locale), format(d, TimeFormat::AM_PM, locale), GetHour12(d.hour), d.minute, d.second);
					case TimeFormat::ShortWeekdayDateTime_12Hour:
						return String::format("%d.%d.%d(%s) %s %d:%02d:%02d", d.year, d.month, d.day, getWeekdayText(d.dayOfWeek, TimeTextType::Short, locale), format(d, TimeFormat::AM_PM, locale), GetHour12(d.hour), d.minute, d.second);
					case TimeFormat::WeekdayDate:
						return String::format("%d\xEB\x85\x84 %d\xEC\x9B\x94 %d\xEC\x9D\xBC %s", d.year, d.month, d.day, getWeekdayText(d.dayOfWeek, TimeTextType::Long, locale));
					case TimeFormat::MediumWeekdayDate:
						return String::format("%d\xEB\x85\x84 %d\xEC\x9B\x94 %d\xEC\x9D\xBC (%s)", d.year, d.month, d.day, getWeekdayText(d.dayOfWeek, TimeTextType::Short, locale));
					case TimeFormat::ShortWeekdayDate:
						return String::format("%d.%d.%d(%s)", d.year, d.month, d.day, getWeekdayText(d.dayOfWeek, TimeTextType::Short, locale));
				}
				break;
			case Language::Chinese:
			case Language::Japanese:
				switch (fmt) {
					case TimeFormat::DateTime:
					case TimeFormat::MediumDateTime:
						return String::format("%d\xE5\xB9\xB4%d\xE6\x9C\x88%d\xE6\x97\xA5 %02d:%02d:%02d", d.year, d.month, d.day, d.hour, d.minute, d.second);
					case TimeFormat::ShortDateTime:
						return String::format("%d/%d/%d %02d:%02d:%02d", d.year, d.month, d.day, d.hour, d.minute, d.second);
					case TimeFormat::DateTime_12Hour:
					case TimeFormat::MediumDateTime_12Hour:
						return String::format("%d\xE5\xB9\xB4%d\xE6\x9C\x88%d\xE6\x97\xA5 %s%d:%02d:%02d", d.year, d.month, d.day, format(d, TimeFormat::AM_PM, locale), GetHour12(d.hour), d.minute, d.second);
					case TimeFormat::ShortDateTime_12Hour:
						return String::format("%d/%d/%d %s%d:%02d:%02d", d.year, d.month, d.day, format(d, TimeFormat::AM_PM, locale), GetHour12(d.hour), d.minute, d.second);
					case TimeFormat::Date:
					case TimeFormat::MediumDate:
						return String::format("%d\xE5\xB9\xB4%d\xE6\x9C\x88%d\xE6\x97\xA5", d.year, d.month, d.day);
					case TimeFormat::ShortDate:
						return String::format("%d/%d/%d", d.year, d.month, d.day);
					case TimeFormat::Time:
					case TimeFormat::ShortTime:
						return String::format("%02d:%02d:%02d", d.hour, d.minute, d.second);
					case TimeFormat::Time_12Hour:
					case TimeFormat::ShortTime_12Hour:
						return String::format("%s%d:%02d:%02d", format(d, TimeFormat::AM_PM, locale), GetHour12(d.hour), d.minute, d.second);
					case TimeFormat::Year:
						return String::fromInt32(d.year) + "\xE5\xB9\xB4";
					case TimeFormat::Month:
					case TimeFormat::ShortMonth:
						return String::fromUint32(d.month) + "\xE6\x9C\x88";
					case TimeFormat::Day:
						return String::fromUint32(d.day) + "\xE6\x97\xA5";
					case TimeFormat::Hour:
						if (lang == Language::Chinese) {
							if (locale.getScript() == LanguageScript::ChineseTraditional) {
								return String::fromUint32(d.hour) + "\xE9\xBB\x9E";
							} else {
								return String::fromUint32(d.hour) + "\xE7\x82\xB9";
							}
						} else {
							return String::fromUint32(d.hour) + "\xE6\x99\x82";
						}
					case TimeFormat::Hour_12:
						if (lang == Language::Chinese) {
							if (locale.getScript() == LanguageScript::ChineseTraditional) {
								return String::fromUint32(GetHour12(d.hour)) + "\xE9\xBB\x9E";
							} else {
								return String::fromUint32(GetHour12(d.hour)) + "\xE7\x82\xB9";
							}
						} else {
							return String::fromUint32(GetHour12(d.hour)) + "\xE6\x99\x82";
						}
					case TimeFormat::Hour_AM_PM:
						if (lang == Language::Chinese && d.hour < 12) {
							return "\xE5\x87\x8C\xE6\x99\xA8" + format(d, TimeFormat::Hour_12, locale);
						} else {
							return format(d, TimeFormat::AM_PM, locale) + format(d, TimeFormat::Hour_12, locale);
						}
					case TimeFormat::AM_PM:
						if (d.hour >= 12) {
							return getPM_Text(locale);
						} else {
							return getAM_Text(locale);
						}
					case TimeFormat::Minute:
						return String::fromUint32(d.minute) + "\xE5\x88\x86";
					case TimeFormat::Second:
						return String::fromUint32(d.second) + "\xE7\xA7\x92";
					case TimeFormat::YearMonth:
					case TimeFormat::ShortYearMonth:
						return String::format("%d\xE5\xB9\xB4%d\xE6\x9C\x88", d.year, d.month);
					case TimeFormat::MonthDay:
					case TimeFormat::ShortMonthDay:
						return String::format("%d\xE6\x9C\x88%d\xE6\x97\xA5", d.month, d.day);
					case TimeFormat::HourMinute:
						return String::format("%s%d\xE5\x88\x86", format(d, TimeFormat::Hour, locale), d.minute);
					case TimeFormat::HourMinute_12Hour:
					case TimeFormat::ShortHourMinute_12Hour:
						return String::format("%s%d:%02d", format(d, TimeFormat::AM_PM, locale), GetHour12(d.hour), d.minute);
					case TimeFormat::MinuteSecond:
						return String::format("%d\xE5\x88\x86%d\xE7\xA7\x92", d.minute, d.second);
					case TimeFormat::Weekday:
						return getWeekdayText(d.dayOfWeek, TimeTextType::Long, locale);
					case TimeFormat::ShortWeakday:
						return getWeekdayText(d.dayOfWeek, TimeTextType::Short, locale);
					case TimeFormat::WeekdayDateTime:
						return String::format("%d\xE5\xB9\xB4%d\xE6\x9C\x88%d\xE6\x97\xA5 %s %02d:%02d:%02d", d.year, d.month, d.day, getWeekdayText(d.dayOfWeek, TimeTextType::Long, locale), d.hour, d.minute, d.second);
					case TimeFormat::MediumWeekdayDateTime:
						return String::format("%d\xE5\xB9\xB4%d\xE6\x9C\x88%d\xE6\x97\xA5(%s) %02d:%02d:%02d", d.year, d.month, d.day, getWeekdayText(d.dayOfWeek, TimeTextType::Short, locale), d.hour, d.minute, d.second);
					case TimeFormat::ShortWeekdayDateTime:
						return String::format("%d/%d/%d(%s) %02d:%02d:%02d", d.year, d.month, d.day, getWeekdayText(d.dayOfWeek, TimeTextType::Short, locale), d.hour, d.minute, d.second);
					case TimeFormat::WeekdayDateTime_12Hour:
						return String::format("%d\xE5\xB9\xB4%d\xE6\x9C\x88%d\xE6\x97\xA5 %s %s%d:%02d:%02d", d.year, d.month, d.day, getWeekdayText(d.dayOfWeek, TimeTextType::Long, locale), format(d, TimeFormat::AM_PM, locale), GetHour12(d.hour), d.minute, d.second);
					case TimeFormat::MediumWeekdayDateTime_12Hour:
						return String::format("%d\xE5\xB9\xB4%d\xE6\x9C\x88%d\xE6\x97\xA5(%s) %s%d:%02d:%02d", d.year, d.month, d.day, getWeekdayText(d.dayOfWeek, TimeTextType::Short, locale), format(d, TimeFormat::AM_PM, locale), GetHour12(d.hour), d.minute, d.second);
					case TimeFormat::ShortWeekdayDateTime_12Hour:
						return String::format("%d/%d/%d(%s) %s%d:%02d:%02d", d.year, d.month, d.day, getWeekdayText(d.dayOfWeek, TimeTextType::Short, locale), format(d, TimeFormat::AM_PM, locale), GetHour12(d.hour), d.minute, d.second);
					case TimeFormat::WeekdayDate:
						return String::format("%d\xE5\xB9\xB4%d\xE6\x9C\x88%d\xE6\x97\xA5 %s", d.year, d.month, d.day, getWeekdayText(d.dayOfWeek, TimeTextType::Long, locale));
					case TimeFormat::MediumWeekdayDate:
						return String::format("%d\xE5\xB9\xB4%d\xE6\x9C\x88%d\xE6\x97\xA5(%s)", d.year, d.month, d.day, getWeekdayText(d.dayOfWeek, TimeTextType::Short, locale));
					case TimeFormat::ShortWeekdayDate:
						return String::format("%d/%d/%d(%s)", d.year, d.month, d.day, getWeekdayText(d.dayOfWeek, TimeTextType::Short, locale));
				}
				break;
			default:
				switch (fmt) {
					case TimeFormat::DateTime:
						if (IsMDY(locale)) {
							return String::format("%02d:%02d:%02d, %s %d, %d", d.hour, d.minute, d.second, getMonthText(d.month, TimeTextType::Long), d.day, d.year);
						} else {
							return String::format("%02d:%02d:%02d, %d %s %d", d.hour, d.minute, d.second, d.day, getMonthText(d.month, TimeTextType::Long), d.year);
						}
					case TimeFormat::MediumDateTime:
						if (IsMDY(locale)) {
							return String::format("%02d:%02d:%02d, %s %d, %d", d.hour, d.minute, d.second, getMonthText(d.month, TimeTextType::Short), d.day, d.year);
						} else {
							return String::format("%02d:%02d:%02d, %d %s %d", d.hour, d.minute, d.second, d.day, getMonthText(d.month, TimeTextType::Short), d.year);
						}
					case TimeFormat::ShortDateTime:
						if (IsMDY(locale)) {
							return String::format("%02d:%02d:%02d, %d/%d/%d", d.hour, d.minute, d.second, d.month, d.day, d.year);
						} else {
							return String::format("%02d:%02d:%02d, %d/%d/%d", d.hour, d.minute, d.second, d.day, d.month, d.year);
						}
					case TimeFormat::DateTime_12Hour:
						if (IsMDY(locale)) {
							return String::format("%d:%02d:%02d %s, %s %d, %d", GetHour12(d.hour), d.minute, d.second, d.hour>=12?"PM":"AM", getMonthText(d.month, TimeTextType::Long), d.day, d.year);
						} else {
							return String::format("%d:%02d:%02d %s, %d %s %d", GetHour12(d.hour), d.minute, d.second, d.hour>=12?"PM":"AM", d.day, getMonthText(d.month, TimeTextType::Long), d.year);
						}
					case TimeFormat::MediumDateTime_12Hour:
						if (IsMDY(locale)) {
							return String::format("%d:%02d:%02d %s, %s %d, %d", GetHour12(d.hour), d.minute, d.second, d.hour>=12?"PM":"AM", getMonthText(d.month, TimeTextType::Short), d.day, d.year);
						} else {
							return String::format("%d:%02d:%02d %s, %d %s %d", GetHour12(d.hour), d.minute, d.second, d.hour>=12?"PM":"AM", d.day, getMonthText(d.month, TimeTextType::Short), d.year);
						}
					case TimeFormat::ShortDateTime_12Hour:
						if (IsMDY(locale)) {
							return String::format("%d:%02d:%02d %s, %d/%d/%d", GetHour12(d.hour), d.minute, d.second, d.hour>=12?"PM":"AM", d.month, d.day, d.year);
						} else {
							return String::format("%d:%02d:%02d %s, %d/%d/%d", GetHour12(d.hour), d.minute, d.second, d.hour>=12?"PM":"AM", d.day, d.month, d.year);
						}
					case TimeFormat::Date:
						if (IsMDY(locale)) {
							return String::format("%s %d, %d", getMonthText(d.month, TimeTextType::Long), d.day, d.year);
						} else {
							return String::format("%d %s %d", d.day, getMonthText(d.month, TimeTextType::Long), d.year);
						}
					case TimeFormat::MediumDate:
						if (IsMDY(locale)) {
							return String::format("%s %d, %d", getMonthText(d.month, TimeTextType::Short), d.day, d.year);
						} else {
							return String::format("%d %s %d", d.day, getMonthText(d.month, TimeTextType::Short), d.year);
						}
					case TimeFormat::ShortDate:
						if (IsMDY(locale)) {
							return String::format("%d/%d/%d", d.month, d.day, d.year);
						} else {
							return String::format("%d/%d/%d", d.day, d.month, d.year);
						}
					case TimeFormat::Time:
					case TimeFormat::ShortTime:
						return String::format("%02d:%02d:%02d", d.hour, d.minute, d.second);
					case TimeFormat::Time_12Hour:
					case TimeFormat::ShortTime_12Hour:
						return String::format("%d:%02d:%02d %s", GetHour12(d.hour), d.minute, d.second, d.hour>=12?"PM":"AM");
					case TimeFormat::Year:
						return String::fromInt32(d.year);
					case TimeFormat::Month:
						return getMonthText(d.month, TimeTextType::Long);
					case TimeFormat::ShortMonth:
						return getMonthText(d.month, TimeTextType::Short);
					case TimeFormat::Day:
						return String::fromUint32(d.day);
					case TimeFormat::Hour:
						return String::format("%d o'clock", d.hour);
					case TimeFormat::Hour_12:
						return String::format("%d o'clock", GetHour12(d.hour));
					case TimeFormat::Hour_AM_PM:
						return String::format("%d %s", GetHour12(d.hour), d.hour>=12?"PM":"AM");
					case TimeFormat::AM_PM:
						if (d.hour >= 12) {
							return getPM_Text(locale);
						} else {
							return getAM_Text(locale);
						}
					case TimeFormat::Minute:
						return String::fromUint32(d.minute) + " minute";
					case TimeFormat::Second:
						return String::fromUint32(d.second) + " second";
					case TimeFormat::YearMonth:
						return String::format("%s %d", getMonthText(d.month, TimeTextType::Long), d.year);
					case TimeFormat::ShortYearMonth:
						return String::format("%s %d", getMonthText(d.month, TimeTextType::Short), d.year);
					case TimeFormat::MonthDay:
						if (IsMDY(locale)) {
							return String::format("%s %d", getMonthText(d.month, TimeTextType::Long), d.day);
						} else {
							return String::format("%d %s", d.day, getMonthText(d.month, TimeTextType::Long));
						}
					case TimeFormat::ShortMonthDay:
						if (IsMDY(locale)) {
							return String::format("%s %d", getMonthText(d.month, TimeTextType::Short), d.day);
						} else {
							return String::format("%d %s", d.day, getMonthText(d.month, TimeTextType::Short));
						}
					case TimeFormat::HourMinute:
						return String::format("%02d:%02d", d.hour, d.minute);
					case TimeFormat::HourMinute_12Hour:
					case TimeFormat::ShortHourMinute_12Hour:
						return String::format("%d:%02d %s", GetHour12(d.hour), d.minute, d.hour>=12?"PM":"AM");
					case TimeFormat::MinuteSecond:
						return String::format("%02d:%02d", d.minute, d.second);
					case TimeFormat::Weekday:
						return getWeekdayText(d.dayOfWeek, TimeTextType::Long, locale);
					case TimeFormat::ShortWeakday:
						return getWeekdayText(d.dayOfWeek, TimeTextType::Short, locale);
					case TimeFormat::WeekdayDateTime:
						if (IsMDY(locale)) {
							return String::format("%02d:%02d:%02d, %s, %s %d, %d", d.hour, d.minute, d.second, getWeekdayText(d.dayOfWeek, TimeTextType::Long, locale), getMonthText(d.month, TimeTextType::Long), d.day, d.year);
						} else {
							return String::format("%02d:%02d:%02d, %s, %d %s %d", d.hour, d.minute, d.second, getWeekdayText(d.dayOfWeek, TimeTextType::Long, locale), d.day, getMonthText(d.month, TimeTextType::Long), d.year);
						}
					case TimeFormat::MediumWeekdayDateTime:
						if (IsMDY(locale)) {
							return String::format("%02d:%02d:%02d, %s, %s %d, %d", d.hour, d.minute, d.second, getWeekdayText(d.dayOfWeek, TimeTextType::Short, locale), getMonthText(d.month, TimeTextType::Short), d.day, d.year);
						} else {
							return String::format("%02d:%02d:%02d, %s, %d %s %d", d.hour, d.minute, d.second, getWeekdayText(d.dayOfWeek, TimeTextType::Short, locale), d.day, getMonthText(d.month, TimeTextType::Short), d.year);
						}
					case TimeFormat::ShortWeekdayDateTime:
						if (IsMDY(locale)) {
							return String::format("%02d:%02d:%02d, %s, %d/%d/%d", d.hour, d.minute, d.second, getWeekdayText(d.dayOfWeek, TimeTextType::Short, locale), d.month, d.day, d.year);
						} else {
							return String::format("%02d:%02d:%02d, %s, %d/%d/%d", d.hour, d.minute, d.second, getWeekdayText(d.dayOfWeek, TimeTextType::Short, locale), d.day, d.month, d.year);
						}
					case TimeFormat::WeekdayDateTime_12Hour:
						if (IsMDY(locale)) {
							return String::format("%d:%02d:%02d %s, %s, %s %d, %d", GetHour12(d.hour), d.minute, d.second, d.hour>=12?"PM":"AM", getWeekdayText(d.dayOfWeek, TimeTextType::Long, locale), getMonthText(d.month, TimeTextType::Long), d.day, d.year);
						} else {
							return String::format("%d:%02d:%02d %s, %s, %d %s %d", GetHour12(d.hour), d.minute, d.second, d.hour>=12?"PM":"AM", getWeekdayText(d.dayOfWeek, TimeTextType::Long, locale), d.day, getMonthText(d.month, TimeTextType::Long), d.year);
						}
					case TimeFormat::MediumWeekdayDateTime_12Hour:
						if (IsMDY(locale)) {
							return String::format("%d:%02d:%02d %s, %s, %s %d, %d", GetHour12(d.hour), d.minute, d.second, d.hour>=12?"PM":"AM", getWeekdayText(d.dayOfWeek, TimeTextType::Short, locale), getMonthText(d.month, TimeTextType::Short), d.day, d.year);
						} else {
							return String::format("%d:%02d:%02d %s, %s, %d %s %d", GetHour12(d.hour), d.minute, d.second, d.hour>=12?"PM":"AM", getWeekdayText(d.dayOfWeek, TimeTextType::Short, locale), d.day, getMonthText(d.month, TimeTextType::Short), d.year);
						}
					case TimeFormat::ShortWeekdayDateTime_12Hour:
						if (IsMDY(locale)) {
							return String::format("%d:%02d:%02d %s, %s, %d/%d/%d", GetHour12(d.hour), d.minute, d.second, d.hour>=12?"PM":"AM", getWeekdayText(d.dayOfWeek, TimeTextType::Short, locale), d.month, d.day, d.year);
						} else {
							return String::format("%d:%02d:%02d %s, %s, %d/%d/%d", GetHour12(d.hour), d.minute, d.second, d.hour>=12?"PM":"AM", getWeekdayText(d.dayOfWeek, TimeTextType::Short, locale), d.day, d.month, d.year);
						}
					case TimeFormat::WeekdayDate:
						if (IsMDY(locale)) {
							return String::format("%s, %s %d, %d", getWeekdayText(d.dayOfWeek, TimeTextType::Long, locale), getMonthText(d.month, TimeTextType::Long), d.day, d.year);
						} else {
							return String::format("%s, %d %s %d", getWeekdayText(d.dayOfWeek, TimeTextType::Long, locale), d.day, getMonthText(d.month, TimeTextType::Long), d.year);
						}
					case TimeFormat::MediumWeekdayDate:
						if (IsMDY(locale)) {
							return String::format("%s, %s %d, %d", getWeekdayText(d.dayOfWeek, TimeTextType::Short, locale), getMonthText(d.month, TimeTextType::Short), d.day, d.year);
						} else {
							return String::format("%s, %d %s %d", getWeekdayText(d.dayOfWeek, TimeTextType::Short, locale), d.day, getMonthText(d.month, TimeTextType::Short), d.year);
						}
					case TimeFormat::ShortWeekdayDate:
						if (IsMDY(locale)) {
							return String::format("%s, %d/%d/%d", getWeekdayText(d.dayOfWeek, TimeTextType::Short, locale), d.month, d.day, d.year);
						} else {
							return String::format("%s, %d/%d/%d", getWeekdayText(d.dayOfWeek, TimeTextType::Short, locale), d.day, d.month, d.year);
						}
				}
				break;
		}
		return sl_null;
	}

	String Time::format(const TimeComponents& d, TimeFormat fmt) noexcept
	{
		return format(d, fmt, Locale::Unknown);
	}

	String Time::format(TimeFormat fmt, const TimeZone& zone, const Locale& locale) const noexcept
	{
		if (isZero()) {
			return sl_null;
		}
		TimeComponents comps;
		get(comps, zone);
		return format(comps, fmt, locale);
	}

	String Time::format(TimeFormat fmt, const TimeZone& zone) const noexcept
	{
		return format(fmt, zone, Locale::Unknown);
	}

	String Time::format(TimeFormat fmt, const Locale& locale) const noexcept
	{
		return format(fmt, TimeZone::Local, locale);
	}

	String Time::format(TimeFormat fmt) const noexcept
	{
		return format(fmt, TimeZone::Local);
	}

	String Time::format(const StringView& fmt, const Locale& locale) const noexcept
	{
		return String::format(locale, fmt, *this);
	}

	String Time::format(const StringView& fmt) const noexcept
	{
		return String::format(fmt, *this);
	}

	Time Time::fromString(const StringParam& str, const TimeZone& zone) noexcept
	{
		Time ret;
		if (ret.parse(str, zone)) {
			return ret;
		}
		return 0;
	}

	sl_bool Time::equals(const Time& other) const noexcept
	{
		return m_time == other.m_time;
	}

	sl_bool Time::equals(sl_int64 other) const noexcept
	{
		return m_time == other;
	}

	sl_compare_result Time::compare(const Time& other) const noexcept
	{
		return ComparePrimitiveValues(m_time, other.m_time);
	}

	sl_compare_result Time::compare(sl_int64 other) const noexcept
	{
		return ComparePrimitiveValues(m_time, other);
	}

	sl_size Time::getHashCode() const noexcept
	{
		return Rehash64ToSize(m_time);
	}

	namespace {

		template <class CT>
		static sl_reg DoParseComponents(TimeComponents* comps, const CT* data, sl_size i, sl_size n) noexcept
		{
			if (i >= n) {
				return SLIB_PARSE_ERROR;
			}
			sl_int32 YMDHMS[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
			sl_size index = 0;
			sl_size posParsed = i;
			while (i < n && index < 8) {
				if (!(data[i])) {
					break;
				}
				do {
					CT ch = data[i];
					if (SLIB_CHAR_IS_SPACE_TAB(ch)) {
						i++;
					} else {
						break;
					}
				} while (i < n);
				if (i >= n) {
					break;
				}
				sl_int32 value = 0;
				sl_bool flagNumber = sl_false;
				do {
					CT ch = data[i];
					if (SLIB_CHAR_IS_DIGIT(ch)) {
						value = value * 10 + (ch - '0');
						flagNumber = sl_true;
						i++;
					} else {
						break;
					}
				} while (i < n);
				if (!flagNumber) {
					break;
				}
				posParsed = i;

				YMDHMS[index] = value;
				index++;

				if (i >= n) {
					break;
				}
				do {
					CT ch = data[i];
					if (SLIB_CHAR_IS_SPACE_TAB(ch)) {
						i++;
					} else {
						break;
					}
				} while (i < n);

				posParsed = i;

				if (index >= 8) {
					break;
				}

				if (i < n) {
					CT ch = data[i];
					if (!SLIB_CHAR_IS_DIGIT(ch)) {
						if (ch == '/' || ch == '-') {
							if (index >= 3) {
								break;
							}
						} else if (ch == 'T') {
							if (index != 3) {
								break;
							}
						} else if (ch == ':') {
							if (index == 1) {
								index = 4;
								YMDHMS[3] = YMDHMS[0];
								YMDHMS[0] = 1970;
								YMDHMS[1] = 1;
								YMDHMS[2] = 1;
							} else if (index < 4) {
								break;
							} else if (index >= 6) {
								break;
							}
						} else if (ch == '.') {
							if (index >= 4 && index <= 5) {
								break;
							}
							posParsed = i + 1;
						} else {
							break;
						}
						i++;
					}
				} else {
					break;
				}
			}
			if (index >= 3) {
				if (comps) {
					Base::zeroMemory(comps, sizeof(TimeComponents));
					comps->year = YMDHMS[0];
					comps->month = YMDHMS[1];
					comps->day = YMDHMS[2];
					comps->hour = YMDHMS[3];
					comps->minute = YMDHMS[4];
					comps->second = YMDHMS[5];
					comps->milliseconds = YMDHMS[6];
					comps->microseconds = YMDHMS[7];
				}
				return posParsed;
			}
			return SLIB_PARSE_ERROR;
		}

		template <class CT>
		SLIB_INLINE static sl_reg DoParse(Time* _out, const TimeZone& zone, const CT* data, sl_size i, sl_size n) noexcept
		{
			TimeComponents comps;
			sl_reg ret = DoParseComponents(&comps, data, i, n);
			if (ret != SLIB_PARSE_ERROR) {
				if (_out) {
					_out->set(comps, zone);
				}
			}
			return ret;
		}

		template <class CT>
		SLIB_INLINE static sl_reg DoParse(Time* _out, const CT* data, sl_size i, sl_size n) noexcept
		{
			TimeComponents comps;
			sl_reg ret = DoParseComponents(&comps, data, i, n);
			if (ret != SLIB_PARSE_ERROR) {
				if (ret > 0 && (sl_size)ret < n && data[ret] == 'Z') {
					ret++;
					if (_out) {
						_out->set(comps, TimeZone::UTC());
					}
				} else {
					if (_out) {
						_out->set(comps);
					}
				}
			}
			return ret;
		}

	}

	SLIB_DEFINE_CLASS_PARSE_MEMBERS(TimeComponents, DoParseComponents)
	SLIB_DEFINE_CLASS_PARSE_MEMBERS(Time, DoParse)
	SLIB_DEFINE_CLASS_PARSE2_MEMBERS(Time, const TimeZone&, zone, DoParse)

	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(TimeCounter)

	TimeCounter::TimeCounter() noexcept
	{
		reset();
	}

	sl_uint64 TimeCounter::now() noexcept
	{
		return System::getHighResolutionTickCount();
	}

	sl_uint64 TimeCounter::getElapsedMilliseconds() const noexcept
	{
		return getElapsedMilliseconds(now());
	}

	sl_uint64 TimeCounter::getElapsedMilliseconds(sl_uint64 current) const noexcept
	{
		return current - m_timeStart;
	}

	void TimeCounter::reset() noexcept
	{
		reset(now());
	}

	void TimeCounter::reset(sl_uint64 current) noexcept
	{
		m_timeStart = current;
	}


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(TimeKeeper)

	TimeKeeper::TimeKeeper() noexcept: m_flagStarted(sl_false), m_flagRunning(sl_false), m_timeLast(0), m_timeElapsed(0)
	{
	}

	sl_uint64 TimeKeeper::now() noexcept
	{
		return System::getHighResolutionTickCount();
	}

	void TimeKeeper::start() noexcept
	{
		startAndSetTime(0, now());
	}

	void TimeKeeper::start(sl_uint64 current) noexcept
	{
		startAndSetTime(0, current);
	}

	void TimeKeeper::startAndSetTime(sl_uint64 init) noexcept
	{
		startAndSetTime(init, now());
	}

	void TimeKeeper::startAndSetTime(sl_uint64 init, sl_uint64 current) noexcept
	{
		SpinLocker lock(&m_lock);
		if (m_flagStarted) {
			return;
		}
		m_timeLast = current;
		m_timeElapsed = init;
		m_flagStarted = sl_true;
		m_flagRunning = sl_true;
	}

	void TimeKeeper::restart() noexcept
	{
		restartAndSetTime(0, now());
	}

	void TimeKeeper::restart(sl_uint64 current) noexcept
	{
		restartAndSetTime(0, current);
	}

	void TimeKeeper::restartAndSetTime(sl_uint64 init) noexcept
	{
		restartAndSetTime(init, now());
	}

	void TimeKeeper::restartAndSetTime(sl_uint64 init, sl_uint64 current) noexcept
	{
		SpinLocker lock(&m_lock);
		m_timeLast = current;
		m_timeElapsed = init;
		m_flagStarted = sl_true;
		m_flagRunning = sl_true;
	}

	void TimeKeeper::stop() noexcept
	{
		m_flagStarted = sl_false;
	}

	void TimeKeeper::resume() noexcept
	{
		resume(now());
	}

	void TimeKeeper::resume(sl_uint64 current) noexcept
	{
		SpinLocker lock(&m_lock);
		if (m_flagStarted) {
			if (!m_flagRunning) {
				m_timeLast = current;
				m_flagRunning = sl_true;
			}
		}
	}

	void TimeKeeper::pause() noexcept
	{
		pause(now());
	}

	void TimeKeeper::pause(sl_uint64 current) noexcept
	{
		SpinLocker lock(&m_lock);
		if (m_flagStarted) {
			if (m_flagRunning) {
				if (current > m_timeLast) {
					m_timeElapsed += (current - m_timeLast);
				}
				m_flagRunning = sl_false;
			}
		}
	}

	sl_uint64 TimeKeeper::getTime() const noexcept
	{
		return getTime(now());
	}

	sl_uint64 TimeKeeper::getTime(sl_uint64 current) const noexcept
	{
		SpinLocker lock(&m_lock);
		if (!m_flagStarted) {
			return 0;
		}
		if (!m_flagRunning) {
			return m_timeElapsed;
		}
		return m_timeElapsed + (current - m_timeLast);
	}

	void TimeKeeper::setTime(sl_uint64 time) noexcept
	{
		setTime(time, now());
	}

	void TimeKeeper::setTime(sl_uint64 time, sl_uint64 current) noexcept
	{
		SpinLocker lock(&m_lock);
		if (m_flagStarted) {
			m_timeElapsed = time;
			m_timeLast = current;
		}
	}

	void TimeKeeper::update() noexcept
	{
		update(now());
	}

	void TimeKeeper::update(sl_uint64 current) noexcept
	{
		SpinLocker lock(&m_lock);
		if (m_flagStarted) {
			if (m_flagRunning) {
				sl_int64 add = current - m_timeLast;
				if (add > 0) {
					m_timeElapsed += add;
				}
				m_timeLast = current;
			}
		}
	}

	sl_bool TimeKeeper::isStarted() const noexcept
	{
		return m_flagStarted;
	}

	sl_bool TimeKeeper::isStopped() const noexcept
	{
		return !m_flagStarted;
	}

	sl_bool TimeKeeper::isRunning() const noexcept
	{
		return m_flagStarted && m_flagRunning;
	}

	sl_bool TimeKeeper::isNotRunning() const noexcept
	{
		return !(m_flagStarted && m_flagRunning);
	}

	sl_bool TimeKeeper::isPaused() const noexcept
	{
		return m_flagStarted && !m_flagRunning;
	}


	SLIB_DEFINE_ROOT_OBJECT(CTimeZone)

	CTimeZone::CTimeZone() noexcept
	{
	}

	CTimeZone::~CTimeZone() noexcept
	{
	}

	sl_int64 CTimeZone::getOffset()
	{
		return getOffset(Time::now());
	}


	SLIB_DEFINE_OBJECT(GenericTimeZone, CTimeZone)

	GenericTimeZone::GenericTimeZone(sl_int64 offset)
	{
		m_offset = offset;
	}

	GenericTimeZone::~GenericTimeZone()
	{
	}

	sl_int64 GenericTimeZone::getOffset(const Time& time)
	{
		return m_offset;
	}

	namespace {
		static const char g_local[sizeof(TimeZone)] = { 0 };
	}
	const TimeZone& TimeZone::Local = *((const TimeZone*)g_local);
	const TimeZone& Time::LocalZone = *((const TimeZone*)g_local);

	const TimeZone& TimeZone::UTC() noexcept
	{
		SLIB_SAFE_LOCAL_STATIC(TimeZone, utc, TimeZone::create(0));
		return utc;
	}

	TimeZone TimeZone::create(sl_int64 offset) noexcept
	{
		return new GenericTimeZone(offset);
	}

	sl_bool TimeZone::isLocal() const noexcept
	{
		return isNull();
	}

	sl_bool TimeZone::isUTC() const noexcept
	{
		return ref.ptr == UTC().ref.ptr;
	}

	sl_int64 TimeZone::getOffset() const
	{
		return getOffset(Time::now());
	}

	sl_int64 TimeZone::getOffset(const Time& time) const
	{
		Ref<CTimeZone> obj = ref;
		if (obj.isNotNull()) {
			if (isUTC()) {
				return 0;
			}
			return obj->getOffset();
		}
		return time.getLocalTimeOffset();
	}


	sl_bool Atomic<TimeZone>::isLocal() const noexcept
	{
		return isNull();
	}

	sl_bool Atomic<TimeZone>::isUTC() const noexcept
	{
		return ref._ptr == TimeZone::UTC().ref.ptr;
	}

}
