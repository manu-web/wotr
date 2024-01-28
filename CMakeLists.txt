cmake_minimum_required(VERSION 3.10)
project(wotr)
set(CMAKE_CXX_STANDARD 17)

get_filename_component(PARENT_DIR ${PROJECT_SOURCE_DIR} DIRECTORY)
SET(ROCKSDB_DIR "${PARENT_DIR}/rocksdb")

include_directories("${ROCKSDB_DIR}/include")
include_directories("${PROJECT_SOURCE_DIR}")


add_library(wotr STATIC "wotr.cc")
add_library(wotr_shared SHARED "wotr.cc")

# Installation - dunno about this, but copied from titan CMakeLists.txt
# include(GNUInstallDirs)
# install(DIRECTORY include/wotr
#   COMPONENT devel
#   DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
# )
# install(TARGETS wotr
#   ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
#   INCLUDES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
# )
