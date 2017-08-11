set(BIN_NAME a3)

set(CPP_FILES
    ClothSystem.cpp
    TimeStepper.cpp
    camera.cpp
    main.cpp
    particleSystem.cpp
    pendulumSystem.cpp
    simpleSystem.cpp
    Constraint.cpp
    )

set(CPP_HEADERS
    ClothSystem.h
    TimeStepper.h
    camera.h
    extra.h
    particleSystem.h
    pendulumSystem.h
    simpleSystem.h
    Constraint.h
    )

set(LIBS_USED
    vecmath
    ${OPENGL_LIBRARIES}
    ${GLUT_LIBRARIES}
    ${RK4_LIBRARIES}
    )

if (CMAKE_SYSTEM_NAME STREQUAL Linux)                        
    set(LIBS_USED ${LIBS_USED} dl)                           
endif()                                                      

add_executable(${BIN_NAME} ${CPP_FILES} ${CPP_HEADERS})
target_link_libraries(${BIN_NAME} ${LIBS_USED})
install(TARGETS ${BIN_NAME} DESTINATION ${CMAKE_INSTALL_PREFIX})
