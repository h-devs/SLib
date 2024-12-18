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

#include "slib/crypto/ecc.h"

#include "slib/crypto/sha2.h"

#include "slib/math/math.h"
#include "slib/core/string_buffer.h"
#include "slib/core/safe_static.h"

namespace slib
{

	namespace {

		class Curve_secp112r1 : public EllipticCurve
		{
		public:
			Curve_secp112r1()
			{
				id = EllipticCurveId::secp112r1;
				static const sl_uint8 _p[] = {
					0xDB, 0x7C, 0x2A, 0xBF, 0x62, 0xE3, 0x5E, 0x66, 
					0x80, 0x76, 0xBE, 0xAD, 0x20, 0x8B
				};
				p = BigInt::fromBytesBE(_p, sizeof(_p));
				static const sl_uint8 _a[] = {
					0xDB, 0x7C, 0x2A, 0xBF, 0x62, 0xE3, 0x5E, 0x66, 
					0x80, 0x76, 0xBE, 0xAD, 0x20, 0x88
				};
				a = BigInt::fromBytesBE(_a, sizeof(_a));
				static const sl_uint8 _b[] = {
					0x65, 0x9E, 0xF8, 0xBA, 0x04, 0x39, 0x16, 0xEE,
					0xDE, 0x89, 0x11, 0x70, 0x2B, 0x22
				};
				b = BigInt::fromBytesBE(_b, sizeof(_b));
				static const sl_uint8 _gx[] = {
					0x09, 0x48, 0x72, 0x39, 0x99, 0x5A, 0x5E, 0xE7,
					0x6B, 0x55, 0xF9, 0xC2, 0xF0, 0x98
				};
				G.x = BigInt::fromBytesBE(_gx, sizeof(_gx));
				static const sl_uint8 _gy[] = {
					0xa8, 0x9c, 0xe5, 0xaf, 0x87, 0x24, 0xc0, 0xa2,
					0x3e, 0x0e, 0x0f, 0xf7, 0x75, 0x00
				};
				G.y = BigInt::fromBytesBE(_gy, sizeof(_gy));
				static const sl_uint8 _n[] = {
					0xDB, 0x7C, 0x2A, 0xBF, 0x62, 0xE3, 0x5E, 0x76,
					0x28, 0xDF, 0xAC, 0x65, 0x61, 0xC5
				};
				n = BigInt::fromBytesBE(_n, sizeof(_n));
				/*
				static const sl_uint8 _seed[] = {
				0x00, 0xF5, 0x0B, 0x02, 0x8E, 0x4D, 0x69, 0x6E,
				0x67, 0x68, 0x75, 0x61, 0x51, 0x75, 0x29, 0x04,
				0x72, 0x78, 0x3F, 0xB1
				};
				seed = BigInt::fromBytesBE(_seed, sizeof(_seed));
				*/
			}
		};

		class Curve_secp112r2 : public EllipticCurve
		{
		public:
			Curve_secp112r2()
			{
				id = EllipticCurveId::secp112r2;
				static const sl_uint8 _p[] = {
					0xDB, 0x7C, 0x2A, 0xBF, 0x62, 0xE3, 0x5E, 0x66,
					0x80, 0x76, 0xBE, 0xAD, 0x20, 0x8B
				};
				p = BigInt::fromBytesBE(_p, sizeof(_p));
				static const sl_uint8 _a[] = {
					0x61, 0x27, 0xC2, 0x4C, 0x05, 0xF3, 0x8A, 0x0A,
					0xAA, 0xF6, 0x5C, 0x0E, 0xF0, 0x2C
				};
				a = BigInt::fromBytesBE(_a, sizeof(_a));
				static const sl_uint8 _b[] = {
					0x51, 0xDE, 0xF1, 0x81, 0x5D, 0xB5, 0xED, 0x74,
					0xFC, 0xC3, 0x4C, 0x85, 0xD7, 0x09
				};
				b = BigInt::fromBytesBE(_b, sizeof(_b));
				static const sl_uint8 _gx[] = {
					0x4B, 0xA3, 0x0A, 0xB5, 0xE8, 0x92, 0xB4, 0xE1,
					0x64, 0x9D, 0xD0, 0x92, 0x86, 0x43
				};
				G.x = BigInt::fromBytesBE(_gx, sizeof(_gx));
				static const sl_uint8 _gy[] = {
					0xad, 0xcd, 0x46, 0xf5, 0x88, 0x2e, 0x37, 0x47,
					0xde, 0xf3, 0x6e, 0x95, 0x6e, 0x97
				};
				G.y = BigInt::fromBytesBE(_gy, sizeof(_gy));
				static const sl_uint8 _n[] = {
					0x36, 0xDF, 0x0A, 0xAF, 0xD8, 0xB8, 0xD7, 0x59,
					0x7C, 0xA1, 0x05, 0x20, 0xD0, 0x4B
				};
				n = BigInt::fromBytesBE(_n, sizeof(_n));
				/*
				static const sl_uint8 _seed[] = {
				0x00, 0x27, 0x57, 0xA1, 0x11, 0x4D, 0x69, 0x6E,
				0x67, 0x68, 0x75, 0x61, 0x51, 0x75, 0x53, 0x16,
				0xC0, 0x5E, 0x0B, 0xD4
				};
				seed = BigInt::fromBytesBE(_seed, sizeof(_seed));
				*/
			}
		};

		class Curve_secp128r1 : public EllipticCurve
		{
		public:
			Curve_secp128r1()
			{
				id = EllipticCurveId::secp128r1;
				static const sl_uint8 _p[] = {
					0xFF, 0xFF, 0xFF, 0xFD, 0xFF, 0xFF, 0xFF, 0xFF,
					0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
				};
				p = BigInt::fromBytesBE(_p, sizeof(_p));
				static const sl_uint8 _a[] = {
					0xFF, 0xFF, 0xFF, 0xFD, 0xFF, 0xFF, 0xFF, 0xFF,
					0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFC
				};
				a = BigInt::fromBytesBE(_a, sizeof(_a));
				static const sl_uint8 _b[] = {
					0xE8, 0x75, 0x79, 0xC1, 0x10, 0x79, 0xF4, 0x3D,
					0xD8, 0x24, 0x99, 0x3C, 0x2C, 0xEE, 0x5E, 0xD3
				};
				b = BigInt::fromBytesBE(_b, sizeof(_b));
				static const sl_uint8 _gx[] = {
					0x16, 0x1F, 0xF7, 0x52, 0x8B, 0x89, 0x9B, 0x2D,
					0x0C, 0x28, 0x60, 0x7C, 0xA5, 0x2C, 0x5B, 0x86
				};
				G.x = BigInt::fromBytesBE(_gx, sizeof(_gx));
				static const sl_uint8 _gy[] = {
					0xcf, 0x5a, 0xc8, 0x39, 0x5b, 0xaf, 0xeb, 0x13,
					0xc0, 0x2d, 0xa2, 0x92, 0xdd, 0xed, 0x7a, 0x83
				};
				G.y = BigInt::fromBytesBE(_gy, sizeof(_gy));
				static const sl_uint8 _n[] = {
					0xFF, 0xFF, 0xFF, 0xFE, 0x00, 0x00, 0x00, 0x00,
					0x75, 0xA3, 0x0D, 0x1B, 0x90, 0x38, 0xA1, 0x15
				};
				n = BigInt::fromBytesBE(_n, sizeof(_n));
				/*
				static const sl_uint8 _seed[] = {
				0x00, 0x0E, 0x0D, 0x4D, 0x69, 0x6E, 0x67, 0x68,
				0x75, 0x61, 0x51, 0x75, 0x0C, 0xC0, 0x3A, 0x44,
				0x73, 0xD0, 0x36, 0x79
				};
				seed = BigInt::fromBytesBE(_seed, sizeof(_seed));
				*/
			}
		};

