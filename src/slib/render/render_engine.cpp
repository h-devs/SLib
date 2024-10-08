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

#include "slib/render/engine.h"

#include "slib/render/program.h"
#include "slib/graphics/canvas.h"
#include "slib/ui/core.h"
#include "slib/math/transform3d.h"

namespace slib
{

	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(Primitive)

	Primitive::Primitive(): type(PrimitiveType::Triangle), countElements(0)
	{
	}


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(EnginePrimitive)

	EnginePrimitive::EnginePrimitive(const Primitive& primitive): Primitive(primitive)
	{
	}


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(RendererParam)

	RendererParam::RendererParam()
	{
		nRedBits = 8;
		nGreenBits = 8;
		nBlueBits = 8;
		nAlphaBits = 8;
		nAccumBits = 0;
		nDepthBits = 24;
		nStencilBits = 8;
		flagMultisample = sl_false;
	}


	SLIB_DEFINE_OBJECT(Renderer, Object)

	Renderer::Renderer()
	{
		m_flagRenderingContinuously = sl_false;
	}

	Renderer::~Renderer()
	{
	}

	sl_bool Renderer::isRenderingContinuously()
	{
		return m_flagRenderingContinuously;
	}

	void Renderer::setRenderingContinuously(sl_bool flag)
	{
		m_flagRenderingContinuously = flag;
	}

	void Renderer::initWithParam(const RendererParam& param)
	{
		m_onFrame = param.onFrame;
	}

	void Renderer::handleFrame(RenderEngine* engine)
	{
		m_onFrame(engine);
	}


	SLIB_DEFINE_OBJECT(RenderEngine, Object)

	RenderEngine::RenderEngine()
	{
		static volatile sl_int64 _id = 0;
		m_uniqueId = Base::interlockedIncrement64(&_id);

		m_viewportWidth = 0;
		m_viewportHeight = 0;

		m_nCountDrawnElementsOnLastScene = 0;
		m_nCountDrawnPrimitivesOnLastScene = 0;
	}

	RenderEngine::~RenderEngine()
	{
	}

	sl_uint64 RenderEngine::getUniqueId()
	{
		return m_uniqueId;
	}

	sl_bool RenderEngine::isShaderAvailable()
	{
		return sl_true;
	}

	sl_bool RenderEngine::isInputLayoutAvailable()
	{
		return sl_true;
	}

	sl_bool RenderEngine::beginScene()
	{
		m_nCountDrawnElementsOnLastScene = 0;
		m_nCountDrawnPrimitivesOnLastScene = 0;
		return _beginScene();
	}

	void RenderEngine::endScene()
	{
		_endScene();
	}

	void RenderEngine::setViewport(sl_uint32 x, sl_uint32 y, sl_uint32 width, sl_uint32 height)
	{
		m_viewportWidth = width;
		m_viewportHeight = height;
		_setViewport(x, y, width, height);
	}

	SLIB_DEFINE_NESTED_CLASS_DEFAULT_MEMBERS(RenderEngine, ClearParam)

	RenderEngine::ClearParam::ClearParam()
	{
		flagColor = sl_true;
		color = Color::Blue;
		flagDepth = sl_true;
		depth = 1.0f;
		flagStencil = sl_false;
		stencil = 0;
	}

	void RenderEngine::clear(const ClearParam& param)
	{
		_clear(param);
	}

	void RenderEngine::clearColor(const Color& color)
	{
		ClearParam param;
		param.flagColor = sl_true;
		param.color = color;
		param.flagDepth = sl_false;
		_clear(param);
	}

	void RenderEngine::clearColorDepth(const Color& color, float depth)
	{
		ClearParam param;
		param.flagColor = sl_true;
		param.color = color;
		param.flagDepth = sl_true;
		param.depth = depth;
		_clear(param);
	}

	void RenderEngine::clearDepth(float depth)
	{
		ClearParam param;
		param.flagColor = sl_false;
		param.flagDepth = sl_true;
		param.depth = depth;
		_clear(param);
	}

