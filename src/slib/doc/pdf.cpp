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

#include "slib/core/file_io.h"
#include "slib/core/buffered_seekable_reader.h"
#include "slib/core/thread.h"
#include "slib/core/mio.h"
#include "slib/core/memory_buffer.h"
#include "slib/core/string_buffer.h"
#include "slib/core/queue.h"
#include "slib/core/safe_static.h"
#include "slib/crypto/zlib.h"
#include "slib/crypto/lzw.h"
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
#define EXPIRE_DURATION_FONT_BITMAP 5000

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
			SLIB_STATIC_STRING(g_strBaseFont, "BaseFont")
			SLIB_STATIC_STRING(g_strDescendantFonts, "DescendantFonts")
			SLIB_STATIC_STRING(g_strEncoding, "Encoding")
			SLIB_STATIC_STRING(g_strBaseEncoding, "BaseEncoding")
			SLIB_STATIC_STRING(g_strDifferences, "Differences")
			SLIB_STATIC_STRING(g_strFontDescriptor, "FontDescriptor")
			SLIB_STATIC_STRING(g_strFontName, "FontName")
			SLIB_STATIC_STRING(g_strFontFamily, "FontFamily")
			SLIB_STATIC_STRING(g_strAscent, "Ascent")
			SLIB_STATIC_STRING(g_strDescent, "Descent")
			SLIB_STATIC_STRING(g_strLeading, "Leading")
			SLIB_STATIC_STRING(g_strFontWeight, "FontWeight")
			SLIB_STATIC_STRING(g_strItalicAngle, "ItalicAngle")
			SLIB_STATIC_STRING(g_strFlags, "Flags")
			SLIB_STATIC_STRING(g_strFontFile, "FontFile")
			SLIB_STATIC_STRING(g_strFontFile2, "FontFile2")
			SLIB_STATIC_STRING(g_strFontFile3, "FontFile3")
			SLIB_STATIC_STRING(g_strFirstChar, "FirstChar")
			SLIB_STATIC_STRING(g_strLastChar, "LastChar")
			SLIB_STATIC_STRING(g_strWidths, "Widths")
			SLIB_STATIC_STRING(g_strDW, "DW")
			SLIB_STATIC_STRING(g_strCIDToGIDMap, "CIDToGIDMap")
			SLIB_STATIC_STRING(g_strToUnicode, "ToUnicode")
			SLIB_STATIC_STRING(g_strWidth, "Width")
			SLIB_STATIC_STRING(g_strHeight, "Height")
			SLIB_STATIC_STRING(g_strColorSpace, "ColorSpace")
			SLIB_STATIC_STRING(g_strCS, "CS")
			SLIB_STATIC_STRING(g_strDecodeParms, "DecodeParms")
			SLIB_STATIC_STRING(g_strDP, "DP")
			SLIB_STATIC_STRING(g_strPredictor, "Predictor")
			SLIB_STATIC_STRING(g_strEarlyChange, "EarlyChange")
			SLIB_STATIC_STRING(g_strColumns, "Columns")
			SLIB_STATIC_STRING(g_strBitsPerComponent, "BitsPerComponent")
			SLIB_STATIC_STRING(g_strBPC, "BPC")
			SLIB_STATIC_STRING(g_strImageMask, "ImageMask")
			SLIB_STATIC_STRING(g_strIM, "IM")
			SLIB_STATIC_STRING(g_strInterpolate, "Interpolate")
			SLIB_STATIC_STRING(g_strDecode, "Decode")
			SLIB_STATIC_STRING(g_strMatte, "Matte")
			SLIB_STATIC_STRING(g_strColors, "Colors")
			SLIB_STATIC_STRING(g_strRows, "Rows")
			SLIB_STATIC_STRING(g_strEndOfLine, "EndOfLine")
			SLIB_STATIC_STRING(g_strEncodedByteAlign, "EncodedByteAlign")
			SLIB_STATIC_STRING(g_strBlackIs1, "BlackIs1")
			SLIB_STATIC_STRING(g_strSMask, "SMask")
			SLIB_STATIC_STRING(g_strMask, "Mask")
			SLIB_STATIC_STRING(g_strAlternate, "Alternate")
			SLIB_STATIC_STRING(g_strD, "D")
			SLIB_STATIC_STRING(g_strF, "F")
			SLIB_STATIC_STRING(g_strH, "H")
			SLIB_STATIC_STRING(g_strI, "I")
			SLIB_STATIC_STRING(g_strK, "K")
			SLIB_STATIC_STRING(g_strN, "N")
			SLIB_STATIC_STRING(g_strO, "O")
			SLIB_STATIC_STRING(g_strP, "P")
			SLIB_STATIC_STRING(g_strR, "R")
			SLIB_STATIC_STRING(g_strU, "U")
			SLIB_STATIC_STRING(g_strV, "V")
			SLIB_STATIC_STRING(g_strW, "W")

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


			const sl_char16 g_encodingStandard[256] = {
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

			const char* const g_charNamesStandard[256] = {
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

			const sl_char16 g_encodingMacRoman[256] = {
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

			const char* const g_charNamesMacRoman[256] = {
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

			const sl_char16 g_encodingWinAnsi[256] = {
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

			const char* const g_charNamesWinAnsi[256] = {
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

			const sl_char16 g_encodingPdfDoc[256] = {
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

			const sl_char16 g_encodingMacExpert[256] = {
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

			const char* const g_charNamesMacExpert[256] = {
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

			const sl_char16 g_encodingAdobeSymbol[256] = {
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

			const sl_char16 g_encodingMSSymbol[256] = {
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

			const sl_char16 g_encodingZapf[256] = {
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

			class ObjectStream : public Referable
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

			class Context : public Referable, public PdfContentReader
			{
			public:
				sl_uint8 majorVersion = 0;
				sl_uint8 minorVersion = 0;

				PdfDictionary lastTrailer;
				PdfDictionary encrypt;
				PdfDictionary catalog;
				Ref<PageTreeParent> pageTree;

				sl_bool flagDecryptContents = sl_false;
				sl_uint8 encryptionKey[16];
				sl_uint32 lenEncryptionKey = 0;

				Array<CrossReferenceEntry> references;
				ExpiringMap<sl_uint64, PdfValue> objects;
				ExpiringMap< sl_uint64, Ref<ObjectStream> > objectStreams;

			public:
				Context()
				{
					objects.setExpiringMilliseconds(EXPIRE_DURATION_OBJECT);
					objectStreams.setExpiringMilliseconds(EXPIRE_DURATION_OBJECT_STREAM);
				}

			public:
				virtual PdfValue readObject(sl_uint32 pos, sl_uint32& outOffsetAfterEndObj, PdfReference& outRef) = 0;
				virtual sl_bool readDocument() = 0;
				
			public:
				Ref<ObjectStream> getObjectStream(const PdfReference& ref);

				sl_bool getReferenceEntry(sl_uint32 objectNumber, CrossReferenceEntry& entry)
				{
					return references.getAt(objectNumber, &entry);
				}

				void setReferenceEntry(sl_uint32 objectNumber, const CrossReferenceEntry& entry)
				{
					CrossReferenceEntry* pEntry = references.getPointerAt(objectNumber);
					if (pEntry) {
						if (pEntry->type == (sl_uint32)(CrossReferenceEntryType::Free) && entry.type != (sl_uint32)(CrossReferenceEntryType::Free)) {
							*pEntry = entry;
						}
					}
				}

				PdfValue getObject(const PdfReference& ref)
				{
					sl_uint64 _id = GetObjectId(ref);
					PdfValue ret;
					if (objects.get(_id, &ret)) {
						return ret;
					}
					CrossReferenceEntry entry;
					if (getReferenceEntry(ref.objectNumber, entry)) {
						if (entry.type == (sl_uint32)(CrossReferenceEntryType::Normal)) {
							if (entry.generation == ref.generation) {
								PdfReference n;
								sl_uint32 offsetEnd;
								ret = readObject(entry.offset, offsetEnd, n);
								if (ret.isNotUndefined() && ref == n) {
									objects.put(_id, ret);
									return ret;
								}
							}
						} else if (entry.type == (sl_uint32)(CrossReferenceEntryType::Compressed)) {
							if (!(ref.generation)) {
								Ref<ObjectStream> stream = getObjectStream(entry.streamObject);
								if (stream.isNotNull()) {
									sl_uint32 n;
									ret = stream->getItem(entry.generation, n);
									if (ret.isNotUndefined() && _id == n) {
										objects.put(_id, ret);
										return ret;
									}
								}
							}
						}
					}
					return PdfValue();
				}

				PdfValue getObject(const PdfValue& value)
				{
					PdfReference ref;
					if (value.getReference(ref)) {
						return getObject(ref);
					}
					return value;
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

				Memory decodeStreamContent(const PdfReference& ref)
				{
					Ref<PdfStream> stream = getObject(ref).getStream();
					if (stream.isNotNull()) {
						return stream->getDecodedContent(this);
					}
					return sl_null;
				}

				Memory decodeStreamContent(const PdfValue& value)
				{
					PdfReference ref;
					if (value.getReference(ref)) {
						return decodeStreamContent(ref);
					}
					const Ref<PdfStream>& stream = value.getStream();
					if (stream.isNotNull()) {
						return stream->getDecodedContent(this);
					}
					return sl_null;
				}

				sl_bool getStreamLength(const PdfDictionary& properties, sl_uint32& _out)
				{
					return getObject(properties.getValue_NoLock(g_strLength)).getUint(_out);
				}

				List< Ref<PdfPageTreeItem> > buildPageTreeKids(const PdfDictionary& attributes)
				{
					List< Ref<PdfPageTreeItem> > ret;
					ListElements<PdfValue> kidIds(attributes.getValue_NoLock(g_strKids).getArray());
					for (sl_size i = 0; i < kidIds.count; i++) {
						const PdfDictionary& props = getObject(kidIds[i]).getDictionary();
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

				Ref<PdfPage> getPage(sl_uint32 index)
				{
					PageTreeParent* tree = pageTree.get();
					if (tree) {
						return getPage(tree, index);
					}
					return sl_null;
				}

				Memory getPageContent(const PdfValue& contents)
				{
					const PdfArray& array = contents.getArray();
					if (array.isNotNull()) {
						MemoryBuffer buf;
						ListElements<PdfValue> items(array);
						for (sl_size i = 0; i < items.count; i++) {
							buf.add(decodeStreamContent(items[i]));
						}
						return buf.merge();
					} else {
						return decodeStreamContent(getObject(contents));
					}
				}

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
							const String& userHash = encrypt.getValue_NoLock(g_strU).getString();
							if (userHash.getLength() != 32) {
								return sl_false;
							}
							sl_uint32 revision = encrypt.getValue_NoLock(g_strR).getUint();
							sl_uint32 permission = encrypt.getValue_NoLock(g_strP).getInt();
							const String& ownerHash = encrypt.getValue_NoLock(g_strO).getString();
							const String& fileId = lastTrailer.getValue_NoLock(g_strID).getArray().getValueAt_NoLock(0).getString();
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

				sl_bool initDocument()
				{
					catalog = getObject(lastTrailer.getValue_NoLock(g_strRoot)).getDictionary();
					if (catalog.isNull()) {
						return sl_false;
					}
					encrypt = getObject(lastTrailer.getValue_NoLock(g_strEncrypt)).getDictionary();
					// loading page tree
					{
						const PdfDictionary& attrs = getObject(catalog.getValue_NoLock(g_strPages)).getDictionary();
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

			class CrossReferenceTable
			{
			public:
				Ref<Context> context;
				PdfDictionary trailer;
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
										list.add_NoLock(ch);
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
									String ret(list.getData(), list.getCount());
									if (ret.isNotNull()) {
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
							PdfValue value = readValue();
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
						PdfValue var = readValue();
						if (var.isNotUndefined()) {
							ret.add_NoLock(Move(var));
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

				PdfValue readValue()
				{
					sl_char8 ch;
					if (!(peekChar(ch))) {
						return PdfValue();
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

				PdfValue readObject(PdfReference& outRef)
				{
					if (readObjectHeader(outRef)) {
						PdfValue value = readValue();
						if (value.isNotUndefined()) {
							if (skipWhitespaces()) {
								const PdfDictionary& properties = value.getDictionary();
								if (properties.isNotNull()) {
									if (peekCharAndEquals('s')) {
										sl_uint32 pos = getPosition();
										sl_uint32 length;
										if (!(getStreamLength(properties, length))) {
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
										Ref<PdfStream> stream = new PdfStream;
										if (stream.isNull()) {
											return PdfValue();
										}
										stream->initialize(properties, outRef, offsetContent, length);
										value = PdfValue(Move(stream));
									}
								} else {
									if (flagDecryptContents) {
										const String& str = value.getString();
										if (str.isNotNull()) {
											decrypt(outRef, str.getData(), str.getLength());
										}
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

				PdfValue readObject(sl_uint32 pos, sl_uint32& outOffsetAfterEndObj, PdfReference& outRef) override
				{
					if (setPosition(pos)) {
						PdfValue ret = readObject(outRef);
						if (ret.isNotUndefined()) {
							outOffsetAfterEndObj = getPosition();
							return ret;
						}
					}
					return PdfValue();
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
									ListElements<PdfValue> entrySizes(stream->getProperty(g_strW).getArray());
									if (entrySizes.count == 3) {
										if (entrySizes[0].getType() == PdfValueType::Uint && entrySizes[1].getType() == PdfValueType::Uint && entrySizes[2].getType() == PdfValueType::Uint) {
											CList< Pair<sl_uint32, sl_uint32> > listSectionRanges;
											sl_size nEntries = 0;
											PdfValue vIndex = stream->getProperty(g_strIndex);
											if (vIndex.isNotUndefined()) {
												if (vIndex.getType() != PdfValueType::Array) {
													return sl_false;
												}
												ListElements<PdfValue> indices(vIndex.getArray());
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
											Memory content = stream->getDecodedContent(this);
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

				sl_bool readDocument() override
				{
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
							CrossReferenceTable subTable;
							subTable.context = this;
							if (!(readCrossReferenceTable(subTable))) {
								return sl_false;
							}
							lastTrailer = subTable.trailer;
						}
						// initialize reference table
						{
							sl_uint32 countTotalRef = 0;
							lastTrailer.getValue_NoLock(g_strSize).getUint(countTotalRef);
							if (!countTotalRef) {
								return sl_false;
							}
							references = Array<CrossReferenceEntry>::create(countTotalRef);
							if (references.isNull()) {
								return sl_false;
							}
							Base::zeroMemory(references.getData(), countTotalRef * sizeof(CrossReferenceEntry));
						}
						for (;;) {
							if (!(setPosition(posXref))) {
								return sl_false;
							}
							CrossReferenceTable subTable;
							subTable.context = this;
							if (!(readCrossReferenceTable(subTable))) {
								return sl_false;
							}
							PdfValue prev = subTable.trailer.getValue_NoLock(g_strPrev);
							if (prev.isUndefined()) {
								break;
							}
							if (!(prev.getUint(posXref))) {
								return sl_false;
							}
						}
					}
					return initDocument();
				}

			};

			typedef ContextT<BufferedReaderBase> BufferedContext;
			typedef ContextT<MemoryReaderBase> MemoryContext;

			Ref<ObjectStream> Context::getObjectStream(const PdfReference& ref)
			{
				sl_uint64 _id = GetObjectId(ref);
				Ref<ObjectStream> ret;
				if (objectStreams.get(_id, &ret)) {
					return ret;
				}
				Ref<PdfStream> stream = getObject(ref).getStream();
				if (stream.isNotNull()) {
					if (stream->getProperty(g_strType).equalsName(StringView::literal("ObjStm"))) {
						sl_uint32 nObjects;
						if (stream->getProperty(g_strN).getUint(nObjects)) {
							sl_uint32 first;
							if (stream->getProperty(g_strFirst).getUint(first)) {
								Memory content = stream->getDecodedContent(this);
								if (content.isNotNull()) {
									ret = new ObjectStream;
									if (ret.isNotNull()) {
										PdfValue vExtends = stream->getProperty(g_strExtends);
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
										MemoryContext context;
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
											PdfValue innerValue = context.readValue();
											if (innerValue.isUndefined()) {
												return sl_null;
											}
											ret->objects.add_NoLock(innerId, Move(innerValue));
											context.setPosition(pos);
										}
										objectStreams.put(_id, ret);
										return ret;
									}
								}
							}
						}
					}
				}
				return sl_null;
			}

			SLIB_INLINE static Context* GetContext(const Ref<Referable>& ref)
			{
				return (Context*)(ref.get());
			}


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
				CHashMap< String, sl_uint32, HashIgnoreCase<String>, CompareIgnoreCase<String> > names;

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
					}
					m_flagInvalidate = sl_false;
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
					}
					m_flagInvalidate = sl_false;
					return m_handle;
				}

				void invalidate()
				{
					m_flagInvalidate = sl_true;
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
				Canvas* canvas;
				PdfPage* page;
				PdfRenderParam* param;

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
							sl_size nPoints = path->getPointsCount();
							if (!nPoints) {
								return;
							}
							GraphicsPathPoint& ptCurrent = (path->getPoints())[nPoints - 1];
							path->cubicTo(
								ptCurrent.pt.x, ptCurrent.pt.y,
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
					cs.load(page, operands[0].getName());
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
					PdfColorSpace& cs = flagStroking ? colorSpaceForStroking : colorSpaceForNonStroking;
					Color color;
					if (cs.getColor(color, operands.data, operands.count)) {
						setColor(color, flagStroking);
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
					if (operands[0].getArray().getCount()) {
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
					PdfDictionary states = page->getResource(idExtGState, operands[0].getName()).getDictionary();
					if (states.isEmpty()) {
						return;
					}
					{
						SLIB_STATIC_STRING(fieldId, "LW")
						float value;
						if (states.getValue(fieldId).getFloat(value)) {
							SET_HANDLE_STATE(pen, width, value);
						}
					}
					{
						SLIB_STATIC_STRING(fieldId, "LC")
						sl_uint32 value;
						if (states.getValue(fieldId).getUint(value)) {
							SET_HANDLE_STATE(pen, cap, (LineCap)value);
						}
					}
					{
						SLIB_STATIC_STRING(fieldId, "LJ")
						sl_uint32 value;
						if (states.getValue(fieldId).getUint(value)) {
							SET_HANDLE_STATE(pen, join, (LineJoin)value);
						}
					}
					{
						SLIB_STATIC_STRING(fieldId, "ML")
						float value;
						if (states.getValue(fieldId).getFloat(value)) {
							SET_HANDLE_STATE(pen, miterLimit, value);
						}
					}
					{
						SLIB_STATIC_STRING(fieldId, "D")
						if (states.getValue(fieldId).getArray().isNotNull()) {
							SET_HANDLE_STATE(pen, style, PenStyle::Dash);
						}
					}
					{
						SLIB_STATIC_STRING(fieldId, "Font")
						ListElements<PdfValue> values(states.getValue(fieldId).getArray());
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
					PdfReference ref;
					if (page->getFontResource(name, ref)) {
						Ref<PdfDocument> doc = page->getDocument();
						if (doc.isNotNull()) {
							text.font = PdfFont::load(doc.get(), ref, *(param->context));
						}
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
									DrawParam dp;
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
					ListElements<PdfValue> args(operands[0].getArray());
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

				void drawExternalObject(ListElements<PdfValue> operands)
				{
					if (operands.count != 1) {
						return;
					}
					const String& name = operands[0].getName();
					PdfReference ref;
					if (page->getExternalObjectResource(name, ref)) {
						Ref<PdfDocument> doc = page->getDocument();
						if (doc.isNotNull()) {
							Ref<PdfImage> image = PdfImage::load(doc.get(), ref, *(param->context));
							if (image.isNotNull()) {
								if (image->flagImageMask) {
									DrawParam dp;
									dp.useColorMatrix = sl_true;
									dp.colorMatrix.setOverlay(brush.color);
									canvas->draw(0, 0, 1, 1, image->object->flip(FlipMode::Vertical), dp);
								} else {
									canvas->draw(0, 0, 1, 1, image->object->flip(FlipMode::Vertical));
								}
							}
						}
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
						case PdfOperator::BI:
							// begin inline image object
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
							// end inline image object
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
						case PdfOperator::ID:
							// begin inline image data
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
				MemoryContext context;
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
													ListElements<PdfValue> arr(m->getArray());
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
						PdfValue value = context.readValue();
						if (value.isUndefined()) {
							break;
						}
						args.add_NoLock(Move(value));
					}
				}
				return maxLenCode;
			}

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

			static sl_uint8 PredictPath(sl_int32 a, sl_int32 b, sl_int32 c) {
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

			static sl_bool PredictPNG(sl_uint8* bufData, sl_uint32& sizeData, sl_uint32 colors, sl_uint32 bitsPerComponent, sl_uint32 columns) {
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

			static void PredictTIFF(sl_uint8* bufData, sl_uint32 sizeData, sl_uint32 colors, sl_uint32 bitsPerComponent, sl_uint32 columns) {
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

			SLIB_INLINE static sl_uint8 ApplyDecode(sl_uint8 source, sl_uint8 min, sl_uint8 max)
			{
				return (sl_uint8)(min + (((sl_int32)(max - min) * (sl_int32)source) >> 8));
			}
			
			const sl_uint8 g_faxBlackRunIns[] = {
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

			const sl_uint8 g_faxWhiteRunIns[] = {
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
								while (1) {
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
								while (1) {
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

			static Memory CreateImageMemory(const Ref<Image>& image)
			{
				if (image.isNotNull()) {
					return Memory::createStatic(image->getColors(), (image->getStride() * image->getHeight()) << 2, image);
				}
				return sl_null;
			}
			
			static Memory DecodeStreamContent(const Memory& input, PdfFilter filter, const PdfDictionary& params)
			{
				switch (filter) {
					case PdfFilter::ASCIIHex:
						return DecodeASCIIHex(input.getData(), input.getSize());
					case PdfFilter::ASCII85:
						return DecodeASCII85(input.getData(), input.getSize());
					case PdfFilter::Flate:
					case PdfFilter::LZW:
						{
							Memory ret;
							if (filter == PdfFilter::Flate) {
								ret = Zlib::decompress(input.getData(), input.getSize());
							} else {
								ret = LZW::decompress(input.getData(), input.getSize());;
							}
							if (ret.isNotNull() && params.isNotNull()) {
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
						return DecodeRunLength(input.getData(), input.getSize());
					case PdfFilter::CCITTFax:
						{
							PdfCCITTFaxDecodeParams dp;
							dp.setParams(params);
							
						}
					case PdfFilter::DCT:
						return CreateImageMemory(Image::loadFromMemory(input));
					default:
						break;
				}
				return sl_null;
			}

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
	}

	using namespace priv::pdf;


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(PdfName)

	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(PdfValue)
	
	PdfValue::PdfValue(sl_bool v) noexcept: m_var(v, (sl_uint8)(PdfValueType::Boolean)) {}

	PdfValue::PdfValue(sl_int32 v) noexcept: m_var(v, (sl_uint8)(PdfValueType::Int)) {}
	PdfValue::PdfValue(sl_uint32 v) noexcept: m_var(v, (sl_uint8)(PdfValueType::Uint)) {}
	PdfValue::PdfValue(float v) noexcept: m_var(v, (sl_uint8)(PdfValueType::Float)) {}

	PdfValue::PdfValue(const String& v) noexcept: m_var(v, (sl_uint8)(v.isNotNull() ? PdfValueType::String : PdfValueType::Null)) {}
	PdfValue::PdfValue(String&& v) noexcept: m_var(Move(v), (sl_uint8)(v.isNotNull() ? PdfValueType::String : PdfValueType::Null)) {}

	PdfValue::PdfValue(const PdfArray& v) noexcept: m_var(*((VariantList*)((void*)&v)), (sl_uint8)(v.isNotNull() ? PdfValueType::Array : PdfValueType::Null)) {}
	PdfValue::PdfValue(PdfArray&& v) noexcept: m_var(Move(*((VariantList*)((void*)&v))), (sl_uint8)(v.isNotNull() ? PdfValueType::Array : PdfValueType::Null)) {}

	PdfValue::PdfValue(const PdfDictionary& v) noexcept: m_var(*((VariantMap*)((void*)&v)), (sl_uint8)(v.isNotNull() ? PdfValueType::Dictionary : PdfValueType::Null)) {}
	PdfValue::PdfValue(PdfDictionary&& v) noexcept: m_var(Move(*((VariantMap*)((void*)&v))), (sl_uint8)(v.isNotNull() ? PdfValueType::Dictionary : PdfValueType::Null)) {}

	PdfValue::PdfValue(const Ref<PdfStream>& v) noexcept: m_var(v, (sl_uint8)(v.isNotNull() ? PdfValueType::Stream : PdfValueType::Null)) {}
	PdfValue::PdfValue(Ref<PdfStream>&& v) noexcept: m_var(Move(v), (sl_uint8)(v.isNotNull() ? PdfValueType::Stream : PdfValueType::Null)) {}

	PdfValue::PdfValue(const PdfName& v) noexcept: m_var(v.value, (sl_uint8)(v.isNotNull() ? PdfValueType::Name : PdfValueType::Null)) {}
	PdfValue::PdfValue(PdfName&& v) noexcept : m_var(Move(v.value), (sl_uint8)(v.isNotNull() ? PdfValueType::Name : PdfValueType::Null)) {}

	PdfValue::PdfValue(const PdfReference& v) noexcept: m_var(MAKE_OBJECT_ID(v.objectNumber, v.generation), (sl_uint8)(PdfValueType::Reference)) {}

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

	const String& PdfValue::getString() const noexcept
	{
		if (getType() == PdfValueType::String) {
			return *((String*)((void*)(&m_var._value)));
		}
		return String::null();
	}

	const String& PdfValue::getName() const noexcept
	{
		if (getType() == PdfValueType::Name) {
			return *((String*)((void*)(&m_var._value)));
		}
		return String::null();
	}

	sl_bool PdfValue::equalsName(const StringView& name) const noexcept
	{
		if (getType() == PdfValueType::Name) {
			return *((String*)((void*)(&m_var._value))) == name;
		}
		return sl_false;
	}

	const PdfArray& PdfValue::getArray() const& noexcept
	{
		if (getType() == PdfValueType::Array) {
			return *((PdfArray*)((void*)(&m_var._value)));
		}
		return PdfArray::null();
	}

	PdfArray PdfValue::getArray()&& noexcept
	{
		if (getType() == PdfValueType::Array) {
			return *((PdfArray*)((void*)(&m_var._value)));
		}
		return sl_null;
	}

	const PdfDictionary& PdfValue::getDictionary() const& noexcept
	{
		if (getType() == PdfValueType::Dictionary) {
			return *((PdfDictionary*)((void*)(&m_var._value)));
		}
		return PdfDictionary::null();
	}

	PdfDictionary PdfValue::getDictionary()&& noexcept
	{
		if (getType() == PdfValueType::Dictionary) {
			return *((PdfDictionary*)((void*)(&m_var._value)));
		}
		return sl_null;
	}

	const Ref<PdfStream>& PdfValue::getStream() const& noexcept
	{
		if (getType() == PdfValueType::Stream) {
			return *((Ref<PdfStream>*)((void*)(&m_var._value)));
		}
		return Ref<PdfStream>::null();
	}

	Ref<PdfStream> PdfValue::getStream()&& noexcept
	{
		if (getType() == PdfValueType::Stream) {
			return *((Ref<PdfStream>*)((void*)(&m_var._value)));
		}
		return sl_null;
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

	Rectangle PdfValue::getRectangle() const noexcept
	{
		Rectangle ret;
		if (getRectangle(ret)) {
			return ret;
		}
		return Rectangle::zero();
	}

	sl_bool PdfValue::getRectangle(Rectangle& outRect) const noexcept
	{
		ListElements<PdfValue> arr(getArray());
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

	PdfStream::PdfStream() noexcept: m_ref(0, 0), m_offsetContent(0), m_sizeContent(0)
	{
	}

	PdfStream::~PdfStream()
	{
	}

	void PdfStream::initialize(const PdfDictionary& _properties, const PdfReference& ref, sl_uint32 offsetContent, sl_uint32 sizeContent) noexcept
	{
		properties = _properties;
		m_ref = ref;
		m_offsetContent = offsetContent;
		m_sizeContent = sizeContent;
	}

	PdfValue PdfStream::getProperty(const String& name) noexcept
	{
		return properties.getValue_NoLock(name);
	}

	PdfValue PdfStream::getProperty(const String& name, const String& alternateName) noexcept
	{
		PdfValue ret;
		if (properties.get_NoLock(name, &ret)) {
			return ret;
		}
		return properties.getValue_NoLock(alternateName);
	}

	Memory PdfStream::getEncodedContent(PdfContentReader* reader)
	{
		if (!m_offsetContent) {
			return m_contentEncoded;
		}
		return reader->readContent(m_offsetContent, m_sizeContent, m_ref);
	}

	void PdfStream::setEncodedContent(const Memory& content) noexcept
	{
		m_contentEncoded = content;
		m_offsetContent = 0;
		m_sizeContent = 0;
	}

	Memory PdfStream::getDecodedContent(PdfContentReader* reader)
	{
		return getDecodedContent(getEncodedContent(reader));
	}

	Memory PdfStream::getDecodedContent(const Memory& content)
	{
		if (content.isNull()) {
			return sl_null;
		}
		PdfValue vFilter = getProperty(g_strFilter, g_strF);
		if (vFilter.isUndefined()) {
			return content;
		}
		PdfValue decodeParams = getProperty(g_strDecodeParms, g_strDP);
		const PdfArray& arrayFilter = vFilter.getArray();
		if (arrayFilter.isNotNull()) {
			ListElements<PdfValue> filters(arrayFilter);
			if (filters.count) {
				Memory ret = content;
				const PdfArray& arrDecodeParams = decodeParams.getArray();
				for (sl_size i = 0; i < filters.count; i++) {
					PdfFilter filter = Pdf::getFilter(filters[i].getName());
					if (filter != PdfFilter::Unknown) {
						ret = DecodeStreamContent(ret, filter, arrDecodeParams.getValueAt_NoLock(i).getDictionary());
					} else {
						return sl_null;
					}
				}
				return ret;
			}
		} else {
			PdfFilter filter = Pdf::getFilter(vFilter.getName());
			if (filter != PdfFilter::Unknown) {
				return DecodeStreamContent(content, filter, decodeParams.getDictionary());
			}
		}
		return sl_null;
	}


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(PdfFlateOrLZWDecodeParams)

	PdfFlateOrLZWDecodeParams::PdfFlateOrLZWDecodeParams() : predictor(0), columns(0), bitsPerComponent(8), colors(1), earlyChange(1)
	{
	}

	void PdfFlateOrLZWDecodeParams::setParams(const PdfDictionary& dict) noexcept
	{
		dict.getValue_NoLock(g_strPredictor).getUint(predictor);
		dict.getValue_NoLock(g_strColumns).getUint(columns);
		dict.getValue_NoLock(g_strBitsPerComponent).getUint(bitsPerComponent);
		dict.getValue_NoLock(g_strColors).getUint(colors);
		dict.getValue_NoLock(g_strEarlyChange).getUint(earlyChange);
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

	void PdfCCITTFaxDecodeParams::setParams(const PdfDictionary& dict) noexcept
	{
		dict.getValue_NoLock(g_strK).getInt(K);
		dict.getValue_NoLock(g_strColumns).getUint(columns);
		dict.getValue_NoLock(g_strRows).getUint(rows);
		dict.getValue_NoLock(g_strEndOfLine).getBoolean(flagEndOfLine);
		dict.getValue_NoLock(g_strEncodedByteAlign).getBoolean(flagByteAlign);
		dict.getValue_NoLock(g_strBlackIs1).getBoolean(flagBlackIs1);
	}


	PdfResourceContext::PdfResourceContext()
	{
	}

	PdfResourceContext::~PdfResourceContext()
	{
	}


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(PdfColorSpace)

	PdfColorSpace::PdfColorSpace(): type(PdfColorSpaceType::Unknown)
	{
	}

	void PdfColorSpace::load(PdfPage* page, const String& name)
	{
		if (_loadName(name)) {
			return;
		}
		Ref<PdfDocument> doc = page->getDocument();
		if (doc.isNotNull()) {
			load(doc.get(), page->getResource(g_strColorSpace, name, sl_false));
		}
	}

	void PdfColorSpace::load(PdfDocument* doc, const PdfValue& _value)
	{
		PdfValue value;
		if (doc) {
			value = doc->getObject(_value);
		} else {
			value = _value;
		}
		if (value.isNull()) {
			return;
		}
		const String& str = value.getName();
		if (_loadName(str)) {
			return;
		}
		ListElements<PdfValue> arr(value.getArray());
		if (!(arr.count)) {
			return;
		}
		const String& name = arr[0].getName();
		if (name == StringView::literal("CalRGB")) {
			type = PdfColorSpaceType::RGB;
		} else if (name == StringView::literal("CalGray")) {
			type = PdfColorSpaceType::Gray;
		} else if (name ==  StringView::literal("CalCMYK")) {
			type = PdfColorSpaceType::CMYK;
		} else if (name == StringView::literal("Lab")) {
			type = PdfColorSpaceType::Lab;
		} else if (name == StringView::literal("Indexed") || name == StringView::literal("I")) {
			if (arr.count >= 4) {
				if (_loadIndexed(doc, arr[2].getUint(), arr[3])) {
					type = PdfColorSpaceType::Indexed;
				}
			}
		} else if (doc && name == StringView::literal("ICCBased")) {
			if (arr.count >= 2) {
				Ref<PdfStream> stream = doc->getObject(arr[1]).getStream();
				if (stream.isNotNull()) {
					load(sl_null, stream->getProperty(g_strAlternate));
				}
			}
		} else if (name == StringView::literal("Pattern")) {
			if (arr.count >= 2) {
				load(doc, arr[1]);
			}
		} else if (name == StringView::literal("Separation")) {
			if (arr.count >= 3) {
				load(doc, arr[2]);
			}
		} else if (name == StringView::literal("DeviceN")) {
			if (arr.count >= 3) {
				load(doc, arr[2]);
			}
		}
	}

	sl_uint32 PdfColorSpace::getComponentsCount()
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
		if (count < 4) {
			return sl_false;
		}
		float l = values[0].getFloat();
		float a = values[1].getFloat();
		float b = values[2].getFloat();
		sl_uint8 k = (sl_uint8)(values[3].getFloat() * 255);
		Color3f c;
		CIE::convertLabToRGB(l, a, b, c.x, c.y, c.z);
		_out = c;
		return sl_true;
	}

	sl_bool PdfColorSpace::_loadName(const String& name)
	{
		if (name == StringView::literal("DeviceRGB") || name == StringView::literal("RGB")) {
			type = PdfColorSpaceType::RGB;
		} else if (name == StringView::literal("DeviceGray") || name == StringView::literal("G")) {
			type = PdfColorSpaceType::Gray;
		} else if (name == StringView::literal("DeviceCMYK") || name == StringView::literal("CMYK")) {
			type = PdfColorSpaceType::CMYK;
		} else if (name == StringView::literal("Pattern")) {
			type = PdfColorSpaceType::Pattern;
		} else {
			return sl_false;
		}
		return sl_true;
	}

	sl_bool PdfColorSpace::_loadIndexed(PdfDocument* doc, sl_uint32 maxIndex, const PdfValue& vTable)
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
			memTable = doc->decodeStreamContent(vTable);
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



	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(PdfFontDescriptor)

	PdfFontDescriptor::PdfFontDescriptor(): ascent(0), descent(0), leading(0), weight(0), italicAngle(0), flags(0), content(0, 0)
	{
	}

	void PdfFontDescriptor::load(const PdfDictionary& desc) noexcept
	{
		if (desc.isNotNull()) {
			name = desc.getValue_NoLock(g_strFontName).getName();
			family = desc.getValue_NoLock(g_strFontFamily).getString();
			ascent = desc.getValue_NoLock(g_strAscent).getFloat();
			descent = desc.getValue_NoLock(g_strDescent).getFloat();
			leading = desc.getValue_NoLock(g_strLeading).getFloat();
			weight = desc.getValue_NoLock(g_strFontWeight).getFloat();
			italicAngle = desc.getValue_NoLock(g_strItalicAngle).getFloat();
			flags = desc.getValue_NoLock(g_strFlags).getUint();
			if (!(desc.getValue_NoLock(g_strFontFile).getReference(content))) {
				if (!(desc.getValue_NoLock(g_strFontFile2).getReference(content))) {
					desc.getValue_NoLock(g_strFontFile3).getReference(content);
				}
			}
		}
	}


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(PdfCidFontInfo)

	PdfCidFontInfo::PdfCidFontInfo(): subtype(PdfFontSubtype::Unknown), defaultWidth(1000), flagCidIsGid(sl_false)
	{
	}

	void PdfCidFontInfo::load(PdfDocument* doc, const PdfDictionary& dict) noexcept
	{
		if (dict.isNotNull()) {
			subtype = PdfFontResource::getSubtype(dict.getValue_NoLock(g_strSubtype).getName());
			dict.getValue_NoLock(g_strDW).getFloat(defaultWidth);
			{
				ListElements<PdfValue> w(doc->getObject(dict.getValue_NoLock(g_strW)).getArray());
				PdfValue* current = w.data;
				PdfValue* end = current + w.count;
				while (current < end) {
					sl_uint32 code;
					if (!(current->getUint(code))) {
						break;
					}
					current++;
					if (current >= end) {
						break;
					}
					const PdfArray& arr = current->getArray();
					if (arr.isNotNull()) {
						current++;
						ListElements<PdfValue> m(arr);
						for (sl_size i = 0; i < m.count; i++) {
							float width;
							if (m[i].getFloat(width)) {
								widths.put_NoLock(code + (sl_uint32)i, width);
							} else {
								break;
							}
						}
					} else {
						sl_uint32 code2;
						if (!(current->getUint(code2))) {
							break;
						}
						current++;
						if (current >= end) {
							break;
						}
						float width;
						if (current->getFloat(width)) {
							for (sl_uint32 i = code; i <= code2; i++) {
								widths.put_NoLock(i, width);
							}
						} else {
							break;
						}
						current++;
					}
				}
			}
			{
				const PdfValue& vCIDToGIDMap = dict.getValue(g_strCIDToGIDMap);
				cidToGidMapName = vCIDToGIDMap.getName();
				if (cidToGidMapName == StringView::literal("Identity")) {
					flagCidIsGid = sl_true;
				}
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

	PdfFontResource::PdfFontResource(): subtype(PdfFontSubtype::Unknown), firstChar(0), lastChar(0), encoding(PdfEncoding::PdfDoc), codeLength(1)
	{
	}

	sl_bool PdfFontResource::load(PdfDocument* doc, const PdfDictionary& dict)
	{
		if (dict.isNull()) {
			return sl_false;
		}
		subtype = PdfFontResource::getSubtype(dict.getValue_NoLock(g_strSubtype).getName());
		baseFont = dict.getValue_NoLock(g_strBaseFont).getName();
		if (subtype == PdfFontSubtype::Type0) {
			const PdfDictionary& cidFont = doc->getObject(doc->getObject(dict.getValue_NoLock(g_strDescendantFonts)).getArray().getValueAt(0)).getDictionary();
			if (cidFont.isNull()) {
				return sl_false;
			}
			cid.load(doc, cidFont);
			descriptor.load(doc->getObject(cidFont.getValue_NoLock(g_strFontDescriptor)).getDictionary());
		} else {
			descriptor.load(doc->getObject(dict.getValue_NoLock(g_strFontDescriptor)).getDictionary());
		}
		PdfValue vEncoding = dict.getValue_NoLock(g_strEncoding);
		const String& encodingName = vEncoding.getName();
		if (encodingName.isNotNull()) {
			encoding = Pdf::getEncoding(encodingName);
		} else {
			const PdfDictionary& dict = doc->getObject(vEncoding).getDictionary();
			if (dict.isNotNull()) {
				encoding = Pdf::getEncoding(dict.getValue_NoLock(g_strBaseEncoding).getString());
				PdfArray array = dict.getValue_NoLock(g_strDifferences).getArray();
				sl_uint32 n = (sl_uint32)(array.getCount());
				if (n >= 2) {
					PdfValue* diff = array.getData();
					sl_uint32 code;
					if (diff[0].getUint(code)) {
						for (sl_uint32 i = 1; i < n; i++) {
							PdfValue& v = diff[i];
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
		firstChar = dict.getValue_NoLock(g_strFirstChar).getUint();
		lastChar = dict.getValue_NoLock(g_strLastChar).getUint();
		{
			ListElements<PdfValue> arr(dict.getValue_NoLock(g_strWidths).getArray());
			if (arr.count == lastChar - firstChar + 1) {
				widths = Array<float>::create(arr.count);
				if (widths.isNotNull()) {
					float* f = widths.getData();
					for (sl_size i = 0; i < arr.count; i++) {
						f[i] = arr[i].getFloat();
					}
				}
			}
		}
		const PdfValue& vToUnicode = dict.getValue_NoLock(g_strToUnicode);
		if (vToUnicode.isNotNull()) {
			Memory content = doc->decodeStreamContent(vToUnicode);
			if (content.isNotNull()) {
				sl_uint32 n = ParseCMap(content.getData(), content.getSize(), toUnicode);
				if (n == 2) {
					codeLength = 2;
				}
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
		m_cacheGlyphs.setExpiringMilliseconds(EXPIRE_DURATION_FONT_BITMAP);
	}

	PdfFont::~PdfFont()
	{
	}

	Ref<PdfFont> PdfFont::load(PdfDocument* doc, const PdfReference& ref, PdfResourceContext& context)
	{
		Ref<PdfFont> ret;
		if (context.fonts.get(ref.objectNumber, &ret)) {
			return ret;
		}
		const PdfDictionary& dict = doc->getObject(ref).getDictionary();
		if (dict.isNotNull()) {
			ret = new PdfFont;
			if (ret.isNotNull()) {
				if (ret->_load(doc, dict)) {
					context.fonts.put(ref.objectNumber, ret);
					return ret;
				}
			}
		}
		return sl_null;
	}

	sl_bool PdfFont::_load(PdfDocument* doc, const PdfDictionary& dict)
	{
		if (!(PdfFontResource::load(doc, dict))) {
			return sl_false;
		}
		if (descriptor.content.objectNumber) {
			Memory content = doc->decodeStreamContent(descriptor.content);
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
							return face->getGlyphIndex(names[charcode]);
						}
					}
				}
				return 0;
			} else {
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


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(PdfImageResource)

	PdfImageResource::PdfImageResource(): width(0), height(0), bitsPerComponent(8), flagImageMask(sl_false), flagInterpolate(sl_false), flagUseDecodeArray(sl_false), flagUseMatte(sl_false), mask(0, 0), smask(0, 0)
	{
	}

	sl_bool PdfImageResource::load(PdfDocument* doc, PdfStream* stream) noexcept
	{
		const String& subtype = stream->getProperty(g_strSubtype).getName();
		if (subtype == StringView::literal("Image")) {
			stream->getProperty(g_strWidth, g_strW).getUint(width);
			stream->getProperty(g_strHeight, g_strH).getUint(height);
			stream->getProperty(g_strInterpolate, g_strI).getBoolean(flagInterpolate);
			stream->getProperty(g_strImageMask, g_strIM).getBoolean(flagImageMask);
			if (flagImageMask) {
				bitsPerComponent = 1;
				colorSpace.type = PdfColorSpaceType::Gray;
			} else {
				colorSpace.load(doc, stream->getProperty(g_strColorSpace, g_strCS));
				stream->getProperty(g_strBitsPerComponent, g_strBPC).getUint(bitsPerComponent);
				stream->getProperty(g_strMask).getReference(mask);
			}
			ListElements<PdfValue> arrayDecode(stream->getProperty(g_strDecode, g_strD).getArray());
			if (arrayDecode.count) {
				switch (colorSpace.type) {
					case PdfColorSpaceType::RGB:
					case PdfColorSpaceType::Gray:
					case PdfColorSpaceType::CMYK:
						if (flagImageMask) {
							if (arrayDecode.count >= 2) {
								flagUseDecodeArray = sl_true;
								decodeMin[0] = arrayDecode[0].getUint();
								decodeMax[0] = arrayDecode[1].getUint();
							}
						} else {
							sl_uint32 nColors = colorSpace.getComponentsCount();
							if (arrayDecode.count >= nColors * 2) {
								flagUseDecodeArray = sl_true;
								for (sl_uint32 i = 0; i < nColors; i++) {
									decodeMin[i] = Math::clamp0_255((sl_int32)(arrayDecode[i << 1].getFloat() * 255));
									decodeMax[i] = Math::clamp0_255((sl_int32)(arrayDecode[(i << 1) | 1].getFloat() * 255));
								}
								if (nColors == 1) {
									decodeMin[2] = decodeMin[1] = decodeMin[0];
									decodeMax[2] = decodeMax[1] = decodeMax[0];
								}
							}
						}
						break;
					case PdfColorSpaceType::Indexed:
						if (arrayDecode.count == 2) {
							sl_uint32 n = (sl_uint32)(colorSpace.indices.getCount());
							if (n) {
								sl_uint32 m0 = arrayDecode[0].getUint();
								sl_uint32 m1 = arrayDecode[1].getUint();
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
			PdfArray arrayMatte = stream->getProperty(g_strMatte).getArray();
			if (arrayMatte.isNotNull()) {
				if (colorSpace.getColor(matte, arrayMatte.getData(), arrayMatte.getCount())) {
					if (matte != Color::Black) {
						flagUseMatte = sl_true;
					}
				}
			}
			stream->getProperty(g_strSMask).getReference(smask);
			return sl_true;
		}
		return sl_false;
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

	
	SLIB_DEFINE_ROOT_OBJECT(PdfImage)

	PdfImage::PdfImage()
	{
	}

	PdfImage::~PdfImage()
	{
	}

	Ref<PdfImage> PdfImage::load(PdfDocument* doc, const PdfReference& ref, PdfResourceContext& context)
	{
		Ref<PdfImage> ret;
		if (context.images.get(ref.objectNumber, &ret)) {
			return ret;
		}
		ret = new PdfImage;
		if (ret.isNotNull()) {
			if (ret->_load(doc, ref)) {
				context.images.put(ref.objectNumber, ret);
				return ret;
			}
		}
		return sl_null;
	}

	sl_bool PdfImage::_load(PdfDocument* doc, const PdfReference& ref)
	{
		Ref<PdfStream> stream = doc->getObject(ref).getStream();
		if (stream.isNotNull()) {
			if (PdfImageResource::load(doc, stream.get())) {
				Memory content = stream->getDecodedContent(doc);
				if (content.isNotNull()) {
					Referable* ref = content.getRef();
					if (IsInstanceOf<Image>(ref)) {
						object = (Image*)ref;
					} else {
						if (colorSpace.type == PdfColorSpaceType::RGB || colorSpace.type == PdfColorSpaceType::Gray || colorSpace.type == PdfColorSpaceType::CMYK || colorSpace.type == PdfColorSpaceType::Indexed) {
							sl_uint32 nColors = colorSpace.getComponentsCount();
							if (nColors) {
								sl_uint8* data = (sl_uint8*)(content.getData());
								sl_uint32 pitch = (nColors * bitsPerComponent * width + 7) >> 3;
								sl_uint32 height = (sl_uint32)(content.getSize()) / pitch;
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
									if (object.isNotNull() && colorSpace.type != PdfColorSpaceType::CMYK) {
										applyDecode(object.get());
									}
								}
							}
						}
					}
					if (object.isNotNull()) {
						_loadSMask(doc);
						return sl_true;
					}
				}
			}
		}
		return sl_false;
	}

	void PdfImage::_loadSMask(PdfDocument* doc)
	{
		if (object.isNull()) {
			return;
		}
		if (!(smask.objectNumber)) {
			return;
		}
		Ref<PdfStream> stream = doc->getObject(smask).getStream();
		if (stream.isNull()) {
			return;
		}
		PdfImageResource maskDesc;
		if (!(maskDesc.load(doc, stream.get()))) {
			return;
		}
		sl_uint32 widthMask = maskDesc.width;
		if (!(widthMask && maskDesc.bitsPerComponent && maskDesc.colorSpace.type == PdfColorSpaceType::Gray)) {
			return;
		}
		Memory content = stream->getDecodedContent(doc);
		if (content.isNull()) {
			return;
		}
		sl_uint32 pitchMask = (maskDesc.bitsPerComponent * widthMask + 7) >> 3;
		sl_uint32 heightMask = (sl_uint32)(content.getSize()) / pitchMask;
		if (!heightMask) {
			return;
		}
		sl_uint32 widthParent = object->getWidth();
		sl_uint32 heightParent = object->getHeight();
		if (widthParent < widthMask || heightParent < heightMask) {
			if (widthParent < widthMask) {
				widthParent = widthMask;
			}
			if (heightParent < heightMask) {
				heightParent = heightMask;
			}
			Ref<Image> image = object->stretch(widthParent, heightParent, StretchMode::Linear);
			if (image.isNull()) {
				return;
			}
			object = Move(image);
		}
		if (maskDesc.flagInterpolate) {
			flagInterpolate = sl_true;
		}
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
			widthMask = widthParent;
			heightMask = heightParent;
		}
		maskDesc.applyDecode(imageMask.get());
		Color* rowDst = object->getColors();
		Color* rowSrc = imageMask->getColors();
		sl_reg strideDst = object->getStride();
		sl_reg strideSrc = object->getStride();
		if (maskDesc.flagUseMatte) {
			for (sl_uint32 y = 0; y < heightMask; y++) {
				Color* dst = rowDst;
				Color* src = rowSrc;
				for (sl_uint32 x = 0; x < widthMask; x++) {
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
			for (sl_uint32 y = 0; y < heightMask; y++) {
				Color* dst = rowDst;
				Color* src = rowSrc;
				for (sl_uint32 x = 0; x < widthMask; x++) {
					dst->a = (sl_uint8)(((sl_uint32)(dst->a) * (sl_uint32)(src->r)) >> 8);
					dst++;
					src++;
				}
				rowDst += strideDst;
				rowSrc += strideSrc;
			}
		}
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

	PdfValue PdfPageTreeItem::getAttribute(const String& name)
	{
		PdfValue ret = attributes.getValue_NoLock(name);
		if (ret.isNotUndefined()) {
			return ret;
		}
		Ref<PdfPageTreeItem> _parent(parent);
		if (_parent.isNotNull()) {
			return _parent->getAttribute(name);
		}
		return PdfValue();
	}


	PdfRenderContext::PdfRenderContext()
	{
	}

	PdfRenderContext::~PdfRenderContext()
	{
	}
	

	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(PdfRenderParam)

	PdfRenderParam::PdfRenderParam(): canvas(sl_null)
	{
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

	Memory PdfPage::getContentData()
	{
		Ref<PdfDocument> document(m_document);
		if (document.isNotNull()) {
			ObjectLocker locker(document.get());
			Context* context = GetContext(document->m_context);
			if (context) {
				return context->getPageContent(attributes.getValue_NoLock(g_strContents));
			}
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
		MemoryContext context;
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
				ret.add_NoLock(Move(opCurrent));
				opCurrent.operands.setNull();
			} else {
				PdfValue value = context.readValue();
				if (value.isUndefined()) {
					break;
				}
				opCurrent.operands.add_NoLock(Move(value));
			}
		}
		return ret;
	}

	void PdfPage::render(PdfRenderParam& param)
	{
		Canvas* canvas = param.canvas;
		Rectangle bounds = param.bounds;
		canvas->fillRectangle(bounds, Color::White);

		ListElements<PdfOperation> ops(getContent());
		if (!(ops.count)) {
			return;
		}

		if (param.context.isNull()) {
			param.context = new PdfRenderContext;
			if (param.context.isNull()) {
				return;
			}
		}

		sl_bool flagOldAntiAlias = canvas->isAntiAlias();
		canvas->setAntiAlias();

		Renderer renderer;
		renderer.canvas = canvas;
		renderer.page = this;
		renderer.param = &param;

		CanvasStateScope scope(canvas);
		Swap(bounds.top, bounds.bottom);
		canvas->concatMatrix(Transform2::getTransformMatrixFromRectToRect(getMediaBox(), bounds));
		canvas->clipToRectangle(getCropBox());

		for (sl_size i = 0; i < ops.count; i++) {
			renderer.render(ops[i]);
		}

		canvas->setAntiAlias(flagOldAntiAlias);
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

	PdfValue PdfPage::getResources(const String& type, sl_bool flagResolveReference)
	{
		Ref<PdfDocument> doc(m_document);
		if (doc.isNotNull()) {
			ObjectLocker locker(doc.get());
			Context* context = GetContext(doc->m_context);
			if (context) {
				Ref<PdfPageTreeItem> item = this;
				for (;;) {
					const PdfDictionary& dict = context->getObject(item->attributes.getValue_NoLock(g_strResources)).getDictionary();
					if (dict.isNotNull()) {
						PdfValue ret = dict.getValue_NoLock(type);
						if (ret.isNotUndefined()) {
							if (flagResolveReference) {
								return context->getObject(ret);
							} else {
								return ret;
							}
						}
					}
					item = item->parent;
					if (item.isNull()) {
						break;
					}
				}
			}
		}
		return PdfValue();
	}

	PdfValue PdfPage::getResource(const String& type, const String& name, sl_bool flagResolveReference)
	{
		Ref<PdfDocument> doc(m_document);
		if (doc.isNotNull()) {
			ObjectLocker locker(doc.get());
			Context* context = GetContext(doc->m_context);
			if (context) {
				Ref<PdfPageTreeItem> item = this;
				for (;;) {
					const PdfDictionary& dict = context->getObject(item->attributes.getValue_NoLock(g_strResources)).getDictionary();
					if (dict.isNotNull()) {
						PdfValue ret = dict.getValue_NoLock(type).getDictionary().getValue_NoLock(name);
						if (ret.isNotUndefined()) {
							if (flagResolveReference) {
								return context->getObject(ret);
							} else {
								return ret;
							}
						}
					}
					item = item->parent;
					if (item.isNull()) {
						break;
					}
				}
			}
		}
		return PdfValue();
	}

	PdfDictionary PdfPage::getFontResourceAsDictionary(const String& name)
	{
		return getResource(g_strFont, name).getDictionary();
	}

	sl_bool PdfPage::getFontResource(const String& name, PdfReference& outRef)
	{
		return getResource(g_strFont, name, sl_false).getReference(outRef);
	}

	sl_bool PdfPage::getFontResource(const String& name, PdfFontResource& resource)
	{
		Ref<PdfDocument> doc(m_document);
		if (doc.isNotNull()) {
			return resource.load(doc.get(), getFontResourceAsDictionary(name));
		}
		return sl_false;
	}

	PdfValue PdfPage::getExternalObjectResource(const String& name)
	{
		return getResource(g_strXObject, name);
	}

	sl_bool PdfPage::getExternalObjectResource(const String& name, PdfReference& outRef)
	{
		return getResource(g_strXObject, name, sl_false).getReference(outRef);
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
		Ref<FileIO> file = FileIO::openForRead(filePath);
		if (file.isNotNull()) {
			// size
			{
				sl_uint64 size = file->getSize();
				if (!size) {
					return sl_false;
				}
				if (size > MAX_PDF_FILE_SIZE) {
					return sl_false;
				}
				fileSize = (sl_uint32)size;
			}
			Ref<BufferedContext> context = New<BufferedContext>();
			if (context.isNotNull()) {
				if (context->reader.open(file)) {
					if (context->readDocument()) {
						m_context = Move(context);
						return sl_true;
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
		Ref<MemoryContext> context = New<MemoryContext>();
		if (context.isNotNull()) {
			context->source = (sl_char8*)(mem.getData());
			context->sizeSource = fileSize;
			context->refSource = mem.getRef();
			if (context->readDocument()) {
				m_context = Move(context);
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

	PdfValue PdfDocument::getObject(const PdfReference& ref)
	{
		ObjectLocker lock(this);
		Context* context = GetContext(m_context);
		if (context) {
			return context->getObject(ref);
		}
		return PdfValue();
	}

	PdfValue PdfDocument::getObject(const PdfValue& refOrObj)
	{
		ObjectLocker lock(this);
		Context* context = GetContext(m_context);
		if (context) {
			return context->getObject(refOrObj);
		}
		return PdfValue();
	}

	Memory PdfDocument::readContent(sl_uint32 offset, sl_uint32 size, const PdfReference& ref)
	{
		ObjectLocker lock(this);
		Context* context = GetContext(m_context);
		if (context) {
			return context->readContent(offset, size, ref);
		}
		return sl_null;
	}

	Memory PdfDocument::decodeStreamContent(PdfStream* stream)
	{
		return stream->getDecodedContent(this);
	}

	Memory PdfDocument::decodeStreamContent(const PdfReference& ref)
	{
		ObjectLocker lock(this);
		Context* context = GetContext(m_context);
		if (context) {
			return context->decodeStreamContent(ref);
		}
		return sl_null;
	}

	Memory PdfDocument::decodeStreamContent(const PdfValue& stream)
	{
		ObjectLocker lock(this);
		Context* context = GetContext(m_context);
		if (context) {
			return context->decodeStreamContent(stream);
		}
		return sl_null;
	}

	List< Ref<PdfStream> > PdfDocument::getAllStreams()
	{
		List< Ref<PdfStream> > ret;
		ObjectLocker lock(this);
		Context* context = GetContext(m_context);
		if (context) {
			CrossReferenceEntry* refTable = context->references.getData();
			sl_size nRefs = context->references.getCount();
			for (sl_size i = 0; i < nRefs; i++) {
				CrossReferenceEntry& entry = refTable[i];
				if (entry.type != (sl_uint32)(CrossReferenceEntryType::Free)) {
					PdfReference n;
					sl_uint32 offsetEnd;
					Ref<PdfStream> stream = context->readObject(entry.offset, offsetEnd, n).getStream();
					if (stream.isNotNull()) {
						ret.add_NoLock(stream);
					}
				}
			}
		}
		return ret;
	}

	sl_uint32 PdfDocument::getPagesCount()
	{
		ObjectLocker lock(this);
		Context* context = GetContext(m_context);
		if (context) {
			PageTreeParent* tree = context->pageTree.get();
			if (tree) {
				return tree->getPagesCount();
			}
		}
		return 0;
	}

	Ref<PdfPage> PdfDocument::getPage(sl_uint32 index)
	{
		ObjectLocker lock(this);
		Context* context = GetContext(m_context);
		if (context) {
			Ref<PdfPage> page = context->getPage(index);
			if (page.isNotNull()) {
				page->m_document = this;
				return page;
			}
		}
		return sl_null;
	}

	sl_bool PdfDocument::isEncrypted()
	{
		Context* context = GetContext(m_context);
		if (context) {
			return context->encrypt.isNotNull();
		}
		return sl_false;
	}

	sl_bool PdfDocument::isAuthenticated()
	{
		if (isEncrypted()) {
			Context* context = GetContext(m_context);
			if (context) {
				return context->flagDecryptContents;
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
		Context* context = GetContext(m_context);
		if (context) {
			return context->setUserPassword(password);
		}
		return sl_false;
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

	PdfFilter Pdf::getFilter(const StringView& name) noexcept
	{
		if (name == StringView::literal("FlateDecode") || name == StringView::literal("Fl")) {
			return PdfFilter::Flate;
		} else if (name == StringView::literal("DCTDecode") || name == StringView::literal("DCT")) {
			return PdfFilter::DCT;
		} else if (name == StringView::literal("LZWDecode") || name == StringView::literal("LZW")) {
			return PdfFilter::LZW;
		} else if (name == StringView::literal("RunLengthDecode") || name == StringView::literal("RL")) {
			return PdfFilter::RunLength;
		} else if (name == StringView::literal("ASCIIHexDecode") || name == StringView::literal("AHx")) {
			return PdfFilter::ASCIIHex;
		} else if (name == StringView::literal("ASCII85Decode") || name == StringView::literal("A85")) {
			return PdfFilter::ASCII85;
		} else if (name == StringView::literal("CCITTFaxDecode") || name == StringView::literal("CCF")) {
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

}
