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

#include "slib/doc/pdf.h"

#include "slib/core/file.h"
#include "slib/core/buffered_seekable_reader.h"
#include "slib/core/thread.h"
#include "slib/core/variant.h"
#include "slib/core/shared.h"
#include "slib/core/mio.h"
#include "slib/crypto/zlib.h"

#define MAX_PDF_FILE_SIZE 0x40000000
#define MAX_WORD_LENGTH 256
#define MAX_NAME_LENGTH 127
#define MAX_STRING_LENGTH 32767

#define MAKE_OBJECT_ID(NUM, GEN) SLIB_MAKE_QWORD4(GEN, NUM)

namespace slib
{

	namespace priv
	{
		namespace pdf
		{
			/*
				W - whitespace: NUL, TAB, CR, LF, FF, SPACE, 0x80, 0xff
				N - numeric: 0123456789+-.
				D - delimiter: %()/<>[]{}
				R - otherwise.
			*/
			static const char g_charType[] = {
				// NUL  SOH  STX  ETX  EOT  ENQ  ACK  BEL  BS   HT   LF   VT   FF   CR   SO   SI
				   'W', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'W', 'W', 'R', 'W', 'W', 'R', 'R',
				// DLE  DC1  DC2  DC3  DC4  NAK  SYN  ETB  CAN   EM  SUB  ESC  FS   GS   RS   US
				   'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R',
				// SP    !    "    #    $    %    &    ´    (    )    *    +    ,    -    .    /
				   'W', 'R', 'R', 'R', 'R', 'D', 'R', 'R', 'D', 'D', 'R', 'N', 'R', 'N', 'N', 'D',
				//  0    1    2    3    4    5    6    7    8    9    :    ;    <    =    >    ?
				   'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'R', 'R', 'D', 'R', 'D', 'R',
				//  @    A    B    C    D    E    F    G    H    I    J    K    L    M    N    O
				   'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R',
				//  P    Q    R    S    T    U    V    W    X    Y    Z    [    \    ]    ^    _
				   'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'D', 'R', 'D', 'R', 'R',
				//  `    a    b    c    d    e    f    g    h    i    j    k    l    m    n    o
				   'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R',
				//  p    q    r    s    t    u    v    w    x    y    z    {    |    }    ~   DEL
				   'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'D', 'R', 'D', 'R', 'R',
				   'W', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R',
				   'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R',
				   'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R',
				   'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R',
				   'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R',
				   'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R',
				   'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R',
				   'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'W'
			};


			SLIB_INLINE static sl_bool IsWhitespace(sl_uint8 c)
			{
				return g_charType[c] == 'W';
			}

			SLIB_INLINE static sl_bool IsDelimiter(sl_uint8 c)
			{
				return g_charType[c] == 'D';
			}

			SLIB_INLINE static sl_bool IsNumeric(sl_uint8 c)
			{
				return g_charType[c] == 'N';
			}

			SLIB_INLINE static sl_bool IsLineEnding(sl_uint8 c)
			{
				return c == '\r' || c == '\n';
			}

			static sl_bool EqualsName(const Variant& var, const StringView& name)
			{
				if (var.getTag() == 1 && var.isStringType()) {
					return var.getStringView() == name;
				}
				return sl_false;
			}

			static String GetName(const Variant& var)
			{
				if (var.getTag() == 1 && var.isStringType()) {
					return var.getString();
				}
				return sl_null;
			}

