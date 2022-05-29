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
#include "slib/core/endian.h"
#include "slib/core/string.h"
#include "slib/core/memory.h"
#include "slib/core/scoped_buffer.h"

namespace slib
{
	
	namespace priv
	{
		namespace charset
		{

			sl_size Encode16(const sl_char16* utf16, sl_size lenUtf16, sl_uint32 codepage, void* _output, sl_reg sizeOutputBuffer);

			sl_size Decode16(sl_uint32 codepage, const void* _input, sl_size sizeInput, sl_char16* utf16, sl_reg lenUtf16Buffer);

#if defined(SLIB_PLATFORM_IS_APPLE)

			sl_size Encode8(const sl_char8* utf8, sl_size lenUtf8, sl_uint32 codepage, void* output, sl_reg sizeOutputBuffer);

			Memory EncodeString8(const sl_char8* utf8, sl_size lenUtf8, sl_uint32 codepage);

			sl_size Decode8(sl_uint32 codepage, const void* input, sl_size sizeInput, sl_char8* utf8, sl_reg lenUtf8Buffer);

			String DecodeString8(sl_uint32 codepage, const void* data, sl_size size);

			Memory EncodeString16(const sl_char16* utf16, sl_size lenUtf16, sl_uint32 codepage);

			String16 DecodeString16(sl_uint32 codepage, const void* data, sl_size size);

#else

			static Memory EncodeString16(const sl_char16* utf16, sl_size lenUtf16, sl_uint32 codepage)
			{
				sl_size len = Encode16(utf16, lenUtf16, codepage, sl_null, -1);
				if (len) {
					Memory mem = Memory::create(len);
					if (mem.isNotNull()) {
						Encode16(utf16, lenUtf16, codepage, mem.getData(), len);
						return mem;
					}
				}
				return sl_null;
			}

			static String16 DecodeString16(sl_uint32 codepage, const void* data, sl_size size)
			{
				String16 str = String16::allocate(size);
				if (str.isNotNull()) {
					sl_char16* s = str.getData();
					sl_size len = Decode16(codepage, data, size, s, size);
					if (len) {
						s[len] = 0;
						str.setLength(len);
						return str;
					}
				}
				return sl_null;
			}

			static sl_size Encode8(const sl_char8* utf8, sl_size lenUtf8, sl_uint32 codepage, void* output, sl_reg sizeOutputBuffer)
			{
				if (lenUtf8) {
					sl_size len = Charsets::utf8ToUtf16(utf8, lenUtf8, sl_null, -1);
					if (len) {
						SLIB_SCOPED_BUFFER(sl_char16, 1024, buf, len)
						if (buf) {
							Charsets::utf8ToUtf16(utf8, lenUtf8, buf, len);
							return Encode16(buf, len, codepage, output, sizeOutputBuffer);
						}
					}
				}
				return 0;
			}
			
			static sl_size Decode8(sl_uint32 codepage, const void* input, sl_size sizeInput, sl_char8* utf8, sl_reg lenUtf8Buffer)
			{
				if (sizeInput) {
					sl_size len = Decode16(codepage, input, sizeInput, sl_null, -1);
					if (len) {
						SLIB_SCOPED_BUFFER(sl_char16, 1024, buf, len)
						if (buf) {
							Decode16(codepage, input, sizeInput, buf, len);
							return Charsets::utf16ToUtf8(buf, len, utf8, lenUtf8Buffer);
						}
					}
				}
				return 0;
			}
			
			static Memory EncodeString8(const sl_char8* utf8, sl_size lenUtf8, sl_uint32 codepage)
			{
				String16 str = String16::create(utf8, lenUtf8);
				return EncodeString16(str.getData(), str.getLength(), codepage);
			}
			
			static String DecodeString8(sl_uint32 codepage, const void* data, sl_size size)
			{
				SLIB_SCOPED_BUFFER(sl_char16, 1024, buf, size)
				if (buf) {
					sl_size len = Decode16(codepage, data, size, buf, size);
					if (len) {
						return String::create(buf, len);
					}
				}
				return sl_null;
			}
			
#endif

