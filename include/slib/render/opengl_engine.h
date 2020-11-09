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

#ifndef CHECKHEADER_SLIB_RENDER_OPENGL_ENGINE
#define CHECKHEADER_SLIB_RENDER_OPENGL_ENGINE

#include "definition.h"

#include "engine.h"

namespace slib
{

	class SLIB_EXPORT GLRenderEngine : public RenderEngine
	{
		SLIB_DECLARE_OBJECT
		
	protected:
		GLRenderEngine();
		
		~GLRenderEngine();
		
	public:
		virtual Ref<Texture> createTextureFromName(sl_uint32 target, sl_uint32 name, sl_bool flagDeleteOnRelease = sl_true) = 0;
		
		
		virtual sl_int32 getAttributeLocation(sl_uint32 program, const char* name) = 0;
		
		sl_int32 getAttributeLocation(sl_uint32 program, const String& name);
		
		virtual void setVertexFloatArrayAttributePtr(sl_int32 attributeLocation, const void* data, sl_uint32 countComponents, sl_uint32 strideBytes = 0, sl_bool flagDoNormalize = sl_false) = 0;
		
		virtual void setVertexFloatArrayAttribute(sl_int32 attributeLocation, sl_size offsetValuesOnBuffer, sl_uint32 countComponents, sl_uint32 strideBytes = 0, sl_bool flagDoNormalize = sl_false) = 0;
		
		virtual void setVertexInt8ArrayAttributePtr(sl_int32 attributeLocation, const void* data, sl_uint32 countComponents, sl_uint32 strideBytes = 0, sl_bool flagDoNormalize = sl_false) = 0;
		
		virtual void setVertexInt8ArrayAttribute(sl_int32 attributeLocation, sl_size offsetValuesOnBuffer, sl_uint32 countComponents, sl_uint32 strideBytes = 0, sl_bool flagDoNormalize = sl_false) = 0;
		
		virtual void setVertexUint8ArrayAttributePtr(sl_int32 attributeLocation, const void* data, sl_uint32 countComponents, sl_uint32 strideBytes = 0, sl_bool flagDoNormalize = sl_false) = 0;
		
		virtual void setVertexUint8ArrayAttribute(sl_int32 attributeLocation, sl_size offsetValuesOnBuffer, sl_uint32 countComponents, sl_uint32 strideBytes = 0, sl_bool flagDoNormalize = sl_false) = 0;
		
		virtual void setVertexInt16ArrayAttributePtr(sl_int32 attributeLocation, const void* data, sl_uint32 countComponents, sl_uint32 strideBytes = 0, sl_bool flagDoNormalize = sl_false) = 0;
		
		virtual void setVertexInt16ArrayAttribute(sl_int32 attributeLocation, sl_size offsetValuesOnBuffer, sl_uint32 countComponents, sl_uint32 strideBytes = 0, sl_bool flagDoNormalize = sl_false) = 0;
		
		virtual void setVertexUint16ArrayAttributePtr(sl_int32 attributeLocation, const void* data, sl_uint32 countComponents, sl_uint32 strideBytes = 0, sl_bool flagDoNormalize = sl_false) = 0;
		
		virtual void setVertexUint16ArrayAttribute(sl_int32 attributeLocation, sl_size offsetValuesOnBuffer, sl_uint32 countComponents, sl_uint32 strideBytes = 0, sl_bool flagDoNormalize = sl_false) = 0;
		
		virtual void setVertexInt32ArrayAttributePtr(sl_int32 attributeLocation, const void* data, sl_uint32 countComponents, sl_uint32 strideBytes = 0, sl_bool flagDoNormalize = sl_false) = 0;
		
		virtual void setVertexInt32ArrayAttribute(sl_int32 attributeLocation, sl_size offsetValuesOnBuffer, sl_uint32 countComponents, sl_uint32 strideBytes = 0, sl_bool flagDoNormalize = sl_false) = 0;
		
		virtual void setVertexUint32ArrayAttributePtr(sl_int32 attributeLocation, const void* data, sl_uint32 countComponents, sl_uint32 strideBytes = 0, sl_bool flagDoNormalize = sl_false) = 0;
		