			static Memory ApplyFilter(const Memory& input, const StringView& filter)
			{
				if (filter == StringView::literal("FlateDecode")) {
					return Zlib::decompress(input.getData(), input.getSize());
				} else if (filter == StringView::literal("ASCIIHexDecode")) {
					sl_size len = input.getSize();
					Memory ret = Memory::create((len + 1) >> 1);
					if (ret.isNotNull()) {
						sl_char8* str = (sl_char8*)(input.getData());
						sl_uint8* dst = (sl_uint8*)(ret.getData());
						sl_uint8* cur = dst;
						sl_bool flagFirstHex = sl_true;
						sl_uint8 firstHex = 0;
						for (sl_size i = 0; i < len; i++) {
							sl_char8 ch = str[i];
							sl_uint8 h = SLIB_CHAR_HEX_TO_INT(ch);
							if (h < 16) {
								if (flagFirstHex) {
									firstHex = h;
									flagFirstHex = sl_false;
								} else {
									*(cur++) = (firstHex << 4) | h;
									flagFirstHex = sl_true;
								}
							} else if (!(IsWhitespace(ch))) {
								return sl_null;
							}
						}
						if (!flagFirstHex) {
							*(cur++) = firstHex << 4;
						}
						return ret.sub(0, cur - dst);
					}
				} else if (filter == StringView::literal("ASCII85Decode")) {
					sl_size len = input.getSize();
					CList<sl_uint8> list;
					if (list.setCapacity(((len + 4) / 5) << 2)) {
						sl_char8* str = (sl_char8*)(input.getData());
						sl_uint32 indexElement = 0;
						sl_uint32 dword = 0;
						for (sl_size i = 0; i < len; i++) {
							sl_uint8 v = str[i];
							if (v == 'z') {
								if (indexElement) {
									return sl_null;
								} else {
									list.addElements_NoLock(4, 0);
								}
							} else {
								if (v >= '!' && v <= 'u') {
									v -= '!';
									dword = dword * 85 + v;
									indexElement++;
									if (indexElement >= 5) {
										sl_uint8 bytes[4];
										MIO::writeUint32BE(bytes, dword);
										list.addElements_NoLock(bytes, 4);
										indexElement = 0;
										dword = 0;
									}
								} else if (v == '~') {
									if (i + 2 == len) {
										if (str[i + 1] == '>') {
											if (indexElement == 1) {
												return sl_null;
											}
											if (indexElement) {
												for (sl_uint32 i = 0; i < indexElement; i++) {
													dword *= 85;
												}
												sl_uint8 bytes[4];
												MIO::writeUint32BE(bytes, dword);
												list.addElements_NoLock(bytes, indexElement - 1);
											}
											return Memory::create(list.getData(), list.getCount());
										}
									}
									return sl_null;
								} else if (!(IsWhitespace(v))) {
									return sl_null;
								}
							}
						}
					}
				}
				return sl_null;
			}


			class SLIB_EXPORT BufferedParserBase
			{
			public:
				BufferedSeekableReader reader;

			public:
				sl_bool readChar(sl_char8& ch)
				{
					return reader.readInt8((sl_int8*)((void*)&ch));
				}

				sl_bool peekChar(sl_char8& ch)
				{
					return reader.peekInt8((sl_int8*)((void*)&ch));
				}

				sl_reg read(void* buf, sl_size size)
				{
					return reader.readFully(buf, size);
				}

				sl_size getPosition()
				{
					return (sl_size)(reader.getPosition());
				}

				sl_bool setPosition(sl_size pos)
				{
					return reader.seek(pos, SeekPosition::Begin);
				}

				sl_bool movePosition(sl_reg offset)
				{
					return reader.seek(offset, SeekPosition::Current);
				}

				sl_reg readBuffer(sl_char8*& buf)
				{
					for (;;) {
						sl_reg n = reader.read(*((void**)&buf));
						if (n == SLIB_IO_WOULD_BLOCK) {
							if (Thread::isStoppingCurrent()) {
								break;
							}
							reader.waitRead();
						}
						return n;
					}
					return SLIB_IO_ERROR;
				}

				sl_reg findBackward(const StringView& str, sl_size sizeFind)
				{
					return (sl_reg)(reader.findBackward(str.getData(), str.getLength(), -1, sizeFind));
				}

			};

			class SLIB_EXPORT MemoryParserBase
			{
			public:
				sl_char8* buf;
				sl_uint32 size;
				Ref<Referable> ref;

			public:
				sl_bool readChar(sl_char8& ch)
				{
					if (pos < size) {
						ch = buf[pos++];
						return sl_true;
					} else {
						return sl_false;
					}
				}

				sl_bool peekChar(sl_char8& ch)
				{
					if (pos < size) {
						ch = buf[pos];
						return sl_true;
					}
					return sl_false;
				}

				sl_reg read(void* _buf, sl_size size)
				{
					if (pos < size) {
						Base::copyMemory(_buf, buf, size);
						return size - pos;
					}
					return sl_false;
				}

				sl_size getPosition()
				{
					return pos;
				}

				sl_bool setPosition(sl_uint32 _pos)
				{
					if (pos <= _pos) {
						pos = _pos;
						return sl_true;
					}
					return sl_false;
				}

				sl_bool movePosition(sl_int32 offset)
				{
					return setPosition(pos + offset);
				}

