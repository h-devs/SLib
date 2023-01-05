/*
 *   Copyright (c) 2008-2019 SLIBIO <https://github.com/SLIBIO>
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

#include "slib/core/charset.h"

#include "slib/core/base.h"

namespace slib
{

	namespace priv
	{
		namespace charset
		{

			class BigEndianHelper
			{
			public:
				SLIB_INLINE static sl_char16 read16(const void* src, sl_size pos) noexcept
				{
					sl_uint8* s = ((sl_uint8*)src) + (pos << 1);
					return (sl_char16)(((sl_uint16)(s[0]) << 8) | ((sl_uint16)(s[1])));
				}

				SLIB_INLINE static void write16(void* dst, sl_size pos, sl_char16 _v) noexcept
				{
					sl_uint16 v = (sl_uint16)_v;
					sl_uint8* d = ((sl_uint8*)dst) + (pos << 1);
					d[0] = (sl_uint8)(v >> 8);
					d[1] = (sl_uint8)(v);
				}

				SLIB_INLINE static sl_char32 read32(const void* src, sl_size pos) noexcept
				{
					sl_uint8* s = ((sl_uint8*)src) + (pos << 2);
					return (sl_char32)(((sl_uint32)(s[0]) << 24) | ((sl_uint32)(s[1]) << 16) | ((sl_uint32)(s[2]) << 8) | ((sl_uint32)(s[3])));
				}

				SLIB_INLINE static void write32(void* dst, sl_size pos, sl_char32 _v) noexcept
				{
					sl_uint32 v = (sl_uint32)_v;
					sl_uint8* d = ((sl_uint8*)dst) + (pos << 2);
					d[0] = (sl_uint8)(v >> 24);
					d[1] = (sl_uint8)(v >> 16);
					d[2] = (sl_uint8)(v >> 8);
					d[3] = (sl_uint8)(v);
				}

			};

			class LittleEndianHelper
			{
			public:
				SLIB_INLINE static sl_char16 read16(const void* src, sl_size pos) noexcept
				{
					sl_uint8* s = ((sl_uint8*)src) + (pos << 1);
					return (sl_char16)(((sl_uint16)(s[0])) | ((sl_uint16)(s[1]) << 8));
				}

				SLIB_INLINE static void write16(void* dst, sl_size pos, sl_char16 _v) noexcept
				{
					sl_uint16 v = (sl_uint16)_v;
					sl_uint8* d = ((sl_uint8*)dst) + (pos << 1);
					d[0] = (sl_uint8)(v);
					d[1] = (sl_uint8)(v >> 8);
				}

				SLIB_INLINE static sl_char32 read32(const void* src, sl_size pos) noexcept
				{
					sl_uint8* s = ((sl_uint8*)src) + (pos << 2);
					return (sl_char32)(((sl_uint32)(s[0])) | ((sl_uint32)(s[1]) << 8) | ((sl_uint32)(s[2]) << 16) | ((sl_uint32)(s[3]) << 24));
				}

				SLIB_INLINE static void write32(void* dst, sl_size pos, sl_char32 _v) noexcept
				{
					sl_uint32 v = (sl_uint32)_v;
					sl_uint8* d = ((sl_uint8*)dst) + (pos << 2);
					d[0] = (sl_uint8)(v);
					d[1] = (sl_uint8)(v >> 8);
					d[2] = (sl_uint8)(v >> 16);
					d[3] = (sl_uint8)(v >> 24);
				}

			};

			class NoEndianHelper
			{
			public:
				SLIB_INLINE static sl_char16 read16(const void* src, sl_size pos) noexcept
				{
					return ((sl_char16*)src)[pos];
				}

				SLIB_INLINE static void write16(void* dst, sl_size pos, sl_char16 v) noexcept
				{
					((sl_char16*)dst)[pos] = v;
				}

				SLIB_INLINE static sl_char32 read32(const void* src, sl_size pos) noexcept
				{
					return ((sl_char32*)src)[pos];
				}

				SLIB_INLINE static void write32(void* dst, sl_size pos, sl_char32 v) noexcept
				{
					((sl_char32*)dst)[pos] = v;
				}

			};

			class Utf8Helper
			{
			public:
				// 0xC0 <= `ch0` < 0xE0
				SLIB_INLINE static sl_bool getUnicode2(sl_uint32& code, sl_uint8 _ch0, sl_uint8 _ch1)
				{
					sl_uint32 ch1 = _ch1;
					if ((ch1 & 0xC0) == 0x80) {
						sl_uint32 ch0 = _ch0;
						code = ((ch0 & 0x1F) << 6) | (ch1 & 0x3F);
						return sl_true;
					}
					return sl_false;
				}

				// 0xE0 <= `ch0` < 0xF0, `ch1`: 2 bytes
				SLIB_INLINE static sl_bool getUnicode3(sl_uint32& code, sl_uint8 _ch0, const sl_char8* _ch1)
				{
					const sl_char8* s = _ch1;
					sl_uint32 ch1 = (sl_uint8)(*(s++));
					if ((ch1 & 0xC0) == 0x80) {
						sl_uint32 ch2 = (sl_uint8)(*s);
						if ((ch2 & 0xC0) == 0x80) {
							sl_uint32 ch0 = _ch0;
							code = ((ch0 & 0x0F) << 12) | ((ch1 & 0x3F) << 6) | (ch2 & 0x3F);
							return sl_true;
						}
					}
					return sl_false;
				}

				// 0xF0 <= `ch0` < 0xF8, `ch1`: 3 bytes
				SLIB_INLINE static sl_bool getUnicode4(sl_uint32& code, sl_uint8 _ch0, const sl_char8* _ch1)
				{
					const sl_char8* s = _ch1;
					sl_uint32 ch1 = (sl_uint8)(*(s++));
					if ((ch1 & 0xC0) == 0x80) {
						sl_uint32 ch2 = (sl_uint8)(*(s++));
						if ((ch2 & 0xC0) == 0x80) {
							sl_uint32 ch3 = (sl_uint8)(*s);
							if ((ch3 & 0xC0) == 0x80) {
								sl_uint32 ch0 = _ch0;
								code = ((ch0 & 0x07) << 18) | ((ch1 & 0x3F) << 12) | ((ch2 & 0x3F) << 6) | (ch3 & 0x3F);
								return sl_true;
							}
						}
					}
					return sl_false;
				}

				SLIB_INLINE static sl_bool getUnicode(sl_uint32& code, const void* _utf8, sl_size len, sl_size& pos)
				{
					sl_char8* utf8 = (sl_char8*)_utf8;
					sl_uint8 ch = utf8[pos];
					if (ch < 0x80) {
						code = ch;
						pos++;
						return sl_true;
					} else if (ch < 0xC0) {
						// Corrupted data element
					} else if (ch < 0xE0) {
						if (pos + 1 < len) {
							if (getUnicode2(code, ch, utf8[pos + 1])) {
								pos += 2;
								return sl_true;
							}
						}
					} else if (ch < 0xF0) {
						if (pos + 2 < len) {
							if (getUnicode3(code, ch, utf8 + pos + 1)) {
								pos += 3;
								return sl_true;
							}
						}
					} else if (ch < 0xF8) {
						if (pos + 3 < len) {
							if (getUnicode4(code, ch, utf8 + pos + 1)) {
								pos += 4;
								return sl_true;
							}
						}
					}
					return sl_false;
				}

				// Zero-terminated source
				SLIB_INLINE static sl_bool getUnicode(sl_uint32& code, const void* _utf8, sl_size& pos)
				{
					sl_char8* utf8 = (sl_char8*)_utf8;
					sl_uint8 ch = utf8[pos];
					if (ch < 0x80) {
						code = ch;
						pos++;
						return sl_true;
					} else if (ch < 0xC0) {
						// Corrupted data element
					} else if (ch < 0xE0) {
						if (getUnicode2(code, ch, utf8[pos + 1])) {
							pos += 2;
							return sl_true;
						}
					} else if (ch < 0xF0) {
						if (getUnicode3(code, ch, utf8 + pos + 1)) {
							pos += 3;
							return sl_true;
						}
					} else if (ch < 0xF8) {
						if (getUnicode4(code, ch, utf8 + pos + 1)) {
							pos += 4;
							return sl_true;
						}
					}
					return sl_false;
				}

				// 0x0080 <= `code` < 0x07ff
				SLIB_INLINE static void putUnicode2(sl_uint32 code, sl_char8* utf8)
				{
					*(utf8++) = (sl_char8)((code >> 6) | 0xC0);
					*utf8 = (sl_char8)((code & 0x3F) | 0x80);
				}

				// 0x0800 <= `code` < 0x10000
				SLIB_INLINE static void putUnicode3(sl_uint32 code, sl_char8* utf8)
				{
					*(utf8++) = (sl_char8)((code >> 12) | 0xE0);
					*(utf8++) = (sl_char8)(((code >> 6) & 0x3F) | 0x80);
					*utf8 = (sl_char8)((code & 0x3F) | 0x80);
				}

				// 0x10000 <= `code` < 0x110000
				SLIB_INLINE static void putUnicode4(sl_uint32 code, sl_char8* utf8)
				{
					*(utf8++) = (sl_char8)((code >> 18) | 0xF0);
					*(utf8++) = (sl_char8)(((code >> 12) & 0x3F) | 0x80);
					*(utf8++) = (sl_char8)(((code >> 6) & 0x3F) | 0x80);
					*utf8 = (sl_char8)((code & 0x3F) | 0x80);
				}

				SLIB_INLINE static void putUnicode(sl_uint32 code, void* _utf8, sl_size len, sl_size& pos)
				{
					sl_char8* utf8 = (sl_char8*)_utf8;
					if (code < 0x80) {
						if (utf8) {
							utf8[pos] = (sl_char8)code;
						}
						pos++;
					} else if (code < 0x800) {
						if (pos + 1 < len) {
							if (utf8) {
								putUnicode2(code, utf8 + pos);
							}
							pos += 2;
						}
					} else if (code < 0x10000) {
						if (pos + 2 < len) {
							if (utf8) {
								putUnicode3(code, utf8 + pos);
							}
							pos += 3;
						}
					} else if (code < 0x110000) {
						if (pos + 3 < len) {
							if (utf8) {
								putUnicode4(code, utf8 + pos);
							}
							pos += 4;
						}
					}
				}

				SLIB_INLINE static void putUnicode(sl_uint32 code, void* _utf8, sl_size& pos)
				{
					sl_char8* utf8 = (sl_char8*)_utf8;
					if (code < 0x80) {
						if (utf8) {
							utf8[pos] = (sl_char8)code;
						}
						pos++;
					} else if (code < 0x800) {
						if (utf8) {
							putUnicode2(code, utf8 + pos);
						}
						pos += 2;
					} else if (code < 0x10000) {
						if (utf8) {
							putUnicode3(code, utf8 + pos);
						}
						pos += 3;
					} else if (code < 0x110000) {
						if (utf8) {
							putUnicode4(code, utf8 + pos);
						}
						pos += 4;
					}
				}

			};

			template <class EndianHelper>
			class Utf16Helper
			{
			public:
				// 0xD800 <= `ch0` < 0xE000
				SLIB_INLINE static sl_bool getUnicode2(sl_uint32& code, sl_uint16 _ch0, sl_uint16 _ch1)
				{
					sl_uint32 ch0 = _ch0;
					sl_uint32 ch1 = _ch1;
					if (ch0 < 0xDC00 && ch1 >= 0xDC00 && ch1 < 0xE000) {
						code = (((ch0 - 0xD800) << 10) | (ch1 - 0xDC00)) + 0x10000;
						return sl_true;
					}
					return sl_false;
				}

				SLIB_INLINE static sl_bool getUnicode(sl_uint32& code, const void* utf16, sl_size len, sl_size& pos)
				{
					sl_uint16 ch = EndianHelper::read16(utf16, pos);
					if (SLIB_CHAR_IS_SURROGATE(ch)) {
						if (pos + 1 < len) {
							sl_char16 ch1 = EndianHelper::read16(utf16, pos + 1);
							if (getUnicode2(code, ch, ch1)) {
								pos += 2;
								return sl_true;
							}
						}
					} else {
						code = ch;
						pos++;
						return sl_true;
					}
					return sl_false;
				}

				// Null-terminated source
				SLIB_INLINE static sl_bool getUnicode(sl_uint32& code, const void* utf16, sl_size& pos)
				{
					sl_uint16 ch = EndianHelper::read16(utf16, pos);
					if (SLIB_CHAR_IS_SURROGATE(ch)) {
						sl_char16 ch1 = EndianHelper::read16(utf16, pos + 1);
						if (getUnicode2(code, ch, ch1)) {
							pos += 2;
							return sl_true;
						}
					} else {
						code = ch;
						pos++;
						return sl_true;
					}
					return sl_false;
				}

				// 0x10000 <= `code` < 0x110000
				SLIB_INLINE static void putUnicode2(sl_uint32 code, void* utf16, sl_size pos)
				{
					code -= 0x10000;
					EndianHelper::write16(utf16, pos, (sl_char16)(0xD800 + (code >> 10)));
					EndianHelper::write16(utf16, pos + 1, (sl_char16)(0xDC00 + (code & 0x3FF)));
				}

				SLIB_INLINE static void putUnicode(sl_uint32 code, void* utf16, sl_size len, sl_size& pos) noexcept
				{
					if (code >= 0x10000) {
						if (code < 0x110000) {
							if (pos + 1 < len) {
								if (utf16) {
									putUnicode2(code, utf16, pos);
								}
								pos += 2;
							}
						}
					} else {
						if (!SLIB_CHAR_IS_SURROGATE(code)) {
							if (utf16) {
								EndianHelper::write16(utf16, pos, (sl_char16)code);
							}
							pos++;
						}
					}
				}

				SLIB_INLINE static void putUnicode(sl_uint32 code, void* utf16, sl_size& pos) noexcept
				{
					if (code >= 0x10000) {
						if (code < 0x110000) {
							if (utf16) {
								putUnicode2(code, utf16, pos);
							}
							pos += 2;
						}
					} else {
						if (!SLIB_CHAR_IS_SURROGATE(code)) {
							if (utf16) {
								EndianHelper::write16(utf16, pos, (sl_char16)code);
							}
							pos++;
						}
					}
				}

			};

			template <class EndianHelper>
			class Utf32Helper
			{
			public:
				SLIB_INLINE static sl_bool getUnicode(sl_uint32& code, const void* utf32, sl_size len, sl_size& pos)
				{
					code = EndianHelper::read32(utf32, pos);
					pos++;
					return sl_true;
				}

				SLIB_INLINE static sl_bool getUnicode(sl_uint32& code, const void* utf32, sl_size& pos)
				{
					code = EndianHelper::read32(utf32, pos);
					pos++;
					return sl_true;
				}

				SLIB_INLINE static void putUnicode(sl_uint32 code, void* utf32, sl_size len, sl_size& pos) noexcept
				{
					if (utf32) {
						EndianHelper::write32(utf32, pos, code);
					}
					pos++;
				}

				SLIB_INLINE static void putUnicode(sl_uint32 code, void* utf32, sl_size& pos) noexcept
				{
					if (utf32) {
						EndianHelper::write32(utf32, pos, code);
					}
					pos++;
				}

			};

			template <class SrcHelper, class DstHelper>
			sl_size ConvertUtf(const void* src, sl_reg lenSrc, void* dst, sl_reg lenDst) noexcept
			{
				sl_size posDst = 0;
				sl_size posSrc = 0;
				sl_uint32 code;
				if (lenSrc < 0) {
					if (lenDst < 0) {
						for (;;) {
							if (SrcHelper::getUnicode(code, src, posSrc)) {
								if (!code) {
									break;
								}
								DstHelper::putUnicode(code, dst, posDst);
							} else {
								posSrc++;
							}
						}
					} else {
						while (posDst < (sl_size)lenDst) {
							if (SrcHelper::getUnicode(code, src, posSrc)) {
								if (!code) {
									break;
								}
								DstHelper::putUnicode(code, dst, lenDst, posDst);
							} else {
								posSrc++;
							}
						}
					}
				} else {
					if (lenDst < 0) {
						while (posSrc < (sl_size)lenSrc) {
							if (SrcHelper::getUnicode(code, src, lenSrc, posSrc)) {
								DstHelper::putUnicode(code, dst, posDst);
							} else {
								posSrc++;
							}
						}
					} else {
						while (posSrc < (sl_size)lenSrc && posDst < (sl_size)lenDst) {
							if (SrcHelper::getUnicode(code, src, lenSrc, posSrc)) {
								DstHelper::putUnicode(code, dst, lenDst, posDst);
							} else {
								posSrc++;
							}
						}
					}
				}
				return posDst;
			}

			SLIB_INLINE static sl_bool GetUnicode(sl_uint32& outCode, const sl_char8* data, sl_size len, sl_size& pos)
			{
				if (pos >= len) {
					return sl_false;
				}
				return Utf8Helper::getUnicode(outCode, data, len, pos);
			}

			SLIB_INLINE static sl_bool GetUnicode(sl_uint32& outCode, const sl_char16* data, sl_size len, sl_size& pos)
			{
				if (pos >= len) {
					return sl_false;
				}
				return Utf16Helper<NoEndianHelper>::getUnicode(outCode, data, len, pos);
			}

			SLIB_INLINE static sl_bool GetUnicode(sl_uint32& outCode, const sl_char32* data, sl_size len, sl_size& pos)
			{
				if (pos >= len) {
					return sl_false;
				}
				outCode = data[pos++];
				return sl_true;
			}

			template <class CHAR>
			static sl_size GetJoinedCharLength(sl_uint32 firstChar, const CHAR* dataNext, sl_size lenNext)
			{
				if (firstChar >= 0x100) {
					sl_uint32 next;
					sl_size n = 0;
					for (;;) {
						sl_size m = n;
						if (!(GetUnicode(next, dataNext, lenNext, n))) {
							return m;
						}
						if (next >= 0x1f3fb && next <= 0x1f3ff) {
							// EMOJI MODIFIER FITZPATRICK TYPE
							m = n;
							if (!(GetUnicode(next, dataNext, lenNext, n))) {
								return m;
							}
						}
						if (next != 0x200d) {
							// Not ZERO-WIDTH JOINER
							return m;
						}
						if (!(GetUnicode(next, dataNext, lenNext, n))) {
							return m;
						}
					}
				} else {
					switch (firstChar) {
						case '*': case '#':
						case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
							break;
						default:
							return 0;
					}
					sl_uint32 next;
					sl_size n = 0;
					if (GetUnicode(next, dataNext, lenNext, n)) {
						if (next == 0x20e3) {
							return n;
						}
					}
				}
				return 0;
			}

		}
	}

	using namespace priv::charset;

	sl_size Charsets::utf8ToUtf16(const sl_char8* utf8, sl_reg lenUtf8, sl_char16* utf16, sl_reg lenUtf16Buffer) noexcept
	{
		return ConvertUtf< Utf8Helper, Utf16Helper<NoEndianHelper> >(utf8, lenUtf8, utf16, lenUtf16Buffer);
	}

	sl_size Charsets::utf8ToUtf16(const sl_char8* utf8, sl_reg lenUtf8, EndianType endian16, void* utf16, sl_reg sizeUtf16Buffer) noexcept
	{
		sl_reg lenUtf16Buffer = sizeUtf16Buffer < 0 ? -1 : (sizeUtf16Buffer >> 1);
		if (endian16 == EndianType::Big) {
			return ConvertUtf< Utf8Helper, Utf16Helper<BigEndianHelper> >(utf8, lenUtf8, utf16, lenUtf16Buffer) << 1;
		} else {
			return ConvertUtf< Utf8Helper, Utf16Helper<LittleEndianHelper> >(utf8, lenUtf8, utf16, lenUtf16Buffer) << 1;
		}
	}

	sl_size Charsets::utf8ToUtf32(const sl_char8* utf8, sl_reg lenUtf8, sl_char32* utf32, sl_reg lenUtf32Buffer) noexcept
	{
		return ConvertUtf< Utf8Helper, Utf32Helper<NoEndianHelper> >(utf8, lenUtf8, utf32, lenUtf32Buffer);
	}

	sl_size Charsets::utf8ToUtf32(const sl_char8* utf8, sl_reg lenUtf8, EndianType endian32, void* utf32, sl_reg sizeUtf32Buffer) noexcept
	{
		sl_reg lenUtf32Buffer = sizeUtf32Buffer < 0 ? -1 : (sizeUtf32Buffer >> 2);
		if (endian32 == EndianType::Big) {
			return ConvertUtf< Utf8Helper, Utf32Helper<BigEndianHelper> >(utf8, lenUtf8, utf32, lenUtf32Buffer) << 2;
		} else {
			return ConvertUtf< Utf8Helper, Utf32Helper<LittleEndianHelper> >(utf8, lenUtf8, utf32, lenUtf32Buffer) << 2;
		}
	}

	sl_size Charsets::utf16ToUtf8(const sl_char16* utf16, sl_reg lenUtf16, sl_char8* utf8, sl_reg lenUtf8Buffer) noexcept
	{
		return ConvertUtf< Utf16Helper<NoEndianHelper>, Utf8Helper >(utf16, lenUtf16, utf8, lenUtf8Buffer);
	}

	sl_size Charsets::utf16ToUtf8(EndianType endian16, const void* utf16, sl_size sizeUtf16, sl_char8* utf8, sl_reg lenUtf8Buffer) noexcept
	{
		sl_size lenUtf16 = sizeUtf16 >> 1;
		if (endian16 == EndianType::Big) {
			return ConvertUtf< Utf16Helper<BigEndianHelper>, Utf8Helper >(utf16, lenUtf16, utf8, lenUtf8Buffer);
		} else {
			return ConvertUtf< Utf16Helper<LittleEndianHelper>, Utf8Helper >(utf16, lenUtf16, utf8, lenUtf8Buffer);
		}
	}

	sl_size Charsets::utf16ToUtf32(const sl_char16* utf16, sl_reg lenUtf16, sl_char32* utf32, sl_reg lenUtf32Buffer) noexcept
	{
		return ConvertUtf< Utf16Helper<NoEndianHelper>, Utf32Helper<NoEndianHelper> >(utf16, lenUtf16, utf32, lenUtf32Buffer);
	}

	sl_size Charsets::utf16ToUtf32(const sl_char16* utf16, sl_reg lenUtf16, EndianType endian32, void* utf32, sl_reg sizeUtf32Buffer) noexcept
	{
		sl_reg lenUtf32Buffer = sizeUtf32Buffer < 0 ? -1 : (sizeUtf32Buffer >> 2);
		if (endian32 == EndianType::Big) {
			return ConvertUtf< Utf16Helper<NoEndianHelper>, Utf32Helper<BigEndianHelper> >(utf16, lenUtf16, utf32, lenUtf32Buffer) << 2;
		} else {
			return ConvertUtf< Utf16Helper<NoEndianHelper>, Utf32Helper<LittleEndianHelper> >(utf16, lenUtf16, utf32, lenUtf32Buffer) << 2;
		}
	}

	sl_size Charsets::utf16ToUtf32(EndianType endian16, const void* utf16, sl_size sizeUtf16, sl_char32* utf32, sl_reg lenUtf32Buffer) noexcept
	{
		sl_size lenUtf16 = sizeUtf16 >> 1;
		if (endian16 == EndianType::Big) {
			return ConvertUtf< Utf16Helper<BigEndianHelper>, Utf32Helper<NoEndianHelper> >(utf16, lenUtf16, utf32, lenUtf32Buffer);
		} else {
			return ConvertUtf< Utf16Helper<LittleEndianHelper>, Utf32Helper<NoEndianHelper> >(utf16, lenUtf16, utf32, lenUtf32Buffer);
		}
	}

	sl_size Charsets::utf16ToUtf32(EndianType endian16, const void* utf16, sl_size sizeUtf16, EndianType endian32, void* utf32, sl_reg sizeUtf32Buffer) noexcept
	{
		sl_size lenUtf16 = sizeUtf16 >> 1;
		sl_reg lenUtf32Buffer = sizeUtf32Buffer < 0 ? -1 : (sizeUtf32Buffer >> 2);
		if (endian16 == EndianType::Big) {
			if (endian32 == EndianType::Big) {
				return ConvertUtf< Utf16Helper<BigEndianHelper>, Utf32Helper<BigEndianHelper> >(utf16, lenUtf16, utf32, lenUtf32Buffer) << 2;
			} else {
				return ConvertUtf< Utf16Helper<BigEndianHelper>, Utf32Helper<LittleEndianHelper> >(utf16, lenUtf16, utf32, lenUtf32Buffer) << 2;
			}
		} else {
			if (endian32 == EndianType::Big) {
				return ConvertUtf< Utf16Helper<LittleEndianHelper>, Utf32Helper<BigEndianHelper> >(utf16, lenUtf16, utf32, lenUtf32Buffer) << 2;
			} else {
				return ConvertUtf< Utf16Helper<LittleEndianHelper>, Utf32Helper<LittleEndianHelper> >(utf16, lenUtf16, utf32, lenUtf32Buffer) << 2;
			}
		}
	}
	
	sl_size Charsets::utf32ToUtf8(const sl_char32* utf32, sl_reg lenUtf32, sl_char8* utf8, sl_reg lenUtf8Buffer) noexcept
	{
		return ConvertUtf< Utf32Helper<NoEndianHelper>, Utf8Helper >(utf32, lenUtf32, utf8, lenUtf8Buffer);
	}

	sl_size Charsets::utf32ToUtf8(EndianType endian32, const void* utf32, sl_size sizeUtf32, sl_char8* utf8, sl_reg lenUtf8Buffer) noexcept
	{
		sl_reg lenUtf32 = sizeUtf32 >> 2;
		if (endian32 == EndianType::Big) {
			return ConvertUtf< Utf32Helper<BigEndianHelper>, Utf8Helper >(utf32, lenUtf32, utf8, lenUtf8Buffer);
		} else {
			return ConvertUtf< Utf32Helper<LittleEndianHelper>, Utf8Helper >(utf32, lenUtf32, utf8, lenUtf8Buffer);
		}
	}

	sl_size Charsets::utf32ToUtf16(const sl_char32* utf32, sl_reg lenUtf32, sl_char16* utf16, sl_reg lenUtf16Buffer) noexcept
	{
		return ConvertUtf< Utf32Helper<NoEndianHelper>, Utf16Helper<NoEndianHelper> >(utf32, lenUtf32, utf16, lenUtf16Buffer);
	}

	sl_size Charsets::utf32ToUtf16(EndianType endian32, const void* utf32, sl_size sizeUtf32, sl_char16* utf16, sl_reg lenUtf16Buffer) noexcept
	{
		sl_size lenUtf32 = sizeUtf32 >> 2;
		if (endian32 == EndianType::Big) {
			return ConvertUtf< Utf32Helper<BigEndianHelper>, Utf16Helper<NoEndianHelper> >(utf32, lenUtf32, utf16, lenUtf16Buffer);
		} else {
			return ConvertUtf< Utf32Helper<LittleEndianHelper>, Utf16Helper<NoEndianHelper> >(utf32, lenUtf32, utf16, lenUtf16Buffer);
		}
	}

	sl_size Charsets::utf32ToUtf16(const sl_char32* utf32, sl_reg lenUtf32, EndianType endian16, void* utf16, sl_reg sizeUtf16Buffer) noexcept
	{
		sl_size lenUtf16Buffer = sizeUtf16Buffer < 0 ? -1 : (sizeUtf16Buffer >> 1);
		if (endian16 == EndianType::Big) {
			return ConvertUtf< Utf32Helper<NoEndianHelper>, Utf16Helper<BigEndianHelper> >(utf32, lenUtf32, utf16, lenUtf16Buffer) << 1;
		} else {
			return ConvertUtf< Utf32Helper<NoEndianHelper>, Utf16Helper<LittleEndianHelper> >(utf32, lenUtf32, utf16, lenUtf16Buffer) << 1;
		}
	}

	sl_size Charsets::utf32ToUtf16(EndianType endian32, const void* utf32, sl_size sizeUtf32, EndianType endian16, void* utf16, sl_reg sizeUtf16Buffer) noexcept
	{
		sl_size lenUtf32 = sizeUtf32 >> 2;
		sl_size lenUtf16Buffer = sizeUtf16Buffer < 0 ? -1 : (sizeUtf16Buffer >> 1);
		if (endian32 == EndianType::Big) {
			if (endian16 == EndianType::Big) {
				return ConvertUtf< Utf32Helper<BigEndianHelper>, Utf16Helper<BigEndianHelper> >(utf32, lenUtf32, utf16, lenUtf16Buffer) << 1;
			} else {
				return ConvertUtf< Utf32Helper<BigEndianHelper>, Utf16Helper<LittleEndianHelper> >(utf32, lenUtf32, utf16, lenUtf16Buffer) << 1;
			}
		} else {
			if (endian16 == EndianType::Big) {
				return ConvertUtf< Utf32Helper<LittleEndianHelper>, Utf16Helper<BigEndianHelper> >(utf32, lenUtf32, utf16, lenUtf16Buffer) << 1;
			} else {
				return ConvertUtf< Utf32Helper<LittleEndianHelper>, Utf16Helper<LittleEndianHelper> >(utf32, lenUtf32, utf16, lenUtf16Buffer) << 1;
			}
		}
	}

	void Charsets::utf16ToUtf16(const sl_char16* src, EndianType endianDst, void* dst, sl_size len)
	{
		utf16ToUtf16(Endian::get(), src, endianDst, dst, len);
	}

	void Charsets::utf16ToUtf16(EndianType endianSrc, const void* src, sl_char16* dst, sl_size len)
	{
		utf16ToUtf16(endianSrc, src, Endian::get(), dst, len);
	}

	void Charsets::utf16ToUtf16(EndianType endianSrc, const void* src, EndianType endianDst, void* dst, sl_size len)
	{
		if (endianSrc == endianDst) {
			if (dst != src) {
				Base::copyMemory(dst, src, len << 1);
			}
		} else {
			sl_uint8* s = (sl_uint8*)src;
			sl_uint8* d = (sl_uint8*)dst;
			for (sl_size i = 0; i < len; i++) {
				sl_uint8 m1 = *(s++);
				sl_uint8 m2 = *(s++);
				*(d++) = m2;
				*(d++) = m1;
			}
		}
	}

	void Charsets::utf32ToUtf32(const sl_char32* src, EndianType endianDst, void* dst, sl_size len)
	{
		utf32ToUtf32(Endian::get(), src, endianDst, dst, len);
	}

	void Charsets::utf32ToUtf32(EndianType endianSrc, const void* src, sl_char32* dst, sl_size len)
	{
		utf32ToUtf32(endianSrc, src, Endian::get(), dst, len);
	}

	void Charsets::utf32ToUtf32(EndianType endianSrc, const void* src, EndianType endianDst, void* dst, sl_size len)
	{
		if (endianSrc == endianDst) {
			if (dst != src) {
				Base::copyMemory(dst, src, len << 2);
			}
		} else {
			sl_uint8* s = (sl_uint8*)src;
			sl_uint8* d = (sl_uint8*)dst;
			for (sl_size i = 0; i < len; i++) {
				sl_uint8 m1 = *(s++);
				sl_uint8 m2 = *(s++);
				sl_uint8 m3 = *(s++);
				sl_uint8 m4 = *(s++);
				*(d++) = m4;
				*(d++) = m3;
				*(d++) = m2;
				*(d++) = m1;
			}
		}
	}

	sl_bool Charsets::checkUtf8(const void* _buf, sl_size size)
	{
		sl_uint8* buf = (sl_uint8*)_buf;
		for (sl_size i = 0; i < size; i++) {
			sl_uint8 ch = buf[i];
			if (ch >= 0x80) {
				if (ch < 0xC0) {
					return sl_false;
				} else if (ch < 0xE0) {
					if (i + 1 < size) {
						sl_uint32 ch1 = (sl_uint32)((sl_uint8)buf[++i]);
						if ((ch1 & 0xC0) != 0x80) {
							return sl_false;
						}
					} else {
						return sl_false;
					}
				} else if (ch < 0xF0) {
					if (i + 2 < size) {
						sl_uint32 ch1 = (sl_uint32)((sl_uint8)buf[++i]);
						if ((ch1 & 0xC0) == 0x80) {
							sl_uint32 ch2 = (sl_uint32)((sl_uint8)buf[++i]);
							if ((ch2 & 0xC0) != 0x80) {
								return sl_false;
							}
						} else {
							return sl_false;
						}
					} else {
						return sl_false;
					}
				} else if (ch < 0xF8) {
					if (i + 3 < size) {
						sl_uint32 ch1 = (sl_uint32)((sl_uint8)buf[++i]);
						if ((ch1 & 0xC0) == 0x80) {
							sl_uint32 ch2 = (sl_uint32)((sl_uint8)buf[++i]);
							if ((ch2 & 0xC0) == 0x80) {
								sl_uint32 ch3 = (sl_uint32)((sl_uint8)buf[++i]);
								if ((ch3 & 0xC0) != 0x80) {
									return sl_false;
								}
							} else {
								return sl_false;
							}
						} else {
							return sl_false;
						}
					} else {
						return sl_false;
					}
				} else {
					return sl_false;
				}
			}
		}
		return sl_true;
	}

	sl_size Charsets::getUtf8(sl_char32 code, sl_char8* utf8, sl_size lenUtf8Buffer) noexcept
	{
		sl_size n = 0;
		Utf8Helper::putUnicode(code, utf8, lenUtf8Buffer, n);
		return n;
	}

	sl_size Charsets::getUtf8(sl_char32 code, sl_char8* utf8) noexcept
	{
		sl_size n = 0;
		Utf8Helper::putUnicode(code, utf8, n);
		return n;
	}

	sl_size Charsets::getUtf16(sl_char32 code, sl_char16* utf16, sl_size lenUtf16Buffer) noexcept
	{
		sl_size n = 0;
		Utf16Helper<NoEndianHelper>::putUnicode(code, utf16, lenUtf16Buffer, n);
		return n;
	}

	sl_size Charsets::getUtf16(sl_char32 code, sl_char16* utf16) noexcept
	{
		sl_size n = 0;
		Utf16Helper<NoEndianHelper>::putUnicode(code, utf16, n);
		return n;
	}

	sl_size Charsets::getUtfn(sl_char32 code, sl_char8* buf, sl_size lenBuf) noexcept
	{
		sl_size n = 0;
		Utf8Helper::putUnicode(code, buf, lenBuf, n);
		return n;
	}

	sl_size Charsets::getUtfn(sl_char32 code, sl_char8* buf) noexcept
	{
		sl_size n = 0;
		Utf8Helper::putUnicode(code, buf, n);
		return n;
	}

	sl_size Charsets::getUtfn(sl_char32 code, sl_char16* buf, sl_size lenBuf) noexcept
	{
		sl_size n = 0;
		Utf16Helper<NoEndianHelper>::putUnicode(code, buf, lenBuf, n);
		return n;
	}

	sl_size Charsets::getUtfn(sl_char32 code, sl_char16* buf) noexcept
	{
		sl_size n = 0;
		Utf16Helper<NoEndianHelper>::putUnicode(code, buf, n);
		return n;
	}

	sl_size Charsets::getUtfn(sl_char32 code, sl_char32* buf, sl_size lenBuf) noexcept
	{
		if (lenBuf) {
			if (buf) {
				*buf = code;
			}
			return 1;
		} else {
			return 0;
		}
	}

	sl_size Charsets::getUtfn(sl_char32 code, sl_char32* buf) noexcept
	{
		if (buf) {
			*buf = code;
		}
		return 1;
	}

	sl_bool Charsets::getUnicode(sl_char32& outCode, const sl_char8* utf8, sl_size lenUtf8, sl_size& posUtf8)
	{
		return Utf8Helper::getUnicode(*((sl_uint32*)&outCode), utf8, lenUtf8, posUtf8);
	}

	sl_bool Charsets::getUnicode(sl_char32& outCode, const sl_char16* utf16, sl_size lenUtf16, sl_size& posUtf16)
	{
		return Utf16Helper<NoEndianHelper>::getUnicode(*((sl_uint32*)&outCode), utf16, lenUtf16, posUtf16);
	}

	sl_char32 Charsets::getUnicodeFromSurrogateCharacters(sl_char16 ch0, sl_char16 ch1)
	{
		sl_uint32 ch;
		if (Utf16Helper<NoEndianHelper>::getUnicode2(ch, ch0, ch1)) {
			return ch;
		}
		return 0;
	}

	sl_bool Charsets::isEmoji(sl_char32 code)
	{
		if (code < 0x100) {
			switch (code) {
				case '*': case '#':
				case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
				case 0xA9: case 0xAE:
					return sl_true;
			}
		} else if (code < 0x10000) {
			switch (code) {
				case 0x203c: case 0x2049: case 0x20e3: case 0x2122: case 0x2139: case 0x2194: case 0x2195: case 0x2196: case 0x2197: case 0x2198:
				case 0x2199: case 0x21a9: case 0x21aa: case 0x231a: case 0x231b: case 0x2328: case 0x23cf: case 0x23e9: case 0x23ea: case 0x23eb:
				case 0x23ec: case 0x23ed: case 0x23ee: case 0x23ef: case 0x23f0: case 0x23f1: case 0x23f2: case 0x23f3: case 0x23f8: case 0x23f9:
				case 0x23fa: case 0x24c2: case 0x25aa: case 0x25ab: case 0x25b6: case 0x25c0: case 0x25fb: case 0x25fc: case 0x25fd: case 0x25fe:
				case 0x2600: case 0x2601: case 0x2602: case 0x2603: case 0x2604: case 0x260e: case 0x2611: case 0x2614: case 0x2615: case 0x2618:
				case 0x261d: case 0x2620: case 0x2622: case 0x2623: case 0x2626: case 0x262a: case 0x262e: case 0x262f: case 0x2638: case 0x2639:
				case 0x263a: case 0x2640: case 0x2642: case 0x2648: case 0x2649: case 0x264a: case 0x264b: case 0x264c: case 0x264d: case 0x264e:
				case 0x264f: case 0x2650: case 0x2651: case 0x2652: case 0x2653: case 0x265f: case 0x2660: case 0x2663: case 0x2665: case 0x2666:
				case 0x2668: case 0x267b: case 0x267e: case 0x267f: case 0x2692: case 0x2693: case 0x2694: case 0x2695: case 0x2696: case 0x2697:
				case 0x2699: case 0x269b: case 0x269c: case 0x26a0: case 0x26a1: case 0x26aa: case 0x26ab: case 0x26b0: case 0x26b1: case 0x26bd:
				case 0x26be: case 0x26c4: case 0x26c5: case 0x26c8: case 0x26ce: case 0x26cf: case 0x26d1: case 0x26d3: case 0x26d4: case 0x26e9:
				case 0x26ea: case 0x26f0: case 0x26f1: case 0x26f2: case 0x26f3: case 0x26f4: case 0x26f5: case 0x26f7: case 0x26f8: case 0x26f9:
				case 0x26fa: case 0x26fd: case 0x2702: case 0x2705: case 0x2708: case 0x2709: case 0x270a: case 0x270b: case 0x270c: case 0x270d:
				case 0x270f: case 0x2712: case 0x2714: case 0x2716: case 0x271d: case 0x2721: case 0x2728: case 0x2733: case 0x2734: case 0x2744:
				case 0x2747: case 0x274c: case 0x274e: case 0x2753: case 0x2754: case 0x2755: case 0x2757: case 0x2763: case 0x2764: case 0x2795:
				case 0x2796: case 0x2797: case 0x27a1: case 0x27b0: case 0x27bf: case 0x2934: case 0x2935: case 0x2b05: case 0x2b06: case 0x2b07:
				case 0x2b1b: case 0x2b1c: case 0x2b50: case 0x2b55: case 0x3030: case 0x303d: case 0x3297: case 0x3299:
					return sl_true;
			}
		} else {
			if (code >= 0x1f191 && code <= 0x1f19a) {
				return sl_true;
			}
			if (code >= 0x1f1e6 && code <= 0x1f1ff) {
				return sl_true;
			}
			if (code >= 0x1f232 && code <= 0x1f23a) {
				return sl_true;
			}
			if (code >= 0x1f300 && code <= 0x1f64f) {
				return sl_true;
			}
			if (code >= 0x1f680 && code <= 0x1f6ff) {
				return sl_true;
			}
			if (code >= 0x1f900 && code <= 0x1f9ff) {
				return sl_true;
			}
			switch (code) {
				case 0x1f004: case 0x1f0cf: case 0x1f170: case 0x1f171: case 0x1f17e: case 0x1f17f: case 0x1f18e:
				case 0x1f201: case 0x1f202: case 0x1f21a: case 0x1f22f:
				case 0x1f250: case 0x1f251:
					return sl_true;
			}
		}
		return sl_false;
	}

	sl_size Charsets::getJoinedCharLength(sl_char32 firstChar, const sl_char8* dataNext, sl_size lenNext)
	{
		return GetJoinedCharLength(firstChar, dataNext, lenNext);
	}

	sl_size Charsets::getJoinedCharLength(sl_char32 firstChar, const sl_char16* dataNext, sl_size lenNext)
	{
		return GetJoinedCharLength(firstChar, dataNext, lenNext);
	}

	sl_size Charsets::getJoinedCharLength(sl_char32 firstChar, const sl_char32* dataNext, sl_size lenNext)
	{
		return GetJoinedCharLength(firstChar, dataNext, lenNext);
	}

}
