cmake_minimum_required(VERSION 3.15)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)


project(
    inodefs
    VERSION 1.0 
    DESCRIPTION "I-node -ovy souborovy system. Vypracovano jako semestralni prace z predmrtu KIV/ZOS." 
    LANGUAGES CXX)

option(LOGFLAG "Logovani funkce fs(omezení vykonu)" OFF)

if(LOGFLAG)
    message("Logovani zapnuto")
else()
    message("Vypinam logovani")
    add_compile_definitions(NLOG)
endif()

if(CMAKE_BUILD_TYPE EQUAL "DEBUG")
    add_compile_options(-Wall -Wextra -pedantic -Werror)
endif(CMAKE_BUILD_TYPE EQUAL "DEBUG")

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    set(CMAKE_CXX_EXTENSIONS OFF)
    set_property(GLOBAL PROPERTY USE_FOLDERS ON)
endif()

add_subdirectory(src)
#aplikace terminalu pro overeni funkcionality
add_subdirectory(app)
