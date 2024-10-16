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
#include "view_state_map.h"

#include "../geo/earth.h"
#include "../math/view_frustum.h"

namespace slib
{

	class MapPlane;
	class MapSurface;
	class MapTileLoader;
	class MapViewObject;
	class MapViewExtension;

	class SLIB_EXPORT MapViewState
	{
	public:
		// Input States
		double viewportWidth;
		double viewportHeight;
		GeoLocation eyeLocation;
		float tilt;
		float rotation;
		sl_bool flagTileGrid;
		sl_bool flagTerrainGrid;

		// Derived States
		Double3 eyePoint;
		Matrix4T<double> verticalViewTransform;
		Matrix4T<double> viewTransform;
		Matrix4T<double> inverseViewTransform;
		Matrix4T<double> projectionTransform;;
		Matrix4T<double> viewProjectionTransform;
		ViewFrustumT<double> viewFrustum;
		sl_uint64 drawId;

		// Other
		Ref<MapTileLoader> tileLoader;
		Ref<RenderBlendState> defaultBlendState;
		Ref<RenderDepthStencilState> defaultDepthState;
		Ref<RenderRasterizerState> defaultRasterizerState;
		Ref<RenderBlendState> overlayBlendState;
		Ref<RenderDepthStencilState> overlayDepthState;
		Ref<RenderRasterizerState> overlayRasterizerState;

	public:
		MapViewState();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(MapViewState)

	public:
		sl_bool update();

	};

	class SLIB_EXPORT MapViewData
	{
	public:
		MapViewData();

		~MapViewData();

	public:
		typedef SphericalEarth Earth;

	public:
		sl_bool isGlobeMode() const;

		void setGlobeMode(sl_bool flag = sl_true, UIUpdateMode mode = UIUpdateMode::Redraw);

		GeoLocation getEyeLocation() const;

		void setEyeLocation(const GeoLocation& location, UIUpdateMode mode = UIUpdateMode::Redraw);

		// Degrees
		float getEyeRotation() const;

		// Degrees
		void setEyeRotation(float rotation, UIUpdateMode mode = UIUpdateMode::Redraw);

		// Degrees
		float getEyeTilt() const;

		// Degrees
		void setEyeTilt(float tilt, UIUpdateMode mode = UIUpdateMode::Redraw);

		double getMapScale() const;

		void setMapScale(double scale, UIUpdateMode mode = UIUpdateMode::Redraw);

		double getMinimumAltitude() const;

		void setMinimumAltitude(double altitude, UIUpdateMode mode = UIUpdateMode::Redraw);

		double getMaximumAltitude() const;

		void setMaximumAltitude(double altitude, UIUpdateMode mode = UIUpdateMode::Redraw);

		double getMinimumDistanceFromGround() const;

		void setMinimumDistanceFromGround(double distance);

		sl_bool isTileGridVisible() const;

		void setTileGridVisible(sl_bool flag = sl_true, UIUpdateMode mode = UIUpdateMode::Redraw);

		sl_bool isTerrainGridVisible() const;

		void setTerrainGridVisible(sl_bool flag = sl_true, UIUpdateMode mode = UIUpdateMode::Redraw);

	public:
		Ref<View> getView() const;

		Ref<MapPlane> getPlane() const;

		void setPlane(const Ref<MapPlane>& plane, UIUpdateMode mode = UIUpdateMode::Redraw);

		Ref<MapSurface> getSurface() const;

		void setSurface(const Ref<MapSurface>& surface, UIUpdateMode mode = UIUpdateMode::Redraw);

		List< Ref<MapViewObject> > getObjects() const;

		Ref<MapViewObject> getObject(const String& key) const;

		void putObject(const String& name, const Ref<MapViewObject>& object, UIUpdateMode mode = UIUpdateMode::Redraw);

		const MapViewState& getMapState() const;

		MapViewState& getMapState();

		void resize(double width, double height, UIUpdateMode mode = UIUpdateMode::Redraw);

		void movePlane(double dx, double dy, UIUpdateMode mode = UIUpdateMode::Redraw);

		void travelTo(const GeoLocation& location);

		void zoom(double scale, UIUpdateMode mode = UIUpdateMode::Redraw);

		void zoomAt(const Double2& point, double scale, UIUpdateMode mode = UIUpdateMode::Redraw);

		void click(const Double2& pt, UIUpdateMode mode = UIUpdateMode::Redraw);

		void stopMoving();

		// Not thread-safe
		void putExtension(const String& name, const Ref<MapViewExtension>& extension);

		Ref<MapViewExtension> getExtension(const String& name);

