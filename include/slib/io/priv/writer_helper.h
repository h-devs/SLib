/*
 *   Copyright (c) 2008-2024 SLIBIO <https://github.com/SLIBIO>
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

#ifndef CHECKHEADER_SLIB_IO_PRIV_WRITER_HELPER
#define CHECKHEADER_SLIB_IO_PRIV_WRITER_HELPER

#include "../../core/thread.h"
#include "../../core/timeout.h"

namespace slib
{

	class WriterHelper
	{
	public:
		template <class WRITER>
		static sl_reg writeWithWrite32(WRITER* writer, const void* buf, sl_size size, sl_int32 timeout = -1)
		{
#if defined(SLIB_ARCH_IS_64BIT)
			if (size >> 31) {
				return writer->write32(buf, 0x40000000, timeout); // 1GB
			}
#endif
			return writer->write32(buf, (sl_uint32)size, timeout);
		}

		template <class WRITER>
		static sl_reg writeFully(WRITER* writer, const void* _buf, sl_size size, sl_int32 timeout = -1)
		{
			sl_uint8* buf = (sl_uint8*)_buf;
			if (!size) {
				return writer->write(buf, 0, timeout);
			}
			sl_int64 tickEnd = GetTickFromTimeout(timeout);
			sl_size nWrite = 0;
			CurrentThread thread;
			do {
				sl_reg m = writer->write(buf, size, timeout);
				if (m > 0) {
					nWrite += m;
					if (size <= (sl_uint32)m) {
						return nWrite;
					}
					buf += m;
					size -= m;
					timeout = GetTimeoutFromTick(tickEnd);
				} else if (m == SLIB_IO_WOULD_BLOCK || m == SLIB_IO_ENDED) {
					if (nWrite) {
						return nWrite;
					} else {
						return m;
					}
				} else {
					return m;
				}
			} while (thread.isNotStopping());
			return SLIB_IO_ERROR;
		}

		template <class WRITER>
		static sl_bool writeInt8(WRITER* writer, sl_int8 value)
		{
			return writeFully(writer, &value, 1) == 1;
		}

		template <class WRITER>
		static sl_bool writeInt16(WRITER* writer, sl_int16 value, EndianType endian)
		{
			if (endian == Endian::Big) {
				value = Endian::swap16LE(value);
			} else {
				value = Endian::swap16BE(value);
			}
			return writeFully(writer, &value, 2) == 2;
		}

		template <class WRITER>
		static sl_bool writeInt32(WRITER* writer, sl_int32 value, EndianType endian)
		{
			if (endian == Endian::Big) {
				value = Endian::swap32LE(value);
			} else {
				value = Endian::swap32BE(value);
			}
			return writeFully(writer, &value, 4) == 4;
		}

		template <class WRITER>
		static sl_bool writeInt64(WRITER* writer, sl_int64 value, EndianType endian)
		{
			if (endian == Endian::Big) {
				value = Endian::swap64LE(value);
			} else {
				value = Endian::swap64BE(value);
			}
			return writeFully(writer, &value, 8) == 8;
		}

		template <class WRITER>
		static sl_bool writeFloat(WRITER* writer, float value, EndianType endian)
		{
			if (endian == Endian::Big) {
				value = Endian::swapFloatLE(value);
			} else {
				value = Endian::swapFloatBE(value);
			}
			return writeFully(writer, &value, 4) == 4;
		}

		template <class WRITER>
		static sl_bool writeDouble(WRITER* writer, double value, EndianType endian)
		{
			if (endian == Endian::Big) {
				value = Endian::swapDoubleLE(value);
			} else {
				value = Endian::swapDoubleBE(value);
			}
			return writeFully(writer, &value, 8) == 8;
		}
	};

	class BlockWriterHelper
	{
	public:
		template <class WRITER>
		static sl_reg writeAtWithWriteAt32(WRITER* writer, sl_uint64 offset, const void* buf, sl_size size, sl_int32 timeout = -1)
		{
#if defined(SLIB_ARCH_IS_64BIT)
			if (size >> 31) {
				return writer->writeAt32(offset, buf, 0x40000000, timeout);
			}
#endif
			return writer->writeAt32(offset, buf, (sl_uint32)size, timeout);
		}

		template <class WRITER>
		static sl_reg writeFullyAt(WRITER* writer, sl_uint64 offset, const void* _buf, sl_size size, sl_int32 timeout = -1)
		{
			sl_uint8* buf = (sl_uint8*)_buf;
			if (!size) {
				return writer->writeAt(offset, buf, 0, timeout);
			}
			sl_int64 tickEnd = GetTickFromTimeout(timeout);
			sl_size nWrite = 0;
			CurrentThread thread;
			do {
				sl_reg m = writer->writeAt(offset, buf, size, timeout);
				if (m > 0) {
					nWrite += m;
					if (size <= (sl_uint32)m) {
						return nWrite;
					}
					offset += m;
					buf += m;
					size -= m;
					timeout = GetTimeoutFromTick(tickEnd);
				} else if (m == SLIB_IO_WOULD_BLOCK || m == SLIB_IO_ENDED) {
					if (nWrite) {
						return nWrite;
					} else {
						return m;
					}
				} else {
					return m;
				}
			} while (thread.isNotStopping());
			return SLIB_IO_ERROR;
		}

	};

}

#endif
