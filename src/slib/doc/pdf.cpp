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

#include "slib/core/thread.h"
#include "slib/core/mio.h"
#include "slib/core/string_buffer.h"
#include "slib/core/memory_buffer.h"
#include "slib/core/queue.h"
#include "slib/core/safe_static.h"
#include "slib/io/file_io.h"
#include "slib/io/buffered_seekable_reader.h"
#include "slib/io/memory_output.h"
#include "slib/io/sample_reader.h"
#include "slib/data/zlib.h"
#include "slib/data/lzw.h"
#include "slib/crypto/md5.h"
#include "slib/crypto/rc4.h"
#include "slib/graphics/freetype.h"
#include "slib/graphics/font.h"
#include "slib/graphics/canvas.h"
#include "slib/graphics/path.h"
#include "slib/graphics/cmyk.h"
#include "slib/graphics/cie.h"
#include "slib/graphics/image.h"
#include "slib/math/transform2d.h"

#define MAX_PDF_FILE_SIZE 0x40000000
#define MAX_WORD_LENGTH 256
#define MAX_NAME_LENGTH 127
#define MAX_STRING_LENGTH 32767
#define EXPIRE_DURATION_OBJECT 5000
#define EXPIRE_DURATION_OBJECT_STREAM 10000
#define EXPIRE_DURATION_FONT_GLYPH 15000
#define MAX_IMAGE_WIDTH 1000
#define MAX_IMAGE_HEIGHT 700

#define MAKE_OBJECT_ID(NUM, GEN) SLIB_MAKE_QWORD4(GEN, NUM)
#define CAST_VAR(TYPE, VALUE) (*((TYPE*)((void*)(&(VALUE)))))
#define DEFINE_PDF_NAME(NAME) SLIB_STATIC_STRING(NAME, #NAME)

namespace slib
{

	namespace {

		namespace name
		{
			DEFINE_PDF_NAME(Type)
			DEFINE_PDF_NAME(Size)
			DEFINE_PDF_NAME(Length)
			DEFINE_PDF_NAME(Index)
			DEFINE_PDF_NAME(First)
			DEFINE_PDF_NAME(Extends)
			DEFINE_PDF_NAME(Prev)
			DEFINE_PDF_NAME(Filter)
			DEFINE_PDF_NAME(FunctionType)
			DEFINE_PDF_NAME(Range)
			DEFINE_PDF_NAME(Encode)
			DEFINE_PDF_NAME(BitsPerSample)
			DEFINE_PDF_NAME(C0)
			DEFINE_PDF_NAME(C1)
			DEFINE_PDF_NAME(Functions)
			DEFINE_PDF_NAME(Bounds)
			DEFINE_PDF_NAME(Encrypt);
			DEFINE_PDF_NAME(Root)
			DEFINE_PDF_NAME(Catalog)
			DEFINE_PDF_NAME(Pages)
			DEFINE_PDF_NAME(Count)
			DEFINE_PDF_NAME(Parent)
			DEFINE_PDF_NAME(Kids)
			DEFINE_PDF_NAME(Contents)
			DEFINE_PDF_NAME(ID)
			DEFINE_PDF_NAME(MediaBox)
			DEFINE_PDF_NAME(CropBox)
			DEFINE_PDF_NAME(Resources)
			DEFINE_PDF_NAME(ProcSet)
			DEFINE_PDF_NAME(PDF)
			DEFINE_PDF_NAME(ImageC)
			DEFINE_PDF_NAME(Page)
			DEFINE_PDF_NAME(XObject)
			DEFINE_PDF_NAME(Image)
			DEFINE_PDF_NAME(Form)
			DEFINE_PDF_NAME(Pattern)
			DEFINE_PDF_NAME(Font)
			DEFINE_PDF_NAME(Subtype)
			DEFINE_PDF_NAME(BaseFont)
			DEFINE_PDF_NAME(DescendantFonts)
			DEFINE_PDF_NAME(Encoding)
			DEFINE_PDF_NAME(BaseEncoding)
			DEFINE_PDF_NAME(Differences)
			DEFINE_PDF_NAME(FontDescriptor)
			DEFINE_PDF_NAME(FontName)
			DEFINE_PDF_NAME(FontFamily)
			DEFINE_PDF_NAME(Ascent)
			DEFINE_PDF_NAME(Descent)
			DEFINE_PDF_NAME(Leading)
			DEFINE_PDF_NAME(FontWeight)
			DEFINE_PDF_NAME(ItalicAngle)
			DEFINE_PDF_NAME(Flags)
			DEFINE_PDF_NAME(FontFile)
			DEFINE_PDF_NAME(FontFile2)
			DEFINE_PDF_NAME(FontFile3)
			DEFINE_PDF_NAME(FirstChar)
			DEFINE_PDF_NAME(LastChar)
			DEFINE_PDF_NAME(Widths)
			DEFINE_PDF_NAME(DW)
			DEFINE_PDF_NAME(CIDToGIDMap)
			DEFINE_PDF_NAME(ToUnicode)
			DEFINE_PDF_NAME(Identity)
			DEFINE_PDF_NAME(FlateDecode)
			DEFINE_PDF_NAME(Fl)
			DEFINE_PDF_NAME(DCTDecode)
			DEFINE_PDF_NAME(DCT)
			DEFINE_PDF_NAME(LZWDecode)
			DEFINE_PDF_NAME(LZW)
			DEFINE_PDF_NAME(RunLengthDecode)
			DEFINE_PDF_NAME(RL)
			DEFINE_PDF_NAME(ASCIIHexDecode)
			DEFINE_PDF_NAME(AHx)
			DEFINE_PDF_NAME(ASCII85Decode)
			DEFINE_PDF_NAME(A85)
			DEFINE_PDF_NAME(CCITTFaxDecode)
			DEFINE_PDF_NAME(CCF)
			DEFINE_PDF_NAME(Standard)
			DEFINE_PDF_NAME(Width)
			DEFINE_PDF_NAME(Height)
			DEFINE_PDF_NAME(ColorSpace)
			DEFINE_PDF_NAME(CS)
			DEFINE_PDF_NAME(DeviceRGB)
			DEFINE_PDF_NAME(RGB)
			DEFINE_PDF_NAME(DeviceGray)
			DEFINE_PDF_NAME(DeviceCMYK)
			DEFINE_PDF_NAME(CMYK)
			DEFINE_PDF_NAME(CalRGB)
			DEFINE_PDF_NAME(CalGray)
			DEFINE_PDF_NAME(CalCMYK)
			DEFINE_PDF_NAME(Lab)
			DEFINE_PDF_NAME(Indexed)
			DEFINE_PDF_NAME(ICCBased)
			DEFINE_PDF_NAME(Separation)
			DEFINE_PDF_NAME(DeviceN)
			DEFINE_PDF_NAME(DecodeParms)
			DEFINE_PDF_NAME(DP)
			DEFINE_PDF_NAME(Predictor)
			DEFINE_PDF_NAME(EarlyChange)
			DEFINE_PDF_NAME(Columns)
			DEFINE_PDF_NAME(BitsPerComponent)
			DEFINE_PDF_NAME(BPC)
			DEFINE_PDF_NAME(ImageMask)
			DEFINE_PDF_NAME(IM)
			DEFINE_PDF_NAME(Interpolate)
			DEFINE_PDF_NAME(Decode)
			DEFINE_PDF_NAME(Matte)
			DEFINE_PDF_NAME(Colors)
			DEFINE_PDF_NAME(Rows)
			DEFINE_PDF_NAME(EndOfLine)
			DEFINE_PDF_NAME(EncodedByteAlign)
			DEFINE_PDF_NAME(BlackIs1)
			DEFINE_PDF_NAME(SMask)
			DEFINE_PDF_NAME(Mask)
			DEFINE_PDF_NAME(Alternate)
			DEFINE_PDF_NAME(BBox)
			DEFINE_PDF_NAME(Matrix)
			DEFINE_PDF_NAME(Function)
			DEFINE_PDF_NAME(PatternType)
			DEFINE_PDF_NAME(Shading)
			DEFINE_PDF_NAME(ShadingType)
			DEFINE_PDF_NAME(Domain)
			DEFINE_PDF_NAME(Coords)
			DEFINE_PDF_NAME(ObjStm)
			DEFINE_PDF_NAME(XRefStm)
			DEFINE_PDF_NAME(XRef)
			DEFINE_PDF_NAME(D)
			DEFINE_PDF_NAME(F)
			DEFINE_PDF_NAME(G)
			DEFINE_PDF_NAME(H)
			DEFINE_PDF_NAME(I)
			DEFINE_PDF_NAME(K)
			DEFINE_PDF_NAME(N)
			DEFINE_PDF_NAME(O)
			DEFINE_PDF_NAME(P)
			DEFINE_PDF_NAME(R)
			DEFINE_PDF_NAME(U)
			DEFINE_PDF_NAME(V)
			DEFINE_PDF_NAME(W)
		}

		static const sl_uint8 g_encryptionPad[] = { 0x28, 0xbf, 0x4e, 0x5e, 0x4e, 0x75, 0x8a, 0x41, 0x64, 0x00, 0x4e, 0x56, 0xff, 0xfa, 0x01, 0x08, 0x2e, 0x2e, 0x00, 0xb6, 0xd0, 0x68, 0x3e, 0x80, 0x2f, 0x0c, 0xa9, 0xfe, 0x64, 0x53, 0x69, 0x7a };

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


		static const sl_char16 g_encodingStandard[256] = {
			0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
			0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
			0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
			0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0020, 0x0021, 0x0022, 0x0023,
			0x0024, 0x0025, 0x0026, 0x2019, 0x0028, 0x0029, 0x002a, 0x002b, 0x002c,
			0x002d, 0x002e, 0x002f, 0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035,
			0x0036, 0x0037, 0x0038, 0x0039, 0x003a, 0x003b, 0x003c, 0x003d, 0x003e,
			0x003f, 0x0040, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047,
			0x0048, 0x0049, 0x004a, 0x004b, 0x004c, 0x004d, 0x004e, 0x004f, 0x0050,
			0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057, 0x0058, 0x0059,
			0x005a, 0x005b, 0x005c, 0x005d, 0x005e, 0x005f, 0x2018, 0x0061, 0x0062,
			0x0063, 0x0064, 0x0065, 0x0066, 0x0067, 0x0068, 0x0069, 0x006a, 0x006b,
			0x006c, 0x006d, 0x006e, 0x006f, 0x0070, 0x0071, 0x0072, 0x0073, 0x0074,
			0x0075, 0x0076, 0x0077, 0x0078, 0x0079, 0x007a, 0x007b, 0x007c, 0x007d,
			0x007e, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
			0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
			0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
			0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x00a1,
			0x00a2, 0x00a3, 0x2044, 0x00a5, 0x0192, 0x00a7, 0x00a4, 0x0027, 0x201c,
			0x00ab, 0x2039, 0x203a, 0xfb01, 0xfb02, 0x0000, 0x2013, 0x2020, 0x2021,
			0x00b7, 0x0000, 0x00b6, 0x2022, 0x201a, 0x201e, 0x201d, 0x00bb, 0x2026,
			0x2030, 0x0000, 0x00bf, 0x0000, 0x0060, 0x00b4, 0x02c6, 0x02dc, 0x00af,
			0x02d8, 0x02d9, 0x00a8, 0x0000, 0x02da, 0x00b8, 0x0000, 0x02dd, 0x02db,
			0x02c7, 0x2014, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
			0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
			0x00c6, 0x0000, 0x00aa, 0x0000, 0x0000, 0x0000, 0x0000, 0x0141, 0x00d8,
			0x0152, 0x00ba, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x00e6, 0x0000,
			0x0000, 0x0000, 0x0131, 0x0000, 0x0000, 0x0142, 0x00f8, 0x0153, 0x00df,
			0x0000, 0x0000, 0x0000, 0x0000 };

		static const char* const g_charNamesStandard[256] = {
			sl_null, sl_null, sl_null, sl_null, sl_null, sl_null,
			sl_null, sl_null, sl_null, sl_null, sl_null, sl_null,
			sl_null, sl_null, sl_null, sl_null, sl_null, sl_null,
			sl_null, sl_null, sl_null, sl_null, sl_null, sl_null,
			sl_null, sl_null, sl_null, sl_null, sl_null, sl_null,
			sl_null, sl_null,
			"space", "exclam", "quotedbl", "numbersign", "dollar", "percent",
			"ampersand", "quoteright", "parenleft", "parenright", "asterisk",
			"plus", "comma", "hyphen", "period", "slash", "zero", "one", "two",
			"three", "four", "five", "six", "seven", "eight", "nine", "colon",
			"semicolon", "less", "equal", "greater", "question", "at", "A",
			"B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N",
			"O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z",
			"bracketleft", "backslash", "bracketright", "asciicircum", "underscore",
			"quoteleft", "a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k",
			"l", "m", "n", "o", "p", "q", "r", "s", "t", "u", "v", "w", "x",
			"y", "z", "braceleft", "bar", "braceright", "asciitilde", sl_null,
			sl_null, sl_null, sl_null, sl_null, sl_null, sl_null,
			sl_null, sl_null, sl_null, sl_null, sl_null, sl_null,
			sl_null, sl_null, sl_null, sl_null, sl_null, sl_null,
			sl_null, sl_null, sl_null, sl_null, sl_null, sl_null,
			sl_null, sl_null, sl_null, sl_null, sl_null, sl_null,
			sl_null, sl_null, sl_null, "exclamdown", "cent", "sterling",
			"fraction", "yen", "florin", "section", "currency", "quotesingle",
			"quotedblleft", "guillemotleft", "guilsinglleft", "guilsinglright",
			"fi", "fl", sl_null, "endash", "dagger", "daggerdbl", "periodcentered",
			sl_null, "paragraph", "bullet", "quotesinglbase", "quotedblbase",
			"quotedblright", "guillemotright", "ellipsis", "perthousand",
			sl_null, "questiondown", sl_null, "grave", "acute", "circumflex",
			"tilde", "macron", "breve", "dotaccent", "dieresis", sl_null,
			"ring", "cedilla", sl_null, "hungarumlaut", "ogonek", "caron",
			"emdash", sl_null, sl_null, sl_null, sl_null, sl_null,
			sl_null, sl_null, sl_null, sl_null, sl_null, sl_null,
			sl_null, sl_null, sl_null, sl_null, sl_null, "AE",
			sl_null, "ordfeminine", sl_null, sl_null, sl_null, sl_null,
			"Lslash", "Oslash", "OE", "ordmasculine", sl_null, sl_null,
			sl_null, sl_null, sl_null, "ae", sl_null, sl_null,
			sl_null, "dotlessi", sl_null, sl_null, "lslash", "oslash",
			"oe", "germandbls", sl_null, sl_null, sl_null, sl_null
		};

		static const sl_char16 g_encodingMacRoman[256] = {
			0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
			0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
			0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
			0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0020, 0x0021, 0x0022, 0x0023,
			0x0024, 0x0025, 0x0026, 0x0027, 0x0028, 0x0029, 0x002a, 0x002b, 0x002c,
			0x002d, 0x002e, 0x002f, 0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035,
			0x0036, 0x0037, 0x0038, 0x0039, 0x003a, 0x003b, 0x003c, 0x003d, 0x003e,
			0x003f, 0x0040, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047,
			0x0048, 0x0049, 0x004a, 0x004b, 0x004c, 0x004d, 0x004e, 0x004f, 0x0050,
			0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057, 0x0058, 0x0059,
			0x005a, 0x005b, 0x005c, 0x005d, 0x005e, 0x005f, 0x0060, 0x0061, 0x0062,
			0x0063, 0x0064, 0x0065, 0x0066, 0x0067, 0x0068, 0x0069, 0x006a, 0x006b,
			0x006c, 0x006d, 0x006e, 0x006f, 0x0070, 0x0071, 0x0072, 0x0073, 0x0074,
			0x0075, 0x0076, 0x0077, 0x0078, 0x0079, 0x007a, 0x007b, 0x007c, 0x007d,
			0x007e, 0x0000, 0x00c4, 0x00c5, 0x00c7, 0x00c9, 0x00d1, 0x00d6, 0x00dc,
			0x00e1, 0x00e0, 0x00e2, 0x00e4, 0x00e3, 0x00e5, 0x00e7, 0x00e9, 0x00e8,
			0x00ea, 0x00eb, 0x00ed, 0x00ec, 0x00ee, 0x00ef, 0x00f1, 0x00f3, 0x00f2,
			0x00f4, 0x00f6, 0x00f5, 0x00fa, 0x00f9, 0x00fb, 0x00fc, 0x2020, 0x00b0,
			0x00a2, 0x00a3, 0x00a7, 0x2022, 0x00b6, 0x00df, 0x00ae, 0x00a9, 0x2122,
			0x00b4, 0x00a8, 0x0000, 0x00c6, 0x00d8, 0x0000, 0x00b1, 0x0000, 0x0000,
			0x00a5, 0x00b5, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x00aa, 0x00ba,
			0x0000, 0x00e6, 0x00f8, 0x00bf, 0x00a3, 0x00ac, 0x0000, 0x0192, 0x0000,
			0x0000, 0x00ab, 0x00bb, 0x2026, 0x0020, 0x00c0, 0x00c3, 0x00d5, 0x0152,
			0x0153, 0x2013, 0x2014, 0x201c, 0x201d, 0x2018, 0x2019, 0x00f7, 0x0000,
			0x00ff, 0x0178, 0x2044, 0x00a4, 0x2039, 0x203a, 0xfb01, 0xfb02, 0x2021,
			0x00b7, 0x201a, 0x201e, 0x2030, 0x00c2, 0x00ca, 0x00c1, 0x00cb, 0x00c8,
			0x00cd, 0x00ce, 0x00cf, 0x00cc, 0x00d3, 0x00d4, 0x0000, 0x00d2, 0x00da,
			0x00db, 0x00d9, 0x0131, 0x02c6, 0x02dc, 0x00af, 0x02d8, 0x02d9, 0x02da,
			0x00b8, 0x02dd, 0x02db, 0x02c7 };

		static const char* const g_charNamesMacRoman[256] = {
			sl_null, sl_null, sl_null, sl_null, sl_null, sl_null,
			sl_null, sl_null, sl_null, sl_null, sl_null, sl_null,
			sl_null, sl_null, sl_null, sl_null, sl_null, sl_null,
			sl_null, sl_null, sl_null, sl_null, sl_null, sl_null,
			sl_null, sl_null, sl_null, sl_null, sl_null, sl_null,
			sl_null, sl_null,
			"space", "exclam", "quotedbl", "numbersign", "dollar", "percent",
			"ampersand", "quotesingle", "parenleft", "parenright", "asterisk",
			"plus", "comma", "hyphen", "period", "slash", "zero", "one", "two",
			"three", "four", "five", "six", "seven", "eight", "nine", "colon",
			"semicolon", "less", "equal", "greater", "question", "at", "A",
			"B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N",
			"O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z",
			"bracketleft", "backslash", "bracketright", "asciicircum", "underscore",
			"grave", "a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k",
			"l", "m", "n", "o", "p", "q", "r", "s", "t", "u", "v", "w", "x",
			"y", "z", "braceleft", "bar", "braceright", "asciitilde", sl_null,
			"Adieresis", "Aring", "Ccedilla", "Eacute", "Ntilde", "Odieresis",
			"Udieresis", "aacute", "agrave", "acircumflex", "adieresis", "atilde",
			"aring", "ccedilla", "eacute", "egrave", "ecircumflex", "edieresis",
			"iacute", "igrave", "icircumflex", "idieresis", "ntilde", "oacute",
			"ograve", "ocircumflex", "odieresis", "otilde", "uacute", "ugrave",
			"ucircumflex", "udieresis", "dagger", "degree", "cent", "sterling",
			"section", "bullet", "paragraph", "germandbls", "registered",
			"copyright", "trademark", "acute", "dieresis", sl_null, "AE",
			"Oslash", sl_null, "plusminus", sl_null, sl_null, "yen", "mu",
			sl_null, sl_null, sl_null, sl_null, sl_null, "ordfeminine",
			"ordmasculine", sl_null, "ae", "oslash", "questiondown", "exclamdown",
			"logicalnot", sl_null, "florin", sl_null, sl_null, "guillemotleft",
			"guillemotright", "ellipsis", "space", "Agrave", "Atilde", "Otilde",
			"OE", "oe", "endash", "emdash", "quotedblleft", "quotedblright",
			"quoteleft", "quoteright", "divide", sl_null, "ydieresis",
			"Ydieresis", "fraction", "currency", "guilsinglleft", "guilsinglright",
			"fi", "fl", "daggerdbl", "periodcentered", "quotesinglbase",
			"quotedblbase", "perthousand", "Acircumflex", "Ecircumflex", "Aacute",
			"Edieresis", "Egrave", "Iacute", "Icircumflex", "Idieresis", "Igrave",
			"Oacute", "Ocircumflex", sl_null, "Ograve", "Uacute", "Ucircumflex",
			"Ugrave", "dotlessi", "circumflex", "tilde", "macron", "breve",
			"dotaccent", "ring", "cedilla", "hungarumlaut", "ogonek", "caron"
		};

		static const sl_char16 g_encodingWinAnsi[256] = {
			0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
			0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
			0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
			0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0020, 0x0021, 0x0022, 0x0023,
			0x0024, 0x0025, 0x0026, 0x0027, 0x0028, 0x0029, 0x002a, 0x002b, 0x002c,
			0x002d, 0x002e, 0x002f, 0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035,
			0x0036, 0x0037, 0x0038, 0x0039, 0x003a, 0x003b, 0x003c, 0x003d, 0x003e,
			0x003f, 0x0040, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047,
			0x0048, 0x0049, 0x004a, 0x004b, 0x004c, 0x004d, 0x004e, 0x004f, 0x0050,
			0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057, 0x0058, 0x0059,
			0x005a, 0x005b, 0x005c, 0x005d, 0x005e, 0x005f, 0x0060, 0x0061, 0x0062,
			0x0063, 0x0064, 0x0065, 0x0066, 0x0067, 0x0068, 0x0069, 0x006a, 0x006b,
			0x006c, 0x006d, 0x006e, 0x006f, 0x0070, 0x0071, 0x0072, 0x0073, 0x0074,
			0x0075, 0x0076, 0x0077, 0x0078, 0x0079, 0x007a, 0x007b, 0x007c, 0x007d,
			0x007e, 0x2022, 0x20ac, 0x2022, 0x201a, 0x0192, 0x201e, 0x2026, 0x2020,
			0x2021, 0x02c6, 0x2030, 0x0160, 0x2039, 0x0152, 0x2022, 0x017d, 0x2022,
			0x2022, 0x2018, 0x2019, 0x201c, 0x201d, 0x2022, 0x2013, 0x2014, 0x02dc,
			0x2122, 0x0161, 0x203a, 0x0153, 0x2022, 0x017e, 0x0178, 0x0020, 0x00a1,
			0x00a2, 0x00a3, 0x00a4, 0x00a5, 0x00a6, 0x00a7, 0x00a8, 0x00a9, 0x00aa,
			0x00ab, 0x00ac, 0x002d, 0x00ae, 0x00af, 0x00b0, 0x00b1, 0x00b2, 0x00b3,
			0x00b4, 0x00b5, 0x00b6, 0x00b7, 0x00b8, 0x00b9, 0x00ba, 0x00bb, 0x00bc,
			0x00bd, 0x00be, 0x00bf, 0x00c0, 0x00c1, 0x00c2, 0x00c3, 0x00c4, 0x00c5,
			0x00c6, 0x00c7, 0x00c8, 0x00c9, 0x00ca, 0x00cb, 0x00cc, 0x00cd, 0x00ce,
			0x00cf, 0x00d0, 0x00d1, 0x00d2, 0x00d3, 0x00d4, 0x00d5, 0x00d6, 0x00d7,
			0x00d8, 0x00d9, 0x00da, 0x00db, 0x00dc, 0x00dd, 0x00de, 0x00df, 0x00e0,
			0x00e1, 0x00e2, 0x00e3, 0x00e4, 0x00e5, 0x00e6, 0x00e7, 0x00e8, 0x00e9,
			0x00ea, 0x00eb, 0x00ec, 0x00ed, 0x00ee, 0x00ef, 0x00f0, 0x00f1, 0x00f2,
			0x00f3, 0x00f4, 0x00f5, 0x00f6, 0x00f7, 0x00f8, 0x00f9, 0x00fa, 0x00fb,
			0x00fc, 0x00fd, 0x00fe, 0x00ff };

		static const char* const g_charNamesWinAnsi[256] = {
			sl_null, sl_null, sl_null, sl_null, sl_null, sl_null,
			sl_null, sl_null, sl_null, sl_null, sl_null, sl_null,
			sl_null, sl_null, sl_null, sl_null, sl_null, sl_null,
			sl_null, sl_null, sl_null, sl_null, sl_null, sl_null,
			sl_null, sl_null, sl_null, sl_null, sl_null, "space",
			sl_null, sl_null, sl_null,
			"exclam", "quotedbl", "numbersign", "dollar", "percent", "ampersand",
			"quotesingle", "parenleft", "parenright", "asterisk", "plus",
			"comma", "hyphen", "period", "slash", "zero", "one", "two", "three",
			"four", "five", "six", "seven", "eight", "nine", "colon", "semicolon",
			"less", "equal", "greater", "question", "at", "A", "B", "C", "D",
			"E", "F", "G", "H", "I", "J", "K", "L", "M", "N", "O", "P", "Q",
			"R", "S", "T", "U", "V", "W", "X", "Y", "Z", "bracketleft",
			"backslash", "bracketright", "asciicircum", "underscore", "grave",
			"a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m",
			"n", "o", "p", "q", "r", "s", "t", "u", "v", "w", "x", "y", "z",
			"braceleft", "bar", "braceright", "asciitilde", "bullet", "Euro",
			"bullet", "quotesinglbase", "florin", "quotedblbase", "ellipsis",
			"dagger", "daggerdbl", "circumflex", "perthousand", "Scaron",
			"guilsinglleft", "OE", "bullet", "Zcaron", "bullet", "bullet",
			"quoteleft", "quoteright", "quotedblleft", "quotedblright", "bullet",
			"endash", "emdash", "tilde", "trademark", "scaron", "guilsinglright",
			"oe", "bullet", "zcaron", "Ydieresis", "space", "exclamdown", "cent",
			"sterling", "currency", "yen", "brokenbar", "section", "dieresis",
			"copyright", "ordfeminine", "guillemotleft", "logicalnot", "hyphen",
			"registered", "macron", "degree", "plusminus", "twosuperior",
			"threesuperior", "acute", "mu", "paragraph", "periodcentered",
			"cedilla", "onesuperior", "ordmasculine", "guillemotright",
			"onequarter", "onehalf", "threequarters", "questiondown", "Agrave",
			"Aacute", "Acircumflex", "Atilde", "Adieresis", "Aring", "AE",
			"Ccedilla", "Egrave", "Eacute", "Ecircumflex", "Edieresis", "Igrave",
			"Iacute", "Icircumflex", "Idieresis", "Eth", "Ntilde", "Ograve",
			"Oacute", "Ocircumflex", "Otilde", "Odieresis", "multiply", "Oslash",
			"Ugrave", "Uacute", "Ucircumflex", "Udieresis", "Yacute", "Thorn",
			"germandbls", "agrave", "aacute", "acircumflex", "atilde", "adieresis",
			"aring", "ae", "ccedilla", "egrave", "eacute", "ecircumflex",
			"edieresis", "igrave", "iacute", "icircumflex", "idieresis", "eth",
			"ntilde", "ograve", "oacute", "ocircumflex", "otilde", "odieresis",
			"divide", "oslash", "ugrave", "uacute", "ucircumflex", "udieresis",
			"yacute", "thorn", "ydieresis"
		};

		static const sl_char16 g_encodingPdfDoc[256] = {
			0x0000, 0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006, 0x0007, 0x0008,
			0x0009, 0x000a, 0x000b, 0x000c, 0x000d, 0x000e, 0x000f, 0x0010, 0x0011,
			0x0012, 0x0013, 0x0014, 0x0015, 0x0016, 0x0017, 0x02d8, 0x02c7, 0x02c6,
			0x02d9, 0x02dd, 0x02db, 0x02da, 0x02dc, 0x0020, 0x0021, 0x0022, 0x0023,
			0x0024, 0x0025, 0x0026, 0x0027, 0x0028, 0x0029, 0x002a, 0x002b, 0x002c,
			0x002d, 0x002e, 0x002f, 0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035,
			0x0036, 0x0037, 0x0038, 0x0039, 0x003a, 0x003b, 0x003c, 0x003d, 0x003e,
			0x003f, 0x0040, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047,
			0x0048, 0x0049, 0x004a, 0x004b, 0x004c, 0x004d, 0x004e, 0x004f, 0x0050,
			0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057, 0x0058, 0x0059,
			0x005a, 0x005b, 0x005c, 0x005d, 0x005e, 0x005f, 0x0060, 0x0061, 0x0062,
			0x0063, 0x0064, 0x0065, 0x0066, 0x0067, 0x0068, 0x0069, 0x006a, 0x006b,
			0x006c, 0x006d, 0x006e, 0x006f, 0x0070, 0x0071, 0x0072, 0x0073, 0x0074,
			0x0075, 0x0076, 0x0077, 0x0078, 0x0079, 0x007a, 0x007b, 0x007c, 0x007d,
			0x007e, 0x0000, 0x2022, 0x2020, 0x2021, 0x2026, 0x2014, 0x2013, 0x0192,
			0x2044, 0x2039, 0x203a, 0x2212, 0x2030, 0x201e, 0x201c, 0x201d, 0x2018,
			0x2019, 0x201a, 0x2122, 0xfb01, 0xfb02, 0x0141, 0x0152, 0x0160, 0x0178,
			0x017d, 0x0131, 0x0142, 0x0153, 0x0161, 0x017e, 0x0000, 0x20ac, 0x00a1,
			0x00a2, 0x00a3, 0x00a4, 0x00a5, 0x00a6, 0x00a7, 0x00a8, 0x00a9, 0x00aa,
			0x00ab, 0x00ac, 0x0000, 0x00ae, 0x00af, 0x00b0, 0x00b1, 0x00b2, 0x00b3,
			0x00b4, 0x00b5, 0x00b6, 0x00b7, 0x00b8, 0x00b9, 0x00ba, 0x00bb, 0x00bc,
			0x00bd, 0x00be, 0x00bf, 0x00c0, 0x00c1, 0x00c2, 0x00c3, 0x00c4, 0x00c5,
			0x00c6, 0x00c7, 0x00c8, 0x00c9, 0x00ca, 0x00cb, 0x00cc, 0x00cd, 0x00ce,
			0x00cf, 0x00d0, 0x00d1, 0x00d2, 0x00d3, 0x00d4, 0x00d5, 0x00d6, 0x00d7,
			0x00d8, 0x00d9, 0x00da, 0x00db, 0x00dc, 0x00dd, 0x00de, 0x00df, 0x00e0,
			0x00e1, 0x00e2, 0x00e3, 0x00e4, 0x00e5, 0x00e6, 0x00e7, 0x00e8, 0x00e9,
			0x00ea, 0x00eb, 0x00ec, 0x00ed, 0x00ee, 0x00ef, 0x00f0, 0x00f1, 0x00f2,
			0x00f3, 0x00f4, 0x00f5, 0x00f6, 0x00f7, 0x00f8, 0x00f9, 0x00fa, 0x00fb,
			0x00fc, 0x00fd, 0x00fe, 0x00ff };

		static const sl_char16 g_encodingMacExpert[256] = {
			0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
			0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
			0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
			0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0020, 0xf721, 0xf6f8, 0xf7a2,
			0xf724, 0xf6e4, 0xf726, 0xf7b4, 0x207d, 0x207e, 0x2025, 0x2024, 0x002c,
			0x002d, 0x002e, 0x2044, 0xf730, 0xf731, 0xf732, 0xf733, 0xf734, 0xf735,
			0xf736, 0xf737, 0xf738, 0xf739, 0x003a, 0x003b, 0x0000, 0xf6de, 0x0000,
			0xf73f, 0x0000, 0x0000, 0x0000, 0x0000, 0xf7f0, 0x0000, 0x0000, 0x00bc,
			0x00bd, 0x00be, 0x215b, 0x215c, 0x215d, 0x215e, 0x2153, 0x2154, 0x0000,
			0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xfb00, 0xfb01, 0xfb02, 0xfb03,
			0xfb04, 0x208d, 0x0000, 0x208e, 0xf6f6, 0xf6e5, 0xf760, 0xf761, 0xf762,
			0xf763, 0xf764, 0xf765, 0xf766, 0xf767, 0xf768, 0xf769, 0xf76a, 0xf76b,
			0xf76c, 0xf76d, 0xf76e, 0xf76f, 0xf770, 0xf771, 0xf772, 0xf773, 0xf774,
			0xf775, 0xf776, 0xf777, 0xf778, 0xf779, 0xf77a, 0x20a1, 0xf6dc, 0xf6dd,
			0xf6fe, 0x0000, 0x0000, 0xf6e9, 0xf6e0, 0x0000, 0x0000, 0x0000, 0x0000,
			0xf7e1, 0xf7e0, 0xf7e2, 0xf7e4, 0xf7e3, 0xf7e5, 0xf7e7, 0xf7e9, 0xf7e8,
			0xf7ea, 0xf7eb, 0xf7ed, 0xf7ec, 0xf7ee, 0xf7ef, 0xf7f1, 0xf7f3, 0xf7f2,
			0xf7f4, 0xf7f6, 0xf7f5, 0xf7fa, 0xf7f9, 0xf7fb, 0xf7fc, 0x0000, 0x2078,
			0x2084, 0x2083, 0x2086, 0x2088, 0x2087, 0xf6fd, 0x0000, 0xf6df, 0x2082,
			0x0000, 0xf7a8, 0x0000, 0xf6f5, 0xf6fd, 0x2085, 0x0000, 0xf6e1, 0xf6e7,
			0xf7fd, 0x0000, 0xf6e3, 0x0000, 0x0000, 0xf7fe, 0x0000, 0x2089, 0x2080,
			0xf6ff, 0xf7e6, 0xf7f8, 0xf7bf, 0x2081, 0xf6e9, 0x0000, 0x0000, 0x0000,
			0x0000, 0x0000, 0x0000, 0xf7b8, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
			0xf6fa, 0x2012, 0xf6e6, 0x0000, 0x0000, 0x0000, 0x0000, 0xf7a1, 0x0000,
			0xf7ff, 0x0000, 0x00b9, 0x00b2, 0x00b3, 0x2074, 0x2075, 0x2076, 0x2077,
			0x2079, 0x2070, 0x0000, 0xf6ec, 0xf6f1, 0x0000, 0x0000, 0x0000, 0xf6ed,
			0xf6f2, 0xf6eb, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xf6ee, 0xf6fb,
			0xf6f4, 0xf7af, 0xf6ea, 0x207f, 0xf6ef, 0xf6e2, 0xf6e8, 0xf6f7, 0xf6fc,
			0x0000, 0x0000, 0x0000, 0x0000 };

