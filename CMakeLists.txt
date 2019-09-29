cmake_minimum_required(VERSION 3.12.4)
get_directory_property(hasParent PARENT_DIRECTORY)
if(NOT hasParent)
	option(unittests "unittests" OFF)
	get_filename_component(_PARENT_DIR ${CMAKE_CURRENT_SOURCE_DIR} DIRECTORY)
	set_property(GLOBAL PROPERTY GLOBAL_FETCHDEPS_BASE ${_PARENT_DIR}/al2o3 )
	include(FetchContent)
	FetchContent_Declare( al2o3 GIT_REPOSITORY https://github.com/DeanoC/al2o3 GIT_TAG master )
	FetchContent_GetProperties(al2o3)
	if(NOT al2o3_POPULATED)
		FetchContent_Populate(al2o3)
		add_subdirectory(${al2o3_SOURCE_DIR} ${al2o3_BINARY_DIR})
	endif()
	INIT_AL2O3(${CMAKE_CURRENT_SOURCE_DIR})
endif ()

SET_MIN_VERSIONS()
set(LibName gfx_shadercompiler)
project(${LibName})

# set to true might allow GLSL (not really tested...) but will slow your compiles down...
set(SUPPORT_GLSL FALSE)

set(Interface
		compiler.h
		)

set(Src
		compiler.cpp
		ShaderConductor/ShaderConductor.hpp
		ShaderConductor/ShaderConductor.cpp

		dxc/Support/ErrorCodes.h
		dxc/Support/exception.h
		dxc/Support/Global.cpp
		dxc/Support/Global.h
		dxc/Support/Unicode.cpp
		dxc/Support/Unicode.h
		dxc/Support/WinAdapter.h
		dxc/Support/WinAdaptor.cpp
		dxc/Support/WinFunctions.h
		dxc/Support/WinFunctions.cpp
		dxc/Support/WinIncludes.h
		dxc/dxcapi.h
		)

set(Deps
		al2o3_platform
		al2o3_memory
		al2o3_vfile
		)

if (SUPPORT_GLSL)
	list(APPEND Deps gfx_shaderc)
else ()
	list(APPEND Deps
			gfx_spirvcross
			gfx_spirvtools
			)
endif ()

ADD_LIB(${LibName} "${Interface}" "${Src}" "${Deps}")

if (SUPPORT_GLSL)
	target_compile_definitions(${LibName} SUPPORT_GLSL)
endif ()

if(APPLE)
	add_library( DxCompiler SHARED IMPORTED )
	target_link_libraries(${LibName} PRIVATE DxCompiler)
	configure_file(
			${CMAKE_CURRENT_SOURCE_DIR}/precompiled/libdxcompiler.dylib
			${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libdxcompiler.dylib COPYONLY)
	set_target_properties( DxCompiler PROPERTIES IMPORTED_LOCATION ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libdxcompiler.dylib)
elseif(WIN32)
	configure_file(
			${CMAKE_CURRENT_SOURCE_DIR}/precompiled/dxcompiler.dll
			${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/dxcompiler.dll COPYONLY)
	configure_file(
			${CMAKE_CURRENT_SOURCE_DIR}/precompiled/dxil.dll
			${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/dxil.dll COPYONLY)
endif()

