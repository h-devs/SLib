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

#ifndef CHECKHEADER_SLIB_DB_MYSQL_DATA
#define CHECKHEADER_SLIB_DB_MYSQL_DATA

#include "definition.h"

#include "../core/string.h"

namespace slib
{

	class SLIB_EXPORT MysqlData
	{
	public:
		// read functions returns the end position of the buffer on success, or returns null on error
		static sl_uint8* readVarchar(String& str, const void* buf, sl_size size);
		static sl_uint8* readDate(String& time, const void* buf, sl_size size);
		static sl_uint8* readDateTime(String& time, const void* buf, sl_size size);
		static sl_uint8* readBigInt(sl_int64& n, const void* buf, sl_size size);
		static sl_uint8* readInt(sl_int32& n, const void* buf, sl_size size);
		static sl_uint8* readMediumInt(sl_int32& n, const void* buf, sl_size size);
		static sl_uint8* readSmallInt(sl_int16& n, const void* buf, sl_size size);
		static sl_uint8* readTinyInt(sl_int8& n, const void* buf, sl_size size);

	};

#define SLIB_MYISAM_BLOCK_MIN_LENGTH 20
#define SLIB_MYISAM_BLOCK_ALIGN_CHECK 3

	class SLIB_EXPORT MyisamBlock
	{
	public:
		sl_uint32 length = 0;
		sl_uint32 lengthRecord = 0;
		sl_uint32 lengthData = 0;

		sl_uint32 startPos = 0;
		sl_int64 nextFilePos = -1;
		sl_int64 prevFilePos = -1;

		sl_bool flagDeleted = sl_false;
		sl_bool flagFirstBlock = sl_false;
		sl_bool flagLastBlock = sl_false;
		sl_bool flagSecondRead = sl_false;

	public:
		MyisamBlock();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(MyisamBlock)

	public:
		sl_bool read(const void* buf, sl_size size);

	};

	/*
		MyISAM Record Structure

		1) Empty(Zero) flags for no-char columns
		Size = (Number of no-char columns + 7) / 8
		2) Null flags for nullable columns
		Size = (Number of nullable columns + 7) / 8
		3) Column contents for non-nullable columns
	*/

}

#endif
