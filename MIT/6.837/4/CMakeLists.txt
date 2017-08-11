cmake_minimum_required(VERSION 2.8)
cmake_policy(SET CMP0015 NEW)

project(a4)

# Silence warnings about deprecated GLUT functions
if(APPLE)
    add_definitions("-Wno-deprecated-declarations")
elseif (WIN32)
    # Silence Windows warnings about unsecure standard C99 functions
    add_definitions("-wd4996")
endif()

if(UNIX)
    # Enable C++11
    add_definitions("-std=c++11")
endif()

# vecmath include directory
include_directories(vecmath/include)

# Install directory
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    get_filename_component(ASSIGNMENT_ROOT "." REALPATH)
    set(CMAKE_INSTALL_PREFIX "${ASSIGNMENT_ROOT}/inst" CACHE PATH "install prefix" FORCE)
endif(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)

add_subdirectory(vecmath)
add_subdirectory(src)
