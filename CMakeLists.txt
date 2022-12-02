cmake_minimum_required(VERSION 3.10)
project(wotr)

add_library(wotr STATIC "wotr.cc")
add_library(wotr_shared SHARED "wotr.cc")

# Installation - dunno about this, but copied from titan CMakeLists.txt
include(GNUInstallDirs)
install(DIRECTORY include/wotr
  COMPONENT devel
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)
install(TARGETS wotr
  COMPONENT devel
  ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
  INCLUDES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)
