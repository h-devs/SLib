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

#include "slib/graphics/color.h"
#include "slib/graphics/yuv.h"
#include "slib/graphics/cmyk.h"
#include "slib/graphics/cie.h"

#include "slib/core/string.h"
#include "slib/core/hash_table.h"
#include "slib/core/safe_static.h"

namespace slib
{

	SLIB_ALIGN(8) sl_uint8 Color::_zero[4] = {0, 0, 0, 0};

	void Color::setBlueF(float v) noexcept
	{
		sl_int32 n = (sl_int32)(v * 255.0f);
		b = (sl_uint8)(Math::clamp0_255(n));
	}

	void Color::setGreenF(float v) noexcept
	{
		sl_int32 n = (sl_int32)(v * 255.0f);
		g = (sl_uint8)(Math::clamp0_255(n));
	}

	void Color::setRedF(float v) noexcept
	{
		sl_int32 n = (sl_int32)(v * 255.0f);
		r = (sl_uint8)(Math::clamp0_255(n));
	}

	void Color::setAlphaF(float v) noexcept
	{
		sl_int32 n = (sl_int32)(v * 255.0f);
		a = (sl_uint8)(Math::clamp0_255(n));
	}

	void Color::blend_PA_NPA(sl_uint32 _r, sl_uint32 _g, sl_uint32 _b, sl_uint32 _a) noexcept
	{
		sl_uint32 _or = r;
		sl_uint32 _og = g;
		sl_uint32 _ob = b;
		sl_uint32 _oa = a;
		sl_uint32 sa = _a;
		_or = (_or * (255 - sa) + _r * sa) / 255;
		_og = (_og * (255 - sa) + _g * sa) / 255;
		_ob = (_ob * (255 - sa) + _b * sa) / 255;
		_oa = (_oa * (255 - sa) + 255 * sa) / 255;
		r = (sl_uint8)(_or);
		g = (sl_uint8)(_og);
		b = (sl_uint8)(_ob);
		a = (sl_uint8)(_oa);
	}

	void Color::blend_PA_NPA(const Color& src) noexcept
	{
		blend_PA_NPA(src.r, src.g, src.b, src.a);
	}

	void Color::blend_PA_PA(sl_uint32 _r, sl_uint32 _g, sl_uint32 _b, sl_uint32 _a) noexcept
	{
		sl_uint32 _or = r;
		sl_uint32 _og = g;
		sl_uint32 _ob = b;
		sl_uint32 oa = a;
		sl_uint32 sa = _a;
		_or = Math::clamp0_255(_or * (255 - sa) / 255 + _r);
		_og = Math::clamp0_255(_og * (255 - sa) / 255 + _g);
		_ob = Math::clamp0_255(_ob * (255 - sa) / 255 + _b);
		oa = (oa * (255 - sa) + 255 * sa) / 255;
		r = (sl_uint8)(_or);
		g = (sl_uint8)(_og);
		b = (sl_uint8)(_ob);
		a = (sl_uint8)(oa);
	}

	void Color::blend_PA_PA(const Color& src) noexcept
	{
		blend_PA_NPA(src.r, src.g, src.b, src.a);
	}

	void Color::blend_NPA_NPA(sl_uint32 _r, sl_uint32 _g, sl_uint32 _b, sl_uint32 _a) noexcept
	{
		sl_uint32 _or = r;
		sl_uint32 _og = g;
		sl_uint32 _ob = b;
		sl_uint32 oa = a;
		sl_uint32 sa = _a;
		sl_uint32 _oa = (oa * (255 - sa) + 255 * sa) + 1;
		_or = Math::clamp0_255((_or * oa * (255 - sa) + _r * (255 * sa + 1)) / _oa);
		_og = Math::clamp0_255((_og * oa * (255 - sa) + _g * (255 * sa + 1)) / _oa);
		_ob = Math::clamp0_255((_ob * oa * (255 - sa) + _b * (255 * sa + 1)) / _oa);
		_oa = (_oa - 1) / 255;
		r = (sl_uint8)(_or);
		g = (sl_uint8)(_og);
		b = (sl_uint8)(_ob);
		a = (sl_uint8)(_oa);
	}