	const Ref<RenderDepthStencilState>& RenderEngine::getDepthStencilState()
	{
		return m_depthStencilState;
	}

	void RenderEngine::setDepthStencilState(const Ref<RenderDepthStencilState>& state)
	{
		if (m_depthStencilState == state) {
			return;
		}
		if (state.isNotNull()) {
			m_depthStencilState = state;
			_setDepthStencilState(state.get());
		}
	}

	const Ref<RenderRasterizerState>& RenderEngine::getRasterizerState()
	{
		return m_rasterizerState;
	}

	void RenderEngine::setRasterizerState(const Ref<RenderRasterizerState>& state)
	{
		if (m_rasterizerState == state) {
			return;
		}
		if (state.isNotNull()) {
			m_rasterizerState = state;
			_setRasterizerState(state.get());
		}
	}

	void RenderEngine::setBlendState(const Ref<RenderBlendState>& state)
	{
		if (m_blendState == state) {
			return;
		}
		if (state.isNotNull()) {
			m_blendState = state;
			_setBlendState(state.get());
		}
	}

	void RenderEngine::setSamplerState(sl_int32 samplerNo, const Ref<RenderSamplerState>& state)
	{
		if (state.isNotNull()) {
			_setSamplerState(samplerNo, state.get());
		}
	}

	sl_bool RenderEngine::beginProgram(const Ref<RenderProgram>& program, RenderProgramState** ppState)
	{
		if (program.isNotNull()) {
			Ref<RenderProgramInstance> instance = linkProgram(program);
			if (instance.isNotNull()) {
				return _beginProgram(program.get(), instance.get(), ppState);
			}
		}
		return sl_false;
	}

	void RenderEngine::endProgram()
	{
		_endProgram();
	}

	void RenderEngine::resetCurrentBuffers()
	{
		_resetCurrentBuffers();
	}

	void RenderEngine::drawPrimitive(Primitive* primitive)
	{
		if (primitive->countElements > 0 && primitive->vertexBuffer.isNotNull()) {
			EnginePrimitive ep(*primitive);
			if (primitive->vertexBuffers.isNotNull()) {
				ListElements< Ref<VertexBuffer> > list(primitive->vertexBuffers);
				for (sl_size i = 0; i < list.count; i++) {
					Ref<VertexBufferInstance> instance = linkVertexBuffer(list[i]);
					if (instance.isNull()) {
						return;
					}
					if (!(ep.vertexBufferInstances.add(Move(instance)))) {
						return;
					}
				}
			} else {
				ep.vertexBufferInstance = linkVertexBuffer(primitive->vertexBuffer);
				if (ep.vertexBufferInstance.isNull()) {
					return;
				}
			}
			if (primitive->indexBuffer.isNotNull()) {
				ep.indexBufferInstance = linkIndexBuffer(primitive->indexBuffer);
				if (ep.indexBufferInstance.isNull()) {
					return;
				}
			}
			_drawPrimitive(&ep);
			m_nCountDrawnElementsOnLastScene += primitive->countElements;
			m_nCountDrawnPrimitivesOnLastScene++;
		}
	}

	void RenderEngine::drawPrimitives(Primitive* primitives, sl_uint32 count)
	{
		for (sl_uint32 i = 0; i < count; i++) {
			drawPrimitive(primitives + i);
		}
	}

	void RenderEngine::drawPrimitive(sl_uint32 countElements, const Ref<VertexBuffer>& vb, const Ref<IndexBuffer>& ib, PrimitiveType type)
	{
		Primitive p;
		p.type = type;
		p.countElements = countElements;
		p.vertexBuffer = vb;
		p.indexBuffer = ib;
		drawPrimitive(&p);
	}

	void RenderEngine::drawPrimitive(sl_uint32 countElements, const Ref<VertexBuffer>& vb, PrimitiveType type)
	{
		Primitive p;
		p.type = type;
		p.countElements = countElements;
		p.vertexBuffer = vb;
		drawPrimitive(&p);
	}

