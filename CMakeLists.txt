cmake_minimum_required(VERSION 3.20)

include("${CMAKE_CURRENT_SOURCE_DIR}/extern/cmake-precompiled-header/PrecompiledHeader.cmake")

project(InnoEngine LANGUAGES CXX VERSION 0.1)

set(CMAKE_CXX_STANDARD 20)

# Function to prepend the subdirectory to source files in subdirectories
FUNCTION(PREPEND var )
   SET(listVar "")
   FOREACH(f ${${var}})
       LIST(APPEND listVar "${CMAKE_CURRENT_SOURCE_DIR}/${f}")
   ENDFOREACH(f)
   SET(${var} "${listVar}" PARENT_SCOPE)
ENDFUNCTION(PREPEND)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin")

add_subdirectory(extern)
add_subdirectory(InnoEngine)
add_subdirectory(Sandbox)	

list (APPEND InnoEngine_ALL_Sources ${InnoEngine_Sources} ${InnoEngine_Shader_Sources})

include_directories(${IMGUI_INCLUDE_DIRS})
include_directories(${DIRECTXTK_INCLUDE_DIRS})
include_directories(${SDL3_INCLUDE_DIRS})
include_directories(${SDL3_IMAGE_INCLUDE_DIRS})
include_directories(${SDL3_SHADERCROSS_INCLUDE_DIRS})

add_library(InnoEngine STATIC ${InnoEngine_ALL_Sources})


# add shaders to the project but do not compile them
if(MSVC)
	set_property(SOURCE ${InnoEngine_Shader_Sources} PROPERTY VS_SETTINGS "ExcludedFromBuild=true")	
endif()

# compiler warning settings
if(MSVC)
	target_compile_options(${PROJECT_NAME} PRIVATE /W4 /WX)
else()
	target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

# set project structure to be the same as the folder structure
foreach(_source IN ITEMS ${InnoEngine_ALL_Sources})
    get_filename_component(_source_path "${_source}" PATH)
    string(REPLACE "${CMAKE_SOURCE_DIR}" "" _group_path "${_source_path}")
    string(REPLACE "/" "\\" _group_path "${_group_path}")
    source_group("${_group_path}" FILES "${_source}")
endforeach()

#set(IE_PCH_ABSOLUTE "${CMAKE_SOURCE_DIR}/${IE_PCH}") # clang will complain about windows specific path if we dont set it to an absolute path here
add_precompiled_header(InnoEngine "${IE_PCH}" SOURCE_CXX "${IE_PCH_SOURCE}")

target_link_directories(InnoEngine 
	PUBLIC ${SDL3_BUILD} 
	PUBLIC ${SDL3_IMAGE_BUILD} 
	PUBLIC ${SDL3_SHADERCROSS_BUILD_DIRS}
)

target_link_libraries(InnoEngine 
	SDL3 
	SDL3_image 
	SDL3_shadercross-static
)

add_executable(Sandbox ${Sandbox_Sources})
target_include_directories(Sandbox PUBLIC "InnoEngine/src") # just include th e

target_link_directories(Sandbox PUBLIC ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY})
target_link_libraries(Sandbox InnoEngine)

# set project structure to be the same as the folder structure
foreach(_source IN ITEMS ${Sandbox_Sources})
    get_filename_component(_source_path "${_source}" PATH)
    string(REPLACE "${CMAKE_SOURCE_DIR}" "" _group_path "${_source_path}")
    string(REPLACE "/" "\\" _group_path "${_group_path}")
    source_group("${_group_path}" FILES "${_source}")
endforeach()

# remove custom compile flags from all 3rdparty sourcefiles (eg precompiled headers)
foreach(_source in ${EXTERNAL_SOURCES})
	set_source_files_properties("${_source}" PROPERTIES COMPILE_FLAGS "")
endforeach()