	void Color::blend_NPA_NPA(const Color& src) noexcept
	{
		blend_NPA_NPA(src.r, src.g, src.b, src.a);
	}

	void Color::blend_NPA_PA(sl_uint32 _r, sl_uint32 _g, sl_uint32 _b, sl_uint32 _a) noexcept
	{
		sl_uint32 _or = r;
		sl_uint32 _og = g;
		sl_uint32 _ob = b;
		sl_uint32 oa = a;
		sl_uint32 sa = _a;
		sl_uint32 _oa = (oa * (255 - sa) + 255 * sa) + 1;
		_or = Math::clamp0_255((_or * oa * (255 - sa) + (_r << 16)) / _oa);
		_og = Math::clamp0_255((_og * oa * (255 - sa) + (_g << 16)) / _oa);
		_ob = Math::clamp0_255((_ob * oa * (255 - sa) + (_b << 16)) / _oa);
		_oa = (_oa - 1) / 255;
		r = (sl_uint8)(_or);
		g = (sl_uint8)(_og);
		b = (sl_uint8)(_ob);
		a = (sl_uint8)(_oa);
	}

	void Color::blend_NPA_PA(const Color& src) noexcept
	{
		blend_NPA_PA(src.r, src.g, src.b, src.a);
	}

	void Color::convertNPAtoPA() noexcept
	{
		sl_uint32 _or = r;
		sl_uint32 _og = g;
		sl_uint32 _ob = b;
		sl_uint32 oa = a;
		oa++;
		_or = (_or * oa) >> 8;
		_og = (_og * oa) >> 8;
		_ob = (_ob * oa) >> 8;
		r = (sl_uint8)(_or & 255);
		g = (sl_uint8)(_og & 255);
		b = (sl_uint8)(_ob & 255);
	}

	void Color::convertPAtoNPA() noexcept
	{
		sl_uint32 _or = r;
		sl_uint32 _og = g;
		sl_uint32 _ob = b;
		sl_uint32 oa = a;
		oa++;
		_or = Math::clamp0_255((_or << 8) / oa);
		_og = Math::clamp0_255((_og << 8) / oa);
		_ob = Math::clamp0_255((_ob << 8) / oa);
		r = (sl_uint8)(_or);
		g = (sl_uint8)(_og);
		b = (sl_uint8)(_ob);
	}

	void Color::multiplyAlpha(float f) noexcept
	{
		a = (sl_uint8)(Math::clamp0_255((sl_int32)(a * f)));
	}

	void Color::multiplyRed(float f) noexcept
	{
		r = (sl_uint8)(Math::clamp0_255((sl_int32)(r * f)));
	}

	void Color::multiplyGreen(float f) noexcept
	{
		g = (sl_uint8)(Math::clamp0_255((sl_int32)(g * f)));
	}

	void Color::multiplyBlue(float f) noexcept
	{
		b = (sl_uint8)(Math::clamp0_255((sl_int32)(b * f)));
	}

	sl_bool Color::equals(const Color& other) const noexcept
	{
		return r == other.r && g == other.g && b == other.b && a == other.a;
	}

	sl_compare_result Color::compare(const Color& other) const noexcept
	{
		return ComparePrimitiveValues(getARGB(), other.getARGB());
	}

	sl_size Color::getHashCode() const noexcept
	{
		return Rehash32(getARGB());
	}

	String Color::toString() const noexcept
	{
		const char* hex = "0123456789abcdef";
		char s[9];
		s[0] = '#';
		s[1] = hex[a >> 4];
		s[2] = hex[a & 15];
		s[3] = hex[r >> 4];
		s[4] = hex[r & 15];
		s[5] = hex[g >> 4];
		s[6] = hex[g & 15];
		s[7] = hex[b >> 4];
		s[8] = hex[b & 15];
		return String(s, 9);
	}

	namespace priv
	{
		namespace color
		{

