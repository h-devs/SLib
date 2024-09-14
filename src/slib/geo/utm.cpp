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

#include "slib/geo/utm.h"

#include "slib/math/math.h"

#define DEFAULT_SCALE_FACTOR 0.9996

namespace slib
{

	sl_bool UTMCoordinate::equals(const UTMCoordinate& other) const noexcept
	{
		return N == other.N && E == other.E;
	}

	sl_bool UTMCoordinate::isAlmostEqual(const UTMCoordinate& other) const noexcept
	{
		return Math::isAlmostZero(N - other.N) && Math::isAlmostZero(E - other.E);
	}


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(UTM)

	UTM::UTM(): referenceLongitude(0), scaleFactor(DEFAULT_SCALE_FACTOR)
	{
	}

	UTM::UTM(double _referenceLongitude): referenceLongitude(_referenceLongitude), scaleFactor(DEFAULT_SCALE_FACTOR)
	{
	}

	UTM::UTM(double _referenceLongitude, double _scaleFactor): referenceLongitude(_referenceLongitude), scaleFactor(_scaleFactor)
	{
	}

	UTMCoordinate UTM::getCoordinate(const LatLon& latLon) const
	{
		double lat = latLon.latitude;
		double lon = latLon.longitude;
		sl_bool flagSouthern = sl_false;
		if (lat < 0) {
			lat = -lat;
			flagSouthern = sl_true;
		}
		lat = Math::getRadianFromDegrees(lat);
		lon = Math::getRadianFromDegrees(lon - referenceLongitude);
		double a = 6378137.0;
		double f = 1 / 298.257223563;
		double k0 = scaleFactor;
		double n = f / (2.0 - f);
		double n2 = n * n;
		double n3 = n2 * n;
		double n4 = n3 * n;
		double A = a / (1.0 + n) * (1.0 + n2 / 4.0 + n4 / 64.0);
		double A0 = k0 * A;
		double a1 = n / 2.0 - n2 * 2.0 / 3.0 + n3 * 5.0 / 16.0;
		double a2 = n2 * 13.0 / 48.0 - n3 * 3.0 / 5.0;
		double a3 = n3 * 61.0 / 240.0;
		double t_1 = 2.0 * Math::sqrt(n) / (1 + n);
		double lat_s = Math::sin(lat);
		double t = Math::sinh(Math::arctanh(lat_s) - t_1 * Math::arctanh(t_1 * lat_s));
		double p = Math::arctan(t / Math::cos(lon));
		double q = Math::arctanh(Math::sin(lon) / Math::sqrt(1 + t * t));
		double E = 500000.0 + A0 * (q + a1 * Math::cos(2.0 * p) * Math::sinh(2.0 * q) + a2 * Math::cos(4.0 * p) * Math::sinh(4.0 * q) + a3 * Math::cos(6.0 * p) * Math::sinh(6.0 * q));
		double N = A0 * (p + a1 * Math::sin(2.0 * p) * Math::cosh(2.0 * q) + a2 * Math::sin(4.0 * p) * Math::cosh(4.0 * q) + a3 * Math::sin(6.0 * p) * Math::cosh(6.0 * q));
		if (flagSouthern) {
			N += 10000000.0;
		}
		return UTMCoordinate(N, E);
	}

	LatLon UTM::getLatLon(const UTMCoordinate& coord) const
	{
		double E = coord.E;
		double N = coord.N;
		sl_bool flagSouthern = sl_false;
		if (N >= 10000000.0) {
			N -= 10000000.0;
			flagSouthern = sl_true;
		}
		double a = 6378137.0;
		double f = 1 / 298.257223563;
		double k0 = scaleFactor;
		double n = f / (2.0 - f);
		double n2 = n * n;
		double n3 = n2 * n;
		double n4 = n3 * n;
		double A = a / (1.0 + n) * (1.0 + n2 / 4.0 + n4 / 64.0);
		double A0 = k0 * A;
		double b1 = n / 2.0 - n2 * 2.0 / 3.0 + n3 * 37.0 / 96.0;
		double b2 = n2 / 48.0 + n3 / 15.0;
		double b3 = n3 * 17.0 / 480.0;
		double g1 = 2.0 * n - n2 * 2.0 / 3.0 - 2.0 * n3;
		double g2 = n2 * 7.0 / 3.0 - n3 * 8.0 / 5.0;
		double g3 = n3 * 56.0 / 15.0;
		double x = N / A0;
		double y = (E - 500000) / A0;
		double p = x - (b1 * Math::sin(2 * x) * Math::cosh(2 * y) + b2 * Math::sin(4 * x) * Math::cosh(4 * y) + b3 * Math::sin(6 * x) * Math::cosh(6 * y));
		double q = y - (b1 * Math::cos(2 * x) * Math::sinh(2 * y) + b2 * Math::cos(4 * x) * Math::sinh(4 * y) + b3 * Math::cos(6 * x) * Math::sinh(6 * y));
		double z = Math::arcsin(Math::sin(p) / Math::cosh(q));
		double lat = z + g1 * Math::sin(2 * z) + g2 * Math::sin(4 * z) + g3 * Math::sin(6 * z);
		double lon = Math::arctan(Math::sinh(q) / Math::cos(p));
		lat = Math::getDegreesFromRadian(lat);
		if (flagSouthern) {
			lat = -lat;
		}
		lon = referenceLongitude + Math::getDegreesFromRadian(lon);
		return LatLon(lat, lon);
	}

}
