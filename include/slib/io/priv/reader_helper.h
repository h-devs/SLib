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

#ifndef CHECKHEADER_SLIB_IO_PRIV_READER_HELPER
#define CHECKHEADER_SLIB_IO_PRIV_READER_HELPER

#include "../../core/thread.h"
#include "../../core/timeout.h"
#include "../../core/memory_buffer.h"

namespace slib
{

	class ReaderHelper
	{
	public:
		template <class READER>
		static sl_reg readWithRead32(READER* reader, void* buf, sl_size size, sl_int32 timeout = -1)
		{
#if defined(SLIB_ARCH_IS_64BIT)
			if (size >> 31) {
				return reader->read32(buf, 0x40000000, timeout); // 1GB
			}
#endif
			return reader->read32(buf, (sl_uint32)size, timeout);
		}

		template <class READER>
		static sl_reg readFully(READER* reader, void* _buf, sl_size size, sl_int32 timeout = -1)
		{
			sl_uint8* buf = (sl_uint8*)_buf;
			if (!size) {
				return reader->read(buf, 0, timeout);
			}
			sl_int64 tickEnd = GetTickFromTimeout(timeout);
			sl_size nRead = 0;
			CurrentThread thread;
			do {
				sl_reg m = reader->read(buf, size, timeout);
				if (m > 0) {
					nRead += m;
					if (size <= (sl_size)m) {
						return nRead;
					}
					buf += m;
					size -= m;
					timeout = GetTimeoutFromTick(tickEnd);
				} else if (m == SLIB_IO_WOULD_BLOCK || m == SLIB_IO_ENDED) {
					if (nRead) {
						return nRead;
					} else {
						return m;
					}
				} else {
					return m;
				}
			} while (thread.isNotStopping());
			return SLIB_IO_ERROR;
		}

		template <class READER>
		static sl_reg readFully(READER* reader, MemoryBuffer& output, sl_size size, sl_size segmentSize, sl_int32 timeout = -1)
		{
			if (!size) {
				char ch;
				return reader->read(&ch, 0, timeout);
			}
			if (!segmentSize) {
				segmentSize = 1024;
			}
			sl_int64 tickEnd = GetTickFromTimeout(timeout);
			CurrentThread thread;
			do {
				sl_size nSegment = SLIB_MIN(segmentSize, size);
				Memory segment = Memory::create(nSegment);
				if (segment.isNull()) {
					return SLIB_IO_ERROR;
				}
				sl_size nRequest = nSegment;
				sl_uint8* buf = (sl_uint8*)(segment.getData());
				sl_size nRead = 0;
				for (;;) {
					sl_reg m = reader->read(buf, nRequest, timeout);
					if (m > 0) {
						nRead += m;
						if (nRequest <= (sl_size)m) {
							break;
						}
						buf += m;
						nRequest -= m;
						timeout = GetTimeoutFromTick(tickEnd);
					} else if (m == SLIB_IO_ENDED || m == SLIB_IO_WOULD_BLOCK) {
						if (nRead) {
							if (nRead < nSegment) {
								segment = segment.sub(0, nRead);
								if (segment.isNull()) {
									return SLIB_IO_ERROR;
								}
							}
							if (!(output.add(Move(segment)))) {
								return SLIB_IO_ERROR;
							}
						}
						sl_size nOutput = output.getSize();
						if (nOutput) {
							return nOutput;
						} else {
							return m;
						}
					} else {
						return m;
					}
					if (thread.isStopping()) {
						return SLIB_IO_ERROR;
					}
				}
				if (!(output.add(Move(segment)))) {
					return SLIB_IO_ERROR;
				}
				if (size <= nSegment) {
					return output.getSize();
				}
				size -= nSegment;
			} while (thread.isNotStopping());
			return SLIB_IO_ERROR;
		}

		template <class READER>
		static Memory readFully(READER* reader, sl_size size, sl_size segmentSize, sl_int32 timeout = -1)
		{
			if (!size) {
				return sl_null;
			}
			if (size != SLIB_SIZE_MAX && (!segmentSize || size < segmentSize)) {
				Memory mem = Memory::create(size);
				if (mem.isNotNull()) {
					sl_reg nRead = readFully(reader, mem.getData(), size, timeout);
					if (nRead == size) {
						return mem;
					} else if (nRead > 0) {
						return mem.sub(0, nRead);
					}
				}
				return sl_null;
			}
			MemoryBuffer buffer;
			sl_reg m = readFully(reader, buffer, size, segmentSize, timeout);
			if (m > 0) {
				return buffer.merge();
			} else {
				return sl_null;
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

	};

	class BlockReaderHelper
	{
	public:
		template <class READER>
		static sl_reg readAtWithReadAt32(READER* reader, sl_uint64 offset, void* buf, sl_size size, sl_int32 timeout = -1)
		{
#if defined(SLIB_ARCH_IS_64BIT)
			if (size >> 31) {
				return reader->readAt32(offset, buf, 0x40000000, timeout);
			}
#endif
			return reader->readAt32(offset, buf, (sl_uint32)size, timeout);
		}

		template <class READER>
		static sl_reg readFullyAt(READER* reader, sl_uint64 offset, void* _buf, sl_size size, sl_int32 timeout = -1)
		{
			sl_uint8* buf = (sl_uint8*)_buf;
			if (!size) {
				return reader->readAt(offset, buf, 0, timeout);
			}
			sl_int64 tickEnd = GetTickFromTimeout(timeout);
			sl_size nRead = 0;
			CurrentThread thread;
			do {
				sl_reg m = reader->readAt(offset, buf, size, timeout);
				if (m > 0) {
					nRead += m;
					if (size <= (sl_size)m) {
						return nRead;
					}
					buf += m;
					offset += m;
					size -= m;
					timeout = GetTimeoutFromTick(tickEnd);
				} else if (m == SLIB_IO_WOULD_BLOCK || m == SLIB_IO_ENDED) {
					if (nRead) {
						return nRead;
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
