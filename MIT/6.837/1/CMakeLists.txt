cmake_minimum_required(VERSION 2.8)

project(a1)

# Silence warnings about deprecated GLUT functions
if(APPLE)
    add_definitions("-Wno-deprecated-declarations")
endif()

find_package(OpenGL REQUIRED)

# GLUT
if (WIN32)
    include_directories(windows/freeglut/include)
    if (CMAKE_SIZEOF_VOID_P EQUAL 4)
	# 32-bit target arch
	link_directories(windows/freeglut/lib)
    else()
	# 64-bit target arch
	link_directories(windows/freeglut/lib/x64)
    endif()
    set(GLUT_LIBRARIES freeglut)
else()
    find_package(GLUT REQUIRED)
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
