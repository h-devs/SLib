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
		Reference = 11,
		Image = 12
	};

	enum class PdfFunctionType
	{
		Unknown = -1,
		Sampled = 0,
		Exponential = 2,
		Stiching = 3,
		PostScript = 4
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

	enum class PdfPatternType
	{
		Unknown = 0,
		Tiling = 1,
		Shading = 2
	};

	enum class PdfShadingType
	{
		Unknown = 0,
		Function = 1,
		Axial = 2,
		Radial = 3,
		Free = 4, // Free-form Gouraud-shaded triangle mesh
		Lattice = 5, // Lattice-form Gouraud-shaded triangle mesh
		Coons = 6, // Coons patch mesh
		Tensor = 7 // Tensor-product patch mesh
	};

	enum class PdfExternalObjectType
	{
		Unknown = 0,
		Image = 1,
		Form = 2
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
	class PdfDictionary;
	class PdfArray;
	class PdfStream;
	class PdfPage;
	class PdfFont;
	class PdfExternalObject;
	class PdfImage;
	class PdfOperation;
	class Canvas;
	class Image;
	class Color;
	class Brush;
	class Font;
	class FreeType;
	class FreeTypeGlyph;
	class IWriter;

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

		explicit PdfValue(sl_bool v) noexcept;

		PdfValue(sl_int32 v) noexcept;
		PdfValue(sl_uint32 v) noexcept;
		PdfValue(float v) noexcept;

		PdfValue(const String& v) noexcept;
		PdfValue(String&& v) noexcept;

		PdfValue(const PdfName& v) noexcept;
		PdfValue(PdfName&& v) noexcept;

		PdfValue(const PdfReference& v) noexcept;

		PdfValue(const Ref<PdfArray>& v) noexcept;
		PdfValue(Ref<PdfArray>&& v) noexcept;
		PdfValue(PdfArray* v) noexcept;

		PdfValue(const Ref<PdfDictionary>& v) noexcept;
		PdfValue(Ref<PdfDictionary>&& v) noexcept;
		PdfValue(PdfDictionary*) noexcept;

		PdfValue(const Ref<PdfStream>& v) noexcept;
		PdfValue(Ref<PdfStream>&& v) noexcept;
		PdfValue(PdfStream*) noexcept;

		PdfValue(const Ref<PdfImage>& v) noexcept;
		PdfValue(Ref<PdfImage>&& v) noexcept;
		PdfValue(PdfImage* v) noexcept;

		PdfValue(const Rectangle& v) noexcept;

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

		const String& getString() const& noexcept;

		String getString()&& noexcept;

		const String& getName() const& noexcept;

		String getName()&& noexcept;

		sl_bool equalsName(const StringView& name) const noexcept;

		PdfReference getReference() const noexcept;

		sl_bool getReference(PdfReference& _out) const noexcept;

		const Ref<PdfArray>& getArray() const& noexcept;

		Ref<PdfArray> getArray()&& noexcept;

		// Not resolve indirect reference
		const List<PdfValue>& getElements() const& noexcept;

		// Not resolve indirect reference
		List<PdfValue> getElements() && noexcept;

		sl_uint32 getElementCount() const noexcept;

		PdfValue getElement(sl_size index, sl_bool flagResolveReference = sl_true) const;

		const Ref<PdfDictionary>& getDictionary() const& noexcept;

		Ref<PdfDictionary> getDictionary()&& noexcept;

		PdfValue getItem(const String& name, sl_bool flagResolveReference = sl_true) const;

		PdfValue getItem(const String& name, const String& alternateName, sl_bool flagResolveReference = sl_true) const;

		const Ref<PdfStream>& getStream() const& noexcept;

		Ref<PdfStream> getStream()&& noexcept;

		Memory getDecodedStreamContent() const;

		const Ref<PdfImage>& getImage() const& noexcept;

		Ref<PdfImage> getImage() && noexcept;


		Rectangle getRectangle() const noexcept;

		sl_bool getRectangle(Rectangle& _out) const noexcept;

		Matrix3 getMatrix() const noexcept;

		sl_bool getMatrix(Matrix3& _out) const noexcept;

	public:
		PdfValue operator[](const String& name) const;

		PdfValue operator[](sl_size index) const;

	private:
		Variant m_var;

	};

	class SLIB_EXPORT PdfArray : public CList<PdfValue>
	{
		SLIB_DECLARE_OBJECT

	public:
		PdfArray(Referable* context) noexcept;

		~PdfArray();

	public:
		sl_uint32 getCount() const noexcept;

		PdfValue get(sl_size index, sl_bool flagResolveReference = sl_true) const;

	public:
		static Ref<PdfArray> create(const Rectangle& rc);

	private:
		WeakRef<Referable> m_context;

	};

	class SLIB_EXPORT PdfDictionary : public CHashMap<String, PdfValue>
	{
		SLIB_DECLARE_OBJECT

	public:
		PdfDictionary(Referable* context) noexcept;

		~PdfDictionary();

	public:
		PdfValue get(const String& name, sl_bool flagResolveReference = sl_true) const;

		PdfValue get(const String& name, const String& alternateName, sl_bool flagResolveReference = sl_true) const;

	private:
		WeakRef<Referable> m_context;

	};

	class SLIB_EXPORT PdfStream : public Referable
	{
		SLIB_DECLARE_OBJECT

	public:
		Ref<PdfDictionary> properties;

	public:
		PdfStream(Referable* context) noexcept;

		~PdfStream();

	public:
		void initialize(const Ref<PdfDictionary>& properties, const PdfReference& ref, sl_uint32 offsetContent, sl_uint32 sizeContent) noexcept;

		PdfValue getProperty(const String& name, sl_bool flagResolveReference = sl_true) noexcept;

		PdfValue getProperty(const String& name, const String& alternateName, sl_bool flagResolveReference = sl_true) noexcept;

		Memory getEncodedContent();

		void setEncodedContent(const Memory& content) noexcept;

		Memory getDecodedContent();

		Memory getDecodedContent(const Memory& content);

		Memory getFilterInput(PdfFilter filter);

		Memory decodeContent(const MemoryView& input, PdfFilter filter, PdfDictionary* decodeParam);

		sl_bool isJpegImage() noexcept;

		void setJpegFilter() noexcept;

		void setLength(sl_uint32 len) noexcept;

	public:
		static Ref<PdfStream> create(const Memory& content);

		static Ref<PdfStream> createJpegImage(sl_uint32 width, sl_uint32 height, const Memory& content);

	private:
		WeakRef<Referable> m_context;
		AtomicMemory m_contentEncoded;
		PdfReference m_ref;
		sl_uint32 m_offsetContent;
		sl_uint32 m_sizeContent;

	};

	class SLIB_EXPORT PdfFunction
	{
	public:
		PdfFunctionType type;
		sl_uint32 countInput;
		sl_uint32 countOutput;
		Array< Pair<float, float> > domain; // input range
		Array< Pair<float, float> > range; // output range. used for Sampled/PostScript types
		
		// Sampled Function type
		sl_uint32 bitsPerSample;
		Array< Pair<sl_uint32, sl_uint32> > encodeSampled;
		Array<sl_uint32> size;
		Array<sl_uint32> stride;
		Array< Pair<float, float> > decode;
		Array< Array<float> > samples;

		// Exponential
		Array<float> C0;
		Array<float> C1;
		float N;

		// Stiching
		Array<PdfFunction> functions;
		Array<float> bounds;
		Array< Pair<float, float> > encodeStiching;

	public:
		PdfFunction();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(PdfFunction)

	public:
		sl_bool load(const PdfValue& value);

		sl_bool call(float* input, sl_size nInput, float* output, sl_size nOutput);

	};

	class SLIB_EXPORT PdfResourceProvider
	{
	public:
		virtual PdfValue getResources(const String& type, sl_bool flagResolveReference = sl_true) = 0;

		virtual PdfValue getResource(const String& type, const String& name, sl_bool flagResolveReference = sl_true) = 0;

	public:
		sl_bool getFontResource(const String& name, PdfReference& outRef);

		sl_bool getExternalObjectResource(const String& name, PdfReference& outRef);

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
		void setParams(PdfDictionary* dict) noexcept;

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
		void setParams(PdfDictionary* dict) noexcept;

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
		void load(const PdfValue& value, PdfResourceProvider* res = sl_null);

		sl_uint32 getComponentCount();

		sl_bool getColor(Color& _out, const PdfValue* values, sl_size count);

		sl_bool getColorAt(Color& _out, sl_uint32 index);

		static sl_bool getColorFromRGB(Color& _out, const PdfValue* values, sl_size count);

		static sl_bool getColorFromGray(Color& _out, const PdfValue* values, sl_size count);

		static sl_bool getColorFromCMYK(Color& _out, const PdfValue* values, sl_size count);

		static sl_bool getColorFromLab(Color& _out, const PdfValue* values, sl_size count);

	private:
		void _load(const PdfValue& value, PdfResourceProvider* res, sl_bool flagICCBasedAlternate);

		sl_bool _loadName(const String& name);

		void _loadArray(PdfArray* array, sl_bool flagICCBasedAlternate);

		sl_bool _loadIndexed(sl_uint32 maxIndex, const PdfValue& table);

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
		Ref<PdfStream> content;

	public:
		PdfFontDescriptor();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(PdfFontDescriptor)

	public:
		void load(PdfDictionary* dict) noexcept;

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
		void load(PdfDictionary* dict) noexcept;

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
		sl_bool load(PdfDictionary* dict);

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

	protected:
		PdfFont();

		~PdfFont();

	public:
		static Ref<PdfFont> load(PdfDictionary* dict);

	public:
		sl_uint32 getGlyphIndex(sl_uint32 charcode, sl_char32 unicode);

		Ref<FreeTypeGlyph> getGlyph(sl_uint32 charcode, sl_char32 unicode);

		float getCharWidth(sl_uint32 charcode, sl_char32 unicode);

	protected:
		sl_bool _load(PdfDictionary* dict);

	private:
		ExpiringMap< sl_uint32, Ref<FreeTypeGlyph> > m_cacheGlyphs;

	};

	class SLIB_EXPORT PdfExternalObject : public Referable
	{
		SLIB_DECLARE_OBJECT

	public:
		PdfExternalObjectType type;

	protected:
		PdfExternalObject(PdfExternalObjectType type);

		~PdfExternalObject();

	public:
		static Ref<PdfExternalObject> load(PdfStream* stream);

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

		PdfValue mask;
		Ref<PdfStream> smask;

	public:
		PdfImageResource();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(PdfImageResource)

	public:
		sl_bool load(PdfStream* stream, PdfResourceProvider* resources = sl_null) noexcept;

		void applyDecode4(sl_uint8* colors, sl_uint32 cols, sl_uint32 rows, sl_reg pitch) noexcept;
		
		void applyDecode(Image* image) noexcept;

	protected:
		void _load(PdfStream* stream, PdfResourceProvider* resources) noexcept;

	};

	class SLIB_EXPORT PdfImage : public PdfExternalObject, public PdfImageResource
	{
		SLIB_DECLARE_OBJECT

	public:
		Ref<Image> object;

	protected:
		PdfImage();

		~PdfImage();

	public:
		static Ref<PdfImage> load(PdfStream* stream, PdfResourceProvider* resources = sl_null);

		static Ref<PdfImage> loadInline(PdfResourceProvider* resources, const void* data, sl_uint32& size);

	protected:
		static Ref<PdfImage> _load(PdfStream* stream, PdfResourceProvider* resources = sl_null);

		sl_bool _load(PdfStream* stream, PdfResourceProvider* resources, Memory& content);

		sl_bool _loadContent(Memory& content);

		sl_bool _loadInline(PdfResourceProvider* resources, const Ref<PdfDictionary>& properties, sl_uint8* data, sl_uint32& size);

		sl_bool _loadInlineContent(PdfStream* stream, sl_uint8* data, sl_uint32 size);

		void _loadSMask();

		static void _growSize(Ref<Image>& image, sl_uint32 minWidth, sl_uint32 minHeight);

		static void _restrictSize(Ref<Image>& image);

		friend class PdfExternalObject;

	};

	class SLIB_EXPORT PdfFormResource
	{
	public:
		Rectangle bounds;
		Matrix3 matrix;
		Ref<PdfDictionary> resources;
		List<PdfOperation> content;

	public:
		PdfFormResource();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(PdfFormResource)

	public:
		sl_bool load(PdfStream* stream);

	protected:
		void _load(PdfStream* stream);

	};

	class SLIB_EXPORT PdfForm : public PdfExternalObject, public PdfFormResource, public PdfResourceProvider
	{
		SLIB_DECLARE_OBJECT

	protected:
		PdfForm();

		~PdfForm();

	public:
		static Ref<PdfForm> load(PdfStream* stream);

	public:
		PdfValue getResources(const String& type, sl_bool flagResolveReference) override;

		PdfValue getResource(const String& type, const String& name, sl_bool flagResolveReference) override;

	protected:
		static Ref<PdfForm> _load(PdfStream* stream);

		sl_bool _load(PdfStream* stream, const MemoryView& content);

	private:
		friend class PdfExternalObject;
	};

	class SLIB_EXPORT PdfShadingResource
	{
	public:
		PdfShadingType type;
		PdfColorSpace colorSpace;

		// Axial & Radial
		float domainStart, domainEnd;
		Point coordsStart, coordsEnd;
		PdfFunction function; // 1-in, n-out
		Array<PdfFunction> functions; // n 1-in, 1-out
		// Radial
		float radiusStart, radiusEnd;

	public:
		PdfShadingResource();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(PdfShadingResource)

	public:
		sl_bool load(PdfDictionary* dict);

		sl_bool getColor(float t, Color& _out);

		Ref<Brush> getBrush(const Matrix3& transform);

	};

	class SLIB_EXPORT PdfPatternResource
	{
	public:
		PdfPatternType type;
		PdfShadingResource shading;
		Matrix3 matrix;

	public:
		PdfPatternResource();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(PdfPatternResource)

	public:
		sl_bool load(PdfDictionary* dict);

	};

	class SLIB_EXPORT PdfOperation
	{
	public:
		PdfOperator op;
		List<PdfValue> operands;

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
		PdfReference reference;
		WeakRef<PdfPageTreeItem> parent;
		Ref<PdfDictionary> attributes; // NotNull

	protected:
		PdfPageTreeItem() noexcept;

		~PdfPageTreeItem();

	public:
		sl_bool isPage();

		PdfValue getAttribute(const String& name);

	protected:
		sl_bool m_flagPage;

	};

	class SLIB_EXPORT PdfResourceCache : public Referable
	{
	public:
		sl_bool flagUseFontsCache;
		ExpiringMap< sl_uint32, Ref<PdfFont> > fonts;
		sl_bool flagUseExternalObjectsCache;
		ExpiringMap< sl_uint32, Ref<PdfExternalObject> > externalObjects;

	public:
		PdfResourceCache();

		~PdfResourceCache();

	};

	class SLIB_EXPORT PdfRenderParam
	{
	public:
		Canvas* canvas;
		Rectangle bounds;
		Ref<PdfResourceCache> cache;

	public:
		PdfRenderParam();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(PdfRenderParam)

	};

	class SLIB_EXPORT PdfPage : public PdfPageTreeItem, public PdfResourceProvider
	{
		SLIB_DECLARE_OBJECT

	public:
		PdfPage(Referable* context) noexcept;

		~PdfPage();

	public:
		Memory getContentData();

		List<PdfOperation> getContent();

		static List<PdfOperation> parseContent(PdfResourceProvider* resources, const void* data, sl_size size);

		void render(PdfRenderParam& param);

		Rectangle getMediaBox();

		Rectangle getCropBox();

		PdfValue getResources(const String& type, sl_bool flagResolveReference = sl_true) override;

		PdfValue getResource(const String& type, const String& name, sl_bool flagResolveReference = sl_true) override;

	protected:
		WeakRef<Referable> m_context;
		AtomicList<PdfOperation> m_content;
		sl_bool m_flagContent;

	};

	class SLIB_EXPORT PdfDocumentParam
	{
	public:
		StringParam filePath;
		Memory content;

		StringParam password;

	public:
		PdfDocumentParam();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(PdfDocumentParam)

	};

	class SLIB_EXPORT PdfDocument : public Referable
	{
		SLIB_DECLARE_OBJECT

	public:
		sl_uint32 fileSize;

	protected:
		PdfDocument();

		~PdfDocument();

	public:
		static Ref<PdfDocument> create();

		static Ref<PdfDocument> open(const PdfDocumentParam& param);

		static Ref<PdfDocument> openFile(const StringParam& filePath, const StringParam& password = sl_null);

		static Ref<PdfDocument> openMemory(const Memory& mem, const StringParam& password = sl_null);

	public:
		sl_uint32 getMaximumObjectNumber();

		PdfValue getObject(const PdfReference& ref);

		PdfValue getObject(sl_uint32 objectNumber, sl_uint32& outGeneration);

		Ref<PdfStream> getStream(sl_uint32 objectNumber, sl_uint32& outGeneration);

		sl_bool setObject(const PdfReference& ref, const PdfValue& value);

		sl_bool addObject(const PdfValue& value, PdfReference& outRef);

		sl_bool deleteObject(const PdfReference& ref);

		sl_uint32 getPageCount();

		Ref<PdfPage> getPage(sl_uint32 index);

		sl_bool addJpegImagePage(sl_uint32 width, sl_uint32 height, const Memory& jpeg);

		sl_bool insertJpegImagePage(sl_uint32 index, sl_uint32 width, sl_uint32 height, const Memory& jpeg);

		sl_bool deletePage(sl_uint32 index);

		Memory save();

		sl_bool save(IWriter* writer);

		Ref<PdfFont> getFont(const PdfReference& ref, PdfResourceCache& cache);

		Ref<PdfExternalObject> getExternalObject(const PdfReference& ref, PdfResourceCache& cache);

		sl_bool isEncrypted();

		sl_bool isAuthenticated();

	private:
		Ref<Referable> m_context; // NotNull

		friend class Pdf;
	};

	class SLIB_EXPORT Pdf
	{
	public:
		static const sl_char16* getUnicodeTable(PdfEncoding encoding) noexcept;

		static const char* const* getCharNameTable(PdfEncoding encoding) noexcept;

		static PdfFilter getFilter(const StringView& name) noexcept;

		static PdfEncoding getEncoding(const StringView& name) noexcept;


		static sl_bool isPdfFile(const StringParam& path);

		static sl_bool isEncryptedFile(const StringParam& path);

	};

}

#endif
