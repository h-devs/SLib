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

#ifndef CHECKHEADER_SLIB_UI_MAP_VIEW_EXT
#define CHECKHEADER_SLIB_UI_MAP_VIEW_EXT

#include "map_view.h"

#include "../geo/geo_rectangle.h"
#include "../geo/dem.h"
#include "../math/triangle.h"
#include "../render/program_ext.h"

#define SLIB_MAP_VIEW_LAYER_COUNT 5

namespace slib
{

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

	class SLIB_EXPORT MapRegion
	{
	public:
		MapLocation center;
		double radiusE; // Easting
		double radiusN; // Northing

	public:
		sl_bool intersect(const MapRegion& other) const;
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
		static Ref<MapTileDirectory> open(const String& rootPath, const Function<String(MapTileAddress&)>& formator);

	public:
		sl_bool readData(Memory& _out, const MapTileAddress& address, sl_uint32 timeout) override;

	private:
		String m_root;
		Function<String(MapTileAddress&)> m_formator;
	};

	class SLIB_EXPORT MapUrlReader : public MapTileReader
	{
		SLIB_DECLARE_OBJECT

	protected:
		MapUrlReader();

		~MapUrlReader();

	public:
		static Ref<MapUrlReader> create(const String& url, const Function<String(MapTileAddress&)>& formator);

	public:
		sl_bool readData(Memory& _out, const MapTileAddress& address, sl_uint32 timeout) override;

	protected:
		sl_bool readUrl(Memory& _out, const String& url);

	private:
		String m_root;
		Function<String(MapTileAddress&)> m_formator;
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

		double getScale();

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

		double getViewLengthFromMapLength(double length);

		double getMapLengthFromViewLength(double length);

		MapRegion getViewportRegion();

		sl_bool containsRegion(const MapRegion& region);

		Matrix3 getRenderingTransformAt(const MapLocation& location);

		void draw(Canvas* canvas, MapViewData* data);

	public:
		virtual GeoLocation getEyeLocation();

		virtual void setEyeLocation(const GeoLocation& location);

		virtual LatLon getLatLonFromMapLocation(const MapLocation& location) = 0;

		virtual MapLocation getMapLocationFromLatLon(const LatLon& location) = 0;

		virtual void clearCache() = 0;

	protected:
		virtual void onDraw(Canvas* canvas, MapViewData* data) = 0;

	protected:
		MapLocation m_center;
		MapRange m_range;
		double m_scale;
		double m_minScale;
		double m_maxScale;
		RectangleT<double> m_viewport;
		AtomicRef<Drawable> m_background;
	};

	typedef render3d::vertex::PositionTexture MapTileVertex;

	class MapSurfaceConfiguration;

	class SLIB_EXPORT MapViewTile : public CRef
	{
		SLIB_DECLARE_OBJECT

	public:
		MapViewTile();

		~MapViewTile();

	public:
		MapTileLocationI location;
		GeoRectangle region;
		Double3 points[4]; // Bottom Left, Bottom Right, Top Left, Top Right
		Double3 center;

		Memory dem;
		Primitive primitive;
		Double3 pointsWithDEM[4];

	public:
		sl_bool build(const MapSurfaceConfiguration& config, const Rectangle* demRegion = sl_null);

		void buildVertex(MapTileVertex& vertex, double latitude, double longitude, double altitude, sl_real tx, sl_real ty);

	};

	class SLIB_EXPORT MapFlatPrimitive
	{
	public:
		List<Triangle> mesh;
		GeoLocation location;
		Double2 size;
		Color color;
		HatchStyle fillStyle;

	public:
		MapFlatPrimitive();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(MapFlatPrimitive)

	public:
		// Call from render thread
		const Ref<VertexBuffer>& getVertexBuffer();

		void invalidateVertexBuffer();

	private:
		Ref<VertexBuffer> m_vertexBuffer;
	};

	class SLIB_EXPORT MapSurfaceConfiguration
	{
	public:
		sl_uint32 baseLevel;
		sl_uint32 baseTileCountE; // Easting
		sl_uint32 baseTileCountN; // Northing
		sl_uint32 minimumLevel;
		sl_uint32 maximumLevel;
		double eastingRangeInDegrees;
		double northingRangeInDegrees;
		sl_uint32 tileDimensionInPixels;
		sl_uint32 minimumTileMatrixOrder;
		sl_uint32 maximumTileMatrixOrder;
		DEM::DataType demType;
		sl_bool flagFlipDemY;

	public:
		MapSurfaceConfiguration();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(MapSurfaceConfiguration)

	};

	class SLIB_EXPORT MapSurfaceParam : public MapSurfaceConfiguration
	{
	public:
		Function<void(MapTileLocationI&)> toReaderLocation;

		Ref<MapTileReader> picture;
		Ref<MapTileReader> dem;
		Ref<MapTileReader> layers[SLIB_MAP_VIEW_LAYER_COUNT];

	public:
		MapSurfaceParam();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(MapSurfaceParam)

	};

	class MapSurfacePlane;

	class SLIB_EXPORT MapSurface : public Object
	{
		SLIB_DECLARE_OBJECT

	protected:
		MapSurface();

		~MapSurface();

	public:
		static Ref<MapSurface> create(const MapSurfaceParam& param);

	public:
		virtual void render(RenderEngine* engine, MapViewData* data) = 0;

		virtual const List< Ref<MapViewTile> >& getTiles() = 0;

		virtual double getAltitudeAt(MapTileLoader* loader, const LatLon& location) = 0;

		virtual void clearCache() = 0;

	public:
		const MapSurfaceConfiguration& getConfiguration();

