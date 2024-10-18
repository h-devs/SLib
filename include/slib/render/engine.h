/*
 *   Copyright (c) 2008-2018 SLIBIO <https://github.com/SLIBIO>
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

#ifndef CHECKHEADER_SLIB_RENDER_ENGINE
#define CHECKHEADER_SLIB_RENDER_ENGINE

#include "constants.h"
#include "base.h"
#include "state.h"
#include "buffer.h"
#include "texture.h"
#include "program.h"

#include "../core/time.h"
#include "../core/function.h"
#include "../graphics/color.h"
#include "../graphics/font.h"
#include "../math/line_segment.h"
#include "../math/line3.h"

namespace slib
{

	enum class PrimitiveType
	{
		Triangle = 0,
		TriangleStrip = 1,
		TriangleFan = 2,
		Line = 3,
		LineStrip = 4,
		LineLoop = 5,
		Point = 6
	};

	class SLIB_EXPORT Primitive
	{
	public:
		PrimitiveType type;
		sl_uint32 elementCount;
		Ref<VertexBuffer> vertexBuffer;
		Ref<IndexBuffer> indexBuffer;
		ListParam< Ref<VertexBuffer> > vertexBuffers;

	public:
		Primitive();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(Primitive)

	};

	class SLIB_EXPORT EnginePrimitive : public Primitive
	{
	public:
		Ref<VertexBufferInstance> vertexBufferInstance;
		Ref<IndexBufferInstance> indexBufferInstance;
		ListParam< Ref<VertexBufferInstance> > vertexBufferInstances;

	public:
		EnginePrimitive(const Primitive& primitive);

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(EnginePrimitive)

	};

	class SLIB_EXPORT RendererParam
	{
	public:
		sl_int32 nRedBits;
		sl_int32 nGreenBits;
		sl_int32 nBlueBits;
		sl_int32 nAlphaBits;
		sl_int32 nAccumBits;
		sl_int32 nDepthBits;
		sl_int32 nStencilBits;
		sl_bool flagMultisample;

		Function<void(RenderEngine*)> onFrame;

	public:
		RendererParam();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(RendererParam)

	};

	class RenderEngine;

	class SLIB_EXPORT Renderer : public Object
	{
		SLIB_DECLARE_OBJECT

	protected:
		Renderer();

		~Renderer();

	public:
		virtual void release() = 0;

		virtual void requestRender() = 0;

	public:
		sl_bool isRenderingContinuously();

		void setRenderingContinuously(sl_bool flag);

	protected:
		void initWithParam(const RendererParam& param);

		void handleFrame(RenderEngine* engine);

	protected:
		sl_bool m_flagRenderingContinuously;
		Function<void(RenderEngine*)> m_onFrame;

	};


	enum class RenderEngineType
	{
		Any = 0,

		GL = 0x01010000,
		OpenGL = 0x01010100,
		OpenGL_ES = 0x01010200,

		D3D = 0x02010000,
		D3D8 = 0x02010800,
		D3D9 = 0x02010900,
		D3D10 = 0x02010A00,
		D3D10_1 = 0x02010A01,
		D3D11 = 0x02010B00,

		MASK_GL = 0x0fff0000,
		MASK_OpenGL = 0x0fffff00,
		MASK_OpenGL_ES = 0x0fffff00,

		MASK_D3D = 0x0fff0000,
		MASK_D3D8 = 0x0fffff00,
		MASK_D3D9 = 0x0fffff00,
		MASK_D3D10 = 0x0fffff00,
		MASK_D3D10_1 = 0x0fffffff,
		MASK_D3D11 = 0x0fffff00
	};

#define SLIB_RENDER_CHECK_ENGINE_TYPE(v, TYPE) ((((sl_uint32)(v)) & ((sl_uint32)(RenderEngineType::MASK_##TYPE))) == (sl_uint32)(RenderEngineType::TYPE))

	// Not Thread-Safe
	class SLIB_EXPORT RenderEngine : public Object
	{
		SLIB_DECLARE_OBJECT

	protected:
		RenderEngine();

		~RenderEngine();

	public:
		sl_uint64 getUniqueId();

		virtual RenderEngineType getEngineType() = 0;

		virtual sl_bool isShaderAvailable();

		virtual sl_bool isInputLayoutAvailable();

		sl_bool beginScene();

		void endScene();

		void setViewport(sl_uint32 x, sl_uint32 y, sl_uint32 width, sl_uint32 height);

		class ClearParam
		{
		public:
			sl_bool flagColor;
			Color color;
			sl_bool flagDepth;
			float depth;
			sl_bool flagStencil;
			sl_uint32 stencil;

		public:
			ClearParam();
			SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(ClearParam)
		};

		void clear(const ClearParam& param);

		void clearColor(const Color& color);

		void clearColorDepth(const Color& color, float depth = 1.0f);

		void clearDepth(float depth = 1.0f);

		const Ref<RenderDepthStencilState>& getDepthStencilState();

		void setDepthStencilState(const Ref<RenderDepthStencilState>& state);

		const Ref<RenderRasterizerState>& getRasterizerState();

		void setRasterizerState(const Ref<RenderRasterizerState>& state);

		const Ref<RenderBlendState>& getBlendState();

		void setBlendState(const Ref<RenderBlendState>& state);

		void setSamplerState(sl_int32 samplerNo, const Ref<RenderSamplerState>& state);

		sl_bool beginProgram(const Ref<RenderProgram>& program, RenderProgramState** ppState = sl_null /* RenderProgramState** */);

		void endProgram();

		void resetCurrentBuffers();

		void drawPrimitive(Primitive* primitive);

		void drawPrimitives(Primitive* primitives, sl_uint32 count);

		void drawPrimitive(sl_uint32 elementCount, const Ref<VertexBuffer>& vb, const Ref<IndexBuffer>& ib, PrimitiveType type = PrimitiveType::Triangle);

		void drawPrimitive(sl_uint32 elementCount, const Ref<VertexBuffer>& vb, PrimitiveType type = PrimitiveType::Triangle);

		void applyTexture(const Ref<Texture>& texture, sl_int32 sampler);

		void setInputLayout(RenderInputLayout* layout);

		Ref<TextureInstance> linkTexture(const Ref<Texture>& texture, sl_int32 sampler);

		Ref<VertexBufferInstance> linkVertexBuffer(const Ref<VertexBuffer>& vb);

		Ref<IndexBufferInstance> linkIndexBuffer(const Ref<IndexBuffer>& ib);

		Ref<RenderProgramInstance> linkProgram(const Ref<RenderProgram>& program);

		void setLineWidth(sl_real width);

		// Position(0, 0, 1, 1)
		void drawRectangle2D();

		// `program`: inherited render2d::program::Position
		void drawRectangle2D(const Ref<RenderProgram>& program, const Matrix3& transform, const Color4F& color);

		void drawRectangle2D(const Matrix3& transform, const Color4F& color);

		// `program`: inherited render2d::program::Position
		void drawRectangle2D(const Ref<RenderProgram>& program, const Rectangle& rectDst, const Color4F& color);

		void drawRectangle2D(const Rectangle& rectDst, const Color4F& color);

		// Position(0, 0, 1, 1) TexCoord(0, 0, 1, 1)
		void drawTexture2D();

		// `program`: inherited render2d::program::PositionTexture
		void drawTexture2D(const Ref<RenderProgram>& program, const Matrix3& transform, const Ref<Texture>& texture, const Rectangle& rectSrc, const Color4F& color);

		void drawTexture2D(const Matrix3& transform, const Ref<Texture>& texture, const Rectangle& rectSrc, const Color4F& color);

		// `program`: inherited render2d::program::PositionTexture
		void drawTexture2D(const Ref<RenderProgram>& program, const Matrix3& transform, const Ref<Texture>& texture, const Rectangle& rectSrc, sl_real alpha = 1);

		void drawTexture2D(const Matrix3& transform, const Ref<Texture>& texture, const Rectangle& rectSrc, sl_real alpha = 1);

		// `program`: inherited render2d::program::PositionTexture
		void drawTexture2D(const Ref<RenderProgram>& program, const Matrix3& transform, const Ref<Texture>& texture, const Color4F& color);

		void drawTexture2D(const Matrix3& transform, const Ref<Texture>& texture, const Color4F& color);

		// `program`: inherited render2d::program::PositionTexture
		void drawTexture2D(const Ref<RenderProgram>& program, const Matrix3& transform, const Ref<Texture>& texture, sl_real alpha = 1);

		void drawTexture2D(const Matrix3& transform, const Ref<Texture>& texture, sl_real alpha = 1);

		// `program`: inherited render2d::program::PositionTexture
		void drawTexture2D(const Ref<RenderProgram>& program, const Rectangle& rectDst, const Ref<Texture>& texture, const Rectangle& rectSrc, const Color4F& color);

		void drawTexture2D(const Rectangle& rectDst, const Ref<Texture>& texture, const Rectangle& rectSrc, const Color4F& color);

		// `program`: inherited render2d::program::PositionTexture
		void drawTexture2D(const Ref<RenderProgram>& program, const Rectangle& rectDst, const Ref<Texture>& texture, const Rectangle& rectSrc, sl_real alpha = 1);

		void drawTexture2D(const Rectangle& rectDst, const Ref<Texture>& texture, const Rectangle& rectSrc, sl_real alpha = 1);

		// `program`: inherited render2d::program::PositionTexture
		void drawTexture2D(const Ref<RenderProgram>& program, const Rectangle& rectDst, const Ref<Texture>& texture, const Color4F& color);

		void drawTexture2D(const Rectangle& rectDst, const Ref<Texture>& texture, const Color4F& color);

		// `program`: inherited render2d::program::PositionTexture
		void drawTexture2D(const Ref<RenderProgram>& program, const Rectangle& rectDst, const Ref<Texture>& texture, sl_real alpha = 1);

		void drawTexture2D(const Rectangle& rectDst, const Ref<Texture>& texture, sl_real alpha = 1);

		// `program`: inherited render2d::program::Position
		void drawLines(const Ref<RenderProgram>& program, LineSegment* lines, sl_uint32 n, const Color4F& color);

		void drawLines(LineSegment* lines, sl_uint32 n, const Color4F& color);

		// `program`: inherited render3d::program::Position
		void drawLines(const Ref<RenderProgram>& program, Line3* lines, sl_uint32 n, const Color4F& color);

		void drawLines(Line3* lines, sl_uint32 n, const Color4F& color);

		void drawText(const Matrix3& transform, const StringParam& text, const Ref<FontAtlas>& atlas, const Color4F& color);

		void drawText(sl_real x, sl_real y, sl_real scaleX, sl_real scaleY, const StringParam& text, const Ref<FontAtlas>& atlas, const Color4F& color = Color4F(1.0f, 1.0f, 1.0f, 1.0f), const Alignment& align = Alignment::BottomLeft);

		void drawDebugText();

		Point screenToViewport(const Point& ptViewport);

		Point screenToViewport(sl_real x, sl_real y);

		Point viewportToScreen(const Point& ptScreen);

		Point viewportToScreen(sl_real x, sl_real y);

		Rectangle screenToViewport(const Rectangle& rc);

		Rectangle screenToViewport(sl_real x, sl_real y, sl_real width, sl_real height);

		Rectangle viewportToScreen(const Rectangle& rc);

		Rectangle viewportToScreen(sl_real x, sl_real y, sl_real width, sl_real height);

		sl_uint32 getViewportWidth();

		sl_uint32 getViewportHeight();

		sl_uint32 getCountOfDrawnElementsOnLastScene();

		sl_uint32 getCountOfDrawnPrimitivesOnLastScene();

		const Ref<VertexBuffer>& getDefaultVertexBufferForDrawRectangle2D();

		const Ref<RenderProgram>& getDefaultRenderProgramForDrawRectangle2D();

		const Ref<VertexBuffer>& getDefaultVertexBufferForDrawTexture2D();

		const Ref<RenderProgram>& getDefaultRenderProgramForDrawTexture2D();

		const Ref<RenderProgram>& getDefaultRenderProgramForDrawLine2D();

		const Ref<RenderProgram>& getDefaultRenderProgramForDrawLine3D();

	protected:
		virtual Ref<RenderProgramInstance> _createProgramInstance(RenderProgram* program) = 0;

		virtual Ref<VertexBufferInstance> _createVertexBufferInstance(VertexBuffer* buffer) = 0;

		virtual Ref<IndexBufferInstance> _createIndexBufferInstance(IndexBuffer* buffer) = 0;

		virtual Ref<TextureInstance> _createTextureInstance(Texture* texture, sl_int32 sampler) = 0;

		virtual sl_bool _beginScene() = 0;

		virtual void _endScene() = 0;

		virtual void _setViewport(sl_uint32 x, sl_uint32 y, sl_uint32 width, sl_uint32 height) = 0;

		virtual void _clear(const ClearParam& param) = 0;

		virtual void _setDepthStencilState(RenderDepthStencilState* state) = 0;

		virtual void _setRasterizerState(RenderRasterizerState* state) = 0;

		virtual void _setBlendState(RenderBlendState* state) = 0;

		virtual void _setSamplerState(sl_int32 samplerNo, RenderSamplerState* state) = 0;

		virtual sl_bool _beginProgram(RenderProgram* program, RenderProgramInstance* instance, RenderProgramState** ppState) = 0;

		virtual void _endProgram() = 0;

		virtual void _resetCurrentBuffers() = 0;

		virtual void _drawPrimitive(EnginePrimitive* primitive) = 0;

		virtual void _applyTexture(Texture* texture, TextureInstance* instance, sl_int32 sampler) = 0;

		virtual void _setInputLayout(RenderInputLayout* layout) = 0;

		virtual void _setLineWidth(sl_real width) = 0;

	protected:
		sl_uint64 m_uniqueId;

		sl_uint32 m_viewportWidth;
		sl_uint32 m_viewportHeight;

		Ref<RenderDepthStencilState> m_depthStencilState;
		Ref<RenderRasterizerState> m_rasterizerState;
		Ref<RenderBlendState> m_blendState;

		Ref<CRef> m_canvasContext;

		// debug
		sl_uint32 m_nCountDrawnElementsOnLastScene;
		sl_uint32 m_nCountDrawnPrimitivesOnLastScene;
		Time m_timeLastDebugText;
		Ref<Texture> m_textureDebug;
		Ref<Font> m_fontDebug;

		Ref<VertexBuffer> m_defaultVertexBufferForDrawRectangle2D;
		Ref<RenderProgram> m_defaultRenderProgramForDrawRectangle2D;

		Ref<VertexBuffer> m_defaultVertexBufferForDrawTexture2D;
		Ref<RenderProgram> m_defaultRenderProgramForDrawTexture2D;

		Ref<RenderProgram> m_defaultRenderProgramForDrawLine2D;
		Ref<RenderProgram> m_defaultRenderProgramForDrawLine3D;

		Ref<RenderDepthStencilState> m_stateDepthStencilForDrawDebug;
		Ref<RenderSamplerState> m_stateSamplerForDrawDebug;

	};

	template <class StateType>
	class SLIB_EXPORT RenderProgramScope
	{
	public:
		RenderProgramScope(): m_engine(sl_null), m_program(sl_null), m_state(sl_null)
		{};

		~RenderProgramScope()
		{
			end();
		}

	public:
		sl_bool begin(RenderEngine* engine, const Ref<RenderProgram>& program)
		{
			if (program.isNotNull()) {
				if (engine->beginProgram(program, reinterpret_cast<RenderProgramState**>(&m_state))) {
					m_engine = engine;
					m_program = program;
					return sl_true;
				}
			}
			return sl_false;
		}

		sl_bool begin(const Ref<RenderEngine>& engine, const Ref<RenderProgram>& program)
		{
			return begin(engine.get(), program);
		}

		void end()
		{
			if (m_engine) {
				m_engine->endProgram();
				m_engine = sl_null;
			}
		}

		StateType* operator->()
		{
			return m_state;
		}

		StateType* getState()
		{
			return m_state;
		}

	public:
		RenderEngine* m_engine;
		Ref<RenderProgram> m_program;
		StateType* m_state;

	};

}

#endif
