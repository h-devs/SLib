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

package slib.platform.android.ui;

import java.util.Vector;

import slib.android.Logger;
import slib.platform.android.SlibActivity;

import android.app.Activity;
import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.BlurMaskFilter;
import android.graphics.Canvas;
import android.graphics.ColorMatrixColorFilter;
import android.graphics.Paint;
import android.graphics.Point;
import android.graphics.Rect;
import android.graphics.RectF;
import android.os.Build;
import android.renderscript.Allocation;
import android.renderscript.Element;
import android.renderscript.RenderScript;
import android.renderscript.ScriptIntrinsicBlur;

public class UiBitmap {
	
	public Bitmap bitmap;
	
	private UiBitmap() {}

	public static UiBitmap create(int width, int height) {
		try {
			Bitmap bitmap = Bitmap.createBitmap(width, height, Bitmap.Config.ARGB_8888);
			if (bitmap != null) {
				UiBitmap ret = new UiBitmap();
				ret.bitmap = bitmap;
				return ret;
			}
		} catch (Throwable e) {
			Logger.exception(e);
		}
		return null;
	}

	public static UiBitmap load(byte[] data) {
		try {
			Bitmap bitmap = BitmapFactory.decodeByteArray(data, 0, data.length);
			if (bitmap != null) {
				if (bitmap.getWidth() > 0 && bitmap.getHeight() > 0) {
					UiBitmap ret = new UiBitmap();
					ret.bitmap = bitmap;
					return ret;
				}
				bitmap.recycle();
			}
		} catch (Throwable e) {
			Logger.exception(e);
		}
		return null;
	}

	public static UiBitmap load(Activity activity, byte[] data) {
		if (activity != null) {
			Point size = Util.getDisplaySize(activity);
			int s = Math.min(size.x, size.y);
			return load(data, s, s);
		} else {
			return load(data);
		}
	}

	public static UiBitmap load(byte[] data, int maxWidth, int maxHeight) {
		byte[] buf = getTempStorage();
		try {
			BitmapFactory.Options options = new BitmapFactory.Options();
			options.inTempStorage = buf;
			options.inJustDecodeBounds = true;
			BitmapFactory.decodeByteArray(data, 0, data.length, options);
			if (maxWidth > 0 && maxHeight > 0){
				final int height = options.outHeight;
				final int width = options.outWidth;
				int inSampleSize = 1;
				if ((width / inSampleSize) > maxWidth || (height / inSampleSize) > maxHeight) {
					inSampleSize *= 2;
				}
				options.inSampleSize = inSampleSize;
			}
			options.inJustDecodeBounds = false;
			Bitmap bitmap = BitmapFactory.decodeByteArray(data, 0, data.length, options);
			if (bitmap != null) {
				if (bitmap.getWidth() > 0 && bitmap.getHeight() > 0) {
					UiBitmap ret = new UiBitmap();
					ret.bitmap = bitmap;
					returnTempStorage(buf);
					return ret;
				}
				bitmap.recycle();
			}
		} catch (Throwable e) {
			Logger.exception(e);
		}
		returnTempStorage(buf);
		return null;
	}
	
	public int getWidth() {
		try {
			return bitmap.getWidth();
		} catch (Throwable e) {
			Logger.exception(e);
			return 0;
		}
	}
	
	public int getHeight() {
		try {
			return bitmap.getHeight();
		} catch (Throwable e) {
			Logger.exception(e);
			return 0;
		}
	}
	
	public void recycle() {
		try {
			bitmap.recycle();
			bitmap = null;
		} catch (Throwable e) {
			Logger.exception(e);
		}
	}
	
	public void read(int x, int y, int width, int height, int[] out, int stride) {
		try {
			bitmap.getPixels(out, 0, stride, x, y, width, height);
		} catch (Throwable e) {
			Logger.exception(e);
		}
	}
	
	public void write(int x, int y, int width, int height, int[] in, int stride) {
		try {
			bitmap.setPixels(in, 0, stride, x, y, width, height);
		} catch (Throwable e) {
			Logger.exception(e);
		}
	}
	
