include(FindPackageHandleStandardArgs)
include(macros)

# =============================================================================
# Check for required variables
# =============================================================================

if (NOT FREETYPE_ROOT_DIR)
  message(FATAL_ERROR "FREETYPE_ROOT_DIR not found. Please locate before proceeding.")
endif()

# =============================================================================
# Check for required includes
# =============================================================================

find_path (FREETYPE_INCLUDE
  NAMES freetype/freetype.h freetype.h
  PATHS "${FREETYPE_ROOT_DIR}/include"
  NO_DEFAULT_PATH
)

if (NOT FREETYPE_INCLUDE)
  message (SEND_ERROR "FREETYPE headers not found. Please locate FREETYPE_INCLUDE.")
endif()

# =============================================================================
# Check for required libraries
# =============================================================================

find_library (FREETYPE_LIBRARY freetype ${FREETYPE_ROOT_DIR}/lib)

find_program (FREETYPE_DLL freetype.dll PATHS "${FREETYPE_ROOT_DIR}" "${FREETYPE_ROOT_DIR}/bin" NO_DEFAULT_PATH)


FIND_PACKAGE_HANDLE_STANDARD_ARGS (FREETYPE DEFAULT_MSG
                                   FREETYPE_INCLUDE
                                   FREETYPE_LIBRARY)


# =============================================================================
# Set "public" variables
# =============================================================================
if (FREETYPE_FOUND)
  set (FREETYPE_INCLUDE_DIRS ${FREETYPE_INCLUDE})

  set (FREETYPE_LIBRARIES ${FREETYPE_LIBRARY})
  set (FREETYPE_DLLS ${FREETYPE_DLL})
endif()
