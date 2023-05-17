cmake_minimum_required(VERSION 3.0)

project(Camera)

include ($ENV{SLIB_PATH}/tool/slib-app.cmake)

add_executable(Camera
  main.cpp
  ../src/app.cpp
)

set_target_properties(Camera PROPERTIES LINK_FLAGS "${SLIB_LINK_STATIC_FLAGS}")

target_link_libraries (
  Camera
  slib
  pthread
  dl
)
