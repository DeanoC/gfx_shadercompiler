#include "al2o3_platform/platform.h"
#include "al2o3_memory/memory.h"
#include "gfx_shadercompiler/compiler.h"
#include "shaderc/shaderc.h"
#include "shaderc/spvc.h"
#include "ShaderConductor/ShaderConductor.hpp"
#include "al2o3_vfile/memory.h"

static shaderc_source_language KhrLanguageConverter(ShaderCompiler_Language sourceLanguage) {
	switch (sourceLanguage) {
	case ShaderCompiler_LANG_HLSL: return shaderc_source_language_hlsl;
	case ShaderCompiler_LANG_GLSL: return shaderc_source_language_glsl;
	}
	return shaderc_source_language_glsl;
}
static shaderc_optimization_level KhrOptimizationConverter(ShaderCompiler_Optimizations optimizations) {
	switch (optimizations) {
	case ShaderCompiler_OPT_None: return shaderc_optimization_level_zero;
	case ShaderCompiler_OPT_Size: return shaderc_optimization_level_size;
	case ShaderCompiler_OPT_Performance0: return shaderc_optimization_level_performance;
	case ShaderCompiler_OPT_Performance1: return shaderc_optimization_level_performance;
	case ShaderCompiler_OPT_Performance2: return shaderc_optimization_level_performance;
	case ShaderCompiler_OPT_Performance3: return shaderc_optimization_level_performance;
	}
	return shaderc_optimization_level_zero;
}
static shaderc_shader_kind KhrTypeConverter(ShaderCompiler_ShaderType type) {
	switch (type) {
	case ShaderCompiler_ST_VertexShader: return shaderc_vertex_shader;
	case ShaderCompiler_ST_FragmentShader: return shaderc_fragment_shader;
	case ShaderCompiler_ST_ComputeShader: return shaderc_compute_shader;
	case ShaderCompiler_ST_GeometryShader: return shaderc_geometry_shader;
	case ShaderCompiler_ST_TessControlShader: return shaderc_tess_control_shader;
	case ShaderCompiler_ST_TessEvaluationShader: return shaderc_tess_evaluation_shader;
	case ShaderCompiler_ST_RaygenShader: return shaderc_raygen_shader;
	case ShaderCompiler_ST_AnyHitShader: return shaderc_anyhit_shader;
	case ShaderCompiler_ST_ClosestHitShader: return shaderc_closesthit_shader;
	case ShaderCompiler_ST_MissShader: return shaderc_miss_shader;
	case ShaderCompiler_ST_IntersectionShader: return shaderc_intersection_shader;
	case ShaderCompiler_ST_CallableShader: return shaderc_callable_shader;
	case ShaderCompiler_ST_TaskShader: return shaderc_task_shader;
	case ShaderCompiler_ST_MeshShader: return shaderc_mesh_shader;
	}
	return shaderc_vertex_shader;
}
static ShaderConductor::ShaderStage SCShaderStageConvertor(ShaderCompiler_ShaderType type) {
	switch (type) {

	case ShaderCompiler_ST_VertexShader: return ShaderConductor::ShaderStage::VertexShader;
	case ShaderCompiler_ST_FragmentShader: return ShaderConductor::ShaderStage::PixelShader;
	case ShaderCompiler_ST_ComputeShader: return ShaderConductor::ShaderStage::ComputeShader;
	case ShaderCompiler_ST_GeometryShader: return ShaderConductor::ShaderStage::GeometryShader;
	case ShaderCompiler_ST_TessControlShader: return ShaderConductor::ShaderStage::HullShader;
	case ShaderCompiler_ST_TessEvaluationShader: return ShaderConductor::ShaderStage::DomainShader;

	case ShaderCompiler_ST_RaygenShader:
	case ShaderCompiler_ST_AnyHitShader:
	case ShaderCompiler_ST_ClosestHitShader:
	case ShaderCompiler_ST_MissShader:
	case ShaderCompiler_ST_IntersectionShader:
	case ShaderCompiler_ST_CallableShader:
	case ShaderCompiler_ST_TaskShader:
	case ShaderCompiler_ST_MeshShader: return ShaderConductor::ShaderStage::ComputeShader;
	}
	return ShaderConductor::ShaderStage::ComputeShader;
}

