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

#ifndef CHECKHEADER_SLIB_DOC_PDF
#define CHECKHEADER_SLIB_DOC_PDF

#include "definition.h"

#include "../core/variant.h"
#include "../core/expiring_map.h"
#include "../core/flags.h"
#include "../math/rectangle.h"
#include "../graphics/color.h"

namespace slib
{

	enum class PdfValueType
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
		Reference = 11
	};

	enum class PdfOperator
	{
		Unknown = 0,
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

	enum class PdfCMapOperator
	{
		Unknown = 0,
		def,
		begincodespacerange,
		endcodespacerange,
		beginbfchar,
		endbfchar,
		beginbfrange,
		endbfrange
	};

	enum class PdfFontSubtype
	{
		Unknown = -1,
		Type0 = 0,
		Type1 = 1,
		TrueType = 2,
		Type3 = 3,
		Type5 = 5,
		CIDFontType0 = 10,
		CIDFontType2 = 12,
		MMType1 = 100 // Multiple Master Font
	};

	enum class PdfEncoding
	{
		Unknown = 0,
		Standard = 1,
		MacRoman = 2,
		WinAnsi = 3,
		PdfDoc = 4,
		MacExpert = 5,
		Symbol = 6,
		MSSymbol = 7,
		Zapf = 8,
		IdentityH = 0x1000,
		IdentityV = 0x1001
	};

	enum class PdfFilter
	{
		Unknown = 0,
		ASCIIHex = 1,
		ASCII85 = 2,
		Flate = 0x100,
		LZW = 0x101,
		RunLength = 0x102,
		DCT = 0x200,
		CCITTFax = 0x201
	};

	enum class PdfColorSpaceType
	{
		Unknown = 0,
		RGB = 1,
		Gray = 2,
		CMYK = 3,
		Lab = 4,
		Indexed = 5,
		Pattern = 6
	};

	enum class PdfImageType
	{
		Normal = 0,
		Jpeg = 1,
		Fax = 2
	};

	enum class PdfTextRenderingMode
	{
		Fill = 0,
		Stroke = 1,
		FillStroke = 2,
		Invisible = 3,
		FillClip = 4,
		StrokeClip = 5,
		FillStrokeClip = 6,
		Clip = 7,
	};

	SLIB_DEFINE_FLAGS(PdfFontFlags, {
		Normal = 0,
		FixedPitch = 1,
		Serif = 2,
		Symbolic = 4,
		Script = 8,
		NonSymbolic = 32,
		Italic = 64,
		AllCap = (1 << 16),
		SmallCap = (1 << 17),
		Bold = (1 << 18)
	})

	class PdfDocument;
	class PdfValue;
	class PdfStream;
	class PdfPage;
	class PdfFont;
	class PdfImage;
	class Canvas;
	class Image;
	class Color;
	class Font;
	class FreeType;
	class FreeTypeGlyph;

	typedef HashMap<String, PdfValue> PdfDictionary;
	typedef List<PdfValue> PdfArray;

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

	class SLIB_EXPORT PdfValue
	{
	public:
		PdfValue() noexcept {}

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(PdfValue)

	public:
		PdfValue(sl_null_t) noexcept: m_var(sl_null, (sl_uint8)(PdfValueType::Null)) {}

		PdfValue(sl_bool v) noexcept;

		PdfValue(sl_int32 v) noexcept;
		PdfValue(sl_uint32 v) noexcept;
		PdfValue(float v) noexcept;

		PdfValue(const String& v) noexcept;
		PdfValue(String&& v) noexcept;

		PdfValue(const PdfArray& v) noexcept;
		PdfValue(PdfArray&& v) noexcept;

		PdfValue(const PdfDictionary& v) noexcept;
		PdfValue(PdfDictionary&& v) noexcept;

		PdfValue(const Ref<PdfStream>& v) noexcept;
		PdfValue(Ref<PdfStream>&& v) noexcept;

		PdfValue(const PdfName& v) noexcept;
		PdfValue(PdfName&& v) noexcept;

		PdfValue(const PdfReference& v) noexcept;

	public:
		const Variant& getVariant() const noexcept
		{
			return m_var;
		}

		Variant& getVariant() noexcept
		{
			return m_var;
		}

		SLIB_CONSTEXPR PdfValueType getType() const
		{
			return (PdfValueType)(m_var.getTag());
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

		const PdfArray& getArray() const& noexcept;

		PdfArray getArray()&& noexcept;

		const PdfDictionary& getDictionary() const& noexcept;

		PdfDictionary getDictionary()&& noexcept;

		const Ref<PdfStream>& getStream() const& noexcept;

		Ref<PdfStream> getStream()&& noexcept;

		PdfReference getReference() const noexcept;

		sl_bool getReference(PdfReference& _out) const noexcept;


		Rectangle getRectangle() const noexcept;

		sl_bool getRectangle(Rectangle& outRect) const noexcept;

	private:
		Variant m_var;

	};

	class SLIB_EXPORT PdfContentReader
	{
	public:
		virtual Memory readContent(sl_uint32 offset, sl_uint32 size, const PdfReference& ref) = 0;

	};

	class SLIB_EXPORT PdfStream : public Referable
	{
		SLIB_DECLARE_OBJECT

	public:
		PdfDictionary properties;

	public:
		PdfStream() noexcept;

		~PdfStream();

	public:
		void initialize(const PdfDictionary& properties, const PdfReference& ref, sl_uint32 offsetContent, sl_uint32 sizeContent) noexcept;

		PdfValue getProperty(const String& name) noexcept;

		PdfValue getProperty(const String& name, const String& alternateName) noexcept;

		Memory getEncodedContent(PdfContentReader* reader);

		void setEncodedContent(const Memory& content) noexcept;

		Memory getDecodedContent(PdfContentReader* reader);

		Memory getDecodedContent(const Memory& content);

	private:
		AtomicMemory m_contentEncoded;
		PdfReference m_ref;
		sl_uint32 m_offsetContent;
		sl_uint32 m_sizeContent;

	};

	class SLIB_EXPORT PdfFlateOrLZWDecodeParams
	{
	public:
		sl_uint32 predictor;
		sl_uint32 columns;
		sl_uint32 bitsPerComponent;
		sl_uint32 colors;
		sl_uint32 earlyChange;

	public:
		PdfFlateOrLZWDecodeParams();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(PdfFlateOrLZWDecodeParams)

	public:
		void setParams(const PdfDictionary& dict) noexcept;

		sl_uint32 predict(void* content, sl_uint32 size) noexcept;

	};

	class SLIB_EXPORT PdfCCITTFaxDecodeParams
	{
	public:
		sl_int32 K; // encoding
		sl_uint32 columns;
		sl_uint32 rows;
		sl_bool flagEndOfLine;
		sl_bool flagByteAlign;
		sl_bool flagBlackIs1;

	public:
		PdfCCITTFaxDecodeParams();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(PdfCCITTFaxDecodeParams)

	public:
		void setParams(const PdfDictionary& dict) noexcept;

	};

	class SLIB_EXPORT PdfResourceContext : public Referable
	{
	public:
		ExpiringMap< sl_uint32, Ref<PdfFont> > fonts;
		ExpiringMap< sl_uint32, Ref<PdfImage> > images;

	public:
		PdfResourceContext();

		~PdfResourceContext();

	};

	class SLIB_EXPORT PdfColorSpace
	{
	public:
		PdfColorSpaceType type;
		Array<Color> indices;
		
	public:
		PdfColorSpace();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(PdfColorSpace)

	public:
		void load(PdfPage* page, const String& name);

		void load(PdfDocument* doc, const PdfValue& value);

		sl_uint32 getComponentsCount();

		sl_bool getColor(Color& _out, const PdfValue* values, sl_size count);

		sl_bool getColorAt(Color& _out, sl_uint32 index);

		static sl_bool getColorFromRGB(Color& _out, const PdfValue* values, sl_size count);

		static sl_bool getColorFromGray(Color& _out, const PdfValue* values, sl_size count);

		static sl_bool getColorFromCMYK(Color& _out, const PdfValue* values, sl_size count);

		static sl_bool getColorFromLab(Color& _out, const PdfValue* values, sl_size count);

	private:
		sl_bool _loadName(const String& name);

		sl_bool _loadIndexed(PdfDocument* doc, sl_uint32 maxIndex, const PdfValue& table);

	};

	class SLIB_EXPORT PdfFontDescriptor
	{
	public:
		String name;
		String family;
		float ascent;
		float descent;
		float leading;
		float weight;
		float italicAngle;
		sl_uint32 flags;
		PdfReference content;

	public:
		PdfFontDescriptor();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(PdfFontDescriptor)

	public:
		void load(const PdfDictionary& dict) noexcept;

	};

	class SLIB_EXPORT PdfCidFontInfo
	{
	public:
		PdfFontSubtype subtype;
		float defaultWidth;
		HashMap<sl_uint32, float> widths;
		String cidToGidMapName;
		sl_bool flagCidIsGid;

	public:
		PdfCidFontInfo();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(PdfCidFontInfo)

	public:
		void load(PdfDocument* doc, const PdfDictionary& dict) noexcept;

		float getWidth(sl_uint32 code);

	};

	class SLIB_EXPORT PdfFontResource
	{
	public:
		PdfFontSubtype subtype;
		String baseFont;
		sl_uint32 firstChar;
		sl_uint32 lastChar;
		Array<float> widths;
		PdfEncoding encoding;
		HashMap<sl_uint32, String> encodingMap;
		PdfFontDescriptor descriptor;
		PdfCidFontInfo cid;
		HashMap<sl_uint16, sl_uint32> toUnicode;
		sl_uint32 codeLength;

	public:
		PdfFontResource();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(PdfFontResource)

	public:
		sl_bool load(PdfDocument* doc, const PdfDictionary& dict);

		sl_char32 getUnicode(sl_uint32 charcode);

	public:
		static PdfFontSubtype getSubtype(const StringView& subtype) noexcept;

	};

	class SLIB_EXPORT PdfFont : public Referable, public PdfFontResource
	{
		SLIB_DECLARE_OBJECT

	public:
		Ref<FreeType> face;
		sl_real scale;

	public:
		PdfFont();

		~PdfFont();

	public:
		static Ref<PdfFont> load(PdfDocument* doc, const PdfReference& ref, PdfResourceContext& context);

	public:
		sl_uint32 getGlyphIndex(sl_uint32 charcode, sl_char32 unicode);

		Ref<FreeTypeGlyph> getGlyph(sl_uint32 charcode, sl_char32 unicode);

		float getCharWidth(sl_uint32 charcode, sl_char32 unicode);

	protected:
		sl_bool _load(PdfDocument* doc, const PdfDictionary& dict);

	private:
		ExpiringMap< sl_uint32, Ref<FreeTypeGlyph> > m_cacheGlyphs;

	};

	class SLIB_EXPORT PdfImageResource
	{
	public:
		sl_uint32 width;
		sl_uint32 height;
		PdfColorSpace colorSpace;
		sl_uint32 bitsPerComponent;
		sl_bool flagImageMask;
		sl_bool flagInterpolate;
		sl_bool flagUseDecodeArray;
		sl_uint8 decodeMin[4];
		sl_uint8 decodeMax[4];
		sl_bool flagUseMatte;
		Color matte;

		PdfReference mask;
		PdfReference smask;

	public:
		PdfImageResource();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(PdfImageResource)

	public:
		sl_bool load(PdfDocument* doc, PdfStream* stream) noexcept;

		void applyDecode4(sl_uint8* colors, sl_uint32 cols, sl_uint32 rows, sl_reg pitch) noexcept;
		
		void applyDecode(Image* image) noexcept;

	};

	class SLIB_EXPORT PdfImage : public Referable, public PdfImageResource
	{
		SLIB_DECLARE_OBJECT

	public:
		Ref<Image> object;

	public:
		PdfImage();

		~PdfImage();

	public:
		static Ref<PdfImage> load(PdfDocument* doc, const PdfReference& ref, PdfResourceContext& context);

	protected:
		sl_bool _load(PdfDocument* doc, const PdfReference& ref);

		void _loadSMask(PdfDocument* doc);

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
		static PdfOperator getOperator(const StringView& opName) noexcept;

		static PdfCMapOperator getCMapOperator(const StringView& opName) noexcept;

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

		PdfValue getAttribute(const String& name);

	protected:
		sl_bool m_flagPage;

	};

	class SLIB_EXPORT PdfRenderContext : public PdfResourceContext
	{
	public:
		PdfRenderContext();

		~PdfRenderContext();

	};

	class SLIB_EXPORT PdfRenderParam
	{
	public:
		Canvas* canvas;
		Rectangle bounds;

		Ref<PdfRenderContext> context;

	public:
		PdfRenderParam();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(PdfRenderParam)

	};

	class SLIB_EXPORT PdfPage : public PdfPageTreeItem
	{
		SLIB_DECLARE_OBJECT

	public:
		PdfPage() noexcept;

		~PdfPage();

	public:
		Ref<PdfDocument> getDocument();

		Memory getContentData();

		List<PdfOperation> getContent();

		static List<PdfOperation> parseContent(const void* data, sl_size size);

		void render(PdfRenderParam& param);

		Rectangle getMediaBox();

		Rectangle getCropBox();

		PdfValue getResources(const String& type, sl_bool flagResolveReference = sl_true);

		PdfValue getResource(const String& type, const String& name, sl_bool flagResolveReference = sl_true);

		PdfDictionary getFontResourceAsDictionary(const String& name);

		sl_bool getFontResource(const String& name, PdfReference& outRef);

		sl_bool getFontResource(const String& name, PdfFontResource& outResource);

		PdfValue getExternalObjectResource(const String& name);

		sl_bool getExternalObjectResource(const String& name, PdfReference& outRef);

	protected:
		WeakRef<PdfDocument> m_document;
		AtomicList<PdfOperation> m_content;
		sl_bool m_flagContent;

		friend class PdfDocument;
	};

	class SLIB_EXPORT PdfDocument : public Object, public PdfContentReader
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
		PdfValue getObject(const PdfReference& ref);

		PdfValue getObject(const PdfValue& refOrValue);

		Memory decodeStreamContent(PdfStream* stream);

		Memory decodeStreamContent(const PdfReference& ref);

		Memory decodeStreamContent(const PdfValue& refOrStream);

		List< Ref<PdfStream> > getAllStreams();

		sl_uint32 getPagesCount();

		Ref<PdfPage> getPage(sl_uint32 index);

		sl_bool isEncrypted();

		static sl_bool isEncryptedFile(const StringParam& path);

		sl_bool isAuthenticated();

		sl_bool setUserPassword(const StringView& password);
		
	public:
		// PdfContentReader
		Memory readContent(sl_uint32 offset, sl_uint32 size, const PdfReference& ref) override;

	protected:
		sl_bool _openFile(const StringParam& filePath);

		sl_bool _openMemory(const Memory& mem);

	private:
		Ref<Referable> m_context;

		friend class PdfPage;

	};

	class SLIB_EXPORT Pdf
	{
	public:
		static const sl_char16* getUnicodeTable(PdfEncoding encoding) noexcept;

		static const char* const* getCharNameTable(PdfEncoding encoding) noexcept;

		static PdfFilter getFilter(const StringView& name) noexcept;

		static PdfEncoding getEncoding(const StringView& name) noexcept;

	};

}

#endif
