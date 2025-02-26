# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

#.rst:
# FindSDL3_image
# -------------
#
# Locate SDL3_image library
#
# This module defines:
#
# ::
#
#   SDL3_IMAGE_LIBRARIES, the name of the library to link against
#   SDL3_IMAGE_INCLUDE_DIRS, where to find the headers
#   SDL3_IMAGE_FOUND, if false, do not try to link against
#   SDL3_IMAGE_VERSION_STRING - human-readable string containing the
#                              version of SDL3_image
#
#
#
# For backward compatibility the following variables are also set:
#
# ::
#
#   SDL3IMAGE_LIBRARY (same value as SDL3_IMAGE_LIBRARIES)
#   SDL3IMAGE_INCLUDE_DIR (same value as SDL3_IMAGE_INCLUDE_DIRS)
#   SDL3IMAGE_FOUND (same value as SDL3_IMAGE_FOUND)
#
#
#
# $SDLDIR is an environment variable that would correspond to the
# ./configure --prefix=$SDLDIR used in building SDL.
#
# Created by Eric Wing.  This was influenced by the FindSDL.cmake
# module, but with modifications to recognize OS X frameworks and
# additional Unix paths (FreeBSD, etc).

if(NOT SDL3_IMAGE_INCLUDE_DIR AND SDL3IMAGE_INCLUDE_DIR)
  set(SDL3_IMAGE_INCLUDE_DIR ${SDL3IMAGE_INCLUDE_DIR} CACHE PATH "directory cache
entry initialized from old variable name")
endif()
find_path(SDL3_IMAGE_INCLUDE_DIR SDL_image.h
  HINTS
    ENV SDL3IMAGEDIR
    ENV SDL3DIR
    ${SDL3_DIR}
  PATH_SUFFIXES SDL3
                # path suffixes to search inside ENV{SDL3DIR}
                include/SDL3 include
)

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
  set(VC_LIB_PATH_SUFFIX lib/x64)
else()
  set(VC_LIB_PATH_SUFFIX lib/x86)
endif()

if(NOT SDL3_IMAGE_LIBRARY AND SDL3IMAGE_LIBRARY)
  set(SDL3_IMAGE_LIBRARY ${SDL3IMAGE_LIBRARY} CACHE FILEPATH "file cache entry
initialized from old variable name")
endif()
find_library(SDL3_IMAGE_LIBRARY
  NAMES SDL3_image
  HINTS
    ENV SDL3IMAGEDIR
    ENV SDL3DIR
    ${SDL3_DIR}
  PATH_SUFFIXES lib ${VC_LIB_PATH_SUFFIX}
)

if(SDL3_IMAGE_INCLUDE_DIR AND EXISTS "${SDL3_IMAGE_INCLUDE_DIR}/SDL3_image.h")
  file(STRINGS "${SDL3_IMAGE_INCLUDE_DIR}/SDL3_image.h" SDL3_IMAGE_VERSION_MAJOR_LINE REGEX "^#define[ \t]+SDL3_IMAGE_MAJOR_VERSION[ \t]+[0-9]+$")
  file(STRINGS "${SDL3_IMAGE_INCLUDE_DIR}/SDL3_image.h" SDL3_IMAGE_VERSION_MINOR_LINE REGEX "^#define[ \t]+SDL3_IMAGE_MINOR_VERSION[ \t]+[0-9]+$")
  file(STRINGS "${SDL3_IMAGE_INCLUDE_DIR}/SDL3_image.h" SDL3_IMAGE_VERSION_PATCH_LINE REGEX "^#define[ \t]+SDL3_IMAGE_PATCHLEVEL[ \t]+[0-9]+$")
  string(REGEX REPLACE "^#define[ \t]+SDL3_IMAGE_MAJOR_VERSION[ \t]+([0-9]+)$" "\\1" SDL3_IMAGE_VERSION_MAJOR "${SDL3_IMAGE_VERSION_MAJOR_LINE}")
  string(REGEX REPLACE "^#define[ \t]+SDL3_IMAGE_MINOR_VERSION[ \t]+([0-9]+)$" "\\1" SDL3_IMAGE_VERSION_MINOR "${SDL3_IMAGE_VERSION_MINOR_LINE}")
  string(REGEX REPLACE "^#define[ \t]+SDL3_IMAGE_PATCHLEVEL[ \t]+([0-9]+)$" "\\1" SDL3_IMAGE_VERSION_PATCH "${SDL3_IMAGE_VERSION_PATCH_LINE}")
  set(SDL3_IMAGE_VERSION_STRING ${SDL3_IMAGE_VERSION_MAJOR}.${SDL3_IMAGE_VERSION_MINOR}.${SDL3_IMAGE_VERSION_PATCH})
  unset(SDL3_IMAGE_VERSION_MAJOR_LINE)
  unset(SDL3_IMAGE_VERSION_MINOR_LINE)
  unset(SDL3_IMAGE_VERSION_PATCH_LINE)
  unset(SDL3_IMAGE_VERSION_MAJOR)
  unset(SDL3_IMAGE_VERSION_MINOR)
  unset(SDL3_IMAGE_VERSION_PATCH)
endif()

set(SDL3_IMAGE_LIBRARIES ${SDL3_IMAGE_LIBRARY})
set(SDL3_IMAGE_INCLUDE_DIRS ${SDL3_IMAGE_INCLUDE_DIR})

include(FindPackageHandleStandardArgs)

FIND_PACKAGE_HANDLE_STANDARD_ARGS(SDL3_image
                                  REQUIRED_VARS SDL3_IMAGE_LIBRARIES SDL3_IMAGE_INCLUDE_DIRS
                                  VERSION_VAR SDL3_IMAGE_VERSION_STRING)

# for backward compatibility
set(SDL3IMAGE_LIBRARY ${SDL3_IMAGE_LIBRARIES})
set(SDL3IMAGE_INCLUDE_DIR ${SDL3_IMAGE_INCLUDE_DIRS})
set(SDL3IMAGE_FOUND ${SDL3_IMAGE_FOUND})

mark_as_advanced(SDL3_IMAGE_LIBRARY SDL3_IMAGE_INCLUDE_DIR)