			sl_size Encode32(const sl_char32* utf32, sl_size lenUtf32, sl_uint32 codepage, void* output, sl_reg sizeOutputBuffer)
			{
				if (lenUtf32) {
					sl_size len = Charsets::utf32ToUtf16(utf32, lenUtf32, sl_null, -1);
					if (len) {
						SLIB_SCOPED_BUFFER(sl_char16, 1024, buf, len)
						if (buf) {
							Charsets::utf32ToUtf16(utf32, lenUtf32, buf, len);
							return Encode16(buf, len, codepage, output, sizeOutputBuffer);
						}
					}
				}
				return 0;
			}

			sl_size Decode32(sl_uint32 codepage, const void* input, sl_size sizeInput, sl_char32* utf32, sl_reg lenUtf32Buffer)
			{
				if (sizeInput) {
					sl_size len = Decode16(codepage, input, sizeInput, sl_null, -1);
					if (len) {
						SLIB_SCOPED_BUFFER(sl_char16, 1024, buf, len)
						if (buf) {
							Decode16(codepage, input, sizeInput, buf, len);
							return Charsets::utf16ToUtf32(buf, len, utf32, lenUtf32Buffer);
						}
					}
				}
				return 0;
			}

			static Memory EncodeString32(const sl_char32* utf32, sl_size lenUtf32, sl_uint32 codepage)
			{
				String16 str = String16::create(utf32, lenUtf32);
				return EncodeString16(str.getData(), str.getLength(), codepage);
			}

			static String32 DecodeString32(sl_uint32 codepage, const void* data, sl_size size)
			{
				SLIB_SCOPED_BUFFER(sl_char16, 1024, buf, size)
				if (buf) {
					sl_size len = Decode16(codepage, data, size, buf, size);
					if (len) {
						return String32::create(buf, len);
					}
				}
				return sl_null;
			}

			static sl_size Utf8ToUtf8(const void* input, sl_size lenInput, void* output, sl_reg lenOutputBuffer) noexcept
			{
				sl_size n;
				if (lenOutputBuffer < 0) {
					n = lenInput;
				} else {
					n = lenOutputBuffer;
					if (lenInput < n) {
						n = lenInput;
					}
				}
				if (output) {
					Base::copyMemory(output, input, n);
				}
				return n;
			}

			static sl_size Utf16ToUtf16(EndianType endianInput, const void* input, sl_size lenInput, EndianType endianOutput, void* output, sl_reg lenOutputBuffer) noexcept
			{
				sl_size n;
				if (lenOutputBuffer < 0) {
					n = lenInput;
				} else {
					n = lenOutputBuffer;
					if (lenInput < n) {
						n = lenInput;
					}
				}
				if (output) {
					Charsets::utf16ToUtf16(endianInput, input, endianOutput, output, n);
				}
				return n;
			}

			static sl_size Utf32ToUtf32(EndianType endianInput, const void* input, sl_size lenInput, EndianType endianOutput, void* output, sl_reg lenOutputBuffer) noexcept
			{
				sl_size n;
				if (lenOutputBuffer < 0) {
					n = lenInput;
				} else {
					n = lenOutputBuffer;
					if (lenInput < n) {
						n = lenInput;
					}
				}
				if (output) {
					Charsets::utf32ToUtf32(endianInput, input, endianOutput, output, n);
				}
				return n;
			}

			static sl_uint32 ToWindowsCodepage(Charset charset) noexcept
			{
				return ((sl_uint32)charset) & 0xffff;
			}
			
		}
	}
	
	using namespace priv::charset;
	
	sl_size Charsets::encode8(const sl_char8* utf8, sl_size lenUtf8, Charset charset, void* output, sl_reg sizeOutputBuffer)
	{
		switch (charset) {
			case Charset::Unknown:
				return 0;
			case Charset::UTF8:
				return Utf8ToUtf8(utf8, lenUtf8, output, sizeOutputBuffer);
			case Charset::UTF16BE:
				return utf8ToUtf16(utf8, lenUtf8, Endian::Big, output, sizeOutputBuffer);
			case Charset::UTF16LE:
				return utf8ToUtf16(utf8, lenUtf8, Endian::Little, output, sizeOutputBuffer);
			case Charset::UTF32BE:
				return utf8ToUtf32(utf8, lenUtf8, Endian::Big, output, sizeOutputBuffer);
			case Charset::UTF32LE:
				return utf8ToUtf32(utf8, lenUtf8, Endian::Little, output, sizeOutputBuffer);
			default:
				return Encode8(utf8, lenUtf8, ToWindowsCodepage(charset), output, sizeOutputBuffer);
		}
	}
	
