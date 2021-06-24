/*
 *   Copyright (c) 2008-2021 SLIBIO <https://github.com/SLIBIO>
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

import android.view.Gravity;

public class Constants {

	public static int getAndroidAlignment(int align) {
		int ret = 0;
		int horz = align & 3;
		if (horz == 1) {
			ret |= Gravity.LEFT;
		} else if (horz == 2) {
			ret |= Gravity.RIGHT;
		} else {
			ret |= Gravity.CENTER_HORIZONTAL;
		}
		int vert = align & 12;
		if (vert == 4) {
			ret |= Gravity.TOP;
		} else if (vert == 8) {
			ret |= Gravity.BOTTOM;
		} else {
			ret |= Gravity.CENTER_VERTICAL;
		}
		return ret;
	}

	public static int getSlibAlignment(int gravity) {
		int ret = 0;
		if ((gravity & Gravity.LEFT) != 0) {
			ret |= 1;
		} else if ((gravity & Gravity.RIGHT) != 0) {
			ret |= 2;
		}
		if ((gravity & Gravity.TOP) != 0) {
			ret |= 4;
		} else if ((gravity & Gravity.BOTTOM) != 0) {
			ret |= 8;
		}
		return ret;
	}

}