	void RenderEngine::applyTexture(const Ref<Texture>& _texture, sl_int32 sampler)
	{
		Texture* texture = _texture.get();
		if (texture) {
			if (IsInstanceOf<EngineTexture>(texture)) {
				_applyTexture(texture, sl_null, sampler);
			} else {
				Ref<TextureInstance> instance = linkTexture(_texture, sampler);
				if (instance.isNull()) {
					_applyTexture(sl_null, sl_null, sampler);
				}
			}
		} else {
			_applyTexture(sl_null, sl_null, sampler);
		}
	}

	void RenderEngine::setInputLayout(RenderInputLayout* layout)
	{
		_setInputLayout(layout);
	}

	Ref<TextureInstance> RenderEngine::linkTexture(const Ref<Texture>& texture, sl_int32 sampler)
	{
		if (texture.isNotNull()) {
			Ref<TextureInstance> instance = texture->getInstance(this);
			if (instance.isNotNull()) {
				_applyTexture(texture, instance.get(), sampler);
				return instance;
			}
			instance = _createTextureInstance(texture.get(), sampler);
			if (instance.isNotNull()) {
				return instance;
			}
		}
		return sl_null;
	}

	Ref<VertexBufferInstance> RenderEngine::linkVertexBuffer(const Ref<VertexBuffer>& vb)
	{
		if (vb.isNotNull()) {
			Ref<VertexBufferInstance> instance = vb->getInstance(this);
			if (instance.isNotNull()) {
				return instance;
			}
			instance = _createVertexBufferInstance(vb.get());
			if (instance.isNotNull()) {
				return instance;
			}
		}
		return sl_null;
	}

	Ref<IndexBufferInstance> RenderEngine::linkIndexBuffer(const Ref<IndexBuffer>& ib)
	{
		if (ib.isNotNull()) {
			Ref<IndexBufferInstance> instance = ib->getInstance(this);
			if (instance.isNotNull()) {
				return instance;
			}
			instance = _createIndexBufferInstance(ib.get());
			if (instance.isNotNull()) {
				return instance;
			}
		}
		return sl_null;
	}

	Ref<RenderProgramInstance> RenderEngine::linkProgram(const Ref<RenderProgram>& program)
	{
		if (program.isNotNull()) {
			Ref<RenderProgramInstance> instance = program->getInstance(this);
			if (instance.isNotNull()) {
				return instance;
			}
			instance = _createProgramInstance(program.get());
			if (instance.isNotNull()) {
				return instance;
			}
		}
		return sl_null;
	}

	void RenderEngine::setLineWidth(sl_real width)
	{
		_setLineWidth(width);
	}

	void RenderEngine::drawRectangle2D()
	{
		drawPrimitive(4, getDefaultVertexBufferForDrawRectangle2D(), PrimitiveType::TriangleStrip);
	}

	namespace {

		static void MakeTransform2D(Matrix3& mat, const Rectangle& rectDst)
		{
			sl_real x = rectDst.left;
			sl_real y = rectDst.bottom;
			sl_real w = rectDst.right - rectDst.left;
			sl_real h = rectDst.top - rectDst.bottom;
			mat.m00 = w; mat.m01 = 0; mat.m02 = 0;
			mat.m10 = 0; mat.m11 = h; mat.m12 = 0;
			mat.m20 = x; mat.m21 = y; mat.m22 = 1;
		}

		static void MakeTextureTransform2D(Matrix3& mat, const Rectangle& rt)
		{
			sl_real x = rt.left;
			sl_real y = rt.top;
			sl_real w = rt.right - rt.left;
			sl_real h = rt.bottom - rt.top;
			mat.m00 = w; mat.m01 = 0; mat.m02 = 0;
			mat.m10 = 0; mat.m11 = h; mat.m12 = 0;
			mat.m20 = x; mat.m21 = y; mat.m22 = 1;
		}

	}