		class Curve_secp128r2 : public EllipticCurve
		{
		public:
			Curve_secp128r2()
			{
				id = EllipticCurveId::secp128r2;
				static const sl_uint8 _p[] = {
					0xFF, 0xFF, 0xFF, 0xFD, 0xFF, 0xFF, 0xFF, 0xFF,
					0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
				};
				p = BigInt::fromBytesBE(_p, sizeof(_p));
				static const sl_uint8 _a[] = {
					0xD6, 0x03, 0x19, 0x98, 0xD1, 0xB3, 0xBB, 0xFE,
					0xBF, 0x59, 0xCC, 0x9B, 0xBF, 0xF9, 0xAE, 0xE1
				};
				a = BigInt::fromBytesBE(_a, sizeof(_a));
				static const sl_uint8 _b[] = {
					0x5E, 0xEE, 0xFC, 0xA3, 0x80, 0xD0, 0x29, 0x19,
					0xDC, 0x2C, 0x65, 0x58, 0xBB, 0x6D, 0x8A, 0x5D
				};
				b = BigInt::fromBytesBE(_b, sizeof(_b));
				static const sl_uint8 _gx[] = {
					0x7B, 0x6A, 0xA5, 0xD8, 0x5E, 0x57, 0x29, 0x83,
					0xE6, 0xFB, 0x32, 0xA7, 0xCD, 0xEB, 0xC1, 0x40
				};
				G.x = BigInt::fromBytesBE(_gx, sizeof(_gx));
				static const sl_uint8 _gy[] = {
					0x27, 0xb6, 0x91, 0x6a, 0x89, 0x4d, 0x3a, 0xee,
					0x71, 0x06, 0xfe, 0x80, 0x5f, 0xc3, 0x4b, 0x44
				};
				G.y = BigInt::fromBytesBE(_gy, sizeof(_gy));
				static const sl_uint8 _n[] = {
					0x3F, 0xFF, 0xFF, 0xFF, 0x7F, 0xFF, 0xFF, 0xFF,
					0xBE, 0x00, 0x24, 0x72, 0x06, 0x13, 0xB5, 0xA3
				};
				n = BigInt::fromBytesBE(_n, sizeof(_n));
				/*
				static const sl_uint8 _seed[] = {
				0x00, 0x4D, 0x69, 0x6E, 0x67, 0x68, 0x75, 0x61,
				0x51, 0x75, 0x12, 0xD8, 0xF0, 0x34, 0x31, 0xFC,
				0xE6, 0x3B, 0x88, 0xF4
				};
				seed = BigInt::fromBytesBE(_seed, sizeof(_seed));
				*/
			}
		};

		class Curve_secp160k1 : public EllipticCurve
		{
		public:
			Curve_secp160k1()
			{
				id = EllipticCurveId::secp160k1;
				static const sl_uint8 _p[] = {
					0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
					0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
					0xFE, 0xFF, 0xFF, 0xAC, 0x73
				};
				p = BigInt::fromBytesBE(_p, sizeof(_p));
				a = BigInt::null();
				b = BigInt::fromUint32(7);
				static const sl_uint8 _gx[] = {
					0x00, 0x3B, 0x4C, 0x38, 0x2C, 0xE3, 0x7A, 0xA1,
					0x92, 0xA4, 0x01, 0x9E, 0x76, 0x30, 0x36, 0xF4,
					0xF5, 0xDD, 0x4D, 0x7E, 0xBB
				};
				G.x = BigInt::fromBytesBE(_gx, sizeof(_gx));
				static const sl_uint8 _gy[] = {
					0x00, 0x93, 0x8c, 0xf9, 0x35, 0x31, 0x8f, 0xdc,
					0xed, 0x6b, 0xc2, 0x82, 0x86, 0x53, 0x17, 0x33,
					0xc3, 0xf0, 0x3c, 0x4f, 0xee
				};
				G.y = BigInt::fromBytesBE(_gy, sizeof(_gy));
				static const sl_uint8 _n[] = {
					0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
					0x00, 0x00, 0x01, 0xB8, 0xFA, 0x16, 0xDF, 0xAB,
					0x9A, 0xCA, 0x16, 0xB6, 0xB3
				};
				n = BigInt::fromBytesBE(_n, sizeof(_n));
			}
		};

