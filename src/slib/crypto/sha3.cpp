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

#include "slib/math/math.h"
#include "slib/core/mio.h"
#include "slib/core/assert.h"

// Referenced to OpenSSL implementation

#if defined(__i386) || defined(__i386__) || defined(_M_IX86)
#define KECCAK_COMPLEMENTING_TRANSFORM
#endif

#if defined(_M_AMD64) || defined(__x86_64__) || defined(__aarch64__) || defined(__mips64) || defined(__ia64) || (defined(__VMS) && !defined(__vax))
# define BIT_INTERLEAVE (0)
# define NOT_BIT_INTERLEAVE
#else
# define BIT_INTERLEAVE (sizeof(void*) < 8)
#endif

#define ROL32(a, offset) (((a) << (offset)) | ((a) >> ((32 - (offset)) & 31)))

namespace slib
{

	namespace {

		static sl_uint64 ROL64(sl_uint64 val, int offset)
		{
			if (offset) {
#ifndef NOT_BIT_INTERLEAVE
				if (BIT_INTERLEAVE) {
					sl_uint32 hi = (sl_uint32)(val >> 32), lo = (sl_uint32)val;
					if (offset & 1) {
						sl_uint32 tmp = hi;
						offset >>= 1;
						hi = ROL32(lo, offset);
						lo = ROL32(tmp, offset + 1);
					} else {
						offset >>= 1;
						lo = ROL32(lo, offset);
						hi = ROL32(hi, offset);
					}
					return ((sl_uint64)hi << 32) | lo;
				}
#endif
				return (val << offset) | (val >> (64 - offset));
			} else {
				return val;
			}
		}

#ifdef NOT_BIT_INTERLEAVE
# define BitInterleave(X) (X)
#else
		static sl_uint64 BitInterleave(sl_uint64 Ai)
		{
			if (BIT_INTERLEAVE) {
				sl_uint32 hi = (sl_uint32)(Ai >> 32), lo = (sl_uint32)Ai;
				sl_uint32 t0, t1;

				t0 = lo & 0x55555555;
				t0 |= t0 >> 1;  t0 &= 0x33333333;
				t0 |= t0 >> 2;  t0 &= 0x0f0f0f0f;
				t0 |= t0 >> 4;  t0 &= 0x00ff00ff;
				t0 |= t0 >> 8;  t0 &= 0x0000ffff;

				t1 = hi & 0x55555555;
				t1 |= t1 >> 1;  t1 &= 0x33333333;
				t1 |= t1 >> 2;  t1 &= 0x0f0f0f0f;
				t1 |= t1 >> 4;  t1 &= 0x00ff00ff;
				t1 |= t1 >> 8;  t1 <<= 16;

				lo &= 0xaaaaaaaa;
				lo |= lo << 1;  lo &= 0xcccccccc;
				lo |= lo << 2;  lo &= 0xf0f0f0f0;
				lo |= lo << 4;  lo &= 0xff00ff00;
				lo |= lo << 8;  lo >>= 16;

				hi &= 0xaaaaaaaa;
				hi |= hi << 1;  hi &= 0xcccccccc;
				hi |= hi << 2;  hi &= 0xf0f0f0f0;
				hi |= hi << 4;  hi &= 0xff00ff00;
				hi |= hi << 8;  hi &= 0xffff0000;

				Ai = ((sl_uint64)(hi | lo) << 32) | (t1 | t0);
			}
			return Ai;
		}
#endif

#ifdef NOT_BIT_INTERLEAVE
# define BitDeinterleave(X) (X)
#else
		static sl_uint64 BitDeinterleave(sl_uint64 Ai)
		{
			if (BIT_INTERLEAVE) {
				sl_uint32 hi = (sl_uint32)(Ai >> 32), lo = (sl_uint32)Ai;
				sl_uint32 t0, t1;

				t0 = lo & 0x0000ffff;
				t0 |= t0 << 8;  t0 &= 0x00ff00ff;
				t0 |= t0 << 4;  t0 &= 0x0f0f0f0f;
				t0 |= t0 << 2;  t0 &= 0x33333333;
				t0 |= t0 << 1;  t0 &= 0x55555555;

				t1 = hi << 16;
				t1 |= t1 >> 8;  t1 &= 0xff00ff00;
				t1 |= t1 >> 4;  t1 &= 0xf0f0f0f0;
				t1 |= t1 >> 2;  t1 &= 0xcccccccc;
				t1 |= t1 >> 1;  t1 &= 0xaaaaaaaa;

				lo >>= 16;
				lo |= lo << 8;  lo &= 0x00ff00ff;
				lo |= lo << 4;  lo &= 0x0f0f0f0f;
				lo |= lo << 2;  lo &= 0x33333333;
				lo |= lo << 1;  lo &= 0x55555555;

				hi &= 0xffff0000;
				hi |= hi >> 8;  hi &= 0xff00ff00;
				hi |= hi >> 4;  hi &= 0xf0f0f0f0;
				hi |= hi >> 2;  hi &= 0xcccccccc;
				hi |= hi >> 1;  hi &= 0xaaaaaaaa;

				Ai = ((sl_uint64)(hi | lo) << 32) | (t1 | t0);
			}
			return Ai;
		}
#endif

