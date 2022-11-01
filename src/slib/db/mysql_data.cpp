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

#include "slib/db/mysql_data.h"

#include "slib/core/mio.h"
#include "slib/core/string.h"
#include "slib/core/variant.h"

namespace slib
{

	sl_uint8* MysqlData::readVarchar(String& str, const void* _buf, sl_size size)
	{
		if (!size) {
			return sl_null;
		}
		sl_uint8* buf = (sl_uint8*)_buf;
		sl_uint32 n = buf[0];
		if (n == 255) {
			if (size < 3) {
				return sl_null;
			}
			n = MIO::readUint16BE(buf + 1);
			if (3 + n > size) {
				return sl_null;
			}
			buf += 3;
		} else {
			if (1 + n > size) {
				return sl_null;
			}
			buf++;
		}
		for (sl_size i = 0; i < n; i++) {
			sl_uint8 ch = buf[i];
			if (ch < 0x20 && !(SLIB_CHAR_IS_WHITE_SPACE(ch))) {
				return sl_null;
			}
		}
		if (Charsets::checkUtf8(buf, n)) {
			str = String((char*)buf, n);
			return buf + n;
		} else {
			return sl_null;
		}
	}

	sl_uint8* MysqlData::readDate(String& time, const void* _buf, sl_size size)
	{
		sl_uint8* buf = (sl_uint8*)_buf;
		if (size < 3) {
			return sl_null;
		}
		sl_uint8 day = buf[0] & 0x1f;
		if (day > 31) {
			return sl_null;
		}
		sl_uint8 month = ((buf[1] & 1) << 3) | (buf[0] >> 5);
		if (month > 12) {
			return sl_null;
		}
		sl_uint32 year = ((sl_uint32)(buf[2]) << 7) | (buf[1] >> 1);
		time = String::format("%04d-%02d-%02d", year, month, day);
		return buf + 3;
	}

	sl_uint8* MysqlData::readDateTime(String& time, const void* _buf, sl_size size)
	{
		sl_uint8* buf = (sl_uint8*)_buf;
		if (size < 8) {
			return sl_null;
		}
		sl_uint64 t = MIO::readUint64LE(buf);
		sl_uint8 second = (sl_uint8)(t % 100);
		if (second >= 60) {
			return sl_null;
		}
		t /= 100;
		sl_uint8 minute = (sl_uint8)(t % 100);
		if (minute >= 60) {
			return sl_null;
		}
		t /= 100;
		sl_uint8 hour = (sl_uint8)(t % 100);
		if (hour >= 24) {
			return sl_null;
		}
		t /= 100;
		sl_uint8 day = (sl_uint8)(t % 100);
		if (day > 31) {
			return sl_null;
		}
		t /= 100;
		sl_uint8 month = (sl_uint8)(t % 100);
		if (month > 12) {
			return sl_null;
		}
		t /= 100;
		sl_uint32 year = (sl_uint32)t;
		time = String::format("%04d-%02d-%02d %02d:%02d:%02d", year, month, day, hour, minute, second);
		return buf + 8;
	}

	sl_uint8* MysqlData::readBigInt(sl_int64& n, const void* _buf, sl_size size)
	{
		sl_uint8* buf = (sl_uint8*)_buf;
		if (size < 8) {
			return sl_null;
		}
		n = MIO::readInt64LE(buf);
		return buf + 8;
	}

	sl_uint8* MysqlData::readInt(sl_int32& n, const void* _buf, sl_size size)
	{
		sl_uint8* buf = (sl_uint8*)_buf;
		if (size < 4) {
			return sl_null;
		}
		n = MIO::readUint32LE(buf);
		return buf + 4;
	}

	sl_uint8* MysqlData::readMediumInt(sl_int32& n, const void* _buf, sl_size size)
	{
		sl_uint8* buf = (sl_uint8*)_buf;
		if (size < 3) {
			return sl_null;
		}
		n = MIO::readInt24LE(buf);
		return buf + 3;
	}

	sl_uint8* MysqlData::readSmallInt(sl_int16& n, const void* _buf, sl_size size)
	{
		sl_uint8* buf = (sl_uint8*)_buf;
		if (size < 2) {
			return sl_null;
		}
		n = MIO::readInt16LE(buf);
		return buf + 2;
	}

	sl_uint8* MysqlData::readTinyInt(sl_int8& n, const void* _buf, sl_size size)
	{
		sl_uint8* buf = (sl_uint8*)_buf;
		if (size < 1) {
			return sl_null;
		}
		n = buf[0];
		return buf + 1;
	}

	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(MyisamBlock)

