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

#include "slib/crypto/gcm.h"

#include "slib/core/base.h"

namespace slib
{

	void GCM_Table::generateTable(const void* _h)
	{
		Uint128 H;
		H.setBytesBE(_h);

/*
	  In 4-bit table
		M[8]=H, M[4]=H*P, M[2]=H*P*P, M[1]=H*P*P*P, M[0]=0
	 And, in GF(2^128)
		f = 1 + P + P^2 + P^7 + P^128 => (P^128 + x) mod f = x - (1 + P + P^2 + P^7) = x + (1 + P + P^2 + P^7)
		  H*P = (H&1) ? ((H>>1)|0xE1000000000000000000000000000000) : (H>>1)
*/

		M[0].setZero();
		M[8] = H;

		sl_uint32 i = 4;
		while (i > 0) {
			sl_uint64 K = ((sl_uint64)((-((sl_int32)(H.low & 1))) & 0xe1000000)) << 32;
			H.shiftRight();
			H.high ^= K;
			M[i] = H;
			i >>= 1;
		}

		// n = a + b => M[n] = M[a] + M[b]
		i = 2;
		while (i < 16) {
			for (sl_uint32 j = 1; j < i; j++) {
				M[i + j] = M[i] ^ M[j];
			}
			i <<= 1;
		}
	}

	void GCM_Table::multiplyH(const void* _x, void* _o) const
	{
		const sl_uint8* X = (const sl_uint8*)_x;
		sl_uint8* O = (sl_uint8*)_o;

		static const sl_uint64 R[16] = {
			SLIB_UINT64(0x0000000000000000),
			SLIB_UINT64(0x1c20000000000000),
			SLIB_UINT64(0x3840000000000000),
			SLIB_UINT64(0x2460000000000000),
			SLIB_UINT64(0x7080000000000000),
			SLIB_UINT64(0x6ca0000000000000),
			SLIB_UINT64(0x48c0000000000000),
			SLIB_UINT64(0x54e0000000000000),
			SLIB_UINT64(0xe100000000000000),
			SLIB_UINT64(0xfd20000000000000),
			SLIB_UINT64(0xd940000000000000),
			SLIB_UINT64(0xc560000000000000),
			SLIB_UINT64(0x9180000000000000),
			SLIB_UINT64(0x8da0000000000000),
			SLIB_UINT64(0xa9c0000000000000),
			SLIB_UINT64(0xb5e0000000000000)
		};

		Uint128 Z;
		Z.setZero();
		for (sl_uint32 i = 15; i >= 1; i--) {
			// process low 4-bit
			{
				sl_uint32 n = X[i] & 15;
				Z ^= M[n];
				sl_uint32 r = Z.low & 15;
				Z >>= 4;
				Z.high ^= R[r];
			}
			// process high 4-bit
			{
				sl_uint32 n = (X[i] >> 4) & 15;
				Z ^= M[n];
				sl_uint32 r = Z.low & 15;
				Z >>= 4;
				Z.high ^= R[r];
			}
		}
		// process low 4-bit
		{
			sl_uint32 n = X[0] & 15;
			Z ^= M[n];
			sl_uint32 r = Z.low & 15;
			Z >>= 4;
			Z.high ^= R[r];
		}
		// process high 4-bit
		{
			sl_uint32 n = (X[0] >> 4) & 15;
			Z ^= M[n];
		}
		Z.getBytesBE(O);
	}

	void GCM_Table::multiplyData(void* _x, const void* _d, sl_size lenD) const
	{
		sl_uint8* X = (sl_uint8*)_x;
		const sl_uint8* D = (const sl_uint8*)_d;

		sl_size n = lenD >> 4;
		for (sl_size i = 0; i < n; i++) {
			for (sl_size k = 0; k < 16; k++) {
				X[k] ^= *D;
				D++;
			}
			multiplyH(X, X);
		}
		n = lenD & 15;
		if (n) {
			for (sl_size k = 0; k < n; k++) {
				X[k] ^= *D;
				D++;
			}
			multiplyH(X, X);
		}
	}

	void GCM_Table::multiplyLength(void* _x, sl_size len1, sl_size len2) const
	{
		sl_uint8* X = (sl_uint8*)_x;
		sl_uint64 v = len1 << 3;
		X[0] ^= (sl_uint8)(v >> 56);
		X[1] ^= (sl_uint8)(v >> 48);
		X[2] ^= (sl_uint8)(v >> 40);
		X[3] ^= (sl_uint8)(v >> 32);
		X[4] ^= (sl_uint8)(v >> 24);
		X[5] ^= (sl_uint8)(v >> 16);
		X[6] ^= (sl_uint8)(v >> 8);
		X[7] ^= (sl_uint8)(v);
		v = len2 << 3;
		X[8] ^= (sl_uint8)(v >> 56);
		X[9] ^= (sl_uint8)(v >> 48);
		X[10] ^= (sl_uint8)(v >> 40);
		X[11] ^= (sl_uint8)(v >> 32);
		X[12] ^= (sl_uint8)(v >> 24);
		X[13] ^= (sl_uint8)(v >> 16);
		X[14] ^= (sl_uint8)(v >> 8);
		X[15] ^= (sl_uint8)(v);
		multiplyH(X, X);
	}

	void GCM_Table::calculateGHash(const void* A, sl_size lenA, const void* C, sl_size lenC, void* _out) const
	{
		sl_uint8* O = (sl_uint8*)_out;
		Base::zeroMemory(O, 16);
		multiplyData(O, A, lenA);
		multiplyData(O, C, lenC);
		multiplyLength(O, lenA, lenC);
	}

