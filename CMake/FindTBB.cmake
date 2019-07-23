# This script finds OCCT libraries.
#
# The script looks for TBB release package,
# including tbb and tbbmalloc libraries. Both release
# and debug builds are considered.
#
# The script requires
#
#  TBB_ROOT_DIR - root directory of TBB binary release package.
#
# Once done the script will define
#
#  TBB_FOUND - package is succesfully found.
#  TBB_INCLUDE_DIRS - directory containing public headers files.
#  TBB_LIBRARIES_RELEASE - release version of libraries.
#  TBB_LIBRARIES_DEBUG - debug version of libraries.
#  TBB_DLLS_RELEASE - release version of dlls.
#  TBB_DLLS_DEBUG - debug version of dlls.

include(FindPackageHandleStandardArgs)

# =============================================================================
# Check for required variables
# =============================================================================

if (NOT TBB_ROOT_DIR)
  message(FATAL_ERROR "TBB_ROOT_DIR not found. Please locate before proceeding.")
endif()

# =============================================================================
# Check for required includes
# =============================================================================

find_path (TBB_INCLUDE
  NAMES tbb/tbb.h
  PATHS "${TBB_ROOT_DIR}/include"
  NO_DEFAULT_PATH
)

if (NOT TBB_INCLUDE)
  message (SEND_ERROR "TBB headers not found. Please locate TBB_INCLUDE.")
endif()

# =============================================================================
# Check for required libraries
# =============================================================================

macro(TBB_find_library name)
  if (TBB_COMPILER)
    set (compiler ${TBB_COMPILER})
  else()
    if (MSVC)
      if (MSVC70)
        set (compiler vc7)
      elseif (MSVC80)
        set (compiler vc8)
      elseif (MSVC90)
        set (compiler vc9)
      elseif (MSVC10)
        set (compiler vc10)
      elseif (MSVC11)
        set (compiler vc11)
      elseif (MSVC12)
        set (compiler vc12)
      elseif (MSVC14)
        set (compiler vc14)
      endif()
    elseif (DEFINED CMAKE_COMPILER_IS_GNUCC)
      execute_process(COMMAND ${CMAKE_C_COMPILER} -dumpversion OUTPUT_VARIABLE GCC_VERSION)
      string(REGEX MATCHALL "[0-9]+" GCC_VERSION_COMPONENTS ${GCC_VERSION})
      list(GET GCC_VERSION_COMPONENTS 0 GCC_MAJOR)
      list(GET GCC_VERSION_COMPONENTS 1 GCC_MINOR)
      set (compiler "gcc${GCC_MAJOR}.${GCC_MINOR}")
    elseif (DEFINED CMAKE_COMPILER_IS_GNUCXX)
      set (compiler gxx)
    elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
      set (compiler clang)
    elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
      set (compiler icc)
    else()
      set (compiler ${CMAKE_GENERATOR})
      string (REGEX REPLACE " " "" compiler ${compiler})
    endif()
  endif()

  math (EXPR compiler_bitness "32 + 32*(${CMAKE_SIZEOF_VOID_P}/8)")
  if (${compiler_bitness} STREQUAL 32)
    set (tbb_arch_name ia32)
  else()
    set (tbb_arch_name intel64)
  endif()
  find_library(TBB_${name}_RELEASE_LIBRARY ${name}       PATHS "${TBB_ROOT_DIR}/lib/${tbb_arch_name}/${compiler}" NO_DEFAULT_PATH)
  find_library(TBB_${name}_DEBUG_LIBRARY   ${name}_debug PATHS "${TBB_ROOT_DIR}/lib/${tbb_arch_name}/${compiler}" NO_DEFAULT_PATH)
  find_library(TBB_${name}_RELEASE_LIBRARY ${name})
  find_library(TBB_${name}_DEBUG_LIBRARY   ${name}_debug)
  if(WIN32)
    find_program(TBB_${name}_RELEASE_DLL ${name}.dll       PATHS "${TBB_ROOT_DIR}/bin/${tbb_arch_name}/${compiler}" NO_DEFAULT_PATH)
    find_program(TBB_${name}_DEBUG_DLL   ${name}_debug.dll PATHS "${TBB_ROOT_DIR}/bin/${tbb_arch_name}/${compiler}" NO_DEFAULT_PATH)
  endif()
  set (TBB_COMPILER ${compiler} CACHE STRING "TBB library compiler \${TBB_ROOT_DIR}/lib/${tbb_arch_name}/\${TBB_COMPILER}" FORCE)
  if (NOT TBB_${name}_RELEASE_LIBRARY AND NOT TBB_${name}_DEBUG_LIBRARY)
    message (SEND_ERROR "${name} library not found. Please locate TBB_${name}_RELEASE_LIBRARY or (and) TBB_${name}_DEBUG_LIBRARY.")
  elseif (WIN32 AND NOT TBB_${name}_RELEASE_DLL AND NOT TBB_${name}_DEBUG_DLL)
    message (SEND_ERROR "${name} dll not found. Please locate TBB_${name}_RELEASE_DLL or (and) TBB_${name}_DEBUG_DLL.")
  else()
    set (TBB_${name}_LIBRARY_FOUND ON)
  endif()
endmacro()

TBB_find_library(tbb)
TBB_find_library(tbbmalloc)

FIND_PACKAGE_HANDLE_STANDARD_ARGS (TBB DEFAULT_MSG TBB_INCLUDE TBB_tbb_LIBRARY_FOUND TBB_tbbmalloc_LIBRARY_FOUND)

# =============================================================================
# Set "public" variables
# =============================================================================

if(TBB_FOUND)
  set(TBB_INCLUDE_DIRS      ${TBB_INCLUDE})
  set(TBB_LIBRARIES_RELEASE ${TBB_tbb_RELEASE_LIBRARY} ${TBB_tbbmalloc_RELEASE_LIBRARY})
  set(TBB_LIBRARIES_DEBUG   ${TBB_tbb_DEBUG_LIBRARY}   ${TBB_tbbmalloc_DEBUG_LIBRARY})
  set(TBB_DLLS_RELEASE      ${TBB_tbb_RELEASE_DLL}     ${TBB_tbbmalloc_RELEASE_DLL})
  set(TBB_DLLS_DEBUG        ${TBB_tbb_DEBUG_DLL}       ${TBB_tbbmalloc_DEBUG_DLL})
endif()