		static const sl_uint8 g_rhotates[5][5] = {
			{ 0,  1, 62, 28, 27 },
			{ 36, 44,  6, 55, 20 },
			{ 3, 10, 43, 25, 39 },
			{ 41, 45, 15, 21,  8 },
			{ 18,  2, 61, 56, 14 }
		};

		static const sl_uint64 g_iotas[] = {
			BIT_INTERLEAVE ? SLIB_UINT64(0x0000000000000001) : SLIB_UINT64(0x0000000000000001),
			BIT_INTERLEAVE ? SLIB_UINT64(0x0000008900000000) : SLIB_UINT64(0x0000000000008082),
			BIT_INTERLEAVE ? SLIB_UINT64(0x8000008b00000000) : SLIB_UINT64(0x800000000000808a),
			BIT_INTERLEAVE ? SLIB_UINT64(0x8000808000000000) : SLIB_UINT64(0x8000000080008000),
			BIT_INTERLEAVE ? SLIB_UINT64(0x0000008b00000001) : SLIB_UINT64(0x000000000000808b),
			BIT_INTERLEAVE ? SLIB_UINT64(0x0000800000000001) : SLIB_UINT64(0x0000000080000001),
			BIT_INTERLEAVE ? SLIB_UINT64(0x8000808800000001) : SLIB_UINT64(0x8000000080008081),
			BIT_INTERLEAVE ? SLIB_UINT64(0x8000008200000001) : SLIB_UINT64(0x8000000000008009),
			BIT_INTERLEAVE ? SLIB_UINT64(0x0000000b00000000) : SLIB_UINT64(0x000000000000008a),
			BIT_INTERLEAVE ? SLIB_UINT64(0x0000000a00000000) : SLIB_UINT64(0x0000000000000088),
			BIT_INTERLEAVE ? SLIB_UINT64(0x0000808200000001) : SLIB_UINT64(0x0000000080008009),
			BIT_INTERLEAVE ? SLIB_UINT64(0x0000800300000000) : SLIB_UINT64(0x000000008000000a),
			BIT_INTERLEAVE ? SLIB_UINT64(0x0000808b00000001) : SLIB_UINT64(0x000000008000808b),
			BIT_INTERLEAVE ? SLIB_UINT64(0x8000000b00000001) : SLIB_UINT64(0x800000000000008b),
			BIT_INTERLEAVE ? SLIB_UINT64(0x8000008a00000001) : SLIB_UINT64(0x8000000000008089),
			BIT_INTERLEAVE ? SLIB_UINT64(0x8000008100000001) : SLIB_UINT64(0x8000000000008003),
			BIT_INTERLEAVE ? SLIB_UINT64(0x8000008100000000) : SLIB_UINT64(0x8000000000008002),
			BIT_INTERLEAVE ? SLIB_UINT64(0x8000000800000000) : SLIB_UINT64(0x8000000000000080),
			BIT_INTERLEAVE ? SLIB_UINT64(0x0000008300000000) : SLIB_UINT64(0x000000000000800a),
			BIT_INTERLEAVE ? SLIB_UINT64(0x8000800300000000) : SLIB_UINT64(0x800000008000000a),
			BIT_INTERLEAVE ? SLIB_UINT64(0x8000808800000001) : SLIB_UINT64(0x8000000080008081),
			BIT_INTERLEAVE ? SLIB_UINT64(0x8000008800000000) : SLIB_UINT64(0x8000000000008080),
			BIT_INTERLEAVE ? SLIB_UINT64(0x0000800000000001) : SLIB_UINT64(0x0000000080000001),
			BIT_INTERLEAVE ? SLIB_UINT64(0x8000808200000000) : SLIB_UINT64(0x8000000080008008)
		};