	void RenderEngine::drawRectangle2D(const Ref<render2d::program::Position>& program, const Matrix3& transform, const Color4F& color)
	{
		RenderProgramScope<render2d::state::Position> scope;
		if (scope.begin(this, program)) {
			scope->setTransform(transform);
			scope->setColor(color);
			drawRectangle2D();
		}
	}

	void RenderEngine::drawRectangle2D(const Matrix3& transform, const Color4F& color)
	{
		drawRectangle2D(getDefaultRenderProgramForDrawRectangle2D(), transform, color);
	}

	void RenderEngine::drawRectangle2D(const Ref<render2d::program::Position>& program, const Rectangle& rectDst, const Color4F& color)
	{
		Matrix3 transform;
		MakeTransform2D(transform, rectDst);
		drawRectangle2D(program, transform, color);
	}

	void RenderEngine::drawRectangle2D(const Rectangle& rectDst, const Color4F& color)
	{
		Matrix3 transform;
		MakeTransform2D(transform, rectDst);
		drawRectangle2D(getDefaultRenderProgramForDrawRectangle2D(), transform, color);
	}

	void RenderEngine::drawTexture2D()
	{
		drawPrimitive(4, getDefaultVertexBufferForDrawTexture2D(), PrimitiveType::TriangleStrip);
	}

	void RenderEngine::drawTexture2D(const Ref<render2d::program::PositionTexture>& program, const Matrix3& transform, const Ref<Texture>& texture, const Rectangle& rectSrc, const Color4F& color)
	{
		if (texture.isNotNull() && program.isNotNull()) {
			RenderProgramScope<render2d::state::PositionTexture> scope;
			if (scope.begin(this, program)) {
				scope->setTransform(transform);
				scope->setTexture(texture);
				Matrix3 textureTransform;
				MakeTextureTransform2D(textureTransform, rectSrc);
				scope->setTextureTransform(textureTransform);
				scope->setColor(color);
				drawTexture2D();
			}
		}
	}

	void RenderEngine::drawTexture2D(const Matrix3& transform, const Ref<Texture>& texture, const Rectangle& rectSrc, const Color4F& color)
	{
		drawTexture2D(getDefaultRenderProgramForDrawTexture2D(), transform, texture, rectSrc, color);
	}

	void RenderEngine::drawTexture2D(const Ref<render2d::program::PositionTexture>& program, const Matrix3& transform, const Ref<Texture>& texture, const Rectangle& rectSrc, sl_real alpha)
	{
		drawTexture2D(program, transform, texture, rectSrc, Vector4(1, 1, 1, alpha));
	}

	void RenderEngine::drawTexture2D(const Matrix3& transform, const Ref<Texture>& texture, const Rectangle& rectSrc, sl_real alpha)
	{
		drawTexture2D(getDefaultRenderProgramForDrawTexture2D(), transform, texture, rectSrc, Vector4(1, 1, 1, alpha));
	}

	void RenderEngine::drawTexture2D(const Ref<render2d::program::PositionTexture>& program, const Matrix3& transform, const Ref<Texture>& texture, const Color4F& color)
	{
		drawTexture2D(program, transform, texture, Rectangle(0, 0, 1, 1), color);
	}

	void RenderEngine::drawTexture2D(const Matrix3& transform, const Ref<Texture>& texture, const Color4F& color)
	{
		drawTexture2D(getDefaultRenderProgramForDrawTexture2D(), transform, texture, Rectangle(0, 0, 1, 1), color);
	}

	void RenderEngine::drawTexture2D(const Ref<render2d::program::PositionTexture>& program, const Matrix3& transform, const Ref<Texture>& texture, sl_real alpha)
	{
		drawTexture2D(program, transform, texture, Rectangle(0, 0, 1, 1), alpha);
	}

	void RenderEngine::drawTexture2D(const Matrix3& transform, const Ref<Texture>& texture, sl_real alpha)
	{
		drawTexture2D(getDefaultRenderProgramForDrawTexture2D(), transform, texture, Rectangle(0, 0, 1, 1), alpha);
	}

