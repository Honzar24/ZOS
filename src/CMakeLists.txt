include_directories(inodfs PUBLIC ../include/)

file(GLOB HEADER_LIST CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/include/*.hpp")
set(SOURCES_LIST superBlock.cpp fileSystem.cpp inode.cpp)


add_library(inodfs  ${HEADER_LIST} ${SOURCES_LIST})


