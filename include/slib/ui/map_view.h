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

#define SLIB_MAP_VIEW_LAYER_COUNT 5

namespace slib
{

	class MapView;
	class MapViewData;

	template <class T>
	class SLIB_EXPORT MapTileLocationT
	{
	public:
		sl_uint32 level;
		T E; // longitude
		T N; // latitude

	public:
		MapTileLocationT() {}

		MapTileLocationT(sl_uint32 _level, T e, T n)
		{
			level = _level;
			E = e;
			N = n;
		}

		template <class O>
		MapTileLocationT(const MapTileLocationT<O>& other)
		{
			level = other.level;
			E = (T)(other.E);
			N = (T)(other.N);
		}

		template <class O>
		MapTileLocationT& operator=(const MapTileLocationT<O>& other)
		{
			level = other.level;
			E = (T)(other.E);
			N = (T)(other.N);
			return *this;
		}

		sl_bool equals(const MapTileLocationT& other) const
		{
			return level == other.level && E == other.E && N == other.N;
		}

		sl_compare_result compare(const MapTileLocationT& other) const
		{
			sl_compare_result c = ComparePrimitiveValues(level, other.level);
			if (c) {
				return c;
			}
			c = ComparePrimitiveValues(E, other.E);
			if (c) {
				return c;
			}
			c = ComparePrimitiveValues(N, other.N);
			return c;
		}

		SLIB_DEFINE_CLASS_DEFAULT_COMPARE_OPERATORS
	};

	typedef MapTileLocationT<double> MapTileLocation;
	typedef MapTileLocationT<sl_int32> MapTileLocationI;

	template <>
	SLIB_INLINE sl_size Hash<MapTileLocationI>::operator()(const MapTileLocationI& location) const noexcept
	{
		sl_uint64 c = location.level;
		c <<= 29;
		c ^= location.E;
		c <<= 29;
		c ^= location.N;
		return HashPrimitiveValue(c);
	}

	class SLIB_EXPORT MapTileAddress : public MapTileLocationI
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
		double N; // Northing
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

