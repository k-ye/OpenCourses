cmake_minimum_required(VERSION 2.8)
cmake_policy(SET CMP0015 NEW)

project(a1)

# Silence warnings about deprecated GLUT functions
if(APPLE)
    add_definitions("-Wno-deprecated-declarations")
endif()

find_package(OpenGL REQUIRED)

if (WIN32)
    include_directories(windows/include)
    link_directories(windows/lib)
    set(GLUT_LIBRARIES freeglut)
    set(FLTK_LIBRARIES fltk fltkforms fltkgl fltkimages fltkjpeg fltkpng fltkzlib)
else()
    find_package(GLUT REQUIRED)
    find_package(FLTK REQUIRED)
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
