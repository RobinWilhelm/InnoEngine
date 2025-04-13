include(FetchContent)

set(3RD_PARTY_FOLDER_NAME "vendor")

set(FETCHCONTENT_BASE_DIR ${PROJECT_SOURCE_DIR}/${3RD_PARTY_FOLDER_NAME})
set(FETCHCONTENT_QUIET FALSE)
set(CMAKE_FOLDER ${3RD_PARTY_FOLDER_NAME})

message("Configure msdf-atlas-gen: ")
FetchContent_Declare(
  msdf-atlas-gen
  GIT_REPOSITORY https://github.com/Chlumsky/msdf-atlas-gen.git
  GIT_TAG v1.3
  GIT_PROGRESS TRUE
  BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/${3RD_PARTY_FOLDER_NAME}/msdf-atlas-gen
)

set(MSDF_ATLAS_BUILD_STANDALONE OFF)
set(MSDF_ATLAS_USE_VCPKG ON)
set(MSDF_ATLAS_USE_SKIA OFF)
set(MSDF_ATLAS_NO_ARTERY_FONT ON)
set(MSDF_ATLAS_DYNAMIC_RUNTIME ON)
FetchContent_MakeAvailable(msdf-atlas-gen)


message("Configure box2d: ")
FetchContent_Declare(
  box2d
  GIT_REPOSITORY https://github.com/erincatto/box2d.git
  GIT_TAG v3.0.0
  GIT_PROGRESS TRUE
  BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/${3RD_PARTY_FOLDER_NAME}/box2d
)
FetchContent_MakeAvailable(box2d)

#https://google.github.io/googletest/quickstart-cmake.html
message("Configure google test: ")
FetchContent_Declare(
  googletest
  #GIT_REPOSITORY https://github.com/google/googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.16.0.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

set(CMAKE_FOLDER "")