static ShaderConductor::ShadingLanguage ScOutputTypeConvertor(ShaderCompiler_OutputType type) {
	switch (type) {

	case ShaderCompiler_OT_SPIRV: return ShaderConductor::ShadingLanguage::SpirV;
	case ShaderCompiler_OT_DXIL:return ShaderConductor::ShadingLanguage::Dxil;
	case ShaderCompiler_OT_HLSL: return ShaderConductor::ShadingLanguage::Hlsl;
	case ShaderCompiler_OT_GLSL: return ShaderConductor::ShadingLanguage::SpirV;
	case ShaderCompiler_OT_MSL_OSX: return ShaderConductor::ShadingLanguage::Msl_macOS;
	case ShaderCompiler_OT_MSL_IOS: return ShaderConductor::ShadingLanguage::Msl_iOS;
	}
	return ShaderConductor::ShadingLanguage::SpirV;
}
static void ScOptimizationConverter(ShaderCompiler_Optimizations optimizations, ShaderConductor::Compiler::Options& options) {
	switch (optimizations) {
	case ShaderCompiler_OPT_None:
		options.optimizationLevel = 0;
		options.disableOptimizations = true;
		options.enableDebugInfo = true;
		break;
	case ShaderCompiler_OPT_Performance0:
		options.optimizationLevel = 0;
		options.disableOptimizations = false;
		options.enableDebugInfo = false;
		break;
	case ShaderCompiler_OPT_Performance1:
		options.optimizationLevel = 1;
		options.disableOptimizations = false;
		options.enableDebugInfo = false;
		break;
	case ShaderCompiler_OPT_Performance2:
		options.optimizationLevel = 2;
		options.disableOptimizations = false;
		options.enableDebugInfo = false;
		break;
	case ShaderCompiler_OPT_Size:
	case ShaderCompiler_OPT_Performance3:
		options.optimizationLevel = 3;
		options.disableOptimizations = false;
		options.enableDebugInfo = false;
		break;

	}
}
static char const *CopyString(char const *msg) {
	size_t const msgSize = strlen(msg);
	char *log = (char *) MEMORY_MALLOC(msgSize + 1);
	memcpy((void *) log, (void *) msg, msgSize);
	log[msgSize] = 0;
	return log;
}
static char const *CopyString(char const *msg, size_t const msgSize) {
	char *log = (char *) MEMORY_MALLOC(msgSize + 1);
	memcpy((void *) log, (void *) msg, msgSize);
	log[msgSize] = 0;
	return log;
}

typedef struct ShaderCompiler_Context {
	ShaderCompiler_Language inputLanguage;
	ShaderCompiler_OutputType outputType;

	// shader conductor settings
	ShaderConductor::Compiler::Options scOptions;
	ShaderConductor::Compiler::TargetDesc scTarget;

	// khronos settings
	shaderc_compiler_t khrCompiler;
	shaderc_compile_options_t khrOptions;
	shaderc_spvc_compiler_t khrSpvcCompiler;
	shaderc_spvc_compile_options_t khrSpvcOptions;

} ShaderCompiler_Context;

