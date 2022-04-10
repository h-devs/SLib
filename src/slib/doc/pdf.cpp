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
#include "slib/core/thread.h"
#include "slib/core/variant.h"

#define MAX_PDF_FILE_SIZE 0x40000000
#define MAX_WORD_LENGTH 256
#define MAX_NAME_LENGTH 127
#define MAX_STRING_LENGTH 32767

namespace slib
{

	PdfDocument::PdfDocument() : majorVersion(0), minorVersion(0), fileSize(0), offsetOfLastCrossRef(0)
	{
	}

	PdfDocument::~PdfDocument()
	{
	}

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

		}
	}

	using namespace priv::pdf;

	sl_bool PdfDocument::readWord(String& _out)
	{
		sl_char8 word[MAX_WORD_LENGTH];
		sl_size lenWord = 0;
		while (1) {
			void* _buf;
			sl_reg n = m_reader.read(_buf);
			if (n > 0) {
				sl_char8* buf = (sl_char8*)_buf;
				for (sl_reg i = 0; i < n; i++) {
					char ch = buf[i];
					if (IsWhitespace(ch) || IsDelimiter(ch)) {
						m_reader.seek(i - n, SeekPosition::Current);
						_out = String(word, lenWord);
						return sl_true;
					} else {
						if (lenWord >= sizeof(word)) {
							return sl_false;
						}
						word[lenWord++] = ch;
					}
				}
			} else {
				if (n == SLIB_IO_WOULD_BLOCK) {
					if (Thread::isStoppingCurrent()) {
						break;
					}
					m_reader.waitRead();
				} else if (n == SLIB_IO_ENDED) {
					_out = String(word, lenWord);
					return sl_true;
				} else {
					break;
				}
			}
		}
		return sl_false;
	}

	sl_bool PdfDocument::readWordAndEquals(const StringView& _word, sl_bool flagCheckDelimiter)
	{
		sl_size lenWord = _word.getLength();
		if (!lenWord) {
			return sl_false;
		}
		const sl_char8* word = _word.getData();
		sl_size posWord = 0;
		while (1) {
			void* _buf;
			sl_reg n = m_reader.read(_buf);
			if (n > 0) {
				sl_char8* buf = (sl_char8*)_buf;
				for (sl_reg i = 0; i < n; i++) {
					char ch = buf[i];
					if (IsWhitespace(ch) || (IsDelimiter(ch) && flagCheckDelimiter)) {
						m_reader.seek(i - n, SeekPosition::Current);
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
			} else {
				if (n == SLIB_IO_WOULD_BLOCK) {
					if (Thread::isStoppingCurrent()) {
						break;
					}
					m_reader.waitRead();
				} else if (n == SLIB_IO_ENDED) {
					return posWord == lenWord;
				} else {
					break;
				}
			}
		}
		return sl_false;
	}

	sl_bool PdfDocument::skipWhitespaces(sl_bool flagSkipComments)
	{
		sl_bool flagComment = sl_false;
		while (1) {
			void* _buf;
			sl_reg n = m_reader.read(_buf);
			if (n > 0) {
				sl_char8* buf = (sl_char8*)_buf;
				for (sl_reg i = 0; i < n; i++) {
					sl_char8 ch = buf[i];
					if (flagComment) {
						if (IsLineEnding(ch)) {
							flagComment = sl_false;
						}
					} else if (ch == '%' && flagSkipComments) {
						flagComment = sl_true;
					} else if (!(IsWhitespace(ch))) {
						m_reader.seek(i - n, SeekPosition::Current);
						return sl_true;
					}
				}
			} else {
				if (n == SLIB_IO_WOULD_BLOCK) {
					if (Thread::isStoppingCurrent()) {
						break;
					}
					m_reader.waitRead();
				} else if (n == SLIB_IO_ENDED) {
					return sl_true;
				} else {
					break;
				}
			}
		}
		return sl_false;
	}

	sl_bool PdfDocument::readObject(Variant& outObject)
	{
		sl_int8 ch;
		if (!(m_reader.peekInt8(&ch))) {
			return sl_false;
		}
		if (ch == 'n') {
			if (!(readWordAndEquals("null"))) {
				return sl_false;
			}
			outObject.setNull();
			return sl_true;
		} else if (ch == 't') {
			if (!(readWordAndEquals("true"))) {
				return sl_false;
			}
			outObject.setBoolean(sl_true);
			return sl_true;
		} else if (ch == 'f') {
			if (!(readWordAndEquals("false"))) {
				return sl_false;
			}
			outObject.setBoolean(sl_false);
			return sl_true;
		} else if (IsNumeric(ch)) {
			if (ch >= '0' && ch <= '9') {
				sl_uint64 posBackup = m_reader.getPosition();
				sl_uint32 num, version;
				if (readReference(num, version)) {
					outObject.setUint64(SLIB_MAKE_QWORD4(version, num));
					return sl_true;
				}
				m_reader.seek(posBackup, SeekPosition::Begin);
			}
			sl_bool flagNegative = sl_false;
			if (ch == '-' || ch == '+') {
				m_reader.seek(1, SeekPosition::Current);
				if (ch == '-') {
					flagNegative = sl_true;
				}
				if (!(skipWhitespaces())) {
					return sl_false;
				}
				if (!(m_reader.peekInt8(&ch))) {
					return sl_false;
				}
			}
			if (ch == '.') {
				m_reader.seek(1, SeekPosition::Current);
				double f;
				if (!(readFraction(f))) {
					return sl_false;
				}
				if (flagNegative) {
					f = -f;
				}
				outObject.setDouble(f);
				return sl_true;
			} else if (ch >= '0' && ch <= '9') {
				sl_uint32 value;
				sl_bool flagEndsWithPoint = sl_false;
				if (!(readUint(value, sl_true))) {
					return sl_false;
				}
				if (m_reader.peekInt8(&ch)) {
					if (ch == '.') {
						m_reader.seek(1, SeekPosition::Current);
						double f;
						if (!(readFraction(f, sl_true))) {
							return sl_false;
						}
						f += (double)value;
						if (flagNegative) {
							f = -f;
						}
						outObject.setDouble(f);
					}
				}
				if (flagNegative) {
					outObject.setInt32(-((sl_int32)value));
				} else {
					outObject.setUint32((sl_uint32)value);
				}
				return sl_true;
			} else {
				return sl_false;
			}
		} else if (ch == '(') {
			String s;
			if (!(readString(s))) {
				return sl_false;
			}
			outObject.setString(Move(s));
			outObject.tag = 0;
			return sl_true;
		} else if (ch == '<') {
			m_reader.seek(1, SeekPosition::Current);
			if (!(m_reader.peekInt8(&ch))) {
				return sl_false;
			}
			m_reader.seek(-1, SeekPosition::Current);
			if (ch == '<') {
				VariantMap map;
				if (!(readDictionary(map))) {
					return sl_false;
				}
				outObject.setVariantMap(Move(map));
				return sl_true;
			} else {
				String s;
				if (!(readHexString(s))) {
					return sl_false;
				}
				outObject.setString(Move(s));
				outObject.tag = 0;
				return sl_true;
			}
		} else if (ch == '/') {
			String name;
			if (!(readName(name))) {
				return sl_false;
			}
			outObject.setString(Move(name));
			outObject.tag = 1;
			return sl_true;
		} else if (ch == '[') {
			VariantList list;
			if (!(readArray(list))) {
				return sl_false;
			}
			outObject.setVariantList(Move(list));
			return sl_true;
		}
		return sl_false;
	}

	sl_bool PdfDocument::readDictionary(VariantMap& outMap)
	{
		sl_char8 buf[2];
		if (m_reader.readFully(buf, 2) != 2) {
			return sl_false;
		}
		if (buf[0] != '<' || buf[1] != '<') {
			return sl_false;
		}
		while (1) {
			if (!(skipWhitespaces())) {
				break;
			}
			sl_int8 ch;
			if (!(m_reader.peekInt8(&ch))) {
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
				if (!(readObject(value))) {
					return sl_false;
				}
				outMap.add_NoLock(Move(name), Move(value));
			} else if (ch == '>') {
				m_reader.seek(1, SeekPosition::Current);
				if (!(m_reader.readInt8(&ch))) {
					return sl_false;
				}
				return ch == '>';
			} else {
				return sl_false;
			}
		}
		return sl_false;
	}

	sl_bool PdfDocument::readArray(VariantList& outList)
	{
		sl_int8 ch;
		if (!(m_reader.readInt8(&ch))) {
			return sl_false;
		}
		if (ch != '[') {
			return sl_false;
		}
		while (1) {
			if (!(skipWhitespaces())) {
				break;
			}
			if (!(m_reader.peekInt8(&ch))) {
				return sl_false;
			}
			if (ch == ']') {
				m_reader.seek(1, SeekPosition::Current);
				return sl_true;
			}
			Variant var;
			if (readObject(var)) {
				outList.add_NoLock(Move(var));
			} else {
				return sl_false;
			}
		}
		return sl_false;
	}

	sl_bool PdfDocument::readName(String& _out)
	{
		sl_char8 name[MAX_WORD_LENGTH];
		sl_size lenName = 0;
		sl_int8 ch;
		if (!(m_reader.readInt8(&ch))) {
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
		while (1) {
			void* _buf;
			sl_reg n = m_reader.read(_buf);
			if (n > 0) {
				sl_char8* buf = (sl_char8*)_buf;
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
							m_reader.seek(i - n, SeekPosition::Current);
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
			} else {
				if (n == SLIB_IO_WOULD_BLOCK) {
					if (Thread::isStoppingCurrent()) {
						break;
					}
					m_reader.waitRead();
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
		}
		return sl_false;
	}

	sl_bool PdfDocument::readUint(sl_uint32& outValue, sl_bool flagAllowEmpty)
	{
		outValue = 0;
		sl_uint32 nDigits = 0;
		while (1) {
			void* _buf;
			sl_reg n = m_reader.read(_buf);
			if (n > 0) {
				sl_char8* buf = (sl_char8*)_buf;
				for (sl_reg i = 0; i < n; i++) {
					char ch = buf[i];
					if (IsWhitespace(ch) || IsDelimiter(ch) || ch == '.') {
						m_reader.seek(i - n, SeekPosition::Current);
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
			} else {
				if (n == SLIB_IO_WOULD_BLOCK) {
					if (Thread::isStoppingCurrent()) {
						break;
					}
					m_reader.waitRead();
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
		}
		return sl_false;
	}

	sl_bool PdfDocument::readInt(sl_int32& outValue)
	{
		sl_int8 ch;
		if (!(m_reader.peekInt8(&ch))) {
			return sl_false;
		}
		sl_bool flagNegative = sl_false;
		if (ch == '-' || ch == '+') {
			m_reader.seek(1, SeekPosition::Current);
			if (ch == '-') {
				flagNegative = sl_true;
			}
			if (!(skipWhitespaces())) {
				return sl_false;
			}
		}
		sl_uint32 value;
		if (!(readUint(value))) {
			return sl_false;
		}
		if (flagNegative) {
			outValue = -((sl_int32)value);
		} else {
			outValue = value;
		}
		return sl_true;
	}

	sl_bool PdfDocument::readFraction(double& outValue, sl_bool flagAllowEmpty)
	{
		outValue = 0;
		sl_uint32 nDigits = 0;
		double exp = 0.1;
		while (1) {
			void* _buf;
			sl_reg n = m_reader.read(_buf);
			if (n > 0) {
				sl_char8* buf = (sl_char8*)_buf;
				for (sl_reg i = 0; i < n; i++) {
					char ch = buf[i];
					if (IsWhitespace(ch) || IsDelimiter(ch)) {
						m_reader.seek(i - n, SeekPosition::Current);
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
			} else {
				if (n == SLIB_IO_WOULD_BLOCK) {
					if (Thread::isStoppingCurrent()) {
						break;
					}
					m_reader.waitRead();
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
		}
		return sl_false;
	}

	sl_bool PdfDocument::readString(String& outValue)
	{
		sl_int8 ch;
		if (!(m_reader.readInt8(&ch))) {
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
			void* _buf;
			sl_reg n = m_reader.read(_buf);
			if (n > 0) {
				sl_char8* buf = (sl_char8*)_buf;
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
									m_reader.seek(i + 1 - n, SeekPosition::Current);
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
				if (n == SLIB_IO_WOULD_BLOCK) {
					if (Thread::isStoppingCurrent()) {
						break;
					}
					m_reader.waitRead();
				} else {
					break;
				}
			}
		}
		return sl_false;
	}

	sl_bool PdfDocument::readHexString(String& outValue)
	{
		sl_int8 ch;
		if (!(m_reader.readInt8(&ch))) {
			return sl_false;
		}
		if (ch != '<') {
			return sl_false;
		}
		sl_bool flagFirstHex = sl_true;
		sl_uint32 firstHexValue = 0;
		CList<sl_char8> list;
		while (list.getCount() < MAX_STRING_LENGTH) {
			void* _buf;
			sl_reg n = m_reader.read(_buf);
			if (n > 0) {
				sl_char8* buf = (sl_char8*)_buf;
				for (sl_reg i = 0; i < n; i++) {
					ch = buf[i];
					if (ch == '>') {
						if (!flagFirstHex) {
							list.add_NoLock(firstHexValue << 4);
						}
						m_reader.seek(i + 1 - n, SeekPosition::Current);
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
				if (n == SLIB_IO_WOULD_BLOCK) {
					if (Thread::isStoppingCurrent()) {
						break;
					}
					m_reader.waitRead();
				} else {
					break;
				}
			}
		}
		return sl_false;
	}

	sl_bool PdfDocument::readReference(sl_uint32& objectNumber, sl_uint32& version)
	{
		if (!(readUint(objectNumber))) {
			return sl_false;
		}
		if (!objectNumber) {
			return sl_false;
		}
		if (!(skipWhitespaces())) {
			return sl_false;
		}
		if (!(readUint(version))) {
			return sl_false;
		}
		if (!(skipWhitespaces())) {
			return sl_false;
		}
		sl_int8 ch;
		if (!(m_reader.readInt8(&ch))) {
			return sl_false;
		}
		return ch == 'R';
	}

	sl_bool PdfDocument::openFile(const StringParam& filePath)
	{
		File file = File::openForRead(filePath);
		if (file.isOpened()) {
			RefT< IO<File> > ref = NewRefT< IO<File> >(Move(file));
			if (ref.isNotNull()) {
				if (setReader(ref)) {
					if (readHeader()) {
						return sl_true;
					}
				}
			}
		}
		return sl_false;
	}

	sl_bool PdfDocument::setReader(const Ptr<IReader, ISeekable>& reader)
	{
		return m_reader.open(reader);
	}

	sl_bool PdfDocument::readHeader()
	{
		// size
		{
			sl_uint64 size = m_reader.getSize();
			if (!size) {
				return sl_false;
			}
			if (size > MAX_PDF_FILE_SIZE) {
				return sl_false;
			}
			fileSize = (sl_uint32)size;
		}

		char version[8];
		if (m_reader.readFully(version, 8) != 8) {
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

		majorVersion = SLIB_CHAR_DIGIT_TO_INT(version[5]);
		minorVersion = SLIB_CHAR_DIGIT_TO_INT(version[7]);

		// last startxref, trailer
		{
			sl_int64 pos = m_reader.findBackward("startxref", 9, -1, 4096);
			if (pos <= 0) {
				return sl_false;
			}
			if (!(m_reader.seek(pos - 1, SeekPosition::Begin))) {
				return sl_false;
			}
			sl_int8 c;
			if (!(m_reader.readInt8(&c))) {
				return sl_false;
			}
			if (!(IsWhitespace(c))) {
				return sl_false;
			}
			if (!(readWordAndEquals(StringView::literal("startxref")))) {
				return sl_false;
			}
			if (!(skipWhitespaces())) {
				return sl_false;
			}
			sl_uint32 posCrossRef;
			if (!(readUint(posCrossRef))) {
				return sl_false;
			}
			if (!(skipWhitespaces(sl_false))) {
				return sl_false;
			}
			if (!(readWordAndEquals(StringView::literal("%%EOF"), sl_false))) {
				return sl_false;
			}
			offsetOfLastCrossRef = posCrossRef;

			pos = m_reader.findBackward("trailer", 7, pos, 4096);
			if (pos <= 0) {
				return sl_false;
			}
			if (!(m_reader.seek(pos - 1, SeekPosition::Begin))) {
				return sl_false;
			}
			if (!(m_reader.readInt8(&c))) {
				return sl_false;
			}
			if (!(IsWhitespace(c))) {
				return sl_false;
			}
			if (!(readWordAndEquals(StringView::literal("trailer")))) {
				return sl_false;
			}
			if (!(skipWhitespaces())) {
				return sl_false;
			}
			if (!(readDictionary(lastTrailer))) {
				return sl_false;
			}
		}
		
		return sl_true;
	}

	sl_bool PdfDocument::isEncrypted(const Ptr<IReader, ISeekable>& reader)
	{
		PdfDocument doc;
		if (doc.setReader(reader)) {
			if (doc.readHeader()) {
				return sl_true;
			}
		}
		return sl_false;
	}

	sl_bool PdfDocument::isEncryptedFile(const StringParam& path)
	{
		IO<File> file = File::openForRead(path);
		if (file.isOpened()) {
			return isEncrypted(&file);
		}
		return sl_false;
	}

}
