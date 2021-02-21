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

#include "slib/core/io.h"
#include "slib/core/memory_io.h"
#include "slib/core/memory_reader.h"
#include "slib/core/memory_writer.h"
#include "slib/core/memory_output.h"
#include "slib/core/buffered_reader.h"
#include "slib/core/buffered_writer.h"
#include "slib/core/buffered_seekable_reader.h"
#include "slib/core/io_util.h"

#include "slib/core/mio.h"
#include "slib/core/string_buffer.h"
#include "slib/core/thread.h"
#include "slib/core/scoped.h"
#include "slib/math/bigint.h"

namespace slib
{
	
	namespace priv
	{
		namespace endian
		{
			
			static sl_bool checkLittleEndianRuntime()
			{
				sl_uint32 n = 0x12345678;
				return *(sl_uint8*)(&n) == 0x78;
			}
			
			static sl_bool checkBigEndianRuntime()
			{
				sl_uint32 n = 0x12345678;
				return *(sl_uint8*)(&n) != 0x78;
			}

		}
	}
	
	sl_bool Endian::checkLittleEndianRuntime()
	{
		static sl_bool f = priv::endian::checkLittleEndianRuntime();
		return f;
	}
	
	sl_bool Endian::checkBigEndianRuntime()
	{
		static sl_bool f = priv::endian::checkBigEndianRuntime();
		return f;
	}

	
	IReader::IReader()
	{
	}

	IReader::~IReader()
	{
	}

	sl_int32 IReader::read32(void* buf, sl_uint32 size)
	{
		return (sl_int32)(read(buf, size));
	}

	sl_reg IReader::read(void* buf, sl_size size)
	{
		if (size >= 0x40000000) {
			size = 0x40000000; // 1GB
		}
		return read32(buf, (sl_uint32)size);
	}

	sl_reg IReader::readFully(void* _buf, sl_size size)
	{
		char* buf = (char*)_buf;
		if (size == 0) {
			return 0;
		}
		sl_size nRead = 0;
		while (nRead < size) {
			sl_size n = size - nRead;
			sl_reg m = read(buf + nRead, n);
			if (m < 0) {
				if (nRead) {
					return nRead;
				} else {
					return m;
				}
			}
			nRead += m;
			if (Thread::isStoppingCurrent()) {
				return nRead;
			}
			if (m == 0) {
				Thread::sleep(1);
				if (Thread::isStoppingCurrent()) {
					return nRead;
				}
			}
		}
		return nRead;
	}

	sl_bool IReader::readInt8(sl_int8* output)
	{
		if (read32(output, 1) == 1) {
			return sl_true;
		} else {
			return sl_false;
		}
	}

	sl_int8 IReader::readInt8(sl_int8 def)
	{
		sl_int8 ret;
		if (readInt8(&ret)) {
			return ret;
		} else {
			return def;
		}
	}

	sl_bool IReader::readUint8(sl_uint8* output)
	{
		if (read32(output, 1) == 1) {
			return sl_true;
		} else {
			return sl_false;
		}
	}

	sl_uint8 IReader::readUint8(sl_uint8 def)
	{
		sl_uint8 ret;
		if (readUint8(&ret)) {
			return ret;
		} else {
			return def;
		}
	}

