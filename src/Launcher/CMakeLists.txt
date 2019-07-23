cmake_minimum_required (VERSION 2.8.10 FATAL_ERROR)

project (CADRays)

if (POLICY CMP0020)
  cmake_policy (SET CMP0020 NEW)
endif()

if (POLICY CMP0043)
  cmake_policy (SET CMP0043 NEW)
endif()

# =============================================================================
# Look for required dependencies
# =============================================================================

find_package(OCCT REQUIRED)
find_package(OpenGL REQUIRED)
find_package(FreeType REQUIRED)
find_package(FreeImage REQUIRED)
find_package(TCL REQUIRED)
find_package(GLFW REQUIRED)
find_package(assimp REQUIRED)

set (OPENGL_LIBRARIES ${OPENGL_gl_LIBRARY} ${OPENGL_glu_LIBRARY})

get_target_property (UTILS_INCLUDE_DIRS Utils INTERFACE_INCLUDE_DIRECTORIES)
get_target_property (IMGUI_INCLUDE_DIRS ImGui INTERFACE_INCLUDE_DIRECTORIES)
get_target_property (IMPORTEXPORT_INCLUDE_DIRS ImportExport INTERFACE_INCLUDE_DIRECTORIES)

# =============================================================================
# Define production steps
# =============================================================================

include_directories (
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${UTILS_INCLUDE_DIRS}
  ${IMGUI_INCLUDE_DIRS}
  ${IMPORTEXPORT_INCLUDE_DIRS}
  ${FREETYPE_INCLUDE_DIRS}
  ${OCCT_INCLUDE_DIRS}
  ${TCL_INCLUDE_DIRS}
  ${GLFW_INCLUDE_DIRS}
  ${ASSIMP_INCLUDE_DIR}
)

file (GLOB_RECURSE ProjectSources *.c*)
file (GLOB_RECURSE ProjectHeaders *.h*)

if (WIN32)
  enable_language (RC)
  set (EXE_ICON CADRays.rc)
endif()

source_group ("Header Files" FILES
  ${ProjectHeaders}
)

source_group ("Resources" FILES
  ${EXE_ICON}
)

add_executable (${PROJECT_NAME} ${ProjectSources} ${ProjectHeaders} ${EXE_ICON})

add_dependencies (${PROJECT_NAME} ImGui)
add_dependencies (${PROJECT_NAME} ImportExport)

target_link_libraries_config_aware (${PROJECT_NAME} OCCT)

target_link_libraries (${PROJECT_NAME} ${OPENGL_LIBRARIES})
target_link_libraries (${PROJECT_NAME} ${TCL_LIBRARIES})
target_link_libraries (${PROJECT_NAME} ${GLFW_LIBRARIES})
target_link_libraries (${PROJECT_NAME} Utils)
target_link_libraries (${PROJECT_NAME} ImGui)
target_link_libraries (${PROJECT_NAME} ImportExport)

# =============================================================================
# Define MSVC debug environment
# =============================================================================

if (MSVC)
  foreach (config ${CMAKE_DEBUG_CONFIGURATIONS})
    foreach (lib ${OCCT_DLLS_DEBUG} ${FREETYPE_DLLS} ${FREEIMAGE_DLLS} ${TBB_DLLS_DEBUG} ${TCL_DLLS} ${ASSIMP_DLL})
      get_filename_component (lib_dir ${lib} PATH)
      list (APPEND VSTUDIO_${config}_ENVIRONMENT ${lib_dir})
    endforeach()
  endforeach()
  
  foreach (config ${CMAKE_RELEASE_CONFIGURATIONS})
    foreach (lib ${OCCT_DLLS_RELEASE} ${FREETYPE_DLLS} ${FREEIMAGE_DLLS} ${TBB_DLLS_RELEASE} ${TCL_DLLS} ${ASSIMP_DLL})
      get_filename_component (lib_dir ${lib} PATH)
      list (APPEND VSTUDIO_${config}_ENVIRONMENT ${lib_dir})
    endforeach()
  endforeach()

  foreach (depends ImGui ImportExport)
    foreach (config ${CMAKE_DEBUG_CONFIGURATIONS} ${CMAKE_RELEASE_CONFIGURATIONS})
      list (APPEND VSTUDIO_${config}_ENVIRONMENT ${CMAKE_BINARY_DIR}/${depends}/${config})
    endforeach()
  endforeach()

  foreach (config ${CMAKE_DEBUG_CONFIGURATIONS} ${CMAKE_RELEASE_CONFIGURATIONS})
    list (REMOVE_DUPLICATES VSTUDIO_${config}_ENVIRONMENT)
  endforeach()

  configure_file (${CMAKE_MODULE_PATH}/VStudio.vcxproj.user.in ${CMAKE_BINARY_DIR}/src/Launcher/CADRays.vcxproj.user @ONLY) 
