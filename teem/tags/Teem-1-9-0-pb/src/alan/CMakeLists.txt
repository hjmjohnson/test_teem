ADD_LIBRARY(alan
coreAlan.c
enumsAlan.c
methodsAlan.c
)


TARGET_LINK_LIBRARIES(alan air biff ell nrrd)
INSTALL_TARGETS(/lib alan)
INSTALL_FILES(/include/teem ".h" alan.h)
