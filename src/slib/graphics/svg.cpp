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

		static sl_bool ParseScalar(sl_char8*& s, sl_char8* end, Scalar& _out)
		{
			sl_reg result = String::parseFloat(&_out, s, 0, end - s);
			if (result == SLIB_PARSE_ERROR) {
				return sl_false;
			}
			s += result;
			return sl_true;
		}

		static sl_bool ParseColor(sl_char8*& s, sl_char8* end, Color& _out)
		{
			sl_reg result = Color::parse(&_out, s, 0, end - s);
			if (result == SLIB_PARSE_ERROR) {
				return sl_false;
			}
			s += result;
			return sl_true;
		}

		template <class Base, sl_bool flagBaseClass = __is_class(Base)>
		class Define
		{
		public:
			Base value;
			sl_bool flagDefined = sl_false;

		public:
			SLIB_INLINE Base& operator*()
			{
				return value;
			}

		};

		template <class Base>
		class Define<Base, sl_true> : public Base
		{
		public:
			sl_bool flagDefined = sl_false;

		public:
			SLIB_INLINE Base& operator*()
			{
				return *this;
			}

		};

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
		SLIB_INLINE static sl_bool ParseValue(const StringView& str, T& _out)
		{
			sl_bool flagDefined;
			return ParseValue(str, _out, flagDefined);
		}

		template <class T>
		static sl_bool ParseValue(sl_char8*& s, sl_char8* end, List<T>& _out)
		{
			for (;;) {
				T value;
				if (!(ParseValue(s, end, value))) {
					return sl_false;
				}
				if (!(_out.add_NoLock(Move(value)))) {
					return sl_false;
				}
				if (s >= end) {
					break;
				}
				sl_char8 c = *s;
				if (c != ',' && !SLIB_CHAR_IS_WHITE_SPACE(c)) {
					break;
				}
				SkipWhitespaces(s, end);
				if (s >= end) {
					break;
				}
				if (*s == ',') {
					s++;
					SkipWhitespaces(s, end);
					if (s >= end) {
						return sl_false;
					}
				}
			}
			return sl_true;
		}

		SLIB_INLINE static void ParseValue(String&& str, String& _out)
		{
			_out = Move(str);
		}

		SLIB_INLINE static sl_bool ParseValue(sl_char8*& s, sl_char8* end, Scalar& _out)
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

		enum class PaintType
		{
			Solid = 0
		};

		class Paint : public Referable
		{
		public:
			PaintType type;
			
		};

		class SolidPaint : public Paint
		{
		public:
			Color color;

		public:
			SolidPaint()
			{
				type = PaintType::Solid;
			}

			SolidPaint(const Color& _color)
			{
				type = PaintType::Solid;
				color = _color;
			}

		public:
			static const Ref<SolidPaint>& black()
			{
				SLIB_SAFE_LOCAL_STATIC(Ref<SolidPaint>, ret, new SolidPaint(Color::Black))
				return ret;
			}

		};

		static sl_bool ParseValue(sl_char8*& s, sl_char8* end, Ref<Paint>& _out)
		{
			Color color;
			if (ParseColor(s, end, color)) {
				if (color.isZero()) {
					_out.setNull();
					return sl_true;
				}
				SolidPaint* ret = new SolidPaint;
				if (ret) {
					ret->color = color;
					_out = ret;
					return sl_true;
				}
			}
			return sl_false;
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

		static void SkipPathValueSeparator(sl_char8*& start, sl_char8* end)
		{
			while (start < end) {
				sl_char8 c = *start;
				if (!SLIB_CHAR_IS_WHITE_SPACE(c) && c != ',') {
					break;
				}
				start++;
			}
		}

		static sl_bool ParsePathValue(sl_char8*& s, sl_char8* end, sl_svg_scalar& _out)
		{
			SkipPathValueSeparator(s, end);
			return ParseScalar(s, end, _out);
		}

		static sl_bool ParsePathValues(sl_char8*& s, sl_char8* end, sl_svg_scalar* _out, sl_size count)
		{
			for (sl_size i = 0; i < count; i++) {
				if (!(ParsePathValue(s, end, _out[i]))) {
					return sl_false;
				}
			}
			return sl_true;
		}

		static sl_bool ParseValue(sl_char8*& s, sl_char8* end, Ref<GraphicsPath>& _out)
		{
			Ref<GraphicsPath> path = GraphicsPath::create();
			if (path.isNull()) {
				return sl_false;
			}
			sl_svg_scalar lastX = 0;
			sl_svg_scalar lastY = 0;
			sl_svg_scalar lastControlX = 0;
			sl_svg_scalar lastControlY = 0;
			for (;;) {
				if (s >= end) {
					break;
				}
				sl_char8 cmd = *(s++);
				sl_svg_scalar v[8];
				sl_bool flagParseResult = sl_false;
				switch (cmd) {
					case 'M':
						flagParseResult = ParsePathValues(s, end, v, 2);
						if (flagParseResult) {
							lastX = v[0];
							lastY = v[1];
							path->moveTo(lastX, lastY);
							while (ParsePathValues(s, end, v, 2)) {
								lastX = v[0];
								lastY = v[1];
								path->lineTo(lastX, lastY);
							}
							lastControlX = lastX;
							lastControlY = lastY;
						}
						break;
					case 'm':
						flagParseResult = ParsePathValues(s, end, v, 2);
						if (flagParseResult) {
							lastX += v[0];
							lastY += v[1];
							path->moveTo(lastX, lastY);
							while (ParsePathValues(s, end, v, 2)) {
								lastX += v[0];
								lastY += v[1];
								path->lineTo(lastX, lastY);
							}
							lastControlX = lastX;
							lastControlY = lastY;
						}
						break;
					case 'L':
						while (ParsePathValues(s, end, v, 2)) {
							flagParseResult = sl_true;
							lastX = v[0];
							lastY = v[1];
							path->lineTo(lastX, lastY);
						}
						lastControlX = lastX;
						lastControlY = lastY;
						break;
					case 'l':
						while (ParsePathValues(s, end, v, 2)) {
							flagParseResult = sl_true;
							lastX += v[0];
							lastY += v[1];
							path->lineTo(lastX, lastY);
						}
						lastControlX = lastX;
						lastControlY = lastY;
						break;
					case 'H':
						while (ParsePathValue(s, end, *v)) {
							flagParseResult = sl_true;
							lastX = *v;
							path->lineTo(lastX, lastY);
						}
						lastControlX = lastX;
						break;
					case 'h':
						while (ParsePathValue(s, end, *v)) {
							flagParseResult = sl_true;
							lastX += *v;
							path->lineTo(lastX, lastY);
						}
						lastControlX = lastX;
						break;
					case 'V':
						while (ParsePathValue(s, end, *v)) {
							flagParseResult = sl_true;
							lastY = *v;
							path->lineTo(lastX, lastY);
						}
						lastControlY = lastY;
						break;
					case 'v':
						while (ParsePathValue(s, end, *v)) {
							flagParseResult = sl_true;
							lastY += *v;
							path->lineTo(lastX, lastY);
						}
						lastControlY = lastY;
						break;
					case 'C':
						while (ParsePathValues(s, end, v, 6)) {
							flagParseResult = sl_true;
							lastControlX = v[2];
							lastControlY = v[3];
							lastX = v[4];
							lastY = v[5];
							path->cubicTo(v[0], v[1], lastControlX, lastControlY, lastX, lastY);
						}
						break;
					case 'c':
						while (ParsePathValues(s, end, v, 6)) {
							flagParseResult = sl_true;
							sl_svg_scalar cx = lastX + v[0];
							sl_svg_scalar cy = lastY + v[1];
							lastControlX = lastX + v[2];
							lastControlY = lastY + v[3];
							lastX += v[4];
							lastY += v[5];
							path->cubicTo(cx, cy, lastControlX, lastControlY, lastX, lastY);
						}
						break;
					case 'S':
						while (ParsePathValues(s, end, v, 4)) {
							flagParseResult = sl_true;
							sl_svg_scalar cx = lastX + (lastX - lastControlX);
							sl_svg_scalar cy = lastY + (lastY - lastControlY);
							lastControlX = v[0];
							lastControlY = v[1];
							lastX = v[2];
							lastY = v[3];
							path->cubicTo(cx, cy, lastControlX, lastControlY, lastX, lastY);
						}
						break;
					case 's':
						while (ParsePathValues(s, end, v, 4)) {
							flagParseResult = sl_true;
							sl_svg_scalar cx = lastX + (lastX - lastControlX);
							sl_svg_scalar cy = lastY + (lastY - lastControlY);
							lastControlX = lastX + v[0];
							lastControlY = lastY + v[1];
							lastX += v[2];
							lastY += v[3];
							path->cubicTo(cx, cy, lastControlX, lastControlY, lastX, lastY);
						}
						break;
					case 'Q':
						while (ParsePathValues(s, end, v, 4)) {
							flagParseResult = sl_true;
							lastControlX = v[0];
							lastControlY = v[1];
							lastX = v[2];
							lastY = v[3];
							path->conicTo(lastControlX, lastControlY, lastX, lastY);
						}
						break;
					case 'q':
						while (ParsePathValues(s, end, v, 4)) {
							flagParseResult = sl_true;
							lastControlX = lastX + v[0];
							lastControlY = lastY + v[1];
							lastX += v[2];
							lastY += v[3];
							path->conicTo(lastControlX, lastControlY, lastX, lastY);
						}
						break;
					case 'T':
						while (ParsePathValues(s, end, v, 2)) {
							flagParseResult = sl_true;
							lastControlX = lastX + (lastX - lastControlX);
							lastControlY = lastY + (lastY - lastControlY);
							lastX = v[0];
							lastY = v[1];
							path->conicTo(lastControlX, lastControlY, lastX, lastY);
						}
						break;
					case 't':
						while (ParsePathValues(s, end, v, 2)) {
							flagParseResult = sl_true;
							lastControlX = lastX + (lastX - lastControlX);
							lastControlY = lastY + (lastY - lastControlY);
							lastX += v[0];
							lastY += v[1];
							path->conicTo(lastControlX, lastControlY, lastX, lastY);
						}
						break;
					case 'A':
						while (ParsePathValues(s, end, v, 7)) {
							flagParseResult = sl_true;
							sl_svg_scalar sx = lastX;
							sl_svg_scalar sy = lastY;
							lastX = v[5];
							lastY = v[6];
							path->addArc(sx, sy, lastX, lastY, v[0], v[1], v[2], !(Math::isAlmostZero(v[3])), !(Math::isAlmostZero(v[4])));
						}
						break;
					case 'a':
						while (ParsePathValues(s, end, v, 7)) {
							flagParseResult = sl_true;
							sl_svg_scalar sx = lastX;
							sl_svg_scalar sy = lastY;
							lastX += v[5];
							lastY += v[6];
							path->addArc(sx, sy, lastX, lastY, v[0], v[1], v[2], !(Math::isAlmostZero(v[3])), !(Math::isAlmostZero(v[4])));
						}
						break;
					case 'Z':
					case 'z':
						path->closeSubpath();
				}
				if (!flagParseResult) {
					break;
				}
				SkipPathValueSeparator(s, end);
			}
			_out = Move(path);
			return sl_true;
		}

#define PARSE_ATTRIBUTE(NAME, ATTR) \
		SLIB_STATIC_STRING(name_##NAME, ATTR) \
		ParseValue(getAttribute(name_##NAME), NAME);

		struct RenderParam
		{
			Scalar containerWidth;
			Scalar containerHeight;
		};

		class Group;
		class Document;

		class Element : public Referable
		{
		public:
			Element* parent = sl_null;
			Document* document = sl_null;
			Ref<XmlElement> xml;

			Define< List<CascadingStyleDeclarations> > styles;

			Define< Ref<Paint> > stroke;
			Define<Length> strokeWidth;
			Define< List<Length> > strokeDashArray;
			Define<Length> strokeDashOffset;
			Define<LineCap> strokeLineCap;
			Define<LineJoin> strokeLineJoin;
			Define<Scalar> strokeMiterLimit;
			Define<Scalar> strokeOpacity;

			Define< Ref<Paint> > fill;
			Define<Scalar> fillOpacity;
			Define<FillMode> fillRule;

			Define< Ref<Pen> > pen;
			Ref<Brush> brush;

		public:
			virtual void load() = 0;

			virtual void render(Canvas* canvas, RenderParam& param) = 0;

			String getAttribute(const String& name);

#define DEFINE_ELEMENT_ATTRIBUTE(TYPE, NAME, GETTER, ATTR, DEFAULT) \
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

			DEFINE_ELEMENT_ATTRIBUTE(Ref<Paint>, stroke, getStroke, "stroke", sl_null)
			DEFINE_ELEMENT_ATTRIBUTE(Length, strokeWidth, getStrokeWidth, "stroke-width", (Scalar)1)
			DEFINE_ELEMENT_ATTRIBUTE(List<Length>, strokeDashArray, getStrokeDashArray, "stroke-dasharray", sl_null)
			DEFINE_ELEMENT_ATTRIBUTE(Length, strokeDashOffset, getStrokeDashOffset, "stroke-dashoffset", 0)
			DEFINE_ELEMENT_ATTRIBUTE(LineCap, strokeLineCap, getStrokeLineCap, "stroke-linecap", LineCap::Flat)
			DEFINE_ELEMENT_ATTRIBUTE(LineJoin, strokeLineJoin, getStrokeLineJoin, "stroke-linejoin", LineJoin::Miter)
			DEFINE_ELEMENT_ATTRIBUTE(Scalar, strokeMiterLimit, getStrokeMiterLimit, "stroke-miterlimit", (Scalar)4)
			DEFINE_ELEMENT_ATTRIBUTE(Scalar, strokeOpacity, getStrokeOpacity, "stroke-opacity", (Scalar)1)

			DEFINE_ELEMENT_ATTRIBUTE(Ref<Paint>, fill, getFill, "fill", SolidPaint::black())
			DEFINE_ELEMENT_ATTRIBUTE(Scalar, fillOpacity, getFillOpacity, "fill-opacity", (Scalar)1)
			DEFINE_ELEMENT_ATTRIBUTE(FillMode, fillRule, getFillRule, "fill-rule", FillMode::Winding)

			void loadPen()
			{
				getStroke();
				getStrokeWidth();
				getStrokeOpacity();
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
						if (paint->type == PaintType::Solid) {
							PenDesc desc;
							desc.color = ((SolidPaint*)(paint.get()))->color;
							Scalar opacity = getStrokeOpacity();
							if (opacity <= (Scalar)0.999) {
								desc.color.a = (sl_uint8)(Math::clamp0_255((sl_int32)((Scalar)(desc.color.a) * opacity)));
							}
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
				const Ref<Paint>& paint = getFill();
				if (paint.isNotNull()) {
					if (paint->type == PaintType::Solid) {
						Color color = ((SolidPaint*)(paint.get()))->color;
						Scalar opacity = getFillOpacity();
						if (opacity <= (Scalar)0.999) {
							color.a = (sl_uint8)(Math::clamp0_255((sl_int32)((Scalar)(color.a) * opacity)));
						}
						brush = Brush::createSolidBrush(color);
					}
				}
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
				sl_size n = xml->getChildCount();
				for (sl_size i = 0; i < n; i++) {
					Ref<XmlElement> child = xml->getChildElement(i);
					if (child.isNotNull()) {
						loadChild(Move(child));
					}
				}
			}

			Element* loadChild(Ref<XmlElement>&& xml)
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
						element->xml = Move(xml);
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
					item->render(canvas, param);
				}
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
					canvas->drawRoundRect(left, top, w, h, radiusX, radiusY, getPen(param), brush);
				} else {
					canvas->drawRectangle(left, top, w, h, getPen(param), brush);
				}
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
				PARSE_ATTRIBUTE(cx, "cx")
				PARSE_ATTRIBUTE(cy, "cy")
				PARSE_ATTRIBUTE(rx, "rx")
				PARSE_ATTRIBUTE(ry, "ry")
				if (rx.flagDefined) {
					if (!(ry.flagDefined)) {
						ry = rx;
					}
				} else {
					rx = ry;
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
				canvas->drawEllipse(centerX - radiusX, centerY - radiusY, radiusX + radiusX, radiusY + radiusY, getPen(param), brush);
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
				canvas->drawEllipse(centerX - radius, centerY - radius, diameter, diameter, getPen(param), brush);
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

		};

		class Polyline : public Element
		{
		public:
			List<Point> points;

		public:
			void load() override
			{
				List<Scalar> pts;
				PARSE_ATTRIBUTE(pts, "points")
				ListElements<Scalar> items(pts);
				if (items.count >= 4) {
					for (sl_size i = 0; i < items.count; i += 2) {
						points.add_NoLock(items[i], items[i + 1]);
					}
				}
				loadPen();
			}

			void render(Canvas* canvas, RenderParam& param) override
			{
				if (points.isNotNull()) {
					canvas->drawLines(points, getPen(param));
				}
			}

		};

		class Polygon : public Element
		{
		public:
			List<Point> points;

		public:
			void load() override
			{
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
					canvas->drawPolygon(points, getPen(param), brush);
				}
			}

		};

		class Path : public Element
		{
		public:
			Ref<GraphicsPath> shape;

		public:
			void load() override
			{
				PARSE_ATTRIBUTE(shape, "d")
				loadPen();
				loadBrush();
			}

			void render(Canvas* canvas, RenderParam& param) override
			{
				if (shape.isNotNull()) {
					shape->setFillMode(getFillRule());
					canvas->drawPath(shape, getPen(param), brush);
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
				PARSE_ATTRIBUTE(viewBox, "viewBox")
				PARSE_ATTRIBUTE(width, "width")
				PARSE_ATTRIBUTE(height, "height")
				PARSE_ATTRIBUTE(x, "x")
				PARSE_ATTRIBUTE(y, "y")
				PARSE_ATTRIBUTE(preserveAspectRatio, "preserveAspectRatio")
				Group::load();
			}

			void render(Canvas* canvas, RenderParam& _param) override
			{
				RenderParam param = _param;
				if (viewBox.flagDefined) {
					if (Math::isAlmostZero(viewBox.width) || Math::isAlmostZero(viewBox.height)) {
						return;
					}
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
					Matrix3f matrix = Transform2f::getTransformMatrixFromRectToRect(Rectangle(viewBox.x, viewBox.y, viewBox.x + viewBox.width, viewBox.y + viewBox.height), rectTarget);
					param.containerWidth = viewBox.width;
					param.containerHeight = viewBox.height;
					CanvasStateScope scope(canvas);
					canvas->clipToRectangle(rectDst);
					canvas->concatMatrix(matrix);
					Group::render(canvas, param);
				} else {
					Scalar _x = x.getValue(param.containerWidth);
					Scalar _y = y.getValue(param.containerHeight);
					if (width.flagDefined) {
						param.containerWidth = width.getValue(param.containerWidth);
					}
					if (height.flagDefined) {
						param.containerHeight = height.getValue(param.containerHeight);
					}
					if (Math::isAlmostZero(_x) && Math::isAlmostZero(_y)) {
						Group::render(canvas, param);
					} else {
						CanvasStateScope scope(canvas);
						canvas->translate(_x, _y);
						Group::render(canvas, param);
					}
				}
			}

			void render(Canvas* canvas, const Rectangle& rectDraw)
			{
				if (children.isEmpty()) {
					return;
				}
				RenderParam param;
				if (viewBox.flagDefined) {
					if (Math::isAlmostZero(viewBox.width) || Math::isAlmostZero(viewBox.height)) {
						return;
					}
					param.containerWidth = viewBox.width;
					param.containerHeight = viewBox.height;
					Matrix3f matrix = Transform2f::getTransformMatrixFromRectToRect(Rectangle(viewBox.x, viewBox.y, viewBox.x + viewBox.width, viewBox.y + viewBox.height), rectDraw);
					CanvasStateScope scope(canvas);
					canvas->concatMatrix(matrix);
					Group::render(canvas, param);
				} else {
					param.containerWidth = rectDraw.getWidth();
					param.containerHeight = rectDraw.getHeight();
					Group::render(canvas, param);
				}
			}

		protected:
			Size getSize(Scalar containerWidth, Scalar containerHeight)
			{
				Scalar w, h;
				if (width.flagDefined) {
					w = width.getValue(containerWidth);
					if (height.flagDefined) {
						h = height.getValue(containerHeight);
					} else {
						h = w * viewBox.height / viewBox.width;
					}
				} else {
					if (height.flagDefined) {
						h = height.getValue(containerHeight);
						w = h * viewBox.width / viewBox.height;
					} else {
						w = viewBox.width;
						h = viewBox.height;
					}
				}
				return Size(w, h);
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
			CascadingStyleSheet styleSheet;
			sl_uint32 currentLinkDeepLevel = 0;

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
				};
				Ref<XmlDocument> xmlDocument = Xml::parse(MemoryView(mem, size), param);
				if (xmlDocument.isNull()) {
					return sl_false;
				}
				xml = xmlDocument->getRoot();
				if (xml.isNull()) {
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

			Size getDocumentSize(Scalar containerWidth, Scalar containerHeight)
			{
				if (!(viewBox.flagDefined)) {
					return Size(containerWidth, containerHeight);
				}
				if (Math::isAlmostZero(viewBox.width) || Math::isAlmostZero(viewBox.height)) {
					return Size(containerWidth, containerHeight);
				}
				return getSize(containerWidth, containerHeight);
			}

		};

		String Element::getAttribute(const String& name)
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

		void Use::load()
		{
			if (document->currentLinkDeepLevel > MAX_LINK_DEEP_LEVEL) {
				return;
			}

			String href;
			PARSE_ATTRIBUTE(href, "href")
			href = href.trim();
			if (href.isEmpty()) {
				PARSE_ATTRIBUTE(href, "xlink:href")
				href = href.trim();
			}
			if (href.isEmpty()) {
				return;
			}
			if (!(href.startsWith('#'))) {
				return;
			}
			StringView id = StringView(href).substring(1).trim();
			if (id.isEmpty()) {
				return;
			}
			Ref<XmlDocument> doc = xml->getDocument();
			if (doc.isNull()) {
				return;
			}
			Ref<XmlElement> child = doc->findChildElementById(id);
			if (child.isNull()) {
				return;
			}
			XmlElement* _child = child.get();

			PARSE_ATTRIBUTE(x, "x")
			PARSE_ATTRIBUTE(y, "y")

			document->currentLinkDeepLevel++;
			Element* element = loadChild(Move(child));
			document->currentLinkDeepLevel--;

			if (element && _child->getName() == StringView::literal("svg")) {
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

	Svg::Svg()
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

	Size Svg::getSize(sl_svg_scalar containerWidth, sl_svg_scalar containerHeight)
	{
		return ((Document*)(m_document.get()))->getDocumentSize(containerWidth, containerHeight);
	}

	void Svg::render(Canvas* canvas, const Rectangle& rectDraw)
	{
		((Document*)(m_document.get()))->render(canvas, rectDraw);
	}

	sl_svg_scalar Svg::getGlobalFontSize()
	{
		return g_fontSize;
	}

	void Svg::setGlobalFontSize(sl_svg_scalar size)
	{
		g_fontSize = size;
	}

	void Svg::onDrawAll(Canvas* canvas, const Rectangle& rectDst, const DrawParam& param)
	{
		render(canvas, rectDst);
	}

}