	void RenderEngine::drawTexture2D(const Ref<render2d::program::PositionTexture>& program, const Rectangle& rectDst, const Ref<Texture>& texture, const Rectangle& rectSrc, const Color4F& color)
	{
		if (texture.isNotNull()) {
			Matrix3 transform;
			MakeTransform2D(transform, rectDst);
			drawTexture2D(program, transform, texture, rectSrc, color);
		}
	}

	void RenderEngine::drawTexture2D(const Rectangle& rectDst, const Ref<Texture>& texture, const Rectangle& rectSrc, const Color4F& color)
	{
		if (texture.isNotNull()) {
			Matrix3 transform;
			MakeTransform2D(transform, rectDst);
			drawTexture2D(getDefaultRenderProgramForDrawTexture2D(), transform, texture, rectSrc, color);
		}
	}

	void RenderEngine::drawTexture2D(const Ref<render2d::program::PositionTexture>& program, const Rectangle& rectDst, const Ref<Texture>& texture, const Rectangle& rectSrc, sl_real alpha)
	{
		if (texture.isNotNull()) {
			Matrix3 transform;
			MakeTransform2D(transform, rectDst);
			drawTexture2D(program, transform, texture, rectSrc, Vector4(1, 1, 1, alpha));
		}
	}

	void RenderEngine::drawTexture2D(const Rectangle& rectDst, const Ref<Texture>& texture, const Rectangle& rectSrc, sl_real alpha)
	{
		if (texture.isNotNull()) {
			Matrix3 transform;
			MakeTransform2D(transform, rectDst);
			drawTexture2D(getDefaultRenderProgramForDrawTexture2D(), transform, texture, rectSrc, Vector4(1, 1, 1, alpha));
		}
	}

	void RenderEngine::drawTexture2D(const Ref<render2d::program::PositionTexture>& program, const Rectangle& rectDst, const Ref<Texture>& texture, const Color4F& color)
	{
		drawTexture2D(program, rectDst, texture, Rectangle(0, 0, 1, 1), color);
	}

	void RenderEngine::drawTexture2D(const Rectangle& rectDst, const Ref<Texture>& texture, const Color4F& color)
	{
		drawTexture2D(rectDst, texture, Rectangle(0, 0, 1, 1), color);
	}

	void RenderEngine::drawTexture2D(const Ref<render2d::program::PositionTexture>& program, const Rectangle& rectDst, const Ref<Texture>& texture, sl_real alpha)
	{
		drawTexture2D(program, rectDst, texture, Rectangle(0, 0, 1, 1), alpha);
	}

	void RenderEngine::drawTexture2D(const Rectangle& rectDst, const Ref<Texture>& texture, sl_real alpha)
	{
		drawTexture2D(rectDst, texture, Rectangle(0, 0, 1, 1), alpha);
	}

	const Ref<VertexBuffer>& RenderEngine::getDefaultVertexBufferForDrawRectangle2D()
	{
		static render2d::vertex::Position v[] = {
			{ { 0, 0 } }
			, { { 1, 0 } }
			, { { 0, 1 } }
			, { { 1, 1 } }
		};
		Ref<VertexBuffer>& ret = m_defaultVertexBufferForDrawRectangle2D;
		if (ret.isNull()) {
			ret = VertexBuffer::create(v, sizeof(v));
		}
		return ret;
	}

	const Ref<render2d::program::Position>& RenderEngine::getDefaultRenderProgramForDrawRectangle2D()
	{
		Ref<render2d::program::Position>& ret = m_defaultRenderProgramForDrawRectangle2D;
		if (ret.isNull()) {
			ret = new render2d::program::Position;
		}
		return ret;
	}

	const Ref<VertexBuffer>& RenderEngine::getDefaultVertexBufferForDrawTexture2D()
	{
		static render2d::vertex::PositionTexture v[] = {
			{ { 0, 0 }, { 0, 0 } }
			, { { 1, 0 }, { 1, 0 } }
			, { { 0, 1 }, { 0, 1 } }
			, { { 1, 1 }, { 1, 1 } }
		};
		Ref<VertexBuffer>& ret = m_defaultVertexBufferForDrawTexture2D;
		if (ret.isNull()) {
			ret = VertexBuffer::create(v, sizeof(v));
		}
		return ret;
	}

