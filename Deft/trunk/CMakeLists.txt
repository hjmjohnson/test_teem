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
