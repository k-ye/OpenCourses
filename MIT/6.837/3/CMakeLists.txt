cmake_minimum_required(VERSION 2.8)
cmake_policy(SET CMP0015 NEW)

project(a3)

# Silence warnings about deprecated GLUT functions
if(APPLE)
    add_definitions("-Wno-deprecated-declarations")
endif()

if(UNIX)
    # Enable C++11
    add_definitions("-std=c++11")
endif()

find_package(OpenGL REQUIRED)

link_directories(lib)

if (WIN32)
    include_directories(windows/freeglut/include)
    link_directories(windows/freeglut/lib)
    set(GLUT_LIBRARIES freeglut.lib)
    set(RK4_LIBRARIES RK4.lib)
else()
    find_package(GLUT REQUIRED)

    if (APPLE)
	set(RK4_LIBRARIES libRK4_mac.a)
    else()
	set(RK4_LIBRARIES libRK4.a)
    endif()
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
