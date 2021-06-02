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

#ifndef CHECKHEADER_SLIB_GEO_LATLON
#define CHECKHEADER_SLIB_GEO_LATLON

#include "definition.h"

#include "../core/default_members.h"

namespace slib
{
	
	class SLIB_EXPORT LatLon
	{
	public:
		double latitude;
		double longitude;

	public:
		constexpr LatLon(): latitude(0), longitude(0) {}

		constexpr LatLon(double _latitude, double _longitude): latitude(_latitude), longitude(_longitude) {}
		
		SLIB_DEFINE_CLASS_DEFAULT_MEMBERS_INLINE(LatLon)

	public:
		sl_bool equals(const LatLon& other) const noexcept;

		sl_bool isAlmostEqual(const LatLon& other) const noexcept;
	
	public:
		static LatLon getCenter(const LatLon* list, sl_size count) noexcept;

		static double normalizeLatitude(double latitude) noexcept;

		static double normalizeLongitude(double longitude) noexcept;

		void normalize() noexcept;

		LatLon lerp(const LatLon& target, float factor) const noexcept;

	};
	
}

#endif