			/*

			Color names supported by all web-browsers

			http://www.w3schools.com/colors/colors_names.asp

			*/

#define PRIV_MAP_COLOR(NAME) \
			{ \
				SLIB_STATIC_STRING(_s, #NAME); \
				mapName.put(_s.toLower(), (sl_uint32)(Color::NAME)); \
			}

			class NameMap
			{
			public:
				HashTable<String, Color> mapName;


				NameMap() noexcept
				{
					PRIV_MAP_COLOR(AliceBlue)
					PRIV_MAP_COLOR(AntiqueWhite)
					PRIV_MAP_COLOR(Aqua)
					PRIV_MAP_COLOR(Aquamarine)
					PRIV_MAP_COLOR(Azure)
					PRIV_MAP_COLOR(Beige)
					PRIV_MAP_COLOR(Bisque)
					PRIV_MAP_COLOR(Black)
					PRIV_MAP_COLOR(BlanchedAlmond)
					PRIV_MAP_COLOR(Blue)
					PRIV_MAP_COLOR(BlueViolet)
					PRIV_MAP_COLOR(Brown)
					PRIV_MAP_COLOR(BurlyWood)
					PRIV_MAP_COLOR(CadetBlue)
					PRIV_MAP_COLOR(Chartreuse)
					PRIV_MAP_COLOR(Chocolate)
					PRIV_MAP_COLOR(Coral)
					PRIV_MAP_COLOR(CornflowerBlue)
					PRIV_MAP_COLOR(Cornsilk)
					PRIV_MAP_COLOR(Crimson)
					PRIV_MAP_COLOR(Cyan)
					PRIV_MAP_COLOR(DarkBlue)
					PRIV_MAP_COLOR(DarkCyan)
					PRIV_MAP_COLOR(DarkGoldenrod)
					PRIV_MAP_COLOR(DarkGray)
					PRIV_MAP_COLOR(DarkGreen)
					PRIV_MAP_COLOR(DarkKhaki)
					PRIV_MAP_COLOR(DarkMagenta)
					PRIV_MAP_COLOR(DarkOliveGreen)
					PRIV_MAP_COLOR(DarkOrange)
					PRIV_MAP_COLOR(DarkOrchid)
					PRIV_MAP_COLOR(DarkRed)
					PRIV_MAP_COLOR(DarkSalmon)
					PRIV_MAP_COLOR(DarkSeaGreen)
					PRIV_MAP_COLOR(DarkSlateBlue)
					PRIV_MAP_COLOR(DarkSlateGray)
					PRIV_MAP_COLOR(DarkTurquoise)
					PRIV_MAP_COLOR(DarkViolet)
					PRIV_MAP_COLOR(DeepPink)
					PRIV_MAP_COLOR(DeepSkyBlue)
					PRIV_MAP_COLOR(DimGray)
					PRIV_MAP_COLOR(DodgerBlue)
					PRIV_MAP_COLOR(Firebrick)
					PRIV_MAP_COLOR(FloralWhite)
					PRIV_MAP_COLOR(ForestGreen)
					PRIV_MAP_COLOR(Fuchsia)
					PRIV_MAP_COLOR(Gainsboro)
					PRIV_MAP_COLOR(GhostWhite)
					PRIV_MAP_COLOR(Gold)
					PRIV_MAP_COLOR(Goldenrod)
					PRIV_MAP_COLOR(Gray)
					PRIV_MAP_COLOR(Green)
					PRIV_MAP_COLOR(GreenYellow)
					PRIV_MAP_COLOR(Honeydew)
					PRIV_MAP_COLOR(HotPink)
					PRIV_MAP_COLOR(IndianRed)
					PRIV_MAP_COLOR(Indigo)
					PRIV_MAP_COLOR(Ivory)
					PRIV_MAP_COLOR(Khaki)
					PRIV_MAP_COLOR(Lavender)
					PRIV_MAP_COLOR(LavenderBlush)
					PRIV_MAP_COLOR(LawnGreen)
					PRIV_MAP_COLOR(LemonChiffon)
					PRIV_MAP_COLOR(LightBlue)
					PRIV_MAP_COLOR(LightCoral)
					PRIV_MAP_COLOR(LightCyan)
					PRIV_MAP_COLOR(LightGoldenrodYellow)
					PRIV_MAP_COLOR(LightGray)
					PRIV_MAP_COLOR(LightGreen)
					PRIV_MAP_COLOR(LightPink)
					PRIV_MAP_COLOR(LightSalmon)
					PRIV_MAP_COLOR(LightSeaGreen)
					PRIV_MAP_COLOR(LightSkyBlue)
					PRIV_MAP_COLOR(LightSlateGray)
					PRIV_MAP_COLOR(LightSteelBlue)
					PRIV_MAP_COLOR(LightYellow)
					PRIV_MAP_COLOR(Lime)
					PRIV_MAP_COLOR(LimeGreen)
					PRIV_MAP_COLOR(Linen)
					PRIV_MAP_COLOR(Magenta)
					PRIV_MAP_COLOR(Maroon)
					PRIV_MAP_COLOR(MediumAquamarine)
					PRIV_MAP_COLOR(MediumBlue)
					PRIV_MAP_COLOR(MediumOrchid)
					PRIV_MAP_COLOR(MediumPurple)
					PRIV_MAP_COLOR(MediumSeaGreen)
					PRIV_MAP_COLOR(MediumSlateBlue)
					PRIV_MAP_COLOR(MediumSpringGreen)
					PRIV_MAP_COLOR(MediumTurquoise)
					PRIV_MAP_COLOR(MediumVioletRed)
					PRIV_MAP_COLOR(MidnightBlue)
					PRIV_MAP_COLOR(MintCream)
					PRIV_MAP_COLOR(MistyRose)
					PRIV_MAP_COLOR(Moccasin)
					PRIV_MAP_COLOR(NavajoWhite)
					PRIV_MAP_COLOR(Navy)
					PRIV_MAP_COLOR(OldLace)
					PRIV_MAP_COLOR(Olive)
					PRIV_MAP_COLOR(OliveDrab)
					PRIV_MAP_COLOR(Orange)
					PRIV_MAP_COLOR(OrangeRed)
					PRIV_MAP_COLOR(Orchid)
					PRIV_MAP_COLOR(PaleGoldenrod)
					PRIV_MAP_COLOR(PaleGreen)
					PRIV_MAP_COLOR(PaleTurquoise)
					PRIV_MAP_COLOR(PaleVioletRed)
					PRIV_MAP_COLOR(PapayaWhip)
					PRIV_MAP_COLOR(PeachPuff)
					PRIV_MAP_COLOR(Peru)
					PRIV_MAP_COLOR(Pink)
					PRIV_MAP_COLOR(Plum)
					PRIV_MAP_COLOR(PowderBlue)
					PRIV_MAP_COLOR(Purple)
					PRIV_MAP_COLOR(Red)
					PRIV_MAP_COLOR(RosyBrown)
					PRIV_MAP_COLOR(RoyalBlue)
					PRIV_MAP_COLOR(SaddleBrown)
					PRIV_MAP_COLOR(Salmon)
					PRIV_MAP_COLOR(SandyBrown)
					PRIV_MAP_COLOR(SeaGreen)
					PRIV_MAP_COLOR(SeaShell)
					PRIV_MAP_COLOR(Sienna)
					PRIV_MAP_COLOR(Silver)
					PRIV_MAP_COLOR(SkyBlue)
					PRIV_MAP_COLOR(SlateBlue)
					PRIV_MAP_COLOR(SlateGray)
					PRIV_MAP_COLOR(Snow)
					PRIV_MAP_COLOR(SpringGreen)
					PRIV_MAP_COLOR(SteelBlue)
					PRIV_MAP_COLOR(Tan)
					PRIV_MAP_COLOR(Teal)
					PRIV_MAP_COLOR(Thistle)
					PRIV_MAP_COLOR(Tomato)
					PRIV_MAP_COLOR(Transparent)
					PRIV_MAP_COLOR(Turquoise)
					PRIV_MAP_COLOR(Violet)
					PRIV_MAP_COLOR(Wheat)
					PRIV_MAP_COLOR(White)
					PRIV_MAP_COLOR(WhiteSmoke)
					PRIV_MAP_COLOR(Yellow)
					PRIV_MAP_COLOR(YellowGreen)
					PRIV_MAP_COLOR(Zero)
					PRIV_MAP_COLOR(None)
				}

				sl_bool getColorFromName(const String& nameLower, Color& color) noexcept
				{
					return mapName.get(nameLower, &color);
				}

			};

			SLIB_SAFE_STATIC_GETTER(NameMap, GetNameMap)

			template <class CT>
			static sl_reg Parse(Color* _out, const CT* str, sl_size pos, sl_size len) noexcept
			{
				if (pos >= len) {
					return SLIB_PARSE_ERROR;
				}

				if (str[pos] == '#') {

					pos++;
					sl_size start = pos;

					for (; pos < len; pos++) {
						CT c = str[pos];
						if (!SLIB_CHAR_IS_HEX(c)) {
							break;
						}
					}

					sl_size n = pos - start;
					if (n == 6 || n == 8) {
						if (_out) {
							sl_uint32 r, g, b, a;
							sl_uint32 c0 = str[start];
							sl_uint32 c1 = str[start + 1];
							sl_uint32 c2 = str[start + 2];
							sl_uint32 c3 = str[start + 3];
							sl_uint32 c4 = str[start + 4];
							sl_uint32 c5 = str[start + 5];
							if (n == 6) {
								a = 255;
								r = (SLIB_CHAR_HEX_TO_INT(c0) << 4) | SLIB_CHAR_HEX_TO_INT(c1);
								g = (SLIB_CHAR_HEX_TO_INT(c2) << 4) | SLIB_CHAR_HEX_TO_INT(c3);
								b = (SLIB_CHAR_HEX_TO_INT(c4) << 4) | SLIB_CHAR_HEX_TO_INT(c5);
							} else {
								sl_uint32 c6 = str[start + 6];
								sl_uint32 c7 = str[start + 7];
								a = (SLIB_CHAR_HEX_TO_INT(c0) << 4) | SLIB_CHAR_HEX_TO_INT(c1);
								r = (SLIB_CHAR_HEX_TO_INT(c2) << 4) | SLIB_CHAR_HEX_TO_INT(c3);
								g = (SLIB_CHAR_HEX_TO_INT(c4) << 4) | SLIB_CHAR_HEX_TO_INT(c5);
								b = (SLIB_CHAR_HEX_TO_INT(c6) << 4) | SLIB_CHAR_HEX_TO_INT(c7);
							}
							*_out = Color(r, g, b, a);
						}
						return pos;
					}

				} else {

					sl_size start = pos;

					for (; pos < len; pos++) {
						CT c = str[pos];
						if (!SLIB_CHAR_IS_ALPHA(c)) {
							break;
						}
					}

					sl_size n = pos - start;

					if (n > 0) {

						if ((n == 3 || n == 4) && (str[start] == 'r' || str[start] == 'R') && (str[start + 1] == 'g' || str[start + 1] == 'G') && (str[start + 2] == 'b' || str[start + 2] == 'B')) {

							if (n == 4) {
								if (str[start + 3] != 'a' && str[start + 3] != 'A') {
									return SLIB_PARSE_ERROR;
								}
							}

							for (; pos < len; pos++) {
								CT c = str[pos];
								if (!SLIB_CHAR_IS_SPACE_TAB(c)) {
									break;
								}
							}
							if (pos >= len) {
								return SLIB_PARSE_ERROR;
							}

							if (str[pos] != '(') {
								return SLIB_PARSE_ERROR;
							}
							pos++;

							sl_reg iRet;
							sl_uint32 comp[3];
							sl_real a = 1;

							for (sl_size i = 0; i < n; i++) {

								for (; pos < len; pos++) {
									CT c = str[pos];
									if (!SLIB_CHAR_IS_SPACE_TAB(c)) {
										break;
									}
								}
								if (pos >= len) {
									return SLIB_PARSE_ERROR;
								}
								if (i == 3) {
									iRet = StringTypeFromCharType<CT>::Type::parseFloat(&a, str, pos, len);
									if (iRet == SLIB_PARSE_ERROR) {
										return SLIB_PARSE_ERROR;
									}
									if (a > 1) {
										return SLIB_PARSE_ERROR;
									}
									if (a < 0) {
										return SLIB_PARSE_ERROR;
									}
								} else {
									iRet = StringTypeFromCharType<CT>::Type::parseUint32(10, comp + i, str, pos, len);
									if (iRet == SLIB_PARSE_ERROR) {
										return SLIB_PARSE_ERROR;
									}
									if (comp[i] > 255) {
										return SLIB_PARSE_ERROR;
									}
								}
								pos = iRet;
								for (; pos < len; pos++) {
									CT c = str[pos];
									if (!SLIB_CHAR_IS_SPACE_TAB(c)) {
										break;
									}
								}
								if (pos >= len) {
									return SLIB_PARSE_ERROR;
								}
								if (i < n - 1) {
									if (str[pos] != ',') {
										return SLIB_PARSE_ERROR;
									}
								} else {
									if (str[pos] != ')') {
										return SLIB_PARSE_ERROR;
									}
								}
								pos++;
							}

							if (_out) {
								if (n == 4) {
									*_out = Color(comp[0], comp[1], comp[2], (sl_uint32)(a * 255));
								} else {
									*_out = Color(comp[0], comp[1], comp[2]);
								}
							}

							return pos;

						} else if (n < 64) {

							sl_char8 s[64];
							for (sl_size i = 0; i < n; i++) {
								sl_char8 ch = (sl_char8)(str[start + i]);
								s[i] = SLIB_CHAR_UPPER_TO_LOWER(ch);
							}

							NameMap* nm = GetNameMap();
							if (nm) {
								Color color;
								if (nm->getColorFromName(String(s, n), color)) {
									if (_out) {
										*_out = color;
									}
									return pos;
								}
							}

						}
					}
				}
				return SLIB_PARSE_ERROR;
			}

		}
	}

