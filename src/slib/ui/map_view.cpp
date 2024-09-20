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
#include "slib/ui/resource.h"
#include "slib/core/thread_pool.h"
#include "slib/core/stringify.h"
#include "slib/core/safe_static.h"

#define LAYER_COUNT SLIB_MAP_VIEW_LAYER_COUNT
#define EARTH_CIRCUMFERENCE SLIB_GEO_EARTH_CIRCUMFERENCE_EQUATORIAL
#define METER_PER_DEGREE ((double)EARTH_CIRCUMFERENCE / 360.0)
#define ALTITUDE_PER_METER 0.8660254037844386 // (1 - 0.5^2)^0.5

namespace slib
{

	namespace object_types
	{
		enum {
			MapViewObject = MapView + 1,
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

	Ref<MapTileDirectory> MapTileDirectory::open(const String& rootPath, const Function<String(MapTileLocationI&)>& formator)
	{
		Ref<MapTileDirectory> ret = new MapTileDirectory;
		if (ret.isNotNull()) {
			ret->m_root = rootPath;
			ret->m_formator = formator;
			return ret;
		}
		return sl_null;
	}

	sl_bool MapTileDirectory::readData(Memory& _out, const MapTileAddress& address, sl_uint32 timeout)
	{
		MapTileLocationI location = address;
		String path = m_formator(location);
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
	}

	SLIB_DEFINE_OBJECT(MapSurface, MapPlane)