				sl_reg readBuffer(sl_char8*& _out)
				{
					if (pos < size) {
						_out = buf + pos;
						return size - pos;
					}
					return SLIB_IO_ERROR;
				}

				sl_reg findBackward(const StringView& str, sl_size sizeFind)
				{
					sl_char8* p;
					if (size > sizeFind) {
						p = (sl_char8*)(Base::findMemoryBackward(buf + size - sizeFind, sizeFind, str.getData(), str.getLength()));
					} else {
						p = (sl_char8*)(Base::findMemoryBackward(buf, size, str.getData(), str.getLength()));
					}
					if (p) {
						return p - buf;
					} else {
						return -1;
					}
				}

			private:
				sl_uint32 pos = 0;

			};

			template <class BASE>
			class SLIB_EXPORT Parser : public BASE
			{
			public:
				HashMap<sl_uint64, sl_uint32> objectOffsets;

			public:
				sl_bool readWord(String& _out)
				{
					sl_char8 word[MAX_WORD_LENGTH];
					sl_size lenWord = 0;
					for (;;) {
						sl_char8* buf;
						sl_reg n = readBuffer(buf);
						if (n > 0) {
							for (sl_reg i = 0; i < n; i++) {
								sl_char8 ch = buf[i];
								if (IsWhitespace(ch) || IsDelimiter(ch)) {
									movePosition(i - n);
									_out = String(word, lenWord);
									return sl_true;
								} else {
									if (lenWord >= sizeof(word)) {
										return sl_false;
									}
									word[lenWord++] = ch;
								}
							}
						} else if (n == SLIB_IO_ENDED) {
							_out = String(word, lenWord);
							return sl_true;
						} else {
							break;
						}
					}
					return sl_false;
				}

				sl_bool readWordAndEquals(const StringView& _word)
				{
					sl_size lenWord = _word.getLength();
					if (!lenWord) {
						return sl_false;
					}
					const sl_char8* word = _word.getData();
					sl_size posWord = 0;
					for (;;) {
						sl_char8* buf;
						sl_reg n = readBuffer(buf);
						if (n > 0) {
							for (sl_reg i = 0; i < n; i++) {
								sl_char8 ch = buf[i];
								if (IsWhitespace(ch) || IsDelimiter(ch)) {
									movePosition(i - n);
									return posWord == lenWord;
								} else {
									if (posWord >= lenWord) {
										return sl_false;
									}
									if (word[posWord++] != ch) {
										return sl_false;
									}
								}
							}
						} else if (n == SLIB_IO_ENDED) {
							return posWord == lenWord;
						} else {
							break;
						}
					}
					return sl_false;
				}

				sl_bool skipWhitespaces()
				{
					sl_bool flagComment = sl_false;
					for (;;) {
						sl_char8* buf;
						sl_reg n = readBuffer(buf);
						if (n > 0) {
							for (sl_reg i = 0; i < n; i++) {
								sl_char8 ch = buf[i];
								if (flagComment) {
									if (IsLineEnding(ch)) {
										flagComment = sl_false;
									}
								} else if (ch == '%') {
									flagComment = sl_true;
								} else if (!(IsWhitespace(ch))) {
									movePosition(i - n);
									return sl_true;
								}
							}
						} else if (n == SLIB_IO_ENDED) {
							return sl_true;
						} else {
							break;
						}
					}
					return sl_false;
				}

				sl_bool readName(String& _out)
				{
					sl_char8 name[MAX_WORD_LENGTH];
					sl_size lenName = 0;
					sl_char8 ch;
					if (!(readChar(ch))) {
						return sl_false;
					}
					if (ch != '/') {
						return sl_false;
					}
					if (!(skipWhitespaces())) {
						return sl_false;
					}
					sl_bool flagReadHex = sl_false;
					sl_uint32 hex = 0;
					sl_uint32 posHex = 0;
					for (;;) {
						sl_char8* buf;
						sl_reg n = readBuffer(buf);
						if (n > 0) {
							for (sl_reg i = 0; i < n; i++) {
								ch = buf[i];
								if (flagReadHex) {
									sl_uint32 h = SLIB_CHAR_HEX_TO_INT(ch);
									if (h >= 16) {
										return sl_false;
									}
									hex = (hex << 4) | h;
									posHex++;
									if (posHex >= 2) {
										if (lenName >= sizeof(name)) {
											return sl_false;
										}
										name[lenName++] = (sl_char8)hex;;
										flagReadHex = sl_false;
									}
								} else {
									if (IsWhitespace(ch) || IsDelimiter(ch)) {
										movePosition(i - n);
										_out = String(name, lenName);
										return sl_true;
									} else if (ch == '#') {
										flagReadHex = sl_true;
										hex = 0;
										posHex = 0;
									} else {
										if (lenName >= sizeof(name)) {
											return sl_false;
										}
										name[lenName++] = ch;
									}
								}
							}
						} else if (n == SLIB_IO_ENDED) {
							if (flagReadHex) {
								return sl_false;
							}
							_out = String(name, lenName);
							return sl_true;
						} else {
							break;
						}
					}
					return sl_false;
				}

