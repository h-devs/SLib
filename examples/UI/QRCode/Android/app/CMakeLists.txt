cmake_minimum_required(VERSION 3.4.1)

include (${SLIB_PATH}/tool/slib-app.cmake)

file (GLOB SOURCE_FILES
 ${CMAKE_CURRENT_LIST_DIR}/src/main/cpp/*.cpp
 ${CMAKE_CURRENT_LIST_DIR}/../../src/*.cpp
)

link_directories("${SLIB_PATH}/external/lib/Android/${ANDROID_ABI}")

add_library(native-lib SHARED
 ${SOURCE_FILES}
)

target_link_libraries(native-lib slib zxing log GLESv2)
