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

#ifndef CHECKHEADER_SLIB_RENDER_PROGRAM
#define CHECKHEADER_SLIB_RENDER_PROGRAM

#include "definition.h"

#include "base.h"
#include "texture.h"

#include "../math/matrix4.h"
#include "../graphics/color.h"

namespace slib
{

	class Primitive;
	class RenderEngine;
	class RenderProgramInstance;
	class RenderProgramConstant;

	enum class RenderProgramStateKind
	{
		None = 0,
		Uniform = 1,
		Input = 2
	};

	enum class RenderInputType
	{
		Float = 0,
		Float2 = 1,
		Float3 = 2,
		Float4 = 3,
		UByte4 = 0x10,
		Short2 = 0x11,
		Short4 = 0x12
	};

	enum class RenderInputSemanticName
	{
		Undefined = 255,
		Position = 0,
		BlendWeight = 1,
		BlendIndices = 2,
		Normal = 3,
		PSize = 4,
		TexCoord = 5,
		Tangent = 6,
		BiNormal = 7,
		TessFactor = 8,
		PositionT = 9,
		Color = 10,
		Fog = 11,
		Depth = 12,
		Sample = 13
	};

	struct RenderInputDesc
	{
		RenderInputType type;
		sl_uint32 offset;
		RenderInputSemanticName semanticName;
		sl_uint32 semanticIndex;
	};

	class SLIB_EXPORT RenderProgramStateItem
	{
	public:
		const char* name;
		RenderProgramStateKind kind;

		Ref<RenderProgramConstant> uniform;
		RenderInputDesc input;

	public:
		// None
		RenderProgramStateItem();

		// Uniform
		RenderProgramStateItem(const char* name);

		// Input
		RenderProgramStateItem(const char* name, RenderInputType type, sl_uint32 offset, RenderInputSemanticName semanticName = RenderInputSemanticName::Undefined, sl_uint32 semanticIndex = 0);

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(RenderProgramStateItem)

	};


	class SLIB_EXPORT RenderProgramConstant : public Referable
	{
		SLIB_DECLARE_OBJECT

	public:
		RenderProgramConstant();

		~RenderProgramConstant();

	public:
		virtual void setFloatValue(float value) = 0;

		virtual void setFloatArray(const float* arr, sl_uint32 n) = 0;

		virtual void setIntValue(sl_int32 value) = 0;

		virtual void setIntArray(const sl_int32* value, sl_uint32 n) = 0;

		virtual void setFloat2Value(const Vector2& value) = 0;

		virtual void setFloat2Array(const Vector2* arr, sl_uint32 n) = 0;

		virtual void setFloat3Value(const Vector3& value) = 0;

		virtual void setFloat3Array(const Vector3* arr, sl_uint32 n) = 0;

		virtual void setFloat4Value(const Vector4& value) = 0;

		virtual void setFloat4Array(const Vector4* arr, sl_uint32 n) = 0;

		virtual void setMatrix3Value(const Matrix3& value) = 0;

		virtual void setMatrix3Array(const Matrix3* arr, sl_uint32 n) = 0;

		virtual void setMatrix4Value(const Matrix4& value) = 0;

		virtual void setMatrix4Array(const Matrix4* arr, sl_uint32 n) = 0;

		virtual void setTexture(const Ref<Texture>& texture, sl_reg sampler = 0) = 0;

		virtual void setTextureArray(const Ref<Texture>* textures, const sl_reg* samplers, sl_uint32 n) = 0;

	};


	class SLIB_EXPORT RenderInputLayout : public Referable
	{
		SLIB_DECLARE_OBJECT

	public:
		RenderInputLayout();

		~RenderInputLayout();

	public:
		virtual void load() = 0;

		virtual void unload() = 0;

	};


	class SLIB_EXPORT RenderProgramState : public Referable
	{
		SLIB_DECLARE_OBJECT

	public:
		RenderProgramInstance* programInstance;

	public:
		RenderProgramState();
		
		~RenderProgramState();
		
		SLIB_DELETE_CLASS_DEFAULT_MEMBERS(RenderProgramState)
		
	public:
		Ref<RenderProgramConstant> getConstant(const char* name);

