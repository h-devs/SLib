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

#ifndef CHECKHEADER_SLIB_IO_PRIV_READER_HELPER
#define CHECKHEADER_SLIB_IO_PRIV_READER_HELPER

#include "../../core/mio.h"
#include "../../core/thread.h"
#include "../../core/memory_buffer.h"

namespace slib
{

	class ReaderHelper
	{
	public:
		template <class READER>
		static sl_reg readWithRead32(READER* reader, void* buf, sl_size size)
		{
#if !defined(SLIB_ARCH_IS_64BIT)
			return reader->read32(buf, (sl_uint32)size);
#else
			if (size >> 31) {
				return reader->read32(buf, 0x40000000); // 1GB
			} else {
				return reader->read32(buf, (sl_uint32)size);
			}
#endif
		}

		template <class READER>
		static sl_reg readFully(READER* reader, void* _buf, sl_size size)
		{
			sl_uint8* buf = (sl_uint8*)_buf;
			if (!size) {
				return reader->read(buf, 0);
			}
			sl_size nRead = 0;
			CurrentThread thread;
			for (;;) {
				sl_reg m = reader->read(buf, size);
				if (m > 0) {
					nRead += m;
					if (size <= (sl_size)m) {
						return nRead;
					}
					buf += m;
					size -= m;
				} else if (m == SLIB_IO_WOULD_BLOCK) {
					reader->waitRead();
				} else if (m == SLIB_IO_ENDED) {
					return nRead;
				} else {
					return m;
				}
				if (thread.isStopping()) {
					return SLIB_IO_ERROR;
				}
			}
		}

		template <class READER>
		static Memory readFully(READER* reader)
		{
			MemoryBuffer mb;
			char buf[1024];
			CurrentThread thread;
			for (;;) {
				sl_reg m = reader->read(buf, sizeof(buf));
				if (m > 0) {
					if (!(mb.addNew(buf, m))) {
						return sl_null;
					}
				} else if (m == SLIB_IO_ENDED) {
					return mb.merge();
				} else if (m == SLIB_IO_WOULD_BLOCK) {
					reader->waitRead();
				} else {
					return sl_null;
				}
				if (thread.isStopping()) {
					return sl_null;
				}
			}
		}

		template <class READER>
		static sl_bool readInt8(READER* reader, sl_int8* output)
		{
			if (readFully(reader, output, 1) == 1) {
				return sl_true;
			} else {
				return sl_false;
			}
		}

		template <class READER>
		static sl_int8 readInt8(READER* reader, sl_int8 def)
		{
			sl_int8 ret;
			if (readInt8(reader, &ret)) {
				return ret;
			} else {
				return def;
			}
		}

		template <class READER>
		static sl_bool readUint8(READER* reader, sl_uint8* output)
		{
			if (readFully(reader, output, 1) == 1) {
				return sl_true;
			} else {
				return sl_false;
			}
		}

		template <class READER>
		static sl_uint8 readUint8(READER* reader, sl_uint8 def)
		{
			sl_uint8 ret;
			if (readUint8(reader, &ret)) {
				return ret;
			} else {
				return def;
			}
		}

		template <class READER>
		static sl_bool readInt16(READER* reader, sl_int16* output, EndianType endian)
		{
			if (readFully(reader, output, 2) == 2) {
				if (output) {
					if (endian == Endian::Big) {
						*output = Endian::swap16LE(*output);
					} else {
						*output = Endian::swap16BE(*output);
					}
				}
				return sl_true;
			} else {
				return sl_false;
			}
		}

		template <class READER>
		static sl_int16 readInt16(READER* reader, sl_int16 def, EndianType endian)
		{
			sl_int16 ret;
			if (readInt16(reader, &ret, endian)) {
				return ret;
			} else {
				return def;
			}
		}

		template <class READER>
		static sl_bool readUint16(READER* reader, sl_uint16* output, EndianType endian)
		{
			if (readFully(reader, output, 2) == 2) {
				if (output) {
					if (endian == Endian::Big) {
						*output = Endian::swap16LE(*output);
					} else {
						*output = Endian::swap16BE(*output);
					}
				}
				return sl_true;
			} else {
				return sl_false;
			}
		}

		template <class READER>
		static sl_uint16 readUint16(READER* reader, sl_uint16 def, EndianType endian)
		{
			sl_uint16 ret;
			if (readUint16(reader, &ret, endian)) {
				return ret;
			} else {
				return def;
			}
		}

		template <class READER>
		static sl_bool readInt32(READER* reader, sl_int32* output, EndianType endian)
		{
			if (readFully(reader, output, 4) == 4) {
				if (output) {
					if (endian == Endian::Big) {
						*output = Endian::swap32LE(*output);
					} else {
						*output = Endian::swap32BE(*output);
					}
				}
				return sl_true;
			} else {
				return sl_false;
			}
		}

		template <class READER>
		static sl_int32 readInt32(READER* reader, sl_int32 def, EndianType endian)
		{
			sl_int32 ret;
			if (readInt32(reader, &ret, endian)) {
				return ret;
			} else {
				return def;
			}
		}