	sl_size Charsets::decode8(Charset charset, const void* input, sl_size sizeInput, sl_char8* utf8, sl_reg lenUtf8Buffer)
	{
		switch (charset) {
			case Charset::Unknown:
				return 0;
			case Charset::UTF8:
				return Utf8ToUtf8(input, sizeInput, utf8, lenUtf8Buffer);
			case Charset::UTF16BE:
				return utf16ToUtf8(Endian::Big, input, sizeInput, utf8, lenUtf8Buffer);
			case Charset::UTF16LE:
				return utf16ToUtf8(Endian::Little, input, sizeInput, utf8, lenUtf8Buffer);
			case Charset::UTF32BE:
				return utf32ToUtf8(Endian::Big, input, sizeInput, utf8, lenUtf8Buffer);
			case Charset::UTF32LE:
				return utf32ToUtf8(Endian::Little, input, sizeInput, utf8, lenUtf8Buffer);
			default:
				return Decode8(ToWindowsCodepage(charset), input, sizeInput, utf8, lenUtf8Buffer);
		}
	}
	
	sl_size Charsets::encode16(const sl_char16* utf16, sl_size lenUtf16, Charset charset, void* output, sl_reg sizeOutputBuffer)
	{
		switch (charset) {
			case Charset::Unknown:
				return 0;
			case Charset::UTF8:
				return utf16ToUtf8(utf16, lenUtf16, (sl_char8*)output, sizeOutputBuffer);
			case Charset::UTF16BE:
				return Utf16ToUtf16(Endian::get(), utf16, lenUtf16, Endian::Big, output, sizeOutputBuffer < 0 ? -1 : (((sl_size)sizeOutputBuffer) >> 1)) << 1;
			case Charset::UTF16LE:
				return Utf16ToUtf16(Endian::get(), utf16, lenUtf16, Endian::Little, output, sizeOutputBuffer < 0 ? -1 : (((sl_size)sizeOutputBuffer) >> 1)) << 1;
			case Charset::UTF32BE:
				return utf16ToUtf32(utf16, lenUtf16, Endian::Big, output, sizeOutputBuffer);
			case Charset::UTF32LE:
				return utf16ToUtf32(utf16, lenUtf16, Endian::Little, output, sizeOutputBuffer);
			default:
				return Encode16(utf16, lenUtf16, ToWindowsCodepage(charset), output, sizeOutputBuffer);
		}
	}
	
	sl_size Charsets::decode16(Charset charset, const void* input, sl_size sizeInput, sl_char16* utf16, sl_reg lenUtf16Buffer)
	{
		switch (charset) {
			case Charset::Unknown:
				return 0;
			case Charset::UTF8:
				return utf8ToUtf16((sl_char8*)input, sizeInput, utf16, lenUtf16Buffer);
			case Charset::UTF16BE:
				return Utf16ToUtf16(Endian::Big, input, sizeInput >> 1, Endian::get(), utf16, lenUtf16Buffer);
			case Charset::UTF16LE:
				return Utf16ToUtf16(Endian::Little, input, sizeInput >> 1, Endian::get(), utf16, lenUtf16Buffer);
			case Charset::UTF32BE:
				return utf32ToUtf16(Endian::Big, input, sizeInput, utf16, lenUtf16Buffer);
			case Charset::UTF32LE:
				return utf32ToUtf16(Endian::Little, input, sizeInput, utf16, lenUtf16Buffer);
			default:
				return Decode16(ToWindowsCodepage(charset), input, sizeInput, utf16, lenUtf16Buffer);
		}
	}