	SLIB_DEFINE_CLASS_PARSE_MEMBERS(Color, priv::color::Parse)


	Color4f ColorMatrix::transformColor(const Color4f& src) const noexcept
	{
		Color4f ret;
		ret.x = src.dot(red);
		ret.y = src.dot(green);
		ret.z = src.dot(blue);
		ret.w = src.dot(alpha);
		ret += bias;
		return ret;
	}

	Color4f ColorMatrix::transformColor(const Color& src) const noexcept
	{
		return transformColor((Color4f)src);
	}

	Color4f ColorMatrix::transformColor(const Color3f& src) const noexcept
	{
		return transformColor(Color4f(src.x, src.y, src.z, 1));
	}

	void ColorMatrix::setOverlay(const Color4f& c) noexcept
	{
		red = Color4f::zero();
		green = Color4f::zero();
		blue = Color4f::zero();
		alpha = Color4f(0, 0, 0, c.w);
		bias = Color4f(c.x, c.y, c.z, 0);
	}

	void ColorMatrix::setOverlay(const Color& c) noexcept
	{
		setOverlay((Color4f)c);
	}

	void ColorMatrix::setOverlay(const Color3f& c) noexcept
	{
		setOverlay(Color4f(c.x, c.y, c.z, 1));
	}


#define YUV_YG 18997 /* round(1.164 * 64 * 256 * 256 / 257) */
#define YUV_YGB 1160 /* 1.164 * 64 * 16 - adjusted for even error distribution */
#define YUV_UB -128 /* -min(128, round(2.018 * 64)) */
#define YUV_UG 25 /* -round(-0.391 * 64) */
#define YUV_VG 52 /* -round(-0.813 * 64) */
#define YUV_VR -102 /* -round(1.596 * 64) */
#define YUV_BB (YUV_UB * 128 - YUV_YGB)
#define YUV_BG (YUV_UG * 128 + YUV_VG * 128 - YUV_YGB)
#define YUV_BR (YUV_VR * 128 - YUV_YGB)

