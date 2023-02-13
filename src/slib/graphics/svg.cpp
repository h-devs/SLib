/*
 *   Copyright (c) 2008-2023 SLIBIO <https://github.com/SLIBIO>
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

#include "slib/graphics/svg.h"

#include "slib/graphics/canvas.h"
#include "slib/graphics/css.h"
#include "slib/graphics/util.h"
#include "slib/math/transform2d.h"
#include "slib/core/memory.h"
#include "slib/core/asset.h"
#include "slib/core/stringx.h"
#include "slib/core/safe_static.h"
#include "slib/io/file.h"
#include "slib/data/xml.h"

#define MAX_LINK_DEEP_LEVEL 5

namespace slib
{

	namespace {

		typedef sl_svg_scalar Scalar;

		static Scalar g_fontSize = (Scalar)12;

		template <class BASE>
		class Define : public BASE
		{
		public:
			sl_bool flagDefined = sl_false;

		public:
			SLIB_INLINE BASE& operator*()
			{
				return *this;
			}

		};

		template <class BASE>
		class DefinePrimitive
		{
		public:
			BASE value;
			sl_bool flagDefined = sl_false;

		public:
			SLIB_INLINE BASE& operator*()
			{
				return value;
			}

		};

		static void SkipWhitespaces(sl_char8*& start, sl_char8* end)
		{
			while (start < end) {
				sl_char8 c = *start;
				if (!SLIB_CHAR_IS_WHITE_SPACE(c)) {
					break;
				}
				start++;
			}
		}

		static void SkipNoWhitespaces(sl_char8*& start, sl_char8* end)
		{
			while (start < end) {
				sl_char8 c = *start;
				if (SLIB_CHAR_IS_WHITE_SPACE(c)) {
					break;
				}
				start++;
			}
		}

		static void SkipValueSeparator(sl_char8*& s, sl_char8* end)
		{
			SkipWhitespaces(s, end);
			if (s < end) {
				if (*s == ',') {
					s++;
					SkipWhitespaces(s, end);
				}
			}
		}

		static sl_bool SkipPattern(sl_char8*& s, sl_char8* end, const StringView& pattern)
		{
			if (StringView(s, end - s).startsWith(pattern)) {
				s += pattern.getLength();
				return sl_true;
			} else {
				return sl_false;
			}
		}

		static sl_bool ParseScalar(sl_char8*& s, sl_char8* end, Scalar& _out)
		{
			sl_reg result = String::parseFloat(&_out, s, 0, end - s);
			if (result == SLIB_PARSE_ERROR) {
				return sl_false;
			}
			s += result;
			return sl_true;
		}

		static sl_bool ParseValue(sl_char8*& s, sl_char8* end, Scalar& _out)
		{
			if (!(ParseScalar(s, end, _out))) {
				return sl_false;
			}
			if (s >= end) {
				return sl_true;
			}
			if (*s == '%') {
				s++;
				_out *= (Scalar)0.01;
			}
			return sl_true;
		}

		static sl_bool ParseValue(sl_char8*& s, sl_char8* end, String& _out)
		{
			sl_reg result = CascadingStyleSheet::parseStringValue(&_out, s, 0, end - s);
			if (result == SLIB_PARSE_ERROR) {
				return sl_false;
			}
			s += result;
			return sl_true;
		}

		static sl_bool ParseValue(sl_char8*& s, sl_char8* end, Color& _out)
		{
			sl_reg result = Color::parse(&_out, s, 0, end - s);
			if (result == SLIB_PARSE_ERROR) {
				return sl_false;
			}
			s += result;
			return sl_true;
		}

		class Length
		{
		public:
			Scalar value = 0;
			sl_bool flagPercentage = sl_false;

		public:
			Scalar getValue(Scalar container)
			{
				if (flagPercentage) {
					return container * value / (Scalar)100;
				} else {
					return value;
				}
			}

			Length& operator=(Scalar _value)
			{
				value = _value;
				flagPercentage = sl_false;
				return *this;
			}

		};

		static sl_bool ParseValue(sl_char8*& s, sl_char8* end, Length& _out)
		{
			if (!(ParseScalar(s, end, _out.value))) {
				return sl_false;
			}
			SkipWhitespaces(s, end);
			sl_size n = end - s;
			if (n >= 1) {
				if (*s == '%') {
					_out.flagPercentage = sl_true;
					s++;
				} else if (n >= 2) {
					switch (*s) {
						case 'e':
							{
								sl_char8 c = s[1];
								if (c == 'm') {
									_out.value *= g_fontSize;
									s += 2;
								} else if (c == 'x') {
									_out.value *= g_fontSize / (Scalar)2;
									s += 2;
								}
								break;
							}
						case 'p':
							{
								sl_char8 c = s[1];
								if (c == 'x') {
									s += 2;
								} else if (c == 't') {
									_out.value *= (Scalar)1.33333333;
									s += 2;
								} else if (c == 'c') {
									_out.value *= (Scalar)16;
									s += 2;
								}
								break;
							}
						case 'i':
							if (s[1] == 'n') {
								_out.value *= (Scalar)96;
								s += 2;
							}
							break;
						case 'c':
							if (s[1] == 'm') {
								_out.value *= (Scalar)37.7952755;
								s += 2;
							}
							break;
						case 'm':
							if (s[1] == 'm') {
								_out.value *= (Scalar)3.77952755;
								s += 2;
							}
							break;
					}
				}
			}
			return sl_true;
		}

		struct ViewBox
		{
			Scalar x = 0;
			Scalar y = 0;
			Scalar width = 0;
			Scalar height = 0;
		};

		static sl_bool ParseValue(sl_char8*& s, sl_char8* end, ViewBox& _out)
		{
			for (sl_uint32 i = 0; i < 4; i++) {
				if (s >= end) {
					return sl_false;
				}
				sl_char8 c = *s;
				if (i) {
					if (c == ',') {
						s++;
						SkipWhitespaces(s, end);
						if (s >= end) {
							return i == 3;
						}
						c = *s;
					}
				}
				if (c != ',') {
					Scalar value;
					if (!(ParseScalar(s, end, value))) {
						return sl_false;
					}
					switch (i) {
						case 0:
							_out.x = value;
							break;
						case 1:
							_out.y = value;
							break;
						case 2:
							_out.width = value;
							break;
						case 3:
							_out.height = value;
							break;
					}
					SkipWhitespaces(s, end);
				}
			}
			return sl_true;
		}

		struct PreserveAspectRatio
		{
			Alignment align = Alignment::MiddleCenter;
			sl_bool flagContain = sl_true;
		};

		static sl_bool ParseValue(sl_char8*& s, sl_char8* end, PreserveAspectRatio& _out)
		{
			sl_char8* t = s;
			SkipNoWhitespaces(t, end);
			StringView align(s, t - s);
			if (align == StringView::literal("none")) {
				_out.align = Alignment::Default;
			} else if (align == StringView::literal("xMinYMin")) {
				_out.align = Alignment::TopLeft;
			} else if (align == StringView::literal("xMidYMin")) {
				_out.align = Alignment::TopCenter;
			} else if (align == StringView::literal("xMaxYMin")) {
				_out.align = Alignment::TopRight;
			} else if (align == StringView::literal("xMinYMid")) {
				_out.align = Alignment::MiddleLeft;
			} else if (align == StringView::literal("xMidYMid")) {
				_out.align = Alignment::MiddleCenter;
			} else if (align == StringView::literal("xMaxYMid")) {
				_out.align = Alignment::MiddleRight;
			} else if (align == StringView::literal("xMinYMax")) {
				_out.align = Alignment::BottomLeft;
			} else if (align == StringView::literal("xMidYMax")) {
				_out.align = Alignment::BottomCenter;
			} else if (align == StringView::literal("xMaxYMax")) {
				_out.align = Alignment::BottomRight;
			} else {
				return sl_false;
			}
			s = t;
			SkipWhitespaces(s, end);
			if (s >= end) {
				return sl_true;
			}
			t = s;
			SkipNoWhitespaces(t, end);
			StringView suffix(s, t - s);
			if (suffix == StringView::literal("meet")) {
				_out.flagContain = sl_true;
			} else if (suffix == StringView::literal("slice")) {
				_out.flagContain = sl_false;
			} else {
				return sl_true;
			}
			s = t;
			return sl_true;
		}

		static sl_bool ParseValue(sl_char8*& s, sl_char8* end, LineCap& _out)
		{
			sl_char8* t = s;
			SkipNoWhitespaces(t, end);
			StringView v(s, t - s);
			if (v == StringView::literal("butt")) {
				_out = LineCap::Flat;
			} else if (v == StringView::literal("round")) {
				_out = LineCap::Round;
			} else if (v == StringView::literal("square")) {
				_out = LineCap::Square;
			} else {
				return sl_false;
			}
			s = t;
			return sl_true;
		}

		static sl_bool ParseValue(sl_char8*& s, sl_char8* end, LineJoin& _out)
		{
			sl_char8* t = s;
			SkipNoWhitespaces(t, end);
			StringView v(s, t - s);
			if (v == StringView::literal("arcs")) {
				_out = LineJoin::Round; // Not Supported Correctly
			} else if (v == StringView::literal("bevel")) {
				_out = LineJoin::Bevel;
			} else if (v == StringView::literal("miter")) {
				_out = LineJoin::Miter;
			} else if (v == StringView::literal("miter-clip")) {
				_out = LineJoin::Miter; // Not Supported Correctly
			} else if (v == StringView::literal("round")) {
				_out = LineJoin::Round;
			} else {
				return sl_false;
			}
			s = t;
			return sl_true;
		}

		static sl_bool ParseValue(sl_char8*& s, sl_char8* end, FillMode& _out)
		{
			sl_char8* t = s;
			SkipNoWhitespaces(t, end);
			StringView v(s, t - s);
			if (v == StringView::literal("nonzero")) {
				_out = FillMode::Winding;
			} else if (v == StringView::literal("evenodd")) {
				_out = FillMode::Alternate;
			} else {
				return sl_false;
			}
			s = t;
			return sl_true;
		}

		template <class T>
		static sl_size ParseValues(sl_char8*& s, sl_char8* end, T* _out, sl_size count)
		{
			for (sl_size i = 0; i < count; i++) {
				SkipValueSeparator(s, end);
				sl_char8* t = s;
				if (!(ParseValue(t, end, _out[i]))) {
					return i;
				}
				s = t;
			}
			return count;
		}

		template <class T>
		static sl_bool ParseValue(sl_char8*& s, sl_char8* end, List<T>& _out)
		{
			while (s < end) {
				T value;
				if (!(ParseValue(s, end, value))) {
					return sl_false;
				}
				if (!(_out.add_NoLock(Move(value)))) {
					return sl_false;
				}
				SkipValueSeparator(s, end);
			}
			return sl_true;
		}

		template <class T>
		static sl_reg ParseFunctionCall(sl_char8*& s, sl_char8* end, T* _out, sl_size count)
		{
			SkipWhitespaces(s, end);
			if (s >= end) {
				return -1;
			}
			if (*s != '(') {
				return -1;
			}
			s++;
			sl_size n = ParseValues(s, end, _out, count);
			SkipWhitespaces(s, end);
			if (s >= end) {
				return -1;
			}
			if (*s != ')') {
				return -1;
			}
			s++;
			return n;
		}

		static sl_bool ParseValue(sl_char8*& s, sl_char8* end, Ref<GraphicsPath>& _out)
		{
			Ref<GraphicsPath> path = GraphicsPath::create();
			if (path.isNull()) {
				return sl_false;
			}
			Scalar lastX = 0;
			Scalar lastY = 0;
			Scalar lastControlX = 0;
			Scalar lastControlY = 0;
			for (;;) {
				if (s >= end) {
					break;
				}
				sl_char8 cmd = *(s++);
				Scalar v[8];
				sl_bool flagParseResult = sl_false;

				switch (cmd) {
					case 'M':
						flagParseResult = ParseValues(s, end, v, 2) == 2;
						if (flagParseResult) {
							lastX = v[0];
							lastY = v[1];
							path->moveTo(lastX, lastY);
							while (ParseValues(s, end, v, 2) == 2) {
								lastX = v[0];
								lastY = v[1];
								path->lineTo(lastX, lastY);
							}
							lastControlX = lastX;
							lastControlY = lastY;
						}
						break;
					case 'm':
						flagParseResult = ParseValues(s, end, v, 2) == 2;
						if (flagParseResult) {
							lastX += v[0];
							lastY += v[1];
							path->moveTo(lastX, lastY);
							while (ParseValues(s, end, v, 2) == 2) {
								lastX += v[0];
								lastY += v[1];
								path->lineTo(lastX, lastY);
							}
							lastControlX = lastX;
							lastControlY = lastY;
						}
						break;
					case 'L':
						while (ParseValues(s, end, v, 2) == 2) {
							flagParseResult = sl_true;
							lastX = v[0];
							lastY = v[1];
							path->lineTo(lastX, lastY);
						}
						lastControlX = lastX;
						lastControlY = lastY;
						break;
					case 'l':
						while (ParseValues(s, end, v, 2) == 2) {
							flagParseResult = sl_true;
							lastX += v[0];
							lastY += v[1];
							path->lineTo(lastX, lastY);
						}
						lastControlX = lastX;
						lastControlY = lastY;
						break;
					case 'H':
						while (ParseValues(s, end, v, 1) == 1) {
							flagParseResult = sl_true;
							lastX = *v;
							path->lineTo(lastX, lastY);
						}
						lastControlX = lastX;
						break;
					case 'h':
						while (ParseValues(s, end, v, 1) == 1) {
							flagParseResult = sl_true;
							lastX += *v;
							path->lineTo(lastX, lastY);
						}
						lastControlX = lastX;
						break;
					case 'V':
						while (ParseValues(s, end, v, 1) == 1) {
							flagParseResult = sl_true;
							lastY = *v;
							path->lineTo(lastX, lastY);
						}
						lastControlY = lastY;
						break;
					case 'v':
						while (ParseValues(s, end, v, 1) == 1) {
							flagParseResult = sl_true;
							lastY += *v;
							path->lineTo(lastX, lastY);
						}
						lastControlY = lastY;
						break;
					case 'C':
						while (ParseValues(s, end, v, 6) == 6) {
							flagParseResult = sl_true;
							lastControlX = v[2];
							lastControlY = v[3];
							lastX = v[4];
							lastY = v[5];
							path->cubicTo(v[0], v[1], lastControlX, lastControlY, lastX, lastY);
						}
						break;
					case 'c':
						while (ParseValues(s, end, v, 6) == 6) {
							flagParseResult = sl_true;
							Scalar cx = lastX + v[0];
							Scalar cy = lastY + v[1];
							lastControlX = lastX + v[2];
							lastControlY = lastY + v[3];
							lastX += v[4];
							lastY += v[5];
							path->cubicTo(cx, cy, lastControlX, lastControlY, lastX, lastY);
						}
						break;
					case 'S':
						while (ParseValues(s, end, v, 4) == 4) {
							flagParseResult = sl_true;
							Scalar cx = lastX + (lastX - lastControlX);
							Scalar cy = lastY + (lastY - lastControlY);
							lastControlX = v[0];
							lastControlY = v[1];
							lastX = v[2];
							lastY = v[3];
							path->cubicTo(cx, cy, lastControlX, lastControlY, lastX, lastY);
						}
						break;
					case 's':
						while (ParseValues(s, end, v, 4) == 4) {
							flagParseResult = sl_true;
							Scalar cx = lastX + (lastX - lastControlX);
							Scalar cy = lastY + (lastY - lastControlY);
							lastControlX = lastX + v[0];
							lastControlY = lastY + v[1];
							lastX += v[2];
							lastY += v[3];
							path->cubicTo(cx, cy, lastControlX, lastControlY, lastX, lastY);
						}
						break;
					case 'Q':
						while (ParseValues(s, end, v, 4) == 4) {
							flagParseResult = sl_true;
							lastControlX = v[0];
							lastControlY = v[1];
							lastX = v[2];
							lastY = v[3];
							path->conicTo(lastControlX, lastControlY, lastX, lastY);
						}
						break;
					case 'q':
						while (ParseValues(s, end, v, 4) == 4) {
							flagParseResult = sl_true;
							lastControlX = lastX + v[0];
							lastControlY = lastY + v[1];
							lastX += v[2];
							lastY += v[3];
							path->conicTo(lastControlX, lastControlY, lastX, lastY);
						}
						break;
					case 'T':
						while (ParseValues(s, end, v, 2) == 2) {
							flagParseResult = sl_true;
							lastControlX = lastX + (lastX - lastControlX);
							lastControlY = lastY + (lastY - lastControlY);
							lastX = v[0];
							lastY = v[1];
							path->conicTo(lastControlX, lastControlY, lastX, lastY);
						}
						break;
					case 't':
						while (ParseValues(s, end, v, 2) == 2) {
							flagParseResult = sl_true;
							lastControlX = lastX + (lastX - lastControlX);
							lastControlY = lastY + (lastY - lastControlY);
							lastX += v[0];
							lastY += v[1];
							path->conicTo(lastControlX, lastControlY, lastX, lastY);
						}
						break;
					case 'A':
						while (ParseValues(s, end, v, 7) == 7) {
							flagParseResult = sl_true;
							Scalar sx = lastX;
							Scalar sy = lastY;
							lastX = v[5];
							lastY = v[6];
							path->addArc(sx, sy, lastX, lastY, v[0], v[1], v[2], !(Math::isAlmostZero(v[3])), !(Math::isAlmostZero(v[4])));
						}
						break;
					case 'a':
						while (ParseValues(s, end, v, 7) == 7) {
							flagParseResult = sl_true;
							Scalar sx = lastX;
							Scalar sy = lastY;
							lastX += v[5];
							lastY += v[6];
							path->addArc(sx, sy, lastX, lastY, v[0], v[1], v[2], !(Math::isAlmostZero(v[3])), !(Math::isAlmostZero(v[4])));
						}
						break;
					case 'Z':
					case 'z':
						path->closeSubpath();
						flagParseResult = sl_true;
						break;
				}
				if (!flagParseResult) {
					break;
				}
				SkipValueSeparator(s, end);
			}
			_out = Move(path);
			return sl_true;
		}

		static sl_bool ParseValue(sl_char8*& s, sl_char8* end, Matrix3& _out)
		{
			_out = Matrix3::identity();
			Scalar v[6];
			for (;;) {
				SkipWhitespaces(s, end);
				if (SkipPattern(s, end, StringView::literal("matrix"))) {
					if (ParseFunctionCall(s, end, v, 6) != 6) {
						return sl_false;
					}
					Matrix3 t(v[0], v[1], 0, v[2], v[3], 0, v[4], v[5], (Scalar)1);
					_out = t * _out;
				} else if (SkipPattern(s, end, StringView::literal("translate"))) {
					sl_reg n = ParseFunctionCall(s, end, v, 3);
					if (n == 2) {
						Transform2::preTranslate(_out, v[0], v[1]);
					} else if (n == 1) {
						Transform2::preTranslate(_out, v[0], 0);
					} else {
						return sl_false;
					}
				} else if (SkipPattern(s, end, StringView::literal("scale"))) {
					sl_reg n = ParseFunctionCall(s, end, v, 3);
					if (n == 2) {
						Transform2::preScale(_out, v[0], v[1]);
					} else if (n == 1) {
						Transform2::preScale(_out, v[0], v[0]);
					} else {
						return sl_false;
					}
				} else if (SkipPattern(s, end, StringView::literal("rotate"))) {
					sl_reg n = ParseFunctionCall(s, end, v, 3);
					if (n == 1) {
						Transform2::preRotate(_out, Math::getRadianFromDegrees(v[0]));
					} else if (n == 3) {
						Transform2::preRotate(_out, v[1], v[2], Math::getRadianFromDegrees(v[0]));
					} else {
						return sl_false;
					}
				} else if (SkipPattern(s, end, StringView::literal("skewX"))) {
					if (ParseFunctionCall(s, end, v, 1) != 1) {
						return sl_false;
					}
					Transform2::preSkewX(_out, Math::tan(Math::getRadianFromDegrees(v[0])));
				} else if (SkipPattern(s, end, StringView::literal("skewY"))) {
					if (ParseFunctionCall(s, end, v, 1) != 1) {
						return sl_false;
					}
					Transform2::preSkewY(_out, Math::tan(Math::getRadianFromDegrees(v[0])));
				} else {
					break;
				}
			}
			return sl_true;
		}

		template <class T>
		static sl_bool ParseValue(const StringView& str, T& outValue, sl_bool& outFlagDefined)
		{
			sl_char8* data = str.getData();
			sl_char8* end = data + str.getLength();
			SkipWhitespaces(data, end);
			if (data == end) {
				return sl_true;
			}
			if (!(ParseValue(data, end, outValue))) {
				return sl_false;
			}
			SkipWhitespaces(data, end);
			if (data != end) {
				return sl_false;
			}
			outFlagDefined = sl_true;
			return sl_true;
		}

		template <class T>
		SLIB_INLINE static sl_bool ParseValue(const StringView& str, Define<T>& _out)
		{
			return ParseValue(str, *_out, _out.flagDefined);
		}

		template <class T>
		SLIB_INLINE static sl_bool ParseValue(const StringView& str, DefinePrimitive<T>& _out)
		{
			return ParseValue(str, *_out, _out.flagDefined);
		}

		template <class T>
		SLIB_INLINE static sl_bool ParseValue(const StringView& str, T& _out)
		{
			sl_bool flagDefined;
			return ParseValue(str, _out, flagDefined);
		}

		struct RenderParam
		{
			Scalar containerWidth;
			Scalar containerHeight;
		};

		static void ApplyOpacity(Color& c, Scalar opacity)
		{
			if (opacity <= (Scalar)0.999) {
				c.a = (sl_uint8)(Math::clamp0_255((sl_int32)((Scalar)(c.a) * opacity)));
			}
		}

#define GET_ATTRIBUTE(TYPE, NAME, ATTR) \
		SLIB_STATIC_STRING(name_##NAME, ATTR) \
		TYPE NAME = getAttribute(name_##NAME).trim();

#define PARSE_ATTRIBUTE(NAME, ATTR) \
		SLIB_STATIC_STRING(name_##NAME, ATTR) \
		ParseValue(getAttribute(name_##NAME), NAME);

#define GET_STYLER_ATTRIBUTE(STYLER, TYPE, NAME, ATTR) \
		SLIB_STATIC_STRING(name_##NAME, ATTR) \
		TYPE NAME = STYLER.getAttribute(name_##NAME).trim();

#define PARSE_STYLER_ATTRIBUTE(STYLER, NAME, ATTR) \
		SLIB_STATIC_STRING(name_##NAME, ATTR) \
		ParseValue(STYLER.getAttribute(name_##NAME), NAME);

		class Group;
		class Document;

		class Styler
		{
		public:
			XmlElement* xml = sl_null;
			Document* document = sl_null;
			Define< List<CascadingStyleDeclarations> > styles;

		public:
			String getAttribute(const String& name);

		};

		enum class PaintType
		{
			Color = 0,
			Url = 1,
			LinearGradient = 2,
			RadialGradient = 3
		};

		class Paint : public Referable
		{
		public:
			PaintType type;

		};

		class ColorPaint : public Paint
		{
		public:
			Color color;

		public:
			ColorPaint()
			{
				type = PaintType::Color;
			}

			ColorPaint(const Color& _color)
			{
				type = PaintType::Color;
				color = _color;
			}

		public:
			static const Ref<ColorPaint>& black()
			{
				SLIB_SAFE_LOCAL_STATIC(Ref<ColorPaint>, ret, new ColorPaint(Color::Black))
					return ret;
			}

		};

		enum class GradientUnits
		{
			ObjectBoundingBox = 0,
			UserSpaceOnUse = 1,
		};

		class UrlPaint : public Paint
		{
		public:
			String url;

			// Gradient
			GradientUnits gradientUnits;
			Define<Matrix3> transform;
			List<Color> stopColors;
			List<Scalar> stopOffsets;

			// Linear Gradient
			Define<Length> x1, x2, y1, y2;

			// Radial Gradient
			Define<Length> cx, cy, fr, fx, fy, r;

		public:
			UrlPaint()
			{
				type = PaintType::Url;
			}

		public:
			void load(Document* doc, Scalar opacity);

		private:
			void loadGradient(Styler& e, Scalar _opacity)
			{
				GET_STYLER_ATTRIBUTE(e, String, strGradientUnits, "gradientUnits")
				if (strGradientUnits == StringView::literal("userSpaceOnUse")) {
					gradientUnits = GradientUnits::UserSpaceOnUse;
				} else {
					gradientUnits = GradientUnits::ObjectBoundingBox;
				}
				PARSE_STYLER_ATTRIBUTE(e, transform, "transform")
				if (!(transform.flagDefined)) {
					PARSE_STYLER_ATTRIBUTE(e, transform, "gradientTransform")
				}
				ListElements< Ref<XmlElement> > stops(e.xml->getChildElements());
				for (sl_size i = 0; i < stops.count; i++) {
					Styler stop;
					stop.xml = stops[i].get();
					stop.document = e.document;
					DefinePrimitive<Scalar> offset, opacity;
					Define<Color> color;
					PARSE_STYLER_ATTRIBUTE(stop, offset, "offset")
					PARSE_STYLER_ATTRIBUTE(stop, color, "stop-color")
					PARSE_STYLER_ATTRIBUTE(stop, opacity, "stop-opacity")
					if (offset.flagDefined && color.flagDefined) {
						if (opacity.flagDefined) {
							ApplyOpacity(*color, *opacity * _opacity);
						} else {
							ApplyOpacity(*color, _opacity);
						}
						stopOffsets.add_NoLock(*offset);
						stopColors.add_NoLock(*color);
					}
				}
				if (stopOffsets.getCount() != stopColors.getCount()) {
					stopOffsets.setNull();
					stopColors.setNull();
				}
			}

			void loadLinearGradient(Styler& e, Scalar opacity)
			{
				loadGradient(e, opacity);
				PARSE_STYLER_ATTRIBUTE(e, x1, "x1")
				PARSE_STYLER_ATTRIBUTE(e, x2, "x2")
				PARSE_STYLER_ATTRIBUTE(e, y1, "y1")
				PARSE_STYLER_ATTRIBUTE(e, y2, "y2")
				if (!(x2.flagDefined)) {
					x2.flagPercentage = sl_true;
					x2.value = (Scalar)100;
				}
				type = PaintType::LinearGradient;
			}

			void loadRadialGradient(Styler& e, Scalar opacity)
			{
				loadGradient(e, opacity);
				PARSE_STYLER_ATTRIBUTE(e, cx, "cx")
				if (!(cx.flagDefined)) {
					cx.flagPercentage = sl_true;
					cx.value = (Scalar)50;
				}
				PARSE_STYLER_ATTRIBUTE(e, cy, "cy")
				if (!(cy.flagDefined)) {
					cy.flagPercentage = sl_true;
					cy.value = (Scalar)50;
				}
				PARSE_STYLER_ATTRIBUTE(e, fr, "fr")
				PARSE_STYLER_ATTRIBUTE(e, fx, "fx")
				if (!(fx.flagDefined)) {
					*fx = *cx;
				}
				PARSE_STYLER_ATTRIBUTE(e, fy, "fy")
				if (!(fy.flagDefined)) {
					*fy = *cy;
				}
				PARSE_STYLER_ATTRIBUTE(e, r, "r")
				if (!(r.flagDefined)) {
					r.flagPercentage = sl_true;
					r.value = (Scalar)50;
				}
				type = PaintType::RadialGradient;
			}

		};

		static sl_bool ParseValue(sl_char8*& s, sl_char8* end, Ref<Paint>& _out)
		{
			Color color;
			if (ParseValue(s, end, color)) {
				if (color.isZero()) {
					_out.setNull();
					return sl_true;
				}
				ColorPaint* ret = new ColorPaint;
				if (ret) {
					ret->color = color;
					_out = ret;
					return sl_true;
				}
			} else if (SkipPattern(s, end, StringView::literal("url"))) {
				String url;
				if (ParseFunctionCall(s, end, &url, 1) == 1) {
					UrlPaint* ret = new UrlPaint;
					if (ret) {
						ret->url = url.trim();
						_out = ret;
						return sl_true;
					}
				}
			}
			return sl_false;
		}

		class Element : public Referable, public Styler
		{
		public:
			Element* parent = sl_null;

			Define< Ref<Paint> > stroke;
			Define<Length> strokeWidth;
			Define< List<Length> > strokeDashArray;
			Define<Length> strokeDashOffset;
			DefinePrimitive<LineCap> strokeLineCap;
			DefinePrimitive<LineJoin> strokeLineJoin;
			DefinePrimitive<Scalar> strokeMiterLimit;
			DefinePrimitive<Scalar> strokeOpacity;

			Define< Ref<Paint> > fill;
			DefinePrimitive<Scalar> fillOpacity;
			DefinePrimitive<FillMode> fillRule;

			Define<Matrix3> transform;
			DefinePrimitive<Scalar> opacity;

			Define< Ref<Pen> > pen;
			Define< Ref<Brush> > brush;
			DefinePrimitive<Scalar> finalOpacity;

		public:
			virtual void load()
			{
				PARSE_ATTRIBUTE(transform, "transform")
				PARSE_ATTRIBUTE(opacity, "opacity")
			}

			virtual void render(Canvas* canvas, RenderParam& param) = 0;

			virtual Rectangle getBounds(RenderParam& param) = 0;

			Ref<XmlElement> getXmlByUrl(const StringView& url);

#define DEFINE_INHERITED_ATTRIBUTE(TYPE, NAME, GETTER, ATTR, DEFAULT) \
			TYPE& GETTER() \
			{ \
				if (NAME.flagDefined) { \
					return *NAME; \
				} \
				SLIB_STATIC_STRING(name, ATTR); \
				String value = getAttribute(name); \
				if (value.isNotNull()) { \
					ParseValue(value, NAME); \
					if (NAME.flagDefined) { \
						return *NAME; \
					} \
				} \
				if (parent) { \
					*NAME = parent->GETTER(); \
				} else { \
					*NAME = DEFAULT; \
				} \
				NAME.flagDefined = sl_true; \
				return *NAME; \
			}

			DEFINE_INHERITED_ATTRIBUTE(Ref<Paint>, stroke, getStroke, "stroke", sl_null)
			DEFINE_INHERITED_ATTRIBUTE(Length, strokeWidth, getStrokeWidth, "stroke-width", (Scalar)1)
			DEFINE_INHERITED_ATTRIBUTE(List<Length>, strokeDashArray, getStrokeDashArray, "stroke-dasharray", sl_null)
			DEFINE_INHERITED_ATTRIBUTE(Length, strokeDashOffset, getStrokeDashOffset, "stroke-dashoffset", 0)
			DEFINE_INHERITED_ATTRIBUTE(LineCap, strokeLineCap, getStrokeLineCap, "stroke-linecap", LineCap::Flat)
			DEFINE_INHERITED_ATTRIBUTE(LineJoin, strokeLineJoin, getStrokeLineJoin, "stroke-linejoin", LineJoin::Miter)
			DEFINE_INHERITED_ATTRIBUTE(Scalar, strokeMiterLimit, getStrokeMiterLimit, "stroke-miterlimit", (Scalar)4)
			DEFINE_INHERITED_ATTRIBUTE(Scalar, strokeOpacity, getStrokeOpacity, "stroke-opacity", (Scalar)1)

			DEFINE_INHERITED_ATTRIBUTE(Ref<Paint>, fill, getFill, "fill", ColorPaint::black())
			DEFINE_INHERITED_ATTRIBUTE(Scalar, fillOpacity, getFillOpacity, "fill-opacity", (Scalar)1)
			DEFINE_INHERITED_ATTRIBUTE(FillMode, fillRule, getFillRule, "fill-rule", FillMode::Winding)

			Scalar getFinalOpacity()
			{
				if (finalOpacity.flagDefined) {
					return *finalOpacity;
				}
				if (opacity.flagDefined) {
					if (parent) {
						*finalOpacity = parent->getFinalOpacity() * (*opacity);
					} else {
						*finalOpacity = *opacity;
					}
				} else {
					if (parent) {
						*finalOpacity = parent->getFinalOpacity();
					} else {
						*finalOpacity = (Scalar)1;
					}
				}
				finalOpacity.flagDefined = sl_true;
				return *finalOpacity;
			}

			void loadPen()
			{
				getStroke();
				getStrokeOpacity();
				getStrokeWidth();
				getStrokeDashArray();
				getStrokeLineCap();
				getStrokeLineJoin();
				getStrokeMiterLimit();
			}

			Ref<Pen>& getPen(RenderParam& param)
			{
				sl_bool flagCreate = sl_false;
				Scalar width = (Scalar)1;
				if (pen.flagDefined) {
					if (pen.isNotNull()) {
						width = getStrokeWidth().getValue(param.containerWidth);
						if (!(Math::isAlmostZero(pen->getWidth() - width))) {
							flagCreate = sl_true;
							pen.setNull();
						}
					}
				} else {
					pen.flagDefined = sl_true;
					flagCreate = sl_true;
					width = getStrokeWidth().getValue(param.containerWidth);
				}
				if (flagCreate) {
					const Ref<Paint>& paint = getStroke();
					if (paint.isNotNull()) {
						if (paint->type == PaintType::Color) {
							PenDesc desc;
							desc.color = ((ColorPaint*)(paint.get()))->color;
							ApplyOpacity(desc.color, getFinalOpacity() * getStrokeOpacity());
							desc.width = width;
							desc.style = getStrokeDashArray().isNotNull() ? PenStyle::Dash : PenStyle::Solid;
							desc.cap = getStrokeLineCap();
							desc.join = getStrokeLineJoin();
							desc.miterLimit = getStrokeMiterLimit();
							*pen = Pen::create(desc);
						}
					}
				}
				return *pen;
			}

			void loadBrush()
			{
				getFill();
				Scalar opacity = getFillOpacity() * getFinalOpacity();
				if (fill.isNotNull()) {
					if (fill->type == PaintType::Url) {
						((UrlPaint*)(fill.get()))->load(document, opacity);
					}
				}
				getFillRule();
			}

			Ref<Brush>& getBrush(RenderParam& param)
			{
				const Ref<Paint>& paint = getFill();
				if (paint.isNotNull()) {
					if (paint->type == PaintType::Color) {
						if (brush.flagDefined) {
							return *brush;
						}
						createColorBrush(*((ColorPaint*)(paint.get())));
					} else if (paint->type == PaintType::LinearGradient) {
						createLinearGradientBrush(*((UrlPaint*)(paint.get())), param);
					} else if (paint->type == PaintType::RadialGradient) {
						createRadialGradientBrush(*((UrlPaint*)(paint.get())), param);
					}
				}
				brush.flagDefined = sl_true;
				return *brush;
			}

			void createColorBrush(ColorPaint& paint)
			{
				Color color = paint.color;
				ApplyOpacity(color, getFinalOpacity() * getFillOpacity());
				*brush = Brush::createSolidBrush(color);
			}

			void createLinearGradientBrush(UrlPaint& paint, RenderParam& param)
			{
				Point pt1, pt2;
				if (paint.gradientUnits == GradientUnits::UserSpaceOnUse) {
					pt1.x = paint.x1.getValue(param.containerWidth);
					pt1.y = paint.y1.getValue(param.containerHeight);
					pt2.x = paint.x2.getValue(param.containerWidth);
					pt2.y = paint.y2.getValue(param.containerHeight);
				} else {
					Rectangle bounds = getBounds(param);
					pt1.x = paint.x1.getValue((Scalar)1) * bounds.getWidth() + bounds.left;
					pt1.y = paint.y1.getValue((Scalar)1) * bounds.getHeight() + bounds.top;
					pt2.x = paint.x2.getValue((Scalar)1) * bounds.getWidth() + bounds.left;
					pt2.y = paint.y2.getValue((Scalar)1) * bounds.getHeight() + bounds.top;
				}
				if (paint.transform.flagDefined) {
					pt1 = paint.transform.transformPosition(pt1);
					pt2 = paint.transform.transformPosition(pt2);
				}
				if (brush.flagDefined && brush.isNotNull()) {
					BrushDesc& desc = brush->getDesc();
					if (desc.style == BrushStyle::LinearGradient) {
						GradientBrushDetail* detail = (GradientBrushDetail*)(desc.detail.get());
						if (pt1.isAlmostEqual(detail->point1) && pt2.isAlmostEqual(detail->point2)) {
							return;
						}
					}
				}
				*brush = Brush::createLinearGradientBrush(pt1, pt2, (sl_uint32)(paint.stopColors.getCount()), paint.stopColors.getData(), paint.stopOffsets.getData());
			}

			void createRadialGradientBrush(UrlPaint& paint, RenderParam& param)
			{
				Point center;
				Scalar r;
				if (paint.gradientUnits == GradientUnits::UserSpaceOnUse) {
					center.x = paint.cx.getValue(param.containerWidth);
					center.y = paint.cy.getValue(param.containerHeight);
					r = paint.r.getValue(param.containerWidth);
				} else {
					Rectangle bounds = getBounds(param);
					center.x = paint.cx.getValue((Scalar)1) * bounds.getWidth() + bounds.left;
					center.y = paint.cy.getValue((Scalar)1) * bounds.getHeight() + bounds.top;
					r = paint.r.getValue((Scalar)1) * bounds.getWidth();
				}
				if (paint.transform.flagDefined) {
					center = paint.transform.transformPosition(center);
					r = paint.transform.transformDirection(Point(r, 0)).getLength();
				}
				if (brush.flagDefined && brush.isNotNull()) {
					BrushDesc& desc = brush->getDesc();
					if (desc.style == BrushStyle::RadialGradient) {
						GradientBrushDetail* detail = (GradientBrushDetail*)(desc.detail.get());
						if (center.isAlmostEqual(detail->point1) && Math::isAlmostZero(detail->radius - r)) {
							return;
						}
					}
				}
				*brush = Brush::createRadialGradientBrush(center, r, (sl_uint32)(paint.stopColors.getCount()), paint.stopColors.getData(), paint.stopOffsets.getData());
			}

		};

		class Loaders : public CHashMap< String, Function<Element*()> >
		{
		public:
			Loaders();

		};

		SLIB_SAFE_STATIC_GETTER(Loaders, GetLoaders)

		class Group : public Element
		{
		public:
			CList< Ref<Element> > children;

		public:
			void load() override
			{
				Element::load();
				sl_size n = xml->getChildCount();
				for (sl_size i = 0; i < n; i++) {
					Ref<XmlElement> child = xml->getChildElement(i);
					if (child.isNotNull()) {
						loadChild(child.get());
					}
				}
			}

			Element* loadChild(XmlElement* xml)
			{
				Loaders* loaders = GetLoaders();
				if (!loaders) {
					return sl_null;
				}
				String name = xml->getName();
				Function<Element*()> getter = loaders->getValue_NoLock(name);
				if (getter.isNotNull()) {
					Ref<Element> element = getter();
					if (element.isNotNull()) {
						element->parent = this;
						element->document = document;
						element->xml = xml;
						element->load();
						Element* ret = element.get();
						children.add_NoLock(Move(element));
						return ret;
					}
				}
				return sl_null;
			}

			void render(Canvas* canvas, RenderParam& param) override
			{
				ListElements< Ref<Element> > items(children);
				for (sl_size i = 0; i < items.count; i++) {
					Ref<Element>& item = items[i];
					if (item->transform.flagDefined) {
						CanvasStateScope scope(canvas);
						canvas->concatMatrix(*(item->transform));
						item->render(canvas, param);
					} else {
						item->render(canvas, param);
					}
				}
			}

			Rectangle getBounds(RenderParam& param) override
			{
				ListElements< Ref<Element> > items(children);
				if (!(items.count)) {
					return Rectangle::zero();
				}
				Rectangle ret;
				for (sl_size i = 0; i < items.count; i++) {
					if (i) {
						ret.mergeRectangle(items[i]->getBounds(param));
					} else {
						ret = items[i]->getBounds(param);
					}
				}
				return ret;
			}

		};

		class Rect : public Element
		{
		public:
			Length x;
			Length y;
			Length width;
			Length height;
			Define<Length> rx;
			Define<Length> ry;

		public:
			void load() override
			{
				Element::load();
				PARSE_ATTRIBUTE(x, "x")
				PARSE_ATTRIBUTE(y, "y")
				PARSE_ATTRIBUTE(width, "width")
				PARSE_ATTRIBUTE(height, "height")
				PARSE_ATTRIBUTE(rx, "rx")
				PARSE_ATTRIBUTE(ry, "ry")
				loadPen();
				loadBrush();
			}

			void render(Canvas* canvas, RenderParam& param) override
			{
				Scalar left = x.getValue(param.containerWidth);
				Scalar top = y.getValue(param.containerHeight);
				Scalar w = width.getValue(param.containerWidth);
				Scalar h = height.getValue(param.containerHeight);
				if (rx.flagDefined || ry.flagDefined) {
					Scalar radiusX, radiusY;
					if (rx.flagDefined) {
						if (ry.flagDefined) {
							radiusX = rx.getValue(param.containerWidth);
							radiusY = ry.getValue(param.containerHeight);
						} else {
							radiusX = radiusY = rx.getValue(param.containerWidth);
						}
					} else {
						radiusX = radiusY = ry.getValue(param.containerHeight);
					}
					canvas->drawRoundRect(left, top, w, h, radiusX, radiusY, getPen(param), getBrush(param));
				} else {
					canvas->drawRectangle(left, top, w, h, getPen(param), getBrush(param));
				}
			}

			Rectangle getBounds(RenderParam& param) override
			{
				Scalar left = x.getValue(param.containerWidth);
				Scalar top = y.getValue(param.containerHeight);
				Scalar w = width.getValue(param.containerWidth);
				Scalar h = height.getValue(param.containerHeight);
				return Rectangle(left, top, left + w, top + h);
			}

		};

		class Ellipse : public Element
		{
		public:
			Length cx;
			Length cy;
			Define<Length> rx;
			Define<Length> ry;

		public:
			void load() override
			{
				Element::load();
				PARSE_ATTRIBUTE(cx, "cx")
				PARSE_ATTRIBUTE(cy, "cy")
				PARSE_ATTRIBUTE(rx, "rx")
				PARSE_ATTRIBUTE(ry, "ry")
				if (rx.flagDefined) {
					if (!(ry.flagDefined)) {
						*ry = *rx;
					}
				} else {
					*rx = *ry;
				}
				loadPen();
				loadBrush();
			}

			void render(Canvas* canvas, RenderParam& param) override
			{
				Scalar centerX = cx.getValue(param.containerWidth);
				Scalar centerY = cy.getValue(param.containerHeight);
				Scalar radiusX = rx.getValue(param.containerWidth);
				Scalar radiusY = ry.getValue(param.containerWidth);
				canvas->drawEllipse(centerX - radiusX, centerY - radiusY, radiusX + radiusX, radiusY + radiusY, getPen(param), getBrush(param));
			}

			Rectangle getBounds(RenderParam& param) override
			{
				Scalar centerX = cx.getValue(param.containerWidth);
				Scalar centerY = cy.getValue(param.containerHeight);
				Scalar radiusX = rx.getValue(param.containerWidth);
				Scalar radiusY = ry.getValue(param.containerWidth);
				return Rectangle(centerX - radiusX, centerY - radiusY, centerX + radiusX, centerY + radiusY);
			}

		};

		class Circle : public Element
		{
		public:
			Length cx;
			Length cy;
			Length r;

		public:
			void load() override
			{
				Element::load();
				PARSE_ATTRIBUTE(cx, "cx")
				PARSE_ATTRIBUTE(cy, "cy")
				PARSE_ATTRIBUTE(r, "r")
				loadPen();
				loadBrush();
			}

			void render(Canvas* canvas, RenderParam& param) override
			{
				Scalar centerX = cx.getValue(param.containerWidth);
				Scalar centerY = cy.getValue(param.containerHeight);
				Scalar radius = r.getValue(param.containerWidth);
				Scalar diameter = radius + radius;
				canvas->drawEllipse(centerX - radius, centerY - radius, diameter, diameter, getPen(param), getBrush(param));
			}

			Rectangle getBounds(RenderParam& param) override
			{
				Scalar centerX = cx.getValue(param.containerWidth);
				Scalar centerY = cy.getValue(param.containerHeight);
				Scalar radius = r.getValue(param.containerWidth);
				return Rectangle(centerX - radius, centerY - radius, centerX + radius, centerY + radius);
			}

		};

		class Line : public Element
		{
		public:
			Length x1;
			Length y1;
			Length x2;
			Length y2;

		public:
			void load() override
			{
				Element::load();
				PARSE_ATTRIBUTE(x1, "x1")
				PARSE_ATTRIBUTE(y1, "y1")
				PARSE_ATTRIBUTE(x2, "x2")
				PARSE_ATTRIBUTE(y2, "y2")
				loadPen();
			}

			void render(Canvas* canvas, RenderParam& param) override
			{
				canvas->drawLine(x1.getValue(param.containerWidth), y1.getValue(param.containerHeight), x2.getValue(param.containerWidth), y2.getValue(param.containerHeight), getPen(param));
			}

			Rectangle getBounds(RenderParam& param) override
			{
				Scalar _x1 = x1.getValue(param.containerWidth);
				Scalar _y1 = y1.getValue(param.containerHeight);
				Scalar _x2 = x2.getValue(param.containerWidth);
				Scalar _y2 = y2.getValue(param.containerHeight);
				return Rectangle(Math::min(_x1, _x2), Math::min(_y1, _y2), Math::max(_x1, _x2), Math::max(_y1, _y2));
			}

		};

		class Polyline : public Element
		{
		public:
			List<Point> points;

		public:
			void load() override
			{
				Element::load();
				List<Scalar> pts;
				PARSE_ATTRIBUTE(pts, "points")
				ListElements<Scalar> items(pts);
				if (items.count >= 4) {
					for (sl_size i = 0; i < items.count; i += 2) {
						points.add_NoLock(items[i], items[i + 1]);
					}
				}
				loadPen();
				loadBrush();
			}

			void render(Canvas* canvas, RenderParam& param) override
			{
				Ref<Brush>& brush = getBrush(param);
				if (brush.isNotNull()) {
					canvas->fillPolygon(points, brush, getFillRule());
				}
				Ref<Pen>& pen = getPen(param);
				if (pen.isNotNull()) {
					if (points.isNotNull()) {
						canvas->drawLines(points, pen);
					}
				}
			}

			Rectangle getBounds(RenderParam& param) override
			{
				Rectangle ret;
				ret.setFromPoints(points);
				return ret;
			}

		};

		class Polygon : public Element
		{
		public:
			List<Point> points;

		public:
			void load() override
			{
				Element::load();
				List<Scalar> pts;
				PARSE_ATTRIBUTE(pts, "points")
				ListElements<Scalar> items(pts);
				if (items.count >= 4) {
					for (sl_size i = 0; i < items.count; i += 2) {
						points.add_NoLock(items[i], items[i + 1]);
					}
				}
				loadPen();
				loadBrush();
			}

			void render(Canvas* canvas, RenderParam& param) override
			{
				if (points.isNotNull()) {
					canvas->drawPolygon(points, getPen(param), getBrush(param), getFillRule());
				}
			}

			Rectangle getBounds(RenderParam& param) override
			{
				Rectangle ret;
				ret.setFromPoints(points);
				return ret;
			}

		};

		class Path : public Element
		{
		public:
			Ref<GraphicsPath> shape;

		public:
			void load() override
			{
				Element::load();
				PARSE_ATTRIBUTE(shape, "d")
				loadPen();
				loadBrush();
			}

			void render(Canvas* canvas, RenderParam& param) override
			{
				if (shape.isNotNull()) {
					shape->setFillMode(getFillRule());
					canvas->drawPath(shape, getPen(param), getBrush(param));
				}
			}

			Rectangle getBounds(RenderParam& param) override
			{
				if (shape.isNotNull()) {
					return shape->getBounds();
				} else {
					return Rectangle::zero();
				}
			}

		};

		class Viewport : public Group
		{
		public:
			Define<ViewBox> viewBox;
			Define<Length> width;
			Define<Length> height;
			Length x;
			Length y;
			PreserveAspectRatio preserveAspectRatio;

		public:
			void load() override
			{
				Group::load();
				PARSE_ATTRIBUTE(viewBox, "viewBox")
				PARSE_ATTRIBUTE(width, "width")
				PARSE_ATTRIBUTE(height, "height")
				PARSE_ATTRIBUTE(x, "x")
				PARSE_ATTRIBUTE(y, "y")
				PARSE_ATTRIBUTE(preserveAspectRatio, "preserveAspectRatio")

				if (width.flagDefined) {
					if (!(height.flagDefined)) {
						*height = *width;
					}
				} else {
					if (height.flagDefined) {
						*width = *height;
					} else {
						if (viewBox.flagDefined) {
							width.value = viewBox.width;
							height.value = viewBox.height;
						}
					}
				}
				if (!(viewBox.flagDefined)) {
					if (width.flagDefined || height.flagDefined) {
						viewBox.width = width.value;
						viewBox.height = height.value;
					}
				}
			}

			void render(Canvas* canvas, RenderParam& _param) override
			{
				if (Math::isAlmostZero(viewBox.width) || Math::isAlmostZero(viewBox.height)) {
					return;
				}
				RenderParam param = _param;
				Size size = getSize(param.containerWidth, param.containerHeight);
				Rectangle rectDst;
				rectDst.left = x.getValue(param.containerWidth);
				rectDst.top = y.getValue(param.containerHeight);
				rectDst.setSize(size);
				Rectangle rectTarget = rectDst;
				if (preserveAspectRatio.align != Alignment::Default) {
					Rectangle r;
					if (GraphicsUtil::calculateAlignRectangle(r, rectDst, viewBox.width, viewBox.height, preserveAspectRatio.flagContain ? ScaleMode::Contain : ScaleMode::Cover, preserveAspectRatio.align)) {
						rectTarget = r;
					}
				}
				param.containerWidth = viewBox.width;
				param.containerHeight = viewBox.height;
				Rectangle rectViewBox(viewBox.x, viewBox.y, viewBox.x + viewBox.width, viewBox.y + viewBox.height);
				if (rectViewBox.isAlmostEqual(rectTarget)) {
					Group::render(canvas, param);
				} else {
					CanvasStateScope scope(canvas);
					canvas->clipToRectangle(rectDst);
					canvas->concatMatrix(Transform2f::getTransformMatrixFromRectToRect(rectViewBox, rectTarget));
					Group::render(canvas, param);
				}
			}

			void render(Canvas* canvas, const Rectangle& rectDraw)
			{
				if (children.isEmpty()) {
					return;
				}
				if (Math::isAlmostZero(viewBox.width) || Math::isAlmostZero(viewBox.height)) {
					return;
				}
				RenderParam param;
				param.containerWidth = viewBox.width;
				param.containerHeight = viewBox.height;
				Rectangle rectViewBox(viewBox.x, viewBox.y, viewBox.x + viewBox.width, viewBox.y + viewBox.height);
				if (rectViewBox.isAlmostEqual(rectDraw)) {
					Group::render(canvas, param);
				} else {
					CanvasStateScope scope(canvas);
					canvas->concatMatrix(Transform2f::getTransformMatrixFromRectToRect(rectViewBox, rectDraw));
					Group::render(canvas, param);
				}
			}

			Rectangle getBounds(RenderParam& param) override
			{
				if (viewBox.flagDefined) {
					if (Math::isAlmostZero(viewBox.width) || Math::isAlmostZero(viewBox.height)) {
						return Rectangle::zero();
					}
					Size size = getSize(param.containerWidth, param.containerHeight);
					Rectangle ret;
					ret.left = x.getValue(param.containerWidth);
					ret.top = y.getValue(param.containerHeight);
					ret.setSize(size);
					return ret;
				} else {
					Scalar _x = x.getValue(param.containerWidth);
					Scalar _y = y.getValue(param.containerHeight);
					Scalar w = width.getValue(param.containerWidth);
					Scalar h = height.getValue(param.containerHeight);
					return Rectangle(_x, _y, _x + w, _y + h);
				}
			}

			Size getSize(Scalar containerWidth, Scalar containerHeight)
			{
				return Size(width.getValue(containerWidth), height.getValue(containerHeight));
			}

		};

		class Use : public Group
		{
		public:
			Length x;
			Length y;

		public:
			void load() override;

			void render(Canvas* canvas, RenderParam& param) override
			{
				if (children.isEmpty()) {
					return;
				}
				CanvasStateScope scope(canvas);
				canvas->translate(x.getValue(param.containerWidth), y.getValue(param.containerHeight));
				Group::render(canvas, param);
			}

		};

		Loaders::Loaders()
		{
#define ADD_LOADER(NAME, CLASS) \
			{ \
				SLIB_STATIC_STRING(name, #NAME) \
				add_NoLock(name, []() { \
					return new CLASS; \
				}); \
			}

			ADD_LOADER(svg, Viewport)
			ADD_LOADER(g, Group)
			ADD_LOADER(rect, Rect)
			ADD_LOADER(circle, Circle)
			ADD_LOADER(ellipse, Ellipse)
			ADD_LOADER(line, Line)
			ADD_LOADER(polyline, Polyline)
			ADD_LOADER(polygon, Polygon)
			ADD_LOADER(path, Path)
			ADD_LOADER(use, Use)
		}

		class Document : public Viewport
		{
		public:
			Ref<XmlDocument> xmlDocument;
			CascadingStyleSheet styleSheet;
			sl_uint32 currentLinkDeepLevel = 0;
			CHashMap< String, Ref<XmlElement> > xmlElementsById;

		public:
			sl_bool load(const void* mem, sl_size size)
			{
				XmlParseParam param;
				param.flagLogError = sl_false;
				param.onEndElement = [this](XmlParseControl*, XmlElement* xml) {
					String name = xml->getName();
					if (name == StringView::literal("style")) {
						loadStyle(xml->getText());
					}
					SLIB_STATIC_STRING(_id, "id")
					String id = xml->getAttribute(_id);
					if (id.isNotEmpty()) {
						xmlElementsById.put_NoLock(id, xml);
					}
				};
				xmlDocument = Xml::parse(MemoryView(mem, size), param);
				if (xmlDocument.isNull()) {
					return sl_false;
				}
				xml = xmlDocument->getRoot().get();
				if (!xml) {
					return sl_false;
				}
				document = this;
				Viewport::load();
				return sl_true;
			}

			void loadStyle(const StringView& content)
			{
				styleSheet.addStyles(content);
			}

		};

		void UrlPaint::load(Document* document, Scalar opacity)
		{
			Ref<XmlElement> e = document->getXmlByUrl(url);
			if (e.isNotNull()) {
				String type = e->getName();
				if (type == StringView::literal("linearGradient")) {
					Styler gradient;
					gradient.xml = e;
					gradient.document = document;
					loadLinearGradient(gradient, opacity);
				} else if (type == StringView::literal("radialGradient")) {
					Styler gradient;
					gradient.xml = e;
					gradient.document = document;
					loadRadialGradient(gradient, opacity);
				}
			}
		}

		String Styler::getAttribute(const String& name)
		{
			if (!(styles.flagDefined)) {
				SLIB_STATIC_STRING(_style, "style")
				*styles = document->styleSheet.getElementDeclarations(xml, xml->getAttribute(_style));
				styles.flagDefined = sl_true;
			}
			String ret = CascadingStyleSheet::getDeclarationValue(styles, name);
			if (ret.isNotNull()) {
				return ret;
			}
			return xml->getAttribute(name);
		}

		Ref<XmlElement> Element::getXmlByUrl(const StringView& url)
		{
			if (url.isEmpty()) {
				return sl_null;
			}
			if (!(url.startsWith('#'))) {
				return sl_null;
			}
			StringView id = url.substring(1).trim();
			if (id.isEmpty()) {
				return sl_null;
			}
			return document->xmlElementsById.getValue_NoLock(id);
		}

		void Use::load()
		{
			if (document->currentLinkDeepLevel > MAX_LINK_DEEP_LEVEL) {
				return;
			}

			GET_ATTRIBUTE(String, href, "href")
			if (href.isEmpty()) {
				GET_ATTRIBUTE(, href, "xlink:href")
			}
			Ref<XmlElement> child = getXmlByUrl(href);
			if (child.isNull()) {
				return;
			}

			PARSE_ATTRIBUTE(x, "x")
			PARSE_ATTRIBUTE(y, "y")

			document->currentLinkDeepLevel++;
			Element* element = loadChild(child.get());
			document->currentLinkDeepLevel--;

			if (element && child->getName() == StringView::literal("svg")) {
				Define<Length> width;
				PARSE_ATTRIBUTE(width, "width")
				if (width.flagDefined) {
					((Viewport*)element)->width = width;
				}
				Define<Length> height;
				PARSE_ATTRIBUTE(height, "height")
				if (height.flagDefined) {
					((Viewport*)element)->height = height;
				}
			}
		}

	}

	SLIB_DEFINE_OBJECT(Svg, Drawable)

	Svg::Svg(): m_flagQuerySize(sl_false)
	{
	}

	Svg::~Svg()
	{
	}

	Ref<Svg> Svg::loadFromMemory(const void* mem, sl_size size)
	{
		Ref<Document> document = new Document;
		if (document.isNotNull()) {
			if (document->load(mem, size)) {
				Ref<Svg> svg = new Svg;
				if (svg.isNotNull()) {
					svg->m_document = Move(document);
					return svg;
				}
			}
		}
		return sl_null;
	}

	Ref<Svg> Svg::loadFromMemory(const MemoryView& mem)
	{
		return loadFromMemory(mem.data, mem.size);
	}

	Ref<Svg> Svg::loadFromFile(const StringParam& filePath)
	{
		Memory mem = File::readAllBytes(filePath);
		if (mem.isNotNull()) {
			return loadFromMemory(mem);
		}
		return sl_null;
	}

	Ref<Svg> Svg::loadFromAsset(const StringParam& path)
	{
		Memory mem = Assets::readAllBytes(path);
		if (mem.isNotNull()) {
			return loadFromMemory(mem);
		}
		return sl_null;
	}

	Size Svg::getSize(Scalar containerWidth, Scalar containerHeight)
	{
		return ((Document*)(m_document.get()))->getSize(containerWidth, containerHeight);
	}

	void Svg::render(Canvas* canvas, const Rectangle& rectDraw)
	{
		ObjectLocker locker(this);
		((Document*)(m_document.get()))->render(canvas, rectDraw);
	}

	Scalar Svg::getGlobalFontSize()
	{
		return g_fontSize;
	}

	void Svg::setGlobalFontSize(Scalar size)
	{
		g_fontSize = size;
	}

	void Svg::_querySize()
	{
		if (m_flagQuerySize) {
			return;
		}
		m_size = getSize();
		m_flagQuerySize = sl_true;
	}

	sl_real Svg::getDrawableWidth()
	{
		_querySize();
		return m_size.x;
	}

	sl_real Svg::getDrawableHeight()
	{
		_querySize();
		return m_size.y;
	}

	void Svg::onDrawAll(Canvas* canvas, const Rectangle& rectDst, const DrawParam& param)
	{
		render(canvas, rectDst);
	}

}
