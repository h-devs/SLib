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
			SLIB_STATIC_STRING(g_strMediaBox, "MediaBox")
			SLIB_STATIC_STRING(g_strCropBox, "CropBox")
			SLIB_STATIC_STRING(g_strResources, "Resources")
			SLIB_STATIC_STRING(g_strXObject, "XObject")
			SLIB_STATIC_STRING(g_strFont, "Font")
			SLIB_STATIC_STRING(g_strSubtype, "Subtype")
			SLIB_STATIC_STRING(g_strFontDescriptor, "FontDescriptor")
			SLIB_STATIC_STRING(g_strFontName, "FontName")
			SLIB_STATIC_STRING(g_strFontFamily, "FontFamily")
			SLIB_STATIC_STRING(g_strAscent, "Ascent")
			SLIB_STATIC_STRING(g_strDescent, "Descent")
			SLIB_STATIC_STRING(g_strLeading, "Leading")
			SLIB_STATIC_STRING(g_strFontWeight, "FontWeight")
			SLIB_STATIC_STRING(g_strItalicAngle, "ItalicAngle")
			SLIB_STATIC_STRING(g_strFontFile, "FontFile")
			SLIB_STATIC_STRING(g_strFontFile2, "FontFile2")
			SLIB_STATIC_STRING(g_strFontFile3, "FontFile3")
			SLIB_STATIC_STRING(g_strFirstChar, "FirstChar")
			SLIB_STATIC_STRING(g_strLastChar, "LastChar")
			SLIB_STATIC_STRING(g_strWidths, "Widths")
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

			static Memory ApplyFilter(const Memory& input, const StringView& filter)
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

			static void ComputeEncryptionKey(sl_uint8* outKey, sl_uint32 lenKey, const StringView& password, sl_uint32 revision, const String& ownerHash, sl_uint32 permission, const String& fileId)
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

			static void ComputeUserPasswordHash(sl_uint8* outHash, const sl_uint8* encryptionKey, sl_uint32 lengthKey, sl_uint32 revision, const String& fileId)
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

			enum class CrossReferenceEntryType
			{
				Free = 0,
				Normal = 1,
				Compressed = 2
			};

			struct CrossReferenceEntry
			{
				union {
					sl_uint32 nextFreeObject; // For free entry
					sl_uint32 offset; // For normal entry: 10 digits in Cross-Reference-Table
					sl_uint32 streamObject; // For compressed entry
				};
				CrossReferenceEntryType type : 2;
				sl_uint32 generation : 30; // For free, normal entry: 5 digits in Cross-Reference-Table. For compressed entry, objectIndex
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

			class PageTreeParent : public PdfPageTreeItem
			{
			public:
				List< Ref<PdfPageTreeItem> > kids;
				sl_bool flagKids = sl_false;
				sl_uint32 count = 0;
				sl_bool flagCount = sl_false;

			public:
				sl_uint32 getPagesCount()
				{
					if (flagCount) {
						return count;
					}
					count = attributes.getValue_NoLock(g_strCount).getUint();
					flagCount = sl_true;
					return count;
				}

			};

			class Context : public Referable
			{
			public:
				Array<CrossReferenceEntry> references;
				ExpiringMap<sl_uint64, PdfObject> objects;
				ExpiringMap< sl_uint64, Ref<ObjectStream> > objectStreams;

			public:
				Context()
				{
					objects.setExpiringMilliseconds(EXPIRE_DURATION_OBJECT);
					objectStreams.setExpiringMilliseconds(EXPIRE_DURATION_OBJECT_STREAM);
				}

			public:
				sl_bool getReferenceEntry(sl_uint32 objectNumber, CrossReferenceEntry& entry)
				{
					return references.getAt(objectNumber, &entry);
				}

				void setReferenceEntry(sl_uint32 objectNumber, const CrossReferenceEntry& entry)
				{
					CrossReferenceEntry* pEntry = references.getPointerAt(objectNumber);
					if (pEntry) {
						if (pEntry->type == CrossReferenceEntryType::Free && entry.type != CrossReferenceEntryType::Free) {
							*pEntry = entry;
						}
					}
				}

			};

			class CrossReferenceTable
			{
			public:
				Ref<Context> context;
				PdfDictionary trailer;
			};

			class ParserBase : public Referable
			{
			public:
				Ref<Context> context;

				sl_uint8 majorVersion = 0;
				sl_uint8 minorVersion = 0;

				PdfDictionary lastTrailer;
				PdfDictionary encrypt;
				PdfDictionary catalog;
				Ref<PageTreeParent> pageTree;

				sl_bool flagDecryptContents = sl_false;
				sl_uint8 encryptionKey[16];
				sl_uint32 lenEncryptionKey = 0;

			public:
				virtual sl_bool readDocument() = 0;
				virtual Ref<PdfPage> getPage(sl_uint32 index) = 0;
				virtual Memory getPageContent(const PdfObject& contents) = 0;
				virtual PdfObject getObject(const PdfReference& ref) = 0;
				virtual PdfObject getObject(const PdfObject& obj) = 0;

			public:
				sl_bool setUserPassword(const StringView& password)
				{
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
							ComputeEncryptionKey(key, lengthKey, password, revision, ownerHash, permission, fileId);
							sl_uint8 userHashGen[32];
							ComputeUserPasswordHash(userHashGen, key, lengthKey, revision, fileId);
							if (Base::equalsMemory(userHashGen, userHash.getData(), 16)) {
								flagDecryptContents = sl_true;
								Base::copyMemory(encryptionKey, key, lengthKey);
								lenEncryptionKey = lengthKey;
								return sl_true;
							}
						}
					}
					return sl_false;
				}

			};

			class BufferedReaderBase
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

				sl_reg findBackward(const StringView& str, sl_reg startFind, sl_size sizeFind)
				{
					return (sl_reg)(reader.findBackward(str.getData(), str.getLength(), startFind, sizeFind));
				}

			};

			class MemoryReaderBase
			{
			public:
				sl_char8* source;
				sl_uint32 sizeSource;
				Ref<Referable> refSource;

			public:
				sl_bool readChar(sl_char8& ch)
				{
					if (pos < sizeSource) {
						ch = source[pos++];
						return sl_true;
					} else {
						return sl_false;
					}
				}

				sl_bool peekChar(sl_char8& ch)
				{
					if (pos < sizeSource) {
						ch = source[pos];
						return sl_true;
					}
					return sl_false;
				}

				sl_reg read(void* _out, sl_size nOut)
				{
					if (pos < sizeSource) {
						if (pos + nOut > sizeSource) {
							nOut = sizeSource - pos;
						}
						if (nOut) {
							Base::copyMemory(_out, source + pos, nOut);
							pos += (sl_uint32)nOut;
						}
						return nOut;
					}
					return SLIB_IO_ERROR;
				}

				sl_size getPosition()
				{
					return pos;
				}

				sl_bool setPosition(sl_size _pos)
				{
					if (_pos <= sizeSource) {
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
					sl_size _pos = pos;
					if (_pos < sizeSource) {
						_out = source + _pos;
						pos = sizeSource;
						return sizeSource - _pos;
					}
					return SLIB_IO_ERROR;
				}

				sl_reg findBackward(const StringView& str, sl_reg _startFind, sl_size sizeFind)
				{
					sl_char8* bufStart;
					sl_size startFind;
					if (_startFind >= 0) {
						startFind = _startFind;
						if (startFind > sizeSource) {
							return -1;
						}
					} else {
						startFind = sizeSource;
					}
					if (sizeFind >= startFind) {
						bufStart = source;
						sizeFind = startFind;
					} else {
						bufStart = source + startFind - sizeFind;
					}
					sl_char8* p = (sl_char8*)(Base::findMemoryBackward(bufStart, sizeFind, str.getData(), str.getLength()));
					if (p) {
						return p - source;
					} else {
						return -1;
					}
				}

			private:
				sl_uint32 pos = 0;

			};

			template <class READER_BASE>
			class Parser : public ParserBase, public READER_BASE
			{
			public:
				using READER_BASE::readChar;
				using READER_BASE::peekChar;
				using READER_BASE::read;
				using READER_BASE::getPosition;
				using READER_BASE::setPosition;
				using READER_BASE::movePosition;
				using READER_BASE::readBuffer;
				using READER_BASE::findBackward;

			public:
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

				sl_bool readCharAndIsWhitespace()
				{
					sl_char8 ch;
					if (readChar(ch)) {
						return IsWhitespace(ch);
					}
					return sl_false;
				}

				sl_size readWord(void* buf, sl_size nSizeLimit)
				{
					if (!nSizeLimit) {
						return 0;
					}
					sl_char8* word = (sl_char8*)buf;
					sl_size posWord = 0;
					for (;;) {
						sl_char8* buf;
						sl_reg n = readBuffer(buf);
						if (n > 0) {
							for (sl_reg i = 0; i < n; i++) {
								sl_char8 ch = buf[i];
								if (IsWhitespace(ch) || IsDelimiter(ch)) {
									movePosition(i - n);
									return posWord;
								} else {
									if (posWord >= nSizeLimit) {
										return sl_false;
									}
									word[posWord++] = ch;
								}
							}
						} else if (n == SLIB_IO_ENDED) {
							return posWord;
						} else {
							break;
						}
					}
					return 0;
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
											if (nOpen) {
												list.add_NoLock(')');
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
					return getObject(properties.getValue_NoLock(g_strLength)).getUint(_out);
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

				PdfObject readNumber()
				{
					sl_char8 ch;
					if (peekChar(ch)) {
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
					}
					return PdfObject();
				}

				PdfOperator readOperator()
				{
					sl_char8 buf[3];
					sl_size len = readWord(buf, 3);
					return PdfOperation::getOperator(StringView(buf, len));
				}

				PdfObject readValue()
				{
					sl_char8 ch;
					if (!(peekChar(ch))) {
						return PdfObject();
					}
					switch (ch) {
						case 'n':
							if (readWordAndEquals(StringView::literal("null"))) {
								return sl_null;
							}
							break;
						case 't':
							if (readWordAndEquals(StringView::literal("true"))) {
								return sl_true;
							}
							break;
						case 'f':
							if (readWordAndEquals(StringView::literal("false"))) {
								return sl_false;
							}
							break;
						case '(':
							{
								String s = readString();
								if (s.isNotNull()) {
									return s;
								}
							}
							break;
						case '<':
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
							break;
						case '/':
							{
								PdfName name = readName();
								if (name.isNotNull()) {
									return name;
								}
							}
							break;
						case '[':
							{
								PdfArray list = readArray();
								if (list.isNotNull()) {
									return list;
								}
							}
							break;
						default:
							if (IsNumeric(ch)) {
								return readNumber();
							} else {
								PdfOperator op = readOperator();
								if (op != PdfOperator::Unknown) {
									return op;
								}
							}
							break;
					}
					return PdfObject();
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
					sl_uint32 l = lenEncryptionKey;
					Base::copyMemory(key, encryptionKey, l);
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
										if (flagDecryptContents) {
											decrypt(outRef, content.getData(), content.getSize());
										}
										stream->properties = Move(properties);
										stream->content = Move(content);
										obj = PdfObject(Move(stream));
									}
								} else {
									if (flagDecryptContents) {
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

				PdfObject getObject(const PdfReference& ref) override
				{
					if (context.isNull()) {
						return PdfObject();
					}
					sl_uint64 _id = GetObjectId(ref);
					PdfObject ret;
					if (context->objects.get(_id, &ret)) {
						return ret;
					}
					CrossReferenceEntry entry;
					if (context->getReferenceEntry(ref.objectNumber, entry)) {
						if (entry.type == CrossReferenceEntryType::Normal) {
							if (entry.generation == ref.generation) {
								if (setPosition(entry.offset)) {
									PdfReference n;
									ret = readObject(n);
									if (ret.isNotUndefined() && ref == n) {
										context->objects.put(_id, ret);
										return ret;
									}
								}
							}
						} else if (entry.type == CrossReferenceEntryType::Compressed) {
							if (!(ref.generation)) {
								Ref<ObjectStream> stream = getObjectStream(entry.streamObject);
								if (stream.isNotNull()) {
									sl_uint32 n;
									ret = stream->getItem(entry.generation, n);
									if (ret.isNotUndefined() && _id == n) {
										context->objects.put(_id, ret);
										return ret;
									}
								}
							}
						}
					}
					return PdfObject();
				}

				PdfObject getObject(const PdfObject& obj) override
				{
					PdfReference ref;
					if (obj.getReference(ref)) {
						return getObject(ref);
					}
					return obj;
				}

				Ref<ObjectStream> getObjectStream(const PdfReference& ref)
				{
					if (context.isNull()) {
						return sl_null;
					}
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
											Parser<MemoryReaderBase> parser;
											parser.source = (sl_char8*)(content.getData());
											parser.sizeSource = (sl_uint32)(content.getSize());
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

				PdfDictionary readTrailer()
				{
					if (readWordAndEquals(StringView::literal("trailer"))) {
						if (skipWhitespaces()) {
							return readDictionary();
						}
					}
					return PdfDictionary();
				}

				sl_bool readCrossReferenceEntry(CrossReferenceEntry& entry)
				{
					if (readUint(entry.offset)) {
						if (skipWhitespaces()) {
							sl_uint32 gen;
							if (readUint(gen)) {
								entry.generation = gen;
								if (skipWhitespaces()) {
									sl_char8 ch;
									if (readChar(ch)) {
										if (ch == 'f') {
											entry.type = CrossReferenceEntryType::Free;
										} else if (ch == 'n') {
											entry.type = CrossReferenceEntryType::Normal;
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

				sl_bool readCrossReferenceSection(CrossReferenceTable& table)
				{
					sl_uint32 firstObjectNumber;
					if (readUint(firstObjectNumber)) {
						if (skipWhitespaces()) {
							sl_uint32 count;
							if (readUint(count)) {
								for (sl_uint32 i = 0; i < count; i++) {
									if (!(skipWhitespaces())) {
										return sl_false;
									}
									CrossReferenceEntry entry;
									if (!(readCrossReferenceEntry(entry))) {
										return sl_false;
									}
									table.context->setReferenceEntry(firstObjectNumber + i, entry);
								}
								return sl_true;
							}
						}
					}
					return sl_false;
				}

				sl_bool readCrossReferenceTable(CrossReferenceTable& table)
				{
					sl_char8 ch;
					if (!(peekChar(ch))) {
						return sl_false;
					}
					if (ch == 'x') {
						// cross reference table
						if (readWordAndEquals(StringView::literal("xref"))) {
							for (;;) {
								if (!(skipWhitespaces())) {
									return sl_false;
								}
								if (!(peekChar(ch))) {
									break;
								}
								if (ch == 't') {
									table.trailer = readTrailer();
									return table.trailer.isNotNull();
								} else if (ch >= '0' && ch <= '9') {
									if (!(readCrossReferenceSection(table))) {
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
						const Ref<PdfStream> stream = readObject(refStream).getStream();
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
														table.context->setReferenceEntry(range.first + i, entry);
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

				sl_bool readStartXref(sl_uint32& posStartXref, sl_uint32& posXref)
				{
					sl_reg pos = findBackward(StringView::literal("startxref"), -1, 4096);
					if (pos > 0) {
						if (setPosition(pos - 1)) {
							if (readCharAndIsWhitespace()) {
								if (readWordAndEquals(StringView::literal("startxref"))) {
									posStartXref = (sl_uint32)pos;
									if (skipWhitespaces()) {
										if (readUint(posXref)) {
											return sl_true;
										}
									}
								}
							}
						}
					}
					return sl_false;
				}

				PdfDictionary readLastTrailer(sl_uint32 startXref)
				{
					sl_reg pos = findBackward(StringView::literal("trailer"), startXref, 4096);
					if (pos > 0) {
						if (setPosition(pos - 1)) {
							if (readCharAndIsWhitespace()) {
								return readTrailer();
							}
						}
					}
					return PdfDictionary();
				}

				sl_bool readDocument() override
				{
					context = new Context;
					if (context.isNull()) {
						return sl_false;
					}
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

					majorVersion = SLIB_CHAR_DIGIT_TO_INT(version[5]);
					minorVersion = SLIB_CHAR_DIGIT_TO_INT(version[7]);

					// read last trailer and initialize reference table
					{
						sl_uint32 posStartXref, posXref;
						if (!(readStartXref(posStartXref, posXref))) {
							return sl_false;
						}
						lastTrailer = readLastTrailer(posStartXref);
						if (lastTrailer.isNull()) {
							return sl_false;
						}

						sl_uint32 countTotalRef = 0;
						lastTrailer.getValue_NoLock(g_strSize).getUint(countTotalRef);
						if (!countTotalRef) {
							return sl_false;
						}
						context->references = Array<CrossReferenceEntry>::create(countTotalRef);
						if (context->references.isNull()) {
							return sl_false;
						}
						Base::zeroMemory(context->references.getData(), countTotalRef * sizeof(CrossReferenceEntry));

						for (;;) {
							if (!(setPosition(posXref))) {
								return sl_false;
							}
							CrossReferenceTable subTable;
							subTable.context = context;
							if (!(readCrossReferenceTable(subTable))) {
								return sl_false;
							}
							PdfObject prev = subTable.trailer.getValue_NoLock(g_strPrev);
							if (prev.isUndefined()) {
								break;
							}
							if (!(prev.getUint(posXref))) {
								return sl_false;
							}
						}
					}

					catalog = getObject(lastTrailer.getValue_NoLock(g_strRoot)).getDictionary();
					if (catalog.isNull()) {
						return sl_false;
					}
					encrypt = getObject(lastTrailer.getValue_NoLock(g_strEncrypt)).getDictionary();

					// loading page tree
					{
						PdfDictionary attrs = getObject(catalog.getValue_NoLock(g_strPages)).getDictionary();
						if (attrs.isNotNull()) {
							pageTree = new PageTreeParent;
							if (pageTree.isNotNull()) {
								pageTree->attributes = Move(attrs);
							}
						}
					}
					if (encrypt.isNotNull()) {
						setUserPassword(sl_null);
					}
					return sl_true;
				}

				List< Ref<PdfPageTreeItem> > buildPageTreeKids(const PdfDictionary& attributes)
				{
					List< Ref<PdfPageTreeItem> > ret;
					ListElements<PdfObject> kidIds(attributes.getValue_NoLock(g_strKids).getArray());
					for (sl_size i = 0; i < kidIds.count; i++) {
						PdfDictionary props = getObject(kidIds[i]).getDictionary();
						Ref<PdfPageTreeItem> item;
						if (props.getValue_NoLock(g_strType).equalsName(StringView::literal("Page"))) {
							item = new PdfPage;
						} else {
							item = new PageTreeParent;
						}
						if (item.isNotNull()) {
							item->attributes = Move(props);
							ret.add_NoLock(Move(item));
						}
					}
					return ret;
				}

				Ref<PdfPage> getPage(PageTreeParent* parent, sl_uint32 index)
				{
					if (index >= parent->getPagesCount()) {
						return sl_null;
					}
					if (!(parent->flagKids)) {
						parent->kids = buildPageTreeKids(parent->attributes);
						parent->flagKids = sl_true;
					}
					sl_uint32 n = 0;
					ListElements< Ref<PdfPageTreeItem> > kids(parent->kids);
					for (sl_size i = 0; i < kids.count; i++) {
						Ref<PdfPageTreeItem>& item = kids[i];
						if (item->isPage()) {
							if (index == n) {
								item->parent = parent;
								return Ref<PdfPage>::from(item);
							}
							n++;
						} else {
							PageTreeParent* pItem = (PageTreeParent*)(item.get());
							sl_uint32 m = pItem->getPagesCount();
							if (index < n + m) {
								return getPage(pItem, index - n);
							}
							n += m;
						}
					}
					return sl_null;
				}

				Ref<PdfPage> getPage(sl_uint32 index) override
				{
					PageTreeParent* tree = pageTree.get();
					if (tree) {
						return getPage(tree, index);
					}
					return sl_null;
				}

				Memory getPageContent(const PdfObject& contents) override
				{
					PdfArray array = contents.getArray();
					if (array.isNotNull()) {
						MemoryBuffer buf;
						ListElements<PdfObject> items(array);
						for (sl_size i = 0; i < items.count; i++) {
							buf.add(getObject(items[i]).getStreamContent());
						}
						return buf.merge();
					} else {
						return getObject(contents).getStreamContent();
					}
				}

			};

			typedef Parser<BufferedReaderBase> BufferedParser;
			typedef Parser<MemoryReaderBase> MemoryParser;

			SLIB_INLINE static ParserBase* GetParser(const Ref<Referable>& ref)
			{
				return (ParserBase*)(ref.get());
			}

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

	PdfObject::PdfObject(PdfOperator v) noexcept: m_var((sl_uint32)v, (sl_uint8)(PdfObjectType::Operator)) {}

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
			return sl_true;
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

	PdfOperator PdfObject::getOperator() const noexcept
	{
		if (getType() == PdfObjectType::Operator) {
			return (PdfOperator)(m_var._m_uint32);
		}
		return PdfOperator::Unknown;
	}

	Rectangle PdfObject::getRectangle() const noexcept
	{
		Rectangle ret;
		if (getRectangle(ret)) {
			return ret;
		}
		return Rectangle::zero();
	}

	sl_bool PdfObject::getRectangle(Rectangle& outRect) const noexcept
	{
		ListElements<PdfObject> arr(getArray());
		if (arr.count == 4) {
			outRect.left = arr[0].getFloat();
			outRect.top = arr[1].getFloat();
			outRect.right = arr[2].getFloat();
			outRect.bottom = arr[3].getFloat();
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
					ret = ApplyFilter(ret, filter);
				} else {
					return sl_null;
				}
			}
			return ret;
		} else {
			String filter = objFilter.getName();
			if (filter.isNotNull()) {
				return ApplyFilter(content, filter);
			}
		}
		return sl_null;
	}
	

	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(PdfFontDescriptor)

	PdfFontDescriptor::PdfFontDescriptor(): ascent(0), descent(0), leading(0), weight(0), italicAngle(0), content(0, 0)
	{
	}


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(PdfFontResource)

	PdfFontResource::PdfFontResource() : firstChar(0), lastChar(0)
	{
	}


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(PdfOperation)

	PdfOperation::PdfOperation() noexcept
	{
	}

	PdfOperator PdfOperation::getOperator(const StringView& opName)
	{
		sl_char8* s = opName.getData();
		sl_size len = opName.getLength();
		sl_char8 s1;
		if (len == 1) {
			switch (*s) {
				case 'b':
					return PdfOperator::b;
				case 'B':
					return PdfOperator::B;
				case 'c':
					return PdfOperator::c;
				case 'd':
					return PdfOperator::d;
				case 'f':
					return PdfOperator::f;
				case 'F':
					return PdfOperator::F;
				case 'g':
					return PdfOperator::g;
				case 'G':
					return PdfOperator::G;
				case 'h':
					return PdfOperator::h;
				case 'i':
					return PdfOperator::i;
				case 'j':
					return PdfOperator::j;
				case 'J':
					return PdfOperator::J;
				case 'k':
					return PdfOperator::k;
				case 'K':
					return PdfOperator::K;
				case 'l':
					return PdfOperator::l;
				case 'm':
					return PdfOperator::m;
				case 'M':
					return PdfOperator::M;
				case 'n':
					return PdfOperator::n;
				case 'q':
					return PdfOperator::q;
				case 'Q':
					return PdfOperator::Q;
				case 's':
					return PdfOperator::s;
				case 'S':
					return PdfOperator::S;
				case 'v':
					return PdfOperator::v;
				case 'w':
					return PdfOperator::w;
				case 'W':
					return PdfOperator::W;
				case 'y':
					return PdfOperator::y;
				case '\'':
					return PdfOperator::apos;
				case '"':
					return PdfOperator::quot;
				default:
					break;
			}
		} else if (len == 2) {
			switch (*s) {
				case 'b':
					if (s[1] == '*') {
						return PdfOperator::b_;
					}
					break;
				case 'B':
					switch (s[1]) {
						case '*':
							return PdfOperator::B_;
						case 'I':
							return PdfOperator::BI;
						case 'T':
							return PdfOperator::BT;
						case 'X':
							return PdfOperator::BX;
						default:
							break;
					}
					break;
				case 'c':
					s1 = s[1];
					if (s1 == 'm') {
						return PdfOperator::cm;
					} else if (s1 == 's') {
						return PdfOperator::cs;
					}
					break;
				case 'd':
					s1 = s[1];
					if (s1 == '0') {
						return PdfOperator::d0;
					} else if (s1 == '1') {
						return PdfOperator::d1;
					}
					break;
				case 'C':
					if (s[1] == 'S') {
						return PdfOperator::CS;
					}
					break;
				case 'D':
					s1 = s[1];
					if (s1 == 'o') {
						return PdfOperator::Do;
					} else if (s1 == 'P') {
						return PdfOperator::DP;
					}
					break;
				case 'E':
					s1 = s[1];
					if (s1 == 'I') {
						return PdfOperator::EI;
					} else if (s1 == 'T') {
						return PdfOperator::ET;
					} else if (s1 == 'X') {
						return PdfOperator::EX;
					}
					break;
				case 'f':
					if (s[1] == '*') {
						return PdfOperator::f_;
					}
					break;
				case 'g':
					if (s[1] == 's') {
						return PdfOperator::gs;
					}
					break;
				case 'I':
					if (s[1] == 'D') {
						return PdfOperator::ID;
					}
					break;
				case 'M':
					if (s[1] == 'P') {
						return PdfOperator::MP;
					}
					break;
				case 'r':
					s1 = s[1];
					if (s1 == 'e') {
						return PdfOperator::re;
					} else if (s1 == 'g') {
						return PdfOperator::rg;
					} else if (s1 == 'i') {
						return PdfOperator::ri;
					}
					break;
				case 'R':
					if (s[1] == 'G') {
						return PdfOperator::RG;
					}
					break;
				case 's':
					s1 = s[1];
					if (s1 == 'c') {
						return PdfOperator::sc;
					} else if (s1 == 'h') {
						return PdfOperator::sh;
					}
					break;
				case 'S':
					if (s[1] == 'C') {
						return PdfOperator::SC;
					}
					break;
				case 'T':
					switch (s[1]) {
						case '*':
							return PdfOperator::T_;
						case 'c':
							return PdfOperator::Tc;
						case 'd':
							return PdfOperator::Td;
						case 'D':
							return PdfOperator::TD;
						case 'f':
							return PdfOperator::Tf;
						case 'j':
							return PdfOperator::Tj;
						case 'J':
							return PdfOperator::TJ;
						case 'L':
							return PdfOperator::TL;
						case 'm':
							return PdfOperator::Tm;
						case 'r':
							return PdfOperator::Tr;
						case 's':
							return PdfOperator::Ts;
						case 'w':
							return PdfOperator::Tw;
						case 'z':
							return PdfOperator::Tz;
						default:
							break;
					}
					break;
				case 'W':
					if (s[1] == '*') {
						return PdfOperator::W_;
					}
					break;
				default:
					break;
			}
		} else if (len == 3) {
			switch (*s) {
				case 'B':
					s1 = s[1];
					if (s1 == 'D') {
						if (s[2] == 'C') {
							return PdfOperator::BDC;
						}
					} else if (s1 == 'M') {
						if (s[2] == 'C') {
							return PdfOperator::BMC;
						}
					}
					break;
				case 'E':
					if (s[1] == 'M') {
						if (s[2] == 'C') {
							return PdfOperator::EMC;
						}
					}
					break;
				case 's':
					if (s[1] == 'c') {
						if (s[2] == 'n') {
							return PdfOperator::scn;
						}
					}
					break;
				case 'S':
					if (s[1] == 'C') {
						if (s[2] == 'N') {
							return PdfOperator::SCN;
						}
					}
					break;
				default:
					break;
			}
		}
		return PdfOperator::Unknown;
	}


	PdfPageTreeItem::PdfPageTreeItem() noexcept: m_flagPage(sl_false)
	{
	}

	PdfPageTreeItem::~PdfPageTreeItem()
	{
	}

	sl_bool PdfPageTreeItem::isPage()
	{
		return m_flagPage;
	}

	PdfObject PdfPageTreeItem::getAttribute(const String& name)
	{
		PdfObject ret = attributes.getValue_NoLock(name);
		if (ret.isNotUndefined()) {
			return ret;
		}
		Ref<PdfPageTreeItem> _parent(parent);
		if (_parent.isNotNull()) {
			return _parent->getAttribute(name);
		}
		return PdfObject();
	}


	SLIB_DEFINE_OBJECT(PdfPage, PdfPageTreeItem)

	PdfPage::PdfPage() noexcept
	{
		m_flagPage = sl_true;
		m_flagContent = sl_false;
	}

	PdfPage::~PdfPage()
	{
	}

	Ref<PdfDocument> PdfPage::getDocument()
	{
		return m_document;
	}

	Memory PdfPage::getContentStream()
	{
		Ref<PdfDocument> document(m_document);
		if (document.isNotNull()) {
			ObjectLocker locker(document.get());
			ParserBase* parser = GetParser(document->m_parser);
			if (parser) {
				return parser->getPageContent(attributes.getValue_NoLock(g_strContents));
			}
		}
		return sl_null;
	}

	List<PdfOperation> PdfPage::getContent()
	{
		if (m_flagContent) {
			return m_content;
		}
		Memory data = getContentStream();
		if (data.isNotNull()) {
			List<PdfOperation> ret = parseContent((sl_char8*)(data.getData()), data.getSize());
			if (ret.isNotNull()) {
				m_content = ret;
				m_flagContent = sl_true;
				return ret;
			}
		}
		m_flagContent = sl_true;
		return sl_null;
	}

	List<PdfOperation> PdfPage::parseContent(const void* data, sl_size size)
	{
		List<PdfOperation> ret;
		MemoryParser parser;
		parser.source = (sl_char8*)data;
		parser.sizeSource = (sl_uint32)size;
		PdfOperation opCurrent;
		for (;;) {
			if (!(parser.skipWhitespaces())) {
				break;
			}
			PdfObject obj = parser.readValue();
			if (obj.isUndefined()) {
				break;
			}
			PdfOperator op = obj.getOperator();
			if (op != PdfOperator::Unknown) {
				opCurrent.op = op;
				ret.add_NoLock(Move(opCurrent));
				opCurrent.operands.setNull();
			} else {
				opCurrent.operands.add_NoLock(Move(obj));
			}
		}
		return ret;
	}

	Rectangle PdfPage::getMediaBox()
	{
		return getAttribute(g_strMediaBox).getRectangle();
	}

	Rectangle PdfPage::getCropBox()
	{
		Rectangle ret;
		if (getAttribute(g_strCropBox).getRectangle(ret)) {
			return ret;
		}
		return getMediaBox();
	}

	PdfObject PdfPage::getResources(const String& type)
	{
		Ref<PdfDocument> doc(m_document);
		if (doc.isNotNull()) {
			ObjectLocker locker(doc.get());
			ParserBase* parser = GetParser(doc->m_parser);
			if (parser) {
				Ref<PdfPageTreeItem> item = this;
				for (;;) {
					PdfDictionary dict = parser->getObject(item->attributes.getValue_NoLock(g_strResources)).getDictionary();
					if (dict.isNotNull()) {
						PdfObject ret = dict.getValue_NoLock(type);
						if (ret.isNotUndefined()) {
							return parser->getObject(ret);
						}
					}
					item = item->parent;
					if (item.isNull()) {
						break;
					}
				}
			}
		}
		return PdfObject();
	}

	PdfObject PdfPage::getResource(const String& type, const String& name)
	{
		Ref<PdfDocument> doc(m_document);
		if (doc.isNotNull()) {
			ObjectLocker locker(doc.get());
			ParserBase* parser = GetParser(doc->m_parser);
			if (parser) {
				Ref<PdfPageTreeItem> item = this;
				for (;;) {
					PdfDictionary dict = parser->getObject(item->attributes.getValue_NoLock(g_strResources)).getDictionary();
					if (dict.isNotNull()) {
						PdfObject ret = dict.getValue_NoLock(type).getDictionary().getValue_NoLock(name);
						if (ret.isNotUndefined()) {
							return parser->getObject(ret);
						}
					}
					item = item->parent;
					if (item.isNull()) {
						break;
					}
				}
			}
		}
		return PdfObject();
	}

	PdfDictionary PdfPage::getFontResourceAsDictionary(const String& name)
	{
		return getResource(g_strFont, name).getDictionary();
	}

	sl_bool PdfPage::getFontResource(const String& name, PdfFontResource& resource)
	{
		Ref<PdfDocument> doc(m_document);
		if (doc.isNotNull()) {
			PdfDictionary dict = getFontResourceAsDictionary(name);
			if (dict.isNotNull()) {
				const String& subType = dict.getValue_NoLock(g_strSubtype).getName();
				if (subType == StringView::literal("Type1") || subType == StringView::literal("TrueType")) {
					PdfDictionary desc = doc->getObject(dict.getValue_NoLock(g_strFontDescriptor)).getDictionary();
					if (desc.isNotEmpty()) {
						resource.family = desc.getValue_NoLock(g_strFontFamily).getString();
						resource.ascent = desc.getValue_NoLock(g_strAscent).getFloat();
						resource.descent = desc.getValue_NoLock(g_strDescent).getFloat();
						resource.leading = desc.getValue_NoLock(g_strLeading).getFloat();
						resource.weight = desc.getValue_NoLock(g_strFontWeight).getFloat();
						resource.italicAngle = desc.getValue_NoLock(g_strItalicAngle).getFloat();
						if (!(desc.getValue_NoLock(g_strFontFile).getReference(resource.content))) {
							if (!(desc.getValue_NoLock(g_strFontFile2).getReference(resource.content))) {
								desc.getValue_NoLock(g_strFontFile3).getReference(resource.content);
							}
						}

						resource.firstChar = dict.getValue_NoLock(g_strFirstChar).getInt();
						resource.lastChar = dict.getValue_NoLock(g_strLastChar).getInt();
						ListElements<PdfObject> widths(dict.getValue_NoLock(g_strWidths).getArray());
						if (widths.count == resource.lastChar - resource.firstChar + 1) {
							resource.widths = Array<float>::create(widths.count);
							if (resource.widths.isNotNull()) {
								float* f = resource.widths.getData();
								for (sl_size i = 0; i < widths.count; i++) {
									f[i] = widths[i].getFloat();
								}
							}
						}
						return sl_true;
					}
				}
			}
		}
		return sl_false;
	}

	PdfObject PdfPage::getExternalObjectResource(const String& name)
	{
		return getResource(g_strXObject, name);
	}


	SLIB_DEFINE_OBJECT(PdfDocument, Object)

	PdfDocument::PdfDocument(): fileSize(0)
	{
	}

	PdfDocument::~PdfDocument()
	{
	}

	sl_bool PdfDocument::_openFile(const StringParam& filePath)
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
				Ref<BufferedParser> parser = New<BufferedParser>();
				if (parser.isNotNull()) {
					if (parser->reader.open(reader)) {
						if (parser->readDocument()) {
							m_parser = Move(parser);
							return sl_true;
						}
					}
				}
			}
		}
		return sl_false;
	}

	Ref<PdfDocument> PdfDocument::openFile(const StringParam& filePath)
	{
		Ref<PdfDocument> doc = new PdfDocument;
		if (doc.isNotNull()) {
			if (doc->_openFile(filePath)) {
				return doc;
			}
		}
		return sl_null;
	}

	sl_bool PdfDocument::_openMemory(const Memory& mem)
	{
		if (mem.isNull()) {
			return sl_false;
		}
		fileSize = (sl_uint32)(mem.getSize());
		if (fileSize > MAX_PDF_FILE_SIZE) {
			return sl_false;
		}
		Ref<MemoryParser> parser = New<MemoryParser>();
		if (parser.isNotNull()) {
			parser->source = (sl_char8*)(mem.getData());
			parser->sizeSource = fileSize;
			parser->refSource = mem.getRef();
			if (parser->readDocument()) {
				m_parser = Move(parser);
				return sl_true;
			}
		}
		return sl_false;
	}

	Ref<PdfDocument> PdfDocument::openMemory(const Memory& mem)
	{
		Ref<PdfDocument> doc = new PdfDocument;
		if (doc.isNotNull()) {
			if (doc->_openMemory(mem)) {
				return doc;
			}
		}
		return sl_null;
	}

	PdfObject PdfDocument::getObject(const PdfReference& ref)
	{
		ObjectLocker lock(this);
		ParserBase* parser = GetParser(m_parser);
		if (parser) {
			return parser->getObject(ref);
		}
		return PdfObject();
	}

	PdfObject PdfDocument::getObject(const PdfObject& refOrObj)
	{
		ObjectLocker lock(this);
		ParserBase* parser = GetParser(m_parser);
		if (parser) {
			return parser->getObject(refOrObj);
		}
		return PdfObject();
	}

	sl_uint32 PdfDocument::getPagesCount()
	{
		ParserBase* parser = GetParser(m_parser);
		if (parser) {
			PageTreeParent* tree = parser->pageTree.get();
			if (tree) {
				return tree->getPagesCount();
			}
		}
		return 0;
	}

	Ref<PdfPage> PdfDocument::getPage(sl_uint32 index)
	{
		ObjectLocker lock(this);
		ParserBase* parser = GetParser(m_parser);
		if (parser) {
			Ref<PdfPage> page = parser->getPage(index);
			if (page.isNotNull()) {
				page->m_document = this;
				return page;
			}
		}
		return sl_null;
	}

	sl_bool PdfDocument::isEncrypted()
	{
		ParserBase* parser = GetParser(m_parser);
		if (parser) {
			return parser->encrypt.isNotNull();
		}
		return sl_false;
	}

	sl_bool PdfDocument::isAuthenticated()
	{
		if (isEncrypted()) {
			ParserBase* parser = GetParser(m_parser);
			if (parser) {
				return parser->flagDecryptContents;
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
		ObjectLocker lock(this);
		ParserBase* parser = GetParser(m_parser);
		if (parser) {
			return parser->setUserPassword(password);
		}
		return sl_false;
	}

}