		static const char* const g_charNamesMacExpert[256] = {
			sl_null, sl_null, sl_null, sl_null, sl_null, sl_null,
			sl_null, sl_null, sl_null, sl_null, sl_null, sl_null,
			sl_null, sl_null, sl_null, sl_null, sl_null, sl_null,
			sl_null, sl_null, sl_null, sl_null, sl_null, sl_null,
			sl_null, sl_null, sl_null, sl_null, sl_null, sl_null,
			sl_null, sl_null,
			"space", "exclamsmall", "Hungarumlautsmall", "centoldstyle",
			"dollaroldstyle", "dollarsuperior", "ampersandsmall", "Acutesmall",
			"parenleftsuperior", "parenrightsuperior", "twodotenleader",
			"onedotenleader", "comma", "hyphen", "period", "fraction",
			"zerooldstyle", "oneoldstyle", "twooldstyle", "threeoldstyle",
			"fouroldstyle", "fiveoldstyle", "sixoldstyle", "sevenoldstyle",
			"eightoldstyle", "nineoldstyle", "colon", "semicolon", sl_null,
			"threequartersemdash", sl_null, "questionsmall", sl_null,
			sl_null, sl_null, sl_null, "Ethsmall", sl_null, sl_null,
			"onequarter", "onehalf", "threequarters", "oneeighth", "threeeighths",
			"fiveeighths", "seveneighths", "onethird", "twothirds", sl_null,
			sl_null, sl_null, sl_null, sl_null, sl_null, "ff", "fi",
			"fl", "ffi", "ffl", "parenleftinferior", sl_null, "parenrightinferior",
			"Circumflexsmall", "hypheninferior", "Gravesmall", "Asmall", "Bsmall",
			"Csmall", "Dsmall", "Esmall", "Fsmall", "Gsmall", "Hsmall", "Ismall",
			"Jsmall", "Ksmall", "Lsmall", "Msmall", "Nsmall", "Osmall", "Psmall",
			"Qsmall", "Rsmall", "Ssmall", "Tsmall", "Usmall", "Vsmall", "Wsmall",
			"Xsmall", "Ysmall", "Zsmall", "colonmonetary", "onefitted", "rupiah",
			"Tildesmall", sl_null, sl_null, "asuperior", "centsuperior",
			sl_null, sl_null, sl_null, sl_null, "Aacutesmall",
			"Agravesmall", "Acircumflexsmall", "Adieresissmall", "Atildesmall",
			"Aringsmall", "Ccedillasmall", "Eacutesmall", "Egravesmall",
			"Ecircumflexsmall", "Edieresissmall", "Iacutesmall", "Igravesmall",
			"Icircumflexsmall", "Idieresissmall", "Ntildesmall", "Oacutesmall",
			"Ogravesmall", "Ocircumflexsmall", "Odieresissmall", "Otildesmall",
			"Uacutesmall", "Ugravesmall", "Ucircumflexsmall", "Udieresissmall",
			sl_null, "eightsuperior", "fourinferior", "threeinferior",
			"sixinferior", "eightinferior", "seveninferior", "Scaronsmall",
			sl_null, "centinferior", "twoinferior", sl_null, "Dieresissmall",
			sl_null, "Caronsmall", "osuperior", "fiveinferior", sl_null,
			"commainferior", "periodinferior", "Yacutesmall", sl_null,
			"dollarinferior", sl_null, sl_null, "Thornsmall", sl_null,
			"nineinferior", "zeroinferior", "Zcaronsmall", "AEsmall", "Oslashsmall",
			"questiondownsmall", "oneinferior", "Lslashsmall", sl_null,
			sl_null, sl_null, sl_null, sl_null, sl_null, "Cedillasmall",
			sl_null, sl_null, sl_null, sl_null, sl_null, "OEsmall",
			"figuredash", "hyphensuperior", sl_null, sl_null, sl_null,
			sl_null, "exclamdownsmall", sl_null, "Ydieresissmall", sl_null,
			"onesuperior", "twosuperior", "threesuperior", "foursuperior",
			"fivesuperior", "sixsuperior", "sevensuperior", "ninesuperior",
			"zerosuperior", sl_null, "esuperior", "rsuperior", "tsuperior",
			sl_null, sl_null, "isuperior", "ssuperior", "dsuperior",
			sl_null, sl_null, sl_null, sl_null, sl_null, "lsuperior",
			"Ogoneksmall", "Brevesmall", "Macronsmall", "bsuperior", "nsuperior",
			"msuperior", "commasuperior", "periodsuperior", "Dotaccentsmall",
			"Ringsmall", sl_null, sl_null, sl_null, sl_null };

		static const sl_char16 g_encodingAdobeSymbol[256] = {
			0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
			0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
			0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
			0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0020, 0x0021, 0x2200, 0x0023,
			0x2203, 0x0025, 0x0026, 0x220B, 0x0028, 0x0029, 0x2217, 0x002B, 0x002C,
			0x2212, 0x002E, 0x002F, 0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035,
			0x0036, 0x0037, 0x0038, 0x0039, 0x003A, 0x003B, 0x003C, 0x003D, 0x003E,
			0x003F, 0x2245, 0x0391, 0x0392, 0x03A7, 0x0394, 0x0395, 0x03A6, 0x0393,
			0x0397, 0x0399, 0x03D1, 0x039A, 0x039B, 0x039C, 0x039D, 0x039F, 0x03A0,
			0x0398, 0x03A1, 0x03A3, 0x03A4, 0x03A5, 0x03C2, 0x03A9, 0x039E, 0x03A8,
			0x0396, 0x005B, 0x2234, 0x005D, 0x22A5, 0x005F, 0xF8E5, 0x03B1, 0x03B2,
			0x03C7, 0x03B4, 0x03B5, 0x03C6, 0x03B3, 0x03B7, 0x03B9, 0x03D5, 0x03BA,
			0x03BB, 0x03BC, 0x03BD, 0x03BF, 0x03C0, 0x03B8, 0x03C1, 0x03C3, 0x03C4,
			0x03C5, 0x03D6, 0x03C9, 0x03BE, 0x03C8, 0x03B6, 0x007B, 0x007C, 0x007D,
			0x223C, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
			0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
			0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
			0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x20AC, 0x03D2,
			0x2032, 0x2264, 0x2044, 0x221E, 0x0192, 0x2663, 0x2666, 0x2665, 0x2660,
			0x2194, 0x2190, 0x2191, 0x2192, 0x2193, 0x00B0, 0x00B1, 0x2033, 0x2265,
			0x00D7, 0x221D, 0x2202, 0x2022, 0x00F7, 0x2260, 0x2261, 0x2248, 0x2026,
			0xF8E6, 0xF8E7, 0x21B5, 0x2135, 0x2111, 0x211C, 0x2118, 0x2297, 0x2295,
			0x2205, 0x2229, 0x222A, 0x2283, 0x2287, 0x2284, 0x2282, 0x2286, 0x2208,
			0x2209, 0x2220, 0x2207, 0xF6DA, 0xF6D9, 0xF6DB, 0x220F, 0x221A, 0x22C5,
			0x00AC, 0x2227, 0x2228, 0x21D4, 0x21D0, 0x21D1, 0x21D2, 0x21D3, 0x25CA,
			0x2329, 0xF8E8, 0xF8E9, 0xF8EA, 0x2211, 0xF8EB, 0xF8EC, 0xF8ED, 0xF8EE,
			0xF8EF, 0xF8F0, 0xF8F1, 0xF8F2, 0xF8F3, 0xF8F4, 0x0000, 0x232A, 0x222B,
			0x2320, 0xF8F5, 0x2321, 0xF8F6, 0xF8F7, 0xF8F8, 0xF8F9, 0xF8FA, 0xF8FB,
			0xF8FC, 0xF8FD, 0xF8FE, 0x0000,
		};

		static const sl_char16 g_encodingMSSymbol[256] = {
			0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
			0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
			0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
			0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 32,     33,     8704,   35,
			8707,   37,     38,     8715,   40,     41,     8727,   43,     44,
			8722,   46,     47,     48,     49,     50,     51,     52,     53,
			54,     55,     56,     57,     58,     59,     60,     61,     62,
			63,     8773,   913,    914,    935,    916,    917,    934,    915,
			919,    921,    977,    922,    923,    924,    925,    927,    928,
			920,    929,    931,    932,    933,    962,    937,    926,    936,
			918,    91,     8756,   93,     8869,   95,     8254,   945,    946,
			967,    948,    949,    966,    947,    951,    953,    981,    954,
			955,    956,    957,    959,    960,    952,    961,    963,    964,
			965,    982,    969,    958,    968,    950,    123,    124,    125,
			8764,   0,      0,      0,      0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
			0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
			0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
			0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 978,
			8242,   8804,   8725,   8734,   402,    9827,   9830,   9828,   9824,
			8596,   8592,   8593,   8594,   8595,   176,    177,    8243,   8805,
			215,    8733,   8706,   8729,   247,    8800,   8801,   8776,   8943,
			0,      0,      8629,   0,      8465,   8476,   8472,   8855,   8853,
			8709,   8745,   8746,   8835,   8839,   8836,   8834,   8838,   8712,
			8713,   8736,   8711,   174,    169,    8482,   8719,   8730,   8901,
			172,    8743,   8744,   8660,   8656,   8657,   8658,   8659,   9674,
			9001,   0,      0,      0,      8721,   0,      0,      0,      0,
			0,      0,      0,      0,      0,      0,      0x0000, 9002,   8747,
			8992,   0,      8993,   0,      0,      0,      0,      0,      0,
			0x0000, 0x0000, 0x0000, 0x0000 };

