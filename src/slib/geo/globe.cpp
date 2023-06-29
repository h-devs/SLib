/*
 *   Copyright (c) 2008-2018 SLIBIO <https://github.com/SLIBIO>
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

#include "slib/geo/globe.h"

#include "slib/math/math.h"

namespace slib
{

	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(Globe)

	Globe::Globe() = default;

	void Globe::_initializeParameters()
	{
		double f = radiusPolar / radiusEquatorial;
		inverseFlattening = 1 / (1 - f);
		eccentricitySquared = 1 - f * f;
	}

	Globe::Globe(double _radiusEquatorial, double _radiusPolar)
	{
		radiusEquatorial = _radiusEquatorial;
		radiusPolar = _radiusPolar;
		_initializeParameters();
	}

	Globe::Globe(double _radiusEquatorial, double _radiusPolar, double _inverseFlattening, double _eccentricitySquared)
	{
		radiusEquatorial = _radiusEquatorial;
		radiusPolar = _radiusPolar;

		inverseFlattening = _inverseFlattening;
		eccentricitySquared = _eccentricitySquared;
	}

	double Globe::getEquatorialRadius() const
	{
		return radiusEquatorial;
	}

	void Globe::setEquatorialRadius(double radius)
	{
		radiusEquatorial = radius;
		_initializeParameters();
	}

	double Globe::getPolarRadius() const
	{
		return radiusPolar;
	}

	void Globe::setPolarRadius(double radius)
	{
		radiusPolar = radius;
		_initializeParameters();
	}

	double Globe::getInverseFlattening() const
	{
		return inverseFlattening;
	}

	double Globe::getEccentricitySquared() const
	{
		return eccentricitySquared;
	}

	Double3 Globe::getSurfaceNormal(double latitude, double longitude) const
	{
		double _latitude = Math::getRadianFromDegrees(latitude);
		double _longitude = Math::getRadianFromDegrees(longitude);
		Double3 R;
		R.y = (1 - eccentricitySquared) * Math::sin(_latitude) / radiusPolar / radiusPolar;
		double xz = Math::cos(_latitude) / radiusEquatorial / radiusEquatorial;
		R.x = xz * Math::sin(_longitude);
		R.z = -xz * Math::cos(_longitude);
		R.normalize();
		return R;
	}

	Double3 Globe::getSurfaceNormal(const LatLon& latlon) const
	{
		return getSurfaceNormal(latlon.latitude, latlon.longitude);
	}

	Double3 Globe::getSurfaceNormal(const GeoLocation& location) const
	{
		return getSurfaceNormal(location.latitude, location.longitude);
	}

	Double3 Globe::getNorthPointingTangent(double latitude, double longitude) const
	{
		double _latitude = Math::getRadianFromDegrees(latitude);
		double _longitude = Math::getRadianFromDegrees(longitude);

		_latitude = -_latitude;
		Double3 R;
		R.y = Math::cos(_latitude);
		double xz = Math::sin(_latitude);
		R.x = xz * Math::sin(_longitude);
		R.z = -xz * Math::cos(_longitude);
		R.normalize();
		return R;
	}

	Double3 Globe::getNorthPointingTangent(const LatLon& latlon) const
	{
		return getNorthPointingTangent(latlon.latitude, latlon.longitude);
	}

	Double3 Globe::getNorthPointingTangent(const GeoLocation& location) const
	{
		return getNorthPointingTangent(location.latitude, location.longitude);
	}

	Double3 Globe::getSurfaceNormalAtCartesianPosition(double x, double y, double z) const
	{
		double a2 = radiusEquatorial * radiusEquatorial;
		double b2 = radiusPolar * radiusPolar;
		Double3 R;
		R.x = x / a2;
		R.y = y / b2;
		R.z = z / a2;
		R.normalize();
		return R;
	}

	Double3 Globe::getSurfaceNormalAtCartesianPosition(const Double3& position) const
	{
		return getSurfaceNormalAtCartesianPosition(position.x, position.y, position.z);
	}

	Double3 Globe::getCartesianPosition(double latitude, double longitude, double altitude) const
	{
		double _latitude = Math::getRadianFromDegrees(latitude);
		double _longitude = Math::getRadianFromDegrees(longitude);

		double sinLat = Math::sin(_latitude);
		// get radius of vertical in prime meridian
		double rv = radiusEquatorial / Math::sqrt(1 - eccentricitySquared * sinLat * sinLat);
		Double3 B;
		B.y = sinLat * (rv * (1 - eccentricitySquared) + altitude);
		double rxz = Math::cos(_latitude) * (rv + altitude);
		B.x = rxz * Math::sin(_longitude);
		B.z = -rxz * Math::cos(_longitude);
		return B;
	}

	Double3 Globe::getCartesianPosition(const LatLon& latlon) const
	{
		return getCartesianPosition(latlon.latitude, latlon.longitude, 0);
	}

	Double3 Globe::getCartesianPosition(const GeoLocation& location) const
	{
		return getCartesianPosition(location.latitude, location.longitude, location.altitude);
	}

/*
	According to H. Vermeille
 "An analytical method to transform geocentric into geodetic coordinates"
 http://www.springerlink.com/content/3t6837t27t351227/fulltext.pdf
 
	Referenced to NASA WorldWind Java version
 */
	GeoLocation Globe::getGeoLocation(double x, double y, double z) const
	{
		double X = -z;
		double Y = x;
		double Z = y;
		double XXpYY = X * X + Y * Y;
		double sqrtXXpYY = Math::sqrt(XXpYY);

		double a = radiusEquatorial;
		double ra2 = 1 / (a * a);
		double e2 = eccentricitySquared;
		double e4 = e2 * e2;

		// Step 1
		double p = XXpYY * ra2;
		double q = Z * Z * (1 - e2) * ra2;
		double r = (p + q - e4) / 6;

		double h;
		double phi;

		double evoluteBorderTest = 8 * r * r * r + e4 * p * q;
		if (evoluteBorderTest > 0 || q != 0) {

			double u;

			if (evoluteBorderTest > 0) {
				// Step 2: general case
				double rad1 = Math::sqrt(evoluteBorderTest);
				double rad2 = Math::sqrt(e4 * p * q);

				// 10*e2 is my arbitrary decision of what Vermeille means by "near... the cusps of the evolute".
				if (evoluteBorderTest > 10 * e2)
				{
					double rad3 = Math::cbrt((rad1 + rad2) * (rad1 + rad2));
					u = r + 0.5 * rad3 + 2 * r * r / rad3;
				} else {
					u = r + 0.5 * Math::cbrt((rad1 + rad2) * (rad1 + rad2)) + 0.5 * Math::cbrt((rad1 - rad2) * (rad1 - rad2));
				}
			} else {
				// Step 3: near evolute
				double rad1 = Math::sqrt(-evoluteBorderTest);
				double rad2 = Math::sqrt(-8 * r * r * r);
				double rad3 = Math::sqrt(e4 * p * q);
				double atan = 2 * Math::arctan2(rad3, rad1 + rad2) / 3;

				u = -4 * r * Math::sin(atan) * Math::cos(SLIB_PI_LONG / 6 + atan);
			}

			double v = Math::sqrt(u * u + e4 * q);
			double w = e2 * (u + v - q) / (2 * v);
			double k = (u + v) / (Math::sqrt(w * w + u + v) + w);
			double D = k * sqrtXXpYY / (k + e2);
			double sqrtDDpZZ = Math::sqrt(D * D + Z * Z);

			h = (k + e2 - 1) * sqrtDDpZZ / k;
			phi = 2 * Math::arctan2(Z, sqrtDDpZZ + D);

		} else {
			// Step 4: singular disk
			double rad1 = Math::sqrt(1 - e2);
			double rad2 = Math::sqrt(e2 - p);
			double e = Math::sqrt(e2);

			h = -a * rad1 * rad2 / e;
			phi = rad2 / (e * rad2 + rad1 * Math::sqrt(p));
		}

		// Compute lambda
		double lambda;
		double s2 = 1.4142135623730950488016887242097; // Math::sqrt(2.0);
		if ((s2 - 1) * Y < sqrtXXpYY + X) {
			// case 1: -135deg < lambda < 135deg
			lambda = 2 * Math::arctan2(Y, sqrtXXpYY + X);
		} else if (sqrtXXpYY + Y < (s2 + 1) * X) {
			// case 2: -225deg(135deg) < lambda < 45deg
			lambda = -SLIB_PI_LONG * 0.5 + 2 * Math::arctan2(X, sqrtXXpYY - Y);
		} else {
			// if (sqrtXXpYY-Y<(s2=1)*X)  // is the test, if needed, but it's not
			// case 3: 45deg < lambda < 225deg(-135deg)
			lambda = SLIB_PI_LONG * 0.5 - 2 * Math::arctan2(X, sqrtXXpYY + Y);
		}

		return GeoLocation(Math::getDegreesFromRadian(phi), Math::getDegreesFromRadian(lambda), h);
	}

	GeoLocation Globe::getGeoLocation(const Double3& position) const
	{
		return getGeoLocation(position.x, position.y, position.z);
	}


	SphericalGlobe::SphericalGlobe() = default;

	SphericalGlobe::SphericalGlobe(const SphericalGlobe& other) = default;

	SphericalGlobe::SphericalGlobe(double _radius) : radius(_radius)
	{
	}

	SphericalGlobe& SphericalGlobe::operator=(const SphericalGlobe& other) = default;

	double SphericalGlobe::getRadius() const
	{
		return radius;
	}

	void SphericalGlobe::setRadius(double _radius)
	{
		radius = _radius;
	}

	Double3 SphericalGlobe::getSurfaceNormal(double latitude, double longitude) const
	{
		double _latitude = Math::getRadianFromDegrees(latitude);
		double _longitude = Math::getRadianFromDegrees(longitude);
		Double3 R;
		R.y = Math::sin(_latitude);
		double xz = Math::cos(_latitude);
		R.x = xz * Math::sin(_longitude);
		R.z = -xz * Math::cos(_longitude);
		R.normalize();
		return R;
	}

	Double3 SphericalGlobe::getSurfaceNormal(const LatLon& latlon) const
	{
		return getSurfaceNormal(latlon.latitude, latlon.longitude);
	}

	Double3 SphericalGlobe::getSurfaceNormal(const GeoLocation& location) const
	{
		return getSurfaceNormal(location.latitude, location.longitude);
	}

	Double3 SphericalGlobe::getNorthPointingTangent(double latitude, double longitude) const
	{
		double _latitude = Math::getRadianFromDegrees(latitude);
		double _longitude = Math::getRadianFromDegrees(longitude);

		_latitude = -_latitude;
		Double3 R;
		R.y = Math::cos(_latitude);
		double xz = Math::sin(_latitude);
		R.x = xz * Math::sin(_longitude);
		R.z = -xz * Math::cos(_longitude);
		R.normalize();
		return R;
	}

	Double3 SphericalGlobe::getNorthPointingTangent(const LatLon& latlon) const
	{
		return getNorthPointingTangent(latlon.latitude, latlon.longitude);
	}

	Double3 SphericalGlobe::getNorthPointingTangent(const GeoLocation& location) const
	{
		return getNorthPointingTangent(location.latitude, location.longitude);
	}

	Double3 SphericalGlobe::getCartesianPosition(double latitude, double longitude, double altitude) const
	{
		double _latitude = Math::getRadianFromDegrees(latitude);
		double _longitude = Math::getRadianFromDegrees(longitude);

		Double3 B;
		double R = radius + altitude;
		B.y = Math::sin(_latitude) * R;
		double rxz = Math::cos(_latitude) * R;
		B.x = rxz * Math::sin(_longitude);
		B.z = -rxz * Math::cos(_longitude);
		return B;
	}

	Double3 SphericalGlobe::getCartesianPosition(const LatLon& latlon) const
	{
		return getCartesianPosition(latlon.latitude, latlon.longitude, 0);
	}

	Double3 SphericalGlobe::getCartesianPosition(const GeoLocation& location) const
	{
		return getCartesianPosition(location.latitude, location.longitude, location.altitude);
	}

	GeoLocation SphericalGlobe::getGeoLocation(double x, double y, double z) const
	{
		double len = Math::sqrt(x * x + y * y + z * z);
		GeoLocation R;
		R.altitude = len - radius;
		if (Math::isLessThanEpsilon(len)) {
			R.longitude = 0;
			R.latitude = 0;
		} else {
			R.latitude = Math::arcsin(y / len) * 180 / SLIB_PI_LONG;
			double rxz = Math::sqrt(x * x + z * z);
			double _v = Math::arcsin(x / rxz) * 180 / SLIB_PI_LONG;
			if (z > 0) {
				_v = 180 - _v;
			}
			R.longitude = _v;
		}
		return R;
	}

	GeoLocation SphericalGlobe::getGeoLocation(const Double3& position) const
	{
		return getGeoLocation(position.x, position.y, position.z);
	}

}
