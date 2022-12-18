set (TARGET_NAME zxing)

project(${TARGET_NAME})
set (ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/../..")

include ("${ROOT_DIR}/../../build/CMake/external/configure.inc.cmake")

file (GLOB SRC_LIST
 "${ROOT_DIR}/src/*.cpp"
 "${ROOT_DIR}/src/aztec/*.cpp"
 "${ROOT_DIR}/src/datamatrix/*.cpp"
 "${ROOT_DIR}/src/maxicode/*.cpp"
 "${ROOT_DIR}/src/oned/*.cpp"
 "${ROOT_DIR}/src/oned/rss/*.cpp"
 "${ROOT_DIR}/src/pdf417/*.cpp"
 "${ROOT_DIR}/src/qrcode/*.cpp"
 "${ROOT_DIR}/src/textcodec/*.cpp"
)

include ("${ROOT_DIR}/../../build/CMake/external/common.inc.cmake")

target_include_directories (
 ${TARGET_NAME}
 PRIVATE "${ROOT_DIR}/src"
)
target_compile_options (
 ${TARGET_NAME}
 PRIVATE -Wno-all -g0
)