static bool CompileShaderKhronos(
		ShaderCompiler_Context *ctx,
		ShaderCompiler_ShaderType shaderType,
		char const *name,
		char const *entryPoint,
		char const *src,
		ShaderCompiler_Output *output
) {
	// there are two phases, HLSL/GLSL to SPIRV then SPIRV -> HLSL, MSL, GLSL
	memset(output, 0, sizeof(ShaderCompiler_Output));

	shaderc_shader_kind kind = KhrTypeConverter(shaderType);
	shaderc_compilation_result_t result = shaderc_compile_into_spv(ctx->khrCompiler,
																																 src,
																																 strlen(src),
																																 kind,
																																 name,
																																 entryPoint,
																																 ctx->khrOptions);

	if (shaderc_result_get_compilation_status(result) != shaderc_compilation_status_success) {
		// don't work return error message
		output->log = CopyString(shaderc_result_get_error_message(result));
		shaderc_result_release(result);
		return false;
	}

	// copy any warnings over but not fatal
	if (shaderc_result_get_num_warnings(result)) {
		output->log = CopyString(shaderc_result_get_error_message(result));
	}

	// if we are going to SPIRV we are done
	if (ctx->outputType == ShaderCompiler_OT_SPIRV) {
		size_t const size = shaderc_result_get_length(result);
		output->shader = MEMORY_MALLOC(size);
		memcpy((void *) output->shader, (void *) shaderc_result_get_bytes(result), size);
		output->shaderSize = size;
		return true;
	}

	shaderc_spvc_compile_options_set_entry_point(ctx->khrSpvcOptions, entryPoint);

	shaderc_spvc_compilation_result_t oresult = nullptr;

	uint32_t const *spirv = (uint32_t const *) shaderc_result_get_bytes(result);
	size_t const spirvSize = shaderc_result_get_length(result) / 4;

	switch (ctx->outputType) {
	case ShaderCompiler_OT_GLSL: oresult = shaderc_spvc_compile_into_glsl(ctx->khrSpvcCompiler,
																																				spirv,
																																				spirvSize,
																																				ctx->khrSpvcOptions);
		break;
	case ShaderCompiler_OT_HLSL: oresult = shaderc_spvc_compile_into_hlsl(ctx->khrSpvcCompiler,
																																				spirv,
																																				spirvSize,
																																				ctx->khrSpvcOptions);
		break;
	case ShaderCompiler_OT_MSL_OSX:
	case ShaderCompiler_OT_MSL_IOS: oresult = shaderc_spvc_compile_into_msl(ctx->khrSpvcCompiler,
																																					spirv,
																																					spirvSize,
																																					ctx->khrSpvcOptions);
		break;
	default:
	case ShaderCompiler_OT_SPIRV: // already processed
	ASSERT(false);
	}

	bool ret = false;
	if (shaderc_spvc_result_get_status(oresult) == shaderc_compilation_status_success) {
		output->shader = CopyString(shaderc_spvc_result_get_output(oresult));
		output->shaderSize = strlen((char *) output->shader) + 1;
		ret = true;
	}

	shaderc_spvc_result_release(oresult);
	return ret;
}

static bool CompileShaderShaderConductor(
		ShaderCompiler_Context *ctx,
		ShaderCompiler_ShaderType shaderType,
		char const *name,
		char const *entryPoint,
		char const *src,
		ShaderCompiler_Output *output
) {
	using namespace ShaderConductor;
	memset(output, 0, sizeof(ShaderCompiler_Output));

	Compiler::SourceDesc source{};
	source.fileName = name;
	source.source = src;
	source.stage = SCShaderStageConvertor(shaderType);
	source.entryPoint = entryPoint;
	source.numDefines = 0; // TODO

	try {
		auto result = Compiler::Compile(source, ctx->scOptions, ctx->scTarget);

		if (result.hasError) {
			output->log = CopyString((char *) result.errorWarningMsg->Data(), result.errorWarningMsg->Size());
			DestroyBlob(result.errorWarningMsg);
			return false;
		}
		if (result.errorWarningMsg != nullptr) {
			output->log = CopyString((char *) result.errorWarningMsg->Data(), result.errorWarningMsg->Size());
			DestroyBlob(result.errorWarningMsg);
		}

		size_t const size = result.target->Size() + (result.isText ? 1 : 0);
		output->shader = MEMORY_MALLOC(size);
		memcpy((void *) output->shader, result.target->Data(), size);
		if(result.isText) ((char*)output->shader)[size-1] = 0;

		output->shaderSize = size;

		DestroyBlob(result.target);
	} catch (std::exception const &e) {
		LOGERROR(e.what());
	}
	return true;
}

