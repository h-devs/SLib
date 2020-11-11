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

#include "../math/matrix4.h"
#include "../graphics/color.h"

namespace slib
{

	class Primitive;
	class RenderEngine;
	class RenderProgram;
	class RenderProgramInstance;
	class Texture;

	enum class RenderProgramStateKind
	{
		None = 0,
		Uniform = 1,
		Input = 2
	};

	enum class RenderUniformType
	{
		Float = 0,
		Float2 = 1,
		Float3 = 2,
		Float4 = 4,
		Int = 0x10,
		Int2 = 0x11,
		Int3 = 0x12,
		Int4 = 0x13,
		Matrix3 = 0x21,
		Matrix4 = 0x22,
		Sampler = 0x30
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
		Depth = 12
	};

	struct RenderInputDesc
	{
		RenderInputType type;
		sl_uint32 offset;
		RenderInputSemanticName semanticName;
		sl_uint32 semanticIndex;
		sl_uint32 slot;
	};

	enum class RenderShaderType
	{
		Undefined = 0,
		Vertex = 1,
		Pixel = 2
	};

	struct RenderUniformLocation
	{
		RenderShaderType shader;
		sl_reg location;
	};

	class SLIB_EXPORT RenderProgramStateItem
	{
	public:
		const char* name;
		RenderProgramStateKind kind;

		RenderUniformLocation uniform;
		RenderInputDesc input;

	public:
		// None
		RenderProgramStateItem();

		// Uniform
		RenderProgramStateItem(const char* name, sl_reg uniformLocation = -1, RenderShaderType type = RenderShaderType::Undefined);

		// Input
		RenderProgramStateItem(const char* name, RenderInputType type, sl_uint32 offset, RenderInputSemanticName semanticName = RenderInputSemanticName::Undefined, sl_uint32 semanticIndex = 0, sl_uint32 slot = 0);

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(RenderProgramStateItem)

	};

	struct RenderInputLayoutItem : RenderInputDesc
	{
		const char* name;
	};

	class SLIB_EXPORT RenderInputLayoutParam
	{
	public:
		ListParam<sl_uint32> strides; // per slot
		ListParam<RenderInputLayoutItem> items;

	public:
		RenderInputLayoutParam();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(RenderInputLayoutParam)

	};


	class SLIB_EXPORT RenderInputLayout : public Referable
	{
		SLIB_DECLARE_OBJECT

	public:
		RenderInputLayout();

		~RenderInputLayout();

	};


	class SLIB_EXPORT RenderProgramState : public Referable
	{
		SLIB_DECLARE_OBJECT

	public:
		RenderProgramState();
		
		~RenderProgramState();
		
		SLIB_DELETE_CLASS_DEFAULT_MEMBERS(RenderProgramState)
		
	public:
		RenderProgramInstance* getProgramInstance();

		void setProgramInstance(RenderProgramInstance* instance);

		RenderInputLayout* getInputLayout();

		void updateInputLayout(RenderProgram* program, sl_bool forceUpdate = sl_false);

		sl_bool getUniformLocation(const char* name, RenderUniformLocation* outLocation);

		void setUniform(const RenderUniformLocation& location, RenderUniformType type, const void* data, sl_uint32 nItems);

		void setFloatValue(const RenderUniformLocation& location, float value);

		void setFloatArray(const RenderUniformLocation& location, const float* arr, sl_uint32 n);

		void setIntValue(const RenderUniformLocation& location, sl_int32 value);

		void setIntArray(const RenderUniformLocation& location, const sl_int32* value, sl_uint32 n);

		void setFloat2Value(const RenderUniformLocation& location, const Vector2& value);

		void setFloat2Array(const RenderUniformLocation& location, const Vector2* arr, sl_uint32 n);

		void setFloat3Value(const RenderUniformLocation& location, const Vector3& value);

		void setFloat3Array(const RenderUniformLocation& location, const Vector3* arr, sl_uint32 n);

		void setFloat4Value(const RenderUniformLocation& location, const Vector4& value);

		void setFloat4Array(const RenderUniformLocation& location, const Vector4* arr, sl_uint32 n);

		void setMatrix3Value(const RenderUniformLocation& location, const Matrix3& value);

		void setMatrix3Array(const RenderUniformLocation& location, const Matrix3* arr, sl_uint32 n);

		void setMatrix4Value(const RenderUniformLocation& location, const Matrix4& value);

		void setMatrix4Array(const RenderUniformLocation& location, const Matrix4* arr, sl_uint32 n);

		void setSampler(const RenderUniformLocation& location, const Ref<Texture>& texture, sl_reg sampler = 0);

