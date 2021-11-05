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

#include "slib/crypto/sha3.h"

#include "slib/core/mio.h"
#include "slib/core/math.h"

namespace slib
{

	namespace priv
	{
		namespace sha3
		{

			// The round constants, pre-interleaved
			static const BitInterleaved64 g_roundConstants[24] = {
				{ 0x00000001, 0x00000000 },{ 0x00000000, 0x00000089 },
				{ 0x00000000, 0x8000008b },{ 0x00000000, 0x80008080 },
				{ 0x00000001, 0x0000008b },{ 0x00000001, 0x00008000 },
				{ 0x00000001, 0x80008088 },{ 0x00000001, 0x80000082 },
				{ 0x00000000, 0x0000000b },{ 0x00000000, 0x0000000a },
				{ 0x00000001, 0x00008082 },{ 0x00000000, 0x00008003 },
				{ 0x00000001, 0x0000808b },{ 0x00000001, 0x8000000b },
				{ 0x00000001, 0x8000008a },{ 0x00000001, 0x80000081 },
				{ 0x00000000, 0x80000081 },{ 0x00000000, 0x80000008 },
				{ 0x00000000, 0x00000083 },{ 0x00000000, 0x80008003 },
				{ 0x00000001, 0x80008088 },{ 0x00000000, 0x80000088 },
				{ 0x00000001, 0x00008000 },{ 0x00000000, 0x80008082 }
			};

			static const sl_uint8 g_rotationConstants[5][5] = {
				{ 0,  1, 62, 28, 27, },
				{ 36, 44,  6, 55, 20, },
				{ 3, 10, 43, 25, 39, },
				{ 41, 45, 15, 21,  8, },
				{ 18,  2, 61, 56, 14, }
			};

			// Integers [-1,20] mod 5. To avoid a divmod. Indices are constants; not data-dependant.
			static const sl_uint8 g_tableMod5[] = {
				4,
				0,
				1, 2, 3, 4, 0, 1, 2, 3, 4, 0,
				1, 2, 3, 4, 0, 1, 2, 3, 4, 0
			};
			#define MOD5(x) (g_tableMod5[(x) + 1])

			// Convert AaBbCcDd -> ABCDabcd
			SLIB_INLINE static sl_uint32 ShuffleOut(sl_uint32 x)
			{
				sl_uint32 t;
				t = (x ^ (x >> 1)) & 0x22222222; x = x ^ t ^ (t << 1);
				t = (x ^ (x >> 2)) & 0x0c0c0c0c; x = x ^ t ^ (t << 2);
				t = (x ^ (x >> 4)) & 0x00f000f0; x = x ^ t ^ (t << 4);
				t = (x ^ (x >> 8)) & 0x0000ff00; x = x ^ t ^ (t << 8);
				return x;
			}

			// Convert ABCDabcd -> AaBbCcDd
			SLIB_INLINE static sl_uint32 ShuffleIn(sl_uint32 x)
			{
				sl_uint32 t;
				t = (x ^ (x >> 8)) & 0x0000ff00; x = x ^ t ^ (t << 8);
				t = (x ^ (x >> 4)) & 0x00f000f0; x = x ^ t ^ (t << 4);
				t = (x ^ (x >> 2)) & 0x0c0c0c0c; x = x ^ t ^ (t << 2);
				t = (x ^ (x >> 1)) & 0x22222222; x = x ^ t ^ (t << 1);
				return x;
			}

			SLIB_INLINE static void ReadBi(BitInterleaved64* _out, const void* _data)
			{
				sl_uint8* data = (sl_uint8*)_data;
				sl_uint32 lo = MIO::readUint32LE(data);
				sl_uint32 hi = MIO::readUint32LE(data + 4);
				lo = ShuffleOut(lo);
				hi = ShuffleOut(hi);
				_out->odd = (lo & 0x0000ffff) | (hi << 16);
				_out->even = (lo >> 16) | (hi & 0xffff0000);
			}

			SLIB_INLINE static void WriteBi(const BitInterleaved64* _in, void* _data)
			{
				sl_uint8* data = (sl_uint8*)_data;
				sl_uint32 lo = (_in->odd & 0x0000ffff) | (_in->even << 16);
				sl_uint32 hi = (_in->odd >> 16) | (_in->even & 0xffff0000);
				lo = ShuffleIn(lo);
				hi = ShuffleIn(hi);
				MIO::writeUint32LE(data, lo);
				MIO::writeUint32LE(data + 4, hi);
			}

			SLIB_INLINE static void RotateBi1(BitInterleaved64* _out, const BitInterleaved64* _in)
			{
				// in bit-interleaved representation, a rotation of 1 is a swap plus a single rotation of the odd word
				_out->odd = (_in->even << 1) | (_in->even >> 31);
				_out->even = _in->odd;
			}

			SLIB_INLINE static void RotateBiN(BitInterleaved64* _out, const BitInterleaved64* _in, sl_uint8 rotation)
			{
				sl_uint32 half = rotation >> 1;
				if (rotation & 1) {
					_out->odd = Math::rotateLeft(_in->even, half + 1);
					_out->even = Math::rotateLeft(_in->odd, half);
				} else {
					_out->even = Math::rotateLeft(_in->even, half);
					_out->odd = Math::rotateLeft(_in->odd, half);
				}
			}


			SHA3Base::SHA3Base() noexcept
			{
				rdata_len = 0;
			}

			SHA3Base::~SHA3Base()
			{
			}

