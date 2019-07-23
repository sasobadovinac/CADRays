
# COMPILER_BITNESS variable
macro (MAKE_COMPILER_BITNESS)
  math (EXPR COMPILER_BITNESS "32 + 32*(${CMAKE_SIZEOF_VOID_P}/8)")
endmacro()

# OS_WITH_BIT
macro (MAKE_OS_WITH_BITNESS)

  MAKE_COMPILER_BITNESS()

  if (WIN32)
    set (OS_WITH_BIT "win${COMPILER_BITNESS}")
  elseif(APPLE)
    set (OS_WITH_BIT "mac${COMPILER_BITNESS}")
  else ()
    set (OS_WITH_BIT "lin${COMPILER_BITNESS}")
  endif()
endmacro()

# COMPILER variable
macro (MAKE_COMPILER_SHORT_NAME)
  if (MSVC)
    if (MSVC70)
      set (COMPILER vc7)
    elseif (MSVC80)
      set (COMPILER vc8)
    elseif (MSVC90)
      set (COMPILER vc9)
    elseif (MSVC10)
      set (COMPILER vc10)
    elseif (MSVC11)
      set (COMPILER vc11)
    elseif (MSVC12)
      set (COMPILER vc12)
    elseif (MSVC14)
      set (COMPILER vc14)
    endif()
  elseif (DEFINED CMAKE_COMPILER_IS_GNUCC)
    set (COMPILER gcc)
  elseif (DEFINED CMAKE_COMPILER_IS_GNUCXX)
    set (COMPILER gxx)
  elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    set (COMPILER clang)
  elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
    set (COMPILER icc)
  else()
    set (COMPILER ${CMAKE_GENERATOR})
    string (REGEX REPLACE " " "" COMPILER ${COMPILER})
  endif()
endmacro()

# 3rdparty found
macro (FIND_THIRD_PARTY thirdPartyDir component componentFolder)
  set(THIRDPARTY_COMPONENT_FOUND NO)
  if (NOT ${thirdPartyDir} STREQUAL "")
    FIND_PRODUCT_DIR("${3RDPARTY_DIR}" ${component} ${componentFolder})
    if (NOT ${componentFolder})
      set (componentFolder "")
    else()
      set(THIRDPARTY_COMPONENT_FOUND YES)
    endif()
  else()
    set(THIRDPARTY_COMPONENT_FOUND NO)
  endif()
endmacro()

# copies file on each build
macro (add_copy_command target source destanation)
  if (EXISTS ${source})
    add_custom_command(
            TARGET ${target} PRE_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
                    ${source}
                    ${destanation})
  endif()
endmacro()

# COMPILER variable
macro (_FIND_LIBRARY prefix name)
  set (FIND_PATH_PARAMS NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH)
  
  MAKE_OS_WITH_BITNESS()
  MAKE_COMPILER_SHORT_NAME()
  
  # Set correct prefix.
  if (${prefix} STREQUAL ${name} OR ${name} STREQUAL "")
    set (TOOLKIT_PREFIX "${prefix}")
    set (MSG_PREFIX "${prefix}")
  else()
    set (TOOLKIT_PREFIX "${prefix}_${name}")
    set (MSG_PREFIX "${prefix} ${name}")
  endif()

  # Find library files.
  if(WIN32)
    find_program (${TOOLKIT_PREFIX}_RELEASE_DLL NAMES ${name}.dll PATHS "${${prefix}_ROOT_DIR}/${OS_WITH_BIT}/${COMPILER}/bin"  ${FIND_PATH_PARAMS})
    find_program (${TOOLKIT_PREFIX}_DEBUG_DLL   NAMES ${name}.dll PATHS "${${prefix}_ROOT_DIR}/${OS_WITH_BIT}/${COMPILER}/bind" ${FIND_PATH_PARAMS})
  endif()
    
  find_library (${TOOLKIT_PREFIX}_RELEASE_LIBRARY ${name} PATHS "${${prefix}_ROOT_DIR}/${OS_WITH_BIT}/${COMPILER}/lib"  ${FIND_PATH_PARAMS})
  find_library (${TOOLKIT_PREFIX}_DEBUG_LIBRARY   ${name} PATHS "${${prefix}_ROOT_DIR}/${OS_WITH_BIT}/${COMPILER}/libd" ${FIND_PATH_PARAMS})
  
  if (NOT ${TOOLKIT_PREFIX}_RELEASE_LIBRARY AND NOT ${TOOLKIT_PREFIX}_DEBUG_LIBRARY)
    message (SEND_ERROR "${MSG_PREFIX} library not found. Please locate ${TOOLKIT_PREFIX}_RELEASE_LIBRARY or (and) ${TOOLKIT_PREFIX}_DEBUG_LIBRARY.") 
  elseif (WIN32 AND NOT ${TOOLKIT_PREFIX}_RELEASE_DLL AND NOT ${TOOLKIT_PREFIX}_DEBUG_DLL)
    message (SEND_ERROR "${MSG_PREFIX} dll not found. Please locate ${TOOLKIT_PREFIX}_RELEASE_DLL or (and) ${TOOLKIT_PREFIX}_DEBUG_DLL.")
  else()
    if (NOT ${TOOLKIT_PREFIX}_RELEASE_LIBRARY)
      message (STATUS "Warning: ${MSG_PREFIX} release library not found. Please locate ${TOOLKIT_PREFIX}_RELEASE_LIBRARY.") 
    elseif (NOT ${TOOLKIT_PREFIX}_DEBUG_LIBRARY)
      message (STATUS "Warning: ${MSG_PREFIX} debug library not found. Please locate ${TOOLKIT_PREFIX}_DEBUG_LIBRARY.") 
    endif()
    
    if (WIN32)
      if (NOT ${TOOLKIT_PREFIX}_RELEASE_DLL)
        message (STATUS "Warning: ${MSG_PREFIX} release dll not found. Please locate ${TOOLKIT_PREFIX}_RELEASE_DLL.")
      elseif (NOT ${TOOLKIT_PREFIX}_DEBUG_DLL)
        message (STATUS "Warning: ${MSG_PREFIX} debug dll not found. Please locate ${TOOLKIT_PREFIX}_DEBUG_DLL.")
      endif()
    endif()
    
    set (${TOOLKIT_PREFIX}_LIBRARY_FOUND ON)
  endif()