		static const sl_char16 g_encodingZapf[256] = {
			0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
			0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
			0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
			0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0020, 0x2701, 0x2702, 0x2703,
			0x2704, 0x260E, 0x2706, 0x2707, 0x2708, 0x2709, 0x261B, 0x261E, 0x270C,
			0x270D, 0x270E, 0x270F, 0x2710, 0x2711, 0x2712, 0x2713, 0x2714, 0x2715,
			0x2716, 0x2717, 0x2718, 0x2719, 0x271A, 0x271B, 0x271C, 0x271D, 0x271E,
			0x271F, 0x2720, 0x2721, 0x2722, 0x2723, 0x2724, 0x2725, 0x2726, 0x2727,
			0x2605, 0x2729, 0x272A, 0x272B, 0x272C, 0x272D, 0x272E, 0x272F, 0x2730,
			0x2731, 0x2732, 0x2733, 0x2734, 0x2735, 0x2736, 0x2737, 0x2738, 0x2739,
			0x273A, 0x273B, 0x273C, 0x273D, 0x273E, 0x273F, 0x2740, 0x2741, 0x2742,
			0x2743, 0x2744, 0x2745, 0x2746, 0x2747, 0x2748, 0x2749, 0x274A, 0x274B,
			0x25CF, 0x274D, 0x25A0, 0x274F, 0x2750, 0x2751, 0x2752, 0x25B2, 0x25BC,
			0x25C6, 0x2756, 0x25D7, 0x2758, 0x2759, 0x275A, 0x275B, 0x275C, 0x275D,
			0x275E, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
			0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
			0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
			0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x2761,
			0x2762, 0x2763, 0x2764, 0x2765, 0x2766, 0x2767, 0x2663, 0x2666, 0x2665,
			0x2660, 0x2460, 0x2461, 0x2462, 0x2463, 0x2464, 0x2465, 0x2466, 0x2467,
			0x2468, 0x2469, 0x2776, 0x2777, 0x2778, 0x2779, 0x277A, 0x277B, 0x277C,
			0x277D, 0x277E, 0x277F, 0x2780, 0x2781, 0x2782, 0x2783, 0x2784, 0x2785,
			0x2786, 0x2787, 0x2788, 0x2789, 0x278A, 0x278B, 0x278C, 0x278D, 0x278E,
			0x278F, 0x2790, 0x2791, 0x2792, 0x2793, 0x2794, 0x2192, 0x2194, 0x2195,
			0x2798, 0x2799, 0x279A, 0x279B, 0x279C, 0x279D, 0x279E, 0x279F, 0x27A0,
			0x27A1, 0x27A2, 0x27A3, 0x27A4, 0x27A5, 0x27A6, 0x27A7, 0x27A8, 0x27A9,
			0x27AA, 0x27AB, 0x27AC, 0x27AD, 0x27AE, 0x27AF, 0x0000, 0x27B1, 0x27B2,
			0x27B3, 0x27B4, 0x27B5, 0x27B6, 0x27B7, 0x27B8, 0x27B9, 0x27BA, 0x27BB,
			0x27BC, 0x27BD, 0x27BE, 0x0000,
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

		SLIB_INLINE static sl_uint64 GetObjectId(sl_uint32 objectNumber, sl_uint32 generation)
		{
			return MAKE_OBJECT_ID(objectNumber, generation);
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
			sl_uint32 type : 2; // CrossReferenceEntryType
			sl_uint32 generation : 30; // For free, normal entry: 5 digits in Cross-Reference-Table. For compressed entry, objectIndex
		};

		class ObjectStream : public CRef
		{
		public:
			Ref<ObjectStream> extends;
			CList< Pair<sl_uint32, PdfValue > > objects;

		public:
			PdfValue getItem(sl_uint32 index, sl_uint32& _id)
			{
				sl_uint32 n = (sl_uint32)(objects.getCount());
				if (index < n) {
					Pair<sl_uint32, PdfValue >& item = objects[index];
					_id = item.first;
					return item.second;
				} else {
					if (extends.isNotNull()) {
						return extends->getItem(index - n, _id);
					}
					return PdfValue();
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
			sl_uint32 getPageCount()
			{
				if (flagCount) {
					return count;
				}
				count = attributes->get(name::Count).getUint();
				flagCount = sl_true;
				return count;
			}

			void increasePageCount()
			{
				sl_uint32 countNew = getPageCount();
				countNew++;
				count = countNew;
				attributes->put_NoLock(name::Count, countNew);
			}

			void decreasePageCount()
			{
				sl_uint32 countNew = getPageCount();
				if (countNew) {
					countNew--;
					count = countNew;
					attributes->put_NoLock(name::Count, countNew);
				}
			}

			void insertKidAfter(CRef* context, PdfPageTreeItem* item, PdfPageTreeItem* after)
			{
				sl_size index = 0;
				if (after) {
					ListElements< Ref<PdfPageTreeItem> > items(kids);
					for (sl_size i = 0; i < items.count; i++) {
						if (items[i] == after) {
							kids.insert_NoLock(i + 1, item);
							index = i + 1;
							break;
						}
					}
				} else {
					kids.insert_NoLock(0, item);
				}
				Ref<PdfArray> arrKids = attributes->get(name::Kids).getArray();
				if (arrKids.isNull()) {
					arrKids = new PdfArray(context);
					if (arrKids.isNull()) {
						return;
					}
				}
				arrKids->insert_NoLock(index, item->reference);
				attributes->put_NoLock(name::Kids, Move(arrKids));
			}

			void deleteKidAt(sl_uint32 index)
			{
				kids.removeAt_NoLock(index);
				Ref<PdfArray> arrKids = attributes->get(name::Kids).getArray();
				if (arrKids.isNotNull()) {
					arrKids->removeAt_NoLock(index);
					attributes->put_NoLock(name::Kids, Move(arrKids));
				}
			}

		};

		class Context : public CRef, public Lockable
		{
		public:
			sl_uint8 majorVersion = 0;
			sl_uint8 minorVersion = 0;

			Ref<PdfDictionary> lastTrailer; // NotNull
			Ref<PdfDictionary> encrypt;
			Ref<PdfDictionary> catalog; // NotNull

			sl_bool flagDecryptContents = sl_false;
			sl_uint8 encryptionKey[16];
			sl_uint32 lenEncryptionKey = 0;

		protected:
			Context* m_baseContext;
			sl_uint32 m_maxObjectNumber = 0;
			Array<CrossReferenceEntry> m_references;
			ExpiringMap<sl_uint64, PdfValue> m_objectsCache;
			CHashMap< sl_uint32, Pair<PdfValue, sl_uint32> > m_objectsUpdate;
			ExpiringMap< sl_uint64, Ref<ObjectStream> > m_objectStreams;
			Ref<PageTreeParent> m_pageTree;

		public:
			Context(sl_bool flagRefContext)
			{
				m_baseContext = flagRefContext ? this : sl_null;
				_init();
			}

			Context(Context* baseContext)
			{
				m_baseContext = baseContext;
				_init();
			}

		public:
			virtual PdfValue readObject(sl_uint32 pos, sl_uint32& outOffsetAfterEndObj, PdfReference& outRef, sl_bool flagReadOnlyStream) = 0;
			virtual Memory readContent(sl_uint32 offset, sl_uint32 size, const PdfReference& ref) = 0;
			virtual sl_bool readDocument(const PdfDocumentParam& param) = 0;

		public:
			void _init()
			{
				m_objectsCache.setExpiringMilliseconds(EXPIRE_DURATION_OBJECT);
				m_objectStreams.setExpiringMilliseconds(EXPIRE_DURATION_OBJECT_STREAM);
			}

			sl_uint32 getMaximumObjectNumber()
			{
				return m_maxObjectNumber;
			}

			Ref<ObjectStream> getObjectStream(const PdfReference& ref);

			sl_bool getReferenceEntry(sl_uint32 objectNumber, CrossReferenceEntry& entry)
			{
				return m_references.getAt(objectNumber, &entry);
			}

			void setReferenceEntry(sl_uint32 objectNumber, const CrossReferenceEntry& entry)
			{
				CrossReferenceEntry* pEntry = m_references.getPointerAt(objectNumber);
				if (pEntry) {
					if (pEntry->type == (sl_uint32)(CrossReferenceEntryType::Free) && entry.type != (sl_uint32)(CrossReferenceEntryType::Free)) {
						*pEntry = entry;
					}
				}
			}

			PdfValue readObject(sl_uint32 objectNumber, sl_int32& generation, sl_bool flagReadOnlyStream = sl_false)
			{
				CrossReferenceEntry entry;
				if (getReferenceEntry(objectNumber, entry)) {
					if (entry.type == (sl_uint32)(CrossReferenceEntryType::Normal)) {
						if (generation >= 0 && entry.generation != generation) {
							return PdfValue();
						}
						PdfReference n;
						sl_uint32 offsetEnd;
						PdfValue ret = readObject(entry.offset, offsetEnd, n, flagReadOnlyStream);
						if (ret.isNotUndefined() && n.objectNumber == objectNumber) {
							if (generation >= 0) {
								if (n.generation != generation) {
									return PdfValue();
								}
							} else {
								generation = n.generation;
							}
							return ret;
						}
					} else if (entry.type == (sl_uint32)(CrossReferenceEntryType::Compressed)) {
						if (generation > 0) {
							return PdfValue();
						}
						Ref<ObjectStream> stream = getObjectStream(entry.streamObject);
						if (stream.isNotNull()) {
							sl_uint32 n;
							PdfValue ret = stream->getItem(entry.generation, n);
							if (ret.isNotUndefined() && objectNumber == n) {
								if (generation < 0) {
									generation = 0;
								}
								return ret;
							}
						}
					}
				}
				return PdfValue();
			}

			PdfValue getObject(sl_uint32 objectNumber, sl_int32& generation, sl_bool flagReadOnlyStream = sl_false)
			{
				if (!objectNumber) {
					return PdfValue();
				}
				Pair<PdfValue, sl_uint32>* pItem = m_objectsUpdate.getItemPointer(objectNumber);
				if (pItem) {
					if (generation >= 0) {
						if (generation != pItem->second) {
							return sl_false;
						}
					} else {
						generation = pItem->second;
					}
					return pItem->first;
				}
				if (generation >= 0) {
					sl_uint64 _id = GetObjectId(objectNumber, generation);
					PdfValue ret;
					if (m_objectsCache.get(_id, &ret)) {
						return ret;
					}
				}
				PdfValue ret = readObject(objectNumber, generation, flagReadOnlyStream);
				if (ret.isNotUndefined()) {
					m_objectsCache.put(GetObjectId(objectNumber, generation), ret);
					return ret;
				}
				return PdfValue();
			}

			PdfValue getObject(const PdfReference& ref)
			{
				sl_int32 generation = ref.generation;
				return getObject(ref.objectNumber, generation);
			}

			Ref<PdfStream> getStream(sl_uint32 objectNumber, sl_int32& generation)
			{
				return getObject(objectNumber, generation, sl_true).getStream();
			}

			sl_bool _setObject(const PdfReference& ref, const PdfValue& value)
			{
				Pair<PdfValue, sl_uint32>* pItem = m_objectsUpdate.getItemPointer(ref.objectNumber);
				if (pItem) {
					pItem->first = value;
					pItem->second = ref.generation;
					return sl_true;
				} else {
					return m_objectsUpdate.add_NoLock(ref.objectNumber, value, ref.generation);
				}
			}

			sl_bool setObject(const PdfReference& ref, const PdfValue& value)
			{
				if (!(ref.objectNumber)) {
					return sl_false;
				}
				if (value.isUndefined()) {
					return deleteObject(ref);
				}
				if (ref.objectNumber > m_maxObjectNumber) {
					return sl_false;
				}
				return _setObject(ref, value);
			}

			sl_bool isFreeObject(sl_uint32 objectNumber, sl_uint32& outGeneration)
			{
				if (m_objectsUpdate.find_NoLock(objectNumber)) {
					return sl_false;
				}
				CrossReferenceEntry* entry = m_references.getPointerAt(objectNumber);
				if (!entry) {
					outGeneration = 0;
					return sl_true;
				}
				if (entry->type == (sl_uint32)(CrossReferenceEntryType::Free)) {
					outGeneration = entry->generation;
					return sl_true;
				}
				return sl_false;
			}

			sl_bool addObject(const PdfValue& value, PdfReference& outRef)
			{
				if (value.isUndefined()) {
					return sl_false;
				}
				ArrayElements<CrossReferenceEntry> ref(m_references);
				sl_uint32 n = m_maxObjectNumber;
				for (sl_uint32 i = 1; i <= n; i++) {
					if (isFreeObject(i, outRef.generation)) {
						outRef.objectNumber = i;
						return _setObject(outRef, value);
					}
				}
				n++;
				m_maxObjectNumber = n;
				outRef.objectNumber = n;
				outRef.generation = 0;
				return _setObject(outRef, value);
			}

			sl_bool deleteObject(const PdfReference& ref)
			{
				if (!(ref.objectNumber) || ref.objectNumber >= m_maxObjectNumber) {
					return sl_false;
				}
				sl_uint64 _id = GetObjectId(ref);
				m_objectsCache.remove(_id);
				CrossReferenceEntry* entry = m_references.getPointerAt(ref.objectNumber);
				if (entry) {
					m_objectsUpdate.remove_NoLock(ref.objectNumber);
					if (entry->type == (sl_uint32)(CrossReferenceEntryType::Normal)) {
						if (entry->generation == ref.generation) {
							entry->type = (sl_uint32)(CrossReferenceEntryType::Free);
							entry->generation++;
							entry->nextFreeObject = 0;
							return sl_true;
						}
					} else if (entry->type == (sl_uint32)(CrossReferenceEntryType::Compressed)) {
						entry->type = (sl_uint32)(CrossReferenceEntryType::Free);
						entry->generation = 1;
						entry->nextFreeObject = 0;
						return sl_true;
					}
					return sl_false;
				} else {
					return m_objectsUpdate.remove_NoLock(ref.objectNumber);
				}
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

			PageTreeParent* getPageTree()
			{
				if (m_pageTree.isNotNull()) {
					return m_pageTree.get();
				}
				PdfReference refPages;
				if (!(catalog->get(name::Pages, sl_false).getReference(refPages))) {
					return sl_null;
				}
				Ref<PdfDictionary> attrs = getObject(refPages).getDictionary();
				if (attrs.isNull()) {
					return sl_null;
				}
				Ref<PageTreeParent> tree = new PageTreeParent;
				if (tree.isNull()) {
					return sl_null;
				}
				tree->reference = refPages;
				tree->attributes = Move(attrs);
				m_pageTree = Move(tree);
				return m_pageTree.get();
			}

			void preparePageKids(PageTreeParent* parent)
			{
				if (parent->flagKids) {
					return;
				}
				parent->flagKids = sl_true;
				ListElements<PdfValue> arrKids(parent->attributes->get(name::Kids).getElements());
				List< Ref<PdfPageTreeItem> > kids;
				for (sl_uint32 i = 0; i < arrKids.count; i++) {
					PdfReference refKid;
					if (!(arrKids[i].getReference(refKid))) {
						return;
					}
					Ref<PdfDictionary> props = getObject(refKid).getDictionary();
					if (props.isNull()) {
						return;
					}
					Ref<PdfPageTreeItem> item;
					if (props->get(name::Type).equalsName(name::Page)) {
						item = new PdfPage(m_baseContext);
					} else {
						item = new PageTreeParent;
					}
					if (item.isNull()) {
						return;
					}
					item->parent = parent;
					item->reference = refKid;
					item->attributes = Move(props);
					if (!(kids.add_NoLock(Move(item)))) {
						return;
					}
				}
				parent->kids = Move(kids);
			}

			Ref<PdfPage> getPage(PageTreeParent* parent, sl_uint32 index)
			{
				if (index >= parent->getPageCount()) {
					return sl_null;
				}
				preparePageKids(parent);
				sl_uint32 n = 0;
				ListElements< Ref<PdfPageTreeItem> > kids(parent->kids);
				for (sl_size i = 0; i < kids.count; i++) {
					Ref<PdfPageTreeItem>& item = kids[i];
					if (item->isPage()) {
						if (index == n) {
							return Ref<PdfPage>::from(item);
						}
						n++;
					} else {
						PageTreeParent* pItem = (PageTreeParent*)(item.get());
						sl_uint32 m = pItem->getPageCount();
						if (index < n + m) {
							return getPage(pItem, index - n);
						}
						n += m;
					}
				}
				return sl_null;
			}

			Ref<PdfPage> getPage(sl_uint32 index)
			{
				PageTreeParent* tree = getPageTree();
				if (tree) {
					return getPage(tree, index);
				}
				return sl_null;
			}

			Ref<PdfPage> createJpegImagePage(PageTreeParent* parent, sl_uint32 imageWidth, sl_uint32 imageHeight, const Memory& jpeg, sl_real pageWidth, sl_real pageHeight = 0.0f)
			{
				if (pageHeight < SLIB_EPSILON) {
					pageHeight = pageWidth * (sl_real)imageHeight / (sl_real)imageWidth;
				}
				Ref<PdfStream> streamImage = PdfStream::createJpegImage(imageWidth, imageHeight, jpeg);
				if (streamImage.isNull()) {
					return sl_null;
				}
				Ref<PdfDictionary> resources = new PdfDictionary(this);
				if (resources.isNull()) {
					return sl_null;
				}
				Ref<PdfDictionary> xobjects = new PdfDictionary(this);
				if (xobjects.isNull()) {
					return sl_null;
				}
				resources->put_NoLock(name::XObject, xobjects);
				String pageContent = String::format("q\n%s 0 0 %s 0 0 cm\n/BackImage Do\nQ", pageWidth, pageHeight);
				Ref<PdfStream> streamContent = PdfStream::create(pageContent.toMemory());
				if (streamContent.isNull()) {
					return sl_null;
				}
				Ref<PdfDictionary> attrs = new PdfDictionary(this);
				if (attrs.isNull()) {
					return sl_null;
				}
				Ref<PdfArray> procs = new PdfArray(this);
				if (procs.isNull()) {
					return sl_null;
				}
				PdfReference refImage;
				if (addObject(streamImage, refImage)) {
					PdfReference refContent;
					if (addObject(streamContent, refContent)) {
						attrs->put_NoLock(name::Type, PdfName(name::Page));
						attrs->put_NoLock(name::Parent, parent->reference);
						xobjects->put_NoLock("BackImage", refImage);
						attrs->put_NoLock(name::Resources, resources);
						procs->add_NoLock(PdfName(name::PDF));
						procs->add_NoLock(PdfName(name::ImageC)); // Color Image
						attrs->put_NoLock(name::ProcSet, procs);
						attrs->put_NoLock(name::MediaBox, Rectangle(0, 0, pageWidth, pageHeight));
						attrs->put_NoLock(name::Contents, refContent);
						Ref<PdfPage> page = new PdfPage(this);
						if (page.isNotNull()) {
							if (addObject(attrs, page->reference)) {
								page->attributes = Move(attrs);
								page->parent = parent;
								return page;
							}
						}
						deleteObject(refContent);
					}
					deleteObject(refImage);
				}
				return sl_null;
			}

			sl_bool insertJpegImagePage(sl_uint32 index, sl_uint32 imageWidth, sl_uint32 imageHeight, const Memory& jpeg)
			{
				if (jpeg.isNull() || !imageWidth || !imageHeight) {
					return sl_false;
				}
				PageTreeParent* tree = getPageTree();
				if (!tree) {
					return sl_false;
				}
				if (index > tree->getPageCount()) {
					return sl_false;
				}
				sl_real pageWidth;
				Ref<PdfPage> pageNear = getPage(index ? index - 1 : 0);
				if (pageNear.isNotNull()) {
					pageWidth = pageNear->getMediaBox().getWidth();
				} else {
					pageWidth = 612.0f;
				}
				PageTreeParent* parent = tree;
				if (pageNear.isNotNull()) {
					Ref<PdfPageTreeItem> _parent(pageNear->parent);
					if (_parent.isNotNull()) {
						parent = (PageTreeParent*)(_parent.get());
					}
				}
				Ref<PdfPage> page = createJpegImagePage(parent, imageWidth, imageHeight, jpeg, pageWidth);
				if (page.isNull()) {
					return sl_false;
				}
				parent->insertKidAfter(this, page.get(), index ? pageNear.get() : sl_null);
				Ref<PdfPageTreeItem>_parent;
				do {
					parent->increasePageCount();
					setObject(parent->reference, parent->attributes);
					_parent = parent->parent;
					parent = (PageTreeParent*)(_parent.get());
				} while (parent);
				return sl_true;
			}

			sl_bool isUsingPageResource(PdfPageTreeItem* item, const PdfReference& refMatch)
			{
				Ref<PdfDictionary> resources = item->attributes->get(name::Resources).getDictionary();
				if (resources.isNotNull()) {
					HashMapNode<String, PdfValue>* nodeCategory = resources->getFirstNode();
					while (nodeCategory) {
						PdfReference refCategory;
						if (nodeCategory->value.getReference(refCategory)) {
							nodeCategory->value = getObject(refCategory);
						}
						Ref<PdfDictionary> map = nodeCategory->value.getDictionary();
						if (map.isNotNull()) {
							HashMapNode<String, PdfValue>* nodeResource = map->getFirstNode();
							while (nodeResource) {
								PdfReference refResource;
								if (nodeResource->value.getReference(refResource)) {
									if (refResource == refMatch) {
										return sl_true;
									}
								}
								nodeResource = nodeResource->next;
							}
						}
						nodeCategory = nodeCategory->next;
					}
				}
				if (item->isPage()) {
					return sl_false;
				}
				PageTreeParent* parent = (PageTreeParent*)item;
				preparePageKids(parent);
				ListElements< Ref<PdfPageTreeItem> > kids(parent->kids);
				for (sl_size i = 0; i < kids.count; i++) {
					Ref<PdfPageTreeItem>& kid = kids[i];
					if (isUsingPageResource(kid.get(), refMatch)) {
						return sl_true;
					}
				}
				return sl_false;
			}

			void deletePageContent(PdfPage* page)
			{
				PdfValue rContents = page->attributes->get(name::Contents, sl_false);
				PdfValue vContents;
				PdfReference refContents;
				if (rContents.getReference(refContents)) {
					vContents = getObject(refContents);
				} else {
					vContents = Move(refContents);
					refContents.objectNumber = 0;
				}
				ListElements<PdfValue> arrContents(vContents.getElements());
				if (arrContents.count) {
					for (sl_uint32 i = 0; i < arrContents.count; i++) {
						PdfReference ref;
						if (arrContents[i].getReference(ref)) {
							deleteObject(ref);
						}
					}
				}
				if (refContents.objectNumber) {
					deleteObject(refContents);
				}
				PageTreeParent* root = getPageTree();
				if (!root) {
					return;
				}
				Ref<PdfDictionary> resources = page->attributes->get(name::Resources).getDictionary();
				if (resources.isNotNull()) {
					HashMapNode<String, PdfValue>* nodeCategory = resources->getFirstNode();
					while (nodeCategory) {
						PdfReference refCategory;
						if (nodeCategory->value.getReference(refCategory)) {
							nodeCategory->value = getObject(refCategory);
						}
						Ref<PdfDictionary> map = nodeCategory->value.getDictionary();
						if (map.isNotNull()) {
							HashMapNode<String, PdfValue>* nodeResource = map->getFirstNode();
							while (nodeResource) {
								PdfReference refResource;
								if (nodeResource->value.getReference(refResource)) {
									if (!(isUsingPageResource(root, refResource))) {
										deleteObject(refResource);
									}
								}
								nodeResource = nodeResource->next;
							}
						}
						nodeCategory = nodeCategory->next;
					}
				}
			}

			sl_bool deletePage(PageTreeParent* parent, sl_uint32 pageNo)
			{
				if (pageNo >= parent->getPageCount()) {
					return sl_false;
				}
				preparePageKids(parent);
				sl_uint32 n = 0;
				ListElements< Ref<PdfPageTreeItem> > kids(parent->kids);
				for (sl_size i = 0; i < kids.count; i++) {
					Ref<PdfPageTreeItem>& item = kids[i];
					if (item->isPage()) {
						if (pageNo == n) {
							Ref<PdfPage> page = Ref<PdfPage>::from(Move(item));
							deleteObject(page->reference);
							parent->decreasePageCount();
							parent->deleteKidAt((sl_uint32)i);
							setObject(parent->reference, parent->attributes);
							deletePageContent(page.get());
							return sl_true;
						}
						n++;
					} else {
						PageTreeParent* pItem = (PageTreeParent*)(item.get());
						sl_uint32 m = pItem->getPageCount();
						if (pageNo < n + m) {
							if (deletePage(pItem, pageNo - n)) {
								parent->decreasePageCount();
								if (pItem->kids.isEmpty()) {
									deleteObject(pItem->reference);
									parent->deleteKidAt((sl_uint32)i);
								}
								setObject(parent->reference, parent->attributes);
								return sl_true;
							}
							return sl_false;
						}
						n += m;
					}
				}
				return sl_false;
			}

			sl_bool deletePage(sl_uint32 index)
			{
				PageTreeParent* tree = getPageTree();
				if (tree) {
					return deletePage(tree, index);
				}
				return sl_false;
			}

			sl_bool writeChar(IWriter* writer, char c, sl_uint32& offset)
			{
				if (writer->writeFully(&c, 1) == 1) {
					offset++;
					return sl_true;
				}
				return sl_false;
			}

			sl_bool writeText(IWriter* writer, const StringView& s, sl_uint32& offset)
			{
				sl_size n = s.getLength();
				if (!n) {
					return sl_true;
				}
				if (writer->writeFully(s.getData(), n) == n) {
					offset += (sl_uint32)n;
					return sl_true;
				}
				return sl_false;
			}

			sl_bool writeFloat(IWriter* writer, float f, sl_uint32& offset)
			{
				String s;
				sl_int32 n = (sl_int32)(Math::round(f));
				if (Math::isAlmostZero(f - (float)n)) {
					s = String::fromInt32(n);
				} else {
					s = String::fromFloat(f, 5);
				}
				return writeText(writer, s, offset);
			}

			sl_bool writeString(IWriter* writer, const StringView& str, sl_uint32& offset)
			{
				sl_size len = str.getLength();
				if (!len) {
					return writeText(writer, StringView::literal("()"), offset);
				}
				sl_char8* data = str.getData();
				sl_bool flagHex = sl_false;
				sl_uint32 nOpen = 0;
				{
					for (sl_size i = 0; i < len; i++) {
						sl_char8 ch = data[i];
						switch (ch) {
							case '(':
								nOpen++;
								break;
							case ')':
								if (nOpen) {
									nOpen--;
								} else {
									nOpen = SLIB_INT32_MAX;
								}
								break;
							case '\r':
							case '\n':
							case '\t':
							case '\b':
							case '\f':
								break;
							default:
								if (ch < ' ' || ch >= (sl_char8)0x7f) {
									flagHex = sl_true;
								}
								break;
						}
						if (flagHex) {
							break;
						}
					}
				}
				if (flagHex) {
					if (!(writeChar(writer, '<', offset))) {
						return sl_false;
					}
					char buf[1024];
					const char* hex = "0123456789abcdef";
					sl_uint32 m = sizeof(buf) >> 1;
					do {
						sl_size n = len;
						if (n > m) {
							n = m;
						}
						for (sl_size i = 0; i < n; i++) {
							sl_uint8 h = data[i];
							buf[i << 1] =  hex[h >> 4];
							buf[(i << 1) | 1] = hex[h & 15];
						}
						if (!(writeText(writer, StringView(buf, n), offset))) {
							return sl_false;
						}
						data += n;
						len -= n;
					} while (len);
					return writeChar(writer, '>', offset);
				} else {
					if (!(writeChar(writer, '(', offset))) {
						return sl_false;
					}
					sl_size start = 0;
					for (sl_size i = 0; i < len; i++) {
						sl_char8 c = data[i];
						sl_char8 chEscape = 0;
						switch (c) {
							case '\\':
								chEscape = '\\';
								break;
							case '\f':
								chEscape = 'f';
								break;
							case '\b':
								chEscape = 'b';
								break;
							case '(':
							case ')':
								if (nOpen) {
									chEscape = c;
								}
								break;
						}
						if (chEscape) {
							if (!(writeText(writer, StringView(data + start, i - start), offset))) {
								return sl_false;
							}
							if (!(writeChar(writer, '\\', offset))) {
								return sl_false;
							}
							if (!(writeChar(writer, chEscape, offset))) {
								return sl_false;
							}
							start = i + 1;
						}
					}
					if (start < len) {
						if (!(writeText(writer, StringView(data + start, len - start), offset))) {
							return sl_false;
						}
					}
					return writeChar(writer, ')', offset);
				}
			}

			sl_bool writeName(IWriter* writer, const StringView& str, sl_uint32& offset)
			{
				if (!(writeChar(writer, '/', offset))) {
					return sl_false;
				}
				return writeText(writer, str, offset);
			}

			sl_bool writeArray(IWriter* writer, PdfArray* arr, sl_uint32& offset)
			{
				if (!(writeChar(writer, '[', offset))) {
					return sl_false;
				}
				sl_size n = arr->getCount();
				for (sl_size i = 0; i < n; i++) {
					if (i) {
						if (!(writeChar(writer, ' ', offset))) {
							return sl_false;
						}
					}
					if (!(writeValue(writer, arr->getValueAt_NoLock(i), offset))) {
						return sl_false;
					}
				}
				return writeChar(writer, ']', offset);
			}

			sl_bool writeDictionary(IWriter* writer, PdfDictionary* dict, sl_uint32& offset)
			{
				if (!(writeText(writer, StringView::literal("<<"), offset))) {
					return sl_false;
				}
				auto node = dict->getFirstNode();
				while (node) {
					if (!(writeName(writer, node->key, offset))) {
						return sl_false;
					}
					if (!(writeChar(writer, ' ', offset))) {
						return sl_false;
					}
					if (!(writeValue(writer, node->value, offset))) {
						return sl_false;
					}
					node = node->next;
				}
				return writeText(writer, StringView::literal(" >>"), offset);
			}

			sl_bool writeReference(IWriter* writer, const PdfReference& ref, sl_uint32& offset)
			{
				if (!(writeText(writer, String::fromUint32(ref.objectNumber), offset))) {
					return sl_false;
				}
				if (!(writeChar(writer, ' ', offset))) {
					return sl_false;
				}
				if (!(writeText(writer, String::fromUint32(ref.generation), offset))) {
					return sl_false;
				}
				return writeText(writer, StringView::literal(" R"), offset);
			}

			sl_bool writeValue(IWriter* writer, const PdfValue& value, sl_uint32& offset)
			{
				const Variant& var = value.getVariant();
				PdfValueType type = value.getType();
				switch (type) {
					case PdfValueType::Null:
						return writeText(writer, StringView::literal("null"), offset);
					case PdfValueType::Boolean:
						if (var._m_boolean) {
							return writeText(writer, StringView::literal("true"), offset);
						} else {
							return writeText(writer, StringView::literal("false"), offset);
						}
						break;
					case PdfValueType::Uint:
						return writeText(writer, String::fromUint32(var._m_uint32), offset);
					case PdfValueType::Int:
						return writeText(writer, String::fromInt32(var._m_int32), offset);
					case PdfValueType::Float:
						return writeFloat(writer, var._m_float, offset);
					case PdfValueType::String:
						return writeString(writer, CAST_VAR(String, var._value), offset);
					case PdfValueType::Name:
						return writeName(writer, CAST_VAR(String, var._value), offset);
					case PdfValueType::Array:
						return writeArray(writer, CAST_VAR(PdfArray*, var._value), offset);
					case PdfValueType::Dictionary:
						return writeDictionary(writer, CAST_VAR(PdfDictionary*, var._value), offset);
					case PdfValueType::Reference:
						return writeReference(writer, PdfReference(SLIB_GET_DWORD0(var._value), SLIB_GET_DWORD1(var._value)), offset);
					default:
						break;
				}
				return sl_false;
			}

			sl_bool writeStream(IWriter* writer, PdfStream* stream, sl_uint32& offset)
			{
				Memory content = stream->getEncodedContent();
				sl_size size = content.getSize();
				Ref<PdfDictionary> dict = stream->properties;
				if (dict.isNull()) {
					dict = new PdfDictionary(sl_null);
					if (dict.isNull()) {
						return sl_false;
					}
					dict->put_NoLock(name::Length, (sl_uint32)size);
				}
				if (!(writeDictionary(writer, dict.get(), offset))) {
					return sl_false;
				}
				if (!(writeText(writer, StringView::literal("\nstream\n"), offset))) {
					return sl_false;
				}
				if (size) {
					if (writer->writeFully(content.getData(), size) != size) {
						return sl_false;
					}
					offset += (sl_uint32)size;
				}
				return writeText(writer, StringView::literal("\nendstream"), offset);
			}

			sl_bool writeObject(IWriter* writer, const PdfReference& ref, const PdfValue& obj, sl_uint32& offset)
			{
				if (!(writeText(writer, String::fromUint32(ref.objectNumber), offset))) {
					return sl_false;
				}
				if (!(writeChar(writer, ' ', offset))) {
					return sl_false;
				}
				if (!(writeText(writer, String::fromUint32(ref.generation), offset))) {
					return sl_false;
				}
				if (!(writeText(writer, StringView::literal(" obj\n"), offset))) {
					return sl_false;
				}
				Ref<PdfStream> stream = obj.getStream();
				if (stream.isNotNull()) {
					if (!(writeStream(writer, stream.get(), offset))) {
						return sl_false;
					}
				} else {
					if (!(writeValue(writer, obj, offset))) {
						return sl_false;
					}
				}
				return writeText(writer, StringView::literal("\nendobj\n"), offset);
			}

			sl_bool save(IWriter* writer)
			{
				sl_uint32 nObjects = m_maxObjectNumber + 1;
				Array<sl_uint32> arrObjectOffsets = Array<sl_uint32>::create(nObjects);
				if (arrObjectOffsets.isNull()) {
					return sl_false;
				}
				Array<sl_uint16> arrGenerations = Array<sl_uint16>::create(nObjects);
				if (arrGenerations.isNull()) {
					return sl_false;
				}
				sl_uint32* objectOffsets = arrObjectOffsets.getData();
				Base::zeroMemory(objectOffsets, nObjects << 2);
				sl_uint16* generations = arrGenerations.getData();
				Base::zeroMemory(generations, nObjects << 1);

				sl_uint32 offsetCurrent = 0;

				if (!(writeText(writer, String::format("%%PDF-%d.%d\n%%\xB5\xB5\xB5\xB5\n", majorVersion, minorVersion), offsetCurrent))) {
					return sl_false;
				}

				for (sl_uint32 iObj = 1; iObj < nObjects; iObj++) {
					sl_int32 generation = -1;
					PdfValue obj;
					Pair<PdfValue, sl_uint32>* pItem = m_objectsUpdate.getItemPointer(iObj);
					if (pItem) {
						obj = pItem->first;
						generation = pItem->second;
					} else {
						obj = readObject(iObj, generation);
					}
					sl_bool flagWriteObject;
					if (obj.isNotUndefined()) {
						flagWriteObject = sl_true;
						const Ref<PdfStream>& stream = obj.getStream();
						if (stream.isNotNull()) {
							PdfValue type = stream->getProperty(name::Type);
							if (type.equalsName(name::ObjStm) || type.equalsName(name::XRef)) {
								flagWriteObject = sl_false;
							}
						}
						if (flagWriteObject) {
							objectOffsets[iObj] = offsetCurrent;
							if (!(writeObject(writer, PdfReference(iObj, generation), obj, offsetCurrent))) {
								return sl_false;
							}
						}
					} else {
						flagWriteObject = sl_false;
					}
					if (!flagWriteObject) {
						if (generation < 0) {
							CrossReferenceEntry* entry = m_references.getPointerAt(iObj);
							if (entry) {
								if (entry->generation) {
									generations[iObj] = (sl_uint16)(entry->generation);
								}
							}
						}
					}
				}

				if (!(writeChar(writer, '\n', offsetCurrent))) {
					return sl_false;
				}
				sl_uint32 offsetXref = offsetCurrent;
				if (!(writeText(writer, StringView::literal("xref\n"), offsetCurrent))) {
					return sl_false;
				}
				generations[0] = 65535;
				sl_uint32 start = 0;
				sl_uint32 end;
				do {
					end = nObjects;
					{
						for (sl_uint32 i = start; i < nObjects; i++) {
							if (!(objectOffsets[i] || generations[i])) {
								end = i;
								break;
							}
						}
					}
					if (!(writeText(writer, String::format("%d %d\n", start, end - start), offsetCurrent))) {
						return sl_false;
					}
					{
						for (sl_uint32 i = start; i < end; i++) {
							if (!(writeText(writer, String::format("%010d %05d %c\n", objectOffsets[i], generations[i], objectOffsets[i] ? 'n' : 'f'), offsetCurrent))) {
								return sl_false;
							}
						}
					}
					start = nObjects;
					{
						for (sl_uint32 i = end; i < nObjects; i++) {
							if (objectOffsets[i] || generations[i]) {
								start = i;
								break;
							}
						}
					}
				} while (start < nObjects);

				if (!(writeText(writer, StringView::literal("trailer\n"), offsetCurrent))) {
					return sl_false;
				}
				lastTrailer->remove_NoLock(name::Prev);
				lastTrailer->remove_NoLock(name::XRefStm);
				lastTrailer->remove_NoLock(name::Encrypt);
				lastTrailer->put_NoLock(name::Size, end);
				if (!(writeDictionary(writer, lastTrailer.get(), offsetCurrent))) {
					return sl_false;
				}

				if (!(writeText(writer, String::format("\nstartxref\n%d\n", offsetXref), offsetCurrent))) {
					return sl_false;
				}
				return writeText(writer, StringView::literal("%%EOF"), offsetCurrent);
			}

			Ref<PdfFont> getFont(const PdfReference& ref, PdfResourceCache& cache)
			{
				if (cache.flagUseFontsCache) {
					Ref<PdfFont> ret;
					if (cache.fonts.get(ref.objectNumber, &ret)) {
						return ret;
					}
				}
				Ref<PdfDictionary> dict = getObject(ref).getDictionary();
				if (dict.isNotNull()) {
					Ref<PdfFont> ret = PdfFont::load(dict);
					if (cache.flagUseFontsCache) {
						cache.fonts.put(ref.objectNumber, ret);
					}
					return ret;
				}
				return sl_null;
			}

			Ref<PdfExternalObject> getExternalObject(const PdfReference& ref, PdfResourceCache& cache)
			{
				if (cache.flagUseExternalObjectsCache) {
					Ref<PdfExternalObject> ret;
					if (cache.externalObjects.get(ref.objectNumber, &ret)) {
						return ret;
					}
				}
				Ref<PdfStream> stream = getObject(ref).getStream();
				if (stream.isNotNull()) {
					Ref<PdfExternalObject> ret = PdfExternalObject::load(stream.get());
					if (cache.flagUseExternalObjectsCache) {
						cache.externalObjects.put(ref.objectNumber, ret);
					}
					return ret;
				}
				return sl_null;
			}

			sl_bool createDocument()
			{
				majorVersion = 1;
				minorVersion = 4;
				lastTrailer = new PdfDictionary(this);
				if (lastTrailer.isNull()) {
					return sl_false;
				}
				catalog = new PdfDictionary(this);
				if (catalog.isNull()) {
					return sl_false;
				}
				PdfReference refCatalog;
				if (!(addObject(catalog, refCatalog))) {
					return sl_false;
				}
				catalog->add_NoLock(name::Type, PdfName(name::Catalog));
				Ref<PdfDictionary> rootPageTree = new PdfDictionary(this);
				if (rootPageTree.isNull()) {
					return sl_false;
				}
				PdfReference refPages;
				if (!(addObject(rootPageTree, refPages))) {
					return sl_false;
				}
				rootPageTree->add_NoLock(name::Type, PdfName(name::Pages));
				rootPageTree->add_NoLock(name::Kids, new PdfArray(this));
				rootPageTree->add_NoLock(name::Count, (sl_uint32)0);
				catalog->add_NoLock(name::Pages, refPages);
				lastTrailer->add_NoLock(name::Root, refCatalog);
				return sl_true;
			}

			sl_bool initDocument(const PdfDocumentParam& param)
			{
				catalog = lastTrailer->get(name::Root).getDictionary();
				if (catalog.isNull()) {
					return sl_false;
				}
				encrypt = lastTrailer->get(name::Encrypt).getDictionary();
				if (encrypt.isNotNull()) {
					StringData password(param.password);
					return setUserPassword(password);
				} else {
					return sl_true;
				}
			}

			sl_bool setUserPassword(const StringView& password)
			{
				if (encrypt.isNull()) {
					return sl_false;
				}
				if (encrypt->get(name::Filter).equalsName(name::Standard)) {
					sl_uint32 encryptionAlgorithm = encrypt->get(name::V).getUint();
					if (encryptionAlgorithm == 1) {
						sl_uint32 lengthKey = encrypt->get(name::Length).getUint();
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
						String userHash = encrypt->get(name::U).getString();
						if (userHash.getLength() != 32) {
							return sl_false;
						}
						sl_uint32 revision = encrypt->get(name::R).getUint();
						sl_uint32 permission = encrypt->get(name::P).getInt();
						String ownerHash = encrypt->get(name::O).getString();
						String fileId = lastTrailer->get(name::ID)[0].getString();
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

			sl_reg peek(void* buf, sl_size size)
			{
				sl_reg n = reader.readFully(buf, size);
				if (n > 0) {
					if (!(reader.seek(-n, SeekPosition::Current))) {
						return SLIB_IO_ERROR;
					}
				}
				return n;
			}

			sl_uint32 getPosition()
			{
				return (sl_uint32)(reader.getPosition());
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

			Memory readFully(sl_size size)
			{
				if (size) {
					Memory mem = Memory::create(size);
					if (mem.isNotNull()) {
						if (reader.readFully(mem.getData(), size) == size) {
							return mem;
						}
					}
				}
				return sl_null;
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
			Ref<CRef> refSource;

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
				return SLIB_IO_ENDED;
			}

			sl_reg peek(void* _out, sl_size nOut)
			{
				if (pos < sizeSource) {
					if (pos + nOut > sizeSource) {
						nOut = sizeSource - pos;
					}
					if (nOut) {
						Base::copyMemory(_out, source + pos, nOut);
					}
					return nOut;
				}
				return SLIB_IO_ENDED;
			}

			sl_uint32 getPosition()
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
				sl_uint32 _pos = pos;
				if (_pos < sizeSource) {
					_out = source + _pos;
					pos = sizeSource;
					return sizeSource - _pos;
				}
				return SLIB_IO_ERROR;
			}

			Memory readFully(sl_size size)
			{
				if (size) {
					sl_uint32 _pos = pos;
					if (_pos + size <= sizeSource) {
						Memory ret = Memory::createStatic(source + _pos, size);
						if (ret.isNotNull()) {
							pos += (sl_uint32)size;
							return ret;
						}
					}
				}
				return sl_null;
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
		class ContextT : public Context, public READER_BASE
		{
		public:
			using READER_BASE::readChar;
			using READER_BASE::peekChar;
			using READER_BASE::read;
			using READER_BASE::peek;
			using READER_BASE::getPosition;
			using READER_BASE::setPosition;
			using READER_BASE::movePosition;
			using READER_BASE::readBuffer;
			using READER_BASE::readFully;
			using READER_BASE::findBackward;
			using Context::getObject;

		public:
			ContextT(sl_bool flagRefContext): Context(flagRefContext) {}
			ContextT(Context* baseContext): Context(baseContext) {}

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

			sl_bool tryReadWordAndEquals(const StringView& word)
			{
				sl_uint32 pos = getPosition();
				if (readWordAndEquals(word)) {
					return sl_true;
				}
				setPosition(pos);
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

			String readString(const PdfReference& objectId)
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
										case '\r':
											if (i + 1 < n && buf[i + 1] == '\n') {
												i++;
											}
											ch = 0;
											break;
										case '\n':
											ch = 0;
											break;
										default:
											return sl_null;
									}
									if (ch) {
										list.add_NoLock(ch);
									}
								}
								flagEscape = sl_false;
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
											String ret(list.getData(), list.getCount());
											if (ret.isNotNull()) {
												if (flagDecryptContents && objectId.objectNumber) {
													decrypt(objectId, ret.getData(), ret.getLength());
												}
												return ret;
											} else {
												return String::getEmpty();
											}
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

			String readHexString(const PdfReference& objectId)
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
								String ret(list.getData(), list.getCount());
								if (ret.isNotNull()) {
									if (flagDecryptContents && objectId.objectNumber) {
										decrypt(objectId, ret.getData(), ret.getLength());
									}
									return ret;
								} else {
									return String::getEmpty();
								}
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

			Ref<PdfDictionary> readDictionary(const PdfReference& objectId)
			{
				sl_char8 buf[2];
				if (read(buf, 2) != 2) {
					return sl_null;
				}
				if (buf[0] != '<' || buf[1] != '<') {
					return sl_null;
				}
				Ref<PdfDictionary> ret = new PdfDictionary(m_baseContext);
				if (ret.isNull()) {
					return sl_null;
				}
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
						PdfValue value = readValue(objectId);
						if (value.isUndefined()) {
							return sl_null;
						}
						ret->add_NoLock(Move(name), Move(value));
					} else if (ch == '>') {
						movePosition(1);
						if (!(readCharAndEquals('>'))) {
							return sl_null;
						}
						return ret;
					} else {
						return sl_null;
					}
				}
				return sl_null;
			}

			Ref<PdfArray> readArray(const PdfReference& objectId)
			{
				if (!(readCharAndEquals('['))) {
					return sl_null;
				}
				Ref<PdfArray> ret = new PdfArray(m_baseContext);
				if (ret.isNull()) {
					return sl_null;
				}
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
						return ret;
					}
					PdfValue var = readValue(objectId);
					if (var.isNotUndefined()) {
						ret->add_NoLock(Move(var));
					} else {
						return sl_null;
					}
				}
				return sl_null;
			}

			PdfValue readNumber()
			{
				sl_char8 ch;
				if (peekChar(ch)) {
					if (ch >= '0' && ch <= '9') {
						sl_uint32 posBackup = getPosition();
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
							return PdfValue();
						}
						if (!(peekChar(ch))) {
							return PdfValue();
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
						if (readUint(value, sl_true)) {
							if (peekCharAndEquals('.')) {
								movePosition(1);
								double f;
								if (!(readFraction(f, sl_true))) {
									return PdfValue();
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
				return PdfValue();
			}

			PdfOperator readOperator()
			{
				sl_char8 buf[4];
				sl_reg len = peek(buf, sizeof(buf));
				if (len > 0) {
					for (sl_reg i = 0; i < len; i++) {
						if (IsWhitespace(buf[i]) || IsDelimiter(buf[i])) {
							len = i;
							break;
						}
					}
					if (len > 0 && len <= 3) {
						PdfOperator op = PdfOperation::getOperator(StringView(buf, len));
						if (op != PdfOperator::Unknown) {
							movePosition(len);
							return op;
						}
					}
				}
				return PdfOperator::Unknown;
			}

			PdfCMapOperator readCMapOperator()
			{
				sl_char8 buf[20];
				sl_reg len = peek(buf, sizeof(buf));
				if (len > 0) {
					for (sl_reg i = 0; i < len; i++) {
						if (IsWhitespace(buf[i]) || IsDelimiter(buf[i])) {
							len = i;
							break;
						}
					}
					if (len > 0 && len <= 19) {
						PdfCMapOperator op = PdfOperation::getCMapOperator(StringView(buf, len));
						if (op != PdfCMapOperator::Unknown) {
							movePosition(len);
							return op;
						}
					}
				}
				return PdfCMapOperator::Unknown;
			}

			PdfValue readValue(const PdfReference& objectId, sl_bool flagReadOnlyStream = sl_false)
			{
				sl_char8 ch;
				if (!(peekChar(ch))) {
					return PdfValue();
				}
				if (flagReadOnlyStream) {
					if (ch != '<') {
						return PdfValue();
					}
				}
				switch (ch) {
					case 'n':
						if (tryReadWordAndEquals(StringView::literal("null"))) {
							return sl_null;
						}
						break;
					case 't':
						if (tryReadWordAndEquals(StringView::literal("true"))) {
							return sl_true;
						}
						break;
					case 'f':
						if (tryReadWordAndEquals(StringView::literal("false"))) {
							return sl_false;
						}
						break;
					case '(':
						{
							String s = readString(objectId);
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
								Ref<PdfDictionary> map = readDictionary(objectId);
								if (map.isNotNull()) {
									return map;
								}
							} else {
								String s = readHexString(objectId);
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
							Ref<PdfArray> list = readArray(objectId);
							if (list.isNotNull()) {
								return list;
							}
						}
						break;
					default:
						if (IsNumeric(ch)) {
							return readNumber();
						}
						break;
				}
				return PdfValue();
			}

			Memory readContent(sl_uint32 offset, sl_uint32 size, const PdfReference& ref) override
			{
				if (setPosition(offset)) {
					Memory ret = readFully(size);
					if (ret.isNotNull()) {
						if (flagDecryptContents) {
							if (!(ret.getRef())) {
								ret = Memory::create(ret.getData(), size);
								if (ret.isNull()) {
									return sl_null;
								}
							}
							decrypt(ref, ret.getData(), size);
						}
						return ret;
					}
				}
				return sl_null;
			}

			sl_uint32 getStreamOffset(sl_uint32 length, sl_uint32& outOffsetAfterEndStream)
			{
				if (readWordAndEquals(StringView::literal("stream"))) {
					sl_char8 ch;
					if (readChar(ch)) {
						if (ch == '\r') {
							if (!(readChar(ch))) {
								return 0;
							}
							if (ch != '\n') {
								return 0;
							}
						} else if (ch != '\n') {
							return 0;
						}
						sl_uint32 offset = getPosition();
						if (length) {
							if (!(movePosition(length))) {
								return 0;
							}
						}
						if (skipWhitespaces()) {
							if (readWordAndEquals(StringView::literal("endstream"))) {
								outOffsetAfterEndStream = getPosition();
								return offset;
							}
						}
					}
				}
				return 0;
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

			PdfValue readObject(PdfReference& outRef, sl_bool flagReadOnlyStream = sl_false)
			{
				if (readObjectHeader(outRef)) {
					PdfValue value = readValue(outRef, flagReadOnlyStream);
					if (value.isNotUndefined()) {
						if (skipWhitespaces()) {
							const Ref<PdfDictionary>& properties = value.getDictionary();
							if (properties.isNotNull()) {
								if (peekCharAndEquals('s')) {
									sl_uint32 pos = getPosition();
									sl_uint32 length;
									if (!(properties->get(name::Length).getUint(length))) {
										return PdfValue();
									}
									setPosition(pos); // Protect position while getting stream length
									sl_uint32 offsetEndStream;
									sl_uint32 offsetContent = getStreamOffset(length, offsetEndStream);
									if (!offsetContent) {
										return PdfValue();
									}
									if (!(skipWhitespaces())) {
										return PdfValue();
									}
									Ref<PdfStream> stream = new PdfStream(m_baseContext);
									if (stream.isNull()) {
										return PdfValue();
									}
									stream->initialize(properties, outRef, offsetContent, length);
									value = PdfValue(Move(stream));
								}
							}
							if (readWordAndEquals(StringView::literal("endobj"))) {
								return value;
							}
						}
					}
				}
				return PdfValue();
			}

			PdfValue readObject(sl_uint32 pos, sl_uint32& outOffsetAfterEndObj, PdfReference& outRef, sl_bool flagReadOnlyStream) override
			{
				if (setPosition(pos)) {
					PdfValue ret = readObject(outRef, flagReadOnlyStream);
					if (ret.isNotUndefined()) {
						outOffsetAfterEndObj = getPosition();
						return ret;
					}
				}
				return PdfValue();
			}

			Ref<PdfDictionary> readTrailer()
			{
				if (readWordAndEquals(StringView::literal("trailer"))) {
					if (skipWhitespaces()) {
						return readDictionary(PdfReference(0, 0));
					}
				}
				return sl_null;
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
										entry.type = (sl_uint32)(CrossReferenceEntryType::Free);
									} else if (ch == 'n') {
										entry.type = (sl_uint32)(CrossReferenceEntryType::Normal);
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

			sl_bool readCrossReferenceSection()
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
								setReferenceEntry(firstObjectNumber + i, entry);
							}
							return sl_true;
						}
					}
				}
				return sl_false;
			}

			sl_bool readCrossReferenceStream(Ref<PdfDictionary>& outTrailer)
			{
				PdfReference refStream;
				PdfValue vStream = readObject(refStream);
				if (refStream.generation) {
					return sl_false;
				}
				const Ref<PdfStream>& stream = vStream.getStream();
				if (stream.isNull()) {
					return sl_false;
				}
				if (!(stream->getProperty(name::Type).equalsName(name::XRef))) {
					return sl_false;
				}
				sl_uint32 size;
				if (!(stream->getProperty(name::Size).getUint(size))) {
					return sl_false;
				}
				Ref<PdfArray> entrySizes = stream->getProperty(name::W).getArray();
				if (entrySizes.isNull()) {
					return sl_false;
				}
				if (entrySizes->getCount() != 3) {
					return sl_false;
				}
				if (!(entrySizes->get(0).getType() == PdfValueType::Uint && entrySizes->get(1).getType() == PdfValueType::Uint && entrySizes->get(2).getType() == PdfValueType::Uint)) {
					return sl_false;
				}
				CList< Pair<sl_uint32, sl_uint32> > listSectionRanges;
				sl_size nEntries = 0;
				PdfValue vIndex = stream->getProperty(name::Index);
				if (vIndex.isNotUndefined()) {
					const Ref<PdfArray>& indices = vIndex.getArray();
					if (indices.isNull()) {
						return sl_false;
					}
					sl_uint32 n = indices->getCount();
					if (n & 1) {
						return sl_false;
					}
					for (sl_size i = 0; i < n; i += 2) {
						sl_uint32 start, count;
						if (!(indices->get(i).getUint(start))) {
							return sl_false;
						}
						if (!(indices->get(i + 1).getUint(count))) {
							return sl_false;
						}
						listSectionRanges.add_NoLock(start, count);
						nEntries += count;
					}
				} else {
					listSectionRanges.add_NoLock(0, size);
					nEntries = size;
				}
				sl_uint32 sizeType = entrySizes->get(0).getUint();
				sl_uint32 sizeOffset = entrySizes->get(1).getUint();
				sl_uint32 sizeGeneration = entrySizes->get(2).getUint();
				sl_uint32 sizeEntry = sizeType + sizeOffset + sizeGeneration;
				Memory content = stream->getDecodedContent();
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
							entry.type = type;
							p += sizeType;
							entry.offset = ReadUint(p, sizeOffset, 0);
							p += sizeOffset;
							entry.generation = ReadUint(p, sizeGeneration, 0);
							p += sizeGeneration;
							setReferenceEntry(range.first + i, entry);
						}
					}
					outTrailer = stream->properties;
					return sl_true;
				}
				return sl_false;
			}

			sl_bool readCrossReferenceTable(Ref<PdfDictionary>& outTrailer)
			{
				if (!(readWordAndEquals(StringView::literal("xref")))) {
					return sl_false;
				}
				sl_char8 ch;
				for (;;) {
					if (!(skipWhitespaces())) {
						return sl_false;
					}
					if (!(peekChar(ch))) {
						break;
					}
					if (ch == 't') {
						outTrailer = readTrailer();
						if (outTrailer.isNotNull()) {
							sl_uint32 offsetXRefStream;
							if (outTrailer->get(name::XRefStm).getUint(offsetXRefStream)) {
								if (setPosition(offsetXRefStream)) {
									readCrossReferenceStream(outTrailer);
								}
							}
							return sl_true;
						} else {
							return sl_false;
						}
					} else if (ch >= '0' && ch <= '9') {
						if (!(readCrossReferenceSection())) {
							return sl_false;
						}
					} else {
						break;
					}
				}
				return sl_true;
			}

			sl_bool readCrossReferences(Ref<PdfDictionary>& outTrailer)
			{
				sl_char8 ch;
				if (!(peekChar(ch))) {
					return sl_false;
				}
				if (ch == 'x') {
					return readCrossReferenceTable(outTrailer);
				} else {
					return readCrossReferenceStream(outTrailer);
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

			sl_bool readDocument(const PdfDocumentParam& param) override
			{
				sl_char8 version[8];
				if (read(version, 8) != 8) {
					return sl_false;
				}
				if (version[0] != '%' || version[1] != 'P' || version[2] != 'D' || version[3] != 'F' || version[4] != '-' || version[6] != '.') {
					return sl_false;
				}

				sl_char8 c5 = version[5];
				if (!SLIB_CHAR_IS_DIGIT(c5)) {
					return sl_false;
				}
				sl_char8 c7 = version[7];
				if (!SLIB_CHAR_IS_DIGIT(c7)) {
					return sl_false;
				}

				majorVersion = SLIB_CHAR_DIGIT_TO_INT(c5);
				minorVersion = SLIB_CHAR_DIGIT_TO_INT(c7);

				// read last trailer and read reference table
				{
					sl_uint32 posStartXref, posXref;
					if (!(readStartXref(posStartXref, posXref))) {
						return sl_false;
					}
					// last trailer
					{
						if (!(setPosition(posXref))) {
							return sl_false;
						}
						if (!(readCrossReferences(lastTrailer))) {
							return sl_false;
						}
					}
					// initialize reference table
					{
						sl_uint32 countTotalRef = 0;
						lastTrailer->get(name::Size).getUint(countTotalRef);
						if (!countTotalRef) {
							return sl_false;
						}
						m_references = Array<CrossReferenceEntry>::create(countTotalRef);
						if (m_references.isNull()) {
							return sl_false;
						}
						Base::zeroMemory(m_references.getData(), countTotalRef * sizeof(CrossReferenceEntry));
						m_maxObjectNumber = countTotalRef - 1;
					}
					for (;;) {
						if (!(setPosition(posXref))) {
							return sl_false;
						}
						Ref<PdfDictionary> trailer;
						if (!(readCrossReferences(trailer))) {
							return sl_false;
						}
						PdfValue prev = trailer->get(name::Prev);
						if (prev.isUndefined()) {
							break;
						}
						if (!(prev.getUint(posXref))) {
							return sl_false;
						}
					}
				}
				return initDocument(param);
			}

		};

		typedef ContextT<BufferedReaderBase> BufferedContext;
		typedef ContextT<MemoryReaderBase> MemoryContext;

		Ref<ObjectStream> Context::getObjectStream(const PdfReference& ref)
		{
			sl_uint64 _id = GetObjectId(ref);
			Ref<ObjectStream> ret;
			if (m_objectStreams.get(_id, &ret)) {
				return ret;
			}
			Ref<PdfStream> stream = getObject(ref).getStream();
			if (stream.isNull()) {
				return sl_null;
			}
			if (!(stream->getProperty(name::Type).equalsName(name::ObjStm))) {
				return sl_null;
			}
			sl_uint32 nObjects;
			if (!(stream->getProperty(name::N).getUint(nObjects))) {
				return sl_null;
			}
			sl_uint32 first;
			if (!(stream->getProperty(name::First).getUint(first))) {
				return sl_null;
			}
			Memory content = stream->getDecodedContent();
			if (content.isNull()) {
				return sl_null;
			}
			ret = new ObjectStream;
			if (ret.isNull()) {
				return sl_null;
			}
			PdfValue vExtends = stream->getProperty(name::Extends, sl_false);
			if (vExtends.isNotUndefined()) {
				PdfReference refExtends;
				if (!(vExtends.getReference(refExtends))) {
					return sl_null;
				}
				if (refExtends == ref) {
					return sl_null;
				}
				ret->extends = getObjectStream(refExtends);
				if (ret->extends.isNull()) {
					return sl_null;
				}
			}
			MemoryContext context(m_baseContext);
			context.source = (sl_char8*)(content.getData());
			context.sizeSource = (sl_uint32)(content.getSize());
			for (sl_uint32 i = 0; i < nObjects; i++) {
				if (!(context.skipWhitespaces())) {
					return sl_null;
				}
				sl_uint32 innerId;
				if (!(context.readUint(innerId))) {
					return sl_null;
				}
				if (!(context.skipWhitespaces())) {
					return sl_null;
				}
				sl_uint32 offset;
				if (!(context.readUint(offset))) {
					return sl_null;
				}
				sl_uint32 pos = context.getPosition();
				if (!(context.setPosition(first + offset))) {
					return sl_null;
				}
				PdfValue innerValue = context.readValue(PdfReference(0, 0));
				if (innerValue.isUndefined()) {
					return sl_null;
				}
				ret->objects.add_NoLock(innerId, Move(innerValue));
				context.setPosition(pos);
			}
			m_objectStreams.put(_id, ret);
			return ret;
		}

		SLIB_INLINE static Context* GetContext(const Ref<CRef>& ref)
		{
			return (Context*)(ref.get());
		}

		SLIB_INLINE static Ref<Context> GetContextRef(const Ref<CRef>& ref)
		{
			return Ref<Context>::from(ref);
		}

	}


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(PdfName)

	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(PdfValue)

	PdfValue::PdfValue(sl_bool v) noexcept: m_var(v, (sl_uint8)(PdfValueType::Boolean)) {}

	PdfValue::PdfValue(sl_int32 v) noexcept: m_var(v, (sl_uint8)(PdfValueType::Int)) {}
	PdfValue::PdfValue(sl_uint32 v) noexcept: m_var(v, (sl_uint8)(PdfValueType::Uint)) {}
	PdfValue::PdfValue(float v) noexcept: m_var(v, (sl_uint8)(PdfValueType::Float)) {}

	PdfValue::PdfValue(const String& v) noexcept: m_var(v, (sl_uint8)(v.isNotNull() ? PdfValueType::String : PdfValueType::Null)) {}
	PdfValue::PdfValue(String&& v) noexcept: m_var(Move(v), (sl_uint8)(v.isNotNull() ? PdfValueType::String : PdfValueType::Null)) {}

	PdfValue::PdfValue(const PdfName& v) noexcept: m_var(v.value, (sl_uint8)(v.isNotNull() ? PdfValueType::Name : PdfValueType::Null)) {}
	PdfValue::PdfValue(PdfName&& v) noexcept: m_var(Move(v.value), (sl_uint8)(v.isNotNull() ? PdfValueType::Name : PdfValueType::Null)) {}

	PdfValue::PdfValue(const PdfReference& v) noexcept: m_var(MAKE_OBJECT_ID(v.objectNumber, v.generation), (sl_uint8)(PdfValueType::Reference)) {}

	PdfValue::PdfValue(const Ref<PdfArray>& v) noexcept: m_var(v, (sl_uint8)(v.isNotNull() ? PdfValueType::Array : PdfValueType::Null)) {}
	PdfValue::PdfValue(Ref<PdfArray>&& v) noexcept: m_var(Move(v), (sl_uint8)(v.isNotNull() ? PdfValueType::Array : PdfValueType::Null)) {}
	PdfValue::PdfValue(PdfArray* v) noexcept : m_var(Ref<PdfArray>(v), (sl_uint8)(v ? PdfValueType::Array : PdfValueType::Null)) {}

	PdfValue::PdfValue(const Ref<PdfDictionary>& v) noexcept: m_var(v, (sl_uint8)(v.isNotNull() ? PdfValueType::Dictionary : PdfValueType::Null)) {}
	PdfValue::PdfValue(Ref<PdfDictionary>&& v) noexcept: m_var(Move(v), (sl_uint8)(v.isNotNull() ? PdfValueType::Dictionary : PdfValueType::Null)) {}
	PdfValue::PdfValue(PdfDictionary* v) noexcept : m_var(Ref<PdfDictionary>(v), (sl_uint8)(v ? PdfValueType::Dictionary : PdfValueType::Null)) {}

	PdfValue::PdfValue(const Ref<PdfStream>& v) noexcept: m_var(v, (sl_uint8)(v.isNotNull() ? PdfValueType::Stream : PdfValueType::Null)) {}
	PdfValue::PdfValue(Ref<PdfStream>&& v) noexcept: m_var(Move(v), (sl_uint8)(v.isNotNull() ? PdfValueType::Stream : PdfValueType::Null)) {}
	PdfValue::PdfValue(PdfStream* v) noexcept : m_var(Ref<PdfStream>(v), (sl_uint8)(v ? PdfValueType::Stream : PdfValueType::Null)) {}

	PdfValue::PdfValue(const Ref<PdfImage>& v) noexcept: m_var(v, (sl_uint8)(v.isNotNull() ? PdfValueType::Image : PdfValueType::Null)) {}
	PdfValue::PdfValue(Ref<PdfImage>&& v) noexcept: m_var(Move(v), (sl_uint8)(v.isNotNull() ? PdfValueType::Image : PdfValueType::Null)) {}
	PdfValue::PdfValue(PdfImage* v) noexcept : m_var(Ref<PdfImage>(v), (sl_uint8)(v ? PdfValueType::Image : PdfValueType::Null)) {}

	PdfValue::PdfValue(const Rectangle& v) noexcept: PdfValue(PdfArray::create(v)) {}

	sl_bool PdfValue::getBoolean() const noexcept
	{
		if (getType() == PdfValueType::Boolean) {
			return m_var._m_boolean;
		}
		return sl_false;
	}

	sl_bool PdfValue::getBoolean(sl_bool& _out) const noexcept
	{
		if (getType() == PdfValueType::Boolean) {
			_out = m_var._m_boolean;
			return sl_true;
		}
		return sl_false;
	}

	sl_uint32 PdfValue::getUint() const noexcept
	{
		sl_uint32 ret;
		if (getUint(ret)) {
			return ret;
		}
		return 0;
	}

	sl_bool PdfValue::getUint(sl_uint32& _out) const noexcept
	{
		PdfValueType type = getType();
		if (type == PdfValueType::Uint) {
			_out = m_var._m_uint32;
			return sl_true;
		} else if (type == PdfValueType::Int) {
			sl_int32 n = m_var._m_int32;
			if (n >= 0) {
				_out = n;
				return sl_true;
			}
		}
		return sl_false;
	}

	sl_int32 PdfValue::getInt() const noexcept
	{
		sl_int32 ret;
		if (getInt(ret)) {
			return ret;
		}
		return 0;
	}

	sl_bool PdfValue::getInt(sl_int32& _out) const noexcept
	{
		PdfValueType type = getType();
		if (type == PdfValueType::Int || type == PdfValueType::Uint) {
			_out = m_var._m_int32;
			return sl_true;
		}
		return sl_false;
	}

	float PdfValue::getFloat() const noexcept
	{
		float ret;
		if (getFloat(ret)) {
			return ret;
		}
		return 0;
	}

	sl_bool PdfValue::getFloat(float& _out) const noexcept
	{
		PdfValueType type = getType();
		if (type == PdfValueType::Float) {
			_out = m_var._m_float;
			return sl_true;
		} else if (type == PdfValueType::Uint) {
			_out = (float)(m_var._m_uint32);
			return sl_true;
		} else if (type == PdfValueType::Int) {
			_out = (float)(m_var._m_int32);
			return sl_true;
		}
		return sl_false;
	}

	sl_bool PdfValue::isNumeric() const noexcept
	{
		PdfValueType type = getType();
		return type == PdfValueType::Int || type == PdfValueType::Uint || type == PdfValueType::Float;
	}

	const String& PdfValue::getString() const& noexcept
	{
		if (getType() == PdfValueType::String) {
			return CAST_VAR(String, m_var._value);
		}
		return String::null();
	}

	String PdfValue::getString()&& noexcept
	{
		if (getType() == PdfValueType::String) {
			return CAST_VAR(String, m_var._value);
		}
		return sl_null;
	}

	const String& PdfValue::getName() const& noexcept
	{
		if (getType() == PdfValueType::Name) {
			return CAST_VAR(String, m_var._value);
		}
		return String::null();
	}

	String PdfValue::getName()&& noexcept
	{
		if (getType() == PdfValueType::Name) {
			return CAST_VAR(String, m_var._value);
		}
		return sl_null;
	}

	sl_bool PdfValue::equalsName(const StringView& name) const noexcept
	{
		if (getType() == PdfValueType::Name) {
			return CAST_VAR(String, m_var._value) == name;
		}
		return sl_false;
	}

	PdfReference PdfValue::getReference() const noexcept
	{
		PdfReference ret;
		if (getType() == PdfValueType::Reference) {
			ret.objectNumber = SLIB_GET_DWORD0(m_var._value);
			ret.generation = SLIB_GET_DWORD1(m_var._value);
		} else {
			ret.objectNumber = 0;
			ret.generation = 0;
		}
		return ret;
	}

	sl_bool PdfValue::getReference(PdfReference& _out) const noexcept
	{
		if (getType() == PdfValueType::Reference) {
			_out.objectNumber = SLIB_GET_DWORD0(m_var._value);
			_out.generation = SLIB_GET_DWORD1(m_var._value);
			return sl_true;
		}
		return sl_false;
	}

	const Ref<PdfArray>& PdfValue::getArray() const& noexcept
	{
		if (getType() == PdfValueType::Array) {
			return CAST_VAR(Ref<PdfArray>, m_var._value);
		}
		return Ref<PdfArray>::null();
	}

	Ref<PdfArray> PdfValue::getArray()&& noexcept
	{
		if (getType() == PdfValueType::Array) {
			return CAST_VAR(Ref<PdfArray>, m_var._value);
		}
		return sl_null;
	}

	const List<PdfValue>& PdfValue::getElements() const& noexcept
	{
		if (getType() == PdfValueType::Array) {
			return CAST_VAR(List<PdfValue>, m_var._value);
		}
		return List<PdfValue>::null();
	}

	List<PdfValue> PdfValue::getElements()&& noexcept
	{
		if (getType() == PdfValueType::Array) {
			return CAST_VAR(List<PdfValue>, m_var._value);
		}
		return sl_null;
	}

	sl_uint32 PdfValue::getElementCount() const noexcept
	{
		if (getType() == PdfValueType::Array) {
			return CAST_VAR(PdfArray*, m_var._value)->getCount();
		}
		return 0;
	}

	PdfValue PdfValue::getElement(sl_size index, sl_bool flagResolveReference) const
	{
		if (getType() == PdfValueType::Array) {
			return CAST_VAR(PdfArray*, m_var._value)->get(index, flagResolveReference);
		} else {
			return PdfValue();
		}
	}

	const Ref<PdfDictionary>& PdfValue::getDictionary() const& noexcept
	{
		if (getType() == PdfValueType::Dictionary) {
			return CAST_VAR(Ref<PdfDictionary>, m_var._value);
		}
		return Ref<PdfDictionary>::null();
	}

	Ref<PdfDictionary> PdfValue::getDictionary()&& noexcept
	{
		if (getType() == PdfValueType::Dictionary) {
			return CAST_VAR(Ref<PdfDictionary>, m_var._value);
		}
		return sl_null;
	}

	PdfValue PdfValue::getItem(const String& name, sl_bool flagResolveReference) const
	{
		if (getType() == PdfValueType::Dictionary) {
			return CAST_VAR(PdfDictionary*, m_var._value)->get(name, flagResolveReference);
		} else {
			return PdfValue();
		}
	}

	PdfValue PdfValue::getItem(const String& name, const String& alternateName, sl_bool flagResolveReference) const
	{
		if (getType() == PdfValueType::Dictionary) {
			return CAST_VAR(PdfDictionary*, m_var._value)->get(name, alternateName, flagResolveReference);
		} else {
			return PdfValue();
		}
	}

	const Ref<PdfStream>& PdfValue::getStream() const& noexcept
	{
		if (getType() == PdfValueType::Stream) {
			return CAST_VAR(Ref<PdfStream>, m_var._value);
		}
		return Ref<PdfStream>::null();
	}

	Ref<PdfStream> PdfValue::getStream()&& noexcept
	{
		if (getType() == PdfValueType::Stream) {
			return CAST_VAR(Ref<PdfStream>, m_var._value);
		}
		return sl_null;
	}

	Memory PdfValue::getDecodedStreamContent() const
	{
		if (getType() == PdfValueType::Stream) {
			return CAST_VAR(PdfStream*, m_var._value)->getDecodedContent();
		}
		return sl_null;
	}

	const Ref<PdfImage>& PdfValue::getImage() const& noexcept
	{
		if (getType() == PdfValueType::Image) {
			return CAST_VAR(Ref<PdfImage>, m_var._value);
		}
		return Ref<PdfImage>::null();
	}

	Ref<PdfImage> PdfValue::getImage()&& noexcept
	{
		if (getType() == PdfValueType::Image) {
			return CAST_VAR(Ref<PdfImage>, m_var._value);
		}
		return sl_null;
	}

	Rectangle PdfValue::getRectangle() const noexcept
	{
		Rectangle ret;
		if (getRectangle(ret)) {
			return ret;
		}
		return Rectangle::zero();
	}

	sl_bool PdfValue::getRectangle(Rectangle& _out) const noexcept
	{
		const Ref<PdfArray>& arr = getArray();
		if (arr.isNull()) {
			return sl_false;
		}
		if (arr->getCount() == 4) {
			_out.left = arr->get(0).getFloat();
			_out.top = arr->get(1).getFloat();
			_out.right = arr->get(2).getFloat();
			_out.bottom = arr->get(3).getFloat();
			return sl_true;
		}
		return sl_false;
	}

	Matrix3 PdfValue::getMatrix() const noexcept
	{
		Matrix3 ret;
		if (getMatrix(ret)) {
			return ret;
		}
		return Matrix3::zero();
	}

	sl_bool PdfValue::getMatrix(Matrix3& _out) const noexcept
	{
		const Ref<PdfArray>& arr = getArray();
		if (arr.isNull()) {
			return sl_false;
		}
		if (arr->getCount() == 6) {
			_out.m00 = arr->get(0).getFloat();
			_out.m01 = arr->get(1).getFloat();
			_out.m02 = 0;
			_out.m10 = arr->get(2).getFloat();
			_out.m11 = arr->get(3).getFloat();
			_out.m12 = 0;
			_out.m20 = arr->get(4).getFloat();
			_out.m21 = arr->get(5).getFloat();
			_out.m22 = 1;
			return sl_true;
		}
		return sl_false;
	}

	PdfValue PdfValue::operator[](const String& name) const
	{
		if (getType() == PdfValueType::Dictionary) {
			return CAST_VAR(PdfDictionary*, m_var._value)->get(name);
		} else {
			return PdfValue();
		}
	}

	PdfValue PdfValue::operator[](sl_size index) const
	{
		if (getType() == PdfValueType::Array) {
			return CAST_VAR(PdfArray*, m_var._value)->get(index);
		} else {
			return PdfValue();
		}
	}


	SLIB_DEFINE_OBJECT(PdfArray, CListBase)

	PdfArray::PdfArray(CRef* context) noexcept: m_context(context) {}

	PdfArray::~PdfArray()
	{
	}

	sl_uint32 PdfArray::getCount() const noexcept
	{
		return (sl_uint32)(CList::getCount());
	}

	PdfValue PdfArray::get(sl_size index, sl_bool flagResolveReference) const
	{
		if (!flagResolveReference) {
			return getValueAt_NoLock(index);
		}
		PdfValue ret;
		if (getAt_NoLock(index, &ret)) {
			PdfReference ref;
			if (ret.getReference(ref)) {
				Ref<Context> context = GetContextRef(m_context);
				if (context.isNotNull()) {
					ObjectLocker locker(context.get());
					return context->getObject(ref);
				}
			} else {
				return ret;
			}
		}
		return PdfValue();
	}

	Ref<PdfArray> PdfArray::create(const Rectangle& v)
	{
		Ref<PdfArray> ret = new PdfArray(sl_null);
		if (ret.isNotNull()) {
			ret->add_NoLock(v.left);
			ret->add_NoLock(v.top);
			ret->add_NoLock(v.right);
			ret->add_NoLock(v.bottom);
			return ret;
		} else {
			return sl_null;
		}
	}


	SLIB_DEFINE_OBJECT(PdfDictionary, CHashMapBase)

	PdfDictionary::PdfDictionary(CRef* context) noexcept: m_context(context) {}

	PdfDictionary::~PdfDictionary()
	{
	}

	PdfValue PdfDictionary::get(const String& name, sl_bool flagResolveReference) const
	{
		if (!flagResolveReference) {
			return getValue_NoLock(name);
		}
		PdfValue ret;
		if (get_NoLock(name, &ret)) {
			PdfReference ref;
			if (ret.getReference(ref)) {
				Ref<Context> context = GetContextRef(m_context);
				if (context.isNotNull()) {
					ObjectLocker locker(context.get());
					return context->getObject(ref);
				}
			} else {
				return ret;
			}
		}
		return PdfValue();
	}

	PdfValue PdfDictionary::get(const String& name, const String& alternateName, sl_bool flagResolveReference) const
	{
		PdfValue ret = get(name, flagResolveReference);
		if (ret.isNotUndefined()) {
			return ret;
		}
		return get(alternateName, flagResolveReference);
	}



	SLIB_DEFINE_ROOT_OBJECT(PdfStream)

	PdfStream::PdfStream(CRef* context) noexcept: m_context(context), m_ref(0, 0), m_offsetContent(0), m_sizeContent(0)
	{
	}

	PdfStream::~PdfStream()
	{
	}

	void PdfStream::initialize(const Ref<PdfDictionary>& _properties, const PdfReference& ref, sl_uint32 offsetContent, sl_uint32 sizeContent) noexcept
	{
		properties = _properties;
		m_ref = ref;
		m_offsetContent = offsetContent;
		m_sizeContent = sizeContent;
	}

	PdfValue PdfStream::getProperty(const String& name, sl_bool flagResolveReference) noexcept
	{
		if (properties.isNotNull()) {
			return properties->get(name, flagResolveReference);
		} else {
			return PdfValue();
		}
	}

	PdfValue PdfStream::getProperty(const String& name, const String& alternateName, sl_bool flagResolveReference) noexcept
	{
		if (properties.isNotNull()) {
			return properties->get(name, alternateName, flagResolveReference);
		} else {
			return PdfValue();
		}
	}

	Memory PdfStream::getEncodedContent()
	{
		if (!m_offsetContent) {
			return m_contentEncoded;
		}
		Ref<Context> context = GetContextRef(m_context);
		if (context.isNotNull()) {
			ObjectLocker locker(context.get());
			return context->readContent(m_offsetContent, m_sizeContent, m_ref);
		}
		return sl_null;
	}

	void PdfStream::setEncodedContent(const Memory& content) noexcept
	{
		m_contentEncoded = content;
		m_offsetContent = 0;
		m_sizeContent = 0;
	}

	Memory PdfStream::getDecodedContent()
	{
		return getDecodedContent(getEncodedContent());
	}

	Memory PdfStream::getDecodedContent(const Memory& content)
	{
		if (content.isNull()) {
			return sl_null;
		}
		PdfValue vFilter = getProperty(name::Filter, name::F);
		if (vFilter.isUndefined()) {
			return content;
		}
		PdfValue decodeParams = getProperty(name::DecodeParms, name::DP);
		const Ref<PdfArray>& arrFilter = vFilter.getArray();
		if (arrFilter.isNotNull()) {
			Memory ret = content;
			const Ref<PdfArray>& arrDecodeParams = decodeParams.getArray();
			sl_uint32 nFilters = arrFilter->getCount();
			for (sl_size i = 0; i < nFilters; i++) {
				PdfFilter filter = Pdf::getFilter(arrFilter->get(i).getName());
				if (filter != PdfFilter::Unknown) {
					Ref<PdfDictionary> params;
					if (arrDecodeParams.isNotNull()) {
						params = arrDecodeParams->get(i).getDictionary();
					}
					ret = decodeContent(ret, filter, params.get());
				} else {
					return sl_null;
				}
			}
			return ret;
		} else {
			PdfFilter filter = Pdf::getFilter(vFilter.getName());
			if (filter != PdfFilter::Unknown) {
				return decodeContent(content, filter, decodeParams.getDictionary().get());
			}
		}
		return sl_null;
	}

	Memory PdfStream::getFilterInput(PdfFilter filterMatch)
	{
		if (filterMatch == PdfFilter::Unknown) {
			return sl_null;
		}
		PdfValue vFilter = getProperty(name::Filter, name::F);
		if (vFilter.isUndefined()) {
			return sl_null;
		}
		const Ref<PdfArray>& arrFilter = vFilter.getArray();
		if (arrFilter.isNotNull()) {
			sl_uint32 nFilters = arrFilter->getCount();
			{
				sl_size i = 0;
				for (;;) {
					if (i >= nFilters) {
						return sl_null;
					}
					PdfFilter filter = Pdf::getFilter(arrFilter->get(i).getName());
					if (filter == filterMatch) {
						break;
					}
					i++;
				}
			}
			{
				Memory content = getEncodedContent();
				PdfValue decodeParams = getProperty(name::DecodeParms, name::DP);
				const Ref<PdfArray>& arrDecodeParams = decodeParams.getArray();
				for (sl_size i = 0; i < nFilters; i++) {
					PdfFilter filter = Pdf::getFilter(arrFilter->get(i).getName());
					if (filter == filterMatch) {
						return content;
					}
					if (filter != PdfFilter::Unknown) {
						Ref<PdfDictionary> params;
						if (arrDecodeParams.isNotNull()) {
							params = arrDecodeParams->get(i).getDictionary();
						}
						content = decodeContent(content, filter, params.get());
					} else {
						return sl_null;
					}
				}
			}
		} else {
			PdfFilter filter = Pdf::getFilter(vFilter.getName());
			if (filter == filterMatch) {
				return getEncodedContent();
			}
		}
		return sl_null;
	}

	namespace {

		static Memory DecodeASCIIHex(const void* _input, sl_size len)
		{
			Memory ret = Memory::create((len + 1) >> 1);
			if (ret.isNull()) {
				return sl_null;
			}
			sl_char8* str = (sl_char8*)_input;
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
				} else if (!(IsWhitespace(ch)) && ch != '>') {
					return sl_null;
				}
			}
			if (!flagFirstHex) {
				*(cur++) = firstHex << 4;
			}
			return ret.sub(0, cur - dst);
		}

		static Memory CreateMemoryFromList(CList<sl_uint8>* pList)
		{
			sl_size n = pList->getCount();
			if (!n) {
				return sl_null;
			}
			pList->setCapacity_NoLock(n);
			return Memory::createStatic(pList->getData(), pList->getCount(), pList);
		}

		static Memory DecodeASCII85(const void* _input, sl_size len)
		{
			List<sl_uint8> list = List<sl_uint8>::create(0, ((len + 4) / 5) << 2);
			CList<sl_uint8>* pList = list.ref.get();
			if (!pList) {
				return sl_null;
			}
			sl_char8* str = (sl_char8*)_input;
			sl_uint32 indexElement = 0;
			sl_uint32 dword = 0;
			for (sl_size i = 0; i < len; i++) {
				sl_uint8 v = str[i];
				if (v == 'z') {
					if (indexElement) {
						return sl_null;
					} else {
						pList->addElements_NoLock(4, 0);
					}
				} else {
					if (v >= '!' && v <= 'u') {
						v -= '!';
						dword = dword * 85 + v;
						indexElement++;
						if (indexElement >= 5) {
							sl_uint8 bytes[4];
							MIO::writeUint32BE(bytes, dword);
							pList->addElements_NoLock(bytes, 4);
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
									pList->addElements_NoLock(bytes, indexElement - 1);
								}
								return CreateMemoryFromList(pList);
							}
						}
						return sl_null;
					} else if (!(IsWhitespace(v)) && v != '>') {
						return sl_null;
					}
				}
			}
			return sl_null;
		}

		static Memory DecodeRunLength(const void* _input, sl_size size)
		{
			if (!size) {
				return sl_null;
			}
			List<sl_uint8> list = List<sl_uint8>::create();
			CList<sl_uint8>* pList = list.ref.get();
			if (!pList) {
				return sl_null;
			}
			sl_uint8* input = (sl_uint8*)_input;
			sl_size pos = 0;
			for (;;) {
				sl_uint8 len = input[pos];
				if (len == 0x80) {
					break;
				}
				if (len & 0x80) {
					pos++;
					if (pos >= size) {
						return sl_null;
					}
					sl_uint8 v = input[pos];
					pos++;
					if (!(pList->addElements_NoLock(257 - len, v))) {
						return sl_null;
					}
				} else {
					pos++;
					len++;
					if (pos + len > size) {
						return sl_null;
					}
					if (!(pList->addElements_NoLock(input + pos, len))) {
						return sl_null;
					}
					pos += len;
				}
				if (pos >= size) {
					break;
				}
			}
			return CreateMemoryFromList(pList);
		}

		static Memory CreateImageMemory(const Ref<Image>& image)
		{
			if (image.isNotNull()) {
				return Memory::createStatic(image->getColors(), (image->getStride() * image->getHeight()) << 2, image);
			}
			return sl_null;
		}

		static void InvertBlackWhite(Image* image)
		{
			Color* colors = image->getColors();
			sl_uint32 w = image->getWidth();
			sl_uint32 h = image->getHeight();
			for (sl_uint32 i = 0; i < h; i++) {
				for (sl_uint32 j = 0; j < w; j++) {
					if (colors[j].r) {
						colors[j] = Color::Black;
					} else {
						colors[j] = Color::White;
					}
				}
				colors += image->getStride();
			}
		}

		static const sl_uint8 g_faxBlackRunIns[] = {
			0,          2,          0x02,       3,          0,          0x03,
			2,          0,          2,          0x02,       1,          0,
			0x03,       4,          0,          2,          0x02,       6,
			0,          0x03,       5,          0,          1,          0x03,
			7,          0,          2,          0x04,       9,          0,
			0x05,       8,          0,          3,          0x04,       10,
			0,          0x05,       11,         0,          0x07,       12,
			0,          2,          0x04,       13,         0,          0x07,
			14,         0,          1,          0x18,       15,         0,
			5,          0x08,       18,         0,          0x0f,       64,
			0,          0x17,       16,         0,          0x18,       17,
			0,          0x37,       0,          0,          10,         0x08,
			0x00,       0x07,       0x0c,       0x40,       0x07,       0x0d,
			0x80,       0x07,       0x17,       24,         0,          0x18,
			25,         0,          0x28,       23,         0,          0x37,
			22,         0,          0x67,       19,         0,          0x68,
			20,         0,          0x6c,       21,         0,          54,
			0x12,       1984 % 256, 1984 / 256, 0x13,       2048 % 256, 2048 / 256,
			0x14,       2112 % 256, 2112 / 256, 0x15,       2176 % 256, 2176 / 256,
			0x16,       2240 % 256, 2240 / 256, 0x17,       2304 % 256, 2304 / 256,
			0x1c,       2368 % 256, 2368 / 256, 0x1d,       2432 % 256, 2432 / 256,
			0x1e,       2496 % 256, 2496 / 256, 0x1f,       2560 % 256, 2560 / 256,
			0x24,       52,         0,          0x27,       55,         0,
			0x28,       56,         0,          0x2b,       59,         0,
			0x2c,       60,         0,          0x33,       320 % 256,  320 / 256,
			0x34,       384 % 256,  384 / 256,  0x35,       448 % 256,  448 / 256,
			0x37,       53,         0,          0x38,       54,         0,
			0x52,       50,         0,          0x53,       51,         0,
			0x54,       44,         0,          0x55,       45,         0,
			0x56,       46,         0,          0x57,       47,         0,
			0x58,       57,         0,          0x59,       58,         0,
			0x5a,       61,         0,          0x5b,       256 % 256,  256 / 256,
			0x64,       48,         0,          0x65,       49,         0,
			0x66,       62,         0,          0x67,       63,         0,
			0x68,       30,         0,          0x69,       31,         0,
			0x6a,       32,         0,          0x6b,       33,         0,
			0x6c,       40,         0,          0x6d,       41,         0,
			0xc8,       128,        0,          0xc9,       192,        0,
			0xca,       26,         0,          0xcb,       27,         0,
			0xcc,       28,         0,          0xcd,       29,         0,
			0xd2,       34,         0,          0xd3,       35,         0,
			0xd4,       36,         0,          0xd5,       37,         0,
			0xd6,       38,         0,          0xd7,       39,         0,
			0xda,       42,         0,          0xdb,       43,         0,
			20,         0x4a,       640 % 256,  640 / 256,  0x4b,       704 % 256,
			704 / 256,  0x4c,       768 % 256,  768 / 256,  0x4d,       832 % 256,
			832 / 256,  0x52,       1280 % 256, 1280 / 256, 0x53,       1344 % 256,
			1344 / 256, 0x54,       1408 % 256, 1408 / 256, 0x55,       1472 % 256,
			1472 / 256, 0x5a,       1536 % 256, 1536 / 256, 0x5b,       1600 % 256,
			1600 / 256, 0x64,       1664 % 256, 1664 / 256, 0x65,       1728 % 256,
			1728 / 256, 0x6c,       512 % 256,  512 / 256,  0x6d,       576 % 256,
			576 / 256,  0x72,       896 % 256,  896 / 256,  0x73,       960 % 256,
			960 / 256,  0x74,       1024 % 256, 1024 / 256, 0x75,       1088 % 256,
			1088 / 256, 0x76,       1152 % 256, 1152 / 256, 0x77,       1216 % 256,
			1216 / 256, 0xff };

		static const sl_uint8 g_faxWhiteRunIns[] = {
			0,          0,          0,          6,          0x07,       2,
			0,          0x08,       3,          0,          0x0B,       4,
			0,          0x0C,       5,          0,          0x0E,       6,
			0,          0x0F,       7,          0,          6,          0x07,
			10,         0,          0x08,       11,         0,          0x12,
			128,        0,          0x13,       8,          0,          0x14,
			9,          0,          0x1b,       64,         0,          9,
			0x03,       13,         0,          0x07,       1,          0,
			0x08,       12,         0,          0x17,       192,        0,
			0x18,       1664 % 256, 1664 / 256, 0x2a,       16,         0,
			0x2B,       17,         0,          0x34,       14,         0,
			0x35,       15,         0,          12,         0x03,       22,
			0,          0x04,       23,         0,          0x08,       20,
			0,          0x0c,       19,         0,          0x13,       26,
			0,          0x17,       21,         0,          0x18,       28,
			0,          0x24,       27,         0,          0x27,       18,
			0,          0x28,       24,         0,          0x2B,       25,
			0,          0x37,       256 % 256,  256 / 256,  42,         0x02,
			29,         0,          0x03,       30,         0,          0x04,
			45,         0,          0x05,       46,         0,          0x0a,
			47,         0,          0x0b,       48,         0,          0x12,
			33,         0,          0x13,       34,         0,          0x14,
			35,         0,          0x15,       36,         0,          0x16,
			37,         0,          0x17,       38,         0,          0x1a,
			31,         0,          0x1b,       32,         0,          0x24,
			53,         0,          0x25,       54,         0,          0x28,
			39,         0,          0x29,       40,         0,          0x2a,
			41,         0,          0x2b,       42,         0,          0x2c,
			43,         0,          0x2d,       44,         0,          0x32,
			61,         0,          0x33,       62,         0,          0x34,
			63,         0,          0x35,       0,          0,          0x36,
			320 % 256,  320 / 256,  0x37,       384 % 256,  384 / 256,  0x4a,
			59,         0,          0x4b,       60,         0,          0x52,
			49,         0,          0x53,       50,         0,          0x54,
			51,         0,          0x55,       52,         0,          0x58,
			55,         0,          0x59,       56,         0,          0x5a,
			57,         0,          0x5b,       58,         0,          0x64,
			448 % 256,  448 / 256,  0x65,       512 % 256,  512 / 256,  0x67,
			640 % 256,  640 / 256,  0x68,       576 % 256,  576 / 256,  16,
			0x98,       1472 % 256, 1472 / 256, 0x99,       1536 % 256, 1536 / 256,
			0x9a,       1600 % 256, 1600 / 256, 0x9b,       1728 % 256, 1728 / 256,
			0xcc,       704 % 256,  704 / 256,  0xcd,       768 % 256,  768 / 256,
			0xd2,       832 % 256,  832 / 256,  0xd3,       896 % 256,  896 / 256,
			0xd4,       960 % 256,  960 / 256,  0xd5,       1024 % 256, 1024 / 256,
			0xd6,       1088 % 256, 1088 / 256, 0xd7,       1152 % 256, 1152 / 256,
			0xd8,       1216 % 256, 1216 / 256, 0xd9,       1280 % 256, 1280 / 256,
			0xda,       1344 % 256, 1344 / 256, 0xdb,       1408 % 256, 1408 / 256,
			0,          3,          0x08,       1792 % 256, 1792 / 256, 0x0c,
			1856 % 256, 1856 / 256, 0x0d,       1920 % 256, 1920 / 256, 10,
			0x12,       1984 % 256, 1984 / 256, 0x13,       2048 % 256, 2048 / 256,
			0x14,       2112 % 256, 2112 / 256, 0x15,       2176 % 256, 2176 / 256,
			0x16,       2240 % 256, 2240 / 256, 0x17,       2304 % 256, 2304 / 256,
			0x1c,       2368 % 256, 2368 / 256, 0x1d,       2432 % 256, 2432 / 256,
			0x1e,       2496 % 256, 2496 / 256, 0x1f,       2560 % 256, 2560 / 256,
			0xff,
		};

		class FaxImageDecoder
		{
		public:
			Ref<Image> output;

		private:
			sl_uint32 columns;
			sl_uint32 rows;

			sl_int32 encoding;
			sl_bool flagEndOfLine;
			sl_bool flagByteAlign;

			sl_uint8* content;
			sl_uint32 bitPos = 0;
			sl_uint32 bitSize;

		private:
			SLIB_INLINE sl_bool getNextBit()
			{
				sl_bool ret = (content[bitPos >> 3] >> (7 - (bitPos & 7))) & 1;
				bitPos++;
				return ret;
			}

			sl_int32 getRun(const sl_uint8* ins_array) {
				sl_uint32 code = 0;
				sl_int32 ins_off = 0;
				for (;;) {
					sl_uint8 ins = ins_array[ins_off++];
					if (ins == 0xff) {
						return -1;
					}
					if (bitPos >= bitSize) {
						return -1;
					}
					code <<= 1;
					if (getNextBit()) {
						code++;
					}
					sl_int32 next_off = ins_off + ins * 3;
					for (; ins_off < next_off; ins_off += 3) {
						if (ins_array[ins_off] == code) {
							return SLIB_MAKE_WORD(ins_array[ins_off + 2], ins_array[ins_off + 1]);
						}
					}
				}
			}

			void skipEOL()
			{
				sl_uint32 start = bitPos;
				while (bitPos < bitSize) {
					if (getNextBit()) {
						if (bitPos - start <= 11) {
							bitPos = start;
						}
						return;
					}
				}
			}

			static sl_bool getBit(const Color* ref, sl_int32 index)
			{
				if (ref && index >= 0) {
					return ref[index].r != 0;
				} else {
					return sl_true;
				}
			}

			static sl_uint32 findBit(const Color* ref, sl_uint32 length, sl_uint32 start, sl_bool bit) {
				if (ref) {
					while (start < length) {
						if (bit) {
							if (ref[start].r) {
								return start;
							}
						} else {
							if (!(ref[start].r)) {
								return start;
							}
						}
						start++;
					}
					return length;
				} else {
					if (bit && start < length) {
						return start;
					} else {
						return length;
					}
				}
			}

			static void fillBits(Color* dst, sl_int32 limit, sl_int32 start, sl_int32 end)
			{
				if (end > limit) {
					end = limit;
				}
				while (start < end) {
					dst[start] = Color::Black;
					start++;
				}
			}

			void G4_findB1B2(Color* ref, sl_int32 a0, sl_bool a0color, sl_uint32& b1, sl_uint32& b2)
			{
				sl_bool bitFirst = getBit(ref, a0);
				b1 = findBit(ref, columns, a0 + 1, !bitFirst);
				if (b1 >= columns) {
					b1 = b2 = columns;
					return;
				}
				if (bitFirst == !a0color) {
					b1 = findBit(ref, columns, b1 + 1, bitFirst);
					bitFirst = !bitFirst;
				}
				if (b1 >= columns) {
					b1 = b2 = columns;
					return;
				}
				b2 = findBit(ref, columns, b1 + 1, bitFirst);
			}

			void decodeScanline_G4(Color* colors, Color* ref)
			{
				sl_int32 a0 = -1;
				sl_bool a0color = sl_true;
				for (;;) {
					if (bitPos >= bitSize) {
						return;
					}
					sl_uint32 b1, b2;
					G4_findB1B2(ref, a0, a0color, b1, b2);
					sl_int32 v_delta = 0;
					if (!(getNextBit())) {
						if (bitPos >= bitSize) {
							return;
						}
						bool bit1 = getNextBit();
						if (bitPos >= bitSize) {
							return;
						}
						bool bit2 = getNextBit();
						if (bit1) {
							v_delta = bit2 ? 1 : -1;
						} else if (bit2) {
							sl_int32 run_len1 = 0;
							for (;;) {
								sl_int32 run = getRun(a0color ? g_faxWhiteRunIns : g_faxBlackRunIns);
								run_len1 += run;
								if (run < 64) {
									break;
								}
							}
							if (a0 < 0) {
								run_len1++;
							}
							if (run_len1 < 0) {
								return;
							}
							sl_int32 a1 = a0 + run_len1;
							if (!a0color) {
								fillBits(colors, columns, a0, a1);
							}

							sl_int32 run_len2 = 0;
							for (;;) {
								int run = getRun(a0color ? g_faxBlackRunIns : g_faxWhiteRunIns);
								run_len2 += run;
								if (run < 64) {
									break;
								}
							}
							if (run_len2 < 0) {
								return;
							}
							sl_int32 a2 = a1 + run_len2;
							if (a0color) {
								fillBits(colors, columns, a1, a2);
							}

							a0 = a2;
							if (a0 < (sl_int32)columns) {
								continue;
							}
							return;
						} else {
							if (bitPos >= bitSize) {
								return;
							}
							if (getNextBit()) {
								if (!a0color) {
									fillBits(colors, columns, a0, b2);
								}
								if (b2 >= columns) {
									return;
								}
								a0 = b2;
								continue;
							}

							if (bitPos >= bitSize) {
								return;
							}

							sl_bool next_bit1 = getNextBit();
							if (bitPos >= bitSize) {
								return;
							}
							sl_bool next_bit2 = getNextBit();
							if (next_bit1) {
								v_delta = next_bit2 ? 2 : -2;
							} else if (next_bit2) {
								if (bitPos >= bitSize) {
									return;
								}
								v_delta = getNextBit() ? 3 : -3;
							} else {
								if (bitPos >= bitSize) {
									return;
								}
								if (getNextBit()) {
									bitPos += 3;
									continue;
								}
								bitPos += 5;
								return;
							}
						}
					}

					sl_int32 a1 = b1 + v_delta;
					if (!a0color) {
						fillBits(colors, columns, a0, a1);
					}
					if (a1 >= (sl_int32)columns) {
						return;
					}

					// The position of picture element must be monotonic increasing.
					if (a0 >= a1) {
						return;
					}

					a0 = a1;
					a0color = !a0color;
				}
			}

			void decodeScanline_1D(Color* colors)
			{
				sl_bool color = sl_true;
				sl_int32 start = 0;
				for (;;) {
					if (bitPos >= bitSize) {
						return;
					}
					sl_int32 run_len = 0;
					for (;;) {
						sl_int32 run = getRun(color ? g_faxWhiteRunIns : g_faxBlackRunIns);
						if (run < 0) {
							while (bitPos < bitSize) {
								if (getNextBit()) {
									return;
								}
							}
							return;
						}
						run_len += run;
						if (run < 64) {
							break;
						}
					}
					if (!color) {
						fillBits(colors, columns, start, start + run_len);
					}
					start += run_len;
					if (start >= (sl_int32)columns) {
						break;
					}
					color = !color;
				}
			}

			void decodeScanline(Color* colors, Color* ref)
			{
				skipEOL();
				if (bitPos >= bitSize) {
					return;
				}
				Base::resetMemory(colors, columns << 2, 255);
				if (encoding < 0) {
					decodeScanline_G4(colors, ref);
				} else if (!encoding) {
					decodeScanline_1D(colors);
				} else {
					if (getNextBit()) {
						decodeScanline_1D(colors);
					} else {
						decodeScanline_G4(colors, ref);
					}
				}
				if (flagEndOfLine) {
					skipEOL();
				}
				if (flagByteAlign && bitPos < bitSize) {
					sl_uint32 bitPos0 = bitPos;
					sl_uint32 bitPos1 = (bitPos + 7) & 0xFFFFFFF8;
					while (flagByteAlign && bitPos0 < bitPos1) {
						if ((content[bitPos0 >> 3] >> (7 - (bitPos0 & 3))) & 1) {
							flagByteAlign = sl_false;
						} else {
							bitPos0++;
						}
					}
					if (flagByteAlign) {
						bitPos = bitPos1;
					}
				}
			}

		public:
			sl_bool run(const void* _content, sl_size size, sl_uint32 width, sl_uint32 height, const PdfCCITTFaxDecodeParams& params)
			{
				columns = params.columns;
				if (!columns) {
					columns = 1728;
				}
				if (columns > width) {
					columns = width;
				}
				rows = params.rows;
				if (!rows || rows > height) {
					rows = height;
				}
				output = Image::allocate(columns, rows);
				if (output.isNull()) {
					return sl_false;
				}
				encoding = params.K;
				flagEndOfLine = params.flagEndOfLine;
				flagByteAlign = params.flagByteAlign;
				content = (sl_uint8*)_content;
				bitSize = (sl_uint32)size << 3;
				Color* colors = output->getColors();
				Color* top = sl_null;
				for (sl_uint32 iRow = 0; iRow < rows; iRow++) {
					decodeScanline(colors, top);
					top = colors;
					colors += columns;
				}
				if (params.flagBlackIs1) {
					InvertBlackWhite(output.get());
				}
				return sl_true;
			}

		};

		static Ref<Image> DecodeFaxImage(const void* content, sl_size size, sl_uint32 width, sl_uint32 height, const PdfCCITTFaxDecodeParams& params)
		{
			FaxImageDecoder decoder;
			if (decoder.run(content, size, width, height, params)) {
				return decoder.output;
			}
			return sl_null;
		}

	}

