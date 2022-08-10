/*
 *   Copyright (c) 2008-2018 SLIBIO <https://github.com/SLIBIO>
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

#include "slib/crypto/base64.h"

#include "slib/core/memory.h"

#define BASE64_CHARS "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"
#define BASE64_CHARS_URL "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_"

namespace slib
{
	
	namespace priv
	{
		namespace base64
		{
			
			template <class CHAR>
			static typename StringTypeFromCharType<CHAR>::Type Encode(const char* patterns, const void* buf, sl_size size, CHAR padding)
			{
				typedef typename StringTypeFromCharType<CHAR>::Type STRING;
				if (!size) {
					return sl_null;
				}
				const sl_uint8* input = (const sl_uint8*)buf;
				sl_uint32 last = (sl_uint32)(size % 3);
				sl_size countBlock = (size / 3) + (last ? 1 : 0);
				STRING ret = STRING::allocate(countBlock << 2);
				if (ret.isEmpty()) {
					return ret;
				}
				CHAR* _output = ret.getData();
				CHAR* output = _output;
				sl_uint32 n = 0;
				for (sl_size iBlock = 0; iBlock < countBlock; iBlock++) {
					sl_uint8 n0 = input[0];
					sl_uint8 n1 = (n + 1 < size) ? input[1] : 0;
					sl_uint8 n2 = (n + 2 < size) ? input[2] : 0;
					output[0] = patterns[n0 >> 2];
					output[1] = patterns[((n0 & 0x03) << 4) + ((n1 & 0xF0) >> 4)];
					output[2] = patterns[((n1 & 0x0F) << 2) + ((n2 & 0xC0) >> 6)];
					output[3] = patterns[n2 & 0x3F];
					input += 3;
					n += 3;
					output += 4;
				}
				if (padding) {
					if (last == 1) {
						*(output - 2) = padding;
					}
					if (last >= 1) {
						*(output - 1) = padding;
					}
				} else {
					if (last == 1) {
						sl_size nLen = (countBlock << 2) - 2;
						_output[nLen] = 0;
						ret.setLength(nLen);
					} else if (last > 1) {
						sl_size nLen = (countBlock << 2) - 1;
						_output[nLen] = 0;
						ret.setLength(nLen);
					}
				}
				return ret;
			}
			
			static sl_uint32 GetIndex(sl_uint32 c)
			{
				if (c >= 'A' && c <= 'Z') {
					return c - 'A';
				}
				if (c >= 'a' && c <= 'z') {
					return 26 + (c - 'a');
				}
				if (c >= '0' && c <= '9') {
					return 52 + (c - '0');
				}
				if (c == '+') {
					return 62;
				}
				if (c == '/') {
					return 63;
				}
				if (c == '-') {
					return 62;
				}
				if (c == '_') {
					return 63;
				}
				return 64;
			}
			
			template <class CHAR>
			static sl_size Decode(const CHAR* input, sl_size len, void* buf, CHAR padding)
			{
				sl_uint8* output = (sl_uint8*)buf;
				// trim right (CR, LF)
				while (len > 0) {
					CHAR ch = input[len - 1];
					if (SLIB_CHAR_IS_WHITE_SPACE(ch)) {
						len--;
					}
					else {
						break;
					}
				}
				sl_uint32 data[4];
				sl_size indexInput = 0;
				sl_size indexOutput = 0;
				sl_uint32 posInBlock = 0;
				while (indexInput < len) {
					CHAR ch = input[indexInput];
					if (SLIB_CHAR_IS_WHITE_SPACE(ch) || ch == padding) {
						indexInput++;
						continue;
					}
					sl_uint32 sig = GetIndex(ch);
					if (sig >= 64) {
						return 0;
					}
					data[posInBlock] = sig;
					switch (posInBlock) {
					case 0:
						++posInBlock;
						break;
					case 1:
						output[indexOutput] = (sl_uint8)((data[0] << 2) + ((data[1] & 0x30) >> 4));
						++indexOutput;
						++posInBlock;
						break;
					case 2:
						output[indexOutput] = (sl_uint8)(((data[1] & 0xf) << 4) + ((data[2] & 0x3c) >> 2));
						++indexOutput;
						++posInBlock;
						break;
					case 3:
						output[indexOutput] = (sl_uint8)(((data[2] & 0x3) << 6) + data[3]);
						++indexOutput;
						posInBlock = 0;
						break;
					}
					indexInput++;
				}
				return indexOutput;
			}

			template <class CHAR>
			static Memory Decode(const CHAR* input, sl_size len, CHAR padding)
			{
				sl_size size = Base64::getDecodeOutputSize(len);
				Memory mem = Memory::create(size);
				if (mem.isNull()) {
					return sl_null;
				}
				sl_size sizeOutput = Decode(input, len, mem.getData(), padding);
				if (sizeOutput) {
					if (size == sizeOutput) {
						return mem;
					}
					else {
						return mem.sub(0, sizeOutput);
					}
				}
				return sl_null;
			}

		}
	}
	
	using namespace priv::base64;
	
	String Base64::encode(const void* buf, sl_size size, sl_char8 padding)
	{
		return Encode(BASE64_CHARS, buf, size, padding);
	}

	String Base64::encodeUrl(const void* buf, sl_size size, sl_char8 padding)
	{
		return Encode(BASE64_CHARS_URL, buf, size, padding);
	}

	String16 Base64::encode16(const void* buf, sl_size size, sl_char16 padding)
	{
		return Encode(BASE64_CHARS, buf, size, padding);
	}

	String16 Base64::encodeUrl16(const void* buf, sl_size size, sl_char16 padding)
	{
		return Encode(BASE64_CHARS_URL, buf, size, padding);
	}

	String32 Base64::encode32(const void* buf, sl_size size, sl_char32 padding)
	{
		return Encode(BASE64_CHARS, buf, size, padding);
	}

	String32 Base64::encodeUrl32(const void* buf, sl_size size, sl_char32 padding)
	{
		return Encode(BASE64_CHARS_URL, buf, size, padding);
	}

	String Base64::encode(const MemoryView& mem, sl_char8 padding)
	{
		return Encode(BASE64_CHARS, mem.data, mem.size, padding);
	}

	String Base64::encodeUrl(const MemoryView& mem, sl_char8 padding)
	{
		return Encode(BASE64_CHARS_URL, mem.data, mem.size, padding);
	}

	String16 Base64::encode16(const MemoryView& mem, sl_char16 padding)
	{
		return Encode(BASE64_CHARS, mem.data, mem.size, padding);
	}

	String16 Base64::encodeUrl16(const MemoryView& mem, sl_char16 padding)
	{
		return Encode(BASE64_CHARS_URL, mem.data, mem.size, padding);
	}

	String32 Base64::encode32(const MemoryView& mem, sl_char32 padding)
	{
		return Encode(BASE64_CHARS, mem.data, mem.size, padding);
	}

	String32 Base64::encodeUrl32(const MemoryView& mem, sl_char32 padding)
	{
		return Encode(BASE64_CHARS_URL, mem.data, mem.size, padding);
	}

	String Base64::encode(const StringView& str, sl_char8 padding)
	{
		return Encode(BASE64_CHARS, str.getData(), str.getLength(), padding);
	}

	String Base64::encodeUrl(const StringView& str, sl_char8 padding)
	{
		return Encode(BASE64_CHARS_URL, str.getData(), str.getLength(), padding);
	}
	
	sl_size Base64::getDecodeOutputSize(sl_size len)
	{
		sl_size size = (len >> 2) * 3;
		if ((len & 3) == 2) {
			size++;
		} else if ((len & 3) == 3) {
			size += 2;
		}
		return size;
	}

	sl_size Base64::decode(const StringParam& _str, void* buf, sl_char32 padding)
	{
		if (_str.is8BitsStringType()) {
			StringData str(_str);
			return Decode(str.getData(), str.getLength(), buf, (sl_char8)padding);
		} else if (_str.is16BitsStringType()) {
			StringData16 str(_str);
			return Decode(str.getData(), str.getLength(), buf, (sl_char16)padding);
		} else {
			StringData32 str(_str);
			return Decode(str.getData(), str.getLength(), buf, (sl_char32)padding);
		}
	}

	Memory Base64::decode(const StringParam& _str, sl_char32 padding)
	{
		if (_str.is8BitsStringType()) {
			StringData str(_str);
			return Decode(str.getData(), str.getLength(), (sl_char8)padding);
		}
		else if (_str.is16BitsStringType()) {
			StringData16 str(_str);
			return Decode(str.getData(), str.getLength(), (sl_char16)padding);
		}
		else {
			StringData32 str(_str);
			return Decode(str.getData(), str.getLength(), (sl_char32)padding);
		}
	}

}