		Ref<RenderInputLayout> createInputLayout(sl_uint32 stride, const RenderProgramStateItem* items, sl_uint32 nItems);

	};


	class SLIB_EXPORT RenderProgramInstance : public RenderBaseObjectInstance
	{
		SLIB_DECLARE_OBJECT
		
	protected:
		RenderProgramInstance();
		
		~RenderProgramInstance();
		
	public:
		virtual Ref<RenderProgramConstant> getConstant(const char* name) = 0;
		
		virtual Ref<RenderInputLayout> createInputLayout(sl_uint32 stride, const RenderProgramStateItem* items, sl_uint32 nItems) = 0;

	};


	class SLIB_EXPORT RenderProgram : public RenderBaseObject
	{
		SLIB_DECLARE_OBJECT
		
	protected:
		RenderProgram();
		
		~RenderProgram();

	public:
		virtual Ref<RenderProgramState> onCreate(RenderEngine* engine) = 0;
		
		virtual sl_bool onInit(RenderEngine* engine, RenderProgramState* state);
		
		virtual sl_bool onPreRender(RenderEngine* engine, RenderProgramState* state);
		
		virtual void onPostRender(RenderEngine* engine, RenderProgramState* state);


		virtual String getGLSLVertexShader(RenderEngine* engine);
		
		virtual String getGLSLFragmentShader(RenderEngine* engine);

		virtual String getHLSLVertexShader(RenderEngine* engine);

		virtual String getHLSLPixelShader(RenderEngine* engine);


