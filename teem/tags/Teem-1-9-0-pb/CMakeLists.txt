# This is the root Teem CMakeLists file.
CMAKE_MINIMUM_REQUIRED(VERSION 2.0)
PROJECT(TEEM C)

#-----------------------------------------------------------------------------
# TEEM version number.  Must reflect the values in teem/src/air.h
SET(TEEM_VERSION_MAJOR "1")
SET(TEEM_VERSION_MINOR "9")
SET(TEEM_VERSION_PATCH "0")

# Version string should not include patch level.  The major.minor is
# enough to distinguish available features of the toolkit.
SET(TEEM_VERSION_STRING "${TEEM_VERSION_MAJOR}.${TEEM_VERSION_MINOR}")

SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/CMake")

#-----------------------------------------------------------------------------
# Test for some required system information.
INCLUDE (CMakeBackwardCompatibilityC)

#-----------------------------------------------------------------------------
# Output directories.
SET (LIBRARY_OUTPUT_PATH ${TEEM_BINARY_DIR}/bin CACHE INTERNAL "Single output directory for building all libraries.")
SET (EXECUTABLE_OUTPUT_PATH ${TEEM_BINARY_DIR}/bin CACHE INTERNAL "Single output directory for building all executables.")
#output directory for installing all include files.")
MARK_AS_ADVANCED(LIBRARY_OUTPUT_PATH EXECUTABLE_OUTPUT_PATH)
SET(TEEM_LIBRARY_PATH "${LIBRARY_OUTPUT_PATH}")
SET(TEEM_EXECUTABLE_PATH "${EXECUTABLE_OUTPUT_PATH}")
SET(C_TEST_PATH ${EXECUTABLE_OUTPUT_PATH})


FILE(MAKE_DIRECTORY ${TEEM_BINARY_DIR}/include)
FILE(MAKE_DIRECTORY ${TEEM_BINARY_DIR}/include/teem)

#-----------------------------------------------------------------------------
# Find platform-specific differences in the handling of IEEE floating point 
# special values.

INCLUDE(${TEEM_SOURCE_DIR}/CMake/TestQnanhibit.cmake)
TEST_QNANHIBIT(QNANHIBIT_VALUE ${TEEM_SOURCE_DIR}/CMake)
IF(QNANHIBIT_VALUE)
  SET(QNANHIBIT 1 CACHE INTERNAL "The 22nd bit of 32-bit floating-point quiet NaN.")
ELSE(QNANHIBIT_VALUE)
  SET(QNANHIBIT 0 CACHE INTERNAL "The 22nd bit of 32-bit floating-point quiet NaN.")
ENDIF(QNANHIBIT_VALUE)


#-----------------------------------------------------------------------------
# TEEM build configuration options.
OPTION(BUILD_SHARED_LIBS "Build Teem with shared libraries." OFF)
SET(TEEM_BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS})
IF(NOT TEEM_BUILD_SHARED_LIBS)
  ADD_DEFINITIONS(-DTEEM_STATIC)
ENDIF(NOT TEEM_BUILD_SHARED_LIBS)

OPTION(TEEM_ZLIB "Build Teem with support for gzip compression." OFF)
OPTION(TEEM_PNG "Build Teem with support for PNG images." OFF)

# Can't build PNG without ZLIB, so force it on.
IF(TEEM_PNG AND NOT TEEM_ZLIB)
  SET(TEEM_ZLIB ON)
ENDIF(TEEM_PNG AND NOT TEEM_ZLIB)

SET(TEEM_ZLIB_LIB "")
SET(TEEM_PNG_LIB "")
IF(TEEM_ZLIB)
  # Find ZLIB
  FIND_PACKAGE(ZLIB)
  IF(ZLIB_FOUND)
    ADD_DEFINITIONS(-DTEEM_ZLIB=1)
    SET(TEEM_ZLIB_LIB ${ZLIB_LIBRARIES})
    SET(TEEM_ZLIB_IPATH ${ZLIB_INCLUDE_DIR})
  ELSE(ZLIB_FOUND)
    SET(TEEM_ZLIB OFF)
  ENDIF(ZLIB_FOUND)

  IF(TEEM_PNG)
    FIND_PACKAGE(PNG)
    IF(PNG_FOUND)
      ADD_DEFINITIONS(-DTEEM_PNG=1 ${PNG_DEFINITIONS})
      SET(TEEM_PNG_LIB ${PNG_LIBRARIES})
      SET(TEEM_PNG_IPATH ${PNG_INCLUDE_DIR})
    ELSE(PNG_FOUND)
      SET(TEEM_PNG OFF)
    ENDIF(PNG_FOUND)
  ENDIF(TEEM_PNG)
