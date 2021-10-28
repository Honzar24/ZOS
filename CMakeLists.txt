cmake_minimum_required(VERSION 3.15)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

project(
    inodefs
    VERSION 1.0 
    DESCRIPTION "I-node -ový souborový systém. Vypracováno jako semestralní práce z předmětu KIV/ZOS." 
    LANGUAGES CXX)

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    set(CMAKE_CXX_EXTENSIONS OFF)
    set_property(GLOBAL PROPERTY USE_FOLDERS ON)
endif()

add_subdirectory(src)
#aplikace terminálu pro ověření funkcionality
add_subdirectory(app)
