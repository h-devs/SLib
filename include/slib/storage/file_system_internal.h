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

#ifndef CHECKHEADER_SLIB_STORAGE_FILE_SYSTEM_INTERNAL
#define CHECKHEADER_SLIB_STORAGE_FILE_SYSTEM_INTERNAL

#include "file_system.h"

#include "../core/log.h"

#ifndef TAG
#define TAG "FileSystem"
#endif
#define LOG(...) SLIB_LOG(TAG, ##__VA_ARGS__)
#define LOG_DEBUG(...) SLIB_LOG_DEBUG(TAG, ##__VA_ARGS__)
#define LOG_ERROR(...) SLIB_LOG_ERROR(TAG, ##__VA_ARGS__)

#define SET_ERROR_AND_RETURN(error, ret)	\
	FileSystem::setLastError(error); \
	return ret;

#define PATH_FROM_CONTEXT(context)			(context ? context->path : sl_null)

#endif