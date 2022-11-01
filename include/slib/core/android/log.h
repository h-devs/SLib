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

#ifndef CHECKHEADER_SLIB_CORE_ANDROID_LOG
#define CHECKHEADER_SLIB_CORE_ANDROID_LOG

#include "../definition.h"

#ifdef SLIB_PLATFORM_IS_ANDROID

#include "../log.h"

namespace slib
{
	namespace android
	{

		void Log(LogPriority priority, const StringParam& tag, const StringParam& content);

		template <class ARG, class... ARGS>
		SLIB_INLINE static void Log(LogPriority priority, const StringParam& tag, const StringView& fmt, ARG&& arg, ARGS&&... args)
		{
			Log(priority, tag, String::format(fmt, Forward<ARG>(arg), Forward<ARGS>(args)...));
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

	}
}

#endif

#endif