		class Curve_secp160r1 : public EllipticCurve
		{
		public:
			Curve_secp160r1()
			{
				id = EllipticCurveId::secp160r1;
				static const sl_uint8 _p[] = {
					0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
					0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
					0xFF, 0x7F, 0xFF, 0xFF, 0xFF
				};
				p = BigInt::fromBytesBE(_p, sizeof(_p));
				static const sl_uint8 _a[] = {
					0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
					0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
					0xFF, 0x7F, 0xFF, 0xFF, 0xFC
				};
				a = BigInt::fromBytesBE(_a, sizeof(_a));
				static const sl_uint8 _b[] = {
					0x00, 0x1C, 0x97, 0xBE, 0xFC, 0x54, 0xBD, 0x7A,
					0x8B, 0x65, 0xAC, 0xF8, 0x9F, 0x81, 0xD4, 0xD4,
					0xAD, 0xC5, 0x65, 0xFA, 0x45
				};
				b = BigInt::fromBytesBE(_b, sizeof(_b));
				static const sl_uint8 _gx[] = {
					0x00, 0x4A, 0x96, 0xB5, 0x68, 0x8E, 0xF5, 0x73,
					0x28, 0x46, 0x64, 0x69, 0x89, 0x68, 0xC3, 0x8B,
					0xB9, 0x13, 0xCB, 0xFC, 0x82
				};
				G.x = BigInt::fromBytesBE(_gx, sizeof(_gx));
				static const sl_uint8 _gy[] = {
					0x00, 0x23, 0xa6, 0x28, 0x55, 0x31, 0x68, 0x94,
					0x7d, 0x59, 0xdc, 0xc9, 0x12, 0x04, 0x23, 0x51,
					0x37, 0x7a, 0xc5, 0xfb, 0x32
				};
				G.y = BigInt::fromBytesBE(_gy, sizeof(_gy));
				static const sl_uint8 _n[] = {
					0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
					0x00, 0x00, 0x01, 0xF4, 0xC8, 0xF9, 0x27, 0xAE,
					0xD3, 0xCA, 0x75, 0x22, 0x57
				};
				n = BigInt::fromBytesBE(_n, sizeof(_n));
				/*
				static const sl_uint8 _seed[] = {
				0x10, 0x53, 0xCD, 0xE4, 0x2C, 0x14, 0xD6, 0x96,
				0xE6, 0x76, 0x87, 0x56, 0x15, 0x17, 0x53, 0x3B,
				0xF3, 0xF8, 0x33, 0x45
				};
				seed = BigInt::fromBytesBE(_seed, sizeof(_seed));
				*/
			}
		};

		class Curve_secp160r2 : public EllipticCurve
		{
		public:
			Curve_secp160r2()
			{
				id = EllipticCurveId::secp160r2;
				static const sl_uint8 _p[] = {
					0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
					0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
					0xFE, 0xFF, 0xFF, 0xAC, 0x73
				};
				p = BigInt::fromBytesBE(_p, sizeof(_p));
				static const sl_uint8 _a[] = {
					0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
					0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
					0xFE, 0xFF, 0xFF, 0xAC, 0x70
				};
				a = BigInt::fromBytesBE(_a, sizeof(_a));
				static const sl_uint8 _b[] = {
					0x00, 0xB4, 0xE1, 0x34, 0xD3, 0xFB, 0x59, 0xEB,
					0x8B, 0xAB, 0x57, 0x27, 0x49, 0x04, 0x66, 0x4D,
					0x5A, 0xF5, 0x03, 0x88, 0xBA
				};
				b = BigInt::fromBytesBE(_b, sizeof(_b));
				static const sl_uint8 _gx[] = {
					0x00, 0x52, 0xDC, 0xB0, 0x34, 0x29, 0x3A, 0x11,
					0x7E, 0x1F, 0x4F, 0xF1, 0x1B, 0x30, 0xF7, 0x19,
					0x9D, 0x31, 0x44, 0xCE, 0x6D
				};
				G.x = BigInt::fromBytesBE(_gx, sizeof(_gx));
				static const sl_uint8 _gy[] = {
					0x00, 0xfe, 0xaf, 0xfe, 0xf2, 0xe3, 0x31, 0xf2,
					0x96, 0xe0, 0x71, 0xfa, 0x0d, 0xf9, 0x98, 0x2c,
					0xfe, 0xa7, 0xd4, 0x3f, 0x2e
				};
				G.y = BigInt::fromBytesBE(_gy, sizeof(_gy));
				static const sl_uint8 _n[] = {
					0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
					0x00, 0x00, 0x00, 0x35, 0x1E, 0xE7, 0x86, 0xA8,
					0x18, 0xF3, 0xA1, 0xA1, 0x6B
				};
				n = BigInt::fromBytesBE(_n, sizeof(_n));
				/*
				static const sl_uint8 _seed[] = {
				0xB9, 0x9B, 0x99, 0xB0, 0x99, 0xB3, 0x23, 0xE0,
				0x27, 0x09, 0xA4, 0xD6, 0x96, 0xE6, 0x76, 0x87,
				0x56, 0x15, 0x17, 0x51
				};
				seed = BigInt::fromBytesBE(_seed, sizeof(_seed));
				*/
			}
		};

		class Curve_secp192k1 : public EllipticCurve
		{
		public:
			Curve_secp192k1()
			{
				id = EllipticCurveId::secp192k1;
				static const sl_uint8 _p[] = {
					0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
					0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
					0xFF, 0xFF, 0xFF, 0xFE, 0xFF, 0xFF, 0xEE, 0x37
				};
				p = BigInt::fromBytesBE(_p, sizeof(_p));
				a = BigInt::null();
				b = BigInt::fromUint32(3);
				static const sl_uint8 _gx[] = {
					0xDB, 0x4F, 0xF1, 0x0E, 0xC0, 0x57, 0xE9, 0xAE,
					0x26, 0xB0, 0x7D, 0x02, 0x80, 0xB7, 0xF4, 0x34,
					0x1D, 0xA5, 0xD1, 0xB1, 0xEA, 0xE0, 0x6C, 0x7D
				};
				G.x = BigInt::fromBytesBE(_gx, sizeof(_gx));
				static const sl_uint8 _gy[] = {
					0x9b, 0x2f, 0x2f, 0x6d, 0x9c, 0x56, 0x28, 0xa7,
					0x84, 0x41, 0x63, 0xd0, 0x15, 0xbe, 0x86, 0x34,
					0x40, 0x82, 0xaa, 0x88, 0xd9, 0x5e, 0x2f, 0x9d
				};
				G.y = BigInt::fromBytesBE(_gy, sizeof(_gy));
				static const sl_uint8 _n[] = {
					0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
					0xFF, 0xFF, 0xFF, 0xFE, 0x26, 0xF2, 0xFC, 0x17,
					0x0F, 0x69, 0x46, 0x6A, 0x74, 0xDE, 0xFD, 0x8D
				};
				n = BigInt::fromBytesBE(_n, sizeof(_n));
			}
		};

		class Curve_secp224k1 : public EllipticCurve
		{
		public:
			Curve_secp224k1()
			{
				id = EllipticCurveId::secp224k1;
				static const sl_uint8 _p[] = {
					0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
					0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
					0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
					0xFE, 0xFF, 0xFF, 0xE5, 0x6D
				};
				p = BigInt::fromBytesBE(_p, sizeof(_p));
				a = BigInt::null();
				b = BigInt::fromUint32(5);
				static const sl_uint8 _gx[] = {
					0x00, 0xA1, 0x45, 0x5B, 0x33, 0x4D, 0xF0, 0x99,
					0xDF, 0x30, 0xFC, 0x28, 0xA1, 0x69, 0xA4, 0x67,
					0xE9, 0xE4, 0x70, 0x75, 0xA9, 0x0F, 0x7E, 0x65,
					0x0E, 0xB6, 0xB7, 0xA4, 0x5C
				};
				G.x = BigInt::fromBytesBE(_gx, sizeof(_gx));
				static const sl_uint8 _gy[] = {
					0x00, 0x7e, 0x08, 0x9f, 0xed, 0x7f, 0xba, 0x34,
					0x42, 0x82, 0xca, 0xfb, 0xd6, 0xf7, 0xe3, 0x19,
					0xf7, 0xc0, 0xb0, 0xbd, 0x59, 0xe2, 0xca, 0x4b,
					0xdb, 0x55, 0x6d, 0x61, 0xa5
				};
				G.y = BigInt::fromBytesBE(_gy, sizeof(_gy));
				static const sl_uint8 _n[] = {
					0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
					0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xDC,
					0xE8, 0xD2, 0xEC, 0x61, 0x84, 0xCA, 0xF0, 0xA9,
					0x71, 0x76, 0x9F, 0xB1, 0xF7
				};
				n = BigInt::fromBytesBE(_n, sizeof(_n));
			}
		};

