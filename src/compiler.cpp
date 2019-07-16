#include "al2o3_platform/platform.h"
#include "al2o3_memory/memory.h"
#include "gfx_shadercompiler/compiler.h"
#include "shaderc/shaderc.h"
#include "shaderc/spvc.h"
#include "ShaderConductor/ShaderConductor.hpp"

static shaderc_source_language LanguageConverter(ShaderCompiler_Language sourceLanguage)
{
	switch(sourceLanguage)
	{
	case ShaderCompiler_LANG_HLSL: return shaderc_source_language_hlsl;
	case ShaderCompiler_LANG_GLSL: return shaderc_source_language_glsl;
	}
}
static  shaderc_optimization_level OptimizationConverter(ShaderCompiler_Optimizations optimizations)
{
	switch(optimizations)
	{
	case ShaderCompiler_OPT_None: return shaderc_optimization_level_zero;
	case ShaderCompiler_OPT_Size: return shaderc_optimization_level_size;
	case ShaderCompiler_OPT_Performance: return shaderc_optimization_level_performance;
	}
}


static shaderc_shader_kind TypeConverter(ShaderCompiler_ShaderType type)
{
	switch(type)
	{
	case ShaderCompiler_ST_VertexShader: 					return shaderc_vertex_shader;
	case ShaderCompiler_ST_FragmentShader:				return shaderc_fragment_shader;
	case ShaderCompiler_ST_ComputeShader:					return shaderc_compute_shader;
	case ShaderCompiler_ST_GeometryShader:				return shaderc_geometry_shader;
	case ShaderCompiler_ST_TessControlShader:			return shaderc_tess_control_shader;
	case ShaderCompiler_ST_TessEvaluationShader:	return shaderc_tess_evaluation_shader;
	case ShaderCompiler_ST_RaygenShader:					return shaderc_raygen_shader;
	case ShaderCompiler_ST_AnyHitShader:					return shaderc_anyhit_shader;
	case ShaderCompiler_ST_ClosestHitShader:			return shaderc_closesthit_shader;
	case ShaderCompiler_ST_MissShader:						return shaderc_miss_shader;
	case ShaderCompiler_ST_IntersectionShader:		return shaderc_intersection_shader;
	case ShaderCompiler_ST_CallableShader:				return shaderc_callable_shader;
	case ShaderCompiler_ST_TaskShader:						return shaderc_task_shader;
	case ShaderCompiler_ST_MeshShader:						return shaderc_mesh_shader;
	}
}

static char const* CopyString(char const* msg) {
	size_t const msgSize = strlen(msg);
	char* log = (char*)MEMORY_MALLOC(msgSize+1);
	memcpy((void*)log, (void*)msg, msgSize);
	log[msgSize] = 0;
	return log;
}

AL2O3_EXTERN_C bool CompileShaderKhronos(
		char const* name,
		ShaderCompiler_Language language,
		ShaderCompiler_ShaderType shaderType,
		char const* src,
		char const* entryPoint,
		ShaderCompiler_Optimizations optimizations,
		ShaderCompiler_OutputType outputType,
		ShaderCompiler_Output* output
) {

	// there are two phases, HLSL/GLSL to SPIRV then SPIRV -> HLSL, MSL, GLSL

	memset(output, 0, sizeof(ShaderCompiler_Output));

	shaderc_source_language lang = LanguageConverter(language);
	shaderc_shader_kind kind = TypeConverter(shaderType);
	shaderc_optimization_level opti = OptimizationConverter(optimizations);

	shaderc_compiler_t compiler = shaderc_compiler_initialize();

	shaderc_compile_options_t options = shaderc_compile_options_initialize();
	shaderc_compile_options_set_source_language(options, lang);
	shaderc_compile_options_set_optimization_level(options, opti);

	shaderc_compilation_result_t result = shaderc_compile_into_spv( compiler,
																				src,
																				strlen(src),
																				kind,
																				name,
																				entryPoint,
																				options);


	if(shaderc_result_get_compilation_status(result) != shaderc_compilation_status_success) {
		// don't work return error message
		output->log = CopyString(shaderc_result_get_error_message(result));

		shaderc_result_release(result);
		shaderc_compiler_release(compiler);
		shaderc_compile_options_release(options);
		return false;
	}

	// copy any warnings over but not fatal
	if(shaderc_result_get_num_warnings(result)) {
		output->log = CopyString(shaderc_result_get_error_message(result));
	}

	// if we are going to SPIRV we are done
	if(outputType == ShaderCompiler_OT_SPIRV) {
		size_t const size = shaderc_result_get_length(result);
		output->shader = MEMORY_MALLOC(size);
		memcpy((void*)output->shader, (void*)shaderc_result_get_bytes(result), size);
		output->shaderSize = size;
		shaderc_compiler_release(compiler);
		shaderc_compile_options_release(options);
		return true;
	}
	// done with compiler now
	shaderc_compiler_release(compiler);
	shaderc_compile_options_release(options);

	shaderc_spvc_compiler_t ocompiler = shaderc_spvc_compiler_initialize();
	shaderc_spvc_compile_options_t ooptions = shaderc_spvc_compile_options_initialize();
	shaderc_spvc_compile_options_set_entry_point(ooptions, entryPoint);
	shaderc_spvc_compile_options_set_hlsl_shader_model(ooptions, 51); // TODO pass int

	shaderc_spvc_compilation_result_t oresult = NULL;

	switch(outputType) {
	case ShaderCompiler_OT_GLSL:
		oresult = shaderc_spvc_compile_into_glsl(ocompiler, (uint32_t const*) output->shader, output->shaderSize, ooptions);
		break;
	case ShaderCompiler_OT_HLSL:
		oresult = shaderc_spvc_compile_into_hlsl(ocompiler, (uint32_t const*) output->shader, output->shaderSize, ooptions);
		break;
	case ShaderCompiler_OT_MSL_OSX:
	case ShaderCompiler_OT_MSL_IOS:
		oresult = shaderc_spvc_compile_into_msl(ocompiler, (uint32_t const*) output->shader, output->shaderSize, ooptions);
		break;
	default:
	case ShaderCompiler_OT_SPIRV:
		ASSERT(false);
	}

	bool ret = false;
	if(shaderc_spvc_result_get_status(oresult) == shaderc_compilation_status_success) {
		MEMORY_FREE((void*)output->shader);
		output->shader = CopyString(shaderc_spvc_result_get_output(oresult));
		output->shaderSize = strlen((char*)output->shader) + 1;
		ret = true;
	}

	shaderc_spvc_result_release(oresult);
	shaderc_spvc_compile_options_release(ooptions);
	shaderc_spvc_compiler_release(ocompiler);
	return true;
}

