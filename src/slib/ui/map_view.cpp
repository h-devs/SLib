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

#include "slib/ui/map_view.h"

#include "slib/graphics/image.h"
#include "slib/io/file.h"
#include "slib/data/expiring_map.h"
#include "slib/system/system.h"
#include "slib/device/cpu.h"
#include "slib/geo/earth.h"
#include "slib/core/thread_pool.h"
#include "slib/core/variant.h"
#include "slib/core/safe_static.h"

namespace slib
{

	namespace object_types
	{
		enum {
			MapLayer = MapView + 1,
			MapPlane,
			MapSurface,
			MapTileReader,
			MapTileDirectory,
			MapTileCache,
			MapTileLoader
		};
	}

	namespace
	{
		class SharedContext
		{
		public:
			Ref<DispatchLoop> dispatchLoop;

		public:
			SharedContext()
			{
				dispatchLoop = DispatchLoop::create();
			}
		};

		SLIB_SAFE_STATIC_GETTER(SharedContext, GetSharedContext)
	}

	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(MapTileAddress)

	MapTileAddress::MapTileAddress()
	{
	}

	sl_bool MapTileAddress::equals(const MapTileAddress& other) const noexcept
	{
		return level == other.level && y == other.y && x == other.x && subPath == other.subPath;
	}

	sl_compare_result MapTileAddress::compare(const MapTileAddress& other) const noexcept
	{
		sl_compare_result c = ((const MapTileLocationi&)*this).compare((const MapTileLocationi&)other);
		if (c) {
			return c;
		}
		return subPath.compare(other.subPath);
	}

	sl_size MapTileAddress::getHashCode() const noexcept
	{
		sl_size h = Hash<MapTileLocationi>()(*this);
		if (subPath.isNull()) {
			return h;
		}
		return h ^ subPath.getHashCode();
	}

	SLIB_DEFINE_OBJECT(MapTileReader, Object)

	MapTileReader::MapTileReader()
	{
	}

	MapTileReader::~MapTileReader()
	{
	}

	sl_bool MapTileReader::readImage(Ref<Image>& _out, const MapTileAddress& address, sl_uint32 timeout)
	{
		Memory data;
		if (readData(data, address, timeout)) {
			if (data.isNotNull()) {
				_out = Image::loadFromMemory(data);
			} else {
				_out.setNull();
			}
			return sl_true;
		}
		return sl_false;
	}

	SLIB_DEFINE_OBJECT(MapTileDirectory, MapTileReader)

	MapTileDirectory::MapTileDirectory()
	{
	}

	MapTileDirectory::~MapTileDirectory()
	{
	}

	Ref<MapTileDirectory> MapTileDirectory::open(const String& rootPath, const String& pathFormat)
	{
		Ref<MapTileDirectory> ret = new MapTileDirectory;
		if (ret.isNotNull()) {
			ret->m_root = rootPath;
			ret->m_format = pathFormat;
			return ret;
		}
		return sl_null;
	}

	sl_bool MapTileDirectory::readData(Memory& _out, const MapTileAddress& address, sl_uint32 timeout)
	{
		if (address.subPath.isNotNull()) {
			_out = File::readAllBytes(File::concatPath(m_root, String::format(m_format, address.level, address.y, address.x), address.subPath));
		} else {
			_out = File::readAllBytes(File::concatPath(m_root, String::format(m_format, address.level, address.y, address.x)));
		}
		if (_out.isNotNull()) {
			return sl_true;
		}
		return File::isDirectory(m_root);
	}

	SLIB_DEFINE_OBJECT(MapTileCache, Object)

	MapTileCache::MapTileCache()
	{
	}

	MapTileCache::~MapTileCache()
	{
	}

	namespace
	{
		class MapTileCacheImpl : public MapTileCache
		{
		public:
			HashMap< MapTileAddress, Ref<CRef> > m_drawingObjects;
			HashMap< MapTileAddress, Ref<CRef> > m_lastDrawnObjects;
			ExpiringMap< MapTileAddress, Ref<CRef> > m_expiringObjects;
			sl_uint32 m_nMaxObjects = 0;

		public:
			static Ref<MapTileCacheImpl> create(sl_uint32 nMaxCount, sl_uint32 expiringMilliseconds)
			{
				Ref<MapTileCacheImpl> ret = new MapTileCacheImpl;
				if (ret.isNotNull()) {
					ret->initialize(nMaxCount, expiringMilliseconds);
					return ret;
				}
				return sl_null;
			}