		class Curve_secp256k1 : public EllipticCurve
		{
		public:
			Curve_secp256k1()
			{
				id = EllipticCurveId::secp256k1;
				static const sl_uint8 _p[] = {
					0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
					0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
					0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
					0xFF, 0xFF, 0xFF, 0xFE, 0xFF, 0xFF, 0xFC, 0x2F
				};
				p = BigInt::fromBytesBE(_p, sizeof(_p));
				a = BigInt::null();
				b = BigInt::fromUint32(7);
				static const sl_uint8 _gx[] = {
					0x79, 0xBE, 0x66, 0x7E, 0xF9, 0xDC, 0xBB, 0xAC,
					0x55, 0xA0, 0x62, 0x95, 0xCE, 0x87, 0x0B, 0x07,
					0x02, 0x9B, 0xFC, 0xDB, 0x2D, 0xCE, 0x28, 0xD9,
					0x59, 0xF2, 0x81, 0x5B, 0x16, 0xF8, 0x17, 0x98
				};
				G.x = BigInt::fromBytesBE(_gx, sizeof(_gx));
				static const sl_uint8 _gy[] = {
					0x48, 0x3a, 0xda, 0x77, 0x26, 0xa3, 0xc4, 0x65,
					0x5d, 0xa4, 0xfb, 0xfc, 0x0e, 0x11, 0x08, 0xa8,
					0xfd, 0x17, 0xb4, 0x48, 0xa6, 0x85, 0x54, 0x19,
					0x9c, 0x47, 0xd0, 0x8f, 0xfb, 0x10, 0xd4, 0xb8
				};
				G.y = BigInt::fromBytesBE(_gy, sizeof(_gy));
				static const sl_uint8 _n[] = {
					0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
					0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFE,
					0xBA, 0xAE, 0xDC, 0xE6, 0xAF, 0x48, 0xA0, 0x3B,
					0xBF, 0xD2, 0x5E, 0x8C, 0xD0, 0x36, 0x41, 0x41
				};
				n = BigInt::fromBytesBE(_n, sizeof(_n));
			}

		};

		class Curve_secp384r1 : public EllipticCurve
		{
		public:
			Curve_secp384r1()
			{
				id = EllipticCurveId::secp384r1;
				static const sl_uint8 _p[] = {
					0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
					0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
					0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
					0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFE,
					0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00,
					0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF
				};
				p = BigInt::fromBytesBE(_p, sizeof(_p));
				static const sl_uint8 _a[] = {
					0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
					0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
					0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
					0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFE,
					0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00,
					0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFC
				};
				a = BigInt::fromBytesBE(_a, sizeof(_a));
				static const sl_uint8 _b[] = {
					0xB3, 0x31, 0x2F, 0xA7, 0xE2, 0x3E, 0xE7, 0xE4,
					0x98, 0x8E, 0x05, 0x6B, 0xE3, 0xF8, 0x2D, 0x19,
					0x18, 0x1D, 0x9C, 0x6E, 0xFE, 0x81, 0x41, 0x12,
					0x03, 0x14, 0x08, 0x8F, 0x50, 0x13, 0x87, 0x5A,
					0xC6, 0x56, 0x39, 0x8D, 0x8A, 0x2E, 0xD1, 0x9D,
					0x2A, 0x85, 0xC8, 0xED, 0xD3, 0xEC, 0x2A, 0xEF
				};
				b = BigInt::fromBytesBE(_b, sizeof(_b));
				static const sl_uint8 _gx[] = {
					0xAA, 0x87, 0xCA, 0x22, 0xBE, 0x8B, 0x05, 0x37,
					0x8E, 0xB1, 0xC7, 0x1E, 0xF3, 0x20, 0xAD, 0x74,
					0x6E, 0x1D, 0x3B, 0x62, 0x8B, 0xA7, 0x9B, 0x98,
					0x59, 0xF7, 0x41, 0xE0, 0x82, 0x54, 0x2A, 0x38,
					0x55, 0x02, 0xF2, 0x5D, 0xBF, 0x55, 0x29, 0x6C,
					0x3A, 0x54, 0x5E, 0x38, 0x72, 0x76, 0x0A, 0xB7 
				};
				G.x = BigInt::fromBytesBE(_gx, sizeof(_gx));
				static const sl_uint8 _gy[] = {
					0x36, 0x17, 0xde, 0x4a, 0x96, 0x26, 0x2c, 0x6f,
					0x5d, 0x9e, 0x98, 0xbf, 0x92, 0x92, 0xdc, 0x29,
					0xf8, 0xf4, 0x1d, 0xbd, 0x28, 0x9a, 0x14, 0x7c,
					0xe9, 0xda, 0x31, 0x13, 0xb5, 0xf0, 0xb8, 0xc0,
					0x0a, 0x60, 0xb1, 0xce, 0x1d, 0x7e, 0x81, 0x9d,
					0x7a, 0x43, 0x1d, 0x7c, 0x90, 0xea, 0x0e, 0x5f
				};
				G.y = BigInt::fromBytesBE(_gy, sizeof(_gy));
				static const sl_uint8 _n[] = {
					0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
					0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
					0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
					0xC7, 0x63, 0x4D, 0x81, 0xF4, 0x37, 0x2D, 0xDF,
					0x58, 0x1A, 0x0D, 0xB2, 0x48, 0xB0, 0xA7, 0x7A,
					0xEC, 0xEC, 0x19, 0x6A, 0xCC, 0xC5, 0x29, 0x73
				};
				n = BigInt::fromBytesBE(_n, sizeof(_n));
				/*
				static const sl_uint8 _seed[] = {
				0xA3, 0x35, 0x92, 0x6A, 0xA3, 0x19, 0xA2, 0x7A,
				0x1D, 0x00, 0x89, 0x6A, 0x67, 0x73, 0xA4, 0x82,
				0x7A, 0xCD, 0xAC, 0x73
				};
				seed = BigInt::fromBytesBE(_seed, sizeof(_seed));
				*/
			}
		};

