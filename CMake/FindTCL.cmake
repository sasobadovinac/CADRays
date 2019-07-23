# This script finds TCL libraries.
#
# The script looks for TCL release package,
#
# The script requires
#
#  TCL_ROOT_DIR - root directory of TCL binary release package.
#
# Once done the script will define
#
#  TCL_FOUND - package is succesfully found.
#  TCL_INCLUDE_DIRS - directory containing public headers files.
#  TCL_LIBRARIES - libraries.
#  TCL_DLLS - dlls.

include(FindPackageHandleStandardArgs)

# =============================================================================
# Check for required variables
# =============================================================================

if (NOT TCL_ROOT_DIR)
  message(FATAL_ERROR "TCL_ROOT_DIR not found. Please locate before proceeding.")
endif()

# =============================================================================
# Check for required includes
# =============================================================================

find_path (TCL_INCLUDE
  NAMES tcl.h
  PATHS "${TCL_ROOT_DIR}/include"
  NO_DEFAULT_PATH
)

if (NOT TCL_INCLUDE)
  message (SEND_ERROR "TCL headers not found. Please locate TCL_INCLUDE.")
endif()

# =============================================================================
# Check for required libraries
# =============================================================================

find_library(TCL_LIBRARY tcl86 tcl85 PATHS "${TCL_ROOT_DIR}/lib" NO_DEFAULT_PATH)

if(WIN32)
  find_program(TCL_DLL tcl86.dll tcl85.dll  PATHS "${TCL_ROOT_DIR}/bin" NO_DEFAULT_PATH)
  find_program(TK_DLL tk86.dll tk85.dll  PATHS "${TCL_ROOT_DIR}/bin" NO_DEFAULT_PATH)
endif()

FIND_PACKAGE_HANDLE_STANDARD_ARGS (TCL DEFAULT_MSG TCL_INCLUDE TCL_LIBRARY)

# =============================================================================
# Set "public" variables
# =============================================================================

if(TCL_FOUND)
  set(TCL_INCLUDE_DIRS ${TCL_INCLUDE})
  set(TCL_LIBRARIES    ${TCL_LIBRARY})
  set(TCL_DLLS         ${TCL_DLL} ${TK_DLL})
endif()
