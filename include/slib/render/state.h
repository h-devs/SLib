/*
 *   Copyright (c) 2008-2020 SLIBIO <https://github.com/SLIBIO>
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

#ifndef CHECKHEADER_SLIB_RENDER_STATE
#define CHECKHEADER_SLIB_RENDER_STATE

#include "constants.h"
#include "base.h"

#include "../math/vector4.h"

namespace slib
{

	class SLIB_EXPORT RenderDepthStencilParam
	{
	public:
		sl_bool flagTestDepth;
		sl_bool flagWriteDepth;
		RenderFunctionOperation depthFunction;
		sl_bool flagStencil;
		sl_uint32 stencilReadMask;
		sl_uint32 stencilWriteMask;
		sl_uint32 stencilRef;

	public:
		RenderDepthStencilParam();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(RenderDepthStencilParam)

	};

	class SLIB_EXPORT RenderDepthStencilState : public RenderBaseObject
	{
		SLIB_DECLARE_OBJECT

	protected:
		RenderDepthStencilState();

		~RenderDepthStencilState();

	public:
		static Ref<RenderDepthStencilState> create(const RenderDepthStencilParam& param);

		static Ref<RenderDepthStencilState> create(sl_bool flagUseDepth = sl_true);

	public:
		const RenderDepthStencilParam& getParam();

		void setStencilRef(sl_uint32 ref);

	protected:
		RenderDepthStencilParam m_param;

	};


	class SLIB_EXPORT RenderRasterizerParam
	{
	public:
		sl_bool flagCull;
		sl_bool flagCullCCW;
		sl_bool flagWireFrame;
		sl_bool flagMultiSample;
		sl_int32 depthBias;
		float depthBiasClamp;
		float slopeScaledDepthBias;

	public:
		RenderRasterizerParam();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(RenderRasterizerParam)

	};

	class SLIB_EXPORT RenderRasterizerState : public RenderBaseObject
	{
		SLIB_DECLARE_OBJECT

	protected:
		RenderRasterizerState();

		~RenderRasterizerState();

	public:
		static Ref<RenderRasterizerState> create(const RenderRasterizerParam& param);

		static Ref<RenderRasterizerState> create(sl_bool flagCull = sl_true);

	public:
		const RenderRasterizerParam& getParam();

	protected:
		RenderRasterizerParam m_param;

	};


	class SLIB_EXPORT RenderBlendParam
	{
	public:
		sl_bool flagBlending;
		RenderBlendingOperation operation;
		RenderBlendingOperation operationAlpha;
		RenderBlendingFactor blendSrc;
		RenderBlendingFactor blendSrcAlpha;
		RenderBlendingFactor blendDst;
		RenderBlendingFactor blendDstAlpha;
		Vector4 blendConstant;

	public:
		RenderBlendParam();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(RenderBlendParam)

	};


	class SLIB_EXPORT RenderBlendState : public RenderBaseObject
	{
		SLIB_DECLARE_OBJECT

	protected:
		RenderBlendState();

		~RenderBlendState();

	public:
		static Ref<RenderBlendState> create(const RenderBlendParam& param);

		static Ref<RenderBlendState> create(sl_bool flagBlending = sl_false);

	public:
		const RenderBlendParam& getParam();

		void setConstant(const Vector4& v);

	protected:
		RenderBlendParam m_param;

	};


	class SLIB_EXPORT RenderSamplerParam
	{
	public:
		TextureFilterMode minFilter;
		TextureFilterMode magFilter;
		TextureWrapMode wrapX;
		TextureWrapMode wrapY;

	public:
		RenderSamplerParam();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(RenderSamplerParam)

	};

	class SLIB_EXPORT RenderSamplerState : public RenderBaseObject
	{
		SLIB_DECLARE_OBJECT

	protected:
		RenderSamplerState();

		~RenderSamplerState();

	public:
		static Ref<RenderSamplerState> create(const RenderSamplerParam& param);

		static Ref<RenderSamplerState> create(TextureWrapMode wrapX = TextureWrapMode::Clamp, TextureWrapMode wrapY = TextureWrapMode::Clamp);

	public:
		const RenderSamplerParam& getParam();

	protected:
		RenderSamplerParam m_param;

	};

}

#endif