			void SHA3Base::start() noexcept
			{
				Base::zeroMemory(A, sizeof(A));
				rdata_len = 0;
			}

			void SHA3Base::update(const void* _input, sl_size sizeInput) noexcept
			{
				const sl_uint8* input = (const sl_uint8*)_input;
				if (!sizeInput) {
					return;
				}
				if (rdata_len > 0) {
					sl_uint32 n = rate - rdata_len;
					if (sizeInput < n) {
						Base::copyMemory(rdata + rdata_len, input, sizeInput);
						rdata_len += (sl_uint32)sizeInput;
						return;
					} else {
						Base::copyMemory(rdata + rdata_len, input, n);
						_updateBlock(rdata);
						rdata_len = 0;
						sizeInput -= n;
						input += n;
						if (!sizeInput) {
							return;
						}
					}
				}
				while (sizeInput >= rate) {
					_updateBlock(input);
					sizeInput -= rate;
					input += rate;
				}
				if (sizeInput) {
					Base::copyMemory(rdata, input, sizeInput);
					rdata_len = (sl_uint32)sizeInput;
				}
			}

			void SHA3Base::finish(void* _output) noexcept
			{
				// Append SHA-3 Domain Suffix (01) and padding (10*1)
				{
					sl_uint32 rate_1 = rate - 1;
					if (rdata_len < rate_1) {
						rdata[rdata_len] = (sl_uint8)0x06;
						for (sl_uint32 i = rdata_len + 1; i < rate_1; i++) {
							rdata[i] = 0;
						}
						rdata[rate_1] = (sl_uint8)0x80;
					} else {
						rdata[rdata_len] = (sl_uint8)0x86;
					}
					_updateBlock(rdata);
					rdata_len = 0;
				}
				// Extract
				{
					sl_uint8* output = (sl_uint8*)_output;
					sl_uint32 n = nhash;
					sl_uint32 lanes = (n + 7) >> 3;
					for (sl_uint32 x = 0, y = 0, i = 0; i < lanes; i++) {
						if (n >= 8) {
							WriteBi(&A[x][y], output);
							output += 8;
							n -= 8;
						} else {
							sl_uint8 buf[8];
							WriteBi(&A[x][y], buf);
							Base::copyMemory(output, buf, n);
							return;
						}
						x++;
						if (x == 5) {
							y++;
							x = 0;
						}
					}
				}
			}

			void SHA3Base::_updateBlock(const sl_uint8* data) noexcept
			{
				// absorb
				{
					sl_uint32 lanes = rate >> 3;
					for (sl_uint32 x = 0, y = 0, i = 0; i < lanes; i++) {
						BitInterleaved64 bi;
						ReadBi(&bi, data);
						A[x][y].odd ^= bi.odd;
						A[x][y].even ^= bi.even;
						data += 8;
						x++;
						if (x == 5) {
							y++;
							x = 0;
						}
					}
				}
				// permute
				for (sl_uint32 r = 0; r < 24; r++)
				{
					// theta
					{
						sl_uint32 x;
						BitInterleaved64 C[5], D[5];
						for (x = 0; x < 5; x++) {
							C[x].odd = A[x][0].odd ^ A[x][1].odd ^ A[x][2].odd ^ A[x][3].odd ^ A[x][4].odd;
							C[x].even = A[x][0].even ^ A[x][1].even ^ A[x][2].even ^ A[x][3].even ^ A[x][4].even;
						}
						for (x = 0; x < 5; x++) {
							BitInterleaved64 r;
							RotateBi1(&r, &C[MOD5(x + 1)]);
							D[x].odd = C[MOD5(x - 1)].odd ^ r.odd;
							D[x].even = C[MOD5(x - 1)].even ^ r.even;
							for (int y = 0; y < 5; y++) {
								A[x][y].odd ^= D[x].odd;
								A[x][y].even ^= D[x].even;
							}
						}
					}
					// rho pi chi
					{
						sl_uint32 x, y;
						BitInterleaved64 B[5][5] = { { { 0 } } };
						for (x = 0; x < 5; x++) {
							for (y = 0; y < 5; y++) {
								RotateBiN(&B[y][MOD5(2 * x + 3 * y)], &A[x][y], g_rotationConstants[y][x]);
							}
						}
						for (x = 0; x < 5; x++) {
							sl_uint32 x1 = MOD5(x + 1);
							sl_uint32 x2 = MOD5(x + 2);
							for (y = 0; y < 5; y++) {
								A[x][y].odd = B[x][y].odd ^ ((~B[x1][y].odd) & B[x2][y].odd);
								A[x][y].even = B[x][y].even ^ ((~B[x1][y].even) & B[x2][y].even);
							}
						}
					}
					// iota
					A[0][0].odd ^= g_roundConstants[r].odd;
					A[0][0].even ^= g_roundConstants[r].even;
				}
			}

		}
	}

	SHA3_224::SHA3_224() noexcept
	{
		rate = BlockSize;
		nhash = HashSize;
	}

	SHA3_224::~SHA3_224()
	{
	}


	SHA3_256::SHA3_256() noexcept
	{
		rate = BlockSize;
		nhash = HashSize;
	}

	SHA3_256::~SHA3_256()
	{
	}


	SHA3_384::SHA3_384() noexcept
	{
		rate = BlockSize;
		nhash = HashSize;
	}

	SHA3_384::~SHA3_384()
	{
	}


	SHA3_512::SHA3_512() noexcept
	{
		rate = BlockSize;
		nhash = HashSize;
	}

	SHA3_512::~SHA3_512()
	{
	}

}