	void YUV::convertRGBToYUV(sl_uint8 R, sl_uint8 G, sl_uint8 B, sl_uint8& Y, sl_uint8& U, sl_uint8& V) noexcept
	{
		sl_int32 _r = R;
		sl_int32 _g = G;
		sl_int32 _b = B;
		Y = (sl_uint8)(Math::clamp0_255((66 * _r + 129 * _g + 25 * _b + 0x1080) >> 8)); // y
		U = (sl_uint8)(Math::clamp0_255((112 * _b - 74 * _g - 38 * _r + 0x8080) >> 8)); // u
		V = (sl_uint8)(Math::clamp0_255((112 * _r - 94 * _g - 18 * _b + 0x8080) >> 8)); // v
	}

	void YUV::convertYUVToRGB(sl_uint8 Y, sl_uint8 U, sl_uint8 V, sl_uint8& R, sl_uint8& G, sl_uint8& B) noexcept
	{
		sl_int32 _y = Y;
		sl_int32 _u = U;
		sl_int32 _v = V;
		sl_int32 y1 = (sl_uint32)(_y * 0x0101 * YUV_YG) >> 16;
		B = (sl_uint8)(Math::clamp0_255((sl_int32)(YUV_BB - (_u * YUV_UB) + y1) >> 6));
		G = (sl_uint8)(Math::clamp0_255((sl_int32)(YUV_BG - (_v * YUV_VG + _u * YUV_UG) + y1) >> 6));
		R = (sl_uint8)(Math::clamp0_255((sl_int32)(YUV_BR - (_v * YUV_VR) + y1) >> 6));
	}