	protected:
		RenderProgramInstance* m_programInstance;
		Ref<RenderInputLayout> m_inputLayout;

	};


	class SLIB_EXPORT RenderProgramInstance : public RenderBaseObjectInstance
	{
		SLIB_DECLARE_OBJECT
		
	protected:
		RenderProgramInstance();
		
		~RenderProgramInstance();
		
	public:
		virtual Ref<RenderInputLayout> createInputLayout(const RenderInputLayoutParam& param) = 0;

		virtual sl_bool getUniformLocation(const char* name, RenderUniformLocation* outLocation) = 0;

		virtual void setUniform(const RenderUniformLocation& location, RenderUniformType type, const void* data, sl_uint32 nItems) = 0;

	protected:
		Ref<RenderInputLayout> m_inputLayout;

	};


	class SLIB_EXPORT RenderProgram : public RenderBaseObject
	{
		SLIB_DECLARE_OBJECT
		
	protected:
		RenderProgram();
		
		~RenderProgram();

	public:
		virtual Ref<RenderProgramState> onCreate(RenderEngine* engine) = 0;
		
		virtual sl_bool onInit(RenderEngine* engine, RenderProgramInstance* instance, RenderProgramState* state);
		
		virtual sl_bool onPreRender(RenderEngine* engine, RenderProgramInstance* instance, RenderProgramState* state);
		
		virtual void onPostRender(RenderEngine* engine, RenderProgramInstance* instance, RenderProgramState* state);

		virtual sl_bool getInputLayoutParam(RenderProgramState* state, RenderInputLayoutParam& param);
		

		virtual String getGLSLVertexShader(RenderEngine* engine);
		
		virtual String getGLSLFragmentShader(RenderEngine* engine);

		virtual String getHLSLVertexShader(RenderEngine* engine);

		virtual Memory getHLSLCompiledVertexShader(RenderEngine* engine);

		virtual String getHLSLPixelShader(RenderEngine* engine);

