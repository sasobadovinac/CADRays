if(CMAKE_SIZEOF_VOID_P EQUAL 8)
	set(ASSIMP_ARCHITECTURE "64")
elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
	set(ASSIMP_ARCHITECTURE "32")
endif(CMAKE_SIZEOF_VOID_P EQUAL 8)
	
if(WIN32)
	set(ASSIMP_ROOT_DIR CACHE PATH "ASSIMP root directory")

	# Find path of each library
	find_path(ASSIMP_INCLUDE_DIR
		NAMES
			assimp/anim.h
		HINTS
			${ASSIMP_ROOT_DIR}/include
	)

	if(MSVC12)
		set(ASSIMP_MSVC_VERSION "vc120")
	elseif(MSVC14)	
		set(ASSIMP_MSVC_VERSION "vc140")
	endif(MSVC12)
	
	if(MSVC12 OR MSVC14)
        if ("${BUILD_CONFIGURATION}" STREQUAL "Debug")
            find_library(ASSIMP_LIBRARY_DEBUG assimp-${ASSIMP_MSVC_VERSION}-mdd assimp-${ASSIMP_MSVC_VERSION}-mtd PATHS ${ASSIMP_ROOT_DIR}/lib NO_DEFAULT_PATH)
            set(ASSIMP_LIBRARY 
                debug        ${ASSIMP_LIBRARY_DEBUG}
            )

            find_program(ASSIMP_DLL_DEBUG assimp-${ASSIMP_MSVC_VERSION}-mdd.dll assimp-${ASSIMP_MSVC_VERSION}-mtd.dll PATHS ${ASSIMP_ROOT_DIR}/bin NO_DEFAULT_PATH)
            set(ASSIMP_DLL
                debug		${ASSIMP_DLL_DEBUG}
            )
        else()
            find_library(ASSIMP_LIBRARY_RELEASE assimp-${ASSIMP_MSVC_VERSION}-md assimp-${ASSIMP_MSVC_VERSION}-mt PATHS ${ASSIMP_ROOT_DIR}/lib NO_DEFAULT_PATH)
            set(ASSIMP_LIBRARY 
                optimized 	${ASSIMP_LIBRARY_RELEASE}
            )

            find_program(ASSIMP_DLL_RELEASE assimp-${ASSIMP_MSVC_VERSION}-md.dll assimp-${ASSIMP_MSVC_VERSION}-mt.dll PATHS ${ASSIMP_ROOT_DIR}/bin NO_DEFAULT_PATH)
            set(ASSIMP_DLL
                optimized 	${ASSIMP_DLL_RELEASE}
            )
        endif()
    endif()
	
else(WIN32)

	find_path(
	  assimp_INCLUDE_DIRS
	  NAMES postprocess.h scene.h version.h config.h cimport.h
	  PATHS /usr/local/include/assimp
	)

	find_library(
	  assimp_LIBRARIES
	  NAMES assimp
	  PATHS /usr/local/lib/
	)

	if (assimp_INCLUDE_DIRS AND assimp_LIBRARIES)
	  SET(assimp_FOUND TRUE)
	ENDIF (assimp_INCLUDE_DIRS AND assimp_LIBRARIES)

	if (assimp_FOUND)
	  if (NOT assimp_FIND_QUIETLY)
		message(STATUS "Found asset importer library: ${assimp_LIBRARIES}")
	  endif (NOT assimp_FIND_QUIETLY)
	else (assimp_FOUND)
	  if (assimp_FIND_REQUIRED)
		message(FATAL_ERROR "Could not find asset importer library")
	  endif (assimp_FIND_REQUIRED)
	endif (assimp_FOUND)
	
endif(WIN32)