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

#ifndef CHECKHEADER_SLIB_DOC_ZIP
#define CHECKHEADER_SLIB_DOC_ZIP

#include "definition.h"

#include "../core/string.h"
#include "../core/list.h"
#include "../core/memory.h"
#include "../core/time.h"
#include "../core/nullable.h"
#include "../io/io.h"

namespace slib
{

	enum class ZipCompressionMethod
	{
		Store = 0,
		Deflated = 8,
		Zstandard = 93
	};

	class SLIB_EXPORT ZipFileInfo
	{
	public:
		String filePath; // path in zip file
		Time lastModifiedTime;
		ZipCompressionMethod compressionMethod;
		Nullable<sl_int32> compressionLevel; // 0-9 (Deflate)
		StringParam password; // for archive

	public:
		ZipFileInfo();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(ZipFileInfo)

	};

	class SLIB_EXPORT ZipElement : public ZipFileInfo
	{
	public:
		Memory content;

	public:
		ZipElement();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(ZipElement)

	};

	class SLIB_EXPORT Zip
	{
	public:
		static Memory archive(const ListParam<ZipElement>& elements);

	};

}

#endif
