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

#ifndef CHECKHEADER_SLIB_PLATFORM_ANDROID_VERSION
#define CHECKHEADER_SLIB_PLATFORM_ANDROID_VERSION

namespace slib
{

	enum class AndroidSdkVersion
	{
		CUR_DEVELOPMENT = 0,
		BASE = 1,
		BASE_1_1 = 2,
		CUPCAKE = 3,
		DONUT = 4,
		ECLAIR = 5,
		ECLAIR_0_1 = 6,
		ECLAIR_MR1 = 7,
		FROYO = 8,
		GINGERBREAD = 9,
		GINGERBREAD_MR1 = 10,
		HONEYCOMB = 11,
		HONEYCOMB_MR1 = 12,
		HONEYCOMB_MR2 = 13,
		ICE_CREAM_SANDWICH = 14,
		ICE_CREAM_SANDWICH_MR1 = 15,
		JELLY_BEAN = 16,
		JELLY_BEAN_MR1 = 17,
		JELLY_BEAN_MR2 = 18,
		KITKAT = 19,
		KITKAT_WATCH = 20,
		L = 21,
		LOLLIPOP = 21,
		LOLLIPOP_MR1 = 22,
		M = 23,
		N = 24,
		N_MR1 = 25,
		O = 26,
		O_MR1 = 27,
		P = 28,
		Q = 29
	};

}

#endif
