# This script finds GLFW libraries.
#
# The script looks for GLFW release package,
#
# The script requires
#
#  GLFW_ROOT_DIR - root directory of GLFW binary release package.
#
# Once done the script will define
#
#  GLFW_FOUND - package is succesfully found.
#  GLFW_INCLUDE_DIRS - directory containing public headers files.
#  GLFW_LIBRARIES - libraries.

include(FindPackageHandleStandardArgs)

# =============================================================================
# Check for required variables
# =============================================================================

if (NOT GLFW_ROOT_DIR)
  message(FATAL_ERROR "GLFW_ROOT_DIR not found. Please locate before proceeding.")
endif()

# =============================================================================
# Check for required includes
# =============================================================================

find_path (GLFW_INCLUDE
  NAMES glfw3.h
  PATHS "${GLFW_ROOT_DIR}"
  PATH_SUFFIXES include include/GLFW 
  NO_DEFAULT_PATH
)

if (NOT GLFW_INCLUDE)
  message (SEND_ERROR "GLFW headers not found. Please locate GLFW_INCLUDE.")
endif()

# =============================================================================
# Check for required libraries
# =============================================================================

find_library(GLFW_LIBRARY NAMES glfw3 glfw3dll PATHS "${GLFW_ROOT_DIR}/lib" NO_DEFAULT_PATH)
find_program(GLFW_DLL glfw3.dll PATHS ${GLFW_ROOT_DIR}/bin NO_DEFAULT_PATH)

FIND_PACKAGE_HANDLE_STANDARD_ARGS (GLFW DEFAULT_MSG GLFW_INCLUDE GLFW_LIBRARY)

# =============================================================================
# Set "public" variables
# =============================================================================

if(GLFW_FOUND)
  set(GLFW_INCLUDE_DIRS ${GLFW_INCLUDE})
  set(GLFW_LIBRARIES    ${GLFW_LIBRARY})
endif()