endif()

configure_file (${CMAKE_MODULE_PATH}/sign.bat.in ${CMAKE_BINARY_DIR}/sign.bat @ONLY) 

# =============================================================================
# Define install steps
# =============================================================================

install (TARGETS ${PROJECT_NAME} RUNTIME DESTINATION "${CMAKE_INSTALL_PREFIX}")

install_shared_libraries (OCCT_DLLS DESTINATION "${CMAKE_INSTALL_PREFIX}")
install_shared_libraries (FREETYPE_DLLS DESTINATION "${CMAKE_INSTALL_PREFIX}")
install_shared_libraries (FREEIMAGE_DLLS DESTINATION "${CMAKE_INSTALL_PREFIX}")
install_shared_libraries (TBB_DLLS DESTINATION "${CMAKE_INSTALL_PREFIX}")
install_shared_libraries (TCL_DLLS DESTINATION "${CMAKE_INSTALL_PREFIX}")
install_shared_libraries (ASSIMP_DLL DESTINATION "${CMAKE_INSTALL_PREFIX}")
install_shared_libraries (GLFW_DLL DESTINATION "${CMAKE_INSTALL_PREFIX}")
install_shared_libraries (FFMPEG_DLLS DESTINATION "${CMAKE_INSTALL_PREFIX}")

set (CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS_SKIP ON)
set (CMAKE_INSTALL_DEBUG_LIBRARIES ON)
unset (CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS)
include (InstallRequiredSystemLibraries)
set (CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS_DEBUG ${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS})

set (CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS_SKIP ON)
unset (CMAKE_INSTALL_DEBUG_LIBRARIES)
unset (CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS)
include (InstallRequiredSystemLibraries)
set (CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS_RELEASE ${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS})

# install (PROGRAMS ${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS_DEBUG} DESTINATION "${CMAKE_INSTALL_PREFIX}" CONFIGURATIONS ${CMAKE_DEBUG_CONFIGURATIONS})
# install (PROGRAMS ${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS_RELEASE} DESTINATION "${CMAKE_INSTALL_PREFIX}" CONFIGURATIONS ${CMAKE_RELEASE_CONFIGURATIONS})

# copy data
install (DIRECTORY ${CMAKE_SOURCE_DIR}/data DESTINATION ${CMAKE_INSTALL_PREFIX}
         FILES_MATCHING PATTERN "*.png" PATTERN "*.ttf" PATTERN "DrawPlugin" PATTERN "*.tcl" PATTERN "*.jpg" PATTERN "*.html" PATTERN "*.txt")

install (DIRECTORY ${OCCT_ROOT_DIR}/src/DrawResources DESTINATION ${CMAKE_INSTALL_PREFIX}/occt-tcl/src)
install (DIRECTORY ${OCCT_ROOT_DIR}/src/Shaders DESTINATION ${CMAKE_INSTALL_PREFIX}/occt-tcl/src)

install (FILES "${CMAKE_SOURCE_DIR}/data/.cadrays-installed" DESTINATION ${CMAKE_INSTALL_PREFIX})