ENDIF(TEEM_ZLIB)

# Try and locate BZIP2 stuff
OPTION(TEEM_BZIP2 "Build Teem with support for bzip compression." OFF)
SET(TEEM_BZIP2_LIB "")

IF(TEEM_BZIP2)
  FIND_PACKAGE(BZ2)

  IF(BZ2_FOUND)
    ADD_DEFINITIONS(-DTEEM_BZIP2=1)
    SET(TEEM_BZIP2_LIB ${BZ2_LIBRARIES})
    SET(TEEM_BZIP2_IPATH ${BZ2_INCLUDE_DIR})
  ELSE(BZ2_FOUND)
    SET(TEEM_BZIP2 OFF )
  ENDIF(BZ2_FOUND)
ENDIF(TEEM_BZIP2)

#-----------------------------------------------------------------------------
#Include Teem Defines
# Set compiler flags for endian-ness.
IF(CMAKE_WORDS_BIGENDIAN)
  ADD_DEFINITIONS(-DTEEM_ENDIAN=4321)
ELSE(CMAKE_WORDS_BIGENDIAN)
  ADD_DEFINITIONS(-DTEEM_ENDIAN=1234)
ENDIF(CMAKE_WORDS_BIGENDIAN)

# Set compiler flags for 32 or 64 bit architecture (based on the size
# of a void pointer).
IF(CMAKE_SIZEOF_VOID_P MATCHES  8)
  ADD_DEFINITIONS(-DTEEM_32BIT=0)
ELSE(CMAKE_SIZEOF_VOID_P MATCHES 8)
  ADD_DEFINITIONS(-DTEEM_32BIT=1)
ENDIF(CMAKE_SIZEOF_VOID_P MATCHES 8)

#The QNANHIBIT variable is configured by the root level CMakeLists.txt
IF(QNANHIBIT)
  ADD_DEFINITIONS(-DTEEM_QNANHIBIT=1)
ELSE(QNANHIBIT)
  ADD_DEFINITIONS(-DTEEM_QNANHIBIT=0)
ENDIF(QNANHIBIT)

#DirectIO is the fast way to do multi-gigabyte I/O and currently only available
#for SGI platforms.  Use of DirectIO is enabled manually for now.
#OPTION(USE_DIRECTIO "Use DirectIO for Nrrd file IO.  Only valid on SGI systems." 0)
#MARK_AS_ADVANCED(USE_DIRECTIO)
#IF(USE_DIRECTIO)
#  ADD_DEFINITIONS(-DTEEM_DIO=1)
#ELSE(USE_DIRECTIO)
ADD_DEFINITIONS(-DTEEM_DIO=0)
#ENDIF(USE_DIRECTIO)

SET(TEEM_LIBRARIES air hest biff nrrd ell unrrdu dye moss gage bane
  limn hoover alan echo ten
  mite coil push)


FOREACH(header_file
    air/air.h
    hest/hest.h
    biff/biff.h
    nrrd/nrrd.h
    nrrd/nrrdDefines.h
    nrrd/nrrdMacros.h
    nrrd/nrrdEnums.h
    ell/ell.h
    ell/ellMacros.h
    unrrdu/unrrdu.h
    dye/dye.h
    alan/alan.h
    limn/limn.h
    gage/gage.h
    moss/moss.h
    echo/echo.h
    bane/bane.h
    ten/ten.h
    ten/tenMacros.h
    hoover/hoover.h
    mite/mite.h
    nrrd/nrrdDefines.h
    nrrd/nrrdMacros.h
    nrrd/nrrdEnums.h
    push/push.h
    )
  MESSAGE(STATUS "Copy header file: ${header_file}")
  GET_FILENAME_COMPONENT(file_no_path "${header_file}" NAME)
  CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/src/${header_file}"
    "${CMAKE_CURRENT_BINARY_DIR}/include/teem/${file_no_path}" COPYONLY IMMEDIATE)
ENDFOREACH(header_file)

#---
# Include directory 
INCLUDE_DIRECTORIES(
  "${TEEM_SOURCE_DIR}/include/"
  "${TEEM_BINARY_DIR}/include/"
  )

#---
# Install directory
SET (CMAKE_INSTALL_PREFIX "${TEEM_BINARY_DIR}")