	Memory PdfStream::decodeContent(const MemoryView& input, PdfFilter filter, PdfDictionary* params)
	{
		switch (filter) {
			case PdfFilter::ASCIIHex:
				return DecodeASCIIHex(input.data, input.size);
			case PdfFilter::ASCII85:
				return DecodeASCII85(input.data, input.size);
			case PdfFilter::Flate:
			case PdfFilter::LZW:
				{
					Memory ret;
					if (filter == PdfFilter::Flate) {
						ret = Zlib::decompress(input.data, input.size);
					} else {
						ret = LZW::decompress(input.data, input.size);
					}
					if (ret.isNotNull() && params) {
						PdfFlateOrLZWDecodeParams dp;
						dp.setParams(params);
						sl_uint32 n = dp.predict(ret.getData(), (sl_uint32)(ret.getSize()));
						if (n) {
							ret = ret.sub(0, n);
						}
					}
					return ret;
				}
			case PdfFilter::RunLength:
				return DecodeRunLength(input.data, input.size);
			case PdfFilter::CCITTFax:
				if (params) {
					PdfCCITTFaxDecodeParams dp;
					dp.setParams(params);
					sl_uint32 width = getProperty(name::Width, name::W).getUint();
					sl_uint32 height = getProperty(name::Height, name::H).getUint();
					return CreateImageMemory(DecodeFaxImage(input.data, input.size, width, height, dp));
				}
				break;
			case PdfFilter::DCT:
				return CreateImageMemory(Image::loadFromMemory(input));
			default:
				break;
		}
		return sl_null;
	}

