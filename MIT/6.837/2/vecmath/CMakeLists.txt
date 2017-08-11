set(LIB_NAME vecmath)

set(CPP_HEADER_DIR include)
set(CPP_SRC_DIR src)

set(CPP_HEADERS
    ${CPP_HEADER_DIR}/Matrix2f.h
    ${CPP_HEADER_DIR}/Matrix3f.h
    ${CPP_HEADER_DIR}/Matrix4f.h
    ${CPP_HEADER_DIR}/Quat4f.h
    ${CPP_HEADER_DIR}/Vector2f.h
    ${CPP_HEADER_DIR}/Vector3f.h
    ${CPP_HEADER_DIR}/Vector4f.h
    ${CPP_HEADER_DIR}/vecmath.h
    )

set(CPP_FILES
    ${CPP_SRC_DIR}/Matrix2f.cpp
    ${CPP_SRC_DIR}/Matrix3f.cpp
    ${CPP_SRC_DIR}/Matrix4f.cpp
    ${CPP_SRC_DIR}/Quat4f.cpp
    ${CPP_SRC_DIR}/Vector2f.cpp
    ${CPP_SRC_DIR}/Vector3f.cpp
    ${CPP_SRC_DIR}/Vector4f.cpp
    )

add_library(${LIB_NAME} STATIC ${CPP_FILES} ${CPP_HEADERS})