		Ref<RenderProgramInstance> getInstance(RenderEngine* engine);
		
	};


#define SLIB_RENDER_INPUT_ITEM(NAME, ...) \
	class T_##NAME : public slib::RenderProgramStateItem \
	{ \
	public: \
		SLIB_INLINE T_##NAME() : RenderProgramStateItem(__VA_ARGS__) {} \
	} NAME;

#define SLIB_RENDER_PROGRAM_STATE_BEGIN(TYPE, VERTEX_TYPE) \
	class TYPE : public slib::RenderProgramState \
	{ \
	public: \
		sl_uint32 vertexSize; \
		Ref<RenderInputLayout> inputLayout; \
		typedef VERTEX_TYPE VertexType; \
		SLIB_INLINE TYPE() : vertexSize(sizeof(VertexType)) {}

#define SLIB_RENDER_PROGRAM_STATE_END \
		SLIB_RENDER_INPUT_ITEM(_endOfInputs) \
	};

#define SLIB_RENDER_PROGRAM_STATE_UNIFORM_INT(NAME, SHADER_NAME) \
	SLIB_RENDER_INPUT_ITEM(NAME, #SHADER_NAME) \
	SLIB_INLINE void set##NAME(sl_int32 value) { if (NAME.uniform.isNotNull()) NAME.uniform->setIntValue(value); }

#define SLIB_RENDER_PROGRAM_STATE_UNIFORM_INT_ARRAY(NAME, SHADER_NAME) \
	SLIB_RENDER_INPUT_ITEM(NAME, #SHADER_NAME) \
	SLIB_INLINE void set##NAME(const sl_int32* values, sl_uint32 count) { if (NAME.uniform.isNotNull()) NAME.uniform->setIntArray(values, count); }

#define SLIB_RENDER_PROGRAM_STATE_UNIFORM_FLOAT(NAME, SHADER_NAME) \
	SLIB_RENDER_INPUT_ITEM(NAME, #SHADER_NAME) \
	SLIB_INLINE void set##NAME(float value) { if (NAME.uniform.isNotNull()) NAME.uniform->setFloatValue(value); }

#define SLIB_RENDER_PROGRAM_STATE_UNIFORM_FLOAT_ARRAY(NAME, SHADER_NAME) \
	SLIB_RENDER_INPUT_ITEM(NAME, #SHADER_NAME) \
	SLIB_INLINE void set##NAME(const float* values, sl_uint32 count) { if (NAME.uniform.isNotNull()) NAME.uniform->setFloatArray(values, count); }

#define SLIB_RENDER_PROGRAM_STATE_UNIFORM_VECTOR2(NAME, SHADER_NAME) \
	SLIB_RENDER_INPUT_ITEM(NAME, #SHADER_NAME) \
	SLIB_INLINE void set##NAME(const slib::Vector2& value) { if (NAME.uniform.isNotNull()) NAME.uniform->setFloat2Value(value); }

#define SLIB_RENDER_PROGRAM_STATE_UNIFORM_VECTOR2_ARRAY(NAME, SHADER_NAME) \
	SLIB_RENDER_INPUT_ITEM(NAME, #SHADER_NAME) \
	SLIB_INLINE void set##NAME(const slib::Vector2* values, sl_uint32 count) { if (NAME.uniform.isNotNull()) NAME.uniform->setFloat2Array(values, count); }

#define SLIB_RENDER_PROGRAM_STATE_UNIFORM_VECTOR3(NAME, SHADER_NAME) \
	SLIB_RENDER_INPUT_ITEM(NAME, #SHADER_NAME) \
	SLIB_INLINE void set##NAME(const slib::Vector3& value) { if (NAME.uniform.isNotNull()) NAME.uniform->setFloat3Value(value); }

#define SLIB_RENDER_PROGRAM_STATE_UNIFORM_VECTOR3_ARRAY(NAME, SHADER_NAME) \
	SLIB_RENDER_INPUT_ITEM(NAME, #SHADER_NAME) \
	SLIB_INLINE void set##NAME(const slib::Vector3* values, sl_uint32 count) { if (NAME.uniform.isNotNull()) NAME.uniform->setFloat3Array(values, count); }

#define SLIB_RENDER_PROGRAM_STATE_UNIFORM_VECTOR4(NAME, SHADER_NAME) \
	SLIB_RENDER_INPUT_ITEM(NAME, #SHADER_NAME) \
	SLIB_INLINE void set##NAME(const slib::Vector4& value) { if (NAME.uniform.isNotNull()) NAME.uniform->setFloat4Value(value); }

#define SLIB_RENDER_PROGRAM_STATE_UNIFORM_VECTOR4_ARRAY(NAME, SHADER_NAME) \
	SLIB_RENDER_INPUT_ITEM(NAME, #SHADER_NAME) \
	SLIB_INLINE void set##NAME(const slib::Vector4* values, sl_uint32 count) { if (NAME.uniform.isNotNull()) NAME.uniform->setFloat4Array(values, count); }

#define SLIB_RENDER_PROGRAM_STATE_UNIFORM_MATRIX3(NAME, SHADER_NAME) \
	SLIB_RENDER_INPUT_ITEM(NAME, #SHADER_NAME) \
	SLIB_INLINE void set##NAME(const slib::Matrix3& value) { if (NAME.uniform.isNotNull()) NAME.uniform->setMatrix3Value(value); }

#define SLIB_RENDER_PROGRAM_STATE_UNIFORM_MATRIX3_ARRAY(NAME, SHADER_NAME) \
	SLIB_RENDER_INPUT_ITEM(NAME, #SHADER_NAME) \
	SLIB_INLINE void set##NAME(const slib::Matrix3* values, sl_uint32 count) { if (NAME.uniform.isNotNull()) NAME.uniform->setMatrix3Array(values, count); }

#define SLIB_RENDER_PROGRAM_STATE_UNIFORM_MATRIX4(NAME, SHADER_NAME) \
	SLIB_RENDER_INPUT_ITEM(NAME, #SHADER_NAME) \
	SLIB_INLINE void set##NAME(const slib::Matrix4& value) { if (NAME.uniform.isNotNull()) NAME.uniform->setMatrix4Value(value); }

#define SLIB_RENDER_PROGRAM_STATE_UNIFORM_MATRIX4_ARRAY(NAME, SHADER_NAME) \
	SLIB_RENDER_INPUT_ITEM(NAME, #SHADER_NAME) \
	SLIB_INLINE void set##NAME(const slib::Matrix4* values, sl_uint32 count) { if (NAME.uniform.isNotNull()) NAME.uniform->setMatrix4Array(values, count); }

#define SLIB_RENDER_PROGRAM_STATE_UNIFORM_TEXTURE(NAME, SHADER_NAME) \
	SLIB_RENDER_INPUT_ITEM(NAME, #SHADER_NAME) \
	SLIB_INLINE void set##NAME(const Ref<slib::Texture>& texture) { if (NAME.uniform.isNotNull()) NAME.uniform->setTexture(texture); }

#define SLIB_RENDER_PROGRAM_STATE_UNIFORM_TEXTURE_ARRAY(NAME, SHADER_NAME) \
	SLIB_RENDER_INPUT_ITEM(NAME, #SHADER_NAME) \
	SLIB_INLINE void set##NAME(const Ref<slib::Texture>* textures, sl_uint32 count) { if (NAME.uniform.isNotNull()) NAME.uniform->setTextureArray(textures, count); }

#define SLIB_RENDER_PROGRAM_STATE_INPUT_FLOAT(MEMBER, SHADER_NAME, ...) \
	SLIB_RENDER_INPUT_ITEM(SHADER_NAME, #SHADER_NAME, RenderInputType::Float, (sl_uint32)(sl_size)(&(((VertexType*)0)->MEMBER)), ##__VA_ARGS__)

#define SLIB_RENDER_PROGRAM_STATE_INPUT_FLOAT2(MEMBER, SHADER_NAME, ...) \
	SLIB_RENDER_INPUT_ITEM(SHADER_NAME, #SHADER_NAME, RenderInputType::Float2, (sl_uint32)(sl_size)(&(((VertexType*)0)->MEMBER)), ##__VA_ARGS__)

#define SLIB_RENDER_PROGRAM_STATE_INPUT_FLOAT3(MEMBER, SHADER_NAME, ...) \
	SLIB_RENDER_INPUT_ITEM(SHADER_NAME, #SHADER_NAME, RenderInputType::Float3, (sl_uint32)(sl_size)(&(((VertexType*)0)->MEMBER)), ##__VA_ARGS__)

#define SLIB_RENDER_PROGRAM_STATE_INPUT_FLOAT4(MEMBER, SHADER_NAME, ...) \
	SLIB_RENDER_INPUT_ITEM(SHADER_NAME, #SHADER_NAME, RenderInputType::Float4, (sl_uint32)(sl_size)(&(((VertexType*)0)->MEMBER)), ##__VA_ARGS__)

#define SLIB_RENDER_PROGRAM_STATE_INPUT_UBYTE4(MEMBER, SHADER_NAME, ...) \
	SLIB_RENDER_INPUT_ITEM(SHADER_NAME, #SHADER_NAME, RenderInputType::UByte4, (sl_uint32)(sl_size)(&(((VertexType*)0)->MEMBER)), ##__VA_ARGS__)

#define SLIB_RENDER_PROGRAM_STATE_INPUT_SHORT2(MEMBER, SHADER_NAME, ...) \
	SLIB_RENDER_INPUT_ITEM(SHADER_NAME, #SHADER_NAME, RenderInputType::Short2, (sl_uint32)(sl_size)(&(((VertexType*)0)->MEMBER)), ##__VA_ARGS__)

#define SLIB_RENDER_PROGRAM_STATE_INPUT_SHORT4(MEMBER, SHADER_NAME, ...) \
	SLIB_RENDER_INPUT_ITEM(SHADER_NAME, #SHADER_NAME, RenderInputType::Short4, (sl_uint32)(sl_size)(&(((VertexType*)0)->MEMBER)), ##__VA_ARGS__)

	namespace priv
	{
		namespace render_program
		{

			class SLIB_EXPORT RenderProgramTemplate : public RenderProgram
			{
			public:
				sl_bool onInit(RenderEngine* engine, RenderProgramState* state) override;
				
				sl_bool onPreRender(RenderEngine* engine, RenderProgramState* state) override;
				
				void onPostRender(RenderEngine* engine, RenderProgramState* state) override;

			};

		}
	}


	template <class StateType>
	class SLIB_EXPORT RenderProgramT : public priv::render_program::RenderProgramTemplate
	{
	public:
		Ref<RenderProgramState> onCreate(RenderEngine* engine) override
		{
			return new StateType;
		}
		
	};


	struct RenderVertex2D_PositionTexture
	{
		Vector2 position;
		Vector2 texCoord;
	};

	SLIB_RENDER_PROGRAM_STATE_BEGIN(RenderProgramState2D_PositionTexture, RenderVertex2D_PositionTexture)
		SLIB_RENDER_PROGRAM_STATE_UNIFORM_MATRIX3(Transform, u_Transform)
		SLIB_RENDER_PROGRAM_STATE_UNIFORM_TEXTURE(Texture, u_Texture)
		SLIB_RENDER_PROGRAM_STATE_UNIFORM_MATRIX3(TextureTransform, u_TextureTransform)
		SLIB_RENDER_PROGRAM_STATE_UNIFORM_VECTOR4(Color, u_Color)

		SLIB_RENDER_PROGRAM_STATE_INPUT_FLOAT2(position, a_Position)
		SLIB_RENDER_PROGRAM_STATE_INPUT_FLOAT2(texCoord, a_TexCoord)
	SLIB_RENDER_PROGRAM_STATE_END

	class SLIB_EXPORT RenderProgram2D_PositionTexture : public RenderProgramT<RenderProgramState2D_PositionTexture>
	{
	public:
		String getGLSLVertexShader(RenderEngine* engine) override;
		
		String getGLSLFragmentShader(RenderEngine* engine) override;
		
	};

	class SLIB_EXPORT RenderProgram2D_PositionTextureYUV : public RenderProgram2D_PositionTexture
	{
	public:
		String getGLSLFragmentShader(RenderEngine* engine) override;
		
	};

	class SLIB_EXPORT RenderProgram2D_PositionTextureOES : public RenderProgram2D_PositionTexture
	{
	public:
		String getGLSLFragmentShader(RenderEngine* engine) override;
		
	};


	struct RenderVertex2D_PositionColor
	{
		Vector2 position;
		Color4f color;
	};

	SLIB_RENDER_PROGRAM_STATE_BEGIN(RenderProgramState2D_PositionColor, RenderVertex2D_PositionColor)
		SLIB_RENDER_PROGRAM_STATE_UNIFORM_MATRIX3(Transform, u_Transform)
		SLIB_RENDER_PROGRAM_STATE_UNIFORM_VECTOR4(Color, u_Color)

		SLIB_RENDER_PROGRAM_STATE_INPUT_FLOAT2(position, a_Position)
		SLIB_RENDER_PROGRAM_STATE_INPUT_FLOAT4(color, a_Color)
	SLIB_RENDER_PROGRAM_STATE_END

	class SLIB_EXPORT RenderProgram2D_PositionColor : public RenderProgramT<RenderProgramState2D_PositionColor>
	{
	public:
		String getGLSLVertexShader(RenderEngine* engine) override;
		
		String getGLSLFragmentShader(RenderEngine* engine) override;
		
	};


	struct RenderVertex2D_Position
	{
		Vector2 position;
	};

	SLIB_RENDER_PROGRAM_STATE_BEGIN(RenderProgramState2D_Position, RenderVertex2D_Position)
		SLIB_RENDER_PROGRAM_STATE_UNIFORM_MATRIX3(Transform, u_Transform)
		SLIB_RENDER_PROGRAM_STATE_UNIFORM_VECTOR4(Color, u_Color)

		SLIB_RENDER_PROGRAM_STATE_INPUT_FLOAT2(position, a_Position)
	SLIB_RENDER_PROGRAM_STATE_END

	class SLIB_EXPORT RenderProgram2D_Position : public RenderProgramT<RenderProgramState2D_Position>
	{
	public:
		String getGLSLVertexShader(RenderEngine* engine) override;
		
		String getGLSLFragmentShader(RenderEngine* engine) override;
		
	};


	struct RenderVertex3D_PositionNormalColor
	{
		Vector3 position;
		Vector3 normal;
		Color4f color;
	};

	SLIB_RENDER_PROGRAM_STATE_BEGIN(RenderProgramState3D_PositionNormalColor, RenderVertex3D_PositionNormalColor)
		SLIB_RENDER_PROGRAM_STATE_UNIFORM_MATRIX4(Transform, u_Transform)
		SLIB_RENDER_PROGRAM_STATE_UNIFORM_MATRIX4(MatrixModelViewIT, u_MatrixModelViewIT)
		SLIB_RENDER_PROGRAM_STATE_UNIFORM_VECTOR3(DirectionalLight, u_DirectionalLight)
		SLIB_RENDER_PROGRAM_STATE_UNIFORM_VECTOR3(DiffuseColor, u_DiffuseColor)
		SLIB_RENDER_PROGRAM_STATE_UNIFORM_VECTOR3(AmbientColor, u_AmbientColor)
		SLIB_RENDER_PROGRAM_STATE_UNIFORM_FLOAT(Alpha, u_Alpha)

		SLIB_RENDER_PROGRAM_STATE_INPUT_FLOAT3(position, a_Position)
		SLIB_RENDER_PROGRAM_STATE_INPUT_FLOAT3(normal, a_Normal)
		SLIB_RENDER_PROGRAM_STATE_INPUT_FLOAT4(color, a_Color)
	SLIB_RENDER_PROGRAM_STATE_END

	class SLIB_EXPORT RenderProgram3D_PositionNormalColor : public RenderProgramT<RenderProgramState3D_PositionNormalColor>
	{
	public:
		String getGLSLVertexShader(RenderEngine* engine) override;
		
		String getGLSLFragmentShader(RenderEngine* engine) override;
		
	};


	struct RenderVertex3D_PositionColor
	{
		Vector3 position;
		Color4f color;
	};

	SLIB_RENDER_PROGRAM_STATE_BEGIN(RenderProgramState3D_PositionColor, RenderVertex3D_PositionColor)
		SLIB_RENDER_PROGRAM_STATE_UNIFORM_MATRIX4(Transform, u_Transform)
		SLIB_RENDER_PROGRAM_STATE_UNIFORM_VECTOR4(Color, u_Color)

		SLIB_RENDER_PROGRAM_STATE_INPUT_FLOAT3(position, a_Position)
		SLIB_RENDER_PROGRAM_STATE_INPUT_FLOAT4(color, a_Color)
	SLIB_RENDER_PROGRAM_STATE_END

	class SLIB_EXPORT RenderProgram3D_PositionColor : public RenderProgramT<RenderProgramState3D_PositionColor>
	{
	public:
		String getGLSLVertexShader(RenderEngine* engine) override;
		
		String getGLSLFragmentShader(RenderEngine* engine) override;
		
	};


	struct RenderVertex3D_PositionNormalTexture
	{
		Vector3 position;
		Vector3 normal;
		Vector2 texCoord;
	};

	SLIB_RENDER_PROGRAM_STATE_BEGIN(RenderProgramState3D_PositionNormalTexture, RenderVertex3D_PositionNormalTexture)
		SLIB_RENDER_PROGRAM_STATE_UNIFORM_MATRIX4(Transform, u_Transform)
		SLIB_RENDER_PROGRAM_STATE_UNIFORM_MATRIX4(MatrixModelViewIT, u_MatrixModelViewIT)
		SLIB_RENDER_PROGRAM_STATE_UNIFORM_VECTOR3(DirectionalLight, u_DirectionalLight)
		SLIB_RENDER_PROGRAM_STATE_UNIFORM_VECTOR3(DiffuseColor, u_DiffuseColor)
		SLIB_RENDER_PROGRAM_STATE_UNIFORM_VECTOR3(AmbientColor, u_AmbientColor)
		SLIB_RENDER_PROGRAM_STATE_UNIFORM_FLOAT(Alpha, u_Alpha)
		SLIB_RENDER_PROGRAM_STATE_UNIFORM_TEXTURE(Texture, u_Texture)

		SLIB_RENDER_PROGRAM_STATE_INPUT_FLOAT3(position, a_Position)
		SLIB_RENDER_PROGRAM_STATE_INPUT_FLOAT3(normal, a_Normal)
		SLIB_RENDER_PROGRAM_STATE_INPUT_FLOAT2(texCoord, a_TexCoord)
	SLIB_RENDER_PROGRAM_STATE_END

	class SLIB_EXPORT RenderProgram3D_PositionNormalTexture : public RenderProgramT<RenderProgramState3D_PositionNormalTexture>
	{
	public:
		String getGLSLVertexShader(RenderEngine* engine) override;
		
		String getGLSLFragmentShader(RenderEngine* engine) override;
		
	};


	struct RenderVertex3D_PositionTexture
	{
		Vector3 position;
		Vector2 texCoord;
	};

	SLIB_RENDER_PROGRAM_STATE_BEGIN(RenderProgramState3D_PositionTexture, RenderVertex3D_PositionTexture)
		SLIB_RENDER_PROGRAM_STATE_UNIFORM_MATRIX4(Transform, u_Transform)
		SLIB_RENDER_PROGRAM_STATE_UNIFORM_VECTOR3(DiffuseColor, u_Color)
		SLIB_RENDER_PROGRAM_STATE_UNIFORM_TEXTURE(Texture, u_Texture)

		SLIB_RENDER_PROGRAM_STATE_INPUT_FLOAT3(position, a_Position)
		SLIB_RENDER_PROGRAM_STATE_INPUT_FLOAT2(texCoord, a_TexCoord)
	SLIB_RENDER_PROGRAM_STATE_END

	class SLIB_EXPORT RenderProgram3D_PositionTexture : public RenderProgramT<RenderProgramState3D_PositionTexture>
	{
	public:
		String getGLSLVertexShader(RenderEngine* engine) override;
		
		String getGLSLFragmentShader(RenderEngine* engine) override;
		
	};


	struct RenderVertex3D_PositionNormal
	{
		Vector3 position;
		Vector3 normal;
	};

	SLIB_RENDER_PROGRAM_STATE_BEGIN(RenderProgramState3D_PositionNormal, RenderVertex3D_PositionNormal)
		SLIB_RENDER_PROGRAM_STATE_UNIFORM_MATRIX4(Transform, u_Transform)
		SLIB_RENDER_PROGRAM_STATE_UNIFORM_MATRIX4(MatrixModelViewIT, u_MatrixModelViewIT)
		SLIB_RENDER_PROGRAM_STATE_UNIFORM_VECTOR3(DirectionalLight, u_DirectionalLight)
		SLIB_RENDER_PROGRAM_STATE_UNIFORM_VECTOR3(DiffuseColor, u_DiffuseColor)
		SLIB_RENDER_PROGRAM_STATE_UNIFORM_VECTOR3(AmbientColor, u_AmbientColor)
		SLIB_RENDER_PROGRAM_STATE_UNIFORM_FLOAT(Alpha, u_Alpha)

		SLIB_RENDER_PROGRAM_STATE_INPUT_FLOAT3(position, a_Position)
		SLIB_RENDER_PROGRAM_STATE_INPUT_FLOAT3(normal, a_Normal)
	SLIB_RENDER_PROGRAM_STATE_END

	class SLIB_EXPORT RenderProgram3D_PositionNormal : public RenderProgramT<RenderProgramState3D_PositionNormal>
	{
	public:
		String getGLSLVertexShader(RenderEngine* engine) override;
		
		String getGLSLFragmentShader(RenderEngine* engine) override;
		
	};


	struct RenderVertex3D_Position
	{
		Vector3 position;
	};

	SLIB_RENDER_PROGRAM_STATE_BEGIN(RenderProgramState3D_Position, RenderVertex3D_Position)
		SLIB_RENDER_PROGRAM_STATE_UNIFORM_MATRIX4(Transform, u_Transform)
		SLIB_RENDER_PROGRAM_STATE_UNIFORM_VECTOR4(Color, u_Color)

		SLIB_RENDER_PROGRAM_STATE_INPUT_FLOAT3(position, a_Position)
	SLIB_RENDER_PROGRAM_STATE_END

	class SLIB_EXPORT RenderProgram3D_Position : public RenderProgramT<RenderProgramState3D_Position>
	{
	public:
		String getGLSLVertexShader(RenderEngine* engine) override;
		
		String getGLSLFragmentShader(RenderEngine* engine) override;
		
	};

}

#endif