		static void Round(sl_uint64 R[5][5], sl_uint64 A[5][5], sl_uint32 i)
		{
			sl_uint64 C[5], D[5];

			SLIB_ASSERT(i < (sizeof(g_iotas) / sizeof(g_iotas[0])));

			C[0] = A[0][0] ^ A[1][0] ^ A[2][0] ^ A[3][0] ^ A[4][0];
			C[1] = A[0][1] ^ A[1][1] ^ A[2][1] ^ A[3][1] ^ A[4][1];
			C[2] = A[0][2] ^ A[1][2] ^ A[2][2] ^ A[3][2] ^ A[4][2];
			C[3] = A[0][3] ^ A[1][3] ^ A[2][3] ^ A[3][3] ^ A[4][3];
			C[4] = A[0][4] ^ A[1][4] ^ A[2][4] ^ A[3][4] ^ A[4][4];

			D[0] = ROL64(C[1], 1) ^ C[4];
			D[1] = ROL64(C[2], 1) ^ C[0];
			D[2] = ROL64(C[3], 1) ^ C[1];
			D[3] = ROL64(C[4], 1) ^ C[2];
			D[4] = ROL64(C[0], 1) ^ C[3];

			C[0] = A[0][0] ^ D[0]; /* rotate by 0 */
			C[1] = ROL64(A[1][1] ^ D[1], g_rhotates[1][1]);
			C[2] = ROL64(A[2][2] ^ D[2], g_rhotates[2][2]);
			C[3] = ROL64(A[3][3] ^ D[3], g_rhotates[3][3]);
			C[4] = ROL64(A[4][4] ^ D[4], g_rhotates[4][4]);

#ifdef KECCAK_COMPLEMENTING_TRANSFORM
			R[0][0] = C[0] ^ (C[1] | C[2]) ^ g_iotas[i];
			R[0][1] = C[1] ^ (~C[2] | C[3]);
			R[0][2] = C[2] ^ (C[3] & C[4]);
			R[0][3] = C[3] ^ (C[4] | C[0]);
			R[0][4] = C[4] ^ (C[0] & C[1]);
#else
			R[0][0] = C[0] ^ (~C[1] & C[2]) ^ g_iotas[i];
			R[0][1] = C[1] ^ (~C[2] & C[3]);
			R[0][2] = C[2] ^ (~C[3] & C[4]);
			R[0][3] = C[3] ^ (~C[4] & C[0]);
			R[0][4] = C[4] ^ (~C[0] & C[1]);
#endif

			C[0] = ROL64(A[0][3] ^ D[3], g_rhotates[0][3]);
			C[1] = ROL64(A[1][4] ^ D[4], g_rhotates[1][4]);
			C[2] = ROL64(A[2][0] ^ D[0], g_rhotates[2][0]);
			C[3] = ROL64(A[3][1] ^ D[1], g_rhotates[3][1]);
			C[4] = ROL64(A[4][2] ^ D[2], g_rhotates[4][2]);

#ifdef KECCAK_COMPLEMENTING_TRANSFORM
			R[1][0] = C[0] ^ (C[1] | C[2]);
			R[1][1] = C[1] ^ (C[2] & C[3]);
			R[1][2] = C[2] ^ (C[3] | ~C[4]);
			R[1][3] = C[3] ^ (C[4] | C[0]);
			R[1][4] = C[4] ^ (C[0] & C[1]);
#else
			R[1][0] = C[0] ^ (~C[1] & C[2]);
			R[1][1] = C[1] ^ (~C[2] & C[3]);
			R[1][2] = C[2] ^ (~C[3] & C[4]);
			R[1][3] = C[3] ^ (~C[4] & C[0]);
			R[1][4] = C[4] ^ (~C[0] & C[1]);
#endif

			C[0] = ROL64(A[0][1] ^ D[1], g_rhotates[0][1]);
			C[1] = ROL64(A[1][2] ^ D[2], g_rhotates[1][2]);
			C[2] = ROL64(A[2][3] ^ D[3], g_rhotates[2][3]);
			C[3] = ROL64(A[3][4] ^ D[4], g_rhotates[3][4]);
			C[4] = ROL64(A[4][0] ^ D[0], g_rhotates[4][0]);

#ifdef KECCAK_COMPLEMENTING_TRANSFORM
			R[2][0] = C[0] ^ (C[1] | C[2]);
			R[2][1] = C[1] ^ (C[2] & C[3]);
			R[2][2] = C[2] ^ (~C[3] & C[4]);
			R[2][3] = ~C[3] ^ (C[4] | C[0]);
			R[2][4] = C[4] ^ (C[0] & C[1]);
#else
			R[2][0] = C[0] ^ (~C[1] & C[2]);
			R[2][1] = C[1] ^ (~C[2] & C[3]);
			R[2][2] = C[2] ^ (~C[3] & C[4]);
			R[2][3] = C[3] ^ (~C[4] & C[0]);
			R[2][4] = C[4] ^ (~C[0] & C[1]);
#endif

			C[0] = ROL64(A[0][4] ^ D[4], g_rhotates[0][4]);
			C[1] = ROL64(A[1][0] ^ D[0], g_rhotates[1][0]);
			C[2] = ROL64(A[2][1] ^ D[1], g_rhotates[2][1]);
			C[3] = ROL64(A[3][2] ^ D[2], g_rhotates[3][2]);
			C[4] = ROL64(A[4][3] ^ D[3], g_rhotates[4][3]);

#ifdef KECCAK_COMPLEMENTING_TRANSFORM
			R[3][0] = C[0] ^ (C[1] & C[2]);
			R[3][1] = C[1] ^ (C[2] | C[3]);
			R[3][2] = C[2] ^ (~C[3] | C[4]);
			R[3][3] = ~C[3] ^ (C[4] & C[0]);
			R[3][4] = C[4] ^ (C[0] | C[1]);
#else
			R[3][0] = C[0] ^ (~C[1] & C[2]);
			R[3][1] = C[1] ^ (~C[2] & C[3]);
			R[3][2] = C[2] ^ (~C[3] & C[4]);
			R[3][3] = C[3] ^ (~C[4] & C[0]);
			R[3][4] = C[4] ^ (~C[0] & C[1]);
#endif

			C[0] = ROL64(A[0][2] ^ D[2], g_rhotates[0][2]);
			C[1] = ROL64(A[1][3] ^ D[3], g_rhotates[1][3]);
			C[2] = ROL64(A[2][4] ^ D[4], g_rhotates[2][4]);
			C[3] = ROL64(A[3][0] ^ D[0], g_rhotates[3][0]);
			C[4] = ROL64(A[4][1] ^ D[1], g_rhotates[4][1]);

#ifdef KECCAK_COMPLEMENTING_TRANSFORM
			R[4][0] = C[0] ^ (~C[1] & C[2]);
			R[4][1] = ~C[1] ^ (C[2] | C[3]);
			R[4][2] = C[2] ^ (C[3] & C[4]);
			R[4][3] = C[3] ^ (C[4] | C[0]);
			R[4][4] = C[4] ^ (C[0] & C[1]);
#else
			R[4][0] = C[0] ^ (~C[1] & C[2]);
			R[4][1] = C[1] ^ (~C[2] & C[3]);
			R[4][2] = C[2] ^ (~C[3] & C[4]);
			R[4][3] = C[3] ^ (~C[4] & C[0]);
			R[4][4] = C[4] ^ (~C[0] & C[1]);
#endif
		}

	}

