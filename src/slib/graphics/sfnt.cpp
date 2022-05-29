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

#include "slib/graphics/sfnt.h"

#include "slib/core/mio.h"
#include "slib/core/scoped_buffer.h"

#define TAG_ttcf SLIB_MAKE_DWORD('t', 't', 'c', 'f')
#define TAG_OTTO SLIB_MAKE_DWORD('O', 'T', 'T', 'O')
#define TAG_true SLIB_MAKE_DWORD('t', 'r', 'u', 'e')
#define TAG_typ1 SLIB_MAKE_DWORD('t', 'y', 'p', '1')

#define TAG_name SLIB_MAKE_DWORD('n', 'a', 'm', 'e')

namespace slib
{

	namespace priv
	{
		namespace sfnt
		{

			struct COLLECTION_HEADER
			{
				sl_uint8 version[4];
				sl_uint8 count[4];
			};

			struct FACE_HEADER
			{
				sl_uint8 numTables[2];
				sl_uint8 searchRange[2];
				sl_uint8 entrySelector[2];
				sl_uint8 rangeShift[2];
			};

			struct FACE_TABLE_OFFSET
			{
				char name[4];
				sl_uint8 checksum[4];
				sl_uint8 offset[4];
				sl_uint8 length[4];
			};

			struct NAME_TABLE_HEADER
			{
				sl_uint8 format[2];
				sl_uint8 count[2];
				sl_uint8 stringOffset[2];
			};

			struct NAME_TABLE_ENTRY
			{
				sl_uint8 platformId[2];
				sl_uint8 encodingId[2];
				sl_uint8 languageId[2];
				sl_uint8 nameId[2];
				sl_uint8 length[2];
				sl_uint8 offset[2];
			};

			struct FaceTableOffset
			{
				sl_uint32 name;
				sl_uint32 offset;
				sl_uint32 length;
			};

			class FontLoader
			{
			public:
				IReader* reader;
				ISeekable* seekable;

				sl_uint32 posFileStart = 0;
				sl_bool flagIsCollection = sl_false;

			private:
				sl_uint32 getPosition()
				{
					return (sl_uint32)(seekable->getPosition());
				}

				sl_bool setPosition(sl_size pos)
				{
					return seekable->seek(pos, SeekPosition::Begin);
				}

				sl_bool readFully(void* buf, sl_size size)
				{
					return reader->readFully(buf, size) == size;
				}

				sl_bool readLong(sl_uint32& n)
				{
					return reader->readUint32(&n, EndianType::Big);
				}

			public:
				sl_bool readFirstTag()
				{
					posFileStart = getPosition();
					sl_uint32 tag;
					if (!(readLong(tag))) {
						return sl_false;
					}
					if (tag == TAG_ttcf) {
						flagIsCollection = sl_true;
					} else {
						if (tag != 0x00010000 && tag != 0x00020000 && tag != TAG_OTTO && tag != TAG_true && tag != TAG_typ1) {
							return sl_false;
						}
					}
					return sl_true;
				}

				Array<sl_uint32> loadFaceOffsets()
				{
					if (readFirstTag()) {
						if (flagIsCollection) {
							COLLECTION_HEADER header;
							if (!(readFully(&header, sizeof(header)))) {
								return sl_null;
							}
							sl_uint32 n = MIO::readUint32BE(header.count);
							if (n) {
								Array<sl_uint32> offsets = Array<sl_uint32>::create(n);
								if (offsets.isNotNull()) {
									sl_uint32* data = offsets.getData();
									for (sl_uint32 i = 0; i < n; i++) {
										if (!(readLong(data[i]))) {
											return sl_null;
										}
									}
								}
								return offsets;
							}
						} else {
							return Array<sl_uint32>::create(&posFileStart, 1);
						}
					}
					return sl_null;
				}

				sl_bool readFaceTableOffset(FaceTableOffset& _out)
				{
					FACE_TABLE_OFFSET record;
					if (!(readFully(&record, sizeof(record)))) {
						return sl_false;
					}
					_out.name = MIO::readUint32BE(record.name);
					_out.offset = MIO::readUint32BE(record.offset);
					_out.length = MIO::readUint32BE(record.length);
					return sl_true;
				}

				sl_bool loadFaceTableOffsets(List<FaceTableOffset>& _out)
				{
					sl_uint32 tag;
					if (readLong(tag)) {
						FACE_HEADER header;
						if (readFully(&header, sizeof(header))) {
							sl_uint32 n = MIO::readUint16BE(header.numTables);
							if (n) {
								for (sl_uint32 i = 0; i < n; i++) {
									FaceTableOffset entry;
									if (!(readFaceTableOffset(entry))) {
										return sl_false;
									}
									if (entry.name == TAG_name) {
										_out.add_NoLock(Move(entry));
									}
								}
								return sl_true;
							}
						}
					}
					return sl_false;
				}