	sl_size Charsets::encode32(const sl_char32* utf32, sl_size lenUtf32, Charset charset, void* output, sl_reg sizeOutputBuffer)
	{
		switch (charset) {
		case Charset::Unknown:
			return 0;
		case Charset::UTF8:
			return utf32ToUtf8(utf32, lenUtf32, (sl_char8*)output, sizeOutputBuffer);
		case Charset::UTF16BE:
			return utf32ToUtf16(utf32, lenUtf32, Endian::Big, output, sizeOutputBuffer);
		case Charset::UTF16LE:
			return utf32ToUtf16(utf32, lenUtf32, Endian::Little, output, sizeOutputBuffer);
		case Charset::UTF32BE:
			return Utf32ToUtf32(Endian::get(), utf32, lenUtf32, Endian::Big, output, sizeOutputBuffer < 0 ? -1 : (((sl_size)sizeOutputBuffer) >> 2)) << 2;
		case Charset::UTF32LE:
			return Utf32ToUtf32(Endian::get(), utf32, lenUtf32, Endian::Little, output, sizeOutputBuffer < 0 ? -1 : (((sl_size)sizeOutputBuffer) >> 2)) << 2;
		default:
			return Encode32(utf32, lenUtf32, ToWindowsCodepage(charset), output, sizeOutputBuffer);
		}
	}

	sl_size Charsets::decode32(Charset charset, const void* input, sl_size sizeInput, sl_char32* utf32, sl_reg lenUtf32Buffer)
	{
		switch (charset) {
		case Charset::Unknown:
			return 0;
		case Charset::UTF8:
			return utf8ToUtf32((sl_char8*)input, sizeInput, utf32, lenUtf32Buffer);
		case Charset::UTF16BE:
			return utf16ToUtf32(Endian::Big, input, sizeInput, utf32, lenUtf32Buffer);
		case Charset::UTF16LE:
			return utf16ToUtf32(Endian::Little, input, sizeInput, utf32, lenUtf32Buffer);
		case Charset::UTF32BE:
			return Utf32ToUtf32(Endian::Big, input, sizeInput >> 2, Endian::get(), utf32, lenUtf32Buffer);
		case Charset::UTF32LE:
			return Utf32ToUtf32(Endian::Little, input, sizeInput >> 2, Endian::get(), utf32, lenUtf32Buffer);
		default:
			return Decode32(ToWindowsCodepage(charset), input, sizeInput, utf32, lenUtf32Buffer);
		}
	}

	Memory Charsets::encode8(const sl_char8* src, sl_size len, Charset charset)
	{
		if (src && len) {
			switch (charset) {
				case Charset::Unknown:
				case Charset::UTF8:
				case Charset::UTF16BE:
				case Charset::UTF16LE:
				case Charset::UTF32BE:
				case Charset::UTF32LE:
					{
						sl_size size = encode8(src, len, charset, sl_null, -1);
						if (size) {
							Memory mem = Memory::create(size);
							if (mem.isNotNull()) {
								encode8(src, len, charset, mem.getData(), size);
								return mem;
							}
						}
						break;
					}
				default:
					{
						sl_uint32 codepage = ToWindowsCodepage(charset);
						return EncodeString8(src, len, codepage);
					}
			}
		}
		return sl_null;
	}

	String Charsets::decode8(Charset charset, const void* text, sl_size size)
	{
		if (size) {
			switch (charset) {
				case Charset::Unknown:
				case Charset::UTF8:
				case Charset::UTF16BE:
				case Charset::UTF16LE:
				case Charset::UTF32BE:
				case Charset::UTF32LE:
					{
						sl_size len = Charsets::decode8(charset, text, size, sl_null, -1);
						if (len) {
							String str = String::allocate(len);
							if (str.isNotNull()) {
								Charsets::decode8(charset, text, size, str.getData(), len);
								return str;
							}
						}
						break;
					}
				default:
					{
						sl_uint32 codepage = ToWindowsCodepage(charset);
						return DecodeString8(codepage, text, size);
					}
			}
		}
		return sl_null;
	}

	Memory Charsets::encode16(const sl_char16* src, sl_size len, Charset charset)
	{
		if (src && len) {
			switch (charset) {
				case Charset::Unknown:
				case Charset::UTF8:
				case Charset::UTF16BE:
				case Charset::UTF16LE:
				case Charset::UTF32BE:
				case Charset::UTF32LE:
					{
						sl_size size = Charsets::encode16(src, len, charset, sl_null, -1);
						if (size) {
							Memory mem = Memory::create(size);
							if (mem.isNotNull()) {
								Charsets::encode16(src, len, charset, mem.getData(), size);
								return mem;
							}
						}
						break;
					}
				default:
					{
						sl_uint32 codepage = ToWindowsCodepage(charset);
						return EncodeString16(src, len, codepage);
					}
			}
		}
		return sl_null;
	}