		public:
			void initialize(sl_uint32 nMaxCount, sl_uint32 expiringMilliseconds)
			{
				SharedContext* context = GetSharedContext();
				if (!context) {
					return;
				}
				m_expiringObjects.setupTimer(expiringMilliseconds, context->dispatchLoop);
				m_nMaxObjects = nMaxCount;
			}

			void startDrawing() override
			{
				ObjectLocker lock(this);
				m_lastDrawnObjects = Move(m_drawingObjects);
			}

			void endDrawing() override
			{
				HashMap< MapTileAddress, Ref<CRef> > temp;
				{
					ObjectLocker lock(this);
					temp = Move(m_lastDrawnObjects);
				}
				ObjectLocker lock(&m_expiringObjects);
				if (m_expiringObjects.getCount() < m_nMaxObjects) {
					auto node = temp.getFirstNode();
					while (node) {
						m_expiringObjects.put_NoLock(node->key, Move(node->value));
						node = node->next;
					}
				} else {
					temp = m_expiringObjects.pushInternalMap(Move(temp));
					SharedContext* context = GetSharedContext();
					if (context) {
						// Free on dispatch loop
						context->dispatchLoop->dispatch([temp]() {});
					}
				}
			}

			sl_bool getObject(const MapTileAddress& address, Ref<CRef>& _out) override
			{
				{
					ObjectLocker lock(this);
					if (m_drawingObjects.get_NoLock(address, &_out)) {
						return sl_true;
					}
					auto node = m_lastDrawnObjects.find_NoLock(address);
					if (node) {
						_out = Move(node->value);
						m_drawingObjects.put_NoLock(address, _out);
						m_lastDrawnObjects.removeAt(node);
						return sl_true;
					}
				}
				if (m_expiringObjects.remove(address, &_out)) {
					ObjectLocker lock(this);
					m_drawingObjects.put_NoLock(address, _out);
					return sl_true;
				}
				return sl_false;
			}

			void saveObject(const MapTileAddress& address, const Ref<CRef>& object) override
			{
				if (m_drawingObjects.getCount() > m_nMaxObjects) {
					return;
				}
				ObjectLocker lock(this);
				m_drawingObjects.put_NoLock(address, object);
			}
		};
	}

	Ref<MapTileCache> MapTileCache::create(sl_uint32 nMaxCount, sl_uint32 expiringMilliseconds)
	{
		return Ref<MapTileCache>::cast(MapTileCacheImpl::create(nMaxCount, expiringMilliseconds));
	}

	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(MapTileLoadParam)

	MapTileLoadParam::MapTileLoadParam()
	{
		timeout = 10000;
		flagLoadNow = sl_false;
	}

	SLIB_DEFINE_OBJECT(MapTileLoader, Object)

	MapTileLoader::MapTileLoader()
	{
	}

	MapTileLoader::~MapTileLoader()
	{
	}

	namespace
	{
		class MapTileLoaderImpl : public MapTileLoader
		{
		public:
			Ref<ThreadPool> m_threads;
			sl_uint32 m_nMaxQueue;

			class Request : public MapTileLoadParam
			{
			public:
				sl_bool flagImage;
				Function<Ref<CRef>(Memory&)> loader;
				Function<void(Ref<CRef>&)> onComplete;
			};
			Queue<Request> m_requests;

		public:
			static Ref<MapTileLoaderImpl> create(sl_uint32 nThreads, sl_uint32 nMaxQueue)
			{
				if (!nThreads) {
					nThreads = Cpu::getCoreCount();
				}
				Ref<MapTileLoaderImpl> ret = new MapTileLoaderImpl;
				if (ret.isNull()) {
					return sl_null;
				}
				Ref<ThreadPool> threads = ThreadPool::create(SLIB_FUNCTION_MEMBER(ret.get(), onLoad), nThreads);
				if (threads.isNull()) {
					return sl_null;
				}
				ret->m_threads = Move(threads);
				ret->m_nMaxQueue = 0;
				return ret;
			}

