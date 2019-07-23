cmake_minimum_required (VERSION 2.8.10 FATAL_ERROR)

set (CMAKE_SUPPRESS_REGENERATION TRUE)

# set build configurations list
if (NOT BUILD_CONFIGURATION)
  set (BUILD_CONFIGURATION "Release" CACHE STRING "Build type of OCCT" FORCE)
  SET_PROPERTY(CACHE BUILD_CONFIGURATION PROPERTY STRINGS Release Debug RelWithDebInfo)
endif()

set (CMAKE_CONFIGURATION_TYPES ${BUILD_CONFIGURATION} CACHE INTERNAL "" FORCE)

project (CADRaysProject)

# =============================================================================
# Configure CMake
# =============================================================================

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMake" ${CMAKE_MODULE_PATH})

include (DefineMacro)
include (DefineCxxFlags)
include (macros)

set (CMAKE_INCLUDE_CURRENT_DIR ON)

if(WIN32)
  add_definitions (-DWNT -DWINVER=0x0500)
else()
  add_definitions (-fexceptions -fPIC -DOCC_CONVERT_SIGNALS)
endif()

# Suppression of 3rdparty warnings in MSVC with #pragma warnings (disable)
if(MSVC)
  add_definitions (/wd4718)
else()
  add_definitions (-Wno-unknown-pragmas)
endif()

set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# =============================================================================
# Prompt user options
# =============================================================================

set (CMAKE_BUILD_TYPE ${BUILD_CONFIGURATION} CACHE INTERNAL "Build type" FORCE )

# =============================================================================
# Configure (prompt) primary 3rdparties and dependencies
# =============================================================================
set (3RDPARTY_DIR_LABEL "The directory containing required 3rdparty products")
set (3RDPARTY_DIR "" CACHE PATH ${3RDPARTY_DIR_LABEL})

# search for 3rdparty dir
if ("${3RDPARTY_DIR}" STREQUAL "")
  if (DEFINED ENV{3RDPARTY_DIR})
    set (3RDPARTY_DIR "$ENV{3RDPARTY_DIR}" CACHE PATH ${3RDPARTY_DIR_LABEL} FORCE)
  elseif (EXISTS "${CMAKE_SOURCE_DIR}/../3rdparty/")
    get_filename_component (3RDPARTY_DIR "${CMAKE_SOURCE_DIR}/../3rdparty/" ABSOLUTE)
    set (3RDPARTY_DIR "${3RDPARTY_DIR}" CACHE PATH ${3RDPARTY_DIR_LABEL} FORCE)
  endif()
endif()

find_package(OpenCASCADE QUIET PATHS "${OpenCASCADE_DIR}" "${OpenCASCADE_INSTALL_PREFIX}" NO_DEFAULT_PATH)

if (NOT OpenCASCADE_FOUND)
  message (FATAL_ERROR "coult not find OpenCASCADE, please set OpenCASCADE_DIR variable" )
else()
  set (OCCT_ROOT_DIR "${OpenCASCADE_INSTALL_PREFIX}" CACHE PATH "OpenCASCADE directory" FORCE)
  message (STATUS "Using OpenCASCADE from \"${OpenCASCADE_DIR}\"" )
endif()

add_definitions (-DCASROOT_DIR="${OCCT_ROOT_DIR}")

# Solution folder property
set_property (GLOBAL PROPERTY USE_FOLDERS ON)

# occt pre-requisites
option (OCCT_USES_TBB "Set this option if OCCT uses TBB library." ON)
option (OCCT_USES_FFMPEG "Set this option if OCCT uses FFMPEG library." ON)

# find freeimage
FIND_THIRD_PARTY("${3RDPARTY_DIR}" FREEIMAGE FREEIMAGE_DIR_NAME)
if (THIRDPARTY_COMPONENT_FOUND)
  set (FREEIMAGE_ROOT_DIR "${3RDPARTY_DIR}/${FREEIMAGE_DIR_NAME}" CACHE PATH "Path to FreeImage library.")
endif()

# find tbb for occt
if (OCCT_USES_TBB)
  FIND_THIRD_PARTY("${3RDPARTY_DIR}" TBB TBB_DIR_NAME)
  if (THIRDPARTY_COMPONENT_FOUND)
    set (TBB_ROOT_DIR "${3RDPARTY_DIR}/${TBB_DIR_NAME}" CACHE PATH "Path to TBB multithreading libraries.")
  endif()

  find_package(TBB REQUIRED)
endif()

# find freetype for occt
FIND_THIRD_PARTY("${3RDPARTY_DIR}" FreeType FreeType_DIR_NAME)
if (THIRDPARTY_COMPONENT_FOUND)
  set (FREETYPE_ROOT_DIR "${3RDPARTY_DIR}/${FreeType_DIR_NAME}" CACHE PATH "Path to FreeType library.")
endif()

# FreeType is mandatory for OCCT
find_package(FreeType REQUIRED)

# find ffmpeg for occt
if (OCCT_USES_FFMPEG)
  FIND_THIRD_PARTY("${3RDPARTY_DIR}" FFMPEG FFMPEG_DIR_NAME)
  if (THIRDPARTY_COMPONENT_FOUND)
    set (FFMPEG_ROOT_DIR "${3RDPARTY_DIR}/${FFMPEG_DIR_NAME}" CACHE PATH "Path to FFMPEG libraries.")
  endif()

  find_package(FFMPEG REQUIRED)
endif()

# setup desktop 3rdparty
if (WIN32)
  find_package (WindowsSDK)
endif()

FIND_THIRD_PARTY("${3RDPARTY_DIR}" TCL TCL_DIR_NAME)
if (THIRDPARTY_COMPONENT_FOUND)
  set (TCL_ROOT_DIR "${3RDPARTY_DIR}/${TCL_DIR_NAME}" CACHE PATH "Path to TCL library.")
endif()

FIND_THIRD_PARTY("${3RDPARTY_DIR}" assimp ASSIMP_DIR_NAME)
if (THIRDPARTY_COMPONENT_FOUND)
  set (ASSIMP_ROOT_DIR "${3RDPARTY_DIR}/${ASSIMP_DIR_NAME}" CACHE PATH "Path to Assimp library.")
endif()

FIND_THIRD_PARTY("${3RDPARTY_DIR}" GLFW GLFW_DIR_NAME)
if (THIRDPARTY_COMPONENT_FOUND)
  set (GLFW_ROOT_DIR "${3RDPARTY_DIR}/${GLFW_DIR_NAME}" CACHE PATH "Path to GLFW library.")
endif()

set (CMAKE_PREFIX_PATH "${WINDOWSSDK_DIRS};" ${CMAKE_PREFIX_PATH})

# =============================================================================
# Define internal variables
# =============================================================================

set (CMAKE_DEBUG_CONFIGURATIONS Debug)
set (CMAKE_RELEASE_CONFIGURATIONS Release RelWithDebInfo MinSizeRel)

# =============================================================================
# Include projects or dependencies into solution
# =============================================================================

add_subdirectory (src/Utils)
add_subdirectory (src/ImportExport)
add_subdirectory (src/ImGui)
add_subdirectory (src/Launcher)

# =============================================================================
# Handle shared resources
# =============================================================================

# TODO