cmake_minimum_required(VERSION 3.10)

# Read VERSION file at the repo root
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" REAPER_LIBSNDFILEWRAPPER_VERSION LIMIT_COUNT 1)
string(STRIP "${REAPER_LIBSNDFILEWRAPPER_VERSION}" REAPER_LIBSNDFILEWRAPPER_VERSION)

project (reaper_libsndfilewrapper VERSION ${REAPER_LIBSNDFILEWRAPPER_VERSION})

# Build mode:
#  - ON  (default): drop the built plugin straight into the user's REAPER
#                   plugin folder so a simple `make` reloads it in Reaper.
#  - OFF (CI / installer): leave it in the build tree so build_osx.sh /
#                          build_win.bat / build_linux.sh can stage + package it.
option(REAPER_LIBSNDFILEWRAPPER_INSTALL_USER_PLUGINS
       "Copy the built plugin into the user's REAPER UserPlugins folder"
       ON)

SET(CMAKE_BUILD_TYPE MinSizeRel)

# Statically link the MSVC runtime so the plugin does not depend on the VC++
# redistributable. CMP0091 (CMake >= 3.15) lets us select the runtime via an
# abstraction that also propagates into the vendored libsndfile sub-build.
if(POLICY CMP0091)
    cmake_policy(SET CMP0091 NEW)
endif()
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

IF(APPLE)
   # Universal binary for Apple Silicon + Intel REAPER.
   SET( CMAKE_OSX_ARCHITECTURES "arm64;x86_64" )
   SET( CMAKE_OSX_DEPLOYMENT_TARGET 10.13 )
   SET ( LINK_FLAGS MINSIZEREL "-dead_strip" )

   FIND_LIBRARY(CARBON_LIBRARY Carbon)
   FIND_LIBRARY(COCOA_LIBRARY Cocoa)

   SET(CMAKE_MACOSX_RPATH 1)
ENDIF (APPLE)

# set unique name for swell functions
ADD_DEFINITIONS(-DSWELL_APP_PREFIX=SWELL_LIBSNDFILEWRAPPER)

################################################
# INCLUDE DIRECTORIES
include_directories (BEFORE ${CMAKE_CURRENT_SOURCE_DIR}/WDL/WDL)
include_directories (BEFORE ${CMAKE_CURRENT_SOURCE_DIR}/reaper_libsndfilewrapper)

################################################
# vendored libsndfile (statically linked, core formats only - no external
# codec libraries). The plugin's headline formats (RF64, CAF, W64, PAF, AU,
# AIFF, ...) are all part of the core, so FLAC/Ogg/Vorbis/Opus/MPEG are off.
#
# libsndfile 1.2.2 declares cmake_minimum_required(VERSION 3.1..3.18); CMake
# >= 4.0 rejects the <3.5 minimum. This override lets the pinned release
# configure under modern CMake without patching the submodule.
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
# The static libsndfile.a is linked into our shared plugin, so it must be
# position-independent (required on Linux/ELF; a no-op on macOS/Windows).
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(ENABLE_EXTERNAL_LIBS      OFF CACHE BOOL "" FORCE)
set(ENABLE_MPEG               OFF CACHE BOOL "" FORCE)
set(BUILD_PROGRAMS            OFF CACHE BOOL "" FORCE)
set(BUILD_EXAMPLES            OFF CACHE BOOL "" FORCE)
set(BUILD_TESTING             OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS         OFF CACHE BOOL "" FORCE)
set(ENABLE_CPACK              OFF CACHE BOOL "" FORCE)
set(ENABLE_PACKAGE_CONFIG     OFF CACHE BOOL "" FORCE)
set(INSTALL_PKGCONFIG_MODULE  OFF CACHE BOOL "" FORCE)
# NOTE: do NOT also set libsndfile's legacy ENABLE_STATIC_RUNTIME here. The
# static MSVC runtime is already selected via CMAKE_MSVC_RUNTIME_LIBRARY +
# CMP0091 above, which propagates into the libsndfile sub-build. libsndfile
# 1.2.2 aborts configuration if both knobs are set ("Both ENABLE_STATIC_RUNTIME
# and CMAKE_MSVC_RUNTIME_LIBRARY are set.").
add_subdirectory(libsndfile)

