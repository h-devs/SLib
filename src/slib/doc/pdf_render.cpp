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

#define FONT_SCALE 72.0f

namespace slib
{

	namespace priv
	{
		namespace pdf
		{

			class FontResource : public PdfFontResource
			{
			public:
				Ref<Font> object;
				float scale = 1;

			public:
				float getCharWidth(sl_int32 ch)
				{
					if (widths.isNotNull() && ch >= firstChar && ch <= lastChar) {
						return widths[ch - firstChar] / 1000.0f;
					}
					return (object->measureText(StringView32((sl_char32*)&ch, 1))).x / FONT_SCALE;
				}

			};

			class TextState
			{
			public:
				float charSpace = 0;
				float wordSpace = 0;
				float widthScale = 1;
				float leading = 0;
				float rise = 0;
				FontResource font;
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
				PdfRenderParam param;

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
					SET_HANDLE_STATE(pen, style, PenStyle::Dash);
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
						text.leading = -ty;
					}
				}

				String loadEmbededFont(PdfReference& ref)
				{
					if (ref.objectNumber) {
						return param.onLoadFont(ref);
					}
					return sl_null;
				}

				Ref<Font> loadFont(PdfFontResource& res)
				{
					FontDesc fd;
					fd.familyName = loadEmbededFont(res.content);
					if (fd.familyName.isNull()) {
						fd.familyName = res.family;
					}
					fd.size = res.ascent * FONT_SCALE / 1000.0f;
					fd.flagBold = res.weight >= 600.0f;
					fd.flagItalic = Math::abs(res.italicAngle) > 10;
					Ref<Font> font = Font::create(fd);
					if (font.isNotNull()) {
						return font;
					}
					fd.familyName = Font::getDefaultFontFamily();
					return Font::create(fd);
				}

				void setFont(const String& name, float fontScale)
				{
					PdfFontResource res;
					if (page->getFontResource(name, res)) {
						(PdfFontResource&)(text.font) = Move(res);
						text.font.object = loadFont(text.font);
						text.font.scale = fontScale;
					}
				}

				void setTextFont(ListElements<PdfObject> operands)
				{
					if (operands.count != 2) {
						return;
					}
					setFont(operands[0].getName(), operands[1].getFloat());
				}

				void drawText(const String32& str)
				{
					sl_char32* array = str.getData();
					sl_size len = str.getLength();
					CanvasStateScope scope(canvas);
					Matrix3 mat = text.matrix;
					Transform2::preTranslate(mat, 0, text.rise);
					float scaleX = text.font.scale / FONT_SCALE;
					Transform2::preScale(mat, scaleX * text.widthScale, - text.font.scale / FONT_SCALE);
					canvas->concatMatrix(mat);
					if (text.font.object.isNotNull()) {
						sl_real x = 0;
						for (sl_size i = 0; i < len; i++) {
							sl_char32 ch = array[i];
							canvas->drawText(StringView32(&ch, 1), x / scaleX, 0, text.font.object, pen.color);
							x += text.font.getCharWidth(ch) * text.font.scale;
							if (ch == ' ') {
								x += text.wordSpace;
							} else {
								x += text.charSpace;
							}
						}
						Transform2::preTranslate(text.matrix, x * text.widthScale, 0);
					}
				}

				void adjustTextMatrix(float f)
				{
					Transform2::preTranslate(text.matrix, - f / 1000.0f * text.font.scale * text.widthScale, 0);
				}

				void showText(ListElements<PdfObject> operands)
				{
					if (operands.count != 1) {
						return;
					}
					const String& text = operands[0].getString();
					drawText(String32::from(text));
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
							drawText(String32::from(s));
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
					drawText(String32::from(text));
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

		}
	}

	using namespace priv::pdf;


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(PdfRenderParam)

	PdfRenderParam::PdfRenderParam()
	{
	}

	void PdfPage::render(const PdfRenderParam& param)
	{
		ListElements<PdfOperation> ops(getContent());
		if (!(ops.count)) {
			return;
		}

		Canvas* canvas = param.canvas;

		Renderer renderer;
		renderer.canvas = canvas;
		renderer.page = this;
		renderer.param = param;

		Rectangle bounds = param.bounds;
		canvas->fillRectangle(bounds, Color::White);
		Swap(bounds.top, bounds.bottom);

		CanvasStateScope scope(canvas);
		canvas->concatMatrix(Transform2::getTransformMatrixFromRectToRect(getMediaBox(), bounds));
		canvas->clipToRectangle(getCropBox());

		for (sl_size i = 0; i < ops.count; i++) {
			renderer.render(ops[i]);
		}
	}

}
