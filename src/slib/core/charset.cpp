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

			template <EndianType endian>
			SLIB_INLINE static sl_char16 Read16(const void* src, sl_size pos) noexcept
			{
				sl_uint8* s = ((sl_uint8*)src) + (pos << 1);
				if (endian == EndianType::Little) {
					return (sl_char16)(((sl_uint16)(s[0])) | ((sl_uint16)(s[1]) << 8));
				} else {
					return (sl_char16)(((sl_uint16)(s[0]) << 8) | ((sl_uint16)(s[1])));
				}
			}

			template <EndianType endian>
			SLIB_INLINE static void Write16(void* dst, sl_size pos, sl_char16 _v) noexcept
			{
				sl_uint16 v = (sl_uint16)_v;
				sl_uint8* d = ((sl_uint8*)dst) + (pos << 1);
				if (endian == EndianType::Little) {
					d[0] = (sl_uint8)(v);
					d[1] = (sl_uint8)(v >> 8);
				} else {
					d[0] = (sl_uint8)(v >> 8);
					d[1] = (sl_uint8)(v);
				}
			}

			template <EndianType endian>
			SLIB_INLINE static sl_char32 Read32(const void* src, sl_size pos) noexcept
			{
				sl_uint8* s = ((sl_uint8*)src) + (pos << 2);
				if (endian == EndianType::Little) {
					return (sl_char32)(((sl_uint32)(s[0])) | ((sl_uint32)(s[1]) << 8) | ((sl_uint32)(s[2]) << 16) | ((sl_uint32)(s[3]) << 24));
				} else {
					return (sl_char32)(((sl_uint32)(s[0]) << 24) | ((sl_uint32)(s[1]) << 16) | ((sl_uint32)(s[2]) << 8) | ((sl_uint32)(s[3])));
				}
			}

			template <EndianType endian>
			SLIB_INLINE static void Write32(void* dst, sl_size pos, sl_char32 _v) noexcept
			{
				sl_uint32 v = (sl_uint32)_v;
				sl_uint8* d = ((sl_uint8*)dst) + (pos << 2);
				if (endian == EndianType::Little) {
					d[0] = (sl_uint8)(v);
					d[1] = (sl_uint8)(v >> 8);
					d[2] = (sl_uint8)(v >> 16);
					d[3] = (sl_uint8)(v >> 24);
				} else {
					d[0] = (sl_uint8)(v >> 24);
					d[1] = (sl_uint8)(v >> 16);
					d[2] = (sl_uint8)(v >> 8);
					d[3] = (sl_uint8)(v);
				}
			}

			template <EndianType endian>
			static sl_size ConvertUtf8ToUtf16(const sl_char8* utf8, sl_reg lenUtf8, void* utf16, sl_reg lenUtf16Buffer) noexcept
			{
				sl_bool flagSz = sl_false;
				if (lenUtf8 < 0) {
					flagSz = sl_true;
					lenUtf8 = SLIB_REG_MAX;
				}
				sl_reg n = 0;
				sl_reg i = 0;
				for (;;) {
					if (lenUtf16Buffer >= 0) {
						if (n >= lenUtf16Buffer) {
							break;
						}
					}
					sl_uint32 ch;
					if (flagSz) {
						ch = (sl_uint32)((sl_uint8)utf8[i]);
						if (!ch) {
							break;
						}
					} else {
						if (i < lenUtf8) {
							ch = (sl_uint32)((sl_uint8)utf8[i]);
						} else {
							break;
						}
					}
					if (ch < 0x80) {
						if (utf16) {
							Write16<endian>(utf16, n++, (sl_char16)ch);
						} else {
							n++;
						}
					} else if (ch < 0xC0) {
						// Corrupted data element
					} else if (ch < 0xE0) {
						if (i + 1 < lenUtf8) {
							sl_uint32 ch1 = (sl_uint32)((sl_uint8)utf8[++i]);
							if ((ch1 & 0xC0) == 0x80) {
								if (utf16) {
									Write16<endian>(utf16, n++, (sl_char16)(((ch & 0x1F) << 6) | (ch1 & 0x3F)));
								} else {
									n++;
								}
							}
						}
					} else if (ch < 0xF0) {
						if (i + 2 < lenUtf8) {
							sl_uint32 ch1 = (sl_uint32)((sl_uint8)utf8[++i]);
							if ((ch1 & 0xC0) == 0x80) {
								sl_uint32 ch2 = (sl_uint32)((sl_uint8)utf8[++i]);
								if ((ch2 & 0xC0) == 0x80) {
									if (utf16) {
										Write16<endian>(utf16, n++, (sl_char16)(((ch & 0x0F) << 12) | ((ch1 & 0x3F) << 6) | (ch2 & 0x3F)));
									} else {
										n++;
									}
								}
							}
						}
					} else if (ch < 0xF8) {
						if (i + 3 < lenUtf8 && (lenUtf16Buffer < 0 || n + 1 < lenUtf16Buffer)) {
							sl_uint32 ch1 = (sl_uint32)((sl_uint8)utf8[++i]);
							if ((ch1 & 0xC0) == 0x80) {
								sl_uint32 ch2 = (sl_uint32)((sl_uint8)utf8[++i]);
								if ((ch2 & 0xC0) == 0x80) {
									sl_uint32 ch3 = (sl_uint32)((sl_uint8)utf8[++i]);
									if ((ch3 & 0xC0) == 0x80) {
										if (utf16) {
											sl_char32 u32 = (sl_char32)(((ch & 0x07) << 18) | ((ch1 & 0x3F) << 12) | ((ch2 & 0x3F) << 6) | (ch3 & 0x3F));
											u32 -= 0x10000;
											Write16<endian>(utf16, n++, (sl_char16)(0xD800 + (u32 >> 10)));
											Write16<endian>(utf16, n++, (sl_char16)(0xDC00 + (u32 & 0x3FF)));
										} else {
											n += 2;
										}
									}
								}
							}
						}
					} else if (ch < 0xFC) {
						if (i + 4 < lenUtf8 && (lenUtf16Buffer < 0 || n + 1 < lenUtf16Buffer)) {
							sl_uint32 ch1 = (sl_uint32)((sl_uint8)utf8[++i]);
							if ((ch1 & 0xC0) == 0x80) {
								sl_uint32 ch2 = (sl_uint32)((sl_uint8)utf8[++i]);
								if ((ch2 & 0xC0) == 0x80) {
									sl_uint32 ch3 = (sl_uint32)((sl_uint8)utf8[++i]);
									if ((ch3 & 0xC0) == 0x80) {
										sl_uint32 ch4 = (sl_uint32)((sl_uint8)utf8[++i]);
										if ((ch4 & 0xC0) == 0x80) {
											if (utf16) {
												sl_char32 u32 = (sl_char32)(((ch & 0x03) << 24) | ((ch1 & 0x3F) << 18) | ((ch2 & 0x3F) << 12) | ((ch3 & 0x3F) << 6) | (ch4 & 0x3F));
												u32 -= 0x10000;
												Write16<endian>(utf16, n++, (sl_char16)(0xD800 + (u32 >> 10)));
												Write16<endian>(utf16, n++, (sl_char16)(0xDC00 + (u32 & 0x3FF)));
											} else {
												n += 2;
											}
										}
									}
								}
							}
						}
					}
					i++;
				}
				return n;
			}

			static sl_size Utf8ToUtf16(const sl_char8* utf8, sl_reg lenUtf8, EndianType endian, void* utf16, sl_reg lenUtf16Buffer) noexcept
			{
				if (endian == EndianType::Big) {
					return ConvertUtf8ToUtf16<EndianType::Big>(utf8, lenUtf8, utf16, lenUtf16Buffer);
				} else {
					return ConvertUtf8ToUtf16<EndianType::Little>(utf8, lenUtf8, utf16, lenUtf16Buffer);
				}
			}

			template <EndianType endian>
			static sl_size ConvertUtf8ToUtf32(const sl_char8* utf8, sl_reg lenUtf8, void* utf32, sl_reg lenUtf32Buffer) noexcept
			{
				sl_bool flagSz = sl_false;
				if (lenUtf8 < 0) {
					flagSz = sl_true;
					lenUtf8 = SLIB_REG_MAX;
				}
				sl_reg n = 0;
				sl_reg i = 0;
				for (;;) {
					if (lenUtf32Buffer >= 0) {
						if (n >= lenUtf32Buffer) {
							break;
						}
					}
					sl_uint32 ch;
					if (flagSz) {
						ch = (sl_uint32)((sl_uint8)utf8[i]);
						if (!ch) {
							break;
						}
					} else {
						if (i < lenUtf8) {
							ch = (sl_uint32)((sl_uint8)utf8[i]);
						} else {
							break;
						}
					}
					if (ch < 0x80) {
						if (utf32) {
							Write32<endian>(utf32, n++, (sl_char32)ch);
						} else {
							n++;
						}
					} else if (ch < 0xC0) {
						// Corrupted data element
					} else if (ch < 0xE0) {
						if (i + 1 < lenUtf8) {
							sl_uint32 ch1 = (sl_uint32)((sl_uint8)utf8[++i]);
							if ((ch1 & 0xC0) == 0x80) {
								if (utf32) {
									Write32<endian>(utf32, n++, (sl_char32)(((ch & 0x1F) << 6) | (ch1 & 0x3F)));
								} else {
									n++;
								}
							}
						}
					} else if (ch < 0xF0) {
						if (i + 2 < lenUtf8) {
							sl_uint32 ch1 = (sl_uint32)((sl_uint8)utf8[++i]);
							if ((ch1 & 0xC0) == 0x80) {
								sl_uint32 ch2 = (sl_uint32)((sl_uint8)utf8[++i]);
								if ((ch2 & 0xC0) == 0x80) {
									if (utf32) {
										Write32<endian>(utf32, n++, (sl_char32)(((ch & 0x0F) << 12) | ((ch1 & 0x3F) << 6) | (ch2 & 0x3F)));
									} else {
										n++;
									}
								}
							}
						}
					} else if (ch < 0xF8) {
						if (i + 3 < lenUtf8) {
							sl_uint32 ch1 = (sl_uint32)((sl_uint8)utf8[++i]);
							if ((ch1 & 0xC0) == 0x80) {
								sl_uint32 ch2 = (sl_uint32)((sl_uint8)utf8[++i]);
								if ((ch2 & 0xC0) == 0x80) {
									sl_uint32 ch3 = (sl_uint32)((sl_uint8)utf8[++i]);
									if ((ch3 & 0xC0) == 0x80) {
										if (utf32) {
											Write32<endian>(utf32, n++, (sl_char32)(((ch & 0x07) << 18) | ((ch1 & 0x3F) << 12) | ((ch2 & 0x3F) << 6) | (ch3 & 0x3F)));
										} else {
											n++;
										}
									}
								}
							}
						}
					} else if (ch < 0xFC) {
						if (i + 4 < lenUtf8) {
							sl_uint32 ch1 = (sl_uint32)((sl_uint8)utf8[++i]);
							if ((ch1 & 0xC0) == 0x80) {
								sl_uint32 ch2 = (sl_uint32)((sl_uint8)utf8[++i]);
								if ((ch2 & 0xC0) == 0x80) {
									sl_uint32 ch3 = (sl_uint32)((sl_uint8)utf8[++i]);
									if ((ch3 & 0xC0) == 0x80) {
										sl_uint32 ch4 = (sl_uint32)((sl_uint8)utf8[++i]);
										if ((ch4 & 0xC0) == 0x80) {
											if (utf32) {
												Write32<endian>(utf32, n++, (sl_char32)(((ch & 0x03) << 24) | ((ch1 & 0x3F) << 18) | ((ch2 & 0x3F) << 12) | ((ch3 & 0x3F) << 6) | (ch4 & 0x3F)));
											} else {
												n++;
											}
										}
									}
								}
							}
						}
					} else if (ch < 0xFE) {
						if (i + 5 < lenUtf8) {
							sl_uint32 ch1 = (sl_uint32)((sl_uint8)utf8[++i]);
							if ((ch1 & 0xC0) == 0x80) {
								sl_uint32 ch2 = (sl_uint32)((sl_uint8)utf8[++i]);
								if ((ch2 & 0xC0) == 0x80) {
									sl_uint32 ch3 = (sl_uint32)((sl_uint8)utf8[++i]);
									if ((ch3 & 0xC0) == 0x80) {
										sl_uint32 ch4 = (sl_uint32)((sl_uint8)utf8[++i]);
										if ((ch4 & 0xC0) == 0x80) {
											sl_uint32 ch5 = (sl_uint32)((sl_uint8)utf8[++i]);
											if (utf32) {
												Write32<endian>(utf32, n++, (sl_char32)(((ch & 0x01) << 30) | ((ch1 & 0x3F) << 24) | ((ch2 & 0x3F) << 18) | ((ch3 & 0x3F) << 12) | ((ch4 & 0x3F) << 6) | (ch5 & 0x3F)));
											} else {
												n++;
											}
										}
									}
								}
							}
						}
					}
					i++;
				}
				return n;
			}

			static sl_size Utf8ToUtf32(const sl_char8* utf8, sl_reg lenUtf8, EndianType endian, void* utf32, sl_reg lenUtf32Buffer) noexcept
			{
				if (endian == EndianType::Big) {
					return ConvertUtf8ToUtf32<EndianType::Big>(utf8, lenUtf8, utf32, lenUtf32Buffer);
				} else {
					return ConvertUtf8ToUtf32<EndianType::Little>(utf8, lenUtf8, utf32, lenUtf32Buffer);
				}
			}

			template <EndianType endian>
			sl_size ConvertUtf16ToUtf8(const void* utf16, sl_reg lenUtf16, sl_char8* utf8, sl_reg lenUtf8Buffer) noexcept
			{
				sl_bool flagSz = sl_false;
				if (lenUtf16 < 0) {
					flagSz = sl_true;
					lenUtf16 = SLIB_REG_MAX;
				}
				sl_reg n = 0;
				sl_reg i = 0;
				for (;;) {
					if (lenUtf8Buffer >= 0) {
						if (n >= lenUtf8Buffer) {
							break;
						}
					}
					sl_uint32 ch;
					if (flagSz) {
						ch = (sl_uint16)(Read16<endian>(utf16, i));
						if (!ch) {
							break;
						}
					} else {
						if (i < lenUtf16) {
							ch = (sl_uint16)(Read16<endian>(utf16, i));
						} else {
							break;
						}
					}
					if (ch < 0x80) {
						if (utf8) {
							utf8[n++] = (sl_char8)(ch);
						} else {
							n++;
						}
					} else if (ch < 0x800) {
						if (lenUtf8Buffer < 0 || n + 1 < lenUtf8Buffer) {
							if (utf8) {
								utf8[n++] = (sl_char8)((ch >> 6) | 0xC0);
								utf8[n++] = (sl_char8)((ch & 0x3F) | 0x80);
							} else {
								n += 2;
							}
						}
					} else {
						if (ch < 0xD800 || ch >= 0xE000) {
							if (lenUtf8Buffer < 0 || n + 2 < lenUtf8Buffer) {
								if (utf8) {
									utf8[n++] = (sl_char8)((ch >> 12) | 0xE0);
									utf8[n++] = (sl_char8)(((ch >> 6) & 0x3F) | 0x80);
									utf8[n++] = (sl_char8)((ch & 0x3F) | 0x80);
								} else {
									n += 3;
								}
							}
						} else {
							if (i + 1 < lenUtf16) {
								sl_uint32 ch1 = (sl_uint16)(Read16<endian>(utf16, ++i));
								if (ch < 0xDC00 && ch1 >= 0xDC00 && ch1 < 0xE000) {
									sl_char32 u32 = (sl_char32)(((ch - 0xD800) << 10) | (ch1 - 0xDC00)) + 0x10000;
									if (lenUtf8Buffer < 0 || n + 3 < lenUtf8Buffer) {
										if (utf8) {
											utf8[n++] = (sl_char8)((u32 >> 18) | 0xF0);
											utf8[n++] = (sl_char8)(((u32 >> 12) & 0x3F) | 0x80);
											utf8[n++] = (sl_char8)(((u32 >> 6) & 0x3F) | 0x80);
											utf8[n++] = (sl_char8)((u32 & 0x3F) | 0x80);
										} else {
											n += 4;
										}
									}
								}
							}
						}
					}
					i++;
				}
				return n;
			}

			static sl_size Utf16ToUtf8(EndianType endian, const void* utf16, sl_reg lenUtf16, sl_char8* utf8, sl_reg lenUtf8Buffer) noexcept
			{
				if (endian == EndianType::Big) {
					return ConvertUtf16ToUtf8<EndianType::Big>(utf16, lenUtf16, utf8, lenUtf8Buffer);
				} else {
					return ConvertUtf16ToUtf8<EndianType::Little>(utf16, lenUtf16, utf8, lenUtf8Buffer);
				}
			}

			template <EndianType endian>
			sl_size ConvertUtf32ToUtf8(const void* utf32, sl_reg lenUtf32, sl_char8* utf8, sl_reg lenUtf8Buffer) noexcept
			{
				sl_bool flagSz = sl_false;
				if (lenUtf32 < 0) {
					flagSz = sl_true;
					lenUtf32 = SLIB_REG_MAX;
				}
				sl_reg n = 0;
				sl_reg i = 0;
				for (;;) {
					if (lenUtf8Buffer >= 0) {
						if (n >= lenUtf8Buffer) {
							break;
						}
					}
					sl_uint32 ch;
					if (flagSz) {
						ch = Read32<endian>(utf32, i);
						if (!ch) {
							break;
						}
					} else {
						if (i < lenUtf32) {
							ch = Read32<endian>(utf32, i);
						} else {
							break;
						}
					}
					if (ch < 0x80) {
						if (utf8) {
							utf8[n++] = (sl_char8)(ch);
						} else {
							n++;
						}
					} else if (ch < 0x800) {
						if (lenUtf8Buffer < 0 || n + 1 < lenUtf8Buffer) {
							if (utf8) {
								utf8[n++] = (sl_char8)((ch >> 6) | 0xC0);
								utf8[n++] = (sl_char8)((ch & 0x3F) | 0x80);
							} else {
								n += 2;
							}
						}
					} else if (ch < 0x10000) {
						if (lenUtf8Buffer < 0 || n + 2 < lenUtf8Buffer) {
							if (utf8) {
								utf8[n++] = (sl_char8)((ch >> 12) | 0xE0);
								utf8[n++] = (sl_char8)(((ch >> 6) & 0x3F) | 0x80);
								utf8[n++] = (sl_char8)((ch & 0x3F) | 0x80);
							} else {
								n += 3;
							}
						}
					} else if (ch < 0x200000) {
						if (lenUtf8Buffer < 0 || n + 3 < lenUtf8Buffer) {
							if (utf8) {
								utf8[n++] = (sl_char8)((ch >> 18) | 0xF0);
								utf8[n++] = (sl_char8)(((ch >> 12) & 0x3F) | 0x80);
								utf8[n++] = (sl_char8)(((ch >> 6) & 0x3F) | 0x80);
								utf8[n++] = (sl_char8)((ch & 0x3F) | 0x80);
							} else {
								n += 4;
							}
						}
					} else if (ch < 0x4000000) {
						if (lenUtf8Buffer < 0 || n + 4 < lenUtf8Buffer) {
							if (utf8) {
								utf8[n++] = (sl_char8)((ch >> 24) | 0xF8);
								utf8[n++] = (sl_char8)(((ch >> 18) & 0x3F) | 0x80);
								utf8[n++] = (sl_char8)(((ch >> 12) & 0x3F) | 0x80);
								utf8[n++] = (sl_char8)(((ch >> 6) & 0x3F) | 0x80);
								utf8[n++] = (sl_char8)((ch & 0x3F) | 0x80);
							} else {
								n += 5;
							}
						}
					} else if (ch < 0x80000000) {
						if (lenUtf8Buffer < 0 || n + 5 < lenUtf8Buffer) {
							if (utf8) {
								utf8[n++] = (sl_char8)((ch >> 30) | 0xFC);
								utf8[n++] = (sl_char8)(((ch >> 24) & 0x3F) | 0x80);
								utf8[n++] = (sl_char8)(((ch >> 18) & 0x3F) | 0x80);
								utf8[n++] = (sl_char8)(((ch >> 12) & 0x3F) | 0x80);
								utf8[n++] = (sl_char8)(((ch >> 6) & 0x3F) | 0x80);
								utf8[n++] = (sl_char8)((ch & 0x3F) | 0x80);
							} else {
								n += 6;
							}
						}
					}
					i++;
				}
				return n;
			}

			static sl_size Utf32ToUtf8(EndianType endian, const void* utf32, sl_reg lenUtf32, sl_char8* utf8, sl_reg lenUtf8Buffer) noexcept
			{
				if (endian == EndianType::Big) {
					return ConvertUtf32ToUtf8<EndianType::Big>(utf32, lenUtf32, utf8, lenUtf8Buffer);
				} else {
					return ConvertUtf32ToUtf8<EndianType::Little>(utf32, lenUtf32, utf8, lenUtf8Buffer);
				}
			}

			template <EndianType endian16, EndianType endian32>
			sl_size ConvertUtf16ToUtf32(const void* utf16, sl_reg lenUtf16, void* utf32, sl_reg lenUtf32Buffer) noexcept
			{
				sl_bool flagSz = sl_false;
				if (lenUtf16 < 0) {
					flagSz = sl_true;
					lenUtf16 = SLIB_REG_MAX;
				}
				sl_reg n = 0;
				sl_reg i = 0;
				for (;;) {
					if (lenUtf32Buffer >= 0) {
						if (n >= lenUtf32Buffer) {
							break;
						}
					}
					sl_uint32 ch;
					if (flagSz) {
						ch = (sl_uint32)(Read16<endian16>(utf16, i));
						if (!ch) {
							break;
						}
					} else {
						if (i < lenUtf16) {
							ch = (sl_uint32)(Read16<endian16>(utf16, i));
						} else {
							break;
						}
					}
					if (ch < 0xD800 || ch >= 0xE000) {
						if (utf32) {
							Write32<endian32>(utf32, n++, (sl_char32)ch);
						} else {
							n++;
						}
					} else {
						if (i + 1 < lenUtf16) {
							sl_uint32 ch1 = (sl_uint32)(Read16<endian16>(utf16, ++i));
							if (ch < 0xDC00 && ch1 >= 0xDC00 && ch1 < 0xE000) {
								if (utf32) {
									Write32<endian32>(utf32, n++, (sl_char32)((((ch - 0xD800) << 10) | (ch1 - 0xDC00)) + 0x10000));
								} else {
									n++;
								}
							}
						}
					}
					i++;
				}
				return n;
			}

			static sl_size Utf16ToUtf32(EndianType endian16, const void* utf16, sl_reg lenUtf16, EndianType endian32, void* utf32, sl_reg lenUtf32Buffer) noexcept
			{
				if (endian16 == EndianType::Big) {
					if (endian32 == EndianType::Big) {
						return ConvertUtf16ToUtf32<EndianType::Big, EndianType::Big>(utf16, lenUtf16, utf32, lenUtf32Buffer);
					} else {
						return ConvertUtf16ToUtf32<EndianType::Big, EndianType::Little>(utf16, lenUtf16, utf32, lenUtf32Buffer);
					}
				} else {
					if (endian32 == EndianType::Big) {
						return ConvertUtf16ToUtf32<EndianType::Little, EndianType::Big>(utf16, lenUtf16, utf32, lenUtf32Buffer);
					} else {
						return ConvertUtf16ToUtf32<EndianType::Little, EndianType::Little>(utf16, lenUtf16, utf32, lenUtf32Buffer);
					}
				}
			}

			template <EndianType endian32, EndianType endian16>
			sl_size ConvertUtf32ToUtf16(const void* utf32, sl_reg lenUtf32, void* utf16, sl_reg lenUtf16Buffer) noexcept
			{
				sl_bool flagSz = sl_false;
				if (lenUtf32 < 0) {
					flagSz = sl_true;
					lenUtf32 = SLIB_REG_MAX;
				}
				sl_reg n = 0;
				sl_reg i = 0;
				for (;;) {
					if (lenUtf16Buffer >= 0) {
						if (n >= lenUtf16Buffer) {
							break;
						}
					}
					sl_uint32 ch;
					if (flagSz) {
						ch = Read32<endian32>(utf32, i);
						if (!ch) {
							break;
						}
					} else {
						if (i < lenUtf32) {
							ch = Read32<endian32>(utf32, i);
						} else {
							break;
						}
					}
					if (ch >= 0x10000) {
						if (ch < 0x110000) {
							if (lenUtf16Buffer < 0 || n + 1 < lenUtf16Buffer) {
								if (utf16) {
									ch -= 0x10000;
									Write16<endian16>(utf16, n++, (sl_char16)(0xD800 + (ch >> 10)));
									Write16<endian16>(utf16, n++, (sl_char16)(0xDC00 + (ch & 0x3FF)));
								} else {
									n += 2;
								}
							}
						}
					} else {
						if (ch < 0xD800 || ch >= 0xE000) {
							if (utf16) {
								Write16<endian16>(utf16, n++, (sl_char16)(ch));
							} else {
								n++;
							}
						}
					}
					i++;
				}
				return n;
			}

			static sl_size Utf32ToUtf16(EndianType endian32, const void* utf32, sl_reg lenUtf32, EndianType endian16, void* utf16, sl_reg lenUtf16Buffer) noexcept
			{
				if (endian32 == EndianType::Big) {
					if (endian16 == EndianType::Big) {
						return ConvertUtf32ToUtf16<EndianType::Big, EndianType::Big>(utf32, lenUtf32, utf16, lenUtf16Buffer);
					} else {
						return ConvertUtf32ToUtf16<EndianType::Big, EndianType::Little>(utf32, lenUtf32, utf16, lenUtf16Buffer);
					}
				} else {
					if (endian16 == EndianType::Big) {
						return ConvertUtf32ToUtf16<EndianType::Little, EndianType::Big>(utf32, lenUtf32, utf16, lenUtf16Buffer);
					} else {
						return ConvertUtf32ToUtf16<EndianType::Little, EndianType::Little>(utf32, lenUtf32, utf16, lenUtf16Buffer);
					}
				}
			}

		}
	}

	using namespace priv::charset;

	sl_size Charsets::utf8ToUtf16(const sl_char8* utf8, sl_reg lenUtf8, sl_char16* utf16, sl_reg lenUtf16Buffer) noexcept
	{
		return Utf8ToUtf16(utf8, lenUtf8, Endian::get(), utf16, lenUtf16Buffer);
	}

	sl_size Charsets::utf8ToUtf16(const sl_char8* utf8, sl_reg lenUtf8, EndianType endianUtf16, void* utf16, sl_reg sizeUtf16Buffer) noexcept
	{
		return Utf8ToUtf16(utf8, lenUtf8, endianUtf16, utf16, sizeUtf16Buffer < 0 ? -1 : (sizeUtf16Buffer >> 1)) << 1;
	}

	sl_size Charsets::utf8ToUtf32(const sl_char8* utf8, sl_reg lenUtf8, sl_char32* utf32, sl_reg lenUtf32Buffer) noexcept
	{
		return Utf8ToUtf32(utf8, lenUtf8, Endian::get(), utf32, lenUtf32Buffer);
	}

	sl_size Charsets::utf8ToUtf32(const sl_char8* utf8, sl_reg lenUtf8, EndianType endianUtf32, void* utf32, sl_reg sizeUtf32Buffer) noexcept
	{
		return Utf8ToUtf32(utf8, lenUtf8, endianUtf32, utf32, sizeUtf32Buffer < 0 ? -1 : (sizeUtf32Buffer >> 2)) << 2;
	}

	sl_size Charsets::utf16ToUtf8(const sl_char16* utf16, sl_reg lenUtf16, sl_char8* utf8, sl_reg lenUtf8Buffer) noexcept
	{
		return Utf16ToUtf8(Endian::get(), utf16, lenUtf16, utf8, lenUtf8Buffer);
	}

	sl_size Charsets::utf16ToUtf8(EndianType endianUtf16, const void* utf16, sl_size sizeUtf16, sl_char8* utf8, sl_reg lenUtf8Buffer) noexcept
	{
		return Utf16ToUtf8(endianUtf16, utf16, sizeUtf16 >> 1, utf8, lenUtf8Buffer);
	}

	sl_size Charsets::utf16ToUtf32(const sl_char16* utf16, sl_reg lenUtf16, sl_char32* utf32, sl_reg lenUtf32Buffer) noexcept
	{
		EndianType endian = Endian::get();
		return Utf16ToUtf32(endian, utf16, lenUtf16, endian, utf32, lenUtf32Buffer);
	}

	sl_size Charsets::utf16ToUtf32(const sl_char16* utf16, sl_reg lenUtf16, EndianType endianUtf32, void* utf32, sl_reg sizeUtf32Buffer) noexcept
	{
		return Utf16ToUtf32(Endian::get(), utf16, lenUtf16, endianUtf32, utf32, sizeUtf32Buffer < 0 ? -1 : (sizeUtf32Buffer >> 2)) << 2;
	}

	sl_size Charsets::utf16ToUtf32(EndianType endianUtf16, const void* utf16, sl_size sizeUtf16, sl_char32* utf32, sl_reg lenUtf32Buffer) noexcept
	{
		return Utf16ToUtf32(endianUtf16, utf16, sizeUtf16 >> 1, Endian::get(), utf32, lenUtf32Buffer);
	}

	sl_size Charsets::utf16ToUtf32(EndianType endianUtf16, const void* utf16, sl_size sizeUtf16, EndianType endianUtf32, void* utf32, sl_reg sizeUtf32Buffer) noexcept
	{
		return Utf16ToUtf32(endianUtf16, utf16, sizeUtf16 >> 1, endianUtf32, utf32, sizeUtf32Buffer < 0 ? -1 : (sizeUtf32Buffer >> 2)) << 2;
	}

	sl_size Charsets::utf32ToUtf8(const sl_char32* utf32, sl_reg lenUtf32, sl_char8* utf8, sl_reg lenUtf8Buffer) noexcept
	{
		return Utf32ToUtf8(Endian::get(), utf32, lenUtf32, utf8, lenUtf8Buffer);
	}

	sl_size Charsets::utf32ToUtf8(EndianType endianUtf32, const void* utf32, sl_size sizeUtf32, sl_char8* utf8, sl_reg lenUtf8Buffer) noexcept
	{
		return Utf32ToUtf8(endianUtf32, utf32, sizeUtf32 >> 2, utf8, lenUtf8Buffer);
	}

	sl_size Charsets::utf32ToUtf16(const sl_char32* utf32, sl_reg lenUtf32, sl_char16* utf16, sl_reg lenUtf16Buffer) noexcept
	{
		return Utf32ToUtf16(Endian::get(), utf32, lenUtf32, Endian::get(), utf16, lenUtf16Buffer);
	}

	sl_size Charsets::utf32ToUtf16(EndianType endianUtf32, const void* utf32, sl_size sizeUtf32, sl_char16* utf16, sl_reg lenUtf16Buffer) noexcept
	{
		return Utf32ToUtf16(endianUtf32, utf32, sizeUtf32 >> 2, Endian::get(), utf16, lenUtf16Buffer);
	}

	sl_size Charsets::utf32ToUtf16(const sl_char32* utf32, sl_reg lenUtf32, EndianType endianUtf16, void* utf16, sl_reg sizeUtf16Buffer) noexcept
	{
		return Utf32ToUtf16(Endian::get(), utf32, lenUtf32, endianUtf16, utf16, sizeUtf16Buffer < 0 ? -1 : (sizeUtf16Buffer >> 1)) << 1;
	}

	sl_size Charsets::utf32ToUtf16(EndianType endianUtf32, const void* utf32, sl_size sizeUtf32, EndianType endianUtf16, void* utf16, sl_reg sizeUtf16Buffer) noexcept
	{
		return Utf32ToUtf16(endianUtf32, utf32, sizeUtf32 >> 2, endianUtf16, utf16, sizeUtf16Buffer < 0 ? -1 : (sizeUtf16Buffer >> 1)) << 1;
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
				} else if (ch < 0xFC) {
					if (i + 4 < size) {
						sl_uint32 ch1 = (sl_uint32)((sl_uint8)buf[++i]);
						if ((ch1 & 0xC0) == 0x80) {
							sl_uint32 ch2 = (sl_uint32)((sl_uint8)buf[++i]);
							if ((ch2 & 0xC0) == 0x80) {
								sl_uint32 ch3 = (sl_uint32)((sl_uint8)buf[++i]);
								if ((ch3 & 0xC0) == 0x80) {
									sl_uint32 ch4 = (sl_uint32)((sl_uint8)buf[++i]);
									if ((ch4 & 0xC0) != 0x80) {
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
				} else {
					return sl_false;
				}
			}
		}
		return sl_true;
	}

}
