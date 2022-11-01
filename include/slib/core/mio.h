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

#ifndef CHECKHEADER_SLIB_CORE_MIO
#define CHECKHEADER_SLIB_CORE_MIO

#include "endian.h"

// MIO allows reading/writing from unaligned addresses

namespace slib
{

	class SLIB_EXPORT MIO
	{
	public:
		static void write8(void* dst, sl_uint8 value) noexcept
		{
			char* a = (char*)dst;
			char* b = (char*)(&value);
			a[0] = b[0];
		}

		static void write16(void* dst, sl_uint16 value) noexcept
		{
			char* a = (char*)dst;
			char* b = (char*)(&value);
			a[0] = b[0];
			a[1] = b[1];
		}

		static void write32(void* dst, sl_uint32 value) noexcept
		{
			char* a = (char*)dst;
			char* b = (char*)(&value);
			a[0] = b[0];
			a[1] = b[1];
			a[2] = b[2];
			a[3] = b[3];
		}

		static void write64(void* dst, sl_uint64 value) noexcept
		{
			char* a = (char*)dst;
			char* b = (char*)(&value);
			a[0] = b[0];
			a[1] = b[1];
			a[2] = b[2];
			a[3] = b[3];
			a[4] = b[4];
			a[5] = b[5];
			a[6] = b[6];
			a[7] = b[7];
		}

		static sl_uint8 read8(const void* src) noexcept
		{
			return *((char*)src);
		}

		static sl_uint16 read16(const void* src) noexcept
		{
			sl_uint16 value;
			char* a = (char*)(&value);
			char* b = (char*)(src);
			a[0] = b[0];
			a[1] = b[1];
			return value;
		}

		static sl_uint32 read32(const void* src) noexcept
		{
			sl_uint32 value;
			char* a = (char*)(&value);
			char* b = (char*)(src);
			a[0] = b[0];
			a[1] = b[1];
			a[2] = b[2];
			a[3] = b[3];
			return value;
		}

		static sl_uint64 read64(const void* src) noexcept
		{
			sl_uint64 value;
			char* a = (char*)(&value);
			char* b = (char*)(src);
			a[0] = b[0];
			a[1] = b[1];
			a[2] = b[2];
			a[3] = b[3];
			a[4] = b[4];
			a[5] = b[5];
			a[6] = b[6];
			a[7] = b[7];
			return value;
		}


		static sl_int8 readInt8(const void* src) noexcept
		{
			return read8(src);
		}

		static void writeInt8(void* dst, sl_int8 v) noexcept
		{
			write8(dst, v);
		}

		static sl_uint8 readUint8(const void* src) noexcept
		{
			return read8(src);
		}

		static void writeUint8(void* dst, sl_uint8 v) noexcept
		{
			write8(dst, v);
		}


		static sl_int16 readInt16LE(const void* src) noexcept
		{
			sl_uint8* s = (sl_uint8*)src;
			return ((sl_uint16)(s[0])) | ((sl_uint16)(s[1]) << 8);
		}

		static sl_int16 readInt16BE(const void* src) noexcept
		{
			sl_uint8* s = (sl_uint8*)src;
			return ((sl_uint16)(s[0]) << 8) | ((sl_uint16)(s[1]));
		}

		static sl_int16 readInt16(const void* src) noexcept
		{
			return readInt16LE(src);
		}

		static sl_int16 readInt16(const void* src, EndianType endian) noexcept
		{
			if (endian == Endian::Big) {
				return readInt16BE(src);
			} else {
				return readInt16LE(src);
			}
		}

		static void writeInt16LE(void* dst, sl_int16 _v) noexcept
		{
			sl_uint16 v = _v;
			sl_uint8* d = (sl_uint8*)dst;
			d[0] = (sl_uint8)(v);
			d[1] = (sl_uint8)(v >> 8);
		}

		static void writeInt16BE(void* dst, sl_int16 _v) noexcept
		{
			sl_uint16 v = _v;
			sl_uint8* d = (sl_uint8*)dst;
			d[0] = (sl_uint8)(v >> 8);
			d[1] = (sl_uint8)(v);
		}

		static void writeInt16(void* dst, sl_int16 v) noexcept
		{
			writeInt16LE(dst, v);
		}

		static void writeInt16(void* dst, sl_int16 v, EndianType endian) noexcept
		{
			if (endian == Endian::Big) {
				writeInt16BE(dst, v);
			} else {
				writeInt16LE(dst, v);
			}
		}


		static sl_uint16 readUint16LE(const void* src) noexcept
		{
			sl_uint8* s = (sl_uint8*)src;
			return ((sl_uint16)(s[0])) | ((sl_uint16)(s[1]) << 8);
		}

