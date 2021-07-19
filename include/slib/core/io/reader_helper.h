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

#ifndef CHECKHEADER_SLIB_CORE_IO_READER_HELPER
#define CHECKHEADER_SLIB_CORE_IO_READER_HELPER

#include "../mio.h"
#include "../thread.h"
#include "../string.h"
#include "../time.h"
#include "../memory_buffer.h"
#include "../scoped_buffer.h"

namespace slib
{

	class ReaderHelper
	{
	public:
		template <class READER>
		static sl_reg readWithRead32(READER* reader, void* _buf, sl_size size)
		{
#if !defined(SLIB_ARCH_IS_64BIT)
			return reader->read32(_buf, (sl_uint32)size);
#else
			if (!(size >> 31)) {
				return reader->read32(_buf, (sl_uint32)size);
			}
			char* buf = (char*)_buf;
			sl_size nRead = 0;
			while (nRead < size) {
				sl_size n = size - nRead;
				if (n > 0x40000000) {
					n = 0x40000000; // 1GB
				}
				sl_int32 m = reader->read32(buf + nRead, (sl_uint32)n);
				if (m > 0) {
					nRead += m;
				} else {
					if (nRead) {
						return nRead;
					} else {
						return m;
					}
				}
			}
			return nRead;
#endif
		}
		
		template <class READER>
		static sl_reg readFully(READER* reader, void* _buf, sl_size size)
		{
			if (!size) {
				return SLIB_IO_EMPTY_CONTENT;
			}
			char* buf = (char*)_buf;
			sl_size nRead = 0;
			while (nRead < size) {
				sl_size n = size - nRead;
				sl_reg m = reader->read(buf + nRead, n);
				if (m > 0) {
					nRead += m;
				} else if (m == SLIB_IO_WOULD_BLOCK && Thread::isNotStoppingCurrent()) {
					Thread::sleep(1);
				} else {
					if (nRead) {
						return nRead;
					} else {
						return m;
					}
				}
			}
			return nRead;
		}
		