		virtual void setVertexUint32ArrayAttribute(sl_int32 attributeLocation, sl_size offsetValuesOnBuffer, sl_uint32 countComponents, sl_uint32 strideBytes = 0, sl_bool flagDoNormalize = sl_false) = 0;
		
		virtual void disableVertexArrayAttribute(sl_int32 attributeLocation) = 0;
		
		
		virtual sl_int32 getUniformLocation(sl_uint32 program, const char* name) = 0;
		
		sl_int32 getUniformLocation(sl_uint32 program, const String& name);
		
		virtual void setUniformFloatValue(sl_int32 uniformLocation, float value) = 0;
		
		virtual void setUniformFloatArray(sl_int32 uniformLocation, const void* values, sl_uint32 count) = 0;
		
		virtual void setUniformIntValue(sl_int32 uniformLocation, sl_int32 value) = 0;
		
		virtual void setUniformIntArray(sl_int32 uniformLocation, const void* values, sl_uint32 count) = 0;
		
		virtual void setUniformFloat2Value(sl_int32 uniformLocation, float v1, float v2) = 0;
		
		virtual void setUniformFloat2Value(sl_int32 uniformLocation, const Vector2& value) = 0;
		
		virtual void setUniformFloat2Array(sl_int32 uniformLocation, const void* values, sl_uint32 count) = 0;
		
		virtual void setUniformInt2Value(sl_int32 uniformLocation, sl_int32 v1, sl_int32 v2) = 0;
		
		virtual void setUniformInt2Array(sl_int32 uniformLocation, const void* values, sl_uint32 count) = 0;
		
		virtual void setUniformFloat3Value(sl_int32 uniformLocation, float v1, float v2, float v3) = 0;
		
		virtual void setUniformFloat3Value(sl_int32 uniformLocation, const Vector3& value) = 0;
		
		virtual void setUniformFloat3Array(sl_int32 uniformLocation, const void* values, sl_uint32 count) = 0;
		
		virtual void setUniformInt3Value(sl_int32 uniformLocation, sl_int32 v1, sl_int32 v2, sl_int32 v3) = 0;
		
		virtual void setUniformInt3Array(sl_int32 uniformLocation, const void* values, sl_uint32 count) = 0;
		
		virtual void setUniformFloat4Value(sl_int32 uniformLocation, float v1, float v2, float v3, float v4) = 0;
		
		virtual void setUniformFloat4Value(sl_int32 uniformLocation, const Vector4& value) = 0;
		
		virtual void setUniformFloat4Array(sl_int32 uniformLocation, const void* values, sl_uint32 count) = 0;
		
		virtual void setUniformInt4Value(sl_int32 uniformLocation, sl_int32 v1, sl_int32 v2, sl_int32 v3, sl_int32 v4) = 0;
		
		virtual void setUniformInt4Array(sl_int32 uniformLocation, const void* values, sl_uint32 count) = 0;
		
		virtual void setUniformMatrix2Value(sl_int32 uniformLocation, const Matrix2& value) = 0;
		
		virtual void setUniformMatrix2Array(sl_int32 uniformLocation, const void* values, sl_uint32 count) = 0;
		
		virtual void setUniformMatrix3Value(sl_int32 uniformLocation, const Matrix3& value) = 0;
		
		virtual void setUniformMatrix3Array(sl_int32 uniformLocation, const void* values, sl_uint32 count) = 0;
		
		virtual void setUniformMatrix4Value(sl_int32 uniformLocation, const Matrix4& value) = 0;
		
		virtual void setUniformMatrix4Array(sl_int32 uniformLocation, const void* values, sl_uint32 count) = 0;
		
		virtual void setUniformTextureSampler(sl_int32 uniformLocation, sl_uint32 samplerNo) = 0;
		
		virtual void setUniformTextureSamplerArray(sl_int32 uniformLocation, const sl_reg* values, sl_uint32 count) = 0;
		
	};

}

#endif