		static sl_uint16 readUint16BE(const void* src) noexcept
		{
			sl_uint8* s = (sl_uint8*)src;
			return ((sl_uint16)(s[0]) << 8) | ((sl_uint16)(s[1]));
		}

		static sl_uint16 readUint16(const void* src) noexcept
		{
			return readUint16LE(src);
		}

		static sl_uint16 readUint16(const void* src, EndianType endian) noexcept
		{
			if (endian == Endian::Big) {
				return readUint16BE(src);
			} else {
				return readUint16LE(src);
			}
		}

		static void writeUint16LE(void* dst, sl_uint16 v) noexcept
		{
			sl_uint8* d = (sl_uint8*)dst;
			d[0] = (sl_uint8)(v);
			d[1] = (sl_uint8)(v >> 8);
		}

		static void writeUint16BE(void* dst, sl_uint16 v) noexcept
		{
			sl_uint8* d = (sl_uint8*)dst;
			d[0] = (sl_uint8)(v >> 8);
			d[1] = (sl_uint8)(v);
		}

		static void writeUint16(void* dst, sl_uint16 v) noexcept
		{
			writeUint16LE(dst, v);
		}

		static void writeUint16(void* dst, sl_uint16 v, EndianType endian) noexcept
		{
			if (endian == Endian::Big) {
				writeUint16BE(dst, v);
			} else {
				writeUint16LE(dst, v);
			}
		}


		static sl_int32 readInt24LE(const void* src) noexcept
		{
			sl_uint8* s = (sl_uint8*)src;
			sl_uint32 n = ((sl_uint32)(s[0])) | ((sl_uint32)(s[1]) << 8) | ((sl_uint32)(s[2]) << 16);
			if (n & 0x800000) {
				return n | 0xff000000;
			} else {
				return n;
			}
		}

		static sl_int32 readInt24BE(const void* src) noexcept
		{
			sl_uint8* s = (sl_uint8*)src;
			sl_uint32 n = ((sl_uint32)(s[0]) << 16) | ((sl_uint32)(s[1]) << 8) | ((sl_uint32)(s[2]));
			if (n & 0x800000) {
				return n | 0xff000000;
			} else {
				return n;
			}
		}

		static sl_int32 readInt24(const void* src) noexcept
		{
			return readInt24LE(src);
		}

		static sl_uint32 readInt24(const void* src, EndianType endian) noexcept
		{
			if (endian == Endian::Big) {
				return readInt24BE(src);
			} else {
				return readInt24LE(src);
			}
		}

		static void writeInt24LE(void* dst, sl_int32 v) noexcept
		{
			sl_uint8* d = (sl_uint8*)dst;
			d[0] = (sl_uint8)(v);
			d[1] = (sl_uint8)(v >> 8);
			d[2] = (sl_uint8)(v >> 16);
		}

		static void writeInt24BE(void* dst, sl_int32 v) noexcept
		{
			sl_uint8* d = (sl_uint8*)dst;
			d[0] = (sl_uint8)(v >> 16);
			d[1] = (sl_uint8)(v >> 8);
			d[2] = (sl_uint8)(v);
		}

		static void writeUint24(void* dst, sl_int32 v) noexcept
		{
			writeInt24LE(dst, v);
		}

		static void writeUint24(void* dst, sl_int32 v, EndianType endian) noexcept
		{
			if (endian == Endian::Big) {
				writeInt24BE(dst, v);
			} else {
				writeInt24LE(dst, v);
			}
		}


		static sl_uint32 readUint24LE(const void* src) noexcept
		{
			sl_uint8* s = (sl_uint8*)src;
			return ((sl_uint32)(s[0])) | ((sl_uint32)(s[1]) << 8) | ((sl_uint32)(s[2]) << 16);
		}

		static sl_uint32 readUint24BE(const void* src) noexcept
		{
			sl_uint8* s = (sl_uint8*)src;
			return ((sl_uint32)(s[0]) << 16) | ((sl_uint32)(s[1]) << 8) | ((sl_uint32)(s[2]));
		}

		static sl_uint32 readUint24(const void* src) noexcept
		{
			return readUint24LE(src);
		}

		static sl_uint32 readUint24(const void* src, EndianType endian) noexcept
		{
			if (endian == Endian::Big) {
				return readUint24BE(src);
			} else {
				return readUint24LE(src);
			}
		}

		static void writeUint24LE(void* dst, sl_uint32 v) noexcept
		{
			sl_uint8* d = (sl_uint8*)dst;
			d[0] = (sl_uint8)(v);
			d[1] = (sl_uint8)(v >> 8);
			d[2] = (sl_uint8)(v >> 16);
		}

