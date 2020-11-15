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

#include "slib/render/base.h"

#include "slib/render/engine.h"

namespace slib
{

	SLIB_DEFINE_OBJECT(RenderBaseObjectInstance, Object)
	
	RenderBaseObjectInstance::RenderBaseObjectInstance()
	{
		m_flagUpdated = sl_false;
	}
	
	RenderBaseObjectInstance::~RenderBaseObjectInstance()
	{
	}
	
	void RenderBaseObjectInstance::link(RenderEngine* engine, RenderBaseObject* object)
	{
		m_engine = engine;
		object->m_instance = this;
	}
	
	Ref<RenderEngine> RenderBaseObjectInstance::getEngine()
	{
		return m_engine;
	}
	
	void RenderBaseObjectInstance::onUpdate(RenderBaseObject* object)
	{
	}

	sl_bool RenderBaseObjectInstance::canUpdate()
	{
		return sl_false;
	}

	void RenderBaseObjectInstance::tryUpdate(RenderBaseObject* object)
	{
		if (canUpdate()) {
			doUpdate(object);
		}
	}

	void RenderBaseObjectInstance::doUpdate(RenderBaseObject* object)
	{
		if (m_flagUpdated) {
			m_flagUpdated = sl_false;
			onUpdate(object);
		}
	}
	
	sl_bool RenderBaseObjectInstance::isUpdated()
	{
		return m_flagUpdated;
	}
	
	
	SLIB_DEFINE_OBJECT(RenderBaseObject, Object)
	
	RenderBaseObject::RenderBaseObject()
	{
	}
	
	RenderBaseObject::~RenderBaseObject()
	{
	}
	
	Ref<RenderBaseObjectInstance> RenderBaseObject::getInstance(RenderEngine* engine)
	{
		if (engine) {
			Ref<RenderBaseObjectInstance> instance = m_instance;
			if (instance.isNotNull()) {
				Ref<RenderEngine> _engine(instance->m_engine);
				if (_engine.isNotNull()) {
					if (_engine.get() == engine) {
						return instance;
					}
				} else {
					m_instance.setNull();
				}
			}
		}
		return sl_null;
	}

	RenderObjectFlags RenderBaseObject::getFlags()
	{
		return m_flags;
	}

	void RenderBaseObject::setFlags(const RenderObjectFlags& flags)
	{
		m_flags = flags;
	}


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(RenderDepthStencilParam)

	RenderDepthStencilParam::RenderDepthStencilParam() :
		flagTestDepth(sl_true),
		flagWriteDepth(sl_true),
		depthFunction(RenderFunctionOperation::Less),
		flagStencil(sl_false),
		stencilReadMask(0xff),
		stencilWriteMask(0xff),
		stencilRef(0)
	{
	}

	SLIB_DEFINE_ROOT_OBJECT(RenderDepthStencilState)

	RenderDepthStencilState::RenderDepthStencilState()
	{
	}

	RenderDepthStencilState::~RenderDepthStencilState()
	{
	}

	Ref<RenderDepthStencilState> RenderDepthStencilState::create(const RenderDepthStencilParam& param)
	{
		Ref<RenderDepthStencilState> state = new RenderDepthStencilState;
		if (state.isNotNull()) {
			state->m_param = param;
			return state;
		}
		return sl_null;
	}

	const RenderDepthStencilParam& RenderDepthStencilState::getParam()
	{
		return m_param;
	}

	void RenderDepthStencilState::setStencilRef(sl_uint32 ref)
	{
		m_param.stencilRef = ref;
	}


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(RenderRasterizerParam)

	RenderRasterizerParam::RenderRasterizerParam() :
		flagCull(sl_true),
		flagCullCCW(sl_true),
		flagWireFrame(sl_false),
		flagMultiSample(sl_false)
	{
	}

	SLIB_DEFINE_ROOT_OBJECT(RenderRasterizerState)

	RenderRasterizerState::RenderRasterizerState()
	{
	}

	RenderRasterizerState::~RenderRasterizerState()
	{
	}

	Ref<RenderRasterizerState> RenderRasterizerState::create(const RenderRasterizerParam& param)
	{
		Ref<RenderRasterizerState> state = new RenderRasterizerState;
		if (state.isNotNull()) {
			state->m_param = param;
			return state;
		}
		return sl_null;
	}

	const RenderRasterizerParam& RenderRasterizerState::getParam()
	{
		return m_param;
	}


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(RenderBlendParam)

	RenderBlendParam::RenderBlendParam() :
		flagBlending(sl_false),
		operation(RenderBlendingOperation::Add),
		operationAlpha(RenderBlendingOperation::Add),
		blendDst(RenderBlendingFactor::OneMinusSrcAlpha),
		blendDstAlpha(RenderBlendingFactor::OneMinusSrcAlpha),
		blendSrc(RenderBlendingFactor::SrcAlpha),
		blendSrcAlpha(RenderBlendingFactor::One),
		blendConstant(Vector4::zero())
	{
	}

	SLIB_DEFINE_ROOT_OBJECT(RenderBlendState)

	RenderBlendState::RenderBlendState()
	{
	}

	RenderBlendState::~RenderBlendState()
	{
	}

	Ref<RenderBlendState> RenderBlendState::create(const RenderBlendParam& param)
	{
		Ref<RenderBlendState> state = new RenderBlendState;
		if (state.isNotNull()) {
			state->m_param = param;
			return state;
		}
		return sl_null;
	}

	const RenderBlendParam& RenderBlendState::getParam()
	{
		return m_param;
	}

	void RenderBlendState::setConstant(const Vector4& v)
	{
		m_param.blendConstant = v;
	}


	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(RenderSamplerParam)

	RenderSamplerParam::RenderSamplerParam() :
		minFilter(TextureFilterMode::Linear),
		magFilter(TextureFilterMode::Linear),
		wrapX(TextureWrapMode::Clamp),
		wrapY(TextureWrapMode::Clamp)
	{
	}

	SLIB_DEFINE_ROOT_OBJECT(RenderSamplerState)

	RenderSamplerState::RenderSamplerState()
	{
	}

	RenderSamplerState::~RenderSamplerState()
	{
	}

	Ref<RenderSamplerState> RenderSamplerState::create(const RenderSamplerParam& param)
	{
		Ref<RenderSamplerState> state = new RenderSamplerState;
		if (state.isNotNull()) {
			state->m_param = param;
			return state;
		}
		return sl_null;
	}

	const RenderSamplerParam& RenderSamplerState::getParam()
	{
		return m_param;
	}

}