	sl_bool PdfStream::isJpegImage() noexcept
	{
		String subtype = getProperty(name::Subtype).getName();
		if (subtype == name::Image) {
			PdfValue vFilter = getProperty(name::Filter, name::F);
			if (vFilter.isUndefined()) {
				return sl_false;
			}
			const Ref<PdfArray>& arrFilter = vFilter.getArray();
			if (arrFilter.isNotNull()) {
				sl_uint32 nFilters = arrFilter->getCount();
				for (sl_uint32 i = 0; i < nFilters; i++) {
					PdfFilter filter = Pdf::getFilter(arrFilter->get(i).getName());
					if (filter == PdfFilter::DCT) {
						return sl_true;
					}
				}
			} else {
				PdfFilter filter = Pdf::getFilter(vFilter.getName());
				return filter == PdfFilter::DCT;
			}
		}
		return sl_false;
	}

	void PdfStream::setJpegFilter() noexcept
	{
		if (properties.isNotNull()) {
			properties->put_NoLock(name::Filter, PdfName(name::DCT));
		}
	}

	void PdfStream::setLength(sl_uint32 len) noexcept
	{
		if (properties.isNotNull()) {
			properties->put_NoLock(name::Length, len);
		}
	}

	Ref<PdfStream> PdfStream::create(const Memory& content)
	{
		Ref<PdfDictionary> properties = new PdfDictionary(sl_null);
		if (properties.isNotNull()) {
			Ref<PdfStream> ret = new PdfStream(sl_null);
			if (ret.isNotNull()) {
				properties->put_NoLock(name::Length, (sl_uint32)(content.getSize()));
				ret->properties = Move(properties);
				ret->setEncodedContent(content);
				return ret;
			}
		}
		return sl_null;
	}

	Ref<PdfStream> PdfStream::createJpegImage(sl_uint32 width, sl_uint32 height, const Memory& content)
	{
		Ref<PdfDictionary> properties = new PdfDictionary(sl_null);
		if (properties.isNotNull()) {
			Ref<PdfStream> ret = new PdfStream(sl_null);
			if (ret.isNotNull()) {
				properties->put_NoLock(name::Type, PdfName(name::XObject));
				properties->put_NoLock(name::Subtype, PdfName(name::Image));
				properties->put_NoLock(name::Length, (sl_uint32)(content.getSize()));
				properties->put_NoLock(name::Filter, PdfName(name::DCTDecode));
				properties->put_NoLock(name::Width, width);
				properties->put_NoLock(name::Height, height);
				properties->put_NoLock(name::ColorSpace, PdfName(name::DeviceRGB));
				properties->put_NoLock(name::BitsPerComponent, (sl_uint32)8);
				ret->properties = Move(properties);
				ret->setEncodedContent(content);
				return ret;
			}
		}
		return sl_null;
	}


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(PdfFunction)

	PdfFunction::PdfFunction(): type(PdfFunctionType::Unknown), countInput(0), countOutput(0), bitsPerSample(0), N(0)
	{
	}

	sl_bool PdfFunction::load(const PdfValue& value)
	{
		PdfDictionary* dict;
		PdfStream* stream = sl_null;
		{
			const Ref<PdfDictionary>& refDict = value.getDictionary();
			if (refDict.isNotNull()) {
				dict = refDict.get();
			} else {
				const Ref<PdfStream>& refStream = value.getStream();
				if (refStream.isNotNull()) {
					stream = refStream.get();
					dict = stream->properties.get();
				} else {
					return sl_false;
				}
			}
			if (!dict) {
				return sl_false;
			}
		}
		PdfFunctionType _type;
		{
			sl_uint32 n;
			if (dict->get(name::FunctionType).getUint(n)) {
				_type = (PdfFunctionType)n;
			} else {
				return sl_false;
			}
		}
		{
			Ref<PdfArray> arr = dict->get(name::Domain).getArray();
			if (arr.isNull()) {
				return sl_false;
			}
			sl_uint32 n = arr->getCount();
			if (n & 1) {
				return sl_false;
			}
			countInput = n >> 1;
			if (!countInput) {
				return sl_false;
			}
			domain = Array< Pair<float, float> >::create(countInput);
			if (domain.isNull()) {
				return sl_false;
			}
			for (sl_uint32 i = 0; i < countInput; i++) {
				domain[i].first = arr->get(i << 1).getFloat();
				domain[i].second = arr->get((i << 1) | 1).getFloat();
			}
		}

		{
			Ref<PdfArray> arr = dict->get(name::Range).getArray();
			if (arr.isNotNull()) {
				sl_uint32 n = arr->getCount();
				if (n & 1) {
					return sl_false;
				}
				countOutput = n >> 1;
			} else {
				countOutput = 0;
			}
			if (countOutput) {
				range = Array< Pair<float, float> >::create(countOutput);
				if (range.isNull()) {
					return sl_false;
				}
				for (sl_uint32 i = 0; i < countOutput; i++) {
					range[i].first = arr->get(i << 1).getFloat();
					range[i].second = arr->get((i << 1) | 1).getFloat();
				}
			} else {
				if (_type == PdfFunctionType::Sampled || _type == PdfFunctionType::PostScript) {
					return sl_false;
				}
			}
		}

		if (_type == PdfFunctionType::Sampled) {

			if (!stream) {
				return sl_false;
			}

			bitsPerSample = dict->get(name::BitsPerSample).getUint();
			switch (bitsPerSample) {
				case 1:
				case 2:
				case 4:
				case 8:
				case 12:
				case 24:
				case 32:
					break;
				default:
					return sl_false;
			}

			Ref<PdfArray> arrEncode = dict->get(name::Encode).getArray();
			if (arrEncode.isNotNull()) {
				if (arrEncode->getCount() != countInput << 1) {
					return sl_false;
				}
				encodeSampled = Array< Pair<sl_uint32, sl_uint32> >::create(countInput);
				if (encodeSampled.isNull()) {
					return sl_false;
				}
				for (sl_uint32 i = 0; i < countInput; i++) {
					encodeSampled[i].first = arrEncode->get(i << 1).getUint();
					encodeSampled[i].second = arrEncode->get((i << 1) | 1).getUint();
				}
			}

			sl_uint32 nSamples = 1;
			Ref<PdfArray> arrSize = dict->get(name::Size).getArray();
			if (arrSize.isNull()) {
				return sl_false;
			}
			if (arrSize->getCount() == countInput) {
				size = Array<sl_uint32>::create(countInput);
				if (size.isNull()) {
					return sl_false;
				}
				stride = Array<sl_uint32>::create(countInput);
				if (stride.isNull()) {
					return sl_false;
				}
				for (sl_uint32 i = 0; i < countInput; i++) {
					sl_uint32 n = arrSize->get(i).getUint();
					if (!n) {
						return sl_false;
					}
					size[i] = n;
					stride[i] = nSamples;
					nSamples *= n;
				}
			} else {
				return sl_false;
			}

			Memory content = stream->getDecodedContent();
			if (content.isNull()) {
				return sl_false;
			}

			Ref<PdfArray> arrDecode = dict->get(name::Decode).getArray();
			if (arrDecode.isNotNull()) {
				if (arrDecode->getCount() != countOutput << 1) {
					return sl_false;
				}
				decode = Array< Pair<float, float> >::create(countOutput);
				if (decode.isNull()) {
					return sl_false;
				}
				for (sl_uint32 i = 0; i < countOutput; i++) {
					decode[i].first = arrDecode->get(i << 1).getFloat();
					decode[i].second = arrDecode->get((i << 1) | 1).getFloat();
				}
			}

			samples = Array< Array<float> >::create(countOutput);
			if (samples.isNull()) {
				return sl_false;
			}

			SampleReader reader(content.getData(), content.getSize(), bitsPerSample);
			{
				for (sl_uint32 i = 0; i < countOutput; i++) {
					float add, scale;
					if (decode.isNotNull()) {
						add = decode[i].first;
						scale = (decode[i].second - decode[i].first) / (float)(1 << bitsPerSample);
					} else {
						add = range[i].first;
						scale = (range[i].second - range[i].first) / (float)(1 << bitsPerSample);
					}
					float min = range[i].first;
					float max = range[i].second;
					Array<float> s = Array<float>::create(nSamples);
					if (s.isNull()) {
						return sl_false;
					}
					for (sl_uint32 k = 0; k < nSamples; k++) {
						sl_uint32 n;
						if (reader.read(n)) {
							s[k] = Math::clamp((float)n * scale + add, min, max);
						} else {
							return sl_false;
						}
					}
					samples[i] = Move(s);
				}
			}
			type = _type;

		} else if (_type == PdfFunctionType::Exponential) {

			if (countInput != 1) {
				return sl_false;
			}

			if (!(dict->get(name::N).getFloat(N))) {
				return sl_false;
			}
			Ref<PdfArray> arrC0 = dict->get(name::C0).getArray();
			if (arrC0.isNotNull()) {
				sl_uint32 n = arrC0->getCount();
				if (!n) {
					return sl_false;
				}
				if (countOutput) {
					if (countOutput != n) {
						return sl_false;
					}
				} else {
					countOutput = n;
				}
				C0 = Array<float>::create(countOutput);
				if (C0.isNull()) {
					return sl_false;
				}
				for (sl_uint32 i = 0; i < countOutput; i++) {
					C0[i] = arrC0->get(i).getFloat();
				}
			}
			Ref<PdfArray> arrC1 = dict->get(name::C1).getArray();
			if (arrC1.isNotNull()) {
				sl_uint32 n = arrC1->getCount();
				if (!n) {
					return sl_false;
				}
				if (countOutput) {
					if (countOutput != n) {
						return sl_false;
					}
				} else {
					countOutput = n;
				}
				C1 = Array<float>::create(countOutput);
				if (C1.isNull()) {
					return sl_false;
				}
				for (sl_uint32 i = 0; i < countOutput; i++) {
					C1[i] = arrC1->get(i).getFloat();
				}
			}
			if (!countOutput) {
				countOutput = 1;
			}
			type = _type;

		} else if (_type == PdfFunctionType::Stiching) {

			if (countInput != 1) {
				return sl_false;
			}

			Ref<PdfArray> arrFuncs = dict->get(name::Functions).getArray();
			if (arrFuncs.isNull()) {
				return sl_false;
			}
			sl_uint32 k = arrFuncs->getCount();
			if (k) {
				functions = Array<PdfFunction>::create(k);
				for (sl_uint32 i = 0; i < k; i++) {
					if (!(functions[i].load(arrFuncs->get(i)))) {
						return sl_false;
					}
					if (countOutput) {
						if (functions[i].countOutput != countOutput) {
							return sl_false;
						}
					} else {
						countOutput = functions[i].countOutput;
					}
				}
			} else {
				return sl_false;
			}

			Ref<PdfArray> arrBounds = dict->get(name::Bounds).getArray();
			if (arrBounds.isNull()) {
				return sl_false;
			}
			if (arrBounds->getCount() == k - 1) {
				bounds = Array<float>::create(k - 1);
				if (bounds.isNull()) {
					return sl_false;
				}
				for (sl_uint32 i = 0; i < k - 1; i++) {
					bounds[i] = arrBounds->get(i).getFloat();
				}
			} else {
				return sl_false;
			}

			Ref<PdfArray> arrEncode = dict->get(name::Encode).getArray();
			if (arrEncode.isNull()) {
				return sl_false;
			}
			if (arrEncode->getCount() == k << 1) {
				encodeStiching = Array< Pair<float, float> >::create(k);
				if (encodeStiching.isNull()) {
					return sl_false;
				}
				for (sl_uint32 i = 0; i < k; i++) {
					encodeStiching[i].first = arrEncode->get(i << 1).getFloat();
					encodeStiching[i].second = arrEncode->get((i << 1) | 1).getFloat();
				}
			} else {
				return sl_false;
			}
			type = _type;

		} else {
			return sl_false;
		}
		return sl_true;
	}

	sl_bool PdfFunction::call(float* input, sl_size nInput, float* output, sl_size nOutput)
	{
		if (nInput != countInput || nOutput != countOutput) {
			return sl_false;
		}
		if (type == PdfFunctionType::Sampled) {
			for (sl_uint32 i = 0; i < countOutput; i++) {
				sl_uint32 index = 0;
				for (sl_uint32 j = 0; j < countInput; j++) {
					sl_int32 maxIndex = size[j] - 1;
					float f = input[j];
					float add, scale;
					if (encodeSampled.isNotNull()) {
						add = (float)(encodeSampled[j].first);
						scale = (float)(encodeSampled[j].second - encodeSampled[j].first);
					} else {
						add = 0;
						scale = (float)maxIndex;
					}
					float drange = domain[j].second - domain[j].first;
					if (Math::isAlmostZero(drange)) {
						f = 0;
					} else {
						f = (f - domain[j].first) / drange;
					}
					f = add + f * scale;
					sl_int32 n = Math::clamp((sl_int32)f, 0, maxIndex);
					index += n * stride[j];
				}
				output[i] = samples[i][index];
			}
			return sl_true;
		} else if (type == PdfFunctionType::Exponential) {
			for (sl_uint32 i = 0; i < countOutput; i++) {
				float f = Math::pow(input[0], N);
				float c0 = 0, c1 = 1;
				if (C0.isNotNull()) {
					c0 = C0[i];
				}
				if (C1.isNotNull()) {
					c1 = C1[i];
				}
				f = c0 + f * (c1 - c0);
				if (range.isNotNull()) {
					f = Math::clamp(f, range[i].first, range[i].second);
				}
				output[i] = f;
			}
			return sl_true;
		} else if (type == PdfFunctionType::Stiching) {
			float f = input[0];
			float start = domain[0].first;
			float end = domain[0].second;
			f = Math::clamp(f, start, end);
			sl_uint32 k = (sl_uint32)(functions.getCount());
			for (sl_uint32 i = 0; i < k; i++) {
				float limit;
				if (i == k - 1) {
					limit = end;
				} else {
					limit = bounds[i];
				}
				if (i == k - 1 || f < limit) {
					float r = limit - start;
					if (Math::isAlmostZero(r)) {
						f = 0;
					} else {
						f = (f - start) / r;
					}
					f = encodeStiching[i].first + f * (encodeStiching[i].second - encodeStiching[i].first);
					return functions[i].call(&f, 1, output, nOutput);
				}
				start = limit;
			}
		}
		return sl_false;
	}


	sl_bool PdfResourceProvider::getFontResource(const String& name, PdfReference& outRef)
	{
		return getResource(name::Font, name, sl_false).getReference(outRef);
	}

	sl_bool PdfResourceProvider::getExternalObjectResource(const String& name, PdfReference& outRef)
	{
		return getResource(name::XObject, name, sl_false).getReference(outRef);
	}


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(PdfFlateOrLZWDecodeParams)

	PdfFlateOrLZWDecodeParams::PdfFlateOrLZWDecodeParams(): predictor(0), columns(0), bitsPerComponent(8), colors(1), earlyChange(1)
	{
	}

	void PdfFlateOrLZWDecodeParams::setParams(PdfDictionary* dict) noexcept
	{
		dict->get(name::Predictor).getUint(predictor);
		dict->get(name::Columns).getUint(columns);
		dict->get(name::BitsPerComponent).getUint(bitsPerComponent);
		dict->get(name::Colors).getUint(colors);
		dict->get(name::EarlyChange).getUint(earlyChange);
	}

	namespace {

		static sl_uint8 PredictPath(sl_int32 a, sl_int32 b, sl_int32 c)
		{
			sl_int32 p = a + b - c;
			sl_int32 pa = Math::abs(p - a);
			sl_int32 pb = Math::abs(p - b);
			sl_int32 pc = Math::abs(p - c);
			if (pa <= pb && pa <= pc) {
				return (sl_uint8)a;
			} else if (pb <= pc) {
				return (sl_uint8)b;
			} else {
				return (sl_uint8)c;
			}
		}

		static sl_bool PredictPNG(sl_uint8* bufData, sl_uint32& sizeData, sl_uint32 colors, sl_uint32 bitsPerComponent, sl_uint32 columns)
		{
			sl_uint32 nBytesPerPixel = (colors * bitsPerComponent + 7) >> 3;
			sl_uint32 sizeRow = (colors * bitsPerComponent * columns + 7) >> 3;
			if (!sizeRow) {
				return sl_false;
			}
			sl_uint32 nFullRows = sizeData / (sizeRow + 1);
			sl_uint32 sizeRemain = sizeData % (sizeRow + 1);
			sl_uint32 nRows = sizeRemain ? nFullRows + 1 : nFullRows;
			if (!nRows) {
				return sl_false;
			}
			if (sizeRemain) {
				sizeData = sizeRow * nFullRows + sizeRemain - 1;
			} else {
				sizeData = sizeRow * nRows;
			}
			sl_uint8* pRowDst = bufData;
			sl_uint8* pRowSrc = bufData;
			for (sl_uint32 iRow = 0; iRow < nRows; iRow++) {
				sl_uint32 n = iRow < nFullRows ? sizeRow : sizeRemain - 1;
				sl_uint8 tag = *pRowSrc;
				if (tag) {
					for (sl_uint32 i = 0; i < n; i++) {
						sl_uint8 diff = pRowSrc[i + 1];
						sl_uint8 base = 0;
						switch (tag) {
							case 1:
								if (i >= nBytesPerPixel) {
									base = pRowDst[(sl_int32)(i - nBytesPerPixel)];
								}
								break;
							case 2:
								if (iRow) {
									base = pRowDst[(sl_int32)(i - sizeRow)];
								}
								break;
							case 3:
								{
									sl_uint8 left = 0;
									if (i >= nBytesPerPixel) {
										left = pRowDst[(sl_int32)(i - nBytesPerPixel)];
									}
									sl_uint8 up = 0;
									if (iRow) {
										up = pRowDst[(sl_int32)(i - sizeRow)];
									}
									base = (sl_uint8)(((sl_uint32)up + (sl_uint32)left) >> 1);
								}
								break;
							case 4:
								{
									sl_uint8 left = 0, upperLeft = 0;
									if (i >= nBytesPerPixel) {
										left = pRowDst[(sl_int32)(i - nBytesPerPixel)];
										if (iRow) {
											upperLeft = pRowDst[(sl_int32)(i - nBytesPerPixel - sizeRow)];
										}
									}
									sl_uint8 up = 0;
									if (iRow) {
										up = pRowDst[(sl_int32)(i - sizeRow)];
									}
									base = PredictPath(left, up, upperLeft);
								}
								break;
						}
						pRowDst[i] = base + diff;
					}
				} else {
					Base::moveMemory(pRowDst, pRowSrc + 1, n);
				}
				pRowSrc += sizeRow + 1;
				pRowDst += sizeRow;
			}
			return sl_true;
		}