		template <class READER>
		static sl_bool readUint32(READER* reader, sl_uint32* output, EndianType endian)
		{
			if (readFully(reader, output, 4) == 4) {
				if (output) {
					if (endian == Endian::Big) {
						*output = Endian::swap32LE(*output);
					} else {
						*output = Endian::swap32BE(*output);
					}
				}
				return sl_true;
			} else {
				return sl_false;
			}
		}

		template <class READER>
		static sl_uint32 readUint32(READER* reader, sl_uint32 def, EndianType endian)
		{
			sl_uint32 ret;
			if (readUint32(reader, &ret, endian)) {
				return ret;
			} else {
				return def;
			}
		}

		template <class READER>
		static sl_bool readInt64(READER* reader, sl_int64* output, EndianType endian)
		{
			if (readFully(reader, output, 8) == 8) {
				if (output) {
					if (endian == Endian::Big) {
						*output = Endian::swap64LE(*output);
					} else {
						*output = Endian::swap64BE(*output);
					}
				}
				return sl_true;
			} else {
				return sl_false;
			}
		}

		template <class READER>
		static sl_int64 readInt64(READER* reader, sl_int64 def, EndianType endian)
		{
			sl_int64 ret;
			if (readInt64(reader, &ret, endian)) {
				return ret;
			} else {
				return def;
			}
		}

		template <class READER>
		static sl_bool readUint64(READER* reader, sl_uint64* output, EndianType endian)
		{
			if (readFully(reader, output, 8) == 8) {
				if (output) {
					if (endian == Endian::Big) {
						*output = Endian::swap64LE(*output);
					} else {
						*output = Endian::swap64BE(*output);
					}
				}
				return sl_true;
			} else {
				return sl_false;
			}
		}

		template <class READER>
		static sl_uint64 readUint64(READER* reader, sl_uint64 def, EndianType endian)
		{
			sl_uint64 ret;
			if (readUint64(reader, &ret, endian)) {
				return ret;
			} else {
				return def;
			}
		}

		template <class READER>
		static sl_bool readFloat(READER* reader, float* output, EndianType endian)
		{
			if (readFully(reader, output, 4) == 4) {
				if (output) {
					if (endian == Endian::Big) {
						*output = Endian::swapFloatLE(*output);
					} else {
						*output = Endian::swapFloatBE(*output);
					}
				}
				return sl_true;
			} else {
				return sl_false;
			}
		}

		template <class READER>
		static float readFloat(READER* reader, float def, EndianType endian)
		{
			float ret;
			if (readFloat(reader, &ret, endian)) {
				return ret;
			} else {
				return def;
			}
		}

		template <class READER>
		static sl_bool readDouble(READER* reader, double* output, EndianType endian)
		{
			if (readFully(reader, output, 8) == 8) {
				if (output) {
					if (endian == Endian::Big) {
						*output = Endian::swapDoubleLE(*output);
					} else {
						*output = Endian::swapDoubleBE(*output);
					}
				}
				return sl_true;
			} else {
				return sl_false;
			}
		}

		template <class READER>
		static double readDouble(READER* reader, double def, EndianType endian)
		{
			double ret;
			if (readDouble(reader, &ret, endian)) {
				return ret;
			} else {
				return def;
			}
		}

		template <class READER>
		static Memory readToMemory(READER* reader, sl_size size)
		{
			Memory mem = Memory::create(size);
			if (mem.isNotNull()) {
				sl_reg nRead = readFully(reader, mem.getData(), size);
				if (nRead == size) {
					return mem;
				} else if (nRead > 0) {
					return mem.sub(0, nRead);
				}
			}
			return sl_null;
		}

	};

	class BlockReaderHelper
	{
	public:
		template <class READER>
		static sl_reg readAtWithReadAt32(READER* reader, sl_uint64 offset, void* buf, sl_size size)
		{
#if !defined(SLIB_ARCH_IS_64BIT)
			return reader->readAt32(offset, buf, (sl_uint32)size);
#else
			if (size >> 31) {
				return reader->readAt32(offset, buf, 0x40000000);
			} else {
				return reader->readAt32(offset, buf, (sl_uint32)size);
			}
#endif
		}

		template <class READER>
		static sl_reg readFullyAt(READER* reader, sl_uint64 offset, void* _buf, sl_size size)
		{
			sl_uint8* buf = (sl_uint8*)_buf;
			if (!size) {
				return reader->readAt(offset, buf, 0);
			}
			sl_size nRead = 0;
			CurrentThread thread;
			for (;;) {
				sl_reg m = reader->readAt(offset, buf, size);
				if (m > 0) {
					nRead += m;
					if (size <= (sl_size)m) {
						return nRead;
					}
					buf += m;
					offset += m;
					size -= m;
				} else if (m == SLIB_IO_WOULD_BLOCK) {
					reader->waitRead();
				} else if (m == SLIB_IO_ENDED) {
					return nRead;
				} else {
					return m;
				}
				if (thread.isStopping()) {
					return SLIB_IO_ERROR;
				}
			}
		}

	};

}

#endif