				sl_bool readUint(sl_uint32& outValue, sl_bool flagAllowEmpty = sl_false)
				{
					outValue = 0;
					sl_uint32 nDigits = 0;
					for (;;) {
						sl_char8* buf;
						sl_reg n = readBuffer(buf);
						if (n > 0) {
							for (sl_reg i = 0; i < n; i++) {
								sl_char8 ch = buf[i];
								if (IsWhitespace(ch) || IsDelimiter(ch) || ch == '.') {
									movePosition(i - n);
									if (flagAllowEmpty) {
										return sl_true;
									} else {
										return nDigits != 0;
									}
								} else if (ch >= '0' && ch <= '9') {
									if (nDigits >= 20) {
										return sl_false;
									}
									outValue = outValue * 10 + (ch - '0');
									nDigits++;
								} else {
									return sl_false;
								}
							}
						} else if (n == SLIB_IO_ENDED) {
							if (flagAllowEmpty) {
								return sl_true;
							} else {
								return nDigits != 0;
							}
						} else {
							break;
						}
					}
					return sl_false;
				}

				sl_bool readFraction(double& outValue, sl_bool flagAllowEmpty = sl_false)
				{
					outValue = 0;
					sl_uint32 nDigits = 0;
					double exp = 0.1;
					for (;;) {
						sl_char8* buf;
						sl_reg n = readBuffer(buf);
						if (n > 0) {
							for (sl_reg i = 0; i < n; i++) {
								sl_char8 ch = buf[i];
								if (IsWhitespace(ch) || IsDelimiter(ch)) {
									movePosition(i - n);
									if (flagAllowEmpty) {
										return sl_true;
									} else {
										return nDigits != 0;
									}
								} else if (ch >= '0' && ch <= '9') {
									outValue += (double)(ch - '0') * exp;
									exp /= 10.0;
									nDigits++;
								} else {
									return sl_false;
								}
							}
						} else if (n == SLIB_IO_ENDED) {
							if (flagAllowEmpty) {
								return sl_true;
							} else {
								return nDigits != 0;
							}
						} else {
							break;
						}
					}
					return sl_false;
				}

				sl_bool readString(String& outValue)
				{
					sl_char8 ch;
					if (!(readChar(ch))) {
						return sl_false;
					}
					if (ch != '(') {
						return sl_false;
					}
					CList<sl_char8> list;
					sl_uint32 nOpen = 0;
					sl_bool flagEscape = sl_false;
					sl_uint32 octal = 0;
					sl_uint32 nOctal = 0;
					while (list.getCount() < MAX_STRING_LENGTH) {
						sl_char8* buf;
						sl_reg n = readBuffer(buf);
						if (n > 0) {
							for (sl_reg i = 0; i < n; i++) {
								ch = buf[i];
								if (flagEscape) {
									if (ch >= '0' && ch <= '7') {
										octal = ch - '0';
										nOctal = 1;
									} else {
										switch (ch) {
										case 'n':
											ch = '\n';
											break;
										case 'r':
											ch = '\r';
											break;
										case 't':
											ch = '\t';
											break;
										case 'b':
											ch = '\b';
											break;
										case 'f':
											ch = '\f';
											break;
										case '(':
											ch = '(';
											break;
										case ')':
											ch = ')';
											break;
										case '\\':
											ch = '\\';
											break;
										default:
											return sl_false;
										}
									}
								} else {
									if (nOctal) {
										if (nOctal < 3 && ch >= '0' && ch <= '7') {
											octal = (octal << 3) | (ch - '0');
											nOctal++;
										} else {
											list.add_NoLock(octal);
											nOctal = 0;
										}
									}
									if (!nOctal) {
										if (ch == '\\') {
											flagEscape = sl_true;
										} else if (ch == '(') {
											list.add_NoLock('(');
											nOpen++;
										} else if (ch == ')') {
											list.add_NoLock(')');
											if (nOpen) {
												nOpen--;
											} else {
												movePosition(i + 1 - n);
												outValue = String(list.getData(), list.getCount());
												return sl_true;
											}
										} else {
											list.add_NoLock(ch);
										}
									}
								}
							}
						} else {
							break;
						}
					}
					return sl_false;
				}