	SHA3::SHA3(sl_size hashSize, sl_uint32 blockSize, sl_bool flagSHAKE) noexcept
	{
		SLIB_ASSERT(blockSize <= 200)
		m_hashSize = hashSize;
		m_rate = blockSize;
		m_suffix = flagSHAKE ? 31 : 6;
		m_rlen = 0;
	}

	SHA3::~SHA3()
	{
	}

	void SHA3::start() noexcept
	{
		Base::zeroMemory(m_state, sizeof(m_state));
		m_rlen = 0;
	}

	void SHA3::_update(const sl_uint8* data) noexcept
	{
		sl_uint64* A = &(m_state[0][0]);
		sl_uint32 w = m_rate >> 3;
		for (sl_uint32 i = 0; i < w; i++) {
			A[i] ^= BitInterleave(MIO::readUint64LE(data));
			data += 8;
		}
		_keccak();
	}

	void SHA3::_keccak()
	{
		sl_uint64(*A)[5] = m_state;
#ifdef KECCAK_COMPLEMENTING_TRANSFORM
		A[0][1] = ~A[0][1];
		A[0][2] = ~A[0][2];
		A[1][3] = ~A[1][3];
		A[2][2] = ~A[2][2];
		A[3][2] = ~A[3][2];
		A[4][0] = ~A[4][0];
#endif
		sl_uint64 T[5][5];
		for (sl_uint32 i = 0; i < 24; i += 2) {
			Round(T, A, i);
			Round(A, T, i + 1);
		}
#ifdef KECCAK_COMPLEMENTING_TRANSFORM
		A[0][1] = ~A[0][1];
		A[0][2] = ~A[0][2];
		A[1][3] = ~A[1][3];
		A[2][2] = ~A[2][2];
		A[3][2] = ~A[3][2];
		A[4][0] = ~A[4][0];
#endif
	}

