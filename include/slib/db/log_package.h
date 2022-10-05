/*
*   Copyright (c) 2008-2022 SLIBIO <https://github.com/SLIBIO>
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

#ifndef CHECKHEADER_SLIB_DB_LOG_PACKAGE_HEADER
#define CHECKHEADER_SLIB_DB_LOG_PACKAGE_HEADER

#include "definition.h"

#include "../core/string.h"
#include "../core/file.h"
#include "../core/memory.h"
#include "../core/list.h"
#include "../core/pair.h"

namespace slib
{

	class LogPackageAppender
	{
	public:
		LogPackageAppender();

		~LogPackageAppender();

	public:
		sl_bool open(const StringParam& pathContent);

		sl_bool open(const StringParam& pathContent, const StringParam& pathIndex);

		sl_bool appendRecord(sl_uint64 id, const MemoryView& content);

	protected:
		File m_fileContent;
		File m_fileIndex;

	};

	class LogPackageReader
	{
	public:
		LogPackageReader();

		~LogPackageReader();

	public:
		sl_bool open(const StringParam& pathContent);

		sl_bool open(const StringParam& pathContent, const StringParam& pathIndex);

		Memory readRecord(sl_uint64 id, sl_size maxSize = SLIB_SIZE_MAX);

		List< Pair<sl_uint64, Memory> > readRecords(sl_uint64 startId, sl_uint64 endId, sl_size maxSize = SLIB_SIZE_MAX);

	protected:
		Memory _readRecord(sl_uint64 position, sl_size size);

	protected:
		File m_fileContent;

		struct Index
		{
			sl_uint64 position;
			sl_uint64 size;
			sl_uint64 id;
		};
		Index* m_indices;
		sl_size m_nIndices;

	};

}

#endif