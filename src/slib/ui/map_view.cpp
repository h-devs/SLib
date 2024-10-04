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
#include "slib/network/url_request.h"
#include "slib/data/expiring_map.h"
#include "slib/system/system.h"
#include "slib/device/cpu.h"
#include "slib/math/triangle.h"
#include "slib/math/transform2d.h"
#include "slib/math/transform3d.h"
#include "slib/ui/core.h"
#include "slib/ui/resource.h"
#include "slib/ui/priv/view_state_map.h"
#include "slib/core/thread_pool.h"
#include "slib/core/stringify.h"
#include "slib/core/safe_static.h"

#define LAYER_COUNT SLIB_MAP_VIEW_LAYER_COUNT
#define EARTH_CIRCUMFERENCE SLIB_GEO_EARTH_CIRCUMFERENCE_EQUATORIAL
#define METER_PER_DEGREE ((double)EARTH_CIRCUMFERENCE / 360.0)
#define EXPAND_FACTOR 4.0

#define MAP_FOV_Y (SLIB_PI / 3.0)
#define ALTITUDE_RATIO 0.8660254037844386 // (1 - 0.5^2)^0.5

namespace slib
{

	namespace object_types
	{
		enum {
			MapViewObject = MapView + 1,
			MapPlane,
			MapSurface,
			MapSurfacePlane,
			MapViewTile,
			MapTileReader,
			MapTileDirectory,
			MapUrlReader,
			MapTileCache,
			MapTileLoader,
			MapViewExtension,
			MapViewObjectList,
			MapViewSprite
		};
	}

	namespace
	{
		class SharedContext
		{
		public:
			Ref<DispatchLoop> dispatchLoop;
			ExpiringMap< CRef*, Ref<Texture> > renderTextCache;

		public:
			SharedContext()
			{
				dispatchLoop = DispatchLoop::create();
				renderTextCache.setupTimer(10000, dispatchLoop);
			}
		};

		SLIB_SAFE_STATIC_GETTER(SharedContext, GetSharedContext)

		typedef MapView::Earth MapEarth;
	}

	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(MapTileAddress)

	MapTileAddress::MapTileAddress()
	{
	}

	sl_bool MapTileAddress::equals(const MapTileAddress& other) const noexcept
	{
		return level == other.level && E == other.E && N == other.N && subPath == other.subPath;
	}

	sl_compare_result MapTileAddress::compare(const MapTileAddress& other) const noexcept
	{
		sl_compare_result c = ((const MapTileLocationI&)*this).compare((const MapTileLocationI&)other);
		if (c) {
			return c;
		}
		return subPath.compare(other.subPath);
	}