				sl_bool readHexString(String& outValue)
				{
					sl_char8 ch;
					if (!(readChar(ch))) {
						return sl_false;
					}
					if (ch != '<') {
						return sl_false;
					}
					sl_bool flagFirstHex = sl_true;
					sl_uint32 firstHexValue = 0;
					CList<sl_char8> list;
					while (list.getCount() < MAX_STRING_LENGTH) {
						sl_char8* buf;
						sl_reg n = readBuffer(buf);
						if (n > 0) {
							for (sl_reg i = 0; i < n; i++) {
								ch = buf[i];
								if (ch == '>') {
									if (!flagFirstHex) {
										list.add_NoLock(firstHexValue << 4);
									}
									movePosition(i + 1 - n);
									outValue = String(list.getData(), list.getCount());
									return sl_true;
								} else {
									sl_uint32 h = SLIB_CHAR_HEX_TO_INT(ch);
									if (h < 16) {
										if (flagFirstHex) {
											firstHexValue = h;
											flagFirstHex = sl_false;
										} else {
											list.add_NoLock((firstHexValue << 4) | h);
											flagFirstHex = sl_true;
										}
									} else if (!(IsWhitespace(ch))) {
										return sl_false;
									}
								}
							}
						} else {
							break;
						}
					}
					return sl_false;
				}

				sl_bool readReference(sl_uint32& objectNumber, sl_uint32& version)
				{
					if (readUint(objectNumber)) {
						if (objectNumber) {
							if (skipWhitespaces()) {
								if (readUint(version)) {
									if (skipWhitespaces()) {
										sl_char8 ch;
										if (readChar(ch)) {
											return ch == 'R';
										}
									}
								}
							}
						}
					}
					return sl_false;
				}

				sl_bool readStreamContent(sl_uint32 length, Memory& _out)
				{
					if (readWordAndEquals(StringView::literal("stream"))) {
						sl_char8 ch;
						if (readChar(ch)) {
							if (ch == '\r') {
								if (!(readChar(ch))) {
									return sl_false;
								}
								if (ch != '\n') {
									return sl_false;
								}
							} else if (ch != '\n') {
								return sl_false;
							}
							if (length) {
								Memory mem = Memory::create(length);
								if (read(mem.getData(), length) != mem.getSize()) {
									return sl_false;
								}
								_out = Move(mem);
							}
							if (skipWhitespaces()) {
								return readWordAndEquals(StringView::literal("endstream"));
							}
						}
					}
					return sl_false;
				}

				sl_bool getStreamLength(VariantMap& properties, sl_uint32& _out)
				{
					SLIB_STATIC_STRING(strLength, "Length")
					Variant varLength = properties.getValue(strLength);
					if (varLength.isUint64() && varLength.getTag() == 1) {
						if (!(getObject(varLength.getUint64(), varLength))) {
							return sl_false;
						}
					}
					if (varLength.isIntegerType()) {
						sl_int32 n = varLength.getInt32();
						if (n >= 0) {
							_out = n;
							return sl_true;
						}
					}
					return sl_false;
				}

				sl_bool readDictionary(VariantMap& outMap)
				{
					sl_char8 buf[2];
					if (read(buf, 2) != 2) {
						return sl_false;
					}
					if (buf[0] != '<' || buf[1] != '<') {
						return sl_false;
					}
					for (;;) {
						if (!(skipWhitespaces())) {
							break;
						}
						sl_char8 ch;
						if (!(peekChar(ch))) {
							return sl_false;
						}
						if (ch == '/') {
							String name;
							if (!(readName(name))) {
								return sl_false;
							}
							if (!(skipWhitespaces())) {
								return sl_false;
							}
							Variant value;
							if (!(readValue(value))) {
								return sl_false;
							}
							outMap.add_NoLock(Move(name), Move(value));
						} else if (ch == '>') {
							movePosition(1);
							if (!(readChar(ch))) {
								return sl_false;
							}
							return ch == '>';
						} else {
							return sl_false;
						}
					}
					return sl_false;
				}