				sl_bool getName(sl_uint32 offsetString, const NAME_TABLE_ENTRY& entry, String& _out)
				{
					sl_uint16 platformId = MIO::readUint16BE(entry.platformId);
					sl_uint16 encodingId = MIO::readUint16BE(entry.encodingId);
					sl_bool flagUtf16 = sl_false;
					switch ((TrueTypePlatformId)platformId) {
						case TrueTypePlatformId::AppleUnicode:
						case TrueTypePlatformId::ISO:
							flagUtf16 = sl_true;
							break;
						case TrueTypePlatformId::Macintosh:
							break;
						case TrueTypePlatformId::Microsoft:
							switch ((TrueTypeEncodingId)encodingId) {
							case TrueTypeEncodingId::Microsoft_Symbol:
							case TrueTypeEncodingId::Microsoft_Unicode:
							case TrueTypeEncodingId::Microsoft_UCS4:
								flagUtf16 = sl_true;
								break;
							default:
								break;
							}
							break;
						default:
							break;
					}
					sl_uint32 len = MIO::readUint16BE(entry.length);
					if (len) {
						offsetString += MIO::readUint16BE(entry.offset);
						sl_uint32 offsetOld = getPosition();
						if (!(setPosition(offsetString))) {
							return sl_false;
						}
						SLIB_SCOPED_BUFFER(char, 512, buf, len)
						if (!buf) {
							return sl_false;
						}
						if (!(readFully(buf, len))) {
							return sl_false;
						}
						if (flagUtf16) {
							_out = String::fromUtf16BE(buf, len);
						} else {
							_out = String::fromUtf8(buf, len);
						}
						if (!(setPosition(offsetOld))) {
							return sl_false;
						}
					} else {
						_out.setEmpty();
					}
					return sl_true;
				}

				sl_bool readFamilyNamesFromNameTable(List<String>& _out)
				{
					sl_uint32 offset = getPosition();
					NAME_TABLE_HEADER header;
					if (!(readFully(&header, sizeof(header)))) {
						return sl_false;
					}
					sl_uint32 offsetString = offset + MIO::readUint16BE(header.stringOffset);
					sl_uint16 n = MIO::readUint16BE(header.count);
					for (sl_uint16 i = 0; i < n; i++) {
						NAME_TABLE_ENTRY entry;
						if (!(readFully(&entry, sizeof(entry)))) {
							return sl_false;
						}
						sl_uint16 nameId = MIO::readUint16BE(entry.nameId);
						if (nameId == (sl_uint16)(TrueTypeNameId::FontFamily)) {
							String name;
							if (!(getName(offsetString, entry, name))) {
								return sl_false;
							}
							if (name.isNotEmpty()) {
								_out.addIfNotExist_NoLock(Move(name));
							}
						}
					}
					return sl_true;
				}

				sl_bool loadFaceFamilyNames(List<String>& _out)
				{
					List<FaceTableOffset> _offsets;
					if (!(loadFaceTableOffsets(_offsets))) {
						return sl_false;
					}
					ListElements<FaceTableOffset> offsets(_offsets);
					for (sl_size i = 0; i < offsets.count; i++) {
						FaceTableOffset& entry = offsets[i];
						if (!(setPosition(entry.offset))) {
							return sl_false;
						}
						if (entry.name == TAG_name) {
							if (!(readFamilyNamesFromNameTable(_out))) {
								return sl_false;
							}
						}
					}
					return sl_true;
				}

				List< List<String> > loadFamilyNames()
				{
					List< List<String> > ret;
					ArrayElements<sl_uint32> offsets(loadFaceOffsets());
					for (sl_size i = 0; i < offsets.count; i++) {
						if (!(setPosition(offsets[i]))) {
							return sl_null;
						}
						List<String> names;
						if (loadFaceFamilyNames(names)) {
							ret.add_NoLock(Move(names));
						}
					}
					return ret;
				}

			};

		}
	}

	using namespace priv::sfnt;

	List< List<String> > SFNT::getFontFamilyNames(const Ptr<IReader, ISeekable>& reader)
	{
		if (reader.isNull()) {
			return sl_null;
		}
		FontLoader loader;
		loader.reader = reader;
		loader.seekable = reader;
		return loader.loadFamilyNames();
	}

}
