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
#include "slib/core/mio.h"
#include "slib/core/memory_buffer.h"
#include "slib/core/string_buffer.h"
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
			SLIB_STATIC_STRING(g_strBaseFont, "BaseFont")
			SLIB_STATIC_STRING(g_strDescendantFonts, "DescendantFonts")
			SLIB_STATIC_STRING(g_strEncoding, "Encoding")
			SLIB_STATIC_STRING(g_strBaseEncoding, "BaseEncoding")
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
			SLIB_STATIC_STRING(g_strDW, "DW")
			SLIB_STATIC_STRING(g_strToUnicode, "ToUnicode")
			SLIB_STATIC_STRING(g_strWidth, "Width")
			SLIB_STATIC_STRING(g_strHeight, "Height")
			SLIB_STATIC_STRING(g_strColorSpace, "ColorSpace")
			SLIB_STATIC_STRING(g_strBitsPerComponent, "BitsPerComponent")
			SLIB_STATIC_STRING(g_strPredictor, "Predictor")
			SLIB_STATIC_STRING(g_strColors, "Colors")
			SLIB_STATIC_STRING(g_strColumns, "Columns")
			SLIB_STATIC_STRING(g_strSMask, "SMask")
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
							} else if (!(IsWhitespace(ch)) && ch != '>') {
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
								} else if (!(IsWhitespace(v)) && v != '>') {
									return sl_null;
								}
							}
						}
					}
				} else if (filter == StringView::literal("DCTDecode") || filter == StringView::literal("DCT")) {
					return input;
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
				
			public:
				PdfObject getObject(const PdfObject& obj)
				{
					PdfReference ref;
					if (obj.getReference(ref)) {
						return getObject(ref);
					}
					return obj;
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
				using READER_BASE::peek;
				using READER_BASE::getPosition;
				using READER_BASE::setPosition;
				using READER_BASE::movePosition;
				using READER_BASE::readBuffer;
				using READER_BASE::findBackward;
				using ParserBase::getObject;

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
					sl_size pos = getPosition();
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

				PdfObject readValue()
				{
					sl_char8 ch;
					if (!(peekChar(ch))) {
						return PdfObject();
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
										sl_size pos = getPosition();
										sl_uint32 length;
										if (!(getStreamLength(properties, length))) {
											return PdfObject();
										}
										setPosition(pos); // Protect position while getting stream length
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
										stream->contentUnfiltered = Move(content);
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
									Memory content = obj->getContent();
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
											Memory content = stream->getContent();
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
							subTable.context = context;
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
							context->references = Array<CrossReferenceEntry>::create(countTotalRef);
							if (context->references.isNull()) {
								return sl_false;
							}
							Base::zeroMemory(context->references.getData(), countTotalRef * sizeof(CrossReferenceEntry));
						}
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

				List< Ref<PdfPageTreeItem> > buildPageTreeKids(const PdfDictionary& attributes)
				{
					List< Ref<PdfPageTreeItem> > ret;
					ListElements<PdfObject> kidIds(attributes.getValue_NoLock(g_strKids).getArray());
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
					const PdfArray& array = contents.getArray();
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

			static HashMap<sl_uint16, String32> ParseCMap(const void* _content, sl_size size)
			{
				sl_uint8* content = Base::findMemory(_content, size, "begincmap", 9);
				if (!content) {
					return sl_null;
				}
				content += 9;
				size -= 9;
				HashMap<sl_uint16, String32> ret;
				MemoryParser parser;
				parser.source = (sl_char8*)content;
				parser.sizeSource = (sl_uint32)size;
				CList<PdfObject> args;
				for (;;) {
					if (!(parser.skipWhitespaces())) {
						break;
					}
					PdfCMapOperator op = parser.readCMapOperator();
					if (op != PdfCMapOperator::Unknown) {
						switch (op) {
						case PdfCMapOperator::endbfchar:
							{
								PdfObject* m = args.getData();
								PdfObject* end = m + args.getCount();
								while (m + 1 < end) {
									const String& strCode = (m++)->getString();
									if (strCode.getLength() == 2) {
										sl_char8* s = strCode.getData();
										sl_uint16 code = SLIB_MAKE_WORD(s[0], s[1]);
										const String& strValue = (m++)->getString();
										if (strValue.isNotNull()) {
											String32 value = String32::fromUtf16BE(strValue.getData(), strValue.getLength());
											if (value.isNotEmpty()) {
												ret.put_NoLock(code, Move(value));
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
						case PdfCMapOperator::endbfrange:
							{
								PdfObject* m = args.getData();
								PdfObject* end = m + args.getCount();
								while (m + 2 < end) {
									const String& strCode1 = (m++)->getString();
									const String& strCode2 = (m++)->getString();
									if (strCode1.getLength() == 2 && strCode2.getLength() == 2) {
										sl_char8* s = strCode1.getData();
										sl_uint16 code1 = SLIB_MAKE_WORD(s[0], s[1]);
										s = strCode2.getData();
										sl_uint16 code2 = SLIB_MAKE_WORD(s[0], s[1]);
										const String& strValue = m->getString();
										if (strValue.isNotNull()) {
											String32 value = String32::fromUtf16BE(strValue.getData(), strValue.getLength());
											if (value.isNotEmpty()) {
												sl_char32* k = value.getData() + value.getLength() - 1;
												for (sl_uint16 code = code1; code <= code2; code++) {
													ret.put_NoLock(code, value.duplicate());
													(*k)++;
												}
											} else {
												break;
											}
										} else {
											ListElements<PdfObject> arr(m->getArray());
											if (arr.count == code2 - code1 + 1) {
												for (sl_size i = 0; i < arr.count; i++) {
													const String& strValue = arr[i].getString();
													String32 value = String32::fromUtf16BE(strValue.getData(), strValue.getLength());
													if (value.isNotEmpty()) {
														ret.put_NoLock(code1 + (sl_uint16)i, Move(value));
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
								}
							}
							break;
						default:
							break;
						}
						args.removeAll_NoLock();
					} else {
						PdfObject obj = parser.readValue();
						if (obj.isUndefined()) {
							break;
						}
						args.add_NoLock(Move(obj));
					}
				}
				return ret;
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
				sl_uint32 nRows = sizeData / (sizeRow + 1);
				if (!nRows) {
					return sl_false;
				}
				sizeData = sizeRow * nRows;
				sl_uint8* pRowDst = bufData;
				sl_uint8* pRowSrc = bufData;
				for (sl_uint32 iRow = 0; iRow < nRows; iRow++) {
					sl_uint8 tag = *pRowSrc;
					if (tag) {
						for (sl_uint32 i = 0; i < sizeRow; i++) {
							sl_uint8 diff = pRowSrc[i + 1];
							sl_uint8 base = 0;
							switch (tag) {
								case 1:
									if (i >= nBytesPerPixel) {
										base = pRowSrc[i - nBytesPerPixel];
									}
									break;
								case 2:
									if (iRow) {
										base = pRowSrc[i - sizeRow];
									}
									break;
								case 3:
									{
										sl_uint8 left = 0;
										if (i >= nBytesPerPixel) {
											left = pRowSrc[i - nBytesPerPixel];
										}
										sl_uint8 up = 0;
										if (iRow) {
											up = pRowSrc[i - sizeRow];
										}
										base = (sl_uint8)(((sl_uint32)up + (sl_uint32)left) >> 1);
									}
									break;
								case 4:
									{
										sl_uint8 left = 0, upperLeft = 0;
										if (i >= nBytesPerPixel) {
											left = pRowSrc[i - nBytesPerPixel];
											if (iRow) {
												upperLeft = pRowSrc[i - nBytesPerPixel - sizeRow];
											}
										}
										sl_uint8 up = 0;
										if (iRow) {
											up = pRowSrc[i - sizeRow];
										}
										base = PredictPath(left, up, upperLeft);
									}
									break;
							}
							pRowDst[i] = base + diff;
						}
					} else {
						Base::moveMemory(pRowDst, pRowSrc + 1, sizeRow);
					}
					pRowSrc += sizeRow + 1;
					pRowDst += sizeRow;
				}
				return sl_true;
			}

			sl_bool PredictTIFF(sl_uint8* bufData, sl_uint32& sizeData, sl_uint32 colors, sl_uint32 bitsPerComponent, sl_uint32 columns) {
				sl_uint32 sizeRow = (colors * bitsPerComponent * columns + 7) >> 3;
				if (!sizeRow) {
					return sl_false;
				}
				sl_uint32 nRows = sizeData / sizeRow;
				sizeData = sizeRow * nRows;
				sl_uint32 nBitsRow = sizeRow << 3;
				sl_uint32 nBytesPerPixel = (bitsPerComponent * colors) >> 3;
				sl_uint8* row = bufData;
				for (sl_uint32 iRow = 0; iRow < nRows; iRow++) {
					if (bitsPerComponent == 1) {
						sl_uint8 bit = (*row >> 7) & 1;
						for (sl_uint32 i = 1; i < nBitsRow; i++) {
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
						if (bitsPerComponent == 16) {
							sl_uint16 pixel = MIO::readUint16BE(row);
							for (sl_uint32 i = nBytesPerPixel; i + 1 < sizeRow; i += 2) {
								pixel += MIO::readUint16BE(row + i);
								MIO::writeUint16BE(row + i, pixel);
							}
						} else {
							for (sl_uint32 i = nBytesPerPixel; i < sizeRow; i++) {
								row[i] += row[i - 1];
							}
						}
					}
					row += sizeRow;
				}
				return sl_true;
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
			return (*((Ref<PdfStream>*)((void*)(&m_var._value))))->getContent();
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

	PdfStream::PdfStream() noexcept: m_flagFiltered(sl_false)
	{
	}

	PdfStream::~PdfStream()
	{
	}

	PdfObject PdfStream::getProperty(const String& name) noexcept
	{
		return properties.getValue_NoLock(name);
	}

	Memory PdfStream::getContent() noexcept
	{
		PdfObject objFilter = getProperty(g_strFilter);
		if (objFilter.isUndefined()) {
			return contentUnfiltered;
		}
		if (m_flagFiltered) {
			return m_contentFiltered;
		}
		MutexLocker locker(&m_lock);
		if (m_flagFiltered) {
			return m_contentFiltered;
		}
		Memory contentFiltered;
		const PdfArray& arrayFilter = objFilter.getArray();
		if (arrayFilter.isNotNull()) {
			ListElements<PdfObject> filters(arrayFilter);
			if (filters.count) {
				contentFiltered = contentUnfiltered;
				for (sl_size i = 0; i < filters.count; i++) {
					const String& filter = filters[i].getName();
					if (filter.isNotNull()) {
						contentFiltered = ApplyFilter(contentFiltered, filter);
					} else {
						return sl_null;
					}
				}
			}
		} else {
			const String& filter = objFilter.getName();
			if (filter.isNotNull()) {
				contentFiltered = ApplyFilter(contentUnfiltered, filter);
			}
		}
		if (contentFiltered.isNotNull()) {
			m_contentFiltered = contentFiltered;
			contentUnfiltered.setNull();
			m_flagFiltered = sl_true;
			return contentFiltered;
		}
		return sl_null;
	}
	

	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(PdfFontDescriptor)

	PdfFontDescriptor::PdfFontDescriptor(): ascent(0), descent(0), leading(0), weight(0), italicAngle(0), content(0, 0)
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
			if (!(desc.getValue_NoLock(g_strFontFile).getReference(content))) {
				if (!(desc.getValue_NoLock(g_strFontFile2).getReference(content))) {
					desc.getValue_NoLock(g_strFontFile3).getReference(content);
				}
			}
		}
	}


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(PdfCidFontInfo)

	PdfCidFontInfo::PdfCidFontInfo(): subtype(PdfFontSubtype::Unknown), defaultWidth(1000)
	{
	}

	void PdfCidFontInfo::load(PdfDocument* doc, const PdfDictionary& dict) noexcept
	{
		if (dict.isNotNull()) {
			subtype = PdfFontResource::getSubtype(dict.getValue_NoLock(g_strSubtype).getName());
			dict.getValue_NoLock(g_strDW).getFloat(defaultWidth);
			{
				ListElements<PdfObject> w(doc->getObject(dict.getValue_NoLock(g_strW)).getArray());
				PdfObject* current = w.data;
				PdfObject* end = current + w.count;
				while (current < end) {
					sl_int32 code;
					if (!(current->getInt(code))) {
						break;
					}
					current++;
					if (current >= end) {
						break;
					}
					const PdfArray& arr = current->getArray();
					if (arr.isNotNull()) {
						current++;
						ListElements<PdfObject> m(arr);
						for (sl_size i = 0; i < m.count; i++) {
							float width;
							if (m[i].getFloat(width)) {
								widths.put_NoLock(code + (sl_int32)i, width);
							} else {
								break;
							}
						}
					} else {
						sl_int32 code2;
						if (!(current->getInt(code2))) {
							break;
						}
						current++;
						if (current >= end) {
							break;
						}
						float width;
						if (current->getFloat(width)) {
							for (sl_int32 i = code; i <= code2; i++) {
								widths.put_NoLock(i, width);
							}
						} else {
							break;
						}
						current++;
					}
				}
			}
		}
	}

	float PdfCidFontInfo::getWidth(sl_int32 code)
	{
		float ret;
		if (widths.get_NoLock(code, &ret)) {
			return ret;
		}
		return defaultWidth;
	}


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(PdfFontResource)

	PdfFontResource::PdfFontResource(): subtype(PdfFontSubtype::Unknown), firstChar(0), lastChar(0), encoding(PdfEncoding::PdfDoc)
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
			PdfObject objEncoding = dict.getValue_NoLock(g_strEncoding);
			const String& encodingName = objEncoding.getName();
			if (encodingName.isNotNull()) {
				encoding = Pdf::getEncoding(encodingName);
			} else {
				PdfReference ref;
				if (objEncoding.getReference(ref)) {
					const PdfDictionary& dict = doc->getObject(ref).getDictionary();
					if (dict.isNotNull()) {
						const String& s = dict.getValue_NoLock(g_strBaseEncoding).getString();
						if (s.isNotNull()) {
							encoding = Pdf::getEncoding(s);
						}
					}
				}
			}
		}
		firstChar = dict.getValue_NoLock(g_strFirstChar).getInt();
		lastChar = dict.getValue_NoLock(g_strLastChar).getInt();
		{
			ListElements<PdfObject> arr(dict.getValue_NoLock(g_strWidths).getArray());
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
		PdfObject refCmap = dict.getValue_NoLock(g_strToUnicode);
		if (refCmap.isNotNull()) {
			Memory content = doc->getObject(refCmap).getStreamContent();
			if (content.isNotNull()) {
				cmap = ParseCMap(content.getData(), content.getSize());
			}
		}
		return sl_true;
	}

	String32 PdfFontResource::getUnicode(sl_int32 code)
	{
		if (cmap.isNotNull()) {
			return cmap.getValue_NoLock((sl_uint16)code);
		} else {
			if (code >= 0 && code < 256) {
				const sl_char16* map = Pdf::getUnicodeTable(encoding);
				if (map) {
					return String32(map[(sl_uint8)code], 1);
				} else {
					return String32((sl_uint8)code, 1);
				}
			}
		}
		return sl_null;
	}

	sl_bool PdfFontResource::getCharWidth(sl_int32 ch, float& width) noexcept
	{
		if (subtype == PdfFontSubtype::Type0) {
			width = cid.getWidth(ch) / 1000.0f;
			return sl_true;
		}
		if (widths.isNotNull() && ch >= firstChar && ch <= lastChar) {
			width = widths[ch - firstChar] / 1000.0f;
			return sl_true;
		}
		return sl_false;
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


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(PdfImageResource)

	PdfImageResource::PdfImageResource(): flagJpeg(sl_false), flagFlate(sl_false), width(0), height(0), bitsPerComponent(0), colorSpace(PdfColorSpace::Unknown), colorSpaceRef(0, 0), predictor(0), colors(0), columns(0), smask(0, 0)
	{
	}

	sl_bool PdfImageResource::load(PdfStream* stream) noexcept
	{
		const String& subtype = stream->properties.getValue_NoLock(g_strSubtype).getName();
		if (subtype == StringView::literal("Image")) {
			const PdfObject& objFilter = stream->properties.getValue_NoLock(g_strFilter);
			String filter = objFilter.getName();
			if (filter.isNull()) {
				const PdfArray& arrFilter = objFilter.getArray();
				if (arrFilter.isNotNull()) {
					filter = arrFilter.getLastValue_NoLock().getName();
				}
			}
			if (filter == StringView::literal("DCTDecode") || filter == StringView::literal("DCT")) {
				flagJpeg = sl_true;
			} else if (filter == StringView::literal("FlateDecode") || filter == StringView::literal("Fl")) {
				flagFlate = sl_true;
			}
			stream->properties.getValue_NoLock(g_strWidth).getUint(width);
			stream->properties.getValue_NoLock(g_strHeight).getUint(height);
			stream->properties.getValue_NoLock(g_strBitsPerComponent).getUint(bitsPerComponent);
			const PdfObject& objColorSpace = stream->properties.getValue_NoLock(g_strColorSpace);
			colorSpace = Pdf::getColorSpace(objColorSpace.getName());
			if (colorSpace == PdfColorSpace::Unknown) {
				objColorSpace.getReference(colorSpaceRef);
			}
			stream->properties.getValue_NoLock(g_strPredictor).getUint(predictor);
			stream->properties.getValue_NoLock(g_strColors).getUint(colors);
			stream->properties.getValue_NoLock(g_strColumns).getUint(columns);
			stream->properties.getValue_NoLock(g_strSMask).getReference(smask);
			return sl_true;
		}
		return sl_false;
	}

	sl_uint32 PdfImageResource::predict(void* content, sl_uint32 size) noexcept
	{
		if (!flagFlate) {
			return 0;
		}
		if (predictor >= 10) {
			if (PredictPNG((sl_uint8*)content, size, colors, bitsPerComponent, columns)) {
				return size;
			}
		} else if (predictor == 2) {
			if (PredictTIFF((sl_uint8*)content, size, colors, bitsPerComponent, columns)) {
				return size;
			}
		} else {
			return size;
		}
		return 0;
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
			PdfOperator op = parser.readOperator();
			if (op != PdfOperator::Unknown) {
				opCurrent.op = op;
				ret.add_NoLock(Move(opCurrent));
				opCurrent.operands.setNull();
			} else {
				PdfObject obj = parser.readValue();
				if (obj.isUndefined()) {
					break;
				}
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

	PdfObject PdfPage::getResources(const String& type, sl_bool flagResolveReference)
	{
		Ref<PdfDocument> doc(m_document);
		if (doc.isNotNull()) {
			ObjectLocker locker(doc.get());
			ParserBase* parser = GetParser(doc->m_parser);
			if (parser) {
				Ref<PdfPageTreeItem> item = this;
				for (;;) {
					const PdfDictionary& dict = parser->getObject(item->attributes.getValue_NoLock(g_strResources)).getDictionary();
					if (dict.isNotNull()) {
						PdfObject ret = dict.getValue_NoLock(type);
						if (ret.isNotUndefined()) {
							if (flagResolveReference) {
								return parser->getObject(ret);
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
		return PdfObject();
	}

	PdfObject PdfPage::getResource(const String& type, const String& name, sl_bool flagResolveReference)
	{
		Ref<PdfDocument> doc(m_document);
		if (doc.isNotNull()) {
			ObjectLocker locker(doc.get());
			ParserBase* parser = GetParser(doc->m_parser);
			if (parser) {
				Ref<PdfPageTreeItem> item = this;
				for (;;) {
					const PdfDictionary& dict = parser->getObject(item->attributes.getValue_NoLock(g_strResources)).getDictionary();
					if (dict.isNotNull()) {
						PdfObject ret = dict.getValue_NoLock(type).getDictionary().getValue_NoLock(name);
						if (ret.isNotUndefined()) {
							if (flagResolveReference) {
								return parser->getObject(ret);
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
		return PdfObject();
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

	PdfObject PdfPage::getExternalObjectResource(const String& name)
	{
		return getResource(g_strXObject, name);
	}

	sl_bool PdfPage::getExternalObjectResource(const String& name, PdfReference& outRef)
	{
		return getResource(g_strXObject, name, sl_false).getReference(outRef);
	}

	sl_bool PdfPage::getImageResource(const String& name, PdfImageResource& outRes)
	{
		const Ref<PdfStream>& stream = getExternalObjectResource(name).getStream();
		if (stream.isNotNull()) {
			return outRes.load(stream.get());
		}
		return sl_false;
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

	PdfEncoding Pdf::getEncoding(const StringView& name) noexcept
	{
		if (name == StringView::literal("StandardEncoding")) {
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

	PdfColorSpace Pdf::getColorSpace(const StringView& name) noexcept
	{
		if (name == StringView::literal("DeviceRGB")) {
			return PdfColorSpace::RGB;
		} else if (name == StringView::literal("DeviceGray")) {
			return PdfColorSpace::Gray;
		} else if (name == StringView::literal("DeviceCMYK")) {
			return PdfColorSpace::CMYK;
		} else {
			return PdfColorSpace::Unknown;
		}
	}

}
