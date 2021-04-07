cmake_minimum_required(VERSION 3.4.1)

include (${SLIB_PATH}/tool/slib-app.cmake)

file (GLOB SOURCE_FILES
 ${CMAKE_CURRENT_LIST_DIR}/src/main/cpp/*.cpp

 ${CMAKE_CURRENT_LIST_DIR}/../../src/app.cpp
)

#link_directories("${CMAKE_CURRENT_LIST_DIR}/src/main/jniLibs/${ANDROID_ABI}")
link_directories("${SLIB_PATH}/external/lib/Android/${ANDROID_ABI}")
link_directories("${SLIB_PATH}/external/bin/FFmpeg/Android/${ANDROID_ABI}")

add_library(native-lib SHARED
 ${SOURCE_FILES}
)

#target_link_libraries(native-lib slib avcodec avformat avutil swresample GLESv2 OpenSLES log )
target_link_libraries(native-lib slib avformat avcodec avutil swresample mp3lame openh264 kvazaar opus vpx z GLESv2 OpenSLES log)
