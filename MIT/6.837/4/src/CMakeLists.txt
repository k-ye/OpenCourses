set(BIN_NAME a4)

set(CPP_FILES
    main.cpp
    stb.cpp
    Camera.cpp
    Image.cpp
    Mesh.cpp
    Renderer.cpp
    SceneParser.cpp
    Texture.cpp
    VecUtils.cpp
    )

set(CPP_HEADERS
    Camera.h
    Group.h
    Hit.h
    Image.h
    Light.h
    Material.h
    Mesh.h
    Object3D.h
    Plane.h
    Ray.h
    Renderer.h
    SceneParser.h
    Sphere.h
    Transform.h
    Triangle.h
    VecUtils.h
    )

set(LIBS_USED
    vecmath
    )

if (CMAKE_SYSTEM_NAME STREQUAL Linux)                        
    set(LIBS_USED ${LIBS_USED} dl)                           
endif()                                                      

add_executable(${BIN_NAME} ${CPP_FILES} ${CPP_HEADERS})
target_link_libraries(${BIN_NAME} ${LIBS_USED})
install(TARGETS ${BIN_NAME} DESTINATION ${CMAKE_INSTALL_PREFIX})
