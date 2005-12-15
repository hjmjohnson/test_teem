IF(NOT BUILD_SHARED_LIBS)
	ADD_DEFINITIONS(-DDEFT_STATIC)
ENDIF(NOT BUILD_SHARED_LIBS)

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
  ADD_DEFINITIONS(-DGLEW_STATIC)
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
IF(BUILD_SHARED_LIBS)
	INSTALL_TARGETS(/lib deftLib)
ENDIF(BUILD_SHARED_LIBS)

TARGET_LINK_LIBRARIES(deftLib
	ten limn gage ell nrrd biff hest air
	${FLTK_LIBRARIES}
	${OPENGL_gl_LIBRARY}
	${OPENGL_glu_LIBRARY}
	${DEFT_EXTRA_LIBRARIES}
	)

ADD_EXECUTABLE(Deft_dti main-dti.cxx)
TARGET_LINK_LIBRARIES(Deft_dti deftLib)
INSTALL_TARGETS(/bin Deft_dti)

ADD_EXECUTABLE(Deft_sqd main-sqd.cxx)
TARGET_LINK_LIBRARIES(Deft_sqd deftLib)
INSTALL_TARGETS(/bin Deft_sqd)
