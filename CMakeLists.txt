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
	
	SET(CMAKE_MACOSX_RPATH 1)
ENDIF (APPLE)

# THESE ARE THE MAIN SOURCE FILES
set (reaper_libsndfilewrapper_sources
	${CMAKE_CURRENT_SOURCE_DIR}/reaper_libsndfilewrapper/libsndfileimport.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/reaper_libsndfilewrapper/main.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/reaper_libsndfilewrapper/wrapperclass.cpp	
	${CMAKE_CURRENT_SOURCE_DIR}/reaper_libsndfilewrapper/pcmsink_libsndfile.cpp
	)


IF(WIN32)
	
	set (win_resource_files
		${CMAKE_CURRENT_SOURCE_DIR}/reaper_libsndfilewrapper/res.rc
		)
	
	set(all_source_files ${reaper_libsndfilewrapper_sources} ${win_resource_files})
		
ENDIF(WIN32)

IF(NOT WIN32)
	
#	FILE ( GLOB swell_sources ${CMAKE_CURRENT_SOURCE_DIR}/WDL/swell/*.mm )
	
#	FILE ( GLOB swell_sources2 ${CMAKE_CURRENT_SOURCE_DIR}/WDL/swell/*.cpp )
	
#	list(REMOVE_ITEM swell_sources2 "${CMAKE_CURRENT_SOURCE_DIR}/WDL/swell/swell-dlg-generic.cpp")
#	list(REMOVE_ITEM swell_sources2 "${CMAKE_CURRENT_SOURCE_DIR}/WDL/swell/swell-gdi-generic.cpp")
	
	# this filelist is from the swell example: (app_main.cpp)
	set (swell_sources
		${CMAKE_CURRENT_SOURCE_DIR}/WDL/swell/swell-dlg.mm
		${CMAKE_CURRENT_SOURCE_DIR}/WDL/swell/swell-gdi.mm
		${CMAKE_CURRENT_SOURCE_DIR}/WDL/swell/swell-ini.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/WDL/swell/swell-kb.mm 
		${CMAKE_CURRENT_SOURCE_DIR}/WDL/swell/swell-menu.mm 
		${CMAKE_CURRENT_SOURCE_DIR}/WDL/swell/swell-misc.mm 
		${CMAKE_CURRENT_SOURCE_DIR}/WDL/swell/swell-miscdlg.mm 
		${CMAKE_CURRENT_SOURCE_DIR}/WDL/swell/swell-wnd.mm 
		${CMAKE_CURRENT_SOURCE_DIR}/WDL/swell/swell.cpp 
		${CMAKE_CURRENT_SOURCE_DIR}/WDL/swell/swell-appstub.mm
		)
	
	set(all_source_files ${reaper_libsndfilewrapper_sources} ${swell_sources})
	
ENDIF(NOT WIN32)

# set unique name for swell functions
ADD_DEFINITIONS(-DSWELL_APP_PREFIX=SWELL_LIBSNDFILEWRAPPER)

################################################
# INCLUDE DIRECTORIES
	
include_directories (BEFORE ${CMAKE_CURRENT_SOURCE_DIR}/jmde)
include_directories (BEFORE ${CMAKE_CURRENT_SOURCE_DIR}/WDL)
include_directories (BEFORE ${CMAKE_CURRENT_SOURCE_DIR}/reaper_libsndfilewrapper)

################################################
# additional cmake find packages
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
find_package(libsndfile REQUIRED)

include_directories (BEFORE ${LIBSNDFILE_INCLUDE_DIR})



add_library (reaper_libsndfilewrapper SHARED
	${all_source_files}
	)

SET_TARGET_PROPERTIES(reaper_libsndfilewrapper PROPERTIES PREFIX "")

# linking not necessary - libsndfile library loaded at runtime!
# target_link_libraries (reaper_libsndfilewrapper libsndfile)

IF(APPLE)
	target_link_libraries (reaper_libsndfilewrapper ${CARBON_LIBRARY} ${COCOA_LIBRARY})
ENDIF(APPLE)


################################################
# change to static linking for ms visual c++
IF(WIN32)

set(CompilerFlags
        CMAKE_CXX_FLAGS
        CMAKE_CXX_FLAGS_DEBUG
        CMAKE_CXX_FLAGS_RELEASE
        CMAKE_C_FLAGS
        CMAKE_C_FLAGS_DEBUG
        CMAKE_C_FLAGS_RELEASE
        )
foreach(CompilerFlag ${CompilerFlags})
  string(REPLACE "/MD" "/MT" ${CompilerFlag} "${${CompilerFlag}}")
endforeach()

ENDIF(WIN32)


##############################################
## INSTALLERS


# DragNDrop for APPLE

IF(APPLE)
	
	SET (CPACK_DMG_VOLUME_NAME reaper_libsndfilewrapper)
	SET (CPACK_DMG_FORMAT UDRO)
	
	include(CPack)
	
ENDIF(APPLE)
