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

import java.util.HashMap;

import slib.android.Logger;
import android.graphics.Paint;
import android.graphics.Paint.FontMetrics;
import android.graphics.PointF;
import android.graphics.Typeface;

public class UiFont {

	private static HashMap<String, Typeface> typefaces = new HashMap<String, Typeface>();

	private Typeface font;
	private float size;
	private boolean flagUnderline;
	private boolean flagStrikeout;
	private Paint paint;
	private float height;

	public UiFont(Typeface font, float size, boolean flagUnderline, boolean flagStrikeout) {
		this.font = font;
		this.size = size;
		this.flagUnderline = flagUnderline;
		this.flagStrikeout = flagStrikeout;

		paint = new Paint();
		applyPaint(paint);
		height = paint.descent() - paint.ascent();
	}

	public Typeface getTypeface() {
		return font;
	}

	public float getSize() {
		return size;
	}

	public static void registerTypeface(String name, Typeface typeface) {
		if (typeface != null) {
			typefaces.put(name, typeface);
		}
	}

	public static UiFont create(String fontName, float size, int style) {

		UiFont font = null;

		try {
			Typeface base;
			if (fontName != null) {
				base = typefaces.get(fontName);
			} else {
				base = Typeface.DEFAULT;
			}
			Typeface face = null;
			int _style = style & 3;
			if (base != null) {
				face = Typeface.create(base, _style);
			} else {
				face = Typeface.create(fontName, _style);
			}

			if (face != null) {
				boolean flagUnderline = (style & 4) != 0;
				boolean flagStrikeout = (style & 8) != 0;
				font = new UiFont(face, size, flagUnderline, flagStrikeout);
			}
		} catch (Throwable e) {
			Logger.exception(e);
		}

		return font;
	}

	public void applyPaint(Paint paint) {
		paint.setAntiAlias(true);
		paint.setTypeface(font);
		paint.setUnderlineText(flagUnderline);
		paint.setStrikeThruText(flagStrikeout);
		paint.setTextSize(size);
	}

	public FontMetrics getFontMetrics() {
		try {
			return paint.getFontMetrics();
		} catch (Exception e) {
			Logger.exception(e);
			return null;
		}
	}

	public PointF measureText(String text) {
		float width = 0;
		float height = this.height;
		try {
			if (text != null && text.length() != 0) {
				width = paint.measureText(text);
			}
			FontMetrics fm = paint.getFontMetrics();
			height = -fm.ascent + fm.descent;
		} catch (Exception e) {
			Logger.exception(e);
		}
		return new PointF(width, height);
	}

}
