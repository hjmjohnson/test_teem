PROJECT(Deft)

FIND_PACKAGE(OpenGL REQUIRED)
FIND_PACKAGE(TEEM REQUIRED)
INCLUDE(${TEEM_USE_FILE})
FIND_PACKAGE(FLTK2 REQUIRED)
INCLUDE(${FLTK2_USE_FILE})

# GLEW stuff
FIND_PATH(GLEW_INCLUDE_PATH GL/glew.h)
FIND_LIBRARY(GLEW_LIBRARY glew)
FIND_PATH(GLEW_SOURCE_DIR src/glew.c)

ADD_DEFINITIONS(-DDEFT_STATIC)

SET(DEFT_SRCS
  Volume.cxx Object.cxx Group.cxx
  enums.cxx misc.cxx Viewer.cxx ViewerUI.cxx Scene.cxx
  Slider.cxx Actor.cxx Contour.cxx GlyphOld.cxx
  HyperStreamline.cxx HyperStreamlineUI.cxx
  TensorGlyph.cxx TensorGlyphUI.cxx
  Plane.cxx Gage.cxx Values.cxx Cmap.cxx
  PolyData.cxx PolyProbe.cxx
  TriPlane.cxx TriPlaneUI.cxx
  )

IF(GLEW_SOURCE_DIR)
  SET(DEFT_SRCS ${DEFT_SRCS} "${GLEW_SOURCE_DIR}/src/glew.c")
  INCLUDE_DIRECTORIES("${GLEW_SOURCE_DIR}/include")
ELSE(GLEW_SOURCE_DIR)
  INCLUDE_DIRECTORIES("${GLEW_INCLUDE_PATH}")
  SET(DEFT_EXTRA_LIBRARIES ${GLEW_LIBRARY})
ENDIF(GLEW_SOURCE_DIR)

INCLUDE_DIRECTORIES(
  ${FLTK_INCLUDE_DIR}
  ${TEEM_INCLUDE_DIR}
  ${OPENGL_INCLUDE_PATH}
  )

LINK_DIRECTORIES(
  ${TEEM_LIBRARY_DIR}
  )

ADD_LIBRARY(deftLib ${DEFT_SRCS})

TARGET_LINK_LIBRARIES(deftLib
  ten limn gage ell nrrd biff hest air
  ${FLTK_LIBRARIES}
  ${OPENGL_gl_LIBRARY}
  ${OPENGL_glu_LIBRARY}
  ${DEFT_EXTRA_LIBRARIES}
  )

ADD_EXECUTABLE(Deft-dti main-dti.cxx)
TARGET_LINK_LIBRARIES(Deft-dti deftLib)

ADD_EXECUTABLE(Deft-sqd main-sqd.cxx)
TARGET_LINK_LIBRARIES(Deft-sqd deftLib)
