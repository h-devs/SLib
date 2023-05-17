cmake_minimum_required(VERSION 3.4.1)

include (${SLIB_PATH}/tool/slib-app.cmake)

file (GLOB SOURCE_FILES
 ${CMAKE_CURRENT_LIST_DIR}/src/main/cpp/*.cpp
 ${CMAKE_CURRENT_LIST_DIR}/../../src/*.cpp
 ${SLIB_PATH}/extra/qrcode/zxing.cpp
 ${SLIB_PATH}/extra/qrcode/zxing_scanner.cpp
 ${SLIB_PATH}/extra/qrcode/external/zxing/zxing_unity.cpp
)

include_directories("${SLIB_PATH}/extra/qrcode")

add_library(native-lib SHARED
 ${SOURCE_FILES}
)

target_link_libraries(native-lib slib log GLESv2)
