cmake_minimum_required (VERSION 2.8.10 FATAL_ERROR)

project (ImGui)

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

set (OPENGL_LIBRARIES ${OPENGL_gl_LIBRARY} ${OPENGL_glu_LIBRARY})

get_target_property (UTILS_INCLUDE_DIRS Utils INTERFACE_INCLUDE_DIRECTORIES)

# =============================================================================
# Define production steps
# =============================================================================

include_directories (
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${UTILS_INCLUDE_DIRS}
  ${FREETYPE_INCLUDE_DIRS}
  ${OCCT_INCLUDE_DIRS}
)

file (GLOB_RECURSE ProjectSources *.cxx)
file (GLOB_RECURSE ProjectHeaders *.h)

source_group ("Header Files" FILES
  ${ProjectHeaders}
)

add_library (${PROJECT_NAME} SHARED
  ${ProjectSources}
  ${ProjectHeaders}
)

target_link_libraries_config_aware (${PROJECT_NAME} OCCT)
target_link_libraries (${PROJECT_NAME} ${OPENGL_LIBRARIES})
target_link_libraries (${PROJECT_NAME} Utils)

set_target_properties (${PROJECT_NAME} PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}")

# =============================================================================
# Define install steps
# =============================================================================

install (TARGETS ${PROJECT_NAME} RUNTIME DESTINATION "${CMAKE_INSTALL_PREFIX}")