cmake_minimum_required(VERSION 2.8)

project (reaper_libsndfilewrapper)

SET(CMAKE_BUILD_TYPE MinSizeRel)

IF(APPLE)
   SET( CMAKE_OSX_ARCHITECTURES i386;x86_64 )
   SET( CMAKE_OSX_DEPLOYMENT_TARGET 10.5 )
   SET ( LINK_FLAGS MINSIZEREL "-dead_strip" )
   # SET( CMAKE_SHARED_LINKER_FLAGS_MINSIZEREL "-dead_strip" )
   # SET( CMAKE_MODULE_LINKER_FLAGS_MINSIZEREL "-dead_strip" )
   # SET( CMAKE_EXE_LINKER_FLAGS_MINSIZEREL "-dead_strip" )
	ADD_DEFINITIONS(
		-std=c++98
		-stdlib=libstdc++
		-Wno-c++11-extensions #avoid c++11 warning msg
	)
	
	FIND_LIBRARY(CARBON_LIBRARY Carbon)
	FIND_LIBRARY(COCOA_LIBRARY Cocoa)
ENDIF (APPLE)

################################################
# INCLUDES
	
include_directories (BEFORE ${CMAKE_CURRENT_SOURCE_DIR}/jmde)
include_directories (BEFORE ${CMAKE_CURRENT_SOURCE_DIR}/WDL)
include_directories (BEFORE ${CMAKE_CURRENT_SOURCE_DIR}/reaper_libsndfilewrapper)

################################################
# additional cmake find packages
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
find_package(libsndfile REQUIRED)

include_directories (BEFORE ${LIBSNDFILE_INCLUDE_DIR})

set (reaper_libsndfilewrapper_sources
	${CMAKE_CURRENT_SOURCE_DIR}/reaper_libsndfilewrapper/libsndfileimport.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/reaper_libsndfilewrapper/main.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/reaper_libsndfilewrapper/wrapperclass.cpp	
	)

		
add_library (reaper_libsndfilewrapper SHARED
	${reaper_libsndfilewrapper_sources}
	)

SET_TARGET_PROPERTIES(reaper_libsndfilewrapper PROPERTIES PREFIX "")

# linking not necessary - library loaded at runtime	
# target_link_libraries (reaper_libsndfilewrapper libsndfile)

IF(APPLE)
	target_link_libraries (reaper_libsndfilewrapper ${CARBON_LIBRARY} ${COCOA_LIBRARY})
ENDIF(APPLE)