		class Curve_secp521r1 : public EllipticCurve
		{
		public:
			Curve_secp521r1()
			{
				id = EllipticCurveId::secp521r1;
				static const sl_uint8 _p[] = {
					0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
					0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
					0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
					0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
					0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
					0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
					0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
					0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
					0xFF, 0xFF
				};
				p = BigInt::fromBytesBE(_p, sizeof(_p));
				static const sl_uint8 _a[] = {
					0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
					0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
					0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
					0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
					0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
					0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
					0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
					0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
					0xFF, 0xFC
				};
				a = BigInt::fromBytesBE(_a, sizeof(_a));
				static const sl_uint8 _b[] = {
					0x00, 0x51, 0x95, 0x3E, 0xB9, 0x61, 0x8E, 0x1C,
					0x9A, 0x1F, 0x92, 0x9A, 0x21, 0xA0, 0xB6, 0x85,
					0x40, 0xEE, 0xA2, 0xDA, 0x72, 0x5B, 0x99, 0xB3,
					0x15, 0xF3, 0xB8, 0xB4, 0x89, 0x91, 0x8E, 0xF1,
					0x09, 0xE1, 0x56, 0x19, 0x39, 0x51, 0xEC, 0x7E,
					0x93, 0x7B, 0x16, 0x52, 0xC0, 0xBD, 0x3B, 0xB1,
					0xBF, 0x07, 0x35, 0x73, 0xDF, 0x88, 0x3D, 0x2C,
					0x34, 0xF1, 0xEF, 0x45, 0x1F, 0xD4, 0x6B, 0x50,
					0x3F, 0x00
				};
				b = BigInt::fromBytesBE(_b, sizeof(_b));
				static const sl_uint8 _gx[] = {
					0x00, 0xC6, 0x85, 0x8E, 0x06, 0xB7, 0x04, 0x04,
					0xE9, 0xCD, 0x9E, 0x3E, 0xCB, 0x66, 0x23, 0x95,
					0xB4, 0x42, 0x9C, 0x64, 0x81, 0x39, 0x05, 0x3F,
					0xB5, 0x21, 0xF8, 0x28, 0xAF, 0x60, 0x6B, 0x4D,
					0x3D, 0xBA, 0xA1, 0x4B, 0x5E, 0x77, 0xEF, 0xE7,
					0x59, 0x28, 0xFE, 0x1D, 0xC1, 0x27, 0xA2, 0xFF,
					0xA8, 0xDE, 0x33, 0x48, 0xB3, 0xC1, 0x85, 0x6A,
					0x42, 0x9B, 0xF9, 0x7E, 0x7E, 0x31, 0xC2, 0xE5,
					0xBD, 0x66
				};
				G.x = BigInt::fromBytesBE(_gx, sizeof(_gx));
				static const sl_uint8 _gy[] = {
					0x01, 0x18, 0x39, 0x29, 0x6a, 0x78, 0x9a, 0x3b,
					0xc0, 0x04, 0x5c, 0x8a, 0x5f, 0xb4, 0x2c, 0x7d,
					0x1b, 0xd9, 0x98, 0xf5, 0x44, 0x49, 0x57, 0x9b,
					0x44, 0x68, 0x17, 0xaf, 0xbd, 0x17, 0x27, 0x3e,
					0x66, 0x2c, 0x97, 0xee, 0x72, 0x99, 0x5e, 0xf4,
					0x26, 0x40, 0xc5, 0x50, 0xb9, 0x01, 0x3f, 0xad,
					0x07, 0x61, 0x35, 0x3c, 0x70, 0x86, 0xa2, 0x72,
					0xc2, 0x40, 0x88, 0xbe, 0x94, 0x76, 0x9f, 0xd1,
					0x66, 0x50
				};
				G.y = BigInt::fromBytesBE(_gy, sizeof(_gy));
				static const sl_uint8 _n[] = {
					0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
					0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
					0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
					0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
					0xFF, 0xFA, 0x51, 0x86, 0x87, 0x83, 0xBF, 0x2F,
					0x96, 0x6B, 0x7F, 0xCC, 0x01, 0x48, 0xF7, 0x09,
					0xA5, 0xD0, 0x3B, 0xB5, 0xC9, 0xB8, 0x89, 0x9C,
					0x47, 0xAE, 0xBB, 0x6F, 0xB7, 0x1E, 0x91, 0x38,
					0x64, 0x09
				};
				n = BigInt::fromBytesBE(_n, sizeof(_n));
				/*
				static const sl_uint8 _seed[] = {
				0xD0, 0x9E, 0x88, 0x00, 0x29, 0x1C, 0xB8, 0x53,
				0x96, 0xCC, 0x67, 0x17, 0x39, 0x32, 0x84, 0xAA,
				0xA0, 0xDA, 0x64, 0xBA
				};
				seed = BigInt::fromBytesBE(_seed, sizeof(_seed));
				*/
			}
		};

	}


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(ECPoint)

	ECPoint::ECPoint() noexcept
	{
	}

	sl_bool ECPoint::isO() const noexcept
	{
		return y.isZero();
	}

	Memory ECPoint::toUncompressedFormat(const EllipticCurve& curve) const noexcept
	{
		return toUncompressedFormat(curve.n.getMostSignificantBytes());
	}

	Memory ECPoint::toUncompressedFormat(sl_size nBytesPerComponent) const noexcept
	{
		if (isO()) {
			sl_uint8 c = 0;
			return Memory::create(&c, 1);
		}
		if (!nBytesPerComponent) {
			nBytesPerComponent = Math::max(x.getMostSignificantBytes(), y.getMostSignificantBytes());
		}
		Memory ret = Memory::create((nBytesPerComponent << 1) + 1);
		if (ret.isNotNull()) {
			sl_uint8* buf = (sl_uint8*)(ret.getData());
			buf[0] = 4;
			x.getBytesBE(buf + 1, nBytesPerComponent);
			y.getBytesBE(buf + 1 + nBytesPerComponent, nBytesPerComponent);
			return ret;
		}
		return sl_null;
	}

	Memory ECPoint::toCompressedFormat(const EllipticCurve& curve) const noexcept
	{
		return toCompressedFormat(curve.n.getMostSignificantBytes());
	}