	public:
		void invalidate(UIUpdateMode mode = UIUpdateMode::Redraw);

		void drawPlane(Canvas* canvas);

		void renderGlobe(RenderEngine* engine);

		void renderTexture(RenderEngine* engine, const Point& center, const Size& size, const Ref<Texture>& texture, const Color4F& color = Color4F(1.0f, 1.0f, 1.0f, 1.0f));

		void renderImage(RenderEngine* engine, const Point& center, const Size& size, const Ref<Image>& image, const Color4F& color = Color4F(1.0f, 1.0f, 1.0f, 1.0f));

		void renderText(RenderEngine* engine, const Point& pt, const StringParam& text, const Ref<FontAtlas>& atlas, const Color& color = Color::White, const Alignment& align = Alignment::MiddleCenter);

	public:
		sl_bool getLatLonFromViewPoint(const Double2& point, LatLon& _out) const;

		Double2 getViewPointFromLatLon(const LatLon& location) const;

		sl_bool getLocationFromViewPoint(const Double2& point, GeoLocation& _out) const;

		Double2 getViewPointFromLocation(const GeoLocation& location) const;

		sl_bool getEarthPointFromViewPoint(const Double2& point, Double3& _out) const;

		Double2 getViewPointFromEarthPoint(const Double3& point) const;

		double getAltitudeAt(const LatLon& location) const;

		GeoLocation getLocationFromLatLon(const LatLon& location) const;

		sl_bool isLocationVisible(const GeoLocation& location) const;

		sl_bool isEarthPointVisible(const Double3& point) const;

		static double getDegreeFromEarthLength(double length);

		static double getEarthLengthFromDegree(double degrees);

		// `height`: meters
		static double getAltitudeFromViewportHeight(double height);

		// returns height meters
		static double getViewportHeightFromAltitude(double altitude);

		static double getMetersFromPixels(double pixels);

		static double getPixelsFromMeters(double meters);

		static double getScaleFromAltitude(double altitude, double viewportHeight);

		static double getAltitudeFromScale(double scale, double viewportHeight);

		static Matrix4T<double> getWorldTransformAt(const GeoLocation& location);

	protected:
		void setEyeLocation(const GeoLocation& location, UIEvent* ev, UIUpdateMode mode = UIUpdateMode::Redraw);

		void setEyeRotation(float rotation, UIEvent* ev, UIUpdateMode mode = UIUpdateMode::Redraw);

		void setEyeTilt(float tilt, UIEvent* ev, UIUpdateMode mode = UIUpdateMode::Redraw);

		void movePlane(double dx, double dy, UIEvent* ev, UIUpdateMode mode = UIUpdateMode::Redraw);

		void zoom(double scale, UIEvent* ev, UIUpdateMode mode = UIUpdateMode::Redraw);

		void zoomAt(const Double2& point, double scale, UIEvent* ev, UIUpdateMode mode = UIUpdateMode::Redraw);

	protected:
		// View implementations
		virtual void doInvalidate(UIUpdateMode mode);

		virtual void notifyChangeLocation(const GeoLocation& location, UIEvent* ev);
		void invokeChangeLocation(const GeoLocation& location, UIEvent* ev);

		virtual void notifyChangeRotation(double rotation, UIEvent* ev);
		void invokeChangeRotation(double rotation, UIEvent* ev);

		virtual void notifyChangeTilt(double tilt, UIEvent* ev);
		void invokeChangeTilt(double tilt, UIEvent* ev);

	protected:
		sl_bool _initState();

		void _resizePlane(MapPlane* plane, double width, double height);

		void _onCompleteLazyLoading();

	protected:
		Mutex m_lock;
		WeakRef<View> m_view;

		sl_bool m_flagGlobeMode;
		Ref<MapPlane> m_plane;
		Ref<MapSurface> m_surface;
		HashMap< String, Ref<MapViewObject> > m_objects;
		HashMap< String, Ref<MapViewExtension> > m_extensions; // Not thread-safe

		MapViewState m_state;
		double m_minAltitude;
		double m_maxAltitude;
		double m_minDistanceFromGround;

		class Motion
		{
		public:
			AtomicRef<Timer> timer;
			AtomicWeakRef<View> view;
			MapViewData* parent;
			AtomicRef<UIEvent> ev;

			sl_bool flagRunning;
			sl_uint64 startTick;
			sl_uint64 lastTick;

			GeoLocation location;
			GeoLocation startLocation;
			GeoLocation endLocation;
			sl_bool flagTravel;

			float rotation;
			float startRotation;
			float endRotation;

			float tilt;
			float startTilt;
			float endTilt;

		public:
			Motion();

			~Motion();

