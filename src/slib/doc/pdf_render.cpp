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

#include "slib/graphics/canvas.h"
#include "slib/graphics/path.h"
#include "slib/graphics/cmyk.h"
#include "slib/core/queue.h"
#include "slib/math/transform2d.h"

namespace slib
{

	namespace priv
	{
		namespace pdf
		{

			class TextState
			{
			public:
				float charSpace = 0;
				float wordSpace = 0;
				float widthScale = 1;
				float leading = 0;
				float rise = 0;
				Ref<Font> font;
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
				BrushState brush;
				PenState pen;
			};

			enum class PdfColorSpace
			{
				Unknown,
				RGB,
				Gray,
				CMYK
			};

			class Renderer : public RenderState
			{
			public:
				Canvas* canvas;
				PdfPage* page;

				Ref<GraphicsPath> path;
				TextState text;

				Stack<RenderState> states;
				PdfColorSpace colorSpaceForStroking = PdfColorSpace::Unknown;
				PdfColorSpace colorSpaceForNonStroking = PdfColorSpace::Unknown;

			public:
				sl_bool preparePath()
				{
					if (path.isNull()) {
						path = GraphicsPath::create();
						return path.isNotNull();
					}
					return sl_false;
				}

				void moveTo(ListElements<PdfObject> operands)
				{
					if (operands.count != 2) {
						return;
					}
					if (!(preparePath())) {
						return;
					}
					path->moveTo(operands[0].getFloat(), operands[1].getFloat());
				}

				void lineTo(ListElements<PdfObject> operands)
				{
					if (operands.count != 2) {
						return;
					}
					if (!(preparePath())) {
						return;
					}
					path->lineTo(operands[0].getFloat(), operands[1].getFloat());
				}

				void curveTo(ListElements<PdfObject> operands)
				{
					if (operands.count != 6) {
						return;
					}
					if (!(preparePath())) {
						return;
					}
					path->cubicTo(operands[0].getFloat(), operands[1].getFloat(),
						operands[2].getFloat(), operands[3].getFloat(),
						operands[4].getFloat(), operands[5].getFloat());
				}

				void appendRect(ListElements<PdfObject> operands)
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

				void setLineDashPattern(ListElements<PdfObject> operands)
				{
					if (operands.count != 2) {
						return;
					}
					SET_HANDLE_STATE(pen, style, PenStyle::Dash);
				}

				void setColor(const Color& color, sl_bool flagStroking)
				{
					if (flagStroking) {
						SET_HANDLE_STATE(pen, color, color);
					} else {
						SET_HANDLE_STATE(brush, color, color);
					}
				}

				void setColorSpace(ListElements<PdfObject> operands, sl_bool flagStroking)
				{
					if (operands.count != 1) {
						return;
					}
					PdfColorSpace cs;
					const String& name = operands[0].getName();
					if (name == StringView::literal("DeviceRGB")) {
						cs = PdfColorSpace::RGB;
					} else if (name == StringView::literal("DeviceGray")) {
						cs = PdfColorSpace::Gray;
					} else if (name == StringView::literal("DeviceCMYK")) {
						cs = PdfColorSpace::CMYK;
					} else {
						return;
					}
					if (flagStroking) {
						colorSpaceForStroking = cs;
					} else {
						colorSpaceForNonStroking = cs;
					}
				}

				void setColor(ListElements<PdfObject> operands, sl_bool flagStroking)
				{
					PdfColorSpace cs = flagStroking ? colorSpaceForStroking : colorSpaceForNonStroking;
					switch (cs) {
						case PdfColorSpace::RGB:
							setRGB(operands, flagStroking);
							break;
						case PdfColorSpace::Gray:
							setGrayLevel(operands, flagStroking);
							break;
						case PdfColorSpace::CMYK:
							setCMYK(operands, flagStroking);
							break;
					}
				}

				void setRGB(ListElements<PdfObject> operands, sl_bool flagStroking)
				{
					if (operands.count != 3) {
						return;
					}
					sl_uint8 r = (sl_uint8)(operands[0].getFloat() * 255);
					sl_uint8 g = (sl_uint8)(operands[1].getFloat() * 255);
					sl_uint8 b = (sl_uint8)(operands[2].getFloat() * 255);
					setColor(Color(r, g, b), flagStroking);
				}

				void setGrayLevel(ListElements<PdfObject> operands, sl_bool flagStroking)
				{
					if (operands.count != 1) {
						return;
					}
					sl_uint8 g = (sl_uint8)(operands[0].getFloat() * 255);
					setColor(Color(g, g, g), flagStroking);
				}

				void setCMYK(ListElements<PdfObject> operands, sl_bool flagStroking)
				{
					if (operands.count != 4) {
						return;
					}
					sl_uint8 c = (sl_uint8)(operands[0].getFloat() * 255);
					sl_uint8 m = (sl_uint8)(operands[1].getFloat() * 255);
					sl_uint8 y = (sl_uint8)(operands[2].getFloat() * 255);
					sl_uint8 k = (sl_uint8)(operands[3].getFloat() * 255);
					sl_uint8 r, g, b;
					CMYK::convertCMYKToRGB(c, m, y, k, r, g, b);
					setColor(Color(r, g, b), flagStroking);
				}

				void setLineJoin(ListElements<PdfObject> operands)
				{
					if (operands.count != 1) {
						return;
					}
					SET_HANDLE_STATE(pen, join, (LineJoin)(operands[0].getUint()));
				}

				void setLineCap(ListElements<PdfObject> operands)
				{
					if (operands.count != 1) {
						return;
					}
					SET_HANDLE_STATE(pen, cap, (LineCap)(operands[0].getUint()));
				}

