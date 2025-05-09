set(NAME "InnoEngine")

include(${PROJECT_SOURCE_DIR}/cmake/StaticAnalyzers.cmake)
#include(${PROJECT_SOURCE_DIR}/cmake/PrecompiledHeader.cmake)

set(INNOENGINE_PCH "InnoEngine/iepch.h")
set(INNOENGINE_PCH_SOURCE "InnoEngine/iepch.cpp") 

file(GLOB_RECURSE INNOENGINE_HEADERS RELATIVE ${CMAKE_CURRENT_LIST_DIR} "InnoEngine/*.h" "InnoEngine/*.hpp")
file(GLOB_RECURSE INNOENGINE_SOURCES RELATIVE ${CMAKE_CURRENT_LIST_DIR} "InnoEngine/*.cpp")
list(FILTER INNOENGINE_SOURCES EXCLUDE REGEX ".*\.Test\.cpp")
file(GLOB_RECURSE INNOENGINE_SHADERS RELATIVE ${CMAKE_CURRENT_LIST_DIR} 
        "InnoEngine/graphics/shader/*.frag.hlsl"  
        "InnoEngine/graphics/shader/*.vert.hlsl"  
        "InnoEngine/graphics/shader/*.verti.hlsl"  
        "InnoEngine/graphics/shader/*.fragi.hlsl" 
        "InnoEngine/graphics/shader/*.comp.hlsl" 
        "InnoEngine/graphics/shader/*.compi.hlsl")

# just show them in the project for easier editing 
set(INNOENGINE_OTHER_SOURCES ${PROJECT_SOURCE_DIR}/README.md ${PROJECT_SOURCE_DIR}/Todo.txt)

        
if(MSVC)
    add_library(${NAME} STATIC ${INNOENGINE_HEADERS} ${INNOENGINE_SOURCES} ${INNOENGINE_SHADERS} ${INNOENGINE_OTHER_SOURCES})
    mirror_source_structure(${INNOENGINE_HEADERS} ${INNOENGINE_SOURCES} ${INNOENGINE_SHADERS} ${INNOENGINE_OTHER_SOURCES})
    set_property(SOURCE ${INNOENGINE_SHADERS} ${INNOENGINE_OTHER_SOURCES} PROPERTY VS_SETTINGS "ExcludedFromBuild=true")	
else()
    add_library(${NAME} STATIC ${INNOENGINE_HEADERS} ${INNOENGINE_SOURCES})
endif()

target_precompile_headers(${NAME} PRIVATE ${INNOENGINE_PCH})
#add_precompiled_header(${NAME} "${INNOENGINE_PCH}" SOURCE_CXX "${INNOENGINE_PCH_SOURCE}")

    
target_include_directories(${NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(${NAME} PRIVATE project_warnings msdf-atlas-gen)
target_link_libraries(${NAME} PUBLIC box2d)

find_package(Vulkan REQUIRED)
target_link_libraries(${NAME} PRIVATE Vulkan::Vulkan)

find_package(SDL3 CONFIG REQUIRED)
target_link_libraries(${NAME} PUBLIC SDL3::SDL3)

find_package(SDL3_image CONFIG REQUIRED)
target_link_libraries(${NAME} PRIVATE $<IF:$<TARGET_EXISTS:SDL3_image::SDL3_image-shared>,SDL3_image::SDL3_image-shared,SDL3_image::SDL3_image-static>)

find_package(imgui CONFIG REQUIRED)
target_link_libraries(${NAME} PUBLIC imgui::imgui)

find_package(directxtk CONFIG REQUIRED)
target_link_libraries(${NAME} PUBLIC Microsoft::DirectXTK)

find_package(nlohmann_json CONFIG REQUIRED)
target_link_libraries(${NAME} PRIVATE nlohmann_json::nlohmann_json)

file(GLOB_RECURSE INNOENGINE_TEST_SOURCES RELATIVE ${CMAKE_CURRENT_LIST_DIR} "InnoEngine/*.Test.cpp")
add_executable(
  InnoEngine_Test
  ${INNOENGINE_TEST_SOURCES}
  ${INNOENGINE_HEADERS}
)
target_link_libraries(
  InnoEngine_Test
  PRIVATE ${NAME}
  PRIVATE GTest::gtest_main
)
target_include_directories(InnoEngine_Test PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

gtest_discover_tests(InnoEngine_Test)