	const Ref<render2d::program::PositionTexture>& RenderEngine::getDefaultRenderProgramForDrawTexture2D()
	{
		Ref<render2d::program::PositionTexture>& ret = m_defaultRenderProgramForDrawTexture2D;
		if (ret.isNull()) {
			ret = new render2d::program::PositionTexture;
		}
		return ret;
	}

	void RenderEngine::drawLines(const Ref<render2d::program::Position>& program, LineSegment* lines, sl_uint32 n, const Color4F& color)
	{
		if (program.isNull()) {
			return;
		}
		if (n) {
			Ref<VertexBuffer> vb = VertexBuffer::create(lines, sizeof(LineSegment)*n);
			if (vb.isNotNull()) {
				RenderProgramScope<render2d::state::Position> scope;
				if (scope.begin(this, program)) {
					scope->setTransform(Matrix3::identity());
					scope->setColor(color);
					drawPrimitive(n * 2, vb, PrimitiveType::Line);
				}
			}
		}
	}

	void RenderEngine::drawLines(LineSegment* lines, sl_uint32 n, const Color4F& color)
	{
		drawLines(getDefaultRenderProgramForDrawLine2D(), lines, n, color);
	}

	const Ref<render2d::program::Position>& RenderEngine::getDefaultRenderProgramForDrawLine2D()
	{
		Ref<render2d::program::Position>& ret = m_defaultRenderProgramForDrawLine2D;
		if (ret.isNull()) {
			ret = new render2d::program::Position;
		}
		return ret;
	}

	void RenderEngine::drawLines(const Ref<render3d::program::Position>& program, Line3* lines, sl_uint32 n, const Color4F& color)
	{
		if (n) {
			Ref<VertexBuffer> vb = VertexBuffer::create(lines, sizeof(Line3)*n);
			if (vb.isNotNull()) {
				RenderProgramScope<render3d::state::Position> scope;
				if (scope.begin(this, program)) {
					scope->setTransform(Matrix4::identity());
					scope->setColor(color);
					drawPrimitive(n * 2, vb, PrimitiveType::Line);
				}
			}
		}
	}

	void RenderEngine::drawLines(Line3* lines, sl_uint32 n, const Color4F& color)
	{
		drawLines(getDefaultRenderProgramForDrawLine3D(), lines, n, color);
	}

	const Ref<render3d::program::Position>& RenderEngine::getDefaultRenderProgramForDrawLine3D()
	{
		Ref<render3d::program::Position>& ret = m_defaultRenderProgramForDrawLine3D;
		if (ret.isNull()) {
			ret = new render3d::program::Position;
		}
		return ret;
	}

#define DEBUG_WIDTH 512
#define DEBUG_HEIGHT 30