				void setMiterLimit(ListElements<PdfObject> operands)
				{
					if (operands.count != 1) {
						return;
					}
					SET_HANDLE_STATE(pen, miterLimit, operands[0].getFloat());
				}

				void concatMatrix(ListElements<PdfObject> operands)
				{
					if (operands.count != 6) {
						return;
					}
					Matrix3 mat(operands[0].getFloat(), operands[1].getFloat(), 0,
						operands[2].getFloat(), operands[3].getFloat(), 0,
						operands[4].getFloat(), operands[5].getFloat(), 1);
					canvas->concatMatrix(mat);
				}

				void fill(sl_bool flagEvenOddRule)
				{
					if (path.isNotNull()) {
						if (flagEvenOddRule) {
							path->setFillMode(FillMode::Alternate);
						} else {
							path->setFillMode(FillMode::Winding);
						}
						canvas->fillPath(path, brush.getHandle());
					}
				}

				void stroke()
				{
					if (path.isNotNull()) {
						canvas->drawPath(path, pen.getHandle());
					}
				}

				void beginText()
				{
					text = TextState();
				}

				void setTextCharSpace(ListElements<PdfObject> operands)
				{
					if (operands.count != 1) {
						return;
					}
					text.charSpace = operands[0].getFloat();
				}

				void setTextWordSpace(ListElements<PdfObject> operands)
				{
					if (operands.count != 1) {
						return;
					}
					text.wordSpace = operands[0].getFloat();
				}

				void setTextWidthScale(ListElements<PdfObject> operands)
				{
					if (operands.count != 1) {
						return;
					}
					text.widthScale = operands[0].getFloat() / 100.0f;
				}

				void setTextLeading(ListElements<PdfObject> operands)
				{
					if (operands.count != 1) {
						return;
					}
					text.leading = operands[0].getFloat();
				}

				void setTextRise(ListElements<PdfObject> operands)
				{
					if (operands.count != 1) {
						return;
					}
					text.rise = operands[0].getFloat();
				}

				void setTextMatrix(ListElements<PdfObject> operands)
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
					Transform2::preTranslate(text.lineMatrix, Vector2(tx, ty));
					text.matrix = text.lineMatrix;
				}

				void moveTextMatrix(ListElements<PdfObject> operands, sl_bool flagSetLeading)
				{
					if (operands.count != 2) {
						return;
					}
					float ty = operands[1].getFloat();
					moveTextMatrix(operands[0].getFloat(), ty);
					if (flagSetLeading) {
						text.leading = -ty;
					}
				}

				Ref<Font> getFont(const String& name, float size)
				{
					PdfDictionary dict = page->getFontResourceAsDictionary(name);
					if (dict.isNotNull()) {
						SLIB_STATIC_STRING(idSubtype, "Subtype")
						SLIB_STATIC_STRING(idBaseFont, "BaseFont")
						const String& subType = dict.getValue_NoLock(idSubtype).getName();
						const String& baseFont = dict.getValue_NoLock(idBaseFont).getName();
						if (subType == StringView::literal("Type1")) {
							return Font::create(name, size);
						} else if (subType == StringView::literal("TrueType")) {
							return Font::create(name, size);
						}
					}
					return Font::create(Font::getDefaultFontFamily(), size);
				}

				void setTextFont(ListElements<PdfObject> operands)
				{
					if (operands.count != 2) {
						return;
					}
					text.font = getFont(operands[0].getName(), operands[1].getFloat());
				}

				void saveGraphicsState()
				{
					canvas->save();
					states.push(*this);
				}

				void restoreGraphicsState()
				{
					canvas->restore();
					states.pop(this);
				}

				void render(PdfOperation& operation)
				{
					switch (operation.op) {
						case PdfOperator::b:
							closePath();
							fill(sl_false);
							stroke();
							break;
						case PdfOperator::B:
							fill(sl_false);
							stroke();
							break;
						case PdfOperator::b_:
							closePath();
							fill(sl_true);
							stroke();
							break;
						case PdfOperator::B_:
							fill(sl_true);
							stroke();
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
							curveTo(operation.operands);
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
							// invoke named XObject
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
							// set parameters from graphics state parameter dictionary
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
							// set color (for stroking, ICCBased and special color spaces)
							break;
						case PdfOperator::scn:
							// set color (for non-stroking, ICCBased and special color spaces)
							break;
						case PdfOperator::sh:
							// paint area defined by shading pattern
							break;
						case PdfOperator::T_:
							moveTextMatrix(0, text.leading);
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
							// show text
							break;
						case PdfOperator::TJ:
							// show text, allowing individual glphy positioning
							break;
						case PdfOperator::TL:
							setTextLeading(operation.operands);
							break;
						case PdfOperator::Tm:
							setTextMatrix(operation.operands);
							break;
						case PdfOperator::Tr:
							// set text rendering mode
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
							// curve to (initial point replicated)
							break;
						case PdfOperator::w:
							// set line width
							break;
						case PdfOperator::W:
							// set clipping path (nonzero winding number rule)
							break;
						case PdfOperator::W_:
							// W*: set clipping path (even-odd rule)
							break;
						case PdfOperator::y:
							// curve to (final point replicated)
							break;
						case PdfOperator::apos:
							// ': move to next line and show text
							break;
						case PdfOperator::quot:
							// ": set word and character spacing, move to next line, and show text
							break;
						default:
							break;
					}
				}

			};

		}
	}

	using namespace priv::pdf;

	void PdfPage::render(Canvas* canvas, const Rectangle& rcDst)
	{
		ListElements<PdfOperation> ops(getContent());
		if (!(ops.count)) {
			return;
		}
		Renderer renderer;
		renderer.canvas = canvas;
		renderer.page = this;
		for (sl_size i = 0; i < ops.count; i++) {
			renderer.render(ops[i]);
		}
	}

}
