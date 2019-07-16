#pragma once
#ifndef GFX_SHADERCOMPILER_COMPILER_H_
#define GFX_SHADERCOMPILER_COMPILER_H_

typedef enum ShaderCompiler_Language {
	ShaderCompiler_LANG_HLSL,
	ShaderCompiler_LANG_GLSL
} ShaderCompiler_Language;

// not all types of shader will be supported on all languages etc.
typedef enum ShaderCompiler_ShaderType
{
	ShaderCompiler_ST_VertexShader,
	ShaderCompiler_ST_FragmentShader,
	ShaderCompiler_ST_ComputeShader,

	ShaderCompiler_ST_GeometryShader,
	ShaderCompiler_ST_TessControlShader,
	ShaderCompiler_ST_TessEvaluationShader,

	// its unclear whether these always becomr compute shaders as most backends
	// dodn't differ but glslang frontend does so...
	ShaderCompiler_ST_RaygenShader,
	ShaderCompiler_ST_AnyHitShader,
	ShaderCompiler_ST_ClosestHitShader,
	ShaderCompiler_ST_MissShader,
	ShaderCompiler_ST_IntersectionShader,
	ShaderCompiler_ST_CallableShader,
	ShaderCompiler_ST_TaskShader,
	ShaderCompiler_ST_MeshShader,

} ShaderCompiler_ShaderType;

typedef enum ShaderCompiler_Optimizations {
	ShaderCompiler_OPT_None,
	ShaderCompiler_OPT_Size,
	ShaderCompiler_OPT_Performance,
} ShaderCompiler_Optimizations;

typedef enum ShaderCompiler_OutputType {
	ShaderCompiler_OT_SPIRV,
	ShaderCompiler_OT_DXIL,
	ShaderCompiler_OT_HLSL,
	ShaderCompiler_OT_GLSL,
	ShaderCompiler_OT_MSL_OSX,
	ShaderCompiler_OT_MSL_IOS,
} ShaderCompiler_OutputType;

// both shader and log must be freed by the caller if not null
typedef struct ShaderCompiler_Output {
	uint64_t shaderSize;
	void const* shader;
	char const* log;
} ShaderCompiler_Output;

AL2O3_EXTERN_C bool ShaderCompiler_CompileShader(
		char const* name,
		ShaderCompiler_Language language,
		ShaderCompiler_ShaderType shaderType,
		char const* src,
		char const* entryPoint,
		ShaderCompiler_Optimizations optimizations,
		ShaderCompiler_OutputType outputType,
		ShaderCompiler_Output* output
		);
#endif