AL2O3_EXTERN_C ShaderCompiler_ContextHandle ShaderCompiler_Create() {
	auto ctx = (ShaderCompiler_Context *) MEMORY_CALLOC(1, sizeof(ShaderCompiler_Context));
	if (!ctx) return nullptr;

	ctx->scOptions = ShaderConductor::Compiler::Options{};
	ctx->scTarget = ShaderConductor::Compiler::TargetDesc{};
	ctx->khrCompiler = shaderc_compiler_initialize();
	ctx->khrOptions = shaderc_compile_options_initialize();
	ctx->khrSpvcCompiler = shaderc_spvc_compiler_initialize();
	ctx->khrSpvcOptions = shaderc_spvc_compile_options_initialize();


	// 'debug' info is also needed for reflection
	shaderc_compile_options_set_generate_debug_info(ctx->khrOptions);
	shaderc_spvc_compile_options_set_separate_shader_objects(ctx->khrSpvcOptions, true);
	shaderc_spvc_compile_options_set_flatten_multidimensional_arrays(ctx->khrSpvcOptions, false);
	shaderc_spvc_compile_options_set_vulkan_semantics(ctx->khrSpvcOptions, false);
	shaderc_spvc_compile_options_set_msl_swizzle_texture_samples(ctx->khrSpvcOptions, false);
	shaderc_spvc_compile_options_set_msl_platform(ctx->khrSpvcOptions, shaderc_spvc_msl_platform_macos);

	// set defaults
	ShaderCompiler_SetLanguage(ctx, ShaderCompiler_LANG_HLSL);
	ShaderCompiler_SetOptimizationLevel(ctx, ShaderCompiler_OPT_Performance3);

#if AL2O3_PLATFORM == AL2O3_PLATFORM_APPLE_MAC
	ShaderCompiler_SetOutput(ctx, ShaderCompiler_OT_MSL_OSX, 0);
#elif AL2O3_PLATFORM == AL2O3_PLATFORM_WINDOWS
	ShaderCompiler_SetOutput(ctx, ShaderCompiler_OT_DXIL, 60);
#else
	ShaderCompiler_SetOutput(ctx, ShaderCompiler_OT_SPIRV, 11);
#endif
	return ctx;
}

AL2O3_EXTERN_C void ShaderCompiler_Destroy(ShaderCompiler_ContextHandle handle) {
	auto ctx = (ShaderCompiler_Context *) handle;
	if (!ctx) return;

	shaderc_spvc_compile_options_release(ctx->khrSpvcOptions);
	shaderc_spvc_compiler_release(ctx->khrSpvcCompiler);
	shaderc_compile_options_release(ctx->khrOptions);
	shaderc_compiler_release(ctx->khrCompiler);
	MEMORY_FREE(ctx);
}

AL2O3_EXTERN_C void ShaderCompiler_SetLanguage(ShaderCompiler_ContextHandle handle, ShaderCompiler_Language language) {
	auto ctx = (ShaderCompiler_Context *) handle;
	if (!ctx) return;

	// shaderconductor doesn't support GLSL input so forces us all platforms to khronos
	shaderc_source_language const lang = KhrLanguageConverter(language);
	shaderc_compile_options_set_source_language(ctx->khrOptions, lang);
	ctx->inputLanguage = language;
}

AL2O3_EXTERN_C void ShaderCompiler_SetOutput(ShaderCompiler_ContextHandle handle,
																						 ShaderCompiler_OutputType output,
																						 uint32_t outputVersion) {
	auto ctx = (ShaderCompiler_Context *) handle;
	if (!ctx) return;

	ctx->outputType = output;

	switch (output) {
	case ShaderCompiler_OT_SPIRV:
		ctx->scTarget.language = ShaderConductor::ShadingLanguage::SpirV;
		if(outputVersion == 0) outputVersion = 11;
		switch (outputVersion) {
		case 10: shaderc_compile_options_set_target_spirv(ctx->khrOptions, shaderc_spirv_version_1_0);
			break;
		case 11: shaderc_compile_options_set_target_spirv(ctx->khrOptions, shaderc_spirv_version_1_1);
			break;
		case 12: shaderc_compile_options_set_target_spirv(ctx->khrOptions, shaderc_spirv_version_1_2);
			break;
		case 13: shaderc_compile_options_set_target_spirv(ctx->khrOptions, shaderc_spirv_version_1_3);
			break;
		case 14: shaderc_compile_options_set_target_spirv(ctx->khrOptions, shaderc_spirv_version_1_4);
			break;
		default: LOGERRORF("Unsupported SPIRV output version", outputVersion);
		}
		break;
	case ShaderCompiler_OT_DXIL:
		if(outputVersion == 0) outputVersion = 60;
		ctx->scTarget.language = ShaderConductor::ShadingLanguage::Dxil;
		ctx->scOptions.shaderModel.major_ver = (outputVersion / 10);
		ctx->scOptions.shaderModel.minor_ver = (outputVersion % 10);
		break;
	case ShaderCompiler_OT_HLSL:
		if(outputVersion == 0) outputVersion = 60;
		ctx->scTarget.language  = ShaderConductor::ShadingLanguage::Hlsl;
		shaderc_spvc_compile_options_set_hlsl_shader_model(ctx->khrSpvcOptions, outputVersion);
		ctx->scOptions.shaderModel.major_ver = (outputVersion / 10);
		ctx->scOptions.shaderModel.minor_ver = (outputVersion % 10);
		break;

	case ShaderCompiler_OT_GLSL:
		if(outputVersion == 0) outputVersion = 450;

		ctx->scTarget.language = ShaderConductor::ShadingLanguage::Glsl;
		switch (outputVersion) {
		case 300: ctx->scTarget.version = "300";
			break;
		case 400: ctx->scTarget.version = "400";
			break;
		default:
		case 450: ctx->scTarget.version = "450";
			break;
		}
		break;
		break;
	case ShaderCompiler_OT_MSL_OSX:
		ctx->scTarget.language = ShaderConductor::ShadingLanguage::Msl_macOS;
		break;
	case ShaderCompiler_OT_MSL_IOS:
		ctx->scTarget.language = ShaderConductor::ShadingLanguage::Msl_iOS;
		break;

	}
}

