set(NAME "Sample")

include(${PROJECT_SOURCE_DIR}/cmake/StaticAnalyzers.cmake)

file(GLOB_RECURSE SAMPLE_HEADERS RELATIVE ${CMAKE_CURRENT_LIST_DIR} "Sample/*.h" "Sample/*.hpp")
file(GLOB_RECURSE SAMPLE_SOURCES RELATIVE ${CMAKE_CURRENT_LIST_DIR} "Sample/*.cpp")

set(SAMPLE_SOURCES ${SAMPLE_HEADERS} ${SAMPLE_SOURCES})

add_executable(${NAME} ${SAMPLE_SOURCES})

if (MSVC)
    mirror_source_structure("${SAMPLE_SOURCES}")
endif()

target_include_directories(${NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(${NAME} PRIVATE InnoEngine)


set_target_properties( ${NAME}
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/sandbox_environment"
)

if(MSVC)
set_target_properties( ${NAME}
    PROPERTIES    
        VS_DEBUGGER_WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/sandbox_environment/Debug"
)
endif()
