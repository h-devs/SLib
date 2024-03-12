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

#ifndef CHECKHEADER_SLIB_IO_TEXT
#define CHECKHEADER_SLIB_IO_TEXT

#include "definition.h"

#include "../core/string.h"

namespace slib
{

	class SLIB_EXPORT TextIO
	{
	public:
		template <class READER>
		static String readUTF8(READER* reader, sl_size size)
		{
			sl_char8 sbuf[3];
			if (size >= 3) {
				sl_reg iRet = reader->readFully(sbuf, 3);
				if (iRet == 3) {
					size -= 3;
					if ((sl_uint8)(sbuf[0]) == 0xEF && (sl_uint8)(sbuf[1]) == 0xBB && (sl_uint8)(sbuf[2]) == 0xBF) {
						return _read8(reader, size);
					} else {
						return _read8(reader, size, sbuf, 3);
					}
				} else if (iRet > 0) {
					return String(sbuf, iRet);
				}
			} else {
				return _read8(reader, size);
			}
			return sl_null;
		}

		template <class READER>
		static String16 readUTF16(READER* reader, sl_size size, EndianType endian = Endian::Little)
		{
			sl_size len = size >> 1;
			if (!len) {
				return String16::getEmpty();
			}
			sl_uint16 first;
			if (reader->readUint16(&first, endian)) {
				len--;
				if (first == 0xFEFF) {
					return _read16(reader, len, endian);
				} else {
					return _read16(reader, len, first, endian);
				}
			}
			return sl_null;
		}

