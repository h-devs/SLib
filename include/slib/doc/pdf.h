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

#include "../core/variant.h"
#include "../math/rectangle.h"

namespace slib
{

	enum class PdfObjectType
	{
		Undefined = 0,
		Null = 1,
		Boolean = 2,
		Uint = 3,
		Int = 4,
		Float = 5,
		String = 6,
		Name = 7,
		Array = 8,
		Dictionary = 9,
		Stream = 10,
		Reference = 11,
		Operator = 12
	};

	enum class PdfOperator
	{
		Unknown,
		b, // close path, fill(nonzero winding number rule), stroke
		B, // fill(nonzero winding number rule), stroke
		b_, // b*: closePath, fill(even-odd rule), stroke
		B_, // B*: fill(even-odd rule), stroke
		BDC, // begin marked-content sequence with property list
		BI, // begin inline image object
		BMC, // begin marked-content sequence
		BT, // begin text object
		BX, // begin compatibility section
		c, // curve to (three control point)
		cm, // concat matrix to current transformation matrix
		CS, // set color-space (for stroking)
		cs, // set color-space (for non-stroking)
		d, // set line dash pattern
		d0, // set char width (glphy with in Type3 font)
		d1, // set cache device (glphy with and bounding box in Type3 font)
		Do, // invoke named XObject
		DP, // define marked-content point with property list
		EI, // end inline image object
		EMC, // End marked-content sequence
		ET, // end text object
		EX, // end compatibility section
		f, // fill(nonzero winding number rule)
		F, // fill(nonzero winding number rule, obsolute)
		f_, // f*: fill(even-odd rule)
		G, // set gray level for stroking
		g, // set gray level for non-stroking
		gs, // set parameters from graphics state parameter dictionary
		h, // close path
		i, // set flatness tolerance
		ID, // begin inline image data
		j, // set line-join
		J, // set line-cap
		K, // set cmyk-color (for stroking)
		k, // set cmyk-color (for non-stroking)
		l, // line to
		m, // move to
		M, // set miter limit
		MP, // define marked-content point
		n, // end path without filling or stroking
		q, // save graphics state
		Q, // restore graphics state
		re, // append rectangle to path
		RG, // set rgb-color (for stroking)
		rg, // set rgb-color (for non-stroking)
		ri, // set color rendering intent
		s, // close path, stroke
		S, // stroke
		SC, // set color (for stroking)
		sc, // set color (for non-stroking)
		SCN, // set color (for stroking, ICCBased and special color spaces)
		scn, // set color (for non-stroking, ICCBased and special color spaces)
		sh, // paint area defined by shading pattern
		T_, // T*: move to start of next text line
		Tc, // set character spacing
		Td, // move text position
		TD, // move text position and leading
		Tf, // select font and size
		Tj, // show text
		TJ, // show text, allowing individual glphy positioning
		TL, // set text leading
		Tm, // set text matrix and text line matrix
		Tr, // set text rendering mode
		Ts, // set text rise
		Tw, // set word spacing
		Tz, // set horizontal text scaling
		v, // curve to (initial point replicated)
		w, // set line width
		W, // set clipping path (nonzero winding number rule)
		W_, // W*: set clipping path (even-odd rule)
		y, // curve to (final point replicated)
		apos, // ': move to next line and show text
		quot, // ": set word and character spacing, move to next line, and show text
	};

	class PdfObject;
	class PdfStream;
	class PdfPage;
	class PdfDocument;
	class Canvas;
	class Font;

	typedef HashMap<String, PdfObject> PdfDictionary;
	typedef List<PdfObject> PdfArray;

	class SLIB_EXPORT PdfReference
	{
	public:
		sl_uint32 objectNumber;
		sl_uint32 generation;

	public:
		PdfReference() noexcept {}

		SLIB_CONSTEXPR PdfReference(sl_uint32 _num): objectNumber(_num), generation(0) {}

		SLIB_CONSTEXPR PdfReference(sl_uint32 _num, sl_uint32 _gen): objectNumber(_num), generation(_gen) {}

		SLIB_DEFINE_CLASS_DEFAULT_MEMBERS_INLINE(PdfReference)

	public:
		SLIB_CONSTEXPR sl_bool operator==(const PdfReference& other) const
		{
			return objectNumber == other.objectNumber && generation == other.generation;
		}

	};

	class SLIB_EXPORT PdfName
	{
	public:
		SLIB_CONSTEXPR PdfName() {}

		PdfName(const String& _name) noexcept: value(_name) {}
		PdfName(String&& _name) noexcept: value(Move(_name)) {}

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(PdfName)

	public:
		SLIB_CONSTEXPR sl_bool isNull() const
		{
			return value.isNull();
		}

		SLIB_CONSTEXPR sl_bool isNotNull() const
		{
			return value.isNotNull();
		}

	public:
		String value;

	};

	class SLIB_EXPORT PdfObject
	{
	public:
		PdfObject() noexcept {}

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(PdfObject)

	public:
		PdfObject(sl_null_t) noexcept: m_var(sl_null, (sl_uint8)(PdfObjectType::Null)) {}

		PdfObject(sl_bool v) noexcept;

		PdfObject(sl_int32 v) noexcept;
		PdfObject(sl_uint32 v) noexcept;
		PdfObject(float v) noexcept;

		PdfObject(const String& v) noexcept;
		PdfObject(String&& v) noexcept;

