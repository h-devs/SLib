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

namespace slib
{

	namespace priv
	{
		namespace pdf
		{

			class TextState
			{
			public:
			};

			class RenderState
			{
			public:
				Ref<GraphicsPath> path;
				Ref<Brush> brush = Brush::createSolidBrush(Color::Black);
				BrushDesc brushDesc;
				Ref<Pen> pen = Pen::createSolidPen(1, Color::Black);
				PenDesc penDesc;

				TextState text;

			};

			class Renderer : public RenderState
			{
			public:
				// params
				Canvas* canvas;
				PdfPage* page;
				
			public:
				sl_bool preparePath()
				{
					if (path.isNull()) {
						path = GraphicsPath::create();
						return path.isNotNull();
					}
					return sl_false;
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

				void closePath()
				{
					if (path.isNotNull()) {
						path->closeSubpath();
					}
				}

				void createPen()
				{
					pen = Pen::create(penDesc);
				}

				void setLineDashPattern(ListElements<PdfObject> operands)
				{
					if (operands.count != 2) {
						return;
					}
					penDesc.style = PenStyle::Dash;
					createPen();
				}

				void createBrush()
				{
					brush = Brush::create(brushDesc);
				}

				void setColor(const Color& color, sl_bool flagStroking)
				{
					if (flagStroking) {
						penDesc.color = color;
						createPen();
					} else {
						brushDesc.color = color;
						createBrush();
					}
				}

				void setGrayLevel(ListElements<PdfObject> operands, sl_bool flagStroking)
				{
					if (operands.count != 1) {
						return;
					}
					sl_uint8 g = (sl_uint8)(operands[0].getFloat() * 255);
					setColor(Color(g, g, g), flagStroking);
				}

				void setLineJoin(ListElements<PdfObject> operands)
				{
					if (operands.count != 1) {
						return;
					}
					penDesc.join = (LineJoin)(operands[0].getUint());
				}

				void setLineCap(ListElements<PdfObject> operands)
				{
					if (operands.count != 1) {
						return;
					}
					penDesc.cap = (LineCap)(operands[0].getUint());
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
						canvas->fillPath(path, brush);
					}
				}

				void stroke()
				{
					if (path.isNotNull()) {
						canvas->drawPath(path, pen);
					}
				}

				void beginText()
				{
					text = TextState();
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
							// set color-space (for stroking)
							break;
						case PdfOperator::cs:
							// set color-space (for non-stroking)
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
							// set cmyk-color (for stroking)
							break;
						case PdfOperator::k:
							// set cmyk-color (for non-stroking)
							break;
						case PdfOperator::l:
							// line to
							break;
						case PdfOperator::m:
							// move to
							break;
						case PdfOperator::M:
							// set miter limit
							break;
						case PdfOperator::MP:
							// define marked-content point
							break;
						case PdfOperator::n:
							// end path without filling or stroking
							break;
						case PdfOperator::q:
							// save graphics state
							break;
						case PdfOperator::Q:
							// restore graphics state
							break;
						case PdfOperator::re:
							// append rectangle to path
							break;
						case PdfOperator::RG:
							// set rgb-color (for stroking)
							break;
						case PdfOperator::rg:
							// set rgb-color (for non-stroking)
							break;
						case PdfOperator::ri:
							// set color rendering intent
							break;
						case PdfOperator::s:
							// close path, stroke
							break;
						case PdfOperator::S:
							// stroke
							break;
						case PdfOperator::SC:
							// set color (for stroking)
							break;
						case PdfOperator::sc:
							// set color (for non-stroking)
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
							// T*: move to start of next text line
							break;
						case PdfOperator::Tc:
							// set character spacing
							break;
						case PdfOperator::Td:
							// move text position
							break;
						case PdfOperator::TD:
							// move text position and leading
							break;
						case PdfOperator::Tf:
							// select font and size
							break;
						case PdfOperator::Tj:
							// show text
							break;
						case PdfOperator::TJ:
							// show text, allowing individual glphy positioning
							break;
						case PdfOperator::TL:
							// set text leading
							break;
						case PdfOperator::Tm:
							// set text matrix and text line matrix
							break;
						case PdfOperator::Tr:
							// set text rendering mode
							break;
						case PdfOperator::Ts:
							// set text rise
							break;
						case PdfOperator::Tw:
							// set word spacing
							break;
						case PdfOperator::Tz:
							// set horizontal text scaling
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
