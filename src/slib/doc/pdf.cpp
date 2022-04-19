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
#include "slib/core/shared.h"
#include "slib/core/mio.h"
#include "slib/core/memory_buffer.h"
#include "slib/core/expiring_map.h"
#include "slib/crypto/zlib.h"
#include "slib/crypto/md5.h"
#include "slib/crypto/rc4.h"

#define MAX_PDF_FILE_SIZE 0x40000000
#define MAX_WORD_LENGTH 256
#define MAX_NAME_LENGTH 127
#define MAX_STRING_LENGTH 32767
#define EXPIRE_DURATION_OBJECT 5000
#define EXPIRE_DURATION_OBJECT_STREAM 10000

#define MAKE_OBJECT_ID(NUM, GEN) SLIB_MAKE_QWORD4(GEN, NUM)

namespace slib
{

	namespace priv
	{
		namespace pdf
		{

			SLIB_STATIC_STRING(g_strType, "Type")
			SLIB_STATIC_STRING(g_strSize, "Size")
			SLIB_STATIC_STRING(g_strLength, "Length")
			SLIB_STATIC_STRING(g_strIndex, "Index")
			SLIB_STATIC_STRING(g_strFirst, "First")
			SLIB_STATIC_STRING(g_strExtends, "Extends")
			SLIB_STATIC_STRING(g_strPrev, "Prev")
			SLIB_STATIC_STRING(g_strFilter, "Filter")
			SLIB_STATIC_STRING(g_strEncrypt, "Encrypt");
			SLIB_STATIC_STRING(g_strRoot, "Root")
			SLIB_STATIC_STRING(g_strPages, "Pages")
			SLIB_STATIC_STRING(g_strCount, "Count")
			SLIB_STATIC_STRING(g_strKids, "Kids")
			SLIB_STATIC_STRING(g_strContents, "Contents")
			SLIB_STATIC_STRING(g_strID, "ID")
			SLIB_STATIC_STRING(g_strN, "N")
			SLIB_STATIC_STRING(g_strO, "O")
			SLIB_STATIC_STRING(g_strP, "P")
			SLIB_STATIC_STRING(g_strR, "R")
			SLIB_STATIC_STRING(g_strU, "U")
			SLIB_STATIC_STRING(g_strV, "V")
			SLIB_STATIC_STRING(g_strW, "W")

			static sl_uint8 g_encryptionPad[] = { 0x28, 0xbf, 0x4e, 0x5e, 0x4e, 0x75, 0x8a, 0x41, 0x64, 0x00, 0x4e, 0x56, 0xff, 0xfa, 0x01, 0x08, 0x2e, 0x2e, 0x00, 0xb6, 0xd0, 0x68, 0x3e, 0x80, 0x2f, 0x0c, 0xa9, 0xfe, 0x64, 0x53, 0x69, 0x7a };

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

			SLIB_INLINE static sl_uint64 GetObjectId(const PdfReference& ref)
			{
				return MAKE_OBJECT_ID(ref.objectNumber, ref.generation);
			}

			static sl_uint32 ReadUint(const void* _src, sl_uint32 size, sl_uint32 def)
			{
				if (!size) {
					return def;
				}
				sl_uint32 ret = 0;
				sl_uint8* src = (sl_uint8*)_src;
				for (sl_uint32 i = 0; i < size; i++) {
					ret = (ret << 8) | src[i];
				}
				return ret;
			}


			enum class CrossReferenceEntryType
			{
				Free = 0,
				Normal = 1,
				Compressed = 2
			};

			struct SLIB_EXPORT CrossReferenceEntry
			{
				CrossReferenceEntryType type;
				union {
					sl_uint32 nextFreeObject; // For free entry
					sl_uint32 offset; // For normal entry: 10 digits in Cross-Reference-Table
					sl_uint32 streamObject; // For compressed entry
				};
				union {
					sl_uint32 generation; // For free, normal entry: 5 digits in Cross-Reference-Table
					sl_uint32 objectIndex; // For compressed entry
				};
			};

			class ObjectStream : public Referable
			{
			public:
				Ref<ObjectStream> extends;
				CList< Pair<sl_uint32, PdfObject > > objects;

			public:
				PdfObject getItem(sl_uint32 index, sl_uint32& _id)
				{
					sl_uint32 n = (sl_uint32)(objects.getCount());
					if (index < n) {
						Pair<sl_uint32, PdfObject >& item = objects[index];
						_id = item.first;
						return item.second;
					} else {
						if (extends.isNotNull()) {
							return extends->getItem(index - n, _id);
						}
						return PdfObject();
					}
				}

			};

			class PageTree
			{
			public:
				PdfDictionary properties;
				List<PageTree> kids;
				sl_bool flagKids = sl_false;
				sl_uint32 count = 0;
				sl_bool flagCount = sl_false;
				sl_bool flagPage = sl_false;

			public:
				sl_uint32 getPagesCount()
				{
					if (flagPage) {
						return 1;
					}
					if (flagCount) {
						return count;
					}
					count = properties.getValue_NoLock(g_strCount).getUint();
					flagCount = sl_true;
					return count;
				}

			};

			class Context : public Referable
			{
			public:
				CHashMap<sl_uint64, CrossReferenceEntry> references;
				ExpiringMap<sl_uint64, PdfObject> objects;
				ExpiringMap< sl_uint64, Ref<ObjectStream> > objectStreams;
				PageTree pageTree;

				sl_bool flagEncrypt = sl_false;
				sl_uint8 encryptionKey[16];
				sl_uint32 lenEncryptionKey = 0;

			public:
				Context()
				{
					objects.setExpiringMilliseconds(EXPIRE_DURATION_OBJECT);
					objectStreams.setExpiringMilliseconds(EXPIRE_DURATION_OBJECT_STREAM);
				}

			};

