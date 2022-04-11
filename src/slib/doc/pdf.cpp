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


	SLIB_DEFINE_OBJECT(PdfDocument, Object)

	PdfDocument::PdfDocument() : majorVersion(0), minorVersion(0), fileSize(0)
	{
	}

	PdfDocument::~PdfDocument()
	{
	}

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

	sl_bool PdfDocument::readWordAndEquals(const StringView& _word)
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
					if (IsWhitespace(ch) || IsDelimiter(ch)) {
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

	sl_bool PdfDocument::skipWhitespaces()
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
					} else if (ch == '%') {
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

	sl_bool PdfDocument::readObject(sl_uint64& outId, Variant& outValue)
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

	sl_bool PdfDocument::getObject(sl_uint64 _id, Variant& _out)
	{
		sl_uint32 offset;
		if (objectOffsets.get(_id, &offset)) {
			if (m_reader.seek(offset, SeekPosition::Begin)) {
				sl_uint64 n;
				if (readObject(n, _out)) {
					return _id == n;
				}
			}
		}
		return sl_false;
	}

	sl_bool PdfDocument::readValue(Variant& _out)
	{
		sl_int8 ch;
		if (!(m_reader.peekInt8(&ch))) {
			return sl_false;
		}
		if (ch == 'n') {
			if (readWordAndEquals("null")) {
				_out.setNull();
				return sl_true;
			}
		} else if (ch == 't') {
			if (readWordAndEquals("true")) {
				_out.setBoolean(sl_true);
				return sl_true;
			}
		} else if (ch == 'f') {
			if (readWordAndEquals("false")) {
				_out.setBoolean(sl_false);
				return sl_true;
			}
		} else if (IsNumeric(ch)) {
			if (ch >= '0' && ch <= '9') {
				sl_uint64 posBackup = m_reader.getPosition();
				sl_uint32 num, version;
				if (readReference(num, version)) {
					_out.setUint64(MAKE_OBJECT_ID(version, num));
					_out.setTag(1);
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
			m_reader.seek(1, SeekPosition::Current);
			if (m_reader.peekInt8(&ch)) {
				m_reader.seek(-1, SeekPosition::Current);
				if (ch == '<') {
					VariantMap map;
					Ref<PdfStream> stream;
					if (readDictionaryOrStream(map, stream)) {
						if (stream.isNotNull()) {
							_out.setRef(Move(stream));
						} else {
							_out.setVariantMap(Move(map));
						}
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
				if (!(readValue(value))) {
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
			if (readValue(var)) {
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
		if (readUint(objectNumber)) {
			if (objectNumber) {
				if (skipWhitespaces()) {
					if (readUint(version)) {
						if (skipWhitespaces()) {
							sl_int8 ch;
							if (m_reader.readInt8(&ch)) {
								return ch == 'R';
							}
						}
					}
				}
			}
		}
		return sl_false;
	}

	sl_bool PdfDocument::readStreamContent(sl_uint32 length, Memory& _out)
	{
		if (readWordAndEquals(StringView::literal("stream"))) {
			sl_int8 ch;
			if (m_reader.readInt8(&ch)) {
				if (ch == '\r') {
					if (!(m_reader.readInt8(&ch))) {
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
					if (!(m_reader.readFully(mem.getData(), length))) {
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

	sl_bool PdfDocument::getStreamLength(VariantMap& properties, sl_uint32& _out)
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

	sl_bool PdfDocument::readDictionaryOrStream(VariantMap& outMap, Ref<PdfStream>& outStream)
	{
		VariantMap map;
		if (!(readDictionary(map))) {
			return sl_false;
		}
		sl_uint32 length;
		if (getStreamLength(map, length)) {
			sl_uint64 pos = m_reader.getPosition();
			Memory content;
			if (readStreamContent(length, content)) {
				Ref<PdfStream> stream = new PdfStream;
				if (stream.isNotNull()) {
					stream->properties = Move(map);
					stream->content = Move(content);
					outStream = Move(stream);
					return sl_true;
				}
			}
			m_reader.seek(pos, SeekPosition::Begin);
		}
		outMap = Move(map);
		return sl_true;
	}

	sl_bool PdfDocument::readStream(Ref<PdfStream>& outStream)
	{
		VariantMap properties;
		if (readDictionary(properties)) {
			sl_uint32 length;
			if (getStreamLength(properties, length)) {
				Memory content;
				if (readStreamContent(length, content)) {
					Ref<PdfStream> stream = new PdfStream;
					if (stream.isNotNull()) {
						stream->properties = Move(properties);
						stream->content = Move(content);
						outStream = Move(stream);
						return sl_true;
					}
				}
			}
		}
		return sl_false;
	}

	sl_bool PdfDocument::readCrossReferenceTable(PdfCrossReferenceTable& table)
	{
		return sl_false;
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

		sl_int64 pos = m_reader.findBackward("startxref", 9, -1, 4096);
		if (pos > 0) {
			if (m_reader.seek(pos - 1, SeekPosition::Begin)) {
				sl_int8 c;
				if (m_reader.readInt8(&c)) {
					if (IsWhitespace(c)) {
						if (readWordAndEquals(StringView::literal("startxref"))) {
							if (skipWhitespaces()) {
								sl_uint32 posXref;
								if (readUint(posXref)) {
									if (readDictionary(lastTrailer)) {
										return sl_true;
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
