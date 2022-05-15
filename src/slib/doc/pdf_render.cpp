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
#include "slib/graphics/image.h"
#include "slib/core/queue.h"
#include "slib/core/mio.h"
#include "slib/math/transform2d.h"

#define FONT_SCALE 72.0f

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
				BrushState brush;
				PenState pen;
			};

			class Renderer : public RenderState
			{
			public:
				Canvas* canvas;
				PdfPage* page;
				PdfRenderParam* param;

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
					return sl_true;
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

				void curveTo(ListElements<PdfObject> operands, sl_bool flagReplicateInitialPoint, sl_bool flagReplicateFinalPoint)
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
					PdfColorSpace cs = Pdf::getColorSpace(operands[0].getName());
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

				// ICCBased and special color spaces
				void setSpecialColor(ListElements<PdfObject> operands, sl_bool flagStroking)
				{
					setColor(Color(100, 100, 100), flagStroking);
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

				void setLineWidth(ListElements<PdfObject> operands)
				{
					if (operands.count != 1) {
						return;
					}
					SET_HANDLE_STATE(pen, width, operands[0].getFloat());
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

				void setLineDashPattern(ListElements<PdfObject> operands)
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

				void setMiterLimit(ListElements<PdfObject> operands)
				{
					if (operands.count != 1) {
						return;
					}
					SET_HANDLE_STATE(pen, miterLimit, operands[0].getFloat());
				}

				void setGraphicsState(ListElements<PdfObject> operands)
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
						ListElements<PdfObject> values(states.getValue(fieldId).getArray());
						if (values.count == 2) {
							setFont(values[0].getName(), values[1].getFloat());
						}
					}
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

				void setClipping(sl_bool flagEvenOddRule)
				{
					if (path.isNotNull()) {
						if (flagEvenOddRule) {
							path->setFillMode(FillMode::Alternate);
						} else {
							path->setFillMode(FillMode::Winding);
						}
						canvas->clipToPath(path);
					}
				}

				void beginText()
				{
					text.matrix = Matrix3::identity();
					text.lineMatrix = Matrix3::identity();
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
					Transform2::preTranslate(text.lineMatrix, tx, ty);
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
						text.leading = ty;
					}
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

				void setTextFont(ListElements<PdfObject> operands)
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

					CanvasStateScope scope(canvas);
					Matrix3 mat = text.matrix;
					Transform2::preTranslate(mat, 0, text.rise);
					float scaleX = text.fontScale / FONT_SCALE;
					Transform2::preScale(mat, scaleX * text.widthScale, -text.fontScale / FONT_SCALE);
					canvas->concatMatrix(mat);

					sl_real x = 0;
					sl_char8* array = str.getData();
					sl_size len = str.getLength();
					sl_size nCh;
					if (font.cmap.isNotNull()) {
						nCh = 2;
					} else {
						nCh = 1;
					}
					for (sl_size i = 0; i + nCh <= len; i += nCh) {
						sl_int32 ch = nCh == 2 ? SLIB_MAKE_WORD(array[i], array[i+1]) : (sl_uint8)array[i];
						String32 s = font.getUnicode(ch);
						if (s.isNotEmpty()) {
							if (s.getLength() == 1 && *(s.getData()) == ' ') {
								x += text.wordSpace;
							} else {
								canvas->drawText(s, x / scaleX, - font.object->getFontHeight() / 2, font.object, pen.color);
								x += text.charSpace;
							}
							x += font.getCharWidth(ch) * text.fontScale;
						}
					}
					Transform2::preTranslate(text.matrix, x * text.widthScale, 0);
				}

				void adjustTextMatrix(float f)
				{
					Transform2::preTranslate(text.matrix, - f / 1000.0f * text.fontScale * text.widthScale, 0);
				}

				void showText(ListElements<PdfObject> operands)
				{
					if (operands.count != 1) {
						return;
					}
					const String& text = operands[0].getString();
					drawText(text);
				}

				void showTextWithPositions(ListElements<PdfObject> operands)
				{
					if (operands.count != 1) {
						return;
					}
					ListElements<PdfObject> args(operands[0].getArray());
					for (sl_size i = 0; i < args.count; i++) {
						PdfObject& obj = args[i];
						const String& s = obj.getString();
						if (s.isNotNull()) {
							drawText(s);
						} else {
							float f;
							if (obj.getFloat(f)) {
								adjustTextMatrix(f);
							}
						}
					}
				}

				void showTextWithSpacingParams(ListElements<PdfObject> operands)
				{
					if (operands.count != 3) {
						return;
					}
					text.wordSpace = operands[0].getFloat();
					text.charSpace = operands[1].getFloat();
					moveTextMatrix(0, text.leading);
					const String& text = operands[2].getString();
					drawText(text);
				}

				void drawExternalObject(ListElements<PdfObject> operands)
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
								canvas->draw(0, 0, 1, 1, image->object->flip(FlipMode::Vertical));
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
							setSpecialColor(operation.operands, sl_true);
							break;
						case PdfOperator::scn:
							setSpecialColor(operation.operands, sl_false);
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
							moveTextMatrix(0, text.leading);
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

			enum class TruetypeName
			{
				COPYRIGHT = 0,
				FONT_FAMILY = 1,
				FONT_SUBFAMILY = 2,
				UNIQUE_ID = 3,
				FULL_NAME = 4,
				VERSION_STRING = 5,
				PS_NAME = 6,
				TRADEMARK = 7,
				MANUFACTURER = 8,
				DESIGNER = 9,
				DESCRIPTION = 10,
				VENDOR_URL = 11,
				DESIGNER_URL = 12,
				LICENSE = 13,
				LICENSE_URL = 14,
				TYPOGRAPHIC_FAMILY = 16,
				TYPOGRAPHIC_SUBFAMILY = 17,
				MAC_FULL_NAME = 18,
				SAMPLE_TEXT = 19,
				CID_FINDFONT_NAME = 20,
				WWS_FAMILY = 21,
				WWS_SUBFAMILY = 22,
				LIGHT_BACKGROUND = 23,
				DARK_BACKGROUND = 24,
				VARIATIONS_PREFIX = 25
			};

			static List<String> GetTruetypeNames(const void* _content, sl_size size, TruetypeName name)
			{
				sl_uint8* content = (sl_uint8*)_content;
				struct TTF_HEADER
				{
					sl_uint8 version[2];
					sl_uint8 numTables[4];
					sl_uint8 searchRange[2];
					sl_uint8 entrySelector[2];
					sl_uint8 rangeShift[2];
				};
				struct TTF_OFFSET_TABLE
				{
					char name[4];
					sl_uint8 checksum[4];
					sl_uint8 offset[4];
					sl_uint8 length[4];
				};
				struct TTF_NAME_TABLE_HEADER
				{
					sl_uint8 format[2];
					sl_uint8 count[2];
					sl_uint8 stringOffset[2];
				};
				struct TTF_NAME_TABLE_ENTRY
				{
					sl_uint8 platformId[2];
					sl_uint8 encodingId[2];
					sl_uint8 languageId[2];
					sl_uint8 nameId[2];
					sl_uint8 length[2];
					sl_uint8 offset[2];
				};
				TTF_HEADER* header = (TTF_HEADER*)content;
				if (size < sizeof(TTF_HEADER)) {
					return sl_null;
				}
				sl_uint8* offsetTables = content + sizeof(TTF_HEADER);
				sl_uint32 numTables = MIO::readUint32BE(header->numTables);
				if (size < sizeof(TTF_HEADER) + sizeof(TTF_OFFSET_TABLE) * numTables) {
					return sl_null;
				}
				List<String> ret;
				for (sl_uint32 i = 0; i < numTables; i++) {
					TTF_OFFSET_TABLE* offsetTable = (TTF_OFFSET_TABLE*)(offsetTables + sizeof(TTF_OFFSET_TABLE) * i);
					if (Base::equalsMemory(offsetTable->name, "name", 4)) {
						sl_uint32 offset = MIO::readUint32BE(offsetTable->offset);
						if (offset + sizeof(TTF_NAME_TABLE_HEADER) <= size) {
							TTF_NAME_TABLE_HEADER* nameHeader = (TTF_NAME_TABLE_HEADER*)(content + offset);
							sl_uint8* entries = content + offset + sizeof(TTF_NAME_TABLE_HEADER);
							sl_uint16 n = MIO::readUint16BE(nameHeader->count);
							if (offset + sizeof(TTF_NAME_TABLE_HEADER) + sizeof(TTF_NAME_TABLE_ENTRY) * n <= size) {
								for (sl_uint16 i = 0; i < n; i++) {
									TTF_NAME_TABLE_ENTRY* entry = (TTF_NAME_TABLE_ENTRY*)(entries + sizeof(TTF_NAME_TABLE_ENTRY) * i);
									sl_uint16 nameId = MIO::readUint16BE(entry->nameId);
									if (nameId == (sl_uint16)name) {
										sl_uint16 platformId = MIO::readUint16BE(entry->platformId);
										sl_uint16 encodingId = MIO::readUint16BE(entry->encodingId);
										sl_bool flagUtf16 = sl_false;
										switch (platformId) {
										case 0: // APPLE_UNICODE
										case 2: // ISO
											flagUtf16 = sl_true;
											break;
										case 1: // MACINTOSH
											break;
										case 3: // MICROSOFT
											switch (encodingId) {
											case 0: //SYMBOL
											case 1: // UNICODE
											case 7: // UCS4
												flagUtf16 = sl_true;
												break;
											case 2: // SJIS
											case 3: // PRC
											case 4: // BIG5
											case 5: // WANSUNG
											case 6: // JOHAB
											default:
												break;
											}
											break;
										default:
											break;
										}
										sl_uint32 len = (sl_uint32)(MIO::readUint16BE(entry->length));
										sl_uint32 offsetString = offset + MIO::readUint16BE(nameHeader->stringOffset) + MIO::readUint16BE(entry->offset);
										if (offsetString + len <= size) {
											if (flagUtf16) {
												ret.add_NoLock(String::fromUtf16BE(content + offsetString, len));
											} else {
												ret.add_NoLock(String::fromUtf8(content + offsetString, len));
											}
										}
									}
								}
							}
						}
					}
				}
				return ret;
			}

			SLIB_INLINE static sl_uint8 GetColor4Bits(sl_uint8* row, sl_uint32 index)
			{
				sl_uint32 k = index >> 1;
				sl_uint8 c = (index & 1) ? (row[k] & 15) : (row[k] >> 4);
				return c;
			}

			SLIB_INLINE static sl_uint8 GetColor2Bits(sl_uint8* row, sl_uint32 index)
			{
				sl_uint32 k = index >> 2;
				sl_uint32 m = (3 - (index & 3)) << 1;
				sl_uint8 c = (row[k] >> m) & 3;
				return c;
			}

			SLIB_INLINE static sl_uint8 GetColor1Bits(sl_uint8* row, sl_uint32 index)
			{
				sl_uint32 k = index >> 3;
				sl_uint32 m = 7 - (index & 7);
				return ((row[k] >> m) & 1) ? 1 : 0;
			}

			static Ref<Image> CreateImageObject(sl_uint8* data, sl_uint32 size, sl_uint32 width, sl_uint32 colors, sl_uint32 bitsPerComponent, Color* indices, sl_uint32 nIndices)
			{
				if (width && colors && bitsPerComponent) {
					sl_uint32 sizeRow = (colors * bitsPerComponent * width + 7) >> 3;
					sl_uint32 height = size / sizeRow;
					if (height) {
						Memory mem = Memory::create(width * height * sizeof(Color));
						if (mem.isNotNull()) {
							ImageDesc desc;
							desc.colors = (Color*)(mem.getData());
							desc.width = width;
							desc.height = height;
							desc.stride = width;
							desc.ref = mem.getRef();
							Color* color = desc.colors;
							sl_uint8* row = data;
							sl_uint32 nPlane = sizeRow * height;
							for (sl_uint32 iRow = 0; iRow < height; iRow++) {
								sl_uint8* col = row;
								for (sl_uint32 iCol = 0; iCol < width; iCol++) {
									if (colors == 3) {
										// RGB
										switch (bitsPerComponent) {
											case 8:
												color->r = *(col++);
												color->g = *(col++);
												color->b = *(col++);
												break;
											case 16:
												color->r = *col;
												col += 2;
												color->g = *col;
												col += 2;
												color->b = *col;
												col += 2;
												break;
											case 4:
												color->r = GetColor4Bits(row, iCol * 3) * 17;
												color->g = GetColor4Bits(row, iCol * 3 + 1) * 17;
												color->b = GetColor4Bits(row, iCol * 3 + 2) * 17;
												break;
											case 2:
												color->r = GetColor2Bits(row, iCol * 3) * 85;
												color->g = GetColor2Bits(row, iCol * 3 + 1) * 85;
												color->b = GetColor2Bits(row, iCol * 3 + 2) * 85;
												break;
											case 1:
												color->r = GetColor1Bits(row, iCol * 3) ? 255 : 0;
												color->g = GetColor1Bits(row, iCol * 3 + 1) ? 255 : 0;
												color->b = GetColor1Bits(row, iCol * 3 + 2) ? 255 : 0;
												break;
											default:
												return sl_null;
										}
									} else if (colors == 4) {
										// CMYK
										sl_uint8 C, M, Y, K;
										if (bitsPerComponent == 8) {
											C = *(col++);
											M = *(col++);
											Y = *(col++);
											K = *(col++);
										} else if (bitsPerComponent == 16) {
											C = *col;
											col += 2;
											M = *col;
											col += 2;
											Y = *col;
											col += 2;
											K = *col;
											col += 2;
										} else {
											return sl_null;
										}
										CMYK::convertCMYKToRGB(C, M, Y, K, color->r, color->g, color->b);
									} else if (colors == 1) {
										sl_uint8 c;
										sl_uint32 c2;
										switch (bitsPerComponent) {
											case 8:
												c = *(col++);
												c2 = c;
												break;
											case 16:
												c = *(col++);
												c2 = (((sl_uint32)c) << 8) | *(col++);
												break;
											case 4:
												c2 = GetColor4Bits(row, iCol);
												c = c2 * 17;
												break;
											case 2:
												c2 = GetColor2Bits(row, iCol);
												c = c2 * 85;
												break;
											case 1:
												c2 = GetColor1Bits(row, iCol);
												c = c2 ? 255 : 0;
												break;
											default:
												return sl_null;
										}
										if (indices) {
											if (c2 < nIndices) {
												*color = indices[c2];
											} else {
												color->r = 0;
												color->g = 0;
												color->b = 0;
											}
										} else {
											color->r = c;
											color->g = c;
											color->b = c;
										}
									}
									color->a = 255;
									color++;
								}
								row += sizeRow;
							}
							return Image::create(desc);
						}
					}
				}
				return sl_null;
			}

		}
	}

	using namespace priv::pdf;

	PdfResourceContext::PdfResourceContext()
	{
	}

	PdfResourceContext::~PdfResourceContext()
	{
	}


	SLIB_DEFINE_ROOT_OBJECT(PdfFont)

	PdfFont::PdfFont()
	{
	}

	PdfFont::~PdfFont()
	{
	}

	Ref<PdfFont> PdfFont::load(PdfDocument* doc, const PdfReference& ref, PdfResourceContext& context)
	{
		const PdfDictionary& dict = doc->getObject(ref).getDictionary();
		if (dict.isNotNull()) {
			Ref<PdfFont> ret;
			if (context.fonts.get(ref.objectNumber, &ret)) {
				return ret;
			}
			ret = new PdfFont;
			if (ret.isNotNull()) {
				if (ret->_load(doc, dict, context)) {
					context.fonts.put(ref.objectNumber, ret);
					return ret;
				}
			}
		}
		return sl_null;
	}

	sl_bool PdfFont::_load(PdfDocument* doc, const PdfDictionary& dict, PdfResourceContext& context)
	{
		if (!(PdfFontResource::load(doc, dict))) {
			return sl_false;
		}
		List<String> familiesInFont;
		if (descriptor.content.objectNumber) {
			Memory content = doc->getObject(descriptor.content).getStreamContent();
			if (content.isNotNull()) {
				if (!(context.embeddedFonts.get(descriptor.content.objectNumber, &embeddedFont))) {
					embeddedFont = EmbeddedFont::load(content);
					context.embeddedFonts.put(descriptor.content.objectNumber, embeddedFont);
				}
				if (subtype == PdfFontSubtype::TrueType || (subtype == PdfFontSubtype::Type0 && cid.subtype == PdfFontSubtype::CIDFontType2)) {
					familiesInFont = GetTruetypeNames(content.getData(), content.getSize(), TruetypeName::FONT_FAMILY);
				}
			}
		}

		FontDesc fd;
		fd.familyName = descriptor.family;
		if (fd.familyName.isEmpty()) {
			if (familiesInFont.isNotNull()) {
				List<String> families(Font::getAllFamilyNames());
				ListElements<String> f(familiesInFont);
				for (sl_size i = 0; i < f.count; i++) {
					if (families.contains(f[i])) {
						fd.familyName = f[i];
						break;
					}
				}
			}
		}
		fd.size = descriptor.ascent * FONT_SCALE / 1000.0f;
		fd.flagBold = descriptor.weight >= 600.0f;
		fd.flagItalic = Math::abs(descriptor.italicAngle) > 10;
		object = Font::create(fd);
		return object.isNotNull();
	}

	float PdfFont::getCharWidth(sl_int32 ch)
	{
		float ret;
		if (PdfFontResource::getCharWidth(ch, ret)) {
			return ret;
		}
		String32 s = getUnicode(ch);
		if (s.isNotEmpty()) {
			return object->measureText(s).x / FONT_SCALE;
		} else {
			return 0;
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
			if (ret->_load(doc, ref, context, sl_false)) {
				context.images.put(ref.objectNumber, ret);
				return ret;
			}
		}
		return sl_null;
	}

	sl_bool PdfImage::_load(PdfDocument* doc, const PdfReference& ref, PdfResourceContext& context, sl_bool flagSMask)
	{
		const Ref<PdfStream>& stream = doc->getObject(ref).getStream();
		if (stream.isNotNull()) {
			if (PdfImageResource::load(stream.get())) {
				Memory content = stream->getContent();
				if (content.isNotNull()) {
					if (flagJpeg) {
						object = PlatformDrawable::loadFromMemory(content);
					} else {
						sl_uint8* data = (sl_uint8*)(content.getData());
						sl_uint32 size = (sl_uint32)(content.getSize());
						sl_uint32 w = width;
						if (flagFlate) {
							size = predict(data, size);
							if (!size) {
								return sl_false;
							}
							if (columns) {
								w = columns;
							}
						}
						Array<Color> indices;
						if (colorSpaceRef.objectNumber) {
							ListElements<PdfObject> arr(doc->getObject(colorSpaceRef).getArray());
							if (arr.count >= 4) {
								if (arr[0].getName() == StringView::literal("Indexed")) {
									sl_uint32 maxIndex = arr[2].getUint();
									if (maxIndex) {
										PdfObject objTable = doc->getObject(arr[3]);
										const String& strTable = objTable.getString();
										Memory memTable;
										sl_uint8* table;
										sl_size nTable;
										if (strTable.isNotNull()) {
											table = (sl_uint8*)(strTable.getData());
											nTable = strTable.getLength();
										} else {
											memTable = objTable.getStreamContent();
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
													c++;
												}
											}
										}
									}
								}
							}
						}
						sl_uint32 nColors = colors;
						if (!nColors) {
							if (colorSpace == PdfColorSpace::RGB) {
								nColors = 3;
							} else if (colorSpace == PdfColorSpace::CMYK) {
								nColors = 4;
							} else {
								nColors = 1;
							}
						}
						Ref<Image> image = CreateImageObject(data, size, w, nColors, bitsPerComponent, indices.getData(), (sl_uint32)(indices.getCount()));
						if (image.isNotNull()) {
							object = image;
							if (!flagSMask && smask.objectNumber) {
								PdfImage mask;
								if (mask._load(doc, smask, context, sl_true)) {
									Ref<Image> imageMask = CastRef<Image>(mask.object);
									if (imageMask.isNotNull()) {
										if (imageMask->getWidth() == image->getWidth() && imageMask->getHeight() == image->getHeight()) {
											ImageDesc descColor, descAlpha;
											image->getDesc(descColor);
											imageMask->getDesc(descAlpha);
											sl_uint32 n = descColor.width * descColor.height;
											Color* c = descColor.colors;
											Color* a = descAlpha.colors;
											for (sl_uint32 i = 0; i < n; i++) {
												c->a = a->r;
												c++;
												a++;
											}
										}
									}
								}
							}
						}
					}
					return object.isNotNull();
				}
			}
		}
		return sl_false;
	}


	PdfRenderContext::PdfRenderContext()
	{
	}

	PdfRenderContext::~PdfRenderContext()
	{
	}


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(PdfRenderParam)

	PdfRenderParam::PdfRenderParam()
	{
	}

	void PdfPage::render(PdfRenderParam& param)
	{
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

		Canvas* canvas = param.canvas;
		sl_bool flagOldAntiAlias = canvas->isAntiAlias();
		canvas->setAntiAlias();

		Renderer renderer;
		renderer.canvas = canvas;
		renderer.page = this;
		renderer.param = &param;

		Rectangle bounds = param.bounds;
		canvas->fillRectangle(bounds, Color::White);
		Swap(bounds.top, bounds.bottom);

		CanvasStateScope scope(canvas);
		canvas->concatMatrix(Transform2::getTransformMatrixFromRectToRect(getMediaBox(), bounds));
		canvas->clipToRectangle(getCropBox());

		for (sl_size i = 0; i < ops.count; i++) {
			renderer.render(ops[i]);
		}

		canvas->setAntiAlias(flagOldAntiAlias);
	}

}