		static void writeUint24BE(void* dst, sl_uint32 v) noexcept
		{
			sl_uint8* d = (sl_uint8*)dst;
			d[0] = (sl_uint8)(v >> 16);
			d[1] = (sl_uint8)(v >> 8);
			d[2] = (sl_uint8)(v);
		}

		static void writeUint24(void* dst, sl_uint32 v) noexcept
		{
			writeUint24LE(dst, v);
		}

		static void writeUint24(void* dst, sl_uint32 v, EndianType endian) noexcept
		{
			if (endian == Endian::Big) {
				writeUint24BE(dst, v);
			} else {
				writeUint24LE(dst, v);
			}
		}


		static sl_int32 readInt32LE(const void* src) noexcept
		{
			sl_uint8* s = (sl_uint8*)src;
			return ((sl_uint32)(s[0])) | ((sl_uint32)(s[1]) << 8) | ((sl_uint32)(s[2]) << 16) | ((sl_uint32)(s[3]) << 24);
		}

		static sl_int32 readInt32BE(const void* src) noexcept
		{
			sl_uint8* s = (sl_uint8*)src;
			return ((sl_uint32)(s[0]) << 24) | ((sl_uint32)(s[1]) << 16) | ((sl_uint32)(s[2]) << 8) | ((sl_uint32)(s[3]));
		}

		static sl_int32 readInt32(const void* src) noexcept
		{
			return readInt32LE(src);
		}

		static sl_int32 readInt32(const void* src, EndianType endian) noexcept
		{
			if (endian == Endian::Big) {
				return readInt32BE(src);
			} else {
				return readInt32LE(src);
			}
		}

		static void writeInt32LE(void* dst, sl_int32 _v) noexcept
		{
			sl_uint32 v = _v;
			sl_uint8* d = (sl_uint8*)dst;
			d[0] = (sl_uint8)(v);
			d[1] = (sl_uint8)(v >> 8);
			d[2] = (sl_uint8)(v >> 16);
			d[3] = (sl_uint8)(v >> 24);
		}

		static void writeInt32BE(void* dst, sl_int32 _v) noexcept
		{
			sl_uint32 v = _v;
			sl_uint8* d = (sl_uint8*)dst;
			d[0] = (sl_uint8)(v >> 24);
			d[1] = (sl_uint8)(v >> 16);
			d[2] = (sl_uint8)(v >> 8);
			d[3] = (sl_uint8)(v);
		}

		static void writeInt32(void* dst, sl_int32 v) noexcept
		{
			writeInt32LE(dst, v);
		}

		static void writeInt32(void* dst, sl_int32 v, EndianType endian) noexcept
		{
			if (endian == Endian::Big) {
				writeInt32BE(dst, v);
			} else {
				writeInt32LE(dst, v);
			}
		}


		static sl_uint32 readUint32LE(const void* src) noexcept
		{
			sl_uint8* s = (sl_uint8*)src;
			return ((sl_uint32)(s[0])) | ((sl_uint32)(s[1]) << 8) | ((sl_uint32)(s[2]) << 16) | ((sl_uint32)(s[3]) << 24);
		}

		static sl_uint32 readUint32BE(const void* src) noexcept
		{
			sl_uint8* s = (sl_uint8*)src;
			return ((sl_uint32)(s[0]) << 24) | ((sl_uint32)(s[1]) << 16) | ((sl_uint32)(s[2]) << 8) | ((sl_uint32)(s[3]));
		}

		static sl_uint32 readUint32(const void* src) noexcept
		{
			return readUint32LE(src);
		}

		static sl_uint32 readUint32(const void* src, EndianType endian) noexcept
		{
			if (endian == Endian::Big) {
				return readUint32BE(src);
			} else {
				return readUint32LE(src);
			}
		}

		static void writeUint32LE(void* dst, sl_uint32 v) noexcept
		{
			sl_uint8* d = (sl_uint8*)dst;
			d[0] = (sl_uint8)(v);
			d[1] = (sl_uint8)(v >> 8);
			d[2] = (sl_uint8)(v >> 16);
			d[3] = (sl_uint8)(v >> 24);
		}

		static void writeUint32BE(void* dst, sl_uint32 v) noexcept
		{
			sl_uint8* d = (sl_uint8*)dst;
			d[0] = (sl_uint8)(v >> 24);
			d[1] = (sl_uint8)(v >> 16);
			d[2] = (sl_uint8)(v >> 8);
			d[3] = (sl_uint8)(v);
		}