		PdfObject(const PdfArray& v) noexcept;
		PdfObject(PdfArray&& v) noexcept;

		PdfObject(const PdfDictionary& v) noexcept;
		PdfObject(PdfDictionary&& v) noexcept;

		PdfObject(const Ref<PdfStream>& v) noexcept;
		PdfObject(Ref<PdfStream>&& v) noexcept;

		PdfObject(const PdfName& v) noexcept;
		PdfObject(PdfName&& v) noexcept;

		PdfObject(const PdfReference& v) noexcept;

		PdfObject(PdfOperator op) noexcept;

	public:
		const Variant& getVariant() const noexcept
		{
			return m_var;
		}

		Variant& getVariant() noexcept
		{
			return m_var;
		}

		SLIB_CONSTEXPR PdfObjectType getType() const
		{
			return (PdfObjectType)(m_var.getTag());
		}

		SLIB_CONSTEXPR sl_bool isUndefined() const
		{
			return m_var.isUndefined();
		}

		SLIB_CONSTEXPR sl_bool isNotUndefined() const
		{
			return m_var.isNotUndefined();
		}

		SLIB_CONSTEXPR sl_bool isNull() const
		{
			return m_var.isNull();
		}

		SLIB_CONSTEXPR sl_bool isNotNull() const
		{
			return m_var.isNotNull();
		}

		sl_bool getBoolean() const noexcept;

		sl_bool getBoolean(sl_bool& _out) const noexcept;

		sl_uint32 getUint() const noexcept;

		sl_bool getUint(sl_uint32& _out) const noexcept;

		sl_int32 getInt() const noexcept;

		sl_bool getInt(sl_int32& _out) const noexcept;

		float getFloat() const noexcept;

		sl_bool getFloat(float& _out) const noexcept;

		sl_bool isNumeric() const noexcept;

		const String& getString() const noexcept;

		const String& getName() const noexcept;

		sl_bool equalsName(const StringView& name) const noexcept;

		const PdfArray& getArray() const noexcept;

		const PdfDictionary& getDictionary() const noexcept;

		const Ref<PdfStream>& getStream() const noexcept;

		Memory getStreamContent() const noexcept;

		PdfReference getReference() const noexcept;

		sl_bool getReference(PdfReference& _out) const noexcept;

		PdfOperator getOperator() const noexcept;

	private:
		Variant m_var;

	};

	class SLIB_EXPORT PdfStream : public Referable
	{
		SLIB_DECLARE_OBJECT

	public:
		PdfDictionary properties;
		Memory content;

	public:
		PdfStream() noexcept;

		~PdfStream();

	public:
		PdfObject getProperty(const String& name) noexcept;

		Memory getOriginalContent() noexcept;

	};

	class SLIB_EXPORT PdfFontResource
	{
	public:
		Ref<Font> font;

	public:
		PdfFontResource();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(PdfFontResource)

	};

	class SLIB_EXPORT PdfOperation
	{
	public:
		PdfOperator op;
		PdfArray operands;

	public:
		PdfOperation() noexcept;

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(PdfOperation)

	public:
		static PdfOperator getOperator(const StringView& opName);

	};

	class PdfPageTreeItem : public Referable
	{
	public:
		WeakRef<PdfPageTreeItem> parent;
		PdfDictionary attributes;

	public:
		PdfPageTreeItem() noexcept;

		~PdfPageTreeItem();

	public:
		sl_bool isPage();

		PdfObject getAttribute(const String& name);

	protected:
		sl_bool m_flagPage;

	};

	class SLIB_EXPORT PdfPage : public PdfPageTreeItem
	{
		SLIB_DECLARE_OBJECT

	public:
		PdfPage() noexcept;

		~PdfPage();

	public:
		Ref<PdfDocument> getDocument();

		Memory getContentStream();

		List<PdfOperation> getContent();

		static List<PdfOperation> parseContent(const void* data, sl_size size);

		void render(Canvas* canvas, const Rectangle& rcDst);

		PdfObject getResources(const String& type);

		PdfObject getResource(const String& type, const String& name);

		PdfDictionary getFontResourceAsDictionary(const String& name);

		sl_bool getFontResource(const String& name, PdfFontResource& outResource);

		PdfObject getExternalObjectResource(const String& name);

	protected:
		WeakRef<PdfDocument> m_document;
		AtomicList<PdfOperation> m_content;
		sl_bool m_flagContent;

		friend class PdfDocument;
	};

	class SLIB_EXPORT PdfDocument : public Object
	{
		SLIB_DECLARE_OBJECT

	public:
		sl_uint32 fileSize;

	protected:
		PdfDocument();

		~PdfDocument();

	public:
		static Ref<PdfDocument> openFile(const StringParam& filePath);

		static Ref<PdfDocument> openMemory(const Memory& mem);

	public:
		PdfObject getObject(const PdfReference& ref);

		PdfObject getObject(const PdfObject& refOrObj);

		sl_uint32 getPagesCount();

		Ref<PdfPage> getPage(sl_uint32 index);

		sl_bool isEncrypted();

		static sl_bool isEncryptedFile(const StringParam& path);

		sl_bool isAuthenticated();

		sl_bool setUserPassword(const StringView& password);
		
	protected:
		sl_bool _openFile(const StringParam& filePath);

		sl_bool _openMemory(const Memory& mem);

	private:
		Ref<Referable> m_parser;

		friend class PdfPage;

	};

}

#endif
