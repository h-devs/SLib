/*
 *   Copyright (c) 2008-2019 SLIBIO <https://github.com/SLIBIO>
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

#if defined(SLIB_PLATFORM_IS_UNIX)

#include "slib/core/time.h"

#include <time.h>
#include <sys/time.h>
#include <errno.h>

#define TIME_SECOND SLIB_INT64(1000000)

namespace slib
{

	sl_bool Time::_toPlatformComponents(TimeComponents& output, sl_int64 _t, sl_bool flagUTC) noexcept
	{
		time_t t = (time_t)_t;
		tm v;
		if (flagUTC) {
			if (!(gmtime_r(&t, &v))) {
				return sl_false;
			}
		} else {
			if (!(localtime_r(&t, &v))) {
				return sl_false;
			}
		}
		output.year = v.tm_year + 1900;
		output.month = v.tm_mon + 1;
		output.day = v.tm_mday;
		output.dayOfWeek = v.tm_wday;
		output.hour = v.tm_hour;
		output.minute = v.tm_min;
		output.second = v.tm_sec;
		return sl_true;
	}

	sl_bool Time::_toPlatformSeconds(sl_int64& output, sl_int32 year, sl_int32 month, sl_int32 day, sl_bool flagUTC) noexcept
	{
		tm v = {0};
		v.tm_year = year - 1900;
		v.tm_mon = month - 1;
		v.tm_mday = day;
		v.tm_isdst = -1;
		time_t t;
		if (flagUTC) {
			t = timegm(&v);
		} else {
			t = mktime(&v);
		}
		if (t == (time_t)-1 && errno == EOVERFLOW) {
			return sl_false;
		}
		output = (sl_int64)t;
		return sl_true;
	}

	void Time::_setNow() noexcept
	{
		sl_uint64 t;
		timeval tv;
		if (0 == gettimeofday(&tv, sl_null)) {
			t = tv.tv_sec;
			t *= TIME_SECOND;
			t += tv.tv_usec;
		} else {
			t = time(sl_null);
			t *= TIME_SECOND;
		}
		m_time = t;
	}

	sl_bool Time::_setToSystem() const noexcept
	{
		sl_int64 t = m_time;
		timeval tv;
		tv.tv_sec = (time_t)(t / TIME_SECOND);
		int m = (int)(t % TIME_SECOND);
		if (m < 0) {
			m += TIME_SECOND;
			tv.tv_sec -= 1;
		}
		tv.tv_usec = m;
		return !(settimeofday(&tv, sl_null));
	}

	sl_int64 Time::getLocalTimeOffset(sl_int32 year, sl_int32 month, sl_int32 day) noexcept
	{
		tm v = {0};
		v.tm_year = year - 1900;
		v.tm_mon = month - 1;
		v.tm_mday = day;
		v.tm_isdst = -1;
		if (mktime(&v) == (time_t)-1 && errno == EOVERFLOW) {
			time_t t = time(sl_null);
			if (!(localtime_r(&t, &v))) {
				return 0;
			}
		}
		return (sl_int64)(v.tm_gmtoff);
	}

}

#endif