	Memory ECPoint::toCompressedFormat(sl_size nBytesPerComponent) const noexcept
	{
		if (isO()) {
			sl_uint8 c = 0;
			return Memory::create(&c, 1);
		}
		if (!nBytesPerComponent) {
			nBytesPerComponent = Math::max(x.getMostSignificantBytes(), y.getMostSignificantBytes());
		}
		Memory ret = Memory::create(nBytesPerComponent + 1);
		if (ret.isNotNull()) {
			sl_uint8* buf = (sl_uint8*)(ret.getData());
			buf[0] = y.isOdd() ? 3 : 2;
			x.getBytesBE(buf + 1, nBytesPerComponent);
			return ret;
		}
		return sl_null;
	}

	Memory ECPoint::toHybridFormat(const EllipticCurve& curve) const noexcept
	{
		return toHybridFormat(curve.n.getMostSignificantBytes());
	}

	Memory ECPoint::toHybridFormat(sl_size nBytesPerComponent) const noexcept
	{
		if (isO()) {
			sl_uint8 c = 0;
			return Memory::create(&c, 1);
		}
		if (!nBytesPerComponent) {
			nBytesPerComponent = Math::max(x.getMostSignificantBytes(), y.getMostSignificantBytes());
		}
		Memory ret = Memory::create((nBytesPerComponent << 1) + 1);
		if (ret.isNotNull()) {
			sl_uint8* buf = (sl_uint8*)(ret.getData());
			buf[0] = y.isOdd() ? 7 : 6;
			x.getBytesBE(buf + 1, nBytesPerComponent);
			y.getBytesBE(buf + 1 + nBytesPerComponent, nBytesPerComponent);
			return ret;
		}
		return sl_null;
	}

	sl_bool ECPoint::parseBinaryFormat(const MemoryView& mem, const EllipticCurve* curve) noexcept
	{
		const sl_uint8* buf = (const sl_uint8*)(mem.data);
		sl_size size = mem.size;
		if (size) {
			sl_uint8 type = buf[0];
			switch (type) {
				case 0:
					if (size == 1) {
						x.setNull();
						y.setNull();
						return sl_true;
					}
					break;
				case 2:
				case 3:
					if (curve) {
						size--;
						BigInt _x = BigInt::fromBytesBE(buf + 1, size);
						if (_x.isNotNull()) {
							BigInt _y = curve->getY(_x, type & 1);
							if (_y.isNotNull()) {
								x = Move(_x);
								y = Move(_y);
								return sl_true;
							}
						}
					}
					break;
				case 4:
				case 6:
				case 7:
					size--;
					if (!(size & 1)) {
						size >>= 1;
						BigInt _x = BigInt::fromBytesBE(buf + 1, size);
						if (_x.isNotNull()) {
							BigInt _y = BigInt::fromBytesBE(buf + 1 + size, size);
							if (_y.isNotNull()) {
								x = Move(_x);
								y = Move(_y);
								if (type == 4) {
									return sl_true;
								} else {
									return (type & 1) == y.isOdd() ? 1 : 0;
								}
							}
						}
					}
					break;
			}
		}
		return sl_false;
	}

	sl_bool ECPoint::parseBinaryFormat(const EllipticCurve& curve, const MemoryView& mem) noexcept
	{
		return parseBinaryFormat(mem, &curve);
	}


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(EllipticCurve)

	EllipticCurve::EllipticCurve() noexcept: id(EllipticCurveId::Unknown), h(1)
	{
	}

	sl_bool EllipticCurve::isDefined() const noexcept
	{
		return p.isNotNull() && a.isNotNull() && b.isNotNull() && !(G.isO()) && n.isNotNull();
	}

	sl_bool EllipticCurve::setCurveId(EllipticCurveId _id) noexcept
	{
		switch (_id) {
			case EllipticCurveId::secp112r1:
				*this = EllipticCurve::secp112r1();
				break;
			case EllipticCurveId::secp112r2:
				*this = EllipticCurve::secp112r2();
				break;
			case EllipticCurveId::secp128r1:
				*this = EllipticCurve::secp128r1();
				break;
			case EllipticCurveId::secp128r2:
				*this = EllipticCurve::secp128r2();
				break;
			case EllipticCurveId::secp160k1:
				*this = EllipticCurve::secp160k1();
				break;
			case EllipticCurveId::secp160r1:
				*this = EllipticCurve::secp160r1();
				break;
			case EllipticCurveId::secp160r2:
				*this = EllipticCurve::secp160r2();
				break;
			case EllipticCurveId::secp192k1:
				*this = EllipticCurve::secp192k1();
				break;
			case EllipticCurveId::secp224k1:
				*this = EllipticCurve::secp224k1();
				break;
			case EllipticCurveId::secp256k1:
				*this = EllipticCurve::secp256k1();
				break;
			case EllipticCurveId::secp384r1:
				*this = EllipticCurve::secp384r1();
				break;
			case EllipticCurveId::secp521r1:
				*this = EllipticCurve::secp521r1();
				break;
			default:
				return sl_false;
		}
		id = _id;
		return sl_true;
	}

	ECPoint EllipticCurve::addPoint(const ECPoint& p1, const ECPoint& p2) const noexcept
	{
		if (p1.isO()) {
			return p2;
		} else if (p2.isO()) {
			return p1;
		}
		if (p1.x == p2.x) {
			if (p1.y + p2.y == p) {
				return ECPoint();
			} else {
				return doublePoint(p1);
			}
		} else {
			ECPoint ret;
			BigInt lambda = BigInt::mod((p2.y - p1.y) * BigInt::inverseMod(p2.x - p1.x, p), p, sl_true);
			ret.x = BigInt::mod((lambda * lambda) - p1.x - p2.x, p, sl_true);
			ret.y = BigInt::mod((lambda * (p1.x - ret.x)) - p1.y, p, sl_true);
			return ret;
		}
	}

	ECPoint EllipticCurve::doublePoint(const ECPoint& pt) const noexcept
	{
		if (pt.isO()) {
			return pt;
		}
		ECPoint ret;
		BigInt x2 = pt.x * pt.x;
		BigInt lambda = BigInt::mod((x2 + x2 + x2 + a) * BigInt::inverseMod(pt.y + pt.y, p), p, sl_true);
		ret.x = BigInt::mod((lambda * lambda) - pt.x - pt.x, p, sl_true);
		ret.y = BigInt::mod((lambda * (pt.x - ret.x)) - pt.y, p, sl_true);
		return ret;
	}

	ECPoint EllipticCurve::multiplyPoint(const ECPoint& pt, const BigInt& _k) const noexcept
	{
		CBigInt* k = _k.ref.get();
		if (!k) {
			return ECPoint();
		}
		if (k->isZero()) {
			return ECPoint();
		}
		if (k->equals(1)) {
			return pt;
		}
		sl_size nBits = k->getMostSignificantBits();
		ECPoint ret;
		ECPoint pt2 = pt;
		for (sl_size i = 0; i < nBits; i++) {
			if (k->getBit(i)) {
				ret = addPoint(ret, pt2);
			}
			pt2 = doublePoint(pt2);
		}
		return ret;
	}