		virtual Memory getHLSLCompiledPixelShader(RenderEngine* engine);


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
		List<RenderInputLayoutItem> inputLayout; \
		typedef VERTEX_TYPE VertexType; \
		SLIB_INLINE TYPE() : vertexSize(sizeof(VertexType)) {}

#define SLIB_RENDER_PROGRAM_STATE_END \
		SLIB_RENDER_INPUT_ITEM(_endOfInputs) \
	};

#define SLIB_RENDER_PROGRAM_STATE_UNIFORM_INT(NAME, SHADER_NAME, ...) \
	SLIB_RENDER_INPUT_ITEM(NAME, #SHADER_NAME, ##__VA_ARGS__) \
	SLIB_INLINE void set##NAME(sl_int32 value) { if (NAME.uniform.location >= 0) setIntValue(NAME.uniform, value); }

#define SLIB_RENDER_PROGRAM_STATE_UNIFORM_INT_ARRAY(NAME, SHADER_NAME, ...) \
	SLIB_RENDER_INPUT_ITEM(NAME, #SHADER_NAME, ##__VA_ARGS__) \
	SLIB_INLINE void set##NAME(const sl_int32* values, sl_uint32 count) { if (NAME.uniform.location >= 0) setIntArray(NAME.uniform, values, count); }

#define SLIB_RENDER_PROGRAM_STATE_UNIFORM_FLOAT(NAME, SHADER_NAME, ...) \
	SLIB_RENDER_INPUT_ITEM(NAME, #SHADER_NAME, ##__VA_ARGS__) \
	SLIB_INLINE void set##NAME(float value) { if (NAME.uniform.location >= 0) setFloatValue(NAME.uniform, value); }

#define SLIB_RENDER_PROGRAM_STATE_UNIFORM_FLOAT_ARRAY(NAME, SHADER_NAME, ...) \
	SLIB_RENDER_INPUT_ITEM(NAME, #SHADER_NAME, ##__VA_ARGS__) \
	SLIB_INLINE void set##NAME(const float* values, sl_uint32 count) { if (NAME.uniform.location >= 0) setFloatArray(NAME.uniform, values, count); }

#define SLIB_RENDER_PROGRAM_STATE_UNIFORM_VECTOR2(NAME, SHADER_NAME, ...) \
	SLIB_RENDER_INPUT_ITEM(NAME, #SHADER_NAME, ##__VA_ARGS__) \
	SLIB_INLINE void set##NAME(const slib::Vector2& value) { if (NAME.uniform.location >= 0) setFloat2Value(NAME.uniform, value); }

#define SLIB_RENDER_PROGRAM_STATE_UNIFORM_VECTOR2_ARRAY(NAME, SHADER_NAME, ...) \
	SLIB_RENDER_INPUT_ITEM(NAME, #SHADER_NAME, ##__VA_ARGS__) \
	SLIB_INLINE void set##NAME(const slib::Vector2* values, sl_uint32 count) { if (NAME.uniform.location >= 0) setFloat2Array(NAME.uniform, values, count); }

#define SLIB_RENDER_PROGRAM_STATE_UNIFORM_VECTOR3(NAME, SHADER_NAME, ...) \
	SLIB_RENDER_INPUT_ITEM(NAME, #SHADER_NAME, ##__VA_ARGS__) \
	SLIB_INLINE void set##NAME(const slib::Vector3& value) { if (NAME.uniform.location >= 0) setFloat3Value(NAME.uniform, value); }

#define SLIB_RENDER_PROGRAM_STATE_UNIFORM_VECTOR3_ARRAY(NAME, SHADER_NAME, ...) \
	SLIB_RENDER_INPUT_ITEM(NAME, #SHADER_NAME, ##__VA_ARGS__) \
	SLIB_INLINE void set##NAME(const slib::Vector3* values, sl_uint32 count) { if (NAME.uniform.location >= 0) setFloat3Array(NAME.uniform, values, count); }

#define SLIB_RENDER_PROGRAM_STATE_UNIFORM_VECTOR4(NAME, SHADER_NAME, ...) \
	SLIB_RENDER_INPUT_ITEM(NAME, #SHADER_NAME, ##__VA_ARGS__) \
	SLIB_INLINE void set##NAME(const slib::Vector4& value) { if (NAME.uniform.location >= 0) setFloat4Value(NAME.uniform, value); }

#define SLIB_RENDER_PROGRAM_STATE_UNIFORM_VECTOR4_ARRAY(NAME, SHADER_NAME, ...) \
	SLIB_RENDER_INPUT_ITEM(NAME, #SHADER_NAME, ##__VA_ARGS__) \
	SLIB_INLINE void set##NAME(const slib::Vector4* values, sl_uint32 count) { if (NAME.uniform.location >= 0) setFloat4Array(NAME.uniform, values, count); }

#define SLIB_RENDER_PROGRAM_STATE_UNIFORM_MATRIX3(NAME, SHADER_NAME, ...) \
	SLIB_RENDER_INPUT_ITEM(NAME, #SHADER_NAME, ##__VA_ARGS__) \
	SLIB_INLINE void set##NAME(const slib::Matrix3& value) { if (NAME.uniform.location >= 0) setMatrix3Value(NAME.uniform, value); }

#define SLIB_RENDER_PROGRAM_STATE_UNIFORM_MATRIX3_ARRAY(NAME, SHADER_NAME, ...) \
	SLIB_RENDER_INPUT_ITEM(NAME, #SHADER_NAME, ##__VA_ARGS__) \
	SLIB_INLINE void set##NAME(const slib::Matrix3* values, sl_uint32 count) { if (NAME.uniform.location >= 0) setMatrix3Array(NAME.uniform, values, count); }

#define SLIB_RENDER_PROGRAM_STATE_UNIFORM_MATRIX4(NAME, SHADER_NAME, ...) \
	SLIB_RENDER_INPUT_ITEM(NAME, #SHADER_NAME, ##__VA_ARGS__) \
	SLIB_INLINE void set##NAME(const slib::Matrix4& value) { if (NAME.uniform.location >= 0) setMatrix4Value(NAME.uniform, value); }

#define SLIB_RENDER_PROGRAM_STATE_UNIFORM_MATRIX4_ARRAY(NAME, SHADER_NAME, ...) \
	SLIB_RENDER_INPUT_ITEM(NAME, #SHADER_NAME, ##__VA_ARGS__) \
	SLIB_INLINE void set##NAME(const slib::Matrix4* values, sl_uint32 count) { if (NAME.uniform.location >= 0) setMatrix4Array(NAME.uniform, values, count); }

#define SLIB_RENDER_PROGRAM_STATE_UNIFORM_TEXTURE(NAME, SHADER_NAME, ...) \
	SLIB_RENDER_INPUT_ITEM(NAME, #SHADER_NAME, ##__VA_ARGS__) \
	SLIB_INLINE void set##NAME(const Ref<slib::Texture>& texture) { if (NAME.uniform.location >= 0) setSampler(NAME.uniform, texture); }

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
				sl_bool onInit(RenderEngine* engine, RenderProgramInstance* instance, RenderProgramState* state) override;
				
				sl_bool getInputLayoutParam(RenderProgramState* state, RenderInputLayoutParam& param) override;

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