		virtual sl_bool readObject(Ref<CRef>& _out, const MapTileAddress& address, const Function<Ref<CRef>(Memory&)>& loader, sl_uint32 timeout);

	};

	class SLIB_EXPORT MapTileDirectory : public MapTileReader
	{
		SLIB_DECLARE_OBJECT

	protected:
		MapTileDirectory();

		~MapTileDirectory();

	public:
		static Ref<MapTileDirectory> open(const String& rootPath, const Function<String(MapTileLocationI&)>& formator);

	public:
		sl_bool readData(Memory& _out, const MapTileAddress& address, sl_uint32 timeout) override;

	private:
		String m_root;
		Function<String(MapTileLocationI&)> m_formator;
	};

	class SLIB_EXPORT MapTileCache : public Object
	{
		SLIB_DECLARE_OBJECT

	protected:
		MapTileCache();

		~MapTileCache();

	public:
		static Ref<MapTileCache> create(sl_uint32 nMaxActiveCount = 100, sl_uint32 expiringMilliseconds = 10000);

	public:
		sl_uint32 getMaximumActiveCount();

	public:
		virtual sl_uint32 getLastActiveCount() = 0;

		virtual void endStep() = 0;

		virtual sl_bool getObject(const MapTileAddress& address, Ref<CRef>& _out) = 0;

		virtual sl_bool saveObject(const MapTileAddress& address, const Ref<CRef>& object, sl_bool flagEndless = sl_false) = 0;

		virtual void clear() = 0;

	protected:
		sl_uint32 m_nMaxCount;
	};

	class SLIB_EXPORT MapTileLoadParam
	{
	public:
		Ref<MapTileReader> reader;
		Ref<MapTileCache> cache;
		MapTileAddress address;
		sl_uint32 timeout;
		sl_bool flagLoadNow;
		sl_bool flagEndless;

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
		static Ref<MapTileLoader> create(const Function<void()>& onCompleteLazyLoading, sl_uint32 nThreads = 0, sl_uint32 nMaxQueue = 100);

		static Ref<MapTileLoader> create(sl_uint32 nThreads = 0, sl_uint32 nMaxQueue = 100);

	public:
		enum TYPE
		{
			DATA,
			IMAGE,
			OBJECT
		};

		sl_bool loadData(Memory& _out, const MapTileLoadParam& param, const Function<void(Memory&)>& onCompleteLazyLoading);

		sl_bool loadImage(Ref<Image>& _out, const MapTileLoadParam& param, const Function<void(Ref<Image>&)>& onCompleteLazyLoading);

		sl_bool loadObject(Ref<CRef>& _out, const MapTileLoadParam& param, const Function<Ref<CRef>(Memory&)>& loader, const Function<void(Ref<CRef>&)>& onCompleteLazyLoading);

	protected:
		virtual sl_bool load(Ref<CRef>& _out, TYPE type, const MapTileLoadParam& param, const Function<Ref<CRef>(Memory&)>& loader, const Function<void(Ref<CRef>&)>& onCompleteLazyLoading) = 0;

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

		const RectangleT<double>& getViewport();

		void setViewport(const RectangleT<double>& rect);

		Ref<Drawable> getBackground();

		void setBackground(const Ref<Drawable>& background);

	public:
		Double2 getViewPointFromMapLocation(const MapLocation& location);

		MapLocation getMapLocationFromViewPoint(const Double2& point);

		void draw(Canvas* canvas, const Rectangle& rect, MapViewData* data);

	public:
		virtual GeoLocation getEyeLocation() = 0;

		virtual void setEyeLocation(const GeoLocation& location) = 0;

		virtual LatLon getLatLonFromMapLocation(const MapLocation& location) = 0;

		virtual MapLocation getMapLocationFromLatLon(const LatLon& location) = 0;

		virtual void clearCache() = 0;

	protected:
		virtual void onDraw(Canvas* canvas, const Rectangle& rect, MapViewData* data) = 0;

	protected:
		MapLocation m_center;
		MapRange m_range;
		double m_scale;
		double m_scaleMin;
		double m_scaleMax;
		RectangleT<double> m_viewport;
		AtomicRef<Drawable> m_background;
	};

	class SLIB_EXPORT MapSurfaceParam
	{
	public:
		sl_uint32 baseLevel;
		sl_uint32 maxLevel;
		sl_uint32 baseTileCountE; // Easting
		sl_uint32 baseTileCountN; // Northing
		double degreeLengthE; // Easting
		double degreeLengthN; // Northing
		sl_uint32 tileLength; // Pixels
		Function<void(MapTileLocationI&)> toReaderLocation;

		Ref<MapTileReader> picture;
		Ref<MapTileReader> dem;
		Ref<MapTileReader> layers[SLIB_MAP_VIEW_LAYER_COUNT];

	public:
		MapSurfaceParam();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(MapSurfaceParam)

	};

	class SLIB_EXPORT MapSurface : public MapPlane
	{
		SLIB_DECLARE_OBJECT

	protected:
		MapSurface();

		~MapSurface();

	public:
		static Ref<MapSurface> create(const MapSurfaceParam& param);

	public:
		sl_uint32 getBaseLevel();

		sl_uint32 getMaximumLevel();

		sl_uint32 getBaseTileCountE();

		sl_uint32 getBaseTileCountN();

		double getDegreeLengthE();

		double getDegreeLengthN();

		sl_uint32 getTileLength();

		Ref<MapTileReader> getPictureReader();

		void setPictureReader(const Ref<MapTileReader>& reader);

		Ref<MapTileReader> getDemReader();

		void setDemReader(const Ref<MapTileReader>& reader);

		Ref<MapTileReader> getLayerReader(sl_uint32 layer);

		void setLayerReader(sl_uint32 layer, const Ref<MapTileReader>& reader);

		sl_bool isLayerVisible(sl_uint32 layer);

		void setLayerVisible(sl_uint32 layer, sl_bool flag = sl_true);

		float getLayerOpacity(sl_uint32 layer);

		void setLayerOpacity(sl_uint32 layer, float opacity);

		GeoLocation getEyeLocation() override;

		void setEyeLocation(const GeoLocation& location) override;

		LatLon getLatLonFromMapLocation(const MapLocation& location) override;

		MapLocation getMapLocationFromLatLon(const LatLon& location) override;

		// Normalized tile location (No reader location)
		LatLon getLatLonFromTileLocation(const MapTileLocationI& location);

		// Normalized tile location (No reader location)
		MapTileLocation getTileLocationFromLatLon(sl_uint32 level, const LatLon& latLon);
		
		// Normalized tile location (No reader location)
		MapLocation getMapLocationFromTileLocation(const MapTileLocationI& location);

		// Normalized tile location (No reader location)
		MapTileLocation getTileLocationFromMapLocation(sl_uint32 level, const MapLocation& location);

		MapTileLocationI getReaderLocation(const MapTileLocationI& location);

	protected:
		sl_uint32 m_maxLevel;
		sl_uint32 m_baseLevel;
		sl_uint32 m_baseTileCountE;
		sl_uint32 m_baseTileCountN;
		double m_degreeLengthE;
		double m_degreeLengthN;
		sl_uint32 m_tileLength; // Pixels
		Function<void(MapTileLocationI&)> m_toReaderLocation;

		AtomicRef<MapTileReader> m_readerPicture;
		AtomicRef<MapTileReader> m_readerDEM;
		struct Layer
		{
			AtomicRef<MapTileReader> reader;
			sl_bool flagVisible;
			float opacity;
		};
		Layer m_layers[SLIB_MAP_VIEW_LAYER_COUNT];
	};

	class SLIB_EXPORT MapViewObject : public Object
	{
		SLIB_DECLARE_OBJECT

	protected:
		MapViewObject();

		~MapViewObject();

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

		List< Ref<MapViewObject> > getObjects() const;

		Ref<MapViewObject> getObject(const String& key) const;

		void putObject(const String& name, const Ref<MapViewObject>& object, UIUpdateMode mode = UIUpdateMode::Redraw);

		GeoLocation getEyeLocation() const;

		void setEyeLocation(const GeoLocation& location, UIUpdateMode mode = UIUpdateMode::Redraw);

		sl_bool getLatLonFromViewPoint(const Double2& point, LatLon& _out) const;

		Double2 getViewPointFromLatLon(const LatLon& location) const;

		void resize(double width, double height, UIUpdateMode mode = UIUpdateMode::Redraw);

		void movePlane(double dx, double dy, UIUpdateMode mode = UIUpdateMode::Redraw);

		void zoom(double scale, UIUpdateMode mode = UIUpdateMode::Redraw);

		void zoomAt(const Double2& point, double scale, UIUpdateMode mode = UIUpdateMode::Redraw);

		void click(const Double2& pt, UIUpdateMode mode = UIUpdateMode::Redraw);

		void stopMoving();

	public:
		MapTileLoader* getTileLoader();

		void drawPlane(Canvas* canvas, const Rectangle& rect);

		void renderGlobe(RenderEngine* engine);

		void invalidate(UIUpdateMode mode = UIUpdateMode::Redraw);

	protected:
		void _resizePlane(MapPlane* plane, double width, double height);

		void _onCompleteLazyLoading();

	protected:
		Mutex m_lock;
		View* m_view;
		sl_bool m_flagGlobeMode;
		Ref<MapPlane> m_plane;
		Ref<MapSurface> m_surface;
		CHashMap< String, Ref<MapViewObject> > m_objects;

		GeoLocation m_eyeLocation;
		double m_width;
		double m_height;

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

		void onResize(sl_ui_len width, sl_ui_len height) override;

	protected:
		sl_uint32 m_nLastTouches;
		Point m_ptLastEvent;
		sl_bool m_flagMouseDown;
		Point m_ptMouseDown;
		sl_uint64 m_tickMouseDown;
		sl_bool m_flagClicking;
		sl_bool m_flagThrowMoving;
	};

}

#endif