	public Graphics getCanvas() {
		try {
			Canvas canvas = new Canvas(bitmap);
			return new Graphics(canvas);
		} catch (Throwable e) {
			Logger.exception(e);
			return null;
		}
	}
	
	void draw(Graphics graphics,
			float dx1, float dy1, float dx2, float dy2,
			int sx1, int sy1, int sx2, int sy2, 
			float _alpha, float blur,
			float[] cm) 
	{
		try {
			int alpha = (int)(_alpha * graphics.getAlpha());
			if (alpha <= 0) {
				return;
			}
			Canvas canvas = graphics.getCanvas();
			RectF rd = new RectF(dx1, dy1, dx2, dy2);
			Rect rs = new Rect(sx1, sy1, sx2, sy2);
			int iblur = (int)(Math.ceil(blur));
			if (alpha < 255 || cm != null || iblur >= 1) {
				Paint paint = new Paint();
				if (alpha < 255) {
					paint.setAlpha(alpha);
				}
				if (cm != null) {
					ColorMatrixColorFilter cf = new ColorMatrixColorFilter(cm);
					paint.setColorFilter(cf);
				}
				if (iblur >= 1) {
					if (iblur > 25) {
						iblur = 25;
					}
					Bitmap bitmapBlured = blurBitmap(bitmap, rs, (int)(dx2 - dx1), (int)(dy2 - dy1), iblur);
					if (bitmapBlured != null) {
						rd.left -= iblur;
						rd.top -= iblur;
						rd.right += iblur;
						rd.bottom += iblur;
						canvas.drawBitmap(bitmapBlured, new Rect(0, 0, bitmapBlured.getWidth(), bitmapBlured.getHeight()), rd, paint);
					}
				} else {
					canvas.drawBitmap(bitmap, rs, rd, paint);
				}
			} else {
				canvas.drawBitmap(bitmap, rs, rd, null);
			}
		} catch (Throwable e) {
			Logger.exception(e);
		}
	}
	
	public void draw(Graphics graphics,
			float dx1, float dy1, float dx2, float dy2,
			int sx1, int sy1, int sx2, int sy2,
			float alpha, float blur)
	{
		draw(graphics, dx1, dy1, dx2, dy2, sx1, sy1, sx2, sy2, alpha, blur, null);
	}

	public void draw(Graphics graphics,
			float dx1, float dy1, float dx2, float dy2,
			int sx1, int sy1, int sx2, int sy2,
			float alpha, float blur,
			float crx, float cry, float crz, float crw,
			float cgx, float cgy, float cgz, float cgw,
			float cbx, float cby, float cbz, float cbw,
			float cax, float cay, float caz, float caw,
			float ccx, float ccy, float ccz, float ccw)
	{
		float[] f = new float[] {
			crx, cry, crz, crw, ccx * 255,
			cgx, cgy, cgz, cgw, ccy * 255,
			cbx, cby, cbz, cbw, ccz * 255,
			cax, cay, caz, caw, ccw * 255,
		};
		draw(graphics, dx1, dy1, dx2, dy2, sx1, sy1, sx2, sy2, alpha, blur, f);
	}
	
	static void drawPixels(Graphics graphics,
			float dx1, float dy1, float dx2, float dy2,
			int[] pixels, int stride, int sw, int sh,
			float _alpha, float blur,
			float[] cm) 
	{
		try {
			if (sw == 0 || sh == 0) {
				return;
			}
			int alpha = (int)(_alpha * graphics.getAlpha());
			if (alpha <= 0) {
				return;
			}
			Canvas canvas = graphics.getCanvas();
			canvas.save();
			canvas.scale((dx2 - dx1) / sw, (dy2 - dy1) / sh, dx1, dy1);
			if (alpha < 255 || cm != null || blur > 0.5f) {
				Paint paint = new Paint();
				if (alpha < 255) {
					paint.setAlpha(alpha);
				}
				if (cm != null) {
					ColorMatrixColorFilter cf = new ColorMatrixColorFilter(cm);
					paint.setColorFilter(cf);
				}
				if (blur > 0.5f) {
					paint.setMaskFilter(new BlurMaskFilter(blur, BlurMaskFilter.Blur.NORMAL));
				}
				canvas.drawBitmap(pixels, 0, stride, dx1, dy1, sw, sh, true, paint);
			} else {
				canvas.drawBitmap(pixels, 0, stride, dx1, dy1, sw, sh, true, null);
			}
			canvas.restore();
		} catch (Throwable e) {
			Logger.exception(e);
		}
	}
	