		static void PredictTIFF(sl_uint8* bufData, sl_uint32 sizeData, sl_uint32 colors, sl_uint32 bitsPerComponent, sl_uint32 columns)
		{
			sl_uint32 sizeRow = (colors * bitsPerComponent * columns + 7) >> 3;
			if (!sizeRow) {
				return;
			}
			sl_uint32 nFullRows = sizeData / sizeRow;
			sl_uint32 sizeRemain = sizeData % sizeRow;
			sl_uint32 nRows = sizeRemain ? nFullRows + 1 : nFullRows;
			if (!nRows) {
				return;
			}
			sl_uint8* row = bufData;
			for (sl_uint32 iRow = 0; iRow < nRows; iRow++) {
				if (bitsPerComponent == 1) {
					sl_uint32 n = (iRow == nFullRows ? sizeRemain : sizeRow) << 3;
					sl_uint8 bit = (*row >> 7) & 1;
					for (sl_uint32 i = 1; i < n; i++) {
						sl_uint32 x = i >> 3;
						sl_uint32 iBit = 7 - (i & 7);
						bit ^= ((row[x] >> iBit) & 1);
						if (bit) {
							row[x] |= 1 << iBit;
						} else {
							row[x] &= ~(1 << iBit);
						}
					}
				} else {
					sl_uint32 nBytesPerPixel = (bitsPerComponent * colors) >> 3;
					sl_uint32 n = iRow == nFullRows ? sizeRemain : sizeRow;
					if (bitsPerComponent == 16) {
						sl_uint16 pixel = MIO::readUint16BE(row);
						for (sl_uint32 i = nBytesPerPixel; i + 1 < n; i += 2) {
							pixel += MIO::readUint16BE(row + i);
							MIO::writeUint16BE(row + i, pixel);
						}
					} else {
						for (sl_uint32 i = nBytesPerPixel; i < n; i++) {
							row[i] += row[i - 1];
						}
					}
				}
				row += sizeRow;
			}
		}

	}

	sl_uint32 PdfFlateOrLZWDecodeParams::predict(void* content, sl_uint32 size) noexcept
	{
		if (predictor >= 10) {
			sl_uint32 c = columns ? columns : 1;
			if (PredictPNG((sl_uint8*)content, size, colors, bitsPerComponent, c)) {
				return size;
			}
		} else if (predictor == 2) {
			sl_uint32 c = columns ? columns : 1;
			PredictTIFF((sl_uint8*)content, size, colors, bitsPerComponent, c);
			return size;
		}
		return 0;
	}


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(PdfCCITTFaxDecodeParams)

	PdfCCITTFaxDecodeParams::PdfCCITTFaxDecodeParams(): K(0), columns(0), rows(0), flagEndOfLine(sl_false), flagByteAlign(sl_false), flagBlackIs1(sl_false)
	{
	}

	void PdfCCITTFaxDecodeParams::setParams(PdfDictionary* dict) noexcept
	{
		dict->get(name::K).getInt(K);
		dict->get(name::Columns).getUint(columns);
		dict->get(name::Rows).getUint(rows);
		dict->get(name::EndOfLine).getBoolean(flagEndOfLine);
		dict->get(name::EncodedByteAlign).getBoolean(flagByteAlign);
		dict->get(name::BlackIs1).getBoolean(flagBlackIs1);
	}


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(PdfColorSpace)

	PdfColorSpace::PdfColorSpace(): type(PdfColorSpaceType::Unknown)
	{
	}

	sl_bool PdfColorSpace::load(const PdfValue& value, PdfResourceProvider* res)
	{
		return _load(value, res, sl_false);
	}

	sl_bool PdfColorSpace::_load(const PdfValue& value, PdfResourceProvider* res, sl_bool flagICCBasedAlternate)
	{
		const String& name = value.getName();
		if (name.isNotNull()) {
			if (_loadName(name)) {
				return sl_true;
			}
			if (res) {
				return load(res->getResource(name::ColorSpace, name));
			}
			return sl_false;
		}
		const Ref<PdfArray>& array = value.getArray();
		if (array.isNotNull()) {
			return _loadArray(array.get(), flagICCBasedAlternate);
		}
		return sl_false;
	}

	sl_bool PdfColorSpace::_loadName(const String& v)
	{
		if (v == name::DeviceRGB || v == name::RGB) {
			type = PdfColorSpaceType::RGB;
		} else if (v == name::DeviceGray || v == name::G) {
			type = PdfColorSpaceType::Gray;
		} else if (v == name::DeviceCMYK || v == name::CMYK) {
			type = PdfColorSpaceType::CMYK;
		} else if (v == name::Pattern) {
			type = PdfColorSpaceType::Pattern;
		} else {
			return sl_false;
		}
		return sl_true;
	}

	sl_bool PdfColorSpace::_loadArray(PdfArray* arr, sl_bool flagICCBasedAlternate)
	{
		sl_uint32 n = arr->getCount();
		if (!n) {
			return sl_false;
		}
		const String& strType = arr->get(0).getName();
		if (strType == name::CalRGB) {
			type = PdfColorSpaceType::RGB;
			return sl_true;
		} else if (strType == name::CalGray) {
			type = PdfColorSpaceType::Gray;
			return sl_true;
		} else if (strType == name::CalCMYK) {
			type = PdfColorSpaceType::CMYK;
			return sl_true;
		} else if (strType == name::Lab) {
			type = PdfColorSpaceType::Lab;
			return sl_true;
		} else if (strType == name::Indexed || strType == name::I) {
			if (n >= 4) {
				if (_loadIndexed(arr->get(2).getUint(), arr->get(3))) {
					type = PdfColorSpaceType::Indexed;
					return sl_true;
				}
			}
		} else if (strType == name::ICCBased) {
			if (flagICCBasedAlternate) {
				return sl_false;
			}
			if (n >= 2) {
				Ref<PdfStream> stream = arr->get(1).getStream();
				if (stream.isNotNull()) {
					if (_load(stream->getProperty(name::Alternate), sl_null, sl_true)) {
						return sl_true;
					}
					sl_uint32 N = stream->getProperty(name::N).getUint();
					if (N == 1) {
						type = PdfColorSpaceType::Gray;
						return sl_true;
					} else if (N == 3) {
						type = PdfColorSpaceType::RGB;
						return sl_true;
					} else if (N == 4) {
						type = PdfColorSpaceType::CMYK;
						return sl_true;
					}
				}
			}
		} else if (strType == name::Pattern) {
			if (n >= 2) {
				return _loadName(arr->get(1).getString());
			}
		} else if (strType == name::Separation) {
			if (n >= 3) {
				return _loadName(arr->get(2).getString());
			}
		} else if (strType == name::DeviceN) {
			if (n >= 3) {
				return _loadName(arr->get(2).getString());
			}
		}
		return sl_false;
	}

	sl_bool PdfColorSpace::_loadIndexed(sl_uint32 maxIndex, const PdfValue& vTable)
	{
		if (!maxIndex) {
			return sl_false;
		}
		const String& strTable = vTable.getString();
		Memory memTable;
		sl_uint8* table;
		sl_size nTable;
		if (strTable.isNotNull()) {
			table = (sl_uint8*)(strTable.getData());
			nTable = strTable.getLength();
		} else {
			memTable = vTable.getDecodedStreamContent();
			table = (sl_uint8*)(memTable.getData());
			nTable = memTable.getSize();
		}
		if (nTable >= (maxIndex + 1) * 3) {
			indices = Array<Color>::create(maxIndex + 1);
			if (indices.isNotNull()) {
				Color* c = indices.getData();
				for (sl_uint32 i = 0; i <= maxIndex; i++) {
					c->r = *(table++);
					c->g = *(table++);
					c->b = *(table++);
					c->a = 255;
					c++;
				}
				return sl_true;
			}
		}
		return sl_false;
	}

	sl_uint32 PdfColorSpace::getComponentCount()
	{
		switch (type) {
			case PdfColorSpaceType::RGB:
			case PdfColorSpaceType::Lab:
				return 3;
			case PdfColorSpaceType::CMYK:
				return 4;
			case PdfColorSpaceType::Gray:
			case PdfColorSpaceType::Indexed:
				return 1;
			default:
				break;
		}
		return 0;
	}

	sl_bool PdfColorSpace::getColor(Color& _out, const PdfValue* values, sl_size count)
	{
		switch (type) {
			case PdfColorSpaceType::RGB:
				return getColorFromRGB(_out, values, count);
			case PdfColorSpaceType::Gray:
				return getColorFromGray(_out, values, count);
			case PdfColorSpaceType::CMYK:
				return getColorFromCMYK(_out, values, count);
			case PdfColorSpaceType::Lab:
				return getColorFromLab(_out, values, count);
			case PdfColorSpaceType::Indexed:
				if (count >= 1) {
					sl_uint32 index;
					if (values[0].getUint(index)) {
						return getColorAt(_out, index);
					}
				}
				break;
			default:
				break;
		}
		return sl_false;
	}

	sl_bool PdfColorSpace::getColorAt(Color& _out, sl_uint32 index)
	{
		if (index < indices.getCount()) {
			_out = indices[index];
			return sl_true;
		}
		return sl_false;
	}

	sl_bool PdfColorSpace::getColorFromRGB(Color& _out, const PdfValue* values, sl_size count)
	{
		if (count < 3) {
			return sl_false;
		}
		sl_uint8 r = (sl_uint8)(values[0].getFloat() * 255);
		sl_uint8 g = (sl_uint8)(values[1].getFloat() * 255);
		sl_uint8 b = (sl_uint8)(values[2].getFloat() * 255);
		_out = Color(r, g, b);
		return sl_true;
	}

	sl_bool PdfColorSpace::getColorFromGray(Color& _out, const PdfValue* values, sl_size count)
	{
		if (count < 1) {
			return sl_false;
		}
		sl_uint8 g = (sl_uint8)(values[0].getFloat() * 255);
		_out = Color(g, g, g);
		return sl_true;
	}

	sl_bool PdfColorSpace::getColorFromCMYK(Color& _out, const PdfValue* values, sl_size count)
	{
		if (count < 4) {
			return sl_false;
		}
		sl_uint8 c = (sl_uint8)(values[0].getFloat() * 255);
		sl_uint8 m = (sl_uint8)(values[1].getFloat() * 255);
		sl_uint8 y = (sl_uint8)(values[2].getFloat() * 255);
		sl_uint8 k = (sl_uint8)(values[3].getFloat() * 255);
		sl_uint8 r, g, b;
		CMYK::convertCMYKToRGB(c, m, y, k, r, g, b);
		_out = Color(r, g, b);
		return sl_true;
	}

	sl_bool PdfColorSpace::getColorFromLab(Color& _out, const PdfValue* values, sl_size count)
	{
		if (count < 3) {
			return sl_false;
		}
		float l = values[0].getFloat();
		float a = values[1].getFloat();
		float b = values[2].getFloat();
		Color3F c;
		CIE::convertLabToRGB(l, a, b, c.x, c.y, c.z);
		_out = c;
		return sl_true;
	}


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(PdfFontDescriptor)

	PdfFontDescriptor::PdfFontDescriptor(): ascent(0), descent(0), leading(0), weight(0), italicAngle(0), flags(0)
	{
	}

	void PdfFontDescriptor::load(PdfDictionary* desc) noexcept
	{
		name = desc->get(name::FontName).getName();
		family = desc->get(name::FontFamily).getString();
		ascent = desc->get(name::Ascent).getFloat();
		descent = desc->get(name::Descent).getFloat();
		leading = desc->get(name::Leading).getFloat();
		weight = desc->get(name::FontWeight).getFloat();
		italicAngle = desc->get(name::ItalicAngle).getFloat();
		flags = desc->get(name::Flags).getUint();
		content = desc->get(name::FontFile).getStream();
		if (content.isNull()) {
			content = desc->get(name::FontFile2).getStream();
			if (content.isNull()) {
				content = desc->get(name::FontFile3).getStream();
			}
		}
	}


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(PdfCidFontInfo)

	PdfCidFontInfo::PdfCidFontInfo(): subtype(PdfFontSubtype::Unknown), defaultWidth(1000), flagCidIsGid(sl_false)
	{
	}

	void PdfCidFontInfo::load(PdfDictionary* dict) noexcept
	{
		subtype = PdfFontResource::getSubtype(dict->get(name::Subtype).getName());
		dict->get(name::DW).getFloat(defaultWidth);
		Ref<PdfArray> w = dict->get(name::W).getArray();
		if (w.isNotNull()) {
			sl_uint32 index = 0;
			sl_uint32 nW = w->getCount();
			while (index < nW) {
				sl_uint32 code;
				if (!(w->get(index).getUint(code))) {
					break;
				}
				index++;
				if (index >= nW) {
					break;
				}
				Ref<PdfArray> m = w->get(index).getArray();
				if (m.isNotNull()) {
					index++;
					sl_uint32 nM = m->getCount();
					for (sl_uint32 i = 0; i < nM; i++) {
						float width;
						if (m->get(i).getFloat(width)) {
							widths.put_NoLock(code + i, width);
						} else {
							break;
						}
					}
				} else {
					sl_uint32 code2;
					if (!(w->get(index).getUint(code2))) {
						break;
					}
					index++;
					if (index >= nW) {
						break;
					}
					float width;
					if (w->get(index).getFloat(width)) {
						for (sl_uint32 i = code; i <= code2; i++) {
							widths.put_NoLock(i, width);
						}
					} else {
						break;
					}
					index++;
				}
			}
		}
		{
			PdfValue vCIDToGIDMap = dict->get(name::CIDToGIDMap);
			cidToGidMapName = vCIDToGIDMap.getName();
			if (cidToGidMapName == name::Identity) {
				flagCidIsGid = sl_true;
			}
		}
	}

	float PdfCidFontInfo::getWidth(sl_uint32 code)
	{
		float ret;
		if (widths.get_NoLock(code, &ret)) {
			return ret;
		}
		return defaultWidth;
	}


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(PdfFontResource)

	PdfFontResource::PdfFontResource(): subtype(PdfFontSubtype::Unknown), firstChar(0), lastChar(0), encoding(PdfEncoding::Standard), codeLength(1)
	{
	}

	namespace {

		static sl_bool DecodeCMapCode(const String& strCode, sl_uint16& outCode, sl_uint32& maxLenCode)
		{
			sl_size n = strCode.getLength();
			if (!n) {
				return sl_false;
			}
			sl_uint8* s = (sl_uint8*)(strCode.getData());
			if (n == 1) {
				outCode = *s;
				if (maxLenCode < 1) {
					maxLenCode = 1;
				}
				return sl_true;
			}
			if (n == 2) {
				outCode = SLIB_MAKE_WORD(*s, s[1]);
				if (maxLenCode < 2) {
					maxLenCode = 2;
				}
				return sl_true;
			}
			return sl_false;
		}

		static sl_bool DecodeCMapValue(const String& strValue, sl_uint32& outValue)
		{
			sl_size n = strValue.getLength();
			if (!n) {
				return sl_false;
			}
			sl_uint8* s = (sl_uint8*)(strValue.getData());
			if (n == 1) {
				outValue = *s;
				return sl_true;
			}
			if (n >= 4) {
				sl_char16 m[2] = { SLIB_MAKE_WORD(*s, s[1]), SLIB_MAKE_WORD(s[2], s[3]) };
				return Charsets::utf16ToUtf32(m, 2, (sl_char32*)&outValue, 1) == 1;
			} else {
				outValue = SLIB_MAKE_WORD(*s, s[1]);
				return sl_true;
			}
		}

		// returns maximum code length
		static sl_uint32 ParseCMap(const void* _content, sl_size size, HashMap<sl_uint16, sl_uint32>& map)
		{
			sl_uint8* content = Base::findMemory(_content, size, "begincmap", 9);
			if (!content) {
				return 0;
			}
			content += 9;
			size -= 9;
			MemoryContext context(sl_null);
			context.source = (sl_char8*)content;
			context.sizeSource = (sl_uint32)size;
			sl_uint32 maxLenCode = 0;
			CList<PdfValue> args;
			for (;;) {
				if (!(context.skipWhitespaces())) {
					break;
				}
				PdfCMapOperator op = context.readCMapOperator();
				if (op != PdfCMapOperator::Unknown) {
					switch (op) {
					case PdfCMapOperator::endbfchar:
						{
							PdfValue* m = args.getData();
							PdfValue* end = m + args.getCount();
							while (m + 1 < end) {
								sl_uint16 code;
								if (DecodeCMapCode((m++)->getString(), code, maxLenCode)) {
									sl_uint32 value;
									if (DecodeCMapValue((m++)->getString(), value)) {
										map.put_NoLock(code, value);
									} else {
										break;
									}
								} else {
									break;
								}
							}
						}
						break;
					case PdfCMapOperator::endbfrange:
						{
							PdfValue* m = args.getData();
							PdfValue* end = m + args.getCount();
							while (m + 2 < end) {
								sl_uint16 code1;
								if (DecodeCMapCode((m++)->getString(), code1, maxLenCode)) {
									sl_uint16 code2;
									if (DecodeCMapCode((m++)->getString(), code2, maxLenCode)) {
										if (code2 >= code1) {
											const String& strValue = m->getString();
											if (strValue.isNotNull()) {
												sl_uint32 value;
												if (DecodeCMapValue(strValue, value)) {
													for (sl_uint32 code = code1; code <= code2; code++) {
														map.put_NoLock((sl_uint16)code, value);
														value++;
													}
												} else {
													break;
												}
											} else {
												ListElements<PdfValue> arr(m->getElements());
												if (arr.count == code2 - code1 + 1) {
													for (sl_size i = 0; i < arr.count; i++) {
														sl_uint32 value;
														if (DecodeCMapValue(arr[i].getString(), value)) {

															map.put_NoLock((sl_uint16)(code1 + i), value);

														}
													}
												} else {
													break;
												}
											}
											m++;
										} else {
											break;
										}
									} else {
										break;
									}
								} else {
									break;
								}
							}
						}
						break;
					default:
						break;
					}
					args.removeAll_NoLock();
				} else {
					PdfValue value = context.readValue(PdfReference(0, 0));
					if (value.isUndefined()) {
						break;
					}
					args.add_NoLock(Move(value));
				}
			}
			return maxLenCode;
		}

	}

	sl_bool PdfFontResource::load(PdfDictionary* dict)
	{
		subtype = PdfFontResource::getSubtype(dict->get(name::Subtype).getName());
		baseFont = dict->get(name::BaseFont).getName();
		if (subtype == PdfFontSubtype::Type0) {
			Ref<PdfDictionary> cidFont = dict->get(name::DescendantFonts)[0].getDictionary();
			if (cidFont.isNull()) {
				return sl_false;
			}
			cid.load(cidFont.get());
			Ref<PdfDictionary> fd = cidFont->get(name::FontDescriptor).getDictionary();
			if (fd.isNotNull()) {
				descriptor.load(fd.get());
			}
		} else {
			Ref<PdfDictionary> fd = dict->get(name::FontDescriptor).getDictionary();
			if (fd.isNotNull()) {
				descriptor.load(fd.get());
			}
		}
		PdfValue vEncoding = dict->get(name::Encoding);
		const String& encodingName = vEncoding.getName();
		if (encodingName.isNotNull()) {
			encoding = Pdf::getEncoding(encodingName);
		} else {
			const Ref<PdfDictionary>& dictEncoding = vEncoding.getDictionary();
			if (dictEncoding.isNotNull()) {
				encoding = Pdf::getEncoding(dictEncoding->get(name::BaseEncoding).getString());
				Ref<PdfArray> diff = dictEncoding->get(name::Differences).getArray();
				if (diff.isNotNull()) {
					sl_uint32 n = diff->getCount();
					if (n >= 2) {
						sl_uint32 code;
						if (diff->get(0).getUint(code)) {
							for (sl_uint32 i = 1; i < n; i++) {
								PdfValue v = diff->get(i);
								if (!(v.getUint(code))) {
									const String& name = v.getName();
									if (name.isNotNull()) {
										encodingMap.put_NoLock(code, name);
										code++;
									} else {
										break;
									}
								}
							}
						}
					}
				}
			}
		}
		firstChar = dict->get(name::FirstChar).getUint();
		lastChar = dict->get(name::LastChar).getUint();
		Ref<PdfArray> arrWidths = dict->get(name::Widths).getArray();
		if (arrWidths.isNotNull()) {
			sl_uint32 n = arrWidths->getCount();
			if (n == lastChar - firstChar + 1) {
				widths = Array<float>::create(n);
				if (widths.isNotNull()) {
					float* f = widths.getData();
					for (sl_size i = 0; i < n; i++) {
						f[i] = arrWidths->get(i).getFloat();
					}
				}
			}
		}
		Memory memToUnicode = dict->get(name::ToUnicode).getDecodedStreamContent();
		if (memToUnicode.isNotNull()) {
			sl_uint32 n = ParseCMap(memToUnicode.getData(), memToUnicode.getSize(), toUnicode);
			if (n == 2) {
				codeLength = 2;
			}
		}
		if (encoding == PdfEncoding::IdentityH || encoding == PdfEncoding::IdentityV) {
			codeLength = 2;
		}
		return sl_true;
	}

	sl_char32 PdfFontResource::getUnicode(sl_uint32 charcode)
	{
		sl_uint32 ret;
		if (toUnicode.get_NoLock((sl_uint16)charcode, &ret)) {
			return ret;
		}
		if (charcode < 256) {
			const sl_char16* map = Pdf::getUnicodeTable(encoding);
			if (map) {
				return map[charcode];
			}
		}
		return charcode;
	}

	PdfFontSubtype PdfFontResource::getSubtype(const StringView& subtype) noexcept
	{
		if (subtype == StringView::literal("TrueType")) {
			return PdfFontSubtype::TrueType;
		}
		if (subtype == StringView::literal("Type1")) {
			return PdfFontSubtype::Type1;
		}
		if (subtype == StringView::literal("Type3")) {
			return PdfFontSubtype::Type3;
		}
		if (subtype == StringView::literal("Type0")) {
			return PdfFontSubtype::Type0;
		}
		if (subtype == StringView::literal("CIDFontType0")) {
			return PdfFontSubtype::CIDFontType0;
		}
		if (subtype == StringView::literal("CIDFontType2")) {
			return PdfFontSubtype::CIDFontType2;
		}
		if (subtype == StringView::literal("MMType1")) {
			return PdfFontSubtype::MMType1;
		}
		return PdfFontSubtype::Unknown;
	}


	SLIB_DEFINE_ROOT_OBJECT(PdfFont)

	PdfFont::PdfFont()
	{
		m_cacheGlyphs.setExpiringMilliseconds(EXPIRE_DURATION_FONT_GLYPH);
	}

	PdfFont::~PdfFont()
	{
	}

	Ref<PdfFont> PdfFont::load(PdfDictionary* dict)
	{
		Ref<PdfFont> ret = new PdfFont;
		if (ret.isNotNull()) {
			if (ret->_load(dict)) {
				return ret;
			}
		}
		return sl_null;
	}

	namespace {

		static const char* g_arrayBase14FontNames[14][10] = {
			{ "Courier", "CourierNew", "CourierNewPSMT", sl_null },
			{ "Courier-Bold", "CourierNew,Bold", "Courier,Bold", "CourierNewPS-BoldMT", "CourierNew-Bold", sl_null },
			{ "Courier-Oblique", "CourierNew,Italic", "Courier,Italic", "CourierNewPS-ItalicMT", "CourierNew-Italic", sl_null },
			{ "Courier-BoldOblique", "CourierNew,BoldItalic", "Courier,BoldItalic", "CourierNewPS-BoldItalicMT", "CourierNew-BoldItalic", sl_null },
			{ "Helvetica", "ArialMT", "Arial", sl_null },
			{ "Helvetica-Bold", "Arial-BoldMT", "Arial,Bold", "Arial-Bold", "Helvetica,Bold", sl_null },
			{ "Helvetica-Oblique", "Arial-ItalicMT", "Arial,Italic", "Arial-Italic", "Helvetica,Italic", "Helvetica-Italic", sl_null },
			{ "Helvetica-BoldOblique", "Arial-BoldItalicMT", "Arial,BoldItalic", "Arial-BoldItalic", "Helvetica,BoldItalic", "Helvetica-BoldItalic", sl_null },
			{ "Times-Roman", "TimesNewRomanPSMT", "TimesNewRoman", "TimesNewRomanPS", sl_null },
			{ "Times-Bold", "TimesNewRomanPS-BoldMT", "TimesNewRoman,Bold", "TimesNewRomanPS-Bold", "TimesNewRoman-Bold", sl_null },
			{ "Times-Italic", "TimesNewRomanPS-ItalicMT", "TimesNewRoman,Italic", "TimesNewRomanPS-Italic", "TimesNewRoman-Italic", sl_null },
			{ "Times-BoldItalic", "TimesNewRomanPS-BoldItalicMT", "TimesNewRoman,BoldItalic", "TimesNewRomanPS-BoldItalic", "TimesNewRoman-BoldItalic", sl_null },
			{ "Symbol", "Symbol,Italic", "Symbol,Bold", "Symbol,BoldItalic", "SymbolMT", "SymbolMT,Italic", "SymbolMT,Bold", "SymbolMT,BoldItalic", sl_null },
			{ "ZapfDingbats", sl_null }
		};

		struct FontMapping
		{
			const char* name;
			sl_bool flagBold;
			sl_bool flagItalic;
		};

		static FontMapping g_arrayBase14FontMappings[14][4] = {
			{ { "Courier", 0, 0}, { "Courier New", 0, 0 }, {0, 0, 0} },
			{ { "Courier-Bold", 0, 0 }, { "Courier New Bold", 0, 0 }, { "Courier New", 1, 0 }, { 0, 0, 0 } },
			{ { "Courier-BoldOblique", 0, 0 }, { "Courier New Bold Italic", 0, 0 }, { "Courier New", 1, 1 }, { 0, 0, 0 } },
			{ { "Courier-Oblique", 0, 0 }, { "Courier New Italic", 0, 0 }, { "Courier New", 0, 1 }, { 0, 0, 0 } },
			{ { "Helvetica", 0, 0}, { "Arial", 0, 0 }, {0, 0, 0} },
			{ { "Helvetica-Bold", 0, 0 }, { "Arial Bold", 0, 0 }, { "Arial", 1, 0 }, { 0, 0, 0 } },
			{ { "Helvetica-BoldOblique", 0, 0 }, { "Arial Bold Italic", 0, 0 }, { "Arial", 1, 1 }, { 0, 0, 0 } },
			{ { "Helvetica-Oblique", 0, 0 }, { "Arial Italic", 0, 0 }, { "Arial", 0, 1 }, { 0, 0, 0 } },
			{ { "Times-Roman", 0, 0}, { "Times New Roman", 0, 0 }, {0, 0, 0} },
			{ { "Times-Bold", 0, 0 }, { "Times New Roman Bold", 0, 0 }, { "Times New Roman", 1, 0 }, { 0, 0, 0 } },
			{ { "Times-BoldItalic", 0, 0 }, { "Times New Roman Bold Italic", 0, 0 }, { "Times New Roman", 1, 1 }, { 0, 0, 0 } },
			{ { "Times-Italic", 0, 0 }, { "Times New Roman Italic", 0, 0 }, { "Times New Roman", 0, 1 }, { 0, 0, 0 } },
			{ { "Symbol", 0, 0}, {0, 0, 0} },
			{ { "ZapfDingbats", 0, 0}, {0, 0, 0} }
		};

		class BaseFonts
		{
		private:
			CHashMap< String, sl_uint32, Hash_IgnoreCase<String>, Compare_IgnoreCase<String> > names;

		public:
			BaseFonts()
			{
				for (sl_uint32 i = 0; i < 14; i++) {
					const char** p = g_arrayBase14FontNames[i];
					while (*p) {
						names.put_NoLock(*p, i);
						p++;
					}
				}
			}

		public:
			Ref<FreeType> open(const String& name)
			{
				sl_uint32 index;
				if (names.get_NoLock(name, &index)) {
					FontMapping* mapping = g_arrayBase14FontMappings[index];
					while (mapping->name) {
						Ref<FreeType> font = FreeType::loadSystemFont(mapping->name, mapping->flagBold, mapping->flagItalic);
						if (font.isNotNull()) {
							return font;
						}
						mapping++;
					}
				}
				return sl_null;
			}

		};

		SLIB_SAFE_STATIC_GETTER(BaseFonts, GetBaseFonts)

		static Ref<FreeType> OpenBaseFont(const String& name)
		{
			BaseFonts* fonts = GetBaseFonts();
			if (fonts) {
				return fonts->open(name);
			}
			return sl_null;
		}

	}

	sl_bool PdfFont::_load(PdfDictionary* dict)
	{
		if (!(PdfFontResource::load(dict))) {
			return sl_false;
		}
		if (descriptor.content.isNotNull()) {
			Memory content = descriptor.content->getDecodedContent();
			if (content.isNotNull()) {
				face = FreeType::loadFromMemory(content);
			}
		} else {
			face = OpenBaseFont(baseFont);
			if (face.isNull()) {
				if (descriptor.family.isNotEmpty()) {
					face = FreeType::loadSystemFont(descriptor.family, descriptor.flags & PdfFontFlags::Bold, descriptor.flags & PdfFontFlags::Italic);
				}
			}
		}
		if (face.isNotNull()) {
			face->setSize(32);
			face->selectCharmap(descriptor.flags & PdfFontFlags::Symbolic);
			scale = 1.0f / 32.0f;
			return sl_true;
		}
		return sl_false;
	}

	sl_uint32 PdfFont::getGlyphIndex(sl_uint32 charcode, sl_char32 unicode)
	{
		if (cid.flagCidIsGid) {
			return charcode;
		}
		if (face.isNotNull()) {
			if (subtype == PdfFontSubtype::Type1) {
				{
					String name;
					if (encodingMap.get_NoLock(charcode, &name)) {
						sl_uint32 glyphId = face->getGlyphIndex(name.getData());
						if (glyphId) {
							return glyphId;
						}
					}
				}
				if (charcode < 256) {
					const char* const* names = Pdf::getCharNameTable(encoding);
					if (names) {
						const char* name = names[charcode];
						if (name) {
							sl_uint32 glyphId = face->getGlyphIndex(names[charcode]);
							if (glyphId) {
								return glyphId;
							}

						}
					}
				}
			}
			sl_uint32 glyphId;
			if (face->isUnicodeEncoding()) {
				glyphId = face->getGlyphIndex(unicode);
			} else {
				glyphId = face->getGlyphIndex(charcode);
			}
			if (glyphId) {
				return glyphId;
			}
		}
		return charcode;
	}

	Ref<FreeTypeGlyph> PdfFont::getGlyph(sl_uint32 charcode, sl_char32 unicode)
	{
		Ref<FreeTypeGlyph> glyph;
		if (m_cacheGlyphs.get(charcode, &glyph)) {
			return glyph;
		}
		sl_uint32 glyphId = getGlyphIndex(charcode, unicode);
		if (glyphId) {
			glyph = face->getGlyph(glyphId);
			m_cacheGlyphs.put(charcode, glyph);
			return glyph;
		}
		return sl_null;
	}

	float PdfFont::getCharWidth(sl_uint32 charcode, sl_char32 unicode)
	{
		if (subtype == PdfFontSubtype::Type0) {
			return cid.getWidth(charcode) / 1000.0f;
		}
		if (widths.isNotNull() && charcode >= firstChar && charcode <= lastChar) {
			return widths[charcode - firstChar] / 1000.0f;
		}
		if (face.isNotNull()) {
			Ref<FreeTypeGlyph> glyph = getGlyph(charcode, unicode);
			if (glyph.isNotNull()) {
				return glyph->advance * scale;
			}
		}
		return 0;
	}


	SLIB_DEFINE_ROOT_OBJECT(PdfExternalObject)

	PdfExternalObject::PdfExternalObject(PdfExternalObjectType _type): type(_type)
	{
	}

	PdfExternalObject::~PdfExternalObject()
	{
	}

	Ref<PdfExternalObject> PdfExternalObject::load(PdfStream* stream)
	{
		String subtype = stream->getProperty(name::Subtype).getName();
		if (subtype == name::Image) {
			return PdfImage::_load(stream);
		} else if (subtype == name::Form) {
			return PdfForm::_load(stream);
		} else {
			return sl_null;
		}
	}


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(PdfImageResource)

	PdfImageResource::PdfImageResource(): width(0), height(0), bitsPerComponent(8), flagImageMask(sl_false), flagInterpolate(sl_false), flagUseDecodeArray(sl_false), flagUseMatte(sl_false)
	{
	}

	sl_bool PdfImageResource::load(PdfStream* stream, PdfResourceProvider* resources) noexcept
	{
		String subtype = stream->getProperty(name::Subtype).getName();
		if (subtype == name::Image) {
			_load(stream, resources);
			return sl_true;
		}
		return sl_false;
	}

	void PdfImageResource::_load(PdfStream* stream, PdfResourceProvider* resources) noexcept
	{
		stream->getProperty(name::Width, name::W).getUint(width);
		stream->getProperty(name::Height, name::H).getUint(height);
		stream->getProperty(name::Interpolate, name::I).getBoolean(flagInterpolate);
		stream->getProperty(name::ImageMask, name::IM).getBoolean(flagImageMask);
		if (flagImageMask) {
			bitsPerComponent = 1;
			colorSpace.type = PdfColorSpaceType::Gray;
		} else {
			colorSpace.load(stream->getProperty(name::ColorSpace, name::CS), resources);
			stream->getProperty(name::BitsPerComponent, name::BPC).getUint(bitsPerComponent);
			mask = stream->getProperty(name::Mask);
		}
		Ref<PdfArray> arrayDecode = stream->getProperty(name::Decode, name::D).getArray();
		if (arrayDecode.isNotNull()) {
			switch (colorSpace.type) {
				case PdfColorSpaceType::RGB:
				case PdfColorSpaceType::Gray:
				case PdfColorSpaceType::CMYK:
					if (flagImageMask) {
						if (arrayDecode->getCount() >= 2) {
							flagUseDecodeArray = sl_true;
							decodeMin[0] = arrayDecode->get(0).getUint();
							decodeMax[0] = arrayDecode->get(1).getUint();
						}
					} else {
						sl_uint32 nColors = colorSpace.getComponentCount();
						if (arrayDecode->getCount() >= nColors * 2) {
							flagUseDecodeArray = sl_true;
							for (sl_uint32 i = 0; i < nColors; i++) {
								decodeMin[i] = Math::clamp0_255((sl_int32)(arrayDecode->get(i << 1).getFloat() * 255));
								decodeMax[i] = Math::clamp0_255((sl_int32)(arrayDecode->get((i << 1) | 1).getFloat() * 255));
							}
							if (nColors == 1) {
								decodeMin[2] = decodeMin[1] = decodeMin[0];
								decodeMax[2] = decodeMax[1] = decodeMax[0];
							}
						}
					}
					break;
				case PdfColorSpaceType::Indexed:
					if (arrayDecode->getCount() == 2) {
						sl_uint32 n = (sl_uint32)(colorSpace.indices.getCount());
						if (n) {
							sl_uint32 m0 = arrayDecode->get(0).getUint();
							sl_uint32 m1 = arrayDecode->get(1).getUint();
							Array<Color> newIndices = Array<Color>::create(n);
							if (newIndices.isNotNull()) {
								Color* d = newIndices.getData();
								Color* s = colorSpace.indices.getData();
								for (sl_uint32 i = 0; i < n; i++) {
									sl_uint32 m = m0 + (((m1 - m0) * i) >> bitsPerComponent);
									if (m >= n) {
										m = n - 1;
									}
									d[i] = s[m];
								}
								colorSpace.indices = Move(newIndices);
							}
						}
					}
					break;
				default:
					break;
			}
		}
		List<PdfValue> arrayMatte = stream->getProperty(name::Matte).getElements();
		if (arrayMatte.isNotNull()) {
			if (colorSpace.getColor(matte, arrayMatte.getData(), arrayMatte.getCount())) {
				if (matte != Color::Black) {
					flagUseMatte = sl_true;
				}
			}
		}
		smask = stream->getProperty(name::SMask).getStream();
	}
	
