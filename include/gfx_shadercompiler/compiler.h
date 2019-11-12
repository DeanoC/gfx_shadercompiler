#pragma once

#include "al2o3_vfile/vfile.h"

typedef enum ShaderCompiler_Language {
	ShaderCompiler_LANG_HLSL,
	ShaderCompiler_LANG_GLSL
} ShaderCompiler_Language;

// not all types of shader will be supported on all languages etc.
typedef enum ShaderCompiler_ShaderType {
	ShaderCompiler_ST_VertexShader,
	ShaderCompiler_ST_FragmentShader,
	ShaderCompiler_ST_ComputeShader,

	ShaderCompiler_ST_GeometryShader,
	ShaderCompiler_ST_TessControlShader,
	ShaderCompiler_ST_TessEvaluationShader,

	// its unclear whether these always become compute shaders as most backends
	// don't differ but glslang frontend does so...
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
	ShaderCompiler_OPT_Size, 					// ShaderConductor don't support size will equal P3
	ShaderCompiler_OPT_Performance0, 	// khronos just has a single performance level
	ShaderCompiler_OPT_Performance1,
	ShaderCompiler_OPT_Performance2,
	ShaderCompiler_OPT_Performance3,
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
	void const *shader;
	char const *log;
} ShaderCompiler_Output;

// stand alone compile function for simple one offs compile
AL2O3_EXTERN_C bool ShaderCompiler_CompileShader(
		ShaderCompiler_Language language,
		ShaderCompiler_ShaderType shaderType,
		char const *name,
		char const *entryPoint,
		VFile_Handle file,
		ShaderCompiler_Optimizations optimizations,
		ShaderCompiler_OutputType outputType,
		uint32_t outputVersion,
		ShaderCompiler_Output *output);

typedef struct ShaderCompiler_Context *ShaderCompiler_ContextHandle;

// creates a shader compiler context with defaults of HLSL, Perfomance 2 and
// output set to the platform default renderer type
AL2O3_EXTERN_C ShaderCompiler_ContextHandle ShaderCompiler_Create();
AL2O3_EXTERN_C void ShaderCompiler_Destroy(ShaderCompiler_ContextHandle handle);

AL2O3_EXTERN_C void ShaderCompiler_SetLanguage(ShaderCompiler_ContextHandle handle, ShaderCompiler_Language language);
AL2O3_EXTERN_C void ShaderCompiler_SetOutput(ShaderCompiler_ContextHandle handle,
																						 ShaderCompiler_OutputType output,
																						 uint32_t outputVersion);

// set the outputVersion to 0 to let the compiler pick a reasonable value (SM6_0)
AL2O3_EXTERN_C void ShaderCompiler_SetOptimizationLevel(ShaderCompiler_ContextHandle handle,
																												ShaderCompiler_Optimizations level);

//AL2O3_EXTERN_C void ShaderCompiler_AddSystemHeader(ShaderCompiler_ContextHandle handle, char const * text, uint32_t textSize);
//AL2O3_EXTERN_C void ShaderCompiler_AddHeaderCallback(ShaderCompiler_ContextHandle handle);

AL2O3_EXTERN_C bool ShaderCompiler_Compile(
		ShaderCompiler_ContextHandle handle,
		ShaderCompiler_ShaderType type,
		char const *name,
		char const *entryPoint,
		VFile_Handle file,
		ShaderCompiler_Output *output
);
