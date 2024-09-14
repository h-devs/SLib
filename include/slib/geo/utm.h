/*
*   Copyright (c) 2008-2023 SLIBIO <https://github.com/SLIBIO>
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

#ifndef CHECKHEADER_SLIB_GEO_UTM
#define CHECKHEADER_SLIB_GEO_UTM

#include "latlon.h"

// Universal Transverse Mercator coordinate system

namespace slib
{

	class SLIB_EXPORT UTMCoordinate
	{
	public:
		double N; // distance from equator (+10000000 for southern hemisphere), unit: m
		double E; // distance from reference meridian + 500000, unit: m

	public:
		SLIB_CONSTEXPR UTMCoordinate(): N(0), E(0) {}

		SLIB_CONSTEXPR UTMCoordinate(double n, double e): N(n), E(e) {}

		SLIB_DEFINE_CLASS_DEFAULT_MEMBERS_INLINE(UTMCoordinate)

	public:
		sl_bool equals(const UTMCoordinate& other) const noexcept;

		sl_bool isAlmostEqual(const UTMCoordinate& other) const noexcept;

	};

	class SLIB_EXPORT UTM
	{
	public:
		double referenceLongitude; // reference meridian of longitude
		double scaleFactor;

	public:
		UTM();

		UTM(double referenceLongitude);

		UTM(double referenceLongitude, double scaleFactor);

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(UTM)

	public:
		UTMCoordinate getCoordinate(const LatLon& latLon) const;

		LatLon getLatLon(const UTMCoordinate& coord) const;

	};

}

#endif