	SLIB_DEFINE_NESTED_CLASS_DEFAULT_MEMBERS(PdfPage, RenderParam)

	PdfPage::RenderParam::RenderParam(): canvas(sl_null)
	{
	}

	namespace {
		SLIB_INLINE static sl_uint8 ApplyDecode(sl_uint8 source, sl_uint8 min, sl_uint8 max)
		{
			return (sl_uint8)(min + (((sl_int32)(max - min) * (sl_int32)source) >> 8));
		}
	}

	void PdfImageResource::applyDecode4(sl_uint8* colors, sl_uint32 cols, sl_uint32 rows, sl_reg pitch) noexcept
	{
		if (!flagUseDecodeArray) {
			return;
		}
		for (sl_uint32 y = 0; y < rows; y++) {
			sl_uint8* c = colors;
			for (sl_uint32 x = 0; x < cols; x++) {
				c[0] = ApplyDecode(c[0], decodeMin[0], decodeMax[0]);
				c[1] = ApplyDecode(c[1], decodeMin[1], decodeMax[1]);
				c[2] = ApplyDecode(c[2], decodeMin[2], decodeMax[2]);
				c[3] = ApplyDecode(c[3], decodeMin[3], decodeMax[3]);
				c += 4;
			}
			colors += pitch;
		}
	}

	void PdfImageResource::applyDecode(Image* image) noexcept
	{
		if (!(flagImageMask || flagUseDecodeArray)) {
			return;
		}
		sl_uint32 cols = image->getWidth();
		sl_uint32 rows = image->getHeight();
		Color* colors = image->getColors();
		sl_reg stride = image->getStride();
		if (flagImageMask) {
			Color color0, color1;
			if (flagUseDecodeArray && decodeMin[0] == 1 && decodeMax[0] == 0) {
				color0 = Color::Transparent;
				color1 = Color::Black;
			} else {
				color0 = Color::Black;
				color1 = Color::Transparent;
			}
			for (sl_uint32 y = 0; y < rows; y++) {
				Color* c = colors;
				for (sl_uint32 x = 0; x < cols; x++) {
					if (c->r) {
						*c = color1;
					} else {
						*c = color0;
					}
					c++;
				}
				colors += stride;
			}
		} else {
			for (sl_uint32 y = 0; y < rows; y++) {
				Color* c = colors;
				for (sl_uint32 x = 0; x < cols; x++) {
					c->r = ApplyDecode(c->r, decodeMin[0], decodeMax[0]);
					c->g = ApplyDecode(c->g, decodeMin[1], decodeMax[1]);
					c->b = ApplyDecode(c->b, decodeMin[2], decodeMax[2]);
					c++;
				}
				colors += stride;
			}
		}
	}


	SLIB_DEFINE_OBJECT(PdfImage, PdfExternalObject)

	PdfImage::PdfImage(): PdfExternalObject(PdfExternalObjectType::Image)
	{
	}

	PdfImage::~PdfImage()
	{
	}

	Ref<PdfImage> PdfImage::load(PdfStream* stream, PdfResourceProvider* resources)
	{
		if (stream) {
			String subtype = stream->getProperty(name::Subtype).getName();
			if (subtype == name::Image) {
				return _load(stream, resources);
			}
		}
		return sl_null;
	}

	Ref<PdfImage> PdfImage::loadInline(PdfResourceProvider* resources, const void* data, sl_uint32& size)
	{
		MemoryContext context(sl_null);
		context.source = (sl_char8*)data;
		context.sizeSource = size;

		Ref<PdfDictionary> properties = new PdfDictionary(sl_null);
		if (properties.isNull()) {
			return sl_null;
		}
		for (;;) {
			if (!(context.skipWhitespaces())) {
				return sl_null;
			}
			sl_char8 ch;
			if (!(context.peekChar(ch))) {
				return sl_null;
			}
			if (ch == '/') {
				String name = context.readName();
				if (name.isNull()) {
					return sl_null;
				}
				if (!(context.skipWhitespaces())) {
					return sl_null;
				}
				PdfValue value = context.readValue(PdfReference(0, 0));
				if (value.isUndefined()) {
					return sl_null;
				}
				properties->add_NoLock(Move(name), Move(value));
			} else if (ch == 'I') {
				context.movePosition(1);
				if (!(context.readCharAndEquals('D'))) {
					return sl_null;
				}
				sl_char8 ch;
				if (!(context.peekChar(ch))) {
					return sl_null;
				}
				if (IsWhitespace(ch)) {
					context.movePosition(1);
				}
				break;
			} else {
				return sl_null;
			}
		}

		Ref<PdfImage> ret = new PdfImage;
		if (ret.isNotNull()) {
			sl_uint32 pos = context.getPosition();
			sl_uint32 len = size - pos;
			if (ret->_loadInline(resources, properties, (sl_uint8*)data + pos, len)) {
				size = pos + len;
				return ret;
			}
		}
		return sl_null;
	}

	Ref<PdfImage> PdfImage::_load(PdfStream* stream, PdfResourceProvider* resources)
	{
		Memory content = stream->getDecodedContent();
		if (content.isNotNull()) {
			Ref<PdfImage> ret = new PdfImage;
			if (ret.isNotNull()) {
				if (ret->_load(stream, resources, content)) {
					return ret;
				}
			}
		}
		return sl_null;
	}

	sl_bool PdfImage::_load(PdfStream* stream, PdfResourceProvider* resources, Memory& content)
	{
		PdfImageResource::_load(stream, resources);
		return _loadContent(content);
	}

	namespace {
		static Ref<Image> CreateImageObject(sl_uint32 width, sl_uint32 height, sl_uint8* data, sl_reg pitch, PdfColorSpaceType colorSpace, sl_uint32 bitsPerComponent, Color* indices, sl_uint32 nIndices)
		{
			if (width && height && bitsPerComponent) {
				switch (colorSpace) {
					case PdfColorSpaceType::RGB:
						return Image::createFromRGB(width, height, data, bitsPerComponent, pitch);
					case PdfColorSpaceType::CMYK:
						return Image::createFromCMYK(width, height, data, bitsPerComponent, pitch);
					case PdfColorSpaceType::Gray:
						return Image::createFromGray(width, height, data, bitsPerComponent, pitch);
					case PdfColorSpaceType::Indexed:
						if (indices) {
							return Image::createFromIndexed(width, height, data, indices, nIndices, bitsPerComponent, pitch);
						}
						break;
					default:
						break;
				}
			}
			return sl_null;
		}
	}

	sl_bool PdfImage::_loadContent(Memory& content)
	{
		CRef* ref = content.getRef();
		if (IsInstanceOf<Image>(ref)) {
			object = (Image*)ref;
		} else {
			if (colorSpace.type == PdfColorSpaceType::RGB || colorSpace.type == PdfColorSpaceType::Gray || colorSpace.type == PdfColorSpaceType::CMYK || colorSpace.type == PdfColorSpaceType::Indexed) {
				sl_uint32 nColors = colorSpace.getComponentCount();
				if (nColors) {
					sl_uint8* data = (sl_uint8*)(content.getData());
					sl_uint32 pitch = (nColors * bitsPerComponent * width + 7) >> 3;
					sl_uint32 heightFile = (sl_uint32)(content.getSize()) / pitch;
					if (height) {
						if (height > heightFile) {
							height = heightFile;
						}
					} else {
						height = heightFile;
					}
					if (height) {
						if (colorSpace.type == PdfColorSpaceType::CMYK) {
							if (!(content.getRef())) {
								content = content.duplicate();
								if (content.isNull()) {
									return sl_false;
								}
								data = (sl_uint8*)(content.getData());
							}
							applyDecode4(data, width, height, pitch);
						}
						object = CreateImageObject(width, height, data, pitch, colorSpace.type, bitsPerComponent, colorSpace.indices.getData(), (sl_uint32)(colorSpace.indices.getCount()));
					}
				}
			}
		}
		if (object.isNotNull()) {
			_restrictSize(object);
			if (colorSpace.type != PdfColorSpaceType::CMYK) {
				applyDecode(object.get());
			}
			_loadSMask();
			return sl_true;
		}
		return sl_false;
	}

	sl_bool PdfImage::_loadInline(PdfResourceProvider* resources, const Ref<PdfDictionary>& properties, sl_uint8* data, sl_uint32& size)
	{
		PdfStream stream(sl_null);
		stream.properties = properties;
		PdfImageResource::_load(&stream, resources);

		sl_uint32 posEnd = 0;
		for (;;) {
			sl_uint8* pt = Base::findMemory(data + posEnd, size - posEnd, "EI", 2);
			if (!pt) {
				return sl_false;
			}
			posEnd = (sl_uint32)(pt - data);
			if (!posEnd) {
				posEnd += 2;
				continue;
			}
			if (IsWhitespace(*(pt - 1))) {
				if (_loadInlineContent(&stream, data, posEnd - 1)) {
					size = posEnd + 2;
					return sl_true;
				}
			}
			if (_loadInlineContent(&stream, data, posEnd)) {
				size = posEnd + 2;
				return sl_true;
			}
			posEnd += 2;
		}
	}

	sl_bool PdfImage::_loadInlineContent(PdfStream* stream, sl_uint8* data, sl_uint32 size)
	{
		Memory content = stream->getDecodedContent(Memory::createStatic(data, size));
		if (content.isNull()) {
			return sl_false;
		}
		return _loadContent(content);
	}

	void PdfImage::_loadSMask()
	{
		if (object.isNull()) {
			return;
		}
		if (smask.isNull()) {
			return;
		}
		PdfImageResource maskDesc;
		if (!(maskDesc.load(smask.get()))) {
			return;
		}
		sl_uint32 widthMask = maskDesc.width;
		if (!(widthMask && maskDesc.bitsPerComponent && maskDesc.colorSpace.type == PdfColorSpaceType::Gray)) {
			return;
		}
		Memory content = smask->getDecodedContent();
		if (content.isNull()) {
			return;
		}
		sl_uint32 pitchMask = (maskDesc.bitsPerComponent * widthMask + 7) >> 3;
		sl_uint32 heightMask = maskDesc.height;
		sl_uint32 heightMaskFile = (sl_uint32)(content.getSize()) / pitchMask;
		if (heightMask) {
			if (heightMask > heightMaskFile) {
				heightMask = heightMaskFile;
			}
		} else {
			heightMask = heightMaskFile;
		}
		if (!heightMask) {
			return;
		}
		if (widthMask <= MAX_IMAGE_WIDTH && heightMask <= MAX_IMAGE_HEIGHT) {
			_growSize(object, widthMask, heightMask);
		}
		sl_uint32 widthParent = object->getWidth();
		sl_uint32 heightParent = object->getHeight();
		if (widthMask == widthParent && heightMask == heightParent && !(maskDesc.flagUseMatte) && !(maskDesc.flagUseDecodeArray)) {
			object->multiplyAlphaFromGray(widthMask, heightMask, content.getData(), maskDesc.bitsPerComponent, pitchMask);
			return;
		}
		Ref<Image> imageMask = Image::createFromGray(widthMask, heightMask, content.getData(), maskDesc.bitsPerComponent, pitchMask);
		if (imageMask.isNull()) {
			return;
		}
		if (widthMask != widthParent || heightMask != heightParent) {
			imageMask = imageMask->stretch(widthParent, heightParent, StretchMode::Linear);
			if (imageMask.isNull()) {
				return;
			}
		}
		maskDesc.applyDecode(imageMask.get());
		Color* rowDst = object->getColors();
		Color* rowSrc = imageMask->getColors();
		sl_reg strideDst = object->getStride();
		sl_reg strideSrc = imageMask->getStride();
		if (maskDesc.flagUseMatte) {
			for (sl_uint32 y = 0; y < heightParent; y++) {
				Color* dst = rowDst;
				Color* src = rowSrc;
				for (sl_uint32 x = 0; x < widthParent; x++) {
					Color c = matte;
					c.blend_PA_NPA(dst->r, dst->g, dst->b, src->r);
					c.convertPAtoNPA();
					c.a = src->r;
					*dst = c;
					dst++;
					src++;
				}
				rowDst += strideDst;
				rowSrc += strideSrc;
			}
		} else {
			for (sl_uint32 y = 0; y < heightParent; y++) {
				Color* dst = rowDst;
				Color* src = rowSrc;
				for (sl_uint32 x = 0; x < widthParent; x++) {
					dst->a = (sl_uint8)(((sl_uint32)(dst->a) * (sl_uint32)(src->r)) >> 8);
					dst++;
					src++;
				}
				rowDst += strideDst;
				rowSrc += strideSrc;
			}
		}
	}

	void PdfImage::_growSize(Ref<Image>& image, sl_uint32 minWidth, sl_uint32 minHeight)
	{
		sl_uint32 width = image->getWidth();
		sl_uint32 height = image->getHeight();
		if (width < minWidth || height < minHeight) {
			if (width < minWidth) {
				width = minWidth;
			}
			if (height < minHeight) {
				height = minHeight;
			}
			Ref<Image> imageNew = image->stretch(width, height, StretchMode::Linear);
			if (imageNew.isNotNull()) {
				image = Move(imageNew);
			}
		}
	}

	void PdfImage::_restrictSize(Ref<Image>& image)
	{
		sl_uint32 width = image->getWidth();
		sl_uint32 height = image->getHeight();
		if (width > MAX_IMAGE_WIDTH || height > MAX_IMAGE_HEIGHT) {
			if (width > MAX_IMAGE_WIDTH) {
				width = MAX_IMAGE_WIDTH;
			}
			if (height > MAX_IMAGE_HEIGHT) {
				height = MAX_IMAGE_HEIGHT;
			}
			Ref<Image> imageNew = image->stretch(width, height, StretchMode::Linear);
			if (imageNew.isNotNull()) {
				image = Move(imageNew);
			}
		}
	}


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(PdfFormResource)

	PdfFormResource::PdfFormResource()
	{
	}

	sl_bool PdfFormResource::load(PdfStream* stream)
	{
		String subtype = stream->getProperty(name::Subtype).getName();
		if (subtype == name::Form) {
			_load(stream);
			return sl_true;
		}
		return sl_false;
	}

	void PdfFormResource::_load(PdfStream* stream)
	{
		bounds = stream->getProperty(name::BBox).getRectangle();
		if (!(stream->getProperty(name::Matrix).getMatrix(matrix))) {
			matrix = Matrix3::identity();
		}
		resources = stream->getProperty(name::Resources).getDictionary();
	}


	SLIB_DEFINE_OBJECT(PdfForm, PdfExternalObject)

	PdfForm::PdfForm(): PdfExternalObject(PdfExternalObjectType::Form)
	{
	}

	PdfForm::~PdfForm()
	{
	}

	Ref<PdfForm> PdfForm::load(PdfStream* stream)
	{
		if (stream) {
			String subtype = stream->getProperty(name::Subtype).getName();
			if (subtype == name::Form) {
				return _load(stream);
			}
		}
		return sl_null;
	}

	Ref<PdfForm> PdfForm::_load(PdfStream* stream)
	{
		Memory content = stream->getDecodedContent();
		if (content.isNotNull()) {
			Ref<PdfForm> ret = new PdfForm;
			if (ret.isNotNull()) {
				if (ret->_load(stream, content)) {
					return ret;
				}
			}
		}
		return sl_null;
	}

	sl_bool PdfForm::_load(PdfStream* stream, const MemoryView& contentData)
	{
		PdfFormResource::_load(stream);
		if (contentData.size) {
			content = PdfPage::parseContent(this, contentData.data, contentData.size);
		}
		return content.isNotEmpty();
	}

	PdfValue PdfForm::getResources(const String& type, sl_bool flagResolveReference)
	{
		if (resources.isNotNull()) {
			return resources->get(type, flagResolveReference);
		} else {
			return PdfValue();
		}
	}

	PdfValue PdfForm::getResource(const String& type, const String& name, sl_bool flagResolveReference)
	{
		if (resources.isNotNull()) {
			PdfValue values = resources->get(type);
			if (values.isUndefined()) {
				return PdfValue();
			}
			return values.getItem(name, flagResolveReference);
		} else {
			return PdfValue();
		}
	}


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(PdfShadingResource)

	PdfShadingResource::PdfShadingResource(): type(PdfShadingType::Unknown)
	{
	}

	sl_bool PdfShadingResource::load(PdfDictionary* dict)
	{
		PdfShadingType _type = (PdfShadingType)(dict->get(name::ShadingType).getUint());
		colorSpace.load(dict->get(name::ColorSpace));
		if (colorSpace.type == PdfColorSpaceType::Unknown || colorSpace.type == PdfColorSpaceType::Pattern) {
			return sl_false;
		}
		if (_type == PdfShadingType::Axial || _type == PdfShadingType::Radial) {
			Ref<PdfArray> arrCoords = dict->get(name::Coords).getArray();
			if (arrCoords.isNull()) {
				return sl_false;
			}
			if (_type == PdfShadingType::Radial) {
				if (arrCoords->getCount() == 6) {
					coordsStart.x = arrCoords->get(0).getFloat();
					coordsStart.y = arrCoords->get(1).getFloat();
					radiusStart = arrCoords->get(2).getFloat();
					coordsEnd.x = arrCoords->get(3).getFloat();
					coordsEnd.y = arrCoords->get(4).getFloat();
					radiusEnd = arrCoords->get(5).getFloat();
				} else {
					return sl_false;
				}
			} else {
				if (arrCoords->getCount() == 4) {
					coordsStart.x = arrCoords->get(0).getFloat();
					coordsStart.y = arrCoords->get(1).getFloat();
					coordsEnd.x = arrCoords->get(2).getFloat();
					coordsEnd.y = arrCoords->get(3).getFloat();
				} else {
					return sl_false;
				}
			}
			Ref<PdfArray> arrDomain = dict->get(name::Domain).getArray();
			if (arrDomain.isNotNull()) {
				if (arrDomain->getCount() != 2) {
					return sl_false;
				}
				domainStart = arrDomain->get(0).getFloat();
				domainEnd = arrDomain->get(1).getFloat();
			} else {
				domainStart = 0;
				domainEnd = 1;
			}
			PdfValue vFunction = dict->get(name::Function);
			const Ref<PdfArray>& arrFunctions = vFunction.getArray();
			if (arrFunctions.isNotNull()) {
				sl_uint32 n = (sl_uint32)(arrFunctions->getCount());
				if (!n) {
					return sl_false;
				}
				if (n != colorSpace.getComponentCount()) {
					return sl_false;
				}
				functions = Array<PdfFunction>::create(n);
				if (functions.isNull()) {
					return sl_false;
				}
				for (sl_uint32 i = 0; i < n; i++) {
					if (!(functions[i].load(arrFunctions->get(i)))) {
						return sl_false;
					}
					if (functions[i].countInput != 1 || functions[i].countOutput != 1) {
						return sl_false;
					}
				}
			} else {
				if (!(function.load(vFunction))) {
					return sl_false;
				}
				if (function.countInput != 1 || function.countOutput != colorSpace.getComponentCount()) {
					return sl_false;
				}
			}
			type = _type;
		} else {
			return sl_false;
		}
		return sl_true;
	}

	sl_bool PdfShadingResource::getColor(float t, Color& _out)
	{
		sl_uint32 n = colorSpace.getComponentCount();
		if (n > 4) {
			n = 4;
		}
		PdfValue c[4];
		if (functions.isNotNull()) {
			for (sl_uint32 i = 0; i < n; i++) {
				float f = 0;
				if (functions[i].call(&t, 1, &f, 1)) {
					c[i] = f;
				} else {
					return sl_false;
				}
			}
		} else {
			float f[4] = { 0 };
			if (function.call(&t, 1, f, n)) {
				for (sl_uint32 i = 0; i < n; i++) {
					c[i] = f[i];
				}
			} else {
				return sl_false;
			}
		}
		return colorSpace.getColor(_out, c, n);
	}

	Ref<Brush> PdfShadingResource::getBrush(const Matrix3& transform)
	{
		if (type == PdfShadingType::Axial || type == PdfShadingType::Radial) {
			Array<Color> c;
			Array<sl_real> loc;
			if (function.type == PdfFunctionType::Stiching) {
				sl_uint32 n = (sl_uint32)(function.bounds.getCount());
				c = Array<Color>::create(n + 2);
				if (c.isNull()) {
					return sl_null;
				}
				loc = Array<sl_real>::create(n + 2);
				if (loc.isNull()) {
					return sl_null;
				}
				for (sl_uint32 i = 0; i < n; i++) {
					float f = function.bounds[i];
					loc[i + 1] = f;
					if (!(getColor(f, c[i + 1]))) {
						return sl_null;
					}
				}
				loc[0] = domainStart;
				if (!(getColor(domainStart, c[0]))) {
					return sl_null;
				}
				loc[n + 1] = domainEnd;
				if (!(getColor(domainEnd, c[n + 1]))) {
					return sl_null;
				}
			} else {
				sl_uint32 n = 16;
				c = Array<Color>::create(n);
				if (c.isNull()) {
					return sl_null;
				}
				loc = Array<sl_real>::create(n);
				if (loc.isNull()) {
					return sl_null;
				}
				for (sl_uint32 i = 0; i < n; i++) {
					float f = domainStart + (float)i * (domainEnd - domainStart) / (float)(n - 1);
					loc[i] = f;
					if (!(getColor(f, c[i]))) {
						return sl_null;
					}
				}
			}
			Point p0 = transform.transformPosition(coordsStart);
			Point p1 = transform.transformPosition(coordsEnd);
			if (type == PdfShadingType::Axial) {
				return Brush::createLinearGradientBrush(p0, p1, (sl_uint32)(c.getCount()), c.getData(), loc.getData());
			} else {
				float r = transform.transformDirection(0, radiusEnd).getLength();
				return Brush::createRadialGradientBrush((p0 + p1) / 2.0f, r, (sl_uint32)(c.getCount()), c.getData(), loc.getData());
			}
		}
		return sl_null;
	}


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(PdfPatternResource)

	PdfPatternResource::PdfPatternResource()
	{
	}

	sl_bool PdfPatternResource::load(PdfDictionary* dict)
	{
		PdfPatternType _type = (PdfPatternType)(dict->get(name::PatternType).getUint());
		if (_type == PdfPatternType::Shading) {
			Ref<PdfDictionary> dictShading = dict->get(name::Shading).getDictionary();
			if (dictShading.isNull()) {
				return sl_false;
			}
			if (!(shading.load(dictShading.get()))) {
				return sl_false;
			}
			if (!(dict->get(name::Matrix).getMatrix(matrix))) {
				matrix = Matrix3::identity();
			}
			type = _type;
		} else {
			return sl_false;
		}
		return sl_true;
	}


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(PdfOperation)

	PdfOperation::PdfOperation() noexcept
	{
	}

	PdfOperator PdfOperation::getOperator(const StringView& opName) noexcept
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

	PdfCMapOperator PdfOperation::getCMapOperator(const StringView& opName) noexcept
	{
		sl_char8* s = opName.getData();
		sl_size len = opName.getLength();
		if (len < 3) {
			return PdfCMapOperator::Unknown;
		}
		sl_char8 s0 = s[0];
		if (s0 == 'b') {
			if (len >= 5) {
				if (s[1] == 'e' && s[2] == 'g' && s[3] == 'i' && s[4] == 'n') {
					if (len == 11) {
						if (Base::equalsMemory(s + 5, "bfchar", 6)) {
							return PdfCMapOperator::beginbfchar;
						}
					} else if (len == 12) {
						if (Base::equalsMemory(s + 5, "bfrange", 7)) {
							return PdfCMapOperator::beginbfrange;
						}
					} else if (len == 19) {
						if (Base::equalsMemory(s + 5, "codespacerange", 14)) {
							return PdfCMapOperator::begincodespacerange;
						}
					}
				}
			}
		} else if (s0 == 'd') {
			if (len == 3) {
				if (s[1] == 'e' && s[2] == 'f') {
					return PdfCMapOperator::def;
				}
			}
		} else if (s0 == 'e') {
			if (s[1] == 'n' && s[2] == 'd') {
				if (len == 9) {
					if (Base::equalsMemory(s + 3, "bfchar", 6)) {
						return PdfCMapOperator::endbfchar;
					}
				} else if (len == 10) {
					if (Base::equalsMemory(s + 3, "bfrange", 7)) {
						return PdfCMapOperator::endbfrange;
					}
				} else if (len == 17) {
					if (Base::equalsMemory(s + 3, "codespacerange", 14)) {
						return PdfCMapOperator::endcodespacerange;
					}
				}
			}
		}
		return PdfCMapOperator::Unknown;
	}


	PdfPageTreeItem::PdfPageTreeItem() noexcept: m_flagPage(sl_false), reference(0, 0)
	{
	}

	PdfPageTreeItem::~PdfPageTreeItem()
	{
	}

	sl_bool PdfPageTreeItem::isPage()
	{
		return m_flagPage;
	}

	PdfValue PdfPageTreeItem::getAttribute(const String& name)
	{
		PdfValue ret = attributes->get(name);
		if (ret.isNotUndefined()) {
			return ret;
		}
		Ref<PdfPageTreeItem> _parent(parent);
		if (_parent.isNotNull()) {
			return _parent->getAttribute(name);
		}
		return PdfValue();
	}


	PdfResourceCache::PdfResourceCache(): flagUseFontsCache(sl_true), flagUseExternalObjectsCache(sl_true)
	{
	}

	PdfResourceCache::~PdfResourceCache()
	{
	}


	SLIB_DEFINE_OBJECT(PdfPage, PdfPageTreeItem)

	PdfPage::PdfPage(CRef* context) noexcept: m_context(context)
	{
		m_flagPage = sl_true;
		m_flagContent = sl_false;
	}

	PdfPage::~PdfPage()
	{
	}

	Memory PdfPage::getContentData()
	{
		PdfValue contents = attributes->get(name::Contents);
		const Ref<PdfArray>& arrContents = contents.getArray();
		if (arrContents.isNotNull()) {
			MemoryBuffer buf;
			sl_uint32 nItems = arrContents->getCount();
			for (sl_uint32 i = 0; i < nItems; i++) {
				buf.add(arrContents->get(i).getDecodedStreamContent());
			}
			return buf.merge();
		} else {
			return contents.getDecodedStreamContent();
		}
		return sl_null;
	}

	List<PdfOperation> PdfPage::getContent()
	{
		if (m_flagContent) {
			return m_content;
		}
		Memory data = getContentData();
		if (data.isNotNull()) {
			List<PdfOperation> ret = parseContent(this, (sl_char8*)(data.getData()), data.getSize());
			if (ret.isNotNull()) {
				m_content = ret;
				m_flagContent = sl_true;
				return ret;
			}
		}
		m_flagContent = sl_true;
		return sl_null;
	}

	List<PdfOperation> PdfPage::parseContent(PdfResourceProvider* resources, const void* data, sl_size size)
	{
		List<PdfOperation> ret;
		MemoryContext context(sl_null);
		context.source = (sl_char8*)data;
		context.sizeSource = (sl_uint32)size;
		PdfOperation opCurrent;
		for (;;) {
			if (!(context.skipWhitespaces())) {
				break;
			}
			PdfOperator op = context.readOperator();
			if (op != PdfOperator::Unknown) {
				opCurrent.op = op;
				if (op == PdfOperator::BI) {
					sl_uint32 pos = context.getPosition();
					sl_uint32 len = (sl_uint32)size - pos;
					Ref<PdfImage> image = PdfImage::loadInline(resources, context.source + pos, len);
					if (image.isNull()) {
						break;
					}
					context.setPosition(pos + len);
					opCurrent.operands.add_NoLock(Move(image));
				}
				ret.add_NoLock(Move(opCurrent));
				opCurrent.operands.setNull();
			} else {
				PdfValue value = context.readValue(PdfReference(0, 0));
				if (value.isUndefined()) {
					break;
				}
				opCurrent.operands.add_NoLock(Move(value));
			}
		}
		return ret;
	}

	namespace {

		class TextState
		{
		public:
			float charSpace = 0;
			float wordSpace = 0;
			float widthScale = 1;
			float leading = 0;
			float rise = 0;
			PdfTextRenderingMode renderingMode = PdfTextRenderingMode::Fill;
			Ref<PdfFont> font;
			float fontScale;
			Matrix3 matrix = Matrix3::identity();
			Matrix3 lineMatrix = Matrix3::identity();

		};

		class PenState : public PenDesc
		{
		public:
			const Ref<Pen>& getHandle()
			{
				if (m_handle.isNull() || m_flagInvalidate) {
					m_handle = Pen::create(*this);
					m_flagInvalidate = sl_false;
				}
				return m_handle;
			}

			void invalidate()
			{
				m_flagInvalidate = sl_true;
			}

		private:
			Ref<Pen> m_handle;
			sl_bool m_flagInvalidate = sl_true;

		};

		class BrushState : public BrushDesc
		{
		public:
			BrushState()
			{
				color = Color::Black;
			}

		public:
			const Ref<Brush>& getHandle()
			{
				if (m_handle.isNull() || m_flagInvalidate) {
					m_handle = Brush::create(*this);
					m_flagInvalidate = sl_false;
				}
				return m_handle;
			}

			void invalidate()
			{
				m_flagInvalidate = sl_true;
			}

			void setHandle(const Ref<Brush>& handle)
			{
				m_handle = handle;
				m_flagInvalidate = sl_false;
			}

		private:
			Ref<Brush> m_handle;
			sl_bool m_flagInvalidate = sl_true;

		};

		template <class HANDLE_STATE, class VALUE>
		static void SetHandleState(HANDLE_STATE& state, VALUE& dst, const VALUE& src)
		{
			if (dst != src) {
				dst = src;
				state.invalidate();
			}
		}

#define SET_HANDLE_STATE(STATE, NAME, VALUE) SetHandleState(STATE, STATE.NAME, VALUE)

		class RenderState
		{
		public:
			PdfColorSpace colorSpaceForStroking;
			PdfColorSpace colorSpaceForNonStroking;
			BrushState brush;
			PenState pen;
			TextState text;
		};

		class Renderer : public RenderState
		{
		public:
			Context* context;
			Canvas* canvas;
			PdfResourceCache* cache;
			PdfResourceProvider* resources;

			Ref<GraphicsPath> path;

			Stack<RenderState> states;

		public:
			sl_bool preparePath()
			{
				if (path.isNull()) {
					path = GraphicsPath::create();
					return path.isNotNull();
				}
				return sl_true;
			}

			void moveTo(ListElements<PdfValue> operands)
			{
				if (operands.count != 2) {
					return;
				}
				if (!(preparePath())) {
					return;
				}
				path->moveTo(operands[0].getFloat(), operands[1].getFloat());
			}

			void lineTo(ListElements<PdfValue> operands)
			{
				if (operands.count != 2) {
					return;
				}
				if (!(preparePath())) {
					return;
				}
				path->lineTo(operands[0].getFloat(), operands[1].getFloat());
			}

			void curveTo(ListElements<PdfValue> operands, sl_bool flagReplicateInitialPoint, sl_bool flagReplicateFinalPoint)
			{
				if (!(preparePath())) {
					return;
				}
				if (flagReplicateInitialPoint || flagReplicateFinalPoint) {
					if (operands.count != 4) {
						return;
					}
					if (flagReplicateInitialPoint) {
						sl_size nPoints = path->getPointCount();
						if (!nPoints) {
							return;
						}
						GraphicsPathPoint& ptCurrent = (path->getPoints())[nPoints - 1];
						path->cubicTo(
							ptCurrent.x, ptCurrent.y,
							operands[0].getFloat(), operands[1].getFloat(),
							operands[2].getFloat(), operands[3].getFloat());
					} else {
						float lastX = operands[2].getFloat();
						float lastY = operands[3].getFloat();
						path->cubicTo(
							operands[0].getFloat(), operands[1].getFloat(),
							lastX, lastY, lastX, lastY);
					}
				} else {
					if (operands.count != 6) {
						return;
					}
					path->cubicTo(operands[0].getFloat(), operands[1].getFloat(),
						operands[2].getFloat(), operands[3].getFloat(),
						operands[4].getFloat(), operands[5].getFloat());
				}
			}