	sl_bool IReader::readInt16(sl_int16* output, EndianType endian)
	{
		if (readFully(output, 2) == 2) {
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

	sl_int16 IReader::readInt16(sl_int16 def, EndianType endian)
	{
		sl_int16 ret;
		if (readInt16(&ret, endian)) {
			return ret;
		} else {
			return def;
		}
	}

	sl_bool IReader::readUint16(sl_uint16* output, EndianType endian)
	{
		if (readFully(output, 2) == 2) {
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

	sl_uint16 IReader::readUint16(sl_uint16 def, EndianType endian)
	{
		sl_uint16 ret;
		if (readUint16(&ret, endian)) {
			return ret;
		} else {
			return def;
		}
	}

	sl_bool IReader::readInt32(sl_int32* output, EndianType endian)
	{
		if (readFully(output, 4) == 4) {
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

	sl_int32 IReader::readInt32(sl_int32 def, EndianType endian)
	{
		sl_int32 ret;
		if (readInt32(&ret, endian)) {
			return ret;
		} else {
			return def;
		}
	}

	sl_bool IReader::readUint32(sl_uint32* output, EndianType endian)
	{
		if (readFully(output, 4) == 4) {
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

	sl_uint32 IReader::readUint32(sl_uint32 def, EndianType endian)
	{
		sl_uint32 ret;
		if (readUint32(&ret, endian)) {
			return ret;
		} else {
			return def;
		}
	}

	sl_bool IReader::readInt64(sl_int64* output, EndianType endian)
	{
		if (readFully(output, 8) == 8) {
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

	sl_int64 IReader::readInt64(sl_int64 def, EndianType endian)
	{
		sl_int64 ret;
		if (readInt64(&ret, endian)) {
			return ret;
		} else {
			return def;
		}
	}

	sl_bool IReader::readUint64(sl_uint64* output, EndianType endian)
	{
		if (readFully(output, 8) == 8) {
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

	sl_uint64 IReader::readUint64(sl_uint64 def, EndianType endian)
	{
		sl_uint64 ret;
		if (readUint64(&ret, endian)) {
			return ret;
		} else {
			return def;
		}
	}

	sl_bool IReader::readFloat(float* output, EndianType endian)
	{
		if (readFully(output, 4) == 4) {
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

	float IReader::readFloat(float def, EndianType endian)
	{
		float ret;
		if (readFloat(&ret, endian)) {
			return ret;
		} else {
			return def;
		}
	}

	sl_bool IReader::readDouble(double* output, EndianType endian)
	{
		if (readFully(output, 8) == 8) {
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

	double IReader::readDouble(double def, EndianType endian)
	{
		double ret;
		if (readDouble(&ret, endian)) {
			return ret;
		} else {
			return def;
		}
	}

	sl_bool IReader::readUint32CVLI(sl_uint32* output)
	{
		sl_uint32 v = 0;
		int m = 0;
		while (1) {
			sl_uint8 n = 0;
			if (readFully(&n, 1) == 1) {
				v += (((sl_uint32)(n & 127)) << m);
				m += 7;
				if ((n & 128) == 0) {
					break;
				}
			} else {
				return sl_false;
			}
		}
		*output = v;
		return sl_true;
	}

	sl_uint32 IReader::readUint32CVLI(sl_uint32 def)
	{
		sl_uint32 ret;
		if (readUint32CVLI(&ret)) {
			return ret;
		} else {
			return def;
		}
	}

	sl_bool IReader::readInt32CVLI(sl_int32* output)
	{
		return readUint32CVLI((sl_uint32*)output);
	}

	sl_int32 IReader::readInt32CVLI(sl_int32 def)
	{
		sl_int32 ret;
		if (readInt32CVLI(&ret)) {
			return ret;
		} else {
			return def;
		}
	}

	sl_bool IReader::readUint64CVLI(sl_uint64* output)
	{
		sl_uint64 v = 0;
		int m = 0;
		while (1) {
			sl_uint8 n = 0;
			if (readFully(&n, 1) == 1) {
				v += (((sl_uint64)(n & 127)) << m);
				m += 7;
				if ((n & 128) == 0) {
					break;
				}
			} else {
				return sl_false;
			}
		}
		*output = v;
		return sl_true;
	}

	sl_uint64 IReader::readUint64CVLI(sl_uint64 def)
	{
		sl_uint64 ret;
		if (readUint64CVLI(&ret)) {
			return ret;
		} else {
			return def;
		}
	}

	sl_bool IReader::readInt64CVLI(sl_int64* output)
	{
		return readUint64CVLI((sl_uint64*)output);
	}

	sl_int64 IReader::readInt64CVLI(sl_int64 def)
	{
		sl_int64 ret;
		if (readInt64CVLI(&ret)) {
			return ret;
		} else {
			return def;
		}
	}

	sl_bool IReader::readSizeCVLI(sl_size* output)
	{
#ifdef SLIB_ARCH_IS_64BIT
		return readUint64CVLI(output);
#else
		return readUint32CVLI(output);
#endif
	}

	sl_size IReader::readSizeCVLI(sl_size def)
	{
#ifdef SLIB_ARCH_IS_64BIT
		return readUint64CVLI(def);
#else
		return readUint32CVLI(def);
#endif
	}

	sl_bool IReader::readIntCVLI(sl_reg* output)
	{
#ifdef SLIB_ARCH_IS_64BIT
		return readInt64CVLI(output);
#else
		return readInt32CVLI(output);
#endif
	}

	sl_reg IReader::readIntCVLI(sl_reg def)
	{
#ifdef SLIB_ARCH_IS_64BIT
		return readInt64CVLI(def);
#else
		return readInt32CVLI(def);
#endif
	}

	Memory IReader::readToMemory(sl_size size)
	{
		Memory mem = Memory::create(size);
		if (mem.isNotNull()) {
			sl_reg nRead = readFully(mem.getData(), size);
			if (nRead == size) {
				return mem;
			} else if (nRead > 0) {
				return mem.sub(0, nRead);
			}
		}
		return sl_null;
	}

	sl_bool IReader::readSectionData(void* mem, sl_size& size)
	{
		sl_size sizeBuf = size;
		if (readSizeCVLI(&size)) {
			if (size <= sizeBuf) {
				if (readFully(mem, size) == (sl_reg)size) {
					return sl_true;
				}
			}
		} else {
			size = 0;
		}
		return sl_false;
	}

	sl_bool IReader::readSection(Memory* mem, sl_size maxSize)
	{
		sl_size size;
		if (readSizeCVLI(&size)) {
			if (size > maxSize) {
				return sl_false;
			}
			if (size == 0) {
				if (mem) {
					mem->setNull();
				}
				return sl_true;
			}
			if (mem) {
				Memory ret = Memory::create(size);
				if (ret.isNotNull()) {
					if (readFully(ret.getData(), size) == (sl_reg)size) {
						*mem = ret;
						return sl_true;
					}
				}
			} else {
				char buf[512];
				while (size) {
					sl_size n = size;
					if (n > sizeof(buf)) {
						n = sizeof(buf);
					}
					if (readFully(buf, n) != n) {
						return sl_false;
					}
					size -= n;
				}
				return sl_true;
			}
		}
		return sl_false;
	}

	Memory IReader::readSection(const Memory& def, sl_size maxSize)
	{
		Memory ret;
		if (readSection(&ret, maxSize)) {
			return ret;
		}
		return def;
	}

	Memory IReader::readSection(sl_size maxSize)
	{
		Memory ret;
		if (readSection(&ret, maxSize)) {
			return ret;
		}
		return sl_null;
	}

	sl_bool IReader::readStringSection(String* str, sl_size maxLen)
	{
		if (str) {
			Memory mem;
			if (readSection(&mem, maxLen)) {
				if (mem.isNull()) {
					str->setNull();
					return sl_true;
				}
				sl_size len = mem.getSize();
				String ret((char*)(mem.getData()), len);
				if (ret.isNotNull()) {
					*str = ret;
					return sl_true;
				}
			}
		} else {
			return readSection(sl_null, maxLen);
		}
		return sl_false;
	}

	String IReader::readStringSection(const String& def, sl_size maxLen)
	{
		String ret;
		if (readStringSection(&ret, maxLen)) {
			return ret;
		} else {
			return def;
		}
	}

	String IReader::readStringSection(sl_size maxLen)
	{
		String ret;
		if (readStringSection(&ret, maxLen)) {
			return ret;
		} else {
			return sl_null;
		}
	}

	sl_bool IReader::readBigInt(BigInt* v, sl_size maxLen)
	{
		Memory mem;
		if (readSection(&mem, maxLen)) {
			if (mem.isNull()) {
				v->setNull();
				return sl_true;
			}
			sl_size len = mem.getSize();
			BigInt ret = BigInt::fromBytesLE(mem.getData(), len);
			if (ret.isNotNull()) {
				*v = ret;
				return sl_true;
			}
		}
		return sl_false;
	}

	BigInt IReader::readBigInt(const BigInt& def, sl_size maxLen)
	{
		BigInt ret;
		if (readBigInt(&ret, maxLen)) {
			return ret;
		} else {
			return def;
		}
	}

	BigInt IReader::readBigInt(sl_size maxLen)
	{
		BigInt ret;
		if (readBigInt(&ret, maxLen)) {
			return ret;
		} else {
			return sl_null;
		}
	}

	sl_bool IReader::readTime(Time* v)
	{
		sl_int64 m;
		if (readInt64(&m)) {
			*v = m;
			return sl_true;
		}
		return sl_false;
	}

	Time IReader::readTime()
	{
		Time ret;
		if (readTime(&ret)) {
			return ret;
		} else {
			return Time::zero();
		}
	}

	Time IReader::readTime(const Time& def)
	{
		Time ret;
		if (readTime(&ret)) {
			return ret;
		} else {
			return def;
		}
	}

	String IReader::readTextUTF8(sl_size size)
	{
		if (size == 0) {
			return String::getEmpty();
		}
		sl_char8 sbuf[3];
		if (size >= 3) {
			if (read(sbuf, 3) == 3) {
				if ((sl_uint8)(sbuf[0]) == 0xEF && (sl_uint8)(sbuf[1]) == 0xBB && (sl_uint8)(sbuf[2]) == 0xBF) {
					size -= 3;
					if (size == 0) {
						return String::getEmpty();
					}
					String ret = String::allocate(size);
					if (ret.isNotNull()) {
						if (read(ret.getData(), size) == (sl_reg)size) {
							return ret;
						}
					}
				} else {
					String ret = String::allocate(size);
					if (ret.isNotNull()) {
						sl_char8* buf = ret.getData();
						Base::copyMemory(buf, sbuf, 3);
						if (size == 3) {
							return ret;
						}
						size -= 3;
						if (read(buf+3, size) == (sl_reg)size) {
							return ret;
						}
					}
				}
			}
		} else {
			String ret = String::allocate(size);
			if (ret.isNotNull()) {
				if (read(ret.getData(), size) == (sl_reg)size) {
					return ret;
				}
			}
		}
		return sl_null;
	}

	String16 IReader::readTextUTF16(sl_size size, EndianType endian)
	{
		if (size == 0) {
			return String16::getEmpty();
		}
		sl_size len = (size >> 1) + (size & 1);
		sl_uint16 first;
		if (readUint16(&first, endian)) {
			len--;
			// check BOM(Byte Order Mark = U+FEFF)
			String16 str;
			sl_char16* buf;
			if (first == 0xFEFF) {
				if (len == 0) {
					return String16::getEmpty();
				}
				str = String16::allocate(len);
				if (str.isNull()) {
					return str;
				}
				buf = str.getData();
			} else {
				str = String16::allocate(len + 1);
				if (str.isNull()) {
					return str;
				}
				buf = str.getData();
				*buf = first;
				buf++;
			}
			if (len == 0) {
				return str;
			}
			buf[len - 1] = 0;
			if (read(buf, size) == (sl_reg)size) {
				if ((endian == Endian::Big && Endian::isLE()) || (endian == Endian::Little && Endian::isBE())) {
					for (sl_size i = 0; i < len; i++) {
						sl_uint16 c = (sl_uint16)(buf[i]);
						buf[i] = (sl_char16)((c >> 8) | (c << 8));
					}
				}
				return str;
			}
		}
		return sl_null;
	}

	String IReader::readText(sl_size size, Charset* outCharset)
	{
		if (size == 0) {
			if (outCharset) {
				*outCharset = Charset::UTF8;
			}
			return String::getEmpty();
		}
		sl_char8 sbuf[3];
		if (size >= 2) {
			if (read(sbuf, 2) == 2) {
				if (size % 2 == 0) {
					sl_bool flagUTF16LE = sbuf[0] == (sl_char8)0xFF && sbuf[1] == (sl_char8)0xFE;
					sl_bool flagUTF16BE = sbuf[0] == (sl_char8)0xFE && sbuf[1] == (sl_char8)0xFF;
					if (flagUTF16LE || flagUTF16BE) {
						if (outCharset) {
							if (flagUTF16LE) {
								*outCharset = Charset::UTF16LE;
							} else {
								*outCharset = Charset::UTF16BE;
							}
						}
						size -= 2;
						SLIB_SCOPED_BUFFER(sl_uint8, 4096, buf, size);
						if (buf) {
							if (read(buf, size) == (sl_reg)size) {
								if (flagUTF16LE) {
									return String::fromUtf16LE(buf, size);
								} else {
									return String::fromUtf16BE(buf, size);
								}
							}
						}
						return sl_null;
					}
				}
				if (outCharset) {
					*outCharset = Charset::UTF8;
				}
				if (size >= 3) {
					if (read(sbuf + 2, 1) == 1) {
						if (sbuf[0] == (sl_char8)0xEF && sbuf[1] == (sl_char8)0xBB && sbuf[2] == (sl_char8)0xBF) {
							size -= 3;
							if (size == 0) {
								return String::getEmpty();
							}
							String ret = String::allocate(size);
							if (ret.isNotNull()) {
								if (read(ret.getData(), size) == (sl_reg)size) {
									return ret;
								}
							}
						} else {
							String ret = String::allocate(size);
							if (ret.isNotNull()) {
								sl_char8* buf = ret.getData();
								Base::copyMemory(buf, sbuf, 3);
								if (size == 3) {
									return ret;
								}
								size -= 3;
								if (read(buf+3, size) == (sl_reg)size) {
									return ret;
								}
							}
						}
					}
				} else {
					return String::fromUtf8(sbuf, 2);
				}
				return sl_null;
			}
		} else {
			String ret = String::allocate(size);
			if (ret.isNotNull()) {
				if (read(ret.getData(), size) == (sl_reg)size) {
					if (outCharset) {
						*outCharset = Charset::UTF8;
					}
					return ret;
				}
			}
		}
		if (outCharset) {
			*outCharset = Charset::UTF8;
		}
		return sl_null;
	}

	String16 IReader::readText16(sl_size size, Charset* outCharset)
	{
		if (size == 0) {
			if (outCharset) {
				*outCharset = Charset::UTF8;
			}
			return String16::getEmpty();
		}
		sl_char8 sbuf[3];
		if (size >= 2) {
			if (read(sbuf, 2) == 2) {
				if (size % 2 == 0) {
					sl_bool flagUTF16LE = sbuf[0] == (sl_char8)0xFF && sbuf[1] == (sl_char8)0xFE;
					sl_bool flagUTF16BE = sbuf[0] == (sl_char8)0xFE && sbuf[1] == (sl_char8)0xFF;
					if (flagUTF16LE || flagUTF16BE) {
						if (outCharset) {
							if (flagUTF16LE) {
								*outCharset = Charset::UTF16LE;
							} else {
								*outCharset = Charset::UTF16BE;
							}
						}
						size -= 2;
						sl_size len = size >> 1;
						if (len == 0) {
							return String16::getEmpty();
						}
						String16 str = String16::allocate(len);
						if (str.isNotNull()) {
							sl_char16* buf = str.getData();
							size = len << 1;
							if (read(buf, size) == (sl_reg)size) {
								if ((flagUTF16BE && Endian::isLE()) || (flagUTF16LE && Endian::isBE())) {
									for (sl_size i = 0; i < len; i++) {
										sl_uint16 c = (sl_uint16)(buf[i]);
										buf[i] = (sl_char16)((c >> 8) | (c << 8));
									}
								}
								return str;
							}
						}
						return sl_null;
					}
				}
				SLIB_SCOPED_BUFFER(sl_char8, 4096, tbuf, size);
				if (tbuf) {
					if (outCharset) {
						*outCharset = Charset::UTF8;
					}
					tbuf[0] = sbuf[0];
					tbuf[1] = sbuf[1];
					sl_char8* buf = tbuf;
					if (size >= 3) {
						if (read(tbuf + 2, size - 2) == (sl_reg)(size - 2)) {
							if (tbuf[0] == (sl_char8)0xEF && tbuf[1] == (sl_char8)0xBB && tbuf[2] == (sl_char8)0xBF) {
								size -= 3;
								if (size == 0) {
									return String16::getEmpty();
								}
								buf = tbuf + 3;
							}
						} else {
							return sl_null;
						}
					}
					return String16::fromUtf8(buf, size);
				}
			}
		} else {
			if (read(sbuf, size) == (sl_reg)size) {
				if (outCharset) {
					*outCharset = Charset::UTF8;
				}
				return String16::fromUtf8(sbuf, size);
			}
		}
		if (outCharset) {
			*outCharset = Charset::UTF8;
		}
		return sl_null;
	}


	IWriter::IWriter()
	{
	}

	IWriter::~IWriter()
	{
	}

	sl_int32 IWriter::write32(const void* buf, sl_uint32 size)
	{
		return (sl_int32)(write(buf, size));
	}

	sl_reg IWriter::write(const void* _buf, sl_size size)
	{
		char* buf = (char*)_buf;
		if (size == 0) {
			return 0;
		}
		sl_size nWrite = 0;
		while (nWrite < size) {
			sl_size n = size - nWrite;
			if (n > 0x40000000) {
				n = 0x40000000; // 1GB
			}
			sl_uint32 n32 = (sl_uint32)n;
			sl_int32 m = write32(buf + nWrite, n32);
			if (m <= 0) {
				break;
			}
			nWrite += m;
			if (m != n32 || Thread::isStoppingCurrent()) {
				return nWrite;
			}
		}
		return nWrite;
	}

	sl_reg IWriter::writeFully(const void* _buf, sl_size size)
	{
		char* buf = (char*)_buf;
		if (size == 0) {
			return 0;
		}
		sl_size nWrite = 0;
		while (nWrite < size) {
			sl_size n = size - nWrite;
			sl_reg m = write(buf + nWrite, n);
			if (m < 0) {
				if (nWrite) {
					return nWrite;
				} else {
					return m;
				}
			}
			nWrite += m;
			if (Thread::isStoppingCurrent()) {
				return nWrite;
			}
			if (m == 0) {
				Thread::sleep(1);
				if (Thread::isStoppingCurrent()) {
					return nWrite;
				}
			}
		}
		return nWrite;
	}

	sl_bool IWriter::writeInt8(sl_int8 value)
	{
		return write32(&value, 1) == 1;
	}

	sl_bool IWriter::writeUint8(sl_uint8 value)
	{
		return write32(&value, 1) == 1;
	}

	sl_bool IWriter::writeInt16(sl_int16 value, EndianType endian)
	{
		if (endian == Endian::Big) {
			value = Endian::swap16LE(value);
		} else {
			value = Endian::swap16BE(value);
		}
		return writeFully(&value, 2) == 2;
	}

	sl_bool IWriter::writeUint16(sl_uint16 value, EndianType endian)
	{
		return writeInt16(value, endian);
	}

	sl_bool IWriter::writeInt32(sl_int32 value, EndianType endian)
	{
		if (endian == Endian::Big) {
			value = Endian::swap32LE(value);
		} else {
			value = Endian::swap32BE(value);
		}
		return writeFully(&value, 4) == 4;
	}

	sl_bool IWriter::writeUint32(sl_uint32 value, EndianType endian)
	{
		return writeInt32(value, endian);
	}

	sl_bool IWriter::writeInt64(sl_int64 value, EndianType endian)
	{
		if (endian == Endian::Big) {
			value = Endian::swap64LE(value);
		} else {
			value = Endian::swap64BE(value);
		}
		return writeFully(&value, 8) == 8;
	}

	sl_bool IWriter::writeUint64(sl_uint64 value, EndianType endian)
	{
		return writeInt64(value, endian);
	}

	sl_bool IWriter::writeFloat(float value, EndianType endian)
	{
		if (endian == Endian::Big) {
			value = Endian::swapFloatLE(value);
		} else {
			value = Endian::swapFloatBE(value);
		}
		return writeFully(&value, 4) == 4;
	}

	sl_bool IWriter::writeDouble(double value, EndianType endian)
	{
		if (endian == Endian::Big) {
			value = Endian::swapDoubleLE(value);
		} else {
			value = Endian::swapDoubleBE(value);
		}
		return writeFully(&value, 8) == 8;
	}

	sl_bool IWriter::writeUint32CVLI(sl_uint32 value)
	{
		sl_bool flagContinue = sl_true;
		do {
			sl_uint8 n = ((sl_uint8)value) & 127;
			value = value >> 7;
			if (value != 0) {
				n |= 128;
			} else {
				flagContinue = sl_false;
			}
			if (writeFully(&n, 1) != 1) {
				return sl_false;
			}
		} while (flagContinue);
		return sl_true;
	}

	sl_bool IWriter::writeInt32CVLI(sl_int32 value)
	{
		return writeUint32CVLI((sl_uint32)value);
	}

	sl_bool IWriter::writeUint64CVLI(sl_uint64 value)
	{
		sl_bool flagContinue = sl_true;
		do {
			sl_uint8 n = ((sl_uint8)value) & 127;
			value = value >> 7;
			if (value != 0) {
				n |= 128;
			} else {
				flagContinue = sl_false;
			}
			if (writeFully(&n, 1) != 1) {
				return sl_false;
			}
		} while (flagContinue);
		return sl_true;
	}

	sl_bool IWriter::writeInt64CVLI(sl_int64 value)
	{
		return writeUint64CVLI((sl_uint64)value);
	}

	sl_bool IWriter::writeSizeCVLI(sl_size value)
	{
#ifdef SLIB_ARCH_IS_64BIT
		return writeUint64CVLI(value);
#else
		return writeUint32CVLI(value);
#endif
	}

	sl_bool IWriter::writeIntCVLI(sl_reg value)
	{
#ifdef SLIB_ARCH_IS_64BIT
		return writeInt64CVLI(value);
#else
		return writeInt32CVLI(value);
#endif
	}

	sl_size IWriter::writeFromMemory(const Memory& mem)
	{
		return writeFully(mem.getData(), mem.getSize());
	}

	sl_bool IWriter::writeSection(const void* mem, sl_size size)
	{
		if (writeSizeCVLI(size)) {
			if (writeFully(mem, size) == (sl_reg)size) {
				return sl_true;
			}
		}
		return sl_false;
	}

	sl_bool IWriter::writeSection(const Memory& mem)
	{
		return writeSection(mem.getData(), mem.getSize());
	}

	sl_bool IWriter::writeStringSection(const StringParam& _str, sl_size maxLen)
	{
		StringData str(_str);
		return writeSection(str.getData(), str.getLength());
	}

	sl_bool IWriter::writeBigInt(const BigInt& v, sl_size maxLen)
	{
		sl_size n = v.getMostSignificantBytes();
		if (n > maxLen) {
			n = maxLen;
		}
		SLIB_SCOPED_BUFFER(char, 1024, buf, n);
		v.getBytesLE(buf, n);
		return writeSection(buf, n);
	}

	sl_bool IWriter::writeTime(const Time& t)
	{
		return writeInt64(t.toInt());
	}

	sl_bool IWriter::writeTextUTF8(const StringParam& _text, sl_bool flagWriteByteOrderMark)
	{
		if (flagWriteByteOrderMark) {
			static sl_char8 sbuf[3] = {(sl_char8)0xEF, (sl_char8)0xBB, (sl_char8)0xBF};
			if (write(sbuf, 3) != 3) {
				return sl_false;
			}
		}
		StringData text(_text);
		sl_size n = text.getLength();
		if (n == 0) {
			return sl_true;
		}
		if (write(text.getData(), n) == (sl_reg)n) {
			return sl_true;
		}
		return sl_false;
	}

#define UTF16_SWAPPING_BUF_SIZE 0x2000

	sl_bool IWriter::writeTextUTF16LE(const StringParam& _text, sl_bool flagWriteByteOrderMark)
	{
		if (flagWriteByteOrderMark) {
			static sl_char8 sbuf[2] = {(sl_char8)0xFE, (sl_char8)0xFF};
			if (write(sbuf, 2) != 2) {
				return sl_false;
			}
		}
		StringData16 text(_text);
		sl_size n = text.getLength();
		if (n == 0) {
			return sl_true;
		}
		if (Endian::isLE()) {
			n <<= 1;
			if (write(text.getData(), n) == (sl_reg)n) {
				return sl_true;
			}
			return sl_false;
		} else {
			sl_char16* s = text.getData();
			sl_char16 buf[UTF16_SWAPPING_BUF_SIZE];
			while (n > 0) {
				sl_size m = UTF16_SWAPPING_BUF_SIZE;
				if (m > n) {
					m = n;
				}
				for (sl_size i = 0; i < m; i++) {
					sl_uint16 c = (sl_uint16)(s[i]);
					buf[i] = (sl_char16)((c >> 8) | (c << 8));
				}
				sl_size l = m << 1;
				if (write(buf, l) != (sl_reg)l) {
					return sl_false;
				}
				n -= m;
				s += m;
			}
			return sl_true;
		}
	}

	sl_bool IWriter::writeTextUTF16BE(const StringParam& _text, sl_bool flagWriteByteOrderMark)
	{
		if (flagWriteByteOrderMark) {
			static sl_char8 sbuf[2] = {(sl_char8)0xFF, (sl_char8)0xFE};
			if (write(sbuf, 2) != 2) {
				return sl_false;
			}
		}
		StringData16 text(_text);
		sl_size n = text.getLength();
		if (n == 0) {
			return sl_true;
		}
		if (Endian::isBE()) {
			n <<= 1;
			if (write(text.getData(), n) == (sl_reg)n) {
				return sl_true;
			}
			return sl_false;
		} else {
			sl_char16* s = text.getData();
			sl_char16 buf[UTF16_SWAPPING_BUF_SIZE];
			while (n > 0) {
				sl_size m = UTF16_SWAPPING_BUF_SIZE;
				if (m > n) {
					m = n;
				}
				for (sl_size i = 0; i < m; i++) {
					sl_uint16 c = (sl_uint16)(s[i]);
					buf[i] = (sl_char16)((c >> 8) | (c << 8));
				}
				sl_size l = m << 1;
				if (write(buf, l) != (sl_reg)l) {
					return sl_false;
				}
				n -= m;
				s += m;
			}
			return sl_true;
		}
	}


	IBlockReader::IBlockReader()
	{
	}

	IBlockReader::~IBlockReader()
	{
	}

	sl_int32 IBlockReader::readAt32(sl_uint64 offset, void* buf, sl_uint32 size)
	{
		return (sl_int32)(readAt(offset, buf, size));
	}

	sl_reg IBlockReader::readAt(sl_uint64 offset, void* buf, sl_size size)
	{
		if (size >= 0x40000000) {
			size = 0x40000000; // 1GB
		}
		return readAt32(offset, buf, (sl_uint32)size);
	}

	sl_reg IBlockReader::readFullyAt(sl_uint64 offset, void* _buf, sl_size size)
	{
		char* buf = (char*)_buf;
		if (size == 0) {
			return 0;
		}
		sl_size nRead = 0;
		while (nRead < size) {
			sl_size n = size - nRead;
			sl_reg m = readAt(offset + nRead, buf + nRead, n);
			if (m < 0) {
				if (nRead) {
					return nRead;
				} else {
					return m;
				}
			}
			nRead += m;
			if (Thread::isStoppingCurrent()) {
				return nRead;
			}
			if (m == 0) {
				Thread::sleep(1);
				if (Thread::isStoppingCurrent()) {
					return nRead;
				}
			}
		}
		return nRead;
	}


	IBlockWriter::IBlockWriter()
	{
	}

	IBlockWriter::~IBlockWriter()
	{
	}

	sl_int32 IBlockWriter::writeAt32(sl_uint64 offset, const void* buf, sl_uint32 size)
	{
		return (sl_int32)(writeAt(offset, buf, size));
	}

	sl_reg IBlockWriter::writeAt(sl_uint64 offset, const void* _buf, sl_size size)
	{
		char* buf = (char*)_buf;
		if (size == 0) {
			return 0;
		}
		sl_size nWrite = 0;
		while (nWrite < size) {
			sl_size n = size - nWrite;
			if (n > 0x40000000) {
				n = 0x40000000; // 1GB
			}
			sl_uint32 n32 = (sl_uint32)n;
			sl_int32 m = writeAt32(offset, buf + nWrite, n32);
			if (m <= 0) {
				break;
			}
			nWrite += m;
			if (m != n32 || Thread::isStoppingCurrent()) {
				return nWrite;
			}
		}
		return nWrite;
	}

	sl_reg IBlockWriter::writeFullyAt(sl_uint64 offset, const void* _buf, sl_size size)
	{
		char* buf = (char*)_buf;
		if (size == 0) {
			return 0;
		}
		sl_size nWrite = 0;
		while (nWrite < size) {
			sl_size n = size - nWrite;
			sl_reg m = writeAt(offset + nWrite, buf + nWrite, n);
			if (m < 0) {
				if (nWrite) {
					return nWrite;
				} else {
					return m;
				}
			}
			nWrite += m;
			if (Thread::isStoppingCurrent()) {
				return nWrite;
			}
			if (m == 0) {
				Thread::sleep(1);
				if (Thread::isStoppingCurrent()) {
					return nWrite;
				}
			}
		}
		return nWrite;
	}


	ISize::ISize()
	{
	}

	ISize::~ISize()
	{
	}

	sl_uint64 ISize::getSize()
	{
		sl_uint64 size;
		if (getSize(size)) {
			return size;
		}
		return 0;
	}


	IResizable::IResizable()
	{
	}

	IResizable::~IResizable()
	{
	}


	ISeekable::ISeekable()
	{
	}

	ISeekable::~ISeekable()
	{
	}

	sl_bool ISeekable::isEnd(sl_bool& outFlag)
	{
		sl_uint64 pos, size;
		if (getPosition(pos) && getSize(size)) {
			outFlag = pos >= size;
			return sl_true;
		}
		return sl_false;
	}

	sl_uint64 ISeekable::getPosition()
	{
		sl_uint64 pos;
		if (getPosition(pos)) {
			return pos;
		}
		return 0;
	}

	sl_bool ISeekable::isEnd()
	{
		sl_bool flag;
		if (isEnd(flag)) {
			return flag;
		}
		return sl_false;
	}

	sl_bool ISeekable::seekToBegin()
	{
		return seek(0, SeekPosition::Begin);
	}

	sl_bool ISeekable::seekToEnd()
	{
		return seek(0, SeekPosition::End);
	}


	IClosable::IClosable()
	{
	}

	IClosable::~IClosable()
	{
	}


	SLIB_DEFINE_OBJECT(Stream, Object)

	Stream::Stream()
	{
	}

	Stream::~Stream()
	{
	}
	

	String SeekableReaderHelper::readLine(IReader* reader, ISeekable* seekable)
	{
		StringBuffer sb;
		char buf[512];
		sl_bool flagNull = sl_true;
		for (;;) {
			sl_reg n = reader->read(buf, sizeof(buf));
			if (n <= 0) {
				break;
			}
			flagNull = sl_false;
			for (sl_reg i = 0; i < n; i++) {
				char ch = buf[i];
				if (ch == '\r' || ch == '\n') {
					if (i) {
						sb.add(String(buf, i));
					}
					if (ch == '\r') {
						if (i == n - 1) {
							if (reader->readUint8('\n') != '\n') {
								seekable->seek(-1, SeekPosition::Current);
							}
						} else {
							if (buf[i + 1] == '\n') {
								if (i != n - 2) {
									seekable->seek(i + 2 - n, SeekPosition::Current);
								}
							} else {
								seekable->seek(i + 1 - n, SeekPosition::Current);
							}
						}
					} else {
						if (i != n - 1) {
							seekable->seek(i + 1 - n, SeekPosition::Current);
						}
					}
					return sb.merge();
				}
			}
			if (!(sb.add(String(buf, n)))) {
				return sl_null;
			}
		}
		if (flagNull) {
			return sl_null;
		}
		return sb.merge();
	}

	String SeekableReaderHelper::readStringUntilWhitespace(IReader* reader, ISeekable* seekable)
	{
		StringBuffer sb;
		char buf[512];
		sl_bool flagNull = sl_true;
		sl_bool flagFoundStart = sl_false;
		for (;;) {
			sl_reg n = reader->read(buf, sizeof(buf));
			if (n <= 0) {
				break;
			}
			flagNull = sl_false;
			sl_reg start = 0;
			if (!flagFoundStart) {
				for (sl_reg i = 0; i < n; i++) {
					sl_char8 ch = buf[i];
					if (ch && !SLIB_CHAR_IS_WHITE_SPACE(buf[i])) {
						flagFoundStart = sl_true;
						start = i;
						break;
					}
				}
			}
			for (sl_reg i = start; i < n; i++) {
				char ch = buf[i];
				if (!ch || SLIB_CHAR_IS_WHITE_SPACE(ch)) {
					sb.add(String(buf + start, i - start));
					if (i != n - 1) {
						seekable->seek(i + 1 - n, SeekPosition::Current);
					}
					return sb.merge();
				}
			}
			sb.add(String(buf + start, n - start));
		}
		if (flagNull) {
			return sl_null;
		}
		return sb.merge();
	}

	String SeekableReaderHelper::readNullTerminatedString(IReader* reader, ISeekable* seekable)
	{
		StringBuffer sb;
		char buf[128];
		sl_bool flagNull = sl_true;
		for (;;) {
			sl_reg n = reader->read(buf, sizeof(buf));
			if (n <= 0) {
				break;
			}
			flagNull = sl_false;
			for (sl_reg i = 0; i < n; i++) {
				char ch = buf[i];
				if (!ch) {
					if (i) {
						sb.add(String(buf, i));
					}
					if (i != n - 1) {
						seekable->seek(i + 1 - n, SeekPosition::Current);
					}
					return sb.merge();
				}
			}
			if (!(sb.add(String(buf, n)))) {
				return sl_null;
			}
		}
		if (flagNull) {
			return sl_null;
		}
		return sb.merge();
	}
	
	Memory SeekableReaderHelper::readAllBytes(IReader* reader, ISeekable* seekable, sl_size maxSize)
	{
#if defined(SLIB_ARCH_IS_64BIT)
		sl_uint64 size = seekable->getSize();
#else
		sl_uint64 _size = seekable->getSize();
		if (_size > 0x7fffffff) {
			_size = 0x7fffffff;
		}
		sl_size size = (sl_size)_size;
#endif
		if (size > maxSize) {
			size = maxSize;
		}
		if (size == 0) {
			return sl_null;
		}
		Memory ret = Memory::create(size);
		if (ret.isNotNull()) {
			char* buf = (char*)(ret.getData());
			if (seekable->seekToBegin()) {
				if (reader->read(buf, size) == (sl_reg)size) {
					return ret;
				}
			}
		}
		return sl_null;
	}
	
	String SeekableReaderHelper::readAllTextUTF8(IReader* reader, ISeekable* seekable, sl_size maxSize)
	{
#if defined(SLIB_ARCH_IS_64BIT)
		sl_uint64 size = seekable->getSize();
#else
		sl_uint64 _size = seekable->getSize();
		if (_size > 0x7fffffff) {
			return sl_null;
		}
		sl_size size = (sl_size)_size;
#endif
		if (size > maxSize) {
			size = maxSize;
		}
		if (seekable->seekToBegin()) {
			return reader->readTextUTF8(size);
		}
		return sl_null;
	}
	
	String16 SeekableReaderHelper::readAllTextUTF16(IReader* reader, ISeekable* seekable, EndianType endian, sl_size maxSize)
	{
#if defined(SLIB_ARCH_IS_64BIT)
		sl_uint64 size = seekable->getSize();
#else
		sl_uint64 _size = seekable->getSize();
		if (_size > 0x7fffffff) {
			return sl_null;
		}
		sl_size size = (sl_size)_size;
#endif
		if (size > maxSize) {
			size = maxSize;
		}
		if (seekable->seekToBegin()) {
			return reader->readTextUTF16(size, endian);
		}
		return sl_null;
	}
	
	String SeekableReaderHelper::readAllText(IReader* reader, ISeekable* seekable, Charset* outCharset, sl_size maxSize)
	{
#if defined(SLIB_ARCH_IS_64BIT)
		sl_uint64 size = seekable->getSize();
#else
		sl_uint64 _size = seekable->getSize();
		if (_size > 0x7fffffff) {
			return sl_null;
		}
		sl_size size = (sl_size)_size;
#endif
		if (size > maxSize) {
			size = maxSize;
		}
		if (seekable->seekToBegin()) {
			return reader->readText(size, outCharset);
		}
		return sl_null;
	}
	
	String16 SeekableReaderHelper::readAllText16(IReader* reader, ISeekable* seekable, Charset* outCharset, sl_size maxSize)
	{
#if defined(SLIB_ARCH_IS_64BIT)
		sl_uint64 size = seekable->getSize();
#else
		sl_uint64 _size = seekable->getSize();
		if (_size > 0x7fffffff) {
			return sl_null;
		}
		sl_size size = (sl_size)_size;
#endif
		if (size > maxSize) {
			size = maxSize;
		}
		if (seekable->seekToBegin()) {
			return reader->readText16(size, outCharset);
		}
		return sl_null;
	}

	sl_int64 SeekableReaderHelper::find(IReader* reader, ISeekable* seekable, const void* _pattern, sl_size nPattern, sl_int64 _startPosition, sl_uint64 sizeFind)
	{
		sl_uint64 size = seekable->getSize();
		if (!size) {
			return -1;
		}
		if (!sizeFind) {
			return -1;
		}
		sl_uint64 startPosition;
		if (_startPosition < 0) {
			startPosition = 0;
		} else {
			startPosition = _startPosition;
			if (startPosition >= size) {
				return -1;
			}
		}
		if (!nPattern) {
			return startPosition;
		}
		sl_uint64 sizeRemain = size - startPosition;
		if (sizeFind > sizeRemain) {
			sizeFind = sizeRemain;
		}
		if (!(seekable->seek(startPosition, SeekPosition::Begin))) {
			return -1;
		}
		sl_uint8* pattern = (sl_uint8*)_pattern;
		sl_uint8 buf[1024];
		sl_reg posMatching = 0;
		sl_uint64 endPosition = startPosition + sizeFind;
		while (startPosition < endPosition) {
			sl_uint64 n = endPosition - startPosition;
			if (n > sizeof(buf)) {
				n = sizeof(buf);
			}
			sl_reg nRead = reader->readFully(buf, (sl_size)n);
			if (nRead <= 0) {
				return -1;
			}
			sl_size bMatching = posMatching != 0;
			sl_reg i = -posMatching;
			for (; i < nRead; i++) {
				sl_size k;
				if (bMatching) {
					k = posMatching;
					bMatching = sl_false;
				} else {
					k = 0;
				}
				for (; k < nPattern; k++) {
					sl_reg j = i + k;
					if (j >= nRead) {
						break;
					}
					if (j >= 0) {
						if (buf[j] != pattern[k]) {
							break;
						}
					} else {
						if (pattern[posMatching + j] != pattern[k]) {
							break;
						}
					}
				}
				if (k == nPattern) {
					return startPosition + i;
				}
				if (i + k == nRead) {
					posMatching = k;
					break;
				}
			}
			if (i == nRead) {
				posMatching = 0;
			}
			startPosition += nRead;
		}
		return -1;
	}

	sl_int64 SeekableReaderHelper::findBackward(IReader* reader, ISeekable* seekable, const void* _pattern, sl_size nPattern, sl_int64 _startPosition, sl_uint64 sizeFind)
	{
		sl_uint64 size = seekable->getSize();
		if (!size) {
			return -1;
		}
		if (!sizeFind) {
			return -1;
		}
		sl_uint64 startPosition;
		if (_startPosition < 0) {
			startPosition = size;
		} else {
			startPosition = _startPosition;
			if (startPosition >= size) {
				startPosition = size;
			}
		}
		if (!nPattern) {
			return startPosition;
		}
		if (sizeFind > startPosition) {
			sizeFind = startPosition;
		}
		sl_uint8* pattern = (sl_uint8*)_pattern;
		sl_uint8 buf[1024];
		sl_reg posMatching = 0;
		sl_uint64 endPosition = startPosition - sizeFind;
		while (endPosition < startPosition) {
			sl_uint64 _n = startPosition - endPosition;
			if (_n > sizeof(buf)) {
				_n = sizeof(buf);
			}
			sl_size n = (sl_size)_n;
			if (!(seekable->seek(startPosition - _n, SeekPosition::Begin))) {
				return -1;
			}
			sl_reg nRead = reader->readFully(buf, n);
			if (nRead != n) {
				return -1;
			}
			sl_size bMatching = posMatching != 0;
			sl_reg i = -posMatching;
			for (; i < nRead; i++) {
				sl_size k;
				if (bMatching) {
					k = posMatching;
					bMatching = sl_false;
				} else {
					k = 0;
				}
				for (; k < nPattern; k++) {
					sl_reg j = i + k;
					if (j >= nRead) {
						break;
					}
					if (j >= 0) {
						if (buf[nRead - 1 - j] != pattern[nPattern - 1 - k]) {
							break;
						}
					} else {
						if (pattern[nPattern - 1 - (posMatching + j)] != pattern[nPattern - 1 - k]) {
							break;
						}
					}
				}
				if (k == nPattern) {
					return startPosition - i - nPattern;
				}
				if (i + k == nRead) {
					posMatching = k;
					break;
				}
			}
			if (i == nRead) {
				posMatching = 0;
			}
			startPosition -= nRead;
		}
		return -1;
	}


	SLIB_DEFINE_OBJECT(IO, Stream)

	IO::IO()
	{
	}

	IO::~IO()
	{
	}


	SLIB_DEFINE_OBJECT(MemoryIO, IO)

	MemoryIO::MemoryIO()
	{
		_initialize();
	}

	MemoryIO::MemoryIO(sl_size size)
	{
		_initialize(size);
	}
	
	MemoryIO::MemoryIO(void* data, sl_size size)
	{
		_initialize(data, size);
	}

	MemoryIO::MemoryIO(const Memory& mem)
	{
		_initialize(mem);
	}

	MemoryIO::~MemoryIO()
	{
		_free();
	}

	void MemoryIO::_initialize()
	{
		m_buf = sl_null;
		m_size = 0;
		m_offset = 0;
		m_flagResizable = sl_true;
	}

	void MemoryIO::_initialize(sl_size size)
	{
		if (size) {
			Memory data = Memory::createResizable(size);
			if (data.isNotNull()) {
				m_buf = data.getData();
				m_size = size;
				m_offset = 0;
				m_flagResizable = sl_true;
				m_data = Move(data);
				return;
			}
		}
		_initialize();
	}

	void MemoryIO::_initialize(void* data, sl_size size)
	{
		m_flagResizable = sl_false;
		m_offset = 0;
		if (data && size) {
			m_buf = data;
			m_size = size;
		} else {
			m_buf = sl_null;
			m_size = 0;
		}
	}

	void MemoryIO::_initialize(const Memory& data)
	{
		m_offset = 0;
		if (data.isNotNull()) {
			m_buf = data.getData();
			m_size = data.getSize();
			m_flagResizable = !(data.isStatic());
			m_data = data;
		} else {
			m_buf = sl_null;
			m_size = 0;
			m_flagResizable = sl_false;
		}
	}

	sl_bool MemoryIO::_growCapacity(sl_size size)
	{
		if (!m_flagResizable) {
			return sl_false;
		}
		sl_size n = m_data.getSize();
		if (size < n) {
			m_size = size;
			return sl_true;
		}
		if (n < 16) {
			n = 16;
		} else {
			n = n + (n >> 1);
		}
		if (n < size) {
			n = size;
		}
		if (m_data.setSize(n)) {
			m_buf = m_data.getData();
			m_size = size;
			return sl_true;
		}
		return sl_false;
	}

	void MemoryIO::initialize()
	{
		m_data.setNull();
		_initialize();
	}

	void MemoryIO::initialize(sl_size size)
	{
		m_data.setNull();
		_initialize(size);
	}

	void MemoryIO::initialize(void* data, sl_size size)
	{
		m_data.setNull();
		_initialize(data, size);
	}

	void MemoryIO::initialize(const Memory& data)
	{
		m_data.setNull();
		_initialize(data);
	}

	void MemoryIO::close()
	{
		m_data.setNull();
		m_buf = sl_null;
		m_size = 0;
		m_offset = 0;
		m_flagResizable = sl_false;
	}

	sl_reg MemoryIO::read(void* buf, sl_size size)
	{
		if (!size) {
			return 0;
		}
		if (m_offset >= m_size) {
			return -1;
		}
		sl_size limit = m_size - m_offset;
		if (size > limit) {
			size = limit;
		}
		if (size > 0) {
			Base::copyMemory(buf, (char*)m_buf + m_offset, size);
			m_offset += size;
		}
		return size;
	}

	sl_reg MemoryIO::write(const void* buf, sl_size size)
	{
		if (!size) {
			return 0;
		}
		if (m_offset >= m_size) {
			return -1;
		}
		sl_size limit = m_size - m_offset;
		if (size > limit) {
			if (m_flagResizable) {
				sl_size limitMax = SLIB_SIZE_MAX - m_offset;
				if (size > limitMax) {
					size = limitMax;
				}
				if (!(_growCapacity(m_offset + size))) {
					size = limit;
				}
			} else {
				size = limit;
			}
		}
		if (size > 0) {
			Base::copyMemory((char*)m_buf + m_offset, buf, size);
			m_offset += size;
		}
		return size;
	}

	sl_bool MemoryIO::seek(sl_int64 offset, SeekPosition pos)
	{
		sl_uint64 p = m_offset;
		if (pos == SeekPosition::Begin) {
			p = 0;
		} else if (pos == SeekPosition::End) {
			p = m_size;
		}
		p += offset;
		if (p > m_size) {
			return sl_false;
		}
		m_offset = (sl_size)p;
		return sl_true;
	}

	sl_bool MemoryIO::getPosition(sl_uint64& outPos)
	{
		outPos = m_offset;
		return sl_true;
	}

	sl_bool MemoryIO::getSize(sl_uint64& outSize)
	{
		outSize = m_size;
		return sl_true;
	}

	sl_bool MemoryIO::setSize(sl_uint64 _size)
	{
		sl_size size = (sl_size)_size;
		if (!m_flagResizable) {
			return sl_false;
		}
		if (m_data.isNull()) {
			if (m_buf) {
				return sl_false;
			}
		}
		if (size < m_data.getSize()) {
			m_size = size;
			if (m_offset > size) {
				m_offset = size;
			}
			return sl_true;
		}
		if (m_data.setSize(size)) {
			m_buf = m_data.getData();
			m_size = size;
			return sl_true;
		}
		return sl_false;
	}

	sl_size MemoryIO::getPosition()
	{
		return m_offset;
	}

	sl_size MemoryIO::getSize()
	{
		return m_size;
	}

	sl_uint8* MemoryIO::getBuffer()
	{
		return (sl_uint8*)m_buf;
	}

	sl_bool MemoryIO::isResizable()
	{
		return m_flagResizable;
	}

	sl_bool MemoryIO::setResizable(sl_bool flag)
	{
		if (m_data.isNull()) {
			if (m_buf) {
				return sl_false;
			}
		}
		m_flagResizable = flag;
		return sl_true;
	}

	Memory MemoryIO::getData()
	{
		if (m_data.isNotNull()) {
			return m_data.sub(0, m_size);
		} else {
			return Memory::createStatic(m_buf, m_size);
		}
	}

	namespace priv
	{
		namespace io
		{
			static sl_bool FixFindMemoryPosition(sl_size& outStartPos, sl_size& outEndPos, sl_size size, sl_int64 startPos, sl_int64 endPos)
			{
				if (startPos < 0) {
					outStartPos = 0;
				} else if ((sl_uint64)startPos >= size) {
					return sl_false;
				} else {
					outStartPos = (sl_size)startPos;
				}
				if (!endPos) {
					return sl_false;
				} else if (endPos< 0) {
					outEndPos = size;
				} else if ((sl_size)endPos > size) {
					outEndPos = size;
				} else {
					outEndPos = (sl_size)endPos;
				}
				if (startPos >= endPos) {
					return sl_false;
				}
				return sl_true;
			}
		}
	}

	sl_int64 MemoryIO::find(const void* pattern, sl_size nPattern, sl_int64 _startPosition, sl_int64 _endPosition)
	{
		sl_size startPosition, endPosition;
		if (!(priv::io::FixFindMemoryPosition(startPosition, endPosition, m_size, _startPosition, _endPosition))) {
			return -1;
		}
		sl_uint8* buf = (sl_uint8*)m_buf;
		sl_uint8* p = Base::findMemory(buf + startPosition, endPosition - startPosition, pattern, nPattern);
		if (p) {
			return p - buf;
		}
		return -1;
	}

	sl_int64 MemoryIO::findBackward(const void* pattern, sl_size nPattern, sl_int64 _startPosition, sl_int64 _endPosition)
	{
		sl_size startPosition, endPosition;
		if (!(priv::io::FixFindMemoryPosition(startPosition, endPosition, m_size, _startPosition, _endPosition))) {
			return -1;
		}
		sl_uint8* buf = (sl_uint8*)m_buf;
		sl_uint8* p = Base::findMemoryBackward(buf + startPosition, endPosition - startPosition, pattern, nPattern);
		if (p) {
			return p - buf;
		}
		return -1;
	}


	SLIB_DEFINE_ROOT_OBJECT(MemoryReader)
	
	MemoryReader::MemoryReader(const Memory& mem)
	{
		initialize(mem);
	}

	MemoryReader::MemoryReader(const void* buf, sl_size size)
	{
		initialize(buf, size);
	}

	MemoryReader::~MemoryReader()
	{
	}
	
	void MemoryReader::initialize(const Memory& mem)
	{
		m_mem = mem;
		m_buf = mem.getData();
		m_size = mem.getSize();
		m_offset = 0;
	}
	
	void MemoryReader::initialize(const void* buf, sl_size size)
	{
		if (buf && size) {
			m_buf = buf;
			m_size = size;
		} else {
			m_buf = sl_null;
			m_size = 0;
		}
		m_offset = 0;
	}

	sl_reg MemoryReader::read(void* buf, sl_size size)
	{
		if (!size) {
			return 0;
		}
		if (m_offset >= m_size) {
			return -1;
		}
		sl_size limit = m_size - m_offset;
		if (size > limit) {
			size = limit;
		}
		if (size > 0) {
			Base::copyMemory(buf, (char*)m_buf + m_offset, size);
			m_offset += size;
		}
		return size;
	}

	sl_bool MemoryReader::seek(sl_int64 offset, SeekPosition pos)
	{
		sl_uint64 p = m_offset;
		if (pos == SeekPosition::Begin) {
			p = 0;
		} else if (pos == SeekPosition::End) {
			p = m_size;
		}
		p += offset;
		if (p > m_size) {
			return sl_false;
		}
		m_offset = (sl_size)p;
		return sl_true;
	}

	sl_bool MemoryReader::getPosition(sl_uint64& outPos)
	{
		outPos = m_offset;
		return sl_true;
	}

	sl_bool MemoryReader::getSize(sl_uint64& outSize)
	{
		outSize = m_size;
		return sl_true;
	}

	sl_size MemoryReader::getPosition()
	{
		return m_offset;
	}

	sl_size MemoryReader::getSize()
	{
		return m_size;
	}

	sl_uint8* MemoryReader::getBuffer()
	{
		return (sl_uint8*)m_buf;
	}

	sl_int64 MemoryReader::find(const void* pattern, sl_size nPattern, sl_int64 _startPosition, sl_int64 _endPosition)
	{
		sl_size startPosition, endPosition;
		if (!(priv::io::FixFindMemoryPosition(startPosition, endPosition, m_size, _startPosition, _endPosition))) {
			return -1;
		}
		sl_uint8* buf = (sl_uint8*)m_buf;
		sl_uint8* p = Base::findMemory(buf + startPosition, endPosition - startPosition, pattern, nPattern);
		if (p) {
			return p - buf;
		}
		return -1;
	}

	sl_int64 MemoryReader::findBackward(const void* pattern, sl_size nPattern, sl_int64 _startPosition, sl_int64 _endPosition)
	{
		sl_size startPosition, endPosition;
		if (!(priv::io::FixFindMemoryPosition(startPosition, endPosition, m_size, _startPosition, _endPosition))) {
			return -1;
		}
		sl_uint8* buf = (sl_uint8*)m_buf;
		sl_uint8* p = Base::findMemoryBackward(buf + startPosition, endPosition - startPosition, pattern, nPattern);
		if (p) {
			return p - buf;
		}
		return -1;
	}

	sl_bool MemoryReader::readInt8(sl_int8* output)
	{
		if (m_offset < m_size) {
			if (output) {
				*output = ((sl_int8*)m_buf)[m_offset];
			}
			m_offset++;
			return sl_true;
		} else {
			m_offset = m_size;
		}
		return sl_false;
	}

	sl_int8 MemoryReader::readInt8(sl_int8 def)
	{
		if (m_offset < m_size) {
			sl_int8 ret = ((sl_int8*)m_buf)[m_offset];
			m_offset++;
			return ret;
		} else {
			m_offset = m_size;
		}
		return def;
	}

	sl_bool MemoryReader::readUint8(sl_uint8* output)
	{
		if (m_offset < m_size) {
			if (output) {
				*output = ((sl_uint8*)m_buf)[m_offset];
			}
			m_offset++;
			return sl_true;
		} else {
			m_offset = m_size;
		}
		return sl_false;
	}

	sl_uint8 MemoryReader::readUint8(sl_uint8 def)
	{
		if (m_offset < m_size) {
			sl_uint8 ret = ((sl_int8*)m_buf)[m_offset];
			m_offset++;
			return ret;
		} else {
			m_offset = m_size;
		}
		return def;
	}

	sl_bool MemoryReader::readInt16(sl_int16* output, EndianType endian)
	{
		sl_size offsetNext = m_offset + 2;
		if (offsetNext <= m_size) {
			if (output) {
				*output = MIO::readInt16((char*)m_buf + m_offset, endian);
			}
			m_offset = offsetNext;
			return sl_true;
		} else {
			m_offset = m_size;
		}
		return sl_false;
	}

	sl_int16 MemoryReader::readInt16(sl_int16 def, EndianType endian)
	{
		sl_size offsetNext = m_offset + 2;
		if (offsetNext <= m_size) {
			sl_int16 ret = MIO::readInt16((char*)m_buf + m_offset, endian);
			m_offset = offsetNext;
			return ret;
		} else {
			m_offset = m_size;
		}
		return def;
	}

	sl_bool MemoryReader::readUint16(sl_uint16* output, EndianType endian)
	{
		sl_size offsetNext = m_offset + 2;
		if (offsetNext <= m_size) {
			if (output) {
				*output = MIO::readUint16((char*)m_buf + m_offset, endian);
			}
			m_offset = offsetNext;
			return sl_true;
		} else {
			m_offset = m_size;
		}
		return sl_false;
	}

	sl_uint16 MemoryReader::readUint16(sl_uint16 def, EndianType endian)
	{
		sl_size offsetNext = m_offset + 2;
		if (offsetNext <= m_size) {
			sl_uint16 ret = MIO::readUint16((char*)m_buf + m_offset, endian);
			m_offset = offsetNext;
			return ret;
		} else {
			m_offset = m_size;
		}
		return def;
	}

	sl_bool MemoryReader::readInt32(sl_int32* output, EndianType endian)
	{
		sl_size offsetNext = m_offset + 4;
		if (offsetNext <= m_size) {
			if (output) {
				*output = MIO::readInt32((char*)m_buf + m_offset, endian);
			}
			m_offset = offsetNext;
			return sl_true;
		} else {
			m_offset = m_size;
		}
		return sl_false;
	}

	sl_int32 MemoryReader::readInt32(sl_int32 def, EndianType endian)
	{
		sl_size offsetNext = m_offset + 4;
		if (offsetNext <= m_size) {
			sl_int32 ret = MIO::readInt32((char*)m_buf + m_offset, endian);
			m_offset = offsetNext;
			return ret;
		} else {
			m_offset = m_size;
		}
		return def;
	}

	sl_bool MemoryReader::readUint32(sl_uint32* output, EndianType endian)
	{
		sl_size offsetNext = m_offset + 4;
		if (offsetNext <= m_size) {
			if (output) {
				*output = MIO::readUint32((char*)m_buf + m_offset, endian);
			}
			m_offset = offsetNext;
			return sl_true;
		} else {
			m_offset = m_size;
		}
		return sl_false;
	}

	sl_uint32 MemoryReader::readUint32(sl_uint32 def, EndianType endian)
	{
		sl_size offsetNext = m_offset + 4;
		if (offsetNext <= m_size) {
			sl_uint32 ret = MIO::readUint32((char*)m_buf + m_offset, endian);
			m_offset = offsetNext;
			return ret;
		} else {
			m_offset = m_size;
		}
		return def;
	}

	sl_bool MemoryReader::readInt64(sl_int64* output, EndianType endian)
	{
		sl_size offsetNext = m_offset + 8;
		if (offsetNext <= m_size) {
			if (output) {
				*output = MIO::readInt64((char*)m_buf + m_offset, endian);
			}
			m_offset = offsetNext;
			return sl_true;
		} else {
			m_offset = m_size;
		}
		return sl_false;
	}

	sl_int64 MemoryReader::readInt64(sl_int64 def, EndianType endian)
	{
		sl_size offsetNext = m_offset + 8;
		if (offsetNext <= m_size) {
			sl_int64 ret = MIO::readInt64((char*)m_buf + m_offset, endian);
			m_offset = offsetNext;
			return ret;
		} else {
			m_offset = m_size;
		}
		return def;
	}

	sl_bool MemoryReader::readUint64(sl_uint64* output, EndianType endian)
	{
		sl_size offsetNext = m_offset + 8;
		if (offsetNext <= m_size) {
			if (output) {
				*output = MIO::readUint64((char*)m_buf + m_offset, endian);
			}
			m_offset = offsetNext;
			return sl_true;
		} else {
			m_offset = m_size;
		}
		return sl_false;
	}

	sl_uint64 MemoryReader::readUint64(sl_uint64 def, EndianType endian)
	{
		sl_size offsetNext = m_offset + 8;
		if (offsetNext <= m_size) {
			sl_uint64 ret = MIO::readUint64((char*)m_buf + m_offset, endian);
			m_offset = offsetNext;
			return ret;
		} else {
			m_offset = m_size;
		}
		return def;
	}

	sl_bool MemoryReader::readFloat(float* output, EndianType endian)
	{
		sl_size offsetNext = m_offset + 4;
		if (offsetNext <= m_size) {
			if (output) {
				*output = MIO::readFloat((char*)m_buf + m_offset, endian);
			}
			m_offset = offsetNext;
			return sl_true;
		} else {
			m_offset = m_size;
		}
		return sl_false;
	}

	float MemoryReader::readFloat(float def, EndianType endian)
	{
		sl_size offsetNext = m_offset + 4;
		if (offsetNext <= m_size) {
			float ret = MIO::readFloat((char*)m_buf + m_offset, endian);
			m_offset = offsetNext;
			return ret;
		} else {
			m_offset = m_size;
		}
		return def;
	}

	sl_bool MemoryReader::readDouble(double* output, EndianType endian)
	{
		sl_size offsetNext = m_offset + 8;
		if (offsetNext <= m_size) {
			if (output) {
				*output = MIO::readDouble((char*)m_buf + m_offset, endian);
			}
			m_offset = offsetNext;
			return sl_true;
		} else {
			m_offset = m_size;
		}
		return sl_false;
	}

	double MemoryReader::readDouble(double def, EndianType endian)
	{
		sl_size offsetNext = m_offset + 8;
		if (offsetNext <= m_size) {
			double ret = MIO::readDouble((char*)m_buf + m_offset, endian);
			m_offset = offsetNext;
			return ret;
		} else {
			m_offset = m_size;
		}
		return def;
	}

	
	SLIB_DEFINE_ROOT_OBJECT(MemoryWriter)

	MemoryWriter::MemoryWriter(const Memory& mem)
	{
		initialize(mem);
	}

	MemoryWriter::MemoryWriter(void* buf, sl_size size)
	{
		initialize(buf, size);
	}

	MemoryWriter::~MemoryWriter()
	{
	}
	
	void MemoryWriter::initialize(const Memory& mem)
	{
		m_mem = mem;
		m_buf = mem.getData();
		m_size = mem.getSize();
		m_offset = 0;
	}
	
	void MemoryWriter::initialize(void* buf, sl_size size)
	{
		if (buf && size) {
			m_buf = buf;
			m_size = size;
		} else {
			m_buf = sl_null;
			m_size = 0;
		}
		m_offset = 0;
	}
	
	sl_reg MemoryWriter::write(const void* buf, sl_size size)
	{
		if (!size) {
			return 0;
		}
		if (m_offset >= m_size) {
			return -1;
		}
		sl_size limit = m_size - m_offset;
		if (size > limit) {
			size = limit;
		}
		if (size > 0) {
			Base::copyMemory((char*)m_buf + m_offset, buf, size);
			m_offset += size;
		}
		return size;
	}

	sl_reg MemoryWriter::write(const Memory& mem)
	{
		return write(mem.getData(), mem.getSize());
	}

	sl_bool MemoryWriter::seek(sl_int64 offset, SeekPosition pos)
	{
		sl_uint64 p = m_offset;
		if (pos == SeekPosition::Begin) {
			p = 0;
		} else if (pos == SeekPosition::End) {
			p = m_size;
		}
		p += offset;
		if (p > m_size) {
			return sl_false;
		}
		m_offset = (sl_size)p;
		return sl_true;
	}

	sl_bool MemoryWriter::getPosition(sl_uint64& outPos)
	{
		outPos = m_offset;
		return sl_true;
	}

	sl_bool MemoryWriter::getSize(sl_uint64& outSize)
	{
		outSize = m_size;
		return sl_true;
	}

	sl_size MemoryWriter::getPosition()
	{
		return m_offset;
	}

	sl_size MemoryWriter::getSize()
	{
		return m_size;
	}

	sl_uint8* MemoryWriter::getBuffer()
	{
		return (sl_uint8*)m_buf;
	}

	sl_bool MemoryWriter::writeInt8(sl_int8 value)
	{
		if (m_offset < m_size) {
			((sl_int8*)m_buf)[m_offset] = value;
			m_offset++;
			return sl_true;
		} else {
			m_offset = m_size;
		}
		return sl_false;
	}

	sl_bool MemoryWriter::writeUint8(sl_uint8 value)
	{
		if (m_offset < m_size) {
			((sl_uint8*)m_buf)[m_offset] = value;
			m_offset++;
			return sl_true;
		} else {
			m_offset = m_size;
		}
		return sl_false;
	}

	sl_bool MemoryWriter::writeInt16(sl_int16 value, EndianType endian)
	{
		sl_size offsetNext = m_offset + 2;
		if (offsetNext <= m_size) {
			MIO::writeInt16((char*)m_buf + m_offset, value, endian);
			m_offset = offsetNext;
			return sl_true;
		} else {
			m_offset = m_size;
		}
		return sl_false;
	}

	sl_bool MemoryWriter::writeUint16(sl_uint16 value, EndianType endian)
	{
		sl_size offsetNext = m_offset + 2;
		if (offsetNext <= m_size) {
			MIO::writeUint16((char*)m_buf + m_offset, value, endian);
			m_offset = offsetNext;
			return sl_true;
		} else {
			m_offset = m_size;
		}
		return sl_false;
	}

	sl_bool MemoryWriter::writeInt32(sl_int32 value, EndianType endian)
	{
		sl_size offsetNext = m_offset + 4;
		if (offsetNext <= m_size) {
			MIO::writeInt32((char*)m_buf + m_offset, value, endian);
			m_offset = offsetNext;
			return sl_true;
		} else {
			m_offset = m_size;
		}
		return sl_false;
	}

	sl_bool MemoryWriter::writeUint32(sl_uint32 value, EndianType endian)
	{
		sl_size offsetNext = m_offset + 4;
		if (offsetNext <= m_size) {
			MIO::writeUint32((char*)m_buf + m_offset, value, endian);
			m_offset = offsetNext;
			return sl_true;
		} else {
			m_offset = m_size;
		}
		return sl_false;
	}

	sl_bool MemoryWriter::writeInt64(sl_int64 value, EndianType endian)
	{
		sl_size offsetNext = m_offset + 8;
		if (offsetNext <= m_size) {
			MIO::writeInt64((char*)m_buf + m_offset, value, endian);
			m_offset = offsetNext;
			return sl_true;
		} else {
			m_offset = m_size;
		}
		return sl_false;
	}

	sl_bool MemoryWriter::writeUint64(sl_uint64 value, EndianType endian)
	{
		sl_size offsetNext = m_offset + 8;
		if (offsetNext <= m_size) {
			MIO::writeUint64((char*)m_buf + m_offset, value, endian);
			m_offset = offsetNext;
			return sl_true;
		} else {
			m_offset = m_size;
		}
		return sl_false;
	}

	sl_bool MemoryWriter::writeFloat(float value, EndianType endian)
	{
		sl_size offsetNext = m_offset + 4;
		if (offsetNext <= m_size) {
			MIO::writeFloat((char*)m_buf + m_offset, value, endian);
			m_offset = offsetNext;
			return sl_true;
		} else {
			m_offset = m_size;
		}
		return sl_false;
	}

	sl_bool MemoryWriter::writeDouble(double value, EndianType endian)
	{
		sl_size offsetNext = m_offset + 8;
		if (offsetNext <= m_size) {
			MIO::writeDouble((char*)m_buf + m_offset, value, endian);
			m_offset = offsetNext;
			return sl_true;
		} else {
			m_offset = m_size;
		}
		return sl_false;
	}

	
	SLIB_DEFINE_ROOT_OBJECT(MemoryOutput)

	MemoryOutput::MemoryOutput()
	{
	}

	MemoryOutput::~MemoryOutput()
	{
	}

	sl_reg MemoryOutput::write(const void* buf, sl_size size)
	{
		if (!size) {
			return 0;
		}
		if (size <= 64) {
			if (m_buffer.addElements_NoLock((sl_uint8*)buf, size)) {
				return size;
			}
		} else {
			if (flush()) {
				Memory mem = Memory::create(buf, size);
				if (mem.isNotNull()) {
					if (m_queue.add(mem)) {
						return size;
					}
				}
			}
		}
		return -1;
	}

	sl_reg MemoryOutput::write(const Memory& mem)
	{
		if (mem.isNull()) {
			return 0;
		}
		if (flush()) {
			if (m_queue.add(mem)) {
				return mem.getSize();
			}
		}
		return -1;
	}

	sl_bool MemoryOutput::flush()
	{
		sl_size n = m_buffer.getCount();
		if (!n) {
			return sl_true;
		}
		MemoryData mem;
		mem.data = m_buffer.getData();
		mem.size = n;
		mem.refer = Move(m_buffer.ref);
		if (m_queue.add(mem)) {
			return sl_true;
		}
		m_buffer.ref = Move(Ref< CList<sl_uint8> >::from(mem.refer));
		return sl_false;
	}

	sl_size MemoryOutput::getSize()
	{
		return m_queue.getSize() + m_buffer.getCount();
	}

	Memory MemoryOutput::getData()
	{
		flush();
		return m_queue.merge();
	}

	sl_bool MemoryOutput::writeInt8(sl_int8 value)
	{
		return m_buffer.add_NoLock(value);
	}

	sl_bool MemoryOutput::writeUint8(sl_uint8 value)
	{
		return m_buffer.add_NoLock(value);
	}

	sl_bool MemoryOutput::writeInt16(sl_int16 value, EndianType endian)
	{
		sl_uint8 v[2];
		MIO::writeInt16(v, value, endian);
		return m_buffer.addElements_NoLock(v, 2);
	}

	sl_bool MemoryOutput::writeUint16(sl_uint16 value, EndianType endian)
	{
		sl_uint8 v[2];
		MIO::writeUint16(v, value, endian);
		return m_buffer.addElements_NoLock(v, 2);
	}

	sl_bool MemoryOutput::writeInt32(sl_int32 value, EndianType endian)
	{
		sl_uint8 v[4];
		MIO::writeInt32(v, value, endian);
		return m_buffer.addElements_NoLock(v, 4);
	}

	sl_bool MemoryOutput::writeUint32(sl_uint32 value, EndianType endian)
	{
		sl_uint8 v[4];
		MIO::writeUint32(v, value, endian);
		return m_buffer.addElements_NoLock(v, 4);
	}

	sl_bool MemoryOutput::writeInt64(sl_int64 value, EndianType endian)
	{
		sl_uint8 v[8];
		MIO::writeInt64(v, value, endian);
		return m_buffer.addElements_NoLock(v, 8);
	}

	sl_bool MemoryOutput::writeUint64(sl_uint64 value, EndianType endian)
	{
		sl_uint8 v[8];
		MIO::writeUint64(v, value, endian);
		return m_buffer.addElements_NoLock(v, 8);
	}

	sl_bool MemoryOutput::writeFloat(float value, EndianType endian)
	{
		sl_uint8 v[4];
		MIO::writeFloat(v, value, endian);
		return m_buffer.addElements_NoLock(v, 4);
	}

	sl_bool MemoryOutput::writeDouble(double value, EndianType endian)
	{
		sl_uint8 v[8];
		MIO::writeDouble(v, value, endian);
		return m_buffer.addElements_NoLock(v, 8);
	}


	SLIB_DEFINE_ROOT_OBJECT(BufferedReader)

	BufferedReader::BufferedReader(): m_reader(sl_null), m_closable(sl_null), m_posInBuf(0), m_sizeRead(0), m_dataBuf(sl_null), m_sizeBuf(0)
	{
	}

	BufferedReader::~BufferedReader()
	{
	}

	Ref<BufferedReader> BufferedReader::create(const Ptrx<IReader, IClosable>& _obj, sl_size bufferSize)
	{
		if (!bufferSize) {
			return sl_null;
		}
		Ptrx<IReader, IClosable> obj = _obj.lock();
		if (!(obj.ptr)) {
			return sl_null;
		}
		Memory buf = Memory::create(bufferSize);
		if (buf.isNull()) {
			return sl_null;
		}

		Ref<BufferedReader> ret = new BufferedReader;
		if (ret.isNotNull()) {
			ret->_init(obj, buf);
			return ret;
		}
		return sl_null;
	}

	sl_reg BufferedReader::read(void* buf, sl_size size)
	{
		if (!size) {
			return -1;
		}
		IReader* reader = m_reader;
		if (!reader) {
			return -1;
		}
		sl_size nAvailable = m_sizeRead - m_posInBuf;
		if (!nAvailable) {
			if (size >= m_sizeBuf) {
				return reader->read(buf, size);
			}
			m_posInBuf = 0;
			sl_reg nRead = reader->read(m_dataBuf, m_sizeBuf);
			if (nRead <= 0) {
				m_sizeRead = 0;
				return nRead;
			}
			m_sizeRead = nRead;
			nAvailable = nRead;
		}
		if (size > nAvailable) {
			size = nAvailable;
		}
		Base::copyMemory(buf, m_dataBuf + m_posInBuf, size);
		m_posInBuf += size;
		return size;
	}

	void BufferedReader::close()
	{
		if (m_closable) {
			m_closable->close();
		}
		m_reader = sl_null;
		m_closable = sl_null;
		m_ref.setNull();
	}

	sl_bool BufferedReader::readInt8(sl_int8* output)
	{
		if (m_posInBuf < m_sizeRead) {
			if (output) {
				*output = m_dataBuf[m_posInBuf];
			}
			m_posInBuf++;
			return sl_true;
		} else {
			return IReader::readInt8(output);
		}
	}

	sl_int8 BufferedReader::readInt8(sl_int8 def)
	{
		if (m_posInBuf < m_sizeRead) {
			sl_int8 ret = m_dataBuf[m_posInBuf];
			m_posInBuf++;
			return ret;
		} else {
			return IReader::readInt8(def);
		}
	}

	sl_bool BufferedReader::readUint8(sl_uint8* output)
	{
		if (m_posInBuf < m_sizeRead) {
			if (output) {
				*output = m_dataBuf[m_posInBuf];
			}
			m_posInBuf++;
			return sl_true;
		} else {
			return IReader::readUint8(output);
		}
	}

	sl_uint8 BufferedReader::readUint8(sl_uint8 def)
	{
		if (m_posInBuf < m_sizeRead) {
			sl_uint8 ret = m_dataBuf[m_posInBuf];
			m_posInBuf++;
			return ret;
		} else {
			return IReader::readUint8(def);
		}
	}

	sl_bool BufferedReader::readInt16(sl_int16* output, EndianType endian)
	{
		sl_size offsetNext = m_posInBuf + 2;
		if (offsetNext <= m_sizeRead) {
			if (output) {
				*output = MIO::readInt16(m_dataBuf + m_posInBuf, endian);
			}
			m_posInBuf = offsetNext;
			return sl_true;
		} else {
			return IReader::readInt16(output, endian);
		}
	}

	sl_int16 BufferedReader::readInt16(sl_int16 def, EndianType endian)
	{
		sl_size offsetNext = m_posInBuf + 2;
		if (offsetNext <= m_sizeRead) {
			sl_int16 ret = MIO::readInt16(m_dataBuf + m_posInBuf, endian);
			m_posInBuf = offsetNext;
			return ret;
		} else {
			return IReader::readInt16(def, endian);
		}
	}

	sl_bool BufferedReader::readUint16(sl_uint16* output, EndianType endian)
	{
		sl_size offsetNext = m_posInBuf + 2;
		if (offsetNext <= m_sizeRead) {
			if (output) {
				*output = MIO::readUint16(m_dataBuf + m_posInBuf, endian);
			}
			m_posInBuf = offsetNext;
			return sl_true;
		} else {
			return IReader::readUint16(output, endian);
		}
	}

	sl_uint16 BufferedReader::readUint16(sl_uint16 def, EndianType endian)
	{
		sl_size offsetNext = m_posInBuf + 2;
		if (offsetNext <= m_sizeRead) {
			sl_uint16 ret = MIO::readUint16(m_dataBuf + m_posInBuf, endian);
			m_posInBuf = offsetNext;
			return ret;
		} else {
			return IReader::readUint16(def, endian);
		}
	}

	sl_bool BufferedReader::readInt32(sl_int32* output, EndianType endian)
	{
		sl_size offsetNext = m_posInBuf + 4;
		if (offsetNext <= m_sizeRead) {
			if (output) {
				*output = MIO::readInt32(m_dataBuf + m_posInBuf, endian);
			}
			m_posInBuf = offsetNext;
			return sl_true;
		} else {
			return IReader::readInt32(output, endian);
		}
	}

	sl_int32 BufferedReader::readInt32(sl_int32 def, EndianType endian)
	{
		sl_size offsetNext = m_posInBuf + 4;
		if (offsetNext <= m_sizeRead) {
			sl_int32 ret = MIO::readInt32(m_dataBuf + m_posInBuf, endian);
			m_posInBuf = offsetNext;
			return ret;
		} else {
			return IReader::readInt32(def, endian);
		}
	}

	sl_bool BufferedReader::readUint32(sl_uint32* output, EndianType endian)
	{
		sl_size offsetNext = m_posInBuf + 4;
		if (offsetNext <= m_sizeRead) {
			if (output) {
				*output = MIO::readUint32(m_dataBuf + m_posInBuf, endian);
			}
			m_posInBuf = offsetNext;
			return sl_true;
		} else {
			return IReader::readUint32(output, endian);
		}
	}

	sl_uint32 BufferedReader::readUint32(sl_uint32 def, EndianType endian)
	{
		sl_size offsetNext = m_posInBuf + 4;
		if (offsetNext <= m_sizeRead) {
			sl_uint32 ret = MIO::readUint32(m_dataBuf + m_posInBuf, endian);
			m_posInBuf = offsetNext;
			return ret;
		} else {
			return IReader::readUint32(def, endian);
		}
	}

	sl_bool BufferedReader::readInt64(sl_int64* output, EndianType endian)
	{
		sl_size offsetNext = m_posInBuf + 8;
		if (offsetNext <= m_sizeRead) {
			if (output) {
				*output = MIO::readInt64(m_dataBuf + m_posInBuf, endian);
			}
			m_posInBuf = offsetNext;
			return sl_true;
		} else {
			return IReader::readInt64(output, endian);
		}
	}

	sl_int64 BufferedReader::readInt64(sl_int64 def, EndianType endian)
	{
		sl_size offsetNext = m_posInBuf + 8;
		if (offsetNext <= m_sizeRead) {
			sl_int64 ret = MIO::readInt64(m_dataBuf + m_posInBuf, endian);
			m_posInBuf = offsetNext;
			return ret;
		} else {
			return IReader::readInt64(def, endian);
		}
	}

	sl_bool BufferedReader::readUint64(sl_uint64* output, EndianType endian)
	{
		sl_size offsetNext = m_posInBuf + 8;
		if (offsetNext <= m_sizeRead) {
			if (output) {
				*output = MIO::readUint64(m_dataBuf + m_posInBuf, endian);
			}
			m_posInBuf = offsetNext;
			return sl_true;
		} else {
			return IReader::readUint64(output, endian);
		}
	}

	sl_uint64 BufferedReader::readUint64(sl_uint64 def, EndianType endian)
	{
		sl_size offsetNext = m_posInBuf + 8;
		if (offsetNext <= m_sizeRead) {
			sl_uint64 ret = MIO::readUint64(m_dataBuf + m_posInBuf, endian);
			m_posInBuf = offsetNext;
			return ret;
		} else {
			return IReader::readUint64(def, endian);
		}
	}

	sl_bool BufferedReader::readFloat(float* output, EndianType endian)
	{
		sl_size offsetNext = m_posInBuf + 4;
		if (offsetNext <= m_sizeRead) {
			if (output) {
				*output = MIO::readFloat(m_dataBuf + m_posInBuf, endian);
			}
			m_posInBuf = offsetNext;
			return sl_true;
		} else {
			return IReader::readFloat(output, endian);
		}
	}

	float BufferedReader::readFloat(float def, EndianType endian)
	{
		sl_size offsetNext = m_posInBuf + 4;
		if (offsetNext <= m_sizeRead) {
			float ret = MIO::readFloat(m_dataBuf + m_posInBuf, endian);
			m_posInBuf = offsetNext;
			return ret;
		} else {
			return IReader::readFloat(def, endian);
		}
	}

	sl_bool BufferedReader::readDouble(double* output, EndianType endian)
	{
		sl_size offsetNext = m_posInBuf + 8;
		if (offsetNext <= m_sizeRead) {
			if (output) {
				*output = MIO::readDouble(m_dataBuf + m_posInBuf, endian);
			}
			m_posInBuf = offsetNext;
			return sl_true;
		} else {
			return IReader::readDouble(output, endian);
		}
	}

	double BufferedReader::readDouble(double def, EndianType endian)
	{
		sl_size offsetNext = m_posInBuf + 8;
		if (offsetNext <= m_sizeRead) {
			double ret = MIO::readDouble(m_dataBuf + m_posInBuf, endian);
			m_posInBuf = offsetNext;
			return ret;
		} else {
			return IReader::readDouble(def, endian);
		}
	}

	void BufferedReader::_init(const Ptrx<IReader, IClosable>& reader, const Memory& buf)
	{
		m_ref = reader.ref;
		m_reader = reader;
		m_closable = reader;

		m_buf = buf;
		m_dataBuf = (sl_uint8*)(buf.getData());
		m_sizeBuf = buf.getSize();
	}

	
	SLIB_DEFINE_ROOT_OBJECT(BufferedWriter)

	BufferedWriter::BufferedWriter(): m_writer(sl_null), m_closable(sl_null), m_dataBuf(sl_null), m_sizeBuf(0), m_sizeWritten(0)
	{
	}

	BufferedWriter::~BufferedWriter()
	{
		flush();
	}

	Ref<BufferedWriter> BufferedWriter::create(const Ptrx<IWriter, IClosable>& _obj, sl_size bufferSize)
	{
		if (!bufferSize) {
			return sl_null;
		}
		Memory buf = Memory::create(bufferSize);
		if (buf.isNull()) {
			return sl_null;
		}
		Ptrx<IWriter, IClosable> obj = _obj.lock();
		if (!(obj.ptr)) {
			return sl_null;
		}
		Ref<BufferedWriter> ret = new BufferedWriter;
		if (ret.isNotNull()) {
			ret->_init(obj, buf);
			return ret;
		}
		return sl_null;
	}

	sl_reg BufferedWriter::write(const void* buf, sl_size size)
	{
		if (!size) {
			return -1;
		}
		IWriter* writer = m_writer;
		if (!writer) {
			return -1;
		}
		if (size <= m_sizeBuf - m_sizeWritten) {
			Base::copyMemory(m_dataBuf + m_sizeWritten, buf, size);
			m_sizeWritten += size;
			return size;
		} else {
			if (flush()) {
				return writer->write(buf, size);
			}
		}
		return -1;
	}

	void BufferedWriter::close()
	{
		flush();
		if (m_closable) {
			m_closable->close();
		}
		m_writer = sl_null;
		m_closable = sl_null;
		m_ref.setNull();
	}

	sl_bool BufferedWriter::flush()
	{
		sl_size size = m_sizeWritten;
		if (!size) {
			return sl_true;
		}
		IWriter* writer = m_writer;
		if (!writer) {
			return sl_false;
		}
		sl_reg n = writer->writeFully(m_dataBuf, size);
		if (n == size) {
			m_sizeWritten = 0;
			return sl_true;
		}
		if (n <= 0) {
			return sl_false;
		}
		size -= n;
		Base::moveMemory(m_dataBuf, m_dataBuf + n, size);
		m_sizeWritten = size;
		return sl_false;
	}

	sl_bool BufferedWriter::writeInt8(sl_int8 value)
	{
		if (m_sizeWritten < m_sizeBuf) {
			m_dataBuf[m_sizeWritten] = value;
			m_sizeWritten++;
			return sl_true;
		} else {
			return IWriter::writeInt8(value);
		}
	}

	sl_bool BufferedWriter::writeUint8(sl_uint8 value)
	{
		if (m_sizeWritten < m_sizeBuf) {
			m_dataBuf[m_sizeWritten] = value;
			m_sizeWritten++;
			return sl_true;
		} else {
			return IWriter::writeUint8(value);
		}
	}

	sl_bool BufferedWriter::writeInt16(sl_int16 value, EndianType endian)
	{
		sl_size offsetNext = m_sizeWritten + 2;
		if (offsetNext <= m_sizeBuf) {
			MIO::writeInt16(m_dataBuf + m_sizeWritten, value, endian);
			m_sizeWritten = offsetNext;
			return sl_true;
		} else {
			return IWriter::writeInt16(value, endian);
		}
	}

	sl_bool BufferedWriter::writeUint16(sl_uint16 value, EndianType endian)
	{
		sl_size offsetNext = m_sizeWritten + 2;
		if (offsetNext <= m_sizeBuf) {
			MIO::writeUint16(m_dataBuf + m_sizeWritten, value, endian);
			m_sizeWritten = offsetNext;
			return sl_true;
		} else {
			return IWriter::writeUint16(value, endian);
		}
	}

	sl_bool BufferedWriter::writeInt32(sl_int32 value, EndianType endian)
	{
		sl_size offsetNext = m_sizeWritten + 4;
		if (offsetNext <= m_sizeBuf) {
			MIO::writeInt32(m_dataBuf + m_sizeWritten, value, endian);
			m_sizeWritten = offsetNext;
			return sl_true;
		} else {
			return IWriter::writeInt32(value, endian);
		}
	}

	sl_bool BufferedWriter::writeUint32(sl_uint32 value, EndianType endian)
	{
		sl_size offsetNext = m_sizeWritten + 4;
		if (offsetNext <= m_sizeBuf) {
			MIO::writeUint32(m_dataBuf + m_sizeWritten, value, endian);
			m_sizeWritten = offsetNext;
			return sl_true;
		} else {
			return IWriter::writeUint32(value, endian);
		}
	}

	sl_bool BufferedWriter::writeInt64(sl_int64 value, EndianType endian)
	{
		sl_size offsetNext = m_sizeWritten + 8;
		if (offsetNext <= m_sizeBuf) {
			MIO::writeInt64(m_dataBuf + m_sizeWritten, value, endian);
			m_sizeWritten = offsetNext;
			return sl_true;
		} else {
			return IWriter::writeInt64(value, endian);
		}
	}

	sl_bool BufferedWriter::writeUint64(sl_uint64 value, EndianType endian)
	{
		sl_size offsetNext = m_sizeWritten + 8;
		if (offsetNext <= m_sizeBuf) {
			MIO::writeUint64(m_dataBuf + m_sizeWritten, value, endian);
			m_sizeWritten = offsetNext;
			return sl_true;
		} else {
			return IWriter::writeUint64(value, endian);
		}
	}

	sl_bool BufferedWriter::writeFloat(float value, EndianType endian)
	{
		sl_size offsetNext = m_sizeWritten + 4;
		if (offsetNext <= m_sizeBuf) {
			MIO::writeFloat(m_dataBuf + m_sizeWritten, value, endian);
			m_sizeWritten = offsetNext;
			return sl_true;
		} else {
			return IWriter::writeFloat(value, endian);
		}
	}

	sl_bool BufferedWriter::writeDouble(double value, EndianType endian)
	{
		sl_size offsetNext = m_sizeWritten + 8;
		if (offsetNext <= m_sizeBuf) {
			MIO::writeDouble(m_dataBuf + m_sizeWritten, value, endian);
			m_sizeWritten = offsetNext;
			return sl_true;
		} else {
			return IWriter::writeDouble(value, endian);
		}
	}

	void BufferedWriter::_init(const Ptrx<IWriter, IClosable>& writer, const Memory& buf)
	{
		m_ref = writer.ref;
		m_writer = writer;
		m_closable = writer;

		m_buf = buf;
		m_dataBuf = (sl_uint8*)(buf.getData());
		m_sizeBuf = buf.getSize();
		m_sizeWritten = 0;
	}


	SLIB_DEFINE_ROOT_OBJECT(BufferedSeekableReader)

	BufferedSeekableReader::BufferedSeekableReader() : m_reader(sl_null), m_seekable(sl_null), m_closable(sl_null), m_posCurrent(0), m_sizeTotal(0), m_posInternal(0), m_dataBuf(sl_null), m_sizeBuf(0), m_sizeRead(0), m_posBuf(0)
	{
	}

	BufferedSeekableReader::~BufferedSeekableReader()
	{
	}

	Ref<BufferedSeekableReader> BufferedSeekableReader::create(const Ptrx<IReader, ISeekable, IClosable>& _obj, sl_size bufferSize)
	{
		if (!bufferSize) {
			return sl_null;
		}
		Ptrx<IReader, ISeekable, IClosable> obj = _obj.lock();
		if (!(obj.ptr)) {
			return sl_null;
		}
		ISeekable* seeker = obj;
		if (!seeker) {
			return sl_null;
		}
		sl_uint64 size = seeker->getSize();
		if (!size) {
			return sl_null;
		}
		Memory buf = Memory::create(bufferSize);
		if (buf.isNull()) {
			return sl_null;
		}

		Ref<BufferedSeekableReader> ret = new BufferedSeekableReader;
		if (ret.isNotNull()) {
			ret->_init(obj, size, buf);
			return ret;
		}
		return sl_null;
	}

	void BufferedSeekableReader::_init(const Ptrx<IReader, ISeekable, IClosable>& reader, sl_uint64 size, const Memory& buf)
	{
		m_ref = reader.ref;
		m_reader = reader;
		m_seekable = reader;
		m_closable = reader;

		m_sizeTotal = size;

		m_buf = buf;
		m_dataBuf = (sl_uint8*)(buf.getData());
		m_sizeBuf = buf.getSize();
	}

	sl_reg BufferedSeekableReader::_readInBuf(void* buf, sl_size size)
	{
		if (m_posCurrent >= m_posBuf) {
			sl_uint64 _offset = m_posCurrent - m_posBuf;
			if (_offset < m_sizeRead) {
				sl_size offset = (sl_size)_offset;
				sl_size nAvailable = m_sizeRead - offset;
				if (size > nAvailable) {
					size = nAvailable;
				}
				Base::copyMemory(buf, m_dataBuf + offset, size);
				m_posCurrent += size;
				return size;
			}
		}
		return -1;
	}

	sl_bool BufferedSeekableReader::_seekInternal(sl_uint64 pos)
	{
		if (pos == m_posInternal) {
			return sl_true;
		}
		ISeekable* seeker = m_seekable;
		if (seeker) {
			if (seeker->seek(pos, SeekPosition::Begin)) {
				m_posInternal = pos;
				return sl_true;
			}
		}
		return sl_false;
	}

	sl_reg BufferedSeekableReader::_readInternal(sl_uint64 pos, void* buf, sl_size size)
	{
		if (_seekInternal(pos)) {
			sl_uint64 n = m_sizeTotal - pos;
			if (size > n) {
				size = (sl_size)n;
			}
			if (!size) {
				return -1;
			}
			IReader* reader = m_reader;
			if (reader) {
				sl_reg nRead = reader->read(buf, size);
				if (nRead > 0) {
					m_posInternal += nRead;
				}
				return nRead;
			}
		}
		return -1;
	}

	sl_reg BufferedSeekableReader::_fillBuf(sl_uint64 pos, sl_size size)
	{
		m_posBuf = pos;
		sl_reg nRead = _readInternal(pos, m_dataBuf, size);
		if (nRead > 0) {
			m_sizeRead = nRead;
		} else {
			m_sizeRead = 0;
		}
		return nRead;
	}

	sl_reg BufferedSeekableReader::_fillBuf(sl_uint64 pos)
	{
		return _fillBuf(pos, m_sizeBuf);
	}

	sl_reg BufferedSeekableReader::_readFillingBuf(sl_uint64 pos, void* buf, sl_size size)
	{
		sl_reg nRead = _fillBuf(pos);
		if (nRead > 0) {
			return _readInBuf(buf, size);
		}
		return nRead;
	}

	sl_reg BufferedSeekableReader::read(void*& buf)
	{
		if (m_posCurrent >= m_sizeTotal) {
			return -1;
		}
		if (m_posCurrent >= m_posBuf) {
			sl_uint64 _offset = m_posCurrent - m_posBuf;
			if (_offset < m_sizeRead) {
				sl_size offset = (sl_size)_offset;
				sl_size sizeRemain = m_sizeRead - offset;
				m_posCurrent += sizeRemain;
				buf = m_dataBuf + offset;
				return sizeRemain;
			}
		}
		sl_reg nRead = _fillBuf(m_posCurrent);
		if (nRead > 0) {
			buf = m_dataBuf;
			m_posCurrent += nRead;
		}
		return nRead;
	}

	sl_reg BufferedSeekableReader::read(void* _buf, sl_size size)
	{
		sl_uint8* buf = (sl_uint8*)_buf;
		if (!size) {
			return -1;
		}
		if (m_posCurrent >= m_sizeTotal) {
			return -1;
		}
		if (!m_sizeRead) {
			return _readFillingBuf(m_posCurrent, buf, size);
		}
		sl_reg nRead = _readInBuf(buf, size);
		if (nRead > 0) {
			return nRead;
		}
		if (m_posCurrent >= m_posBuf) {
			return _readFillingBuf(m_posCurrent, buf, size);
		}
		sl_uint64 _offset = m_posBuf - m_posCurrent;
		if (_offset >= m_sizeBuf) {
			return _readFillingBuf(m_posCurrent, buf, size);
		}
		sl_size offset = (sl_size)_offset;
		sl_size sizeTailData;
		if (offset < size) {
			sizeTailData = size - offset;
			if (sizeTailData > m_sizeRead) {
				sizeTailData = m_sizeRead;
			}
			Base::copyMemory(buf + offset, m_dataBuf, sizeTailData);
			size = offset;
		} else {
			sizeTailData = 0;
		}
		if (m_posBuf >= m_sizeBuf) {
			nRead = _fillBuf(m_posBuf - m_sizeBuf);
			if (nRead <= 0) {
				return nRead;
			}
		} else {
			sl_size pos = (sl_size)m_posBuf;
			sl_size n = pos + m_sizeRead;
			if (n > m_sizeBuf) {
				n = m_sizeBuf;
			}
			n -= pos;
			Base::moveMemory(m_dataBuf + pos, m_dataBuf, n);
			nRead = _fillBuf(0, pos);
			if (nRead == pos) {
				m_sizeRead += n;
			}
		}
		nRead = _readInBuf(buf, size);
		if (nRead == size) {
			m_posCurrent += sizeTailData;
			return size + sizeTailData;
		}
		return nRead;
	}

	sl_bool BufferedSeekableReader::getPosition(sl_uint64& outPos)
	{
		outPos = m_posCurrent;
		return sl_true;
	}

	sl_bool BufferedSeekableReader::getSize(sl_uint64& outSize)
	{
		outSize = m_sizeTotal;
		return sl_true;
	}

	sl_bool BufferedSeekableReader::seek(sl_int64 offset, SeekPosition pos)
	{
		sl_uint64 posNew;
		switch (pos) {
		case SeekPosition::Begin:
			if (offset < 0) {
				return sl_false;
			}
			if ((sl_uint64)offset > m_sizeTotal) {
				return sl_false;
			}
			m_posCurrent = offset;
			break;
		case SeekPosition::End:
			if (offset > 0) {
				return sl_false;
			}
			if ((sl_uint64)(-offset) > m_sizeTotal) {
				return sl_false;
			}
			posNew = m_sizeTotal + offset;
			break;
		case SeekPosition::Current:
		{
			sl_uint64 posCurrent = m_posCurrent;
			if (offset > 0) {
				if ((sl_uint64)offset > m_sizeTotal - posCurrent) {
					return sl_false;
				}
			} else if (offset < 0) {
				if ((sl_uint64)(-offset) > posCurrent) {
					return sl_false;
				}
			} else {
				return sl_true;
			}
			m_posCurrent = posCurrent + offset;
			break;
		}
		}
		return sl_true;
	}

	void BufferedSeekableReader::close()
	{
		if (m_closable) {
			m_closable->close();
		}
		m_reader = sl_null;
		m_seekable = sl_null;
		m_closable = sl_null;
		m_ref.setNull();
	}


	sl_uint64 IOUtil::skip(const Pointerx<IReader, ISeekable>& _reader, sl_uint64 size)
	{
		if (!size) {
			return 0;
		}
		ISeekable* seekable = _reader;
		if (seekable) {
			if (seekable->seek(size, SeekPosition::Current)) {
				return size;
			}
			sl_uint64 pos = seekable->getPosition();
			sl_uint64 total = seekable->getSize();
			if (pos >= total) {
				return 0;
			}
			sl_uint64 remain = total - pos;
			if (size > remain) {
				size = remain;
			}
			if (seekable->seek(size, SeekPosition::Current)) {
				return size;
			}
			return 0;
		} else {
			IReader* reader = _reader;
			if (reader) {
				char buf[1024];
				sl_uint64 nRead = 0;
				while (nRead < size) {
					sl_uint64 nRemain = size - nRead;
					sl_uint32 n = sizeof(buf);
					if (n > nRemain) {
						n = (sl_uint32)nRemain;
					}
					sl_reg m = reader->read(buf, n);
					if (m < 0) {
						return nRead;
					}
					nRead += m;
					if (Thread::isStoppingCurrent()) {
						return nRead;
					}
					if (m == 0) {
						Thread::sleep(1);
						if (Thread::isStoppingCurrent()) {
							return nRead;
						}
					}
				}
				return nRead;
			}
		}
		return 0;
	}

	sl_int64 IOUtil::find(const Pointer<IReader, ISeekable>& reader, const void* pattern, sl_size nPattern, sl_int64 startPosition, sl_uint64 sizeFind)
	{
		return SeekableReaderHelper::find(reader, reader, pattern, nPattern, startPosition, sizeFind);
	}

	sl_int64 IOUtil::findBackward(const Pointer<IReader, ISeekable>& reader, const void* pattern, sl_size nPattern, sl_int64 startPosition, sl_uint64 sizeFind)
	{
		return SeekableReaderHelper::findBackward(reader, reader, pattern, nPattern, startPosition, sizeFind);
	}


	SkippableReader::SkippableReader(): m_reader(sl_null), m_seekable(sl_null), m_pos(0)
	{
	}

	SkippableReader::SkippableReader(const Ptrx<IReader, ISeekable>& reader): m_ref(reader.ref), m_reader(reader), m_seekable(reader), m_pos(0)
	{
	}

	SkippableReader::~SkippableReader()
	{
	}

	sl_bool SkippableReader::setReader(const Ptrx<IReader, ISeekable>& reader)
	{
		m_ref = reader.ref;
		m_reader = reader;
		m_seekable = reader;
		return m_reader != sl_null;
	}

	sl_reg SkippableReader::read(void* buf, sl_size size)
	{
		sl_reg iRead = m_reader->read(buf, size);
		if (iRead > 0) {
			m_pos += iRead;
		}
		return iRead;
	}

	sl_int32 SkippableReader::read32(void* buf, sl_uint32 size)
	{
		sl_int32 iRead = m_reader->read32(buf, size);
		if (iRead > 0) {
			m_pos += iRead;
		}
		return iRead;
	}

	sl_uint64 SkippableReader::skip(sl_uint64 size)
	{
		sl_uint64 nSkip = IOUtil::skip(Pointerx<IReader, ISeekable>(m_reader, m_seekable), size);
		if (nSkip) {
			m_pos += nSkip;
		}
		return nSkip;
	}

	sl_uint64 SkippableReader::getPosition()
	{
		if (m_seekable) {
			return m_seekable->getPosition();
		} else {
			return m_pos;
		}
	}

}