endmacro()

function (SUBDIRECTORY_NAMES MAIN_DIRECTORY RESULT)
  file (GLOB SUB_ITEMS "${MAIN_DIRECTORY}/*")

  foreach (ITEM ${SUB_ITEMS})
    if (IS_DIRECTORY "${ITEM}")
      get_filename_component (ITEM_NAME "${ITEM}" NAME)
      list (APPEND LOCAL_RESULT "${ITEM_NAME}")
    endif()
  endforeach()
  set (${RESULT} ${LOCAL_RESULT} PARENT_SCOPE)
endfunction()

function (FIND_PRODUCT_DIR ROOT_DIR PRODUCT_NAME RESULT)
  MAKE_COMPILER_BITNESS()
  MAKE_COMPILER_SHORT_NAME()

  string (TOLOWER "${PRODUCT_NAME}" lower_PRODUCT_NAME)
  
  if (DEFINED ANDROID_ABI)
    string(COMPARE EQUAL ${ANDROID_ABI} "x86" IS_ANDROID_INTEL)
    
    if (IS_ANDROID_INTEL)
      set (COMPILER_BITNESS "android-intel")
    else()
      set (COMPILER_BITNESS "android-arm")
    endif()
  endif()
  
  list (APPEND SEARCH_TEMPLATES "${lower_PRODUCT_NAME}.*${COMPILER}.*${COMPILER_BITNESS}")
  list (APPEND SEARCH_TEMPLATES "${lower_PRODUCT_NAME}.*[0-9.]+.*${COMPILER}.*${COMPILER_BITNESS}")
  list (APPEND SEARCH_TEMPLATES "${lower_PRODUCT_NAME}.*[0-9.]+.*${COMPILER_BITNESS}")
  list (APPEND SEARCH_TEMPLATES "${lower_PRODUCT_NAME}.*[0-9.]+")
  list (APPEND SEARCH_TEMPLATES "${lower_PRODUCT_NAME}")
  
  SUBDIRECTORY_NAMES ("${ROOT_DIR}" SUBDIR_NAME_LIST)

  foreach (SEARCH_TEMPLATE ${SEARCH_TEMPLATES})
    if (LOCAL_RESULT)
      BREAK()
    endif()

    foreach (SUBDIR_NAME ${SUBDIR_NAME_LIST})
      string (TOLOWER "${SUBDIR_NAME}" lower_SUBDIR_NAME)

      string (REGEX MATCH "${SEARCH_TEMPLATE}" DUMMY_VAR "${lower_SUBDIR_NAME}")
      if (DUMMY_VAR)
        list (APPEND LOCAL_RESULT ${SUBDIR_NAME})
      endif()
    endforeach()
  endforeach()

  if (LOCAL_RESULT)
    list (LENGTH "${LOCAL_RESULT}" LOC_LEN)
    math (EXPR LAST_ELEMENT_INDEX "${LOC_LEN}-1")
    list (GET LOCAL_RESULT ${LAST_ELEMENT_INDEX} DUMMY)
    set (${RESULT} ${DUMMY} PARENT_SCOPE)
  endif()
endfunction()