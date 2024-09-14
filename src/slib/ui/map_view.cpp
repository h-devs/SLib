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
			struct Item
			{
				Ref<CRef> object;
				sl_uint64 lastAccessTick;
			};
			ExpiringMap<MapTileAddress, Item> m_map;
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
				m_map.setupTimer(expiringMilliseconds, context->dispatchLoop);
				m_nMaxObjects = nMaxCount;
			}

			sl_bool getObject(const MapTileAddress& address, Ref<CRef>& _out) override
			{
				ObjectLocker lock(&m_map);
				Item* item = m_map.getItemPointer(address);
				if (item) {
					item->lastAccessTick = System::getTickCount64();
					_out = item->object;
					return sl_true;
				}
				return sl_false;
			}

			void saveObject(const MapTileAddress& address, const Ref<CRef>& object) override
			{
				ObjectLocker lock(&m_map);
				if (m_map.getCount() >= m_nMaxObjects * 2) {
					HashMap<MapTileAddress, Item> const* maps[2];
					maps[0] = &(m_map.getInternalMap0());
					maps[1] = &(m_map.getInternalMap1());
					sl_uint64 minTick = 0, maxTick = 0;
					{
						for (sl_uint32 i = 0; i < 2; i++) {
							auto node = maps[i]->getFirstNode();
							while (node) {
								sl_uint64 tick = node->value.lastAccessTick;
								if (tick > maxTick) {
									maxTick = tick;
								}
								if (!minTick || tick < minTick) {
									minTick = tick;
								}
								node = node->next;
							}
						}
					}
					sl_uint64 meanTick = (minTick + maxTick) / 2;
					{
						for (sl_uint32 i = 0; i < 2; i++) {
							const HashMap<MapTileAddress, Item>* map = maps[i];
							auto node = map->getFirstNode();
							while (node) {
								auto next = node->next;
								if (node->value.lastAccessTick >= meanTick) {
									map->removeAt(node);
								}
								node = next;
							}
						}
					}
				}
				Item item;
				item.object = object;
				item.lastAccessTick = System::getTickCount64();
				m_map.put_NoLock(address, Move(item));
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
			Ref<CRef> load(const MapTileLoadParam& param, sl_bool flagImage, const Function<Ref<CRef>(Memory&)>& loader, const Function<void(Ref<CRef>&)>& onComplete) override
			{
				Ref<CRef> ret;
				if (param.reader.isNull()) {
					return sl_null;
				}
				if (param.cache.isNotNull()) {
					if (param.cache->getObject(param.address, ret)) {
						if (flagImage) {
							if (IsInstanceOf<Image>(ret)) {
								onComplete(ret);
								return ret;
							}
						} else {
							if (loader.isNotNull()) {
								onComplete(ret);
								return ret;
							} else {
								if (IsInstanceOf<CMemory>(ret)) {
									onComplete(ret);
									return ret;
								}
							}
						}
					}
				}
				{
					ObjectLocker lock(&m_requests);
					Link<Request>* link = m_requests.getFront();
					while (link) {
						Request& request = link->value;
						if (request.reader == param.reader && request.address == param.address) {
							m_requests.removeLink(link);
							m_requests.pushLinkAtFront_NoLock(link);
							return sl_null;
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
				return sl_null;
			}

			sl_bool onLoad()
			{
				Link<Request>* link = m_requests.popLinkFromFront();
				if (!link) {
					return sl_false;
				}
				Request& request = link->value;
				Ref<CRef> ret;
				sl_bool flagSuccess = sl_false;
				if (request.flagImage) {
					flagSuccess = request.reader->readImage(*(reinterpret_cast<Ref<Image>*>(&ret)), request.address, request.timeout);
				} else {
					Memory mem;
					flagSuccess = request.reader->readData(mem, request.address, request.timeout);
					if (flagSuccess) {
						if (request.loader.isNotNull()) {
							ret = request.loader(mem);
						} else {
							ret = Move(mem.ref);
						}
					}
				}
				if (flagSuccess) {
					if (request.cache.isNotNull()) {
						request.cache->saveObject(request.address, ret);
					}
					request.onComplete(ret);
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

	Memory MapTileLoader::loadData(const MapTileLoadParam& param, const Function<void(Memory&)>& onComplete)
	{
		return Memory::cast(load(param, sl_false, sl_null, Function<void(Ref<CRef>&)>::cast(onComplete)));
	}

	Ref<Image> MapTileLoader::loadImage(const MapTileLoadParam& param, const Function<void(Ref<Image>&)>& onComplete)
	{
		return Ref<Image>::cast(load(param, sl_true, sl_null, Function<void(Ref<CRef>&)>::cast(onComplete)));
	}

	Ref<CRef> MapTileLoader::loadObject(const MapTileLoadParam& param, const Function<Ref<CRef>(Memory&)>& loader, const Function<void(Ref<CRef>&)>& onComplete)
	{
		return load(param, sl_false, loader, onComplete);
	}


	SLIB_DEFINE_OBJECT(MapPlane, Object)

	MapPlane::MapPlane()
	{
		m_viewX = 0.0;
		m_viewY = 0.0;
		m_viewWidth = 1.0;
		m_viewHeight = 1.0;
		m_range.left = -180.0;
		m_range.top = -90.0;
		m_range.right = 180.0;
		m_range.bottom = 90.0;
		m_scale = 1.0;
		m_scaleMin = 0.0;
		m_scaleMax = 10000000000.0;
	}

	MapPlane::~MapPlane()
	{
	}

	Double2 MapPlane::getViewportLocation()
	{
		return Double2(m_viewX, m_viewY);
	}

	void MapPlane::setViewportLocation(double x, double y)
	{
		m_viewX = x;
		m_viewY = y;
	}

	Double2 MapPlane::getViewportSize()
	{
		return Double2(m_viewWidth, m_viewHeight);
	}

	void MapPlane::setViewportSize(double width, double height)
	{
		m_viewWidth = width;
		m_viewHeight = height;
	}

	const Rectangle& MapPlane::getMapRange()
	{
		return m_range;
	}

	void MapPlane::setMapRange(const Rectangle& rect)
	{
		m_range = rect;
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

	void MapLayer::draw(Canvas* canvas, const Point& pt, MapViewData* data, MapPlane* plane)
	{
	}


	MapViewData::MapViewData()
	{
		m_view = sl_null;
		m_tileLoader = MapTileLoader::create();
		m_eyeLocation.altitude = 10000;
	}

	MapViewData::~MapViewData()
	{
	}

	Ref<MapPlane> MapViewData::getPlane() const
	{
		ObjectLocker locker(this);
		if (m_plane.isNotNull()) {
			return m_plane;
		}
		return m_surface;
	}

	void MapViewData::setPlane(const Ref<MapPlane>& plane, UIUpdateMode mode)
	{
		ObjectLocker locker(this);
		m_plane = plane;
		invalidate(mode);
	}

	Ref<MapSurface> MapViewData::getSurface() const
	{
		ObjectLocker locker(this);
		return m_surface;
	}

	void MapViewData::setSurface(const Ref<MapSurface>& surface, UIUpdateMode mode)
	{
		ObjectLocker locker(this);
		m_surface = surface;
		invalidate(mode);
	}

	List< Ref<MapLayer> > MapViewData::getLayers() const
	{
		ObjectLocker locker(this);
		return m_layers.getAllValues_NoLock();
	}

	void MapViewData::putLayer(const String& name, const Ref<MapLayer>& layer, UIUpdateMode mode)
	{
		ObjectLocker locker(this);
		if (layer.isNotNull()) {
			m_layers.put_NoLock(name, layer);
		} else {
			m_layers.remove_NoLock(name);
		}
		invalidate(mode);
	}

	GeoLocation MapViewData::getEyeLocation() const
	{
		return m_eyeLocation;
	}

	void MapViewData::setEyeLocation(const GeoLocation& location, UIUpdateMode mode)
	{
		m_eyeLocation = location;
		Ref<MapPlane> plane = getPlane();
		if (plane.isNotNull()) {
			plane->setEyeLocation(location);
		}
		invalidate(mode);
	}

	void MapViewData::zoom(double scale, UIUpdateMode mode)
	{
	}

	void MapViewData::draw(Canvas* canvas, const Point& pt)
	{
		Ref<MapPlane> plane = getPlane();
		if (plane.isNull()) {
			return;
		}
		plane->draw(canvas, pt, this);

		ObjectLocker locker(this);
		auto node = m_layers.getFirstNode();
		while (node) {
			Ref<MapLayer>& layer = node->value;
			if (layer->isSupportingPlaneMode()) {
				layer->draw(canvas, pt, this, plane.get());
			}
			node = node->next;
		}
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
		m_flagGlobeMode = sl_false;
		setRedrawMode(RedrawMode::WhenDirty);
	}

	MapView::~MapView()
	{
	}

	sl_bool MapView::isGlobeMode()
	{
		return m_flagGlobeMode;
	}

	void MapView::setGlobeMode(sl_bool flag, UIUpdateMode mode)
	{
		m_flagGlobeMode = flag;
		invalidate(mode);
	}

	void MapView::onDraw(Canvas* canvas)
	{
		if (!m_flagGlobeMode) {
			MapViewData::draw(canvas, Point::zero());
		}
	}

	void MapView::onFrame(RenderEngine* engine)
	{
		if (m_flagGlobeMode) {
		}
		RenderView::onFrame(engine);
	}

	void MapView::onMouseEvent(UIEvent* ev)
	{
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
