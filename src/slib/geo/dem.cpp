/*
 *   Copyright (c) 2008-2024 SLIBIO <https://github.com/SLIBIO>
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

#include "slib/geo/dem.h"

#include "slib/core/mio.h"

namespace slib
{

	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(DEM)

	DEM::DEM(): pixels(sl_null), N(0)
	{
	}

	sl_bool DEM::initialize(sl_uint32 n)
	{
		sl_uint32 m = n * n;
		Array<float> array = Array<float>::create(m);
		if (array.isNull()) {
			return sl_false;
		}
		float* p = array.getData();
		for (sl_uint32 i = 0; i < m; i++) {
			p[i] = 0;
		}
		pixels = p;
		N = n;
		data = Move(array);
		return sl_true;
	}

	sl_bool DEM::initialize(DataType type, const void* data, sl_size size, sl_uint32 n, sl_bool flagFlipY)
	{
		switch (type) {
			case DataType::FloatBE:
				return initializeFromFloatBE(data, size, n, flagFlipY);
			case DataType::FloatLE:
				return initializeFromFloatLE(data, size, n, flagFlipY);
			case DataType::Int16BE:
				return initializeFromInt16BE(data, size, n, flagFlipY);
			case DataType::Int16LE:
				return initializeFromInt16LE(data, size, n, flagFlipY);
			default:
				break;
		}
		return sl_false;
	}

	namespace
	{
		template <class READER, sl_uint32 ELEMENT_SIZE>
		static sl_bool InitializeDEM(DEM& dem, const void* _d, sl_size size, sl_uint32 n, sl_bool flagFlipY)
		{
			if (!size) {
				return sl_false;
			}
			if (!n) {
				n = Math::sqrt((sl_uint32)size / ELEMENT_SIZE);
			}
			sl_uint32 m = n * n;
			if (size != m * ELEMENT_SIZE) {
				return sl_false;
			}
			Array<float> array = Array<float>::create(m);
			if (array.isNull()) {
				return sl_false;
			}
			float* p = array.getData();
			float* f = p;
			sl_uint8* d = (sl_uint8*)_d;
			if (flagFlipY) {
				float* row = p + ((n * n) - n);
				for (sl_uint32 y = 0; y < n; y++) {
					f = row;
					for (sl_uint32 x = 0; x < n; x++) {
						READER::readElement(*f, d);
						d += ELEMENT_SIZE;
						f++;
					}
					row -= n;
				}
			} else {
				for (sl_uint32 y = 0; y < n; y++) {
					for (sl_uint32 x = 0; x < n; x++) {
						READER::readElement(*f, d);
						d += ELEMENT_SIZE;
						f++;
					}
				}
			}
			dem.pixels = p;
			dem.N = n;
			dem.data = Move(array);
			return sl_true;
		}

		class ReadFloatLE
		{
		public:
			SLIB_INLINE static void readElement(float& f, sl_uint8* d)
			{
				f = MIO::readFloatLE(d);
			}
		};

		class ReadFloatBE
		{
		public:
			SLIB_INLINE static void readElement(float& f, sl_uint8* d)
			{
				f = MIO::readFloatBE(d);
			}
		};

		class ReadInt16LE
		{
		public:
			SLIB_INLINE static void readElement(float& f, sl_uint8* d)
			{
				f = (float)(MIO::readInt16LE(d));
			}
		};

		class ReadInt16BE
		{
		public:
			SLIB_INLINE static void readElement(float& f, sl_uint8* d)
			{
				f = (float)(MIO::readInt16BE(d));
			}
		};
	}

	sl_bool DEM::initializeFromFloatLE(const void* d, sl_size size, sl_uint32 n, sl_bool flagFlipY)
	{
		return InitializeDEM<ReadFloatLE, 4>(*this, d, size, n, flagFlipY);
	}

	sl_bool DEM::initializeFromFloatBE(const void* d, sl_size size, sl_uint32 n, sl_bool flagFlipY)
	{
		return InitializeDEM<ReadFloatBE, 4>(*this, d, size, n, flagFlipY);
	}

	sl_bool DEM::initializeFromInt16LE(const void* d, sl_size size, sl_uint32 n, sl_bool flagFlipY)
	{
		return InitializeDEM<ReadInt16LE, 2>(*this, d, size, n, flagFlipY);
	}

	sl_bool DEM::initializeFromInt16BE(const void* d, sl_size size, sl_uint32 n, sl_bool flagFlipY)
	{
		return InitializeDEM<ReadInt16BE, 2>(*this, d, size, n, flagFlipY);
	}

	void DEM::scale(float* _out, sl_uint32 nOutput, const Rectangle& rcSource) const
	{
		sl_uint32 M = nOutput;
		if (M <= 1) {
			return;
		}
		float mx0 = rcSource.left * (float)(N - 1);
		float my0 = rcSource.top * (float)(N - 1);
		float mx1 = rcSource.right * (float)(N - 1);
		float my1 = rcSource.bottom * (float)(N - 1);
		float dmx = mx1 - mx0;
		float dmy = my1 - my0;
		float* po = _out;
		for (sl_uint32 y = 0; y < M; y++) {
			for (sl_uint32 x = 0; x < M; x++) {
				float altitude;
				if (N >= 2) {
					float mx = mx0 + dmx * (float)x / (float)(M - 1);
					float my = my0 + dmy * (float)y / (float)(M - 1);
					sl_int32 mxi = (sl_int32)(mx);
					sl_int32 myi = (sl_int32)(my);
					float mxf;
					float myf;
					if (mxi < 0) {
						mxi = 0;
						mxf = 0.0f;
					} else if (mxi >= (sl_int32)N - 1) {
						mxi = N - 2;
						mxf = 1.0f;
					} else {
						mxf = mx - mxi;
					}
					if (myi < 0) {
						myi = 0;
						myf = 0.0f;
					} else if (myi >= (sl_int32)N - 1) {
						myi = N - 2;
						myf = 1.0f;
					} else {
						myf = my - myi;
					}
					sl_int32 p = mxi + myi * N;
					altitude = (1.0f - mxf) * (1.0f - myf) * pixels[p] + (1.0f - mxf) * myf * pixels[p + N] + mxf * (1.0f - myf) * pixels[p + 1] + mxf * myf * pixels[p + 1 + N];
				} else {
					altitude = 0;
				}
				*po = altitude;
				po++;
			}
		}
	}

	float DEM::getAltitudeAt(float x, float y)
	{
		if (!N) {
			return 0;
		}
		if (N == 1) {
			return pixels[0];
		}
		float mx = x * (float)(N - 1);
		float my = y * (float)(N - 1);
		sl_int32 mxi = (sl_int32)(mx);
		sl_int32 myi = (sl_int32)(my);
		float mxf;
		float myf;
		if (mxi < 0) {
			mxi = 0;
			mxf = 0.0f;
		} else if (mxi >= (sl_int32)N - 1) {
			mxi = N - 2;
			mxf = 1.0f;
		} else {
			mxf = mx - mxi;
		}
		if (myi < 0) {
			myi = 0;
			myf = 0.0f;
		} else if (myi >= (sl_int32)N - 1) {
			myi = N - 2;
			myf = 1.0f;
		} else {
			myf = my - myi;
		}
		sl_int32 p = mxi + myi * N;
		return (1.0f - mxf) * (1.0f - myf) * pixels[p] + (1.0f - mxf) * myf * pixels[p + N] + mxf * (1.0f - myf) * pixels[p + 1] + mxf * myf * pixels[p + 1 + N];
	}

}