	ECPoint EllipticCurve::multiplyG(const BigInt& _k) const noexcept
	{
		return multiplyPoint(G, _k);
	}

	BigInt EllipticCurve::getY(const BigInt& x, sl_bool yBit) const noexcept
	{
		// y ^ 2 = x ^ 3 + ax + b (mod p)
		BigInt y = BigInt::sqrtMod(x * x * x + x * a + b, p);
		if (!yBit == y.isEven()) {
			return y;
		} else {
			return p - y;
		}
	}


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(ECPublicKey)

	ECPublicKey::ECPublicKey() noexcept
	{
	}

	sl_bool ECPublicKey::isDefined() const noexcept
	{
		return Q.x.isNotNull();
	}

	sl_bool ECPublicKey::equals(const ECPublicKey& other) const noexcept
	{
		return Q.x.equals(other.Q.x);
	}

	sl_compare_result ECPublicKey::compare(const ECPublicKey& other) const noexcept
	{
		return Q.x.compare(other.Q.x);
	}

	sl_bool ECPublicKey::checkValid(const EllipticCurve& curve) const noexcept
	{
		if (Q.isO()) {
			return sl_false;
		}
		if (Q.x >= curve.p) {
			return sl_false;
		}
		if (Q.y >= curve.p) {
			return sl_false;
		}
		BigInt dy = BigInt::mod((Q.x * Q.x * Q.x) + (curve.a * Q.x) + curve.b - (Q.y * Q.y), curve.p, sl_true);
		if (dy.isNotZero()) {
			return sl_false;
		}
		ECPoint nQ = curve.multiplyPoint(Q, curve.n);
		if (!(nQ.isO())) {
			return sl_false;
		}
		return sl_true;
	}


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(ECPrivateKey)

	ECPrivateKey::ECPrivateKey() noexcept
	{
	}

	sl_bool ECPrivateKey::isDefined() const noexcept
	{
		return ECPublicKey::isDefined() && d.isNotNull();
	}

	sl_bool ECPrivateKey::generate(const EllipticCurve& curve) noexcept
	{
		if (curve.n < 3) {
			return sl_false;
		}
		BigInt n2 = curve.n - 2;
		sl_size nBits = curve.n.getMostSignificantBits();
		for (;;) {
			d = BigInt::random(nBits);
			if (d.isNull()) {
				return sl_false;
			}
			d = BigInt::mod(d, n2, sl_true);
			if (d.getMostSignificantBits() + 2 < nBits) {
				d = n2 - d + 1;
			} else {
				d += 2;
			}
			Q = curve.multiplyG(d);
			if (checkValid(curve)) {
				break;
			}
		}
		return sl_true;
	}


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(ECPublicKeyWithCurve)

	ECPublicKeyWithCurve::ECPublicKeyWithCurve() noexcept
	{
	}

	sl_bool ECPublicKeyWithCurve::isDefined() const noexcept
	{
		return EllipticCurve::isDefined() && ECPublicKey::isDefined();
	}

	void ECPublicKeyWithCurve::set(const EllipticCurve& curve, const ECPublicKey& key) noexcept
	{
		*((EllipticCurve*)this) = curve;
		*((ECPublicKey*)this) = key;
	}

	void ECPublicKeyWithCurve::set(const EllipticCurve& curve, ECPublicKey&& key) noexcept
	{
		*((EllipticCurve*)this) = curve;
		*((ECPublicKey*)this) = Move(key);
	}


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(ECPrivateKeyWithCurve)

	ECPrivateKeyWithCurve::ECPrivateKeyWithCurve() noexcept
	{
	}

	sl_bool ECPrivateKeyWithCurve::isDefined() const noexcept
	{
		return EllipticCurve::isDefined() && ECPrivateKey::isDefined();
	}

	void ECPrivateKeyWithCurve::set(const EllipticCurve& curve, const ECPrivateKey& key) noexcept
	{
		*((EllipticCurve*)this) = curve;
		*((ECPrivateKey*)this) = key;
	}

	void ECPrivateKeyWithCurve::set(const EllipticCurve& curve, ECPrivateKey&& key) noexcept
	{
		*((EllipticCurve*)this) = curve;
		*((ECPrivateKey*)this) = Move(key);
	}

	sl_bool ECPrivateKeyWithCurve::generate() noexcept
	{
		return ECPrivateKey::generate(*this);
	}


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(ECDSA_Signature)

	ECDSA_Signature::ECDSA_Signature() noexcept
	{
	}

	Memory ECDSA_Signature::serialize() const noexcept
	{
		sl_size n1 = r.getMostSignificantBytes();
		sl_size n2 = s.getMostSignificantBytes();
		sl_size n = Math::max(n1, n2);
		Memory ret = Memory::create(n << 1);
		if (ret.isNotNull()) {
			sl_uint8* signature = (sl_uint8*)(ret.getData());
			r.getBytesBE(signature, n);
			s.getBytesBE(signature + n, n);
			return ret;
		}
		return sl_null;
	}

	sl_bool ECDSA_Signature::deserialize(const MemoryView& mem) noexcept
	{
		const sl_uint8* signature = (const sl_uint8*)(mem.data);
		sl_size size = mem.size;
		if (size & 1) {
			return sl_false;
		}
		size >>= 1;
		r = BigInt::fromBytesBE(signature, size);
		s = BigInt::fromBytesBE(signature + size, size);
		return sl_true;
	}

	namespace {
		static BigInt ECDSA_MakeZ(const EllipticCurve& curve, const void* hash, sl_size hashSize) noexcept
		{
			sl_size nBits = curve.n.getMostSignificantBits();
			if (!nBits) {
				return BigInt::null();
			}
			sl_size hashBits = hashSize << 3;
			if (nBits > hashBits) {
				return BigInt::fromBytesBE(hash, hashSize);
			} else {
				sl_uint32 leftBits = (sl_uint32)(nBits & 7);
				if (leftBits) {
					sl_size nBytes = (nBits >> 3) + 1;
					return BigInt::fromBytesBE(hash, nBytes) >> (8 - leftBits);
				} else {
					return BigInt::fromBytesBE(hash, nBits >> 3);
				}
			}
		}
	}