	MapSurface::MapSurface()
	{
		m_baseLevel = 0;
		m_maxLevel = 20;
		m_baseTileCountE = 1;
		m_baseTileCountN = 1;
		m_degreeLengthE = 360.0;
		m_degreeLengthN = 360.0;
		m_tileLength = 256;
		m_range.right = EARTH_CIRCUMFERENCE / 2.0;
		m_range.left = - m_range.right;
		m_range.top = EARTH_CIRCUMFERENCE / 4.0;
		m_range.bottom = -m_range.top;
		m_scaleMin = 50.0;
		m_scaleMax = EARTH_CIRCUMFERENCE / 2.0;
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
		struct SurfaceVertex
		{
			Vector3 position;
			sl_real altitude;
			Vector2 texCoord;
			Vector2 texCoordLayers[LAYER_COUNT];
		};

		SLIB_RENDER_PROGRAM_STATE_BEGIN(RenderProgramState_SurfaceTile, SurfaceVertex)
			SLIB_RENDER_PROGRAM_STATE_UNIFORM_MATRIX4(Transform, u_Transform)
			SLIB_RENDER_PROGRAM_STATE_UNIFORM_TEXTURE(Texture, u_Texture, RenderShaderType::Pixel, 0)
			SLIB_RENDER_PROGRAM_STATE_UNIFORM_TEXTURE(LayerTexture0, u_LayerTexture0, RenderShaderType::Pixel, 1)
			SLIB_RENDER_PROGRAM_STATE_UNIFORM_TEXTURE(LayerTexture1, u_LayerTexture1, RenderShaderType::Pixel, 2)
			SLIB_RENDER_PROGRAM_STATE_UNIFORM_TEXTURE(LayerTexture2, u_LayerTexture2, RenderShaderType::Pixel, 3)
			SLIB_RENDER_PROGRAM_STATE_UNIFORM_TEXTURE(LayerTexture3, u_LayerTexture3, RenderShaderType::Pixel, 4)
			SLIB_RENDER_PROGRAM_STATE_UNIFORM_TEXTURE(LayerTexture4, u_LayerTexture4, RenderShaderType::Pixel, 5)
			SLIB_RENDER_PROGRAM_STATE_UNIFORM_FLOAT_ARRAY(LayerAlpha, u_LayerAlpha)

			SLIB_RENDER_PROGRAM_STATE_INPUT_FLOAT3(position, a_Position)
			SLIB_RENDER_PROGRAM_STATE_INPUT_FLOAT(altitude, a_Altitude)
			SLIB_RENDER_PROGRAM_STATE_INPUT_FLOAT2(texCoord, a_TexCoord)
			SLIB_RENDER_PROGRAM_STATE_INPUT_FLOAT2(texCoordLayers[0], a_TexCoordLayer0)
			SLIB_RENDER_PROGRAM_STATE_INPUT_FLOAT2(texCoordLayers[1], a_TexCoordLayer1)
			SLIB_RENDER_PROGRAM_STATE_INPUT_FLOAT2(texCoordLayers[2], a_TexCoordLayer2)
			SLIB_RENDER_PROGRAM_STATE_INPUT_FLOAT2(texCoordLayers[3], a_TexCoordLayer3)
			SLIB_RENDER_PROGRAM_STATE_INPUT_FLOAT2(texCoordLayers[4], a_TexCoordLayer4)
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
					attribute float a_Altitude;
					attribute vec2 a_TexCoord;
					attribute vec2 a_TexCoordLayer0;
					attribute vec2 a_TexCoordLayer1;
					attribute vec2 a_TexCoordLayer2;
					attribute vec2 a_TexCoordLayer3;
					attribute vec2 a_TexCoordLayer4;
					varying float v_Altitude;
					varying vec2 v_TexCoord;
					varying vec2 v_TexCoordLayer[5];
					void main() {
						vec4 P = vec4(a_Position, 1.0) * u_Transform;
						gl_Position = P;
						v_Altitude = a_Altitude;
						v_TexCoord = a_TexCoord;
						v_TexCoordLayer[0] = a_TexCoordLayer0;
						v_TexCoordLayer[1] = a_TexCoordLayer1;
						v_TexCoordLayer[2] = a_TexCoordLayer2;
						v_TexCoordLayer[3] = a_TexCoordLayer3;
						v_TexCoordLayer[4] = a_TexCoordLayer4;
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
					uniform float u_LayerAlpha[5];
					varying vec2 v_TexCoord;
					varying vec2 v_TexCoordLayer[5];
					void main() {
						vec4 colorTexture = texture2D(u_Texture, v_TexCoord);
						vec4 colorLayer0 = texture2D(u_LayerTexture0, v_TexCoordLayer[0]);
						vec4 colorLayer1 = texture2D(u_LayerTexture1, v_TexCoordLayer[1]);
						vec4 colorLayer2 = texture2D(u_LayerTexture2, v_TexCoordLayer[2]);
						vec4 colorLayer3 = texture2D(u_LayerTexture3, v_TexCoordLayer[3]);
						vec4 colorLayer4 = texture2D(u_LayerTexture4, v_TexCoordLayer[4]);

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
			Ref<MapTileCache> m_cacheLayers[LAYER_COUNT];

			struct TileImage
			{
				Ref<Image> source;
				sl_bool flagDrawWhole = sl_true;
				Rectangle region;
			};

			class RenderTile : public CRef
			{
			public:
				TileImage picture;
				DEM dem;
				Primitive primitive;
			};
			HashMap<MapTileLocationI, RenderTile> m_renderingTiles;
			HashMap<MapTileLocationI, RenderTile> m_lastRenderedTiles;

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
					for (sl_uint32 i = 0; i < LAYER_COUNT; i++) {
						Ref<MapTileCache> cache = MapTileCache::create(400, 5000);
						if (cache.isNull()) {
							return sl_false;
						}
						m_cacheLayers[i] = Move(cache);
						m_layers[i].reader = param.layers[i];
					}
				}
				return sl_true;
			}

			void clearCache() override
			{
				m_cachePicture->clear();
				for (sl_uint32 i = 0; i < LAYER_COUNT; i++) {
					m_cacheLayers[i]->clear();
				}
			}

			void onDraw(Canvas* canvas, const Rectangle& rect, MapViewData* data) override
			{
				MapTileLoader* loader = data->getTileLoader();
				if (!loader) {
					return;
				}

				double mpp = m_scale / rect.getHeight();
				mpp *= 2.5; // factor
				double p = METER_PER_DEGREE * m_degreeLengthE / (double)(m_baseTileCountE) / (double)m_tileLength;
				sl_uint32 level = m_baseLevel;
				do {
					if (mpp > p) {
						break;
					}
					p /= 2.0;
					level++;
				} while (level < m_maxLevel);

				drawLevel(canvas, rect, loader, level, p);

				{
					m_cachePicture->endStep();
					for (sl_uint32 i = 0; i < LAYER_COUNT; i++) {
						m_cacheLayers[i]->endStep();
					}
				}
			}

			void drawLevel(Canvas* canvas, const Rectangle& rcView, MapTileLoader* loader, sl_uint32 level, double scale)
			{
				double h = m_scale / scale;
				double w = h * rcView.getWidth() / rcView.getHeight();
				double sx = (m_center.E + METER_PER_DEGREE * m_degreeLengthE / 2.0) / scale - w / 2.0;
				double sy = (m_center.N + METER_PER_DEGREE * m_degreeLengthN / 2.0) / scale - h / 2.0;
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
				{
					scale = rcView.getHeight() / h;
					sl_real ts = (sl_real)(m_tileLength * scale);
					for (sl_uint32 ty = tsy; ty < tey; ty++) {
						for (sl_uint32 tx = tsx; tx < tex; tx++) {
							TileImage image;
							if (loadPicture(image, loader, level, tx, ty)) {
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
									if (layer.reader.isNotNull() && layer.flagVisible && layer.opacity > 0.001f) {
										if (loadImage(image, layer.reader, m_cacheLayers[i].get(), loader, level, tx, ty)) {
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
						}
					}
				}
			}

			sl_bool loadPicture(TileImage& _out, MapTileLoader* loader, sl_uint32 level, sl_uint32 tx, sl_uint32 ty)
			{
				return loadImage(_out, m_readerPicture, m_cachePicture.get(), loader, level, tx, ty);
			}

			sl_bool loadImage(TileImage& _out, const Ref<MapTileReader>& reader, MapTileCache* cache, MapTileLoader* loader, sl_uint32 level, sl_uint32 tx, sl_uint32 ty)
			{
				if (reader.isNull()) {
					return sl_false;
				}
				MapTileLoadParam param;
				param.reader = reader;
				param.address.level = level;
				param.address.E = tx;
				param.address.N = ty;
				param.cache = cache;
				param.flagLoadNow = level <= m_baseLevel;
				param.flagEndless = level == m_baseLevel;
				m_toReaderLocation(param.address);
				loader->loadImage(_out.source, param, sl_null);
				if (_out.source.isNotNull()) {
					_out.region.left = 0;
					_out.region.top = 0;
					_out.region.right = 256.0f;
					_out.region.bottom = 256.0f;
					_out.flagDrawWhole = sl_true;
					return sl_true;
				}
				if (level <= m_baseLevel) {
					return sl_false;
				}
				if (!(loadImage(_out, reader, cache, loader, level - 1, tx >> 1, ty >> 1))) {
					return sl_false;
				}
				if (tx & 1) {
					_out.region.left = _out.region.getCenterX();
				} else {
					_out.region.right = _out.region.getCenterX();
				}
				if (ty & 1) {
					_out.region.bottom = _out.region.getCenterY();
				} else {
					_out.region.top = _out.region.getCenterY();
				}
				_out.flagDrawWhole = sl_false;
				return sl_true;
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

	GeoLocation MapSurface::getEyeLocation()
	{
		return { m_center.N / METER_PER_DEGREE, m_center.E / METER_PER_DEGREE, m_scale * ALTITUDE_PER_METER };
	}

	void MapSurface::setEyeLocation(const GeoLocation& location)
	{
		setScale(location.altitude / ALTITUDE_PER_METER);
		setCenterLocation(location.longitude * METER_PER_DEGREE, location.latitude * METER_PER_DEGREE);
	}

	LatLon MapSurface::getLatLonFromMapLocation(const MapLocation& location)
	{
		return { location.N / METER_PER_DEGREE, location.E / METER_PER_DEGREE };
	}

	MapLocation MapSurface::getMapLocationFromLatLon(const LatLon& location)
	{
		return { location.longitude * METER_PER_DEGREE, location.latitude * METER_PER_DEGREE };
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

	MapLocation MapSurface::getMapLocationFromTileLocation(const MapTileLocationI& location)
	{
		if (location.level < m_baseLevel) {
			return { 0, 0 };
		}
		sl_uint64 n = (sl_uint64)1 << (location.level - m_baseLevel);
		sl_uint64 nE = n * m_baseTileCountE;
		sl_uint64 nN = n * m_baseTileCountN;
		MapLocation ret;
		ret.N = ((double)(location.N) / (double)nN - 0.5) * m_degreeLengthN * METER_PER_DEGREE;
		ret.E = ((double)(location.E) / (double)nE - 0.5) * m_degreeLengthE * METER_PER_DEGREE;
		return ret;
	}

	MapTileLocation MapSurface::getTileLocationFromMapLocation(sl_uint32 level, const MapLocation& location)
	{
		if (level < m_baseLevel) {
			return { level, 0, 0 };
		}
		sl_uint64 n = (sl_uint64)1 << (level - m_baseLevel);
		sl_uint64 nE = n * m_baseTileCountE;
		sl_uint64 nN = n * m_baseTileCountN;
		MapTileLocation ret;
		ret.level = level;
		ret.N = (0.5 + location.N / METER_PER_DEGREE / m_degreeLengthN) * nN;
		ret.E = (0.5 + location.E / METER_PER_DEGREE / m_degreeLengthE) * nE;
		return ret;
	}

	MapTileLocationI MapSurface::getReaderLocation(const MapTileLocationI& location)
	{
		MapTileLocationI ret = location;
		m_toReaderLocation(ret);
		return ret;
	}


	SLIB_DEFINE_OBJECT(MapViewObject, Object)

	MapViewObject::MapViewObject()
	{
		m_flagSupportGlobe = sl_false;
		m_flagSupportPlane = sl_false;
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

	void MapViewObject::draw(Canvas* canvas, const Rectangle& rect, MapViewData* data, MapPlane* plane)
	{
	}


	MapViewData::MapViewData()
	{
		m_view = sl_null;
		m_flagGlobeMode = sl_false;
		m_tileLoader = MapTileLoader::create(SLIB_FUNCTION_MEMBER(this, _onCompleteLazyLoading));
		m_eyeLocation.altitude = 10000;
		m_width = 1.0;
		m_height = 1.0;
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
		if (m_plane.isNotNull()) {
			if (!m_flagGlobeMode) {
				m_eyeLocation = m_plane->getEyeLocation();
			}
			if (m_plane != m_surface) {
				m_plane->clearCache();
			}
		}
		m_plane = plane;
		if (plane.isNotNull()) {
			_resizePlane(plane.get(), m_width, m_height);
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
		if (m_surface == surface) {
			return;
		}
		if (m_surface.isNotNull()) {
			if (m_surface != m_plane) {
				m_surface->clearCache();
			}
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

	GeoLocation MapViewData::getEyeLocation() const
	{
		MutexLocker locker(&m_lock);
		if (!m_flagGlobeMode) {
			if (m_plane.isNotNull()) {
				return m_plane->getEyeLocation();
			}
		}
		return m_eyeLocation;
	}

	void MapViewData::setEyeLocation(const GeoLocation& location, UIUpdateMode mode)
	{
		MutexLocker locker(&m_lock);
		m_eyeLocation = location;
		if (!m_flagGlobeMode) {
			if (m_plane.isNotNull()) {
				m_plane->setEyeLocation(location);
			}
		}
		invalidate(mode);
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
		if (Math::isAlmostZero(m_width - width) && Math::isAlmostZero(m_height - height)) {
			return;
		}
		m_width = width;
		m_height = height;
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
		} else {
			MapPlane* plane = m_plane.get();
			if (plane) {
				m_plane->setScale(plane->getScale() * factor);
			}
		}
		invalidate(mode);
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
		double rem = UIResource::getScreenMinimum() / 100.0;
		double height = (double)(getHeight());
		Point pt = ev->getPoint();
		Point pt2 = pt;
		sl_uint32 nTouches = 0;
		sl_bool flagDrag = sl_false;
		UIAction action = ev->getAction();
		switch (action) {
			case UIAction::LeftButtonDown:
			case UIAction::TouchBegin:
				m_ptMouseDown = pt;
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
					movePlane((pt.x - m_ptLastEvent.x) / height, (pt.y - m_ptLastEvent.y) / height);
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