				sl_bool readArray(VariantList& outList)
				{
					sl_char8 ch;
					if (!(readChar(ch))) {
						return sl_false;
					}
					if (ch != '[') {
						return sl_false;
					}
					for (;;) {
						if (!(skipWhitespaces())) {
							break;
						}
						if (!(peekChar(ch))) {
							return sl_false;
						}
						if (ch == ']') {
							movePosition(1);
							return sl_true;
						}
						Variant var;
						if (readValue(var)) {
							outList.add_NoLock(Move(var));
						} else {
							return sl_false;
						}
					}
					return sl_false;
				}

				sl_bool readValue(Variant& _out)
				{
					sl_char8 ch;
					if (!(peekChar(ch))) {
						return sl_false;
					}
					if (ch == 'n') {
						if (readWordAndEquals(StringView::literal("null"))) {
							_out.setNull();
							return sl_true;
						}
					} else if (ch == 't') {
						if (readWordAndEquals(StringView::literal("true"))) {
							_out.setBoolean(sl_true);
							return sl_true;
						}
					} else if (ch == 'f') {
						if (readWordAndEquals(StringView::literal("false"))) {
							_out.setBoolean(sl_false);
							return sl_true;
						}
					} else if (IsNumeric(ch)) {
						if (ch >= '0' && ch <= '9') {
							sl_size posBackup = getPosition();
							sl_uint32 num, version;
							if (readReference(num, version)) {
								_out.setUint64(MAKE_OBJECT_ID(version, num));
								_out.setTag(1);
								return sl_true;
							}
							setPosition(posBackup);
						}
						sl_bool flagNegative = sl_false;
						if (ch == '-' || ch == '+') {
							movePosition(1);
							if (ch == '-') {
								flagNegative = sl_true;
							}
							if (!(skipWhitespaces())) {
								return sl_false;
							}
							if (!(peekChar(ch))) {
								return sl_false;
							}
						}
						if (ch == '.') {
							movePosition(1);
							double f;
							if (readFraction(f)) {
								if (flagNegative) {
									f = -f;
								}
								_out.setDouble(f);
								return sl_true;
							}
						} else if (ch >= '0' && ch <= '9') {
							sl_uint32 value;
							sl_bool flagEndsWithPoint = sl_false;
							if (readUint(value, sl_true)) {
								if (peekChar(ch)) {
									if (ch == '.') {
										movePosition(1);
										double f;
										if (!(readFraction(f, sl_true))) {
											return sl_false;
										}
										f += (double)value;
										if (flagNegative) {
											f = -f;
										}
										_out.setDouble(f);
										return sl_true;
									}
								}
								if (flagNegative) {
									_out.setInt32(-((sl_int32)value));
								} else {
									_out.setUint32((sl_uint32)value);
								}
								return sl_true;
							}
						}
					} else if (ch == '(') {
						String s;
						if (readString(s)) {
							_out.setString(Move(s));
							return sl_true;
						}
					} else if (ch == '<') {
						movePosition(1);
						if (peekChar(ch)) {
							movePosition(-1);
							if (ch == '<') {
								VariantMap map;
								if (readDictionary(map)) {
									_out.setVariantMap(Move(map));
									return sl_true;
								}
							} else {
								String s;
								if (readHexString(s)) {
									_out.setString(Move(s));
									return sl_true;
								}
							}
						}
					} else if (ch == '/') {
						String name;
						if (readName(name)) {
							_out.setString(Move(name));
							_out.setTag(1);
							return sl_true;
						}
					} else if (ch == '[') {
						VariantList list;
						if (readArray(list)) {
							_out.setVariantList(Move(list));
							return sl_true;
						}
					}
					return sl_false;
				}