	String16 Charsets::decode16(Charset charset, const void* text, sl_size size)
	{
		if (size) {
			switch (charset) {
				case Charset::Unknown:
				case Charset::UTF8:
				case Charset::UTF16BE:
				case Charset::UTF16LE:
				case Charset::UTF32BE:
				case Charset::UTF32LE:
					{
						sl_size len = Charsets::decode16(charset, text, size, sl_null, -1);
						if (len) {
							String16 str = String16::allocate(len);
							if (str.isNotNull()) {
								Charsets::decode16(charset, text, size, str.getData(), len);
								return str;
							}
						}
						break;
					}
				default:
					{
						sl_uint32 codepage = ToWindowsCodepage(charset);
						return DecodeString16(codepage, text, size);
					}
			}
		}
		return sl_null;
	}

	Memory Charsets::encode32(const sl_char32* src, sl_size len, Charset charset)
	{
		if (src && len) {
			switch (charset) {
			case Charset::Unknown:
			case Charset::UTF8:
			case Charset::UTF16BE:
			case Charset::UTF16LE:
			case Charset::UTF32BE:
			case Charset::UTF32LE:
				{
					sl_size size = Charsets::encode32(src, len, charset, sl_null, -1);
					if (size) {
						Memory mem = Memory::create(size);
						if (mem.isNotNull()) {
							Charsets::encode32(src, len, charset, mem.getData(), size);
							return mem;
						}
					}
					break;
				}
			default:
				{
					sl_uint32 codepage = ToWindowsCodepage(charset);
					return EncodeString32(src, len, codepage);
				}
			}
		}
		return sl_null;
	}

	String32 Charsets::decode32(Charset charset, const void* text, sl_size size)
	{
		if (size) {
			switch (charset) {
			case Charset::Unknown:
			case Charset::UTF8:
			case Charset::UTF16BE:
			case Charset::UTF16LE:
			case Charset::UTF32BE:
			case Charset::UTF32LE:
				{
					sl_size len = Charsets::decode32(charset, text, size, sl_null, -1);
					if (len) {
						String32 str = String32::allocate(len);
						if (str.isNotNull()) {
							Charsets::decode32(charset, text, size, str.getData(), len);
							return str;
						}
					}
					break;
				}
			default:
				{
					sl_uint32 codepage = ToWindowsCodepage(charset);
					return DecodeString32(codepage, text, size);
				}
			}
		}
		return sl_null;
	}

	String String::decode(Charset charset, const void* text, sl_size size)
	{
		return Charsets::decode8(charset, text, size);
	}
	
	String String::decode(Charset charset, const Memory& mem)
	{
		return Charsets::decode8(charset, mem.getData(), mem.getSize());
	}
	
	String16 String16::decode(Charset charset, const void* text, sl_size size)
	{
		return Charsets::decode16(charset, text, size);
	}
	
	String16 String16::decode(Charset charset, const Memory& mem)
	{
		return Charsets::decode16(charset, mem.getData(), mem.getSize());
	}

	String32 String32::decode(Charset charset, const void* text, sl_size size)
	{
		return Charsets::decode32(charset, text, size);
	}

	String32 String32::decode(Charset charset, const Memory& mem)
	{
		return Charsets::decode32(charset, mem.getData(), mem.getSize());
	}

	Memory String::encode(Charset charset) const
	{
		sl_char8* src = getData();
		sl_size len = getLength();
		return Charsets::encode8(src, len, charset);
	}
	
	Memory String16::encode(Charset charset) const
	{
		sl_char16* src = getData();
		sl_size len = getLength();
		return Charsets::encode16(src, len, charset);
	}
	
	Memory String32::encode(Charset charset) const
	{
		sl_char32* src = getData();
		sl_size len = getLength();
		return Charsets::encode32(src, len, charset);
	}

}