	sl_size MapTileAddress::getHashCode() const noexcept
	{
		sl_size h = Hash<MapTileLocationI>()(*this);
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

	sl_bool MapTileReader::readObject(Ref<CRef>& _out, const MapTileAddress& address, const Function<Ref<CRef>(Memory&)>& loader, sl_uint32 timeout)
	{
		Memory data;
		if (readData(data, address, timeout)) {
			if (data.isNotNull()) {
				_out = loader(data);
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

	Ref<MapTileDirectory> MapTileDirectory::open(const String& rootPath, const Function<String(MapTileAddress&)>& formator)
	{
		Ref<MapTileDirectory> ret = new MapTileDirectory;
		if (ret.isNotNull()) {
			ret->m_root = rootPath;
			ret->m_formator = formator;
			return ret;
		}
		return sl_null;
	}

	sl_bool MapTileDirectory::readData(Memory& _out, const MapTileAddress& _address, sl_uint32 timeout)
	{
		MapTileAddress address = _address;
		String path = m_formator(address);
		if (m_root.isNotNull()) {
			if (address.subPath.isNotNull()) {
				_out = File::readAllBytes(File::concatPath(m_root, path, address.subPath));
			} else {
				_out = File::readAllBytes(File::concatPath(m_root, path));
			}
		} else {
			if (address.subPath.isNotNull()) {
				_out = File::readAllBytes(File::concatPath(path, address.subPath));
			} else {
				_out = File::readAllBytes(path);
			}
		}
		if (_out.isNotNull()) {
			return sl_true;
		}
		return File::isDirectory(m_root);
	}

	SLIB_DEFINE_OBJECT(MapUrlReader, MapTileReader)

	MapUrlReader::MapUrlReader()
	{
	}

	MapUrlReader::~MapUrlReader()
	{
	}

	Ref<MapUrlReader> MapUrlReader::create(const String& url, const Function<String(MapTileAddress&)>& formator)
	{
		Ref<MapUrlReader> ret = new MapUrlReader;
		if (ret.isNotNull()) {
			ret->m_root = url;
			ret->m_formator = formator;
			return ret;
		}
		return sl_null;
	}

	sl_bool MapUrlReader::readData(Memory& _out, const MapTileAddress& _address, sl_uint32 timeout)
	{
		MapTileAddress address = _address;
		String path = m_formator(address);
		if (m_root.isNotNull()) {
			if (address.subPath.isNotNull()) {
				return readUrl(_out, String::concat(m_root, StringView::literal("/"), path, StringView::literal("/"), address.subPath));
			} else {
				return readUrl(_out, String::concat(m_root, StringView::literal("/"), path));
			}
		} else {
			if (address.subPath.isNotNull()) {
				return readUrl(_out, String::concat(StringView::literal("/"), path, StringView::literal("/"), address.subPath));
			} else {
				return readUrl(_out, path);
			}
		}
		return sl_false;
	}

	sl_bool MapUrlReader::readUrl(Memory& _out, const String& url)
	{
		Ref<UrlRequest> request = UrlRequest::sendSynchronous(url);
		if (request.isNotNull()) {
			HttpStatus status = request->getResponseStatus();
			if (status == HttpStatus::OK) {
				_out = request->getResponseContent();
				return sl_true;
			} else if (status == HttpStatus::NotFound) {
				return sl_true;
			}
		}
		return sl_false;
	}

	SLIB_DEFINE_OBJECT(MapTileCache, Object)

	MapTileCache::MapTileCache()
	{
		m_nMaxCount = 0;
	}

	MapTileCache::~MapTileCache()
	{
	}

	namespace
	{
		class MapTileCacheImpl : public MapTileCache
		{
		public:
			HashMap< MapTileAddress, Ref<CRef> > m_activeObjects;
			HashMap< MapTileAddress, Ref<CRef> > m_backupObjects;
			HashMap< MapTileAddress, Ref<CRef> > m_endlessObjects;
			ExpiringMap< MapTileAddress, Ref<CRef> > m_expiringObjects;
			sl_uint32 m_lastActiveCount = 0;

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
				m_nMaxCount = nMaxCount;
			}

			sl_uint32 getLastActiveCount() override
			{
				return m_lastActiveCount;
			}

			void endStep() override
			{
				HashMap< MapTileAddress, Ref<CRef> > temp;
				{
					ObjectLocker lock(this);
					m_lastActiveCount = (sl_uint32)(m_activeObjects.getCount());
					temp = Move(m_backupObjects);
					m_backupObjects = Move(m_activeObjects);
				}
				ObjectLocker lock(&m_expiringObjects);
				if (m_expiringObjects.getCount() < m_nMaxCount) {
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
					if (m_activeObjects.get_NoLock(address, &_out)) {
						return sl_true;
					}
					if (m_endlessObjects.get_NoLock(address, &_out)) {
						return sl_true;
					}
					auto node = m_backupObjects.find_NoLock(address);
					if (node) {
						_out = Move(node->value);
						m_activeObjects.put_NoLock(address, _out);
						m_backupObjects.removeAt(node);
						return sl_true;
					}
				}
				if (m_expiringObjects.remove(address, &_out)) {
					ObjectLocker lock(this);
					m_activeObjects.put_NoLock(address, _out);
					return sl_true;
				}
				return sl_false;
			}

			sl_bool saveObject(const MapTileAddress& address, const Ref<CRef>& object, sl_bool flagEndless) override
			{
				if (flagEndless && object.isNotNull()) {
					if (m_endlessObjects.getCount() > m_nMaxCount) {
						return sl_false;
					}
					ObjectLocker lock(this);
					return m_endlessObjects.put_NoLock(address, object);
				} else {
					if (m_backupObjects.getCount() > m_nMaxCount) {
						return sl_false;
					}
					ObjectLocker lock(this);
					return m_backupObjects.put_NoLock(address, object);
				}
			}

			void clear() override
			{
				HashMap< MapTileAddress, Ref<CRef> > t1, t2, t3, t4, t5;
				{
					ObjectLocker lock(this);
					t1 = Move(m_activeObjects);
					t2 = Move(m_backupObjects);
					t3 = Move(m_endlessObjects);
					t4 = m_expiringObjects.pushInternalMap(sl_null);
					t5 = m_expiringObjects.pushInternalMap(sl_null);
				}
				SharedContext* context = GetSharedContext();
				if (context) {
					context->dispatchLoop->dispatch([t1, t2, t3, t4, t5]() {});
				}
			}
		};
	}

	Ref<MapTileCache> MapTileCache::create(sl_uint32 nMaxCount, sl_uint32 expiringMilliseconds)
	{
		return Ref<MapTileCache>::cast(MapTileCacheImpl::create(nMaxCount, expiringMilliseconds));
	}

	sl_uint32 MapTileCache::getMaximumActiveCount()
	{
		return m_nMaxCount;
	}


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(MapTileLoadParam)

	MapTileLoadParam::MapTileLoadParam()
	{
		timeout = 10000;
		flagLoadNow = sl_false;
		flagEndless = sl_false;
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
			Function<void()> m_onCompleteLazyLoading;

			class Request : public MapTileLoadParam
			{
			public:
				TYPE type;
				Function<Ref<CRef>(Memory&)> loader;
				Function<void(Ref<CRef>&)> onCompleteLazyLoading;
			};
			Queue<Request> m_requests;

		public:
			static Ref<MapTileLoaderImpl> create(const Function<void()>& onCompleteLazyLoading, sl_uint32 nThreads, sl_uint32 nMaxQueue)
			{
				if (!nThreads) {
					nThreads = Cpu::getCoreCount();
				}
				Ref<MapTileLoaderImpl> ret = new MapTileLoaderImpl;
				if (ret.isNull()) {
					return sl_null;
				}
				Ref<ThreadPool> threads = ThreadPool::create(SLIB_FUNCTION_MEMBER(ret.get(), runLazyLoading), nThreads);
				if (threads.isNull()) {
					return sl_null;
				}
				ret->m_threads = Move(threads);
				ret->m_nMaxQueue = nMaxQueue;
				ret->m_onCompleteLazyLoading = onCompleteLazyLoading;
				return ret;
			}

		public:
			sl_bool load( Ref<CRef>& _out, TYPE type, const MapTileLoadParam& param, const Function<Ref<CRef>(Memory&)>& loader, const Function<void(Ref<CRef>&)>& onCompleteLazyLoading) override
			{
				if (param.reader.isNull()) {
					return sl_false;
				}
				if (param.cache.isNull()) {
					if (doLoad(_out, type, param, loader)) {
						return sl_true;
					}
					return sl_false;
				}
				if (param.cache->getObject(param.address, _out)) {
					if (_out.isNull()) {
						return sl_true;
					}
					if (type == IMAGE) {
						if (IsInstanceOf<Image>(_out)) {
							return sl_true;
						}
					} else if (type == DATA) {
						if (IsInstanceOf<CMemory>(_out)) {
							return sl_true;
						}
					} else {
						return sl_true;
					}
				}
				if (param.cache->getLastActiveCount() >= param.cache->getMaximumActiveCount()) {
					return sl_false;
				}
				if (param.flagLoadNow) {
					if (doLoad(_out, type, param, loader)) {
						param.cache->saveObject(param.address, _out, param.flagEndless);
						return sl_true;
					}
					return sl_false;
				}
				_out.setNull();
				{
					ObjectLocker lock(&m_requests);
					Link<Request>* link = m_requests.getFront();
					while (link) {
						Request& request = link->value;
						if (request.reader == param.reader && request.address == param.address) {
							m_requests.removeLink(link);
							link->before = sl_null;
							link->next = sl_null;
							m_requests.pushLinkAtFront_NoLock(link);
							m_threads->wake();
							return sl_true;
						}
						link = link->next;
					}
					Request request;
					(MapTileLoadParam&)request = param;
					request.type = type;
					request.loader = loader;
					request.onCompleteLazyLoading = onCompleteLazyLoading;
					m_requests.pushFront_NoLock(Move(request));
					if (m_nMaxQueue && m_requests.getCount() > m_nMaxQueue) {
						m_requests.popBack_NoLock();
					}
					m_threads->wake();
				}
				return sl_true;
			}

			sl_bool doLoad(Ref<CRef>& _out, TYPE type, const MapTileLoadParam& param, const Function<Ref<CRef>(Memory&)>& loader)
			{
				if (type == IMAGE) {
					return param.reader->readImage(*(reinterpret_cast<Ref<Image>*>(&_out)), param.address, param.timeout);
				} else if (type == DATA) {
					return param.reader->readData(*(reinterpret_cast<Memory*>(&_out)), param.address, param.timeout);
				} else {
					return param.reader->readObject(_out, param.address, loader, param.timeout);
				}
			}

			sl_bool runLazyLoading()
			{
				Link<Request>* link = m_requests.popLinkFromFront();
				if (!link) {
					return sl_false;
				}
				Request& request = link->value;
				Ref<CRef> ret;
				if (doLoad(ret, request.type, request, request.loader)) {
					if (request.cache.isNotNull()) {
						if (request.cache->saveObject(request.address, ret, request.flagEndless)) {
							request.onCompleteLazyLoading(ret);
							m_onCompleteLazyLoading();
						}
					} else {
						request.onCompleteLazyLoading(ret);
						m_onCompleteLazyLoading();
					}
				}
				Queue<Request>::deleteLink(link);
				return sl_true;
			}
		};
	}

	Ref<MapTileLoader> MapTileLoader::create(const Function<void()>& onComplete, sl_uint32 nThreads, sl_uint32 nMaxQueue)
	{
		return Ref<MapTileLoader>::cast(MapTileLoaderImpl::create(onComplete, nThreads, nMaxQueue));
	}

	Ref<MapTileLoader> MapTileLoader::create(sl_uint32 nThreads, sl_uint32 nMaxQueue)
	{
		return create(sl_null, nThreads, nMaxQueue);
	}

	sl_bool MapTileLoader::loadData(Memory& _out, const MapTileLoadParam& param, const Function<void(Memory&)>& onComplete)
	{
		return load(Ref<CRef>::cast(_out.ref), DATA, param, sl_null, Function<void(Ref<CRef>&)>::cast(onComplete));
	}

	sl_bool MapTileLoader::loadImage(Ref<Image>& _out, const MapTileLoadParam& param, const Function<void(Ref<Image>&)>& onComplete)
	{
		return load(Ref<CRef>::cast(_out), IMAGE, param, sl_null, Function<void(Ref<CRef>&)>::cast(onComplete));
	}

	sl_bool MapTileLoader::loadObject(Ref<CRef>& _out, const MapTileLoadParam& param, const Function<Ref<CRef>(Memory&)>& loader, const Function<void(Ref<CRef>&)>& onComplete)
	{
		return load(_out, OBJECT, param, loader, onComplete);
	}


	SLIB_DEFINE_OBJECT(MapPlane, Object)

	MapPlane::MapPlane()
	{
		m_center.E = 0.0;
		m_center.N = 0.0;
		m_range.left = -10000000000.0;
		m_range.bottom = -10000000000.0;
		m_range.right = 10000000000.0;
		m_range.top = 10000000000.0;
		m_scale = 5000.0;
		m_minScale = 5000.0;
		m_maxScale = 50000000;
		m_viewport.left = 0.0;
		m_viewport.top = 0.0;
		m_viewport.right = 1.0;
		m_viewport.bottom = 1.0;
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
		double w = MapViewData::getMetersFromPixels(m_viewport.getWidth()) * m_scale;
		double h = MapViewData::getMetersFromPixels(m_viewport.getHeight()) * m_scale;
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
		m_scale = Math::clamp(scale, m_minScale, m_maxScale);
	}

	double MapPlane::getMinimumScale()
	{
		return m_minScale;
	}

	void MapPlane::setMinimumScale(double scale)
	{
		m_minScale = scale;
		setScale(m_scale);
	}

	double MapPlane::getMaximumScale()
	{
		return m_maxScale;
	}

	void MapPlane::setMaximumScale(double scale)
	{
		m_maxScale = scale;
		setScale(m_scale);
	}

	const RectangleT<double>& MapPlane::getViewport()
	{
		return m_viewport;
	}

	void MapPlane::setViewport(const RectangleT<double>& rect)
	{
		m_viewport = rect;
	}

	Ref<Drawable> MapPlane::getBackground()
	{
		return m_background;
	}

	void MapPlane::setBackground(const Ref<Drawable>& background)
	{
		m_background = background;
	}

	Double2 MapPlane::getViewPointFromMapLocation(const MapLocation& location)
	{
		return { m_viewport.getCenterX() + MapViewData::getPixelsFromMeters((location.E - m_center.E) / m_scale), m_viewport.getCenterY() + MapViewData::getPixelsFromMeters((m_center.N - location.N) / m_scale) };
	}

	MapLocation MapPlane::getMapLocationFromViewPoint(const Double2& point)
	{
		return { m_center.E + MapViewData::getMetersFromPixels(point.x - m_viewport.getCenterX()) * m_scale, m_center.N - MapViewData::getMetersFromPixels(point.y - m_viewport.getCenterY()) * m_scale };
	}

	double MapPlane::getViewLengthFromMapLength(double length)
	{
		return length * m_viewport.getHeight() / m_scale;
	}

	double MapPlane::getMapLengthFromViewLength(double length)
	{
		return length * m_scale / m_viewport.getHeight();
	}

	void MapPlane::draw(Canvas* canvas, MapViewData* data)
	{
		if (m_background.isNotNull()) {
			Ref<Drawable> background = m_background;
			if (background.isNotNull()) {
				canvas->draw(m_viewport, background);
			}
		}
		onDraw(canvas, data);
	}

	GeoLocation MapPlane::getEyeLocation()
	{
		return GeoLocation(getLatLonFromMapLocation(m_center), MapViewData::getAltitudeFromScale(m_scale, m_viewport.getHeight()));
	}

	void MapPlane::setEyeLocation(const GeoLocation& location)
	{
		setScale(MapViewData::getScaleFromAltitude(location.altitude, m_viewport.getHeight()));
		MapLocation m = getMapLocationFromLatLon(location.getLatLon());
		setCenterLocation(m.E, m.N);
	}


	SLIB_DEFINE_OBJECT(MapViewTile, CRef)

	MapViewTile::MapViewTile()
	{
	}

	MapViewTile::~MapViewTile()
	{
	}

	sl_bool MapViewTile::build(const MapSurfaceConfiguration& config, const Rectangle* demRect)
	{
		DEM model;
		model.initialize(config.demType, dem.getData(), dem.getSize(), 0, config.flagFlipDemY);

		double N0 = region.bottomLeft.latitude;
		double E0 = region.bottomLeft.longitude;
		double N1 = region.topRight.latitude;
		double E1 = region.topRight.longitude;
		double dN = N1 - N0;
		double dE = E1 - E0;

		sl_uint32 L = model.N;
		sl_uint32 M;

		Memory memVertices;
		if (L >= 2) {
			float* pixels = model.pixels;
			if (demRect) {
				M = (sl_uint32)(L * demRect->getWidth());
			} else {
				M = L;
			}
			if (M < config.minimumTileMatrixOrder) {
				M = config.minimumTileMatrixOrder;
			} else if (M > config.maximumTileMatrixOrder) {
				M = config.maximumTileMatrixOrder;
			}
			if (demRect || M != L) {
				memVertices = Memory::create(sizeof(MapViewVertex) * M * M);
				if (memVertices.isNull()) {
					return sl_false;
				}
				float mx0, my0, mx1, my1;
				MapViewVertex* v = (MapViewVertex*)(memVertices.getData());
				if (demRect) {
					mx0 = demRect->left * (float)(L - 1);
					my0 = demRect->top * (float)(L - 1);
					mx1 = demRect->right * (float)(L - 1);
					my1 = demRect->bottom * (float)(L - 1);
				} else {
					mx0 = 0.0f;
					my0 = 0.0f;
					mx1 = (float)(L - 1);
					my1 = (float)(L - 1);
				}
				float dmx = mx1 - mx0;
				float dmy = my1 - my0;
				for (sl_uint32 y = 0; y < M; y++) {
					for (sl_uint32 x = 0; x < M; x++) {
						float mx = mx0 + dmx * (float)x / (float)(M - 1);
						float my = my0 + dmy * (float)y / (float)(M - 1);
						sl_int32 mxi = (sl_int32)(mx);
						sl_int32 myi = (sl_int32)(my);
						float mxf;
						float myf;
						if (mxi < 0) {
							mxi = 0;
							mxf = 0.0f;
						} else if (mxi >= (sl_int32)L - 1) {
							mxi = L - 2;
							mxf = 1.0f;
						} else {
							mxf = mx - (float)mxi;
						}
						if (myi < 0) {
							myi = 0;
							myf = 0.0f;
						} else if (myi >= (sl_int32)L - 1) {
							myi = L - 2;
							myf = 1.0f;
						} else {
							myf = my - (float)myi;
						}
						sl_int32 p = mxi + myi * L;
						float altitude = (1.0f - mxf) * (1.0f - myf) * pixels[p] + (1.0f - mxf) * myf * pixels[p + L] + mxf * (1.0f - myf) * pixels[p + 1] + mxf * myf * pixels[p + 1 + L];
						buildVertex(*v, N0 + dN * (double)(M - 1 - y) / (double)(M - 1), E0 + dE * (double)x / (double)(M - 1), altitude, (sl_real)x / (sl_real)(M - 1), (sl_real)y / (sl_real)(M - 1));
						v++;
					}
				}
			} else {
				memVertices = Memory::create(sizeof(MapViewVertex) * M * M);
				if (memVertices.isNull()) {
					return sl_false;
				}
				MapViewVertex* v = (MapViewVertex*)(memVertices.getData());
				for (sl_uint32 y = 0; y < M; y++) {
					for (sl_uint32 x = 0; x < M; x++) {
						buildVertex(*v, N0 + dN * (double)(M - 1 - y) / (double)(M - 1), E0 + dE * (double)x / (double)(M - 1), *pixels, (sl_real)x / (sl_real)(M - 1), (sl_real)y / (sl_real)(M - 1));
						v++;
						pixels++;
					}
				}
			}
		} else {
			double altitude = 0.0;
			if (L) {
				altitude = *(model.pixels);
			}
			M = config.minimumTileMatrixOrder;
			memVertices = Memory::create(sizeof(MapViewVertex) * M * M);
			if (memVertices.isNull()) {
				return sl_false;
			}
			MapViewVertex* v = (MapViewVertex*)(memVertices.getData());
			for (sl_uint32 y = 0; y < M; y++) {
				for (sl_uint32 x = 0; x < M; x++) {
					buildVertex(*v, N0 + dN * (double)(M - 1 - y) / (double)(M - 1), E0 + dE * (double)x / (double)(M - 1), altitude, (sl_real)x / (sl_real)(M - 1), (sl_real)y / (sl_real)(M - 1));
					v++;
				}
			}
		}
		Ref<VertexBuffer> vb = VertexBuffer::create(memVertices);
		if (vb.isNull()) {
			return sl_false;
		}
		{
			MapViewVertex* v = (MapViewVertex*)(memVertices.getData());
			pointsWithDEM[0] = center + v[(M - 1) * M].position; // Bottom Left
			pointsWithDEM[1] = center + v[M * M - 1].position; // Bottom Right
			pointsWithDEM[2] = center + v[0].position; // Top Left
			pointsWithDEM[3] = center + v[M - 1].position; // Top Right
		}
		{
			primitive.countElements = 6 * (M - 1) * (M - 1);
			Memory mem = Memory::create(primitive.countElements << 1);
			if (mem.isNull()) {
				return sl_false;
			}
			sl_uint16* indices = (sl_uint16*)(mem.getData());
			for (sl_uint32 y = 0; y < M - 1; y++) {
				for (sl_uint32 x = 0; x < M - 1; x++) {
					sl_uint16 tl = (sl_uint16)(y * M + x); // Top Left
					sl_uint16 tr = (sl_uint16)(y * M + (x + 1)); // Top Right
					sl_uint16 bl = (sl_uint16)((y + 1) * M + x); // Bottom Left
					sl_uint16 br = (sl_uint16)((y + 1) * M + (x + 1)); // Bottom Right
					*(indices++) = tl;
					*(indices++) = tr;
					*(indices++) = bl;
					*(indices++) = bl;
					*(indices++) = tr;
					*(indices++) = br;
				}
			}
			primitive.indexBuffer = IndexBuffer::create(mem);
			if (primitive.indexBuffer.isNull()) {
				return sl_false;
			}
		}
		primitive.vertexBuffer = Move(vb);
		return sl_true;
	}

	void MapViewTile::buildVertex(MapViewVertex& vertex, double latitude, double longitude, double altitude, sl_real tx, sl_real ty)
	{
		vertex.position = MapEarth::getCartesianPosition(latitude, longitude, altitude) - center;
		vertex.texCoord.x = tx;
		vertex.texCoord.y = ty;
	}


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(MapSurfaceConfiguration)

	MapSurfaceConfiguration::MapSurfaceConfiguration()
	{
		baseLevel = 0;
		baseTileCountE = 1;
		baseTileCountN = 1;
		minimumLevel = 0;
		maximumLevel = 20;
		eastingRangeInDegrees = 360.0;
		northingRangeInDegrees = 360.0;
		tileDimensionInPixels = 256;
		minimumTileMatrixOrder = 15;
		maximumTileMatrixOrder = 65;
		demType = DEM::DataType::FloatLE;
		flagFlipDemY = sl_false;
	}

	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(MapSurfaceParam)

	MapSurfaceParam::MapSurfaceParam()
	{
	}

	SLIB_DEFINE_OBJECT(MapSurface, Object)

	MapSurface::MapSurface()
	{
		for (sl_uint32 i = 0; i < LAYER_COUNT; i++) {
			m_layers[i].flagVisible = sl_true;
			m_layers[i].opacity = 1.0f;
		}
	}

	MapSurface::~MapSurface()
	{
	}

	namespace
	{
		SLIB_RENDER_PROGRAM_STATE_BEGIN(RenderProgramState_SurfaceTile, MapViewVertex)
			SLIB_RENDER_PROGRAM_STATE_UNIFORM_MATRIX4(Transform, u_Transform, RenderShaderType::Vertex, 0)
			SLIB_RENDER_PROGRAM_STATE_UNIFORM_TEXTURE(Texture, u_Texture, RenderShaderType::Pixel, 0)
			SLIB_RENDER_PROGRAM_STATE_UNIFORM_TEXTURE(LayerTexture0, u_LayerTexture0, RenderShaderType::Pixel, 1)
			SLIB_RENDER_PROGRAM_STATE_UNIFORM_TEXTURE(LayerTexture1, u_LayerTexture1, RenderShaderType::Pixel, 2)
			SLIB_RENDER_PROGRAM_STATE_UNIFORM_TEXTURE(LayerTexture2, u_LayerTexture2, RenderShaderType::Pixel, 3)
			SLIB_RENDER_PROGRAM_STATE_UNIFORM_TEXTURE(LayerTexture3, u_LayerTexture3, RenderShaderType::Pixel, 4)
			SLIB_RENDER_PROGRAM_STATE_UNIFORM_TEXTURE(LayerTexture4, u_LayerTexture4, RenderShaderType::Pixel, 5)
			SLIB_RENDER_PROGRAM_STATE_UNIFORM_VECTOR4(TextureRect, u_TextureRect, RenderShaderType::Pixel, 0)
			SLIB_RENDER_PROGRAM_STATE_UNIFORM_VECTOR4_ARRAY(LayerTextureRect, u_LayerTextureRect, RenderShaderType::Pixel, 1)
			SLIB_RENDER_PROGRAM_STATE_UNIFORM_FLOAT_ARRAY(LayerAlpha, u_LayerAlpha, RenderShaderType::Pixel, 6)

			SLIB_RENDER_PROGRAM_STATE_INPUT_FLOAT3(position, a_Position, RenderInputSemanticName::Position)
			SLIB_RENDER_PROGRAM_STATE_INPUT_FLOAT2(texCoord, a_TexCoord, RenderInputSemanticName::TexCoord)
		SLIB_RENDER_PROGRAM_STATE_END

		class RenderProgram_SurfaceTile : public RenderProgramT<RenderProgramState_SurfaceTile>
		{
		public:
			String getGLSLVertexShader(RenderEngine* engine) override
			{
				SLIB_RETURN_STRING(SLIB_STRINGIFY(
					uniform mat4 u_Transform;
					attribute vec3 a_Position;
					attribute vec2 a_TexCoord;
					varying vec2 v_TexCoord;
					void main() {
						vec4 P = vec4(a_Position, 1.0) * u_Transform;
						gl_Position = P;
						v_TexCoord = a_TexCoord;
					}
				))
			}

			String getGLSLFragmentShader(RenderEngine* engine) override
			{
				String source;
				SLIB_RETURN_STRING(SLIB_STRINGIFY(
					uniform sampler2D u_Texture;
					uniform sampler2D u_LayerTexture0;
					uniform sampler2D u_LayerTexture1;
					uniform sampler2D u_LayerTexture2;
					uniform sampler2D u_LayerTexture3;
					uniform sampler2D u_LayerTexture4;
					uniform vec4 u_TextureRect;
					uniform vec4 u_LayerTextureRect[5];
					uniform float u_LayerAlpha[5];
					varying vec2 v_TexCoord;
					void main() {
						vec4 colorTexture = texture2D(u_Texture, v_TexCoord * u_TextureRect.zw + u_TextureRect.xy);
						vec4 colorLayer0 = texture2D(u_LayerTexture0, v_TexCoord * u_LayerTextureRect[0].zw + u_LayerTextureRect[0].xy);
						vec4 colorLayer1 = texture2D(u_LayerTexture1, v_TexCoord * u_LayerTextureRect[1].zw + u_LayerTextureRect[1].xy);
						vec4 colorLayer2 = texture2D(u_LayerTexture2, v_TexCoord * u_LayerTextureRect[2].zw + u_LayerTextureRect[2].xy);
						vec4 colorLayer3 = texture2D(u_LayerTexture3, v_TexCoord * u_LayerTextureRect[3].zw + u_LayerTextureRect[3].xy);
						vec4 colorLayer4 = texture2D(u_LayerTexture4, v_TexCoord * u_LayerTextureRect[4].zw + u_LayerTextureRect[4].xy);

						float a = colorLayer0.a * u_LayerAlpha[0];
						colorLayer0.a = 1.0;
						vec4 c = colorTexture * (1.0 - a) + colorLayer0 * a;

						a = colorLayer1.a * u_LayerAlpha[1];
						colorLayer1.a = 1.0;
						c = c * (1.0 - a) + colorLayer1 * a;

						a = colorLayer2.a * u_LayerAlpha[2];
						colorLayer2.a = 1.0;
						c = c * (1.0 - a) + colorLayer2 * a;

						a = colorLayer3.a * u_LayerAlpha[3];
						colorLayer3.a = 1.0;
						c = c * (1.0 - a) + colorLayer3 * a;

						a = colorLayer4.a * u_LayerAlpha[4];
						colorLayer4.a = 1.0;
						c = c * (1.0 - a) + colorLayer4 * a;

						gl_FragColor = c;
					}
				))
			}

			String getHLSLVertexShader(RenderEngine* engine) override
			{
				SLIB_RETURN_STRING(SLIB_STRINGIFY(
					float4x4 u_Transform : register(c0);
					struct VS_OUTPUT {
						float2 texcoord : TEXCOORD;
						float4 pos : POSITION;
					};
					VS_OUTPUT main(float3 a_Position : POSITION, float2 a_TexCoord : TEXCOORD) {
						VS_OUTPUT ret;
						ret.pos = mul(float4(a_Position, 1.0), u_Transform);
						ret.texcoord = a_TexCoord;
						return ret;
					}
				))
			}

			String getHLSLPixelShader(RenderEngine* engine) override
			{
				SLIB_RETURN_STRING(SLIB_STRINGIFY(
					sampler u_Texture;
					sampler u_LayerTexture0;
					sampler u_LayerTexture1;
					sampler u_LayerTexture2;
					sampler u_LayerTexture3;
					sampler u_LayerTexture4;
					float4 u_TextureRect : register(c0);
					float4 u_LayerTextureRect[5] : register(c1);
					float u_LayerAlpha[5] : register(c6);
					float4 main(float2 v_TexCoord : TEXCOORD) : COLOR {
						float4 colorTexture = tex2D(u_Texture, v_TexCoord * u_TextureRect.zw + u_TextureRect.xy);
						float4 colorLayer0 = tex2D(u_LayerTexture0, v_TexCoord * u_LayerTextureRect[0].zw + u_LayerTextureRect[0].xy);
						float4 colorLayer1 = tex2D(u_LayerTexture1, v_TexCoord * u_LayerTextureRect[1].zw + u_LayerTextureRect[1].xy);
						float4 colorLayer2 = tex2D(u_LayerTexture2, v_TexCoord * u_LayerTextureRect[2].zw + u_LayerTextureRect[2].xy);
						float4 colorLayer3 = tex2D(u_LayerTexture3, v_TexCoord * u_LayerTextureRect[3].zw + u_LayerTextureRect[3].xy);
						float4 colorLayer4 = tex2D(u_LayerTexture4, v_TexCoord * u_LayerTextureRect[4].zw + u_LayerTextureRect[4].xy);

						float a = colorLayer0.a * u_LayerAlpha[0];
						colorLayer0.a = 1.0;
						float4 c = colorTexture * (1.0 - a) + colorLayer0 * a;

						a = colorLayer1.a * u_LayerAlpha[1];
						colorLayer1.a = 1.0;
						c = c * (1.0 - a) + colorLayer1 * a;

						a = colorLayer2.a * u_LayerAlpha[2];
						colorLayer2.a = 1.0;
						c = c * (1.0 - a) + colorLayer2 * a;

						a = colorLayer3.a * u_LayerAlpha[3];
						colorLayer3.a = 1.0;
						c = c * (1.0 - a) + colorLayer3 * a;

						a = colorLayer4.a * u_LayerAlpha[4];
						colorLayer4.a = 1.0;
						c = c * (1.0 - a) + colorLayer4 * a;

						return c;
					}
				))
			}
		};

		class MapSurfaceImpl : public MapSurface
		{
		public:
			Ref<MapTileCache> m_cachePicture;
			Ref<MapTileCache> m_cacheDEM;
			Ref<MapTileCache> m_cacheLayers[LAYER_COUNT];

			class TileImage
			{
			public:
				Ref<Image> source;
				sl_bool flagDrawWhole = sl_true;
				Rectangle region;

			public:
				void convertToSourceCoordinate()
				{
					if (flagDrawWhole) {
						return;
					}
					sl_real w = (sl_real)(source->getWidth());
					region.left *= w;
					region.right *= w;
					sl_real h = (sl_real)(source->getHeight());
					region.top *= h;
					region.bottom *= h;
				}
			};

			struct TileDEM
			{
				Memory source;
				sl_bool flagUseWhole = sl_true;
				Rectangle region;
			};

			HashMap< MapTileLocationI, Ref<MapViewTile> > m_currentTiles;
			HashMap< MapTileLocationI, Ref<MapViewTile> > m_backupTiles;
			List< Ref<MapViewTile> > m_renderingTiles;

			Ref<RenderProgram> m_programSurfaceTile;

		public:
			static Ref<MapSurfaceImpl> create(const MapSurfaceParam& param)
			{
				Ref<MapSurfaceImpl> ret = new MapSurfaceImpl;
				if (ret.isNull()) {
					return sl_null;
				}
				if (ret->initialize(param)) {
					return ret;
				}
				return sl_null;
			}

		public:
			sl_bool initialize(const MapSurfaceParam& param)
			{
				m_config = param;
				if (m_config.minimumLevel < param.baseLevel) {
					m_config.minimumLevel = param.baseLevel;
				}
				m_toReaderLocation = param.toReaderLocation;
				m_readerPicture = param.picture;
				m_readerDEM = param.dem;
				{
					m_cachePicture = MapTileCache::create(400, 10000);
					if (m_cachePicture.isNull()) {
						return sl_false;
					}
					m_cacheDEM = MapTileCache::create(400, 10000);
					if (m_cacheDEM.isNull()) {
						return sl_false;
					}
					for (sl_uint32 i = 0; i < LAYER_COUNT; i++) {
						Ref<MapTileCache> cache = MapTileCache::create(400, 5000);
						if (cache.isNull()) {
							return sl_false;
						}
						m_cacheLayers[i] = Move(cache);
						m_layers[i].reader = param.layers[i];
					}
				}

				m_programSurfaceTile = new RenderProgram_SurfaceTile;
				if (m_programSurfaceTile.isNull()) {
					return sl_false;
				}
				return sl_true;
			}

			void render(RenderEngine* engine, MapViewData* data) override
			{
				m_renderingTiles.setNull();
				const MapViewState& state = data->getMapState();
				sl_uint32 M = 1 << (m_config.minimumLevel - m_config.baseLevel);
				sl_uint32 nN = m_config.baseTileCountN * M;
				sl_uint32 nE = m_config.baseTileCountE * M;
				for (sl_uint32 y = 0; y < nN; y++) {
					for (sl_uint32 x = 0; x < nE; x++) {
						renderTile(engine, state, MapTileLocationI(m_config.minimumLevel, x, y));
					}
				}
				{
					m_backupTiles = Move(m_currentTiles);
					m_cachePicture->endStep();
					m_cacheDEM->endStep();
					for (sl_uint32 i = 0; i < LAYER_COUNT; i++) {
						m_cacheLayers[i]->endStep();
					}
				}
			}

			void renderTile(RenderEngine* engine, const MapViewState& state, const MapTileLocationI& location)
			{
				Ref<MapViewTile> tile = getTile(location);
				if (tile.isNull()) {
					return;
				}
				if (isTileExpandable(state, tile.get())) {
					sl_uint32 E = location.E << 1;
					sl_uint32 N = location.N << 1;
					for (sl_uint32 y = 0; y < 2; y++) {
						for (sl_uint32 x = 0; x < 2; x++) {
							renderTile(engine, state, MapTileLocationI(location.level + 1, E + x, N + y));
						}
					}
					return;
				}
				if (!(isTileFrontFace(state, tile.get()))) {
					return;
				}
				MapTileLoader* loader = state.tileLoader.get();
				TileImage image;
				if (!(loadPicture(image, loader, location))) {
					return;
				}
				TileDEM dem;
				loadDEM(dem, loader, location);
				if (tile->primitive.vertexBuffer.isNull() || tile->dem != dem.source) {
					tile->dem = dem.source;
					if (!(tile->build(m_config, dem.flagUseWhole ? sl_null : &(dem.region)))) {
						return;
					}
				}
				RenderProgramScope<RenderProgramState_SurfaceTile> scope;
				if (scope.begin(engine, m_programSurfaceTile)) {
					scope->setTransform(Transform3T<double>::getTranslationMatrix(tile->center) * state.viewProjectionTransform);
					scope->setTexture(Texture::getBitmapRenderingCache(image.source));
					scope->setTextureRect(Vector4(image.region.left, image.region.top, image.region.getWidth(), image.region.getHeight()));
					float layerAlphas[LAYER_COUNT];
					Ref<Texture> layerTextures[LAYER_COUNT];
					Vector4 layerTextureRects[LAYER_COUNT];
					for (sl_uint32 iLayer = 0; iLayer < LAYER_COUNT; iLayer++) {
						layerAlphas[iLayer] = 0;
						layerTextureRects[iLayer] = Vector4::zero();
						Layer& layer = m_layers[iLayer];
 						if (layer.reader.isNull()) {
							continue;
						}
						if (!(layer.flagVisible)) {
							continue;
						}
						if (layer.opacity < 0.001f) {
							continue;
						}
						if (!(loadImage(image, layer.reader, m_cacheLayers[iLayer].get(), loader, location))) {
							continue;
						}
						layerAlphas[iLayer] = layer.opacity;
						layerTextures[iLayer] = Texture::getBitmapRenderingCache(image.source);
						layerTextureRects[iLayer] = Vector4(image.region.left, image.region.top, image.region.getWidth(), image.region.getHeight());
					}
					scope->setLayerTexture0(layerTextures[0]);
					scope->setLayerTexture1(layerTextures[1]);
					scope->setLayerTexture2(layerTextures[2]);
					scope->setLayerTexture3(layerTextures[3]);
					scope->setLayerTexture4(layerTextures[4]);
					scope->setLayerTextureRect(layerTextureRects, LAYER_COUNT);
					scope->setLayerAlpha(layerAlphas, LAYER_COUNT);
					engine->drawPrimitive(&(tile->primitive));
				}
				m_renderingTiles.add_NoLock(tile);
			}

			Ref<MapViewTile> getTile(const MapTileLocationI& location)
			{
				Ref<MapViewTile> ret;
				if (m_currentTiles.get_NoLock(location, &ret)) {
					return ret;
				}
				if (m_backupTiles.remove_NoLock(location, &ret)) {
					m_currentTiles.put_NoLock(location, ret);
					return ret;
				}
				ret = createTile(location);
				if (ret.isNull()) {
					return sl_null;
				}
				m_currentTiles.put(location, ret);
				return ret;
			}

			Ref<MapViewTile> createTile(const MapTileLocationI& location)
			{
				Ref<MapViewTile> tile = new MapViewTile;
				if (tile.isNull()) {
					return sl_null;
				}
				tile->location = location;

				GeoRectangle& region = tile->region;
				region.bottomLeft = getLatLonFromTileLocation(location);
				region.topRight = getLatLonFromTileLocation(MapTileLocationI(location.level, location.E + 1, location.N + 1));

				tile->pointsWithDEM[0] = tile->points[0] = MapEarth::getCartesianPosition(region.bottomLeft.latitude, region.bottomLeft.longitude, 0);
				tile->pointsWithDEM[1] = tile->points[1] = MapEarth::getCartesianPosition(region.bottomLeft.latitude, region.topRight.longitude, 0);
				tile->pointsWithDEM[2] = tile->points[2] = MapEarth::getCartesianPosition(region.topRight.latitude, region.bottomLeft.longitude, 0);
				tile->pointsWithDEM[3] = tile->points[3] = MapEarth::getCartesianPosition(region.topRight.latitude, region.topRight.longitude, 0);
				tile->center = MapEarth::getCartesianPosition((region.bottomLeft.latitude + region.topRight.latitude) / 2.0, (region.topRight.longitude + region.bottomLeft.longitude) / 2.0, 0);

				return tile;
			}

			static sl_bool isTileFrontFace(const MapViewState& state, MapViewTile* tile)
			{
				if (tile->region.topRight.longitude - tile->region.bottomLeft.longitude > 1.0) {
					return isTileFrontFace(state, tile->points, sl_false);
				} else {
					return isTileFrontFace(state, tile->pointsWithDEM, sl_true);
				}
			}

			static sl_bool isTileFrontFace(const MapViewState& state, const Double3 inputs[4], sl_bool flagUseProjection)
			{
				Vector2 points[4]; // Bottom Left, Bottom Right, Top Left, Top Right;
				if (flagUseProjection) {
					sl_uint32 nBehind = 0;
					for (sl_size i = 0; i < 4; i++) {
						const Double3& input = inputs[i];
						Vector4 pt = state.viewProjectionTransform.multiplyLeft(Double4(input.x, input.y, input.z, 1.0));
						if (pt.w < 0.00001f) {
							nBehind++;
						} else {
							points[i] = Vector2(pt.x / pt.w, pt.y / pt.w);
						}
					}
					if (nBehind == 4) {
						return sl_false;
					}
					if (nBehind) {
						return sl_true;
					}
				} else {
					for (sl_size i = 0; i < 4; i++) {
						const Double3& input = inputs[i];
						Vector3 pt = state.viewTransform.transformPosition(inputs[i]);
						points[i] = Vector2(pt.x, pt.y);
					}
				}
				Triangle triangle;
				triangle.point1.x = points[2].x;
				triangle.point1.y = -points[2].y;
				triangle.point2.x = points[3].x;
				triangle.point2.y = -points[3].y;
				triangle.point3.x = points[0].x;
				triangle.point3.y = -points[0].y;
				if (triangle.isClockwise()) {
					return sl_true;
				}
				triangle.point1.x = points[1].x;
				triangle.point1.y = -points[1].y;
				return !(triangle.isClockwise());
			}

			sl_bool isTileExpandable(const MapViewState& state, MapViewTile* tile)
			{
				// Check Expand
				if (tile->location.level >= m_config.maximumLevel) {
					return sl_false;
				}
				// Check Distance
				{
					double r = MapEarth::getRadius();
					double d = state.eyeLocation.altitude + r / 2.0;
					if ((state.eyePoint - tile->center).getLength2p() > d * d) {
						return sl_false;
					}
				}
				// Check Frustum
				{
					if (!(state.viewFrustum.containsFacets(tile->pointsWithDEM, 4))) {
						return sl_false;
					}
				}
				// Check Normal
				{
					sl_bool f = sl_false;
					for (sl_uint32 i = 0; i < 4; i++) {
						Double3 normal = state.viewTransform.transformDirection(tile->points[i]);
						if (normal.z <= 0.0) {
							f = sl_true;
							break;
						}
					}
					if (!f) {
						return sl_false;
					}
				}

				Vector3 ptBL = state.viewTransform.transformPosition(tile->points[0]);
				Vector3 ptBR = state.viewTransform.transformPosition(tile->points[1]);
				Vector3 ptTL = state.viewTransform.transformPosition(tile->points[2]);
				Vector3 ptTR = state.viewTransform.transformPosition(tile->points[3]);

				// Check Behind
				{
					sl_uint32 nBehind = 0;
					if (Math::isLessThanEpsilon(ptBL.z)) {
						nBehind++;
					}
					if (Math::isLessThanEpsilon(ptBR.z)) {
						nBehind++;
					}
					if (Math::isLessThanEpsilon(ptTL.z)) {
						nBehind++;
					}
					if (Math::isLessThanEpsilon(ptTR.z)) {
						nBehind++;
					}
					if (nBehind == 4) {
						return sl_false;
					}
					if (nBehind) {
						return sl_true;
					}
				}
				// Check Size
				{
					Triangle t;
					t.point1.x = ptBL.x / ptBL.z;
					t.point1.y = ptBL.y / ptBL.z;
					t.point2.x = ptBR.x / ptBR.z;
					t.point2.y = ptBR.y / ptBR.z;
					t.point3.x = ptTL.x / ptTL.z;
					t.point3.y = ptTL.y / ptTL.z;
					sl_real size = Math::abs(t.getSize());
					t.point1.x = ptTR.x / ptTR.z;
					t.point1.y = ptTR.y / ptTR.z;
					size += Math::abs(t.getSize());
					if (size > (sl_real)(65536.0 * EXPAND_FACTOR / state.viewportWidth / state.viewportWidth)) {
						return sl_true;
					}
				}
				return sl_false;
			}

			const List< Ref<MapViewTile> >& getTiles() override
			{
				return m_renderingTiles;
			}

			double getAltitudeAt(MapTileLoader* loader, const LatLon& location) override
			{
				MapTileLocation tloc = getTileLocationFromLatLon(m_config.maximumLevel, location);
				MapTileLocationI tloci = tloc;
				TileDEM dem;
				if (loadDEM(dem, loader, tloci)) {
					DEM model;
					if (model.initialize(m_config.demType, dem.source.getData(), dem.source.getSize(), 0, m_config.flagFlipDemY)) {
						return model.getAltitudeAt(dem.region.left + (float)(tloc.E - (double)(tloci.E)) * dem.region.getWidth(), dem.region.top + (float)(1.0 - (tloc.N - (double)(tloci.N))) * dem.region.getHeight());
					}
				}
				return 0.0;
			}

			void onDrawPlane(Canvas* canvas, const Rectangle& rect, MapSurfacePlane* plane, MapViewData* data) override
			{
				double planeScale = plane->getScale();
				double planeMpp = planeScale / rect.getHeight();
				planeMpp *= 2.5; // factor
				double tileMpp = METER_PER_DEGREE * m_config.eastingRangeInDegrees / (double)(m_config.baseTileCountE) / (double)(m_config.tileDimensionInPixels) / (double)(1 << (m_config.minimumLevel - m_config.baseLevel));
				sl_uint32 level = m_config.minimumLevel;
				do {
					if (planeMpp > tileMpp) {
						break;
					}
					tileMpp /= 2.0;
					level++;
				} while (level < m_config.maximumLevel);

				drawLevel(canvas, rect, level, plane->getCenterLocation(), planeScale, data->getMapState().tileLoader.get(), tileMpp);

				{
					m_cachePicture->endStep();
					for (sl_uint32 i = 0; i < LAYER_COUNT; i++) {
						m_cacheLayers[i]->endStep();
					}
				}
			}

			void drawLevel(Canvas* canvas, const Rectangle& rcView, sl_uint32 level, const MapLocation& center, double planeScale, MapTileLoader* loader, double tileMpp)
			{
				double h = planeScale / tileMpp;
				double w = h * rcView.getWidth() / rcView.getHeight();
				double sx = (center.E + METER_PER_DEGREE * m_config.eastingRangeInDegrees / 2.0) / tileMpp - w / 2.0;
				double sy = (center.N + METER_PER_DEGREE * m_config.northingRangeInDegrees / 2.0) / tileMpp - h / 2.0;
				double ex = sx + w;
				double ey = sy + h;
				sl_uint64 m = (sl_uint64)1 << level;
				double mw = (double)(m_config.tileDimensionInPixels * m_config.baseTileCountE * m);
				double mh = (double)(m_config.tileDimensionInPixels * m_config.baseTileCountN * m);
				sl_uint32 isx = (sl_uint32)(Math::clamp(sx, 0.0, mw));
				sl_uint32 iex = (sl_uint32)(Math::clamp(ex, 0.0, mw));
				sl_uint32 isy = (sl_uint32)(Math::clamp(sy, 0.0, mh));
				sl_uint32 iey = (sl_uint32)(Math::clamp(ey, 0.0, mh));
				if (iex > isx + 4096) {
					iex = isx + 4096;
				}
				if (iey > isy + 4096) {
					iey = isy + 4096;
				}
				sl_uint32 tsx = isx / m_config.tileDimensionInPixels;
				sl_uint32 tsy = isy / m_config.tileDimensionInPixels;
				sl_uint32 tex = iex / m_config.tileDimensionInPixels;
				sl_uint32 tey = iey / m_config.tileDimensionInPixels;
				if (iex % m_config.tileDimensionInPixels) {
					tex++;
				}
				if (iey % m_config.tileDimensionInPixels) {
					tey++;
				}
				double scale = rcView.getHeight() / h;
				sl_real ts = (sl_real)(m_config.tileDimensionInPixels * scale);
				for (sl_uint32 ty = tsy; ty < tey; ty++) {
					for (sl_uint32 tx = tsx; tx < tex; tx++) {
						MapTileLocationI location(level, tx, ty);
						TileImage image;
						if (!(loadPicture(image, loader, location))) {
							continue;
						}
						image.convertToSourceCoordinate();
						Rectangle rcDst;
						rcDst.left = rcView.left + (sl_real)(((double)(tx * m_config.tileDimensionInPixels) - sx) * scale);
						rcDst.top = rcView.bottom - (sl_real)(((double)((ty + 1) * m_config.tileDimensionInPixels) - sy) * scale);
						rcDst.setWidth(ts);
						rcDst.setHeight(ts);
						if (image.flagDrawWhole) {
							canvas->draw(rcDst, image.source);
						} else {
							canvas->draw(rcDst, image.source, image.region);
						}
						for (sl_uint32 i = 0; i < LAYER_COUNT; i++) {
							Layer& layer = m_layers[i];
							if (layer.reader.isNull()) {
								continue;
							}
							if (!(layer.flagVisible)) {
								continue;
							}
							if (layer.opacity < 0.001f) {
								continue;
							}
							if (!(loadImage(image, layer.reader, m_cacheLayers[i].get(), loader, location))) {
								continue;
							}
							image.convertToSourceCoordinate();
							if (layer.opacity < 0.999f) {
								Canvas::DrawParam param;
								param.useAlpha = sl_true;
								param.alpha = layer.opacity;
								if (image.flagDrawWhole) {
									canvas->draw(rcDst, image.source, param);
								} else {
									canvas->draw(rcDst, image.source, image.region, param);
								}
							} else {
								if (image.flagDrawWhole) {
									canvas->draw(rcDst, image.source);
								} else {
									canvas->draw(rcDst, image.source, image.region);
								}
							}
						}
					}
				}
			}

			sl_bool loadPicture(TileImage& _out, MapTileLoader* loader, const MapTileLocationI& location)
			{
				return loadImage(_out, m_readerPicture, m_cachePicture.get(), loader, location);
			}

			sl_bool loadImage(TileImage& _out, const Ref<MapTileReader>& reader, MapTileCache* cache, MapTileLoader* loader, const MapTileLocationI& location)
			{
				if (reader.isNull()) {
					return sl_false;
				}
				MapTileLoadParam param;
				param.reader = reader;
				(MapTileLocationI&)(param.address) = location;
				param.cache = cache;
				param.flagEndless = location.level == m_config.baseLevel;
				m_toReaderLocation(param.address);
				loader->loadImage(_out.source, param, sl_null);
				if (_out.source.isNotNull()) {
					_out.region.left = 0;
					_out.region.top = 0;
					_out.region.right = 1.0f;
					_out.region.bottom = 1.0f;
					_out.flagDrawWhole = sl_true;
					return sl_true;
				}
				if (location.level <= m_config.baseLevel) {
					return sl_false;
				}
				if (!(loadImage(_out, reader, cache, loader, MapTileLocation(location.level - 1, location.E >> 1, location.N >> 1)))) {
					return sl_false;
				}
				if (location.E & 1) {
					_out.region.left = _out.region.getCenterX();
				} else {
					_out.region.right = _out.region.getCenterX();
				}
				if (location.N & 1) {
					_out.region.bottom = _out.region.getCenterY();
				} else {
					_out.region.top = _out.region.getCenterY();
				}
				_out.flagDrawWhole = sl_false;
				return sl_true;
			}

			sl_bool loadDEM(TileDEM& _out, MapTileLoader* loader, const MapTileLocationI& location)
			{
				if (m_readerDEM.isNull()) {
					return sl_false;
				}
				MapTileLoadParam param;
				param.reader = m_readerDEM;
				(MapTileLocationI&)(param.address) = location;
				param.cache = m_cacheDEM;
				param.flagEndless = location.level == m_config.baseLevel;
				m_toReaderLocation(param.address);
				loader->loadData(_out.source, param, sl_null);
				if (_out.source.isNotNull()) {
					_out.region.left = 0;
					_out.region.top = 0;
					_out.region.right = 1.0f;
					_out.region.bottom = 1.0f;
					_out.flagUseWhole = sl_true;
					return sl_true;
				}
				if (location.level <= m_config.baseLevel) {
					return sl_false;
				}
				if (!(loadDEM(_out, loader, MapTileLocation(location.level - 1, location.E >> 1, location.N >> 1)))) {
					return sl_false;
				}
				if (location.E & 1) {
					_out.region.left = _out.region.getCenterX();
				} else {
					_out.region.right = _out.region.getCenterX();
				}
				if (location.N & 1) {
					_out.region.bottom = _out.region.getCenterY();
				} else {
					_out.region.top = _out.region.getCenterY();
				}
				_out.flagUseWhole = sl_false;
				return sl_true;
			}

			void clearCache() override
			{
				m_cachePicture->clear();
				for (sl_uint32 i = 0; i < LAYER_COUNT; i++) {
					m_cacheLayers[i]->clear();
				}
			}
		};
	}

	Ref<MapSurface> MapSurface::create(const MapSurfaceParam& param)
	{
		return Ref<MapSurface>::cast(MapSurfaceImpl::create(param));
	}

	const MapSurfaceConfiguration& MapSurface::getConfiguration()
	{
		return m_config;
	}

	Ref<MapTileReader> MapSurface::getPictureReader()
	{
		return m_readerPicture;
	}

	void MapSurface::setPictureReader(const Ref<MapTileReader>& reader)
	{
		m_readerPicture = reader;
		clearCache();
	}

	Ref<MapTileReader> MapSurface::getDemReader()
	{
		return m_readerDEM;
	}

	void MapSurface::setDemReader(const Ref<MapTileReader>& reader, DEM::DataType type, sl_bool flagFlipY)
	{
		m_readerDEM = reader;
		m_config.demType = type;
		m_config.flagFlipDemY = flagFlipY;
		clearCache();
	}

	Ref<MapTileReader> MapSurface::getLayerReader(sl_uint32 layer)
	{
		if (layer < LAYER_COUNT) {
			return m_layers[layer].reader;
		}
		return sl_null;
	}

	void MapSurface::setLayerReader(sl_uint32 layer, const Ref<MapTileReader>& reader)
	{
		if (layer < LAYER_COUNT) {
			m_layers[layer].reader = reader;
			clearCache();
		}
	}

	sl_bool MapSurface::isLayerVisible(sl_uint32 layer)
	{
		if (layer < LAYER_COUNT) {
			return m_layers[layer].flagVisible;
		}
		return sl_false;
	}

	void MapSurface::setLayerVisible(sl_uint32 layer, sl_bool flag)
	{
		if (layer < LAYER_COUNT) {
			m_layers[layer].flagVisible = flag;
		}
	}

	float MapSurface::getLayerOpacity(sl_uint32 layer)
	{
		if (layer < LAYER_COUNT) {
			return m_layers[layer].opacity;
		}
		return 0.0f;
	}

	void MapSurface::setLayerOpacity(sl_uint32 layer, float opacity)
	{
		if (layer < LAYER_COUNT) {
			m_layers[layer].opacity = opacity;
		}
	}

	LatLon MapSurface::getLatLonFromTileLocation(const MapTileLocationI& location)
	{
		if (location.level < m_config.baseLevel) {
			return { 0, 0 };
		}
		sl_uint64 n = (sl_uint64)1 << (location.level - m_config.baseLevel);
		sl_uint64 nE = n * m_config.baseTileCountE;
		sl_uint64 nN = n * m_config.baseTileCountN;
		LatLon ret;
		ret.latitude = ((double)(location.N) / (double)nN - 0.5) * m_config.northingRangeInDegrees;
		ret.longitude = ((double)(location.E) / (double)nE - 0.5) * m_config.eastingRangeInDegrees;
		return ret;
	}

	MapTileLocation MapSurface::getTileLocationFromLatLon(sl_uint32 level, const LatLon& location)
	{
		if (level < m_config.baseLevel) {
			return { level, 0, 0 };
		}
		sl_uint64 n = (sl_uint64)1 << (level - m_config.baseLevel);
		sl_uint64 nE = n * m_config.baseTileCountE;
		sl_uint64 nN = n * m_config.baseTileCountN;
		MapTileLocation ret;
		ret.level = level;
		ret.N = (0.5 + location.latitude / m_config.northingRangeInDegrees) * nN;
		ret.E = (0.5 + location.longitude / m_config.eastingRangeInDegrees) * nE;
		return ret;
	}

	MapTileLocationI MapSurface::getReaderLocation(const MapTileLocationI& location)
	{
		MapTileLocationI ret = location;
		m_toReaderLocation(ret);
		return ret;
	}


	SLIB_DEFINE_OBJECT(MapSurfacePlane, MapPlane)

	MapSurfacePlane::MapSurfacePlane()
	{
		m_range.right = EARTH_CIRCUMFERENCE / 2.0;
		m_range.left = -m_range.right;
		m_range.top = EARTH_CIRCUMFERENCE / 4.0;
		m_range.bottom = -m_range.top;
	}

	MapSurfacePlane::~MapSurfacePlane()
	{
	}

	Ref<MapSurfacePlane> MapSurfacePlane::create(const Ref<MapSurface>& surface)
	{
		Ref<MapSurfacePlane> ret = new MapSurfacePlane;
		if (ret.isNotNull()) {
			ret->m_surface = surface;
			return ret;
		}
		return sl_null;
	}

	LatLon MapSurfacePlane::getLatLonFromMapLocation(const MapLocation& location)
	{
		return { location.N / METER_PER_DEGREE, location.E / METER_PER_DEGREE };
	}

	MapLocation MapSurfacePlane::getMapLocationFromLatLon(const LatLon& location)
	{
		return { location.longitude * METER_PER_DEGREE, location.latitude * METER_PER_DEGREE };
	}

	void MapSurfacePlane::clearCache()
	{
		m_surface->clearCache();
	}

	void MapSurfacePlane::onDraw(Canvas* canvas, MapViewData* data)
	{
		m_surface->onDrawPlane(canvas, m_viewport, this, data);
	}


	SLIB_DEFINE_OBJECT(MapViewObject, Object)

	MapViewObject::MapViewObject()
	{
		m_flagSupportGlobe = sl_false;
		m_flagSupportPlane = sl_false;
		m_flagOverlay = sl_false;
	}

	MapViewObject::~MapViewObject()
	{
	}

	sl_bool MapViewObject::isSupportingGlobeMode()
	{
		return m_flagSupportGlobe;
	}

	void MapViewObject::setSupportingGlobeMode(sl_bool flag)
	{
		m_flagSupportGlobe = flag;
	}

	sl_bool MapViewObject::isSupportingPlaneMode()
	{
		return m_flagSupportPlane;
	}

	void MapViewObject::setSupportingPlaneMode(sl_bool flag)
	{
		m_flagSupportPlane = flag;
	}

	sl_bool MapViewObject::isOverlay()
	{
		return m_flagOverlay;
	}

	void MapViewObject::setOverlay(sl_bool flag)
	{
		m_flagOverlay = flag;
	}

	void MapViewObject::draw(Canvas* canvas, MapViewData* data, MapPlane* plane)
	{
	}

	void MapViewObject::render(RenderEngine* engine, MapViewData* data, MapSurface* surface)
	{
	}

	SLIB_DEFINE_OBJECT(MapViewObjectList, MapViewObject)

	MapViewObjectList::MapViewObjectList()
	{
		setSupportingGlobeMode();
		setSupportingPlaneMode();
	}

	MapViewObjectList::~MapViewObjectList()
	{
	}

	void MapViewObjectList::draw(Canvas* canvas, MapViewData* data, MapPlane* plane)
	{
		ObjectLocker lock(this);
		ListElements< Ref<MapViewObject> > children(m_children);
		for (sl_size i = 0; i < children.count; i++) {
			Ref<MapViewObject>& child = children[i];
			if (child->isSupportingPlaneMode()) {
				child->draw(canvas, data, plane);
			}
		}
	}

	void MapViewObjectList::render(RenderEngine* engine, MapViewData* data, MapSurface* surface)
	{
		MapViewState& state = data->getMapState();
		ObjectLocker lock(this);
		ListElements< Ref<MapViewObject> > children(m_children);
		for (sl_size i = 0; i < children.count; i++) {
			Ref<MapViewObject>& child = children[i];
			if (child->isSupportingPlaneMode()) {
				if (child->isOverlay()) {
					engine->setDepthStencilState(state.overlayDepthState);
					engine->setBlendState(state.overlayBlendState);
					engine->setRasterizerState(state.overlayRasterizerState);
				} else {
					engine->setDepthStencilState(state.defaultDepthState);
					engine->setBlendState(state.defaultBlendState);
					engine->setRasterizerState(state.defaultRasterizerState);
				}
				child->render(engine, data, surface);
			}
		}
	}

	void MapViewObjectList::addChild(const Ref<MapViewObject>& child)
	{
		ObjectLocker lock(this);
		m_children.add_NoLock(child);
	}

	void MapViewObjectList::removeAll()
	{
		ObjectLocker lock(this);
		m_children.removeAll_NoLock();
	}

	SLIB_DEFINE_OBJECT(MapViewSprite, MapViewObject)

	MapViewSprite::MapViewSprite()
	{
		setSupportingGlobeMode();
		setSupportingPlaneMode();

		m_size = Size::zero();
		m_textColor = Color::White;
		m_textShadowColor = Color::Black;

		m_flagValidAltitude = sl_false;
		m_altitude = 0.0;
		m_viewPoint = Point::zero();
		m_lastDrawId = 0;
	}

	MapViewSprite::MapViewSprite(const LatLon& location, const Ref<Image>& image, const String& text): MapViewSprite()
	{
		initialize(location, image, text);
	}

	MapViewSprite::MapViewSprite(const LatLon& location, const Ref<Image>& image, const String& text, const Ref<Font>& font): MapViewSprite()
	{
		initialize(location, image, text, font);
	}

	MapViewSprite::~MapViewSprite()
	{
	}

	void MapViewSprite::initialize(const LatLon& location, const Ref<Image>& image, const String& text)
	{
		initialize(location, image, text, Ref<Font>::null());
	}

	void MapViewSprite::initialize(const LatLon& location, const Ref<Image>& image, const String& text, const Ref<Font>& font)
	{
		m_location = location;
		m_image = image;
		m_text = text;
		m_font = font;
	}

	const LatLon& MapViewSprite::getLocation()
	{
		return m_location;
	}

	const Ref<Image>& MapViewSprite::getImage()
	{
		return m_image;
	}

	const String& MapViewSprite::getText()
	{
		return m_text;
	}

	const Size& MapViewSprite::getSize()
	{
		return m_size;
	}

	void MapViewSprite::setSize(const Size& size)
	{
		m_size = size;
	}

	const Color& MapViewSprite::getTextColor()
	{
		return m_textColor;
	}

	void MapViewSprite::setTextColor(const Color& color)
	{
		m_textColor = color;
	}

	const Color& MapViewSprite::getTextShadowColor()
	{
		return m_textShadowColor;
	}

	void MapViewSprite::setTextShadowColor(const Color& color)
	{
		m_textShadowColor = color;
	}

	void MapViewSprite::draw(Canvas* canvas, MapViewData* data, MapPlane* plane)
	{
		Point pt = plane->getViewPointFromMapLocation(plane->getMapLocationFromLatLon(m_location));
		sl_real w = m_size.x / 2.0f;
		sl_real h = m_size.y / 2.0f;
		Rectangle rc(pt.x - w, pt.y - h, pt.x + w, pt.y + h);
		if (!(plane->getViewport().intersect(rc))) {
			return;
		}
		m_viewPoint = pt;
		onPreDrawOrRender(data);
		onDrawSprite(canvas, data, plane);
	}

	void MapViewSprite::render(RenderEngine* engine, MapViewData* data, MapSurface* surface)
	{
		Double3 pointEarth = MapEarth::getCartesianPosition(getLocation(data));
		if (!(data->isEarthPointVisible(pointEarth))) {
			return;
		}
		m_viewPoint = data->getViewPointFromEarthPoint(pointEarth);
		onPreDrawOrRender(data);
		onRenderSprite(engine, data, surface);
	}

	sl_bool MapViewSprite::isBeingDrawn(MapViewData* data)
	{
		sl_uint64 lastDrawId = m_lastDrawId;
		if (lastDrawId) {
			if (lastDrawId + 1 >= data->getMapState().drawId) {
				return sl_true;
			}
		}
		return sl_false;
	}

	sl_bool MapViewSprite::getViewPoint(Point& _out, MapViewData* data)
	{
		if (isBeingDrawn(data)) {
			_out = m_viewPoint;
			return sl_true;
		}
		return sl_false;
	}

	double MapViewSprite::getAltitude(MapViewData* data)
	{
		if (m_flagValidAltitude) {
			return m_altitude;
		}
		double altitude = data->getAltitudeAt(m_location);
		m_altitude = altitude;
		return altitude;
	}

	GeoLocation MapViewSprite::getLocation(MapViewData* data)
	{
		return GeoLocation(m_location, getAltitude(data));
	}

	void MapViewSprite::onPreDrawOrRender(MapViewData* data)
	{
		m_lastDrawId = data->getMapState().drawId;
	}

	const Point& MapViewSprite::getViewPoint()
	{
		return m_viewPoint;
	}

	namespace
	{
		static Ref<Font> GetFinalSpriteFont(MapViewData* data, const Ref<Font>& font)
		{
			if (font.isNotNull()) {
				return font;
			}
			Ref<Font> _font = data->getSpriteFont();
			if (_font.isNotNull()) {
				return _font;
			}
			return UI::getDefaultFont();
		}
	}

	void MapViewSprite::onDrawSprite(Canvas* canvas, MapViewData* data, MapPlane* plane)
	{
		sl_real w = m_size.x / 2.0f;
		sl_real h = m_size.y / 2.0f;
		Rectangle rect(m_viewPoint.x - w, m_viewPoint.y - h, m_viewPoint.x + w, m_viewPoint.y + h);
		if (m_image.isNotNull()) {
			canvas->draw(rect, m_image);
		}
		if (m_text.isNotNull()) {
			rect.top = rect.bottom;
			rect.bottom = rect.top + 100.0f;
			Ref<Font> font = GetFinalSpriteFont(data, m_font);
			if (m_textShadowColor.isNotZero()) {
				rect.translate(1.0f, 1.0f);
				canvas->drawText(m_text, rect, font, m_textShadowColor, Alignment::TopCenter);
				rect.translate(-1.0f, -1.0f);
			}
			canvas->drawText(m_text, rect, font, m_textColor, Alignment::TopCenter);
		}
	}

	void MapViewSprite::onRenderSprite(RenderEngine* engine, MapViewData* data, MapSurface* surface)
	{
		data->renderImage(engine, m_viewPoint, m_size, m_image);
		if (m_text.isNotNull()) {
			Ref<Font> font = GetFinalSpriteFont(data, m_font);
			Size offset(0.0f, m_size.y / 2.0f);
			if (m_textShadowColor.isNotZero()) {
				offset.x += 1.0f;
				offset.y += 1.0f;
				data->renderText(engine, m_viewPoint + offset, m_text, m_textShadowColor, font, this);
				offset.x -= 1.0f;
				offset.y -= 1.0f;
			}
			data->renderText(engine, m_viewPoint + offset, m_text, m_textColor, font, this);
		}
	}


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(MapViewState)

	MapViewState::MapViewState()
	{
		viewportWidth = 1.0;
		viewportHeight = 1.0;

		eyeLocation.altitude = 10000;
		tilt = 0.0f;
		rotation = 0.0f;
		flagTileGrid = sl_false;
		flagTerrainGrid = sl_false;

		drawId = 0;
	}
	
	sl_bool MapViewState::update()
	{
		if (viewportHeight < 0.00001) {
			return sl_false;
		}

		if (defaultDepthState.isNull()) {
			RenderDepthStencilParam dp;
			dp.flagTestDepth = sl_true;
			defaultDepthState = RenderDepthStencilState::create(dp);
			if (defaultDepthState.isNull()) {
				return sl_false;
			}
		}
		if (defaultBlendState.isNull()) {
			RenderBlendParam bp;
			bp.flagBlending = sl_false;
			defaultBlendState = RenderBlendState::create(bp);
			if (defaultBlendState.isNull()) {
				return sl_false;
			}
		}
		if (defaultRasterizerState.isNull()) {
			RenderRasterizerParam rp;
			defaultRasterizerState = RenderRasterizerState::create(rp);
			if (defaultRasterizerState.isNull()) {
				return sl_false;
			}
		}
		if (overlayDepthState.isNull()) {
			RenderDepthStencilParam dp;
			dp.flagTestDepth = sl_false;
			dp.flagWriteDepth = sl_false;
			overlayDepthState = RenderDepthStencilState::create(dp);
			if (overlayDepthState.isNull()) {
				return sl_false;
			}
		}
		if (overlayBlendState.isNull()) {
			RenderBlendParam bp;
			bp.flagBlending = sl_true;
			overlayBlendState = RenderBlendState::create(bp);
			if (overlayBlendState.isNull()) {
				return sl_false;
			}
		}
		if (overlayRasterizerState.isNull()) {
			RenderRasterizerParam rp;
			rp.flagCull = sl_false;
			overlayRasterizerState = RenderRasterizerState::create(rp);
			if (overlayRasterizerState.isNull()) {
				return sl_false;
			}
		}

		eyePoint = MapEarth::getCartesianPosition(eyeLocation);

		verticalViewTransform = Transform3T<double>::getLookAtMatrix(eyePoint, Double3(0, 0, 0), Double3(0, 10000, 0)) * Transform3T<double>::getRotationZMatrix(Math::getRadianFromDegrees(rotation));
		viewTransform = verticalViewTransform * Transform3T<double>::getRotationXMatrix(Math::getRadianFromDegrees(tilt));
		inverseViewTransform = viewTransform.inverse();

		double dist = eyeLocation.altitude + 0.1;
		double zNear, zFar;
		if (dist < 5000) {
			zNear = dist / 50;
			zFar = dist * 20 + 1000;
		} else {
			zNear = dist / 5;
			zFar = dist + MapEarth::getRadius() * 4;
		}
		projectionTransform = Transform3T<double>::getPerspectiveProjectionFovYMatrix(MAP_FOV_Y, viewportWidth / viewportHeight, zNear, zFar);
		viewProjectionTransform = viewTransform * projectionTransform;
		viewFrustum = ViewFrustumT<double>::fromMVP(viewProjectionTransform);
		return sl_true;
	}


	SLIB_DEFINE_OBJECT(MapViewExtension, Object)

	MapViewExtension::MapViewExtension()
	{
	}

	MapViewExtension::~MapViewExtension()
	{
	}


	MapViewData::MapViewData()
	{
		m_flagGlobeMode = sl_false;
		m_minAltitude = 50.0;
		m_maxAltitude = 100000000.0;
		m_minDistanceFromGround = 100.0;
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
				setEyeLocation(m_plane->getEyeLocation(), mode);
			} else {
				invalidate(mode);
			}
		} else {
			if (m_plane.isNotNull()) {
				m_plane->setEyeLocation(m_state.eyeLocation);
				invalidate(mode);
			}
		}
	}

