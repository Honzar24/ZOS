include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/include/)

set(SOURCES_LIST superBlock.cpp fileSystem.cpp)

add_library(${PROJECT_NAME} ${SOURCES_LIST})


