# This script finds OCCT libraries.
#
# The script looks for OCCT default installation package,
# including TKernel and TKMath libraries. Both release
# and debug builds are considered.
#
# The script requires
#
#  OCCT_ROOT_DIR - root directory of OCCT library package.
#
# Once done the script will define
#
#  OCCT_FOUND - package is succesfully found.
#  OCCT_INCLUDE_DIRS - directory containing public headers files.
#  OCCT_LIBRARIES_RELEASE - release version of libraries.
#  OCCT_LIBRARIES_DEBUG - debug version of libraries.
#  OCCT_DLLS_RELEASE - release version of dlls.
#  OCCT_DLLS_DEBUG - debug version of dlls.

include(FindPackageHandleStandardArgs)
include(macros)

# =============================================================================
# Check for required variables
# =============================================================================

if (NOT OCCT_ROOT_DIR)
  message(FATAL_ERROR "OCCT_ROOT_DIR not found. Please locate before proceeding.")
endif()

# =============================================================================
# Check for required includes
# =============================================================================
set (FIND_PATH_PARAMS NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH)
find_path (OCCT_INCLUDE NAMES Standard.hxx PATHS "${OCCT_ROOT_DIR}/inc" ${FIND_PATH_PARAMS})

if (NOT OCCT_INCLUDE)
  message (SEND_ERROR "OCCT headers not found. Please locate OCCT_INCLUDE.")
endif()

# =============================================================================
# Check for required libraries
# =============================================================================
set (OCCT_LIBRARIES TKBin TKBinL TKBinTObj TKBinXCAF TKBO TKBool TKBRep TKCAF TKCDF TKDCAF
     TKDraw TKernel TKFeat TKFillet TKG2d TKG3d TKGeomAlgo TKGeomBase TKHLR TKIGES TKLCAF
     TKMath TKMesh TKMeshVS TKOffset TKOpenGl TKPrim TKQADraw TKRWMesh TKService TKShHealing
     TKStd TKStdL TKSTEP TKSTEP209 TKSTEPAttr TKSTEPBase TKSTL TKTObj TKTObjDRAW TKTopAlgo
     TKTopTest TKV3d TKVCAF TKViewerTest TKVRML TKXCAF TKXDEDRAW TKXDEIGES TKXDESTEP TKXMesh
     TKXml TKXmlL TKXmlTObj TKXmlXCAF TKXSBase TKXSDRAW)

list (REMOVE_DUPLICATES OCCT_LIBRARIES)

set (OCCT_LIBRARIES_FOUND)
foreach (Toolkit ${OCCT_LIBRARIES})
  _FIND_LIBRARY (OCCT ${Toolkit})
  list (APPEND OCCT_LIBRARIES_FOUND OCCT_${Toolkit}_LIBRARY_FOUND)
endforeach(Toolkit)

FIND_PACKAGE_HANDLE_STANDARD_ARGS (OCCT DEFAULT_MSG OCCT_INCLUDE ${OCCT_LIBRARIES_FOUND})

# =============================================================================
# Set "public" variables
# =============================================================================

if (OCCT_FOUND)
  set (OCCT_INCLUDE_DIRS ${OCCT_INCLUDE})

  set (OCCT_LIBRARIES_RELEASE)
  set (OCCT_LIBRARIES_DEBUG)
  set (OCCT_DLLS_RELEASE)
  set (OCCT_DLLS_DEBUG)
  foreach (Toolkit ${OCCT_LIBRARIES})
    list (APPEND OCCT_LIBRARIES_RELEASE ${OCCT_${Toolkit}_RELEASE_LIBRARY})
    list (APPEND OCCT_LIBRARIES_DEBUG ${OCCT_${Toolkit}_DEBUG_LIBRARY})
    if(WIN32)
      list (APPEND OCCT_DLLS_RELEASE ${OCCT_${Toolkit}_RELEASE_DLL})
      list (APPEND OCCT_DLLS_DEBUG ${OCCT_${Toolkit}_DEBUG_DLL})
    endif()
  endforeach(Toolkit)
endif()