				sl_bool readObject(sl_uint64& outId, Variant& outValue)
				{
					sl_uint32 num;
					if (readUint(num)) {
						if (skipWhitespaces()) {
							sl_uint32 generation;
							if (readUint(generation)) {
								if (skipWhitespaces()) {
									if (readWordAndEquals(StringView::literal("obj"))) {
										if (skipWhitespaces()) {
											if (readValue(outValue)) {
												if (skipWhitespaces()) {
													if (outValue.isVariantMap()) {
														sl_char8 ch;
														if (peekChar(ch)) {
															if (ch == 's') {
																VariantMap properties = outValue.getVariantMap();
																sl_uint32 length;
																if (getStreamLength(properties, length)) {
																	Memory content;
																	if (readStreamContent(length, content)) {
																		Ref<PdfStream> stream = new PdfStream;
																		if (stream.isNotNull()) {
																			stream->properties = Move(properties);
																			stream->content = Move(content);
																			outValue = Move(stream);
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
													}
													if (readWordAndEquals(StringView::literal("endobj"))) {
														outId = MAKE_OBJECT_ID(num, generation);
														return sl_true;
													}
												}
											}
										}
									}
								}
							}
						}
					}
					return sl_false;
				}

				sl_bool getObject(sl_uint64 _id, Variant& _out)
				{
					sl_uint32 offset;
					if (objectOffsets.get(_id, &offset)) {
						if (setPosition(offset)) {
							sl_uint64 n;
							if (readObject(n, _out)) {
								return _id == n;
							}
						}
					}
					return sl_false;
				}

				sl_bool readCrossReferenceEntry(PdfCrossReferenceEntry& entry)
				{
					if (readUint(entry.offset)) {
						if (skipWhitespaces()) {
							if (readUint(entry.generation)) {
								if (skipWhitespaces()) {
									sl_char8 ch;
									if (readChar(ch)) {
										if (ch == 'f') {
											entry.flagFree = sl_true;
										} else if (ch == 'n') {
											entry.flagFree = sl_false;
										} else {
											return sl_false;
										}
										if (peekChar(ch)) {
											if (!(IsWhitespace(ch))) {
												return sl_false;
											}
										}
										return sl_true;
									}
								}
							}
						}
					}
					return sl_false;
				}

				sl_bool readCrossReferenceSection(PdfCrossReferenceSection& section)
				{
					if (readUint(section.firstObjectNumber)) {
						if (skipWhitespaces()) {
							sl_uint32 count;
							if (readUint(count)) {
								for (sl_uint32 i = 0; i < count; i++) {
									if (!(skipWhitespaces())) {
										return sl_false;
									}
									PdfCrossReferenceEntry entry;
									if (!(readCrossReferenceEntry(entry))) {
										return sl_false;
									}
									section.entries.add_NoLock(entry);
								}
								return sl_true;
							}
						}
					}
					return sl_false;
				}

				sl_bool readCrossReferenceTable(PdfCrossReferenceTable& table)
				{
					sl_char8 ch;
					if (!(peekChar(ch))) {
						return sl_false;
					}
					if (ch == 'x') {
						// cross reference table
						if (readWordAndEquals(StringView::literal("xref"))) {
							for (;;) {
								if (!(peekChar(ch))) {
									break;
								}
								if (ch < '0' || ch > '9') {
									break;
								}
								PdfCrossReferenceSection section;
								if (!(readCrossReferenceSection(section))) {
									return sl_false;
								}
								ListElements<PdfCrossReferenceEntry> entries(section.entries);
								for (sl_size i = 0; i < entries.count; i++) {
									PdfCrossReferenceEntry& entry = entries[i];
									if (!(entry.flagFree)) {
										table.objectOffsets.put_NoLock(MAKE_OBJECT_ID(section.firstObjectNumber + i, entry.generation), entry.offset);
									}
								}
								table.sections.add_NoLock(Move(section));
							}
							if (!(peekChar(ch))) {
								return sl_true;
							}
							if (ch == 't') {
								if (readWordAndEquals(StringView::literal("trailer"))) {
									if (readDictionary(table.trailer)) {
										return sl_true;
									}
								}
							} else {
								return sl_true;
							}
						}
					} else {
						// cross reference stream
						sl_uint64 xrefId;
						Variant varStream;
						if (readObject(xrefId, varStream)) {
							Ref<PdfStream> stream = PdfStream::from(varStream);
							if (stream.isNotNull()) {
								SLIB_STATIC_STRING(strType, "Type")
								if (EqualsName(stream->properties.getValue(strType), StringView::literal("XRef"))) {
									Memory content = stream->getOriginalContent();
								}
							}
						}
					}
					return sl_false;
				}

				sl_bool readDocument(PdfDocument& doc)
				{
					sl_char8 version[8];
					if (read(version, 8) != 8) {
						return sl_false;
					}
					if (version[0] != '%' || version[1] != 'P' || version[2] != 'D' || version[3] != 'F' || version[4] != '-' || version[6] != '.') {
						return sl_false;
					}
					if (!SLIB_CHAR_IS_DIGIT(version[5])) {
						return sl_false;
					}
					if (!SLIB_CHAR_IS_DIGIT(version[7])) {
						return sl_false;
					}

					doc.majorVersion = SLIB_CHAR_DIGIT_TO_INT(version[5]);
					doc.minorVersion = SLIB_CHAR_DIGIT_TO_INT(version[7]);

					sl_reg pos = findBackward(StringView::literal("startxref"), 4096);
					if (pos > 0) {
						if (setPosition(pos - 1)) {
							sl_char8 ch;
							if (readChar(ch)) {
								if (IsWhitespace(ch)) {
									if (readWordAndEquals(StringView::literal("startxref"))) {
										if (skipWhitespaces()) {
											sl_uint32 posXref;
											if (readUint(posXref)) {
												if (setPosition(posXref)) {
													PdfCrossReferenceTable xrefTable;
													if (readCrossReferenceTable(xrefTable)) {
														return sl_true;
													}
												}
											}
										}
									}
								}
							}
						}
					}
					return sl_false;
				}

			};

			typedef Parser<BufferedParserBase> BufferedParser;
			typedef Parser<MemoryParserBase> MemoryParser;

		}
	}

	using namespace priv::pdf;
	

	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(PdfCrossReferenceSection)

	PdfCrossReferenceSection::PdfCrossReferenceSection(): firstObjectNumber(0)
	{
	}


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(PdfCrossReferenceTable)

	PdfCrossReferenceTable::PdfCrossReferenceTable()
	{
	}


	SLIB_DEFINE_ROOT_OBJECT(PdfStream)

	PdfStream::PdfStream()
	{
	}

	PdfStream::~PdfStream()
	{
	}

	Ref<PdfStream> PdfStream::from(const Variant& var)
	{
		return CastRef<PdfStream>(var.getRef());
	}

	Memory PdfStream::getOriginalContent()
	{
		SLIB_STATIC_STRING(strFilter, "Filter")
		Variant varFilter;
		if (!(properties.get(strFilter, &varFilter))) {
			return content;
		}
		if (varFilter.isVariantList()) {
			VariantList list = varFilter.getVariantList();
			if (list.isNotNull()) {
				Memory ret = content;
				ListElements<Variant> filters(list);
				for (sl_size i = 0; i < filters.count; i++) {
					String filter = GetName(filters[i]);
					if (filter.isNotNull()) {
						ret = ApplyFilter(ret, filter);
					} else {
						return sl_null;
					}
				}
				return ret;
			}
		} else {
			String filter = GetName(varFilter);
			if (filter.isNotNull()) {
				return ApplyFilter(content, filter);
			}
		}
		return sl_null;
	}


	SLIB_DEFINE_OBJECT(PdfDocument, Object)

	PdfDocument::PdfDocument() : majorVersion(0), minorVersion(0), fileSize(0)
	{
	}

	PdfDocument::~PdfDocument()
	{
	}

	sl_bool PdfDocument::openFile(const StringParam& filePath)
	{
		File file = File::openForRead(filePath);
		if (file.isOpened()) {
			RefT< IO<File> > reader = NewRefT< IO<File> >(Move(file));
			if (reader.isNotNull()) {
				// size
				{
					sl_uint64 size = reader->getSize();
					if (!size) {
						return sl_false;
					}
					if (size > MAX_PDF_FILE_SIZE) {
						return sl_false;
					}
					fileSize = (sl_uint32)size;
				}
				RefT<BufferedParser> parser = NewRefT<BufferedParser>();
				if (parser.isNotNull()) {
					if (parser->reader.open(reader.get())) {
						if (parser->readDocument(*this)) {
							m_parser = Move(parser);
							return sl_true;
						}
					}
				}
			}
		}
		return sl_false;
	}

	sl_bool PdfDocument::isEncrypted()
	{
		SLIB_STATIC_STRING(s, "Encrypt");
		return lastTrailer.find_NoLock(s) != sl_null;
	}

	sl_bool PdfDocument::isEncryptedFile(const StringParam& path)
	{
		PdfDocument doc;
		if (doc.openFile(path)) {
			return doc.isEncrypted();
		}
		return sl_false;
	}

}