		template <class READER>
		static Memory readFully(READER* reader)
		{
			MemoryBuffer mb;
			char buf[1024];
			for (;;) {
				sl_reg nRead = reader->read(buf, sizeof(buf));
				if (nRead > 0) {
					mb.addNew(buf, nRead);
				} else if (nRead == SLIB_IO_WOULD_BLOCK && Thread::isNotStoppingCurrent()) {
					Thread::sleep(1);
				} else {
					break;
				}
			}
			return mb.merge();
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
		static sl_bool readCVLI(READER* reader, T* output)
		{
			T value = 0;
			sl_uint32 m = 0;
			sl_uint8 n;
			while (readUint8(reader, &n)) {
				value += (((T)(n & 127)) << m);
				m += 7;
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
		static T readCVLI(READER* reader, T def)
		{
			T v;
			if (readCVLI(reader, &v)) {
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
		static sl_bool readSectionData(READER* reader, void* mem, sl_size& size)
		{
			sl_size sizeBuf = size;
			if (readCVLI(reader, &size)) {
				if (size <= sizeBuf) {
					if (readFully(reader, mem, size) == (sl_reg)size) {
						return sl_true;
					}
				}
			} else {
				size = 0;
			}
			return sl_false;
		}

		template <class READER>
		static sl_bool readSection(READER* reader, Memory* mem, sl_size maxSize)
		{
			sl_size size;
			if (readCVLI(reader, &size)) {
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
						if (readFully(reader, ret.getData(), size) == (sl_reg)size) {
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
						if (readFully(reader, buf, n) != n) {
							return sl_false;
						}
						size -= n;
					}
					return sl_true;
				}
			}
			return sl_false;
		}

		template <class READER>
		static Memory readSection(READER* reader, const Memory& def, sl_size maxSize)
		{
			Memory ret;
			if (readSection(reader, &ret, maxSize)) {
				return ret;
			}
			return def;
		}

		template <class READER>
		static Memory readSection(READER* reader, sl_size maxSize)
		{
			Memory ret;
			if (readSection(reader, &ret, maxSize)) {
				return ret;
			}
			return sl_null;
		}

		template <class READER>
		static sl_bool readStringSection(READER* reader, String* str, sl_size maxLen)
		{
			if (str) {
				Memory mem;
				if (readSection(reader, &mem, maxLen)) {
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
				return readSection(reader, sl_null, maxLen);
			}
			return sl_false;
		}

		template <class READER>
		static String readStringSection(READER* reader, const String& def, sl_size maxLen)
		{
			String ret;
			if (readStringSection(reader, &ret, maxLen)) {
				return ret;
			} else {
				return def;
			}
		}

		template <class READER>
		static String readStringSection(READER* reader, sl_size maxLen)
		{
			String ret;
			if (readStringSection(reader, &ret, maxLen)) {
				return ret;
			} else {
				return sl_null;
			}
		}

		template <class READER>
		static sl_bool readTime(READER* reader, Time* v)
		{
			sl_int64 m;
			if (readInt64(reader, &m, EndianType::Little)) {
				*v = m;
				return sl_true;
			}
			return sl_false;
		}

		template <class READER>
		static Time readTime(READER* reader)
		{
			Time ret;
			if (readTime(reader, &ret)) {
				return ret;
			} else {
				return Time::zero();
			}
		}

		template <class READER>
		static Time readTime(READER* reader, const Time& def)
		{
			Time ret;
			if (readTime(reader, &ret)) {
				return ret;
			} else {
				return def;
			}
		}

		template <class READER>
		static String readTextUTF8(READER* reader, sl_size size)
		{
			if (!size) {
				return String::getEmpty();
			}
			sl_char8 sbuf[3];
			if (size >= 3) {
				if (readFully(reader, sbuf, 3) == 3) {
					if ((sl_uint8)(sbuf[0]) == 0xEF && (sl_uint8)(sbuf[1]) == 0xBB && (sl_uint8)(sbuf[2]) == 0xBF) {
						size -= 3;
						if (!size) {
							return String::getEmpty();
						}
						String ret = String::allocate(size);
						if (ret.isNotNull()) {
							if (readFully(reader, ret.getData(), size) == (sl_reg)size) {
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
							if (readFully(reader, buf+3, size) == (sl_reg)size) {
								return ret;
							}
						}
					}
				}
			} else {
				String ret = String::allocate(size);
				if (ret.isNotNull()) {
					if (readFully(reader, ret.getData(), size) == (sl_reg)size) {
						return ret;
					}
				}
			}
			return sl_null;
		}

		template <class READER>
		static String16 readTextUTF16(READER* reader, sl_size size, EndianType endian)
		{
			if (!size) {
				return String16::getEmpty();
			}
			sl_size len = (size >> 1) + (size & 1);
			sl_uint16 first;
			if (readUint16(reader, &first, endian)) {
				len--;
				// check BOM(Byte Order Mark = U+FEFF)
				String16 str;
				sl_char16* buf;
				if (first == 0xFEFF) {
					if (!len) {
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
				if (!len) {
					return str;
				}
				buf[len - 1] = 0;
				if (readFully(reader, buf, size) == (sl_reg)size) {
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

		template <class READER>
		static String readText(READER* reader, sl_size size, Charset* outCharset)
		{
			if (!size) {
				if (outCharset) {
					*outCharset = Charset::UTF8;
				}
				return String::getEmpty();
			}
			sl_char8 sbuf[3];
			if (size >= 2) {
				if (readFully(reader, sbuf, 2) == 2) {
					if (!(size & 1)) {
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
								if (readFully(reader, buf, size) == (sl_reg)size) {
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
						if (reader->read(sbuf + 2, 1) == 1) {
							if (sbuf[0] == (sl_char8)0xEF && sbuf[1] == (sl_char8)0xBB && sbuf[2] == (sl_char8)0xBF) {
								size -= 3;
								if (!size) {
									return String::getEmpty();
								}
								String ret = String::allocate(size);
								if (ret.isNotNull()) {
									if (readFully(reader, ret.getData(), size) == (sl_reg)size) {
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
									if (readFully(reader, buf + 3, size) == (sl_reg)size) {
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
					if (readFully(reader, ret.getData(), size) == (sl_reg)size) {
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

		template <class READER>
		static String16 readText16(READER* reader, sl_size size, Charset* outCharset)
		{
			if (!size) {
				if (outCharset) {
					*outCharset = Charset::UTF8;
				}
				return String16::getEmpty();
			}
			sl_char8 sbuf[3];
			if (size >= 2) {
				if (readFully(reader, sbuf, 2) == 2) {
					if (!(size & 1)) {
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
							if (!len) {
								return String16::getEmpty();
							}
							String16 str = String16::allocate(len);
							if (str.isNotNull()) {
								sl_char16* buf = str.getData();
								size = len << 1;
								if (readFully(reader, buf, size) == (sl_reg)size) {
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
							if (readFully(reader, tbuf + 2, size - 2) == (sl_reg)(size - 2)) {
								if (tbuf[0] == (sl_char8)0xEF && tbuf[1] == (sl_char8)0xBB && tbuf[2] == (sl_char8)0xBF) {
									size -= 3;
									if (!size) {
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
				if (readFully(reader, sbuf, size) == (sl_reg)size) {
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
		
	};

	class BlockReaderHelper
	{
	public:
		template <class READER>
		static sl_reg readAtWithReadAt32(READER* reader, sl_uint64 offset, void* _buf, sl_size size)
		{
#if !defined(SLIB_ARCH_IS_64BIT)
			return reader->readAt32(offset, _buf, (sl_uint32)size);
#else
			if (!(size >> 31)) {
				return reader->readAt32(offset, _buf, (sl_uint32)size);
			}
			char* buf = (char*)_buf;
			sl_size nRead = 0;
			while (nRead < size) {
				sl_size n = size - nRead;
				if (n > 0x40000000) {
					n = 0x40000000; // 1GB
				}
				sl_reg m = reader->readAt32(offset + nRead, buf + nRead, (sl_uint32)n);
				if (m > 0) {
					nRead += m;
				} else {
					if (nRead) {
						return nRead;
					} else {
						return m;
					}
				}
			}
			return nRead;
#endif
		}

		template <class READER>
		static sl_reg readFullyAt(READER* reader, sl_uint64 offset, void* _buf, sl_size size)
		{
			if (!size) {
				return SLIB_IO_EMPTY_CONTENT;
			}
			char* buf = (char*)_buf;
			sl_size nRead = 0;
			while (nRead < size) {
				sl_size n = size - nRead;
				sl_reg m = reader->readAt(offset + nRead, buf + nRead, n);
				if (m > 0) {
					nRead += m;
				} else if (m == SLIB_IO_WOULD_BLOCK && Thread::isNotStoppingCurrent()) {
					Thread::sleep(1);
				} else {
					if (nRead) {
						return nRead;
					} else {
						return m;
					}
				}
			}
			return nRead;
		}

	};

}

#endif