		static void writeUint32(void* dst, sl_uint32 v) noexcept
		{
			writeUint32LE(dst, v);
		}

		static void writeUint32(void* dst, sl_uint32 v, EndianType endian) noexcept
		{
			if (endian == Endian::Big) {
				writeUint32BE(dst, v);
			} else {
				writeUint32LE(dst, v);
			}
		}


		static sl_int64 readInt64LE(const void* src) noexcept
		{
			sl_uint8* s = (sl_uint8*)src;
			return ((sl_uint64)(s[0])) | ((sl_uint64)(s[1]) << 8) | ((sl_uint64)(s[2]) << 16) | ((sl_uint64)(s[3]) << 24)
				| ((sl_uint64)(s[4]) << 32) | ((sl_uint64)(s[5]) << 40) | ((sl_uint64)(s[6]) << 48) | ((sl_uint64)(s[7]) << 56);
		}

		static sl_int64 readInt64BE(const void* src) noexcept
		{
			sl_uint8* s = (sl_uint8*)src;
			return ((sl_uint64)(s[0]) << 56) | ((sl_uint64)(s[1]) << 48) | ((sl_uint64)(s[2]) << 40) | ((sl_uint64)(s[3]) << 32)
				| ((sl_uint64)(s[4]) << 24) | ((sl_uint64)(s[5]) << 16) | ((sl_uint64)(s[6]) << 8) | ((sl_uint64)(s[7]));
		}

		static sl_int64 readInt64(const void* src) noexcept
		{
			return readInt64LE(src);
		}

		static sl_int64 readInt64(const void* src, EndianType endian) noexcept
		{
			if (endian == Endian::Big) {
				return readInt64BE(src);
			} else {
				return readInt64LE(src);
			}
		}


		static void writeInt64LE(void* dst, sl_int64 _v) noexcept
		{
			sl_uint64 v = _v;
			sl_uint8* d = (sl_uint8*)dst;
			d[0] = (sl_uint8)(v);
			d[1] = (sl_uint8)(v >> 8);
			d[2] = (sl_uint8)(v >> 16);
			d[3] = (sl_uint8)(v >> 24);
			d[4] = (sl_uint8)(v >> 32);
			d[5] = (sl_uint8)(v >> 40);
			d[6] = (sl_uint8)(v >> 48);
			d[7] = (sl_uint8)(v >> 56);
		}

		static void writeInt64BE(void* dst, sl_int64 _v) noexcept
		{
			sl_uint64 v = _v;
			sl_uint8* d = (sl_uint8*)dst;
			d[0] = (sl_uint8)(v >> 56);
			d[1] = (sl_uint8)(v >> 48);
			d[2] = (sl_uint8)(v >> 40);
			d[3] = (sl_uint8)(v >> 32);
			d[4] = (sl_uint8)(v >> 24);
			d[5] = (sl_uint8)(v >> 16);
			d[6] = (sl_uint8)(v >> 8);
			d[7] = (sl_uint8)(v);
		}

		static void writeInt64(void* dst, sl_int64 v) noexcept
		{
			writeInt64LE(dst, v);
		}

		static void writeInt64(void* dst, sl_int64 v, EndianType endian) noexcept
		{
			if (endian == Endian::Big) {
				writeInt64BE(dst, v);
			} else {
				writeInt64LE(dst, v);
			}
		}


		static sl_uint64 readUint64LE(const void* src) noexcept
		{
			sl_uint8* s = (sl_uint8*)src;
			return ((sl_uint64)(s[0])) | ((sl_uint64)(s[1]) << 8) | ((sl_uint64)(s[2]) << 16) | ((sl_uint64)(s[3]) << 24)
				| ((sl_uint64)(s[4]) << 32) | ((sl_uint64)(s[5]) << 40) | ((sl_uint64)(s[6]) << 48) | ((sl_uint64)(s[7]) << 56);
		}

		static sl_uint64 readUint64BE(const void* src) noexcept
		{
			sl_uint8* s = (sl_uint8*)src;
			return ((sl_uint64)(s[0]) << 56) | ((sl_uint64)(s[1]) << 48) | ((sl_uint64)(s[2]) << 40) | ((sl_uint64)(s[3]) << 32)
				| ((sl_uint64)(s[4]) << 24) | ((sl_uint64)(s[5]) << 16) | ((sl_uint64)(s[6]) << 8) | ((sl_uint64)(s[7]));
		}

		static sl_uint64 readUint64(const void* src) noexcept
		{
			return readUint64LE(src);
		}

