# This script defines common macro

include (CMakeParseArguments)

macro (target_link_libraries_config_aware target lib)
  if (NOT ${lib}_LIBRARIES_RELEASE AND NOT ${lib}_LIBRARIES_DEBUG)
    target_link_libraries (${target} ${${lib}_LIBRARIES})
  else()
    set (LINK_LIBRARIES_RELEASE)
    set (LINK_LIBRARIES_DEBUG)

    if (NOT ${lib}_LIBRARIES_RELEASE)
      list (APPEND LINK_LIBRARIES_RELEASE ${${lib}_LIBRARIES_DEBUG})
    else()
      list (APPEND LINK_LIBRARIES_RELEASE ${${lib}_LIBRARIES_RELEASE})
    endif()

    foreach (LINK_LIBRARY_RELEASE ${LINK_LIBRARIES_RELEASE})
      target_link_libraries (${target} optimized ${LINK_LIBRARY_RELEASE})
    endforeach()

    if (NOT ${lib}_LIBRARIES_DEBUG)
      list (APPEND LINK_LIBRARIES_DEBUG ${${lib}_LIBRARIES_RELEASE})
    else()
      list (APPEND LINK_LIBRARIES_DEBUG ${${lib}_LIBRARIES_DEBUG})
    endif()

    foreach (LINK_LIBRARY_DEBUG ${LINK_LIBRARIES_DEBUG})
      target_link_libraries (${target} debug ${LINK_LIBRARY_DEBUG})
    endforeach()
  endif()
endmacro()

function(_install_shared_libraries libraries)
  set(options)
  set(oneValueArgs DESTINATION)
  set(multiValueArgs CONFIGURATIONS)
  cmake_parse_arguments(install "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
  foreach (library ${libraries})
     if (UNIX)
       get_filename_component (library_symlink_path ${library} REALPATH)
       execute_process (COMMAND objdump -p ${library}
                        COMMAND grep SONAME
                        COMMAND sed "s/SONAME//;s/^ *//;s/ *$//"
                        COMMAND tr -d "\n"
                        OUTPUT_VARIABLE library_soname
                        ERROR_QUIET)
       if (NOT library_soname)
         execute_process (COMMAND cat ${library}
                          COMMAND grep -o "INPUT ([^)]*)"
                          COMMAND sed "s/INPUT (//;s/)//;s/ *$//"
                          COMMAND tr -d "\n"
                          OUTPUT_VARIABLE library_soname
                          ERROR_QUIET)
         if (library_soname)
           get_filename_component (library_directory ${library} DIRECTORY)
           set (library_symlink_path ${library_directory}/${library_soname})
         endif()
       endif()
       if (NOT library_soname)
         get_filename_component (library_soname ${library} NAME)
       endif()
       if (install_CONFIGURATIONS)
         install(FILES ${library_symlink_path} DESTINATION ${install_DESTINATION} CONFIGURATIONS ${install_CONFIGURATIONS} RENAME ${library_soname})
       else()
         install(FILES ${library_symlink_path} DESTINATION ${install_DESTINATION} RENAME ${library_soname})
       endif()
    else()
       if (install_CONFIGURATIONS)
         install(FILES ${library} DESTINATION ${install_DESTINATION} CONFIGURATIONS ${install_CONFIGURATIONS})
       else()
         install(FILES ${library} DESTINATION ${install_DESTINATION})
       endif()
    endif()
  endforeach()
endfunction()

function(install_shared_libraries libraries)
  set(options)
  set(oneValueArgs DESTINATION)
  set(multiValueArgs)
  cmake_parse_arguments (install "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
  if (NOT ${libraries}_RELEASE AND NOT ${libraries}_DEBUG)
    _install_shared_libraries ("${${libraries}}" DESTINATION ${install_DESTINATION})
  else()
    set (libraries_release)
    set (libraries_debug)

    if (NOT ${libraries}_RELEASE)
      list (APPEND libraries_release ${${libraries}_DEBUG})
    else()
      list (APPEND libraries_release ${${libraries}_RELEASE})
    endif()

    _install_shared_libraries ("${libraries_release}" DESTINATION ${install_DESTINATION} CONFIGURATIONS ${CMAKE_RELEASE_CONFIGURATIONS})

    if (NOT ${libraries}_DEBUG)
      list (APPEND libraries_debug ${${libraries}_RELEASE})
    else()
      list (APPEND libraries_debug ${${libraries}_DEBUG})
    endif()

    _install_shared_libraries ("${libraries_debug}" DESTINATION ${install_DESTINATION} CONFIGURATIONS ${CMAKE_DEBUG_CONFIGURATIONS})
  endif()
endfunction()
