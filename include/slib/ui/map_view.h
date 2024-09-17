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

#ifndef CHECKHEADER_SLIB_UI_MAP_VIEW
#define CHECKHEADER_SLIB_UI_MAP_VIEW

#include "render_view.h"

#include "../geo/geo_location.h"

namespace slib
{

	class MapView;
	class MapViewData;

	template <class T>
	class SLIB_EXPORT MapTileLocationT
	{
	public:
		sl_uint32 level;
		T y; // latitude
		T x; // longitude

	public:
		MapTileLocationT() {}

		MapTileLocationT(sl_uint32 _level, T _y, T _x)
		{
			level = _level;
			y = _y;
			x = _x;
		}

		template <class O>
		MapTileLocationT(const MapTileLocationT<O>& other)
		{
			level = other.level;
			y = (T)(other.y);
			x = (T)(other.x);
		}

		template <class O>
		MapTileLocationT& operator=(const MapTileLocationT<O>& other)
		{
			level = other.level;
			y = (T)(other.y);
			x = (T)(other.x);
			return *this;
		}

		sl_bool equals(const MapTileLocationT& other) const
		{
			return level == other.level && y == other.y && x == other.x;
		}

		sl_compare_result compare(const MapTileLocationT& other) const
		{
			sl_compare_result c = ComparePrimitiveValues(level, other.level);
			if (c) {
				return c;
			}
			c = ComparePrimitiveValues(x, other.x);
			if (c) {
				return c;
			}
			c = ComparePrimitiveValues(y, other.y);
			return c;
		}

		SLIB_DEFINE_CLASS_DEFAULT_COMPARE_OPERATORS
	};

	typedef MapTileLocationT<double> MapTileLocation;
	typedef MapTileLocationT<sl_int32> MapTileLocationi;

	template <>
	SLIB_INLINE sl_size Hash<MapTileLocationi>::operator()(const MapTileLocationi& location) const noexcept
	{
		sl_uint64 c = location.level;
		c <<= 29;
		c ^= location.y;
		c <<= 29;
		c ^= location.x;
		return HashPrimitiveValue(c);
	}

	class SLIB_EXPORT MapTileAddress : public MapTileLocationi
	{
	public:
		String subPath;

	public:
		MapTileAddress();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(MapTileAddress)
		SLIB_DECLARE_CLASS_COMPARE_HASH_MEMBERS(MapTileAddress)
	};

	// bottom < top
	class SLIB_EXPORT MapRange
	{
	public:
		double left;
		double bottom;
		double right;
		double top;
	};

	class SLIB_EXPORT MapLocation
	{
	public:
		double E; // Easting
		double N; // northing
	};

	class SLIB_EXPORT MapTileReader : public Object
	{
		SLIB_DECLARE_OBJECT

	protected:
		MapTileReader();

		~MapTileReader();

	public:
		virtual sl_bool readData(Memory& _out, const MapTileAddress& address, sl_uint32 timeout) = 0;

		virtual sl_bool readImage(Ref<Image>& _out, const MapTileAddress& address, sl_uint32 timeout);

	};

	class SLIB_EXPORT MapTileDirectory : public MapTileReader
	{
		SLIB_DECLARE_OBJECT

	protected:
		MapTileDirectory();

		~MapTileDirectory();

	public:
		static Ref<MapTileDirectory> open(const String& rootPath, const String& pathFormat);

	public:
		sl_bool readData(Memory& _out, const MapTileAddress& address, sl_uint32 timeout) override;

	private:
		String m_root;
		String m_format;
	};

	class SLIB_EXPORT MapTileCache : public Object
	{
		SLIB_DECLARE_OBJECT

	protected:
		MapTileCache();

		~MapTileCache();

	public:
		static Ref<MapTileCache> create(sl_uint32 nMaxCount = 1000, sl_uint32 expiringMilliseconds = 30000);

	public:
		virtual void startDrawing() = 0;

		virtual void endDrawing() = 0;

		virtual sl_bool getObject(const MapTileAddress& address, Ref<CRef>& _out) = 0;

		virtual void saveObject(const MapTileAddress& address, const Ref<CRef>& object) = 0;

	};

	class SLIB_EXPORT MapTileLoadParam
	{
	public:
		Ref<MapTileReader> reader;
		Ref<MapTileCache> cache;
		MapTileAddress address;
		sl_uint32 timeout;
		sl_bool flagLoadNow;

	public:
		MapTileLoadParam();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(MapTileLoadParam)

	};

	class SLIB_EXPORT MapTileLoader : public Object
	{
		SLIB_DECLARE_OBJECT

	protected:
		MapTileLoader();

		~MapTileLoader();

	public:
		static Ref<MapTileLoader> create(sl_uint32 nThreads = 0, sl_uint32 nMaxQueue = 100);

	public:
		sl_bool loadData(Memory& _out, const MapTileLoadParam& param, const Function<void(Memory&)>& onComplete);

		sl_bool loadImage(Ref<Image>& _out, const MapTileLoadParam& param, const Function<void(Ref<Image>&)>& onComplete);

