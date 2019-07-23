include(FindPackageHandleStandardArgs)
include(macros)

# =============================================================================
# Check for required variables
# =============================================================================

if (NOT FFMPEG_ROOT_DIR)
  message(FATAL_ERROR "FFMPEG_ROOT_DIR not found. Please locate before proceeding.")
endif()

# =============================================================================
# Check for required includes
# =============================================================================

find_path (FFMPEG_INCLUDE
  NAMES libavcodec/avcodec.h
  PATHS "${FFMPEG_ROOT_DIR}/include"
  NO_DEFAULT_PATH
)

if (NOT FFMPEG_INCLUDE)
  message (SEND_ERROR "FFMPEG headers not found. Please locate FFMPEG_INCLUDE.")
endif()

# =============================================================================
# Check for required libraries
# =============================================================================
set (FFMPEG_LIBS "AVCODEC" "AVUTIL" "AVFILTER" "AVFORMAT" "SWSCALE")
foreach (LIBRARY_NAME ${FFMPEG_LIBS})
  find_library (FFMPEG_${LIBRARY_NAME}_LIBRARY ${LIBRARY_NAME} ${FFMPEG_ROOT_DIR}/lib)
  file (GLOB FFMPEG_${LIBRARY_NAME}_DLL "${FFMPEG_ROOT_DIR}/bin/${LIBRARY_NAME}[-][0-9]*")
  if (FFMPEG_${LIBRARY_NAME}_DLL AND EXISTS "${FFMPEG_${LIBRARY_NAME}_DLL}")
    set (FFMPEG_${LIBRARY_NAME}_DLL "${FFMPEG_${LIBRARY_NAME}_DLL}" CACHE FILEPATH "FFmpeg shared library (${LIBRARY_NAME})" FORCE)
  endif()
endforeach()

FIND_PACKAGE_HANDLE_STANDARD_ARGS (FFMPEG DEFAULT_MSG
                                   FFMPEG_INCLUDE
                                   FFMPEG_AVCODEC_LIBRARY)


# =============================================================================
# Set "public" variables
# =============================================================================
if (FFMPEG_FOUND)
  set (FFMPEG_INCLUDE_DIRS ${FFMPEG_INCLUDE})

  set (FFMPEG_LIBRARIES
    ${FFMPEG_AVCODEC_LIBRARY}
    ${FFMPEG_AVUTIL_LIBRARY}
    ${FFMPEG_AVFILTER_LIBRARY}
    ${FFMPEG_SWSCALE_LIBRARY}
    ${FFMPEG_AVFORMAT_LIBRARY})
  set (FFMPEG_DLLS
    ${FFMPEG_AVCODEC_DLL}
    ${FFMPEG_AVUTIL_DLL}
    ${FFMPEG_AVFILTER_DLL}
    ${FFMPEG_AVFORMAT_DLL}
    ${FFMPEG_SWSCALE_DLL})
endif()