		public:
			sl_bool load(Ref<CRef>& _out, const MapTileLoadParam& param, sl_bool flagImage, const Function<Ref<CRef>(Memory&)>& loader, const Function<void(Ref<CRef>&)>& onComplete) override
			{
				if (param.reader.isNull()) {
					return sl_false;
				}
				if (param.cache.isNull()) {
					if (doLoad(_out, param, flagImage, loader)) {
						onComplete(_out);
						return sl_true;
					}
					return sl_false;
				}
				if (param.cache->getObject(param.address, _out)) {
					if (_out.isNull()) {
						onComplete(_out);
						return sl_true;
					}
					if (flagImage) {
						if (IsInstanceOf<Image>(_out)) {
							onComplete(_out);
							return sl_true;
						}
					} else {
						if (loader.isNotNull()) {
							onComplete(_out);
							return sl_true;
						} else {
							if (IsInstanceOf<CMemory>(_out)) {
								onComplete(_out);
								return sl_true;
							}
						}
					}
				}
				if (param.flagLoadNow) {
					if (doLoad(_out, param, flagImage, loader)) {
						param.cache->saveObject(param.address, _out);
						onComplete(_out);
						return sl_true;
					}
					return sl_false;
				}
				{
					ObjectLocker lock(&m_requests);
					Link<Request>* link = m_requests.getFront();
					while (link) {
						Request& request = link->value;
						if (request.reader == param.reader && request.address == param.address) {
							m_requests.removeLink(link);
							m_requests.pushLinkAtFront_NoLock(link);
							return sl_true;
						}
						link = link->next;
					}
					Request request;
					(MapTileLoadParam&)request = param;
					request.flagImage = flagImage;
					request.loader = loader;
					request.onComplete = onComplete;
					m_requests.pushFront_NoLock(Move(request));
					if (m_requests.getCount() > m_nMaxQueue) {
						m_requests.popBack_NoLock();
					}
				}
				return sl_true;
			}

			sl_bool doLoad(Ref<CRef>& _out, const MapTileLoadParam& param, sl_bool flagImage, const Function<Ref<CRef>(Memory&)>& loader)
			{
				if (flagImage) {
					return param.reader->readImage(*(reinterpret_cast<Ref<Image>*>(&_out)), param.address, param.timeout);
				} else {
					Memory mem;
					if (param.reader->readData(mem, param.address, param.timeout)) {
						if (mem.isNotNull()) {
							if (loader.isNotNull()) {
								_out = loader(mem);
							} else {
								_out = Move(mem.ref);
							}
						}
						return sl_true;
					}
					return sl_false;
				}
			}

			sl_bool onLoad()
			{
				Link<Request>* link = m_requests.popLinkFromFront();
				if (!link) {
					return sl_false;
				}
				Request& request = link->value;
				Ref<CRef> ret;
				if (doLoad(ret, request, request.flagImage, request.loader)) {
					if (request.cache.isNotNull()) {
						request.cache->saveObject(request.address, ret);
					}
				}
				Queue<Request>::deleteLink(link);
				return sl_true;
			}
		};
	}

	Ref<MapTileLoader> MapTileLoader::create(sl_uint32 nThreads, sl_uint32 nMaxQueue)
	{
		return Ref<MapTileLoader>::cast(MapTileLoaderImpl::create(nThreads, nMaxQueue));
	}

	sl_bool MapTileLoader::loadData(Memory& _out, const MapTileLoadParam& param, const Function<void(Memory&)>& onComplete)
	{
		return load(Ref<CRef>::cast(_out.ref), param, sl_false, sl_null, Function<void(Ref<CRef>&)>::cast(onComplete));
	}

	sl_bool MapTileLoader::loadImage(Ref<Image>& _out, const MapTileLoadParam& param, const Function<void(Ref<Image>&)>& onComplete)
	{
		return load(Ref<CRef>::cast(_out), param, sl_true, sl_null, Function<void(Ref<CRef>&)>::cast(onComplete));
	}

	sl_bool MapTileLoader::loadObject(Ref<CRef>& _out, const MapTileLoadParam& param, const Function<Ref<CRef>(Memory&)>& loader, const Function<void(Ref<CRef>&)>& onComplete)
	{
		return load(_out, param, sl_false, loader, onComplete);
	}


	SLIB_DEFINE_OBJECT(MapPlane, Object)

	MapPlane::MapPlane()
	{
		m_center.E = 0.0;
		m_center.N = 0.0;
		m_range.left = -180.0;
		m_range.bottom = -90.0;
		m_range.right = 180.0;
		m_range.top = 90.0;
		m_scale = 1.0;
		m_scaleMin = 0.0;
		m_scaleMax = 10000000000.0;
	}

	MapPlane::~MapPlane()
	{
	}

	const MapLocation& MapPlane::getCenterLocation()
	{
		return m_center;
	}

	void MapPlane::setCenterLocation(double E, double N)
	{
		double w = m_scale;
		double h = m_scale;
		if (w < m_range.right - m_range.left) {
			w /= 2.0;
			m_center.E = Math::clamp(E, m_range.left + w, m_range.right - w);
		} else {
			m_center.E = (m_range.left + m_range.right) / 2.0;
		}
		if (h < m_range.top - m_range.bottom) {
			h /= 2.0;
			m_center.N = Math::clamp(N, m_range.bottom + h, m_range.top - h);
		} else {
			m_center.N = (m_range.bottom + m_range.top) / 2.0;
		}
	}