	void SHA3::update(const void* _input, sl_size sizeInput) noexcept
	{
		const sl_uint8* input = (const sl_uint8*)_input;
		if (!sizeInput) {
			return;
		}
		if (m_rlen > 0) {
			sl_uint32 n = m_rate - m_rlen;
			if (sizeInput < n) {
				Base::copyMemory(m_rdata + m_rlen, input, sizeInput);
				m_rlen += (sl_uint32)sizeInput;
				return;
			} else {
				Base::copyMemory(m_rdata + m_rlen, input, n);
				_update(m_rdata);
				m_rlen = 0;
				sizeInput -= n;
				input += n;
				if (!sizeInput) {
					return;
				}
			}
		}
		while (sizeInput >= m_rate) {
			_update(input);
			sizeInput -= m_rate;
			input += m_rate;
		}
		if (sizeInput) {
			Base::copyMemory(m_rdata, input, sizeInput);
			m_rlen = (sl_uint32)sizeInput;
		}
	}

	void SHA3::_finish(void* _output, sl_size n) noexcept
	{
		sl_uint64* A = &(m_state[0][0]);
		sl_uint32 r = m_rate;
		// Append SHA-3 Domain Suffix (01 or 1111) and padding (10*1)
		{
			sl_uint32 r1 = r - 1;
			if (m_rlen < r1) {
				m_rdata[m_rlen] = m_suffix;
				for (sl_uint32 i = m_rlen + 1; i < r1; i++) {
					m_rdata[i] = 0;
				}
				m_rdata[r1] = (sl_uint8)0x80;
			} else {
				m_rdata[m_rlen] = (sl_uint8)(0x80 | m_suffix);
			}
			_update(m_rdata);
			m_rlen = 0;
		}
		// Extract
		if (n) {
			sl_uint8* output = (sl_uint8*)_output;
			sl_uint32 w = r >> 3;
			sl_uint32 x = 0;
			sl_uint32 y = 0;
			sl_uint32 i = 0;
			for (;;) {
				sl_uint64 Ai = BitDeinterleave(A[i]);
				if (n >= 8) {
					MIO::writeUint64LE(output, Ai);
					output += 8;
					n -= 8;
					if (!n) {
						return;
					}
				} else {
					sl_uint8 buf[8];
					MIO::writeUint64LE(buf, Ai);
					Base::copyMemory(output, buf, n);
					return;
				}
				i++;
				if (i == w) {
					i = 0;
					x = 0;
					y = 0;
					_keccak();
				} else {
					x++;
					if (x == 5) {
						y++;
						x = 0;
					}
				}
			}
		}
	}

	void SHA3::finish(void* _output) noexcept
	{
		_finish(_output, m_hashSize);
	}


	SHA3_224::SHA3_224() noexcept: SHA3(HashSize, BlockSize) {}

	SHA3_224::~SHA3_224() {}


	SHA3_256::SHA3_256() noexcept : SHA3(HashSize, BlockSize) {}

	SHA3_256::~SHA3_256() {}


	SHA3_384::SHA3_384() noexcept : SHA3(HashSize, BlockSize) {}

	SHA3_384::~SHA3_384() {}


	SHA3_512::SHA3_512() noexcept: SHA3(HashSize, BlockSize) {}
	
	SHA3_512::~SHA3_512() {}


	SHAKE128::SHAKE128() noexcept: SHA3(32, 168, sl_true) {}

	SHAKE128::~SHAKE128() {}

	void SHAKE128::finish(void* output, sl_size size) noexcept
	{
		_finish(output, size);
	}

	void SHAKE128::hash(const void* input, sl_size sizeInput, void* output, sl_size sizeOutput) noexcept
	{
		SHAKE128 h;
		h.start();
		h.update(input, sizeInput);
		h.finish(output, sizeOutput);
	}


	SHAKE256::SHAKE256() noexcept: SHA3(64, 136, sl_true) {}

	SHAKE256::~SHAKE256() {}

	void SHAKE256::finish(void* output, sl_size size) noexcept
	{
		_finish(output, size);
	}

	void SHAKE256::hash(const void* input, sl_size sizeInput, void* output, sl_size sizeOutput) noexcept
	{
		SHAKE256 h;
		h.start();
		h.update(input, sizeInput);
		h.finish(output, sizeOutput);
	}

}
