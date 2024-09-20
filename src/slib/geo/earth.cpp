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

#include "slib/geo/earth.h"

namespace slib
{

	namespace
	{
		struct EarthGlobe
		{
			double radiusEquatorial;
			double radiusPolar;

			double inverseFlattening;
			double eccentricitySquared;
		};

		static EarthGlobe g_earthGlobe = {SLIB_GEO_EARTH_RADIUS_EQUATORIAL_WGS84, SLIB_GEO_EARTH_RADIUS_POLAR_WGS84, SLIB_GEO_EARTH_INVERSE_FLATTENING_WGS84, SLIB_GEO_EARTH_ECCENTRICITY_SQUARED_WGS84};

		static const Globe& g_globe = *((Globe*)((void*)&g_earthGlobe));
	}

	const Globe& Earth::getGlobe()
	{
		return g_globe;
	}

	double Earth::getAverageRadius()
	{
		return SLIB_GEO_EARTH_AVERAGE_RADIUS;
	}

	double Earth::getEquatorialRadius()
	{
		return SLIB_GEO_EARTH_RADIUS_EQUATORIAL_WGS84;
	}

	double Earth::getPolarRadius()
	{
		return SLIB_GEO_EARTH_RADIUS_POLAR_WGS84;
	}

	Double3 Earth::getSurfaceNormal(double latitude, double longitude)
	{
		return g_globe.getSurfaceNormal(latitude, longitude);
	}

	Double3 Earth::getSurfaceNormal(const LatLon& latlon)
	{
		return g_globe.getSurfaceNormal(latlon.latitude, latlon.longitude);
	}

	Double3 Earth::getSurfaceNormal(const GeoLocation& location)
	{
		return g_globe.getSurfaceNormal(location.latitude, location.longitude);
	}

	Double3 Earth::getNorthPointingTangent(double latitude, double longitude)
	{
		return g_globe.getNorthPointingTangent(latitude, longitude);
	}

	Double3 Earth::getNorthPointingTangent(const LatLon& latlon)
	{
		return g_globe.getNorthPointingTangent(latlon.latitude, latlon.longitude);
	}

	Double3 Earth::getNorthPointingTangent(const GeoLocation& location)
	{
		return g_globe.getNorthPointingTangent(location.latitude, location.longitude);
	}

	Double3 Earth::getSurfaceNormalAtCartesianPosition(double x, double y, double z)
	{
		return g_globe.getSurfaceNormalAtCartesianPosition(x, y, z);
	}

	Double3 Earth::getSurfaceNormalAtCartesianPosition(const Double3& position)
	{
		return g_globe.getSurfaceNormalAtCartesianPosition(position.x, position.y, position.z);
	}

	Double3 Earth::getCartesianPosition(double latitude, double longitude, double altitude)
	{
		return g_globe.getCartesianPosition(latitude, longitude, altitude);
	}

	Double3 Earth::getCartesianPosition(const LatLon& latlon)
	{
		return g_globe.getCartesianPosition(latlon.latitude, latlon.longitude, 0);
	}

	Double3 Earth::getCartesianPosition(const GeoLocation& location)
	{
		return g_globe.getCartesianPosition(location.latitude, location.longitude, location.altitude);
	}

	GeoLocation Earth::getGeoLocation(double x, double y, double z)
	{
		return g_globe.getGeoLocation(x, y, z);
	}

	GeoLocation Earth::getGeoLocation(const Double3& position)
	{
		return g_globe.getGeoLocation(position.x, position.y, position.z);
	}


	namespace
	{
		struct EarthSphericalGlobe
		{
			double radius;
		};

		static EarthSphericalGlobe g_earthSphericalGlobe = {SLIB_GEO_EARTH_RADIUS_EQUATORIAL_WGS84};

		static const SphericalGlobe& g_sphericalGlobe = *((SphericalGlobe*)((void*)&g_earthSphericalGlobe));
	}

	const SphericalGlobe& SphericalEarth::getGlobe()
	{
		return g_sphericalGlobe;
	}

	double SphericalEarth::getRadius()
	{
		return SLIB_GEO_EARTH_RADIUS_EQUATORIAL_WGS84;
	}

	Double3 SphericalEarth::getSurfaceNormal(double latitude, double longitude)
	{
		return g_sphericalGlobe.getSurfaceNormal(latitude, longitude);
	}

	Double3 SphericalEarth::getSurfaceNormal(const LatLon& latlon)
	{
		return g_sphericalGlobe.getSurfaceNormal(latlon.latitude, latlon.longitude);
	}

	Double3 SphericalEarth::getSurfaceNormal(const GeoLocation& location)
	{
		return g_sphericalGlobe.getSurfaceNormal(location.latitude, location.longitude);
	}

	Double3 SphericalEarth::getNorthPointingTangent(double latitude, double longitude)
	{
		return g_sphericalGlobe.getNorthPointingTangent(latitude, longitude);
	}

	Double3 SphericalEarth::getNorthPointingTangent(const LatLon& latlon)
	{
		return g_sphericalGlobe.getNorthPointingTangent(latlon.latitude, latlon.longitude);
	}

	Double3 SphericalEarth::getNorthPointingTangent(const GeoLocation& location)
	{
		return g_sphericalGlobe.getNorthPointingTangent(location.latitude, location.longitude);
	}

	Double3 SphericalEarth::getCartesianPosition(double latitude, double longitude, double altitude)
	{
		return g_sphericalGlobe.getCartesianPosition(latitude, longitude, altitude);
	}

	Double3 SphericalEarth::getCartesianPosition(const LatLon& latlon)
	{
		return g_sphericalGlobe.getCartesianPosition(latlon.latitude, latlon.longitude, 0);
	}

	Double3 SphericalEarth::getCartesianPosition(const GeoLocation& location)
	{
		return g_sphericalGlobe.getCartesianPosition(location.latitude, location.longitude, location.altitude);
	}

	GeoLocation SphericalEarth::getGeoLocation(double x, double y, double z)
	{
		return g_sphericalGlobe.getGeoLocation(x, y, z);
	}

	GeoLocation SphericalEarth::getGeoLocation(const Double3& position)
	{
		return g_sphericalGlobe.getGeoLocation(position.x, position.y, position.z);
	}

}