	GeoLocation MapViewData::getEyeLocation() const
	{
		MutexLocker locker(&m_lock);
		if (!m_flagGlobeMode) {
			if (m_plane.isNotNull()) {
				return m_plane->getEyeLocation();
			}
		}
		return m_state.eyeLocation;
	}

	void MapViewData::setEyeLocation(const GeoLocation& location, UIEvent* ev, UIUpdateMode mode)
	{
		if (SLIB_UI_UPDATE_MODE_IS_ANIMATE(mode)) {
			m_motion.prepare(this);
			m_motion.endLocation = location;
			m_motion.ev = ev;
			m_motion.start();
			return;
		}
		GeoLocation newLocation;
		{
			MutexLocker locker(&m_lock);
			newLocation = location;
			if (m_flagGlobeMode) {
				if (newLocation.altitude < m_minAltitude) {
					newLocation.altitude = m_minAltitude;
				}
				if (newLocation.altitude > m_maxAltitude) {
					newLocation.altitude = m_maxAltitude;
				}
			} else {
				if (m_plane.isNotNull()) {
					m_plane->setEyeLocation(location);
					newLocation = m_plane->getEyeLocation();
				}
			}
		}
		m_state.eyeLocation = newLocation;
		invokeChangeLocation(newLocation, ev);
		if (m_flagGlobeMode) {
			setEyeTilt(m_state.tilt, UIUpdateMode::None);
		}
		invalidate(mode);
	}

