/*
 *   Copyright (c) 2008-2018 SLIBIO <https://github.com/SLIBIO>
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

package slib.android.ui;

import android.graphics.Canvas;
import android.graphics.Matrix;
import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.RectF;
import android.graphics.Rect;

import slib.android.Logger;

public class Graphics {

	private Canvas canvas;
	private boolean flagAntiAlias;
	private int alpha;

	public Graphics(Canvas canvas) {
		this.canvas = canvas;
		this.flagAntiAlias = true;
		this.alpha = 255;
	}

	Canvas getCanvas() {
		return canvas;
	}

	public int getWidth() {
		return canvas.getWidth();
	}

	public int getHeight() {
		return canvas.getHeight();
	}

	public void save() {
		try {
			canvas.save();
		} catch (Exception e) {
			Logger.exception(e);
		}
	}

	public void restore() {
		try {
			canvas.restore();
		} catch (Exception e) {
			Logger.exception(e);
		}
	}

	public Rect getClipBounds()
	{
		return canvas.getClipBounds();
	}

	public void clipToRectangle(float left, float top, float right, float bottom) {
		try {
			canvas.clipRect(left, top, right, bottom);
		} catch (Exception e) {
			Logger.exception(e);
		}
	}

	public void clipToPath(UiPath path) {
		if (path == null) {
			return;
		}
		try {
			canvas.clipPath(path.path);
		} catch (Exception e) {
			Logger.exception(e);
		}
	}

	public void concatMatrix(float m00, float m10, float m20, float m01, float m11, float m21, float m02, float m12, float m22) {
		try {
			Matrix mat = new Matrix();
			float[] values = new float[] {m00, m10, m20, m01, m11, m21, m02, m12, m22};
			mat.setValues(values);
			canvas.concat(mat);
		} catch (Exception e) {
			Logger.exception(e);
		}
	}

	public void drawText(String text, float x, float y, UiFont font, int color) {
		if (font == null || text == null) {
			return;
		}
		try {
			Paint paint = createPaint();
			font.applyPaint(paint);
			paint.setColor(Graphics.applyAlphaToColor(color, alpha));
			canvas.drawText(text, x, y - paint.ascent(), paint);
		} catch (Exception e) {
			Logger.exception(e);
		}
	}

	public void drawText(String text, float x, float y, UiFont font, int color, int shadowColor, float shadowRadius, float shadowOffsetX, float shadowOffsetY) {
		if (font == null || text == null) {
			return;
		}
		try {
			Paint paint = createPaint();
			font.applyPaint(paint);
			paint.setColor(Graphics.applyAlphaToColor(color, alpha));
			paint.setShadowLayer(shadowRadius, shadowOffsetX, shadowOffsetY, shadowColor);
			canvas.drawText(text, x, y - paint.ascent(), paint);
		} catch (Exception e) {
			Logger.exception(e);
		}
	}

	public void drawLine(float x1, float y1, float x2, float y2, UiPen pen) {
		if (pen == null) {
			return;
		}
		try {
			Paint paint = createPaint();
			pen.applyPaint(paint, alpha);
			canvas.drawLine(x1, y1, x2, y2, paint);
		} catch (Exception e) {
			Logger.exception(e);
		}
	}

	public void drawLines(float[] points, UiPen pen) {
		if (points == null) {
			return;
		}
		try {
			int n = points.length / 2;
			if (n > 1) {
				Path path = new Path();
				path.moveTo(points[0], points[1]);
				for (int i = 1; i < n; i++) {
					path.lineTo(points[i*2], points[i*2+1]);
				}
				Paint paint = createPaint();
				pen.applyPaint(paint, alpha);
				canvas.drawPath(path, paint);
			}
		} catch (Exception e) {
			Logger.exception(e);
		}
	}

	public void drawArc(float x1, float y1, float x2, float y2, float startDegrees, float sweepDegrees, UiPen pen) {
		if (pen == null) {
			return;
		}
		try {
			Paint paint = createPaint();
			pen.applyPaint(paint, alpha);
			canvas.drawArc(
					new RectF(x1, y1, x2, y2)
					, startDegrees
					, sweepDegrees
					, false, paint);
		} catch (Exception e) {
			Logger.exception(e);
		}
	}

	public void drawRectangle(float x1, float y1, float x2, float y2, UiPen pen, UiBrush brush) {
		try {
			Paint paint = createPaint();
			if (brush != null) {
				brush.applyPaint(paint, alpha);
				canvas.drawRect(x1, y1, x2, y2, paint);
			}
			if (pen != null) {
				pen.applyPaint(paint, alpha);
				canvas.drawRect(x1, y1, x2, y2, paint);
			}
		} catch (Exception e) {
			Logger.exception(e);
		}
	}

	public void drawRoundRectangle(float x1, float y1, float x2, float y2, float rx, float ry, UiPen pen, UiBrush brush) {
		try {
			Paint paint = createPaint();
			if (brush != null) {
				brush.applyPaint(paint, alpha);
				canvas.drawRoundRect(new RectF(x1, y1, x2, y2), rx, ry, paint);
			}
			if (pen != null) {
				pen.applyPaint(paint, alpha);
				canvas.drawRoundRect(new RectF(x1, y1, x2, y2), rx, ry, paint);
			}
		} catch (Exception e) {
			Logger.exception(e);
		}
	}

	public void drawEllipse(float x1, float y1, float x2, float y2, UiPen pen, UiBrush brush) {
		try {
			Paint paint = createPaint();
			if (brush != null) {
				brush.applyPaint(paint, alpha);
				canvas.drawOval(new RectF(x1, y1, x2, y2), paint);
			}
			if (pen != null) {
				pen.applyPaint(paint, alpha);
				canvas.drawOval(new RectF(x1, y1, x2, y2), paint);
			}
		} catch (Exception e) {
			Logger.exception(e);
		}
	}

	public void drawPolygon(float[] points, UiPen pen, UiBrush brush, int fillMode) {
		if (points == null) {
			return;
		}
		try {
			int n = points.length / 2;
			if (n > 1) {
				Path path = new Path();
				path.moveTo(points[0], points[1]);
				for (int i = 1; i < n; i++) {
					path.lineTo(points[i*2], points[i*2+1]);
				}
				path.close();
				path.setFillType(UiPath.getFillType(fillMode));
				Paint paint = createPaint();
				if (brush != null) {
					brush.applyPaint(paint, alpha);
					canvas.drawPath(path, paint);
				}
				if (pen != null) {
					pen.applyPaint(paint, alpha);
					canvas.drawPath(path, paint);
				}
			}
		} catch (Exception e) {
			Logger.exception(e);
		}
	}

	public void drawPath(UiPath path, UiPen pen, UiBrush brush) {
		if (path == null) {
			return;
		}
		try {
			Paint paint = createPaint();
			if (brush != null) {
				brush.applyPaint(paint, alpha);
				canvas.drawPath(path.path, paint);
			}
			if (pen != null) {
				pen.applyPaint(paint, alpha);
				canvas.drawPath(path.path, paint);
			}
		} catch (Exception e) {
			Logger.exception(e);
		}
	}

	public void drawPie(float x1, float y1, float x2, float y2, float startDegrees, float sweepDegrees, UiPen pen, UiBrush brush) {
		try {
			Paint paint = createPaint();
			if (brush != null) {
				brush.applyPaint(paint, alpha);
				canvas.drawArc(
						new RectF(x1, y1, x2, y2)
						, startDegrees
						, sweepDegrees
						, true, paint);
			}
			if (pen != null) {
				pen.applyPaint(paint, alpha);
				canvas.drawArc(
						new RectF(x1, y1, x2, y2)
						, startDegrees
						, sweepDegrees
						, true, paint);
			}
		} catch (Exception e) {
			Logger.exception(e);
		}
	}

	public int getAlpha() {
		return alpha;
	}

	public void setAlpha(int alpha) {
		this.alpha = alpha;
	}

	public void setAlpha(float alpha) {
		int a = (int)(alpha * 255);
		if (a < 0) {
			a = 0;
		}
		if (a > 255) {
			a = 255;
		}
		this.alpha = a;
	}

	public boolean isAntiAlias() {
		return flagAntiAlias;
	}

	public void setAntiAlias(boolean flag) {
		this.flagAntiAlias = flag;
	}

	Paint createPaint()
	{
		Paint paint = new Paint();
		paint.setAntiAlias(flagAntiAlias);
		return paint;
	}

	public static int applyAlphaToColor(int color, int alpha) {
		if (alpha < 255) {
			int a = (color >> 24) & 255;
			a = a * alpha / 255;
			color = (color & 0xFFFFFF) | (a << 24);
		}
		return color;
	}

}