		public:
			void prepare(MapViewData* parent);

			void start();

			void stop();

			void step();
		};

		Motion m_motion;

		friend class Motion;
	};

	class SLIB_EXPORT MapView : public RenderView, public MapViewData
	{
		SLIB_DECLARE_OBJECT

	public:
		MapView();

		~MapView();

	public:
		Ref<Image> getCompass(ViewState state = ViewState::Default);

		void setCompass(const Ref<Drawable>& image, ViewState state, UIUpdateMode mode = UIUpdateMode::Redraw);

		void setCompass(const Ref<Drawable>& image, UIUpdateMode mode = UIUpdateMode::Redraw);

		// Pixels
		sl_ui_len getCompassSize();

		// Pixels
		void setCompassSize(sl_ui_len size, UIUpdateMode mode = UIUpdateMode::Redraw);

		// [0, 1]
		const Point& getCompassCenter();

		// [0, 1]
		void setCompassCenter(const Point& pt, UIUpdateMode mode = UIUpdateMode::Redraw);

		// [0, 1]
		void setCompassCenter(sl_real cx, sl_real cy, UIUpdateMode mode = UIUpdateMode::Redraw);

		const Alignment& getCompassAlignment();

		void setCompassAlignment(const Alignment& align, UIUpdateMode mode = UIUpdateMode::Redraw);

		sl_ui_len getCompassMarginLeft();

		void setCompassMarginLeft(sl_ui_len margin, UIUpdateMode mode = UIUpdateMode::Redraw);

		sl_ui_len getCompassMarginTop();

		void setCompassMarginTop(sl_ui_len margin, UIUpdateMode mode = UIUpdateMode::Redraw);

		sl_ui_len getCompassMarginRight();

		void setCompassMarginRight(sl_ui_len margin, UIUpdateMode mode = UIUpdateMode::Redraw);

		sl_ui_len getCompassMarginBottom();

		void setCompassMarginBottom(sl_ui_len margin, UIUpdateMode mode = UIUpdateMode::Redraw);

		void setCompassMargin(sl_ui_len left, sl_ui_len top, sl_ui_len right, sl_ui_len bottom, UIUpdateMode mode = UIUpdateMode::Redraw);

		void setCompassMargin(sl_ui_len margin, UIUpdateMode mode = UIUpdateMode::Redraw);

		const UIEdgeInsets& getCompassMargin();

		void setCompassMargin(const UIEdgeInsets& margin, UIUpdateMode mode = UIUpdateMode::Redraw);

		// Top Left
		UIPoint getCompassLocation();

	public:
		using RenderView::invalidate;

	protected:
		void renderCompass(RenderEngine* engine);

	public:
		SLIB_DECLARE_EVENT_HANDLER(MapView, ChangeLocation, const GeoLocation& location, UIEvent* ev /* nullable */)

		SLIB_DECLARE_EVENT_HANDLER(MapView, ChangeRotation, double rotation, UIEvent* ev /* nullable */)

		SLIB_DECLARE_EVENT_HANDLER(MapView, ChangeTilt, double tilt , UIEvent* ev /* nullable */)

	protected:
		void init() override;

		void onDraw(Canvas* canvas) override;

		void onFrame(RenderEngine* engine) override;

		void onMouseEvent(UIEvent* ev) override;

		void onMouseWheelEvent(UIEvent* ev) override;

		void onKeyEvent(UIEvent* ev) override;

		void onResize(sl_ui_len width, sl_ui_len height) override;

	private:
		void doInvalidate(UIUpdateMode mode) override;

		void notifyChangeLocation(const GeoLocation& location, UIEvent* ev) override;

		void notifyChangeRotation(double rotation, UIEvent* ev) override;

		void notifyChangeTilt(double tilt, UIEvent* ev) override;

	private:
		sl_bool _isPointInCompass(const Point& pt);

	protected:
		ViewStateMap< Ref<Image> > m_compass;
		Point m_compassCenter;
		sl_ui_len m_compassSize;
		Alignment m_compassAlign;
		UIEdgeInsets m_compassMargin;

		sl_uint32 m_nLastTouches;
		Point m_ptLastEvent;

		sl_bool m_flagLeftDown;
		Point m_ptLeftDown;
		Matrix4T<double> m_transformLeftDown;
		sl_uint64 m_tickLeftDown;
		float m_rotationLeftDown;

		Point m_ptTouchStart1;
		Point m_ptTouchStart2;
		float m_rotationTouchStart;
		double m_altitudeTouchStart;
		sl_bool m_flagTouchRotateStarted;

		sl_bool m_flagClicking;
		ViewState m_compassState;
	};

}

#endif