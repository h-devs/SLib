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

#include "slib/crypto/rc2.h"

namespace slib
{

	namespace priv
	{
		namespace rc2
		{
			
			static const sl_uint8 g_tableKey[256] = {
				0xd9, 0x78, 0xf9, 0xc4, 0x19, 0xdd, 0xb5, 0xed, 0x28, 0xe9, 0xfd, 0x79,
				0x4a, 0xa0, 0xd8, 0x9d, 0xc6, 0x7e, 0x37, 0x83, 0x2b, 0x76, 0x53, 0x8e,
				0x62, 0x4c, 0x64, 0x88, 0x44, 0x8b, 0xfb, 0xa2, 0x17, 0x9a, 0x59, 0xf5,
				0x87, 0xb3, 0x4f, 0x13, 0x61, 0x45, 0x6d, 0x8d, 0x09, 0x81, 0x7d, 0x32,
				0xbd, 0x8f, 0x40, 0xeb, 0x86, 0xb7, 0x7b, 0x0b, 0xf0, 0x95, 0x21, 0x22,
				0x5c, 0x6b, 0x4e, 0x82, 0x54, 0xd6, 0x65, 0x93, 0xce, 0x60, 0xb2, 0x1c,
				0x73, 0x56, 0xc0, 0x14, 0xa7, 0x8c, 0xf1, 0xdc, 0x12, 0x75, 0xca, 0x1f,
				0x3b, 0xbe, 0xe4, 0xd1, 0x42, 0x3d, 0xd4, 0x30, 0xa3, 0x3c, 0xb6, 0x26,
				0x6f, 0xbf, 0x0e, 0xda, 0x46, 0x69, 0x07, 0x57, 0x27, 0xf2, 0x1d, 0x9b,
				0xbc, 0x94, 0x43, 0x03, 0xf8, 0x11, 0xc7, 0xf6, 0x90, 0xef, 0x3e, 0xe7,
				0x06, 0xc3, 0xd5, 0x2f, 0xc8, 0x66, 0x1e, 0xd7, 0x08, 0xe8, 0xea, 0xde,
				0x80, 0x52, 0xee, 0xf7, 0x84, 0xaa, 0x72, 0xac, 0x35, 0x4d, 0x6a, 0x2a,
				0x96, 0x1a, 0xd2, 0x71, 0x5a, 0x15, 0x49, 0x74, 0x4b, 0x9f, 0xd0, 0x5e,
				0x04, 0x18, 0xa4, 0xec, 0xc2, 0xe0, 0x41, 0x6e, 0x0f, 0x51, 0xcb, 0xcc,
				0x24, 0x91, 0xaf, 0x50, 0xa1, 0xf4, 0x70, 0x39, 0x99, 0x7c, 0x3a, 0x85,
				0x23, 0xb8, 0xb4, 0x7a, 0xfc, 0x02, 0x36, 0x5b, 0x25, 0x55, 0x97, 0x31,
				0x2d, 0x5d, 0xfa, 0x98, 0xe3, 0x8a, 0x92, 0xae, 0x05, 0xdf, 0x29, 0x10,
				0x67, 0x6c, 0xba, 0xc9, 0xd3, 0x00, 0xe6, 0xcf, 0xe1, 0x9e, 0xa8, 0x2c,
				0x63, 0x16, 0x01, 0x3f, 0x58, 0xe2, 0x89, 0xa9, 0x0d, 0x38, 0x34, 0x1b,
				0xab, 0x33, 0xff, 0xb0, 0xbb, 0x48, 0x0c, 0x5f, 0xb9, 0xb1, 0xcd, 0x2e,
				0xc5, 0xf3, 0xdb, 0x47, 0xe5, 0xa5, 0x9c, 0x77, 0x0a, 0xa6, 0x20, 0x68,
				0xfe, 0x7f, 0xc1, 0xad,
			};

			static void SetKey(sl_uint16 key[64], const sl_uint8* input, sl_uint32 sizeKey, sl_uint32 nBits)
			{
				if (sizeKey > 128) {
					sizeKey = 128;
				}
				if (!nBits) {
					nBits = 1024;
				}
				if (nBits > 1024) {
					nBits = 1024;
				}
				sl_uint8* k = (sl_uint8*)key;
				*k = 0;
				{
					for (sl_uint32 i = 0; i < sizeKey; i++) {
						k[i] = input[i];
					}
				}
				// expand table
				{
					sl_uint8 d = sizeKey ? k[sizeKey - 1] : 0;
					sl_uint32 j = 0;
					for (sl_uint32 i = sizeKey; i < 128; i++) {
						d = g_tableKey[(k[j] + d) & 0xff];
						k[i] = d;
						j++;
					}
				}
				// reduction
				{
					sl_uint32 j = (nBits + 7) >> 3;
					sl_uint32 i = 128 - j;
					sl_uint32 c = 0xff >> ((-(sl_int32)nBits) & 0x07);
					sl_uint8 d = g_tableKey[k[i] & c];
					k[i] = d;
					while (i) {
						i--;
						d = g_tableKey[k[i + j] ^ d];
						k[i] = d;
					}
				}
				// Final
				{
					sl_uint32 j = 0;
					for (sl_uint32 i = 0; i < 64; i++) {
						key[i] = SLIB_MAKE_WORD(k[j + 1], k[j]);
						j += 2;
					}
				}
			}

		}
	}
	