			class SLIB_EXPORT CrossReferenceTable
			{
			public:
				Ref<Context> context;
				PdfDictionary trailer;
			};

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

				sl_bool setPosition(sl_size _pos)
				{
					if (_pos <= size) {
						pos = (sl_uint32)_pos;
						return sl_true;
					}
					return sl_false;
				}

				sl_bool movePosition(sl_reg offset)
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
				Ref<Context> context;

				using BASE::readChar;
				using BASE::peekChar;
				using BASE::read;
				using BASE::getPosition;
				using BASE::setPosition;
				using BASE::movePosition;
				using BASE::readBuffer;
				using BASE::findBackward;

			public:
				sl_bool getReference(const PdfReference& ref, CrossReferenceEntry& entry)
				{
					return context->references.get(GetObjectId(ref), &entry);
				}

				sl_bool peekCharAndEquals(sl_char8 _ch)
				{
					sl_char8 ch;
					if (peekChar(ch)) {
						return ch == _ch;
					}
					return sl_false;
				}

				sl_bool readCharAndEquals(sl_char8 _ch)
				{
					sl_char8 ch;
					if (readChar(ch)) {
						return ch == _ch;
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

				String readName()
				{
					sl_char8 name[MAX_WORD_LENGTH];
					sl_size lenName = 0;
					if (!(readCharAndEquals('/'))) {
						return sl_null;
					}
					if (!(skipWhitespaces())) {
						return sl_null;
					}
					sl_bool flagReadHex = sl_false;
					sl_uint32 hex = 0;
					sl_uint32 posHex = 0;
					for (;;) {
						sl_char8* buf;
						sl_reg n = readBuffer(buf);
						if (n > 0) {
							for (sl_reg i = 0; i < n; i++) {
								sl_char8 ch = buf[i];
								if (flagReadHex) {
									sl_uint32 h = SLIB_CHAR_HEX_TO_INT(ch);
									if (h >= 16) {
										return sl_null;
									}
									hex = (hex << 4) | h;
									posHex++;
									if (posHex >= 2) {
										if (lenName >= sizeof(name)) {
											return sl_null;
										}
										name[lenName++] = (sl_char8)hex;;
										flagReadHex = sl_false;
									}
								} else {
									if (IsWhitespace(ch) || IsDelimiter(ch)) {
										movePosition(i - n);
										return String(name, lenName);
									} else if (ch == '#') {
										flagReadHex = sl_true;
										hex = 0;
										posHex = 0;
									} else {
										if (lenName >= sizeof(name)) {
											return sl_null;
										}
										name[lenName++] = ch;
									}
								}
							}
						} else if (n == SLIB_IO_ENDED) {
							if (flagReadHex) {
								return sl_null;
							}
							return String(name, lenName);
						} else {
							break;
						}
					}
					return sl_null;
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

				String readString()
				{
					if (!(readCharAndEquals('('))) {
						return sl_null;
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
								sl_char8 ch = buf[i];
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
											return sl_null;
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
												return String(list.getData(), list.getCount());
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
					return sl_null;
				}

				String readHexString()
				{
					if (!(readCharAndEquals('<'))) {
						return sl_null;
					}
					sl_bool flagFirstHex = sl_true;
					sl_uint32 firstHexValue = 0;
					CList<sl_char8> list;
					while (list.getCount() < MAX_STRING_LENGTH) {
						sl_char8* buf;
						sl_reg n = readBuffer(buf);
						if (n > 0) {
							for (sl_reg i = 0; i < n; i++) {
								sl_char8 ch = buf[i];
								if (ch == '>') {
									if (!flagFirstHex) {
										list.add_NoLock(firstHexValue << 4);
									}
									movePosition(i + 1 - n);
									return String(list.getData(), list.getCount());
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
										return sl_null;
									}
								}
							}
						} else {
							break;
						}
					}
					return sl_null;
				}

				sl_bool readReference(PdfReference& outRef)
				{
					if (readUint(outRef.objectNumber)) {
						if (outRef.objectNumber) {
							if (skipWhitespaces()) {
								if (readUint(outRef.generation)) {
									if (skipWhitespaces()) {
										return readCharAndEquals('R');
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

				sl_bool getStreamLength(const PdfDictionary& properties, sl_uint32& _out)
				{
					PdfObject objLength = properties.getValue_NoLock(g_strLength);
					PdfReference ref;
					if (objLength.getReference(ref)) {
						objLength = getObject(ref);
					}
					return objLength.getUint(_out);
				}

				PdfDictionary readDictionary()
				{
					sl_char8 buf[2];
					if (read(buf, 2) != 2) {
						return sl_null;
					}
					if (buf[0] != '<' || buf[1] != '<') {
						return sl_null;
					}
					PdfDictionary ret;
					for (;;) {
						if (!(skipWhitespaces())) {
							break;
						}
						sl_char8 ch;
						if (!(peekChar(ch))) {
							return sl_null;
						}
						if (ch == '/') {
							String name = readName();
							if (name.isNull()) {
								return sl_null;
							}
							if (!(skipWhitespaces())) {
								return sl_null;
							}
							PdfObject value = readValue();
							if (value.isUndefined()) {
								return sl_null;
							}
							ret.add_NoLock(Move(name), Move(value));
						} else if (ch == '>') {
							movePosition(1);
							if (!(readCharAndEquals('>'))) {
								return sl_null;
							}
							if (ret.isNull()) {
								ret.initialize();
							}
							return ret;
						} else {
							return sl_null;
						}
					}
					return sl_null;
				}

				PdfArray readArray()
				{
					if (!(readCharAndEquals('['))) {
						return sl_null;
					}
					PdfArray ret;
					for (;;) {
						if (!(skipWhitespaces())) {
							break;
						}
						sl_char8 ch;
						if (!(peekChar(ch))) {
							return sl_null;
						}
						if (ch == ']') {
							movePosition(1);
							if (ret.isNull()) {
								ret = PdfArray::create();
							}
							return ret;
						}
						PdfObject var = readValue();
						if (var.isNotUndefined()) {
							ret.add_NoLock(Move(var));
						} else {
							return sl_null;
						}
					}
					return sl_null;
				}

				PdfObject readValue()
				{
					sl_char8 ch;
					if (!(peekChar(ch))) {
						return PdfObject();
					}
					if (ch == 'n') {
						if (readWordAndEquals(StringView::literal("null"))) {
							return sl_null;
						}
					} else if (ch == 't') {
						if (readWordAndEquals(StringView::literal("true"))) {
							return sl_true;
						}
					} else if (ch == 'f') {
						if (readWordAndEquals(StringView::literal("false"))) {
							return sl_false;
						}
					} else if (IsNumeric(ch)) {
						if (ch >= '0' && ch <= '9') {
							sl_size posBackup = getPosition();
							PdfReference ref;
							if (readReference(ref)) {
								return ref;
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
								return PdfObject();
							}
							if (!(peekChar(ch))) {
								return PdfObject();
							}
						}
						if (ch == '.') {
							movePosition(1);
							double f;
							if (readFraction(f)) {
								if (flagNegative) {
									f = -f;
								}
								return (float)f;
							}
						} else if (ch >= '0' && ch <= '9') {
							sl_uint32 value;
							sl_bool flagEndsWithPoint = sl_false;
							if (readUint(value, sl_true)) {
								if (peekCharAndEquals('.')) {
									movePosition(1);
									double f;
									if (!(readFraction(f, sl_true))) {
										return PdfObject();
									}
									f += (double)value;
									if (flagNegative) {
										f = -f;
									}
									return (float)f;
								}
								if (flagNegative) {
									return -((sl_int32)value);
								} else {
									return value;
								}
							}
						}
					} else if (ch == '(') {
						String s = readString();
						if (s.isNotNull()) {
							return s;
						}
					} else if (ch == '<') {
						movePosition(1);
						if (peekChar(ch)) {
							movePosition(-1);
							if (ch == '<') {
								PdfDictionary map = readDictionary();
								if (map.isNotNull()) {
									return map;
								}
							} else {
								String s = readHexString();
								if (s.isNotNull()) {
									return s;
								}
							}
						}
					} else if (ch == '/') {
						PdfName name = readName();
						if (name.isNotNull()) {
							return name;
						}
					} else if (ch == '[') {
						PdfArray list = readArray();
						if (list.isNotNull()) {
							return list;
						}
					}
					return sl_null;
				}

				sl_bool readObjectHeader(PdfReference& outRef)
				{
					if (readUint(outRef.objectNumber)) {
						if (skipWhitespaces()) {
							if (readUint(outRef.generation)) {
								if (skipWhitespaces()) {
									if (readWordAndEquals(StringView::literal("obj"))) {
										return skipWhitespaces();
									}
								}
							}
						}
					}
					return sl_false;
				}

				void decrypt(const PdfReference& ref, void* buf, sl_size size)
				{
					RC4 rc;
					sl_uint8 key[21];
					sl_uint32 l = context->lenEncryptionKey;
					Base::copyMemory(key, context->encryptionKey, l);
					MIO::writeUint24LE(key + l, ref.objectNumber);
					MIO::writeUint16LE(key + l + 3, (sl_uint16)(ref.generation));
					l += 5;
					sl_uint8 h[16];
					MD5::hash(key, l, h);
					if (l > 16) {
						l = 16;
					}
					rc.setKey(h, l);
					rc.encrypt(buf, buf, size);
				}

				PdfObject readObject(PdfReference& outRef)
				{
					if (readObjectHeader(outRef)) {
						PdfObject obj = readValue();
						if (obj.isNotUndefined()) {
							if (skipWhitespaces()) {
								const PdfDictionary& properties = obj.getDictionary();
								if (properties.isNotNull()) {
									if (peekCharAndEquals('s')) {
										sl_uint32 length;
										if (!(getStreamLength(properties, length))) {
											return PdfObject();
										}
										Memory content;
										if (!(readStreamContent(length, content))) {
											return PdfObject();
										}
										if (!(skipWhitespaces())) {
											return PdfObject();
										}
										Ref<PdfStream> stream = new PdfStream;
										if (stream.isNull()) {
											return PdfObject();
										}
										if (context->flagEncrypt) {
											decrypt(outRef, content.getData(), content.getSize());
										}
										stream->properties = Move(properties);
										stream->content = Move(content);
										obj = PdfObject(Move(stream));
									}
								} else {
									if (context->flagEncrypt) {
										const String& str = obj.getString();
										if (str.isNotNull()) {
											decrypt(outRef, str.getData(), str.getLength());
										}
									}
								}
								if (readWordAndEquals(StringView::literal("endobj"))) {
									return obj;
								}
							}
						}
					}
					return PdfObject();
				}

				PdfObject getObject(const PdfReference& ref)
				{
					sl_uint64 _id = GetObjectId(ref);
					PdfObject ret;
					if (context->objects.get(_id, &ret)) {
						return ret;
					}
					CrossReferenceEntry entry;
					if (getReference(ref, entry)) {
						if (entry.type == CrossReferenceEntryType::Normal) {
							if (setPosition(entry.offset)) {
								PdfReference n;
								ret = readObject(n);
								if (ret.isNotUndefined() && ref == n) {
									context->objects.put(_id, ret);
									return ret;
								}
							}
						} else if (entry.type == CrossReferenceEntryType::Compressed) {
							Ref<ObjectStream> stream = getObjectStream(entry.streamObject);
							if (stream.isNotNull()) {
								sl_uint32 n;
								ret = stream->getItem(entry.objectIndex, n);
								if (ret.isNotUndefined() && _id == n) {
									context->objects.put(_id, ret);
									return ret;
								}
							}
						}
					}
					return PdfObject();
				}

				PdfObject getObject(const PdfObject& objRef)
				{
					PdfReference ref;
					if (objRef.getReference(ref)) {
						return getObject(ref);
					}
					return PdfObject();
				}

				Ref<ObjectStream> getObjectStream(const PdfReference& ref)
				{
					sl_uint64 _id = GetObjectId(ref);
					Ref<ObjectStream> stream;
					if (context->objectStreams.get(_id, &stream)) {
						return stream;
					}
					Ref<PdfStream> obj = getObject(ref).getStream();
					if (obj.isNotNull()) {
						if (obj->getProperty(g_strType).equalsName(StringView::literal("ObjStm"))) {
							sl_uint32 nObjects;
							if (obj->getProperty(g_strN).getUint(nObjects)) {
								sl_uint32 first;
								if (obj->getProperty(g_strFirst).getUint(first)) {
									Memory content = obj->getOriginalContent();
									if (content.isNotNull()) {
										stream = new ObjectStream;
										if (stream.isNotNull()) {
											PdfObject objExtends = obj->getProperty(g_strExtends);
											if (objExtends.isNotUndefined()) {
												PdfReference refExtends;
												if (!(objExtends.getReference(refExtends))) {
													return sl_null;
												}
												if (refExtends == ref) {
													return sl_null;
												}
												stream->extends = getObjectStream(refExtends);
												if (stream->extends.isNull()) {
													return sl_null;
												}
											}
											Parser<MemoryParserBase> parser;
											parser.buf = (sl_char8*)(content.getData());
											parser.size = (sl_uint32)(content.getSize());
											parser.context = context;
											for (sl_uint32 i = 0; i < nObjects; i++) {
												if (!(parser.skipWhitespaces())) {
													return sl_null;
												}
												sl_uint32 innerId;
												if (!(parser.readUint(innerId))) {
													return sl_null;
												}
												if (!(parser.skipWhitespaces())) {
													return sl_null;
												}
												sl_uint32 offset;
												if (!(parser.readUint(offset))) {
													return sl_null;
												}
												sl_size pos = parser.getPosition();
												if (!(parser.setPosition(first + offset))) {
													return sl_null;
												}
												PdfObject innerObj = parser.readValue();
												if (innerObj.isUndefined()) {
													return sl_null;
												}
												stream->objects.add_NoLock(innerId, Move(innerObj));
												parser.setPosition(pos);
											}
											context->objectStreams.put(_id, stream);
											return stream;
										}
									}
								}
							}
						}
					}
					return sl_null;
				}

			};

			typedef Parser<BufferedParserBase> BufferedParser;
			typedef Parser<MemoryParserBase> MemoryParser;

			SLIB_INLINE static BufferedParser* GetParser(const Ref<Referable>& ref)
			{
				return (CRefT<BufferedParser>*)(ref.get());
			}

			class DocumentHelper
			{
			public:
				static sl_bool readCrossReferenceEntry(BufferedParser* parser, CrossReferenceEntry& entry)
				{
					if (parser->readUint(entry.offset)) {
						if (parser->skipWhitespaces()) {
							if (parser->readUint(entry.generation)) {
								if (parser->skipWhitespaces()) {
									sl_char8 ch;
									if (parser->readChar(ch)) {
										if (ch == 'f') {
											entry.type = CrossReferenceEntryType::Free;
										} else if (ch == 'n') {
											entry.type = CrossReferenceEntryType::Normal;
										} else {
											return sl_false;
										}
										if (parser->peekChar(ch)) {
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

				static sl_bool readCrossReferenceSection(BufferedParser* parser, CrossReferenceTable& table)
				{
					sl_uint32 firstObjectNumber;
					if (parser->readUint(firstObjectNumber)) {
						if (parser->skipWhitespaces()) {
							sl_uint32 count;
							if (parser->readUint(count)) {
								for (sl_uint32 i = 0; i < count; i++) {
									if (!(parser->skipWhitespaces())) {
										return sl_false;
									}
									CrossReferenceEntry entry;
									if (!(readCrossReferenceEntry(parser, entry))) {
										return sl_false;
									}
									if (entry.type == CrossReferenceEntryType::Normal) {
										table.context->references.put_NoLock(MAKE_OBJECT_ID(firstObjectNumber + i, entry.generation), entry);
									}
								}
								return sl_true;
							}
						}
					}
					return sl_false;
				}

				static sl_bool readCrossReferenceTable(BufferedParser* parser, CrossReferenceTable& table)
				{
					sl_char8 ch;
					if (!(parser->peekChar(ch))) {
						return sl_false;
					}
					if (ch == 'x') {
						// cross reference table
						if (parser->readWordAndEquals(StringView::literal("xref"))) {
							for (;;) {
								if (!(parser->skipWhitespaces())) {
									return sl_false;
								}
								if (!(parser->peekChar(ch))) {
									break;
								}
								if (ch == 't') {
									if (parser->readWordAndEquals(StringView::literal("trailer"))) {
										if (!(parser->skipWhitespaces())) {
											return sl_false;
										}
										table.trailer = parser->readDictionary();
										return table.trailer.isNotNull();
									}
									break;
								} else if (ch >= '0' && ch <= '9') {
									if (!(readCrossReferenceSection(parser, table))) {
										return sl_false;
									}
								} else {
									break;
								}
							}
							return sl_true;
						}
					} else {
						// cross reference stream
						PdfReference refStream;
						const Ref<PdfStream> stream = parser->readObject(refStream).getStream();
						if (stream.isNotNull()) {
							if (stream->getProperty(g_strType).equalsName(StringView::literal("XRef"))) {
								sl_uint32 size;
								if (stream->getProperty(g_strSize).getUint(size)) {
									ListElements<PdfObject> entrySizes(stream->getProperty(g_strW).getArray());
									if (entrySizes.count == 3) {
										if (entrySizes[0].getType() == PdfObjectType::Uint && entrySizes[1].getType() == PdfObjectType::Uint && entrySizes[2].getType() == PdfObjectType::Uint) {
											CList< Pair<sl_uint32, sl_uint32> > listSectionRanges;
											sl_size nEntries = 0;
											PdfObject objIndex = stream->getProperty(g_strIndex);
											if (objIndex.isNotUndefined()) {
												if (objIndex.getType() != PdfObjectType::Array) {
													return sl_false;
												}
												ListElements<PdfObject> indices(objIndex.getArray());
												if (indices.count & 1) {
													return sl_false;
												}
												for (sl_size i = 0; i < indices.count; i += 2) {
													sl_uint32 start, count;
													if (!(indices[i].getUint(start))) {
														return sl_false;
													}
													if (!(indices[i + 1].getUint(count))) {
														return sl_false;
													}
													listSectionRanges.add_NoLock(start, count);
													nEntries += count;
												}
											} else {
												listSectionRanges.add_NoLock(0, size);
												nEntries = size;
											}
											sl_uint32 sizeType = entrySizes[0].getUint();
											sl_uint32 sizeOffset = entrySizes[1].getUint();
											sl_uint32 sizeGeneration = entrySizes[2].getUint();
											sl_uint32 sizeEntry = sizeType + sizeOffset + sizeGeneration;
											Memory content = stream->getOriginalContent();
											if (content.getSize() >= sizeEntry * nEntries) {
												sl_uint8* p = (sl_uint8*)(content.getData());
												ListElements< Pair<sl_uint32, sl_uint32> > sectionRanges(listSectionRanges);
												for (sl_size iSection = 0; iSection < sectionRanges.count; iSection++) {
													Pair<sl_uint32, sl_uint32>& range = sectionRanges[iSection];
													for (sl_uint32 i = 0; i < range.second; i++) {
														CrossReferenceEntry entry;
														sl_uint32 type = ReadUint(p, sizeType, 1);
														if (type > 2) {
															return sl_false;
														}
														entry.type = (CrossReferenceEntryType)type;
														p += sizeType;
														entry.offset = ReadUint(p, sizeOffset, 0);
														p += sizeOffset;
														entry.generation = ReadUint(p, sizeGeneration, 0);
														p += sizeGeneration;
														if (entry.type == CrossReferenceEntryType::Normal) {
															table.context->references.add_NoLock(MAKE_OBJECT_ID(range.first + i, entry.generation), entry);
														} else if (entry.type == CrossReferenceEntryType::Compressed) {
															table.context->references.add_NoLock(range.first + i, entry);
														}
													}
												}
												table.trailer = stream->properties;
												return sl_true;
											}
										}

									}
								}
							}
						}
					}
					return sl_false;
				}

				static sl_bool readStartXref(BufferedParser* parser, sl_uint32& posXref)
				{
					sl_reg pos = parser->findBackward(StringView::literal("startxref"), 4096);
					if (pos > 0) {
						if (parser->setPosition(pos - 1)) {
							sl_char8 ch;
							if (parser->readChar(ch)) {
								if (IsWhitespace(ch)) {
									if (parser->readWordAndEquals(StringView::literal("startxref"))) {
										if (parser->skipWhitespaces()) {
											if (parser->readUint(posXref)) {
												return sl_true;
											}
										}
									}
								}
							}
						}
					}
					return sl_false;
				}

				static sl_bool readDocument(BufferedParser* parser, PdfDocument& doc)
				{
					sl_char8 version[8];
					if (parser->read(version, 8) != 8) {
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

					sl_uint32 posXref;
					if (!(readStartXref(parser, posXref))) {
						return sl_false;
					}
					if (parser->setPosition(posXref)) {
						CrossReferenceTable xrefTable;
						xrefTable.context = parser->context;
						if (readCrossReferenceTable(parser, xrefTable)) {
							doc.lastTrailer = xrefTable.trailer;
							if (doc.lastTrailer.isNull()) {
								return sl_false;
							}
							PdfObject prev = xrefTable.trailer.getValue_NoLock(g_strPrev);
							while (prev.isNotUndefined()) {
								sl_uint32 posPrev;
								if (!(prev.getUint(posPrev))) {
									return sl_false;
								}
								if (!(parser->setPosition(posPrev))) {
									return sl_false;
								}
								CrossReferenceTable subTable;
								subTable.context = parser->context;
								if (!(readCrossReferenceTable(parser, subTable))) {
									return sl_false;
								}
								prev = subTable.trailer.getValue_NoLock(g_strPrev);
							}
							doc.catalog = parser->getObject(doc.lastTrailer.getValue_NoLock(g_strRoot)).getDictionary();
							if (doc.catalog.isNull()) {
								return sl_false;
							}
							doc.encrypt = parser->getObject(doc.lastTrailer.getValue_NoLock(g_strEncrypt)).getDictionary();
							PdfDictionary pageTree = parser->getObject(doc.catalog.getValue_NoLock(g_strPages)).getDictionary();
							if (pageTree.isNull()) {
								return sl_false;
							}
							parser->context->pageTree.properties = pageTree;
							return sl_true;
						}
					}
					return sl_false;
				}

				static List<PageTree> buildPageTreeKids(BufferedParser* parser, const PdfDictionary& tree)
				{
					List<PageTree> ret;
					ListElements<PdfObject> kidIds(tree.getValue_NoLock(g_strKids).getArray());
					for (sl_size i = 0; i < kidIds.count; i++) {
						PdfDictionary props = parser->getObject(kidIds[i]).getDictionary();
						PageTree item;
						item.flagPage = props.getValue_NoLock(g_strType).equalsName(StringView::literal("Page"));
						item.properties = Move(props);
						ret.add_NoLock(Move(item));
					}
					return ret;
				}

				static Memory getPageContent(BufferedParser* parser, const PdfObject& contents)
				{
					PdfArray array = contents.getArray();
					if (array.isNotNull()) {
						MemoryBuffer buf;
						ListElements<PdfObject> items(array);
						for (sl_size i = 0; i < items.count; i++) {
							buf.add(parser->getObject(items[i]).getStreamContent());
						}
						return buf.merge();
					} else {
						return parser->getObject(contents).getStreamContent();
					}
				}

				static PdfDictionary getPage(BufferedParser* parser, PageTree& tree, sl_uint32 index, Memory* pOutContent)
				{
					if (index >= tree.getPagesCount()) {
						return sl_null;
					}
					if (!(tree.flagKids)) {
						tree.kids = buildPageTreeKids(parser, tree.properties);
						tree.flagKids = sl_true;
					}
					sl_uint32 n = 0;
					ListElements<PageTree> kids(tree.kids);
					for (sl_size i = 0; i < kids.count; i++) {
						PageTree& item = kids[i];
						sl_uint32 m = item.getPagesCount();
						if (index < n + m) {
							if (item.flagPage) {
								if (pOutContent) {
									*pOutContent = getPageContent(parser, item.properties.getValue_NoLock(g_strContents));
								}
								return item.properties;
							} else {
								return getPage(parser, item, index - n, pOutContent);
							}
						}
						n += m;
					}
					return sl_null;
				}

				static PdfDictionary getPage(BufferedParser* parser, sl_uint32 index, Memory* pOutContent)
				{
					return getPage(parser, parser->context->pageTree, index, pOutContent);
				}

				static Memory applyFilter(const Memory& input, const StringView& filter)
				{
					if (filter == StringView::literal("FlateDecode") || filter == StringView::literal("Fl")) {
						return Zlib::decompress(input.getData(), input.getSize());
					} else if (filter == StringView::literal("ASCIIHexDecode") || filter == StringView::literal("AHx")) {
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
					} else if (filter == StringView::literal("ASCII85Decode") || filter == StringView::literal("A85")) {
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

				static void computeEncryptionKey(sl_uint8* outKey, sl_uint32 lenKey, const StringView& password, sl_uint32 revision, const String& ownerHash, sl_uint32 permission, const String& fileId)
				{
					MD5 hash;
					hash.start();
					sl_size lenPassword = password.getLength();
					if (lenPassword > 32) {
						lenPassword = 32;
					}
					if (lenPassword) {
						hash.update(password.getData(), lenPassword);
					}
					if (lenPassword < 32) {
						hash.update(g_encryptionPad, 32 - lenPassword);
					}
					hash.update(ownerHash.getData(), ownerHash.getLength());
					sl_uint8 bufPermission[4];
					MIO::writeUint32LE(bufPermission, permission);
					hash.update(bufPermission, 4);
					if (fileId.isNotEmpty()) {
						hash.update(fileId.getData(), fileId.getLength());
					}
					if (revision >= 4) {
						static sl_uint8 k[] = { 0xff, 0xff, 0xff, 0xff };
						hash.update(k, 4);
					}
					sl_uint8 h[16];
					hash.finish(h);
					if (lenKey > 16) {
						lenKey = 16;
					}
					if (revision >= 3) {
						for (sl_uint32 i = 0; i < 50; i++) {
							MD5::hash(h, lenKey, h);
						}
					}
					Base::copyMemory(outKey, h, lenKey);
				}

				static void computeUserPasswordHash(sl_uint8* outHash, const sl_uint8* encryptionKey, sl_uint32 lengthKey, sl_uint32 revision, const String& fileId)
				{
					if (revision >= 3) {
						MD5 hash;
						hash.start();
						hash.update(g_encryptionPad, 32);
						if (fileId.isNotEmpty()) {
							hash.update(fileId.getData(), fileId.getLength());
						}
						sl_uint8 h[16];
						hash.finish(h);
						RC4 rc;
						rc.setKey(encryptionKey, lengthKey);
						rc.encrypt(h, outHash, 16);
						for (sl_uint8 i = 1; i <= 19; i++) {
							sl_uint8 k[16];
							for (sl_uint32 j = 0; j < lengthKey; j++) {
								k[j] = encryptionKey[j] ^ i;
							}
							rc.setKey(k, lengthKey);
							rc.encrypt(outHash, outHash, 16);
						}
					} else {
						RC4 rc;
						rc.setKey(encryptionKey, lengthKey);
						rc.encrypt(g_encryptionPad, outHash, 32);
					}
				}

			};

		}
	}

	using namespace priv::pdf;


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(PdfName)

	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(PdfObject)
	
	PdfObject::PdfObject(sl_bool v) noexcept: m_var(v, (sl_uint8)(PdfObjectType::Boolean)) {}

	PdfObject::PdfObject(sl_int32 v) noexcept: m_var(v, (sl_uint8)(PdfObjectType::Int)) {}
	PdfObject::PdfObject(sl_uint32 v) noexcept: m_var(v, (sl_uint8)(PdfObjectType::Uint)) {}
	PdfObject::PdfObject(float v) noexcept: m_var(v, (sl_uint8)(PdfObjectType::Float)) {}

	PdfObject::PdfObject(const String& v) noexcept: m_var(v, (sl_uint8)(v.isNotNull() ? PdfObjectType::String : PdfObjectType::Null)) {}
	PdfObject::PdfObject(String&& v) noexcept: m_var(Move(v), (sl_uint8)(v.isNotNull() ? PdfObjectType::String : PdfObjectType::Null)) {}

	PdfObject::PdfObject(const PdfArray& v) noexcept: m_var(*((VariantList*)((void*)&v)), (sl_uint8)(v.isNotNull() ? PdfObjectType::Array : PdfObjectType::Null)) {}
	PdfObject::PdfObject(PdfArray&& v) noexcept: m_var(Move(*((VariantList*)((void*)&v))), (sl_uint8)(v.isNotNull() ? PdfObjectType::Array : PdfObjectType::Null)) {}

	PdfObject::PdfObject(const PdfDictionary& v) noexcept: m_var(*((VariantMap*)((void*)&v)), (sl_uint8)(v.isNotNull() ? PdfObjectType::Dictionary : PdfObjectType::Null)) {}
	PdfObject::PdfObject(PdfDictionary&& v) noexcept: m_var(Move(*((VariantMap*)((void*)&v))), (sl_uint8)(v.isNotNull() ? PdfObjectType::Dictionary : PdfObjectType::Null)) {}

	PdfObject::PdfObject(const Ref<PdfStream>& v) noexcept: m_var(v, (sl_uint8)(v.isNotNull() ? PdfObjectType::Stream : PdfObjectType::Null)) {}
	PdfObject::PdfObject(Ref<PdfStream>&& v) noexcept: m_var(Move(v), (sl_uint8)(v.isNotNull() ? PdfObjectType::Stream : PdfObjectType::Null)) {}

	PdfObject::PdfObject(const PdfName& v) noexcept: m_var(v.value, (sl_uint8)(v.isNotNull() ? PdfObjectType::Name : PdfObjectType::Null)) {}
	PdfObject::PdfObject(PdfName&& v) noexcept : m_var(Move(v.value), (sl_uint8)(v.isNotNull() ? PdfObjectType::Name : PdfObjectType::Null)) {}

	PdfObject::PdfObject(const PdfReference& v) noexcept: m_var(MAKE_OBJECT_ID(v.objectNumber, v.generation), (sl_uint8)(PdfObjectType::Reference)) {}

	sl_bool PdfObject::getBoolean() const noexcept
	{
		if (getType() == PdfObjectType::Boolean) {
			return m_var._m_boolean;
		}
		return sl_false;
	}

	sl_bool PdfObject::getBoolean(sl_bool& _out) const noexcept
	{
		if (getType() == PdfObjectType::Boolean) {
			_out = m_var._m_boolean;
			return sl_true;
		}
		return sl_false;
	}

	sl_uint32 PdfObject::getUint() const noexcept
	{
		sl_uint32 ret;
		if (getUint(ret)) {
			return ret;
		}
		return 0;
	}

	sl_bool PdfObject::getUint(sl_uint32& _out) const noexcept
	{
		PdfObjectType type = getType();
		if (type == PdfObjectType::Uint) {
			_out = m_var._m_uint32;
			return sl_true;
		} else if (type == PdfObjectType::Int) {
			sl_int32 n = m_var._m_int32;
			if (n >= 0) {
				_out = n;
				return sl_true;
			}
		}
		return sl_false;
	}

	sl_int32 PdfObject::getInt() const noexcept
	{
		sl_int32 ret;
		if (getInt(ret)) {
			return ret;
		}
		return 0;
	}

	sl_bool PdfObject::getInt(sl_int32& _out) const noexcept
	{
		PdfObjectType type = getType();
		if (type == PdfObjectType::Int || type == PdfObjectType::Uint) {
			_out = m_var._m_int32;
			return sl_true;
		}
		return sl_false;
	}

	float PdfObject::getFloat() const noexcept
	{
		float ret;
		if (getFloat(ret)) {
			return ret;
		}
		return 0;
	}

	sl_bool PdfObject::getFloat(float& _out) const noexcept
	{
		PdfObjectType type = getType();
		if (type == PdfObjectType::Float) {
			_out = m_var._m_float;
		} else if (type == PdfObjectType::Uint) {
			_out = (float)(m_var._m_uint32);
			return sl_true;
		} else if (type == PdfObjectType::Int) {
			_out = (float)(m_var._m_int32);
			return sl_true;
		}
		return sl_false;
	}

	sl_bool PdfObject::isNumeric() const noexcept
	{
		PdfObjectType type = getType();
		return type == PdfObjectType::Int || type == PdfObjectType::Uint || type == PdfObjectType::Float;
	}

	const String& PdfObject::getString() const noexcept
	{
		if (getType() == PdfObjectType::String) {
			return *((String*)((void*)(&m_var._value)));
		}
		return String::null();
	}

	const String& PdfObject::getName() const noexcept
	{
		if (getType() == PdfObjectType::Name) {
			return *((String*)((void*)(&m_var._value)));
		}
		return String::null();
	}

	sl_bool PdfObject::equalsName(const StringView& name) const noexcept
	{
		if (getType() == PdfObjectType::Name) {
			return *((String*)((void*)(&m_var._value))) == name;
		}
		return sl_false;
	}

	const PdfArray& PdfObject::getArray() const noexcept
	{
		if (getType() == PdfObjectType::Array) {
			return *((PdfArray*)((void*)(&m_var._value)));
		}
		return PdfArray::null();
	}

	const PdfDictionary& PdfObject::getDictionary() const noexcept
	{
		if (getType() == PdfObjectType::Dictionary) {
			return *((PdfDictionary*)((void*)(&m_var._value)));
		}
		return PdfDictionary::null();
	}

	const Ref<PdfStream>& PdfObject::getStream() const noexcept
	{
		if (getType() == PdfObjectType::Stream) {
			return *((Ref<PdfStream>*)((void*)(&m_var._value)));
		}
		return Ref<PdfStream>::null();
	}

	Memory PdfObject::getStreamContent() const noexcept
	{
		if (getType() == PdfObjectType::Stream) {
			return (*((Ref<PdfStream>*)((void*)(&m_var._value))))->getOriginalContent();
		}
		return sl_null;
	}

	PdfReference PdfObject::getReference() const noexcept
	{
		PdfReference ret;
		if (getType() == PdfObjectType::Reference) {
			ret.objectNumber = SLIB_GET_DWORD0(m_var._value);
			ret.generation = SLIB_GET_DWORD1(m_var._value);
		} else {
			ret.objectNumber = 0;
			ret.generation = 0;
		}
		return ret;
	}

	sl_bool PdfObject::getReference(PdfReference& _out) const noexcept
	{
		if (getType() == PdfObjectType::Reference) {
			_out.objectNumber = SLIB_GET_DWORD0(m_var._value);
			_out.generation = SLIB_GET_DWORD1(m_var._value);
			return sl_true;
		}
		return sl_false;
	}


	SLIB_DEFINE_ROOT_OBJECT(PdfStream)

	PdfStream::PdfStream() noexcept
	{
	}

	PdfStream::~PdfStream()
	{
	}

	PdfObject PdfStream::getProperty(const String& name) noexcept
	{
		return properties.getValue_NoLock(name);
	}

	Memory PdfStream::getOriginalContent() noexcept
	{
		PdfObject objFilter = getProperty(g_strFilter);
		if (objFilter.isUndefined()) {
			return content;
		}
		const PdfArray& arrayFilter = objFilter.getArray();
		if (arrayFilter.isNotNull()) {
			Memory ret = content;
			ListElements<PdfObject> filters(arrayFilter);
			for (sl_size i = 0; i < filters.count; i++) {
				String filter = filters[i].getName();
				if (filter.isNotNull()) {
					ret = DocumentHelper::applyFilter(ret, filter);
				} else {
					return sl_null;
				}
			}
			return ret;
		} else {
			String filter = objFilter.getName();
			if (filter.isNotNull()) {
				return DocumentHelper::applyFilter(content, filter);
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
				Ref<Context> context = new Context;
				if (context.isNotNull()) {
					RefT<BufferedParser> parser = NewRefT<BufferedParser>();
					if (parser.isNotNull()) {
						parser->context = Move(context);
						if (parser->reader.open(reader)) {
							if (DocumentHelper::readDocument(parser.get(), *this)) {
								m_parser = Move(parser);
								if (isEncrypted()) {
									setUserPassword(sl_null);
								}
								return sl_true;
							}
						}
					}
				}
			}
		}
		return sl_false;
	}

	PdfObject PdfDocument::getObject(const PdfReference& ref)
	{
		BufferedParser* parser = GetParser(m_parser);
		if (parser) {
			return parser->getObject(ref);
		}
		return PdfObject();
	}

	sl_uint32 PdfDocument::getPagesCount()
	{
		BufferedParser* parser = GetParser(m_parser);
		if (parser) {
			return parser->context->pageTree.getPagesCount();
		}
		return 0;
	}

	PdfDictionary PdfDocument::getPage(sl_uint32 index, Memory* pOutContent)
	{
		BufferedParser* parser = GetParser(m_parser);
		if (parser) {
			return DocumentHelper::getPage(parser, index, pOutContent);
		}
		return sl_null;
	}

	sl_bool PdfDocument::isEncrypted()
	{
		return encrypt.isNotNull();
	}

	sl_bool PdfDocument::isAuthenticated()
	{
		if (isEncrypted()) {
			BufferedParser* parser = GetParser(m_parser);
			if (parser) {
				return parser->context->flagEncrypt;
			}
			return sl_false;
		} else {
			return sl_true;
		}
	}

	sl_bool PdfDocument::isEncryptedFile(const StringParam& path)
	{
		PdfDocument doc;
		if (doc.openFile(path)) {
			return doc.isEncrypted();
		}
		return sl_false;
	}

	sl_bool PdfDocument::setUserPassword(const StringView& password)
	{
		BufferedParser* parser = GetParser(m_parser);
		if (parser) {
			if (encrypt.getValue_NoLock(g_strFilter).equalsName(StringView::literal("Standard"))) {
				sl_uint32 encryptionAlgorithm = encrypt.getValue_NoLock(g_strV).getUint();
				if (encryptionAlgorithm == 1) {
					sl_uint32 lengthKey = encrypt.getValue_NoLock(g_strLength).getUint();
					if (!lengthKey) {
						lengthKey = 40;
					}
					if (lengthKey & 7) {
						return sl_false;
					}
					lengthKey >>= 3;
					if (lengthKey > 16) {
						return sl_false;
					}
					String userHash = encrypt.getValue_NoLock(g_strU).getString();
					if (userHash.getLength() != 32) {
						return sl_false;
					}
					sl_uint32 revision = encrypt.getValue_NoLock(g_strR).getUint();
					sl_uint32 permission = encrypt.getValue_NoLock(g_strP).getInt();
					String ownerHash = encrypt.getValue_NoLock(g_strO).getString();
					String fileId = lastTrailer.getValue_NoLock(g_strID).getArray().getValueAt_NoLock(0).getString();
					sl_uint8 key[16];
					DocumentHelper::computeEncryptionKey(key, lengthKey, password, revision, ownerHash, permission, fileId);
					sl_uint8 userHashGen[32];
					DocumentHelper::computeUserPasswordHash(userHashGen, key, lengthKey, revision, fileId);
					if (Base::equalsMemory(userHashGen, userHash.getData(), 16)) {
						parser->context->flagEncrypt = sl_true;
						Base::copyMemory(parser->context->encryptionKey, key, lengthKey);
						parser->context->lenEncryptionKey = lengthKey;
						return sl_true;
					}
				}
			}
		}
		return sl_false;
	}

}
