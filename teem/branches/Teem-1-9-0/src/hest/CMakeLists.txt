ADD_LIBRARY(hest
  defaultsHest.c
  methodsHest.c
  parseHest.c
  usage.c
)

TARGET_LINK_LIBRARIES(hest air)

INSTALL_TARGETS(/lib hest)
