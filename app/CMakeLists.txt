include_directories(fsTerminal PUBLIC ${PROJECT_SOURCE_DIR}/include/)

add_executable(fsTerminal main.cpp)

target_link_libraries(fsTerminal ${PROJECT_NAME})