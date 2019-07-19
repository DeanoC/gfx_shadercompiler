#if building standalone and not specified build tests
option(BUILD_TESTS "Whether to build tests" ON)
include(FetchContent)
FetchContent_Declare( al2o3 GIT_REPOSITORY https://github.com/DeanoC/al2o3 GIT_TAG master )
FetchContent_GetProperties(al2o3)
if(NOT al2o3_POPULATED)
	FetchContent_Populate(al2o3)
	add_subdirectory(${al2o3_SOURCE_DIR} ${al2o3_BINARY_DIR})
endif()

SET_MIN_VERSIONS()
set(LibName gfx_shadercompiler)
project(${LibName})

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
		gfx_shaderc
		)
ADD_LIB(${LibName} "${Interface}" "${Src}" "${Deps}")
add_library( ShaderConductor SHARED IMPORTED )
#target_link_libraries(${LibName} PRIVATE ShaderConductor)
add_library( DxCompiler SHARED IMPORTED )
target_link_libraries(${LibName} PRIVATE DxCompiler)
if(APPLE)
#	configure_file(
#			${CMAKE_CURRENT_SOURCE_DIR}/precompiled/libShaderConductor.dylib
#			${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libShaderConductor.dylib COPYONLY)
	configure_file(
			${CMAKE_CURRENT_SOURCE_DIR}/precompiled/libdxcompiler.dylib
			${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libdxcompiler.3.7.dylib COPYONLY)
#	set_target_properties( ShaderConductor PROPERTIES IMPORTED_LOCATION ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libShaderConductor.dylib)
	set_target_properties( DxCompiler PROPERTIES IMPORTED_LOCATION ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libdxcompiler.3.7.dylib)
elseif(WIN32)
	configure_file(
			${CMAKE_CURRENT_SOURCE_DIR}/precompiled/ShaderConductor.dll
			${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/ShaderConductor.dll COPYONLY)
	configure_file(
			${CMAKE_CURRENT_SOURCE_DIR}/precompiled/dxcompiler.dll
			${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/dxcompiler.dll COPYONLY)
	configure_file(
			${CMAKE_CURRENT_SOURCE_DIR}/precompiled/dxil.dll
			${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/dxil.dll COPYONLY)
	set_target_properties( ShaderConductor PROPERTIES IMPORTED_IMPLIB ${CMAKE_CURRENT_SOURCE_DIR}/precompiled/ShaderConductor.lib)
	set_target_properties( ShaderConductor PROPERTIES IMPORTED_LOCATION ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libShaderConductor.dll)

endif()