	MyisamBlock::MyisamBlock()
	{
		length = 0;
		lengthRecord = 0;
		lengthData = 0;

		startPos = 0;
		nextFilePos = -1;
		prevFilePos = -1;

		flagDeleted = sl_false;
		flagFirstBlock = sl_false;
		flagLastBlock = sl_false;
		flagSecondRead = sl_false;
	}

	sl_bool MyisamBlock::read(const void* _buf, sl_size size)
	{
		MyisamBlock& block = *this;
		sl_uint8* buf = (sl_uint8*)_buf;
		if (size < SLIB_MYISAM_BLOCK_MIN_LENGTH) {
			return sl_false;
		}
		sl_uint8 type = buf[0];
		switch (type) {
		case 0: // Deleted
			block.length = MIO::readUint24BE(buf + 1);
			block.nextFilePos = MIO::readUint64BE(buf + 4);
			block.prevFilePos = MIO::readUint64BE(buf + 12);
			block.flagDeleted = sl_true;
			break;
		case 1:
			block.length = MIO::readUint16BE(buf + 1);
			block.lengthData = block.length;
			block.lengthRecord = block.length;
			block.flagFirstBlock = sl_true;
			block.flagLastBlock = sl_true;
			block.startPos = 3;
			break;
		case 2:
			block.length = MIO::readUint24BE(buf + 1);
			block.lengthData = block.length;
			block.lengthRecord = block.length;
			block.flagFirstBlock = sl_true;
			block.flagLastBlock = sl_true;
			block.startPos = 4;
			break;
		case 3:
			block.lengthData = MIO::readUint16BE(buf + 1);
			block.lengthRecord = block.lengthData;
			block.length = block.lengthRecord + buf[3];
			block.flagFirstBlock = sl_true;
			block.flagLastBlock = sl_true;
			block.startPos = 4;
			break;
		case 4:
			block.lengthData = MIO::readUint24BE(buf + 1);
			block.lengthRecord = block.lengthData;
			block.length = block.lengthRecord + buf[4];
			block.flagFirstBlock = sl_true;
			block.flagLastBlock = sl_true;
			block.startPos = 5;
			break;
		case 5:
			block.lengthRecord = MIO::readUint16BE(buf + 1);
			block.length = MIO::readUint16BE(buf + 3);
			block.lengthData = block.length;
			block.nextFilePos = MIO::readUint64BE(buf + 5);
			block.flagFirstBlock = sl_true;
			block.flagSecondRead = sl_true;
			block.startPos = 13;
			break;
		case 6:
			block.lengthRecord = MIO::readUint24BE(buf + 1);
			block.length = MIO::readUint24BE(buf + 4);
			block.lengthData = block.length;
			block.nextFilePos = MIO::readUint64BE(buf + 7);
			block.flagFirstBlock = sl_true;
			block.flagSecondRead = sl_true;
			block.startPos = 15;
			break;
			// The following blocks are identical to 1-6 without lengthRecord
		case 7:
			block.length = MIO::readUint16BE(buf + 1);
			block.lengthData = block.length;
			block.flagLastBlock = sl_true;
			block.startPos = 3;
			break;
		case 8:
			block.length = MIO::readUint24BE(buf + 1);
			block.lengthData = block.length;
			block.flagLastBlock = sl_true;
			block.startPos = 4;
			break;
		case 9:
			block.lengthData = MIO::readUint16BE(buf + 1);
			block.length = block.lengthData + buf[3];
			block.flagLastBlock = sl_true;
			block.startPos = 4;
			break;
		case 10:
			block.lengthData = MIO::readUint24BE(buf + 1);
			block.length = block.lengthData + buf[4];
			block.flagLastBlock = sl_true;
			block.startPos = 5;
			break;
		case 11:
			block.length = MIO::readUint16BE(buf + 1);
			block.lengthData = block.length;
			block.nextFilePos = MIO::readUint64BE(buf + 3);
			block.startPos = 11;
			break;
		case 12:
			block.length = MIO::readUint24BE(buf + 1);
			block.lengthData = block.length;
			block.nextFilePos = MIO::readUint64BE(buf + 4);
			block.startPos = 12;
			break;
		case 13:
			block.lengthRecord = MIO::readUint32BE(buf + 1);
			block.length = MIO::readUint24BE(buf + 5);
			block.lengthData = block.length;
			block.nextFilePos = MIO::readUint64BE(buf + 8);
			block.flagFirstBlock = sl_true;
			block.flagSecondRead = sl_true;
			block.startPos = 16;
			break;
		}
		if ((block.startPos + block.length) & SLIB_MYISAM_BLOCK_ALIGN_CHECK) {
			return sl_false;
		}
		if (!(block.flagDeleted)) {
			if ((block.startPos + block.length) < SLIB_MYISAM_BLOCK_MIN_LENGTH) {
				return sl_false;
			}
		}
		return sl_true;
	}

}
