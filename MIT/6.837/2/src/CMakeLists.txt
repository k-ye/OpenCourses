set(BIN_NAME a1)

set(CPP_FILES
    Joint.cpp
    MatrixStack.cpp
    Mesh.cpp
    ModelerView.cpp
    SkeletalModel.cpp
    bitmap.cpp
    camera.cpp
    main.cpp
    modelerapp.cpp
    modelerui.cpp
    )

set(CPP_HEADERS
    Joint.h
    MatrixStack.h
    Mesh.h
    ModelerView.h
    SkeletalModel.h
    bitmap.h
    camera.h
    extra.h
    modelerapp.h
    modelerui.h
    tuple.h
    )

set(LIBS_USED
    vecmath
    ${OPENGL_LIBRARIES}
    ${FLTK_LIBRARIES}
    ${GLUT_LIBRARIES}
    )

if (CMAKE_SYSTEM_NAME STREQUAL Linux)                        
    set(LIBS_USED ${LIBS_USED} dl)                           
endif()                                                      

include_directories(${FLTK_INCLUDE_DIR})

add_executable(${BIN_NAME} ${CPP_FILES} ${CPP_HEADERS})
target_link_libraries(${BIN_NAME} ${LIBS_USED})
install(TARGETS ${BIN_NAME} DESTINATION ${CMAKE_INSTALL_PREFIX})
