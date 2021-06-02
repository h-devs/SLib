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

#ifndef CHECKHEADER_SLIB_GEO_GEOLOCATION
#define CHECKHEADER_SLIB_GEO_GEOLOCATION

#include "latlon.h"

namespace slib
{
	
	class SLIB_EXPORT GeoLocation
	{
	public:
		double latitude;
		double longitude;
		double altitude; // Unit: m
	
	public:
		constexpr GeoLocation(): latitude(0), longitude(0), altitude(0) {}

		constexpr GeoLocation(double _latitude, double _longitude, double _altitude): latitude(_latitude), longitude(_longitude), altitude(_altitude) {}

		constexpr GeoLocation(const LatLon& latlon, double _altitude): latitude(latlon.latitude), longitude(latlon.longitude), altitude(_altitude) {}
		
		SLIB_DEFINE_CLASS_DEFAULT_MEMBERS_INLINE(GeoLocation)

	public:
		GeoLocation& operator=(const LatLon& other) noexcept
		{
			latitude = other.latitude;
			longitude = other.longitude;
			altitude = 0;
			return *this;
		}

		sl_bool equals(const GeoLocation& other) const noexcept;

		sl_bool isAlmostEqual(const GeoLocation& other) const noexcept;
	
	public:
		LatLon& getLatLon() noexcept;

		const LatLon& getLatLon() const noexcept;

		void setLatLon(const LatLon& v) noexcept;

		void setLatLon(double latitude, double longitude) noexcept;
	
		void normalize() noexcept;
	
		GeoLocation lerp(const GeoLocation& target, float factor) const noexcept;

	};
	
}

#endif