################################################
# THESE ARE THE MAIN SOURCE FILES
set (reaper_libsndfilewrapper_sources
	${CMAKE_CURRENT_SOURCE_DIR}/reaper_libsndfilewrapper/libsndfileimport.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/reaper_libsndfilewrapper/main.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/reaper_libsndfilewrapper/wrapperclass.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/reaper_libsndfilewrapper/pcmsink_libsndfile.cpp
	)

IF(WIN32)
	# resource file for gui
	set (win_resource_files
		${CMAKE_CURRENT_SOURCE_DIR}/reaper_libsndfilewrapper/res.rc
		)
	set(all_source_files ${reaper_libsndfilewrapper_sources} ${win_resource_files})
ENDIF(WIN32)

IF(APPLE)
	# macOS: compile an own SWELL instance from the Cocoa (.mm) sources.
	# (this filelist is from the swell example: app_main.cpp)
	set (swell_sources
		${CMAKE_CURRENT_SOURCE_DIR}/WDL/WDL/swell/swell-dlg.mm
		${CMAKE_CURRENT_SOURCE_DIR}/WDL/WDL/swell/swell-gdi.mm
		${CMAKE_CURRENT_SOURCE_DIR}/WDL/WDL/swell/swell-ini.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/WDL/WDL/swell/swell-kb.mm
		${CMAKE_CURRENT_SOURCE_DIR}/WDL/WDL/swell/swell-menu.mm
		${CMAKE_CURRENT_SOURCE_DIR}/WDL/WDL/swell/swell-misc.mm
		${CMAKE_CURRENT_SOURCE_DIR}/WDL/WDL/swell/swell-miscdlg.mm
		${CMAKE_CURRENT_SOURCE_DIR}/WDL/WDL/swell/swell-wnd.mm
		${CMAKE_CURRENT_SOURCE_DIR}/WDL/WDL/swell/swell.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/WDL/WDL/swell/swell-appstub.mm
		)
	set(all_source_files ${reaper_libsndfilewrapper_sources} ${swell_sources})
ELSEIF(NOT WIN32)
	# Linux / other Unix: bind to REAPER's host SWELL via the modstub. REAPER
	# calls SWELL_dllMain() at load to populate the swell function pointers, so
	# we don't compile/link the full generic SWELL + LICE + GTK stack here.
	add_definitions(-DSWELL_PROVIDED_BY_APP)
	# swell-types.h otherwise defines min()/max() function-like macros that
	# clobber libstdc++'s <cmath> on Linux (Apple's libc++ tolerates it). The
	# plugin doesn't use the bare macros, so suppress them.
	add_definitions(-DWDL_NO_DEFINE_MINMAX)
	set(all_source_files ${reaper_libsndfilewrapper_sources}
		${CMAKE_CURRENT_SOURCE_DIR}/WDL/WDL/swell/swell-modstub-generic.cpp)
ENDIF()

######################################
# set the output directory - Reaper UserPlugin directory (dev convenience)
IF(APPLE AND REAPER_LIBSNDFILEWRAPPER_INSTALL_USER_PLUGINS)
  SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY "$ENV{HOME}/Library/Application Support/REAPER/UserPlugins")
ENDIF()

######################################
add_library (reaper_libsndfilewrapper SHARED
	${all_source_files}
	)

SET_TARGET_PROPERTIES(reaper_libsndfilewrapper PROPERTIES PREFIX "")

# vendored libsndfile (static) - provides sf_* and propagates sndfile.h include
target_link_libraries (reaper_libsndfilewrapper sndfile)

# Expose the project version to the C++ sources so the export/render config
# panel can display "reaper_libsndfilewrapper vX.Y.Z".
target_compile_definitions (reaper_libsndfilewrapper PRIVATE
	REAPER_LIBSNDFILEWRAPPER_VERSION="${REAPER_LIBSNDFILEWRAPPER_VERSION}")

IF(APPLE)
	target_link_libraries (reaper_libsndfilewrapper ${CARBON_LIBRARY} ${COCOA_LIBRARY})
ENDIF(APPLE)

if(MSVC)
	target_compile_definitions(reaper_libsndfilewrapper PRIVATE _CRT_SECURE_NO_WARNINGS)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
	# WDL/swell triggers -Wc++11-narrowing which clang treats as an error.
	target_compile_options(reaper_libsndfilewrapper PRIVATE -Wno-error=c++11-narrowing)
else()
	# GCC: same family of narrowing conversions in WDL; keep them non-fatal.
	target_compile_options(reaper_libsndfilewrapper PRIVATE -Wno-narrowing)
endif()
