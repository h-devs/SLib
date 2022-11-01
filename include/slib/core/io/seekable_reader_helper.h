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

#ifndef CHECKHEADER_SLIB_CORE_IO_SEEKABLE_READER_HELPER
#define CHECKHEADER_SLIB_CORE_IO_SEEKABLE_READER_HELPER

#include "../endian.h"
#include "../memory.h"
#include "../string_buffer.h"
#include "../thread.h"

namespace slib
{

	class SeekableReaderHelper
	{
	public:
		template <class READER, class SEEKABLE>
		static String readLine(READER* reader, SEEKABLE* seekable)
		{
			StringBuffer sb;
			char buf[512];
			sl_bool flagNull = sl_true;
			CurrentThread thread;
			for (;;) {
				sl_reg n = reader->read(buf, sizeof(buf));
				if (n > 0) {
					flagNull = sl_false;
					for (sl_reg i = 0; i < n; i++) {
						char ch = buf[i];
						if (ch == '\r' || ch == '\n') {
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
							if (i) {
								if (sb.getLength()) {
									if (!(sb.addStatic(buf, i))) {
										return sl_null;
									}
								} else {
									return String(buf, i);
								}
							}
							return sb.merge();
						}
					}
					String str(buf, n);
					if (str.isNull()) {
						return sl_null;
					}
					if (!(sb.add(Move(str)))) {
						return sl_null;
					}
				} else if (n == SLIB_IO_WOULD_BLOCK) {
					reader->waitRead();
				} else if (n == SLIB_IO_ENDED) {
					break;
				} else {
					return sl_null;
				}
				if (thread.isStopping()) {
					return sl_null;
				}
			}
			if (flagNull) {
				return sl_null;
			}
			return sb.merge();
		}

		template <class READER, class SEEKABLE>
		static String readNullTerminatedString(READER* reader, SEEKABLE* seekable)
		{
			StringBuffer sb;
			char buf[128];
			sl_bool flagNull = sl_true;
			CurrentThread thread;
			for (;;) {
				sl_reg n = reader->read(buf, sizeof(buf));
				if (n > 0) {
					flagNull = sl_false;
					for (sl_reg i = 0; i < n; i++) {
						char ch = buf[i];
						if (!ch) {
							if (i != n - 1) {
								seekable->seek(i + 1 - n, SeekPosition::Current);
							}
							if (i) {
								if (sb.getLength()) {
									if (!(sb.addStatic(buf, i))) {
										return sl_null;
									}
								} else {
									return String(buf, i);
								}
							}
							return sb.merge();
						}
					}
					String str(buf, n);
					if (str.isNull()) {
						return sl_null;
					}
					if (!(sb.add(Move(str)))) {
						return sl_null;
					}
				} else if (n == SLIB_IO_WOULD_BLOCK) {
					reader->waitRead();
				} else if (n == SLIB_IO_ENDED) {
					break;
				} else {
					return sl_null;
				}
				if (thread.isStopping()) {
					return sl_null;
				}
			}
			if (flagNull) {
				return sl_null;
			}
			return sb.merge();
		}

		template <class READER, class SEEKABLE>
		static Memory readAllBytes(READER* reader, SEEKABLE* seekable, sl_size maxSize)
		{
			sl_uint64 _size = seekable->getSize();
			sl_size size = SLIB_SIZE_FROM_UINT64(_size);
			if (size > maxSize) {
				size = maxSize;
			}
			if (!size) {
				return sl_null;
			}
			if (seekable->seekToBegin()) {
				Memory ret = Memory::create(size);
				if (ret.isNotNull()) {
					char* buf = (char*)(ret.getData());
					sl_reg iRet = reader->readFully(buf, size);
					if (iRet > 0) {
						if ((sl_size)iRet < size) {
							return ret.sub(0, iRet);
						} else {
							return ret;
						}
					}
				}
			}
			return sl_null;
		}

		template <class READER, class SEEKABLE>
		static String readAllTextUTF8(READER* reader, SEEKABLE* seekable, sl_size maxSize)
		{
			sl_uint64 _size = seekable->getSize();
			sl_size size = SLIB_SIZE_FROM_UINT64(_size);
			if (size > maxSize) {
				size = maxSize;
			}
			if (!size) {
				return sl_null;
			}
			if (seekable->seekToBegin()) {
				return reader->readTextUTF8(size);
			}
			return sl_null;
		}

		template <class READER, class SEEKABLE>
		static String16 readAllTextUTF16(READER* reader, SEEKABLE* seekable, EndianType endian, sl_size maxSize)
		{
			sl_uint64 _size = seekable->getSize();
			sl_size size = SLIB_SIZE_FROM_UINT64(_size);
			if (size > maxSize) {
				size = maxSize;
			}
			if (!size) {
				return sl_null;
			}
			if (seekable->seekToBegin()) {
				return reader->readTextUTF16(size, endian);
			}
			return sl_null;
		}

		template <class READER, class SEEKABLE>
		static StringParam readAllText(READER* reader, SEEKABLE* seekable, sl_size maxSize)
		{
			sl_uint64 _size = seekable->getSize();
			sl_size size = SLIB_SIZE_FROM_UINT64(_size);
			if (size > maxSize) {
				size = maxSize;
			}
			if (!size) {
				return sl_null;
			}
			if (seekable->seekToBegin()) {
				return reader->readText(size);
			}
			return sl_null;
		}

		template <class READER, class SEEKABLE>
		static sl_int64 find(READER* reader, SEEKABLE* seekable, const void* _pattern, sl_size nPattern, sl_int64 _startPosition, sl_uint64 sizeFind)
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

		template <class READER, class SEEKABLE>
		static sl_int64 findBackward(READER* reader, SEEKABLE* seekable, const void* _pattern, sl_size nPattern, sl_int64 _startPosition, sl_uint64 sizeFind)
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

	};

}

#endif