AL2O3_EXTERN_C void ShaderCompiler_SetOptimizationLevel(ShaderCompiler_ContextHandle handle,
																												ShaderCompiler_Optimizations level) {
	auto ctx = (ShaderCompiler_Context *) handle;
	if (!ctx) return;

	shaderc_optimization_level khropti = KhrOptimizationConverter(level);
	shaderc_compile_options_set_optimization_level(ctx->khrOptions, khropti);
	ScOptimizationConverter(level, ctx->scOptions);

}

AL2O3_EXTERN_C bool ShaderCompiler_Compile(
		ShaderCompiler_ContextHandle handle,
		ShaderCompiler_ShaderType type,
		char const *name,
		char const *entryPoint,
		VFile_Handle file,
		ShaderCompiler_Output *output
) {
	auto ctx = (ShaderCompiler_Context *) handle;
	if (!ctx) return false;

	bool useShaderConductor = true;

#if AL2O3_PLATFORM == AL2O3_PLATFORM_WINDOWS
//	useShaderConductor = true;
#endif
	if (ctx->inputLanguage == ShaderCompiler_LANG_GLSL &&
			ctx->outputType == ShaderCompiler_OT_DXIL) {
		// currently DXIL and GLSL are not supported. In theory it could be but..
		// TODO GLSL to DXIL via glslang->SpirvCross->hlsl->ShaderConductor->DXIL
		return false;
	}

	if (ctx->inputLanguage == ShaderCompiler_LANG_GLSL) {
		useShaderConductor = false;
	}
	if(ctx->outputType == ShaderCompiler_OT_DXIL) {
		useShaderConductor = true;
	}


	char* src;
	if(VFile_GetType(file) == VFile_Type_Memory) {
		auto memFile = (VFile_MemFile_t*) VFile_GetTypeSpecificData(file);
		src = ((char*) memFile->memory) + memFile->offset;
	} else {
		size_t const fileSize = VFile_Size(file);
		if (fileSize == 0)
			return false;
		src = (char *) MEMORY_TEMP_MALLOC(fileSize + 1);
		VFile_Read(file, src, fileSize);
		src[fileSize] = 0;
	}

	bool ret;
	if (useShaderConductor) {
		ret = CompileShaderShaderConductor(ctx, type, name, entryPoint, src, output);
	} else {
		ret = CompileShaderKhronos(ctx, type, name, entryPoint, src, output);
	}
	if(VFile_GetType(file) != VFile_Type_Memory) {
		MEMORY_TEMP_FREE(src);
	}

	return ret;
}


AL2O3_EXTERN_C bool ShaderCompiler_CompileShader(
		ShaderCompiler_Language language,
		ShaderCompiler_ShaderType shaderType,
		char const *name,
		char const *entryPoint,
		VFile_Handle file,
		ShaderCompiler_Optimizations optimizations,
		ShaderCompiler_OutputType outputType,
		uint32_t outputVersion,
		ShaderCompiler_Output *output) {

	auto ctx = ShaderCompiler_Create();
	ShaderCompiler_SetLanguage(ctx, language);
	ShaderCompiler_SetOutput(ctx, outputType, outputVersion);
	ShaderCompiler_SetOptimizationLevel(ctx, optimizations);
	bool ret = ShaderCompiler_Compile(ctx, shaderType, name, entryPoint, file, output);

	ShaderCompiler_Destroy(ctx);
	return ret;
}