	public static void drawPixels(Graphics graphics,
			float dx1, float dy1, float dx2, float dy2,
			int[] pixels, int stride, int sw, int sh,
			float alpha, float blur)
	{
		drawPixels(graphics, dx1, dy1, dx2, dy2, pixels, stride, sw, sh, alpha, blur, null);
	}
	
	public static void drawPixels(Graphics graphics,
			float dx1, float dy1, float dx2, float dy2,
			int[] pixels, int stride, int sw, int sh,
			float alpha, float blur,
			float crx, float cry, float crz, float crw,
			float cgx, float cgy, float cgz, float cgw,
			float cbx, float cby, float cbz, float cbw,
			float cax, float cay, float caz, float caw,
			float ccx, float ccy, float ccz, float ccw)
	{
		float[] f = new float[] {
			crx, cry, crz, crw, ccx * 255,
			cgx, cgy, cgz, cgw, ccy * 255,
			cbx, cby, cbz, cbw, ccz * 255,
			cax, cay, caz, caw, ccw * 255,
		};
		drawPixels(graphics, dx1, dy1, dx2, dy2, pixels, stride, sw, sh, alpha, blur, f);
	}
	
	static int[] arrayBufferForUi;
	static Vector<int[]> arrayBufferForNonUi = new Vector<int[]>();
	public static int[] getArrayBuffer() {
		try {
			if (UiThread.isUiThread()) {
				if (arrayBufferForUi == null) {
					arrayBufferForUi = new int[1024 * 1024];
				}
				return arrayBufferForUi;
			} else {
				int[] ret = null;
				try {
					ret = arrayBufferForNonUi.remove(0);
				} catch (Exception ex) {}
				if (ret == null) {
					ret = new int[1024 * 1024];
				}
				return ret;
			}
		} catch (Throwable e) {
			Logger.exception(e);
			return null;
		}
	}
	
	public static void returnArrayBuffer(int[] buffer) {
		if (buffer != arrayBufferForUi) {
			if (arrayBufferForNonUi.size() < 5) {
				arrayBufferForNonUi.add(buffer);
			}
		}
	}

	private static byte[] mTempStorage = null;
	private static boolean mInitTempStorage = false;
	private static final Object mSyncTempStorage = new Object();

	public static byte[] getTempStorage() {
		synchronized (mSyncTempStorage) {
			if (!mInitTempStorage) {
				mInitTempStorage = true;
				mTempStorage = new byte[16*1024];
			}
			byte[] temp = mTempStorage;
			mTempStorage = null;
			return temp;
		}
	}

	public static void returnTempStorage(byte[] buf) {
		if (buf == null) {
			return;
		}
		synchronized (mSyncTempStorage) {
			mTempStorage = buf;
		}
	}

	public static Bitmap blurBitmap(Bitmap src, Rect region, int width, int height, int blur) {
		if (width <= 0 || height <= 0 || blur <= 0) {
			return null;
		}
		if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN_MR1) {
			Bitmap bitmap = Bitmap.createBitmap(width + blur * 2, height + blur * 2, src.getConfig());
			if (bitmap != null) {
				Canvas canvas = new Canvas(bitmap);
				canvas.drawBitmap(src, region, new RectF(blur, blur, blur + width, blur + height), null);
				Context context = SlibActivity.applicationContext;
				if (context != null) {
					RenderScript rs = RenderScript.create(context);
					Allocation input = Allocation.createFromBitmap(rs, bitmap, Allocation.MipmapControl.MIPMAP_NONE, Allocation.USAGE_SCRIPT);
					Allocation output = Allocation.createTyped(rs, input.getType());
					ScriptIntrinsicBlur script = ScriptIntrinsicBlur.create(rs, Element.U8_4(rs));
					script.setRadius(blur);
					script.setInput(input);
					script.forEach(output);
					output.copyTo(bitmap);
					return bitmap;
				}
			}
		}
		return null;
	}

}
