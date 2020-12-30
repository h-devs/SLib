/*
*   Copyright (c) 2008-2020 SLIBIO <https://github.com/SLIBIO>
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

#include "slib/math/fft.h"

#include "slib/core/base.h"

// Referenced from fft4g.c in WebRTC (the original source is from http://www.kurims.kyoto-u.ac.jp/~ooura/fft.html [Copyright Takuya OOURA, 1996-2001, Public Domain])

namespace slib
{

	namespace priv
	{
		namespace fft
		{

			static void bitrv2(sl_uint32 n, sl_uint32 *ip, sl_real *a)
			{
				sl_uint32 j, j1, k, k1, l, m, m2;
				sl_real xr, xi, yr, yi;

				ip[0] = 0;
				l = n;
				m = 1;
				while ((m << 3) < l) {
					l >>= 1;
					for (j = 0; j < m; j++) {
						ip[m + j] = ip[j] + l;
					}
					m <<= 1;
				}
				m2 = 2 * m;
				if ((m << 3) == l) {
					for (k = 0; k < m; k++) {
						for (j = 0; j < k; j++) {
							j1 = 2 * j + ip[k];
							k1 = 2 * k + ip[j];
							xr = a[j1];
							xi = a[j1 + 1];
							yr = a[k1];
							yi = a[k1 + 1];
							a[j1] = yr;
							a[j1 + 1] = yi;
							a[k1] = xr;
							a[k1 + 1] = xi;
							j1 += m2;
							k1 += 2 * m2;
							xr = a[j1];
							xi = a[j1 + 1];
							yr = a[k1];
							yi = a[k1 + 1];
							a[j1] = yr;
							a[j1 + 1] = yi;
							a[k1] = xr;
							a[k1 + 1] = xi;
							j1 += m2;
							k1 -= m2;
							xr = a[j1];
							xi = a[j1 + 1];
							yr = a[k1];
							yi = a[k1 + 1];
							a[j1] = yr;
							a[j1 + 1] = yi;
							a[k1] = xr;
							a[k1 + 1] = xi;
							j1 += m2;
							k1 += 2 * m2;
							xr = a[j1];
							xi = a[j1 + 1];
							yr = a[k1];
							yi = a[k1 + 1];
							a[j1] = yr;
							a[j1 + 1] = yi;
							a[k1] = xr;
							a[k1 + 1] = xi;
						}
						j1 = 2 * k + m2 + ip[k];
						k1 = j1 + m2;
						xr = a[j1];
						xi = a[j1 + 1];
						yr = a[k1];
						yi = a[k1 + 1];
						a[j1] = yr;
						a[j1 + 1] = yi;
						a[k1] = xr;
						a[k1 + 1] = xi;
					}
				} else {
					for (k = 1; k < m; k++) {
						for (j = 0; j < k; j++) {
							j1 = 2 * j + ip[k];
							k1 = 2 * k + ip[j];
							xr = a[j1];
							xi = a[j1 + 1];
							yr = a[k1];
							yi = a[k1 + 1];
							a[j1] = yr;
							a[j1 + 1] = yi;
							a[k1] = xr;
							a[k1 + 1] = xi;
							j1 += m2;
							k1 += m2;
							xr = a[j1];
							xi = a[j1 + 1];
							yr = a[k1];
							yi = a[k1 + 1];
							a[j1] = yr;
							a[j1 + 1] = yi;
							a[k1] = xr;
							a[k1 + 1] = xi;
						}
					}
				}
			}

			static void bitrv2conj(sl_uint32 n, sl_uint32 *ip, sl_real *a)
			{
				sl_uint32 j, j1, k, k1, l, m, m2;
				sl_real xr, xi, yr, yi;

				ip[0] = 0;
				l = n;
				m = 1;
				while ((m << 3) < l) {
					l >>= 1;
					for (j = 0; j < m; j++) {
						ip[m + j] = ip[j] + l;
					}
					m <<= 1;
				}
				m2 = 2 * m;
				if ((m << 3) == l) {
					for (k = 0; k < m; k++) {
						for (j = 0; j < k; j++) {
							j1 = 2 * j + ip[k];
							k1 = 2 * k + ip[j];
							xr = a[j1];
							xi = -a[j1 + 1];
							yr = a[k1];
							yi = -a[k1 + 1];
							a[j1] = yr;
							a[j1 + 1] = yi;
							a[k1] = xr;
							a[k1 + 1] = xi;
							j1 += m2;
							k1 += 2 * m2;
							xr = a[j1];
							xi = -a[j1 + 1];
							yr = a[k1];
							yi = -a[k1 + 1];
							a[j1] = yr;
							a[j1 + 1] = yi;
							a[k1] = xr;
							a[k1 + 1] = xi;
							j1 += m2;
							k1 -= m2;
							xr = a[j1];
							xi = -a[j1 + 1];
							yr = a[k1];
							yi = -a[k1 + 1];
							a[j1] = yr;
							a[j1 + 1] = yi;
							a[k1] = xr;
							a[k1 + 1] = xi;
							j1 += m2;
							k1 += 2 * m2;
							xr = a[j1];
							xi = -a[j1 + 1];
							yr = a[k1];
							yi = -a[k1 + 1];
							a[j1] = yr;
							a[j1 + 1] = yi;
							a[k1] = xr;
							a[k1 + 1] = xi;
						}
						k1 = 2 * k + ip[k];
						a[k1 + 1] = -a[k1 + 1];
						j1 = k1 + m2;
						k1 = j1 + m2;
						xr = a[j1];
						xi = -a[j1 + 1];
						yr = a[k1];
						yi = -a[k1 + 1];
						a[j1] = yr;
						a[j1 + 1] = yi;
						a[k1] = xr;
						a[k1 + 1] = xi;
						k1 += m2;
						a[k1 + 1] = -a[k1 + 1];
					}
				} else {
					a[1] = -a[1];
					a[m2 + 1] = -a[m2 + 1];
					for (k = 1; k < m; k++) {
						for (j = 0; j < k; j++) {
							j1 = 2 * j + ip[k];
							k1 = 2 * k + ip[j];
							xr = a[j1];
							xi = -a[j1 + 1];
							yr = a[k1];
							yi = -a[k1 + 1];
							a[j1] = yr;
							a[j1 + 1] = yi;
							a[k1] = xr;
							a[k1 + 1] = xi;
							j1 += m2;
							k1 += m2;
							xr = a[j1];
							xi = -a[j1 + 1];
							yr = a[k1];
							yi = -a[k1 + 1];
							a[j1] = yr;
							a[j1 + 1] = yi;
							a[k1] = xr;
							a[k1 + 1] = xi;
						}
						k1 = 2 * k + ip[k];
						a[k1 + 1] = -a[k1 + 1];
						a[k1 + m2 + 1] = -a[k1 + m2 + 1];
					}
				}
			}

			static void makewt(sl_uint32 nw, sl_uint32 *ip, sl_real *w)
			{
				sl_uint32 j, nwh;
				sl_real delta, x, y;

				ip[0] = nw;
				ip[1] = 1;
				if (nw > 2) {
					nwh = nw >> 1;
					delta = Math::arctan((sl_real)1) / nwh;
					w[0] = 1;
					w[1] = 0;
					w[nwh] = Math::cos(delta * (sl_real)nwh);
					w[nwh + 1] = w[nwh];
					if (nwh > 2) {
						for (j = 2; j < nwh; j += 2) {
							x = Math::cos(delta * (sl_real)j);
							y = Math::sin(delta * (sl_real)j);
							w[j] = x;
							w[j + 1] = y;
							w[nw - j] = y;
							w[nw - j + 1] = x;
						}
						bitrv2(nw, ip + 2, w);
					}
				}
			}

			static void cft1st(sl_uint32 n, sl_real *a, sl_real *w)
			{
				sl_uint32 j, k1, k2;
				sl_real wk1r, wk1i, wk2r, wk2i, wk3r, wk3i;
				sl_real x0r, x0i, x1r, x1i, x2r, x2i, x3r, x3i;

				x0r = a[0] + a[2];
				x0i = a[1] + a[3];
				x1r = a[0] - a[2];
				x1i = a[1] - a[3];
				x2r = a[4] + a[6];
				x2i = a[5] + a[7];
				x3r = a[4] - a[6];
				x3i = a[5] - a[7];
				a[0] = x0r + x2r;
				a[1] = x0i + x2i;
				a[4] = x0r - x2r;
				a[5] = x0i - x2i;
				a[2] = x1r - x3i;
				a[3] = x1i + x3r;
				a[6] = x1r + x3i;
				a[7] = x1i - x3r;
				wk1r = w[2];
				x0r = a[8] + a[10];
				x0i = a[9] + a[11];
				x1r = a[8] - a[10];
				x1i = a[9] - a[11];
				x2r = a[12] + a[14];
				x2i = a[13] + a[15];
				x3r = a[12] - a[14];
				x3i = a[13] - a[15];
				a[8] = x0r + x2r;
				a[9] = x0i + x2i;
				a[12] = x2i - x0i;
				a[13] = x0r - x2r;
				x0r = x1r - x3i;
				x0i = x1i + x3r;
				a[10] = wk1r * (x0r - x0i);
				a[11] = wk1r * (x0r + x0i);
				x0r = x3i + x1r;
				x0i = x3r - x1i;
				a[14] = wk1r * (x0i - x0r);
				a[15] = wk1r * (x0i + x0r);
				k1 = 0;
				for (j = 16; j < n; j += 16) {
					k1 += 2;
					k2 = 2 * k1;
					wk2r = w[k1];
					wk2i = w[k1 + 1];
					wk1r = w[k2];
					wk1i = w[k2 + 1];
					wk3r = wk1r - 2 * wk2i * wk1i;
					wk3i = 2 * wk2i * wk1r - wk1i;
					x0r = a[j] + a[j + 2];
					x0i = a[j + 1] + a[j + 3];
					x1r = a[j] - a[j + 2];
					x1i = a[j + 1] - a[j + 3];
					x2r = a[j + 4] + a[j + 6];
					x2i = a[j + 5] + a[j + 7];
					x3r = a[j + 4] - a[j + 6];
					x3i = a[j + 5] - a[j + 7];
					a[j] = x0r + x2r;
					a[j + 1] = x0i + x2i;
					x0r -= x2r;
					x0i -= x2i;
					a[j + 4] = wk2r * x0r - wk2i * x0i;
					a[j + 5] = wk2r * x0i + wk2i * x0r;
					x0r = x1r - x3i;
					x0i = x1i + x3r;
					a[j + 2] = wk1r * x0r - wk1i * x0i;
					a[j + 3] = wk1r * x0i + wk1i * x0r;
					x0r = x1r + x3i;
					x0i = x1i - x3r;
					a[j + 6] = wk3r * x0r - wk3i * x0i;
					a[j + 7] = wk3r * x0i + wk3i * x0r;
					wk1r = w[k2 + 2];
					wk1i = w[k2 + 3];
					wk3r = wk1r - 2 * wk2r * wk1i;
					wk3i = 2 * wk2r * wk1r - wk1i;
					x0r = a[j + 8] + a[j + 10];
					x0i = a[j + 9] + a[j + 11];
					x1r = a[j + 8] - a[j + 10];
					x1i = a[j + 9] - a[j + 11];
					x2r = a[j + 12] + a[j + 14];
					x2i = a[j + 13] + a[j + 15];
					x3r = a[j + 12] - a[j + 14];
					x3i = a[j + 13] - a[j + 15];
					a[j + 8] = x0r + x2r;
					a[j + 9] = x0i + x2i;
					x0r -= x2r;
					x0i -= x2i;
					a[j + 12] = -wk2i * x0r - wk2r * x0i;
					a[j + 13] = -wk2i * x0i + wk2r * x0r;
					x0r = x1r - x3i;
					x0i = x1i + x3r;
					a[j + 10] = wk1r * x0r - wk1i * x0i;
					a[j + 11] = wk1r * x0i + wk1i * x0r;
					x0r = x1r + x3i;
					x0i = x1i - x3r;
					a[j + 14] = wk3r * x0r - wk3i * x0i;
					a[j + 15] = wk3r * x0i + wk3i * x0r;
				}
			}

			static void cftmdl(sl_uint32 n, sl_uint32 l, sl_real *a, sl_real *w)
			{
				sl_uint32 j, j1, j2, j3, k, k1, k2, m, m2;
				sl_real wk1r, wk1i, wk2r, wk2i, wk3r, wk3i;
				sl_real x0r, x0i, x1r, x1i, x2r, x2i, x3r, x3i;

				m = l << 2;
				for (j = 0; j < l; j += 2) {
					j1 = j + l;
					j2 = j1 + l;
					j3 = j2 + l;
					x0r = a[j] + a[j1];
					x0i = a[j + 1] + a[j1 + 1];
					x1r = a[j] - a[j1];
					x1i = a[j + 1] - a[j1 + 1];
					x2r = a[j2] + a[j3];
					x2i = a[j2 + 1] + a[j3 + 1];
					x3r = a[j2] - a[j3];
					x3i = a[j2 + 1] - a[j3 + 1];
					a[j] = x0r + x2r;
					a[j + 1] = x0i + x2i;
					a[j2] = x0r - x2r;
					a[j2 + 1] = x0i - x2i;
					a[j1] = x1r - x3i;
					a[j1 + 1] = x1i + x3r;
					a[j3] = x1r + x3i;
					a[j3 + 1] = x1i - x3r;
				}
				wk1r = w[2];
				for (j = m; j < l + m; j += 2) {
					j1 = j + l;
					j2 = j1 + l;
					j3 = j2 + l;
					x0r = a[j] + a[j1];
					x0i = a[j + 1] + a[j1 + 1];
					x1r = a[j] - a[j1];
					x1i = a[j + 1] - a[j1 + 1];
					x2r = a[j2] + a[j3];
					x2i = a[j2 + 1] + a[j3 + 1];
					x3r = a[j2] - a[j3];
					x3i = a[j2 + 1] - a[j3 + 1];
					a[j] = x0r + x2r;
					a[j + 1] = x0i + x2i;
					a[j2] = x2i - x0i;
					a[j2 + 1] = x0r - x2r;
					x0r = x1r - x3i;
					x0i = x1i + x3r;
					a[j1] = wk1r * (x0r - x0i);
					a[j1 + 1] = wk1r * (x0r + x0i);
					x0r = x3i + x1r;
					x0i = x3r - x1i;
					a[j3] = wk1r * (x0i - x0r);
					a[j3 + 1] = wk1r * (x0i + x0r);
				}
				k1 = 0;
				m2 = 2 * m;
				for (k = m2; k < n; k += m2) {
					k1 += 2;
					k2 = 2 * k1;
					wk2r = w[k1];
					wk2i = w[k1 + 1];
					wk1r = w[k2];
					wk1i = w[k2 + 1];
					wk3r = wk1r - 2 * wk2i * wk1i;
					wk3i = 2 * wk2i * wk1r - wk1i;
					for (j = k; j < l + k; j += 2) {
						j1 = j + l;
						j2 = j1 + l;
						j3 = j2 + l;
						x0r = a[j] + a[j1];
						x0i = a[j + 1] + a[j1 + 1];
						x1r = a[j] - a[j1];
						x1i = a[j + 1] - a[j1 + 1];
						x2r = a[j2] + a[j3];
						x2i = a[j2 + 1] + a[j3 + 1];
						x3r = a[j2] - a[j3];
						x3i = a[j2 + 1] - a[j3 + 1];
						a[j] = x0r + x2r;
						a[j + 1] = x0i + x2i;
						x0r -= x2r;
						x0i -= x2i;
						a[j2] = wk2r * x0r - wk2i * x0i;
						a[j2 + 1] = wk2r * x0i + wk2i * x0r;
						x0r = x1r - x3i;
						x0i = x1i + x3r;
						a[j1] = wk1r * x0r - wk1i * x0i;
						a[j1 + 1] = wk1r * x0i + wk1i * x0r;
						x0r = x1r + x3i;
						x0i = x1i - x3r;
						a[j3] = wk3r * x0r - wk3i * x0i;
						a[j3 + 1] = wk3r * x0i + wk3i * x0r;
					}
					wk1r = w[k2 + 2];
					wk1i = w[k2 + 3];
					wk3r = wk1r - 2 * wk2r * wk1i;
					wk3i = 2 * wk2r * wk1r - wk1i;
					for (j = k + m; j < l + (k + m); j += 2) {
						j1 = j + l;
						j2 = j1 + l;
						j3 = j2 + l;
						x0r = a[j] + a[j1];
						x0i = a[j + 1] + a[j1 + 1];
						x1r = a[j] - a[j1];
						x1i = a[j + 1] - a[j1 + 1];
						x2r = a[j2] + a[j3];
						x2i = a[j2 + 1] + a[j3 + 1];
						x3r = a[j2] - a[j3];
						x3i = a[j2 + 1] - a[j3 + 1];
						a[j] = x0r + x2r;
						a[j + 1] = x0i + x2i;
						x0r -= x2r;
						x0i -= x2i;
						a[j2] = -wk2i * x0r - wk2r * x0i;
						a[j2 + 1] = -wk2i * x0i + wk2r * x0r;
						x0r = x1r - x3i;
						x0i = x1i + x3r;
						a[j1] = wk1r * x0r - wk1i * x0i;
						a[j1 + 1] = wk1r * x0i + wk1i * x0r;
						x0r = x1r + x3i;
						x0i = x1i - x3r;
						a[j3] = wk3r * x0r - wk3i * x0i;
						a[j3 + 1] = wk3r * x0i + wk3i * x0r;
					}
				}
			}

			static void cftfsub(sl_uint32 n, sl_real *a, sl_real *w)
			{
				sl_uint32 j, j1, j2, j3, l;
				sl_real x0r, x0i, x1r, x1i, x2r, x2i, x3r, x3i;

				l = 2;
				if (n > 8) {
					cft1st(n, a, w);
					l = 8;
					while ((l << 2) < n) {
						cftmdl(n, l, a, w);
						l <<= 2;
					}
				}
				if ((l << 2) == n) {
					for (j = 0; j < l; j += 2) {
						j1 = j + l;
						j2 = j1 + l;
						j3 = j2 + l;
						x0r = a[j] + a[j1];
						x0i = a[j + 1] + a[j1 + 1];
						x1r = a[j] - a[j1];
						x1i = a[j + 1] - a[j1 + 1];
						x2r = a[j2] + a[j3];
						x2i = a[j2 + 1] + a[j3 + 1];
						x3r = a[j2] - a[j3];
						x3i = a[j2 + 1] - a[j3 + 1];
						a[j] = x0r + x2r;
						a[j + 1] = x0i + x2i;
						a[j2] = x0r - x2r;
						a[j2 + 1] = x0i - x2i;
						a[j1] = x1r - x3i;
						a[j1 + 1] = x1i + x3r;
						a[j3] = x1r + x3i;
						a[j3 + 1] = x1i - x3r;
					}
				} else {
					for (j = 0; j < l; j += 2) {
						j1 = j + l;
						x0r = a[j] - a[j1];
						x0i = a[j + 1] - a[j1 + 1];
						a[j] += a[j1];
						a[j + 1] += a[j1 + 1];
						a[j1] = x0r;
						a[j1 + 1] = x0i;
					}
				}
			}

			static void cftbsub(sl_uint32 n, sl_real *a, sl_real *w)
			{
				sl_uint32 j, j1, j2, j3, l;
				sl_real x0r, x0i, x1r, x1i, x2r, x2i, x3r, x3i;

				l = 2;
				if (n > 8) {
					cft1st(n, a, w);
					l = 8;
					while ((l << 2) < n) {
						cftmdl(n, l, a, w);
						l <<= 2;
					}
				}
				if ((l << 2) == n) {
					for (j = 0; j < l; j += 2) {
						j1 = j + l;
						j2 = j1 + l;
						j3 = j2 + l;
						x0r = a[j] + a[j1];
						x0i = -a[j + 1] - a[j1 + 1];
						x1r = a[j] - a[j1];
						x1i = -a[j + 1] + a[j1 + 1];
						x2r = a[j2] + a[j3];
						x2i = a[j2 + 1] + a[j3 + 1];
						x3r = a[j2] - a[j3];
						x3i = a[j2 + 1] - a[j3 + 1];
						a[j] = x0r + x2r;
						a[j + 1] = x0i - x2i;
						a[j2] = x0r - x2r;
						a[j2 + 1] = x0i + x2i;
						a[j1] = x1r - x3i;
						a[j1 + 1] = x1i - x3r;
						a[j3] = x1r + x3i;
						a[j3 + 1] = x1i + x3r;
					}
				} else {
					for (j = 0; j < l; j += 2) {
						j1 = j + l;
						x0r = a[j] - a[j1];
						x0i = -a[j + 1] + a[j1 + 1];
						a[j] += a[j1];
						a[j + 1] = -a[j + 1] - a[j1 + 1];
						a[j1] = x0r;
						a[j1 + 1] = x0i;
					}
				}
			}

		}
	}

	using namespace priv::fft;

	FFT::FFT(sl_uint32 N)
	{
		_init(N);
	}

	FFT::~FFT()
	{
		if (m_ip) {
			delete[] m_ip;
		}
		if (m_w) {
			delete[] m_w;
		}
	}

	void FFT::_init(sl_uint32 n)
	{
		if (n >= 2) {
			sl_uint32* ip = new sl_uint32[2 + (sl_uint32)(Math::sqrt((float)n))];
			if (ip) {
				sl_real* w = new sl_real[n >> 1];
				if (w) {
					makewt(n >> 1, ip, w);
					m_count = n;
					m_ip = ip;
					m_w = w;
					return;
				}
				delete[] ip;
			}
		}
		m_count = 0;
		m_ip = sl_null;
		m_w = sl_null;
	}

	void FFT::transform(Complex* data) const
	{
		sl_uint32 n = m_count;
		if (n < 2) {
			return;
		}
		sl_uint32* ip = m_ip;
		sl_real* w = m_w;
		if (n > 2) {
			bitrv2conj(n << 1, ip + 2, &(data->real));
			cftbsub(n << 1, &(data->real), w);
		} else {
			cftfsub(n << 1, &(data->real), w);
		}
		sl_real* a = &(data->real);
		sl_real s = (sl_real)(n);
		for (sl_uint32 i = 0; i < n; i++) {
			data[i].real /= s;
			data[i].imag /= s;
		}
	}

	void FFT::inverse(Complex* data) const
	{
		sl_uint32 n = m_count;
		if (n < 2) {
			return;
		}
		sl_uint32* ip = m_ip;
		sl_real* w = m_w;
		if (n > 2) {
			bitrv2(n << 1, ip + 2, &(data->real));
			cftfsub(n << 1, &(data->real), w);
		} else {
			cftfsub(n << 1, &(data->real), w);
		}
	}

}
