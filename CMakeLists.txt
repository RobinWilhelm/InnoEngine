
cmake_minimum_required(VERSION 3.30)


project(
	InnoEngine 
	DESCRIPTION "Framework for building graphical applications"
	LANGUAGES CXX 
	VERSION 0.1)


set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(USE_FOLDERS ON) 

include(cmake/Utility.cmake)

# Link project_warnings as "library" to use the warnings
# specified in CompilerWarnings.cmake.
add_library(project_warnings INTERFACE)
include(cmake/CompilerWarnings.cmake)
set_project_warnings(project_warnings)

include(cmake/Dependencies.cmake) 

enable_testing()
include(GoogleTest)

add_subdirectory(src)