	void GCM_Table::calculateCIV(const void* _iv, sl_size lenIV, void* _civ) const
	{
		const sl_uint8* IV = (const sl_uint8*)_iv;
		sl_uint8* CIV = (sl_uint8*)_civ;
		if (lenIV == 12) {
			for (sl_uint32 i = 0; i < 12; i++) {
				CIV[i] = IV[i];
			}
			CIV[12] = 0;
			CIV[13] = 0;
			CIV[14] = 0;
			CIV[15] = 1;
		} else {
			calculateGHash(sl_null, 0, IV, lenIV, CIV);
		}
	}


	GCM_Base::GCM_Base()
	{
		m_posAad = 0;
		m_posEnc = 0;
	}

	void GCM_Base::increaseCIV()
	{
		sl_uint8* CIV = m_civ;
		for (sl_uint32 i = 15; i >= 12; i--) {
			if (++(CIV[i]) != 0) {
				break;
			}
		}
	}

	void GCM_Base::_start()
	{
		for (sl_uint32 i = 0; i < 16; i++) {
			m_ghashx[i] = 0;
		}
		m_posAad = 0;
		m_sizeAad = 0;
		m_posEnc = 0;
		m_sizeEnc = 0;
	}

	void GCM_Base::put(const void* src, sl_size len)
	{
		sl_uint8* GHASH_X = m_ghashx;
		const sl_uint8* A = (const sl_uint8*)src;
		if (!len) {
			return;
		}
		m_sizeAad += len;
		if (m_posAad) {
			sl_uint32 k = m_posAad;
			for (;;) {
				GHASH_X[k++] ^= *(A++);
				len--;
				if (k == 16) {
					m_table.multiplyH(m_ghashx, m_ghashx);
					m_posAad = 0;
					if (!len) {
						return;
					}
					break;
				}
				if (!len) {
					m_posAad = k;
					return;
				}
			}
		}
		for (;;) {
			if (len < 16) {
				for (sl_size k = 0; k < len; k++) {
					GHASH_X[k] ^= *(A++);
				}
				m_posAad = (sl_uint32)len;
				return;
			} else {
				for (sl_uint32 k = 0; k < 16; k++) {
					GHASH_X[k] ^= *(A++);
				}
				m_table.multiplyH(GHASH_X, GHASH_X);
				len -= 16;
				if (!len) {
					return;
				}
			}
		}
	}

	sl_bool GCM_Base::_encrypt(const sl_uint8*& P, sl_uint8*& C, sl_size& len)
	{
		if (!len) {
			return sl_true;
		}
		m_sizeEnc += len;
		if (m_posAad) {
			m_table.multiplyH(m_ghashx, m_ghashx);
			m_posAad = 0;
		}
		if (!m_posEnc) {
			return sl_false;
		}
		sl_uint32 k = m_posEnc;
		for (;;) {
			sl_uint8 c = *(P++) ^ m_gctr[k];
			m_ghashx[k] ^= c;
			*(C++) = c;
			k++;
			len--;
			if (k == 16) {
				m_table.multiplyH(m_ghashx, m_ghashx);
				m_posEnc = 0;
				if (!len) {
					return sl_true;
				}
				break;
			}
			if (!len) {
				m_posEnc = k;
				return sl_true;
			}
		}
		return sl_false;
	}

	sl_bool GCM_Base::_decrypt(const sl_uint8*& C, sl_uint8*& P, sl_size& len)
	{
		if (!len) {
			return sl_true;
		}
		m_sizeEnc += len;
		if (m_posAad) {
			m_table.multiplyH(m_ghashx, m_ghashx);
			m_posAad = 0;
		}
		if (!m_posEnc) {
			return sl_false;
		}
		sl_uint32 k = m_posEnc;
		for (;;) {
			sl_uint8 c = *(C++);
			m_ghashx[k] ^= c;
			*(P++) = c ^ m_gctr[k];
			k++;
			len--;
			if (k == 16) {
				m_table.multiplyH(m_ghashx, m_ghashx);
				m_posEnc = 0;
				if (!len) {
					return sl_true;
				}
				break;
			}
			if (!len) {
				m_posEnc = k;
				return sl_true;
			}
		}
		return sl_false;
	}

	sl_bool GCM_Base::_finish(sl_size lenTag)
	{
		if (lenTag < 4 || lenTag > 16) {
			return sl_false;
		}
		if (m_posAad) {
			m_table.multiplyH(m_ghashx, m_ghashx);
			m_posAad = 0;
		}
		if (m_posEnc) {
			m_table.multiplyH(m_ghashx, m_ghashx);
			m_posEnc = 0;
		}
		m_table.multiplyLength(m_ghashx, m_sizeAad, m_sizeEnc);
		return sl_true;
	}

	sl_bool GCM_Base::finish(void* _tag, sl_size lenTag)
	{
		if (!(_finish(lenTag))) {
			return sl_false;
		}
		sl_uint8* tag = (sl_uint8*)_tag;
		for (sl_size i = 0; i < lenTag; i++) {
			tag[i] = m_ghashx[i] ^ m_gctr0[i];
		}
		return sl_true;
	}

	sl_bool GCM_Base::finishAndCheckTag(const void* _tag, sl_size lenTag)
	{
		if (!(_finish(lenTag))) {
			return sl_false;
		}
		const sl_uint8* tag = (const sl_uint8*)_tag;
		for (sl_size i = 0; i < lenTag; i++) {
			if (tag[i] != (m_ghashx[i] ^ m_gctr0[i])) {
				return sl_false;
			}
		}
		return sl_true;
	}

}