		Ref<MapTileReader> getPictureReader();

		void setPictureReader(const Ref<MapTileReader>& reader);

		Ref<MapTileReader> getDemReader();

		void setDemReader(const Ref<MapTileReader>& reader, DEM::DataType type, sl_bool flagFlipY = sl_false);

		Ref<MapTileReader> getLayerReader(sl_uint32 layer);

		void setLayerReader(sl_uint32 layer, const Ref<MapTileReader>& reader);

		sl_bool isLayerVisible(sl_uint32 layer);

		void setLayerVisible(sl_uint32 layer, sl_bool flag = sl_true);

		float getLayerOpacity(sl_uint32 layer);

		void setLayerOpacity(sl_uint32 layer, float opacity);

		// Normalized tile location (No reader location)
		LatLon getLatLonFromTileLocation(const MapTileLocationI& location);

		// Normalized tile location (No reader location)
		MapTileLocation getTileLocationFromLatLon(sl_uint32 level, const LatLon& latLon);

		MapTileLocationI getReaderLocation(const MapTileLocationI& location);

	protected:
		virtual void onDrawPlane(Canvas* canvas, const Rectangle& viewport, MapSurfacePlane* plane, MapViewData* data) = 0;

	protected:
		MapSurfaceConfiguration m_config;
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

		friend class MapSurfacePlane;
	};

	class SLIB_EXPORT MapSurfacePlane : public MapPlane
	{
		SLIB_DECLARE_OBJECT

	protected:
		MapSurfacePlane();

		~MapSurfacePlane();

	public:
		static Ref<MapSurfacePlane> create(const Ref<MapSurface>& surface);

	public:
		LatLon getLatLonFromMapLocation(const MapLocation& location) override;

		MapLocation getMapLocationFromLatLon(const LatLon& location) override;

		virtual void clearCache() override;

	protected:
		void onDraw(Canvas* canvas, MapViewData* data) override;

	protected:
		Ref<MapSurface> m_surface;
	};

	class SLIB_EXPORT MapViewObject : public Object
	{
		SLIB_DECLARE_OBJECT

	protected:
		MapViewObject();

		~MapViewObject();

	public:
		sl_bool isVisible();

		void setVisible(sl_bool flag = sl_true);

		sl_bool isSupportingGlobeMode();

		void setSupportingGlobeMode(sl_bool flag = sl_true);

		sl_bool isSupportingPlaneMode();

		void setSupportingPlaneMode(sl_bool flag = sl_true);

		sl_bool isOverlay();

		void setOverlay(sl_bool flag = sl_true);

	public:
		virtual void draw(Canvas* canvas, MapViewData* data, MapPlane* plane);

		virtual void render(RenderEngine* engine, MapViewData* data, MapSurface* surface);

	protected:
		sl_bool m_flagVisible : 1;
		sl_bool m_flagSupportGlobe : 1;
		sl_bool m_flagSupportPlane : 1;
		sl_bool m_flagOverlay : 1;
	};

	class SLIB_EXPORT MapViewObjectList : public MapViewObject
	{
		SLIB_DECLARE_OBJECT

	public:
		MapViewObjectList();

		~MapViewObjectList();

	public:
		void draw(Canvas* canvas, MapViewData* data, MapPlane* plane) override;

		void render(RenderEngine* engine, MapViewData* data, MapSurface* surface) override;

		void addChild(const Ref<MapViewObject>& child);

		void removeAll();

	protected:
		List< Ref<MapViewObject> > m_children;
	};

	class MapViewSprite : public MapViewObject
	{
		SLIB_DECLARE_OBJECT

	public:
		MapViewSprite();

		MapViewSprite(const LatLon& location, const Ref<Image>& image, const String& text, const Ref<FontAtlas>& atlas);

		~MapViewSprite();

	public:
		void initialize(const LatLon& location, const Ref<Image>& image, const String& text, const Ref<FontAtlas>& atlas);

	public:
		const LatLon& getLocation();

		const Ref<Image>& getImage();

		const String& getText();

		const Ref<FontAtlas>& getFontAtlas();

		const Size& getSize();

		void setSize(const Size& size);

		const Color& getTextColor();

		void setTextColor(const Color& color);

	public:
		void draw(Canvas* canvas, MapViewData* data, MapPlane* plane) override;

		void render(RenderEngine* engine, MapViewData* data, MapSurface* surface) override;

		sl_bool isBeingDrawn(MapViewData* data);

		sl_bool getViewPoint(Point& _out, MapViewData* data);

		double getAltitude(MapViewData* data);

		GeoLocation getLocation(MapViewData* data);

	protected:
		void onPreDrawOrRender(MapViewData* data);

		const Point& getViewPoint();

	protected:
		virtual void onDrawSprite(Canvas* canvas, MapViewData* data, MapPlane* plane);

		virtual void onRenderSprite(RenderEngine* engine, MapViewData* data, MapSurface* surface);

	protected:
		LatLon m_location;
		Ref<Image> m_image;
		String m_text;
		Ref<FontAtlas> m_fontAtlas;

		Size m_size;
		Color m_textColor;

		sl_bool m_flagValidAltitude;
		double m_altitude;
		Point m_viewPoint;
		sl_uint64 m_lastDrawId;
	};

	class MapViewExtension : public Object
	{
		SLIB_DECLARE_OBJECT

	public:
		MapViewExtension();

		~MapViewExtension();

	public:
		virtual void onChangeLocation(const GeoLocation& location) = 0;

		virtual void onChangeRotation(double rotation) = 0;

		virtual void onChangeTilt(double tilt) = 0;

	};

}

#endif