include_directories(fsTerminal PUBLIC ../include/)

add_executable(fsTerminal main.cpp)

target_link_libraries(fsTerminal inodfs)