	const MapRange& MapPlane::getMapRange()
	{
		return m_range;
	}

	void MapPlane::setMapRange(const MapRange& rect)
	{
		m_range = rect;
		setCenterLocation(m_center.E, m_center.N);
	}

	double MapPlane::getScale()
	{
		return m_scale;
	}

	void MapPlane::setScale(double scale)
	{
		m_scale = Math::clamp(scale, m_scaleMin, m_scaleMax);
	}

	double MapPlane::getMinimumScale()
	{
		return m_scaleMin;
	}

	void MapPlane::setMinimumScale(double scale)
	{
		m_scaleMin = scale;
		setScale(m_scale);
	}

	double MapPlane::getMaximumScale()
	{
		return m_scaleMax;
	}

	void MapPlane::setMaximumScale(double scale)
	{
		m_scaleMax = scale;
		setScale(m_scale);
	}

	Ref<Drawable> MapPlane::getBackground()
	{
		return m_background;
	}

	void MapPlane::setBackground(const Ref<Drawable>& background)
	{
		m_background = background;
	}

	Double2 MapPlane::toViewport(const MapLocation& location)
	{
		return { 0.5 + (location.E - m_center.E) / m_scale, 0.5 - (location.N - m_center.N) / m_scale };
	}

	MapLocation MapPlane::fromViewport(const Double2& point)
	{
		return { m_center.E + (point.x - 0.5) * m_scale, m_center.N - (point.y - 0.5) * m_scale };
	}

	void MapPlane::draw(Canvas* canvas, const Rectangle& rect, MapViewData* data)
	{
		if (m_background.isNotNull()) {
			Ref<Drawable> background = m_background;
			if (background.isNotNull()) {
				canvas->draw(rect, background);
			}
		}
		onDraw(canvas, rect, data);
	}


	SLIB_DEFINE_OBJECT(MapLayer, Object)

	MapLayer::MapLayer()
	{
		m_flagSupportGlobe = sl_false;
		m_flagSupportPlane = sl_false;
	}

	MapLayer::~MapLayer()
	{
	}

	sl_bool MapLayer::isSupportingGlobeMode()
	{
		return m_flagSupportGlobe;
	}

	void MapLayer::setSupportingGlobeMode(sl_bool flag)
	{
		m_flagSupportGlobe = flag;
	}

	sl_bool MapLayer::isSupportingPlaneMode()
	{
		return m_flagSupportPlane;
	}

	void MapLayer::setSupportingPlaneMode(sl_bool flag)
	{
		m_flagSupportPlane = flag;
	}

	void MapLayer::draw(Canvas* canvas, const Rectangle& rect, MapViewData* data, MapPlane* plane)
	{
	}


	MapViewData::MapViewData()
	{
		m_view = sl_null;
		m_flagGlobeMode = sl_false;
		m_tileLoader = MapTileLoader::create();
		m_eyeLocation.altitude = 10000;
	}

	MapViewData::~MapViewData()
	{
	}

	sl_bool MapViewData::isGlobeMode() const
	{
		return m_flagGlobeMode;
	}

	void MapViewData::setGlobeMode(sl_bool flag, UIUpdateMode mode)
	{
		MutexLocker locker(&m_lock);
		if (m_flagGlobeMode == flag) {
			return;
		}
		m_flagGlobeMode = flag;
		if (flag) {
			if (m_plane.isNotNull()) {
				m_eyeLocation = m_plane->getEyeLocation();
			}
		} else {
			if (m_plane.isNotNull()) {
				m_plane->setEyeLocation(m_eyeLocation);
			}
		}
		invalidate(mode);
	}

	MapPlane* MapViewData::_getPlane() const
	{
		if (m_plane.isNotNull()) {
			return m_plane.get();
		}
		return m_surface.get();
	}

	Ref<MapPlane> MapViewData::getPlane() const
	{
		MutexLocker locker(&m_lock);
		return _getPlane();
	}

	void MapViewData::setPlane(const Ref<MapPlane>& plane, UIUpdateMode mode)
	{
		MutexLocker locker(&m_lock);
		if (m_plane == plane) {
			return;
		}
		if (m_plane.isNotNull()) {
			m_eyeLocation = m_plane->getEyeLocation();
		}
		m_plane = plane;
		if (plane.isNotNull()) {
			plane->setEyeLocation(m_eyeLocation);
		}
		invalidate(mode);
	}

