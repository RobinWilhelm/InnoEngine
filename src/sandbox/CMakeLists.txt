set(NAME "Sandbox")

include(${PROJECT_SOURCE_DIR}/cmake/StaticAnalyzers.cmake)

file(GLOB_RECURSE SANDBOX_HEADERS RELATIVE ${CMAKE_CURRENT_LIST_DIR} "Sandbox/*.h" "Sandbox/*.hpp")
file(GLOB_RECURSE SANDBOX_SOURCES RELATIVE ${CMAKE_CURRENT_LIST_DIR} "Sandbox/*.cpp")

set(SANDBOX_SOURCES ${SANDBOX_HEADERS} ${SANDBOX_SOURCES})

add_executable(${NAME} ${SANDBOX_SOURCES})

if (MSVC)
    mirror_source_structure("${SANDBOX_SOURCES}")
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
