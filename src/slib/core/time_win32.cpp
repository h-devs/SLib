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

#include "slib/core/definition.h"

#if defined(SLIB_PLATFORM_IS_WINDOWS)

#include "slib/core/time.h"

#include "slib/platform.h"

namespace slib
{

	sl_bool Time::_toPlatformComponents(TimeComponents& output, sl_int64 t, sl_bool flagUTC) noexcept
	{
		SYSTEMTIME st;
		if (Win32::getSYSTEMTIME(st, Time::fromUnixTimeF(t), flagUTC)) {
			output.year = st.wYear;
			output.month = (sl_uint8)(st.wMonth);
			output.day = (sl_uint8)(st.wDay);
			output.dayOfWeek = (sl_uint8)(st.wDayOfWeek);
			output.hour = (sl_uint8)(st.wHour);
			output.minute = (sl_uint8)(st.wMinute);
			output.second = (sl_uint8)(st.wSecond);
			return sl_true;
		}
		return sl_false;
	}

	sl_bool Time::_toPlatformSeconds(sl_int64& output, sl_int32 year, sl_int32 month, sl_int32 day, sl_bool flagUTC) noexcept
	{
		SYSTEMTIME st = {0};
		st.wYear = (WORD)year;
		st.wMonth = (WORD)month;
		st.wDay = (WORD)day;
		Time t;
		if (Win32::getTime(t, st, flagUTC)) {
			output = t.toUnixTimeF();
			return sl_true;
		}
		return sl_false;
	}

	void Time::_setNow() noexcept
	{
		sl_int64 n = 0;
		GetSystemTimeAsFileTime((PFILETIME)&n);
		setWindowsFileTime(n);
	}

	sl_bool Time::_setToSystem() const noexcept
	{
#if defined(SLIB_PLATFORM_IS_WIN32)
		SYSTEMTIME st;
		sl_int64 n = toWindowsFileTime();
		if (FileTimeToSystemTime((PFILETIME)&n, &st)) {
			if (SetSystemTime(&st)) {
				return sl_true;
			}
		}
#endif
		return sl_false;
	}

	sl_int64 Time::getLocalTimeOffset(sl_int32 year, sl_int32 month, sl_int32 day) noexcept
	{
		SYSTEMTIME st = { 0 };
		st.wYear = (WORD)year;
		st.wMonth = (WORD)month;
		st.wDay = (WORD)day;

		SYSTEMTIME lt;
		if (SystemTimeToTzSpecificLocalTime(NULL, &st, &lt)) {
			FILETIME t1;
			if (SystemTimeToFileTime(&st, &t1)) {
				FILETIME t2;
				if (SystemTimeToFileTime(&lt, &t2)) {
					return (*((sl_int64*)&t2) - *((sl_int64*)&t1)) / 10000000;
				}
			}
		}
		TIME_ZONE_INFORMATION tz;
		if (GetTimeZoneInformation(&tz) != TIME_ZONE_ID_INVALID) {
			return (sl_int64)(tz.Bias * -60);
		}
		return 0;
	}

}

#endif