		sl_bool loadObject(Ref<CRef>& _out, const MapTileLoadParam& param, const Function<Ref<CRef>(Memory&)>& loader, const Function<void(Ref<CRef>&)>& onComplete);

	protected:
		virtual sl_bool load(Ref<CRef>& _out, const MapTileLoadParam& param, sl_bool flagImage, const Function<Ref<CRef>(Memory&)>& loader, const Function<void(Ref<CRef>&)>& onComplete) = 0;

	};

	class SLIB_EXPORT MapPlane : public Object
	{
		SLIB_DECLARE_OBJECT

	public:
		MapPlane();

		~MapPlane();

	public:
		const MapLocation& getCenterLocation();

		void setCenterLocation(double E, double N);

		const MapRange& getMapRange();

		void setMapRange(const MapRange& rect);

		// Reduced Scale
		double getScale();

		// Reduced Scale
		void setScale(double scale);

		double getMinimumScale();

		void setMinimumScale(double scale);

		double getMaximumScale();

		void setMaximumScale(double scale);

		Ref<Drawable> getBackground();

		void setBackground(const Ref<Drawable>& background);

	public:
		Double2 toViewport(const MapLocation& location);

		MapLocation fromViewport(const Double2& point);

		void draw(Canvas* canvas, const Rectangle& rect, MapViewData* data);

	public:
		virtual GeoLocation getEyeLocation() = 0;

		virtual void setEyeLocation(const GeoLocation& location) = 0;

		virtual LatLon toLatLon(const MapLocation& location) = 0;

		virtual MapLocation fromLatLon(const LatLon& location) = 0;

	protected:
		virtual void onDraw(Canvas* canvas, const Rectangle& rect, MapViewData* data) = 0;

	protected:
		MapLocation m_center;
		MapRange m_range;
		double m_scale;
		double m_scaleMin;
		double m_scaleMax;
		AtomicRef<Drawable> m_background;
	};

	class SLIB_EXPORT MapSurface : public MapPlane
	{
		SLIB_DECLARE_OBJECT

	protected:
		MapSurface();

		~MapSurface();

	public:

	};

	class SLIB_EXPORT MapLayer : public Object
	{
		SLIB_DECLARE_OBJECT

	protected:
		MapLayer();

		~MapLayer();

	public:
		sl_bool isSupportingGlobeMode();

		void setSupportingGlobeMode(sl_bool flag = sl_true);

		sl_bool isSupportingPlaneMode();

		void setSupportingPlaneMode(sl_bool flag = sl_true);

	public:
		virtual void draw(Canvas* canvas, const Rectangle& rect, MapViewData* data, MapPlane* plane);

	protected:
		sl_bool m_flagSupportGlobe;
		sl_bool m_flagSupportPlane;

		friend class MapView;
	};

	class SLIB_EXPORT MapViewData
	{
	public:
		MapViewData();

		~MapViewData();

	public:
		sl_bool isGlobeMode() const;

		void setGlobeMode(sl_bool flag = sl_true, UIUpdateMode mode = UIUpdateMode::Redraw);
		
		Ref<MapPlane> getPlane() const;

		void setPlane(const Ref<MapPlane>& plane, UIUpdateMode mode = UIUpdateMode::Redraw);

		Ref<MapSurface> getSurface() const;

		void setSurface(const Ref<MapSurface>& surface, UIUpdateMode mode = UIUpdateMode::Redraw);

		List< Ref<MapLayer> > getLayers() const;

		void putLayer(const String& name, const Ref<MapLayer>& layer, UIUpdateMode mode = UIUpdateMode::Redraw);

		GeoLocation getEyeLocation() const;

		void setEyeLocation(const GeoLocation& location, UIUpdateMode mode = UIUpdateMode::Redraw);

		sl_bool toLatLon(const Double2& point, LatLon& _out) const;

		Double2 toViewport(const LatLon& location) const;

		void zoom(double scale, UIUpdateMode mode = UIUpdateMode::Redraw);

	public:
		MapTileLoader* getTileLoader();

		void drawPlane(Canvas* canvas, const Rectangle& rect);

		void renderGlobe(RenderEngine* engine);

		void invalidate(UIUpdateMode mode = UIUpdateMode::Redraw);

	protected:
		MapPlane* _getPlane() const;

	protected:
		Mutex m_lock;
		View* m_view;
		sl_bool m_flagGlobeMode;
		Ref<MapPlane> m_plane;
		Ref<MapSurface> m_surface;
		CHashMap< String, Ref<MapLayer> > m_layers;

		GeoLocation m_eyeLocation;

		Ref<MapTileLoader> m_tileLoader;

		friend class MapPlaneRenderer;
		friend class MapGlobeRenderer;
	};

	class SLIB_EXPORT MapView : public RenderView, public MapViewData
	{
		SLIB_DECLARE_OBJECT

	public:
		MapView();

		~MapView();

	public:
		using RenderView::invalidate;

	protected:
		void onDraw(Canvas* canvas) override;

		void onFrame(RenderEngine* engine) override;

		void onMouseEvent(UIEvent* ev) override;

		void onMouseWheelEvent(UIEvent* ev) override;

	};

}

#endif