	void MapViewData::setEyeLocation(const GeoLocation& location, UIUpdateMode mode)
	{
		setEyeLocation(location, sl_null, mode);
	}

	float MapViewData::getEyeRotation() const
	{
		return m_state.rotation;
	}

	void MapViewData::setEyeRotation(float rotation, UIEvent* ev, UIUpdateMode mode)
	{
		if (!m_flagGlobeMode) {
			return;
		}
		if (SLIB_UI_UPDATE_MODE_IS_ANIMATE(mode)) {
			m_motion.prepare(this);
			m_motion.endRotation = rotation;
			m_motion.start();
			return;
		}
		m_state.rotation = Math::normalizeDegree(rotation);
		invokeChangeRotation(rotation, ev);
		invalidate(mode);
	}

	void MapViewData::setEyeRotation(float rotation, UIUpdateMode mode)
	{
		setEyeRotation(rotation, sl_null, mode);
	}

	float MapViewData::getEyeTilt() const
	{
		return m_state.tilt;
	}

	void MapViewData::setEyeTilt(float tilt, UIEvent* ev, UIUpdateMode mode)
	{
		if (!m_flagGlobeMode) {
			return;
		}
		if (SLIB_UI_UPDATE_MODE_IS_ANIMATE(mode)) {
			m_motion.prepare(this);
			m_motion.endTilt = tilt;
			m_motion.start();
			return;
		}
		if (tilt < 0.0f) {
			tilt = 0.0f;
		}
		float max = 40.0f;
		float alt = (float)(m_state.eyeLocation.altitude);
		if (alt > 1000.0f) {
			max = max - (alt - 1000.0f) / 500.0f;
			if (max < 0.0f) {
				max = 0.0f;
			}
		}
		if (tilt > max) {
			tilt = max;
		}
		m_state.tilt = tilt;
		invokeChangeTilt(tilt, ev);
		invalidate(mode);
	}