			void appendRect(ListElements<PdfValue> operands)
			{
				if (operands.count != 4) {
					return;
				}
				if (!(preparePath())) {
					return;
				}
				path->addRectangle(operands[0].getFloat(), operands[1].getFloat(),
					operands[2].getFloat(), operands[3].getFloat());
			}

			void closePath()
			{
				if (path.isNotNull()) {
					path->closeSubpath();
				}
			}

			void clearPath()
			{
				path.setNull();
			}

			void setColor(const Color& color, sl_bool flagStroking)
			{
				if (flagStroking) {
					SET_HANDLE_STATE(pen, color, color);
				} else {
					SET_HANDLE_STATE(brush, color, color);
				}
			}

			void setColorSpace(ListElements<PdfValue> operands, sl_bool flagStroking)
			{
				if (operands.count != 1) {
					return;
				}
				PdfColorSpace cs;
				cs.load(operands[0], resources);
				if (flagStroking) {
					colorSpaceForStroking = cs;
				} else {
					colorSpaceForNonStroking = cs;
				}
				if (cs.type == PdfColorSpaceType::Indexed) {
					Color c;
					if (cs.getColorAt(c, 0)) {
						setColor(c, flagStroking);
					} else {
						setColor(Color::Black, flagStroking);
					}
				} else {
					setColor(Color::Black, flagStroking);
				}
			}

			void setColor(ListElements<PdfValue> operands, sl_bool flagStroking)
			{
				if (!(operands.count)) {
					return;
				}
				PdfColorSpace& cs = flagStroking ? colorSpaceForStroking : colorSpaceForNonStroking;
				if (cs.type == PdfColorSpaceType::Pattern) {
					const String& patternName = operands[0].getName();
					if (patternName.isNotNull()) {
						Ref<PdfDictionary> dictPattern = resources->getResource(name::Pattern, patternName).getDictionary();
						if (dictPattern.isNotNull()) {
							PdfPatternResource res;
							if (res.load(dictPattern.get())) {
								if (res.type == PdfPatternType::Shading) {
									Ref<Brush> handle = res.shading.getBrush(res.matrix);
									if (handle.isNotNull()) {
										brush.setHandle(handle);
									}
								}
							}
						}
					}
				} else {
					Color color;
					if (cs.getColor(color, operands.data, operands.count)) {
						setColor(color, flagStroking);
					}
				}
			}

			void setRGB(ListElements<PdfValue> operands, sl_bool flagStroking)
			{
				Color color;
				if (PdfColorSpace::getColorFromRGB(color, operands.data, operands.count)) {
					setColor(color, flagStroking);
				}
			}

			void setGrayLevel(ListElements<PdfValue> operands, sl_bool flagStroking)
			{
				Color color;
				if (PdfColorSpace::getColorFromGray(color, operands.data, operands.count)) {
					setColor(color, flagStroking);
				}
			}

			void setCMYK(ListElements<PdfValue> operands, sl_bool flagStroking)
			{
				Color color;
				if (PdfColorSpace::getColorFromCMYK(color, operands.data, operands.count)) {
					setColor(color, flagStroking);
				}
			}

			void setLineWidth(ListElements<PdfValue> operands)
			{
				if (operands.count != 1) {
					return;
				}
				SET_HANDLE_STATE(pen, width, operands[0].getFloat());
			}

			void setLineJoin(ListElements<PdfValue> operands)
			{
				if (operands.count != 1) {
					return;
				}
				SET_HANDLE_STATE(pen, join, (LineJoin)(operands[0].getUint()));
			}

			void setLineCap(ListElements<PdfValue> operands)
			{
				if (operands.count != 1) {
					return;
				}
				SET_HANDLE_STATE(pen, cap, (LineCap)(operands[0].getUint()));
			}

			void setLineDashPattern(ListElements<PdfValue> operands)
			{
				if (operands.count != 2) {
					return;
				}
				if (operands[0].getElementCount()) {
					SET_HANDLE_STATE(pen, style, PenStyle::Dash);
				} else {
					SET_HANDLE_STATE(pen, style, PenStyle::Solid);
				}
			}

			void setMiterLimit(ListElements<PdfValue> operands)
			{
				if (operands.count != 1) {
					return;
				}
				SET_HANDLE_STATE(pen, miterLimit, operands[0].getFloat());
			}

			void setGraphicsState(ListElements<PdfValue> operands)
			{
				if (operands.count != 1) {
					return;
				}
				SLIB_STATIC_STRING(idExtGState, "ExtGState")
				Ref<PdfDictionary> states = resources->getResource(idExtGState, operands[0].getName()).getDictionary();
				if (states.isNull()) {
					return;
				}
				{
					SLIB_STATIC_STRING(fieldId, "LW")
					float value;
					if (states->get(fieldId).getFloat(value)) {
						SET_HANDLE_STATE(pen, width, value);
					}
				}
				{
					SLIB_STATIC_STRING(fieldId, "LC")
					sl_uint32 value;
					if (states->get(fieldId).getUint(value)) {
						SET_HANDLE_STATE(pen, cap, (LineCap)value);
					}
				}
				{
					SLIB_STATIC_STRING(fieldId, "LJ")
					sl_uint32 value;
					if (states->get(fieldId).getUint(value)) {
						SET_HANDLE_STATE(pen, join, (LineJoin)value);
					}
				}
				{
					SLIB_STATIC_STRING(fieldId, "ML")
					float value;
					if (states->get(fieldId).getFloat(value)) {
						SET_HANDLE_STATE(pen, miterLimit, value);
					}
				}
				{
					SLIB_STATIC_STRING(fieldId, "D")
					if (states->get(fieldId).getElementCount()) {
						SET_HANDLE_STATE(pen, style, PenStyle::Dash);
					}
				}
				{
					SLIB_STATIC_STRING(fieldId, "Font")
					ListElements<PdfValue> values(states->get(fieldId).getElements());
					if (values.count == 2) {
						setFont(values[0].getName(), values[1].getFloat());
					}
				}
			}

			void concatMatrix(ListElements<PdfValue> operands)
			{
				if (operands.count != 6) {
					return;
				}
				Matrix3 mat(operands[0].getFloat(), operands[1].getFloat(), 0,
					operands[2].getFloat(), operands[3].getFloat(), 0,
					operands[4].getFloat(), operands[5].getFloat(), 1);
				canvas->concatMatrix(mat);
			}

			void fillAndStroke(sl_bool flagEvenOddRule, sl_bool flagStroke = sl_true)
			{
				if (path.isNotNull()) {
					if (flagEvenOddRule) {
						path->setFillMode(FillMode::Alternate);
					} else {
						path->setFillMode(FillMode::Winding);
					}
					canvas->fillPath(path, brush.getHandle());
					if (flagStroke) {
						canvas->drawPath(path, pen.getHandle());
					}
					path.setNull();
				}
			}

			void fill(sl_bool flagEvenOddRule)
			{
				fillAndStroke(flagEvenOddRule, sl_false);
			}

			void stroke()
			{
				if (path.isNotNull()) {
					canvas->drawPath(path, pen.getHandle());
					path.setNull();
				}
			}

			void setClipping(sl_bool flagEvenOddRule)
			{
				if (path.isNotNull()) {
					if (flagEvenOddRule) {
						path->setFillMode(FillMode::Alternate);
					} else {
						path->setFillMode(FillMode::Winding);
					}
					canvas->clipToPath(path);
					path.setNull();
				}
			}

			void beginText()
			{
				text.matrix = Matrix3::identity();
				text.lineMatrix = Matrix3::identity();
			}

			void setTextCharSpace(ListElements<PdfValue> operands)
			{
				if (operands.count != 1) {
					return;
				}
				text.charSpace = operands[0].getFloat();
			}

			void setTextWordSpace(ListElements<PdfValue> operands)
			{
				if (operands.count != 1) {
					return;
				}
				text.wordSpace = operands[0].getFloat();
			}

			void setTextWidthScale(ListElements<PdfValue> operands)
			{
				if (operands.count != 1) {
					return;
				}
				text.widthScale = operands[0].getFloat() / 100.0f;
			}

			void setTextLeading(ListElements<PdfValue> operands)
			{
				if (operands.count != 1) {
					return;
				}
				text.leading = operands[0].getFloat();
			}

			void setTextRise(ListElements<PdfValue> operands)
			{
				if (operands.count != 1) {
					return;
				}
				text.rise = operands[0].getFloat();
			}

			void setTextMatrix(ListElements<PdfValue> operands)
			{
				if (operands.count != 6) {
					return;
				}
				text.matrix = Matrix3(operands[0].getFloat(), operands[1].getFloat(), 0,
					operands[2].getFloat(), operands[3].getFloat(), 0,
					operands[4].getFloat(), operands[5].getFloat(), 1);
				text.lineMatrix = text.matrix;
			}

			void moveTextMatrix(float tx, float ty)
			{
				Transform2::preTranslate(text.lineMatrix, tx, ty);
				text.matrix = text.lineMatrix;
			}

			void moveTextMatrix(ListElements<PdfValue> operands, sl_bool flagSetLeading)
			{
				if (operands.count != 2) {
					return;
				}
				float ty = operands[1].getFloat();
				if (flagSetLeading) {
					text.leading = -ty;
				}
				moveTextMatrix(operands[0].getFloat(), ty);
			}

			void moveToNextLine()
			{
				moveTextMatrix(0, -text.leading);
			}

			void setTextRenderingMode(ListElements<PdfValue> operands)
			{
				if (operands.count != 1) {
					return;
				}
				text.renderingMode = (PdfTextRenderingMode)(operands[0].getUint());
			}

			void setFont(const String& name, float fontScale)
			{
				if (!context) {
					return;
				}
				PdfReference ref;
				if (resources->getFontResource(name, ref)) {
					text.font = context->getFont(ref, *cache);
					text.fontScale = fontScale;
				}
			}

			void setTextFont(ListElements<PdfValue> operands)
			{
				if (operands.count != 2) {
					return;
				}
				setFont(operands[0].getName(), operands[1].getFloat());
			}

			void drawText(const String& str)
			{
				if (text.font.isNull()) {
					return;
				}
				PdfFont& font = *(text.font);
				if (Math::isAlmostZero(text.fontScale) || Math::isAlmostZero(text.widthScale)) {
					return;
				}
				sl_real scale = text.fontScale * text.widthScale;

				CanvasStateScope scope(canvas);
				Matrix3 mat = text.matrix;
				Transform2::preTranslate(mat, 0, text.rise);
				Transform2::preScale(mat, scale, text.fontScale);
				canvas->concatMatrix(mat);

				sl_real x = 0;
				sl_char8* codes = str.getData();
				sl_size nCodes = str.getLength();
				sl_uint32 lenCode = font.codeLength;
				for (sl_size iCode = 0; iCode + lenCode <= nCodes; iCode += lenCode) {
					sl_uint32 charcode = lenCode == 2 ? SLIB_MAKE_WORD(codes[iCode], codes[iCode + 1]) : (sl_uint8)(codes[iCode]);
					sl_char32 unicode = font.getUnicode(charcode);
					if (text.renderingMode != PdfTextRenderingMode::Invisible) {
						Ref<FreeTypeGlyph> glyph = font.getGlyph(charcode, unicode);
						if (glyph.isNotNull()) {
							CanvasStateScope scope(canvas);
							mat = Transform2::getScalingMatrix(font.scale, font.scale);
							Transform2::translate(mat, x / scale, 0);
							if (glyph->outline.isNotNull()) {
								canvas->concatMatrix(mat);
								canvas->fillPath(glyph->outline, brush.getHandle());
								if (text.renderingMode == PdfTextRenderingMode::FillStroke || text.renderingMode == PdfTextRenderingMode::FillStrokeClip) {
									canvas->drawPath(glyph->outline, pen.getHandle());
								}
							} else if (glyph->bitmap.isNotNull()) {
								mat.m11 = -(mat.m11);
								canvas->concatMatrix(mat);
								Canvas::DrawParam dp;
								if (glyph->flagGrayBitmap) {
									dp.useColorMatrix = sl_true;
									dp.colorMatrix.setOverlay(brush.color);
								}
								canvas->draw((sl_real)(glyph->bitmapLeft), -(sl_real)(glyph->bitmapTop), glyph->bitmap, dp);
							}
						}
					}
					x += font.getCharWidth(charcode, unicode) * scale;
					x += text.charSpace * text.widthScale;
					if (unicode == ' ') {
						x += text.wordSpace * text.widthScale;
					}
				}
				Transform2::preTranslate(text.matrix, x, 0);
			}

			void adjustTextMatrix(float f)
			{
				Transform2::preTranslate(text.matrix, - f / 1000.0f * text.fontScale * text.widthScale, 0);
			}

			void showText(ListElements<PdfValue> operands)
			{
				if (operands.count != 1) {
					return;
				}
				const String& text = operands[0].getString();
				drawText(text);
			}

			void showTextWithPositions(ListElements<PdfValue> operands)
			{
				if (operands.count != 1) {
					return;
				}
				ListElements<PdfValue> args(operands[0].getElements());
				for (sl_size i = 0; i < args.count; i++) {
					PdfValue& v = args[i];
					const String& s = v.getString();
					if (s.isNotNull()) {
						drawText(s);
					} else {
						float f;
						if (v.getFloat(f)) {
							adjustTextMatrix(f);
						}
					}
				}
			}

			void showTextWithSpacingParams(ListElements<PdfValue> operands)
			{
				if (operands.count != 3) {
					return;
				}
				text.wordSpace = operands[0].getFloat();
				text.charSpace = operands[1].getFloat();
				moveToNextLine();
				const String& text = operands[2].getString();
				drawText(text);
			}

			void drawImage(PdfImage* image)
			{
				if (image->flagImageMask) {
					Canvas::DrawParam dp;
					dp.useColorMatrix = sl_true;
					dp.colorMatrix.setOverlay(brush.color);
					canvas->draw(0, 0, 1, 1, image->object->flip(FlipMode::Vertical), dp);
				} else {
					canvas->draw(0, 0, 1, 1, image->object->flip(FlipMode::Vertical));
				}
			}

			void drawExternalObject(ListElements<PdfValue> operands)
			{
				if (!context) {
					return;
				}
				if (operands.count != 1) {
					return;
				}
				const String& name = operands[0].getName();
				PdfReference ref;
				if (resources->getExternalObjectResource(name, ref)) {
					Ref<PdfExternalObject> xobj = context->getExternalObject(ref, *cache);
					if (xobj.isNotNull()) {
						if (xobj->type == PdfExternalObjectType::Image) {
							PdfImage* image = (PdfImage*)(xobj.get());
							drawImage(image);
						} else {
							PdfForm* form = (PdfForm*)(xobj.get());
							PdfResourceProvider* oldResources = resources;
							resources = form;
							CanvasStateScope scope(canvas);
							canvas->concatMatrix(form->matrix);
							canvas->clipToRectangle(form->bounds);
							ListElements<PdfOperation> ops(form->content);
							for (sl_size i = 0; i < ops.count; i++) {
								render(ops[i]);
							}
							resources = oldResources;
						}
					}
				}
			}

			void drawInlineImage(ListElements<PdfValue> operands)
			{
				if (operands.count != 1) {
					return;
				}
				const Ref<PdfImage>& image = operands[0].getImage();
				if (image.isNotNull()) {
					drawImage(image.get());
				}
			}

			void saveGraphicsState()
			{
				canvas->save();
				states.push(*this);
			}

			void restoreGraphicsState()
			{
				if (states.isEmpty()) {
					return;
				}
				canvas->restore();
				states.pop(this);
			}

			void render(PdfOperation& operation)
			{
				switch (operation.op) {
					case PdfOperator::b:
						closePath();
						fillAndStroke(sl_false);
						break;
					case PdfOperator::B:
						fillAndStroke(sl_false);
						break;
					case PdfOperator::b_:
						closePath();
						fillAndStroke(sl_true);
						break;
					case PdfOperator::B_:
						fillAndStroke(sl_true);
						break;
					case PdfOperator::BDC:
						// begin marked-content sequence with property list
						break;
					case PdfOperator::BMC:
						// begin marked-content sequence
						break;
					case PdfOperator::BT:
						beginText();
						break;
					case PdfOperator::BX:
						// begin compatibility section
						break;
					case PdfOperator::c:
						curveTo(operation.operands, sl_false, sl_false);
						break;
					case PdfOperator::cm:
						concatMatrix(operation.operands);
						break;
					case PdfOperator::CS:
						setColorSpace(operation.operands, sl_true);
						break;
					case PdfOperator::cs:
						setColorSpace(operation.operands, sl_false);
						break;
					case PdfOperator::d:
						setLineDashPattern(operation.operands);
						break;
					case PdfOperator::d0:
						// set char width (glphy with in Type3 font)
						break;
					case PdfOperator::d1:
						// set cache device (glphy with and bounding box in Type3 font)
						break;
					case PdfOperator::Do:
						drawExternalObject(operation.operands);
						break;
					case PdfOperator::DP:
						// define marked-content point with property list
						break;
					case PdfOperator::EI:
						drawInlineImage(operation.operands);
						break;
					case PdfOperator::EMC:
						// End marked-content sequence
						break;
					case PdfOperator::ET:
						// end text object
						break;
					case PdfOperator::EX:
						// end compatibility section
						break;
					case PdfOperator::f:
						fill(sl_false);
						break;
					case PdfOperator::F:
						fill(sl_false);
						break;
					case PdfOperator::f_:
						fill(sl_true);
						break;
					case PdfOperator::G:
						setGrayLevel(operation.operands, sl_true);
						break;
					case PdfOperator::g:
						setGrayLevel(operation.operands, sl_false);
						break;
					case PdfOperator::gs:
						setGraphicsState(operation.operands);
						break;
					case PdfOperator::h:
						closePath();
						break;
					case PdfOperator::i:
						// set flatness tolerance
						break;
					case PdfOperator::j:
						setLineJoin(operation.operands);
						break;
					case PdfOperator::J:
						setLineCap(operation.operands);
						break;
					case PdfOperator::K:
						setCMYK(operation.operands, sl_true);
						break;
					case PdfOperator::k:
						setCMYK(operation.operands, sl_false);
						break;
					case PdfOperator::l:
						lineTo(operation.operands);
						break;
					case PdfOperator::m:
						moveTo(operation.operands);
						break;
					case PdfOperator::M:
						setMiterLimit(operation.operands);
						break;
					case PdfOperator::MP:
						// define marked-content point
						break;
					case PdfOperator::n:
						clearPath();
						break;
					case PdfOperator::q:
						saveGraphicsState();
						break;
					case PdfOperator::Q:
						restoreGraphicsState();
						break;
					case PdfOperator::re:
						appendRect(operation.operands);
						break;
					case PdfOperator::RG:
						setRGB(operation.operands, sl_true);
						break;
					case PdfOperator::rg:
						setRGB(operation.operands, sl_false);
						break;
					case PdfOperator::ri:
						// set color rendering intent
						break;
					case PdfOperator::s:
						closePath();
						stroke();
						break;
					case PdfOperator::S:
						stroke();
						break;
					case PdfOperator::SC:
						setColor(operation.operands, sl_true);
						break;
					case PdfOperator::sc:
						setColor(operation.operands, sl_false);
						break;
					case PdfOperator::SCN:
						setColor(operation.operands, sl_true);
						break;
					case PdfOperator::scn:
						setColor(operation.operands, sl_false);
						break;
					case PdfOperator::sh:
						// paint area defined by shading pattern
						break;
					case PdfOperator::T_:
						moveToNextLine();
						break;
					case PdfOperator::Tc:
						setTextCharSpace(operation.operands);
						break;
					case PdfOperator::Td:
						moveTextMatrix(operation.operands, sl_false);
						break;
					case PdfOperator::TD:
						moveTextMatrix(operation.operands, sl_true);
						break;
					case PdfOperator::Tf:
						setTextFont(operation.operands);
						break;
					case PdfOperator::Tj:
						showText(operation.operands);
						break;
					case PdfOperator::TJ:
						showTextWithPositions(operation.operands);
						break;
					case PdfOperator::TL:
						setTextLeading(operation.operands);
						break;
					case PdfOperator::Tm:
						setTextMatrix(operation.operands);
						break;
					case PdfOperator::Tr:
						setTextRenderingMode(operation.operands);
						break;
					case PdfOperator::Ts:
						setTextRise(operation.operands);
						break;
					case PdfOperator::Tw:
						setTextWordSpace(operation.operands);
						break;
					case PdfOperator::Tz:
						setTextWidthScale(operation.operands);
						break;
					case PdfOperator::v:
						curveTo(operation.operands, sl_true, sl_false);
						break;
					case PdfOperator::w:
						setLineWidth(operation.operands);
						break;
					case PdfOperator::W:
						setClipping(sl_false);
						break;
					case PdfOperator::W_:
						setClipping(sl_true);
						break;
					case PdfOperator::y:
						curveTo(operation.operands, sl_false, sl_true);
						break;
					case PdfOperator::apos:
						moveToNextLine();
						showText(operation.operands);
						break;
					case PdfOperator::quot:
						showTextWithSpacingParams(operation.operands);
						break;
					default:
						break;
				}
			}

		};

	}

	void PdfPage::render(RenderParam& param)
	{
		ListElements<PdfOperation> ops(getContent());
		if (!(ops.count)) {
			return;
		}

		if (param.cache.isNull()) {
			param.cache = new PdfResourceCache;
			if (param.cache.isNull()) {
				return;
			}
		}

		Canvas* canvas = param.canvas;
		Ref<Context> context = GetContextRef(m_context);

		Renderer renderer;
		renderer.context = context.get();
		renderer.canvas = canvas;
		renderer.resources = this;
		renderer.cache = param.cache.get();

		CanvasStateScope stateScope(canvas);
		CanvasAntiAliasScope antiAliasScope(canvas, sl_true);

		Rectangle bounds = param.bounds;
		Swap(bounds.top, bounds.bottom);
		canvas->concatMatrix(Transform2::getTransformMatrixFromRectToRect(getMediaBox(), bounds));
		canvas->clipToRectangle(getCropBox());

		for (sl_size i = 0; i < ops.count; i++) {
			renderer.render(ops[i]);
		}

		// restore states
		{
			RenderState state;
			while (renderer.states.pop(&state)) {
				canvas->restore();
			}
		}
	}

	Rectangle PdfPage::getMediaBox()
	{
		return getAttribute(name::MediaBox).getRectangle();
	}

	Rectangle PdfPage::getCropBox()
	{
		Rectangle ret;
		if (getAttribute(name::CropBox).getRectangle(ret)) {
			return ret;
		}
		return getMediaBox();
	}

	PdfValue PdfPage::getResources(const String& type, sl_bool flagResolveReference)
	{
		Ref<PdfPageTreeItem> item = this;
		for (;;) {
			PdfValue ret = item->attributes->get(name::Resources).getItem(type, flagResolveReference);
			if (ret.isNotUndefined()) {
				return ret;
			}
			item = item->parent;
			if (item.isNull()) {
				break;
			}
		}
		return PdfValue();
	}

	PdfValue PdfPage::getResource(const String& type, const String& name, sl_bool flagResolveReference)
	{
		Ref<PdfPageTreeItem> item = this;
		for (;;) {
			PdfValue ret = item->attributes->get(name::Resources)[type].getItem(name, flagResolveReference);
			if (ret.isNotUndefined()) {
				return ret;
			}
			item = item->parent;
			if (item.isNull()) {
				break;
			}
		}
		return PdfValue();
	}


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(PdfDocumentParam)

	PdfDocumentParam::PdfDocumentParam()
	{
	}


	SLIB_DEFINE_ROOT_OBJECT(PdfDocument)

	PdfDocument::PdfDocument(): fileSize(0)
	{
	}

	PdfDocument::~PdfDocument()
	{
	}

	Ref<PdfDocument> PdfDocument::create()
	{
		Ref<MemoryContext> context = new MemoryContext(sl_true);
		if (context.isNotNull()) {
			context->source = sl_null;
			context->sizeSource = 0;
			if (context->createDocument()) {
				Ref<PdfDocument> ret = new PdfDocument;
				if (ret.isNotNull()) {
					ret->m_context = Move(context);
					return ret;
				}
			}
		}
		return sl_null;
	}

	Ref<PdfDocument> PdfDocument::open(const PdfDocumentParam& param)
	{
		Ref<Context> context;
		if (param.content.isNotNull()) {
			sl_uint32 fileSize = (sl_uint32)(param.content.getSize());
			if (fileSize > MAX_PDF_FILE_SIZE) {
				return sl_null;
			}
			Ref<MemoryContext> contextMem = new MemoryContext(sl_true);
			if (contextMem.isNull()) {
				return sl_null;
			}
			contextMem->source = (sl_char8*)(param.content.getData());
			contextMem->sizeSource = fileSize;
			contextMem->refSource = param.content.getRef();
			context = Move(contextMem);
		} else if (param.filePath.isNotNull()) {
			Ref<FileIO> file = FileIO::openForRead(param.filePath);
			if (file.isNull()) {
				return sl_null;
			}
			sl_uint64 fileSize = file->getSize();
			if (!fileSize) {
				return sl_null;
			}
			if (fileSize > MAX_PDF_FILE_SIZE) {
				return sl_null;
			}
			Ref<BufferedContext> contextFile = new BufferedContext(sl_true);
			if (contextFile.isNull()) {
				return sl_null;
			}
			if (!(contextFile->reader.open(file))) {
				return sl_null;
			}
			context = Move(contextFile);
		} else {
			return sl_null;
		}
		if (context->readDocument(param)) {
			Ref<PdfDocument> ret = new PdfDocument;
			if (ret.isNotNull()) {
				ret->m_context = Move(context);
				return ret;
			}
		}
		return sl_null;
	}

	Ref<PdfDocument> PdfDocument::openFile(const StringParam& filePath, const StringParam& password)
	{
		if (filePath.isNull()) {
			return sl_null;
		}
		PdfDocumentParam param;
		param.filePath = filePath;
		param.password = password;
		return open(param);
	}

	Ref<PdfDocument> PdfDocument::openMemory(const Memory& mem, const StringParam& password)
	{
		if (mem.isNull()) {
			return sl_null;
		}
		PdfDocumentParam param;
		param.content = mem;
		param.password = password;
		return open(param);
	}

	sl_uint32 PdfDocument::getMaximumObjectNumber()
	{
		Context* context = GetContext(m_context);
		return context->getMaximumObjectNumber();
	}

	PdfValue PdfDocument::getObject(const PdfReference& ref)
	{
		Context* context = GetContext(m_context);
		ObjectLocker lock(context);
		return context->getObject(ref);
	}

	PdfValue PdfDocument::getObject(sl_uint32 objectNumber, sl_uint32& outGeneration)
	{
		Context* context = GetContext(m_context);
		ObjectLocker lock(context);
		sl_int32& gen = *((sl_int32*)&outGeneration);
		gen = -1;
		return context->getObject(objectNumber, gen);
	}

	Ref<PdfStream> PdfDocument::getStream(sl_uint32 objectNumber, sl_uint32& outGeneration)
	{
		Context* context = GetContext(m_context);
		ObjectLocker lock(context);
		sl_int32& gen = *((sl_int32*)&outGeneration);
		gen = -1;
		return context->getStream(objectNumber, gen);
	}

	sl_bool PdfDocument::setObject(const PdfReference& ref, const PdfValue& value)
	{
		Context* context = GetContext(m_context);
		ObjectLocker lock(context);
		return context->setObject(ref, value);
	}

	sl_bool PdfDocument::addObject(const PdfValue& value, PdfReference& outRef)
	{
		Context* context = GetContext(m_context);
		ObjectLocker lock(context);
		return context->addObject(value, outRef);
	}

	sl_bool PdfDocument::deleteObject(const PdfReference& ref)
	{
		Context* context = GetContext(m_context);
		ObjectLocker lock(context);
		return context->deleteObject(ref);
	}

	sl_uint32 PdfDocument::getPageCount()
	{
		Context* context = GetContext(m_context);
		ObjectLocker lock(context);
		PageTreeParent* tree = context->getPageTree();
		if (tree) {
			return tree->getPageCount();
		} else {
			return 0;
		}
	}

	Ref<PdfPage> PdfDocument::getPage(sl_uint32 index)
	{
		Context* context = GetContext(m_context);
		ObjectLocker lock(context);
		return context->getPage(index);
	}

	sl_bool PdfDocument::addJpegImagePage(sl_uint32 width, sl_uint32 height, const Memory& jpeg)
	{
		return insertJpegImagePage(getPageCount(), width, height, jpeg);
	}

	sl_bool PdfDocument::insertJpegImagePage(sl_uint32 index, sl_uint32 width, sl_uint32 height, const Memory& jpeg)
	{
		Context* context = GetContext(m_context);
		ObjectLocker lock(context);
		return context->insertJpegImagePage(index, width, height, jpeg);
	}

	sl_bool PdfDocument::deletePage(sl_uint32 index)
	{
		Context* context = GetContext(m_context);
		ObjectLocker lock(context);
		return context->deletePage(index);
	}

	Memory PdfDocument::save()
	{
		MemoryOutput writer;
		if (save(&writer)) {
			return writer.merge();
		}
		return sl_null;
	}

	sl_bool PdfDocument::save(IWriter* writer)
	{
		Context* context = GetContext(m_context);
		ObjectLocker lock(context);
		return context->save(writer);
	}

	Ref<PdfFont> PdfDocument::getFont(const PdfReference& ref, PdfResourceCache& cache)
	{
		Context* context = GetContext(m_context);
		ObjectLocker lock(context);
		return context->getFont(ref, cache);
	}

	Ref<PdfExternalObject> PdfDocument::getExternalObject(const PdfReference& ref, PdfResourceCache& cache)
	{
		Context* context = GetContext(m_context);
		ObjectLocker lock(context);
		return context->getExternalObject(ref, cache);
	}

	sl_bool PdfDocument::isEncrypted()
	{
		Context* context = GetContext(m_context);
		return context->encrypt.isNotNull();
	}

	sl_bool PdfDocument::isAuthenticated()
	{
		if (isEncrypted()) {
			Context* context = GetContext(m_context);
			return context->flagDecryptContents;
		} else {
			return sl_true;
		}
	}


	const sl_char16* Pdf::getUnicodeTable(PdfEncoding encoding) noexcept
	{
		switch (encoding) {
			case PdfEncoding::Standard:
				return g_encodingStandard;
			case PdfEncoding::MacRoman:
				return g_encodingMacRoman;
			case PdfEncoding::WinAnsi:
				return g_encodingWinAnsi;
			case PdfEncoding::PdfDoc:
				return g_encodingPdfDoc;
			case PdfEncoding::MacExpert:
				return g_encodingMacExpert;
			case PdfEncoding::Symbol:
				return g_encodingAdobeSymbol;
			case PdfEncoding::MSSymbol:
				return g_encodingMSSymbol;
			case PdfEncoding::Zapf:
				return g_encodingZapf;
			default:
				break;
		}
		return sl_null;
	}

	const char* const* Pdf::getCharNameTable(PdfEncoding encoding) noexcept
	{
		switch (encoding) {
			case PdfEncoding::Standard:
				return g_charNamesStandard;
			case PdfEncoding::MacRoman:
				return g_charNamesMacRoman;
			case PdfEncoding::WinAnsi:
				return g_charNamesWinAnsi;
			case PdfEncoding::MacExpert:
				return g_charNamesMacExpert;
			default:
				break;
		}
		return sl_null;
	}

	PdfFilter Pdf::getFilter(const StringView& filter) noexcept
	{
		if (filter == name::FlateDecode || filter == name::Fl) {
			return PdfFilter::Flate;
		} else if (filter == name::DCTDecode || filter == name::DCT) {
			return PdfFilter::DCT;
		} else if (filter == name::LZWDecode || filter == name::LZW) {
			return PdfFilter::LZW;
		} else if (filter == name::RunLengthDecode || filter == name::RL) {
			return PdfFilter::RunLength;
		} else if (filter == name::ASCIIHexDecode || filter == name::AHx) {
			return PdfFilter::ASCIIHex;
		} else if (filter == name::ASCII85Decode || filter == name::A85) {
			return PdfFilter::ASCII85;
		} else if (filter == name::CCITTFaxDecode || filter == name::CCF) {
			return PdfFilter::CCITTFax;;
		} else {
			return PdfFilter::Unknown;
		}
	}

	PdfEncoding Pdf::getEncoding(const StringView& name) noexcept
	{
		if (name.isEmpty()) {
			return PdfEncoding::Unknown;
		}
		if (name == StringView::literal("Identity-H")) {
			return PdfEncoding::IdentityH;
		} else if (name == StringView::literal("Identity-V")) {
			return PdfEncoding::IdentityV;
		} else if (name == StringView::literal("StandardEncoding")) {
			return PdfEncoding::Standard;
		} else if (name == StringView::literal("MacRomanEncoding")) {
			return PdfEncoding::MacRoman;
		} else if (name == StringView::literal("WinAnsiEncoding")) {
			return PdfEncoding::WinAnsi;
		} else if (name == StringView::literal("PDFDocEncoding")) {
			return PdfEncoding::PdfDoc;
		} else if (name == StringView::literal("MacExpertEncoding")) {
			return PdfEncoding::MacExpert;
		} else {
			return PdfEncoding::Unknown;
		}
	}

	sl_bool Pdf::isPdfFile(const StringParam& path)
	{
		File file = File::openForRead(path);
		if (file.isOpened()) {
			sl_uint8 c[5];
			if (file.readFully(c, 5) == 5) {
				return Base::equalsMemory(c, "%PDF-", 5);
			}
		}
		return sl_false;
	}

	sl_bool Pdf::isEncryptedFile(const StringParam& path)
	{
		PdfDocument doc;
		if (doc.openFile(path)) {
			return doc.isEncrypted();
		}
		return sl_false;
	}

}
