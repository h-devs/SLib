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

#include "slib/platform/definition.h"

#ifdef SLIB_PLATFORM_IS_ANDROID

#include "slib/platform/android/log.h"

#include <android/log.h>

namespace slib
{

	namespace android
	{

		void Log(LogPriority _priority, const StringParam& _tag, const StringParam& _content)
		{
			StringCstr tag(_tag);
			StringCstr content(_content);
			int priority;
			switch (_priority) {
				case LogPriority::Verbose:
					priority = ANDROID_LOG_VERBOSE;
					break;
				case LogPriority::Debug:
					priority = ANDROID_LOG_DEBUG;
					break;
				case LogPriority::Info:
					priority = ANDROID_LOG_INFO;
					break;
				case LogPriority::Warning:
					priority = ANDROID_LOG_WARN;
					break;
				case LogPriority::Error:
					priority = ANDROID_LOG_ERROR;
					break;
				case LogPriority::Fatal:
					priority = ANDROID_LOG_FATAL;
					break;
				default:
					return;
			}
			__android_log_print(priority, tag.getData(), "%s", content.getData());
		}

	}

}

#endif