	void MapViewData::setEyeTilt(float tilt, UIUpdateMode mode)
	{
		setEyeTilt(tilt, sl_null, mode);
	}

	double MapViewData::getMapScale() const
	{
		MutexLocker lock(&m_lock);
		const Ref<MapPlane>& plane = m_plane;
		if (plane.isNotNull()) {
			return plane->getScale();
		} else {
			return getScaleFromAltitude(m_state.eyeLocation.altitude, m_state.viewportHeight);
		}
	}

	void MapViewData::setMapScale(double scale, UIUpdateMode mode)
	{
		MutexLocker lock(&m_lock);
		if (m_flagGlobeMode) {
			GeoLocation location = m_state.eyeLocation;
			location.altitude = getAltitudeFromScale(scale, m_state.viewportHeight);
			setEyeLocation(location);
		} else {
			const Ref<MapPlane>& plane = m_plane;
			if (plane.isNotNull()) {
				plane->setScale(scale);
				invalidate(mode);
			}
		}
	}

	double MapViewData::getMinimumAltitude() const
	{
		return m_minAltitude;
	}

	void MapViewData::setMinimumAltitude(double altitude, UIUpdateMode mode)
	{
		m_minAltitude = altitude;
		if (SLIB_UI_UPDATE_MODE_IS_INIT(mode)) {
			return;
		}
		setEyeLocation(getEyeLocation(), mode);
	}