	void CMYK::convertRGBToCMYK(sl_uint8 R, sl_uint8 G, sl_uint8 B, sl_uint8& C, sl_uint8& M, sl_uint8& Y, sl_uint8& K) noexcept
	{
		C = 255 - R;
		M = 255 - G;
		Y = 255 - B;
		K = C;
		if (K > M) {
			K = M;
		}
		if (K > Y) {
			K = Y;
		}
		C = (sl_uint8)(Math::clamp0_255(C - K));
		M = (sl_uint8)(Math::clamp0_255(M - K));
		Y = (sl_uint8)(Math::clamp0_255(Y - K));
	}

	void CMYK::convertCMYKToRGB(sl_uint8 C, sl_uint8 M, sl_uint8 Y, sl_uint8 K, sl_uint8& R, sl_uint8& G, sl_uint8& B) noexcept
	{
		R = (sl_uint8)(255 - Math::clamp0_255((sl_uint32)C + (sl_uint32)K));
		G = (sl_uint8)(255 - Math::clamp0_255((sl_uint32)M + (sl_uint32)K));
		B = (sl_uint8)(255 - Math::clamp0_255((sl_uint32)Y + (sl_uint32)K));
	}


	void CIE::convertXyzToRGB(float x, float y, float z, float& r, float& g, float& b) noexcept
	{
		r = Math::sqrt(Math::clamp((3.240449f * x - 1.537136f * y - 0.498531f * z) * 0.830026f, 0.0f, 1.0f));
		g = Math::sqrt(Math::clamp((-0.969265f * x + 1.876011f * y + 0.041556f * z) * 1.05452f, 0.0f, 1.0f));
		b = Math::sqrt(Math::clamp((0.055643f * x - 0.204026f * y + 1.057229f * z) * 1.1003f, 0.0f, 1.0f));
	}

	void CIE::convertLabToRGB(float lstar, float astar, float bstar, float& r, float& g, float& b) noexcept
	{
		float m = (lstar + 16.0f) / 116;
		float l = m + astar / 500.0f;
		float n = m - bstar / 200.0f;
#define LAB_FUNG(x) ((x) >= 6.0f / 29.0f ? (x)*(x)*(x) : (108.0f / 841.0f) * ((x) - (4.0f / 29.0f)))
		float x = LAB_FUNG(l);
		float y = LAB_FUNG(m);
		float z = LAB_FUNG(n);
		convertXyzToRGB(x, y, z, r, g, b);
	}

}
