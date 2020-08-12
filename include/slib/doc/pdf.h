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
#include "../core/buffered_io.h"

namespace slib
{

	class SLIB_EXPORT PdfDocument
	{
	public:
		sl_uint8 majorVersion;
		sl_uint8 minorVersion;
		sl_uint32 fileSize;
		sl_uint32 offsetOfLastCrossRef;
		HashMap<String, String> lastTrailer;

	public:
		PdfDocument();

		~PdfDocument();

	public:
		sl_bool setReader(const Ptr<IReader, ISeekable>& reader);

	public:
		sl_bool readHeader();

	public:
		static sl_bool isEncrypted(const Ptr<IReader, ISeekable>& reader);
		static sl_bool isEncryptedFile(const StringParam& path);

	private:
		String readWord();

		sl_bool readInt64(sl_int64& n);
		sl_bool readUint64(sl_uint64& n);
		sl_bool readInt32(sl_int32& n);
		sl_bool readUint32(sl_uint32& n);

		sl_bool readDictionary(HashMap<String, String>& map);

	private:
		Ref<BufferedSeekableReader> m_reader;

	};

}

#endif