		static sl_uint64 readUint64(const void* src, EndianType endian) noexcept
		{
			if (endian == Endian::Big) {
				return readUint64BE(src);
			} else {
				return readUint64LE(src);
			}
		}

		static void writeUint64LE(void* dst, sl_uint64 v) noexcept
		{
			sl_uint8* d = (sl_uint8*)dst;
			d[0] = (sl_uint8)(v);
			d[1] = (sl_uint8)(v >> 8);
			d[2] = (sl_uint8)(v >> 16);
			d[3] = (sl_uint8)(v >> 24);
			d[4] = (sl_uint8)(v >> 32);
			d[5] = (sl_uint8)(v >> 40);
			d[6] = (sl_uint8)(v >> 48);
			d[7] = (sl_uint8)(v >> 56);
		}

		static void writeUint64BE(void* dst, sl_uint64 v) noexcept
		{
			sl_uint8* d = (sl_uint8*)dst;
			d[0] = (sl_uint8)(v >> 56);
			d[1] = (sl_uint8)(v >> 48);
			d[2] = (sl_uint8)(v >> 40);
			d[3] = (sl_uint8)(v >> 32);
			d[4] = (sl_uint8)(v >> 24);
			d[5] = (sl_uint8)(v >> 16);
			d[6] = (sl_uint8)(v >> 8);
			d[7] = (sl_uint8)(v);
		}

		static void writeUint64(void* dst, sl_uint64 v) noexcept
		{
			writeUint64LE(dst, v);
		}

		static void writeUint64(void* dst, sl_uint64 v, EndianType endian) noexcept
		{
			if (endian == Endian::Big) {
				writeUint64BE(dst, v);
			} else {
				writeUint64LE(dst, v);
			}
		}


		static float readFloatLE(const void* src) noexcept
		{
			sl_uint32 v = readUint32LE(src);
			return *((float*)&v);
		}

		static float readFloatBE(const void* src) noexcept
		{
			sl_uint32 v = readUint32BE(src);
			return *((float*)&v);
		}

		static float readFloat(const void* src) noexcept
		{
			return readFloatLE(src);
		}

		static float readFloat(const void* src, EndianType endian) noexcept
		{
			if (endian == Endian::Big) {
				return readFloatBE(src);
			} else {
				return readFloatLE(src);
			}
		}

		static void writeFloatLE(void* dst, float v) noexcept
		{
			writeUint32LE(dst, *((sl_uint32*)&v));
		}

		static void writeFloatBE(void* dst, float v) noexcept
		{
			writeUint32BE(dst, *((sl_uint32*)&v));
		}

		static void writeFloat(void* dst, float v) noexcept
		{
			writeFloatLE(dst, v);
		}

		static void writeFloat(void* dst, float v, EndianType endian) noexcept
		{
			if (endian == Endian::Big) {
				writeFloatBE(dst, v);
			} else {
				writeFloatLE(dst, v);
			}
		}


		static double readDoubleLE(const void* src) noexcept
		{
			sl_uint64 v = readUint64LE(src);
			return *((double*)&v);
		}

		static double readDoubleBE(const void* src) noexcept
		{
			sl_uint64 v = readUint64BE(src);
			return *((double*)&v);
		}

		static double readDouble(const void* src) noexcept
		{
			return readDoubleLE(src);
		}

		static double readDouble(const void* src, EndianType endian) noexcept
		{
			if (endian == Endian::Big) {
				return readDoubleBE(src);
			} else {
				return readDoubleLE(src);
			}
		}

		static void writeDoubleLE(void* dst, double v) noexcept
		{
			writeUint64LE(dst, *((sl_uint64*)&v));
		}

		static void writeDoubleBE(void* dst, double v) noexcept
		{
			writeUint64BE(dst, *((sl_uint64*)&v));
		}

		static void writeDouble(void* dst, double v) noexcept
		{
			writeDoubleLE(dst, v);
		}

		static void writeDouble(void* dst, double v, EndianType endian) noexcept
		{
			if (endian == Endian::Big) {
				writeDoubleBE(dst, v);
			} else {
				writeDoubleLE(dst, v);
			}
		}


		static void increaseBE(void* _p, sl_size n) noexcept
		{
			sl_uint8* p = ((sl_uint8*)_p) + n - 1;
			while (n > 0) {
				if (++(*p)) {
					break;
				}
				p--;
				n--;
			}
		}

		static void increaseLE(void* _p, sl_size n) noexcept
		{
			sl_uint8* p = ((sl_uint8*)_p);
			while (n > 0) {
				if (++(*p)) {
					break;
				}
				p++;
				n--;
			}
		}

	};

}

#endif