	using namespace priv::rc2;

	RC2::RC2()
	{
	}

	RC2::~RC2()
	{
	}
	
	void RC2::setKey(const void* key, sl_uint32 lenKey, sl_int32 nBits)
	{
		SetKey(m_key, (sl_uint8*)key, lenKey, nBits < 0 ? (lenKey << 3) : nBits);
	}

	void RC2::encrypt(sl_uint32& d0, sl_uint32& d1) const
	{
		sl_uint16 x0 = (sl_uint16)d0;
		sl_uint16 x1 = (sl_uint16)(d0 >> 16);
		sl_uint16 x2 = (sl_uint16)d1;
		sl_uint16 x3 = (sl_uint16)(d1 >> 16);

		sl_uint32 n = 3;
		sl_uint32 i = 5;
		const sl_uint16* p0 = m_key;
		const sl_uint16* p1 = p0;
		for (;;) {
			sl_uint16 t = x0 + (x1 & ~x3) + (x2 & x3) + *(p0++);
			x0 = (t << 1) | (t >> 15);
			t = x1 + (x2 & ~x0) + (x3 & x0) + *(p0++);
			x1 = (t << 2) | (t >> 14);
			t = x2 + (x3 & ~x1) + (x0 & x1) + *(p0++);
			x2 = (t << 3) | (t >> 13);
			t = x3 + (x0 & ~x2) + (x1 & x2) + *(p0++);
			x3 = (t << 5) | (t >> 11);
			if (!(--i)) {
				if (!(--n)) {
					break;
				}
				i = (n == 2) ? 6 : 5;
				x0 += p1[x3 & 0x3f];
				x1 += p1[x0 & 0x3f];
				x2 += p1[x1 & 0x3f];
				x3 += p1[x2 & 0x3f];
			}
		}
		d0 = SLIB_MAKE_DWORD2(x1, x0);
		d1 = SLIB_MAKE_DWORD2(x3, x2);
	}

	void RC2::decrypt(sl_uint32& d0, sl_uint32& d1) const
	{
		sl_uint16 x0 = (sl_uint16)d0;
		sl_uint16 x1 = (sl_uint16)(d0 >> 16);
		sl_uint16 x2 = (sl_uint16)d1;
		sl_uint16 x3 = (sl_uint16)(d1 >> 16);

		sl_uint32 n = 3;
		sl_uint32 i = 5;
		const sl_uint16* p0 = m_key + 63;
		const sl_uint16* p1 = m_key;
		for (;;) {
			sl_uint16 t = (x3 << 11) | (x3 >> 5);
			x3 = t - (x0 & ~x2) - (x1 & x2) - *(p0--);
			t = (x2 << 13) | (x2 >> 3);
			x2 = t - (x3 & ~x1) - (x0 & x1) - *(p0--);
			t = (x1 << 14) | (x1 >> 2);
			x1 = t - (x2 & ~x0) - (x3 & x0) - *(p0--);
			t = (x0 << 15) | (x0 >> 1);
			x0 = t - (x1 & ~x3) - (x2 & x3) - *(p0--);
			if (!(--i)) {
				if (!(--n)) {
					break;
				}
				i = (n == 2) ? 6 : 5;
				x3 = x3 - p1[x2 & 0x3f];
				x2 = x2 - p1[x1 & 0x3f];
				x1 = x1 - p1[x0 & 0x3f];
				x0 = x0 - p1[x3 & 0x3f];
			}
		}
		d0 = SLIB_MAKE_DWORD2(x1, x0);
		d1 = SLIB_MAKE_DWORD2(x3, x2);
	}

	void RC2::encryptBlock(const void* _src, void *_dst) const
	{
		const sl_uint8* IN = (const sl_uint8*)_src;
		sl_uint8* OUT = (sl_uint8*)_dst;

		sl_uint32 d0 = MIO::readUint32LE(IN);
		sl_uint32 d1 = MIO::readUint32LE(IN + 4);
		
		encrypt(d0, d1);

		MIO::writeUint32LE(OUT, d0);
		MIO::writeUint32LE(OUT + 4, d1);
	}

	void RC2::decryptBlock(const void* _src, void *_dst) const
	{
		const sl_uint8* IN = (const sl_uint8*)_src;
		sl_uint8* OUT = (sl_uint8*)_dst;
		
		sl_uint32 d0 = MIO::readUint32LE(IN);
		sl_uint32 d1 = MIO::readUint32LE(IN + 4);
		
		decrypt(d0, d1);
		
		MIO::writeUint32LE(OUT, d0);
		MIO::writeUint32LE(OUT + 4, d1);
	}

}
