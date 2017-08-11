set(BIN_NAME a1)

set(CPP_FILES
    camera.cpp
    curve.cpp
    main.cpp
    parse.cpp
    surf.cpp
    )

set(CPP_HEADERS
    camera.h
    curve.h
    extra.h
    parse.h
    surf.h
    tuple.h
    )

set(LIBS_USED
    vecmath
    )

add_executable(${BIN_NAME} ${CPP_FILES} ${CPP_HEADERS})
target_link_libraries(${BIN_NAME} ${LIBS_USED} ${OPENGL_LIBRARIES} ${GLUT_LIBRARIES})
install(TARGETS ${BIN_NAME} DESTINATION ${CMAKE_INSTALL_PREFIX})