static ShaderConductor::ShaderStage SCShaderStageConvertor(ShaderCompiler_ShaderType type) {
	switch(type) {

	case ShaderCompiler_ST_VertexShader:
		return ShaderConductor::ShaderStage::VertexShader;
	case ShaderCompiler_ST_FragmentShader:
		return ShaderConductor::ShaderStage::PixelShader;
	case ShaderCompiler_ST_ComputeShader:
		return ShaderConductor::ShaderStage::ComputeShader;
	case ShaderCompiler_ST_GeometryShader:
		return ShaderConductor::ShaderStage::GeometryShader;
	case ShaderCompiler_ST_TessControlShader:
		return ShaderConductor::ShaderStage::HullShader;
	case ShaderCompiler_ST_TessEvaluationShader:
		return ShaderConductor::ShaderStage::DomainShader;

	case ShaderCompiler_ST_RaygenShader:
	case ShaderCompiler_ST_AnyHitShader:
	case ShaderCompiler_ST_ClosestHitShader:
	case ShaderCompiler_ST_MissShader:
	case ShaderCompiler_ST_IntersectionShader:
	case ShaderCompiler_ST_CallableShader:
	case ShaderCompiler_ST_TaskShader:
	case ShaderCompiler_ST_MeshShader:
		return ShaderConductor::ShaderStage::ComputeShader;
	}
}

static ShaderConductor::ShadingLanguage OutputTypeConvertor( ShaderCompiler_OutputType type ) {
	switch(type) {

	case ShaderCompiler_OT_SPIRV: return ShaderConductor::ShadingLanguage::SpirV;
	case ShaderCompiler_OT_DXIL:return ShaderConductor::ShadingLanguage::Dxil;
	case ShaderCompiler_OT_HLSL: return ShaderConductor::ShadingLanguage::Hlsl;
	case ShaderCompiler_OT_GLSL: return ShaderConductor::ShadingLanguage::SpirV;
	case ShaderCompiler_OT_MSL_OSX: return ShaderConductor::ShadingLanguage::Msl_macOS;
	case ShaderCompiler_OT_MSL_IOS: return ShaderConductor::ShadingLanguage::Msl_iOS;
	}
}

bool CompileShaderShaderConductor(
		char const* name,
		ShaderCompiler_ShaderType shaderType,
		char const* src,
		char const* entryPoint,
		ShaderCompiler_Optimizations optimizations,
		ShaderCompiler_OutputType outputType,
		ShaderCompiler_Output* output
) {
	using namespace ShaderConductor;
	memset(output, 0, sizeof(ShaderCompiler_Output));

	Compiler::SourceDesc source {};
	Compiler::Options options {};
	Compiler::TargetDesc target {};
	source.fileName = name;
	source.source = src;
	source.stage = SCShaderStageConvertor(shaderType);
	source.entryPoint = entryPoint;
	source.numDefines = 0; // TODO

	options.optimizationLevel = 0; // TODO
	options.shaderModel = {6, 0};

	target.language = OutputTypeConvertor(outputType);
	target.version = nullptr; //??
	auto result = Compiler::Compile(source, options, target);
	if(result.hasError) {
		output->log = CopyString((char*)result.errorWarningMsg->Data());
		return false;
	}
	output->log = CopyString((char*)result.errorWarningMsg->Data());

	size_t const size = result.target->Size();
	output->shader = MEMORY_MALLOC(size);
	memcpy((void*)output->shader, result.target->Data(), size);
	output->shaderSize = size;
	return true;
}

AL2O3_EXTERN_C bool ShaderCompiler_CompileShader(
		char const* name,
		ShaderCompiler_Language language,
		ShaderCompiler_ShaderType shaderType,
		char const* src,
		char const* entryPoint,
		ShaderCompiler_Optimizations optimizations,
		ShaderCompiler_OutputType outputType,
		ShaderCompiler_Output* output) {
	// GLSL requires
	if(language == ShaderCompiler_LANG_GLSL) {
		// TODO GLSL to DXIL via glslang->SpirvCross->hlsl->ShaderConductor->DXIL
		if(outputType == ShaderCompiler_OT_DXIL) {
			return false;
		}

		return CompileShaderKhronos(name, language, shaderType, src, entryPoint, optimizations, outputType, output);
	} else {
		return CompileShaderShaderConductor(name, shaderType, src, entryPoint, optimizations, outputType, output);
	}
}
