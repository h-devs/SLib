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

#ifndef CHECKHEADER_SLIB_CORE_IO_READER_HELPER
#define CHECKHEADER_SLIB_CORE_IO_READER_HELPER

#include "../mio.h"
#include "../thread.h"
#include "../string.h"
#include "../memory_buffer.h"
#include "../scoped_buffer.h"

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

		template <class READER, class T>
		static sl_bool readCVLI(READER* reader, T* output, EndianType endian)
		{
			T value = 0;
			sl_uint8 n;
			sl_uint32 m = 0;
			while (readUint8(reader, &n)) {
				if (endian == EndianType::Little) {
					value |= (((T)(n & 127)) << m);
					m += 7;
				} else {
					value = (value << 7) | (n & 127);
				}
				if (!(n & 128)) {
					if (output) {
						*output = value;
					}
					return sl_true;
				}
			}
			return sl_false;
		}

		template <class READER, class T>
		static T readCVLI(READER* reader, T def, EndianType endian)
		{
			T v;
			if (readCVLI(reader, &v, endian)) {
				return v;
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

		template <class READER>
		static String readTextUTF8(READER* reader, sl_size size)
		{
			sl_char8 sbuf[3];
			if (size >= 3) {
				sl_reg iRet = readFully(reader, sbuf, 3);
				if (iRet == 3) {
					size -= 3;
					if ((sl_uint8)(sbuf[0]) == 0xEF && (sl_uint8)(sbuf[1]) == 0xBB && (sl_uint8)(sbuf[2]) == 0xBF) {
						return _readText8(reader, size);
					} else {
						return _readText8(reader, size, sbuf, 3);
					}
				} else if (iRet > 0) {
					return String(sbuf, iRet);
				}
			} else {
				return _readText8(reader, size);
			}
			return sl_null;
		}

		template <class READER>
		static String16 readTextUTF16(READER* reader, sl_size size, EndianType endian)
		{
			sl_size len = size >> 1;
			if (!len) {
				return String16::getEmpty();
			}
			sl_uint16 first;
			if (readUint16(reader, &first, endian)) {
				len--;
				if (first == 0xFEFF) {
					return _readText16(reader, len, endian);
				} else {
					return _readText16(reader, len, first, endian);
				}
			}
			return sl_null;
		}

		template <class READER>
		static StringParam readText(READER* reader, sl_size size)
		{
			if (!size) {
				return sl_null;
			}
			sl_char8 sbuf[3];
			if (size >= 2) {
				sl_reg iRet = readFully(reader, sbuf, 2);
				if (iRet == 2) {
					if (!(size & 1)) {
						sl_bool flagUTF16LE = sbuf[0] == (sl_char8)0xFF && sbuf[1] == (sl_char8)0xFE;
						sl_bool flagUTF16BE = sbuf[0] == (sl_char8)0xFE && sbuf[1] == (sl_char8)0xFF;
						if (flagUTF16LE || flagUTF16BE) {
							size -= 2;
							return _readText16(reader, size >> 1, flagUTF16LE ? EndianType::Little : EndianType::Big);
						}
					}
					if (size >= 3) {
						iRet = reader->read(sbuf + 2, 1);
						if (iRet == 1) {
							size -= 3;
							if (sbuf[0] == (sl_char8)0xEF && sbuf[1] == (sl_char8)0xBB && sbuf[2] == (sl_char8)0xBF) {
								return _readText8(reader, size);
							} else {
								return _readText8(reader, size, sbuf, 3);
							}
						} else if (!iRet) {
							return String::fromUtf8(sbuf, 2);
						}
					} else {
						return String::fromUtf8(sbuf, 2);
					}
					return StringParam();
				} else if (iRet > 0) {
					return String::fromUtf8(sbuf, iRet);
				}
			} else {
				return _readText8(reader, size);
			}
			return StringParam();
		}

	private:
		template <class READER>
		static String _readText8(READER* reader, sl_size size)
		{
			if (!size) {
				return String::getEmpty();
			}
			String ret = String::allocate(size);
			if (ret.isNotNull()) {
				sl_char8* buf = ret.getData();
				sl_reg iRet = readFully(reader, buf, size);
				if (iRet > 0) {
					if ((sl_size)iRet < size) {
						buf[iRet] = 0;
						ret.setLength(iRet);
					}
					return ret;
				} else if (!iRet) {
					return String::getEmpty();
				}
			}
			return sl_null;
		}

		template <class READER>
		static String _readText8(READER* reader, sl_size size, const sl_char8* prefix, sl_size nPrefix)
		{
			if (!size) {
				return String(prefix, nPrefix);
			}
			String ret = String::allocate(nPrefix + size);
			if (ret.isNotNull()) {
				sl_char8* buf = ret.getData();
				Base::copyMemory(buf, prefix, nPrefix);
				buf += nPrefix;
				sl_reg iRet = readFully(reader, buf, size);
				if (iRet >= 0) {
					if ((sl_size)iRet < size) {
						buf[iRet] = 0;
						ret.setLength(nPrefix + (sl_size)iRet);
					}
					return ret;
				}
			}
			return sl_null;
		}

		template <class READER>
		static String16 _readText16(READER* reader, sl_size len, EndianType endian)
		{
			if (!len) {
				return String16::getEmpty();
			}
			String16 ret = String16::allocate(len);
			if (ret.isNotNull()) {
				sl_char16* buf = ret.getData();
				sl_size size = len << 1;
				sl_reg iRet = readFully(reader, buf, size);
				if (iRet > 0) {
					if ((sl_size)iRet < size) {
						len = ((sl_size)iRet) >> 1;
						buf[len] = 0;
						ret.setLength(len);
					}
					if ((endian == Endian::Big && Endian::isLE()) || (endian == Endian::Little && Endian::isBE())) {
						for (sl_size i = 0; i < len; i++) {
							sl_uint16 c = (sl_uint16)(buf[i]);
							buf[i] = (sl_char16)((c >> 8) | (c << 8));
						}
					}
					return ret;
				}
			}
			return sl_null;
		}

		template <class READER>
		static String16 _readText16(READER* reader, sl_size len, sl_char16 prefix, EndianType endian)
		{
			if (!len) {
				return String16(&prefix, 1);
			}
			String16 ret = String16::allocate(1 + len);
			if (ret.isNotNull()) {
				sl_char16* buf = ret.getData();
				*buf = prefix;
				buf++;
				len--;
				sl_size size = len << 1;
				sl_reg iRet = readFully(reader, buf, size);
				if (iRet > 0) {
					if ((sl_size)iRet < size) {
						len = ((sl_size)iRet) >> 1;
						buf[len] = 0;
						ret.setLength(1 + len);
					}
					if ((endian == Endian::Big && Endian::isLE()) || (endian == Endian::Little && Endian::isBE())) {
						for (sl_size i = 0; i < len; i++) {
							sl_uint16 c = (sl_uint16)(buf[i]);
							buf[i] = (sl_char16)((c >> 8) | (c << 8));
						}
					}
					return ret;
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
