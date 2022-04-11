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

#ifndef CHECKHEADER_SLIB_DOC_PDF
#define CHECKHEADER_SLIB_DOC_PDF

#include "definition.h"

#include "../core/string.h"
#include "../core/hash_map.h"
#include "../core/buffered_seekable_reader.h"

namespace slib
{

	class Variant;

	struct SLIB_EXPORT PdfCrossReferenceEntry
	{
		sl_uint32 offset; // 10 digits
		sl_uint32 generation; // 5 digits. 65535 means the head of the linked list of free objects. First entry (object number 0) is always free and has a generation number of 65535
		sl_bool flagFree; // free(f) or in-use(n)
	};

	class SLIB_EXPORT PdfCrossReferenceSection
	{
	public:
		sl_uint32 firstObjectNumber;
		List<PdfCrossReferenceEntry> entries;

	public:
		PdfCrossReferenceSection();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(PdfCrossReferenceSection)

	};

	class SLIB_EXPORT PdfCrossReferenceTable
	{
	public:
		List<PdfCrossReferenceSection> sections;
		HashMap<sl_uint64, sl_uint32> objectOffsets;
		HashMap<String, Variant> trailer;

	public:
		PdfCrossReferenceTable();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(PdfCrossReferenceTable)

	};

	class SLIB_EXPORT PdfStream : public Referable
	{
		SLIB_DECLARE_OBJECT

	public:
		HashMap<String, Variant> properties;
		Memory content;

	public:
		PdfStream();

		~PdfStream();

	public:
		static Ref<PdfStream> from(const Variant& var);

	};

	class SLIB_EXPORT PdfDocument : public Object
	{
		SLIB_DECLARE_OBJECT

	public:
		sl_uint8 majorVersion;
		sl_uint8 minorVersion;
		sl_uint32 fileSize;
		HashMap<String, Variant> lastTrailer;
		HashMap<sl_uint64, sl_uint32> objectOffsets;

	public:
		PdfDocument();

		~PdfDocument();

	public:
		sl_bool openFile(const StringParam& filePath);

	public:
		sl_bool setReader(const Ptr<IReader, ISeekable>& reader);

		sl_bool readHeader();

	public:
		static sl_bool isEncrypted(const Ptr<IReader, ISeekable>& reader);
		static sl_bool isEncryptedFile(const StringParam& path);

	private:
		sl_bool readWord(String& outWord);
		sl_bool readWordAndEquals(const StringView& word);
		sl_bool skipWhitespaces();

		sl_bool readObject(sl_uint64& outId, Variant& outValue);
		sl_bool getObject(sl_uint64 _id, Variant& _out);
		sl_bool readValue(Variant& _out);
		sl_bool readDictionary(HashMap<String, Variant>& outMap);
		sl_bool readArray(List<Variant>& outList);
		sl_bool readName(String& outName);
		sl_bool readUint(sl_uint32& outValue, sl_bool flagAllowEmpty = sl_false);
		sl_bool readInt(sl_int32& outValue);
		sl_bool readFraction(double& outValue, sl_bool flagAllowEmpty = sl_false);
		sl_bool readString(String& outValue);
		sl_bool readHexString(String& outValue);
		sl_bool readReference(sl_uint32& objectNumber, sl_uint32& version);
		sl_bool readStreamContent(sl_uint32 length, Memory& _out);
		sl_bool getStreamLength(HashMap<String, Variant>& properties, sl_uint32& _out);
		sl_bool readCrossReferenceEntry(PdfCrossReferenceEntry& entry);
		sl_bool readCrossReferenceSection(PdfCrossReferenceSection& section);
		sl_bool readCrossReferenceTable(PdfCrossReferenceTable& table);
		
	private:
		BufferedSeekableReader m_reader;

	};

}

#endif
