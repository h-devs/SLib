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
#include "slib/geo/dem.h"
#include "slib/math/triangle.h"
#include "slib/math/transform3d.h"
#include "slib/ui/resource.h"
#include "slib/core/thread_pool.h"
#include "slib/core/stringify.h"
#include "slib/core/safe_static.h"

#define LAYER_COUNT SLIB_MAP_VIEW_LAYER_COUNT
#define EARTH_CIRCUMFERENCE SLIB_GEO_EARTH_CIRCUMFERENCE_EQUATORIAL
#define METER_PER_DEGREE ((double)EARTH_CIRCUMFERENCE / 360.0)
#define ALTITUDE_PER_METER 0.8660254037844386 // (1 - 0.5^2)^0.5

#define TILE_MIN_VERTEX_COUNT 65

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

		typedef SphericalEarth MapEarth;
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
		if (address.subPath.isNotNull()) {
			_out = File::readAllBytes(File::concatPath(m_root, path, address.subPath));
		} else {
			_out = File::readAllBytes(File::concatPath(m_root, path));
		}
		if (_out.isNotNull()) {
			return sl_true;
		}
		return File::isDirectory(m_root);
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
		m_scale = 1.0;
		m_scaleMin = 0.0;
		m_scaleMax = 10000000000.0;
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
		double w = m_scale * m_viewport.getWidth() / m_viewport.getHeight();
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
		double f = m_viewport.getHeight() / m_scale;
		return { m_viewport.getCenterX() + (location.E - m_center.E) * f, m_viewport.getCenterY() + (m_center.N - location.N) * f };
	}

	MapLocation MapPlane::getMapLocationFromViewPoint(const Double2& point)
	{
		double f = m_scale / m_viewport.getHeight();
		return { m_center.E + (point.x - m_viewport.getCenterX()) * f, m_center.N - (point.y - m_viewport.getCenterY()) * f };
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


	SLIB_DEFINE_OBJECT(MapViewTile, CRef)

	MapViewTile::MapViewTile()
	{
	}

	MapViewTile::~MapViewTile()
	{
	}

	sl_bool MapViewTile::build(DEM::DataType demType, const Rectangle* demRect)
	{
		DEM model;
		model.initialize(demType, dem.getData(), dem.getSize());
		
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
				if (M < TILE_MIN_VERTEX_COUNT) {
					M = TILE_MIN_VERTEX_COUNT;
				}
				memVertices = Memory::create(sizeof(MapViewVertex) * M * M);
				if (memVertices.isNull()) {
					return sl_false;
				}
				MapViewVertex* v = (MapViewVertex*)(memVertices.getData());
				float mx0 = demRect->left * (float)(L - 1);
				float my0 = demRect->top * (float)(L - 1);
				float mx1 = demRect->right * (float)(L - 1);
				float my1 = demRect->bottom * (float)(L - 1);
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
				M = L;
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
			M = TILE_MIN_VERTEX_COUNT;
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
			pointsWithDEM[0] = v[(M - 1) * M].position; // Bottom Left
			pointsWithDEM[1] = v[M * M - 1].position; // Bottom Right
			pointsWithDEM[2] = v[0].position; // Top Left
			pointsWithDEM[3] = v[M - 1].position; // Top Right
			for (sl_uint32 i = 0; i < 4; i++) {
				pointsWithDEM[i] += center;
			}
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
					*(indices++) = (sl_uint16)(y * M + x); // Top Left
					*(indices++) = (sl_uint16)(y * M + (x + 1)); // Top Right
					*(indices++) = (sl_uint16)((y + 1) * M + x); // Bottom Left
					*(indices++) = *(indices - 1);
					*(indices++) = *(indices - 3);
					*(indices++) = (sl_uint16)((y + 1) * M + (x + 1)); // Bottom Right
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


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(MapSurfaceParam)

	MapSurfaceParam::MapSurfaceParam()
	{
		baseLevel = 0;
		maxLevel = 20;
		baseTileCountE = 1;
		baseTileCountN = 1;
		degreeLengthE = 360.0;
		degreeLengthN = 360.0;
		tileLength = 256;
		demType = DEM::DataType::FloatLE;
	}

	SLIB_DEFINE_OBJECT(MapSurface, Object)

	MapSurface::MapSurface()
	{
		m_baseLevel = 0;
		m_maxLevel = 20;
		m_baseTileCountE = 1;
		m_baseTileCountN = 1;
		m_degreeLengthE = 360.0;
		m_degreeLengthN = 360.0;
		m_tileLength = 256;
		m_demType = DEM::DataType::FloatLE;
		{
			for (sl_uint32 i = 0; i < LAYER_COUNT; i++) {
				m_layers[i].flagVisible = sl_true;
				m_layers[i].opacity = 1.0f;
			}
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

			SLIB_RENDER_PROGRAM_STATE_INPUT_FLOAT3(position, a_Position)
			SLIB_RENDER_PROGRAM_STATE_INPUT_FLOAT2(texCoord, a_TexCoord)
		SLIB_RENDER_PROGRAM_STATE_END

		class RenderProgram_SurfaceTile : public RenderProgramT<RenderProgramState_SurfaceTile>
		{
		public:
			String getGLSLVertexShader(RenderEngine* engine) override
			{
				String source;
				source = SLIB_STRINGIFY(
					uniform mat4 u_Transform;
					attribute vec3 a_Position;
					attribute vec2 a_TexCoord;
					varying vec2 v_TexCoord;
					void main() {
						vec4 P = vec4(a_Position, 1.0) * u_Transform;
						gl_Position = P;
						v_TexCoord = a_TexCoord;
					}
				);
				return source;
			}

			String getGLSLFragmentShader(RenderEngine* engine) override
			{
				String source;
				source = SLIB_STRINGIFY(
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
				);
				return source;
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
				m_baseLevel = param.baseLevel;
				m_maxLevel = param.maxLevel;
				m_baseTileCountE = param.baseTileCountE;
				m_baseTileCountN = param.baseTileCountN;
				m_degreeLengthE = param.degreeLengthE;
				m_degreeLengthN = param.degreeLengthN;
				m_toReaderLocation = param.toReaderLocation;
				m_tileLength = param.tileLength;

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
				const MapViewState& state = data->getState();
				for (sl_uint32 y = 0; y < m_baseTileCountN; y++) {
					for (sl_uint32 x = 0; x < m_baseTileCountE; x++) {
						renderTile(engine, state, MapTileLocationI(m_baseLevel, x, y));
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
				// Check Frustum
				{
					if (!(state.viewFrustum.containsFacets(tile->points, 4) || state.viewFrustum.containsFacets(tile->pointsWithDEM, 4))) {
						return;
					}
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
					if (!(tile->build(m_demType, dem.flagUseWhole ? sl_null : &(dem.region)))) {
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

			sl_bool isTileFrontFace(const MapViewState& state, MapViewTile* tile)
			{
				Vector3 ptBL, ptBR, ptTL, ptTR;
				if (tile->region.topRight.longitude - tile->region.bottomLeft.longitude > 1.0) {
					ptBL = state.viewTransform.transformPosition(tile->points[0]);
					ptBR = state.viewTransform.transformPosition(tile->points[1]);
					ptTL = state.viewTransform.transformPosition(tile->points[2]);
					ptTR = state.viewTransform.transformPosition(tile->points[3]);
				} else {
					ptBL = Transform3T<double>::projectToViewport(state.viewProjectionTransform, tile->points[0]);
					ptBR = Transform3T<double>::projectToViewport(state.viewProjectionTransform, tile->points[1]);
					ptTL = Transform3T<double>::projectToViewport(state.viewProjectionTransform, tile->points[2]);
					ptTR = Transform3T<double>::projectToViewport(state.viewProjectionTransform, tile->points[3]);
				}
				Triangle triangle;
				triangle.point1.x = ptTL.x;
				triangle.point1.y = -ptTL.y;
				triangle.point2.x = ptTR.x;
				triangle.point2.y = -ptTR.y;
				triangle.point3.x = ptBL.x;
				triangle.point3.y = -ptBL.y;
				if (triangle.isClockwise()) {
					return sl_true;
				}
				triangle.point1.x = ptBR.x;
				triangle.point1.y = -ptBR.y;
				return !(triangle.isClockwise());
			}

			sl_bool isTileExpandable(const MapViewState& state, MapViewTile* tile)
			{
				// Check Expand
				if (tile->location.level >= m_maxLevel) {
					return sl_false;
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
					if (size > (sl_real)(65536.0 * 25.0 / state.viewportWidth / state.viewportWidth)) {
						return sl_true;
					}
				}
				return sl_false;
			}

			const List< Ref<MapViewTile> >& getTiles() override
			{
				return m_renderingTiles;
			}

			void onDrawPlane(Canvas* canvas, const Rectangle& rect, MapSurfacePlane* plane, MapViewData* data) override
			{
				double planeScale = plane->getScale();
				double planeMpp = planeScale / rect.getHeight();
				planeMpp *= 2.5; // factor
				double tileMpp = METER_PER_DEGREE * m_degreeLengthE / (double)(m_baseTileCountE) / (double)m_tileLength;
				sl_uint32 level = m_baseLevel;
				do {
					if (planeMpp > tileMpp) {
						break;
					}
					tileMpp /= 2.0;
					level++;
				} while (level < m_maxLevel);

				drawLevel(canvas, rect, level, plane->getCenterLocation(), planeScale, data->getState().tileLoader.get(), tileMpp);

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
				double sx = (center.E + METER_PER_DEGREE * m_degreeLengthE / 2.0) / tileMpp - w / 2.0;
				double sy = (center.N + METER_PER_DEGREE * m_degreeLengthN / 2.0) / tileMpp - h / 2.0;
				double ex = sx + w;
				double ey = sy + h;
				sl_uint64 m = (sl_uint64)1 << level;
				double mw = (double)(m_tileLength * m_baseTileCountE * m);
				double mh = (double)(m_tileLength * m_baseTileCountN * m);
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
				sl_uint32 tsx = isx / m_tileLength;
				sl_uint32 tsy = isy / m_tileLength;
				sl_uint32 tex = iex / m_tileLength;
				sl_uint32 tey = iey / m_tileLength;
				if (iex % m_tileLength) {
					tex++;
				}
				if (iey % m_tileLength) {
					tey++;
				}
				double scale = rcView.getHeight() / h;
				sl_real ts = (sl_real)(m_tileLength * scale);
				for (sl_uint32 ty = tsy; ty < tey; ty++) {
					for (sl_uint32 tx = tsx; tx < tex; tx++) {
						MapTileLocationI location(level, tx, ty);
						TileImage image;
						if (!(loadPicture(image, loader, location))) {
							continue;
						}
						image.convertToSourceCoordinate();
						Rectangle rcDst;
						rcDst.left = rcView.left + (sl_real)(((double)(tx * m_tileLength) - sx) * scale);
						rcDst.top = rcView.bottom - (sl_real)(((double)((ty + 1) * m_tileLength) - sy) * scale);
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
				param.flagLoadNow = location.level <= m_baseLevel;
				param.flagEndless = location.level == m_baseLevel;
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
				if (location.level <= m_baseLevel) {
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
				param.flagLoadNow = location.level <= m_baseLevel;
				param.flagEndless = location.level == m_baseLevel;
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
				if (location.level <= m_baseLevel) {
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

	sl_uint32 MapSurface::getBaseLevel()
	{
		return m_baseLevel;
	}

	sl_uint32 MapSurface::getMaximumLevel()
	{
		return m_maxLevel;
	}

	sl_uint32 MapSurface::getBaseTileCountE()
	{
		return m_baseTileCountE;
	}

	sl_uint32 MapSurface::getBaseTileCountN()
	{
		return m_baseTileCountN;
	}

	double MapSurface::getDegreeLengthE()
	{
		return m_degreeLengthE;
	}

	double MapSurface::getDegreeLengthN()
	{
		return m_degreeLengthN;
	}

	sl_uint32 MapSurface::getTileLength()
	{
		return m_tileLength;
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

	void MapSurface::setDemReader(const Ref<MapTileReader>& reader)
	{
		m_readerDEM = reader;
		clearCache();
	}

	DEM::DataType MapSurface::getDemType()
	{
		return m_demType;
	}

	void MapSurface::setDemType(DEM::DataType type)
	{
		m_demType = type;
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
		if (location.level < m_baseLevel) {
			return { 0, 0 };
		}
		sl_uint64 n = (sl_uint64)1 << (location.level - m_baseLevel);
		sl_uint64 nE = n * m_baseTileCountE;
		sl_uint64 nN = n * m_baseTileCountN;
		LatLon ret;
		ret.latitude = ((double)(location.N) / (double)nN - 0.5) * m_degreeLengthN;
		ret.longitude = ((double)(location.E) / (double)nE - 0.5) * m_degreeLengthE;
		return ret;
	}

	MapTileLocation MapSurface::getTileLocationFromLatLon(sl_uint32 level, const LatLon& location)
	{
		if (level < m_baseLevel) {
			return { level, 0, 0 };
		}
		sl_uint64 n = (sl_uint64)1 << (level - m_baseLevel);
		sl_uint64 nE = n * m_baseTileCountE;
		sl_uint64 nN = n * m_baseTileCountN;
		MapTileLocation ret;
		ret.level = level;
		ret.N = (0.5 + location.latitude / m_degreeLengthN) * nN;
		ret.E = (0.5 + location.longitude / m_degreeLengthE) * nE;
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
		m_scaleMin = 50.0;
		m_scaleMax = EARTH_CIRCUMFERENCE / 2.0;
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

	GeoLocation MapSurfacePlane::getEyeLocation()
	{
		return { m_center.N / METER_PER_DEGREE, m_center.E / METER_PER_DEGREE, m_scale * ALTITUDE_PER_METER };
	}

	void MapSurfacePlane::setEyeLocation(const GeoLocation& location)
	{
		setScale(location.altitude / ALTITUDE_PER_METER);
		setCenterLocation(location.longitude * METER_PER_DEGREE, location.latitude * METER_PER_DEGREE);
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

	void MapSurfacePlane::onDraw(Canvas* canvas, const Rectangle& rect, MapViewData* data)
	{
		m_surface->onDrawPlane(canvas, rect, this, data);
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

	void MapViewObject::draw(Canvas* canvas, const Rectangle& rect, MapViewData* data, MapPlane* plane)
	{
	}

	void MapViewObject::render(RenderEngine* engine, MapViewData* data, MapSurface* surface)
	{
	}


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(MapViewState)

	MapViewState::MapViewState()
	{
		viewportWidth = 1.0;
		viewportHeight = 1.0;

		eyeLocation.altitude = 10000;
		tilt = 0.0f;
		rotation = 0.0f;
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
		if (overlayDepthState.isNull()) {
			RenderDepthStencilParam dp;
			dp.flagTestDepth = sl_false;
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
			zFar = dist + MapEarth::getRadius() * 2;
		}
		projectionTransform = Transform3T<double>::getPerspectiveProjectionFovYMatrix(SLIB_PI / 3, viewportWidth / viewportHeight, zNear, zFar);
		viewProjectionTransform = viewTransform * projectionTransform;
		viewFrustum = ViewFrustumT<double>::fromMVP(viewProjectionTransform);
		return sl_true;
	}


	MapViewData::MapViewData()
	{
		m_view = sl_null;
		m_flagGlobeMode = sl_false;
		m_flagRendered = sl_false;

		m_altitudeMin = 50.0;
		m_altitudeMax = 100000000.0;
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
			}
		} else {
			if (m_plane.isNotNull()) {
				m_plane->setEyeLocation(m_state.eyeLocation);
				invalidate(mode);
			}
		}
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

	const MapViewState& MapViewData::getState() const
	{
		return m_state;
	}

	MapViewState& MapViewData::getState()
	{
		return m_state;
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

	void MapViewData::setEyeLocation(const GeoLocation& location, UIUpdateMode mode)
	{
		MutexLocker locker(&m_lock);
		if (!m_flagGlobeMode) {
			if (m_plane.isNotNull()) {
				m_plane->setEyeLocation(location);
				invalidate(mode);
				return;
			}
		}
		m_state.eyeLocation = location;
		if (m_state.eyeLocation.altitude < m_altitudeMin) {
			m_state.eyeLocation.altitude = m_altitudeMin;
		}
		if (m_state.eyeLocation.altitude > m_altitudeMax) {
			m_state.eyeLocation.altitude = m_altitudeMax;
		}
		invalidate(mode);
	}

	double MapViewData::getMinimumAltitude()
	{
		return m_altitudeMin;
	}

	void MapViewData::setMinimumAltitude(double altitude, UIUpdateMode mode)
	{
		m_altitudeMin = altitude;
		if (SLIB_UI_UPDATE_MODE_IS_INIT(mode)) {
			return;
		}
		setEyeLocation(getEyeLocation(), mode);
	}

	double MapViewData::getMaximumAltitude()
	{
		return m_altitudeMax;
	}

	void MapViewData::setMaximumAltitude(double altitude, UIUpdateMode mode)
	{
		m_altitudeMax = altitude;
		if (SLIB_UI_UPDATE_MODE_IS_INIT(mode)) {
			return;
		}
		setEyeLocation(getEyeLocation(), mode);
	}

	sl_bool MapViewData::getLatLonFromViewPoint(const Double2& point, LatLon& _out) const
	{
		MutexLocker locker(&m_lock);
		if (m_flagGlobeMode) {
		} else {
			MapPlane* plane = m_plane.get();
			if (plane) {
				_out = plane->getLatLonFromMapLocation(plane->getMapLocationFromViewPoint(point));
				return sl_true;
			}
		}
		return sl_false;
	}

	Double2 MapViewData::getViewPointFromLatLon(const LatLon& location) const
	{
		MutexLocker locker(&m_lock);
		if (m_flagGlobeMode) {
		} else {
			MapPlane* plane = m_plane.get();
			if (plane) {
				return plane->getViewPointFromMapLocation(plane->getMapLocationFromLatLon(location));
			}
		}
		return { 0.0, 0.0 };
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
		double w = width / height / 2.0;
		RectangleT<double> rect;
		rect.left = -w;
		rect.top = -0.5;
		rect.right = w;
		rect.bottom = 0.5;
		plane->setViewport(rect);
	}

	void MapViewData::movePlane(double dx, double dy, UIUpdateMode mode)
	{
		MutexLocker locker(&m_lock);
		if (m_flagGlobeMode) {
			return;
		}
		MapPlane* plane = m_plane.get();
		if (plane) {
			double scale = plane->getScale();
			MapLocation center = plane->getCenterLocation();
			plane->setCenterLocation(center.E - dx * scale, center.N + dy * scale);
		}
		invalidate(mode);
	}

	void MapViewData::zoom(double factor, UIUpdateMode mode)
	{
		MutexLocker locker(&m_lock);
		if (m_flagGlobeMode) {
			GeoLocation location = m_state.eyeLocation;
			location.altitude *= factor;
			setEyeLocation(location);
			invalidate(mode);
		} else {
			MapPlane* plane = m_plane.get();
			if (plane) {
				m_plane->setScale(plane->getScale() * factor);
				invalidate(mode);
			}
		}
	}

	void MapViewData::zoomAt(const Double2& pt, double factor, UIUpdateMode mode)
	{
		if (m_flagGlobeMode) {
			zoom(factor, mode);
			return;
		}
		MutexLocker locker(&m_lock);
		MapPlane* plane = m_plane.get();
		if (plane) {
			MapLocation c = plane->getCenterLocation();
			MapLocation l1 = plane->getMapLocationFromViewPoint(pt);
			double scale = plane->getScale() * factor;
			plane->setScale(scale);
			MapLocation l2 = plane->getMapLocationFromViewPoint(pt);
			plane->setCenterLocation(c.E - l2.E + l1.E, c.N - l2.N + l1.N);
		}
		invalidate(mode);
	}

	void MapViewData::click(const Double2& pt, UIUpdateMode mode)
	{
		invalidate(mode);
	}

	void MapViewData::stopMoving()
	{
	}

	sl_bool MapViewData::_initState()
	{
		if (m_state.tileLoader.isNull()) {
			m_state.tileLoader = MapTileLoader::create(SLIB_FUNCTION_MEMBER(this, _onCompleteLazyLoading));
			if (m_state.tileLoader.isNull()) {
				return sl_false;
			}
		}
		return sl_true;
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
		if (!(_initState())) {
			return;
		}
		MapPlane* plane = m_plane.get();
		if (!plane) {
			return;
		}
		plane->draw(canvas, rect, this);
		{
			auto node = m_objects.getFirstNode();
			while (node) {
				Ref<MapViewObject>& object = node->value;
				if (object->isSupportingPlaneMode()) {
					object->draw(canvas, rect, this, plane);
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
		if (!(m_state.update())) {
			return;
		}

		engine->setDepthStencilState(m_state.defaultDepthState);
		engine->setBlendState(m_state.defaultBlendState);
		surface->render(engine, this);

		{
			auto node = m_objects.getFirstNode();
			while (node) {
				Ref<MapViewObject>& object = node->value;
				if (object->isSupportingGlobeMode()) {
					if (object->isOverlay()) {
						engine->setDepthStencilState(m_state.overlayDepthState);
						engine->setBlendState(m_state.overlayBlendState);
					} else {
						engine->setDepthStencilState(m_state.defaultDepthState);
						engine->setBlendState(m_state.defaultBlendState);
					}
					object->render(engine, this, surface);
				}
				node = node->next;
			}
		}

		m_flagRendered = sl_true;
	}

	void MapViewData::invalidate(UIUpdateMode mode)
	{
		if (m_view) {
			m_view->invalidate(mode);
		}
	}

	void MapViewData::_onCompleteLazyLoading()
	{
		invalidate();
	}


	SLIB_DEFINE_OBJECT(MapView, RenderView)

	MapView::MapView()
	{
		m_view = this;
		setRedrawMode(RedrawMode::WhenDirty);
		setFocusable();

		m_nLastTouches = 0;
		m_ptLastEvent.x = 0;
		m_ptLastEvent.y = 0;
		m_flagMouseDown = sl_false;
		m_tickMouseDown = 0;
		m_ptMouseDown.x = 0;
		m_ptMouseDown.y = 0;
		m_flagClicking = sl_false;
		m_flagThrowMoving = sl_false;
	}

	MapView::~MapView()
	{
	}

	void MapView::onDraw(Canvas* canvas)
	{
		resize(getWidth(), getHeight());
		drawPlane(canvas, getBounds());
	}

	void MapView::onFrame(RenderEngine* engine)
	{
		resize(getWidth(), getHeight());
		renderGlobe(engine);
		RenderView::onFrame(engine);
	}

	namespace
	{
		static Double2 GetRelativePoint(View* view, const Point& pt)
		{
			double w = (double)(view->getWidth());
			double h = (double)(view->getHeight());
			return { (pt.x - w / 2.0) / h, pt.y / h - 0.5 };
		}
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
		double rem = UIResource::getScreenMinimum() / 100.0;
		Point pt = ev->getPoint();
		Point pt2 = pt;
		sl_uint32 nTouches = 0;
		sl_bool flagDrag = sl_false;
		UIAction action = ev->getAction();
		switch (action) {
			case UIAction::LeftButtonDown:
			case UIAction::TouchBegin:
				m_ptMouseDown = pt;
				m_transformMouseDown = m_state.verticalViewTransform;
				m_tickMouseDown = System::getTickCount64();
				m_flagMouseDown = sl_true;
				m_flagClicking = sl_true;
				m_flagThrowMoving = sl_false;
				stopMoving();
				invalidate();
				break;
			case UIAction::TouchMove:
			case UIAction::TouchEnd:
			case UIAction::LeftButtonDrag:
				flagDrag = sl_true;
			case UIAction::LeftButtonUp:
			case UIAction::TouchCancel:
				if (!m_flagMouseDown) {
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
					m_flagThrowMoving = sl_false;
				} else {
					double dx = pt.x - m_ptMouseDown.x;
					double dy = pt.y - m_ptMouseDown.y;
					if (dx * dx + dy * dy > rem) {
						m_flagClicking = sl_false;
					}
					if (m_flagGlobeMode) {
						GeoLocation eye = m_state.eyeLocation;
						double alt = eye.altitude;
						double f = alt / height * 1.3;
						Vector3 pos = m_transformMouseDown.inverse().transformPosition(-dx * f, dy * f, alt);
						GeoLocation loc = MapEarth::getGeoLocation(pos);
						loc.altitude = alt;
						setEyeLocation(loc);
					} else {
						movePlane((pt.x - m_ptLastEvent.x) / height, (pt.y - m_ptLastEvent.y) / height);
					}
				}
				if (!flagDrag) {
					if (m_flagClicking) {
						double dx = pt.x - m_ptMouseDown.x;
						double dy = pt.y - m_ptMouseDown.y;
						if (dx * dx + dy * dy < rem) {
							click(GetRelativePoint(this, pt));
						}
						m_flagClicking = sl_false;
					}
					m_flagMouseDown = sl_false;
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
			zoomAt(GetRelativePoint(this, ev->getPoint()), 1.0/1.1);
		} else if (delta < 0) {
			zoomAt(GetRelativePoint(this, ev->getPoint()), 1.1);
		}
	}

	void MapView::onResize(sl_ui_len width, sl_ui_len height)
	{
		RenderView::onResize(width, height);
		resize(width, height);
	}

}
