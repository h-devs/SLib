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

#ifndef CHECKHEADER_SLIB_CORE_FILE_UTIL
#define CHECKHEADER_SLIB_CORE_FILE_UTIL

#include "file.h"
#include "string.h"
#include "list.h"

namespace slib
{

	// FilePathSegments is not thread-safe
	class SLIB_EXPORT FilePathSegments
	{
	public:
		sl_bool flagStartsWithSlash;
		sl_bool flagEndsWithSlash;
		sl_uint32 parentLevel;
		List<StringView> segments;

	public:
		FilePathSegments() noexcept;

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(FilePathSegments)

	public:
		void parsePath(const String& path) noexcept;

		String buildPath() const noexcept;

	private:
		String m_path;

	};


	class SLIB_EXPORT DisableWow64FsRedirectionScope
	{
	public:
		DisableWow64FsRedirectionScope() noexcept;

		~DisableWow64FsRedirectionScope() noexcept;

	private:
#ifdef SLIB_PLATFORM_IS_WIN32
		void* m_pOldValue;
#endif

	};

}

#endif
