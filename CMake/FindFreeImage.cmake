include(FindPackageHandleStandardArgs)
include(macros)

# =============================================================================
# Check for required variables
# =============================================================================

if (NOT FREEIMAGE_ROOT_DIR)
  message(FATAL_ERROR "FREEIMAGE_ROOT_DIR not found. Please locate before proceeding.")
endif()

# =============================================================================
# Check for required includes
# =============================================================================

find_path (FREEIMAGE_INCLUDE
  NAMES FreeImage.h
  PATHS "${FREEIMAGE_ROOT_DIR}/include"
  NO_DEFAULT_PATH
)

if (NOT FREEIMAGE_INCLUDE)
  message (SEND_ERROR "FREEIMAGE headers not found. Please locate FREEIMAGE_INCLUDE.")
endif()

# =============================================================================
# Check for required libraries
# =============================================================================

find_library (FREEIMAGE_LIBRARY FreeImage ${FREEIMAGE_ROOT_DIR}/lib)

find_program (FREEIMAGE_DLL FreeImage.dll PATHS "${FREEIMAGE_ROOT_DIR}" "${FREEIMAGE_ROOT_DIR}/bin" NO_DEFAULT_PATH)
find_program (FREEIMAGE_PLUS_DLL FreeImagePlus.dll PATHS "${FREEIMAGE_ROOT_DIR}" "${FREEIMAGE_ROOT_DIR}/bin" NO_DEFAULT_PATH)


FIND_PACKAGE_HANDLE_STANDARD_ARGS (FREEIMAGE DEFAULT_MSG
                                   FREEIMAGE_INCLUDE
                                   FREEIMAGE_LIBRARY)


# =============================================================================
# Set "public" variables
# =============================================================================
if (FREEIMAGE_FOUND)
  set (FREEIMAGE_INCLUDE_DIRS ${FREEIMAGE_INCLUDE})

  set (FREEIMAGE_LIBRARIES ${FREEIMAGE_LIBRARY})
  set (FREEIMAGE_DLLS ${FREEIMAGE_DLL} ${FREEIMAGE_PLUS_DLL})
endif()