	void RenderEngine::drawDebugText()
	{
		Time now = Time::now();
		if (m_timeLastDebugText.isZero()) {
			m_timeLastDebugText = now;
			return;
		}

		Ref<Texture> texture = m_textureDebug;
		if (texture.isNull()) {
			texture = Texture::create(Bitmap::create(DEBUG_WIDTH, DEBUG_HEIGHT));
			if (texture.isNull()) {
				return;
			}
			m_textureDebug = texture;
		}
		Ref<Bitmap> bitmap = texture->getSource();
		if (bitmap.isNull()) {
			return;
		}

		Ref<Font> font = m_fontDebug;
		if (font.isNull()) {
			font = Font::create("Arial", 20);
			if (font.isNull()) {
				return;
			}
		}

		{
			Ref<RenderDepthStencilState> state = m_stateDepthStencilForDrawDebug;
			if (state.isNull()) {
				RenderDepthStencilParam param;
				param.flagTestDepth = sl_false;
				state = RenderDepthStencilState::create(param);
				m_stateDepthStencilForDrawDebug = state;
			}
			setDepthStencilState(state);
		}
		{
			Ref<RenderSamplerState> state = m_stateSamplerForDrawDebug;
			if (state.isNull()) {
				RenderSamplerParam param;
				state = RenderSamplerState::create(param);
				m_stateSamplerForDrawDebug = state;
			}
			setSamplerState(0, state);
		}

		String text;
		text = "FPS:";
		double duration = (now - m_timeLastDebugText).getMillisecondCountF();
		if (duration > 1) {
			text += String::fromDouble(1000.0 / duration, 1, sl_true);
		} else {
			text += "Inf";
		}
		m_timeLastDebugText = now;

		text += " Vertices: ";
		text += String::fromUint32(m_nCountDrawnElementsOnLastScene);
		text += " Primitives: ";
		text += String::fromUint32(m_nCountDrawnPrimitivesOnLastScene);
		Size size = Size::zero();
		bitmap->resetPixels(Color(0, 0, 0, 150));
		{
			Ref<Canvas> canvas = bitmap->getCanvas();
			if (canvas.isNotNull()) {
				size = canvas->measureText(font, text);
				size.x += 5;
				canvas->drawText(text, 0, 3, font, Color::Red);
			}
		}
		texture->update(0, 0, (sl_uint32)(size.x) + 1, DEBUG_HEIGHT);
		drawTexture2D(
					  screenToViewport(0, 0, size.x, DEBUG_HEIGHT)
					  , texture
					  , Rectangle(0, 0, size.x / DEBUG_WIDTH, 1));
	}

	Point RenderEngine::screenToViewport(const Point& ptViewport)
	{
		return Transform3::convertScreenToViewport(ptViewport, (sl_real)m_viewportWidth, (sl_real)m_viewportHeight);
	}

	Point RenderEngine::screenToViewport(sl_real x, sl_real y)
	{
		return Transform3::convertScreenToViewport(Point(x, y), (sl_real)m_viewportWidth, (sl_real)m_viewportHeight);
	}

	Point RenderEngine::viewportToScreen(const Point& ptScreen)
	{
		return Transform3::convertViewportToScreen(ptScreen, (sl_real)m_viewportWidth, (sl_real)m_viewportHeight);
	}

	Point RenderEngine::viewportToScreen(sl_real x, sl_real y)
	{
		return Transform3::convertViewportToScreen(Point(x, y), (sl_real)m_viewportWidth, (sl_real)m_viewportHeight);
	}

	Rectangle RenderEngine::screenToViewport(const Rectangle& rc)
	{
		return Transform3::convertScreenToViewport(rc, (sl_real)m_viewportWidth, (sl_real)m_viewportHeight);
	}

	Rectangle RenderEngine::screenToViewport(sl_real x, sl_real y, sl_real width, sl_real height)
	{
		return Transform3::convertScreenToViewport(Rectangle(x, y, x + width, y + height), (sl_real)m_viewportWidth, (sl_real)m_viewportHeight);
	}

	Rectangle RenderEngine::viewportToScreen(const Rectangle& rc)
	{
		return Transform3::convertViewportToScreen(rc, (sl_real)m_viewportWidth, (sl_real)m_viewportHeight);
	}

	Rectangle RenderEngine::viewportToScreen(sl_real x, sl_real y, sl_real width, sl_real height)
	{
		return Transform3::convertViewportToScreen(Rectangle(x, y, x + width, y + height), (sl_real)m_viewportWidth, (sl_real)m_viewportHeight);
	}

	sl_uint32 RenderEngine::getViewportWidth()
	{
		return m_viewportWidth;
	}

	sl_uint32 RenderEngine::getViewportHeight()
	{
		return m_viewportHeight;
	}

	sl_uint32 RenderEngine::getCountOfDrawnElementsOnLastScene()
	{
		return m_nCountDrawnElementsOnLastScene;
	}

	sl_uint32 RenderEngine::getCountOfDrawnPrimitivesOnLastScene()
	{
		return m_nCountDrawnPrimitivesOnLastScene;
	}

}