#-----------------------------------------------------------------------------
# Dispatch the build into the proper subdirectories.
SUBDIRS(src)

#-----------------------------------------------------------------------------
# For testing
OPTION(BUILD_TESTING "Enable this to perform testing of TEEM" OFF)
IF(BUILD_TESTING)
  SET(DART_ROOT "" CACHE INTERNAL "" FORCE)
  INCLUDE(Dart)
  ENABLE_TESTING()
  MARK_AS_ADVANCED(TCL_TCLSH)
ENDIF(BUILD_TESTING)

#-----------------------------------------------------------------------------
# Help outside projects build TEEM projects.
INCLUDE(CMakeExportBuildSettings)
EXPORT_LIBRARY_DEPENDENCIES(${TEEM_BINARY_DIR}/TEEMLibraryDepends.cmake)
CMAKE_EXPORT_BUILD_SETTINGS(${TEEM_BINARY_DIR}/TEEMBuildSettings.cmake)

SET(CFLAGS "${CMAKE_C_FLAGS}")
SET(CC "${CMAKE_C_COMPILER}")

# For build tree usage
SET(TEEM_LIBRARY_DEPENDS_FILE ${TEEM_BINARY_DIR}/TEEMLibraryDepends.cmake)
SET(TEEM_EXECUTABLE_DIRS ${EXECUTABLE_OUTPUT_PATH})
SET(TEEM_LIBRARY_DIRS ${LIBRARY_OUTPUT_PATH})
SET(TEEM_USE_FILE ${TEEM_SOURCE_DIR}/CMake/TEEMUse.cmake)
SET(TEEM_INCLUDE_DIRS "${TEEM_BINARY_DIR}/include")
SET(TEEM_BUILD_SETTINGS_FILE ${TEEM_BINARY_DIR}/TEEMBuildSettings.cmake)
SET(prefix "${TEEM_BINARY_DIR}")
SET(exec_prefix "${prefix}")
SET(exec_prefix_set "no")
SET(bindir "${prefix}/bin")
SET(includedir "${prefix}")
SET(libdir "${prefix}/bin")
SET(srcdir "${TEEM_SOURCE_DIR}")

CONFIGURE_FILE("${TEEM_SOURCE_DIR}/CMake/TEEMConfig.cmake.in"
  "${TEEM_BINARY_DIR}/TEEMConfig.cmake" @ONLY IMMEDIATE)

# For installed tree usage
SET(TEEM_LIBRARY_DEPENDS_FILE ${CMAKE_INSTALL_PREFIX}/lib/TEEM-${TEEM_VERSION}/TEEMLibraryDepends.cmake)
SET(TEEM_EXECUTABLE_DIRS "${CMAKE_INSTALL_PREFIX}/bin")
SET(TEEM_LIBRARY_DIRS "${CMAKE_INSTALL_PREFIX}/lib")
SET(TEEM_USE_FILE "${CMAKE_INSTALL_PREFIX}/lib/TEEM-${TEEM_VERSION}/TEEMUse.cmake")
SET(TEEM_INCLUDE_DIRS "${CMAKE_INSTALL_PREFIX}/include")
SET(TEEM_BUILD_SETTINGS_FILE ${CMAKE_INSTALL_PREFIX}/lib/TEEM-${TEEM_VERSION}/TEEMBuildSettings.cmake)
SET(prefix "${CMAKE_INSTALL_PREFIX}")
SET(exec_prefix "${prefix}")
SET(exec_prefix_set "no")
SET(bindir "${prefix}/bin")
SET(includedir "${prefix}")
SET(libdir "${prefix}/lib")
SET(srcdir ".")

CONFIGURE_FILE("${TEEM_SOURCE_DIR}/CMake/TEEMConfig.cmake.in"
  "${TEEM_BINARY_DIR}/CMake/TEEMConfig.cmake" @ONLY IMMEDIATE)

INSTALL_FILES(/lib/TEEM-${TEEM_VERSION} FILES "${TEEM_SOURCE_DIR}/CMake/TEEMUse.cmake")
INSTALL_FILES(/lib/TEEM-${TEEM_VERSION} FILES "${TEEM_BINARY_DIR}/TEEMBuildSettings.cmake")
INSTALL_FILES(/lib/TEEM-${TEEM_VERSION} FILES "${TEEM_BINARY_DIR}/TEEMLibraryDepends.cmake")
INSTALL_FILES(/lib/TEEM-${TEEM_VERSION} FILES "${TEEM_BINARY_DIR}/CMake/TEEMConfig.cmake")

