include_directories(${PROJECT_NAME} PUBLIC ../include/)

set(SOURCES_LIST superBlock.cpp fileSystem.cpp inode.cpp)

add_library(${PROJECT_NAME} ${SOURCES_LIST})


