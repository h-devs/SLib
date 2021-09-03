/*
 *   Copyright (c) 2008-2020 SLIBIO <https://github.com/SLIBIO>
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
#include "slib/core/string_buffer.h"
#include "slib/core/thread.h"

#define MAX_PDF_FILE_SIZE 0x40000000

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
			static const char g_charType[256] = {
				// NUL  SOH  STX  ETX  EOT  ENQ  ACK  BEL  BS   HT   LF   VT   FF   CR   SO   SI
				'W', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'W', 'W', 'R', 'W', 'W', 'R', 'R',
				// DLE  DC1  DC2  DC3  DC4  NAK  SYN  ETB  CAN  EM  SUB  ESC  FS   GS   RS   US
				'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R',
				// SP   !   "    #    $    %    &    ´    (    )    *    +    ,    -    .   /
				'W', 'R', 'R', 'R', 'R', 'D', 'R', 'R', 'D', 'D', 'R', 'N', 'R', 'N', 'N', 'D',
				// 0    1    2    3    4    5    6    7    8    9    :    ;    <    =    >   ?
				'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'N', 'R', 'R', 'D', 'R', 'D', 'R',
				// @    A    B    C    D    E    F    G    H    I    J    K    L    M    N   O
				'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R',
				// P    Q    R    S    T    U    V    W    X    Y    Z    [    \    ]    ^   _
				'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'D', 'R', 'D', 'R', 'R',
				// `    a    b    c    d    e    f    g    h    i    j    k    l    m    n   o
				'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R',
				// p    q    r    s    t    u    v    w    x    y    z    {    |    }    ~	DEL
				'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'D', 'R', 'D', 'R', 'R',
				'W', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R',
				'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R',
				'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R',
				'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R',
				'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R',
				'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R',
				'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R',
				'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'R', 'W' };


			SLIB_INLINE static sl_bool IsWhitespace(char c)
			{
				return g_charType[c] == 'W';
			}

			SLIB_INLINE static sl_bool IsNumeric(char c)
			{
				return g_charType[c] == 'N';
			}

			SLIB_INLINE static sl_bool IsDelimiter(char c)
			{
				return g_charType[c] == 'D';
			}

			SLIB_INLINE static sl_bool IsOther(char c)
			{
				return g_charType[c] == 'R';
			}

			SLIB_INLINE static sl_bool IsLineEnding(char c) {
				return c == '\r' || c == '\n';
			}

		}
	}

	using namespace priv::pdf;

	String PdfDocument::readWord()
	{
		StringBuffer sb;
		sl_bool bStartSpace = sl_true;
		while (1) {
			void* _buf;
			sl_reg n = m_reader.read(_buf);
			char* buf = (char*)_buf;
			if (n > 0) {
				sl_size posStartWord = 0;
				for (sl_reg i = 0; i < n; i++) {
					char ch = buf[i];
					if (bStartSpace) {
						if (!(IsWhitespace(ch))) {
							posStartWord = i;
							bStartSpace = sl_false;
						}
					} else {
						if (IsWhitespace(ch)) {
							m_reader.seek(i - n, SeekPosition::Current);
							if (sb.getLength()) {
								sb.addStatic(buf + posStartWord, i - posStartWord);
								return sb.merge();
							} else {
								return String(buf + posStartWord, i - posStartWord);
							}
						}
					}
				}
				if (!bStartSpace) {
					if (!(sb.add(String(buf + posStartWord, n - posStartWord)))) {
						return String::null();
					}
				}
			} else {
				if (n) {
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
		}
		if (sb.getLength() == 0) {
			return String::null();
		} else {
			return sb.merge();
		}
	}

	sl_bool PdfDocument::readInt64(sl_int64& n)
	{
		String s = readWord();
		return s.parseInt64(&n);
	}

	sl_bool PdfDocument::readUint64(sl_uint64& n)
	{
		String s = readWord();
		return s.parseUint64(&n);
	}

	sl_bool PdfDocument::readInt32(sl_int32& n)
	{
		String s = readWord();
		return s.parseInt32(&n);
	}

	sl_bool PdfDocument::readUint32(sl_uint32& n)
	{
		String s = readWord();
		return s.parseUint32(&n);
	}

	sl_bool PdfDocument::readDictionary(HashMap<String, String>& map)
	{
		return sl_true;
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
			if (readWord() != "startxref") {
				return sl_false;
			}
			sl_uint32 posCrossRef;
			if (!(readUint32(posCrossRef))) {
				return sl_false;
			}
			if (readWord() != "%%EOF") {
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
			if (readWord() != "trailer") {
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