		template <class READER>
		static StringParam read(READER* reader, sl_size size)
		{
			if (!size) {
				return sl_null;
			}
			sl_char8 sbuf[3];
			if (size >= 2) {
				sl_reg iRet = reader->readFully(sbuf, 2);
				if (iRet == 2) {
					if (!(size & 1)) {
						sl_bool flagUTF16LE = sbuf[0] == (sl_char8)0xFF && sbuf[1] == (sl_char8)0xFE;
						sl_bool flagUTF16BE = sbuf[0] == (sl_char8)0xFE && sbuf[1] == (sl_char8)0xFF;
						if (flagUTF16LE || flagUTF16BE) {
							size -= 2;
							return _read16(reader, size >> 1, flagUTF16LE ? EndianType::Little : EndianType::Big);
						}
					}
					if (size >= 3) {
						iRet = reader->read(sbuf + 2, 1);
						if (iRet == 1) {
							size -= 3;
							if (sbuf[0] == (sl_char8)0xEF && sbuf[1] == (sl_char8)0xBB && sbuf[2] == (sl_char8)0xBF) {
								return _read8(reader, size);
							} else {
								return _read8(reader, size, sbuf, 3);
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
				return _read8(reader, size);
			}
			return StringParam();
		}

		template <class READER>
		static String readAllUTF8(READER* seekableReader, sl_size maxSize = SLIB_SIZE_MAX)
		{
			sl_uint64 _size = seekableReader->getSize();
			sl_size size = SLIB_SIZE_FROM_UINT64(_size);
			if (size > maxSize) {
				size = maxSize;
			}
			if (!size) {
				return sl_null;
			}
			if (seekableReader->seekToBegin()) {
				return readUTF8(seekableReader, size);
			}
			return sl_null;
		}

		template <class READER>
		static String16 readAllUTF16(READER* seekableReader, EndianType endian = Endian::Little, sl_size maxSize = SLIB_SIZE_MAX)
		{
			sl_uint64 _size = seekableReader->getSize();
			sl_size size = SLIB_SIZE_FROM_UINT64(_size);
			if (size > maxSize) {
				size = maxSize;
			}
			if (!size) {
				return sl_null;
			}
			if (seekableReader->seekToBegin()) {
				return readUTF16(seekableReader, size, endian);
			}
			return sl_null;
		}

		template <class READER>
		static StringParam readAll(READER* seekableReader, sl_size maxSize = SLIB_SIZE_MAX)
		{
			sl_uint64 _size = seekableReader->getSize();
			sl_size size = SLIB_SIZE_FROM_UINT64(_size);
			if (size > maxSize) {
				size = maxSize;
			}
			if (!size) {
				return sl_null;
			}
			if (seekableReader->seekToBegin()) {
				return read(seekableReader, size);
			}
			return sl_null;
		}

		
		template <class WRITER>
		static sl_bool writeUTF8(WRITER* writer, const StringView& text, sl_bool flagWriteByteOrderMark = sl_false)
		{
			if (flagWriteByteOrderMark) {
				static sl_char8 sbuf[3] = { (sl_char8)0xEF, (sl_char8)0xBB, (sl_char8)0xBF };
				if (writer->writeFully(sbuf, 3) != 3) {
					return sl_false;
				}
			}
			sl_size n = text.getLength();
			if (!n) {
				return sl_true;
			}
			if (writer->writeFully(text.getData(), n) == (sl_reg)n) {
				return sl_true;
			}
			return sl_false;
		}

		template <class WRITER>
		static sl_bool writeUTF16LE(WRITER* writer, const StringView16& text, sl_bool flagWriteByteOrderMark = sl_false)
		{
			if (flagWriteByteOrderMark) {
				static sl_char8 sbuf[2] = { (sl_char8)0xFF, (sl_char8)0xFE };
				if (writer->writeFully(sbuf, 2) != 2) {
					return sl_false;
				}
			}
			sl_size n = text.getLength();
			if (!n) {
				return sl_true;
			}
			if (Endian::isLE()) {
				n <<= 1;
				if (writer->writeFully(text.getData(), n) == (sl_reg)n) {
					return sl_true;
				}
				return sl_false;
			} else {
				sl_char16* s = text.getData();
				sl_char16 buf[0x2000];
				while (n > 0) {
					sl_size m = sizeof(buf);
					if (m > n) {
						m = n;
					}
					for (sl_size i = 0; i < m; i++) {
						sl_uint16 c = (sl_uint16)(s[i]);
						buf[i] = (sl_char16)((c >> 8) | (c << 8));
					}
					sl_size l = m << 1;
					if (writer->writeFully(buf, l) != (sl_reg)l) {
						return sl_false;
					}
					n -= m;
					s += m;
				}
				return sl_true;
			}
		}

		template <class WRITER>
		static sl_bool writeUTF16BE(WRITER* writer, const StringView16& text, sl_bool flagWriteByteOrderMark = sl_false)
		{
			if (flagWriteByteOrderMark) {
				static sl_char8 sbuf[2] = { (sl_char8)0xFE, (sl_char8)0xFF };
				if (writer->writeFully(sbuf, 2) != 2) {
					return sl_false;
				}
			}
			sl_size n = text.getLength();
			if (!n) {
				return sl_true;
			}
			if (Endian::isBE()) {
				n <<= 1;
				if (writer->writeFully(text.getData(), n) == (sl_reg)n) {
					return sl_true;
				}
				return sl_false;
			} else {
				sl_char16* s = text.getData();
				sl_char16 buf[0x2000];
				while (n > 0) {
					sl_size m = 0x2000;
					if (m > n) {
						m = n;
					}
					for (sl_size i = 0; i < m; i++) {
						sl_uint16 c = (sl_uint16)(s[i]);
						buf[i] = (sl_char16)((c >> 8) | (c << 8));
					}
					sl_size l = m << 1;
					if (writer->writeFully(buf, l) != (sl_reg)l) {
						return sl_false;
					}
					n -= m;
					s += m;
				}
				return sl_true;
			}
		}

	private:
		template <class READER>
		static String _read8(READER* reader, sl_size size)
		{
			if (!size) {
				return String::getEmpty();
			}
			String ret = String::allocate(size);
			if (ret.isNotNull()) {
				sl_char8* buf = ret.getData();
				sl_reg iRet = reader->readFully(buf, size);
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
		static String _read8(READER* reader, sl_size size, const sl_char8* prefix, sl_size nPrefix)
		{
			if (!size) {
				return String(prefix, nPrefix);
			}
			String ret = String::allocate(nPrefix + size);
			if (ret.isNotNull()) {
				sl_char8* buf = ret.getData();
				Base::copyMemory(buf, prefix, nPrefix);
				buf += nPrefix;
				sl_reg iRet = reader->readFully(buf, size);
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
		static String16 _read16(READER* reader, sl_size len, EndianType endian)
		{
			if (!len) {
				return String16::getEmpty();
			}
			String16 ret = String16::allocate(len);
			if (ret.isNotNull()) {
				sl_char16* buf = ret.getData();
				sl_size size = len << 1;
				sl_reg iRet = reader->readFully(buf, size);
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
		static String16 _read16(READER* reader, sl_size len, sl_char16 prefix, EndianType endian)
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
				sl_reg iRet = reader->readFully(buf, size);
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

}

#endif