	Ref<MapSurface> MapViewData::getSurface() const
	{
		MutexLocker locker(&m_lock);
		return m_surface;
	}

	void MapViewData::setSurface(const Ref<MapSurface>& surface, UIUpdateMode mode)
	{
		MutexLocker locker(&m_lock);
		m_surface = surface;
		invalidate(mode);
	}

	List< Ref<MapLayer> > MapViewData::getLayers() const
	{
		MutexLocker locker(&m_lock);
		return m_layers.getAllValues_NoLock();
	}

	void MapViewData::putLayer(const String& name, const Ref<MapLayer>& layer, UIUpdateMode mode)
	{
		MutexLocker locker(&m_lock);
		if (layer.isNotNull()) {
			m_layers.put_NoLock(name, layer);
		} else {
			m_layers.remove_NoLock(name);
		}
		invalidate(mode);
	}

	GeoLocation MapViewData::getEyeLocation() const
	{
		MutexLocker locker(&m_lock);
		if (!m_flagGlobeMode) {
			MapPlane* plane = _getPlane();
			if (plane) {
				return plane->getEyeLocation();
			}
		}
		return m_eyeLocation;
	}

	void MapViewData::setEyeLocation(const GeoLocation& location, UIUpdateMode mode)
	{
		MutexLocker locker(&m_lock);
		m_eyeLocation = location;
		if (!m_flagGlobeMode) {
			MapPlane* plane = _getPlane();
			if (plane) {
				plane->setEyeLocation(location);
			}
		}
		invalidate(mode);
	}

	sl_bool MapViewData::toLatLon(const Double2& point, LatLon& _out) const
	{
		MutexLocker locker(&m_lock);
		if (m_flagGlobeMode) {
		} else {
			MapPlane* plane = _getPlane();
			if (plane) {
				_out = plane->toLatLon(plane->fromViewport(point));
				return sl_true;
			}
		}
		return sl_false;
	}

	Double2 MapViewData::toViewport(const LatLon& location) const
	{
		MutexLocker locker(&m_lock);
		if (m_flagGlobeMode) {
		} else {
			MapPlane* plane = _getPlane();
			if (plane) {
				return plane->toViewport(plane->fromLatLon(location));
			}
		}
		return { 0.0, 0.0 };
	}

	void MapViewData::zoom(double scale, UIUpdateMode mode)
	{
		MutexLocker locker(&m_lock);
		if (m_flagGlobeMode) {
		} else {
			MapPlane* plane = _getPlane();
			if (plane) {
				plane->setScale(plane->getScale() * scale);
			}
		}
		invalidate(mode);
	}

	MapTileLoader* MapViewData::getTileLoader()
	{
		return m_tileLoader.get();
	}

	void MapViewData::drawPlane(Canvas* canvas, const Rectangle& rect)
	{
		MutexLocker locker(&m_lock);
		if (m_flagGlobeMode) {
			return;
		}
		if (rect.getHeight() < 1.0f) {
			return;
		}
		MapPlane* plane = _getPlane();
		if (!plane) {
			return;
		}
		plane->draw(canvas, rect, this);
		auto node = m_layers.getFirstNode();
		while (node) {
			Ref<MapLayer>& layer = node->value;
			if (layer->isSupportingPlaneMode()) {
				layer->draw(canvas, rect, this, plane);
			}
			node = node->next;
		}
	}

	void MapViewData::renderGlobe(RenderEngine* engine)
	{
	}

	void MapViewData::invalidate(UIUpdateMode mode)
	{
		if (m_view) {
			m_view->invalidate(mode);
		}
	}


	SLIB_DEFINE_OBJECT(MapView, RenderView)

	MapView::MapView()
	{
		m_view = this;
		setRedrawMode(RedrawMode::WhenDirty);
		setFocusable();
	}

	MapView::~MapView()
	{
	}

	void MapView::onDraw(Canvas* canvas)
	{
		drawPlane(canvas, getBounds());
	}

	void MapView::onFrame(RenderEngine* engine)
	{
		renderGlobe(engine);
		RenderView::onFrame(engine);
	}

	void MapView::onMouseEvent(UIEvent* ev)
	{
		UIAction action = ev->getAction();
		if (action == UIAction::LeftButtonDown) {
			invalidate();
		}
		RenderView::onMouseEvent(ev);
	}

	void MapView::onMouseWheelEvent(UIEvent* ev)
	{
		RenderView::onMouseWheelEvent(ev);
		sl_real delta = ev->getDelta();
		if (delta > 0) {
			zoom(0.75);
		} else if (delta < 0) {
			zoom(1.5);
		}
	}

}
