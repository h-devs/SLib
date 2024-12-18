/*
 *   Copyright (c) 2008-2022 SLIBIO <https://github.com/SLIBIO>
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

#ifndef CHECKHEADER_SLIB_RENDER_OPENGL_IMPL
#define CHECKHEADER_SLIB_RENDER_OPENGL_IMPL

#include "slib/render/definition.h"

#include "slib/core/endian.h"
#include "slib/core/log.h"
#include "slib/core/thread.h"
#include "slib/core/scoped_buffer.h"
#include "slib/graphics/image.h"

#define STACK_BUFFER_COUNT 128
#define STACK_IMAGE_SIZE 16384

#ifdef SLIB_PLATFORM_IS_TIZEN
#include <Elementary_GL_Helpers.h>
ELEMENTARY_GLVIEW_GLOBAL_DEFINE()
#endif

#ifdef SLIB_PLATFORM_IS_WIN32
#define NEED_CHECK_ENTRY
#endif

namespace slib
{

	void GL_BASE::setViewport(sl_uint32 x, sl_uint32 y, sl_uint32 width, sl_uint32 height)
	{
		GL_ENTRY(glViewport)(x, y, width, height);
	}

	void GL_BASE::clear(const RenderEngine::ClearParam& param)
	{
		GLbitfield mask = 0;
		if (param.flagColor) {
			GL_ENTRY(glClearColor)(param.color.getRedF(), param.color.getGreenF(), param.color.getBlueF(), param.color.getAlphaF());
			mask |= GL_COLOR_BUFFER_BIT;
		}
		if (param.flagDepth) {
#if defined(PRIV_OPENGL_IMPL)
			GL_ENTRY(glClearDepth)(param.depth);
#else
			GL_ENTRY(glClearDepthf)(param.depth);
#endif
			mask |= GL_DEPTH_BUFFER_BIT;
		}
		if (param.flagStencil) {
			GL_ENTRY(glClearStencil)(param.stencil);
			mask |= GL_STENCIL_BUFFER_BIT;
		}
		if (mask) {
			GL_ENTRY(glClear)(mask);
		}
	}

	void GL_BASE::clearColor(const Color& color)
	{
		RenderEngine::ClearParam param;
		param.flagColor = sl_true;
		param.color = color;
		param.flagDepth = sl_false;
		clear(param);
	}

	void GL_BASE::clearColorDepth(const Color& color, float depth)
	{
		RenderEngine::ClearParam param;
		param.flagColor = sl_true;
		param.color = color;
		param.flagDepth = sl_true;
		param.depth = depth;
		clear(param);
	}

	void GL_BASE::clearDepth(float depth)
	{
		RenderEngine::ClearParam param;
		param.flagColor = sl_false;
		param.flagDepth = sl_true;
		param.depth = depth;
		clear(param);
	}

	void GL_BASE::setDepthTest(sl_bool flag)
	{
		if (flag) {
			GL_ENTRY(glEnable)(GL_DEPTH_TEST);
		} else {
			GL_ENTRY(glDisable)(GL_DEPTH_TEST);
		}
	}

	void GL_BASE::setDepthWriteEnabled(sl_bool flagEnableDepthWrite)
	{
		GL_ENTRY(glDepthMask)(flagEnableDepthWrite ? GL_TRUE : GL_FALSE);
	}

	namespace {
		static GLenum GetFunctionOp(RenderFunctionOperation op)
		{
			switch (op) {
				case RenderFunctionOperation::Never:
					return GL_NEVER;
				case RenderFunctionOperation::Always:
					return GL_ALWAYS;
				case RenderFunctionOperation::Equal:
					return GL_EQUAL;
				case RenderFunctionOperation::NotEqual:
					return GL_NOTEQUAL;
				case RenderFunctionOperation::Less:
					return GL_LESS;
				case RenderFunctionOperation::LessEqual:
					return GL_LEQUAL;
				case RenderFunctionOperation::Greater:
					return GL_GREATER;
				case RenderFunctionOperation::GreaterEqual:
					return GL_GEQUAL;
			}
			return GL_ALWAYS;
		}
	}

	void GL_BASE::setDepthFunction(RenderFunctionOperation op)
	{
		GL_ENTRY(glDepthFunc)(GetFunctionOp(op));
	}

	void GL_BASE::setCullFace(sl_bool flagEnableCull, sl_bool flagCullCCW)
	{
		if (flagEnableCull) {
			GL_ENTRY(glEnable)(GL_CULL_FACE);
			GL_ENTRY(glCullFace)(GL_BACK);
			if (flagCullCCW) {
				GL_ENTRY(glFrontFace)(GL_CW);
			} else {
				GL_ENTRY(glFrontFace)(GL_CCW);
			}
		} else {
			GL_ENTRY(glDisable)(GL_CULL_FACE);
		}
	}

	namespace {
		static GLenum GetBlendingOp(RenderBlendingOperation op)
		{
			switch (op) {
				case RenderBlendingOperation::Add:
					return GL_FUNC_ADD;
				case RenderBlendingOperation::Subtract:
					return GL_FUNC_SUBTRACT;
				case RenderBlendingOperation::ReverseSubtract:
					return GL_FUNC_REVERSE_SUBTRACT;
			}
			return GL_FUNC_ADD;
		}

		static GLenum GetBlendingFactor(RenderBlendingFactor factor)
		{
			switch (factor) {
				case RenderBlendingFactor::One:
					return GL_ONE;
				case RenderBlendingFactor::Zero:
					return GL_ZERO;
				case RenderBlendingFactor::SrcAlpha:
					return GL_SRC_ALPHA;
				case RenderBlendingFactor::OneMinusSrcAlpha:
					return GL_ONE_MINUS_SRC_ALPHA;
				case RenderBlendingFactor::DstAlpha:
					return GL_DST_ALPHA;
				case RenderBlendingFactor::OneMinusDstAlpha:
					return GL_ONE_MINUS_DST_ALPHA;
				case RenderBlendingFactor::SrcColor:
					return GL_SRC_COLOR;
				case RenderBlendingFactor::OneMinusSrcColor:
					return GL_ONE_MINUS_SRC_COLOR;
				case RenderBlendingFactor::DstColor:
					return GL_DST_COLOR;
				case RenderBlendingFactor::OneMinusDstColor:
					return GL_ONE_MINUS_DST_COLOR;
				case RenderBlendingFactor::SrcAlphaSaturate:
					return GL_SRC_ALPHA_SATURATE;
				case RenderBlendingFactor::Constant:
					return GL_CONSTANT_COLOR;
				case RenderBlendingFactor::OneMinusConstant:
					return GL_ONE_MINUS_CONSTANT_COLOR;
				default:
					break;
			}
			return GL_ZERO;
		}
	}

	void GL_BASE::setBlending(const RenderBlendParam& param)
	{
		if (param.flagBlending) {
			GL_ENTRY(glEnable)(GL_BLEND);
#ifdef NEED_CHECK_ENTRY
			if (!GL_ENTRY(glBlendEquation)) {
				GLenum fSrc = GetBlendingFactor(param.blendSrc);
				GLenum fDst = GetBlendingFactor(param.blendDst);
				GL_ENTRY(glBlendFunc)(fSrc, fDst);
				return;
			}
#endif
			GLenum op = GetBlendingOp(param.operation);
			GLenum opAlpha = GetBlendingOp(param.operationAlpha);
			if (op != opAlpha) {
				GL_ENTRY(glBlendEquationSeparate)(op, opAlpha);
			} else {
				GL_ENTRY(glBlendEquation)(op);
			}
			GLenum fSrc = GetBlendingFactor(param.blendSrc);
			GLenum fDst = GetBlendingFactor(param.blendDst);
			GLenum fSrcAlpha = GetBlendingFactor(param.blendSrcAlpha);
			GLenum fDstAlpha = GetBlendingFactor(param.blendDstAlpha);
			if (fSrc == fSrcAlpha && fDst == fDstAlpha) {
				GL_ENTRY(glBlendFunc)(fSrc, fDst);
			} else {
				GL_ENTRY(glBlendFuncSeparate)(fSrc, fDst, fSrcAlpha, fDstAlpha);
			}
			GL_ENTRY(glBlendColor)(param.blendConstant.x, param.blendConstant.y, param.blendConstant.z, param.blendConstant.w);
		} else {
			GL_ENTRY(glDisable)(GL_BLEND);
		}
	}

	namespace {
		static sl_uint32 CreateShader(GLenum type, const String& source)
		{
#ifdef NEED_CHECK_ENTRY
			if (!GL_ENTRY(glCreateShader)) {
				return 0;
			}
#endif
			GLuint shader = GL_ENTRY(glCreateShader)(type);
			if (shader) {
				if (source.isNotEmpty()) {
					const GLchar* sz = source.getData();
					GLint len = (GLint)(source.getLength());
					GL_ENTRY(glShaderSource)(shader, 1, &sz, &len);
					GL_ENTRY(glCompileShader)(shader);
					GLint status = 0;
					GL_ENTRY(glGetShaderiv)(shader, GL_COMPILE_STATUS, &status);
					if (status != GL_FALSE) {
						return shader;
					} else {
						GLchar log[1025];
						GLsizei lenLog = 0;
						GL_ENTRY(glGetShaderInfoLog)(shader, 1024, &lenLog, log);
						log[lenLog] = 0;
						if (type == GL_VERTEX_SHADER) {
							Log("OpenGL Compile Vertex Shader", String(log));
						} else if (type == GL_FRAGMENT_SHADER) {
							Log("OpenGL Compile Fragment Shader", String(log));
						}
					}
				}
				GL_ENTRY(glDeleteShader)(shader);
			}
			return 0;
		}
	}

	sl_uint32 GL_BASE::createVertexShader(const String& source)
	{
		return CreateShader(GL_VERTEX_SHADER, source);
	}

	sl_uint32 GL_BASE::createFragmentShader(const String& source)
	{
		return CreateShader(GL_FRAGMENT_SHADER, source);
	}

	void GL_BASE::deleteShader(sl_uint32 shader)
	{
		if (shader) {
			GL_ENTRY(glDeleteShader)(shader);
		}
	}

	sl_uint32 GL_BASE::createProgram()
	{
		GLuint program = GL_ENTRY(glCreateProgram)();
		return program;
	}

	void GL_BASE::attachShader(sl_uint32 program, sl_uint32 shader)
	{
		if (program && shader) {
			GL_ENTRY(glAttachShader)(program, shader);
		}
	}

	sl_bool GL_BASE::linkProgram(sl_uint32 program)
	{
		if (program) {
			GL_ENTRY(glLinkProgram)(program);
			GLint status = GL_FALSE;
			GL_ENTRY(glGetProgramiv)(program, GL_LINK_STATUS, &status);
			if (status != GL_FALSE) {
				return sl_true;
			}
			GLchar log[1025];
			GLsizei lenLog = 0;
			GL_ENTRY(glGetProgramInfoLog)(program, 1024, &lenLog, log);
			log[lenLog] = 0;
			Log("OpenGL Program", String(log));
		}
		return sl_false;
	}

	sl_uint32 GL_BASE::createProgram(sl_uint32 vertexShader, sl_uint32 fragmentShader)
	{
		sl_uint32 program = GL_ENTRY(glCreateProgram)();
		if (program) {
			attachShader(program, vertexShader);
			attachShader(program, fragmentShader);
			if (linkProgram(program)) {
				return program;
			}
			deleteProgram(program);
		}
		return 0;
	}

	void GL_BASE::useProgram(sl_uint32 program)
	{
		if (program) {
			GL_ENTRY(glUseProgram)(program);
		}
	}

	void GL_BASE::deleteProgram(sl_uint32 program)
	{
		if (program) {
			GL_ENTRY(glDeleteProgram)(program);
		}
	}

	namespace {
		static sl_uint32 CreateBuffer(GLenum target, const void* data, sl_size size, sl_bool flagStatic)
		{
			GLuint buffer = 0;
			GL_ENTRY(glGenBuffers)(1, &buffer);
			if (buffer) {
				GL_ENTRY(glBindBuffer)(target, buffer);
				GL_ENTRY(glBufferData)(target, size, data, flagStatic ? GL_STATIC_DRAW : GL_DYNAMIC_DRAW);
				return buffer;
			}
			return 0;
		}

		static void UpdateBuffer(GLenum target, sl_uint32 buffer, sl_size offset, const void* data, sl_size size)
		{
			if (buffer) {
				GL_ENTRY(glBindBuffer)(target, buffer);
				GL_ENTRY(glBufferSubData)(target, buffer, size, data);
			}
		}
	}

	sl_uint32 GL_BASE::createVertexBuffer(const void* data, sl_size size, sl_bool flagStatic)
	{
		return CreateBuffer(GL_ARRAY_BUFFER, data, size, flagStatic);
	}

	sl_uint32 GL_BASE::createVertexBuffer(sl_size size, sl_bool flagStatic)
	{
		return CreateBuffer(GL_ARRAY_BUFFER, sl_null, size, flagStatic);
	}

	void GL_BASE::updateVertexBuffer(sl_uint32 buffer, sl_size offset, const void* data, sl_size size)
	{
		UpdateBuffer(GL_ARRAY_BUFFER, buffer, offset, data, size);
	}

	void GL_BASE::bindVertexBuffer(sl_uint32 buffer)
	{
		GL_ENTRY(glBindBuffer)(GL_ARRAY_BUFFER, buffer);
	}

	void GL_BASE::unbindVertexBuffer()
	{
		GL_ENTRY(glBindBuffer)(GL_ARRAY_BUFFER, 0);
	}

	sl_uint32 GL_BASE::createIndexBuffer(const void* data, sl_size size, sl_bool flagStatic)
	{
		return CreateBuffer(GL_ELEMENT_ARRAY_BUFFER, data, size, flagStatic);
	}

	sl_uint32 GL_BASE::createIndexBuffer(sl_size size, sl_bool flagStatic)
	{
		return CreateBuffer(GL_ELEMENT_ARRAY_BUFFER, sl_null, size, flagStatic);
	}

	void GL_BASE::updateIndexBuffer(sl_uint32 buffer, sl_size offset, const void* data, sl_size size)
	{
		UpdateBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer, offset, data, size);
	}

	void GL_BASE::bindIndexBuffer(sl_uint32 buffer)
	{
		GL_ENTRY(glBindBuffer)(GL_ELEMENT_ARRAY_BUFFER, buffer);
	}

	void GL_BASE::unbindIndexBuffer()
	{
		GL_ENTRY(glBindBuffer)(GL_ELEMENT_ARRAY_BUFFER, 0);
	}

	void GL_BASE::deleteBuffer(sl_uint32 buffer)
	{
		if (buffer) {
			GLuint b = buffer;
			GL_ENTRY(glDeleteBuffers)(1, &b);
		}
	}

	sl_int32 GL_BASE::getAttributeLocation(sl_uint32 program, const char* name)
	{
		if (program) {
			GLint ret = GL_ENTRY(glGetAttribLocation)(program, name);
			return ret;
		}
		return -1;
	}

	namespace {
		static void SetVertexArrayAttribute(sl_int32 attributeLocation, GLenum type, const void* data, sl_uint32 countComponents, sl_uint32 strideBytes, sl_bool flagDoNormalize)
		{
			if (attributeLocation != -1) {
				GL_ENTRY(glEnableVertexAttribArray)(attributeLocation);
				GL_ENTRY(glVertexAttribPointer)(attributeLocation, countComponents, type, flagDoNormalize ? GL_TRUE : GL_FALSE, strideBytes, data);
			}
		}
	}

#define PRIV_DEFINE_SETVERTEXARRAY(t, TYPE) \
	void GL_BASE::setVertex##t##ArrayAttributePtr(sl_int32 attributeLocation, const void* data, sl_uint32 countComponents, sl_uint32 strideBytes, sl_bool flagDoNormalize) \
	{ \
		SetVertexArrayAttribute(attributeLocation, TYPE, data, countComponents, strideBytes, flagDoNormalize); \
	} \
	void GL_BASE::setVertex##t##ArrayAttribute(sl_int32 attributeLocation, sl_size offsetValuesOnBuffer, sl_uint32 countComponents, sl_uint32 strideBytes, sl_bool flagDoNormalize) \
	{ \
		SetVertexArrayAttribute(attributeLocation, TYPE, (void*)offsetValuesOnBuffer, countComponents, strideBytes, flagDoNormalize); \
	}

	PRIV_DEFINE_SETVERTEXARRAY(Float, GL_FLOAT)
	PRIV_DEFINE_SETVERTEXARRAY(Int8, GL_BYTE)
	PRIV_DEFINE_SETVERTEXARRAY(Uint8, GL_UNSIGNED_BYTE)
	PRIV_DEFINE_SETVERTEXARRAY(Int16, GL_SHORT)
	PRIV_DEFINE_SETVERTEXARRAY(Uint16, GL_UNSIGNED_SHORT)
	PRIV_DEFINE_SETVERTEXARRAY(Int32, GL_INT)
	PRIV_DEFINE_SETVERTEXARRAY(Uint32, GL_UNSIGNED_INT)

	void GL_BASE::disableVertexArrayAttribute(sl_int32 attributeLocation)
	{
		if (attributeLocation != -1) {
			GL_ENTRY(glDisableVertexAttribArray)(attributeLocation);
		}
	}

	sl_int32 GL_BASE::getUniformLocation(sl_uint32 program, const char* name)
	{
		if (program) {
			GLint ret = GL_ENTRY(glGetUniformLocation)(program, name);
			return ret;
		}
		return -1;
	}

	void GL_BASE::setUniformFloatValue(sl_int32 uniformLocation, float value)
	{
		if (uniformLocation != -1) {
			GL_ENTRY(glUniform1f)(uniformLocation, value);
		}
	}

	void GL_BASE::setUniformFloatArray(sl_int32 uniformLocation, const void* values, sl_uint32 count)
	{
		if (uniformLocation != -1) {
			GL_ENTRY(glUniform1fv)(uniformLocation, count, (float*)values);
		}
	}

	void GL_BASE::setUniformIntValue(sl_int32 uniformLocation, sl_int32 value)
	{
		if (uniformLocation != -1) {
			GL_ENTRY(glUniform1i)(uniformLocation, value);
		}
	}

	void GL_BASE::setUniformIntArray(sl_int32 uniformLocation, const void* values, sl_uint32 count)
	{
		if (uniformLocation != -1) {
			GL_ENTRY(glUniform1iv)(uniformLocation, count, (GLint*)values);
		}
	}

	void GL_BASE::setUniformFloat2Value(sl_int32 uniformLocation, float v1, float v2)
	{
		if (uniformLocation != -1) {
			GL_ENTRY(glUniform2f)(uniformLocation, v1, v2);
		}
	}

	void GL_BASE::setUniformFloat2Value(sl_int32 uniformLocation, const Vector2& v)
	{
		if (uniformLocation != -1) {
			GL_ENTRY(glUniform2f)(uniformLocation, v.x, v.y);
		}
	}

	void GL_BASE::setUniformFloat2Array(sl_int32 uniformLocation, const void* values, sl_uint32 count)
	{
		if (uniformLocation != -1) {
			GL_ENTRY(glUniform2fv)(uniformLocation, count, (float*)values);
		}
	}

	void GL_BASE::setUniformInt2Value(sl_int32 uniformLocation, sl_int32 v1, sl_int32 v2)
	{
		if (uniformLocation != -1) {
			GL_ENTRY(glUniform2i)(uniformLocation, v1, v2);
		}
	}

	void GL_BASE::setUniformInt2Array(sl_int32 uniformLocation, const void* values, sl_uint32 count)
	{
		if (uniformLocation != -1) {
			GL_ENTRY(glUniform2iv)(uniformLocation, count, (GLint*)values);
		}
	}

	void GL_BASE::setUniformFloat3Value(sl_int32 uniformLocation, float v1, float v2, float v3)
	{
		if (uniformLocation != -1) {
			GL_ENTRY(glUniform3f)(uniformLocation, v1, v2, v3);
		}
	}

	void GL_BASE::setUniformFloat3Value(sl_int32 uniformLocation, const Vector3& v)
	{
		if (uniformLocation != -1) {
			GL_ENTRY(glUniform3f)(uniformLocation, v.x, v.y, v.z);
		}
	}

	void GL_BASE::setUniformFloat3Array(sl_int32 uniformLocation, const void* values, sl_uint32 count)
	{
		if (uniformLocation != -1) {
			GL_ENTRY(glUniform3fv)(uniformLocation, count, (float*)values);
		}
	}

	void GL_BASE::setUniformInt3Value(sl_int32 uniformLocation, sl_int32 v1, sl_int32 v2, sl_int32 v3)
	{
		if (uniformLocation != -1) {
			GL_ENTRY(glUniform3i)(uniformLocation, v1, v2, v3);
		}
	}

	void GL_BASE::setUniformInt3Array(sl_int32 uniformLocation, const void* values, sl_uint32 count)
	{
		if (uniformLocation != -1) {
			GL_ENTRY(glUniform3iv)(uniformLocation, count, (GLint*)values);
		}
	}

	void GL_BASE::setUniformFloat4Value(sl_int32 uniformLocation, float v1, float v2, float v3, float v4)
	{
		if (uniformLocation != -1) {
			GL_ENTRY(glUniform4f)(uniformLocation, v1, v2, v3, v4);
		}
	}

	void GL_BASE::setUniformFloat4Value(sl_int32 uniformLocation, const Vector4& v)
	{
		if (uniformLocation != -1) {
			GL_ENTRY(glUniform4f)(uniformLocation, v.x, v.y, v.z, v.w);
		}
	}

	void GL_BASE::setUniformFloat4Array(sl_int32 uniformLocation, const void* values, sl_uint32 count)
	{
		if (uniformLocation != -1) {
			GL_ENTRY(glUniform4fv)(uniformLocation, count, (float*)values);
		}
	}

	void GL_BASE::setUniformInt4Value(sl_int32 uniformLocation, sl_int32 v1, sl_int32 v2, sl_int32 v3, sl_int32 v4)
	{
		if (uniformLocation != -1) {
			GL_ENTRY(glUniform4i)(uniformLocation, v1, v2, v3, v4);
		}
	}

	void GL_BASE::setUniformInt4Array(sl_int32 uniformLocation, const void* values, sl_uint32 count)
	{
		if (uniformLocation != -1) {
			GL_ENTRY(glUniform4iv)(uniformLocation, count, (GLint*)values);
		}
	}

	void GL_BASE::setUniformMatrix2Value(sl_int32 uniformLocation, const Matrix2& value)
	{
		if (uniformLocation != -1) {
			float v[4];
			v[0] = value.m00;
			v[1] = value.m10;
			v[2] = value.m01;
			v[3] = value.m11;
			GL_ENTRY(glUniformMatrix2fv)(uniformLocation, 1, GL_FALSE, v);
		}
	}

	void GL_BASE::setUniformMatrix2Array(sl_int32 uniformLocation, const void* values, sl_uint32 count)
	{
		if (uniformLocation != -1) {
			SLIB_SCOPED_BUFFER(float, STACK_BUFFER_COUNT, mats, 4 * count);
			if (!mats) {
				return;
			}
			for (sl_uint32 i = 0; i < count; i++) {
				float* v = mats + i * 4;
				Matrix2& value = ((Matrix2*)(values))[i];
				v[0] = value.m00;
				v[1] = value.m10;
				v[2] = value.m01;
				v[3] = value.m11;
			}
			GL_ENTRY(glUniformMatrix2fv)(uniformLocation, count, GL_FALSE, mats);
		}
	}

	void GL_BASE::setUniformMatrix3Value(sl_int32 uniformLocation, const Matrix3& value)
	{
		if (uniformLocation != -1) {
			float v[9];
			v[0] = value.m00;
			v[1] = value.m10;
			v[2] = value.m20;
			v[3] = value.m01;
			v[4] = value.m11;
			v[5] = value.m21;
			v[6] = value.m02;
			v[7] = value.m12;
			v[8] = value.m22;
			GL_ENTRY(glUniformMatrix3fv)(uniformLocation, 1, GL_FALSE, v);
		}
	}

	void GL_BASE::setUniformMatrix3Array(sl_int32 uniformLocation, const void* values, sl_uint32 count)
	{
		if (uniformLocation != -1) {
			SLIB_SCOPED_BUFFER(float, STACK_BUFFER_COUNT, mats, 9 * count);
			if (!mats) {
				return;
			}
			for (sl_uint32 i = 0; i < count; i++) {
				float* v = mats + i * 9;
				Matrix3& value = ((Matrix3*)(values))[i];
				v[0] = value.m00;
				v[1] = value.m10;
				v[2] = value.m20;
				v[3] = value.m01;
				v[4] = value.m11;
				v[5] = value.m21;
				v[6] = value.m02;
				v[7] = value.m12;
				v[8] = value.m22;
			}
			GL_ENTRY(glUniformMatrix3fv)(uniformLocation, count, GL_FALSE, mats);
		}
	}

	void GL_BASE::setUniformMatrix4Value(sl_int32 uniformLocation, const Matrix4& value)
	{
		if (uniformLocation != -1) {
			float v[16];
			v[0] = value.m00;
			v[1] = value.m10;
			v[2] = value.m20;
			v[3] = value.m30;
			v[4] = value.m01;
			v[5] = value.m11;
			v[6] = value.m21;
			v[7] = value.m31;
			v[8] = value.m02;
			v[9] = value.m12;
			v[10] = value.m22;
			v[11] = value.m32;
			v[12] = value.m03;
			v[13] = value.m13;
			v[14] = value.m23;
			v[15] = value.m33;
			GL_ENTRY(glUniformMatrix4fv)(uniformLocation, 1, GL_FALSE, v);
		}
	}

	void GL_BASE::setUniformMatrix4Array(sl_int32 uniformLocation, const void* values, sl_uint32 count)
	{
		if (uniformLocation != -1) {
			SLIB_SCOPED_BUFFER(float, STACK_BUFFER_COUNT, mats, 16 * count);
			if (!mats) {
				return;
			}
			for (sl_uint32 i = 0; i < count; i++) {
				float* v = mats + i * 16;
				Matrix4& value = ((Matrix4*)(values))[i];
				v[0] = value.m00;
				v[1] = value.m10;
				v[2] = value.m20;
				v[3] = value.m30;
				v[4] = value.m01;
				v[5] = value.m11;
				v[6] = value.m21;
				v[7] = value.m31;
				v[8] = value.m02;
				v[9] = value.m12;
				v[10] = value.m22;
				v[11] = value.m32;
				v[12] = value.m03;
				v[13] = value.m13;
				v[14] = value.m23;
				v[15] = value.m33;
			}
			GL_ENTRY(glUniformMatrix4fv)(uniformLocation, count, GL_FALSE, mats);
		}
	}

	void GL_BASE::setUniformTextureSampler(sl_int32 uniformLocation, sl_uint32 textureNo)
	{
		if (uniformLocation != -1) {
			GL_ENTRY(glUniform1i)(uniformLocation, textureNo);
		}
	}

	namespace {
		SLIB_INLINE static GLenum GetPrimitiveType(PrimitiveType type)
		{
			switch (type) {
				case PrimitiveType::Triangle:
					return GL_TRIANGLES;
				case PrimitiveType::TriangleStrip:
					return GL_TRIANGLE_STRIP;
				case PrimitiveType::TriangleFan:
					return GL_TRIANGLE_FAN;
				case PrimitiveType::Line:
					return GL_LINES;
				case PrimitiveType::LineStrip:
					return GL_LINE_STRIP;
				case PrimitiveType::LineLoop:
					return GL_LINE_LOOP;
				case PrimitiveType::Point:
					return GL_POINTS;
			}
			return GL_TRIANGLES;
		}
	}

	void GL_BASE::drawPrimitives(PrimitiveType type, sl_uint32 countVertices, sl_uint32 startIndex)
	{
		GL_ENTRY(glDrawArrays)(GetPrimitiveType(type), startIndex, countVertices);
	}

	void GL_BASE::drawElements(PrimitiveType type, sl_uint32 countIndices, sl_size offsetBytes)
	{
		GL_ENTRY(glDrawElements)(GetPrimitiveType(type), countIndices, GL_UNSIGNED_SHORT, (void*)offsetBytes);
	}

	void GL_BASE::setLineWidth(float width)
	{
		GL_ENTRY(glLineWidth)(width);
	}

	void GL_BASE::flush()
	{
		GL_ENTRY(glFlush)();
	}

	sl_uint32 GL_BASE::createTexture2D(const BitmapData& bitmapData)
	{
		GLuint texture = 0;
		GL_ENTRY(glGenTextures)(1, &texture);
		if (texture) {
			sl_uint32 width = bitmapData.width;
			sl_uint32 height = bitmapData.height;
			GL_ENTRY(glBindTexture)(GL_TEXTURE_2D, texture);
			if (bitmapData.format == BitmapFormat::RGBA && (!(bitmapData.pitch) || bitmapData.pitch == (sl_int32)(width << 2))) {
				GL_ENTRY(glTexImage2D)(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, bitmapData.data);
			} else {
				sl_uint32 size = width * height;
				SLIB_SCOPED_BUFFER(sl_uint8, STACK_IMAGE_SIZE, glImage, size << 2);
				if (!glImage) {
					return 0;
				}
				BitmapData temp;
				temp.width = width;
				temp.height = height;
				temp.format = BitmapFormat::RGBA;
				temp.data = glImage;
				temp.pitch = width << 2;
				temp.copyPixelsFrom(bitmapData);
				GL_ENTRY(glTexImage2D)(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, glImage);
			}
			return texture;
		}
		return 0;
	}

	sl_uint32 GL_BASE::createTexture2D(sl_uint32 width, sl_uint32 height, const Color* pixels, sl_reg stride)
	{
		if (width > 0 && height > 0) {
			if (pixels) {
				BitmapData bitmapData(width, height, pixels, stride);
				return createTexture2D(bitmapData);
			} else {
				GLuint texture = 0;
				GL_ENTRY(glGenTextures)(1, &texture);
				if (texture) {
					GL_ENTRY(glBindTexture)(GL_TEXTURE_2D, texture);
					GL_ENTRY(glTexImage2D)(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, sl_null);
				}
				return texture;
			}
		} else {
			return 0;
		}
	}

	sl_uint32 GL_BASE::createTexture2D(const Ref<Bitmap>& bitmap, sl_uint32 x, sl_uint32 y, sl_uint32 w, sl_uint32 h)
	{
		if (bitmap.isNull()) {
			return 0;
		}
		if (!w || !h) {
			return 0;
		}
		sl_uint32 bw = bitmap->getWidth();
		sl_uint32 bh = bitmap->getHeight();
		if (!bw || !bh) {
			return 0;
		}
		if (x >= bw) {
			return 0;
		}
		if (y >= bh) {
			return 0;
		}
		if (x + w > bw) {
			return 0;
		}
		if (y + h > bh) {
			return 0;
		}
		if (bitmap->isImage()) {
			Ref<Image> image = Ref<Image>::cast(bitmap);
			return createTexture2D(w, h, image->getColorsAt(x, y), image->getStride());
		} else {
			SLIB_SCOPED_BUFFER(sl_uint8, STACK_IMAGE_SIZE, glImage, (w * h) << 2);
			if (!glImage) {
				return 0;
			}
			BitmapData temp;
			temp.width = w;
			temp.height = h;
			temp.format = BitmapFormat::RGBA;
			temp.data = glImage;
			temp.pitch = w << 2;
			if (bitmap->readPixels(x, y, temp)) {
				return createTexture2D(temp);
			}
			return 0;
		}
	}

	sl_uint32 GL_BASE::createTexture2D(const Ref<Bitmap>& bitmap)
	{
		if (bitmap.isNull()) {
			return 0;
		}
		sl_uint32 w = bitmap->getWidth();
		sl_uint32 h = bitmap->getHeight();
		if (!w) {
			return 0;
		}
		if (!h) {
			return 0;
		}
		if (bitmap->isImage()) {
			Ref<Image> image = Ref<Image>::cast(bitmap);
			return createTexture2D(w, h, image->getColors(), image->getStride());
		} else {
			SLIB_SCOPED_BUFFER(sl_uint8, STACK_IMAGE_SIZE, glImage, (w * h) << 2);
			if (!glImage) {
				return 0;
			}
			BitmapData temp;
			temp.width = w;
			temp.height = h;
			temp.format = BitmapFormat::RGBA;
			temp.data = glImage;
			temp.pitch = w << 2;
			if (bitmap->readPixels(0, 0, temp)) {
				return createTexture2D(temp);
			}
			return 0;
		}
	}

	sl_uint32 GL_BASE::createTexture2DFromMemory(const void* mem, sl_size size)
	{
		Ref<Image> image = Image::loadFromMemory(mem, size);
		return createTexture2D(image);
	}

	void GL_BASE::updateTexture2D(sl_uint32 x, sl_uint32 y, const BitmapData& bitmapData)
	{
		sl_uint32 width = bitmapData.width;
		sl_uint32 height = bitmapData.height;
		if (bitmapData.format == BitmapFormat::RGBA && (!(bitmapData.pitch) || bitmapData.pitch == (sl_int32)(width << 2))) {
			GL_ENTRY(glTexSubImage2D)(GL_TEXTURE_2D, 0, x, y, width, height, GL_RGBA, GL_UNSIGNED_BYTE, bitmapData.data);
		} else {
			sl_uint32 size = width * height;
			SLIB_SCOPED_BUFFER(sl_uint8, STACK_IMAGE_SIZE, glImage, size << 2);
			if (!glImage) {
				return;
			}
			BitmapData temp;
			temp.width = width;
			temp.height = height;
			temp.format = BitmapFormat::RGBA;
			temp.data = glImage;
			temp.pitch = width << 2;
			temp.copyPixelsFrom(bitmapData);
			GL_ENTRY(glTexSubImage2D)(GL_TEXTURE_2D, 0, x, y, width, height, GL_RGBA, GL_UNSIGNED_BYTE, glImage);
		}
	}

	void GL_BASE::updateTexture2D(sl_uint32 x, sl_uint32 y, sl_uint32 width, sl_uint32 height, const Color* pixels, sl_reg stride)
	{
		BitmapData bitmapData(width, height, pixels, stride);
		updateTexture2D(x, y, bitmapData);
	}

	void GL_BASE::updateTexture2D(sl_uint32 x, sl_uint32 y, sl_uint32 w, sl_uint32 h, const Ref<Bitmap>& bitmap, sl_uint32 bx, sl_uint32 by)
	{
		if (bitmap.isNull()) {
			return;
		}
		if (!w || !h) {
			return;
		}
		sl_uint32 bw = bitmap->getWidth();
		sl_uint32 bh = bitmap->getHeight();
		if (!bw || !bh) {
			return;
		}
		if (bx >= bw) {
			return;
		}
		if (by >= bh) {
			return;
		}
		if (bx + w > bw) {
			return;
		}
		if (by + h > bh) {
			return;
		}
		if (bitmap->isImage()) {
			Ref<Image> image = Ref<Image>::cast(bitmap);
			updateTexture2D(x, y, w, h, image->getColorsAt(bx, by), image->getStride());
		} else {
			SLIB_SCOPED_BUFFER(sl_uint8, STACK_IMAGE_SIZE, glImage, (w * h) << 2);
			if (!glImage) {
				return;
			}
			BitmapData temp;
			temp.width = w;
			temp.height = h;
			temp.format = BitmapFormat::RGBA;
			temp.data = glImage;
			temp.pitch = w << 2;
			if (bitmap->readPixels(bx, by, temp)) {
				updateTexture2D(x, y, temp);
			}
		}
	}

	void GL_BASE::setActiveSampler(sl_uint32 textureNo)
	{
#ifdef NEED_CHECK_ENTRY
		if (!GL_ENTRY(glActiveTexture)) {
			return;
		}
#endif
		GL_ENTRY(glActiveTexture)(GL_TEXTURE0 + textureNo);
	}

	void GL_BASE::bindTexture(sl_uint32 target, sl_uint32 texture)
	{
		GL_ENTRY(glBindTexture)(target, texture);
	}

	void GL_BASE::unbindTexture(sl_uint32 target)
	{
		GL_ENTRY(glBindTexture)(target, 0);
	}

	void GL_BASE::bindTexture2D(sl_uint32 texture)
	{
		GL_ENTRY(glBindTexture)(GL_TEXTURE_2D, texture);
	}

	void GL_BASE::unbindTexture2D()
	{
		GL_ENTRY(glBindTexture)(GL_TEXTURE_2D, 0);
	}

	namespace {
		static GLenum GetFilter(TextureFilterMode filter)
		{
			switch (filter) {
				case TextureFilterMode::Linear:
					return GL_LINEAR;
				case TextureFilterMode::Point:
					return GL_NEAREST;
			}
			return GL_NONE;
		}
	}

	void GL_BASE::setTextureFilterMode(sl_uint32 target, TextureFilterMode minFilter, TextureFilterMode magFilter)
	{
		GLenum f;
		f = GetFilter(minFilter);
		if (f != GL_NONE) {
			GL_ENTRY(glTexParameteri)(target, GL_TEXTURE_MIN_FILTER, f);
		}
		f = GetFilter(magFilter);
		if (f != GL_NONE) {
			GL_ENTRY(glTexParameteri)(target, GL_TEXTURE_MAG_FILTER, f);
		}
	}

	void GL_BASE::setTexture2DFilterMode(TextureFilterMode minFilter, TextureFilterMode magFilter)
	{
		setTextureFilterMode(GL_TEXTURE_2D, minFilter, magFilter);
	}

	namespace {
		static GLenum GetWrap(TextureWrapMode wrap)
		{
			switch (wrap) {
				case TextureWrapMode::Repeat:
					return GL_REPEAT;
				case TextureWrapMode::Mirror:
					return GL_MIRRORED_REPEAT;
				case TextureWrapMode::Clamp:
					return GL_CLAMP_TO_EDGE;
			}
			return GL_NONE;
		}
	}

	void GL_BASE::setTextureWrapMode(sl_uint32 target, TextureWrapMode wrapX, TextureWrapMode wrapY)
	{
		GLenum f;
		f = GetWrap(wrapX);
		if (f != GL_NONE) {
			GL_ENTRY(glTexParameteri)(target, GL_TEXTURE_WRAP_S, f);
		}
		f = GetWrap(wrapY);
		if (f != GL_NONE) {
			GL_ENTRY(glTexParameteri)(target, GL_TEXTURE_WRAP_T, f);
		}
	}

	void GL_BASE::setTexture2DWrapMode(TextureWrapMode wrapX, TextureWrapMode wrapY)
	{
		setTextureWrapMode(GL_TEXTURE_2D, wrapX, wrapY);
	}

	void GL_BASE::deleteTexture(sl_uint32 texture)
	{
		if (texture) {
			GLuint t = texture;
			GL_ENTRY(glDeleteTextures)(1, &t);
		}
	}

#ifdef PRIV_OPENGL_IMPL
	void GL_BASE::drawPixels(const BitmapData& bitmapData)
	{
#ifdef SLIB_PLATFORM_IS_WIN32
		sl_uint32 width = bitmapData.width;
		sl_uint32 height = bitmapData.height;
		if (bitmapData.format == BitmapFormat::RGBA && (!(bitmapData.pitch) || bitmapData.pitch == (sl_int32)(width << 2))) {
			GL_ENTRY(glDrawPixels)(width, height, GL_RGBA, GL_UNSIGNED_BYTE, bitmapData.data);
		} else {
			sl_uint32 size = width * height;
			SLIB_SCOPED_BUFFER(sl_uint8, STACK_IMAGE_SIZE, glImage, size << 2);
			if (!glImage) {
				return;
			}
			BitmapData temp;
			temp.width = width;
			temp.height = height;
			temp.format = BitmapFormat::RGBA;
			temp.data = glImage;
			temp.pitch = width << 2;
			temp.copyPixelsFrom(bitmapData);
			GL_ENTRY(glDrawPixels)(width, height, GL_RGBA, GL_UNSIGNED_BYTE, glImage);
		}
#endif
	}

	void GL_BASE::drawPixels(sl_uint32 width, sl_uint32 height, const Color* pixels, sl_reg stride)
	{
#ifdef SLIB_PLATFORM_IS_WIN32
		if (width > 0 && height > 0 && pixels) {
			BitmapData bitmapData(width, height, pixels, stride);
			drawPixels(bitmapData);
		}
#endif
	}

	void GL_BASE::drawPixels(const Ref<Bitmap>& bitmap, sl_uint32 sx, sl_uint32 sy, sl_uint32 w, sl_uint32 h)
	{
#ifdef SLIB_PLATFORM_IS_WIN32
		if (bitmap.isNull()) {
			return;
		}
		if (!w || !h) {
			return;
		}
		sl_uint32 bw = bitmap->getWidth();
		sl_uint32 bh = bitmap->getHeight();
		if (!bw || !bh) {
			return;
		}
		if (sx >= bw) {
			return;
		}
		if (sy >= bh) {
			return;
		}
		if (sx + w > bw) {
			return;
		}
		if (sy + h > bh) {
			return;
		}
		if (bitmap->isImage()) {
			Ref<Image> image = Ref<Image>::cast(bitmap);
			drawPixels(w, h, image->getColorsAt(sx, sy), image->getStride());
		} else {
			SLIB_SCOPED_BUFFER(sl_uint8, STACK_IMAGE_SIZE, glImage, (w * h) << 2);
			if (!glImage) {
				return;
			}
			BitmapData temp;
			temp.width = w;
			temp.height = h;
			temp.format = BitmapFormat::RGBA;
			temp.data = glImage;
			temp.pitch = w << 2;
			if (bitmap->readPixels(sx, sy, temp)) {
				drawPixels(temp);
			}
		}
#endif
	}

	void GL_BASE::drawPixels(const Ref<Bitmap>& bitmap)
	{
#ifdef SLIB_PLATFORM_IS_WIN32
		if (bitmap.isNull()) {
			return;
		}
		sl_uint32 w = bitmap->getWidth();
		sl_uint32 h = bitmap->getHeight();
		if (!w) {
			return;
		}
		if (!h) {
			return;
		}
		if (bitmap->isImage()) {
			Ref<Image> image = Ref<Image>::cast(bitmap);
			drawPixels(w, h, image->getColors(), image->getStride());
		} else {
			SLIB_SCOPED_BUFFER(sl_uint8, STACK_IMAGE_SIZE, glImage, (w * h) << 2);
			if (!glImage) {
				return;
			}
			BitmapData temp;
			temp.width = w;
			temp.height = h;
			temp.format = BitmapFormat::RGBA;
			temp.data = glImage;
			temp.pitch = w << 2;
			if (bitmap->readPixels(0, 0, temp)) {
				drawPixels(temp);
			}
		}
#endif
	}

	void GL_BASE::setRasterPosition(float x, float y)
	{
#ifdef SLIB_PLATFORM_IS_WIN32
		GL_ENTRY(glRasterPos2f)(x, y);
#endif
	}

	void GL_BASE::setPixelZoom(float xf, float yf)
	{
#ifdef SLIB_PLATFORM_IS_WIN32
		GL_ENTRY(glPixelZoom)(xf, yf);
#endif
	}
#endif

	namespace {
		class EngineImpl : public GLRenderEngine
		{
		public:
			CList<sl_uint32> m_listDirtyBufferHandles;
			CList<sl_uint32> m_listDirtyTextureHandles;
			struct GLProgramHandle {
				sl_uint32 program;
				sl_uint32 vertexShader;
				sl_uint32 fragmentShader;
			};
			CList<GLProgramHandle> m_listDirtyProgramHandles;
			CList< Ref<CRef> > m_listDirtyObjects;

			class GLRenderProgramInstance;
			Ref<RenderProgram> m_currentProgram;
			Ref<GLRenderProgramInstance> m_currentProgramInstance;

			class GLRenderInputLayout;
			Ref<GLRenderInputLayout> m_currentInputLayout;

			class GLVertexBufferInstance;
			Ref<GLVertexBufferInstance> m_currentVertexBufferInstance;

			class GLIndexBufferInstance;
			Ref<GLIndexBufferInstance> m_currentIndexBufferInstance;

			Ref<RenderProgram> m_currentProgramRendering;
			Ref<GLRenderProgramInstance> m_currentProgramInstanceRendering;

			Ref<RenderSamplerState> m_currentSamplerStates[8];

			class GLTextureInstance;

			class GLSamplerState {
			public:
				Ref<Texture> texture;
				Ref<GLTextureInstance> instance;
				TextureFilterMode minFilter;
				TextureFilterMode magFilter;
				TextureWrapMode wrapX;
				TextureWrapMode wrapY;
			public:
				void reset()
				{
					texture.setNull();
					instance.setNull();
				}
			};

		public:
			EngineImpl()
			{
			}

			~EngineImpl()
			{
			}

		public:
			void freeDirtyHandles()
			{
				{
					ListLocker<sl_uint32> list(m_listDirtyBufferHandles);
					for (sl_size i = 0; i < list.count; i++) {
						GL_BASE::deleteBuffer(list[i]);
					}
					m_listDirtyBufferHandles.removeAll_NoLock();
				}
				{
					ListLocker<sl_uint32> list(m_listDirtyTextureHandles);
					for (sl_size i = 0; i < list.count; i++) {
						GL_BASE::deleteTexture(list[i]);
					}
					m_listDirtyTextureHandles.removeAll_NoLock();
				}
				{
					ListLocker<GLProgramHandle> list(m_listDirtyProgramHandles);
					for (sl_size i = 0; i < list.count; i++) {
						GL_BASE::deleteProgram(list[i].program);
						GL_BASE::deleteShader(list[i].vertexShader);
						GL_BASE::deleteShader(list[i].fragmentShader);
					}
					m_listDirtyProgramHandles.removeAll_NoLock();
				}
				{
					m_listDirtyObjects.removeAll();
				}
			}

			RenderEngineType getEngineType() override
			{
	#if defined(PRIV_OPENGL_ES_IMPL)
				return RenderEngineType::OpenGL_ES;
	#else
				return RenderEngineType::OpenGL;
	#endif
			}

			sl_bool isShaderAvailable() override
			{
				return GL_BASE::isShaderAvailable();
			}


			struct GLRenderInputLayoutItem
			{
				sl_int32 location;
				GLenum type;
				sl_uint32 offset;
				sl_uint32 count;
				sl_uint32 slot;
			};

			class GLRenderInputLayout : public RenderInputLayout
			{
			public:
				List<GLRenderInputLayoutItem> m_items;
				sl_uint32 m_stride;

			public:
				static Ref<GLRenderInputLayout> create(sl_uint32 program, const RenderInputLayoutParam& param)
				{
					if (!(param.strides.getCount())) {
						return sl_null;
					}
					List<GLRenderInputLayoutItem> items;
					ListElements<RenderInputLayoutItem> inputs(param.items);
					for (sl_size i = 0; i < inputs.count; i++) {
						RenderInputLayoutItem& input = inputs[i];
						if (!(input.slot)) {
							sl_bool flagValidType = sl_true;
							GLRenderInputLayoutItem item;
							switch (input.type) {
							case RenderInputType::Float:
								item.type = GL_FLOAT;
								item.count = 1;
								break;
							case RenderInputType::Float2:
								item.type = GL_FLOAT;
								item.count = 2;
								break;
							case RenderInputType::Float3:
								item.type = GL_FLOAT;
								item.count = 3;
								break;
							case RenderInputType::Float4:
								item.type = GL_FLOAT;
								item.count = 4;
								break;
							case RenderInputType::UByte4:
								item.type = GL_UNSIGNED_BYTE;
								item.count = 4;
								break;
							case RenderInputType::Short2:
								item.type = GL_SHORT;
								item.count = 2;
								break;
							case RenderInputType::Short4:
								item.type = GL_SHORT;
								item.count = 4;
								break;
							default:
								flagValidType = sl_false;
								break;
							}
							if (flagValidType) {
								sl_int32 location = GL_BASE::getAttributeLocation(program, input.name);
								if (location >= 0) {
									item.location = location;
									item.offset = input.offset;
									item.slot = input.slot;
									items.add_NoLock(item);
								}
							}
						}
					}
					if (items.isNotEmpty()) {
						Ref<GLRenderInputLayout> ret = new GLRenderInputLayout;
						if (ret.isNotNull()) {
							ret->m_stride = param.strides[0];
							ret->m_items = items;
							return ret;
						}
					}
					return sl_null;
				}

			public:
				void load()
				{
					ListElements<GLRenderInputLayoutItem> items(m_items);
					for (sl_size i = 0; i < items.count; i++) {
						GLRenderInputLayoutItem& item = items[i];
						SetVertexArrayAttribute(item.location, item.type, (void*)(sl_size)(item.offset), item.count, m_stride, sl_false);
					}
				}

				void unload()
				{
					ListElements<GLRenderInputLayoutItem> items(m_items);
					for (sl_size i = 0; i < items.count; i++) {
						GLRenderInputLayoutItem& item = items[i];
						GL_BASE::disableVertexArrayAttribute(item.location);
					}
				}

			};

			class GLRenderProgramInstance : public RenderProgramInstance
			{
			public:
				sl_uint32 vertexShader;
				sl_uint32 fragmentShader;
				sl_uint32 program;
				Ref<RenderProgramState> state;

			public:
				GLRenderProgramInstance()
				{
					vertexShader = -1;
					fragmentShader = -1;
					program = -1;
				}

				~GLRenderProgramInstance()
				{
					Ref<RenderEngine> engine = getEngine();
					if (engine.isNotNull()) {
						GLProgramHandle handle;
						handle.program = program;
						handle.vertexShader = vertexShader;
						handle.fragmentShader = fragmentShader;
						((EngineImpl*)(engine.get()))->m_listDirtyProgramHandles.add(handle);
					}
				}

			public:
				static Ref<GLRenderProgramInstance> create(EngineImpl* engine, RenderProgram* program)
				{
					String vsSource = convertShader(program->getShader(engine, RenderShaderType::GLSL_Vertex));
					String fsSource = convertShader(program->getShader(engine, RenderShaderType::GLSL_Fragment));
					if (vsSource.isNotEmpty() && fsSource.isNotEmpty()) {
						sl_uint32 vs = GL_BASE::createVertexShader(vsSource);
						if (vs) {
							sl_uint32 fs = GL_BASE::createFragmentShader(fsSource);
							if (fs) {
								sl_uint32 ph = GL_BASE::createProgram(vs, fs);
								if (ph) {
									Ref<RenderProgramState> state = program->onCreate(engine);
									if (state.isNotNull()) {
										Ref<GLRenderProgramInstance> ret = new GLRenderProgramInstance();
										if (ret.isNotNull()) {
											ret->program = ph;
											ret->vertexShader = vs;
											ret->fragmentShader = fs;
											ret->m_engine = engine;
											state->setProgramInstance(ret.get());
											if (program->onInit(engine, ret.get(), state.get())) {
												ret->state = state;
												ret->link(engine, program);
												return ret;
											}
											return sl_null;
										}
									}
									GL_BASE::deleteProgram(ph);
								}
								GL_BASE::deleteShader(fs);
							}
							GL_BASE::deleteShader(vs);
						}
					}
					return sl_null;
				}

				static String convertShader(String glsl)
				{
	#if defined(PRIV_OPENGL_IMPL)
	#else
					if (! (glsl.contains("precision highp float;") || glsl.contains("precision mediump float;") || glsl.contains("precision lowp float;"))) {
						glsl = "precision mediump float;" + glsl;
					}
	#endif
					return glsl;
				}

			public:
				Ref<RenderInputLayout> createInputLayout(const RenderInputLayoutParam& param) override
				{
					Ref<RenderEngine> engine = getEngine();
					if (engine.isNotNull()) {
						return Ref<RenderInputLayout>::cast(GLRenderInputLayout::create(program, param));
					}
					return sl_null;
				}

				sl_bool getUniformLocation(const char* name, RenderUniformLocation* outLocation) override
				{
					sl_int32 location = GL_BASE::getUniformLocation(program, name);
					if (location >= 0) {
						outLocation->location = location;
						return sl_true;
					}
					return sl_false;
				}

				void setUniform(const RenderUniformLocation& l, RenderUniformType type, const void* data, sl_uint32 nItems) override
				{
					sl_int32 location = (sl_int32)(l.location);
					if (location < 0) {
						return;
					}
					switch (type) {
					case RenderUniformType::Float:
						if (nItems == 1) {
							GL_BASE::setUniformFloatValue(location, *((float*)data));
						} else {
							GL_BASE::setUniformFloatArray(location, data, nItems);
						}
						break;
					case RenderUniformType::Float2:
						if (nItems == 1) {
							GL_BASE::setUniformFloat2Value(location, *((Vector2*)data));
						} else {
							GL_BASE::setUniformFloat2Array(location, data, nItems);
						}
						break;
					case RenderUniformType::Float3:
						if (nItems == 1) {
							GL_BASE::setUniformFloat3Value(location, *((Vector3*)data));
						} else {
							GL_BASE::setUniformFloat3Array(location, data, nItems);
						}
						break;
					case RenderUniformType::Float4:
						if (nItems == 1) {
							GL_BASE::setUniformFloat4Value(location, *((Vector4*)data));
						} else {
							GL_BASE::setUniformFloat4Array(location, data, nItems);
						}
						break;
					case RenderUniformType::Int:
						if (nItems == 1) {
							GL_BASE::setUniformIntValue(location, *((sl_int32*)data));
						} else {
							GL_BASE::setUniformIntArray(location, data, nItems);
						}
						break;
					case RenderUniformType::Int2:
						GL_BASE::setUniformInt2Array(location, data, nItems);
						break;
					case RenderUniformType::Int3:
						GL_BASE::setUniformInt3Array(location, data, nItems);
						break;
					case RenderUniformType::Int4:
						GL_BASE::setUniformInt4Array(location, data, nItems);
						break;
					case RenderUniformType::Matrix3:
						if (nItems == 1) {
							GL_BASE::setUniformMatrix3Value(location, *((Matrix3*)data));
						} else {
							GL_BASE::setUniformMatrix3Array(location, data, nItems);
						}
						break;
					case RenderUniformType::Matrix4:
						if (nItems == 1) {
							GL_BASE::setUniformMatrix4Value(location, *((Matrix4*)data));
						} else {
							GL_BASE::setUniformMatrix4Array(location, data, nItems);
						}
						break;
					case RenderUniformType::Sampler:
						GL_BASE::setUniformTextureSampler(location, (sl_uint32)(*((sl_reg*)data)));
					default:
						break;
					}
				}

			};

			Ref<RenderProgramInstance> _createProgramInstance(RenderProgram* program) override
			{
				return GLRenderProgramInstance::create(this, program);
			}

			class GLVertexBufferInstance : public VertexBufferInstance
			{
			public:
				sl_uint32 handle;

			public:
				GLVertexBufferInstance()
				{
					handle = -1;
				}

				~GLVertexBufferInstance()
				{
					Ref<RenderEngine> engine = getEngine();
					if (engine.isNotNull()) {
						((EngineImpl*)(engine.get()))->m_listDirtyBufferHandles.add(handle);
					}
				}

			public:
				static Ref<GLVertexBufferInstance> create(EngineImpl* engine, VertexBuffer* buffer)
				{
					sl_uint32 size = buffer->getSize();
					if (!size) {
						return sl_null;
					}
					Memory content = buffer->getSource();
					if (content.getSize() < size) {
						return sl_null;
					}
					sl_uint32 handle = GL_BASE::createVertexBuffer(content.getData(), size, buffer->getFlags() & RenderObjectFlags::StaticDraw);
					if (handle) {
						Ref<GLVertexBufferInstance> ret = new GLVertexBufferInstance();
						if (ret.isNotNull()) {
							ret->handle = handle;
							ret->link(engine, buffer);
							return ret;
						}
						GL_BASE::deleteBuffer(handle);
					}
					return sl_null;
				}

				void onUpdate(RenderBaseObject* object) override
				{
					VertexBuffer* buffer = (VertexBuffer*)object;
					Memory content = buffer->getSource();
					if (content.getSize() < m_updatedOffset + m_updatedSize) {
						return;
					}
					GL_BASE::updateVertexBuffer(handle, m_updatedOffset, (sl_uint8*)(content.getData()) + m_updatedOffset, m_updatedSize);
				}

			};

			Ref<VertexBufferInstance> _createVertexBufferInstance(VertexBuffer* buffer) override
			{
				return GLVertexBufferInstance::create(this, buffer);
			}

			class GLIndexBufferInstance : public IndexBufferInstance
			{
			public:
				sl_uint32 handle;

			public:
				GLIndexBufferInstance()
				{
					handle = -1;
				}

				~GLIndexBufferInstance()
				{
					Ref<RenderEngine> engine = getEngine();
					if (engine.isNotNull()) {
						((EngineImpl*)(engine.get()))->m_listDirtyBufferHandles.add(handle);
					}
				}

			public:
				static Ref<GLIndexBufferInstance> create(EngineImpl* engine, IndexBuffer* buffer)
				{
					sl_uint32 size = buffer->getSize();
					if (!size) {
						return sl_null;
					}
					Memory content = buffer->getSource();
					if (content.getSize() < size) {
						return sl_null;
					}
					sl_uint32 handle = GL_BASE::createIndexBuffer(content.getData(), size, buffer->getFlags() & RenderObjectFlags::StaticDraw);
					if (handle) {
						Ref<GLIndexBufferInstance> ret = new GLIndexBufferInstance();
						if (ret.isNotNull()) {
							ret->handle = handle;
							ret->link(engine, buffer);
							return ret;
						}
						GL_BASE::deleteBuffer(handle);
					}
					return sl_null;
				}

				void onUpdate(RenderBaseObject* object) override
				{
					IndexBuffer* buffer = (IndexBuffer*)object;
					Memory content = buffer->getSource();
					if (content.getSize() < m_updatedOffset + m_updatedSize) {
						return;
					}
					GL_BASE::updateIndexBuffer(handle, m_updatedOffset, (sl_uint8*)(content.getData()) + m_updatedOffset, m_updatedSize);
				}

			};

			Ref<IndexBufferInstance> _createIndexBufferInstance(IndexBuffer* buffer) override
			{
				return GLIndexBufferInstance::create(this, buffer);
			}

			class GLTextureInstance : public TextureInstance
			{
			public:
				sl_uint32 handle;

			public:
				GLTextureInstance()
				{
					handle = -1;
				}

				~GLTextureInstance()
				{
					Ref<RenderEngine> engine = getEngine();
					if (engine.isNotNull()) {
						((EngineImpl*)(engine.get()))->m_listDirtyTextureHandles.add(handle);
					}
				}

			public:
				static Ref<GLTextureInstance> create(EngineImpl* engine, Texture* texture)
				{
					Ref<Bitmap> content = texture->getSource();
					if (content.isNull()) {
						return sl_null;
					}
					sl_uint32 handle = GL_BASE::createTexture2D(content);
					if (handle) {
						Ref<GLTextureInstance> ret = new GLTextureInstance();
						if (ret.isNotNull()) {
							ret->handle = handle;
							ret->link(engine, texture);
							return ret;
						}
						GL_BASE::deleteTexture(handle);
					}
					return sl_null;
				}

				void onUpdate(RenderBaseObject* object) override
				{
					Texture* texture = (Texture*)object;
					Ref<Bitmap> content = texture->getSource();
					if (content.isNull()) {
						return;
					}
					GL_BASE::bindTexture2D(handle);
					GL_BASE::updateTexture2D(m_updatedRegion.left, m_updatedRegion.top, m_updatedRegion.getWidth(), m_updatedRegion.getHeight(), content, m_updatedRegion.left, m_updatedRegion.top);
				}

			};

			class GLNamedTexture : public EngineTexture
			{
			public:
				WeakRef<EngineImpl> m_engine;
				sl_uint32 m_target;
				sl_uint32 m_name;
				sl_bool m_flagDeleteOnRelease;

			public:
				GLNamedTexture(EngineImpl* engine, sl_uint32 target, sl_uint32 name, sl_bool flagDeleteOnRelease):
			m_engine(engine), m_target(target), m_name(name), m_flagDeleteOnRelease(flagDeleteOnRelease)
				{
				}

				~GLNamedTexture()
				{
					Ref<EngineImpl> engine = m_engine;
					if (engine.isNotNull()) {
						if (m_flagDeleteOnRelease) {
							engine->m_listDirtyTextureHandles.add(m_name);
						}
					}
				}

			public:
				Ref<Bitmap> getSource() override
				{
					return sl_null;
				}

			};

			Ref<TextureInstance> _createTextureInstance(Texture* texture, sl_int32 sampler) override
			{
				GL_BASE::setActiveSampler(sampler);
				Ref<GLTextureInstance> ret = GLTextureInstance::create(this, texture);
				if (ret.isNotNull()) {
					_applySamplerState(sampler);
					return Ref<TextureInstance>::cast(ret);
				}
				return sl_null;
			}

			sl_bool _beginScene() override
			{
				freeDirtyHandles();
				return sl_true;
			}

			void _endScene() override
			{
			}

			void _setViewport(sl_uint32 x, sl_uint32 y, sl_uint32 width, sl_uint32 height) override
			{
				GL_BASE::setViewport(x, y, width, height);
			}

			void _clear(const ClearParam& param) override
			{
				GL_BASE::clear(param);
			}

			void _setDepthStencilState(RenderDepthStencilState* state) override
			{
				const RenderDepthStencilParam& param = state->getParam();
				GL_BASE::setDepthTest(param.flagTestDepth);
				GL_BASE::setDepthWriteEnabled(param.flagWriteDepth);
				GL_BASE::setDepthFunction(param.depthFunction);
			}

			void _setRasterizerState(RenderRasterizerState* state) override
			{
				const RenderRasterizerParam& param = state->getParam();
				GL_BASE::setCullFace(param.flagCull, param.flagCullCCW);
			}

			void _setBlendState(RenderBlendState* state) override
			{
				GL_BASE::setBlending(state->getParam());
			}

			void _setSamplerState(sl_int32 samplerNo, RenderSamplerState* state) override
			{
				if ((sl_uint32)samplerNo >= CountOfArray(m_currentSamplerStates)) {
					return;
				}
				m_currentSamplerStates[samplerNo] = state;
			}

			sl_bool _beginProgram(RenderProgram* program, RenderProgramInstance* _instance, RenderProgramState** ppState) override
			{
				GLRenderProgramInstance* instance = (GLRenderProgramInstance*)_instance;
				if (m_currentProgramInstance != instance) {
					GL_BASE::useProgram(instance->program);
					m_currentProgramInstance = instance;
				}
				if (ppState) {
					*ppState = instance->state.get();
				}
				m_currentProgram = program;
				return sl_true;
			}

			void _endProgram() override
			{
			}

			void _resetCurrentBuffers() override
			{
				m_currentProgram.setNull();
				m_currentProgramInstance.setNull();
				m_currentProgramRendering.setNull();
				m_currentProgramInstanceRendering.setNull();
				m_currentVertexBufferInstance.setNull();
				m_currentIndexBufferInstance.setNull();
			}

			void _drawPrimitive(EnginePrimitive* primitive) override
			{
				if (m_currentProgram.isNull()) {
					return;
				}
				if (m_currentProgramInstance.isNull()) {
					return;
				}

				GLVertexBufferInstance* vb = static_cast<GLVertexBufferInstance*>(primitive->vertexBufferInstance.get());
				if (!vb) {
					return;
				}
				vb->doUpdate(primitive->vertexBuffer.get());
				GLIndexBufferInstance* ib = sl_null;
				if (primitive->indexBufferInstance.isNotNull()) {
					ib = (GLIndexBufferInstance*)(primitive->indexBufferInstance.get());
					ib->doUpdate(primitive->indexBuffer.get());
				}

				sl_bool flagResetProgramState = sl_false;
				if (m_currentProgramInstanceRendering != m_currentProgramInstance) {
					flagResetProgramState = sl_true;
				}
				if (vb != m_currentVertexBufferInstance || ib != m_currentIndexBufferInstance) {
					flagResetProgramState = sl_true;
					m_currentVertexBufferInstance = vb;
					m_currentIndexBufferInstance = ib;
					GL_BASE::bindVertexBuffer(vb->handle);
					if (ib) {
						GL_BASE::bindIndexBuffer(ib->handle);
					}
				}

				if (flagResetProgramState) {
					if (m_currentProgramInstanceRendering.isNotNull()) {
						m_currentProgramRendering->onPostRender(this, m_currentProgramInstanceRendering.get(), m_currentProgramInstanceRendering->state.get());
					}
					m_currentProgramInstanceRendering = m_currentProgramInstance;
					m_currentProgramRendering = m_currentProgram;
					m_currentProgram->onPreRender(this, m_currentProgramInstance.get(), m_currentProgramInstance->state.get());
				}

				if (ib) {
					GL_BASE::drawElements(primitive->type, primitive->elementCount);
				} else {
					GL_BASE::drawPrimitives(primitive->type, primitive->elementCount);
				}
			}

			void _applyTexture(Texture* texture, TextureInstance* _instance, sl_int32 sampler) override
			{
				GL_BASE::setActiveSampler(sampler);
				if (!texture) {
					GL_BASE::unbindTexture2D();
					return;
				}
				if (_instance) {
					GLTextureInstance* instance = (GLTextureInstance*)_instance;
					if (instance->isUpdated()) {
						instance->doUpdate(texture);
					}
					GL_BASE::bindTexture2D(instance->handle);
				} else {
					GLNamedTexture* named = static_cast<GLNamedTexture*>(texture);
					GL_BASE::bindTexture(named->m_target, named->m_name);
				}
				_applySamplerState(sampler);
			}

			void _applySamplerState(sl_int32 sampler)
			{
				const RenderSamplerParam* param = sl_null;
				if ((sl_uint32)sampler < CountOfArray(m_currentSamplerStates)) {
					RenderSamplerState* state = m_currentSamplerStates[sampler].get();
					if (state) {
						param = &(state->getParam());
					}
				}
				if (param) {
					GL_BASE::setTexture2DFilterMode(param->minFilter, param->magFilter);
					GL_BASE::setTexture2DWrapMode(param->wrapX, param->wrapY);
				} else {
					GL_BASE::setTexture2DFilterMode(TextureFilterMode::Linear, TextureFilterMode::Linear);
					GL_BASE::setTexture2DWrapMode(TextureWrapMode::Clamp, TextureWrapMode::Clamp);
				}
			}

			void _setInputLayout(RenderInputLayout* _layout) override
			{
				GLRenderInputLayout* layout = (GLRenderInputLayout*)_layout;
				Ref<GLRenderInputLayout> old = Move(m_currentInputLayout);
				if (old.isNotNull()) {
					old->unload();
				}
				if (layout) {
					layout->load();
				}
				m_currentInputLayout = layout;
			}

			void _setLineWidth(sl_real width) override
			{
				GL_BASE::setLineWidth(width);
			}

			Ref<Texture> createTextureFromName(sl_uint32 target, sl_uint32 name, sl_bool flagDeleteOnRelease) override
			{
				return new GLNamedTexture(this, target, name, flagDeleteOnRelease);
			}

		};
	}

	Ref<GLRenderEngine> GL_BASE::createEngine()
	{
		return new EngineImpl();
	}

}

#endif
