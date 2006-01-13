PROJECT(Deft)

#-----------------------------------------------------------------------------
# Output directories.
SET (LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin CACHE INTERNAL "Single output directory for building all libraries.")
SET (EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin CACHE INTERNAL "Single output directory for building all executables.")


#-----------------------------------------------------------------------------
# Deft requires OpenGL, TEEM, and FLTK2
FIND_PACKAGE(OpenGL REQUIRED)
FIND_PACKAGE(TEEM REQUIRED)
INCLUDE(${TEEM_USE_FILE})
FIND_PACKAGE(FLTK2 REQUIRED)
INCLUDE(${FLTK2_USE_FILE})

#-----------------------------------------------------------------------------
# GLEW stuff
FIND_PATH(GLEW_INCLUDE_PATH GL/glew.h)
FIND_LIBRARY(GLEW_LIBRARY glew)
FIND_PATH(GLEW_SOURCE_DIR src/glew.c)

SUBDIRS(src)
SUBDIRS(cmap)

INSTALL_FILES(/doc FILES ${CMAKE_CURRENT_SOURCE_DIR}/README.txt)
INSTALL_FILES(/share/Deft-0.1 FILES ${CMAKE_CURRENT_SOURCE_DIR}/README.txt)
SET(EXE_EXT "")
IF(WIN32)
  SET(EXE_EXT ".exe")
ENDIF(WIN32)
FOREACH(tool ${TEEM_TOOLS})
  MESSAGE(STATUS "Install ${TEEM_EXECUTABLE_DIRS}/${tool}${EXE_EXT}")
  INSTALL_FILES(/bin FILES "${TEEM_EXECUTABLE_DIRS}/${tool}${EXE_EXT}")
ENDFOREACH(tool)

# If the cmake version includes cpack, use it
IF(EXISTS "${CMAKE_ROOT}/Modules/CPack.cmake")
  SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Deft is a viewer for the DTI")
  SET(CPACK_PACKAGE_VENDOR "GordonInc")
  SET(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/README.txt")
  SET(CPACK_PACKAGE_EXECUTABLE "Deft_sqd")
  SET(CPACK_PACKAGE_EXECUTABLE_LABEL "Deft-SQD")
  #SET(CPACK_PACKAGE_VERSION_MAJOR "${CMake_VERSION_MAJOR}")
  #SET(CPACK_PACKAGE_VERSION_MINOR "${CMake_VERSION_MINOR}")
  #SET(CPACK_PACKAGE_VERSION_PATCH "${CMake_VERSION_PATCH}")
  INCLUDE(CPack)
ENDIF(EXISTS "${CMAKE_ROOT}/Modules/CPack.cmake")