	double MapViewData::getMaximumAltitude() const
	{
		return m_maxAltitude;
	}

	void MapViewData::setMaximumAltitude(double altitude, UIUpdateMode mode)
	{
		m_maxAltitude = altitude;
		if (SLIB_UI_UPDATE_MODE_IS_INIT(mode)) {
			return;
		}
		setEyeLocation(getEyeLocation(), mode);
	}

	double MapViewData::getMinimumDistanceFromGround() const
	{
		return m_minDistanceFromGround;
	}

	void MapViewData::setMinimumDistanceFromGround(double value)
	{
		m_minDistanceFromGround = value;
	}

	Ref<Font> MapViewData::getSpriteFont() const
	{
		return m_spriteFont;
	}

	void MapViewData::setSpriteFont(const Ref<Font>& font)
	{
		m_spriteFont = font;
	}

	sl_bool MapViewData::isTileGridVisible() const
	{
		return m_state.flagTileGrid;
	}

	void MapViewData::setTileGridVisible(sl_bool flag, UIUpdateMode mode)
	{
		m_state.flagTileGrid = flag;
		invalidate(mode);
	}

	sl_bool MapViewData::isTerrainGridVisible() const
	{
		return m_state.flagTerrainGrid;
	}

	void MapViewData::setTerrainGridVisible(sl_bool flag, UIUpdateMode mode)
	{
		m_state.flagTileGrid = flag;
		invalidate(mode);
	}

	Ref<View> MapViewData::getView() const
	{
		return m_view;
	}

	Ref<MapPlane> MapViewData::getPlane() const
	{
		MutexLocker locker(&m_lock);
		return m_plane;
	}

