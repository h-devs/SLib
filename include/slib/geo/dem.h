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

#ifndef CHECKHEADER_SLIB_GEO_DEM
#define CHECKHEADER_SLIB_GEO_DEM

#include "definition.h"

#include "../math/rectangle.h"
#include "../core/array.h"
#include "../core/default_members.h"

// Digital elevation model

namespace slib
{

	class SLIB_EXPORT DEM
	{
	public:
		float* pixels;
		sl_uint32 N;
		Array<float> data;

	public:
		DEM();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(DEM)

	public:
		sl_bool initialize(sl_uint32 N);

		sl_bool initializeFromFloatLE(sl_uint32 N, const void* data, sl_size size);

		sl_bool initializeFromFloatBE(sl_uint32 N, const void* data, sl_size size);

		sl_bool initializeFromInt16LE(sl_uint32 N, const void* data, sl_size size);

		sl_bool initializeFromInt16BE(sl_uint32 N, const void* data, sl_size size);

		void scale(float* _out, sl_uint32 N, const Rectangle& rcSource) const;

		float getAltitudeAt(float x, float y);

	};

}

#endif