	ECDSA_Signature ECDSA::sign(const EllipticCurve& curve, const ECPrivateKey& key, const BigInt& z, BigInt* _k) noexcept
	{
		if (curve.G.isO()) {
			return ECDSA_Signature();
		}
		sl_size nBitsOrder = curve.n.getMostSignificantBits();
		if (nBitsOrder < 2) {
			return ECDSA_Signature();
		}
		BigInt r, s;
		for (;;) {
			sl_bool flagInputK = sl_false;
			BigInt k;
			if (_k) {
				if (_k->isNull()) {
					k = BigInt::mod(BigInt::random(nBitsOrder), curve.n - 1, sl_true) + 1;
				} else {
					flagInputK = sl_true;
					k = *_k;
				}
			} else {
				k = BigInt::mod(BigInt::random(nBitsOrder), curve.n - 1, sl_true) + 1;
			}
			ECPoint kG = curve.multiplyG(k);
			if (kG.isO()) {
				if (flagInputK) {
					return ECDSA_Signature();
				}
				continue;
			}
			r = BigInt::mod(kG.x, curve.n, sl_true);
			if (r.isZero()) {
				if (flagInputK) {
					return ECDSA_Signature();
				}
				continue;
			}
			BigInt k1 = BigInt::inverseMod(k, curve.n);
			s = BigInt::mod(k1 * (z + r * key.d), curve.n, sl_true);
			if (s.isZero()) {
				if (flagInputK) {
					return ECDSA_Signature();
				}
				continue;
			}
			if (!flagInputK) {
				if (_k) {
					*_k = k;
				}
			}
			break;
		}
		return ECDSA_Signature(Move(r), Move(s));
	}

	ECDSA_Signature ECDSA::sign(const EllipticCurve& curve, const ECPrivateKey& key, const void* hash, sl_size size, BigInt* k) noexcept
	{
		return sign(curve, key, ECDSA_MakeZ(curve, hash, size), k);
	}

	ECDSA_Signature ECDSA::sign_SHA256(const EllipticCurve& curve, const ECPrivateKey& key, const void* data, sl_size size, BigInt* k) noexcept
	{
		sl_uint8 hash[SHA256::HashSize];
		SHA256::hash(data, size, hash);
		return sign(curve, key, ECDSA_MakeZ(curve, hash, sizeof(hash)), k);
	}

	ECDSA_Signature ECDSA::sign_SHA384(const EllipticCurve& curve, const ECPrivateKey& key, const void* data, sl_size size, BigInt* k) noexcept
	{
		sl_uint8 hash[SHA384::HashSize];
		SHA384::hash(data, size, hash);
		return sign(curve, key, ECDSA_MakeZ(curve, hash, sizeof(hash)), k);
	}

	ECDSA_Signature ECDSA::sign_SHA512(const EllipticCurve& curve, const ECPrivateKey& key, const void* data, sl_size size, BigInt* k) noexcept
	{
		sl_uint8 hash[SHA512::HashSize];
		SHA512::hash(data, size, hash);
		return sign(curve, key, ECDSA_MakeZ(curve, hash, sizeof(hash)), k);
	}

	sl_bool ECDSA::verify(const EllipticCurve& curve, const ECPublicKey& key, const BigInt& z, const ECDSA_Signature& signature) noexcept
	{
		if (!(key.checkValid(curve))) {
			return sl_false;
		}
		if (signature.r.isZero()) {
			return sl_false;
		}
		if (signature.r >= curve.n) {
			return sl_false;
		}
		if (signature.s.isZero()) {
			return sl_false;
		}
		if (signature.s >= curve.n) {
			return sl_false;
		}
		BigInt s1 = BigInt::inverseMod(signature.s, curve.n);
		BigInt u1 = BigInt::mod(z * s1, curve.n, sl_true);
		BigInt u2 = BigInt::mod(signature.r * s1, curve.n, sl_true);
		ECPoint p1 = curve.multiplyG(u1);
		ECPoint p2 = curve.multiplyPoint(key.Q, u2);
		ECPoint kG = curve.addPoint(p1, p2);
		if (kG.isO()) {
			return sl_false;
		}
		return kG.x == signature.r;
	}

	sl_bool ECDSA::verify(const EllipticCurve& curve, const ECPublicKey& key, const void* hash, sl_size size, const ECDSA_Signature& signature) noexcept
	{
		return verify(curve, key, ECDSA_MakeZ(curve, hash, size), signature);
	}

	sl_bool ECDSA::verify_SHA256(const EllipticCurve& curve, const ECPublicKey& key, const void* data, sl_size size, const ECDSA_Signature& signature) noexcept
	{
		sl_uint8 hash[SHA256::HashSize];
		SHA256::hash(data, size, hash);
		return verify(curve, key, ECDSA_MakeZ(curve, hash, sizeof(hash)), signature);
	}

	sl_bool ECDSA::verify_SHA384(const EllipticCurve& curve, const ECPublicKey& key, const void* data, sl_size size, const ECDSA_Signature& signature) noexcept
	{
		sl_uint8 hash[SHA384::HashSize];
		SHA384::hash(data, size, hash);
		return verify(curve, key, ECDSA_MakeZ(curve, hash, sizeof(hash)), signature);
	}

	sl_bool ECDSA::verify_SHA512(const EllipticCurve& curve, const ECPublicKey& key, const void* data, sl_size size, const ECDSA_Signature& signature) noexcept
	{
		sl_uint8 hash[SHA512::HashSize];
		SHA512::hash(data, size, hash);
		return verify(curve, key, ECDSA_MakeZ(curve, hash, sizeof(hash)), signature);
	}

	BigInt ECDH::getSharedKey(const EllipticCurve& curve, const ECPrivateKey& keyLocal, const ECPublicKey& keyRemote) noexcept
	{
		if (!(keyRemote.checkValid(curve))) {
			return BigInt::null();
		}
		ECPoint pt = curve.multiplyPoint(keyRemote.Q, keyLocal.d);
		return pt.x;
	}


#define DEFINE_CURVE(NAME) \
	const EllipticCurve& EllipticCurve::NAME() noexcept \
	{ \
		SLIB_SAFE_LOCAL_STATIC(Curve_##NAME, ret) \
		SLIB_LOCAL_STATIC_ZERO_INITIALIZED(EllipticCurve, zero) \
		if (SLIB_SAFE_STATIC_CHECK_FREED(ret)) { \
			return zero; \
		} \
		return ret; \
	}

	DEFINE_CURVE(secp112r1)
	DEFINE_CURVE(secp112r2)
	DEFINE_CURVE(secp128r1)
	DEFINE_CURVE(secp128r2)
	DEFINE_CURVE(secp160k1)
	DEFINE_CURVE(secp160r1)
	DEFINE_CURVE(secp160r2)
	DEFINE_CURVE(secp192k1)
	DEFINE_CURVE(secp224k1)
	DEFINE_CURVE(secp256k1)
	DEFINE_CURVE(secp384r1)
	DEFINE_CURVE(secp521r1)

}