	void MapViewData::setPlane(const Ref<MapPlane>& plane, UIUpdateMode mode)
	{
		MutexLocker locker(&m_lock);
		if (m_plane == plane) {
			return;
		}
		GeoLocation location = m_state.eyeLocation;
		if (m_plane.isNotNull()) {
			if (!m_flagGlobeMode) {
				location = m_plane->getEyeLocation();
			}
			m_plane->clearCache();
		}
		m_plane = plane;
		if (plane.isNotNull()) {
			_resizePlane(plane.get(), m_state.viewportWidth, m_state.viewportHeight);
			plane->setEyeLocation(location);
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
		if (m_surface == surface) {
			return;
		}
		if (m_surface.isNotNull()) {
			m_surface->clearCache();
		}
		m_surface = surface;
		invalidate(mode);
	}

	List< Ref<MapViewObject> > MapViewData::getObjects() const
	{
		MutexLocker locker(&m_lock);
		return m_objects.getAllValues_NoLock();
	}

	Ref<MapViewObject> MapViewData::getObject(const String& key) const
	{
		MutexLocker locker(&m_lock);
		return m_objects.getValue_NoLock(key);
	}

	void MapViewData::putObject(const String& name, const Ref<MapViewObject>& object, UIUpdateMode mode)
	{
		MutexLocker locker(&m_lock);
		if (object.isNotNull()) {
			m_objects.put_NoLock(name, object);
		} else {
			m_objects.remove_NoLock(name);
		}
		invalidate(mode);
	}

	const MapViewState& MapViewData::getMapState() const
	{
		return m_state;
	}

	MapViewState& MapViewData::getMapState()
	{
		return m_state;
	}

	void MapViewData::resize(double width, double height, UIUpdateMode mode)
	{
		if (height < 0.00001) {
			return;
		}
		if (Math::isAlmostZero(m_state.viewportWidth - width) && Math::isAlmostZero(m_state.viewportHeight - height)) {
			return;
		}
		m_state.viewportWidth = width;
		m_state.viewportHeight = height;
		MutexLocker locker(&m_lock);
		_resizePlane(m_plane.get(), width, height);
		invalidate(mode);
	}

	void MapViewData::_resizePlane(MapPlane* plane, double width, double height)
	{
		if (!plane) {
			return;
		}
		RectangleT<double> rect;
		rect.left = 0.0;
		rect.top = 0.0;
		rect.right = width;
		rect.bottom = height;
		plane->setViewport(rect);
	}

	void MapViewData::movePlane(double dx, double dy, UIEvent* ev, UIUpdateMode mode)
	{
		MutexLocker locker(&m_lock);
		if (m_flagGlobeMode) {
			return;
		}
		MapPlane* plane = m_plane.get();
		if (!plane) {
			return;
		}
		dx = getMetersFromPixels(dx);
		dy = getMetersFromPixels(dy);
		double scale = plane->getScale();
		MapLocation center = plane->getCenterLocation();
		plane->setCenterLocation(center.E - dx * scale, center.N + dy * scale);
		GeoLocation location = plane->getEyeLocation();
		locker.unlock();
		m_state.eyeLocation = location;
		invokeChangeLocation(location, ev);
		invalidate(mode);
	}

	void MapViewData::movePlane(double dx, double dy, UIUpdateMode mode)
	{
		movePlane(dx, dy, sl_null, mode);
	}

	void MapViewData::travelTo(const GeoLocation& location)
	{
		if (m_flagGlobeMode) {
			m_motion.prepare(this);
			m_motion.endLocation = location;
			m_motion.flagTravel = sl_true;
			m_motion.start();
		} else {
			setEyeLocation(location);
		}
	}

	void MapViewData::zoom(double factor, UIEvent* ev, UIUpdateMode mode)
	{
		MutexLocker locker(&m_lock);
		if (m_flagGlobeMode) {
			GeoLocation location = m_state.eyeLocation;
			location.altitude *= factor;
			locker.unlock();
			setEyeLocation(location, ev, mode);
			return;
		}
		MapPlane* plane = m_plane.get();
		if (!plane) {
			return;
		}
		plane->setScale(plane->getScale() * factor);
		GeoLocation location = plane->getEyeLocation();
		locker.unlock();
		m_state.eyeLocation = location;
		invokeChangeLocation(location, ev);
		invalidate(mode);
	}

	void MapViewData::zoom(double factor, UIUpdateMode mode)
	{
		zoom(factor, sl_null, mode);
	}

	void MapViewData::zoomAt(const Double2& pt, double factor, UIEvent* ev, UIUpdateMode mode)
	{
		if (m_flagGlobeMode) {
			zoom(factor, mode);
			return;
		}
		MutexLocker locker(&m_lock);
		MapPlane* plane = m_plane.get();
		if (!plane) {
			return;
		}
		MapLocation c = plane->getCenterLocation();
		MapLocation l1 = plane->getMapLocationFromViewPoint(pt);
		double scale = plane->getScale() * factor;
		plane->setScale(scale);
		MapLocation l2 = plane->getMapLocationFromViewPoint(pt);
		plane->setCenterLocation(c.E - l2.E + l1.E, c.N - l2.N + l1.N);
		GeoLocation location = plane->getEyeLocation();
		locker.unlock();
		m_state.eyeLocation = location;
		invokeChangeLocation(location, ev);
		invalidate(mode);
	}

	void MapViewData::zoomAt(const Double2& pt, double factor, UIUpdateMode mode)
	{
		zoomAt(pt, factor, sl_null, mode);
	}

	void MapViewData::click(const Double2& pt, UIUpdateMode mode)
	{
		invalidate(mode);
	}

	void MapViewData::stopMoving()
	{
		m_motion.stop();
	}

	void MapViewData::putExtension(const String& name, const Ref<MapViewExtension>& extension)
	{
		if (extension.isNotNull()) {
			m_extensions.put_NoLock(name, extension);
		}
	}

	Ref<MapViewExtension> MapViewData::getExtension(const String& name)
	{
		return m_extensions.getValue_NoLock(name);
	}

	void MapViewData::doInvalidate(UIUpdateMode mode)
	{
	}

	void MapViewData::notifyChangeLocation(const GeoLocation& location, UIEvent* ev)
	{
	}

	void MapViewData::invokeChangeLocation(const GeoLocation& location, UIEvent* ev)
	{
		auto node = m_extensions.getFirstNode();
		while (node) {
			node->value->onChangeLocation(location);
			node = node->next;
		}
		notifyChangeLocation(location, ev);
	}

	void MapViewData::notifyChangeRotation(double rotation, UIEvent* ev)
	{
	}

	void MapViewData::invokeChangeRotation(double rotation, UIEvent* ev)
	{
		auto node = m_extensions.getFirstNode();
		while (node) {
			node->value->onChangeRotation(rotation);
			node = node->next;
		}
		notifyChangeRotation(rotation, ev);
	}

	void MapViewData::notifyChangeTilt(double tilt, UIEvent* ev)
	{
	}

	void MapViewData::invokeChangeTilt(double tilt, UIEvent* ev)
	{
		auto node = m_extensions.getFirstNode();
		while (node) {
			node->value->onChangeTilt(tilt);
			node = node->next;
		}
		notifyChangeTilt(tilt, ev);
	}

	sl_bool MapViewData::_initState()
	{
		m_state.drawId++;
		if (m_state.tileLoader.isNull()) {
			m_state.tileLoader = MapTileLoader::create(SLIB_FUNCTION_MEMBER(this, _onCompleteLazyLoading));
			if (m_state.tileLoader.isNull()) {
				return sl_false;
			}
		}
		return sl_true;
	}

	void MapViewData::invalidate(UIUpdateMode mode)
	{
		doInvalidate(mode);
	}

	void MapViewData::drawPlane(Canvas* canvas)
	{
		MutexLocker locker(&m_lock);
		if (m_flagGlobeMode) {
			return;
		}
		if (m_state.viewportWidth < 1.0 || m_state.viewportHeight < 1.0) {
			return;
		}
		if (!(_initState())) {
			return;
		}
		MapPlane* plane = m_plane.get();
		if (!plane) {
			return;
		}
		plane->draw(canvas, this);
		{
			auto node = m_objects.getFirstNode();
			while (node) {
				Ref<MapViewObject>& object = node->value;
				if (object->isSupportingPlaneMode()) {
					object->draw(canvas, this, plane);
				}
				node = node->next;
			}
		}
	}

	void MapViewData::renderGlobe(RenderEngine* engine)
	{
		MutexLocker locker(&m_lock);

		if (!m_flagGlobeMode) {
			return;
		}
		MapSurface* surface = m_surface.get();
		if (!surface) {
			return;
		}
		if (!(_initState())) {
			return;
		}
		if (m_minDistanceFromGround >= 0.0) {
			double dist = surface->getAltitudeAt(m_state.tileLoader.get(), m_state.eyeLocation.getLatLon());
			dist += m_minDistanceFromGround;
			if (m_state.eyeLocation.altitude < dist) {
				m_state.eyeLocation.altitude = dist;
			}
		}
		if (!(m_state.update())) {
			return;
		}

		engine->setDepthStencilState(m_state.defaultDepthState);
		engine->setBlendState(m_state.defaultBlendState);
		engine->setRasterizerState(m_state.defaultRasterizerState);
		surface->render(engine, this);

		{
			auto node = m_objects.getFirstNode();
			while (node) {
				Ref<MapViewObject>& object = node->value;
				if (object->isSupportingGlobeMode()) {
					if (object->isOverlay()) {
						engine->setDepthStencilState(m_state.overlayDepthState);
						engine->setBlendState(m_state.overlayBlendState);
						engine->setRasterizerState(m_state.overlayRasterizerState);
					} else {
						engine->setDepthStencilState(m_state.defaultDepthState);
						engine->setBlendState(m_state.defaultBlendState);
						engine->setRasterizerState(m_state.defaultRasterizerState);
					}
					object->render(engine, this, surface);
				}
				node = node->next;
			}
		}
	}

	void MapViewData::renderTexture(RenderEngine* engine, const Point& center, const Size& size, const Ref<Texture>& texture, const Color4F& color)
	{
		if (texture.isNull()) {
			return;
		}
		Matrix3 transform = Transform2::getTranslationMatrix(-0.5f, -0.5f) * Transform2::getScalingMatrix(size) * Transform2::getTranslationMatrix(center) * Transform2::getScalingMatrix(2.0f / (sl_real)(m_state.viewportWidth), -2.0f / (sl_real)(m_state.viewportHeight)) * Transform2::getTranslationMatrix(-1.0f, 1.0f);
		engine->drawTexture2D(transform, texture, color);
	}

	void MapViewData::renderImage(RenderEngine* engine, const Point& center, const Size& size, const Ref<Image>& image, const Color4F& color)
	{
		if (image.isNull()) {
			return;
		}
		renderTexture(engine, center, size, Texture::getBitmapRenderingCache(image), color);
	}

	void MapViewData::renderText(RenderEngine* engine, const Point& center, const String& text, const Color& color, const Ref<Font>& font, CRef* ref)
	{
		SharedContext* context = GetSharedContext();
		if (!context) {
			return;
		}
		Ref<Texture> texture;
		if (!(context->renderTextCache.get(ref, &texture))) {
			String _text = text;
			if (_text.getLength() > 50) {
				_text = _text.substring(0, 50);
			}
			SizeI size = font->measureText(_text);
			if (size.x > m_state.viewportWidth) {
				size.x = (sl_int32)(m_state.viewportWidth);
			}
			if (size.y > m_state.viewportHeight) {
				size.x = (sl_int32)(m_state.viewportHeight);
			}
			Ref<Bitmap> bitmap = Bitmap::create(size.x, size.y);
			if (bitmap.isNull()) {
				return;
			}
			Ref<Canvas> canvas = bitmap->getCanvas();
			if (canvas.isNull()) {
				return;
			}
			canvas->drawText(text, 0, 0, font, Color::White);
			canvas.setNull();
			texture = Texture::create(bitmap);
			if (texture.isNull()) {
				return;
			}
			context->renderTextCache.put(ref, texture);
		}
		renderTexture(engine, Point(center.x, center.y + (sl_real)(texture->getHeight() / 2)), SizeI(texture->getWidth(), texture->getHeight()), texture, color);
	}

	sl_bool MapViewData::getLatLonFromViewPoint(const Double2& point, LatLon& _out) const
	{
		GeoLocation location;
		if (getLocationFromViewPoint(point, location)) {
			_out = location.getLatLon();
			return sl_true;
		}
		return sl_false;
	}

	Double2 MapViewData::getViewPointFromLatLon(const LatLon& latLon) const
	{	
		if (m_flagGlobeMode) {
			return getViewPointFromLocation(getLocationFromLatLon(latLon));
		} else {
			return getViewPointFromLocation(GeoLocation(latLon, 0.0));
		}
	}

	sl_bool MapViewData::getLocationFromViewPoint(const Double2& viewPoint, GeoLocation& _out) const
	{
		MutexLocker locker(&m_lock);
		if (m_flagGlobeMode) {
			Double3 earthPoint;
			if (getEarthPointFromViewPoint(viewPoint, earthPoint)) {
				_out = MapEarth::getGeoLocation(earthPoint);
				return sl_true;
			}
		} else {
			MapPlane* plane = m_plane.get();
			if (plane) {
				_out.setLatLon(plane->getLatLonFromMapLocation(plane->getMapLocationFromViewPoint(viewPoint)));
				_out.altitude = 0.0;
				return sl_true;
			}
		}
		return sl_false;
	}

	Double2 MapViewData::getViewPointFromLocation(const GeoLocation& location) const
	{
		if (m_flagGlobeMode) {
			return getViewPointFromEarthPoint(MapEarth::getCartesianPosition(location));
		} else {
			MutexLocker locker(&m_lock);
			MapPlane* plane = m_plane.get();
			if (plane) {
				return plane->getViewPointFromMapLocation(plane->getMapLocationFromLatLon(location.getLatLon()));
			}
			return { 0.0, 0.0 };
		}
	}

	sl_bool MapViewData::getEarthPointFromViewPoint(const Double2& point, Double3& _out) const
	{
		if (!m_flagGlobeMode) {
			return sl_false;
		}
		Line3T<double> line = Transform3T<double>::unprojectScreenPoint(m_state.projectionTransform, point, m_state.viewportWidth, m_state.viewportHeight);
		SphereT<double> globe(Double3::zero(), MapEarth::getRadius() + getAltitudeAt(m_state.eyeLocation.getLatLon()));
		Double3 pt1, pt2;
		line.transform(m_state.inverseViewTransform);
		if (globe.intersectLine(line, &pt1, &pt2) > 0) {
			_out = pt1;
			return sl_true;
		}
		return sl_false;
	}

	Double2 MapViewData::getViewPointFromEarthPoint(const Double3& point) const
	{
		if (!m_flagGlobeMode) {
			return Double2(0.0, 0.0);
		}
		Vector3 pt = Transform3::projectToViewport(m_state.viewProjectionTransform, point);
		sl_real x = (pt.x + 1.0f) * (sl_real)(m_state.viewportWidth) / 2.0f;
		sl_real y = (1.0f - pt.y) * (sl_real)(m_state.viewportHeight) / 2.0f;
		return Vector2(x, y);
	}

	double MapViewData::getAltitudeAt(const LatLon& location) const
	{
		if (m_flagGlobeMode) {
			MapSurface* surface = m_surface.get();
			if (surface) {
				return surface->getAltitudeAt(m_state.tileLoader.get(), location);
			}
		}
		return 0.0;
	}

	GeoLocation MapViewData::getLocationFromLatLon(const LatLon& location) const
	{
		return GeoLocation(location, getAltitudeAt(location));
	}

	sl_bool MapViewData::isLocationVisible(const GeoLocation& location) const
	{
		return isEarthPointVisible(MapEarth::getCartesianPosition(location));
	}

	sl_bool MapViewData::isEarthPointVisible(const Double3& point) const
	{
		// Check Distance
		double e2 = m_state.eyePoint.getLength2p();
		double r2 = MapEarth::getRadius();
		r2 *= r2;
		double p2 = (m_state.eyePoint - point).getLength2p();
		if (p2 > e2 - r2) {
			return sl_false;
		}
		// Check Frustum
		return m_state.viewFrustum.containsPoint(point);
	}

	double MapViewData::getDegreeFromEarthLength(double length)
	{
		return length / METER_PER_DEGREE;
	}

	double MapViewData::getEarthLengthFromDegree(double degrees)
	{
		return METER_PER_DEGREE * degrees;
	}

	double MapViewData::getAltitudeFromViewportHeight(double height)
	{
		return ALTITUDE_RATIO * height;
	}

	double MapViewData::getViewportHeightFromAltitude(double altitude)
	{
		return altitude / ALTITUDE_RATIO;
	}

	double MapViewData::getMetersFromPixels(double pixels)
	{
		return UIResource::pixelToMeter(pixels);
	}

	double MapViewData::getPixelsFromMeters(double meters)
	{
		return UIResource::meterToPixel(meters);
	}

	double MapViewData::getScaleFromAltitude(double altitude, double viewportHeight)
	{
		return getViewportHeightFromAltitude(altitude) / MapView::getMetersFromPixels(viewportHeight);
	}

	double MapViewData::getAltitudeFromScale(double scale, double viewportHeight)
	{
		return MapView::getAltitudeFromViewportHeight(MapView::getMetersFromPixels(viewportHeight) * scale);
	}

	void MapViewData::_onCompleteLazyLoading()
	{
		invalidate();
	}


	MapViewData::Motion::Motion()
	{
		parent = sl_null;
		flagRunning = sl_false;
		startTick = 0;
		lastTick = 0;
		flagTravel = sl_false;
		rotation = 0.0f;
		startRotation = 0.0f;
		endRotation = 0.0f;
		tilt = 0.0f;
		startTilt = 0.0f;
		endTilt = 0.0f;
	}

	MapViewData::Motion::~Motion()
	{
	}

	void MapViewData::Motion::prepare(MapViewData* data)
	{
		stop();
		view = data->m_view;
		parent = data;
		MapViewState& state = data->m_state;
		location = startLocation = endLocation = state.eyeLocation;
		flagTravel = sl_false;
		rotation = startRotation = endRotation = state.rotation;
		tilt = startTilt = endTilt = state.tilt;
	}

	void MapViewData::Motion::start()
	{
		startTick = lastTick = System::getHighResolutionTickCount();
		flagRunning = sl_true;
		WeakRef<View> _view = view;
		timer = Timer::start([_view, this](Timer*) {
			Ref<View> view = _view;
			if (view.isNull()) {
				return;
			}
			step();
		}, 20);
	}

	void MapViewData::Motion::stop()
	{
		step();
		flagRunning = sl_false;
		timer.setNull();
		ev.setNull();
	}

	void MapViewData::Motion::step()
	{
		if (!flagRunning) {
			return;
		}
		sl_uint64 tick = System::getHighResolutionTickCount();
		sl_uint64 idt = tick - lastTick;
		if (!idt) {
			return;
		}
		if (idt > 1000) {
			idt = 1000;
		}
		float dt = (float)idt / 1000.0f;
		sl_bool flagAnimating = sl_false;
		{
			float w = dt * 180.0f;
			float t = endTilt - tilt;
			if (Math::abs(t) <= w) {
				tilt = endTilt;
			} else {
				if (tilt > endTilt) {
					tilt -= w;
				} else {
					tilt += w;
				}
				flagAnimating = sl_true;
			}
		}
		{
			float w = dt * 360.0f;
			float t = Math::normalizeDegreeDistance(endRotation - rotation);
			if (Math::abs(t) <= w) {
				rotation = endRotation;
			} else {
				if (t < 0) {
					rotation -= w;
				} else {
					rotation += w;
				}
				flagAnimating = sl_true;
			}
		}
		if (!(Math::isAlmostZero(location.latitude - endLocation.latitude) && Math::isAlmostZero(location.longitude - endLocation.longitude) && Math::isAlmostZero(location.altitude - endLocation.altitude))) {
			if (tick >= startTick + 1000) {
				location = endLocation;
			} else {
				float f = (float)(tick - startTick) / 1000.0f;
				location.longitude = Interpolation<double>::interpolate(startLocation.longitude, endLocation.longitude, f);
				location.latitude = Interpolation<double>::interpolate(startLocation.latitude, endLocation.latitude, f);
				if (flagTravel) {
					double topAlt = Math::min(startLocation.altitude, endLocation.altitude) + (MapEarth::getCartesianPosition(startLocation) - MapEarth::getCartesianPosition(endLocation)).getLength() / 3.0;
					if (f < 0.5f) {
						location.altitude = Interpolation<double>::interpolate(startLocation.altitude, topAlt, f * 2.0f);
					} else {
						location.altitude = Interpolation<double>::interpolate(topAlt, endLocation.altitude, (f - 0.5f) * 2.0f);
					}
				} else {
					location.altitude = Interpolation<double>::interpolate(startLocation.altitude, endLocation.altitude, f);
				}
				flagAnimating = sl_true;
			}
		}
		lastTick = tick;
		if (!flagAnimating) {
			stop();
		}
		Ref<UIEvent> _ev = ev;
		parent->setEyeLocation(location, _ev.get());
		parent->setEyeRotation(rotation);
		parent->setEyeTilt(tilt);
	}


	SLIB_DEFINE_OBJECT(MapView, RenderView)

	MapView::MapView()
	{
		setRedrawMode(RedrawMode::WhenDirty);
		setFocusable();

		m_compassSize = 150;
		m_compassCenter.x = 0.5f;
		m_compassCenter.y = 0.5f;
		m_compassAlign = Alignment::MiddleCenter;

		m_nLastTouches = 0;
		m_ptLastEvent.x = 0.0f;
		m_ptLastEvent.y = 0.0f;

		m_flagLeftDown = sl_false;
		m_tickLeftDown = 0;
		m_ptLeftDown.x = 0.0f;
		m_ptLeftDown.y = 0.0f;
		m_rotationLeftDown = 0.0f;

		m_rotationTouchStart = 0.0f;
		m_altitudeTouchStart = 1.0;
		m_flagTouchRotateStarted = sl_false;;

		m_flagClicking = sl_false;
		m_compassState = ViewState::Normal;
	}

	MapView::~MapView()
	{
	}

	void MapView::init()
	{
		RenderView::init();

		m_view = this;
	}

	Ref<Image> MapView::getCompass(ViewState state)
	{
		return m_compass.get(state);
	}

	void MapView::setCompass(const Ref<Drawable>& drawable, ViewState state, UIUpdateMode mode)
	{
		if (drawable.isNotNull()) {
			Ref<Image> image = drawable->toImage();
			if (image.isNotNull()) {
				m_compass.set(state, image);
			} else {
				m_compass.remove(state);
			}
		} else {
			m_compass.remove(state);
		}
		invalidate(mode);
	}

	void MapView::setCompass(const Ref<Drawable>& drawable, UIUpdateMode mode)
	{
		setCompass(drawable, ViewState::All, mode);
	}

	sl_ui_len MapView::getCompassSize()
	{
		return m_compassSize;
	}

	void MapView::setCompassSize(sl_ui_len size, UIUpdateMode mode)
	{
		m_compassSize = size;
		invalidate(mode);
	}

	const Point& MapView::getCompassCenter()
	{
		return m_compassCenter;
	}

	void MapView::setCompassCenter(const Point& pt, UIUpdateMode mode)
	{
		m_compassCenter = pt;
		invalidate(mode);
	}

	void MapView::setCompassCenter(sl_real cx, sl_real cy, UIUpdateMode mode)
	{
		m_compassCenter.x = cx;
		m_compassCenter.y = cy;
		invalidate(mode);
	}

	const Alignment& MapView::getCompassAlignment()
	{
		return m_compassAlign;
	}

	void MapView::setCompassAlignment(const Alignment& align, UIUpdateMode mode)
	{
		m_compassAlign = align;
		invalidate(mode);
	}

	sl_ui_len MapView::getCompassMarginLeft()
	{
		return m_compassMargin.left;
	}

	void MapView::setCompassMarginLeft(sl_ui_len margin, UIUpdateMode mode)
	{
		m_compassMargin.left = margin;
		invalidate(mode);
	}

	sl_ui_len MapView::getCompassMarginTop()
	{
		return m_compassMargin.top;
	}

	void MapView::setCompassMarginTop(sl_ui_len margin, UIUpdateMode mode)
	{
		m_compassMargin.top = margin;
		invalidate(mode);
	}

	sl_ui_len MapView::getCompassMarginRight()
	{
		return m_compassMargin.right;
	}

	void MapView::setCompassMarginRight(sl_ui_len margin, UIUpdateMode mode)
	{
		m_compassMargin.right = margin;
		invalidate(mode);
	}

	sl_ui_len MapView::getCompassMarginBottom()
	{
		return m_compassMargin.bottom;
	}

	void MapView::setCompassMarginBottom(sl_ui_len margin, UIUpdateMode mode)
	{
		m_compassMargin.bottom = margin;
		invalidate(mode);
	}

	void MapView::setCompassMargin(sl_ui_len left, sl_ui_len top, sl_ui_len right, sl_ui_len bottom, UIUpdateMode mode)
	{
		m_compassMargin.left = left;
		m_compassMargin.top = top;
		m_compassMargin.right = right;
		m_compassMargin.bottom = bottom;
		invalidate(mode);
	}

	void MapView::setCompassMargin(sl_ui_len margin, UIUpdateMode mode)
	{
		m_compassMargin.left = margin;
		m_compassMargin.top = margin;
		m_compassMargin.right = margin;
		m_compassMargin.bottom = margin;
		invalidate(mode);
	}

	const UIEdgeInsets& MapView::getCompassMargin()
	{
		return m_compassMargin;
	}

	void MapView::setCompassMargin(const UIEdgeInsets& margin, UIUpdateMode mode)
	{
		m_compassMargin = margin;
		invalidate(mode);
	}

	UIPoint MapView::getCompassLocation()
	{
		Alignment align = m_compassAlign;
		Alignment halign = align & Alignment::HorizontalMask;
		Alignment valign = align & Alignment::VerticalMask;
		sl_ui_len size = m_compassSize;
		UIPoint ret;
		if (halign == Alignment::Left) {
			ret.x = m_compassMargin.left;
		} else if (halign == Alignment::Right) {
			ret.x = getWidth() - m_compassMargin.right - size;
		} else {
			ret.x = (getWidth() - m_compassMargin.right + m_compassMargin.left - size) / 2;
		}
		if (valign == Alignment::Top) {
			ret.y = m_compassMargin.top;
		} else if (valign == Alignment::Bottom) {
			ret.y = getHeight() - m_compassMargin.bottom - size;
		} else {
			ret.y = (getHeight() - m_compassMargin.bottom + m_compassMargin.top - size) / 2;
		}
		return ret;
	}

	void MapView::renderCompass(RenderEngine* engine)
	{
		if (!m_flagGlobeMode) {
			return;
		}
		if (m_compass.isNone()) {
			return;
		}
		Ref<Image> compass = m_compass.evaluate(m_compassState);
		if (compass.isNull()) {
			return;
		}
		Ref<Texture> texture = Texture::getBitmapRenderingCache(compass);
		if (texture.isNull()) {
			return;
		}
		sl_real size = (sl_real)m_compassSize;
		if (size < 1.0f) {
			return;
		}
		sl_real halfSize = size / 2.0f;
		Point pt = getCompassLocation();
		Matrix3 transform = Transform2::getTranslationMatrix(-m_compassCenter)
			* Transform2::getScalingMatrix(size, size)
			* Transform2::getRotationMatrix(-Math::getRadianFromDegrees(m_state.rotation))
			* Transform2::getTranslationMatrix(pt.x + halfSize, pt.y + halfSize)
			* Transform2::getScalingMatrix(2.0f / (sl_real)(m_state.viewportWidth), -2.0f / (sl_real)(m_state.viewportHeight))
			* Transform2::getTranslationMatrix(-1.0f, 1.0f);
		engine->setDepthStencilState(m_state.overlayDepthState);
		engine->setBlendState(m_state.overlayBlendState);
		engine->drawTexture2D(transform, texture);
	}

	void MapView::doInvalidate(UIUpdateMode mode)
	{
		invalidate(mode);
	}

	SLIB_DEFINE_EVENT_HANDLER(MapView, ChangeLocation, (const GeoLocation& location, UIEvent* ev), location, ev)

	void MapView::notifyChangeLocation(const GeoLocation& location, UIEvent* ev)
	{
		invokeChangeLocation(location, ev);
	}

	SLIB_DEFINE_EVENT_HANDLER(MapView, ChangeRotation, (double rotation, UIEvent* ev), rotation, ev)

	void MapView::notifyChangeRotation(double rotation, UIEvent* ev)
	{
		invokeChangeRotation(rotation, ev);
	}

	SLIB_DEFINE_EVENT_HANDLER(MapView, ChangeTilt, (double tilt, UIEvent* ev), tilt, ev)

	void MapView::notifyChangeTilt(double tilt, UIEvent* ev)
	{
		invokeChangeTilt(tilt, ev);
	}

	void MapView::onDraw(Canvas* canvas)
	{
		resize(getWidth(), getHeight());
		drawPlane(canvas);
	}

	void MapView::onFrame(RenderEngine* engine)
	{
		resize(getWidth(), getHeight());
		renderGlobe(engine);
		renderCompass(engine);
		RenderView::onFrame(engine);
	}

	sl_bool MapView::_isPointInCompass(const Point& pt)
	{
		sl_real compassSize = (sl_real)(m_compassSize / 2);
		sl_real compassDistance = (pt - Point(getCompassLocation() + Point(compassSize, compassSize))).getLength2p() / compassSize / compassSize;
		if (m_compass.isNotNone() && compassDistance >= 0.01f && compassDistance <= 1.0f) {
			return sl_true;
		}
		return sl_false;
	}

	void MapView::onMouseEvent(UIEvent* ev)
	{
		RenderView::onMouseEvent(ev);
		if (ev->isAccepted()) {
			return;
		}
		double width = (double)(getWidth());
		double height = (double)(getHeight());
		if (width < 0.00001 || height < 0.00001) {
			return;
		}
		UISize screenSize = UI::getScreenSize();
		double rem = (double)(Math::min(screenSize.x, screenSize.y)) / 100.0;
		Point pt = ev->getPoint();
		Point pt2 = pt;
		sl_uint32 nTouches = 0;
		sl_bool flagDrag = sl_false;
		UIAction action = ev->getAction();
		switch (action) {
			case UIAction::LeftButtonDown:
			case UIAction::TouchBegin:
				m_ptLeftDown = pt;
				m_transformLeftDown = m_state.verticalViewTransform;
				m_rotationLeftDown = m_state.rotation;
				m_tickLeftDown = System::getTickCount64();
				m_flagLeftDown = sl_true;
				stopMoving();
				if (m_flagGlobeMode) {
					if (_isPointInCompass(pt)) {
						m_compassState = ViewState::Pressed;
						invalidate();
						break;
					}
				}
				m_compassState = ViewState::Normal;
				m_flagClicking = sl_true;
				invalidate();
				break;
			case UIAction::MouseMove:
				if (m_flagGlobeMode) {
					if (_isPointInCompass(pt)) {
						if (m_compassState == ViewState::Normal) {
							m_compassState = ViewState::Hover;
							invalidate();
						}
					} else {
						if (m_compassState == ViewState::Hover) {
							m_compassState = ViewState::Normal;
							invalidate();
						}
					}
				}
				break;
			case UIAction::MouseLeave:
				if (m_compassState == ViewState::Hover) {
					m_compassState = ViewState::Normal;
					invalidate();
				}
				break;
			case UIAction::TouchMove:
			case UIAction::TouchEnd:
			case UIAction::LeftButtonDrag:
				flagDrag = sl_true;
			case UIAction::LeftButtonUp:
			case UIAction::TouchCancel:
				if (!m_flagLeftDown) {
					break;
				}
				if (m_compassState == ViewState::Pressed) {
					if (!flagDrag) {
						m_compassState = ViewState::Normal;
						invalidate();
					}
					if (nTouches >= 2) {
						break;
					}
					sl_real size = (sl_real)(m_compassSize / 2);
					Vector2 dir = (pt - Point(getCompassLocation() + Point(size, size)));
					sl_real dist = dir.getLength2p() / size / size;
					if (dist < 0.01f) {
						break;
					}
					sl_real rotation = -(Math::getDegreesFromRadian(Transform2::getRotationAngleFromDirToDir(Vector2(0, -1), dir)));
					setEyeRotation(rotation, UIUpdateMode::Animate);
					if (flagDrag) {
						break;
					}
					double dt = (double)(System::getTickCount64() - m_tickLeftDown);
					if (dt > 300) {
						break;
					}
					double dx = pt.x - m_ptLeftDown.x;
					double dy = pt.y - m_ptLeftDown.y;
					if (dx * dx + dy * dy > rem) {
						break;
					}
					if (Math::abs(Math::normalizeDegreeDistance(rotation - m_rotationLeftDown)) < 20) {
						setEyeRotation(0.0f, UIUpdateMode::Animate);
					}
					break;
				}
				nTouches = ev->getTouchPointCount();
				if (nTouches >= 2) {
					pt = ev->getTouchPoint(0).point;
					pt2 = ev->getTouchPoint(1).point;
				} else {
					nTouches = m_nLastTouches;
				}
				if (nTouches >= 2) {
					m_flagClicking = sl_false;
					if (m_nLastTouches < 2) {
						stopMoving();
						m_ptTouchStart1 = pt;
						m_ptTouchStart2 = pt2;
						m_rotationTouchStart = m_state.rotation;
						m_altitudeTouchStart = m_state.eyeLocation.altitude;
						m_flagTouchRotateStarted = sl_false;
					} else {
						Vector2 v1 = m_ptTouchStart2 - m_ptTouchStart1;
						Vector2 v2 = pt2 - pt;
						sl_real len1 = v1.getLength();
						sl_real len2 = v2.getLength();
						if (len1 > rem / 2.0 && len2 > rem / 2.0) {
							sl_real a = Math::getDegreesFromRadian(Transform2::getRotationAngleFromDirToDir(v1, v2));
							sl_real r = m_rotationTouchStart;
							sl_real d = Math::abs(Math::normalizeDegreeDistance(a));
							if (nTouches > 2) {
								m_flagTouchRotateStarted = sl_false;
							}
							if ((d > 10 || m_flagTouchRotateStarted) && nTouches <= 2) {
								r -= a;
								setEyeRotation(r, UIUpdateMode::Animate);
								m_flagTouchRotateStarted = sl_true;
							} else {
								GeoLocation location = m_state.eyeLocation;
								if (len1 > len2) {
									location.altitude = m_altitudeTouchStart * len1 / len2 * 1.4;
								} else if (len1 < len2) {
									location.altitude = m_altitudeTouchStart * len1 / len2 / 1.4;
								}
								setEyeLocation(location, ev, UIUpdateMode::Animate);
							}
						}
					}
				} else {
					double dx = pt.x - m_ptLeftDown.x;
					double dy = pt.y - m_ptLeftDown.y;
					if (dx * dx + dy * dy > rem) {
						m_flagClicking = sl_false;
					}
					if (m_flagGlobeMode) {
						GeoLocation eye = m_state.eyeLocation;
						double alt = eye.altitude;
						double f = alt / height * 1.3;
						Vector3 pos = m_transformLeftDown.inverse().transformPosition(-dx * f, dy * f, alt);
						GeoLocation loc = MapEarth::getGeoLocation(pos);
						loc.altitude = alt;
						setEyeLocation(loc, ev);
					} else {
						movePlane(pt.x - m_ptLastEvent.x, pt.y - m_ptLastEvent.y, ev);
					}
				}
				if (!flagDrag) {
					if (m_flagClicking) {
						double dx = pt.x - m_ptLeftDown.x;
						double dy = pt.y - m_ptLeftDown.y;
						if (dx * dx + dy * dy < rem) {
							click(pt);
						}
						m_flagClicking = sl_false;
					}
					m_flagLeftDown = sl_false;
				}
				break;
			case UIAction::RightButtonDown:
				if (isFocusable()) {
					setFocus();
				}
				break;
			case UIAction::RightButtonDrag:
				{
					float dx = (float)((pt.x - m_ptLastEvent.x) / width * 360.0);
					float dy = (float)((pt.y - m_ptLastEvent.y) / height * 90.0);
					float rotation = m_state.rotation;
					setEyeRotation(rotation - dx);
					float tilt = m_state.tilt;
					setEyeTilt(tilt + dy);
				}
				break;
			default:
				break;
		}
		m_nLastTouches = nTouches;
		m_ptLastEvent = pt;
	}

	void MapView::onMouseWheelEvent(UIEvent* ev)
	{
		RenderView::onMouseWheelEvent(ev);
		if (ev->isAccepted()) {
			return;
		}
		sl_real delta = ev->getDelta();
		if (delta > 0) {
			zoomAt(ev->getPoint(), 1.0/1.1, ev);
		} else if (delta < 0) {
			zoomAt(ev->getPoint(), 1.1, ev);
		}
	}

	void MapView::onKeyEvent(UIEvent* ev)
	{
		if (ev->getAction() == UIAction::KeyDown) {
			Keycode keycode = ev->getKeycode();
			switch (keycode) {
				case Keycode::Minus:
				case Keycode::NumpadMinus:
					zoom(1.1, ev);
					ev->accept();
					return;
				case Keycode::Equal:
				case Keycode::NumpadPlus:
					zoom(1.0 / 1.1, ev);
					ev->accept();
					return;
				default:
					break;
			}
		}
		RenderView::onKeyEvent(ev);
	}

	void MapView::onResize(sl_ui_len width, sl_ui_len height)
	{
		RenderView::onResize(width, height);
		resize(width, height);
	}

}
